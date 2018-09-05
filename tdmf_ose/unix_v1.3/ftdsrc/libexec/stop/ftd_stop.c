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
static char configpaths[512][32];
static int autostart;

extern char *paths;
char *argv0;

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-a | -g group_number] [-s]\n", progname);
    fprintf(stderr, "OPTIONS: -a => all groups\n");
    fprintf(stderr, "         -g => group number\n");
    fprintf(stderr, "         -s => don't clear autostart in pstore\n");
}

#if defined(HPUX)
/*
** find the control daemon and kill it,
** all the devices are closed after termination
*/
static void
kill_ctld(int group)
{
    char name[8];
    int pid, pcnt;


    sprintf(name, "CTL_%03d", group);
    if ((pid = getprocessid(name, 0, &pcnt)) > 0) {
        kill (pid, SIGKILL);
    }
}
#endif

int 
main(int argc, char *argv[])
{
    int  i, fd, group, all, lgcnt, pid, pcnt, chpid;
    char  **setargv;
    char ftdctlpath[] = PATH_BIN_FILES "/" QNM "ctl";
    char *cfgpath;
    char  ps_name[MAXPATHLEN];
#if defined(_AIX)
    int   ch;
#else /* defined(_AIX) */
    char  ch;
#endif /* defined(_AIX) */
    ps_version_1_attr_t attr;
    int rc;

    progname = argv[0];
    if (argc < 2) {
        goto usage_error;
    }

    argv0 = argv[0];

    all = 0;
    group = -1;
    memset(ps_name, 0, sizeof(ps_name));

    autostart = 1;

    /* process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "asg:")) != -1) {
        switch(ch) {
        case 'a':
            if (group != -1) {
                goto usage_error;
            }
            all = 1;
            break;
        case 'g':
            if (all != 0) {
                goto usage_error;
            }
            group = strtol(optarg, NULL, 0);
            break;
        case 's':
            autostart = 0; 
            break;
        default:
            goto usage_error;
        }
    }
    if ((all == 0) && (group < 0)) {
        goto usage_error;
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    paths = (char *)configpaths;
    lgcnt = GETCONFIGS(configpaths, 1, 1);

    if (!lgcnt) {
        reporterr(ERRFAC, M_NOGROUP, ERRWARN);
        exit(1);
    }
    if (all != 0) {
        for (i = 0; i < lgcnt; i++) {
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
            if (ps_get_version_1_attr(ps_name, &attr) != PS_OK) {
                reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
                exit(1);
            }
            rc = ftd_stop_group(ps_name, group, &attr, autostart);
#if defined(HPUX)
            if (rc >= 0)
            	kill_ctld(group);
#endif
        }
    } else {
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
        if (ps_get_version_1_attr(ps_name, &attr) != PS_OK) {
            reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
            exit(1);
        }
        rc = ftd_stop_group(ps_name, group, &attr, autostart);
#if defined(HPUX)
        if (rc >= 0)
        	kill_ctld(group);
#endif
    }
#if defined(HPUX) && defined(OLDCTLD)
    if ((pid = getprocessid(QNM "ctl", 0, &pcnt)) > 0) {
        /* signal the 'hold open' process so that it opens only our currently
           started groups */
        kill (pid, SIGUSR1);
    } else {
        /* if it isn't there, it should be, so exec it. */
        setargv = (char **) ftdmalloc (sizeof(char *) * 2);
        setargv[0] = (char *) ftdmalloc (sizeof(ftdctlpath));
        strcpy (setargv[0], ftdctlpath);
        setargv[1] = (char *) NULL;
        chpid = fork();
        switch (chpid) {
        case 0 : 
            /* child */
            execv(ftdctlpath, setargv);
        default:
            exit(0);
        }
    }
#endif
    exit(0);
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


