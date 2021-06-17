#ifndef _BENCH_H
#define _BENCH_H

#include "tips.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define IS_TIPS

#define TIME_BASED_RUN 0
#define COUNT_BASED_RUN 1 
#define DEFAULT_DS_TYPE "list"
#define DEFAULT_BENCH_TYPE TIME_BASED_RUN
#define DEFAULT_DURATION 10000
#define DEFAULT_OP_COUNT 1000000
#define DEFAULT_INITIAL_SIZE 1000
#define DEFAULT_UPDATE_RATIO 200
#define MAX_UPDATE_RATIO 1000
#define DEFAULT_RANGE 5000
#define DEFAULT_N_THREADS 112
#define DEFAULT_KEY_SIZE 5
#define DEFAULT_VAL_SIZE 5
#define DEFAULT_MAIN_SEED 0
#define BENCH_START 1
#define BENCH_STOP 0

typedef struct thread_data
{
	unsigned int tid;
	unsigned int range;
	unsigned int op_cnt;
	unsigned int update;
	unsigned int key_seed;
	unsigned int val_seed;
	unsigned int ops_seed;
	unsigned int incr_seed;
	size_t key_size;
	size_t val_size;
	char *key1;
	char *key2;
	char *key3;
	tips_thread *self;
	unsigned long n_create;
	unsigned long n_update;
	unsigned long n_delete_key;
	unsigned long n_delete_null;
	unsigned long n_read_key;
	unsigned long n_read_null;
	unsigned long n_scan_key;
	unsigned long n_scan_null;
#ifndef IS_TIPS
	void *ds_root;
#endif
}____cacheline_aligned thread_data;

#ifdef __cplusplus
}
#endif
#endif /* bench.h*/
