#include <stdio.h>
#include <nlist.h>
#include "./drvnlist.h"

#if defined(XCOFF32)
struct nlist drvtxtnlist [] = {
#else /* defined(XCOFF32) */
struct nlist64 drvtxtnlist [] = {
#endif /* defined(XCOFF32) */
{"vnop_readdir",0,0,0,0,0},
};


main()
{
	int i;
	int errs;
	int nlisterr;
	struct nlist poo;

	memset(&poo, 0, sizeof(poo));
	poo.n_name = "family_lock_statistics";
	
	errs=0;
	for(i=0;i<NDRVSYMS;i++){
		nlisterr = knlist(&poo, 1, sizeof(poo));
		if (nlisterr != 0)
			errs++;
	}

	exit(0);

}
