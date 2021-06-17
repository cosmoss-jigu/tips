#include "pmdk.h"
#include "util.h"

static PMEMobjpool *g_pop;
static PMEMoid g_root;
POBJ_LAYOUT_BEGIN(buff);
POBJ_LAYOUT_TOID(buff, PMEMoid);
POBJ_LAYOUT_END(buff);
pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;
alloc_info *info; 
static unsigned int alloc_index = 0;

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
		printf("created new pool\n");
	}
	else {
		if ((g_pop = pmemobj_open(PMEM_PATH_DEF,
					POBJ_LAYOUT_NAME(tips))) == NULL) {
			perror("failed to open th exsisting pool\n");
			return NULL;
		}
		printf("opened existing pool\n");
	}
	/* allocate a root in the nvmem pool, here on root_obj*/
	g_root = pmemobj_root(g_pop, sizeof(root_obj));
	init_alloc_array();
	return pmemobj_direct(g_root);
}

void init_alloc_array()
{
	size_t alloc_size;
	PMEMoid addr;
	int ret;

	alloc_size = ARRAY_SIZE * sizeof(*info);
	ret = pmemobj_alloc(g_pop, &addr, alloc_size, 
			0, NULL, NULL);
	if (ret) {
		perror("alloc info failed");
		exit(0);
	}
	info = (alloc_info *)pmemobj_direct(addr);
	if (!info) {
		perror("alloc array init failed");
		exit(0);
	}
	return;
}

void *nvm_alloc(size_t size)
{
	int ret;
	PMEMoid _addr;

	ret = pmemobj_alloc(g_pop, &_addr, size, 
			0, NULL, NULL);
	if (ret) {
		perror("nvmm memory allocation failed\n");
	}
	return pmemobj_direct(_addr);
}

void *tips_alloc(size_t size)
{
	int ret;
	PMEMoid _addr;
	void *addr;
	PMEMoid *buff;
	
	pthread_mutex_lock(&alloc_lock);
	buff = &info[alloc_index].obj;
	info[alloc_index].size = size;
	++alloc_index;
	pthread_mutex_unlock(&alloc_lock);

	ret = pmemobj_alloc(g_pop, buff, size, 
			alloc_index, NULL, NULL);
	if (ret) {
		perror("tips memory allocation failed\n");
		exit(0);
	}
	_addr = *buff;
	addr = pmemobj_direct(_addr);
	if (!addr) {
		perror("NULL tips alloc");
		exit(0);
	}
	return addr;
}

void *flush_alloc_list(void *arg)
{
	int *p_tid = (int *)arg;
	int tid = *p_tid;
	unsigned int i, start, end;
	PMEMoid _addr;
	void *addr;
	size_t size;
	
	if (alloc_index % 2 != 0) 
		smp_faa(&alloc_index, 1);
	start = (alloc_index/N_FL_THREADS) * tid;
	end = start + (alloc_index/N_FL_THREADS);

	for (i = start; i < end; ++i) {
		_addr = info[i].obj;
		addr = pmemobj_direct(_addr);
		size = info[i].size;
		flush_to_nvm(addr, size);
	}
//	smp_wmb();
	if (alloc_index) {
		pthread_mutex_lock(&alloc_lock);
		alloc_index = 0;
		pthread_mutex_unlock(&alloc_lock);
	}
	return NULL;
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
