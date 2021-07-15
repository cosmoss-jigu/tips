#include "pmdk.h"

static PMEMobjpool *g_pop;
static PMEMoid g_root;

void *init_nvmm_pool()
{
	/* check if the path exsists already, else create one
	 * or open the exsisting file*/
	if (access(PMEM_PATH_DEF, F_OK) != 0) { 
		if ((g_pop = pmemobj_create(PMEM_PATH_DEF, POBJ_LAYOUT_NAME(tips),
					  MEM_SIZE, 0666)) == NULL) {
			perror("failed to create pool\n");
			return NULL;
		}
	}else {
		if ((g_pop = pmemobj_open(PMEM_PATH_DEF,
					POBJ_LAYOUT_NAME(tips))) == NULL) {
			perror("failed to open th exsisting pool\n");
			return NULL;
		}
	}
	/* allocate a root in the nvmem pool, here on root_obj*/
	g_root = pmemobj_root(g_pop, sizeof(root_obj));
	return pmemobj_direct(g_root);
}

void *nvm_alloc(size_t size)
{
	int ret;
	PMEMoid _addr;

	ret = pmemobj_alloc(g_pop, &_addr, size, 0, NULL, NULL);
	if (ret) {
		perror("nvmm memory allocation failed\n");
	}
	return pmemobj_direct(_addr);
}

void nvm_free(void *addr)
{
	PMEMoid _addr;

	_addr = pmemobj_oid(addr);
	pmemobj_free(&_addr);
	return;
}

int destroy_nvmm_pool()
{
	pmemobj_close(g_pop);
	g_pop = NULL;
	return 0;
}
