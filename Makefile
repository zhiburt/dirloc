CC		= gcc
CFLAGS	= -Wall -Wextra -Wconversion -pedantic -std=c99 -D_DEFAULT_SOURCE

SRC		= list.h str.h dirloc.h list.c str.c dirloc.c main.c
DST		= dirloc

RM      = rm -f

build: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(DST)

clean: $(DST)
	$(RM) $(DST)
