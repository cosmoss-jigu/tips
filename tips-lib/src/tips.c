#include <stdlib.h>
#include <assert.h>
#include "tips.h"

static tips_addr *addr;
static int g_n_threads;
static int g_n_workers;
static int g_avg_workers;
static unsigned int tw_processed;
static unsigned int tw_executed;
static unsigned int tw_processed_w;
static unsigned int tw_executed_w;
//static int n_violation;

unsigned long tips_get_clk()
{
	return get_clk();
}

tips_gc *init_gc_attr(int n_threads)
{
	tips_gc *epoch;

	epoch = (tips_gc *)dram_alloc(sizeof(*epoch));
	epoch->epoch_counter = 0;
	epoch->e0 = (void **)dram_calloc(FREE_LIST_SIZE, sizeof(void *));
	epoch->e1 = (void **)dram_calloc(FREE_LIST_SIZE, sizeof(void *));
	epoch->e2 = (void **)dram_calloc(FREE_LIST_SIZE, sizeof(void *));
	epoch->e0_index = epoch->e1_index = epoch->e2_index = 0;
	epoch->thread_list = (tips_thread **)dram_calloc
		(n_threads, sizeof(void *));
	return epoch;
}

void destroy_gc_attr(tips_gc *epoch)
{
	dram_free(epoch->e0);
	dram_free(epoch->e1);
	dram_free(epoch->e2);
	dram_free(epoch->thread_list);
	dram_free(epoch);
	return;
}

int get_current_epoch()
{
	return addr->epoch->epoch_counter;
}

int increment_epoch_counter()
{
	int current_epoch = get_current_epoch();

	current_epoch += 1;
	current_epoch %= 3;
	smp_atomic_store(&(addr->epoch->epoch_counter), current_epoch);
	return current_epoch;
}

int *get_epoch_array_index(int epoch)
{
	if (epoch == epoch_0)
		return &(addr->epoch->e0_index);
	if (epoch == epoch_1)
		return &(addr->epoch->e1_index);
	if (epoch == epoch_2)
		return &(addr->epoch->e2_index);
	/* should not be here*/
	return NULL;
}

void **get_epoch_array(int epoch)
{
	if (epoch == epoch_0) 
		return addr->epoch->e0;
	if (epoch == epoch_1) 
		return addr->epoch->e1;
	if (epoch == epoch_2) 
		return addr->epoch->e2;
	/* should not be here*/
	return NULL;
}

int get_reclaim_epoch()
{	
	int reclaim_epoch;
	int current_epoch = get_current_epoch();
	
	if (current_epoch == 0) {
		reclaim_epoch = 1;
		return reclaim_epoch;
	}
	if (current_epoch == 1) {
		reclaim_epoch = 2;
		return reclaim_epoch;
	}
	if (current_epoch == 2) {
		reclaim_epoch = 0;
		return reclaim_epoch;
	}
	/* should not be here*/
	return -1;
}

void expand_epoch_array(int epoch, int size)
{
	int new_size = 2 * size;

	if (epoch == epoch_0) {
		addr->epoch->e0 = (void **)dram_realloc(addr->epoch->e0, 
				sizeof(void *) * new_size);
		return;
	}
	if (epoch == epoch_1) { 
		addr->epoch->e1 = (void **)dram_realloc(addr->epoch->e1, 
				sizeof(void *) * new_size);
		return;
	}
	if (epoch == epoch_2) {
		addr->epoch->e2 = (void **)dram_realloc(addr->epoch->e2, 
				sizeof(void *) * new_size);
		return;
	}
	/* should not be here*/
	return;
}

int inc_epoch_index_safe(int index, int epoch)
{	
	if (unlikely(index + 1 >= FREE_LIST_SIZE)) {
		expand_epoch_array(epoch, index + 1);	
	}
	index += 1;
	return index;
}

tips_thread **get_thread_list_addr()
{
	return addr->epoch->thread_list;
}

