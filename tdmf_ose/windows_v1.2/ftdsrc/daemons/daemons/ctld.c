/***************************************************************************
 * ftdctl.c - This daemon is a temporary workaround for a problem involving
 *            opening and closing devices from the HP/UX driver.
 *            We need to keep the open count greater than 0 for all local
 *            data devices and the pstore.  Otherwise, the kernel may close
 *            these devices while the driver is attmempting to use them, and
 *            we'll panic the system.
 *
 *            This daemon will no longer be necessary once we obtain info
 *            from HP on how we can modify the reference counts or do some
 *            type of layered open call in the driver.
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#if defined(SOLARIS)
#include <sys/procfs.h>
#elif defined(HPUX)
#include <sys/pstat.h>
#endif
#include <ctype.h>
#include <stdio.h>

#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h"
#else
#include <sys/mnttab.h>
#endif
#include <fcntl.h>
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "common.h"
#include "process.h"
#include "network.h"
#include "cfg_intr.h"

static char configpaths[MAXLG][32];
static void keepdevsopen(void);

/* we should fix this  */
extern char* paths; 

int new_open_devs[1024];
int old_open_devs[1024];
int num_open_devs = 0;
int locked = 0;

/****************************************************************************
 * daemon_init -- initialize process as a daemon 
 ***************************************************************************/
int
daemon_init(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        return -1;
    } else if (pid != 0) {
        exit (0);
    }
    setsid();
    chdir("/");
    umask(0);

    return 0;
} /* daemon_init */
/****************************************************************************
 * sig_handler
 ***************************************************************************/
static void
sig_handler(int s)
{
    switch(s) {
    case SIGUSR1:
        error_tracef( TRACEINF, "ftdctl(): caught SIGUSR1 ***");
        keepdevsopen();
    default:
        break;
    }
    return;
} /* sig_handler */

/****************************************************************************
 * installsigaction -- installs signal action
 ***************************************************************************/
static void
installsigaction(void)
{
    int i;
    int numsig;
    struct sigaction msaction;
    sigset_t block_mask;
    static int msignal[] = { SIGTERM, SIGUSR1 };
 
    sigemptyset(&block_mask);
   
    numsig = sizeof(msignal) / sizeof(*msignal);

    for (i = 0; i < numsig; i++) {
        msaction.sa_handler = sig_handler;
        msaction.sa_mask = block_mask;
        msaction.sa_flags = SA_RESTART;
        sigaction(msignal[i], &msaction, NULL);
    }
   
    return;
} /* installsigaction */

/****************************************************************************
 * keepdevsopen -- opens all local bdevs for all logical groups and pstore, 
 *                 then closes any open fd from last time this function was
 *                 called.
 ***************************************************************************/
static void
keepdevsopen(void)
{
    int lgcnt, i, j, group;
    char configpaths[512][32];
    char cfg_path[32];
    char ps_name[MAXPATHLEN];
    sddisk_t *curdev;
    char blockdevpath[MAXPATHLEN];

    error_tracef( TRACEINF, "ftdctl(): keepdevsopen ***" );
    /* don't allow this routined to be called when it is already running */
    if (locked) { 
	error_tracef( TRACEINF, "ftdctl(): locked ***" );
	return;
}	
    locked = 1;

    /* go through each *started* LG, opening each's block data device */
    /* config.c bogusity */
    paths = (char*)configpaths;

    lgcnt = getconfigs(configpaths, 1, 1);
    j = 0;
    for (i = 0; i < lgcnt; i++) {
        group = cfgtonum(i);
        sprintf(cfg_path, "%s%03d.cfg", PMD_CFG_PREFIX, group);
        error_tracef( TRACEINF, "ftdctl: cfgpath = %s ***", cfg_path ); 
        readconfig(1, 0, 0, cfg_path);
        /* open each local data device in this group */
        curdev = sys[0].group->headsddisk;
        while (curdev) {
#if defined(HPUX)
            convert_lv_name((char *)blockdevpath, (char*) curdev->devname, 0);
#endif
            error_tracef( TRACEINF, "ftdctl: opening local data %s ***", blockdevpath );
            new_open_devs[j] = open(blockdevpath, O_RDWR);
            curdev = curdev->n;
            j++;
        }
        if (getpsname(group, ps_name) == 0) {
           new_open_devs[j] = open (ps_name, O_RDWR);
           j++;
        }
    }
   
    /* close all devs that we had open previously */
    for (i=0; i<num_open_devs; i++) {
	error_tracef( TRACEINF, "ftdctl: closing % of % ***", i, num_open_devs );
        close(old_open_devs[i]);
    }
    num_open_devs = j;
    error_tracef( TRACEINF, "ftdctl: number of open devs %d ***", j );
    /* copy newly opened file descriptors to old open array */
    memcpy(old_open_devs, new_open_devs, 1024 * sizeof(int));
    
    locked = 0;
}
  
/****************************************************************************
 * main
 ***************************************************************************/
int
main (int argc, char** argv, char **environ)
{
    pid_t pid;
    int pcnt; 

    /* -- Make sure we're not already running -- */
    if ((pid = getprocessid("ftdctl", 0, &pcnt)) > 0) {
        if (pid != getpid() || pcnt > 1) {
            exit(0);
        }
    } 
    /* -- Make sure we're root -- */
    if (geteuid()) {
        exit(1);
    }

    /* install signal handler */
    installsigaction();

   /* make this a daemon */
    daemon_init();

    /* set group id */
    setpgid(getpid(),getpid());

    keepdevsopen();
   
    while (1) {
        pause();
    }
    return (0);
} /* main */
