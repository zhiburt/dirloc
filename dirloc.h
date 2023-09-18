#ifndef __DIRLOC_H__
#define __DIRLOC_H__

#include <stdbool.h>
#include <stdint.h>

#include "str.h"

struct file_info {
	struct str *path;
	uint64_t file_size;
	int64_t loc;
};

struct file_iterator;

int collect_files(struct file_info **files, char **list, bool recursive);

struct file_iterator *file_iterator_create(char **list, bool recursive);
int file_iterator_free(struct file_iterator *);
struct file_info *file_iterator_next(struct file_iterator *iter);
bool file_iterator_is_empty(const struct file_iterator *iter);

#endif