tips_bg_thread *init_bg_thread(void *ds_ptr, bool is_load)
{	
	int ret;
	tips_bg_thread *thread;

	thread = (tips_bg_thread *)dram_alloc(sizeof(*thread));
	thread->should_stop = BG_THREAD_ON;
	thread->ds_root = ds_ptr;
	thread->ulog = addr->root->ulog;
	thread->clog_head = thread->clog_tail = -1;
	thread->count = 0;
	thread->n_iters = 0;
	thread->finish_cnt = 0;
	thread->t_entries = 0;
	thread->n_exec = 0;
	thread->fg_rate = 0;
	thread->bg_rate = 0;
	thread->is_max_cnt_reached = false;
	thread->n_workers_iter = g_n_threads/4;
	thread->workers_avg = 0;
	if (thread->n_workers_iter < N_WORKER_MIN || 
			thread->n_workers_iter > N_WORKER_MAX) {
		thread->n_workers_iter = N_WORKER_MIN;
	}
	thread->max_bg_rate = 0;
	thread->max_rate_cnt = 0;
	thread->is_load = is_load;
	thread->worker = (tips_worker *) dram_alloc
		(sizeof(tips_worker) * g_n_workers);
	for (int i = 0; i < g_n_workers; ++i) {
		thread->worker[i].clog_tail = 
			thread->worker[i].clog_head = -1;
		thread->worker[i].t_processed = 0;
		thread->worker[i].n_exec = 0;
		thread->worker[i].n_exec_iter = 0;
		thread->worker[i].rate = 0;
		thread->worker[i].ds_root = ds_ptr;
	}
	ret = pthread_create(&(thread->bg_thread), NULL,
			process_op_entries, thread);
	if (ret) {
		perror("tips_bg_thread create failed\n");
	}
	return thread;
}

void free_bg_thread(tips_bg_thread *th)
{
	dram_free(th->worker);
	dram_free(th);
	return;
}

tips_gc_thread *init_gc_thread(int n_readers)
{	
	int ret;
	tips_gc_thread *thread;

	thread = (tips_gc_thread *)dram_alloc(sizeof(*thread));
	thread->should_stop = 1;
	thread->d = addr->d;
	thread->local_epoch = 0;
	thread->n_readers = n_readers;
	ret = pthread_create(&(thread->gc_thread), NULL, 
			reclaim_ddr_cache, thread);
	if (ret) {
		perror("GC thread creation failed\n");
	}
	return thread;
}

void free_gc_thread(tips_gc_thread *gc)
{
	dram_free(gc);
	return;
}

void tips_init_threads(bool is_load)
{	
	addr->bg = init_bg_thread(addr->root->ds_addr, is_load);
	addr->gc = init_gc_thread(g_n_threads);
	return;
}

void tips_init_bg_thread(bool is_load)
{	
	addr->bg = init_bg_thread(addr->root->ds_addr, is_load);
	return;
}

void tips_update_thread(tips_thread *th) 
{
	int tid = th->id;
	
	if (tid < g_n_threads) {
		addr->load[tid] = th;
	}
	else {
		tid = th->id % g_n_threads;
		addr->workload[tid] = th;
	}
	return;
}

int tips_init(int n_threads, void * ds_ptr, 
		void *pool_root)
{
	int size;

	addr = dram_alloc(sizeof(*addr));
	addr->root = (root_obj *)pool_root;
	addr->root->ds_addr = ds_ptr;
	addr->d = ddr_cache_create();
	addr->root->ulog = ulog_create();
        addr->root->olog_head = NULL;
	addr->epoch = init_gc_attr(n_threads);
	g_n_threads = n_threads;
	g_n_workers = N_WORKER_MAX;
	size = sizeof(tips_thread) * g_n_threads;
	addr->load = dram_alloc(size);
	addr->workload = dram_alloc(size);
	return 0;
}

void add_oplog_root(olog *o)
{
	olog **oplog_head = 
		&(addr->root->olog_head);
	o->next = *oplog_head;
	while (!smp_cas(oplog_head, o->next, o)) {
		o->next = *oplog_head;
		continue;
	}
	flush_to_nvm((void *)& o->next, sizeof(o->next));
	flush_to_nvm((void *)oplog_head, sizeof(void *));
	smp_mb();
	return;
}

olog *tips_assign_oplog()
{	
	olog *o;

	o = oplog_create();
	add_oplog_root(o);
	return o;
}

int tips_deallocate_oplog(int count)
{
	ulog_commit(true);
	oplog_destroy_unused(count, 
			&addr->root->olog_head);
	return 0;
}

void prepare_reader_thread(tips_thread *th)
{	
	tips_gc *epoch;
	if (th) {
		epoch = addr->epoch;
		smp_faa(&(th->run_cnt), 1);
		th->local_epoch = get_current_epoch();
		if (!epoch->thread_list[th->id])
			smp_atomic_store(&(
			epoch->thread_list[th->id]), th);
	}
	return;
}

