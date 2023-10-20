CC		= clang
CFLAGS	= -Wall -Wextra -Wconversion -pedantic -std=c99 -D_DEFAULT_SOURCE

SRC		= list.c str.c dirloc.c main.c
DST		= dirloc

RM      = rm -f

build: $(SRC)
	$(CC) $(CFLAGS) -o $(DST) $(SRC)

clean: $(DST)
	$(RM) $(DST)
