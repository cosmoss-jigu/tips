#ifndef _DDR_GC_H
#define _DDR_GC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

void wakeup_gc_thread();
void sleep_gc_thread();
void *reclaim_ddr_cache(void *);
void collect_ddr_entries(void*, uint64_t);
void try_ddr_reclaim(void *);
void free_ddr_entries(unsigned int); 
int update_index_safe(int);

#ifdef __cplusplus
}
#endif
#endif /* ddr-gc.h*/