void reset_thread(tips_thread *th) 
{	
	if (th)
		smp_faa(&(th->run_cnt), 1);
	return;
}

char *tips_read(uint64_t target_key, ds_read *_read, 
		tips_thread *self)
{
	char *val;
	ddr_entry *target_entry;
	void *ds_root;

	prepare_reader_thread(self);
	target_entry = ddr_cache_read(addr->d, target_key);
	if (unlikely(target_entry && 
				target_entry->is_delete == F_DEL)) {
		reset_thread(self);
		return NULL;
	}
	if (target_entry)
		++self->ddr_hit;
	if (!target_entry) {
		reset_thread(self);
		ds_root = addr->root->ds_addr;
		val = _read(ds_root, target_key);
		return val;
	}
	val = (char *)target_entry->val;
	reset_thread(self);
	return val;
}

void scan_oplog(olog *head, uint64_t s_key, uint64_t e_key, 
		uint64_t *buff, unsigned long scan_clk, tips_thread *self)
{
	olog *iterator = head;
	volatile int start;
	volatile int end;
	uint64_t key;
	volatile unsigned long entry_clk;

	/* TODO: Implement backoff scheme for the oplog scan function
	 * NO scan violation is observed until 30% write and 70% scan 
	 * for ART and B+Tree*/
	while (iterator) {
		start = iterator->prev_tail;
		end = iterator->tail_index;
		while (start != end) {
			entry_clk = iterator->arr[start].enq_ts;
			if (gte_clk(entry_clk, scan_clk))
					break;
			key = iterator->arr[start].key;
			if (key > s_key && key < e_key) 
				++self->scan_violation;
			start = incr_index_safe(start);
		}
		iterator = iterator->next;
	}
//	smp_faa(&n_violation, self->scan_violation);
	return;
}

int tips_scan(uint64_t start_key, int range, uint64_t *buff, 
		tips_thread *self, ds_scan *scan) 
{
	int ret;
	volatile unsigned int scan_clk;
	olog *olog_head;
	uint64_t end_key;

	scan_clk = get_clk();
	olog_head = addr->root->olog_head;

	ret = scan(addr->bg->ds_root, start_key, 
			range, buff);
	end_key = buff[range];
	scan_oplog(olog_head, start_key, end_key, 
			buff, scan_clk, self);
	return ret;
}

int tips_insert(uint64_t key, char *val, olog *o,
		ds_insert *_insert)
{
	int ret;
	
	ret = ddr_cache_write(addr->d, key, 
			val, o, _insert);
	if (ret) {
		perror("tips_insert failed: memory unavailable\n");
		return 1;
	}
	return 0;
}

/* TODO: need to modify like tips_insert*/
int tips_delete(uint64_t key, olog *o, ds_delete 
		*_delete)
{
	int ret;
	volatile unsigned long clk;

	clk = ddr_cache_delete(addr->d, key);
	if (!clk) {
		perror("tips_delete failed: dram unavailable\n");
		return 1;
	}

	ret = oplog_insert_delete(o, key, F_DEL, 
			clk, _delete);
	if (ret) {
		perror("oplog enq failed: NVMM allocation failed \n");
		return 1;
	}
	return 0;	
}

int incr_clog_index_safe(int index)
{
	if (unlikely(index + 1 == MAX_COMB_ENTRIES)) {
		index = 0;
		return index;
	}
	index += 1;
	return index;
}

