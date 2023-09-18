#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dirloc.h"
#include "list.h"
#include "str.h"

struct file_iterator {
	bool recursive;
	struct list pool;
	struct file_info *file_entry;
};

int64_t
file_count_lines(int fd, size_t fsize)
{
	// https://stackoverflow.com/questions/33616284/read-line-by-line-in-the-most-efficient-way-platform-specific/33620968#33620968

	char *buf, *fbuf;
	int64_t cnt = 0;
	int err;

	fbuf = (char *)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (fbuf == NULL) {
		return (-1);
	}

	for (buf = fbuf; buf < &fbuf[fsize]; buf = buf + 1) {
		buf = strchr(buf, '\n');
		if (buf == NULL)
			break;

		cnt++;
	}

	err = munmap(fbuf, fsize);
	if (err < 0)
		return (-1);

	return (cnt);
}

int
file_list_fill_dir(struct list *pool, const struct str *path)
{
	DIR *dir;
	struct dirent *entry;
	struct str *entrypath;
	int has_trailing_slash;

	dir = opendir(path->value);
	if (dir == NULL) {
		return (-2);
	}

	has_trailing_slash = path->length > 0 &&
	    path->value[path->length - 1] == '/';

	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		entrypath = (struct str *)malloc(sizeof(struct str));
		if (entrypath == NULL) {
			return (-1);
		}

		*entrypath = str_copy(path->value, path->length);
		if (entrypath->value == NULL) {
			return (-1);
		}

		if (!has_trailing_slash) {
			str_join(entrypath, "/", 1);
		}

		str_join(entrypath, entry->d_name, strlen(entry->d_name));

		list_push(pool, entrypath);
	}

	closedir(dir);

	return (0);
}

int
collect_files(struct file_info **out, char **files, bool recursive)
{
	int size = 0, fd, err;
	char **path;
	struct str *pool_entry;
	struct str *cpath;
	struct stat st;
	struct file_info *file;
	struct list pool = { .head = NULL, .tail = NULL };

	if (files == NULL)
		return (0);

	// seed pool
	for (path = files; *path != NULL; path++) {
		cpath = (struct str *)malloc(sizeof(struct str));
		if (cpath == NULL) {
			return (-1);
		}

		*cpath = str_copy(*path, strlen(*path));
		if (cpath->value == NULL) {
			return (-1);
		}

		if (recursive) {
			list_push(&pool, cpath);
		} else {
			err = lstat(cpath->value, &st);
			if (err < 0) {
				return (-2);
			}

			switch (st.st_mode & S_IFMT) {
			case S_IFDIR:
				err = file_list_fill_dir(&pool, cpath);
				if (err < 0) {
					return (err);
				}

				free(cpath);
				break;
			default:
				list_push(&pool, cpath);
				break;
			}
		}
	}

	while ((pool_entry = (struct str *)list_pop(&pool)) != NULL) {
		err = lstat(pool_entry->value, &st);
		if (err < 0) {
			return (-2);
		}

		switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			if (!recursive) {
				continue;
			}

			err = file_list_fill_dir(&pool, pool_entry);
			if (err < 0) {
				return (err);
			}

			free(pool_entry->value);
			free(pool_entry);
			break;
		case S_IFREG:
			fd = open(pool_entry->value, O_RDONLY);
			if (fd < 0) {
				return (-2);
			}

			*out = realloc(*out,
			    sizeof(struct file_info) * ((size_t)size + 1));
			if (out == NULL) {
				return (-1);
			}

			file = (*out + size);
			file->path = pool_entry;
			file->file_size = (uint64_t)st.st_size;
			file->loc = st.st_size > 0 ?
			    file_count_lines(fd, (size_t)st.st_size) :
			    0;

			if (file->loc < 0) {
				return (-2);
			}

			size++;
			break;
		default:
			*out = realloc(*out,
			    sizeof(struct file_info) * ((size_t)size + 1));
			if (out == NULL) {
				return (-1);
			}

			file = (*out + size);
			file->path = pool_entry;
			file->file_size = 0;
			file->loc = 0;

			size++;
			break;
		}
	}

	return (size);
}

struct file_iterator *
file_iterator_create(char **files, bool recursive)
{
	int err;
	struct str *path;
	struct stat st;
	struct file_iterator *iter = (struct file_iterator *)malloc(
	    sizeof(struct file_iterator));
	iter->recursive = recursive;
	iter->pool.head = NULL;
	iter->pool.tail = NULL;
	iter->file_entry = malloc(sizeof(struct file_info));
	iter->file_entry->path = NULL;
	iter->file_entry->file_size = 0;
	iter->file_entry->loc = 0;

	// seed pool
	for (; *files != NULL; files++) {
		path = (struct str *)malloc(sizeof(struct str));
		if (path == NULL) {
			return (NULL);
		}

		*path = str_copy(*files, strlen(*files));
		if (path->value == NULL) {
			return (NULL);
		}

		if (recursive) {
			list_push(&iter->pool, path);
			continue;
		}

		err = lstat(path->value, &st);
		if (err < 0) {
			return (NULL);
		}

		switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			err = file_list_fill_dir(&iter->pool, path);
			if (err < 0) {
				return (NULL);
			}

			free(path);
			break;
		default:
			list_push(&iter->pool, path);
			break;
		}
	}

	return (iter);
}

int
file_iterator_free(struct file_iterator *iter)
{
	free(iter->file_entry);
	free(iter);
	return (0);
}

struct file_info *
file_iterator_next(struct file_iterator *iter)
{
	// todo: Could be cool to be able to reset the iterator after a error
	// occured.

	int err, fd;
	int64_t filesize;
	struct str *path;
	struct stat st;

	if (iter->file_entry->path != NULL) {
		free(iter->file_entry->path->value);
		free(iter->file_entry->path);
	}

	while ((path = (struct str *)list_pop(&iter->pool))) {
		err = lstat(path->value, &st);
		if (err < 0) {
			goto free_path;
		}

		switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			if (!iter->recursive) {
				free(path->value);
				free(path);
				continue;
			}

			err = file_list_fill_dir(&iter->pool, path);
			if (err < 0) {
				goto free_path;
			}

			free(path->value);
			free(path);
			break;
		case S_IFREG:
			fd = open(path->value, O_RDONLY);
			if (fd < 0) {
				goto free_path;
			}

			if (st.st_size > 0) {
				filesize = file_count_lines(fd,
				    (size_t)st.st_size);
				if (filesize < 0) {
					goto free_path;
				}
			}

			iter->file_entry->path = path;
			iter->file_entry->file_size = (uint64_t)st.st_size;
			iter->file_entry->loc = st.st_size > 0 ? filesize : 0;

			return (iter->file_entry);
		default:
			iter->file_entry->path = path;
			iter->file_entry->file_size = 0;
			iter->file_entry->loc = 0;

			return (iter->file_entry);
		}
	}

	return (NULL);

free_path:
	free(path->value);
	free(path);
	return (NULL);
}
