#include "alist.h"

void *NewTailOfAL (AList *list) {

	if (AListSize(list) == list->maxsize) {
		IncreaseAListSize (list, AListSize(list) + 1);
	}
	list->free_ptr += list->objsize;
	list->free_index++;
	return (list->free_ptr - list->objsize);
}

void *DelTailOfAL (AList *list) {

	if (AListSize(list) == 0)
		return (0);

	list->free_ptr -= list->objsize;
	list->free_index--;
	return (list->free_ptr);
}
