#include <stdint.h>
#include "hash-list.h"
#include "siphash.h"

pthread_rwlock_t hash_lock = PTHREAD_RWLOCK_INITIALIZER;

#ifdef TEST_LIST
node *dram_init_hash()
{
	node *hash;
	size_t size;

	size = sizeof(*hash) * N_BUCKETS;
	hash = malloc(size);
	memset(hash, 0, size);
	return hash;
}

int dram_hash_insert(void *p_hash, uint64_t key, 
		char *val)
{
	uint64_t bkt;
	node *hash;
	int ret;

	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = dram_list_insert((void *)&hash[bkt], 
			key, val);
	if (ret > 1) {
		perror("hash insert failed\n");
		printf("ret= %d\n", ret);
		return 2;
	}
	return ret;
}

char *dram_hash_read(void *p_hash, uint64_t target_key)
{
	uint64_t bkt;
	node *hash;
	char *val;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(target_key);
	val = dram_list_read((void *)&hash[bkt], 
			target_key);
	if (!val)
		return NULL;
	return val;
}

int dram_hash_delete(void *p_hash, uint64_t key)
{
	uint64_t bkt;
	node *hash;
	int ret;

	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = dram_list_delete((void *)&hash[bkt], key);
	if (!ret) 
		return 1;
	return 0;
}
#endif 

uint64_t get_bucket_num(uint64_t key)
{
	key += ~(key << 32);
	key ^= (key >> 22);
	key += ~(key << 13);
	key ^= (key >> 8);
	key += (key << 3);
	key ^= (key >> 15);
	key += ~(key << 27);
	key ^= (key >> 31);
	if (key > N_BUCKETS) 
		key %= N_BUCKETS;
	return key;	
}

node *init_hash()
{
	node *hash;
	size_t size;

	size = sizeof(*hash) * N_BUCKETS;
	hash = nvm_alloc(size);
	memset(hash, 0, size);

#ifdef ENABLE_BUCKET_LOCK
	for (int i = 0; i < N_BUCKETS; ++i) {
		pthread_rwlock_init(&hash[i].bkt_lock, NULL);
	}
#endif
	flush_to_nvm(hash, size);
	smp_wmb();
	return hash;
}

int hash_insert(void *p_hash, uint64_t key, 
		char *val)
{
	uint64_t bkt;
	node *hash;
	int ret;

	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = list_insert((void *)&hash[bkt], 
			key, val);
	if (ret > 1) {
		perror("hash insert failed\n");
		printf("ret= %d\n", ret);
		return 2;
	}
	return ret;
}

int tips_hash_insert(void *p_hash, uint64_t key, 
		char *val)
{
	uint64_t bkt;
	node *hash;
	int ret;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(key);

#ifndef ENABLE_BUCKET_LOCK
	pthread_rwlock_wrlock(&hash_lock);
#else
	pthread_rwlock_wrlock(&hash[bkt].bkt_lock);
#endif
	ret = tips_list_insert((void *)&hash[bkt], 
			key, val);
	if (ret > 1) {
		perror("hash insert failed\n");
		printf("ret= %d\n", ret);

#ifndef ENABLE_BUCKET_LOCK
		pthread_rwlock_unlock(&hash_lock);
#else
		pthread_rwlock_unlock(&hash[bkt].bkt_lock);
#endif
		return 2;
	}
#ifndef ENABLE_BUCKET_LOCK
		pthread_rwlock_unlock(&hash_lock);
#else
		pthread_rwlock_unlock(&hash[bkt].bkt_lock);
#endif
	return ret;
}

char *hash_read(void *p_hash, uint64_t target_key)
{
	uint64_t bkt;
	node *hash;
	char *val;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(target_key);
	val = list_read((void *)&hash[bkt], 
			target_key);
	if (!val)
		return NULL;
	return val;
}

char *tips_hash_read(void *p_hash, uint64_t target_key)
{
	uint64_t bkt;
	node *hash;
	char *val;
		
	hash = (node *)p_hash;
	bkt = get_bucket_num(target_key);
#ifndef ENABLE_BUCKET_LOCK
	pthread_rwlock_rdlock(&hash_lock);
#else
	pthread_rwlock_rdlock(&hash[bkt].bkt_lock);
#endif
	val = tips_list_read((void *)&hash[bkt], 
			target_key);
	if (!val) {
#ifndef ENABLE_BUCKET_LOCK
		pthread_rwlock_unlock(&hash_lock);
#else
		pthread_rwlock_unlock(&hash[bkt].bkt_lock);
#endif
		return NULL;
	}
#ifndef ENABLE_BUCKET_LOCK
		pthread_rwlock_unlock(&hash_lock);
#else
		pthread_rwlock_unlock(&hash[bkt].bkt_lock);
#endif
	return val;
}

int hash_delete(void *p_hash, uint64_t key)
{
	uint64_t bkt;
	node *hash;
	int ret;

	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = list_delete((void *)&hash[bkt], key);
	if (!ret) 
		return 1;
	return 0;
}

int tips_hash_delete(void *p_hash, uint64_t key)
{
	uint64_t bkt;
	node *hash;
	int ret;
	
	/* TODO: RW lock*/
	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = tips_list_delete((void *)&hash[bkt], key);
	if (!ret) 
		return 1;
	return 0;
}

int lf_hash_insert(void *p_hash, uint64_t key, 
		char *val)
{
	uint64_t bkt;
	node *hash;
	int ret;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = lf_list_insert((void *)&hash[bkt], 
			key, val);
	if (ret > 1) {
		perror("hash insert failed\n");
		printf("ret= %d\n", ret);
		return 2;
	}
	return ret;
}

int tips_lf_hash_insert(void *p_hash, uint64_t key, 
		char *val)
{
	uint64_t bkt;
	node *hash;
	int ret;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = tips_lf_list_insert((void *)&hash[bkt], 
			key, val);
	if (ret > 1) {
		perror("hash insert failed\n");
		printf("ret= %d\n", ret);
		return 2;
	}
	return ret;
}

char *lf_hash_read_bdl(void *p_hash, uint64_t target_key)
{
	uint64_t bkt;
	node *hash;
	char *val;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(target_key);
	val = lf_list_read_bdl((void *)&hash[bkt], 
			target_key);
	if (!val) {
		return NULL;
	}
	return val;
}

char *lf_hash_read_dl(void *p_hash, uint64_t target_key)
{
	uint64_t bkt;
	node *hash;
	char *val;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(target_key);
	val = lf_list_read_dl((void *)&hash[bkt], 
			target_key);
	if (!val) {
		return NULL;
	}
	return val;
}

char *tips_lf_hash_read(void *p_hash, uint64_t target_key)
{
	uint64_t bkt;
	node *hash;
	char *val;
	
	hash = (node *)p_hash;
	bkt = get_bucket_num(target_key);
	val = tips_list_read((void *)&hash[bkt], 
			target_key);
	if (!val) {
		return NULL;
	}
	return val;
}

int tips_lf_hash_delete(void *p_hash, uint64_t key)
{
	uint64_t bkt;
	node *hash;
	int ret;

	hash = (node *)p_hash;
	bkt = get_bucket_num(key);
	ret = tips_lf_list_delete((void *)&hash[bkt], 
			key);
	if (!ret) 
		return 1;
	return 0;
}

