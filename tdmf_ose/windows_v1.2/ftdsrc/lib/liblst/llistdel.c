#include <stdlib.h> 
#include "llist.h"

void DelHeadOfLL (list)
	LList *list;
{
	void *head = list->head;

	list->head = NextLLElement (head);
	if (list->head == 0) {
		list->tail = 0;
	}
	FreeLLElement (head);
}

void RemHeadOfLL (list)
	LList *list;
{
	void *head = list->head;

	list->head = NextLLElement(head);
	if (list->head == 0) {
		list->tail = 0;
	}
}

int DelNextOfLL(list, curr)
    LList *list;
    void *curr;
{
    void *next;

    if (curr == (void*)NULL
    || (next = (void*)NextLLElement(curr)) == (void*)NULL) {
        /*
         * curr is 0 or
         * curr was the tail
         */
        return 0;
    }
    NextLLElement(curr) = NextLLElement(next);
    if (next == TailOfLL(list)) {
        TailOfLL(list) = curr;
    }
    FreeLLElement(next);

    return 1;
}

// Just remove the element from the list without
// freeing the memory associated with the element
int RemNextOfLL(list, curr)
    LList *list;
    void *curr;
{
    void *next;

    if (curr == (void*)NULL
    || (next = (void*)NextLLElement(curr)) == (void*)NULL) {
        /*
         * curr is 0 or
         * curr was the tail
         */
        return 0;
    }
    NextLLElement(curr) = NextLLElement(next);
    if (next == TailOfLL(list)) {
        TailOfLL(list) = curr;
    }

    return 1;
}

int DelCurrOfLL(list, curr)
    LList *list;
    void *curr;
{
    int i;
    void *prev, *p;

    if (curr == HeadOfLL(list)) {
        DelHeadOfLL(list);
        if (IsEmptyLL(list)) {
            return 0;
        }
    } else {
        i = 0;
        ForEachLLElement(list, p) {
           if (i == 0) {
               prev = p;
           } else {
               if (p == curr) {
                   DelNextOfLL(list, prev);
                   break;
               } else {
                   prev = p;
               }
           } 
           i++;
        }
    }

    return 1;
}

int RemCurrOfLL(list, curr)
    LList *list;
    void *curr;
{
    int i;
    void *prev, *p;

    if (curr == HeadOfLL(list)) {
        RemHeadOfLL(list);
        if (IsEmptyLL(list)) {
            return 0;
        }
    } else {
        i = 0;
        ForEachLLElement(list, p) {
           if (i == 0) {
               prev = p;
           } else {
               if (p == curr) {
                   RemNextOfLL(list, prev);
                   break;
               } else {
                   prev = p;
               }
           } 
           i++;
        }
    }

    return 1;
}


