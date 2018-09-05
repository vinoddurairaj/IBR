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
 * ftd_fo.c - Module to send failover command to the target (RMD) server
 ***************************************************************************/
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>

#if defined(SOLARIS)
#include <sys/procfs.h>
#elif defined(HPUX)
#include <sys/pstat.h>
#endif

#include <stdio.h>

#ifdef NEED_SYS_MNTTAB
#include "ftd_mount.h"
#else
#include <sys/mnttab.h>
#endif


#include <fcntl.h>
#include "ftdio.h"
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "common.h"
#include "process.h"
#include "ipc.h"
#include "ftdif.h"
#include "stat_intr.h"

extern int sendfailover (int fd, int group_number, int boot_drive_migration, int shutdown_source, int keep_AIX_target_running);

char *argv0;

#if defined(SOLARIS) /* 1LG-ManyDEV */ 
extern int dummyfd;
#endif

static struct timeval skosh;
static char configpaths[MAXLG][32];
static int boot_drive_migration = 0;
static int shutdown_source = 0;
static int keep_AIX_target_running = 0;
static sigset_t sigs;

extern char *paths;

/****************************************************************************
 * usage
 ***************************************************************************/
static void
usage(void)
{
    fprintf(stderr, "Usage:  -g <group#> [-b] [-s]\n");
    fprintf(stderr, "OPTION: -g   => group number (0 - %d)\n", MAXLG-1);
    fprintf(stderr, "        -b   => indicate Linux boot drive migration\n");
    fprintf(stderr, "        -s   => indicates that the Linux source server is rebooted (shutdown -r) after source failover part\n");
	return;
}

/****************************************************************************
 * main
 ***************************************************************************/
int 
main(int argc, char **argv)
{
    char path[FILE_PATH_LEN];
    int firsttry;
    int result;
    int lgnum;
    int tries;
    int cfgidx;
    int rc;
    int i;
    char *p;
    char group_name[MAXPATHLEN];
    char pname[16];
    int addrflag;
	int ch;
    
    void *voidptr = NULL;

    putenv("LANG=C");

    /* -- Make sure we're root -- */
    if (geteuid() != 0) {
        exit(1);
   	}

    /* reset open file desc limit */ 
    setmaxfiles();
    if (initerrmgt(ERRFAC) < 0) { 
        exit(1);
    }

    setsyslog_prefix("%s", argv[0]);
    p = getenv(LOGINTERNAL);
    if (p)
        log_internal = atoi(p);
    reporterr(ERRFAC, M_INTERNAL, ERRINFO, "dtcsendfailover started.");

    log_command(argc, argv);  /* trace command line in dtcerror.log */

#if defined(SOLARIS)  /* 1LG-ManyDEV */ 
    /*Dummy open for gethostbyname  / gethostbyaddr */
    if ((dummyfd = open("/dev/null", O_RDONLY)) == -1) {
	    reporterr(ERRFAC, M_FILE, ERRWARN, "/dev/null", strerror(errno));
     }
#endif

    if (argc < 2) {
        usage();
        exit(1);
    }
    argv0 = argv[0];

    lgnum = -1;
	boot_drive_migration = 0;
	shutdown_source = 0;
	keep_AIX_target_running = 0;

    /* Process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "g:bsk")) != -1) {
        switch(ch) {
        case 'g':
            lgnum = ftd_strtol(optarg);

            if (lgnum < 0 || lgnum >= MAXLG) {
                fprintf(stderr, "[%s] is invalid number for " GROUPNAME " group\n", optarg);
		        usage();
		        exit(1);
            }
            break;
        case 'b':
            boot_drive_migration = 1;
			break;
        case 's':
            shutdown_source = 1;
			break;
        case 'k':
            keep_AIX_target_running = 1;
			break;
        default:
	        usage();
	        exit(1);
            break;
		}
    }

    if (optind != argc) {
        fprintf(stderr, "Invalid arguments\n");
        usage();
        exit(1);
    }

    GETCONFIGS(configpaths, 1, 1);
    paths = (char*)configpaths;

    skosh.tv_sec = 10;
    skosh.tv_usec = 0;

    sprintf(path, "%s%03d.cur", PMD_CFG_PREFIX, lgnum);
    if ((rc = readconfig(1, 1, 0, path)) != 0) {
        exit(1);
    }

    if (0 != (rc = initnetwork ())) {
        exit(1);
    }
    tries = 0;
    firsttry = 1;
    while (1) {
        rc = 0;
        if (!(rc = createconnection (firsttry))) {
            break;
        }
        (void) select(0, NULL, NULL, NULL, &skosh);
        if (++tries == 90) {
            reporterr(ERRFAC, M_RECONN, ERRWARN);
            tries = 0;
        }
        firsttry = 0;
    }

    if (1 != (rc = sendfailover(mysys->sock, lgnum, boot_drive_migration, shutdown_source, keep_AIX_target_running))) {
        exit(1);
    }
	exit(0);

}
