#include <stdlib.h>
#include <string.h>
#include "llist.h"

#include <stdio.h>

char **
GetNextElemAddrLL(void *elemp)
{
	/* back up 4 bytes to get next element address */
	return (char**)((char*)elemp - sizeof(void*));
}

void
GeneralLListSort (LList *list, int (*cmpfn)(void*, void*))
{
	void	*p, *r, *q, *newhead, *last, *newlast;
	char	**pp, **rp, **qp, *t;
	int		sorted;

	if (SizeOfLL(list) <= 1) {
		return;
	}
	/* create dummy head */
	newhead = NewHeadOfLL(list);

	last = newlast = NULL;
	sorted = 0;
	
	while (!sorted) {
		sorted = 1;
		
		r = HeadOfLL(list);
		rp = GetNextElemAddrLL(r);
		memcpy(&p, rp, sizeof(void*));
		pp = GetNextElemAddrLL(p);
		memcpy(&q, pp, sizeof(void*));
		qp = GetNextElemAddrLL(q);
		
		while (q != last) {
			if (((*cmpfn)(p, q)) > 0) {
				// swap elements
				memcpy(rp, &q, sizeof(void*));
				memcpy(pp, qp, sizeof(void*));
				memcpy(qp, &p, sizeof(void*));

				if (q == list->tail) {
					list->tail = p;
				}

				t = p;
				p = q;
				pp = GetNextElemAddrLL(p);
				q = t;
				qp = GetNextElemAddrLL(q);

				newlast = q;
				sorted = 0;
			}
			r = p;
			rp = GetNextElemAddrLL(r);
			p = q;
			pp = GetNextElemAddrLL(p);
			memcpy(&q, qp, sizeof(void*));
			qp = GetNextElemAddrLL(q);
		}
		last = newlast;
	}

	/* delete dummy head */
	DelHeadOfLL(list);

	return;
}
