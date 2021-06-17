#ifndef _SIPHASH_H
#define _SIPHASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

uint64_t siphash(const uint8_t *in, const size_t inlen, 
		const uint8_t *k);
uint64_t siphash_nocase(const uint8_t *in, const size_t inlen,
		const uint8_t *k); 

#ifdef __cplusplus
}
#endif
#endif /* siphash.h*/
