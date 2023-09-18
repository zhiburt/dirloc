#include <stdio.h>
#include <stdlib.h>

#include "list.h"

void
list_push(struct list *list, void *value)
{
	if (list->head == NULL) {
		list->head = (struct list_entry *)malloc(
		    sizeof(struct list_entry));
		list->head->value = value;
		list->head->next = NULL;
		list->head->prev = NULL;
		list->tail = list->head;
	} else {
		list->tail->next = (struct list_entry *)malloc(
		    sizeof(struct list_entry));
		list->tail->next->value = value;
		list->tail->next->next = NULL;
		list->tail->next->prev = list->tail;
		list->tail = list->tail->next;
	}
}

void *
list_pop(struct list *list)
{
	struct list_entry *tail;
	void *tail_value;

	if (list->tail == NULL) {
		return NULL;
	}

	tail = list->tail;
	tail_value = list->tail->value;
	list->tail = list->tail->prev;

	if (tail == list->head) {
		list->head = NULL;
	}

	free(tail);

	return (tail_value);
}
