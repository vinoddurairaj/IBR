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
 * $Id: udef.c,v 1.3 2001/01/17 17:58:56 hutch Exp $
 *
 */

/* device object un-definition method */

#include <stdio.h>
#include <sys/cfgodm.h>
#include <sys/cfgdb.h>
#include <cf.h>
#include "./drvodm.h"


main(argc, argv)
	int             argc;
	char           *argv[];

{
	extern int      optind;
	extern char    *optarg;

	char           *devname;
	char            sstring[256];

	struct CuDv     cusobj;

	struct Class   *cusdev;

	int             rc;
	int             errflg, c;

	errflg = 0;
	devname = NULL;
	while ((c = getopt(argc, argv, "l:")) != EOF) {
		switch (c) {
		case 'l':
			if (devname != NULL)
				errflg++;
			devname = optarg;
			break;
		default:
			errflg++;
		}
	}
	if (errflg) {
		exit(E_ARGS);
	}
	if (devname == NULL)
		exit(E_LNAME);

	if (odm_initialize() == -1)
		exit(E_ODMINIT);

	if (odm_lock("/etc/objrepos/config_lock", 0) == -1)
		err_exit(E_ODMLOCK);

	if ((int) (cusdev = odm_open_class(CuDv_CLASS)) == -1)
		err_exit(E_ODMOPEN);

	sprintf(sstring, "name = '%s'", devname);
	rc = (int) odm_get_first(cusdev, sstring, &cusobj);
	if (rc == 0) {
		err_exit(E_NOCuDv);
	} else if (rc == -1) {
		err_exit(E_ODMGET);
	}

	/* heed the device object state machine */
	if (cusobj.status != DEFINED)
		err_exit(E_DEVSTATE);

	/* clear any attr's associated with this dev */
	sprintf(sstring, "name = '%s'", devname);
	if ((odm_rm_obj(CuAt_CLASS, sstring)) == -1)
		err_exit(E_ODMDELETE);

	reldevno(devname, TRUE);

	/* clear the dev itself */
	sprintf(sstring, "name = '%s'", devname);
	if ((odm_rm_obj(cusdev, sstring)) == -1)
		err_exit(E_ODMDELETE);

	if (odm_close_class(CuDv_CLASS) == -1)
		err_exit(E_ODMCLOSE);

	odm_terminate();

	exit(0);

}

err_exit(exit_code)
	int             exit_code;
{
	odm_close_class(CuDv_CLASS);
	odm_terminate();
	exit(exit_code);
}
