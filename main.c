#include <sys/stat.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dirloc.h"

struct config {
	bool short_path;
	bool recursive;
	char **files;
	size_t count_files;
};

void
usage_invalid_option(const char *name, const char *opt)
{
	fprintf(stderr, "%s: invalid option -- %s\n", name, opt);
	fprintf(stderr, "Try `%s --help' for more information.\n", name);
}

void
log_invalid_file(const char *file)
{
	fprintf(stderr, "file does not exist '%s'\n", file);
}

void
log_io_file(const char *file, const char *reason)
{
	fprintf(stderr, "IO error %s '%s'\n", reason, file);
}

void
usage(const char *name)
{
	printf("Usage: %s [OPTIONS] [ARGS]\n", name);
	printf("\n");
	printf("[ARGS]\n");
	printf("expected to get a list of files, folders\n");
	printf("\n");
	printf("[OPTIONS]\n");
	printf("  -s						shorten path to a 1 letter, except a file name\n");
	printf("  -r						collect files recursively\n");
	printf("  -f file					file\n");
	printf("  -h, --help					print this help and exit\n");
	printf("\n");
}

bool
is_file_exists(const char *path)
{
	struct stat st;

	return (stat(path, &st) < 0 ? false : true);
}

int
parse_args(struct config *cfg, int argc, char *argv[])
{
	int c, count_opts = 0, count_files = 0;
	char *filename = NULL;
	bool was_file_invalid = false;
	const char *short_opt = ":hsr";
	static struct option long_opt[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "short", no_argument, NULL, 's' },
		{ NULL, 0, NULL, 0 },
	};

	while ((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (c) {
		case -1: /* no more arguments */
		case 0:	 /* long options toggles */
			return 0;
		case 's':
			cfg->short_path = true;
			break;
		case 'r':
			cfg->recursive = true;
			break;
		case 'h':
			usage(argv[0]);
			return (0);
		case '?':
			usage_invalid_option(argv[0], argv[optind - 1]);
			return (-2);
		case ':':
			usage_invalid_option(argv[0], "");
			return (-2);
		default:
			return (-1);
		};

		count_opts++;
	};

	if (count_opts + 1 == argc) {
		return (0);
	}

	count_files = argc - count_opts - 1;
	cfg->files = calloc(sizeof(char *), (size_t)count_files + 1);
	cfg->files[count_files] = NULL;

	for (int i = 0; i < count_files; i++) {
		filename = argv[optind + i];
		cfg->files[i] = filename;

		if (!is_file_exists(filename)) {
			log_invalid_file(filename);
			was_file_invalid = true;
		}
	}

	if (was_file_invalid) {
		free(cfg->files);
		return (-3);
	}

	cfg->count_files = (size_t)count_files;

	return (0);
}

void
file_println(struct file_info *file, bool trim)
{
	char* path;
	char* path_next;

	if (trim) {
		path = strtok(file->path->value, "/");
		path_next = strtok(NULL, "/");

		while (path_next != NULL) {
			if (strcmp(path, ".") == 0) {
				printf("./");
			} else if (strcmp(path, "..") == 0) {
				printf("../");
			} else {
				printf("%c/", path[0]);
			}

			path = path_next;
			path_next = strtok(NULL, "/");
		}

		printf("%s %ld\n", path, file->loc);
	} else {
		printf("%s %ld\n", file->path->value, file->loc);
	}
}

int
main(int argc, char *argv[])
{
	int err;
	struct file_iterator *iter;
	struct file_info *file;
	struct config cfg = {
		.recursive = false,
		.short_path = false,
		.files = NULL,
		.count_files = 0,
	};

	err = parse_args(&cfg, argc, argv);
	if (err < 0) {
		return (err);
	}

	if (cfg.files == NULL) {
		return (0);
	}

	iter = file_iterator_create(cfg.files, cfg.recursive);
	if (iter == NULL) {
		return (-1);
	}

	while((file = file_iterator_next(iter))) {
		file_println(file, cfg.short_path);
	}

	err = file_iterator_free(iter);
	if (err < 0) {
		return (err);
	}

	if (cfg.files != NULL)
		free(cfg.files);

	return (0);
}
