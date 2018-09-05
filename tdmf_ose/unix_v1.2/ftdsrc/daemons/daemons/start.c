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
/***************************************************************************
 * start.c - FullTime Data start daemon command utility
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for starting daemons
 *
 ***************************************************************************/

#include <stdio.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "pathnames.h"
#include "errors.h"
#include "ipc.h"
#include "process.h"

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

char *argv0;
static int all = 0;
static int group = 0;
static int silent = 0;
static int mode;
static int sig;
static char *statstr;
static int full = 0;
static int rflag = 0; 

extern int lgcnt;
extern int targets[MAXLG];

char configpaths[MAXLG][32];
char *paths = (char*)configpaths;
int len;

extern int ftdsock;

/*
 * usage -- This is how the program is used
 */
void usage(void)
{
    if (silent)
        goto quit;

    fprintf(stderr,"\nUsage: %s <options>\n", argv0);
    fprintf(stderr,"One of the following two options is mandatory:\n" );
    fprintf(stderr,"\t-g<group#> :Put all devices in a " CAPGROUPNAME " Group(0 - %d) into %s mode\n", MAXLG-1, statstr);
    fprintf(stderr,"\t-a           :Put all devices into %s mode\n", statstr);
#ifdef STARTRFD
    fprintf(stderr,"Specify one of the following two options to modify behavior:\n" );
    fprintf(stderr,"\t-f           :Perform Initial Sync - Full Refresh\n");
    fprintf(stderr,"\t-c           :Perform Verify Secondary - Full Checksum\n");
    fprintf(stderr,"\t-r           :Perform Restart Sync - Full Refresh\n");
#endif
    fprintf(stderr,"\t-h           :Print This Listing\n");
    fprintf(stderr,"\n");

quit:

    exit(1);
} /* usage */

/*
 * parseargs -- parse the run-time arguments 
 */
void
parseargs(int argc, char **argv)
{
    int devcnt;
    unsigned long lgnum = 0;
    int ch;
    int i;
    int flag=0;
    int cflag=0;
    
    /* At least one argument is required */
    if (argc == 1) {
        usage();
    }
    /* Determine what options were specified */
    devcnt = 0;

    opterr = 0;
#ifdef STARTRFD
    while ((ch = getopt(argc, argv, "fapcg:r")) != -1) {
        switch (ch) {
        case 'f':
            /* perform Full Sync - No dirtybit/chksum */
	    if (cflag || rflag ) {
                usage();
            }
            sig        = FTDQRFDF;
            mode = FTDRFDF;
            flag=1;
            full=1;
            break;
        case 'c':
	    if (full || rflag ) {
	    fprintf (stderr, "Only one of -f, -c and -r options should be specified\n"); 
                usage();
            }
            /* perform verify secondary - All dirtybit/chksum */
            sig        = FTDQRFDC;
            mode = FTDRFD;
            flag=0;
            cflag=1;
            break;
         /* Added new option -r for restarting the full refresh */
         case 'r':
            if (full || cflag ) {
		fprintf (stderr, "Only one of -f, -c and -r options should be specified\n");
                usage();
            }
            rflag=1;
            sig = FTDQRFDR;
            mode = FTDRFDF;
            break;
#else
    while ((ch = getopt(argc, argv, "fapg:")) != -1) {
        switch (ch) {
#endif
        case 'a':
            /* change the operation mode of all devices in all logical groups */
            if (group) {
                fprintf(stderr, "Only one of -a and -g options should be specified\n");
                usage();
            }
            all = 1;
            for (i = 0; i < lgcnt; i++) {
                targets[cfgtonum(i)] = mode;
            }
            flag=0;
            break;
        case 'g':
            /* change the operation mode of all devices in a logical group */
            if (all) {
                fprintf(stderr, "Only one of -a and -g options should be specified\n");
                usage();
            }
            group = 1;
            lgnum = ftd_strtol(optarg);
            if (lgnum < 0 || lgnum >= MAXLG) {
                fprintf(stderr, "[%s] is invalid number for " GROUPNAME " group\n", optarg);
                usage();
            }
            targets[lgnum] = mode;
            flag=0;
            break;
        case 'p':
            silent = 1;
            flag=0;
            break;
        case 'h':
            usage();
            flag=0;
            break;
        default:
            usage();
            flag=0;
            break;
        }
    }
    
    if (optind != argc) {
        fprintf(stderr, "Invalid arguments\n");
        usage();
    }
    if (!(all || group)) {
            fprintf(stderr, "At least, one of -a and -g options should be specified\n");
        usage();
    }

    if(flag==1 && argc==2)
    {
      for (i = 0; i < lgcnt; i++) {
          targets[cfgtonum(i)] = mode;
      }
            flag=0;
    }
    return;
} /* parseargs */

/*
 * main - start the daemons
 */
int main (int argc, char **argv, char **envp)
{
    pid_t mpmdpid;
    pid_t pid;
    char pname[16];
    int conflicts;
    int lgnum;
    int i;
    int pstore_mode;
    int driver_mode;
    int target_found;
    int grps_started = 0;
    int pcnt;

    putenv("LANG=C");

    /* -- Make sure we're root. */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }
  
#if defined (STARTBFD)
#ifdef SUPPORT_BACKFRESH
    statstr        = "backfresh";
    mode = FTDBFD;
    sig        = FTDQBFD;
#else
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    exit(1);
#endif
#elif defined (STARTRFD)
    statstr        = "refresh";
    sig        = FTDQRFD;
    mode = FTDRFD;
#elif defined (STARTPMD)
    statstr        = "primary mirror";
    mode = FTDPMD;
    sig        = FTDQHUP;
#endif

    FTD_TIME_STAMP(FTD_DBG_FLOW1, "%s start, statstr: %s\n", argv[0], statstr);

    argv0 = argv[0];
    log_command(argc, argv);    /* trace command line in dtcerror.log */

    memset(targets, 0, sizeof(targets));

    lgcnt = GETCONFIGS(configpaths, 1, 1);
    parseargs(argc, argv);
    if (lgcnt == 0 && !silent) {
        reporterr(ERRFAC, M_NOGROUP, ERRWARN);
        exit(1);
    }
    /* a target must be specified */
    target_found = 0;
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
            target_found = 1;
            targets[lgnum] = mode;
        }
    }
    if (!target_found) {
        if (!silent) {
            reporterr(ERRFAC, M_NOGROUP, ERRWARN);
        }
        exit(1);
    }
    /* look for mode conflicts */
    conflicts = 0;

    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (!targets[lgnum]) {
            continue;
        }
