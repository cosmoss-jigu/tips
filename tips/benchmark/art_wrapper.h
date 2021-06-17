#include "ART.h"

void *init_art_tree();
char *tips_art_read(void *p_head, uint64_t target_key);
int tips_art_insert(void *p_head, uint64_t target_key,char *value);
int tips_art_remove(void *p_head, uint64_t target_key);
int tips_art_scan(void *p_root, uint64_t start_key, int range, uint64_t *buff);

