#include <assert.h>
#include "list.h"

#ifdef TEST_LIST
node *dram_init_list()
{
	node *head;

	head = malloc(sizeof(*head)); 
	head->key = -1;
	head->val = NULL; 
	head->next = NULL;
	return head;
}

int dram_list_insert(void *p_head, uint64_t key, 
		char *val)
{
	node *head, *current, *new_node;
	node *prev = NULL;

	head = (node *)p_head;
	assert(head->key == -1);
	prev = head;
	current = head->next;
	while (current) {
		if (key <= current->key) 
			break;
		prev = current;
		current = current->next;
	}
	/* key already exists
	 * just update the val*/
	if (current && key == current->key) {
		smp_cas(&current->val, current->val, 
				val); 
		return 1;
	}
	new_node = malloc(sizeof(*new_node));
	new_node->key = key;
	new_node->val = val;
	new_node->next = current;
	smp_cas(&prev->next, prev->next, new_node);
	return 0;
}

char *dram_list_read(void *p_head, uint64_t target_key)
{
	node *head, *current;

	head = (node *)p_head;
	assert(head->key == -1);
	if ((!head->next)) {
		return NULL;
	}
	current = head->next;
	while (current) {
		if (target_key == current->key)
			return current->val;
		current = current->next;
	}
	return NULL;
}

int dram_list_delete(void *p_head, uint64_t target_key)
{
	node *head, *current, *prev;

	head = (node *)p_head;
	assert(head->key == -1);
	if ((!head->next)) {
		return 1;
	}
	prev = head;
	current = head->next;
	if (!current)
		return 1;
	while (current) {
		if (current->key >= target_key) {
			break;
		}
		prev = current;
		current = current->next;
	}
	if (!current || current->key > target_key)
		return 1;
	smp_cas(&prev->next, prev->next, current->next);
	free(current);
	return 0;
}
#endif

#ifndef TEST_LIST
undolog *g_ulog;

void init_undolog()
{
	g_ulog = nvm_alloc(sizeof(*g_ulog));
	g_ulog->key = 0;
	g_ulog->val = NULL;
	g_ulog->status = FREE;
	flush_to_nvm(g_ulog, sizeof(*g_ulog));
	smp_wmb();
	return;
}

node *init_list()
{
	node *head;

	head = tips_alloc(sizeof(*head)); 
	head->key = 0;
	head->val = NULL; 
	head->next = NULL;
	head->clk = get_clk();
	flush_to_nvm(head, sizeof(*head));
	smp_wmb();
	return head;
}

int list_insert(void *p_head, uint64_t p_key, 
		char *p_val)
{
	node *head, *current, *new_node;
	node *prev = NULL;
	int ret = -2;
	char *val;
	uint64_t key = p_key;
	int val_len;

	head = (node *)p_head;
	assert(!head->key);
	val_len = strlen(p_val) + 1;
	val = nvm_alloc(val_len);
	memcpy(val, p_val, val_len);
	flush_to_nvm(val, val_len);
	smp_mb();
	prev = head;
	current = head->next;
	while (current) {
		if (key <= current->key)
			break;
		prev = current;
		current = current->next;
	}
	/* key already exists
	 * just update the val*/
	if (ret == 0) {

		assert(g_ulog->status == FREE);
		g_ulog->key = current->key;
		g_ulog->val = current->val;
		g_ulog->status = IN_USE;
		flush_to_nvm(g_ulog, sizeof(*g_ulog));
		smp_mb();
		smp_cas(&current->val, current->val, 
				val); 
		flush_to_nvm(&current->val, sizeof(void *));
		smp_mb();

		assert(g_ulog->status == IN_USE);
		g_ulog->status = FREE;
		flush_to_nvm(g_ulog, sizeof(*g_ulog));
		smp_mb();
		return 1;
	}
	new_node = nvm_alloc(sizeof(*new_node));
	new_node->key = key;
	new_node->val = val;
	new_node->next = current;
	flush_to_nvm(new_node, sizeof(*new_node));
	smp_mb();

	assert(g_ulog->status == FREE);
	g_ulog->key = prev->key;
	g_ulog->next = prev->next;
	g_ulog->status = IN_USE;
	flush_to_nvm(g_ulog, sizeof(*g_ulog));
	smp_mb();

	smp_cas(&prev->next, prev->next, new_node);
	flush_to_nvm(&prev->next, sizeof(void *));
	smp_mb();

	assert(g_ulog->status == IN_USE);
	g_ulog->status = FREE;
	flush_to_nvm(g_ulog, sizeof(*g_ulog));
	smp_mb();
	return 0;
}

