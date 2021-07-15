#ifndef _BST_H
#define _BST_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "tips.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEFT 0
#define RIGHT 1

typedef struct binary_search_tree
{
	uint64_t key;
	char *val;
	unsigned long clk;
	struct binary_search_tree *parent;
	struct binary_search_tree *left;
	struct binary_search_tree *right;
} bst;

typedef struct bst_root_obj
{
	bst *bst_root;
}bst_root_obj;

void *tips_init_bst();
int tips_bst_insert(void *, uint64_t, char *);
int tips_bst_delete(void *, uint64_t);
char *tips_bst_read(void *, uint64_t);
int tips_lf_bst_insert(void *, uint64_t, char *);
char *tips_lf_bst_read(void *, uint64_t);
int nvm_lf_bst_insert(void *, uint64_t, char *);
char *nvm_lf_bst_read_dl(void *, uint64_t);
char *nvm_lf_bst_read_bdl(void *, uint64_t);

#ifdef __cplusplus
}
#endif

#endif /* bst.h*/
