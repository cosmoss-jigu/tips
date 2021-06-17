#include "bst.h"

pthread_rwlock_t bst_lock = PTHREAD_RWLOCK_INITIALIZER;

void *tips_init_bst()
{
	bst_root_obj *bt_root_obj;

	bt_root_obj = (bst_root_obj *)tips_alloc(sizeof(*bt_root_obj));
	if (!bt_root_obj) {
		perror("init bst failed");
		exit(0);
	}
	bt_root_obj->bst_root = NULL;
	return bt_root_obj;
}

bst *create_bst_node(uint64_t key, char *val, 
		bst *parent, int pos)
{
	bst *node;

	node = (bst *)tips_alloc(sizeof(*node));
	if (!node) {
		perror("bst node create failed");
		exit(0);
	}
	node->key = key;
	node->val = val;
	node->clk = tips_get_clk();
	node->left = node->right = NULL;
	node->parent = parent;
	if (!parent)
		return node;

	/* add the parent to undolog 
	 * before modifying it*/
	ulog_add(parent, sizeof(*parent), 
			parent->clk, NULL);

	/* TODO: modify here for lockfree bst*/
	if (pos == LEFT)
		parent->left = node;
	else 
		parent->right = node;
	return node;
}

int bst_insert(bst *node, bst *parent, 
		uint64_t key, char *val, int pos)
{
	int ret;

	if (!node) {
		create_bst_node(key, val, 
				parent, pos);
		return 0;
	}
	if (key == node->key) {
		/* add to undolog before updating 
		 * the val*/
		ulog_add(node, sizeof(*node), 
				node->clk, NULL);
		node->val = val;
		return 0;
	}
	if (key < node->key) 
		ret = bst_insert(node->left, node, 
				key, val, LEFT);
	else
		ret = bst_insert(node->right, node, 
				key, val, RIGHT);
	return ret;
}

int tips_bst_insert(void *p_root, uint64_t key, 
		char *val)
{	
	bst_root_obj *_root;
	bst **root;
	int ret;
	
	pthread_rwlock_wrlock(&bst_lock);
	_root = (bst_root_obj *)p_root;
	root = &_root->bst_root;
	if (!(*root)) {
		*root = create_bst_node(key, val, 
				NULL, -1);
		pthread_rwlock_unlock(&bst_lock);
		return 0;
	}
	ret = bst_insert(*root, NULL, key, val, -1);
	pthread_rwlock_unlock(&bst_lock);
	return ret;
}

char *bst_search_dl(bst *node, uint64_t target_key)
{
	char *val;
	
	if (!node)
		return NULL;
	if (node->key == target_key) 
		return node->val;
	if (target_key < node->key) {
		flush_to_nvm(node->left, sizeof(*node));
		smp_mb();
		val = bst_search_dl(node->left, target_key);
	}
	else {
		flush_to_nvm(node->right, sizeof(*node));
		smp_mb();
		val = bst_search_dl(node->right, target_key);
	}
	return val;
}

char *bst_search(bst *node, uint64_t target_key)
{
	char *val;
	
	if (!node)
		return NULL;
	if (node->key == target_key)
		return node->val;
	if (target_key < node->key)
		val = bst_search(node->left, target_key);
	else
		val = bst_search(node->right, target_key);
	return val;
}

char *tips_bst_read(void *p_root, uint64_t target_key)
{	
	bst_root_obj *_root;
	bst *root;
	char *val;

	pthread_rwlock_rdlock(&bst_lock);
	_root = (bst_root_obj *)p_root;
	root = _root->bst_root;
	val = bst_search(root, target_key);
	pthread_rwlock_unlock(&bst_lock);
	return val;
}

int get_pos_for_retry(uint64_t key, bst *node, bst *parent)
{
	int pos;

	if (key < parent->key) {
		pos = LEFT;
		node->parent = parent;
	}
	else {
		pos = RIGHT;
		node->parent = parent;
	}
	return pos;
}

bst *create_lf_bst_node(uint64_t key, char *val, 
		bst *parent, int pos)
{
	bst *node, *new_parent, *left = NULL, *right = NULL;
	int ret;

	node = (bst *)tips_alloc(sizeof(*node));
	if (!node) {
		perror("bst node create failed");
		exit(0);
	}
	node->key = key;
	node->val = val;
	node->clk = tips_get_clk();
	node->left = node->right = NULL;
	node->parent = parent;
	if (!parent)
		return node;

cas_retry:
	/* add the parent to undolog 
	 * before modifying it*/
	ulog_add(parent, sizeof(*parent), 
			parent->clk, NULL);

	if (pos == LEFT) 
		ret = smp_cas_v(&parent->left, left, 
				node, new_parent);
	else 
		ret = smp_cas_v(&parent->right, right, 
				node, new_parent);
	if (!ret) {
		pos = get_pos_for_retry(key, node, new_parent);
		parent = new_parent;
		goto cas_retry;
	}
	return node;
}

