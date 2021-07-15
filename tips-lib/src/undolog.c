#include "undolog.h"

ulog *g_ulog;
rbt_node nil;
rbt_node *nilptr = &nil;
pthread_mutex_t ulog_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rbt_lock = PTHREAD_RWLOCK_INITIALIZER;

static unsigned int total;
static unsigned int succ;
static unsigned int add;
static unsigned int skip_max_clk;
static unsigned int skip_exist;
static unsigned int ins_cnt;
//static int reclaim_cnt;

void set_rbt_root(rbt_node **root)
{
	nil.left = nil.right = nil.parent = nilptr;
	nil.color = BLACK;
	*root = nilptr;
}

rbt_node *search_rbt(rbt_node *root, void *target_entry, 
		size_t target_size)
{
	if (root == nilptr || root->entry == target_entry) 
			return root;
	if (target_entry < root->entry)
		return search_rbt(root->left, 
				target_entry, target_size);
	else 
		return search_rbt(root->right, 
				target_entry, target_size);
}

rbt_node *create_rb_node(void *entry, size_t size, 
		undo_entry *u_entry)
{	
	rbt_node *n;
	
	n = dram_alloc(sizeof(*n));
	if (!n)
		perror("rb node alloc failed");
	n->entry = entry;
	n->entry_size = size;
	n->u_entry = u_entry;
	n->left = n->right = nilptr;
	n->color = RED;
	return n;
}

void left_rotate(rbt_node **root, 
		rbt_node *x) 
{	
	rbt_node *y;
	
	y = x->right;
	x->right = y->left;
	if (y->left != nilptr)
		y->left->parent = x;
	y->parent = x->parent;
	if (x->parent == nilptr)
		*root = y;
	else if (x == x->parent->left)
		x->parent->left = y;
	else
		x->parent->right = y;
	y->left = x;
	x->parent = y;
	return;
}

void right_rotate(rbt_node **root, 
		rbt_node *y) 
{	
	rbt_node *x;

	x = y->left;
	y->left = x->right;
	if (x->right != nilptr)
		x->right->parent = y;
	x->parent = y->parent;
	if (y->parent == nilptr)
		*root = x;
	else if (y == y->parent->left)
		y->parent->left = x;
	else 
		y->parent->right = x;
	x->right = y;
	y->parent = x;
	return;
}

void fixup_rbt(rbt_node **root, 
		rbt_node *n)
{	
	rbt_node *uncle;

	while (n->parent->color == RED) {
		if (n->parent == n->parent->parent->left) {
			uncle = n->parent->parent->right;
			if (uncle->color == RED) {
				n->parent->color = BLACK;
				uncle->color = BLACK;
				n->parent->parent->color = RED;
				n = n->parent->parent;
			}
			else {
				if (n == n->parent->right) {
					n = n->parent;
					left_rotate(root, n);
				}
				n->parent->color = BLACK;
				n->parent->parent->color = RED;
				right_rotate(root, n->parent->parent);
			}
		}
		else {
			uncle = n->parent->parent->left;
			if (uncle->color == RED) {	
				n->parent->color = BLACK;
				uncle->color = BLACK;
				n->parent->parent->color = RED;
				n = n->parent->parent;
			}
			else {
				if (n == n->parent->left) {
					n = n->parent;
					right_rotate(root, n);
				}
				n->parent->color = BLACK;
				n->parent->parent->color = RED;
				left_rotate(root, n->parent->parent);
			}
		}
	}
	(*root)->color = BLACK;
}

int insert_rbt(void *entry, size_t size, 
		undo_entry *u_entry, rbt_node **_root)
{	
	rbt_node *new_node;
	rbt_node *root, *parent;

	new_node = create_rb_node(entry, size, 
			u_entry);
	root = *_root;
	parent = nilptr;
	++ins_cnt;
	while (root != nilptr) {
		parent = root;
		if (new_node->entry < root->entry)
			root = root->left;
		else
			root = root->right;
	}
	new_node->parent = parent;
	if (unlikely(parent == nilptr)) 
		*_root = new_node;
	else if (new_node->entry < parent->entry)
		parent->left = new_node;
	else
		parent->right = new_node;
	fixup_rbt(_root, new_node);
	return 0;
}