int combine_op_entry(int start, int end, 
		olog *iterator, tips_bg_thread *th) 
{	
	volatile unsigned long op_entry_clk;
	int index, n_wrk, hash;
	tips_worker *worker;
	volatile int i;

	for (i = start; i <= end; ++i) {
		op_entry_clk = smp_atomic_load(
				&iterator->arr[i].enq_ts);
		assert(op_entry_clk != 0);
		/* if the current entrys' clk is greater than
		 * start_clk just skip, we can take care off it 
		 * in the next iteration */
		if (gte_clk(op_entry_clk, th->start_ts)) {
			/* if the current entry is in future
			 * wrt bg_thread the all the entries after
			 * that will be future as well*/
			break;
		}

		/* add the entry into combiner log
		 * we just store the pointer to the entry
		 * also we store timestamp to have 
		 * faster sorting*/
		iterator->prev_tail = i;
		//get hash key to assign to worker
		hash = get_hash_key(iterator->arr[i].key);
		n_wrk = hash % th->n_workers_iter;
		worker = &th->worker[n_wrk];
		// get the clog tail of the worker
		index = incr_clog_index_safe(worker->clog_tail);
		// add the entry to the clog
		worker->clog[index].entry_ts = op_entry_clk;
		assert(worker->clog[index].entry_ts != 0);
		worker->clog[index].entry = &iterator->arr[i];
		// increment the tail
		worker->clog_tail = index;
		worker->t_processed += 1;
		iterator->n_applied += 1;
		++th->t_entries;
	}
	if (unlikely(i <= end)) {
		return 1;
	}
	else {
		return 0;
	}
}

void try_combine_op_entry(tips_bg_thread *th, 
		olog *olog_head)
{
	olog *iterator;
	volatile int prev_tail, curr_tail;
	int max_range, ret;

	iterator = olog_head;
	max_range = MAX_OP_ENTRIES - 1;
	while (iterator) {
		prev_tail = smp_atomic_load(&iterator->prev_tail);
		curr_tail = smp_atomic_load(&iterator->tail_index);
		if (curr_tail == -1) {
			iterator = iterator->next;
			++th->run_op_cnt;
			continue;
		}
		/* if the tail is unchanged since the last
		 * iteration or if the oplog is empty
		 * just skip this log*/
		if (prev_tail == curr_tail) {
			++th->run_op_cnt;
			iterator = iterator->next;
			continue;
		}
		if (iterator->need_reclaim) {
			assert(iterator->n_used >= MAX_USED);
			if (iterator->n_used >= MAX_USED) 
				th->do_reclaim = true;
			else
				smp_cas(&iterator->need_reclaim, 
						true, false);
		}
		prev_tail = incr_index_safe(prev_tail);
		if (prev_tail > curr_tail) {
			assert(prev_tail > curr_tail);
			ret = combine_op_entry(prev_tail, 
					max_range, iterator, th);
			if (ret) {
				iterator = iterator->next;
				continue;
			}
			/* wrap around*/
			prev_tail = 0;
		}
		combine_op_entry(prev_tail, curr_tail, 
					iterator, th);
		iterator = iterator->next;
	}
	return;
}

int cmp_clog_entries(const void *e1, const void *e2) 
{
	Clog_entry *entry_1, *entry_2;
	unsigned long e1_clk, e2_clk;
	int d = -2; 

	entry_1 = (Clog_entry *)e1;
	entry_2 = (Clog_entry *)e2;
	e1_clk = entry_1->entry_ts;
	e2_clk = entry_2->entry_ts;

	if (e1_clk < e2_clk) {
		return -1;
	}
	else if (e1_clk > e2_clk) {
		return 1;
	}
	else {
		if (entry_1->entry->key == 
				entry_2->entry->key) {
			d = entry_1->entry->is_delete - 
				entry_2->entry->is_delete;
		}
		return d;
	}
}

void _sort_clog_entries(tips_worker *th)
{
	qsort(th->clog, th->clog_tail + 1, sizeof(Clog_entry), 
			cmp_clog_entries);
}

void execute_clog_entry(op_entry *entry, 
		void *ds_root)
{
	int ret;
	uint64_t key;
	char *val;

	key = entry->key;
	val = entry->val;
	if (!key || !val) {
		printf("exec_key= %ld\n", key);
		printf("exec_val= %s\n", val);
	}
	if (entry->is_delete != F_DEL) {
		ret = entry->_insert(ds_root, key, val);
		if (ret > 1) 
			perror("entry not replied successfully\n");
		return;
	}
	ret = entry->_delete(ds_root, entry->key);
	if (ret) 
		perror("entry not replied successfully\n");
	return;
}


void _reclaim_clog(tips_worker *th)
{
	size_t size;
	
	th->clog_head = th->clog_tail = -1;
	size = sizeof(Clog_entry) * MAX_COMB_ENTRIES;
	memset(th->clog, 0, size);
	return;
}

void update_last_replay_clk(volatile 
		unsigned int clk)
{
	smp_atomic_store(&addr->root->last_replay_clk, clk);
	return;
}

