#ifndef _PMDK_H
#define _PMDK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <libpmemobj.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PMEM_PATH_DEF "/mnt/pmem0/tips" 
#define MEM_SIZE 243ul * 1024ul * 1024ul * 1024ul
#define ARRAY_SIZE 50000000
#define N_FL_THREADS 10

typedef struct tips_root_obj
{
	void *ds_addr;
	struct tips_oplog *olog_head;
	struct tips_undolog *ulog;
	volatile unsigned long last_replay_clk;
} root_obj;

typedef struct alloc_info
{
	PMEMoid obj;
	size_t size;
}alloc_info;

void *init_nvmm_pool();
void init_alloc_array();
void *nvm_alloc(size_t);
void *tips_alloc(size_t);
void *flush_alloc_list(void *);
void free_alloc(size_t);
void nvm_free(void *);
int destroy_nvmm_pool();

#ifdef __cplusplus
}
#endif
#endif /* pmdk.h*/
