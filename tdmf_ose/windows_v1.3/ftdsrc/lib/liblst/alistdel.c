#include "alist.h"

void DeleteFromAL (AList *list, int i) {
	register char *ptr;

	if (i < 0 || i >= list->free_index) {
		/* trying to delete a non-existent entry
		 * just causing a return is not very good,
		 * since the program will continue to execute
		 * eventhough an error has occured.  It would
		 * be better to cause a coredump
		 */
		return;
	}

	ptr = (char*)ElemOfAL(list, i+1);

	memcpy (ptr - list->objsize, ptr,
		list->free_ptr - ptr);

	list->free_ptr -= list->objsize;
	list->free_index--;
}