int tips_list_insert(void *p_head, uint64_t key, 
		char *val)
{
	node *head, *current, *new_node;
	node *prev = NULL;

	head = (node *)p_head;
	assert(!head->key);
	prev = head;
	current = head->next;
	while (current) {
		if (key <= current->key)
			break;
		prev = current;
		current = current->next;
	}
	/* key already exists
	 * just update the val*/
	if (current && current->key == key) {
		ulog_add(current, sizeof(node), 
				current->clk, NULL);
		current->val = val;
		return 1;
	}
	new_node = tips_alloc(sizeof(*new_node));
	new_node->key = key;
	new_node->val = val;
	new_node->next = current;
	new_node->clk = tips_get_clk();
	ulog_add(prev, sizeof(node), prev->clk, NULL);
	prev->next = new_node;

/*	smp_cas(&prev->next, prev->next, new_node);*/

	return 0;
}

char *list_read(void *p_head, uint64_t target_key)
{
	node *head, *current;

	head = (node *)p_head;
	assert(!head->key);
	if ((!head->next)) {
		return NULL;
	}
	current = head->next;
	while (current) {
		if (target_key == current->key)
			return current->val;
		current = current->next;
	}
	return NULL;
}

char *lf_list_read_dl(void *p_head, uint64_t target_key)
{
	node *head, *current;

	head = (node *)p_head;
	assert(!head->key);
	if ((!head->next)) {
		return NULL;
	}
	flush_to_nvm(head, sizeof(*head));
	smp_wmb();
	flush_to_nvm(head->next, sizeof(*head));
	smp_wmb();
	current = head->next;
	while (current) {
		if (target_key == current->key)
			return current->val;
		flush_to_nvm(current->next, sizeof(*current));
		smp_wmb();
		current = current->next;
	}
	return NULL;
}

char *lf_list_read_bdl(void *p_head, uint64_t target_key)
{
	node *head, *current;

	head = (node *)p_head;
	assert(!head->key);
	if ((!head->next)) {
		return NULL;
	}
	current = head->next;
	while (current) {
		if (target_key == current->key)
			return current->val;
		current = current->next;
	}
	return NULL;
}

char *tips_list_read(void *p_head, uint64_t target_key)
{
	node *head, *current;

	head = (node *)p_head;
	assert(!head->key);
	if ((!head->next)) {
		return NULL;
	}
	current = head->next;
	while (current) {
		if (target_key == current->key)
			return current->val;
		current = current->next;
	}
	return NULL;
}

int list_delete(void *p_head, uint64_t target_key)
{
	node *head, *current, *prev;

	head = (node *)p_head;
	assert(!head->key);
	if ((!head->next)) {
		return 1;
	}
	prev = head;
	current = head->next;
	if (!current)
		return 1;
	while (current) {
		if (target_key == current->key) {

			assert(g_ulog->status == FREE);
			g_ulog->key = prev->key;
			g_ulog->next = prev->next;
			g_ulog->status = IN_USE;
			flush_to_nvm(g_ulog, sizeof(*g_ulog));
			smp_mb();
			smp_cas(&prev->next, prev->next, 
					current->next);
			/* TODO:free current need to check for reader?
			 * no, because this ops will be in ddr*/
			flush_to_nvm(&prev->next, sizeof(void *));
			smp_mb();
			nvm_free(current);
			assert(g_ulog->status == IN_USE);
			g_ulog->status = FREE;
			flush_to_nvm(g_ulog, sizeof(*g_ulog));
			smp_mb();
			
			return 0;
		}
		prev = current;
		current = current->next;
	}
	/* check for early break since sorted list*/
	return 1;
}

int tips_list_delete(void *p_head, uint64_t target_key)
{
	node *head, *current, *prev;

	head = (node *)p_head;
	assert(head->key == 0);
	if ((!head->next)) {
		return 1;
	}
	prev = head;
	current = head->next;
	while (current) {
		if (current->key >= target_key) 
			break;
		prev = current;
		current = current->next;
	}
	if (!current || current->key > target_key)
		return 1;
	ulog_add(prev, sizeof(node), prev->clk, current);
	prev->next = current->next;
//	smp_cas(&prev->next, prev->next, current->next);
	return 0;
}

int lf_list_insert(void *p_head, uint64_t key, 
		char *p_val)
{
	node *head, *current, *new_node;
	node *prev = NULL;
	int val_len;
	char *val;

	head = (node *)p_head;
	assert(!head->key);
	val_len = strlen(p_val) + 1;
	val = nvm_alloc(val_len);
	memcpy(val, p_val, val_len);
	flush_to_nvm(val, val_len);
	smp_mb();
	flush_to_nvm(head, sizeof(*head));
	smp_mb();
	prev = head;
	flush_to_nvm(head->next, sizeof(*head));
	smp_mb();
	current = head->next;

	while (current) {
		if (key <= current->key)
			break;
		prev = current;
		flush_to_nvm(current->next, sizeof(*current));
		smp_mb();
		current = current->next;
	}
	/* key already exists
	 * just update the val*/
	if (current && current->key == key) {
		current->val = val;
		flush_to_nvm(current->next, sizeof(*current));
		smp_mb();
		return 1;
	}
	new_node = nvm_alloc(sizeof(*new_node));
	new_node->key = key;
	new_node->val = val;
	new_node->next = current;
	flush_to_nvm(new_node, sizeof(*new_node));
	smp_mb();
	smp_cas(&prev->next, prev->next, new_node);
	flush_to_nvm(prev, sizeof(*prev));
	smp_mb();
	return 0;
}

