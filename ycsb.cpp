#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "tbb/tbb.h"
#include <atomic>
#include <thread>
#include <sched.h>
#include <string.h>
#include <time.h>
using namespace std;

#include "ds.h"

char ds[15];

// index types
enum {
    TYPE_ART,
    TYPE_TIPS,
    TYPE_LF,
};

enum {
    OP_INSERT,
    OP_READ,
    OP_SCAN,
    OP_DELETE,
};

enum {
    WORKLOAD_A,
    WORKLOAD_B,
    WORKLOAD_C,
    WORKLOAD_D,
    WORKLOAD_E,
};

enum {
    RANDINT_KEY,
    STRING_KEY,
};

enum {
    UNIFORM,
    ZIPFIAN,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//	thread pinning functions
//
////////////////////////////////////////////////////////////////////////////////////////////////////
cpu_set_t cpu_set[64];
enum {
    NUM_SOCKET = 2,
    NUM_PHYSICAL_CPU_PER_SOCKET = 16,
    SMT_LEVEL = 2,
};

const int OS_CPU_ID[NUM_SOCKET][NUM_PHYSICAL_CPU_PER_SOCKET][SMT_LEVEL] = {
    { /* socket id: 0 */
        { /* physical cpu id: 0 */
          0, 32,     },
        { /* physical cpu id: 1 */
          1, 33,     },
        { /* physical cpu id: 2 */
          2, 34,     },
        { /* physical cpu id: 3 */
          3, 35,     },
        { /* physical cpu id: 4 */
          4, 36,     },
        { /* physical cpu id: 5 */
          5, 37,     },
        { /* physical cpu id: 6 */
          6, 38,     },
        { /* physical cpu id: 7 */
          7, 39,     },
        { /* physical cpu id: 8 */
          8, 40,     },
        { /* physical cpu id: 9 */
          9, 41,     },
        { /* physical cpu id: 10 */
          10, 42,     },
        { /* physical cpu id: 11 */
          11, 43,     },
        { /* physical cpu id: 12 */
          12, 44,     },
        { /* physical cpu id: 13 */
          13, 45,     },
        { /* physical cpu id: 14 */
          14, 46,     },
        { /* physical cpu id: 15 */
          15, 47,     },
    },
    { /* socket id: 1 */
        { /* physical cpu id: 0 */
          16, 48,     },
        { /* physical cpu id: 1 */
          17, 49,     },
        { /* physical cpu id: 2 */
          18, 50,     },
        { /* physical cpu id: 3 */
          19, 51,     },
        { /* physical cpu id: 4 */
          20, 52,     },
        { /* physical cpu id: 5 */
          21, 53,     },
        { /* physical cpu id: 6 */
          22, 54,     },
        { /* physical cpu id: 7 */
          23, 55,     },
        { /* physical cpu id: 8 */
          24, 56,     },
        { /* physical cpu id: 9 */
          25, 57,     },
        { /* physical cpu id: 10 */
          26, 58,     },
        { /* physical cpu id: 11 */
          27, 59,     },
        { /* physical cpu id: 12 */
          28, 60,     },
        { /* physical cpu id: 13 */
          29, 61,     },
        { /* physical cpu id: 14 */
          30, 62,     },
        { /* physical cpu id: 15 */
          31, 63,     },
    },
};

int get_cpu_id()
{
	static int curr_sock = 0;
	static int curr_phy_cpu = 0;
	static int curr_smt = 0;
	int ret;

	ret = OS_CPU_ID[curr_sock][curr_phy_cpu][curr_smt];
	++curr_phy_cpu;
	if (curr_phy_cpu == NUM_PHYSICAL_CPU_PER_SOCKET) {
		curr_phy_cpu = 0;
		++curr_smt;
	}
	if (curr_smt == SMT_LEVEL) {
		++curr_sock;
		curr_smt = 0; 
		if (curr_sock == NUM_SOCKET) {
			curr_sock = 0;
		}
	}
	return ret;
}


/////////////////////////////////////////////////////////////////////////////////
static uint64_t LOAD_SIZE = 32000000;
static uint64_t RUN_SIZE = 32000000;

#ifdef STR_KEY
void ycsb_load_run_string(int index_type, int wl, int kt, int ap, int num_thread,
        std::vector<Key *> &init_keys,
        std::vector<Key *> &keys,
        std::vector<int> &ranges,
        std::vector<int> &ops)
{
    std::string init_file;
    std::string txn_file;

    if (ap == UNIFORM) {
        if (kt == STRING_KEY && wl == WORKLOAD_A) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloada";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloada";
        } else if (kt == STRING_KEY && wl == WORKLOAD_B) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloadb";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloadb";
        } else if (kt == STRING_KEY && wl == WORKLOAD_C) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloadc";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloadc";
        } else if (kt == STRING_KEY && wl == WORKLOAD_D) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloadd";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloadd";
        } else if (kt == STRING_KEY && wl == WORKLOAD_E) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloade";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloade";
        }
    } else {
        if (kt == STRING_KEY && wl == WORKLOAD_A) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloada";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloada";
        } else if (kt == STRING_KEY && wl == WORKLOAD_B) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloadb";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloadb";
        } else if (kt == STRING_KEY && wl == WORKLOAD_C) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloadc";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloadc";
        } else if (kt == STRING_KEY && wl == WORKLOAD_D) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloadd";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloadd";
        } else if (kt == STRING_KEY && wl == WORKLOAD_E) {
            init_file = "./index-microbench/workloads/ycsbkey_load_workloade";
            txn_file = "./index-microbench/workloads/ycsbkey_run_workloade";
        }
    }


    std::ifstream infile_load(init_file);

    std::string op;
    std::string key;
    int range;

    std::string insert("INSERT");
    std::string read("READ");
    std::string scan("SCAN");
    std::string maxKey("z");

    int count = 0;
    uint64_t val;
    while ((count < LOAD_SIZE) && infile_load.good()) {
        infile_load >> op >> key;
        if (op.compare(insert) != 0) {
            std::cout << "READING LOAD FILE FAIL!\n";
            return ;
        }
        val = std::stoul(key.substr(4, key.size()));
        init_keys.push_back(init_keys[count]->make_leaf((char *)key.c_str(), key.size()+1, val));
        count++;
    }

    fprintf(stderr, "Loaded %d keys\n", count);

    std::ifstream infile_txn(txn_file);

    count = 0;
    while ((count < RUN_SIZE) && infile_txn.good()) {
        infile_txn >> op >> key;
        if (op.compare(insert) == 0) {
            ops.push_back(OP_INSERT);
            val = std::stoul(key.substr(4, key.size()));
            keys.push_back(keys[count]->make_leaf((char *)key.c_str(), key.size()+1, val));
            ranges.push_back(1);
        } else if (op.compare(read) == 0) {
            ops.push_back(OP_READ);
            val = std::stoul(key.substr(4, key.size()));
            keys.push_back(keys[count]->make_leaf((char *)key.c_str(), key.size()+1, val));
            ranges.push_back(1);
        } else if (op.compare(scan) == 0) {
            infile_txn >> range;
            ops.push_back(OP_SCAN);
            keys.push_back(keys[count]->make_leaf((char *)key.c_str(), key.size()+1, 0));
            ranges.push_back(range);
        } else {
            std::cout << "UNRECOGNIZED CMD!\n";
            return;
        }
        count++;
    }

    if (index_type == TYPE_ART) {
    } else if (index_type == TYPE_TIPS) {
        void* root = init_nvmm_pool();
        if (!root) {
                perror("init nvmm pool failed");
                exit(1);
        }
	/* change the data structure to be evaluated 
	 * TIPS-hashtable -> "hlist"
	 * TIPS-lf-hashtable -> "lfhlist"
	 * TIPS-bst -> "bst"
	 * TIPS-lfbst -> "lfbst"
	 * b+tree -> "btree"
	 * TIPS-ART -> "art" 
	 * TIPS-CLHT-> "clht" */
        void* ds_ptr = set_ds_type(ds);
        if (!ds_ptr) {
                perror("init data structure failed");
                exit(1);
        }
        int ret = tips_init(num_thread, ds_ptr, root);
        fn_read_str *read_fn_ptr = get_str_read_fn_ptr();
        fn_insert_str *insert_fn_ptr = get_str_insert_fn_ptr();
        fn_scan_str *scan_fn_ptr = get_str_scan_fn_ptr();
        std::atomic<int> next_thread_id;
        vector<tips_thread *> selfArray;
        for (int i = 0; i < 2 * num_thread; i++) {
                tips_thread* self = (tips_thread*) malloc(sizeof(
                                        tips_thread));
                memset(self, 0, sizeof(tips_thread));
                self->id = i;
		self->o = tips_assign_oplog();
                selfArray.push_back(self);
        }
        tips_init_threads(true);
        {
            // Load
            auto starttime = std::chrono::system_clock::now();
            next_thread_id.store(0);
            auto func = [&]() {
                int thread_id = next_thread_id.fetch_add(1);
                tips_thread *self = selfArray[thread_id];
                
                uint64_t start_key = LOAD_SIZE / num_thread * (uint64_t)thread_id;
                uint64_t end_key = start_key + LOAD_SIZE / num_thread;

                for (uint64_t i = start_key; i < end_key; i++) {
                 //       printf("init_key: %ld, %ld\n", i, init_keys[i]);
                    int ret_val = tips_insert_str((char *)init_keys[i]->fkey, 
                                    (char *) &init_keys[i]->value, self->o, insert_fn_ptr);
                    if (!ret_val);
                }
		free(self);
            };
	    int ret;
            std::vector<std::thread> thread_group;

            for (int i = 0; i < num_thread; i++)
                thread_group.push_back(std::thread{func});

            for (int i = 0; i < num_thread; i++)
                thread_group[i].join();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            printf("Throughput: load, %f ,ops/us\n", (LOAD_SIZE * 1.0) / duration.count());
	    /* wait for pt to finish*/
	    tips_finish_load();
	    /* destroy all the load oplog*/
	    ret = tips_deallocate_oplog(num_thread);
	    if (ret)
		perror("oplog deallaoc failed\n");
	    /* init bg thread before starting RUN*/
	    tips_init_bg_thread(false);
        }	
        {
	    double t_ddr_hits = 0;
            auto starttime = std::chrono::system_clock::now();
            next_thread_id.store(0);
            auto func = [&]() {
                int thread_id = next_thread_id.fetch_add(1);
	//	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set[thread_id]);
                uint64_t start_key = RUN_SIZE / num_thread * (uint64_t)thread_id;
                uint64_t end_key = start_key + RUN_SIZE / num_thread;
                tips_thread* self = selfArray[thread_id + num_thread];
                for (uint64_t i = start_key; i < end_key; i++) {
                    if (ops[i] == OP_INSERT) {
                        int ret_val = tips_insert_str((char *)keys[i]->fkey, 
                                    (char *) &keys[i]->value, self->o, insert_fn_ptr);
                    } else if (ops[i] == OP_READ) {
                        char* ret = tips_read_str((char *)keys[i]->fkey, read_fn_ptr, self);
               //         if (ret != NULL && strcmp(ret, (char *)keys[i]->value)) {
                //            std::cout << "[TIPS] wrong key read: " << ret << " expected:" << keys[i] << std::endl;
                 //       }

                    } else if (ops[i] == OP_SCAN) {
			    char **retKeys = (char **)malloc(sizeof(uint64_t) * 1000);
                            tips_scan_str((char *)keys[i]->fkey, ranges[i], retKeys, self, scan_fn_ptr);
			    free(retKeys);
                    }
                }
		t_ddr_hits += self->ddr_hit;
                free(self);
            };

            std::vector<std::thread> thread_group;

            for (int i = 0; i < num_thread; i++)
                thread_group.push_back(std::thread{func});

            for (int i = 0; i < num_thread; i++)
                thread_group[i].join();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            printf("Throughput: run, %f ,ops/us\n", (RUN_SIZE * 1.0) / duration.count());
        }
        tips_dinit();
    }
}
#endif 

