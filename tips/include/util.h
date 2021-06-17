#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "arch.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define ORDO_TIMESTAMPING

#ifndef ORDO_TIMESTAMPING
#define gte_clk(__t1, __t2) ((__t1) >= (__t2))
#define lte_clk(__t1, __t2) ((__t1) <= (__t2))
#define gt_clk(__t1, __t2) ((__t1) > (__t2))
#define lt_clk(__t1, __t2) ((__t1) < (__t2))
#define get_clk() read_tscp()
#define get_clk_relaxed() read_tsc()
#define MAX_CLK (ULONG_MAX - 1)
#else /* ORDO_TIMESTAMPING */

#include "ordo-clock.h"
#define gte_clk(__t1, __t2) (!ordo_lt_clock(__t1, __t2))
#define lte_clk(__t1, __t2) (!ordo_gt_clock(__t1, __t2))
#define gt_clk(__t1, __t2) ordo_gt_clock(__t1, __t2)
#define lt_clk(__t1, __t2) ordo_lt_clock(__t1, __t2)
#define get_clk() ordo_get_clock()
#define get_clk_relaxed() ordo_get_clock_relaxed()
#define init_clk() ordo_clock_init()
#define cmp_clk(__t1, __t2)  ordo_cmp_clock(__t1, __t2)
#define new_clk(__local_clk) ordo_new_clock((__local_clk) + ordo_boundary())
#define advance_clk()
#define MAX_CLK (ULONG_MAX - 1)
	
#endif /* ORDO_TIMESTAMPING */

void *dram_alloc(size_t size);
void *dram_calloc(size_t num, size_t size);
void *dram_realloc(void **ptr, size_t size);
void dram_free(void *addr);
void flush_to_nvm(void *addr, size_t size);


#ifdef __cplusplus
}
#endif
#endif /* util.h*/
