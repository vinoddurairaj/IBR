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
/*
 * ftd_stop.c - Stop one or more logical groups
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_stop.c,v 1.26 2012/10/05 20:58:50 paulclou Exp $
 *
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <errno.h> 
#include <malloc.h> 
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

#include "cfg_intr.h"
#include "ps_intr.h"

#include "ftd_cmd.h"
#include "errors.h"
#include "pathnames.h"
#include "config.h"
#include "platform.h"
#include "devcheck.h"
#include "stop_group.h"
#include "common.h"

static char *progname = NULL;
static char configpaths[MAXLG][32];
static int autostart;
static int clear_ODM = 0;	/* For AIX HACMP, leave dtc entries in ODM upon dtcstop -o cmd (pc080205) */


extern char *paths;
char *argv0;

static void
usage(void)
{
#if defined( _AIX )
    fprintf(stderr, "Usage: %s -a | -g <group#> [-s] [-o]\n", progname);
#else
    fprintf(stderr, "Usage: %s -a | -g <group#> [-s]\n", progname);
#endif
    fprintf(stderr, "OPTIONS: -a => all groups\n");
    fprintf(stderr, "         -g => group number (0 - %d)\n", MAXLG-1);
    fprintf(stderr, "         -s => don't clear autostart in pstore\n");
#if defined( _AIX )
    fprintf(stderr, "         -o => leave dtc entries for specified group(s) in ODM (AIX) (use with -g | -a) \n");	   /* pc080205 */
#endif
}


/* Modifications have been brought for AIX HACMP clusters as per the following:
   1) If the groups of interest are effectively running:
      dtcstop -gn: stop group n and, if successful, remove groups(s) odm entries.
      dtcstop -a: stop all groups and, for those which have been successfully stopped, 
          remove the group(s) odm entries.
      dtcstop -o -gn: stop group n and, if successfull, leave this group's devices in ODM database;
      dtcstop -o -a: stop all groups and, for those which have been successfully stopped, leave their 
          devices in the ODM database.

   2) If the groups of interest are NOT running (for instance, previously stopped ):
      dtcstop -gn:  remove this group's devices from ODM database;
      dtcstop -a: remove all groups' devices from ODM database.
(pc080212) */