void ycsb_load_run_randint(int index_type, int wl, int kt, int ap, int num_thread,
        std::vector<uint64_t> &init_keys,
        std::vector<uint64_t> &keys,
        std::vector<int> &ranges,
        std::vector<int> &ops)
{
    std::string init_file;
    std::string txn_file;

    if (ap == UNIFORM) {
        if (kt == RANDINT_KEY && wl == WORKLOAD_A) {
            init_file = "./index-microbench/workloads/loada_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsa_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_B) {
            init_file = "./index-microbench/workloads/loadb_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsb_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_C) {
            init_file = "./index-microbench/workloads/loadc_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsc_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_D) {
            init_file = "./index-microbench/workloads/loadd_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsd_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_E) {
            init_file = "./index-microbench/workloads/loade_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnse_unif_int.dat";
        }
    } else {
        if (kt == RANDINT_KEY && wl == WORKLOAD_A) {
            init_file = "./index-microbench/workloads/loada_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsa_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_B) {
            init_file = "./index-microbench/workloads/loadb_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsb_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_C) {
            init_file = "./index-microbench/workloads/loadc_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsc_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_D) {
            init_file = "./index-microbench/workloads/loadd_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnsd_unif_int.dat";
        } else if (kt == RANDINT_KEY && wl == WORKLOAD_E) {
            init_file = "./index-microbench/workloads/loade_unif_int.dat";
            txn_file = "./index-microbench/workloads/txnse_unif_int.dat";
        }
    }

    std::ifstream infile_load(init_file);

    std::string op;
    uint64_t key;
    int range;

    std::string insert("INSERT");
    std::string read("READ");
    std::string scan("SCAN");

    int count = 0;
    while ((count < LOAD_SIZE) && infile_load.good()) {
        infile_load >> op >> key;
        if (op.compare(insert) != 0) {
            std::cout << "READING LOAD FILE FAIL!\n";
            return ;
        }
        init_keys.push_back(key);
        count++;
    }

    fprintf(stderr, "Loaded %d keys\n", count);

    std::ifstream infile_txn(txn_file);

    count = 0;
    while ((count < RUN_SIZE) && infile_txn.good()) {
        infile_txn >> op >> key;
        if (op.compare(insert) == 0) {
            ops.push_back(OP_INSERT);
            keys.push_back(key);
            ranges.push_back(1);
        } else if (op.compare(read) == 0) {
            ops.push_back(OP_READ);
            keys.push_back(key);
            ranges.push_back(1);
        } else if (op.compare(scan) == 0) {
            infile_txn >> range;
            ops.push_back(OP_SCAN);
            keys.push_back(key);
            ranges.push_back(range);
        } else {
            std::cout << "UNRECOGNIZED CMD!\n";
            return;
        }
        count++;
    }

    std::atomic<int> range_complete, range_incomplete;
    range_complete.store(0);
    range_incomplete.store(0);

    if (index_type == TYPE_ART) {
    } else if (index_type == TYPE_TIPS) {
        void* root = init_nvmm_pool();
        if (!root) {
                perror("init nvmm pool failed");
                exit(1);
        }
	/* change the data structure to be evaluated 
	 * TIPS-hashtable -> "hlist"
	 * TIPS-lf-hashtable -> "lfhlist"
	 * TIPS-binary tree -> "bst"
	 * TIPS-lf-binary-tree -> "lfbst"
	 * TIPS-b+tree -> "btree"
	 * TIPS-ART -> "art" 
	 * TIPS-CLHT-> "clht" */
        void* ds_ptr = set_ds_type(ds);
        if (!ds_ptr) {
                perror("init data structure failed");
                exit(1);
        }
        int ret = tips_init(num_thread, ds_ptr, root);
        fn_read *read_fn_ptr = get_read_fn_ptr();
        fn_insert *insert_fn_ptr = get_insert_fn_ptr();
        fn_scan *scan_fn_ptr = get_scan_fn_ptr();
        fn_delete *delete_fn_ptr = get_delete_fn_ptr();
        std::atomic<int> next_thread_id;
        vector<tips_thread *> selfArray;
        for (int i = 0; i < 2 * num_thread; i++) {
                tips_thread* self = (tips_thread*) malloc(sizeof(
                                        tips_thread));
		if (!self)
			perror("YCSB thread allocation failed \n");
                memset(self, 0, sizeof(tips_thread));
                self->id = i;
		self->stats = (th_stats *) malloc(sizeof(th_stats));
		if (!self)
			perror("YCSB thread stats allocation failed \n");
		self->o = tips_assign_oplog();
		self->stats->epoch = false;
		tips_update_thread(self);
                selfArray.push_back(self);
        }
        tips_init_threads(true);
        {
            // Load
            auto starttime = std::chrono::system_clock::now();
            next_thread_id.store(0);
            auto func = [&]() {
                int thread_id = next_thread_id.fetch_add(1);
	//	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set[thread_id]);
                tips_thread *self = selfArray[thread_id];
                
                uint64_t start_key = LOAD_SIZE / num_thread * (uint64_t)thread_id;
                uint64_t end_key = start_key + LOAD_SIZE / num_thread;
		
		clock_gettime(CLOCK_MONOTONIC, &self->stats->start);
                for (uint64_t i = start_key; i < end_key; i++) {
                    int ret_val = tips_insert(init_keys[i], 
                                    "12345678", 
                                    self->o, insert_fn_ptr);
                    if (!ret_val);
                }
		free(self);
            };
	    int ret;
            std::vector<std::thread> thread_group;

            for (int i = 0; i < num_thread; i++)
                thread_group.push_back(std::thread{func});

            for (int i = 0; i < num_thread; i++)
                thread_group[i].join();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            printf("Throughput: load, %f ,ops/us\n", (LOAD_SIZE * 1.0) / duration.count());
	    /* wait for pt to finish*/
	    tips_finish_load();
	    /* destroy all the load oplog*/
	    ret = tips_deallocate_oplog(num_thread);
	    if (ret)
		perror("oplog deallaoc failed\n");
	    /* init bg thread before starting RUN*/
	    tips_init_bg_thread(false);
        }	
        {
	   double t_ddr_hits = 0;
            // Run
            auto starttime = std::chrono::system_clock::now();
            next_thread_id.store(0);
            auto func = [&]() {
                int thread_id = next_thread_id.fetch_add(1);
	//	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set[thread_id]);
                uint64_t start_key = RUN_SIZE / num_thread * (uint64_t)thread_id;
                uint64_t end_key = start_key + RUN_SIZE / num_thread;
                tips_thread* self = selfArray[thread_id + num_thread];
		
		clock_gettime(CLOCK_MONOTONIC, &self->stats->start);
                for (uint64_t i = start_key; i < end_key; i++) {
                    if (ops[i] == OP_INSERT) {
                        int ret_val = tips_insert(keys[i], 
                                    "12345678", 
                                    self->o, insert_fn_ptr);
                    } else if (ops[i] == OP_READ) {
                        char* ret = tips_read(keys[i], read_fn_ptr, self);
                        if (ret != NULL && strcmp(ret, "12345678")) {
                //            std::cout << "[TIPS] wrong key read: " << ret << " expected:" << keys[i] << std::endl;
                        }

                    } else if (ops[i] == OP_SCAN) {
			    uint64_t *retKeys = (uint64_t *)malloc(sizeof(uint64_t) * 1000);
                            tips_scan(keys[i], ranges[i], retKeys, self, scan_fn_ptr);
			    free(retKeys);
                    }
                }
		t_ddr_hits += self->ddr_hit;
                free(self);
            };

            std::vector<std::thread> thread_group;

            for (int i = 0; i < num_thread; i++)
                thread_group.push_back(std::thread{func});

            for (int i = 0; i < num_thread; i++)
                thread_group[i].join();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            printf("Throughput: run, %f ,ops/us\n", (RUN_SIZE * 1.0) / duration.count());
        }
        tips_dinit();
    } 	else if (index_type == TYPE_LF) {
        
		void* root = init_nvmm_pool();
        if (!root) {
                perror("init nvmm pool failed");
                exit(1);
        }
	/* change the data structure to be evaluated 
	 * DL-LFHT-> "p_hlist_dl"
	 * BDL-LFHT-> "p_hlist_bdl"
	 * DL-LFBST-> "p_bst_dl"
	 * BDL-LFBST-> "p_bst_bdl"*/
        void* ds_ptr = set_ds_type(ds);
        if (!ds_ptr) {
                perror("init data structure failed");
                exit(1);
        }
        std::atomic<int> next_thread_id;
        {
            // Load
            auto starttime = std::chrono::system_clock::now();
            next_thread_id.store(0);
            auto func = [&]() {
                int thread_id = next_thread_id.fetch_add(1);
                
                uint64_t start_key = LOAD_SIZE / num_thread * (uint64_t)thread_id;
                uint64_t end_key = start_key + LOAD_SIZE / num_thread;

                for (uint64_t i = start_key; i < end_key; i++) {
              //          printf("init_key: %ld, %ld\n", i, init_keys[i]);
                    int ret_val = lf_insert(init_keys[i], 
                                    "12345678");
                    if (!ret_val);
                }
            };

            std::vector<std::thread> thread_group;

            for (int i = 0; i < num_thread; i++)
                thread_group.push_back(std::thread{func});

            for (int i = 0; i < num_thread; i++)
                thread_group[i].join();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            printf("Throughput: load, %f ,ops/us\n", (LOAD_SIZE * 1.0) / duration.count());
        
        }
        {
            // Run
            auto starttime = std::chrono::system_clock::now();
            next_thread_id.store(0);
            auto func = [&]() {
                int thread_id = next_thread_id.fetch_add(1);
                uint64_t start_key = RUN_SIZE / num_thread * (uint64_t)thread_id;
                uint64_t end_key = start_key + RUN_SIZE / num_thread;
                for (uint64_t i = start_key; i < end_key; i++) {
                    if (ops[i] == OP_INSERT) {
                        int ret_val = lf_insert(keys[i], 
                                    "12345678");
                    } else if (ops[i] == OP_READ) {
                //        printf("key: %ld, %ld\n", i, keys[i]);
                        char* ret = lf_read(keys[i]);
                        if (ret != NULL && strcmp(ret, "12345678")) {
                            std::cout << "[TIPS] wrong key read: " << ret << " expected:" << keys[i] << std::endl;
                        }
                    } else if (ops[i] == OP_SCAN) {
                            continue;
                    }
                }
            };

            std::vector<std::thread> thread_group;

            for (int i = 0; i < num_thread; i++)
                thread_group.push_back(std::thread{func});

            for (int i = 0; i < num_thread; i++)
                thread_group[i].join();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            printf("Throughput: run, %f ,ops/us\n", (RUN_SIZE * 1.0) / duration.count());
        }
        destroy_nvmm_pool();
    }
}

