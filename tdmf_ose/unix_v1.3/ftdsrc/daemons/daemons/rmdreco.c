/***************************************************************************
 * rmdreco.c - FullTime Data - Recovers data from partially restarted rmd.
 *
 * (c) Copyright 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * Recovery for the case where pmd goes away, starts to come back and then
 * dies while it is coming back.
 *
 * Also has the responsibility of doing all failover functions.
 *
 * History:
 *   3/1/97 - Warner Losh - original code
 *
 ***************************************************************************
 */

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif
#include "platform.h"

#include <stdlib.h>
#include <signal.h>
#include "errors.h"
#include "config.h"
#include "network.h"
#include "licplat.h"
#include "pathnames.h"
#include "process.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

FILE* dbgfd;

int aflag = 0;			/* Do all the recovery files that we find */
int gflag = 0;			/* Do one logical group */
int dflag = 0;			/* Disable failover */
int vflag = 0;			/* Verbose output */
long lgnum;
char *argv0 = QNM "rmdreco";
char group[16];

char configpaths[MAXLG][32];
char *paths;

void recoverfile(char *fn);
void recoverfiles(void);
void usage(void);

void applygroups(char**);
void restartrmds(void);

extern char *databuf;
extern char *_pmd_configpath;
extern char **pmdenviron;
extern int pmdenvcnt;

char **pmdargv;
int ngroups;

int
main (int argc, char** argv)
{
    pid_t pid;
    int pcnt;
    int ch;
    int i;

    /* -- Make sure we're root. */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this program\n");
        exit(1);
    }
    if (initerrmgt (ERRFAC) < 0) {
        exit(1);
    }

    /* -- Parse the args here */
    opterr = 0;
    while ((ch = getopt(argc, argv, "adg:v")) != -1) {
        switch (ch) {
        case 'a':
            aflag = 1;
            break;
        case 'd':
            dflag = 1;
            break;
        case 'g':
            gflag = 1;
            lgnum = strtol(optarg, NULL, 10);
            break;
        case 'v':
            vflag = 1;
            break;
        default:
            usage();
        }
    }
    if (aflag == 0 && gflag == 0) {
        fprintf(stderr, "You must specify -a or -g.\n");
        usage();
    }
    if (aflag != 0 && gflag != 0) {
        usage();
    }
    if (argc > optind) {
        fprintf(stderr, "Invalid arguments\n");
        usage();
    }
    /* -- OK, now we go and recover */
    applygroups(argv);

    /* -- Tell the pmds to restart */
    restartrmds();

    exit(0);	/* XXX should report better errors */
}

void
applygroups(char **argv)
{
    pid_t pid;
    char fn[MAXPATHLEN];
    int status;
    int fd;
    int i;
    int j;
    long thislg;
    char *dev;

    ngroups = GETCONFIGS(configpaths, 2, 0);
    paths = (char*)configpaths;

    pmdenviron = (char**)ftdmalloc(5*sizeof(char*));
    pmdargv = (char**)ftdmalloc(2*sizeof(char*));
    pmdenvcnt = 0;
 
    for (i = 0; i < ngroups; i++) {
        dev = configpaths[i];
        dev = dev + strlen(dev) - 7;
        thislg = strtol(dev, NULL, 10);
        sprintf(fn, "%s/s%3.3s.off", PATH_CONFIG, dev);

        if (aflag || (thislg == lgnum && gflag)) {
            if (vflag)
                printf("Recovering logical group %s\n", configpaths[i]);
            if (dflag) {
                unlink(fn); 
            } else {
                /* create a .off file for this group */
                if ((fd = open(fn, O_RDONLY | O_CREAT)) == -1) {
                     reporterr(ERRFAC, M_RECOOFF, ERRWARN, thislg, strerror(errno));
                }
                close(fd);
                /* exec RMDA for this group - to apply journal->mirror */
                pid = startapply(thislg, FTDQAPPLY, pmdargv);
            }
        }
    }
}

void
restartrmds(void)
{
    pid_t pid;
    char pname[32];
    int i;
    int pcnt;
    long thislg;
    char *dev;

    ngroups = GETCONFIGS(configpaths, 2, 0);
    paths = (char*)configpaths;

    for (i = 0; i < ngroups; i++) {
        dev = configpaths[i];
        dev = dev + strlen(dev) - 7;
        thislg = strtol(dev, NULL, 10);

        if (aflag || (thislg == lgnum && gflag)) {
            if (vflag)
                printf("Restarting logical group %s\n", configpaths[i]);
            if (dflag) {
                sprintf(pname, "RMD_%03d",cfgtonum(i));
                if ((pid = getprocessid(pname, 0, &pcnt)) > 0) {
                    kill(pid, SIGTERM);
                }
            } else {
                sprintf(pname, "RMD_%03d",cfgtonum(i));
                if ((pid = getprocessid(pname, 0, &pcnt)) > 0) {
                    kill(pid, SIGTERM);
                }
            }
        }
    }
}

void
usage()
{
    fprintf(stderr, "usage: " QNM "rmdreco <options>\n");
    fprintf(stderr,"\
Exactly one of the following two options is mandatory:\n\
\t-g<logical_group_num>  : Fail over logical group.\n\
\t-a                     : Fail over all logical groups\n\
The following options modify the behavior:\n\
\t-d                     : Disable the fail over (go back to primary)\n\
\t-v                     : Verbose mode\n"
    );
    exit(1);
}
