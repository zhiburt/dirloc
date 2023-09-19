#include <sys/stat.h>

#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "dirloc.h"

// clang-format off
enum file_sort_kind {
	FILE_SORT_KIND_NONE,
	FILE_SORT_KIND_PATH = 0x0001,
	FILE_SORT_KIND_LOC 	= 0x0010,
	FILE_SORT_KIND_BYTE = 0x0100,
	FILE_SORT_KIND_DESC = 0x1000,
};
// clang-format on

struct config {
	bool short_path;
	bool recursive;
	enum file_sort_kind sort_kind;
	char *format;
	char **files;
	size_t count_files;
};

static volatile sig_atomic_t sig_shutdown = 0;

void sig_ctrlc_handler(int _signal) {
	(void)_signal;

	sig_shutdown = 1;
}

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
	// clang-format off
	printf("Usage: %s [OPTIONS] [ARGS]\n", name);
	printf("\n");
	printf("[ARGS]\n");
	printf("expected to get a list of files, folders\n");
	printf("\n");
	printf("[OPTIONS]\n");
	printf("  -r					collect files recursively\n");
	printf("  -c					shorten path to a 1 letter, except a file name\n");
	printf("  -s, --sort kind			sort list by [path, loc, byte, r]\n");
	printf("  -f, --format format-string		set custom format of output (options [%%p, %%P, %%l, %%b])\n");
	printf("  					format args:\n");
	printf("  						%%p - path\n");
	printf("  						%%P - path short\n");
	printf("  						%%l - lines of code\n");
	printf("  						%%b - byte size\n");
	printf("\n");
	printf("  -h, --help				print this help and exit\n");
	printf("\n");
	// clang-format on
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
	const char *short_opt = ":hcrs:f:";
	static struct option long_opt[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "sort", optional_argument, NULL, 's' },
		{ "format", required_argument, NULL, 'f' },
		{ "format", required_argument, NULL, 'f' },
		{ NULL, 0, NULL, 0 },
	};

	while ((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (c) {
		case -1: /* no more arguments */
		case 0:	 /* long options toggles */
			return (0);
		case 'c':
			cfg->short_path = true;
			count_opts++;
			break;
		case 'r':
			cfg->recursive = true;
			count_opts++;
			break;
		case 's':
			if (optarg != NULL) {
				if (cfg->sort_kind != FILE_SORT_KIND_NONE &&
				    strcmp(optarg, "r") != 0) {
					usage_invalid_option(argv[0],
					    argv[optind - 1]);
					return (-1);
				}

				if (strcmp(optarg, "path") == 0) {
					cfg->sort_kind = FILE_SORT_KIND_PATH;
				} else if (strcmp(optarg, "loc") == 0) {
					cfg->sort_kind = FILE_SORT_KIND_LOC;
				} else if (strcmp(optarg, "byte") == 0) {
					cfg->sort_kind = FILE_SORT_KIND_BYTE;
				} else if (strcmp(optarg, "r") == 0) {
					if (cfg->sort_kind == FILE_SORT_KIND_NONE) {
						cfg->sort_kind |= FILE_SORT_KIND_LOC;
					}

					cfg->sort_kind |= FILE_SORT_KIND_DESC;
				} else {
					usage_invalid_option(argv[0],
					    argv[optind - 1]);
					return (-1);
				}

				count_opts++;
			} else {
				cfg->sort_kind |= FILE_SORT_KIND_LOC;
			}

			count_opts++;
			break;
		case 'f':
			cfg->format = optarg;
			count_opts += 2;
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
file_println_path(char *value, bool trim)
{
	char *path;
	char *path_next;

	if (!trim) {
		printf("%s", value);
		return;
	}

	path = strtok(value, "/");
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

	printf("%s", path);
}

void
file_println(struct file_info *file, bool trim, const char *format)
{
	if (format == NULL) {
		file_println_path(file->path->value, trim);
		printf(" %ld\n", file->loc);
		return;
	}

	for (char c = *format; c != '\0'; c = *++format) {
		switch (c) {
		case '\\':
			format++;
			switch (*format) {
			case 't':
				printf("\t");
				break;
			case 'n':
				printf("\n");
				break;
			default:
				putchar('\\');
				putchar(c);
				break;
			}
			break;
		case '%':
			format++;
			switch (*format) {
			case 'P':
				file_println_path(file->path->value, true);
				break;
			case 'p':
				file_println_path(file->path->value, false);
				break;
			case 'l':
				printf("%ld", file->loc);
				break;
			case 'b':
				printf("%ld", file->file_size);
				break;
			default:
				putchar('%');
				putchar(*format);
				break;
			}
			break;
		default:
			putchar(c);
			break;
		}
	}

	putchar('\n');
}

int
file_cmp_by_path(const void *a, const void *b)
{
	struct file_info *file_a = ((struct file_info *)a);
	struct file_info *file_b = ((struct file_info *)b);

	return (strcmp(file_a->path->value, file_b->path->value));
}

int
file_cmp_by_path_desc(const void *a, const void *b)
{
	struct file_info *file_a = ((struct file_info *)a);
	struct file_info *file_b = ((struct file_info *)b);

	return (strcmp(file_b->path->value, file_a->path->value));
}

int
file_cmp_by_loc(const void *a, const void *b)
{
	struct file_info *file_a = ((struct file_info *)a);
	struct file_info *file_b = ((struct file_info *)b);

	if (file_a->loc == file_b->loc)
		return (0);
	else if (file_a->loc < file_b->loc)
		return (-1);
	else
		return (1);
}

int
file_cmp_by_loc_desc(const void *a, const void *b)
{
	struct file_info *file_a = ((struct file_info *)a);
	struct file_info *file_b = ((struct file_info *)b);

	if (file_b->loc == file_a->loc)
		return (0);
	else if (file_b->loc < file_a->loc)
		return (-1);
	else
		return (1);
}

int
file_cmp_by_size(const void *a, const void *b)
{
	struct file_info *file_a = ((struct file_info *)a);
	struct file_info *file_b = ((struct file_info *)b);

	if (file_a->file_size == file_b->file_size)
		return (0);
	else if (file_a->file_size < file_b->file_size)
		return (-1);
	else
		return (1);
}

int
file_cmp_by_size_desc(const void *a, const void *b)
{
	struct file_info *file_a = ((struct file_info *)a);
	struct file_info *file_b = ((struct file_info *)b);

	if (file_b->file_size == file_a->file_size)
		return (0);
	else if (file_b->file_size < file_a->file_size)
		return (-1);
	else
		return (1);
}

int
main(int argc, char *argv[])
{
	int count_files, desc_sort, err;
	int (*comparator)(const void *, const void *);
	struct file_iterator *iter;
	struct file_info *file, *files;
	struct config cfg = {
		.recursive = false,
		.short_path = false,
		.files = NULL,
		.format = NULL,
		.sort_kind = FILE_SORT_KIND_NONE,
		.count_files = 0,
	};
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = sig_ctrlc_handler;
	sigaction(SIGINT, &act, NULL);

	err = parse_args(&cfg, argc, argv);
	if (err < 0) {
		return (err);
	}

	if (cfg.files == NULL) {
		return (0);
	}

	if (cfg.sort_kind == FILE_SORT_KIND_NONE) {
		iter = file_iterator_create(cfg.files, cfg.recursive);
		if (iter == NULL) {
			return (-1);
		}

		while ((file = file_iterator_next(iter))) {
			if (sig_shutdown) {
				break;
			}

			file_println(file, cfg.short_path, cfg.format);
		}

		err = file_iterator_free(iter);
		if (err < 0) {
			return (err);
		}
	} else {
		count_files = collect_files(&files, cfg.files, cfg.recursive);
		if (count_files < 0) {
			return (count_files);
		}

		desc_sort = cfg.sort_kind & FILE_SORT_KIND_DESC;
		switch (cfg.sort_kind & ~(uint)FILE_SORT_KIND_DESC) {
		case FILE_SORT_KIND_PATH:
			comparator = desc_sort ? file_cmp_by_path_desc :
						 file_cmp_by_path;
			break;
		case FILE_SORT_KIND_LOC:
			comparator = desc_sort ? file_cmp_by_loc_desc :
						 file_cmp_by_loc;
			break;
		case FILE_SORT_KIND_BYTE:
			comparator = desc_sort ? file_cmp_by_size_desc :
						 file_cmp_by_size;
			break;
		}

		qsort(files, (size_t)count_files, sizeof(struct file_info),
		    comparator);

		for (int i = 0; i < count_files; i++) {
			file = files + i;

			file_println(file, cfg.short_path, cfg.format);

			free(file->path->value);
			free(file->path);
		}

		free(files);
	}

	if (cfg.files != NULL)
		free(cfg.files);

	return (0);
}
