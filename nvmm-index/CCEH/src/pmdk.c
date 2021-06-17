#include "pmdk.h"

static PMEMobjpool *g_pop;

root_obj *init_nvmm_pool(char *path,
                         size_t size,
                         char *layout_name,
                         int *is_created) {
  /* You should not init twice. */
  assert(g_pop == NULL);

  /* check if the path exsists already, else create one
  * or open the exsisting file*/
  if (access(path, F_OK) != 0) {
    if ((g_pop = pmemobj_create(path, layout_name, size*MIB_SIZE, 0666)) == NULL) {
      perror("failed to create pool\n");
      return NULL;
    }
    *is_created = 1;
  } else {
    if ((g_pop = pmemobj_open(path, layout_name)) == NULL) {
      perror("failed to open th exsisting pool\n");
      return NULL;
    }
    *is_created = 0;
  }
  /* allocate a root in the nvmem pool, here on root_obj*/
  PMEMoid g_root = pmemobj_root(g_pop, sizeof(root_obj));
  return (root_obj*)pmemobj_direct(g_root);
}

int destroy_nvmm_pool() {
  pmemobj_close(g_pop);
  g_pop = NULL;
  return 0;
}

void *nvm_alloc(size_t size) {
  int ret;
  PMEMoid _addr;

  ret = pmemobj_alloc(g_pop, &_addr, size, 0, NULL, NULL);
  if (ret) {
    perror("nvmm memory allocation failed\n");
  }
  return pmemobj_direct(_addr);
}

void nvm_free(void *addr) {
  PMEMoid _addr;

  _addr = pmemobj_oid(addr);
  pmemobj_free(&_addr);
  return;
}
