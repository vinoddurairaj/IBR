/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
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
 * $Id: cfg.c,v 1.5 2010/12/20 20:17:03 dkodjo Exp $
 *
 */

/* device object configuration method */

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
#ifdef getminor
#undef getminor
#endif

extern int      optind;
extern char    *optarg;

int
err_return(int err)
{
	odm_terminate();
	return err;
}

int
make_ctl_spec_files(int devno)
{
	char            buf[128];

	/* make FTD_DEV_DIR */
	(void) mkdir(FTD_DEV_DIR, 0744);

	sprintf(buf, "%s/%s", FTD_DEV_DIR, "ctl");
	(void) unlink(buf);
	if (mknod(buf, S_IRUSR | S_IWUSR | S_IFCHR, devno) < 0)
		return E_MKSPECIAL;

	return E_OK;
}

int
main(int argc, char *argv[], char *envp[])
{
	int             c;
	char           *logical_name = 0;
	int             ipl_phase = RUNTIME_CFG;
	struct Class   *cusdev;
	struct Class   *predev;
	struct CuDv    *cusobj;
	struct PdDv    *preobj;
	char            sstring[MAXPATHLEN];
	struct cfg_dd   cfg;
	struct qry_devsw qry;
	struct cfg_kmod kmod;
	struct cfg_load cfg_ld;
	int             majorno;
	int             minorno;
	long           *minor_list;
	int             how_many;
	int             rc;
	ftd_ctl_t       ctl;
	char            cfgbuf[256];

	while ((c = getopt(argc, argv, "l:12")) != EOF)
		switch (c) {
		case 'l':
			if (logical_name)
				return E_ARGS;
			logical_name = optarg;
			break;
		case '1':
			if (ipl_phase != RUNTIME_CFG)
				return E_ARGS;
			ipl_phase = PHASE1;
			break;
		case '2':
			if (ipl_phase != RUNTIME_CFG)
				return E_ARGS;
			ipl_phase = PHASE2;
			break;
		default:
			return E_ARGS;
		}
	if (!logical_name)
		return E_LNAME;

	if (odm_initialize() == -1)
		return E_ODMINIT;

	/* query custom dev class db for our instance ... */
	if ((int) (cusdev = odm_open_class(CuDv_CLASS)) == -1)
		return err_return(E_ODMOPEN);
	sprintf(sstring, "name = '%s'", logical_name);
	if (!(cusobj = odm_get_first(cusdev, sstring, (void *) 0)))
		return err_return(E_NOCuDv);
	else if ((int) cusobj == -1)
		return err_return(E_ODMGET);

	/* query predefined dev class db for our class... */
	if ((int) (predev = odm_open_class(PdDv_CLASS)) == -1)
		return err_return(E_ODMOPEN);
	sprintf(sstring, "uniquetype = '%s'", cusobj->PdDvLn_Lvalue);
	if (!(preobj = odm_get_first(predev, sstring, (void *) 0)))
		return err_return(E_NOPdDv);
	else if ((int) preobj == -1)
		return err_return(E_ODMGET);

	/* twinkly front panel xmas tree effects. woooo. */
	if (ipl_phase != RUNTIME_CFG)
		setleds(preobj->led);

	/* sysconfig ops setups */
	sprintf(&sstring[0], "%s%s", "/usr/lib/drivers/", preobj->DvDr);

	/* heed the device object state machine */
	if (cusobj->status == DEFINED) {

        int driver_might_still_be_loaded = 1;

        // In some error cases, it is possible to leave the driver loaded but initialized.
        // In order to avoid the problems associated with these cases, we'll make sure that
        // The driver is freshly loaded before it gets initialized.
        // C.F. WR PROD00005936
        while(driver_might_still_be_loaded)
        {
            cfg_ld.kmid = 0;
            cfg_ld.path = sstring;
            cfg_ld.libpath = NULL;
  
            if (sysconfig(SYS_QUERYLOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
                perror("SYS_QUERYLOAD");
                err_return(E_SYSCONFIG);
            }
            /* if driver is already loaded, unload it prior to loading */
            if ((cusobj->status == DEFINED) && cfg_ld.kmid) {
                fprintf(stderr, "The driver was already loaded. Unloading.\n");
                
                cfg_ld.path = NULL;
                cfg_ld.libpath = NULL;
                if (sysconfig(SYS_KULOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
                    perror("SYS_KULOAD");
                    err_return(E_SYSCONFIG);
                }
            }
            else
            {
                driver_might_still_be_loaded = 0;
            }
        }
        
	 /*- 
	  * load the driver, 
	  * two methods are provided here, prefer sysconfig() 
	  * over loadext() since loadext is not idempotent...
	  */
#if defined(USE_LOADEXT)
		bzero(&cfg, sizeof(cfg));
		if (!(cfg.kmid = loadext(preobj->DvDr, TRUE, FALSE))) {
			perror("loadext");
			return err_return(E_LOADEXT);
		}
#else				/* defined(USE_LOADEXT) */
		cfg_ld.kmid = 0;
		cfg_ld.libpath = NULL;
		cfg_ld.path = sstring;
		if (sysconfig(SYS_SINGLELOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
			perror("SYS_KLOAD");
			err_return(E_SYSCONFIG);
		}
		if (cfg_ld.kmid == 0)
			err_return(E_LOADEXT);
#endif				/* defined(USE_LOADEXT) */

		/* get dev major/minor numbers */
		if ((majorno = genmajor(preobj->DvDr)) == -1) {
			(void) loadext(preobj->DvDr, FALSE, FALSE);
			return err_return(E_MAJORNO);
		}
		minorno = FTD_CTL;

		cfg.devno = makedev(majorno, minorno);
		if ((rc = make_ctl_spec_files(cfg.devno)) != E_OK) {
			loadext(preobj->DvDr, FALSE, FALSE);
			return err_return(rc);
		}

		/*-
		 * call ddconfig() driver entry point to 
		 * get driver entry points loaded in the
		 * device switch.
		 */
		cfg.cmd = CFG_INIT;
		cfg.kmid = cfg_ld.kmid;

		/*-
		 * parse config file for driver initialization parms.
		 */
		memset(&ctl, 0, sizeof(ftd_ctl_t));
    memset(cfgbuf, 0, sizeof(cfgbuf));
		if (cfg_get_key_value("num_chunks", cfgbuf, 
		                       CFG_IS_NOT_STRINGVAL) != CFG_OK) {
			return err_return(E_CFGINIT);
		}    
		ctl.num_chunks = strtol(cfgbuf, NULL, 0);
    memset(cfgbuf, 0, sizeof(cfgbuf));
		if (cfg_get_key_value("chunk_size", cfgbuf, 
		                       CFG_IS_NOT_STRINGVAL) != CFG_OK) {
			return err_return(E_CFGINIT);
		}
		ctl.chunk_size = strtol(cfgbuf, NULL, 0);
		cfg.ddsptr = (caddr_t)&ctl;
		cfg.ddslen = sizeof(ftd_ctl_t);

		/* call ddconfig */
		if (sysconfig(SYS_CFGDD, &cfg, sizeof(struct cfg_dd)) == -1) {
			(void) loadext(preobj->DvDr, FALSE, FALSE);
			return err_return(E_CFGINIT);
		}

		/* check whether ddconfig() worked */
		qry.devno = cfg.devno;
		if (sysconfig(SYS_QDVSW, &qry, sizeof(struct qry_devsw)) == -1) {
			(void) loadext(preobj->DvDr, FALSE, FALSE);
			return err_return(E_CFGINIT);
		}

		/* Finally move device to available state */
		cusobj->status = AVAILABLE;
		if (odm_change_obj(cusdev, cusobj) == -1)
			return err_return(E_ODMUPDATE);
	}

	/* wrap it up */
	if (odm_close_class(cusdev) == -1 ||
	    odm_close_class(predev) == -1)
		return err_return(E_ODMCLOSE);

	odm_terminate();

	return 0;
}
