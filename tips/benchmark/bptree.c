#include "bptree.h"

pthread_rwlock_t bpt_lock = PTHREAD_RWLOCK_INITIALIZER;
int g_order = DEFAULT_TREE_ORDER;
b_node *queue;

void enqueue(b_node * new_node) 
{	
        b_node * c;

        if (queue == NULL) {
                queue = new_node;
                queue->next = NULL;
        }
        else {
                c = queue;
                while(c->next != NULL) {
                        c = c->next;
                }
                c->next = new_node;
                new_node->next = NULL;
        }
}

b_node * dequeue(void) 
{
        b_node * n = queue;

        queue = queue->next;
        n->next = NULL;
        return n;
}

int path_to_root(b_node * const root, b_node * child) 
{
        int length = 0;

        b_node * c = child;
        while (c != root) {
                c = c->parent;
                length++;
        }
        return length;
}

void print_tree(b_node * const root) 
{

        b_node * n = NULL;
        int i = 0;
        int rank = 0;
        int new_rank = 0;

        if (root == NULL) {
                printf("Empty tree.\n");
                return;
        }
        queue = NULL;
        enqueue(root);
        while(queue != NULL) {
                n = dequeue();
                if (n->parent != NULL && n == n->parent->ptrs[0]) {
                        new_rank = path_to_root(root, n);
                        if (new_rank != rank) {
                                rank = new_rank;
                                printf("\n");
                        }
                }
                for (i = 0; i < n->num_keys; i++) {
                        printf("%ld ", n->keys[i]);
                }
                if (!n->is_leaf)
                        for (i = 0; i <= n->num_keys; i++)
                                enqueue(n->ptrs[i]);
                printf("| ");
        }
        printf("\n");
}


b_root_obj *tips_init_bp_tree()
{	
	b_root_obj *root;

	root = tips_alloc(sizeof(*root));
	root->b_root = NULL;
	flush_to_nvm(root, sizeof(*root));
	smp_wmb();
	printf("b+tree of order %d\n", DEFAULT_TREE_ORDER);
	return root;
}

char *search_record(b_node *leaf, uint64_t target_key, bool flag) 
{
#ifndef B_SEARCH	
	int i;
	static int count;

	for (i = 0; i < leaf->num_keys; i++) {
		if (target_key == leaf->keys[i]) 
			return (char *)leaf->ptrs[i];
	}
	return NULL;
#else
	int start, mid, end;
	static int count;
	start = 0;
	end = leaf->num_keys - 1;
	while (start <= end) {
		mid = start + (end - start) / 2;
		if (leaf->keys[mid] == target_key)
			return (char *)leaf->ptrs[mid];
		if (leaf->keys[mid] < target_key)
			start = mid + 1;
		else
			end = mid - 1;
	}
	return NULL;
#endif
}

b_node *get_leaf(b_node *root, uint64_t target_key)
{
	int i;
	b_node *node = root;

	if (!root)
		return root;
	while (!node->is_leaf) {
		i = 0;
		while (i < node->num_keys) {
			if (target_key >= node->keys[i])
				++i;
			else 
				break;
		}
		node = (b_node *)node->ptrs[i];
	}
	return node;
}

char *tips_bp_tree_read(void *p_root, uint64_t key)
{	
	b_root_obj *_root;
	b_node *root, *leaf;
	char *val;
	
	pthread_rwlock_rdlock(&bpt_lock);
	_root = (b_root_obj *)p_root;
	root = _root->b_root;
	if (!root) {
		pthread_rwlock_unlock(&bpt_lock);
		return NULL;
	}
	leaf = get_leaf(root, key); 
	val = search_record(leaf, key, true);
	pthread_rwlock_unlock(&bpt_lock);
	return val;
/*	if (!val) 
		printf("key %ld not found\n", key);
	else 
		printf("Record at %p -- key %ld, val %s\n", 
				val, key, val);*/
}