void _replay_clog_entries(tips_worker *th, 
		int flag)
{
	int head, tail, i;
	op_entry *entry;
	uint64_t key;
	char *val;
	void *root = th->ds_root;
	ddr_cache *d = addr->d;
	
	head = th->clog_head + 1;
	tail = th->clog_tail;
	for (i = head; i <= tail; ++i) {
		entry = th->clog[i].entry;
		key = th->clog[i].entry->key;
		val = th->clog[i].entry->val;
		execute_clog_entry(entry, root);
		th->n_exec += 1;
		th->n_exec_iter += 1;
		if (!flag) 
			update_ddr_entry_status(d, key, val);
		th->clog_head = i;
	}
	return;
}

void try_reclaim_oplog(tips_bg_thread *th, 
		olog *olog_head)
{
	olog *iterator = olog_head;
	int ret;
	
	ret = ulog_commit(false);
	if (ret) 
		perror("ulog commit failed");
	while (iterator) {
		if (iterator->tail_index == -1) {
			iterator = iterator->next;
			continue;
		}
		reclaim_oplog(iterator);
		iterator = iterator->next;
	}
	commit_free_list();
	assert(th->do_reclaim == true);
	th->do_reclaim = false;
	return;
}

void calculate_rate(tips_worker *worker) 
{
	double time_sec, time_nsec;

	time_nsec = 10e9 * (worker->end.tv_sec - worker->start.tv_sec) 
		+ worker->end.tv_nsec - worker->start.tv_nsec;
	time_sec = time_nsec/10e9;
	worker->rate = worker->n_exec_iter/time_sec;
	return;
}

void *worker(void *arg)
{
	tips_worker *worker = (tips_worker *) arg;

	// check if the clog is not empty
	if (worker->clog_tail == -1)
		return NULL;

	// update the start time for the worker
	clock_gettime(CLOCK_MONOTONIC, &worker->start);

	// if clog is not empty then sort the clog
	_sort_clog_entries(worker);	

	//replay the sorted clog
	_replay_clog_entries(worker, 0);

	// update the end time for the worker
	clock_gettime(CLOCK_MONOTONIC, &worker->end);

	// calculate the rate of processing for the worker
	if (worker->need_rate)
		calculate_rate(worker);

	// reclaim the clog
	_reclaim_clog(worker);

	return NULL;
}

void calculate_fg_rate(tips_bg_thread *th)
{
	tips_thread **fgt;
	struct timespec fg_start;
	struct timespec fg_end;
	double time_nsec, time_sec;
	unsigned int fg_writes;
	double rate, rate_sum = 0;

	// get the correct array of main threads
	if (th->is_load) 
		fgt = addr->load;
	else
		fgt = addr->workload;

	if (!fgt) {
		perror("FG information missing\n");
	}
	
	clock_gettime(CLOCK_MONOTONIC, &fg_end);

	// iterate the array of fg threads to get the n_writes and start time to
	// calulate the processing rate
	for (int i = 0; i < g_n_threads; ++i) {
		fg_start = fgt[i]->stats->start;
		fg_writes = smp_atomic_load(&fgt[i]->stats->n_writes);
		smp_atomic_store(&fgt[i]->stats->epoch, true);
		time_nsec = 10e9 * (fg_end.tv_sec - fg_start.tv_sec) 
			+ fg_end.tv_nsec - fg_start.tv_nsec;
		time_sec = time_nsec/10e9;
		rate = fg_writes/time_sec;
		rate_sum += rate;
	}
	th->fg_rate = rate_sum/g_n_threads;
	return;
}

void calculate_bg_rate(tips_bg_thread *th)
{
	double rate, rate_sum = 0;

	for (int i = 0; i < th->n_workers_iter; ++i) {
		rate = th->worker[i].rate;
//		printf("rate for worker %d: %f\n", i, rate);
		rate_sum += rate;
		th->worker[i].rate = th->worker[i].n_exec_iter = 0;
	}
	th->bg_rate = rate_sum/th->n_workers_iter;
	return;
}

