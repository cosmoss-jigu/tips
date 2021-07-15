#ifndef _UNDOLOG_H
#define _UNDOLOG_H

#include <stdbool.h>
#include <pthread.h>
#include "pmdk.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_UNDO_ENTRIES 1400000 /* 32MB*/

#define UNDO_THRESHOLD 0.65
#define U_MAX_USED MAX_UNDO_ENTRIES * UNDO_THRESHOLD
#define UNDO_FREE_LIST_SIZE 1000
#define RED 1
#define BLACK 0
#define ENABLE_ULOG_STATS

typedef struct undolog_entry
{
	void *ds_entry;
	size_t size;
	char *data;
} ____ptr_aligned undo_entry;

typedef struct tips_rb_tree_node
{
	void *entry;
	size_t entry_size;
	undo_entry *u_entry;
	struct tips_rb_tree_node *left;
	struct tips_rb_tree_node *right;
	struct tips_rb_tree_node *parent;
	int color;
} ____cacheline_aligned rbt_node;

typedef struct uno_flush_thread
{
	int tid;
	pthread_t thread;
}____cacheline_aligned uno_flush_thread;

typedef struct tips_undolog
{
	volatile int tail;
	volatile unsigned int n_used;
	volatile bool need_reclaim;
	volatile unsigned long commit_ts;
	void **free_list;
	unsigned int index;
	rbt_node *rbt_root;
	unsigned int reclaim;
	uno_flush_thread fl_arr[N_FL_THREADS];
	undo_entry arr[MAX_UNDO_ENTRIES];
} ____cacheline_aligned ulog;

ulog *ulog_create();
int ulog_destroy();
int ulog_add(void *, size_t, unsigned long, void *);
int ulog_commit(bool);
void commit_free_list();
rbt_node *create_rb_node(void *, size_t, undo_entry *);
rbt_node *search_rbt(rbt_node *, void *, size_t);
int insert_rbt(void *, size_t, 
		undo_entry *, rbt_node **);
void fixup_rbt(rbt_node **, rbt_node *);
int destroy_rbt();
void print_ulog_stats();
int return_ulog_size();

#ifdef __cplusplus
}
#endif

#endif /* undolog.h*/
