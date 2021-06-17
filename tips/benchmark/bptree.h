#ifndef _BPTREE_H
#define _BPTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "tips.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define DEFAULT_TREE_ORDER 30 
//#define DEFAULT_TREE_ORDER 256
#define DEFAULT_TREE_ORDER 128
#define MAX_KEYS DEFAULT_TREE_ORDER - 1 
#define DEFAULT_VAL_LEN 256
#define B_SEARCH

typedef struct bp_tree_node 
{
	uint64_t keys[DEFAULT_TREE_ORDER];
	void *ptrs[DEFAULT_TREE_ORDER];
	struct bp_tree_node *parent;
	bool is_leaf;
	int num_keys;
	unsigned long clk;
	struct bp_tree_node *next;
}b_node;

typedef struct bp_tree_undolog
{	
	b_node *node;
	void *record;
	bool is_record;
	char *data;
}b_ulog;

typedef struct bp_tree_root_obj
{
	b_node *b_root;
}b_root_obj;

b_root_obj *tips_init_bp_tree();
int tips_bp_tree_insert(void *, uint64_t, char *);
char *tips_bp_tree_read(void *, uint64_t);
int tips_bp_tree_scan(void *, uint64_t, int, uint64_t *);
b_node *insert_into_parent(b_node *, b_node *, 
		uint64_t, b_node *);



#ifdef __cplusplus
}
#endif

#endif /* bptree.h*/
