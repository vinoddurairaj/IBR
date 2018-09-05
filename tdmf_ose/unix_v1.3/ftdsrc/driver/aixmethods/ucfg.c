/*-
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ucfg.c,v 1.3 2001/01/17 17:58:56 hutch Exp $
 *
 */

/* device object un-configuration method */

#include <stdio.h>
#include <sys/cfgdb.h>
#include <sys/cfgodm.h>
#include <sys/sysconfig.h>
#include <sys/device.h>
#include <unistd.h>
#include <cf.h>
#include <fcntl.h>
#include "../../ftd_kern_ctypes.h"
#include "../../ftd_def.h"
#include "../../../lib/libgen/cfg_intr.h"
#include "./drvodm.h"


extern int      optind;
extern char    *optarg;

int
err_return(int err)
{
	odm_terminate();
	return err;
}

int
main(int argc, char *argv[], char *envp[])
{
	int             c;
	char           *logical_name = 0;
	char            sstring[MAXPATHLEN];
	struct Class   *cusdev;
	struct Class   *predev;
	struct CuDv    *cusobj;
	struct PdDv    *preobj;
	struct cfg_dd   cfg;
	struct qry_devsw qry;
	struct cfg_kmod kmod;
	struct cfg_load cfg_ld;
	int             majorno;
	int             minorno;
	long           *minor_list;
	int             how_many;
	int             rc;

	while ((c = getopt(argc, argv, "l:")) != EOF)
		switch (c) {
		case 'l':
			if (logical_name)
				return E_ARGS;
			logical_name = optarg;
			break;
		default:
			return E_ARGS;
		}
	if (!logical_name)
		return E_LNAME;

	if (odm_initialize() == -1)
		return E_ODMINIT;

	if ((int) (cusdev = odm_open_class(CuDv_CLASS)) == -1)
		return err_return(E_ODMOPEN);
	sprintf(sstring, "name = '%s'", logical_name);
	if (!(cusobj = odm_get_first(cusdev, sstring, (void *) 0)))
		return err_return(E_NOCuDv);
	else if ((int) cusobj == -1)
		return err_return(E_ODMGET);

	/* heed the device object state machine */
	if (cusobj->status == DEFINED)
		return err_return(E_OK);

	if ((int) (predev = odm_open_class(PdDv_CLASS)) == -1)
		return err_return(E_ODMOPEN);
	sprintf(sstring, "uniquetype = '%s'", cusobj->PdDvLn_Lvalue);
	if (!(preobj = odm_get_first(predev, sstring, (void *) 0)))
		return err_return(E_NOPdDv);
	else if ((int) preobj == -1)
		return err_return(E_ODMGET);

	/* major/minor numbers */
	if ((majorno = genmajor(preobj->DvDr)) == -1)
		return err_return(E_MAJORNO);
	minorno = FTD_CTL;

	/* configure device down */
	bzero(&cfg, sizeof(cfg));
	cfg.devno = makedev(majorno, minorno);
	cfg.cmd = CFG_TERM;
	cfg.ddsptr = 0;
	cfg.ddslen = 0;
	if (sysconfig(SYS_CFGDD, &cfg, sizeof(struct cfg_dd)) == -1)
		return err_return(E_SYSCONFIG);

	/*-
	 * again, two methogs for unloading the module.
	 * sysconfig() appears to work more reliably.
	 */
#if defined(USE_LOADEXT)
	if (!(cfg.kmid = loadext(preobj->DvDr, FALSE, FALSE)))
		return err_return(E_UNLOADEXT);
#else /* defined(USE_LOADEXT) */
	sprintf(&sstring[0], "%s%s", "/usr/lib/drivers/", preobj->DvDr);
	cfg_ld.kmid = 0;
	cfg_ld.path = sstring;
	cfg_ld.libpath = NULL;
	if (sysconfig(SYS_QUERYLOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
		perror("SYS_QUERYLOAD");
		err_return(E_SYSCONFIG);
	}
	if (cfg_ld.kmid) {
		cfg_ld.path = NULL;
		cfg_ld.libpath = NULL;
		if (sysconfig(SYS_KULOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
			perror("SYS_KULOAD");
			err_return(E_SYSCONFIG);
		}
	}
#endif /* defined(USE_LOADEXT) */

	/* heed the device object state machine */
	cusobj->status = DEFINED;
	if (odm_change_obj(cusdev, cusobj) == -1)
		return err_return(E_ODMUPDATE);

	if (odm_close_class(cusdev) == -1 ||
	    odm_close_class(predev) == -1)
		return err_return(E_ODMCLOSE);

	odm_terminate();

	return 0;
}
