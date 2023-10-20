#ifndef __LIST_H__
#define __LIST_H__

#include <stdlib.h>

struct list_entry {
	void *value;
	struct list_entry *next;
	struct list_entry *prev;
};

struct list {
	struct list_entry *head;
	struct list_entry *tail;
};

void list_push(struct list *list, void *value);
void *list_pop(struct list *list);

#endif
