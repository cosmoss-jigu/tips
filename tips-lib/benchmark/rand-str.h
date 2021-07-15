#ifndef _RAND_STR_H
#define _RAND_STR_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_STR_SIZE 5
#define DEFAULT_KEY_SEED 0x345
#define DEFAULT_VAL_SEED 0x100

void get_string(char *, size_t, unsigned int);
uint64_t get_key(unsigned int);
void get_val(char *, size_t, unsigned int);

#ifdef __cplusplus
}
#endif

#endif /* rand-str.h */
