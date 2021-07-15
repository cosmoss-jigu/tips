#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "oplog.h"


olog *oplog_create()
{
	olog *addr;
	size_t size;
	
	addr = (olog *)nvm_alloc(sizeof(*addr));
	if (!addr) {
		perror("oplog create failed\n");
		return NULL;
	}
	addr->tail_index = addr->reclaim_index = 
		addr->prev_tail = -1;
	addr->head_index = 0;
	addr->n_used = addr->n_applied = 0;
	addr->need_reclaim = false;
	addr->next = NULL;
	flush_to_nvm(addr, sizeof(*addr));
	size = sizeof(op_entry) * MAX_OP_ENTRIES;
	memset(addr->arr, 0, size);
	flush_to_nvm(addr->arr, size);
	smp_mb();
	return addr;
}

int oplog_destroy(olog *addr) 
{	
	nvm_free((void *)addr);
	return 0;
}

int return_olog_size()
{
	return MAX_OP_ENTRIES;
}

int oplog_destroy_unused(int count, olog **head)
{
	olog *last, *current;
	int op_count = 0;
	
//	printf("destroying unsued oplogs...\n");
	last = NULL;
	current = *head;
	while (current && op_count != count) {
		last = current;
		++op_count;
		current = current->next;
	}
	last->next = NULL;
//	printf("skipped %d oplogs...\n", op_count);
	op_count = 0;
	while (current) {
		last = current;
		current = current->next;
		oplog_destroy(last);
		++op_count;
//		printf("freed %d oplogs...\n", op_count);
	}
	return 0;
}

void oplog_destroy_safe(olog **head)
{	
	olog *o_addr;

	while (*head) {
		o_addr = *head;
		*head = o_addr->next;
		oplog_destroy(o_addr);
	}
	*head = NULL;
	return;
}

void check_need_reclaim(olog *o) 
{	
	if (o->n_used >= MAX_USED && 
			o->need_reclaim == false) {
		smp_cas(&o->need_reclaim, false, true);
	}
	return;
}

void update_op_tail(olog *o, volatile int index)
{
	int ret;
	volatile int used;

	ret = smp_cas(&o->tail_index, o->tail_index, index);
	if (!ret) {
		perror("tail update failed\n");
		exit(1);
	}
	flush_to_nvm(( void *)&(o->tail_index), 
			sizeof(o->tail_index));
	smp_mb();
	used = o->n_used;
	while (!smp_cas(&o->n_used, used, used + 1)) {
		used = o->n_used;
	}
	return;
}

int incr_index_safe(int index) 
{
	if (unlikely(index + 1 >= MAX_OP_ENTRIES)) {
		index = 0;
		return index;
	}
	index += 1;
	return index;
}

int oplog_insert_write(olog *o, uint64_t key, char *val, 
		unsigned long clk, ds_insert *ins)
{
	int val_len = 0;
	volatile int tail = o->tail_index;
	volatile int head = o->head_index;

	tail = incr_index_safe(tail);
	while (tail == head) {
		if (unlikely(o->tail_index == -1))  {
			break;
		}
		head = o->head_index;
	//	printf("######oplog full########\n");
		continue;
	}
	/* write key*/
	o->arr[tail].key = key;

	/* allocate and write val*/
	val_len = strlen(val) + 1;
	o->arr[tail].val = nvm_alloc(val_len);
	if (!o->arr[tail].val) {
		perror("oplog:val allocation failed\n");
		return 1;
	}
	memcpy(o->arr[tail].val, val, val_len);

//	o->arr[tail].val = val;

	/* update other fields*/
	o->arr[tail].enq_ts = clk;
	o->arr[tail]._insert = ins;
	flush_to_nvm(o->arr[tail].val, val_len);
	flush_to_nvm(&o->arr[tail], sizeof(op_entry));
	smp_mb();

	/* update tail and persist*/
	update_op_tail(o, tail);	
	check_need_reclaim(o);
	return 0;
}

int oplog_insert_delete(olog *o, uint64_t key, int flag, 
		unsigned long clk, ds_delete *del)
{
	volatile int tail = o->tail_index;
	volatile int head = o->head_index;

	tail = incr_index_safe(tail);
	while (tail == head) {
		if (unlikely(o->tail_index == -1))  {
			break;
		}
		head = o->head_index;
		continue;
	}	
	/* allocate and write key*/
	o->arr[tail].key = key;
	o->arr[tail].val = NULL;

	/* update other fields*/
	smp_atomic_store(&o->arr[tail].enq_ts, clk);
	o->arr[tail].is_delete = flag;
	o->arr[tail]._delete = del;
	flush_to_nvm(&o->arr[tail], sizeof(op_entry));
	smp_wmb();

	/* update tail and persist*/
	update_op_tail(o, tail);	
	check_need_reclaim(o);
	return 0;
}

void reclaim_oplog(olog *o)
{
	volatile unsigned int used, curr_used;

	used = smp_atomic_load(&o->n_used);
	assert(o->n_used >= o->n_applied);
	curr_used = o->n_used - o->n_applied;
	while (!smp_cas(&o->n_used, used, curr_used)) {
		used = smp_atomic_load(&o->n_used);
		curr_used = o->n_used - o->n_applied;
	}
	smp_cas(&o->need_reclaim, true, false);
	o->n_applied = 0;
	smp_atomic_store(&o->head_index, o->prev_tail);
	flush_to_nvm((void *) &o->head_index, 
			sizeof(o->head_index));
	smp_mb();
	return;
}

