#ifndef _llisth
#define _llisth

#include <memory.h>

#ifdef ANSI
#	define PROTO(args)	args	
#else
#	define PROTO(args)	()
#endif


typedef struct {
	void *tail;
	void *head;
	int objsize;
} LList;

#ifdef __cplusplus
extern "C"{ 
#endif

extern LList *CreateLList PROTO ((int objsize));
extern void   FreeLList PROTO ((LList *list));

extern void   InitLList PROTO ((LList *list, int objsize));
extern void   UndoLList PROTO ((LList *list));

extern void   EmptyLList PROTO ((LList *list));
extern void  *IsInLList PROTO ((
	LList *list, void *obj, int (*cmpfn)()
));

extern void  *NewHeadOfLL PROTO ((LList *list));
extern void  *NewTailOfLL PROTO ((LList *list));
extern void  *NewAppendOfLL PROTO ((LList *list, void *prev));

extern int SizeOfLL PROTO ((LList *list));

/***** Element Operations *****/
extern void *AllocateLListElement PROTO ((int size));

#define FreeLListElement(objptr)	free (((void **) (objptr)) - 1)
#define NextLListElement(objptr)	(((void **) (objptr))[-1])

/* The renaming allows either LListElement or LLElement notation */
#define AllocateLLElement(size) AllocateLListElement(size)
#define FreeLLElement(objptr)	FreeLListElement(objptr)
#define NextLLElement(objptr)	NextLListElement(objptr)

#define HeadOfLL(list)	((list)->head)
#define TailOfLL(list)	((list)->tail)

/***** List Query Operations *****/
#define IsLLEmpty(list)		((list)->tail == 0)
#define IsEmptyLL(list)		((list)->tail == 0)
#define ObjSizeOfLL(list) ((list)->objsize)

/***** Add/Insert Operations *****/

#define AddToHeadLL(list, obj)	\
	((void) memcpy ( NewHeadOfLL(list), obj, (list)->objsize))

#define AddToTailLL(list, obj)	\
	((void) memcpy ( NewTailOfLL(list), obj, (list)->objsize))

#define AppendToLL(list, el, obj)	\
	((void) memcpy ( NewAppendOfLL(list, el), obj, (list)->objsize))

extern void GeneralLListSort (LList *list, int (*cmpfn)(void*, void*));

#define SortLL(list, cmpfn)	GeneralLListSort (list, cmpfn)

/***** Delete Operations *****/

extern void DelHeadOfLL PROTO ((LList *list));
extern void RemHeadOfLL PROTO ((LList *list));

extern int DelNextOfLL PROTO((LList *list, void *curr));

#define DelElemOfLL(list, obj, pobj)	\
	DelNextOfLL((list), (obj), (pobj))

extern int DelCurrOfLL PROTO((LList *list, void *curr));

extern int RemNextOfLL PROTO((LList *list, void *curr));

#define RemElemOfLL(list, obj, pobj)	\
	RemNextOfLL((list), (obj), (pobj))

extern int RemCurrOfLL PROTO((LList *list, void *curr));

/***** Looping Constructs *****/

#define ForEachLLElement(list, current) 	\
	for ((current) = HeadOfLL (list); \
		 (current) != 0; \
		 (current) = (void *) NextLListElement(current))

#ifdef __cplusplus 
}
#endif

#endif
