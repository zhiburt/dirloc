#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

struct str
str_copy(const char *src, size_t size)
{
	struct str str = { .value = NULL, .length = 0 };
	char *value;

	value = (char *)malloc(sizeof(char) * size + 1);
	if (value == NULL) {
		return (str);
	}

	value[size] = '\0';
	memcpy(value, src, size);

	str.length = size;
	str.value = value;

	return (str);
}

int str_join(struct str* str, char* part, size_t size) {
	str->value = (char*)realloc(str->value, str->length + size + 1);
	strncpy(str->value + str->length, part, size);
	str->length += size;
	str->value[str->length] = '\0';
	return (0);
}

int
str_length(const struct str *str)
{
	return ((int)str->length);
}

int
str_free(const struct str *str)
{
	free(str->value);
	return (0);
}