int destroy_rbt()
{	
	g_ulog->rbt_root = NULL;
	return 0;
}

ulog *ulog_create()
{	
	size_t size;

	g_ulog = (ulog *)nvm_alloc(sizeof(*g_ulog));
	if (!g_ulog) {
		perror("undolog create failed\n");
		return NULL;
	}
	g_ulog->tail = -1;
	g_ulog->n_used = 0;
	g_ulog->need_reclaim = false;
	g_ulog->rbt_root = NULL;
	g_ulog->commit_ts = MAX_CLK;
	g_ulog->index = 0;
	g_ulog->reclaim = 0;
	size = UNDO_FREE_LIST_SIZE * sizeof(void *);
	g_ulog->free_list = nvm_alloc(size);
	memset(g_ulog->free_list, 0, size);
	flush_to_nvm(g_ulog->free_list, size);
	flush_to_nvm(g_ulog, sizeof(*g_ulog));
	for (int i = 0; i < N_FL_THREADS; ++i) {
		if (i < N_FL_THREADS/2)
			g_ulog->fl_arr[i].tid = i;
		else 
			g_ulog->fl_arr[i].tid = N_FL_THREADS - i - 1;
	}
	size = sizeof(undo_entry) * MAX_UNDO_ENTRIES;
	memset(g_ulog->arr, 0, size);
	flush_to_nvm(g_ulog->arr, size);
	smp_mb();
	return g_ulog;
}

void print_ulog_stats()
{
	printf("total_ulog_calls= %d\n", total);
	printf("ulog_skip_max= %d\n", skip_max_clk);
	printf("ulog_skip_new= %d\n", skip_exist);
	printf("n_ulog_enq= %d\n", add);
	printf("n_ulog_exists= %d\n", succ);
	printf("n_UNO_reclamation= %d\n", g_ulog->reclaim);
	return;
}

int return_ulog_size()
{
	return MAX_UNDO_ENTRIES;
}

int ulog_destroy()
{
	nvm_free(g_ulog);
	return 0;
}

int incr_tail_safe(int index) 
{
	if (unlikely(index + 1 >= MAX_UNDO_ENTRIES)) {
		index = 0;
		return index;
	}
	index += 1;
	if (unlikely(index >= U_MAX_USED)) {
		g_ulog->need_reclaim = true;
	}
	return index;
}

void update_undo_tail(int index)
{
	g_ulog->tail = index;
	flush_to_nvm((void *)&g_ulog->tail, 
			sizeof(g_ulog->tail));
	smp_mb();
	return;
}

