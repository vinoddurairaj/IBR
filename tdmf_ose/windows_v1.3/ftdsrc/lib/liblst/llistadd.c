#include "llist.h"
#include <stdlib.h>

void *AllocateLListElement (size)
	int size;
{
	void **new = malloc (sizeof(void *) + size);

	return ((void *) (new+1));
}

void *NewHeadOfLL (list)
	LList *list;
{
	void *new = AllocateLListElement(list->objsize);

	if (IsLLEmpty (list)) {
		NextLListElement(new) = 0;
		list->head = new;
		list->tail = new;
	}
	else {
		NextLListElement(new) = list->head;
		list->head = new;
	}
	return (new);
}

void *NewTailOfLL (list)
	LList *list;
{	
	void *new = AllocateLListElement(list->objsize);

	NextLListElement(new) = 0;

	if (IsLLEmpty (list)) {
		list->head = new;
		list->tail = new;
	}
	else {
		NextLListElement(list->tail) = new;
		list->tail = new;
	}
	return (new);
}

/* .bp */

void *NewAppendOfLL (list, prev)
	LList *list;
	void *prev;
{
	void *new = AllocateLLElement (list->objsize);

	if (!prev) {
		/* do an add to head */

		NextLListElement(new) = list->head;
		list->head = new;
	}
	else {
		NextLListElement(new) = NextLListElement(prev);
		NextLListElement(prev) = new;
	}
	if (prev == list->tail) {
		/* 
		 * prev is the current tail 
		 * prev could be 0
		 */
		list->tail = new;
	}
	return (new);
}


