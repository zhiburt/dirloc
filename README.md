# dirloc

A simple way to count LOC with batteries included.

```bash
Usage: ./dirloc [OPTIONS] [ARGS]

[ARGS]
expected to get a list of files, folders

[OPTIONS]
  -s                                            shorten path to a 1 letter, except a file name
  -r                                            collect files recursively
  -f file                                       file
  -h, --help                                    print this help and exit
```

# Usage

Calculate lines of code of this repository.

```bash
$ dirloc .
./dirloc 0
./main.c 198
./str.c 46
./dirloc.h 23
./README.md 46
./str.h 16
./.clang-format 196
./.gitignore 0
./list.c 47
./dirloc.c 355
./list.h 19
./Makefile 13
```

Shorten path

```bash
$ dirloc -s ~/projects/tabled/
h/m/p/t/README.md 2092
h/m/p/t/CHANGELOG.md 413
h/m/p/t/Cargo.lock 506
h/m/p/t/.gitignore 12
h/m/p/t/Cargo.toml 13
```

## Parallel (using `parallel`)

```bash
$ parallel dirloc {1} ::: /projects/tabled /projects/expectrl
/projects/tabled/README.md 2092
/projects/tabled/CHANGELOG.md 413
/projects/tabled/Cargo.lock 506
/projects/tabled/.gitignore 12
/projects/tabled/Cargo.toml 13
/projects/expectrl/README.md 116
/projects/expectrl/.cirrus.yml 11
/projects/expectrl/Cargo.lock 460
/projects/expectrl/.gitignore 14
/projects/expectrl/Cargo.toml 38
/projects/expectrl/LICENSE 21
```

## Sorting

```bash
$ dirloc /projects/tabled /projects/expectrl | sort -k2 --numeric-sort
/projects/expectrl/.cirrus.yml 11
/projects/tabled/.gitignore 12
/projects/tabled/Cargo.toml 13
/projects/expectrl/.gitignore 14
/projects/expectrl/LICENSE 21
/projects/expectrl/Cargo.toml 38
/projects/expectrl/README.md 116
/projects/tabled/CHANGELOG.md 413
/projects/expectrl/Cargo.lock 460
/projects/tabled/Cargo.lock 506
/projects/tabled/README.md 2092
```

## TODO:

- A flag for `prefix` to change a default space (' ')
- Include built in `parallel` support
- Include built in `sort` support
- benchmarks
- test BSD build
