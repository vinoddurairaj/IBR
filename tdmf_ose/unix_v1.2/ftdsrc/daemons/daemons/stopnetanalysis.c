/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2012  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/****************************************************************************
 * stopnetanalysis.c - Stop PMDs-RMDs in Network analysis mode
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

#ifdef TDMF_TRACE
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
    fprintf(stderr,"\nUsage: %s <options>\n", argv0);
    fprintf(stderr,"One of the following two options is mandatory:\n" );
    fprintf(stderr,"\t-g <group#> : stop a specific network analysis group (0 - %d)\n", MAXLG-1);
    fprintf(stderr,"\t-a          : stop all network analysis groups\n");
    fprintf(stderr,"\t-h          : print this Listing\n\n");
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
    if (argc < 2) {
        usage();
    }
    /* Determine what options were specified */
    opterr = 0;
    while ((ch = getopt(argc, argv, "ag:")) != -1) {
        switch (ch) {
        case 'a':
            /* Apply to all network analysis groups */
            if (group) {
		        fprintf(stderr, "Only one of -a and -g options should be specified\n");
                usage();
            }
            all = 1;
            for (i = 0; i < lgcnt; i++) {
                targets[cfgtonum(i)] = 1;
            }
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
            targets[lgnum] = 1;
            break;
        default:
            usage();
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
	int found_group = 0;

    putenv("LANG=C");

    /* -- Make sure we're root. */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    argv0 = argv[0];

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    log_command(argc, argv);   /* trace command line in dtcerror.log */

    memset(targets, 0, sizeof(targets));

    // Get the list of groups running in Network analysis mode.
    lgcnt = GETCONFIGS(configpaths, 3, 0);

    if( lgcnt == 0 )
	{
	    reporterr( ERRFAC, M_NONETGRPSTARTED, ERRWARN );
        exit(1);
	}

    parseargs(argc, argv);

	found_group = 0;
    /* kill targeted daemons */
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
            sprintf(pname, "PMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
			    found_group = 1;
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
            }
        }
    }

    if( found_group )
	{
	    // Wait for the specified group(s) to be completely stopped
		for (i = 0; i < lgcnt; i++) {
	        lgnum = cfgtonum(i);
	        if (targets[lgnum]) {
	            sprintf(pname, "PMD_%03d", lgnum);
				while ((getprocessid(pname, 1, &pcnt)) != 0) {
					sleep(1);
				}
			}
		}
	}
	else
	{
	    reporterr( ERRFAC, M_NONETGRPSTARTED, ERRWARN );
        exit(1);
	}

    return 0;
} /* main */