void finalize_n_workers(tips_bg_thread *th) 
{
	int n_workers;
	double rate_diff;
	int max_workers = N_WORKER_MAX;
	int min_workers = N_WORKER_MIN;
	

	if (th->bg_rate <= 0) {
		th->n_workers_iter = max_workers;
		return;
	}

	rate_diff = th->fg_rate/th->bg_rate;
	
	// it means fg is faster than bg
	if (rate_diff >= 1) {
		//check if curr_bg_rate > prev_max_bg_rate
		//yes then 1. update the prev_max_tput and prev_max_count
		//2. check if max workers have reached 
		//if yes set n_workers_iter to curr_count
		//else increase the thread count
		if (th->bg_rate > th->max_bg_rate) {
			th->max_bg_rate = th->bg_rate;
			th->max_rate_cnt = th->n_workers_iter;
			if (th->is_max_cnt_reached) 
				return;
			else {
				n_workers = th->n_workers_iter * 2;
				if (n_workers >= th->n_workers_iter 
						+ N_WORKER_STEP) {
					th->n_workers_iter += N_WORKER_STEP; 
				}
				else {
					th->n_workers_iter = n_workers;
				}
				if (th->n_workers_iter >= max_workers) {
					th->n_workers_iter = max_workers;
					th->is_max_cnt_reached = true;
				}		
				return;
			}
		}
		/* fall back to max if more 60% of 
		 * upper cap is reached*/
		if (th->is_max_cnt_reached || th->n_workers_iter >= 0.60 * N_WORKER_MAX) {
			th->n_workers_iter = th->max_rate_cnt;
			return;
		}
		else {
			/* if max_rate_cnt is 2x the current thread count then 
			fall back to max_rate_cnt*/
			n_workers = th->n_workers_iter * 2;
			if (n_workers >= th->n_workers_iter 
					+ N_WORKER_STEP) {
				th->n_workers_iter += N_WORKER_STEP; 
			}
			else {
				th->n_workers_iter = n_workers;
			}
			if (th->n_workers_iter >= max_workers) {
				th->n_workers_iter = max_workers;
				th->is_max_cnt_reached = true;
			}		
			return;
		}
	}
	// bg on par with fg writers
	else if (rate_diff < 1 && rate_diff >= 0.4) {
		if (th->bg_rate > th->max_bg_rate) {
			th->max_bg_rate = th->bg_rate;
			th->max_rate_cnt = th->n_workers_iter;
		}
		return;
	}
	// bg faster than fg writers
	else if (rate_diff < 0.4){
		th->n_workers_iter /= 2;
		if (th->n_workers_iter <= min_workers) {
			th->n_workers_iter = min_workers;
			return;
		}
		return;
	}
	/* never be here */
	return;
}

void create_workers(tips_bg_thread *th, bool need_rate) 
{
	int ret;
	
	for (int i = 0; i < th->n_workers_iter; ++i) {
		th->worker[i].need_rate = need_rate;
		ret = pthread_create(&th->worker[i].wrk_thread, NULL, 
				worker, &th->worker[i]);
		if (ret) {
			perror("worker thread create failed\n");
		}
	}
	th->workers_avg += th->n_workers_iter;	
	// calculate the processing rate for fg writers
	if (need_rate)
		calculate_fg_rate(th);
	
	for (int i = 0; i < th->n_workers_iter; ++i) {
		pthread_join(th->worker[i].wrk_thread, NULL);
	}
	if (!need_rate) {
		++th->n_iters;
		return;
	}
	// calculate the processing rate for workers 
	calculate_bg_rate(th);

	// finilize the #workers for the next iteration
	finalize_n_workers(th);
	++th->n_iters;
	return;
}

void *finish_bg_replay(tips_bg_thread *th, bool last_run)
{
	olog *olog_head;
	volatile bool finish = false;
	bool need_rate = false;
	volatile unsigned int run_op_cnt = 0;

	th->run_op_cnt = 0;
	if (!last_run)
		run_op_cnt = 2 * g_n_threads;
	else
		run_op_cnt = g_n_threads;
	while(!finish) {
		olog_head = smp_atomic_load
			(&(addr->root->olog_head));
		if (unlikely(!olog_head)) 
			return NULL;
		th->start_ts = get_clk();
		th->status = BG_COMBINE;
		th->need_replay = false;
		th->n_workers_iter = g_n_workers;
		try_combine_op_entry(th, olog_head);

		for (int i = 0; i < g_n_workers; ++i) {
			if (th->worker[i].clog_tail == -1)
				continue;
			if (!th->need_replay) { 
				th->need_replay = true;
				break;
			}
		}
		if (th->need_replay) {
			create_workers(th, need_rate);
		}
		if (th->run_op_cnt == run_op_cnt) 
			finish = true;
		th->run_op_cnt = 0;
	}
	return NULL;
}