int main(int argc, char **argv) {
    if (argc != 7) {
        std::cout << "Usage: ./ycsb [index type] [ycsb workload type] [key distribution] [access pattern] [number of threads]\n";
        std::cout << "1. index type: art hot bwtree masstree clht\n";
        std::cout << "               fastfair levelhash cceh woart\n";
        std::cout << "2. ycsb workload type: a, b, c, e\n";
        std::cout << "3. key distribution: randint, string\n";
        std::cout << "4. access pattern: uniform, zipfian\n";
        std::cout << "5. number of threads (integer)\n";
        return 1;
    }

    printf("%s, %s, workload%s, %s, %s, threads %s\n", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);

    int n_threads = atoi(argv[6]);
    int cpu_id;
    CPU_ZERO(&cpu_set[0]);
    for (int i = 0; i < n_threads; ++i) {
		cpu_id = get_cpu_id();
		CPU_SET(cpu_id, &cpu_set[0]);
    }

    int index_type;
    if (strcmp(argv[1], "art") == 0)
        index_type = TYPE_ART;
    else if (strcmp(argv[1], "tips") == 0)
        index_type = TYPE_TIPS;
    else if (strcmp(argv[1], "lock-free") == 0)
        index_type = TYPE_LF;
    else {
        fprintf(stderr, "Unknown index type: %s\n", argv[1]);
        exit(1);
    }

    if (strcmp(argv[2], "btree") == 0) {
	    strcpy(ds, "btree");
    }
    else if (strcmp(argv[2], "btree-fixed") == 0) {
	    strcpy(ds, "btree-fixed");
    }
    else if (strcmp(argv[2], "art") == 0) {
	    strcpy(ds, "art");
    }
    else if (strcmp(argv[2], "hlist") == 0) {
	    strcpy(ds, "hlist");
    }
    else if (strcmp(argv[2], "lfhlist") == 0) {
	    strcpy(ds, "lfhlist");
    }
    else if (strcmp(argv[2], "bst") == 0) {
	    strcpy(ds, "bst");
    }
    else if (strcmp(argv[2], "lfbst") == 0) {
	    strcpy(ds, "lfbst");
    }
    else if (strcmp(argv[2], "clht") == 0) {
	    strcpy(ds, "clht");
    }
    else if (strcmp(argv[2], "lfdlht") == 0) {
	    strcpy(ds, "p_hlist_dl");
    }
    else if (strcmp(argv[2], "lfbdlht") == 0) {
	    strcpy(ds, "p_hlist_bdl");
    }
    else if (strcmp(argv[2], "lfbdlbst") == 0) {
	    strcpy(ds, "p_bst_bdl");
    }
    else if (strcmp(argv[2], "lfdlbst") == 0) {
	    strcpy(ds, "p_bst_dl");
    }
    else if (strcmp(argv[2], "nvthash") == 0) {
	    strcpy(ds, "nvthash");
    }
    else if (strcmp(argv[2], "nvtbst") == 0) {
	    strcpy(ds, "nvtbst");
    }
    else {
	    printf("Invalid TIPS Index \n");
	    exit(0);
    }

    int wl;
    if (strcmp(argv[3], "a") == 0) {
        wl = WORKLOAD_A;
    } else if (strcmp(argv[3], "b") == 0) {
        wl = WORKLOAD_B;
    } else if (strcmp(argv[3], "c") == 0) {
        wl = WORKLOAD_C;
    } else if (strcmp(argv[3], "d") == 0) {
        wl = WORKLOAD_D;
    } else if (strcmp(argv[3], "e") == 0) {
        wl = WORKLOAD_E;
    } else {
        fprintf(stderr, "Unknown workload: %s\n", argv[3]);
        exit(1);
    }

    int kt;
    if (strcmp(argv[4], "randint") == 0) {
        kt = RANDINT_KEY;
    } else if (strcmp(argv[4], "string") == 0) {
        kt = STRING_KEY;
    } else {
        fprintf(stderr, "Unknown key type: %s\n", argv[4]);
        exit(1);
    }

    int ap;
    if (strcmp(argv[5], "uniform") == 0) {
        ap = UNIFORM;
    } else if (strcmp(argv[5], "zipfian") == 0) {
        ap = ZIPFIAN;
    } else {
        fprintf(stderr, "Unknown access pattern: %s\n", argv[5]);
        exit(1);
    }

    int num_thread = atoi(argv[6]);
    tbb::task_scheduler_init init(num_thread);

    if (kt != STRING_KEY) {
        std::vector<uint64_t> init_keys;
        std::vector<uint64_t> keys;
        std::vector<int> ranges;
        std::vector<int> ops;

        init_keys.reserve(LOAD_SIZE);
        keys.reserve(RUN_SIZE);
        ranges.reserve(RUN_SIZE);
        ops.reserve(RUN_SIZE);

        memset(&init_keys[0], 0x00, LOAD_SIZE * sizeof(uint64_t));
        memset(&keys[0], 0x00, RUN_SIZE * sizeof(uint64_t));
        memset(&ranges[0], 0x00, RUN_SIZE * sizeof(int));
        memset(&ops[0], 0x00, RUN_SIZE * sizeof(int));

        ycsb_load_run_randint(index_type, wl, kt, ap, num_thread, init_keys, keys, ranges, ops);
    } else {
#ifdef STR_KEY	    
        std::vector<Key *> init_keys;
        std::vector<Key *> keys;
        std::vector<int> ranges;
        std::vector<int> ops;

        init_keys.reserve(LOAD_SIZE);
        keys.reserve(RUN_SIZE);
        ranges.reserve(RUN_SIZE);
        ops.reserve(RUN_SIZE);

        memset(&init_keys[0], 0x00, LOAD_SIZE * sizeof(Key *));
        memset(&keys[0], 0x00, RUN_SIZE * sizeof(Key *));
        memset(&ranges[0], 0x00, RUN_SIZE * sizeof(int));
        memset(&ops[0], 0x00, RUN_SIZE * sizeof(int));

        ycsb_load_run_string(index_type, wl, kt, ap, num_thread, init_keys, keys, ranges, ops);
#else
	perror("Invalid key type");
#endif
    }

    return 0;
}