int scan_range(b_node *root, uint64_t start_key, 
		int range, uint64_t *buff)
{
	int i = 0, count = 0;
	b_node *leaf;
	bool finish = false;

	leaf = get_leaf(root, start_key);
	if (!leaf)
		return 0;
	for (i = 0; i < leaf->num_keys; ++i) {
		if (leaf->keys[i] >= start_key)
			break;
	}
	if (i == leaf->num_keys)
		return 0;
	while (leaf) {
		for (; i < leaf->num_keys; ++i) {

			/*if (leaf->keys[i] <= end_key)
				++count;*/
			if (count <= range) {
				buff[i] = leaf->keys[i];
				++count;
			}
			else {
				finish = true;
				break;
			}
		}
		if (finish)
			break;
		leaf = leaf->ptrs[g_order - 1];
		i = 0;
	}
	return count;
}

int tips_bp_tree_scan(void *p_root, uint64_t start_key, int range, 
		uint64_t *buff)
{
	b_root_obj *_root;
	b_node *root;
	int key_count;

	pthread_rwlock_rdlock(&bpt_lock);
	_root = (b_root_obj *)p_root;
	root = _root->b_root;
	key_count = scan_range(root, start_key, 
			range, buff);
	pthread_rwlock_unlock(&bpt_lock);
	return key_count;
}

b_node *create_node()
{
	b_node *new_node;

	new_node = tips_alloc(sizeof(*new_node));
	if (!new_node) {
		perror("bpt node create failed");
		exit(0);
	}
	new_node->is_leaf = false;
	new_node->num_keys = 0;
	new_node->clk = tips_get_clk();
	new_node->parent = new_node->next = NULL;
	return new_node;
}

b_node *create_leaf()
{
	b_node *leaf;

	leaf = create_node();
	leaf->is_leaf = true;
	return leaf;
}

int cut(int len)
{
	if (len % 2 == 0)
		return len/2;
	else
		return len/2 + 1;
}

b_node *create_bp_tree(uint64_t key, char *rec)
{
	b_node *root;

	root = create_leaf();
	root->keys[0] = key;
	root->ptrs[0] = rec;
	root->ptrs[g_order - 1] = NULL;
	root->parent = NULL;
	++root->num_keys;
	return root;
}

b_node *insert_into_leaf(b_node *leaf, uint64_t key, 
		char *rec)
{
	int i; 
	int slot = 0;
	int n_keys = leaf->num_keys;
	
	while (slot < n_keys && leaf->keys[slot] < key) 
		++slot;
	for (i = n_keys; i > slot; --i) {
		leaf->keys[i] = leaf->keys[i - 1];
		leaf->ptrs[i] = leaf->ptrs[i - 1];
	}
	leaf->keys[slot] = key;
	leaf->ptrs[slot] = rec;
	++leaf->num_keys;
	return leaf;
}

b_node *insert_into_new_root(b_node *left, uint64_t key, 
		b_node *right)
{
	b_node *root;
	
	/* this new node will be in the allocator
	 * list and will be flushed at the 
	 * undolog commit*/
	root = create_node();
	root->keys[0] = key;
	root->ptrs[0] = left;
	root->ptrs[1] = right;
	++root->num_keys;
	root->parent = NULL;
	left->parent = root;
	right->parent = root;
	return root;
}

int get_left_index(b_node *parent, b_node *left)
{
	int left_index = 0;

	while(left_index <= parent->num_keys &&
			parent->ptrs[left_index] != left) {
		++left_index;
	}
	return left_index;
}

b_node *insert_into_node(b_node *root, b_node *n, 
		int left_index, uint64_t key, b_node *right)
{
	int i;

	for (i = n->num_keys; i > left_index; --i) {
		/* node n is already backedup in 
		 * the undolog or it is present in 
		 * the allocator list*/
		n->ptrs[i + 1] = n->ptrs[i];
		n->keys[i] = n->keys[i - 1];
	}
	n->ptrs[left_index + 1] = right;
	n->keys[left_index] = key;
	++n->num_keys;
	return root;
}

