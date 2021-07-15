#include <string.h>
#include <pthread.h>
#include "ds.h"
#include "art_wrapper.h"

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

int g_list, g_hlist, g_bst, g_btree, g_lf_bst, g_lf_hash, g_art, g_mt,g_clht, 
	g_p_bst_dl, g_p_bst_bdl, g_p_hlist_dl, g_p_hlist_bdl; 
void *g_root;

void *set_ds_type(const char *type)
{	
	if (!strcmp(type, "list")) {
		g_list = 1;
		g_hlist = 0;
		g_lf_hash = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_btree = 0;
		g_art=0;
		g_mt=0;
		g_clht=0; 
		g_root = init_list();
		printf("Running Linkedlist...\n");
		return g_root;
	}
	if (!strcmp(type, "hlist")) {
		g_hlist = 1;
		g_list = 0;
		g_lf_hash = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_btree = 0;
		g_art=0;
		g_mt=0;
		g_clht=0; 
		g_root = init_hash();
		printf("Running Hashtable...\n");
		return g_root;
	}
	if (!strcmp(type, "lfhlist")) {
		g_lf_hash = 1;
		g_hlist = 0;
		g_list = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_btree = 0;
		g_art=0;
		g_mt=0;
		g_clht=0; 
		g_root = init_hash();
		printf("Running LF-Hashtable...\n");
		return g_root;
	}
	if (!strcmp(type, "slist")) {
		g_hlist = 0;
		g_lf_hash = 0;
		g_list = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_btree = 0;
		g_art=0;
		g_mt=0;
		g_clht=0; 
		return g_root;
	}
	if (!strcmp(type, "bst")) {
		g_bst = 1;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_btree = 0;
		g_art=0;
		g_mt=0;
		g_clht=0; 
		g_root = tips_init_bst();
		printf("Running BST...\n");
		return g_root;
	}
	if (!strcmp(type, "lfbst")) {
		g_lf_bst = 1;
		g_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_btree = 0;
		g_art=0;
		g_mt=0;
		g_clht=0; 
		g_root = tips_init_bst();
		printf("Running LF-BST...\n");
		return g_root;
	}
	if (!strcmp(type, "btree")) {
		g_btree = 1;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_mt=0;
		g_clht=0; 
		g_root = tips_init_bp_tree();
		printf("Running B+tree...\n");
		return g_root;
	}
	if (!strcmp(type, "art")) {
		g_btree = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_art=1;
		g_mt=0;
		g_clht=0; 
		g_root = init_art_tree();
		printf("Running tips-ART...\n");
		return g_root;
	}
	if (!strcmp(type, "clht")) {
		g_btree = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_art=0;
		g_mt=0;
		g_clht=1; 
		g_root = init_clht();
		printf("Running tips-CLHT...\n");
		return g_root;
	}
	if (!strcmp(type, "p_hlist_dl")) {
		g_btree = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_art=0;
		g_mt=0;
		g_clht=0;
		g_p_hlist_dl=1;
		g_p_hlist_bdl=0;
		g_p_bst_dl=0;
		g_p_bst_bdl=0;
		g_root = init_hash();
		printf("Running LF-DL-hlist...\n");
		return g_root;
	}

	if (!strcmp(type, "p_hlist_bdl")) {
		g_btree = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_art=0;
		g_mt=0;
		g_clht=0;
		g_p_hlist_dl=0;
		g_p_hlist_bdl=1;
		g_p_bst_dl=0;
		g_p_bst_bdl=0;
		g_root = init_hash();
		printf("Running LF-BDL-hlist...\n");
		return g_root;
	}

	if (!strcmp(type, "p_bst_bdl")) {
		g_btree = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_art=0;
		g_mt=0;
		g_clht=0;
		g_p_hlist_dl=0;
		g_p_hlist_bdl=0;
		g_p_bst_dl=0;
		g_p_bst_bdl=1;
		g_root = tips_init_bst();
		printf("Running LF-BDL-BST...\n");
		return g_root;
	}

	if (!strcmp(type, "p_bst_dl")) {
		g_btree = 0;
		g_bst = 0;
		g_lf_bst = 0;
		g_list = 0;
		g_hlist = 0;
		g_lf_hash = 0;
		g_art=0;
		g_mt=0;
		g_clht=0;
		g_p_hlist_dl=0;
		g_p_hlist_bdl=0;
		g_p_bst_dl=1;
		g_p_bst_bdl=0;
		g_root = tips_init_bst();
		printf("Running LF-DL-BST...\n");
		return g_root;
	}
	/* should not be here*/
	perror("Invalid Data Structure\n");
	exit(0);
	return NULL;
}

