#ifndef _OPLOG_H
#define _OPLOG_H

#include <stdbool.h>
#include "pmdk.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_OP_ENTRIES 800000 /* 32MB*/
#define RECLAIM_THRESHOLD 0.60
#define MAX_USED MAX_OP_ENTRIES * RECLAIM_THRESHOLD
#define RECLAIMED 0
#define REPLAY_YET 1

typedef int ds_insert (void *, uint64_t, char *);
typedef int ds_delete (void *, uint64_t);

typedef struct oplog_entry
{
	uint64_t key;
	char *val;
	union 
	{
		ds_insert *_insert;
		ds_delete *_delete;
	};
	volatile unsigned long enq_ts;
	int is_delete;
}____ptr_aligned op_entry;

typedef struct tips_oplog
{
	struct tips_oplog *next;
	volatile int tail_index;
	volatile int prev_tail;
	volatile int head_index;
	volatile int reclaim_index;
	volatile unsigned int n_used;
	volatile unsigned int n_applied;
	volatile bool need_reclaim;
	op_entry arr[MAX_OP_ENTRIES];
}____cacheline_aligned olog;

olog *oplog_create();
int oplog_destroy(olog *);
void oplog_destroy_safe(olog **);
int oplog_insert_write(olog *, uint64_t, char *, 
		unsigned long, ds_insert *);
int oplog_insert_delete(olog *, uint64_t, int , 
		unsigned long, ds_delete *);
void update_op_head(olog *);
void update_op_tail(olog *, volatile int);
void update_op_reclaim(olog *, volatile int);
int incr_index_safe(int);
void reclaim_oplog(olog *);
int oplog_destroy_unused(int, olog **);
int return_olog_size();

#ifdef __cplusplus
}
#endif
#endif /* oplog.h*/
