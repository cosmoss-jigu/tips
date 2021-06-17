#ifndef _LIST_H
#define _LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define TEST_LIST
#define FREE 0
#define IN_USE 1

#ifndef TEST_LIST
#include "arch.h"
#include "util.h"
#include "pmdk.h"
#include "tips.h"
#endif

#ifdef TEST_LIST
#define smp_cas(__ptr, __old_val, __new_val)                                   \
	__sync_bool_compare_and_swap(__ptr, __old_val, __new_val)
#endif

typedef struct node
{
	uint64_t key;
	char *val;
	volatile unsigned long clk;
	pthread_rwlock_t bkt_lock;
	struct node *next;
}node;

typedef struct undolog 
{	
	uint64_t key;
	char *val;
	node *next;
	bool status;
}undolog;

void init_undolog();
#ifndef TEST_LIST
node *init_list();
int list_insert(void *, uint64_t, char *);
int list_delete(void *, uint64_t);
char *list_read(void *, uint64_t);
int list_scan(void *, uint64_t, uint64_t);

int tips_list_insert(void *, uint64_t, char *);
int tips_list_delete(void *, uint64_t);
char *tips_list_read(void *, uint64_t);
int tips_lf_list_insert(void *, uint64_t, char *);
int tips_lf_list_delete(void *, uint64_t);
char *lf_list_read_dl(void *, uint64_t);
char *lf_list_read_bdl(void *, uint64_t);
int lf_list_insert(void *, uint64_t, char *);
#endif
node *dram_init_list();
int dram_list_insert(void *, uint64_t, char *);
int dram_list_delete(void *, uint64_t);
char *dram_list_read(void *, uint64_t);

#ifdef __cplusplus
}
#endif

#endif /* list.h*/

