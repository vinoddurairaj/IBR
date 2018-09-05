#include <stdio.h>
#include "alist.h"
#include <stdlib.h>

main ()
{
	char lbuf[1024] ;
	AList *list;
	int i, *ip;

	list = CreateAList (sizeof(int));

	fprintf (stderr, "Enter number: ");
	while (fgets(lbuf, sizeof(lbuf), stdin)) {

		i = atoi(lbuf) ;
		AddToTailAL (list, &i);

		fprintf (stderr, "Enter number: ");
	}
	fprintf(stdout,"\n \n") ;

	ForEachALElement (list, ip) {

		fprintf (stdout, "AList Member: %d\n", *ip) ;
	}

	fprintf(stdout,"An AList structure takes %d bytes.\n",
		sizeof(AList)) ;

	FreeAList(list) ;
}
/*
.ft 8
.sp .5
Enter number: \fC1\fP
Enter number: \fC2\fP
Enter number: \fC3\fP
Enter number: \fC4\fP
...
Enter number: \fC19\fP
Enter number: \fC20\fP
Enter number: \fC^\fPD
 

AList Member: 1
AList Member: 2
...
AList Member: 18
AList Member: 19
AList Member: 20


An AList structure takes 20 bytes.
.ft C
*/
