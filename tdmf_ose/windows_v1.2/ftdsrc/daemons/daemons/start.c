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

#ifdef DEBUG
FILE *dbgfd;
#endif

char *argv0;
static int all = 0;
static int group = 0;
static int silent = 0;
static int mode;
static int sig;
static char *statstr;

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

    fprintf(stderr,"\nUsage: %s <options>\n\n", argv0);
    fprintf(stderr,"\tOne of the following three options is mandatory:\n" );
    fprintf(stderr,"\t\t-g<Logical_Group_Num>	Put all devices in a Logical Group into %s mode\n", statstr);
    fprintf(stderr,"\t\t-a			Put all devices into %s mode\n", statstr);
#ifdef STARTRFD
    fprintf(stderr,"\t-f\t\t		Perform Initial Sync - Full Refresh\n");
    fprintf(stderr,"\t-c\t\t		Perform Verify Secondary - Full Checksum\n");
#endif
    fprintf(stderr,"\t-h\t\t		Print This Listing\n");
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

    argv0 = argv[0];
    /* At least one argument is required */
    if (argc == 1) {
        usage();
    }
    /* Determine what options were specified */
    devcnt = 0;

#ifdef STARTRFD
    while ((ch = getopt(argc, argv, "fapcg:")) != -1) {
        switch (ch) {
        case 'f':
            /* perform Full Sync - No dirtybit/chksum */
            sig	= FTDQRFDF;
            mode = FTDRFDF;
            break;
        case 'c':
            /* perform verify secondary - All dirtybit/chksum */
            sig	= FTDQRFDC;
            mode = FTDRFD;
            break;
#else
    while ((ch = getopt(argc, argv, "fapg:")) != -1) {
        switch (ch) {
#endif
        case 'a':
            /* change the operation mode of all devices in all logical groups */
            if (group) {
                usage();
            }
            all = 1;
            for (i = 0; i < lgcnt; i++) {
                targets[cfgtonum(i)] = mode;
            }
            break;
        case 'g':
            /* change the operation mode of all devices in a logical group */
            if (all) {
                usage();
            }
            group = 1;
            lgnum = strtol(optarg, NULL, 0);
            targets[lgnum] = mode;
            break;
        case 'p':
            silent = 1;
            break;
        case 'h':
            usage();
            break;
        default:
            usage();
            break;
        }
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

#if defined (STARTBFD)
    statstr	= "backfresh";
    mode = FTDBFD;
    sig	= FTDQBFD;
#elif defined (STARTRFD)
    statstr	= "refresh";
    sig	= FTDQRFD;
    mode = FTDRFD;
#elif defined (STARTPMD)
    statstr	= "primary mirror";
    mode = FTDPMD;
    sig	= FTDQHUP;
#endif
    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    /* Make sure we are root */
    if (geteuid()) {
        reporterr(ERRFAC, M_NOTROOT, ERRCRIT);
        exit(1);
    }
    memset(targets, 0, sizeof(targets));

    lgcnt = getconfigs(configpaths, 1, 1);
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

        if (pstore_mode == FTDRFD && mode == FTDRFDF) {
            pstore_mode = FTDRFDF;
        }
        if (pstore_mode == FTDRFDF && mode == FTDRFD) {
            pstore_mode = FTDRFD;
        }
        pid = getprocessid(pname, 0, &pcnt);
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
            } else if (mode == FTDBFD
            && driver_mode == FTD_MODE_REFRESH) {
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
            if ((mode == FTDRFD || mode == FTDRFDF)
            && driver_mode == FTD_MODE_TRACKING) {
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
    exit(0);
} /* main */