b_node *split_insert_node(b_node *root, b_node *old_node, 
		int left_index, uint64_t key, b_node *right)
{
	int i, j, split;
	uint64_t k_prime;
	b_node *new_node, *child;
	uint64_t *temp_keys;
	b_node **temp_ptrs;

	temp_keys = (uint64_t *)malloc(g_order * sizeof(uint64_t));
	if (!temp_keys) {
		perror("node split failed");
		exit(0);
	}
	temp_ptrs = (b_node **)malloc((g_order + 1) * sizeof(b_node *));
	if (!temp_ptrs) {
		perror("node split failed");
		exit(0);
	}
	for (i = 0, j = 0; i < old_node->num_keys + 1; ++i, ++j) {
		if (j == left_index + 1)
			++j;
		temp_ptrs[j] = old_node->ptrs[i];
	}
	for (i = 0, j = 0; i < old_node->num_keys; ++i, ++j) {
		if (j == left_index)
			++j;
		temp_keys[j] = old_node->keys[i];
	}
	temp_ptrs[left_index + 1] = right;
	temp_keys[left_index] = key;
	split = cut(g_order);
	/* new_node will be in the 
	 * allocator list*/
	new_node = create_node();
	old_node->num_keys = 0;
	for (i = 0; i < split - 1; ++i) {
		/* old_node is already backed up 
		 * in the undolog*/
		old_node->ptrs[i] = temp_ptrs[i];
		old_node->keys[i] = temp_keys[i];
		++old_node->num_keys;
	}
	old_node->ptrs[i] = temp_ptrs[i];
	k_prime = temp_keys[split - 1];
	for (++i, j = 0; i < g_order; ++i, ++j) {
		new_node->ptrs[j] = temp_ptrs[i];
		new_node->keys[j] = temp_keys[i];
		++new_node->num_keys;
	}
	new_node->ptrs[j] = temp_ptrs[i];
	free(temp_ptrs);
	free(temp_keys);
	new_node->parent = old_node->parent;
	for (i = 0; i <= new_node->num_keys; ++i) {
		child = new_node->ptrs[i];
		child->parent = new_node;
	}
	return insert_into_parent(root, old_node, 
			k_prime, new_node);
}

b_node *insert_into_parent(b_node *root, b_node *left, 
		uint64_t key, b_node *right)
{
	int left_index;
	b_node *parent;
	size_t key_arr_size;
	size_t val_arr_size;
	int ret;

	parent = left->parent;
	if (!parent)
		/* the new parent will be in the 
		 * allocator list*/
		return insert_into_new_root(left, key, right);

	/* the parent will be modified 
	 * back up the parent in the undolog*/
	ret = ulog_add(parent, sizeof(*parent), 
			parent->clk, NULL);
	if (ret) {
		/* if the parent is backed up in the
		 * undolog successfully then 
		 * backup the key_array and val_array 
		 * associated with this leaf*/
		key_arr_size = (g_order - 1) * sizeof(uint64_t);
		val_arr_size = g_order * sizeof(void *);
		ulog_add(parent->keys, key_arr_size, 
				parent->clk, NULL);
		ulog_add(parent->ptrs, val_arr_size, 
				parent->clk, NULL);
	}
	left_index = get_left_index(parent, left);
	if (parent->num_keys < MAX_KEYS) 
		return insert_into_node(root, parent, 
				left_index, key, right);

	return split_insert_node(root, parent, 
			left_index, key, right);
}

b_node *split_insert_into_leaf(b_node *root, b_node *leaf, 
		uint64_t key, char *rec)
{
	b_node *new_leaf;
	uint64_t *temp_keys;
	void **temp_ptrs;
	int slot = 0;
	int split, i, j;
	uint64_t new_key;
	
	/* this new leaf will be in the allocator list
	 * and will be flushed at the end of the undolog
	 * commit*/
	new_leaf = create_leaf();
	temp_keys = malloc(g_order * sizeof(uint64_t));
	if (!temp_keys) {
		perror("leaf split failed");
		exit(0);
	}
	temp_ptrs = malloc(g_order * sizeof(void *));
	if (!temp_ptrs) {
		perror("leaf split failed");
		exit(0);
	}
	while (slot < g_order - 1 && leaf->keys[slot] < key) 
		++slot;
	for (i = 0, j = 0; i < leaf->num_keys; ++i, ++j) {
		if (j == slot)  
			++j;
		temp_keys[j] = leaf->keys[i];
		temp_ptrs[j] = leaf->ptrs[i];
	}
	temp_keys[slot] = key;
	temp_ptrs[slot] = rec;
	leaf->num_keys = 0;
	split = cut(g_order - 1);
	for (i = 0; i < split; ++i) {
		/* the old leaf has been already backedup
		 * in the undolog or it should be in the
		 * allocator list*/
		leaf->ptrs[i] = temp_ptrs[i];
		leaf->keys[i] = temp_keys[i];
		++leaf->num_keys;
	}
	for (i = split, j = 0; i < g_order; ++i, ++j) {
		new_leaf->keys[j] = temp_keys[i];
		new_leaf->ptrs[j] = temp_ptrs[i];
		++new_leaf->num_keys;
	}
	free(temp_keys);
	free(temp_ptrs);
	new_leaf->ptrs[g_order - 1 ] = leaf->ptrs[g_order - 1];
	leaf->ptrs[g_order - 1] = new_leaf;
	for (i = leaf->num_keys; i < g_order - 1; ++i) {
		leaf->ptrs[i] = NULL;
	}
	for (i = new_leaf->num_keys; i < g_order - 1; ++i) {
		new_leaf->ptrs[i] = NULL;
	}
	new_leaf->parent = leaf->parent;
	new_key = new_leaf->keys[0];
	return insert_into_parent(root, leaf, 
			new_key, new_leaf);
}

int tips_bp_tree_insert(void *p_root, uint64_t key, 
		char *val)
{	
	char *old_val;
	char *free_val;
	b_node *leaf = NULL;
	b_root_obj *_root;
	b_node **root;
	b_node *temp;
	size_t key_arr_size, val_arr_size;
	int ret;

	pthread_rwlock_wrlock(&bpt_lock);
	_root = (b_root_obj *)p_root;
	root = &_root->b_root;
	/* create a new tree when root is empty*/
	if (!(*root)) {
		*root = create_bp_tree(key, val);
		pthread_rwlock_unlock(&bpt_lock);
		return 0;
	}
	
	leaf = get_leaf(*root, key);	

	/* this leaf going to be modified
	 * so add to undolog*/
	ret = ulog_add(leaf, sizeof(*leaf), 
			leaf->clk, NULL);
	if (ret) {
		/* if the leaf is backed up in the
		 * undolog successfully then 
		 * backup the key_array and val_array 
		 * associated with this leaf*/
		key_arr_size = (g_order - 1) * sizeof(uint64_t);
		val_arr_size = g_order * sizeof(void *);
		ulog_add(leaf->keys, key_arr_size, 
				leaf->clk, NULL);
		ulog_add(leaf->ptrs, val_arr_size, 
				leaf->clk, NULL);
	}
	/* search for existing record*/
/*	old_val = search_record(leaf, key, false);
	if (old_val) {
		free_val = old_val;
		old_val = val;
		nvm_free(free_val);
		pthread_rwlock_unlock(&bpt_lock);
		return 0;
	}*/

	/* try inserting new record in the leaf*/
	if (leaf->num_keys < MAX_KEYS) {
		leaf = insert_into_leaf(leaf, key, val);
		pthread_rwlock_unlock(&bpt_lock);
		return 0;
	}
	
	temp = split_insert_into_leaf(*root, leaf, 
			key, val);
	if (temp != *root) {
		/* bptree root has changed
		 * update the root obj*/
		*root = temp;
		flush_to_nvm(_root, sizeof(*_root));
		smp_mb();
	}
	pthread_rwlock_unlock(&bpt_lock);
	return 0;
}

