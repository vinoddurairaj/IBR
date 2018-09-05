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
 * $Id: def.c,v 1.3 2001/01/17 17:58:56 hutch Exp $
 *
 */

/* device object definition method.  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/cfgdb.h>
#include <sys/cfgodm.h>
#include <cf.h>
#include "./drvodm.h"

extern int      genseq();

main(argc, argv, envp)
	int             argc;
	char          **argv;
	char          **envp;
{
	extern int      optind;
	extern char    *optarg;

	char           *class, *subclass, *type;
	char           *lname;
	char            sstring[256];

	struct Class   *predev, *cusdev;

	struct PdDv     PdDv;
	struct CuDv     CuDv;

	int             seqno;
	int             rc;
	int             errflg, c;

	errflg = 0;
	class = subclass = type = lname = NULL;

	while ((c = getopt(argc, argv, "c:s:t:")) != EOF) {
		switch (c) {
		case 'c':	/* Device class */
			if (class != NULL)
				errflg++;
			class = optarg;
			break;
		case 's':	/* Device subclass */
			if (subclass != NULL)
				errflg++;
			subclass = optarg;
			break;
		case 't':	/* Device type */
			if (type != NULL)
				errflg++;
			type = optarg;
			break;
		default:
			errflg++;
		}
	}
	if (errflg) {
		/* error parsing parameters */
		exit(E_ARGS);
	}

	/* class, subclass, and type must be for pseudo device ftd */
	if ((class == NULL) ||
	    (subclass == NULL) ||
	    (type == NULL) ||
	    strcmp(class, ODM_CLASS) ||
	    strcmp(subclass, ODM_SUBCLASS) ||
	    strcmp(type, ODM_TYPE))
		exit(E_TYPE);

	if (odm_initialize() == -1) {
		/* odm initialization failed */
		exit(E_ODMINIT);
	}
	if (odm_lock("/etc/objrepos/config_lock", 0) == -1) {
		/* failed to lock odm data base */
		err_exit(E_ODMLOCK);
	}

	/* get device object poop for this device */
	sprintf(sstring, "type = '%s' AND class = '%s' AND subclass = '%s'",
		type, class, subclass);
	rc = (int) odm_get_first(PdDv_CLASS, sstring, &PdDv);
	if (rc == 0) {
		err_exit(E_NOPdDv);
	} else if (rc == -1) {
		err_exit(E_ODMGET);
	}
	if ((int) (cusdev = odm_open_class(CuDv_CLASS)) == -1) {
		/* error opening class */
		err_exit(E_ODMOPEN);
	}

	/* generate logical name for device */
	if ((seqno = genseq(PdDv.prefix)) < 0) {
		/* error making logical name */
		err_exit(E_MAKENAME);
	}

	/* make only one instance */
	if ( seqno > 0 )
		err_exit(E_OK);

	/* instantiate a new device object */
	sprintf(CuDv.name, "%s%d", PdDv.prefix, seqno);
	CuDv.status = DEFINED;
	CuDv.chgstatus = PdDv.chgstatus;
	CuDv.parent[0] = '\0';
	CuDv.connwhere[0] = '\0';
	CuDv.location[0] = '\0';
	strcpy(CuDv.ddins, PdDv.DvDr);
	strcpy(CuDv.PdDvLn_Lvalue, PdDv.uniquetype);

	/* add new object */
	if (odm_add_obj(cusdev, &CuDv) == -1) {
		err_exit(E_ODMADD);
	}
	if (odm_close_class(CuDv_CLASS) == -1) {
		err_exit(E_ODMCLOSE);
	}
	odm_terminate();

	/* spit out dev object name */
	fprintf(stdout, "%s ", CuDv.name);

	exit(E_OK);

}

err_exit(exitcode)
	char            exitcode;
{
	odm_close_class(CuDv_CLASS);
	odm_close_class(PdDv_CLASS);
	odm_terminate();
	exit(exitcode);
}