bst *_create_lf_bst_node(uint64_t key, char *p_val, 
		bst *parent, int pos)
{
	bst *node;
	int val_len;
	char *val;

	val_len = strlen(p_val) + 1;
	val = nvm_alloc(val_len);
	memcpy(val, p_val, val_len);
	flush_to_nvm(val, val_len);
	smp_mb();
	node = (bst *)nvm_alloc(sizeof(*node));
	if (!node) {
		perror("bst node create failed");
		exit(0);
	}
	node->key = key;
	node->val = val;
	node->clk = tips_get_clk();
	node->left = node->right = NULL;
	node->parent = parent;
	flush_to_nvm(node, sizeof(*node));
	smp_mb();
	if (!parent)
		return node;
	flush_to_nvm(parent, sizeof(*parent));
	smp_mb();
	if (pos == LEFT) {
		smp_atomic_store(&parent->left, node);
		flush_to_nvm(parent->left, sizeof(*parent));
		smp_mb();
	}
	else 
		smp_atomic_store(&parent->right, node);
		flush_to_nvm(parent->right, sizeof(*parent));
		smp_mb();
	return node;
}

int lf_bst_insert(bst *node, bst *parent, 
		uint64_t key, char *val, int pos)
{
	int ret;
	char *curr_val;

	if (!node) {
		create_lf_bst_node(key, val, 
				parent, pos);
		return 0;
	}
	if (key == node->key) {
		/* add to undolog before updating 
		 * the val*/
		ulog_add(node, sizeof(*node), 
				node->clk, NULL);
		curr_val = node->val;
		while (!smp_cas(&node->val, curr_val, val)) {
			curr_val = node->val;
			continue;
		}
		return 0;
	}
	if (key < node->key) 
		ret = lf_bst_insert(node->left, node, 
				key, val, LEFT);
	else
		ret = lf_bst_insert(node->right, node, 
				key, val, RIGHT);
	return ret;
}

int _lf_bst_insert(bst *node, bst *parent, 
		uint64_t key, char *p_val, int pos)
{
	int ret;
	char *curr_val;
	int val_len;
	char *val;

	val_len = strlen(p_val) + 1;
	val = nvm_alloc(val_len);
	memcpy(val, p_val, val_len);
	flush_to_nvm(val, val_len);
	smp_mb();

	if (!node) {
		_create_lf_bst_node(key, val, 
				parent, pos);
		return 0;
	}
	if (key == node->key) {
		curr_val = node->val;
		while (!smp_cas(&node->val, curr_val, val)) {
			curr_val = node->val;
			continue;
		}
		flush_to_nvm(node, sizeof(*node));
		smp_mb();
		return 0;
	}
	if (key < node->key) {
		flush_to_nvm(node->left, sizeof(*node));
		smp_mb();
		ret = _lf_bst_insert(node->left, node, 
				key, val, LEFT);
	}
	else {
		flush_to_nvm(node->right, sizeof(*node));
		smp_mb();
		ret = _lf_bst_insert(node->right, node, 
				key, val, RIGHT);
	}
	return ret;
}

int nvm_lf_bst_insert(void *p_root, uint64_t key, 
		char *val)
{	
	bst_root_obj *_root;
	bst **root;
	bst *temp;
	int ret;
	
	_root = (bst_root_obj *)p_root;
	root = &_root->bst_root;
	temp = smp_atomic_load(root);
	flush_to_nvm(temp, sizeof(*temp));
	smp_mb();
	if (!temp) {
		temp = _create_lf_bst_node(key, val, 
				NULL, -1);
		smp_atomic_store(root, temp);
		flush_to_nvm(root, sizeof(*root));
		smp_mb();
		return 0;
	}
	ret = _lf_bst_insert(temp, NULL, key, val, -1);
	return ret;
}

int tips_lf_bst_insert(void *p_root, uint64_t key, 
		char *val)
{	
	bst_root_obj *_root;
	bst **root;
	bst *temp;
	int ret;
	
	_root = (bst_root_obj *)p_root;
	root = &_root->bst_root;
	temp = smp_atomic_load(root);
	if (!temp) {
		temp = create_lf_bst_node(key, val, 
				NULL, -1);
		smp_atomic_store(root, temp);
		return 0;
	}
	ret = lf_bst_insert(temp, NULL, key, val, -1);
	return ret;
}

char *tips_lf_bst_read(void *p_root, uint64_t target_key)
{	
	bst_root_obj *_root;
	bst *root;
	char *val;

	_root = (bst_root_obj *)p_root;
	root = _root->bst_root;
	val = bst_search(root, target_key);
	return val;
}

char *nvm_lf_bst_read_dl(void *p_root, uint64_t target_key)
{	
	bst_root_obj *_root;
	bst *root;
	char *val;

	_root = (bst_root_obj *)p_root;
	root = _root->bst_root;
	flush_to_nvm(root, sizeof(*root));
	smp_mb();
	val = bst_search_dl(root, target_key);
	return val;
}

char *nvm_lf_bst_read_bdl(void *p_root, uint64_t target_key)
{	
	bst_root_obj *_root;
	bst *root;
	char *val;

	_root = (bst_root_obj *)p_root;
	root = _root->bst_root;
	val = bst_search(root, target_key);
	return val;
}