fn_read *get_read_fn_ptr()
{	
	if (g_list) {
		return tips_list_read;
	}
	if (g_hlist) {
		return tips_hash_read;
	}
	if (g_lf_hash) {
		return tips_lf_hash_read;
	}
	if (g_bst) {
		return tips_bst_read;
	}
	if (g_lf_bst) {
		return tips_lf_bst_read;
	}
	if (g_btree) {
		return tips_bp_tree_read;
	}
	if (g_art) {
		return tips_art_read;
	}
	if (g_clht) { 
		return tips_clht_read;
	}
	if (g_p_hlist_dl) { 
		return lf_hash_read_dl;
	}
	if (g_p_hlist_bdl) { 
		return lf_hash_read_bdl;
	}
	if (g_p_bst_dl) {
		return nvm_lf_bst_read_dl;
	}
	if (g_p_bst_bdl) {
		return nvm_lf_bst_read_bdl;
	}
	return NULL;
}

fn_scan *get_scan_fn_ptr()
{	
	if (g_btree)
		return tips_bp_tree_scan;
	if (g_art)
		return tips_art_scan;
	return NULL;
}

fn_insert *get_insert_fn_ptr()
{
	if (g_list)
		return tips_list_insert;
	if (g_hlist)
		return tips_hash_insert;
	if (g_lf_hash)
		return tips_lf_hash_insert;
	if (g_bst)
		return tips_bst_insert;
	if (g_lf_bst)
		return tips_lf_bst_insert;
	if (g_btree)
		return tips_bp_tree_insert;
	if (g_art) 
		return tips_art_insert;
	if (g_clht) 
		return tips_clht_insert; 
	if (g_p_hlist_dl) { 
		return lf_hash_insert;
	}
	if (g_p_hlist_dl) { 
		return lf_hash_insert;
	}
	if (g_p_bst_dl) {
		return nvm_lf_bst_insert;
	}
	if (g_p_bst_bdl) {
		return nvm_lf_bst_insert;
	}
	
	return NULL;
}

fn_delete *get_delete_fn_ptr()
{
	if (g_list)
		return tips_list_delete;
	if (g_hlist)
		return tips_hash_delete;
	if (g_art) 
		return tips_art_remove;
	return NULL;
}

int lf_insert(uint64_t key, char *val)
{	
	int ret = -1;

	if (g_p_hlist_dl) {
		ret = lf_hash_insert(g_root, key, val);
		return ret;
	}
	if (g_p_hlist_bdl) {
		ret = lf_hash_insert(g_root, key, val);
		return ret;
	}
	if (g_p_bst_dl) {
		ret = nvm_lf_bst_insert(g_root, key, val);
		return ret;
	}
	if (g_p_bst_bdl) {
		ret = nvm_lf_bst_insert(g_root, key, val);
		return ret;
	}
	return ret;
}

int _insert(uint64_t key, char *val)
{	
printf("-insert\n");
	int ret = -1;

	if (g_list) {
		pthread_rwlock_wrlock(&rwlock);
		ret = list_insert(g_root, 
				key, val);
		pthread_rwlock_unlock(&rwlock);
		return ret;
	}
	if (g_hlist) {
		pthread_rwlock_wrlock(&rwlock);
		ret = hash_insert(g_root, 
				key, val);
		pthread_rwlock_unlock(&rwlock);
		return ret;
	}
	return ret;
}

int _delete(uint64_t key)
{	
	int ret = -1;

	if (g_list) {
		pthread_rwlock_wrlock(&rwlock);
		ret = list_delete(g_root, 
				key);
		pthread_rwlock_unlock(&rwlock);
		return ret;
	}
	if (g_hlist) {
		pthread_rwlock_wrlock(&rwlock);
		ret = hash_delete(g_root, 
				key);
		pthread_rwlock_unlock(&rwlock);
		return ret;
	}
	return ret;
}

char *lf_read(uint64_t key)
{
	char *val = NULL;

	if (g_p_hlist_dl) {
		val = lf_hash_read_dl(g_root, key);
		return val;
	}
	if (g_p_hlist_bdl) {
		val = lf_hash_read_bdl(g_root, key);
		return val;
	}
	if (g_p_bst_dl) {
		val = nvm_lf_bst_read_dl(g_root, key);
		return val;
	}
	if (g_p_bst_bdl) {
		val = nvm_lf_bst_read_bdl(g_root, key);
		return val;
	}
	return val;
}

char *_read(uint64_t key)
{
	char *val = NULL;

	if (g_list) {
		pthread_rwlock_rdlock(&rwlock);
		val = list_read(g_root,
				key);
		pthread_rwlock_unlock(&rwlock);
		return val;
	}

	if (g_hlist) {
		pthread_rwlock_rdlock(&rwlock);
		val = hash_read(g_root,
				key);
		pthread_rwlock_unlock(&rwlock);
		return val;
	}
	return val;
}

int _scan(uint64_t start_key, uint64_t end_key)
{
	int count = 0;

/*	if (g_list) {
		pthread_rwlock_rdlock(&rwlock);
		count = list_scan(g_list_root, 
				start_key, end_key);
		pthread_rwlock_unlock(&rwlock);
	}*/
	return count;
}

















