/****************************************************************************
 * kill.c - FullTime Data kill utility
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the high level functionality for ftdkill
 *
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <signal.h>
#include "config.h"
#include "pathnames.h"
#include "errors.h"
#include "process.h"

#ifdef DEBUG
FILE *dbgfd;
#endif

char *argv0;

static int all = 0;
static int group = 0;
static int targets[MAXLG];

char *argv0;

extern int lgcnt;

char configpaths[MAXLG][32];
char *paths = (char*)configpaths;

int action;

/****************************************************************************
 * usage -- This is how the program is used
 ****************************************************************************/
void usage(void)
{
    fprintf(stderr,"\nUsage: %s <options>\n\n", argv0);
    fprintf(stderr,"\tOne of the following two options is mandatory:\n" );
    fprintf(stderr,"\t\t-g<Logical_Group_Num> Kill a specific Logical Group\n");
    fprintf(stderr,"\t\t-a                    Kill all Logical Groups\n");
    fprintf(stderr,"\t-h\t                      Print This Listing\n");
    exit (1);
} /* usage */

/****************************************************************************
 * parseargs -- parse the run-time arguments 
 ****************************************************************************/
void
parseargs(int argc, char **argv)
{
    unsigned long lgnum = 0;
    int ch;
    int i;

    argv0 = argv[0];
    /* At least one argument is required */
    if (argc < 1) {
        usage();
    }
    /* Determine what options were specified */
    while ((ch = getopt(argc, argv, "ag:")) != -1) {
        switch (ch) {
        case 'a':
            /* change the operation mode of all devices in all logical groups */
            if (group) {
                usage();
            }
            all = 1;
            for (i = 0; i < lgcnt; i++) {
                targets[cfgtonum(i)] = 1;
            }
            break;
        case 'g':
            /* change the operation mode of all devices in a logical group */
            if (all) usage();
            group = 1;
            lgnum = strtol(optarg, NULL, 0);
            targets[lgnum] = 1;
            break;
        default:
            usage();
            break;
        }
    }
    if (!(all || group)) {
        usage();
    }
    return;
} /* parseargs */

/**************************************************************************
 * main 
 ****************************************************************************/
int main(int argc, char **argv)
{
    pid_t pid;
    char pname[16];
    int	lgnum;
    int	mode;
    int	pcnt;
    int	i, j;

    argv0 = argv[0];

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    /* Make sure we are root */
    if (geteuid()) {
        reporterr(ERRFAC, M_NOTROOT, ERRCRIT);
        exit(1);
    }
    memset(targets, 0, sizeof(targets));

#if defined(KILLRMD)
    lgcnt = getconfigs(configpaths, 2, 0);
#else
    lgcnt = getconfigs(configpaths, 1, 1);
#endif
    parseargs(argc, argv);

    /* kill targeted daemons */
#if defined(KILLPMD)
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
/*
            readconfig(1, 1, 0, configpaths[i]);
*/
            sprintf(pname, "PMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                kill(pid, SIGTERM);
                for (j = 0; j < 7; j++) {
                    if (getprocessid(pname, 1, &pcnt) == 0) {
                        break;
                    }
                    sleep(1);
                }
                if (j == 7) {
                    kill(pid, SIGKILL);
                }
            } else {
                clearmode(lgnum);
            }
        }
    }
#elif defined(KILLRMD)
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
            sprintf(pname, "RMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                kill(pid, SIGUSR1);
            }
        }
    }
#elif defined(KILLBFD)
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
/*
            readconfig(1, 1, 0, configpaths[i]);
*/
            if (get_pstore_mode(lgnum) != FTDBFD) {
                continue;
            }
            sprintf(pname, "PMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                kill(pid, SIGUSR1);
            } else {
                clearmode(lgnum);
            }
        }
    }
#elif defined(KILLRFD)
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
/*
            readconfig(1, 1, 0, configpaths[i]);
*/
            mode = get_pstore_mode(lgnum);
            if (mode != FTDRFD && mode != FTDRFDF) {
                continue;
            }
            sprintf(pname, "PMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                kill(pid, SIGUSR1);
            } else {
                clearmode(lgnum);
            }
        }
    }
#endif
    exit(0);
} /* main */

