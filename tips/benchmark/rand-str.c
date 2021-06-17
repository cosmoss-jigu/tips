#include <stdio.h>
#include "rand-str.h"

#define SET_SIZE 63
const char rand_set[63] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
			   'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
			   's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', 
			   '1', '2', '3', '4', '5', '6', '7', '8', '9',
			   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
			   'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
			   'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '-'};

void get_str(char *str, size_t size, 
		unsigned int seed)
{
	size_t i; 
	unsigned int index;

	for (i = 0; i < size; ++i) {
		index = rand_r(&seed) % SET_SIZE;
		str[i] = rand_set[index];
	}
	str[size] = '\0';
	return;
}

uint64_t get_key(unsigned int seed)
{	
	uint64_t key;

	key = rand_r(&seed);
	return key;
}

void get_val(char *val, size_t size, 
		unsigned int seed)
{
	get_str(val, size, seed);
	return;
}

/*****************************************************************************
*************************test rand_str function*******************************
******************************************************************************/

/*int main()
{
	unsigned int key_seed = DEFAULT_KEY_SEED;
	unsigned int val_seed = DEFAULT_VAL_SEED;
	size_t key_size = DEFAULT_STR_SIZE;
	size_t val_size = DEFAULT_STR_SIZE;
	char *key, *val;
	
	key = malloc(key_size + 1);
	val = malloc(val_size + 1);
	for (int i = 0; i < 100; ++i) {
		get_key(key, key_size, key_seed);
		get_val(val, val_size, val_seed);
		printf("i= %d\n", i);
		printf("key= %s\t", key);
		printf("val= %s\n", val);
		key_seed += 1;
		val_seed += 1;
	}
	return 0;
}*/
