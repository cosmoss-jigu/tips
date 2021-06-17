#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include "numa-config.h"
#include "ds.h"
#include "rand-str.h"
#include "bench.h"

static volatile int should_stop;
static cpu_set_t cpu_set[112];
pthread_barrier_t thread_barrier;
#ifdef IS_TIPS
fn_read *_g_read;
fn_insert *_g_insert;
//fn_scan *_g_scan;
fn_delete *_g_delete;
#endif

int get_ops(unsigned int seed) 
{
	int ops;

	ops = rand_r(&seed);
	ops %= MAX_UPDATE_RATIO;
	return ops;	
}

int get_cpu_id(int index, bool reset)
{
	static int curr_socket = 0;
	static int curr_phy_cpu = 0;
	static int curr_smt = 0;
	int ret;

	if (reset) {
		curr_socket = 0;
		curr_phy_cpu = 0;
		curr_smt = 0;
		return 1;
	}
	ret = OS_CPU_ID[curr_socket][curr_phy_cpu][curr_smt];
	++curr_phy_cpu;
	if (curr_phy_cpu == NUM_PHYSICAL_CPU_PER_SOCKET) {
		curr_phy_cpu = 0;
		++curr_socket;
		if (curr_socket == NUM_SOCKET) {
			curr_socket = 0;
			++curr_smt;
			if (curr_smt == SMT_LEVEL)
				curr_smt = 0;
		}
	}
	return ret;
}

void *bench(void *data)
{	
	unsigned int op;
	int ret_val; //cmp;
	char *ret;
	thread_data *d = (thread_data *)data;

	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set[d->tid]);
	pthread_barrier_wait(&thread_barrier);
	printf("barrier crossed\n");
	while (should_stop != BENCH_STOP) {
		op = get_ops(d->ops_seed);
		d->ops_seed += d->incr_seed;
		if (op < d->update) {
			d->self->key = get_key(d->key_seed);
			get_val(d->self->val, d->val_size, 
					d->val_seed);
			d->key_seed += d->incr_seed;
			d->val_seed += d->incr_seed;
#ifdef IS_TIPS
			ret_val = tips_insert(d->self->key, d->self->val, 
					d->self->o, _g_insert);
			if (ret_val) 
				perror("tips insert failed\n");
			++d->n_create;
#else
			ret_val = pmdk_insert(d->self->key, d->self->val);
			if (!ret_val)
				++d->n_create;
			else
				++d->n_update;
#endif
		}
//		else if (op % 5 != 0) {
		else {
			d->self->key = get_key(d->key_seed);	
			d->key_seed += d->incr_seed;
#ifdef IS_TIPS
			ret = tips_read(d->self->key, _g_read, d->self);
			if (!ret)
				++d->n_read_null;
			else
				++d->n_read_key;
#else
			ret = pmdk_read(d->self->key); 
			if (!ret)
				++d->n_read_null;
			else
				++d->n_read_key;
#endif
		}
	}

	return NULL;
}

int main()
{
	const char type[] = DEFAULT_DS_TYPE;
	int bench_type = DEFAULT_BENCH_TYPE;
	int duration = DEFAULT_DURATION;
	unsigned int initial = 0;
	unsigned int n_threads = DEFAULT_N_THREADS;
	unsigned int range = DEFAULT_RANGE;
	unsigned int update = DEFAULT_UPDATE_RATIO;
	unsigned int op_cnt = DEFAULT_OP_COUNT;
	unsigned int key_size = DEFAULT_KEY_SIZE;
	unsigned int val_size = DEFAULT_VAL_SIZE;
	unsigned int seed = DEFAULT_MAIN_SEED;
	struct timespec end_time;
	struct timeval bench_start, bench_end;
	pthread_t *threads;
	pthread_attr_t attr;
	thread_data *data;
	int cpu_id;
#ifdef IS_TIPS
	void *ds_ptr, *root;
#endif
	int ret;
	unsigned int i;
	unsigned long reads = 0;
	unsigned long scans = 0;
	unsigned long inserts = 0;
	unsigned long deletes = 0;
	unsigned long total_ops = 0;
	unsigned long total_updates = 0;
	unsigned long total_reads = 0;
	unsigned long total_scans = 0;
	unsigned long total_inserts = 0;
	unsigned long total_deletes = 0;
	double throughput = 0;
	double read_throughput = 0;
	double update_throughput = 0;


	if(!bench_type) {
		printf("***TIPS BENCH - TIME BASED RUN***\n");
		printf("bench duration   : %d\n", duration);
	}
	else {
		printf("***TIPS BENCH - COUNT BASED RUN***\n");
		printf("OP count : %d\n", op_cnt);
	}
	printf("data-structure   : %s\n", type);
	printf("initial set size : %d\n", initial);
	printf("no of threads    : %d\n", n_threads);
	printf("str range        : %d\n", range);
	printf("update ratio     : %d\n", update);
	printf("key size         : %d\n", key_size);
	printf("val size         : %d\n", val_size);
	printf("seed             : %d\n", seed);
	
	end_time.tv_sec = duration / 1000;
	end_time.tv_nsec = (duration % 1000) * 1000000;
	data = (thread_data *) malloc(n_threads * sizeof(*data));
	if (!data) {
		perror("malloc failed");
		exit(1);
	}
	memset(data, 0, n_threads * sizeof(*data));
	threads = (pthread_t *)malloc(n_threads * 
			sizeof(*threads));
	if (!threads) {
		perror("malloc failed\n");
		exit(1);
	}
	srand(seed);
#ifdef IS_TIPS
	root = init_nvmm_pool();
	if (!root) {
		perror("init nvmm pool failed");
		exit(1);
	}
	ds_ptr = set_ds_type(type);
	if (!ds_ptr) {
		perror("init data structure failed");
		exit(1);
	}
#else
	ret = set_pmdk_ds(type);
	if (ret) {
		perror("pmdk ds set failed");
		exit(0);
	}
#endif
#ifdef IS_TIPS
	ret = tips_init(n_threads, ds_ptr, root);
	if (ret) {
		perror("tips_init_failed");
		exit(1);
	}
	_g_read = get_read_fn_ptr();
//	_g_scan = get_scan_fn_ptr();
	_g_insert = get_insert_fn_ptr();
	_g_delete = get_delete_fn_ptr();
#endif
	/*TODO: check for range val*/
	/* TODO: insert initial size*/
	pthread_barrier_init(&thread_barrier, NULL, 
			n_threads + 1);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, 
			PTHREAD_CREATE_JOINABLE);

	/* Thread pinning*/
	CPU_ZERO(&cpu_set[0]);
	for (i = 0; i < n_threads; ++i) {
		cpu_id = get_cpu_id(i, 0);
		CPU_SET(cpu_id, &cpu_set[0]);
	}
	get_cpu_id(0, 1);
	for (i = 0; i < n_threads; ++i) {
		printf("creating thread %d\n", i);
		cpu_id = get_cpu_id(i, 0);
		CPU_ZERO(&cpu_set[i]);
		CPU_SET(cpu_id, &cpu_set[i]);
		data[i].tid = i;
		data[i].range = range;
		data[i].op_cnt = op_cnt;
		data[i].update = update;
		data[i].key_seed = rand() % 50;
		data[i].val_seed = rand() % 500;
		data[i].ops_seed = rand() % 10;
		data[i].incr_seed = i + 1;
		data[i].key_size = key_size;
		data[i].val_size = val_size;
		data[i].n_create= 0;
		data[i].n_update = 0;
		data[i].n_delete_key = 0;
		data[i].n_delete_null = 0;
		data[i].n_read_key = 0;
		data[i].n_read_null = 0;
		data[i].key1 = (char *)malloc(key_size);
		data[i].key2 = (char *)malloc(key_size);
		data[i].key3 = (char *)malloc(key_size);
		data[i].self = (tips_thread *)malloc(sizeof(
					tips_thread));
		memset(data[i].self, 0, sizeof(tips_thread));
#ifdef IS_TIPS
		data[i].self->id = data[i].tid;
#endif
		data[i].self->val = (char *)malloc(val_size);
		ret = pthread_create(&threads[i], &attr, bench, 
				(void *)(&data[i]));
		if (ret) {
			perror("thread create failed");
			exit(1);
		}
	}
	should_stop = BENCH_START;
	pthread_attr_destroy(&attr);
