#ifndef __STR_H__
#define __STR_H__

#include <stdint.h>
#include <stdlib.h>

struct str {
	char *value;
	uint64_t length;
};

struct str str_copy(const char *src, size_t size);
int str_join(struct str* str, char* part, size_t size);
int str_length(const struct str* str);
int str_free(const struct str *str);

#endif