/*
        readconfig(1, 1, 0, configpaths[i]);
*/
        sprintf(pname, "PMD_%03d", lgnum);
        pstore_mode = get_pstore_mode(lgnum);
        driver_mode = get_driver_mode(lgnum);
        if( (pstore_mode == FTDRFDF) && (driver_mode == FTD_MODE_FULLREFRESH) )
        {
           /* Full Refresh has been interrupted (ex: by reboot); restart it (WR 43148) */
           /* Note: if this causes side effects and if we want it only at reboot, we could add a -b option to launchpmds
                  as for dtcstart to indicate boot flag */
           mode = FTDRFD;
           sig = FTDQRFD;
           statstr = "refresh";
        }

        if (pstore_mode == FTDRFD && mode == FTDRFDF) {
            pstore_mode = FTDRFDF;
        }
        if (pstore_mode == FTDRFDF && mode == FTDRFD) {
            pstore_mode = FTDRFDF;
        }
        pid = getprocessid(pname, 0, &pcnt);


        /* Check if group is in PASSTHRU mode, reject if it is */
        if (driver_mode == FTD_MODE_PASSTHRU && (!full) && (!rflag))  
        {   
            targets[lgnum] = 0;
            grps_started = 1;
            reporterr(ERRFAC, M_ACCUMLATE, ERRWARN, lgnum);
        }

        switch(pstore_mode) {
        case FTDPMD:
            if (mode == FTDPMD) {
                if (pid > 0) {
                    reporterr(ERRFAC, M_PMDAGAIN, ERRWARN, pname);
                    targets[lgnum] = 0;
                    grps_started = 1;
                }
            } else if (mode == FTDRFD) {
                /* OK - PMD->RFD */
            } else if (mode == FTDBFD) {
                /* OK - PMD->BFD */
            }
            break;
        case FTDRFD:
            if (mode == FTDRFD) {
                if (pid > 0) {
                    reporterr(ERRFAC, M_RFDAGAIN, ERRWARN, pname);
                    targets[lgnum] = 0;
                    grps_started = 1;
                }
            } else if ((mode == FTDBFD) &&
                       (driver_mode == FTD_MODE_REFRESH)) {
                reporterr(ERRFAC, M_BFDRFD, ERRWARN, pname);
                targets[lgnum] = 0;
                conflicts = 1;
            } else if (mode == FTDPMD) {
                if (pid > 0) {
                    reporterr(ERRFAC, M_PMDAGAIN, ERRWARN, pname);
                    targets[lgnum] = 0;
                    grps_started = 1;
                } else {
                    /* OK - RFD->PMD (PMD will do the right thing)  */
                }
            }
            break;
        case FTDBFD:
            if (mode == FTDRFD) {
                reporterr(ERRFAC, M_RFDBFD, ERRWARN, pname);
                targets[lgnum] = 0;
                conflicts = 1;
            } else if (mode == FTDBFD) {
                if (pid > 0) {
                    reporterr(ERRFAC, M_BFDAGAIN, ERRWARN, pname);
                    targets[lgnum] = 0;
                    grps_started = 1;
                }
            } else if (mode == FTDPMD) {
                if (pid > 0) {
                    reporterr(ERRFAC, M_PMDAGAIN, ERRWARN, pname);
                    targets[lgnum] = 0;
                    grps_started = 1;
                } else {
                    /* OK - BFD->PMD (PMD will do the right thing)  */
                }
            }
            break;
        default:
            if ((mode == FTDRFD || mode == FTDRFDF) &&
                (driver_mode == FTD_MODE_TRACKING)) {
                /* */
            } else {
                if (pid > 0) {
                    if (mode == FTDRFDF) {
                        reporterr(ERRFAC, M_RFDAGAIN, ERRWARN, pname);
                    } else {
                        reporterr(ERRFAC, M_PMDAGAIN, ERRWARN, pname);
                    }
                    targets[lgnum] = 0;
                    grps_started = 1;
                } 
            } 
            break; 
        }
    }
    if (conflicts) {
        close(ftdsock);
        exit(1);
    }
    if (grps_started) {
        for (i = 0; i < MAXLG; i++) {
            if (targets[i]) {
                break;
            }
        }
    }
    if (i == MAXLG) {
        exit(1);
    }
    if ((mpmdpid = getprocessid(FTDD, 0, &pcnt)) <= 0) {
        reporterr(ERRFAC, M_NOMASTER, ERRWARN, argv0);
        exit(1);
    }
    if ((ftdsock = connecttomaster(FTDD, 0)) == -1) {
        exit(1);
    }
    if (ipcsend_sig(ftdsock, 0, &sig) == -1) {
        exit(1);
    }
    if (ipcsend_lg_list(ftdsock) == -1) {
        exit(1);
    }
    return 0;
} /* main */