#ifdef IS_TIPS
	/* TODO: pin bg_thread to a core?*/
	tips_init_threads(true);
#endif
	pthread_barrier_wait(&thread_barrier);
	printf("STARTING BENCH EXECUTION.....\n");
	gettimeofday(&bench_start, NULL);
	if (!bench_type) {
		nanosleep(&end_time, NULL);
	}
	else {
		/* TODO: implement sleep for op_count based run*/
	}
	should_stop = BENCH_STOP;
	gettimeofday(&bench_end, NULL);
	printf("STOPPING BENCH EXECUTION.....\n");
	for (i = 0; i < n_threads; ++i) {
		printf("waiting for thread %d to complete\n", i);
		ret = pthread_join(threads[i], NULL);
		if (ret) {
			perror("error waiting for threads");

		}
	}
	for (i = 0; i < n_threads; ++i) {
		free(data[i].self);
	}
	duration = (bench_end.tv_sec * 1000 + bench_end.tv_usec / 1000) - 
		(bench_start.tv_sec * 1000 + bench_start.tv_usec / 1000 );
	for (i = 0; i < n_threads; ++i) {
/*		printf("thread-%d\n", i);
		printf("#read_key    : %ld\n", data[i].n_read_key);
		printf("#read_null   : %ld\n", data[i].n_read_null);
		printf("#scan_key    : %ld\n", data[i].n_scan_key);
		printf("#scan_null   : %ld\n", data[i].n_scan_null);
		printf("#create      : %ld\n", data[i].n_create);
		printf("#update      : %ld\n", data[i].n_update);
		printf("#delete_key  : %ld\n", data[i].n_delete_key);
		printf("#delete_null : %ld\n", data[i].n_delete_null);*/
		reads = data[i].n_read_key + data[i].n_read_null;
		scans = data[i].n_scan_key + data[i].n_scan_null;
		inserts = data[i].n_create + data[i].n_update;
		deletes = data[i].n_delete_key + data[i].n_delete_null;
/*		printf("#reads       : %ld\t", reads);
		printf("#scans       : %ld\t", scans);
		printf("#inserts     : %ld\t", inserts);
		printf("#deletes     : %ld\n", deletes);*/
		total_reads += reads;
		total_scans += scans;
		total_inserts += inserts;
		total_deletes += deletes;
		total_updates += total_inserts + total_deletes;
		total_ops += total_reads + total_updates;
	}
	throughput = (total_ops * 1000)/duration;
	read_throughput = ((total_reads + total_scans) * 1000)/duration;
	update_throughput = (total_updates * 1000)/duration;
	printf("Duration          : %d\n", duration);
	printf("#Throughput       : %0.2f\n", throughput);
	printf("#Read-Throughput  : %0.2f\n", read_throughput);
	printf("#Update-Throughput : %0.2f\n", update_throughput);
	printf("#Total-scans: %ld\n", total_scans);
#ifdef IS_TIPS
	tips_dinit();
#else
	destroy_pmdk_pool();
#endif
	free(threads);
	free(data);
	return 0;
}
