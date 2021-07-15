#include <stdio.h>
#include <string.h>
#include "ddr.h"
#include "ddr-gc.h"

ddr_cache *ddr_cache_create()
{
	ddr_cache *addr;
	
	addr = (ddr_cache *)dram_alloc(sizeof(*addr));
	if (!addr) {
		perror("ddr cache create failed\n");
		return NULL;
	}
	memset(addr->arr, 0, sizeof(ddr_bucket) * MAX_BUCKET);
	addr->q = (work_q *)dram_alloc(sizeof(work_q));
	addr->q->tail = addr->q->head = -1;
	for (int i = 0; i < MAX_BUCKET; ++i) {
		pthread_spin_init(&addr->arr[i].bucket_lock, 0);
	}
	return addr;
}

int ddr_cache_destroy(ddr_cache *addr)
{
	dram_free(addr->q);
	dram_free(addr);
	return 0;
}

int return_ddr_size()
{
	return MAX_BUCKET;
}

inline uint64_t get_hash_key(uint64_t key)
{	

	key += ~(key << 32);
	key ^= (key >> 22);
	key += ~(key << 13);
	key ^= (key >> 8);
	key += (key << 3);
	key ^= (key >> 15);
	key += ~(key << 27);
	key ^= (key >> 31);
	if (key > MAX_BUCKET) 
		key %= MAX_BUCKET;
	return key;
}

ddr_entry *ddr_cache_read(ddr_cache *d, uint64_t target_key)
{
	uint64_t bucket = get_hash_key(target_key);
	ddr_entry *iterator = d->arr[bucket].head;
	
	while (iterator) {
		if (iterator->key == target_key)
				return iterator;
		iterator = iterator->next;
	}
	return NULL;
}

unsigned long ddr_cache_write(ddr_cache *d, uint64_t key, 
		char *val, olog *o, ds_insert *_insert)
{
	uint64_t bucket;
	ddr_entry *entry, *curr_head;
	ddr_entry **bucket_head;
	int ret;
	volatile unsigned long clk;

	entry = dram_alloc(sizeof(*entry));
	if (!entry) {
		perror("ddr entry allocation failed\n");
		return 1;
	}
	/* prepare ddr entry*/
	entry->key = key;
	entry->val = val;
	entry->is_delete = F_WRITE;
	entry->status = F_NOT_REPLIED;
	bucket = get_hash_key(key);
	/* acquire the bucket lock*/
	pthread_spin_lock(&d->arr[bucket].bucket_lock);

	/* prepare to insert*/
	bucket_head = &(d->arr[bucket].head);
	curr_head = smp_atomic_load(bucket_head);
	entry->next = curr_head;

	/* insert the operation in olog before adding the entry
	 * to the dram cache bucket*/
	clk = get_clk();
	ret = oplog_insert_write(o, key, val, 
			clk, _insert);
	if (ret) {
		perror("oplog enq failed: NVMM unavailable\n");
		return 1;
	}
	/* add the entry to the bucket*/
	while(!smp_cas(bucket_head, curr_head, entry)){
		bucket_head = &(d->arr[bucket].head);
		curr_head = smp_atomic_load(bucket_head);
		entry->next = curr_head;
	}
	pthread_spin_unlock(&d->arr[bucket].bucket_lock);
	smp_faa(&d->arr[bucket].chain_length, 1);
	return 0;
}

unsigned long ddr_cache_delete(ddr_cache *d, 
		uint64_t target_key)
{

	uint64_t bucket;
	ddr_entry *entry;
	ddr_entry **bucket_head;
	ddr_entry *curr_head;
	int ret;
	volatile unsigned long clk;

	entry = dram_alloc(sizeof(*entry));
	if (!entry) {
		perror("ddr entry allocation failed\n");
		return 0;
	}
	entry->key = target_key;
	entry->val = NULL;
	entry->is_delete = F_DEL;
	entry->status = F_NOT_REPLIED;
	bucket = get_hash_key(target_key);
	pthread_spin_lock(&d->arr[bucket].bucket_lock);

retry_cas:
	bucket_head = &(d->arr[bucket].head);
	curr_head = smp_atomic_load(bucket_head);
	entry->next = curr_head;
	ret = smp_cas(bucket_head, curr_head, entry);
	if (!ret) {
		perror("ddr CAS failed");
		goto retry_cas;
	}
	clk = get_clk();
	pthread_spin_unlock(&d->arr[bucket].bucket_lock);
	smp_faa(&d->arr[bucket].chain_length, 1);
	return clk;
}

int validate_kv_pair(ddr_entry *entry, uint64_t t_key, 
		char *t_val)
{
	uint64_t key = entry->key;
	char *val = (char *)entry->val;

	if (key == t_key) {
		if (!t_val)
			return 0;
		return strcmp(val, t_val);
	}
	return 1;
}

void check_need_gc(ddr_cache *d, uint64_t bucket)
{
	unsigned int chain_length;
	volatile bool *is_gc_requested;
	volatile int tail;

	chain_length = d->arr[bucket].chain_length;
	is_gc_requested = &d->arr[bucket].is_gc_requested;
	if (chain_length > MAX_CHAIN_LENGTH && 
			!(*is_gc_requested)) {
		smp_atomic_store(is_gc_requested, 1);
		tail = d->q->tail;
		tail = update_index_safe(d->q->tail);
		d->q->arr[tail] = bucket;
		smp_atomic_store(&(d->q->tail), tail);
	}
	return;
}

void update_ddr_entry_status(ddr_cache *d, uint64_t target_key, 
		char *target_val) 
{
	uint64_t bucket;
	ddr_entry *iterator;
	int ret;

	bucket = get_hash_key(target_key);
	iterator = d->arr[bucket].head;
	while (iterator) {
		ret = validate_kv_pair(iterator, target_key, target_val);
		if (!ret) {
			smp_atomic_store(&(iterator->status), F_REPLIED);
//			smp_faa(&d->arr[bucket].n_applied_entries, 1);
			break;
		}
		iterator = iterator->next;
	}
	check_need_gc(d, bucket);
	return;
}