int ulog_add(void *entry, size_t size, 
		unsigned long clk, void *free_entry)
{
	volatile int tail;
	char **data;
	rbt_node *ret;
	
#ifdef ENABLE_ULOG_STATS
	smp_faa(&total, 1);
#endif 
	if (!entry && !free_entry)
		return 1;
	if (free_entry) {
		g_ulog->free_list[g_ulog->index] 
			= free_entry;
		++g_ulog->index;
		if (!entry) {
			return 0;
		}
	}

	/* return if undolog is not yet 
	 * committed once*/
	if (g_ulog->commit_ts == MAX_CLK) {
	#ifdef ENABLE_ULOG_STATS
		smp_faa(&skip_max_clk, 1);
	#endif 
		return 0;
	}

	/* if the given entry is new(>) undo commit clk
	 * simply return, no need of backing up*/
	if (gte_clk(clk, g_ulog->commit_ts)) {
	#ifdef ENABLE_ULOG_STATS
		smp_faa(&skip_exist, 1);
	#endif 
		return 0;
	}

	/* else, search for the entry in rb_tree
	 * to see if its backed up already*/
	if (unlikely(!g_ulog->rbt_root)) {
		pthread_rwlock_wrlock(&rbt_lock);
		set_rbt_root(&g_ulog->rbt_root);
		pthread_rwlock_unlock(&rbt_lock);
	}
	pthread_rwlock_rdlock(&rbt_lock);
	ret = search_rbt(g_ulog->rbt_root, entry, size);
	pthread_rwlock_unlock(&rbt_lock);

	/* if the entry is present in rbt
	 * simply return*/
	if (ret != nilptr) {
	#ifdef ENABLE_ULOG_STATS
		smp_faa(&succ, 1);
	#endif 
		return 0;
	}

	/* else add the entry into undolog*/
	pthread_mutex_lock(&ulog_lock);
	tail = incr_tail_safe(g_ulog->tail);
	g_ulog->arr[tail].ds_entry = entry;
	g_ulog->arr[tail].size = size;
	data = &g_ulog->arr[tail].data;
	if (!(*data)) {
		*data = nvm_alloc(size);
	}
	memcpy(*data, entry, size);
	flush_to_nvm(&g_ulog->arr[tail], sizeof(undo_entry));
	flush_to_nvm(*data, size);
	smp_mb();
	update_undo_tail(tail);
	pthread_mutex_unlock(&ulog_lock);
	pthread_rwlock_wrlock(&rbt_lock);
	insert_rbt(entry, size, &g_ulog->arr[tail], 
			&g_ulog->rbt_root);
	++add;
	pthread_rwlock_unlock(&rbt_lock);
	return 1;
}

void reset_undo_tail()
{	
	g_ulog->tail = -1;
	flush_to_nvm((void *)&g_ulog->tail, 
			sizeof(g_ulog->tail));
	smp_mb();
	return;
}

void *flush_ulog(void *arg)
{
	int *p_tid = (int *) arg;
	int tid = *p_tid;
	volatile int tail = g_ulog->tail;
	int i, start, end;

	if (tail % 2 != 0)
		smp_faa(&tail, 1);
	start = (tail/N_FL_THREADS) * tid;
	end = start + (tail/N_FL_THREADS);
	for (i = start; i < end; ++i) {
		flush_to_nvm(g_ulog->arr[i].ds_entry, 
				g_ulog->arr[i].size);
	}
	return NULL;
}

int ulog_commit(bool reset) 
{
	int i, ret;
	
	++g_ulog->reclaim;
	for (i = 0; i < N_FL_THREADS; ++i) {
		if (i < N_FL_THREADS/2) {
			ret = pthread_create(&(g_ulog->fl_arr[i].thread), NULL, 
				flush_alloc_list, &g_ulog->fl_arr[i].tid);
			if (ret) {
				perror("fl thread create failed for \n");
			}
		}
		else {
			/* TODO: can skip the first commit?*/
			ret = pthread_create(&(g_ulog->fl_arr[i].thread), NULL, 
				flush_ulog, &g_ulog->fl_arr[i].tid);
			if (ret) {
				perror("fl thread create failed for \n");
			}
		}
	}
	for (i = 0; i < N_FL_THREADS; ++i) {
		pthread_join(g_ulog->fl_arr[i].thread, NULL);
	}
	smp_wmb();
	g_ulog->commit_ts = get_clk();
	flush_to_nvm((void *)&g_ulog->commit_ts, 
			sizeof(g_ulog->commit_ts));
	smp_wmb();
	reset_undo_tail();
	destroy_rbt();
	if (reset) {
		g_ulog->tail = -1;
		g_ulog->n_used = 0;
		g_ulog->need_reclaim = false;
		g_ulog->rbt_root = NULL;
		g_ulog->commit_ts = MAX_CLK;
		g_ulog->index = 0;
	//	g_ulog->reclaim = 0;
		flush_to_nvm(g_ulog, sizeof(*g_ulog));
		smp_wmb();
	}
	return 0;
}

void commit_free_list()
{
	unsigned int i, end;
	void *addr;
	
	if (!g_ulog->free_list)
		return;
	end = g_ulog->index;
	for(i = 0; i < end; ++i) {
		addr = g_ulog->free_list[i];
		nvm_free(addr);
	}
	return;
}
















