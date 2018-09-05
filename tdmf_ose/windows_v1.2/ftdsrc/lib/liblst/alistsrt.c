#include <stdlib.h>
#include <string.h>
#include "alist.h"

/*
 * high is either off the end, or mid from the last iteration.
 * This means high shouldn't be the comparison key.
 * So if low and high are 0 apart, mid gets low.  
 */
void *GetInsertPtrAL (AList *list, void *elptr, int (*cmpfn)(void*, void*)) {
	register char *mid, *low, *high;
	register int comp, objsize = list->objsize;
	int mid_offset, num_elements = NumElemInAL (list);

	/* do binary search */
	if (!list->buffer)
		return (0);

	low = list->buffer;
	high = list->free_ptr;

	while (low != high) {
		mid_offset = (high - low) / 2;
		if (num_elements % 2) {
			/* number of elements is odd */
			mid_offset -= objsize / 2;
			/* will work if objsize is odd also */
		}
		mid = low + mid_offset;
		comp = (*cmpfn)(elptr, (void*)mid);
		if (!comp)
			return (mid);
		if (comp > 0) {
			low = mid + objsize;
		} else
			high = mid;
		if (comp > 0 && !(num_elements % 2)) {
			num_elements /= 2;
			num_elements--;
		} else
			num_elements /= 2;
	}
	return (low);
}
/* .bp */

/* using a selection sort since the most 
 * time-consuming operation is the swap 
 */
void SortAList (AList *list, int (*cmpfn)(void*, void*)) {
	register char *currptr, *minptr, *free_ptr = (char*) TailOfAL(list);
	register int objsize = list->objsize;
	char *switchptr, *tptr = (char*)malloc (objsize);

	for (switchptr = list->buffer; switchptr < free_ptr; switchptr += objsize) {
		minptr = switchptr;
		for (currptr = minptr + objsize; currptr <= free_ptr; 
			currptr += objsize) {
			if ((*cmpfn)(currptr, minptr) < 0) minptr = currptr;
		}
		if (minptr == switchptr)
			continue;
		memcpy(tptr, switchptr, objsize);
		memcpy(switchptr, minptr, objsize);
		memcpy(minptr, tptr, objsize);
	}
	free (tptr);
}

/* .bp */

void *IsInAList (AList *list, void *elptr, int (*cmpfn)(void*,void*)) {
	char *insert_ptr = (char*)GetInsertPtrAL(list, elptr, cmpfn);

	if (insert_ptr && insert_ptr < list->free_ptr
	&& cmpfn(elptr, insert_ptr) == 0) {
		return (insert_ptr);
	}
	return (0);
}
