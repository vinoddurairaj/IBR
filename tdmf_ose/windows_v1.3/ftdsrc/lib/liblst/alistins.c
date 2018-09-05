#include "alist.h"

void *NewInsertOfAL (AList *list, void *generic_ptr) {
/*
 * In order to allow the user to pass in a generic pointer
 * and still be able to do address arithmetic with that pointer,
 * we can copy the (void *) value passed in as an argument
 * in a local (char *) variable.
 */
	register char *insertptr = (char*)generic_ptr ;
	register char *ptr;
	register int   objsize = list->objsize;

    if (AListSize(list) == list->maxsize) {
		int insert_offset = insertptr - list->buffer;

		IncreaseAListSize (list, AListSize(list) + 1);
		insertptr = list->buffer + insert_offset;
	}

	for (ptr = list->free_ptr - 1; ptr >= insertptr; ptr--) {
		ptr[objsize] = ptr[0];
	}

	list->free_ptr += list->objsize;
	list->free_index++;

	return (insertptr);
}
