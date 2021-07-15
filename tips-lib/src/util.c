#include "util.h"

void *dram_alloc(size_t size)
{
	void *addr = malloc(size);
	if (!addr) {
		perror("dram alloc failed\n");
		return NULL;
	}
	return addr;
}

void *dram_calloc(size_t num, size_t size)
{
	void *addr = calloc(num, size);
	if (!addr) {
		perror("dram alloc failed\n");
		return NULL;
	}
	return addr;
}

void *dram_realloc(void **ptr, size_t size )
{
	void *new_addr;
	new_addr = realloc(ptr, size);
	if (!new_addr) {
		perror("dram alloc failed\n");
		return NULL;
	}
	return new_addr;
}

void dram_free(void *addr)
{
	free(addr);
	return;
}

void flush_to_nvm(void *addr, size_t size) 
{
	for(unsigned int i = 0; i < size; i += L1_CACHE_BYTES) {
		clwb(((char *)addr) + i);
	}
}

