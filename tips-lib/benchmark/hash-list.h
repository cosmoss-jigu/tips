#ifndef _HASH_LIST_H
#define _HASH_LIST_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define N_BUCKETS 16000000
#define ENABLE_BUCKET_LOCK

uint64_t get_bucket_num(uint64_t);
node *init_hash();
int hash_insert(void *, uint64_t, char *);
int hash_delete(void *, uint64_t);
char *hash_read(void *, uint64_t);

int tips_hash_insert(void *, uint64_t, char *);
int tips_hash_delete(void *, uint64_t);
char *tips_hash_read(void *, uint64_t);
int tips_lf_hash_insert(void *, uint64_t, char *);
char *tips_lf_hash_read(void *, uint64_t);
int tips_lf_hash_delete(void *, uint64_t);
int lf_hash_insert(void *, uint64_t, char *);
char *lf_hash_read_dl(void *, uint64_t);
char *lf_hash_read_bdl(void *, uint64_t);

node *dram_init_hash();
int dram_hash_insert(void *, uint64_t, char *);
int dram_hash_delete(void *, uint64_t);
char *dram_hash_read(void *, uint64_t);

#ifdef __cplusplus
}
#endif

#endif /* hash-list.h*/
