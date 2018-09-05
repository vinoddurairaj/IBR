#ifndef _alisth
#define _alisth

/* if _alisth is defined, it means the pre-processor
 * has already seen this file
 */

#include <memory.h>
#include "custom.h"

typedef struct {
	char *buffer;		/* buffer where elements are stored		*/
	int   maxsize;		/* maximum number of objects allowed	*/
	char *free_ptr;		/* the pointer to the next free element */
	int   objsize;		/* the size of each element             */
	int   free_index;   /* the index of the next free element   */
} AList;

extern AList *CreateAList PROTO ((int objsize));
extern void   InitAList PROTO ((AList *list, int objsize));
extern void   FreeAList PROTO ((AList *list)) ;
extern void   UndoAList PROTO ((AList *list)) ;
extern void   EmptyAList PROTO ((AList *list)) ;

extern void   IncreaseAListSize PROTO ((AList *list, int new_size));

extern void *NewTailOfAL PROTO ((AList *list));
extern void *NewInsertOfAL PROTO ((AList *list, void *insertptr));
extern void *DelTailOfAL PROTO ((AList *list));
extern void  DeleteFromAL PROTO ((AList *list, int i));

extern void *GetInsertPtrAL PROTO ((
	AList *list, void *elptr, int (*cmpfn)(void*, void*)
));

extern void  SortAList PROTO ((AList *list, int (*cmpfn)(void*, void*)));

extern void *IsInAList PROTO ((
	AList *list, void *elptr, int (*cmpfn)(void*, void*)
));

/* .bp */
#define SizeOfAList(list)	((list)->free_index)

/* three  synomyms for SizeOfAList */
#define SizeOfAL(list)		SizeOfAList(list)
#define NumElemInAL(list)	SizeOfAList(list)
#define AListSize(list)		SizeOfAList(list)

#define TailOfAL(list)		\
	((void *)((list)->free_ptr - (list)->objsize))

#define HeadOfAL(list)		\
	((void *)((list)->buffer))

#define ElemOfAL(list, i)	\
	((void *)((list)->buffer + (i)*(list)->objsize))

#define AddToTailAL(list, elptr) \
	memcpy (NewTailOfAL(list), elptr, (list)->objsize)

#define DelFromTailAL(list, elptr) \
	memcpy ( elptr, DelTailOfAL(list), (list)->objsize)

#define GetTailOfAL(list, elptr) \
	memcpy ( elptr, TailOfAL(list), (list)->objsize)

#define GetHeadOfAL(list, elptr) \
	memcpy ( elptr, HeadOfAL(list), (list)->objsize)

#define GetElemOfAL(list, elptr, i) \
	memcpy (elptr, ElemOfAL(list,i), (list)->objsize)

#define InsertToAL(list, elptr, i) \
	memcpy (NewInsertOfAL(list, ElemOfAL(list, i)), elptr, \
		(list)->objsize)

#define AddToSortedAL(list, elptr, cmpfn) \
	memcpy (NewInsertOfAL(list, GetInsertPtrAL(list, elptr, cmpfn)), \
		elptr, \
		(list)->objsize)

/* a synomym for AddToSortedAL */

#define AddToSortAL(list, elptr, cmpfn)	\
	AddToSortedAL(list, elptr, cmpfn)

#define IsALEmpty(list)	((list)->free_index == 0)
/* a synomym for IsALEmpty */
#define IsAListEmpty(list)	IsALEmpty(list)

/* .bp */
/* loop by pointer */
#define ForEachAListElementPtr(list, current) \
	for (current = (void *) (list)->buffer; \
		  ((char *) current) < ((list)->free_ptr) ; \
		  current = (void *) (((char *) current) + (list)->objsize))

/* alternate names for loop */
#define ForEachAListElement(list, current) \
	ForEachAListElementPtr(list, current)

#define ForEachALElement(list, current) \
	ForEachAListElementPtr(list, current)

#define ForEachALElementPtr(list, current) \
	ForEachAListElementPtr(list, current)


/* loop by index */
#define ForEachALElementIndex(list, i) \
	for ((i) = 0; (i) < (list)->free_index; (i)++)

#endif
