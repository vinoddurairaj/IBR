/***************************************************************************
 * updatepmd.c - FullTime Data update PMD utility
 *
 * (c) Copyright 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the high level functionality for ftdupdatepmd
 *
 ***************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/param.h>
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

/* Assume all options are off. Set default values */
char *argv0;
static int all = 0;
static int group = 0;

extern int targets[MAXLG];
extern int ftdsock;
extern int lgcnt;

char configpaths[MAXPATH][32];
char *paths = (char*)configpaths;

/****************************************************************************
 * usage -- This is how the program is used
 ****************************************************************************/
void usage(void)
{
    fprintf(stderr,"\nUsage: %s <options>\n\n", argv0);
    fprintf(stderr,"\tOne of the following two options is mandatory:\n" );
    fprintf(stderr,"\t\t-g<Logical_Group_Num>	Update a specific Logical Group PMD\n");
    fprintf(stderr,"\t\t-a			Update all Logical Group PMDs\n");
    fprintf(stderr,"\t-h\t		Print This Listing\n");
    exit (1);
} /* usage */

/****************************************************************************
 * parseargs -- parse the run-time arguments 
 ****************************************************************************/
void
parseargs(int argc, char **argv)
{
    int lgnum;
    int ch;
    int i;

    argv0 = argv[0];
    /* At least one argument is required */
    if (argc == 1) {
        usage();
    }
    /* Determine what options were specified */
    while ((ch = getopt(argc, argv, "ag:q:")) != -1) {
        switch (ch) {
        case 'a':
            /* change the operation mode of all devices in all logical groups */
            if (group) {
                usage();
            }
            all = 1;
            for (i = 0; i < lgcnt; i++) {
                targets[cfgtonum(i)] = FTDPMD;
            }
            break;
        case 'g':
            /* change the operation mode of all devices in a logical group */
            if (all) {
                usage();
            }
            group = 1;
            lgnum = strtol(optarg, NULL, 0);
            targets[lgnum] = FTDPMD;
            break;
        default:
            usage();
        }
    }
    return;
} /* parseargs */

/****************************************************************************
 * main - send a state change msg to master daemon
 ****************************************************************************/
int main(int argc, char **argv)
{
    int	n;

    if (initerrmgt(ERRFAC) < 0) {
	exit(1);
    }

    /* Make sure we are root */
    if (geteuid()) {
        reporterr(ERRFAC, M_NOTROOT, ERRCRIT);
        exit(1);
    }
    lgcnt = getconfigs(configpaths, 1, 1);
    memset(targets, 0, sizeof(targets));
    parseargs(argc, argv);

    if ((ftdsock = connecttomaster(FTDD, 0)) == -1) {
        exit(1);
    }
    n = FTDQHUP;
    if (ipcsend_sig(ftdsock, 0, &n) == -1) { 
        exit(1);
    }
    n = FTDPMD;
    if (ipcsend_lg_list(ftdsock) == -1) { 
        exit(1);
    }
    exit (0);
} /* main */
