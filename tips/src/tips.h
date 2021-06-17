#ifndef _TIPS_H
#define _TIPS_H

#include <pthread.h>
#include <time.h>
#include "oplog.h"
#include "undolog.h"
#include "ddr.h"
#include "ddr-gc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BG_THREAD_OFF 0
#define BG_THREAD_ON 1
#define F_READER_THREAD 0x111
#define F_WRITER_THREAD 0x101
#define FREE_LIST_SIZE 25000
#define MAX_COMB_ENTRIES 32000000
#define N_WORKER_MAX 31 // for art, clht, lfht and lfbst
//#define N_WORKER_MAX 2 // for btree, ht and  bst
#define N_WORKER_MIN 1
#define N_WORKER_START 2
#define N_WORKER_STEP 4
#define BG_COMBINE 0x1010
#define BG_REPLAY 0x1111
#define BG_RECLAIM 0x1100

typedef char * (ds_read) (void *, uint64_t);
typedef int (ds_scan) (void *, uint64_t, 
		int, uint64_t *);

enum {
	epoch_0 = 0,
	epoch_1 = 1,
	epoch_2 = 2,
};

typedef struct tips_address
{
	ddr_cache *d;
	root_obj *root;
	struct tips_background_thread *bg;
	struct tips_gc_struct *epoch;
	struct tips_gc_thread *gc;
	struct tips_main_thread **load;
	struct tips_main_thread **workload;
	/*undo log address here*/
}____cacheline_aligned tips_addr;

typedef struct tips_main_thread_stats
{
	unsigned int n_writes;
	bool epoch;
	struct timespec start;
}____cacheline_aligned th_stats;

typedef struct tips_main_thread
{
	unsigned int run_cnt;
	unsigned int local_epoch;
	unsigned int id;
	unsigned int scan_violation;
	unsigned int ddr_hit;
	th_stats *stats;
	olog *o;
	uint64_t key;
	char *val;
	union {
		ds_read *_read;
		ds_insert *_insert;
		ds_delete *_delete;
		ds_scan *_scan;
	};
}____cacheline_aligned tips_thread;

/* this struct is shared by all the main threads*/
typedef struct tips_gc_struct
{
	unsigned int epoch_counter;
	int e0_index;
	int e1_index;
	int e2_index;
	void **e0; /* free list array associated with epoch #0*/
	void **e1;
	void **e2;
	tips_thread **thread_list;
}____cacheline_aligned tips_gc;

typedef struct tips_gc_thread
{
	volatile bool should_stop;
	ddr_cache *d;
	int local_epoch;
	void **epoch_arr;
	int *epoch_index;
	int n_readers;
	unsigned int n_reclaimed;
	pthread_t gc_thread;
}____cacheline_aligned tips_gc_thread;

typedef struct tips_combiner_log_entry
{
	volatile unsigned long entry_ts;
	op_entry *entry;
}____cacheline_aligned Clog_entry;
		
typedef struct tips_worker_thread
{
	pthread_t wrk_thread;
	int tid;
	unsigned int t_processed;
	unsigned int n_exec;
	unsigned int n_exec_iter;
	volatile bool need_rate;
	struct timespec start;
	struct timespec end;
	double rate;
	void *ds_root;
	int clog_tail;
	int clog_head;
	Clog_entry clog[MAX_COMB_ENTRIES];
}____cacheline_aligned tips_worker;

typedef struct tips_background_thread
{
	tips_addr *addr;
	volatile int should_stop;
	ddr_cache *d;
	void *ds_root;
	ulog *ulog;
	volatile int status;
	volatile unsigned long start_ts;
	volatile bool do_reclaim;
	volatile bool need_replay;
	volatile bool is_load;
	unsigned int count;
	unsigned int n_exec;
	unsigned int n_iters;
	int n_workers_iter;
	int workers_avg;
	double fg_rate;
	double bg_rate;
	double max_bg_rate;
	int max_rate_cnt;
	bool is_max_cnt_reached;
	int clog_tail;
	int clog_head;
	unsigned int run_op_cnt;
	unsigned int t_entries;
	unsigned int executed;
	unsigned int finish_cnt;
	pthread_t bg_thread;
	tips_worker *worker;
}____cacheline_aligned tips_bg_thread;


int tips_init(int, void *, void *);
int tips_dinit();
olog *tips_assign_oplog();
void add_oplog_root(olog *);
void tips_update_thread(tips_thread *);
char *tips_read(uint64_t, ds_read *, tips_thread *);
int tips_insert(uint64_t, char *, olog *, ds_insert *);
int tips_delete(uint64_t, olog *o, ds_delete *);
int tips_scan(uint64_t, int, uint64_t *, tips_thread *, ds_scan *);
tips_bg_thread *init_bg_thread(void *, bool);
tips_gc_thread *init_gc_thread(int);
tips_gc *init_gc_attr(int );
void oplog_destroy_safe(olog **);
void *process_op_entries(void *);
void try_combine_op_entry(tips_bg_thread *, olog *);
void sort_clog_entries(tips_bg_thread *);
void execute_clog_entry(op_entry *, void *);
int increment_epoch_counter();
void **get_epoch_array(int);
int *get_epoch_array_index(int);
int inc_epoch_index_safe(int, int);
int get_current_epoch();
int get_reclaim_epoch();
tips_thread **get_thread_list_addr();
void tips_init_threads(bool);
unsigned long tips_get_clk();
int tips_deallocate_oplog(int);
void tips_init_bg_thread(bool);
int tips_finish_load();
/* utility apis unittest only*/
void test_assign_op_addr(olog *oplog);
void test_assign_ddr_addr(ddr_cache *);
void test_assign_root(root_obj *root);
void test_assign_addr(ddr_cache *, olog *);
void test_free_addr();
void turnoff_gc_thread();
void turnoff_bg_thread();
tips_bg_thread *test_bg_thread();


#ifdef __cplusplus
}
#endif
#endif /* _TIPS_H */
