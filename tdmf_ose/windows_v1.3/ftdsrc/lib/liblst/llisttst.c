#include <stdio.h>
#include <stdlib.h>
#include "llist.h"

extern int getword(FILE *fp, char *word);

int
intcmp(int *one, int *two)
{
	return memcmp(one, two, sizeof(int));
}

main ()
{
	LList *list1, *list2;
	int i, *ip;
	char buf[8];
	void **cp;

	list1 = CreateLList (2 * sizeof(int));
	list2 = CreateLList (2 * sizeof(int));

	printf ("Enter add to head/tail test:\n");

	printf ("Enter number: ");
	while (getword (stdin, buf) != EOF && (i = atoi (buf))) {
		AddToTailLL (list2, &i);
		AddToHeadLL (list1, &i);
		cp = (void **) TailOfLL(list2);
		printf ("Tail: tail == 0x%08x, next addr == 0x%08x\n",
			cp, cp-1);
		cp = (void **) HeadOfLL(list1);
		printf ("Head: head == 0x%08x, next addr == 0x%08x\n\n",
			cp, cp-1);
		printf ("Enter number: ");
	}

	printf ("---- AddToHeadLL\n");
	ForEachLLElement (list1, ip) {
		printf ("Head 0x%08x, Tail 0x%08x,  ip 0x%08x, *ip %d , next 0x%08x.\n",
			HeadOfLL(list1),
			TailOfLL(list1),
			ip, *ip,
			NextLListElement(ip));
	}
	putchar ('\n');

	printf ("---- AddToTailLL\n");
	ForEachLLElement (list2, ip) {
		printf ("Head 0x%08x, Tail 0x%08x,  ip 0x%08x, *ip %d , next 0x%08x.\n",
			HeadOfLL(list2),
			TailOfLL(list2),
			ip, *ip,
			NextLListElement(ip));
	}
	putchar ('\n');

	printf ("---- SortLL\n");
	
	SortLL(list2, (*intcmp));

	ForEachLLElement (list2, ip) {
		printf ("Head 0x%08x, Tail 0x%08x,  ip 0x%08x, *ip %d , next 0x%08x.\n",
			HeadOfLL(list2),
			TailOfLL(list2),
			ip, *ip,
			NextLListElement(ip));
	}
	putchar ('\n');

	FreeLList(list1);

	FreeLList(list2);
}