int 
main(int argc, char *argv[])
{
    int  i, fd, group = 0, all, lgcnt, pid, pcnt, chpid;
    char  **setargv;
    char *cfgpath;
    char  ps_name[MAXPATHLEN];
    int   ch;

    int gflag = 0;		/* -g option flag */
    ps_version_1_attr_t attr;
    int rc, error_status;
    
    putenv("LANG=C");

    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    progname = argv[0];
    if (argc < 2) {
        goto usage_error;
    }

    argv0 = argv[0];

    all = 0;
    memset(ps_name, 0, sizeof(ps_name));

    autostart = 1;

    /* process all of the command line options */
    opterr = 0;
    /* Loop until all cmd options have been decoded and validated; note: the 'o' option has
       been added without compilation switch ("asog:" string) even though it pertains only to
       AIX, to avoid yet another switch, since it does not cause any problem (pc080212) */
    while ((ch = getopt(argc, argv, "asog:f")) != -1) {
        switch(ch) {
        case 'a':
            if (gflag) {
                fprintf(stderr, "Only one of -a and -g options should be specified\n");
                goto usage_error;
            }
            all = 1;
            break;
        case 'g':
            if (all != 0) {
                fprintf(stderr, "Only one of -a and -g options should be specified\n");
                goto usage_error;
            }
            if (gflag) {
                fprintf(stderr, "-g options are multiple specified\n");
                goto usage_error;
            }
            group = ftd_strtol(optarg);
            if (group < 0 || group >= MAXLG) {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                goto usage_error;
            }
            gflag = 1;
            break;
        case 's':
            autostart = 0; 
            break;
#if defined( _AIX )
		case 'o':                 /* pc080205 */
		    /* Set flag to remove dtc entries from ODM (AIX HACMP) */
		    clear_ODM = 1;
			break;
#endif
        default:
            goto usage_error;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Invalid arguments\n");
        goto usage_error;
    }

    if (all == 0 && gflag == 0) {
        fprintf(stderr, "At least, one of -a and -g options should be specified\n");
        goto usage_error;
    }

#if defined( _AIX )
    if( clear_ODM && !(gflag || all) )
    { fprintf(stderr, "One of -a or -g option must be specified with -o\n");
      goto usage_error;
    }
#endif

    if (initerrmgt(ERRFAC) < 0) 
    {
      /* If -s option has been given at dtcstop command (such as at system shutdown), ignore
         errors reported at message-logging initialization; it is possible that the filesystem
         where messages are logged has been remounted Read-only. If so, turn off message logging and
         continue. WR 43148. */
      if( !autostart )
        log_errs = 0;
      else
        exit(1);
    }

    log_command(argc, argv);  /* trace command line in dtcerror.log */

    paths = (char *)configpaths;
    lgcnt = GETCONFIGS(configpaths, 1, 1);

    if (!lgcnt) 
    { /* Groups already stopped */
#if defined( _AIX )
      /* Update ODM database if -o option has NOT been specified: 
        -g (clear_ODM == false): remove entries from ODM database for specified group;
        -a (clear_ODM == false): remove entries from ODM database for all groups.
       (pc080211) */            
	     if( !clear_ODM )
	     {  if( gflag )
		    { /* -g: specific group */
	          update_ODM_entry( group, clear_ODM );
      	    }
		    else
		    { /* -a: all groups */
	          update_all_ODM_entries( clear_ODM );
		    }
		 }
#endif
      reporterr(ERRFAC, M_NOGROUP, ERRWARN);
      exit( 0 );    /* Report warning message but treat as success: wanted group(s) stopped; they already are (pc070904) */
    }

    error_status = 0;
    if (all != 0) {
	/* reverse order to deal with 1-many since start does 0 to lgcnt - 1; */
        for (i = lgcnt - 1; i >= 0; i--) {
            group = cfgtonum(i);
            if (GETPSNAME(group, ps_name) != 0) {
                i = numtocfg(group);
                cfgpath = paths+(i*32);
                reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv[0], cfgpath);
                exit(1);
            }
  
            if ((fd = open(ps_name, O_RDONLY)) < 0) {
                reporterr(ERRFAC, M_FILE, ERRCRIT, ps_name, strerror(errno));
                exit(1);
            }
            close(fd);
            
            /* get the attributes of the pstore */
            if (ps_get_version_1_attr(ps_name, &attr, 1) != PS_OK) {
                reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
                exit(1);
            }
            rc = ftd_stop_group(ps_name, group, &attr, autostart, clear_ODM);
			if( rc != 0 )
			{
			    error_status = 1;
			}

        } /*... for( i = 0... */

    } else {    /*... of if( all... */
        if (GETPSNAME(group, ps_name) != 0) {
            i = numtocfg(group);
            cfgpath = paths+(i*32);
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv[0], cfgpath);
            exit(1);
        }
        
        if ((fd = open(ps_name, O_RDONLY)) < 0) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, ps_name, strerror(errno));
            exit(1);
        }
        close(fd);
        
        /* get the attributes of the pstore */
        if (ps_get_version_1_attr(ps_name, &attr, 1) != PS_OK) {
            reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
            exit(1);
        }
        rc = ftd_stop_group(ps_name, group, &attr, autostart, clear_ODM);
		if( rc != 0 )
		{
		    error_status = 1;
		}
    }
    exit(error_status);
    return 0; /* for stupid compiler */

usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}


/* The Following 3 lines of code are intended to be a hack
   only for HP-UX 11.0 .
   HP-UX 11.0 compile complains about not finding the below
   function when they are staticaly linked - also we DON'T want
   to link it dynamically. So for the time being - ignore these
   functions while linking. */
#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif


