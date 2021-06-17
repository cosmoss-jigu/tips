#ifndef _DS_H
#define _DS_H

#include "hash-list.h"
#include "bst.h"
#include "bptree.h"
#include "clht_lb_res.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define IS_DRAM

typedef char * (fn_read) (void *, uint64_t);
typedef int (fn_scan) (void *, uint64_t, int, uint64_t *);
typedef int fn_insert (void *, uint64_t, char *);
typedef int fn_delete (void *, uint64_t);


void *set_ds_type(const char *);
fn_insert *get_insert_fn_ptr();
fn_delete *get_delete_fn_ptr();
fn_read *get_read_fn_ptr();
fn_scan *get_scan_fn_ptr();
int _insert(uint64_t, char *);
int _delete(uint64_t);
char *_read (uint64_t);
int _scan(uint64_t, uint64_t);
int lf_insert(uint64_t, char *);
char *lf_read (uint64_t);


#ifdef __cplusplus
}
#endif

#endif /* ds.h*/