/*int main()
{
	b_root_obj *root;
	int ret;

	uint64_t key0 = 3;
	uint64_t key1 = 1;
	uint64_t key2 = 2;
	uint64_t key3 = 34;
	uint64_t key4 = 4;
	uint64_t key5 = 55;
	uint64_t key6 = 6;
	uint64_t key7 = 7;
	uint64_t key8 = 8;
	uint64_t key9 = 96;
	uint64_t key10 = 101;
	uint64_t key11 = 111;
	uint64_t key12 = 88;
	uint64_t key13 = 63;
	uint64_t key14 = 99;
	char val[16] = "xxxxxx";

	root = init_bp_tree();
	bp_tree_read(root, key0);
	bp_tree_read(root, key1);
	bp_tree_read(root, key2);
	ret = bp_tree_insert(root, key0, val);
	print_tree(root->b_root);
	bp_tree_read(root, key0);
	printf("===========================\n");
	ret = bp_tree_insert(root, key1, val);
	print_tree(root->b_root);
	bp_tree_read(root, key1);
	printf("===========================\n");
	bp_tree_read(root, key0);
	bp_tree_read(root, key1);
	ret = bp_tree_insert(root, key3, val);
	print_tree(root->b_root);
	bp_tree_read(root, key3);
	printf("===========================\n");
	ret = bp_tree_insert(root, key5, val);
	print_tree(root->b_root);
	bp_tree_read(root, key5);
	bp_tree_read(root, key3);
	printf("===========================\n");
	ret = bp_tree_insert(root, key6, val);
	print_tree(root->b_root);
	bp_tree_read(root, key6);
	bp_tree_read(root, key3);
	printf("===========================\n");
	ret = bp_tree_insert(root, key2, val);
	print_tree(root->b_root);
	bp_tree_read(root, key2);
	bp_tree_read(root, key3);
	bp_tree_read(root, key6);
	printf("===========================\n");
	ret = bp_tree_insert(root, key4, val);
	print_tree(root->b_root);
	bp_tree_read(root, key4);
	bp_tree_read(root, key3);
	bp_tree_read(root, key6);
	printf("===========================\n");
	bp_tree_read(root, key0);
	bp_tree_read(root, key1);
	bp_tree_read(root, key2);
	bp_tree_read(root, key3);
	bp_tree_read(root, key4);
	bp_tree_read(root, key6);
	bp_tree_read(root, key5);
	bp_tree_insert(root, key10, val);
	print_tree(root->b_root);
	bp_tree_read(root, key10);
	printf("===========================\n");
	ret = bp_tree_scan(root, key0, key10);
	printf("scan count= %d\n", ret);
	ret = bp_tree_insert(root, key11, val);
	bp_tree_read(root, key11);
	print_tree(root->b_root);
	printf("===========================\n");
	ret = bp_tree_scan(root, key0, key10);
	printf("scan count= %d\n", ret);
	ret = bp_tree_insert(root, key13, val);
	bp_tree_read(root, key13);
	print_tree(root->b_root);
	printf("===========================\n");
	ret = bp_tree_insert(root, key14, val);
	bp_tree_read(root, key14);
	print_tree(root->b_root);
	printf("===========================\n");
	ret = bp_tree_insert(root, key12, val);
	bp_tree_read(root, key12);
	print_tree(root->b_root);
	printf("===========================\n");
	ret = bp_tree_insert(root, key9, val);
	print_tree(root->b_root);
	printf("===========================\n");
	ret = bp_tree_scan(root, key0, key5);
	printf("scan count= %d\n", ret);
	ret = bp_tree_scan(root, key5, key10);
	printf("scan count= %d\n", ret);
	ret = bp_tree_scan(root, key12, key9);
	printf("scan count= %d\n", ret);
	print_tree(root->b_root);
	printf("===========================\n");
	return 0;
}*/