node *alloc_node(uint64_t key, char *val) 
{
	node *new_node;

	new_node = tips_alloc(sizeof(*new_node));
	new_node->key = key;
	new_node->val = val;
	new_node->clk = tips_get_clk();
	return new_node;
}

int tips_lf_list_insert(void *p_head, uint64_t key, 
		char *val)
{
	node *head, *current, *new_node = NULL, *ret_node;
	node *prev = NULL;
	int ret;

	head = (node *)p_head;
	assert(!head->key);
cas_retry:
	prev = head;
	current = head->next;
	while (current) {
		if (key <= current->key)
			break;
		prev = current;
		current = current->next;
	}
	/* key already exists
	 * just update the val*/
	if (current && current->key == key) {
		ulog_add(current, sizeof(node), 
				current->clk, NULL);
		current->val = val;
		return 1;
	}
	if (!new_node) {
		new_node = tips_alloc(sizeof(*new_node));
		new_node->key = key;
		new_node->val = val;
		new_node->clk = tips_get_clk();
	}
cas_retry2:
	ulog_add(prev, sizeof(node), prev->clk, NULL);
cas_retry1:
	new_node->next = current;
	ret = smp_cas_v(&prev->next, current, 
			new_node, ret_node);
	if (!ret) {
		current = ret_node;
		if (key < current->key)
			goto cas_retry1;
		else {
			prev = ret_node;
			current = ret_node->next;
			goto cas_retry2;
		}
	}
	return 0;
}

int tips_lf_list_delete(void *p_head, uint64_t target_key)
{
	node *head, *current, *prev;

	head = (node *)p_head;
	assert(head->key == 0);
	if ((!head->next)) {
		return 1;
	}
	prev = head;
	current = head->next;
	while (current) {
		if (current->key >= target_key) 
			break;
		prev = current;
		current = current->next;
	}
	if (!current || current->key > target_key)
		return 1;
	ulog_add(prev, sizeof(node), prev->clk, current);
	smp_cas(&prev->next, prev->next, current->next);
	return 0;
}


#endif

/***************************************************************************
 **********************test list functions**********************************
 ***************************************************************************/
/*int main()
{
	node *head, *current;
	void *p_head;
	char *ret;
	int ret_val;

	uint64_t key0 = 4;
	char val0[5] = "val0";
	uint64_t key1 = 2;
	char val1[5] = "val1";
	uint64_t key2 = 1;
	char val2[5] = "val2";
	uint64_t key3 = 3;
	char val3[5] = "val3";
	uint64_t key4 = 5;
	char val4[5] = "val4";
	uint64_t key5 = 0;
	char val5[5] = "val5";



	head = dram_init_list();
	p_head = (void *)head;

	dram_list_insert(p_head, key0, val0);
	ret = dram_list_read(p_head, key0);
	printf("val0= %s\n", ret);

	dram_list_insert(p_head, key1, val1);
	ret = dram_list_read(p_head, key1);
	printf("val1= %s\n", ret);

	dram_list_insert(p_head, key2, val2);
	ret = dram_list_read(p_head, key2);
	printf("val2= %s\n", ret);

	dram_list_insert(p_head, key3, val3);
	ret = dram_list_read(p_head, key3);
	printf("val3= %s\n", ret);

	dram_list_insert(p_head, key4, val4);
	ret = dram_list_read(p_head, key4);
	printf("val4= %s\n", ret);

	dram_list_insert(p_head, key5, val5);
	ret = dram_list_read(p_head, key5);
	printf("val5= %s\n", ret);
	
	current = head->next;
	printf("head= %p\n", head);
	while (current) {
		printf("current= %p\n", current);
		printf("current->key= %ld\n", current->key);
		printf("current->val= %s\n", current->val);
		current = current->next;
	}
	printf("=================================\n");
	dram_list_delete(p_head, key1);
	printf("deleting %ld\n", key1);
	current = head->next;
	while (current) {
		printf("current= %p\n", current);
		printf("current->key= %ld\n", current->key);
		printf("current->val= %s\n", current->val);
		current = current->next;
	}
	dram_list_delete(p_head, key0);
	printf("deleting %ld\n", key0);
	current = head->next;
	while (current) {
		printf("current= %p\n", current);
		printf("current->key= %ld\n", current->key);
		printf("current->val= %s\n", current->val);
		current = current->next;
	}
	printf("try deleting non-existing key\n");
	ret_val = dram_list_delete(p_head, key0);
	printf("ret_val= %d\n", ret_val);

	printf("try reading non-existing key\n");
	ret = dram_list_read(p_head, key0);
	if (!ret)
		ret = "noke";
	printf("ret_val= %s\n", ret);
	return 0;
}*/
