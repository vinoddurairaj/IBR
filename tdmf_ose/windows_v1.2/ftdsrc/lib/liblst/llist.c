#include "llist.h"
#include <stdlib.h>

LList *CreateLList (objsize)
	int objsize;
{
	LList *ptr = (LList *) malloc (sizeof (LList));

	InitLList (ptr, objsize);
	return (ptr);
}

void InitLList (list, objsize)
	LList *list;
	int objsize;
{
	list->tail = 0;
	list->head = 0;
	list->objsize = objsize;
}

void FreeLList (list)
	LList *list;
{
	UndoLList (list);
	free (list);
}

void UndoLList (list)
	LList *list;
{
	EmptyLList (list);
	memset(list, 0, sizeof(LList)) ;
}

void EmptyLList (list)
	LList *list;
{
	void *lep;
	void *next ;

	lep = HeadOfLL(list) ;
	while (lep) {
		next = NextLLElement(lep) ;
		FreeLListElement (lep);
		lep = next ;
	}
	list->head = 0;
	list->tail = 0;
}

void *IsInLList (list, obj, cmpfn)
	LList *list;
	void *obj;
	int (*cmpfn) ();
{
	void *lep;

	ForEachLLElement (list, lep) {
		
		if (! (*cmpfn) (obj, lep)) return (lep);
	}
	return (0);
}

int SizeOfLL(list)
	LList *list;
{
	void *lep;
    int cnt;

    cnt = 0;
	ForEachLLElement(list, lep) {
        cnt++;
	}

	return (cnt);
}

