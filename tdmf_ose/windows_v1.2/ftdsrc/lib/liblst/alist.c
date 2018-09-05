#include <stdlib.h>
#include <string.h>
#include "alist.h"

AList *CreateAList (int objsize) {
	AList *list = (AList *) malloc (sizeof (AList));

	InitAList (list, objsize);
	return (list);
}

void InitAList (AList *list, int objsize) {
	list->buffer = 0;
	list->maxsize = 0;
	list->free_ptr = 0;
	list->free_index = 0;
	list->objsize = objsize;
}

void FreeAList (AList *list) {
	UndoAList (list);
	free (list);
}

void UndoAList (AList *list) {
	if (list->buffer)
		free (list->buffer);
	memset(list, 0, sizeof(AList)) ;
}

void EmptyAList (AList *list) {
	list->free_ptr = list->buffer;
	list->free_index = 0;
}

void IncreaseAListSize (AList *list, int new_size) {
	int newbytesize, free_offset;

	if (new_size < list->maxsize)
		return;
	if (new_size < 2 * list->maxsize) {
		new_size = 2 * list->maxsize;
	}
	free_offset = list->free_ptr - list->buffer ;
	newbytesize  = new_size * list->objsize ;

	if (!list->buffer) {
		list->buffer = (char*)malloc(newbytesize);
	} else {
		list->buffer  = (char*)realloc(list->buffer, newbytesize);
	}
	list->maxsize = new_size ;
	list->free_ptr = list->buffer + free_offset;
	/* the free index field remains the same */
}