void *process_op_entries(void *arg)
{
	tips_bg_thread *th = (tips_bg_thread *)arg;
	olog *olog_head;
	volatile unsigned long clk;
	bool need_rate = true;

Retry:
	while (th->should_stop != BG_THREAD_OFF) {
		olog_head = smp_atomic_load
			(&(addr->root->olog_head));
		if (unlikely(!olog_head)) { 
			goto Retry;
		}
		clk = get_clk();
		th->start_ts = clk;
		th->status = BG_COMBINE;
		th->need_replay = false; 
		try_combine_op_entry(th, olog_head);
		th->run_op_cnt = 0;
		for (int i = 0; i < th->n_workers_iter; ++i) {
			if (th->worker[i].clog_tail == -1)
				continue;
			if (!th->need_replay) {
				th->need_replay = true;
				break;
			}
		}
		if (th->need_replay) {
			create_workers(th, need_rate);
		}
		if (th->ulog->need_reclaim == true && 
				th->do_reclaim == false) {
			th->do_reclaim = true;
			smp_atomic_store(&th->status, 
					BG_RECLAIM);
		}
		if (th->do_reclaim) {
			smp_atomic_store(&th->status, 
					BG_RECLAIM);
			try_reclaim_oplog(th, olog_head);
		}
		goto Retry;
	}
	return NULL;
}

int tips_finish_load()
{
	tips_bg_thread *th = addr->bg;

	smp_cas(&(addr->bg->should_stop), BG_THREAD_ON, BG_THREAD_OFF);
	pthread_join(th->bg_thread, NULL);
	printf("TIPS finishing up Load...\n");
	finish_bg_replay(th, false);
	for (int i = 0; i < g_n_workers; ++i) {
		tw_processed += th->worker[i].t_processed;
		tw_executed += th->worker[i].n_exec;
	}
	g_avg_workers = th->workers_avg/th->n_iters;
	free_bg_thread(th);
	return 0;
}

void tips_print_stats(tips_bg_thread *th)
{
#ifdef PRINT_STATS
	int ulog_size = return_ulog_size();
	int olog_size = return_olog_size();
	int ddr_size = return_ddr_size();

	printf("=============== TIPS STATS ===================\n");
	printf("ulog_size= %d\n", ulog_size);
	printf("olog_size= %d\n", olog_size);
	printf("cache_size= %d\n", ddr_size);
	printf("#workers= %d\n", g_n_workers);
	printf("TIPS worker executed %d entries in the LOAD stage\n", tw_executed);
	printf("TIPS worker executed %d entries in the Workload stage\n", tw_executed_w);
	printf("Avg workers spanwed in LOAD stage %d\n", g_avg_workers);
	if (th->n_iters)
		printf("Avg workers spanwed in Workload stage %d\n", th->workers_avg/th->n_iters);
	else
		printf("Avg workers spanwed in Workload stage %d\n", th->workers_avg);
	print_ulog_stats();
	printf("===============================================\n");
#endif
	return;
}

int tips_dinit()
{	
	tips_bg_thread *th = addr->bg;

	printf("TIPS dinit...\n");
	smp_cas(&(addr->bg->should_stop), BG_THREAD_ON, BG_THREAD_OFF);
	pthread_join(addr->bg->bg_thread, NULL);
	finish_bg_replay(addr->bg, true);
	smp_atomic_store(&(addr->gc->should_stop), 0);
	pthread_join(addr->gc->gc_thread, NULL);
	for (int i = 0; i < g_n_workers; ++i) {
		tw_processed_w += th->worker[i].t_processed;
		tw_executed_w += th->worker[i].n_exec;
	}
	tips_print_stats(addr->bg);
	free_bg_thread(addr->bg);
	printf("Perist Thread terminated...\n");
	free_gc_thread(addr->gc);
	destroy_gc_attr(addr->epoch);
	printf("GC thread terminated...\n");
	ddr_cache_destroy(addr->d);
	printf("Dram Cache destroyed...\n");
	ulog_destroy();
	printf("Undolog destroyed...\n");
	addr->root->ulog = NULL;
	oplog_destroy_safe(&addr->root->olog_head);
	printf("Oplog destroyed...\n");
	destroy_nvmm_pool();
	dram_free(addr);
	printf("TIPS terminated safely...\n");
	return 0;
}


