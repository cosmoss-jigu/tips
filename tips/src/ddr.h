#ifndef _DDR_H
#define _DDR_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "util.h"
#include "siphash.h"
#include "oplog.h"
#ifdef __cplusplus
extern "C" {
#endif 


/* ddr % for 32M keys*/
//#define MAX_BUCKET 640229 //~10%
//#define MAX_BUCKET 1701023 //~25%
#define MAX_BUCKET 1600259 //~25%
//#define MAX_BUCKET 3400043 //~50%
//#define MAX_BUCKET 6400013 //~100%
//#define MAX_BUCKET 31999829 //VM%

/* !!!!! do not change anything here !!!!!*/
#define F_WRITE 0x11
#define F_DEL 0x111
#define F_GC_ONLINE 0x10
#define F_GC_OFFLINE 0x01
#define F_REPLIED 0x101
#define F_NOT_REPLIED 0x110
#define MAX_CHAIN_LENGTH 5

typedef struct ddr_entry
{
	uint64_t key;
	void *val;
	int is_delete;
	int status;
	struct ddr_entry *next;
}____cacheline_aligned ddr_entry;

typedef struct ddr_bucket
{	
	ddr_entry *head;
	unsigned int chain_length;
	volatile bool is_gc_detected;
	volatile bool is_gc_requested;
	unsigned int n_applied_entries;
	pthread_spinlock_t bucket_lock;
}____cacheline_aligned ddr_bucket;

typedef struct gc_work_queue
{
	uint64_t arr[MAX_BUCKET];
	volatile int tail;
	volatile int head;
}work_q;

typedef struct tips_ddr_cache
{
	ddr_bucket arr[MAX_BUCKET];
	unsigned int n_ddr_entries;
	work_q *q;
}____cacheline_aligned ddr_cache;


ddr_cache *ddr_cache_create();
int ddr_cache_destroy(ddr_cache *);
uint64_t get_hash_key(uint64_t);
ddr_entry *ddr_cache_read(ddr_cache *, uint64_t);
unsigned long ddr_cache_write(ddr_cache *, uint64_t, 
		char *, olog *, ds_insert *);
unsigned long ddr_cache_delete(ddr_cache *, uint64_t);
void update_ddr_entry_status(ddr_cache *, uint64_t, char *);
int return_ddr_size();

#ifdef __cplusplus
}
#endif
#endif /* ddr.h*/
