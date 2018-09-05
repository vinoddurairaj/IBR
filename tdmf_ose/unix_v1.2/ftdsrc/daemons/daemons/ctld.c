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

#include <syslog.h>
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
#include "ftd_mount.h"
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

/*-
 * control_daemon_sig_handler()
 *
 * handle caught signals
 */
static void
control_daemon_sig_handler(int s)
{
	/* noop */
	return;
} 

/*-
 * control_daemon_installsigaction()
 *
 * install a signal handler
 */
static void
control_daemon_installsigaction(void)
{
    int i;
    int j;
    int numsig;
    struct sigaction caction;
    sigset_t block_mask;
    static int csignal[] = { SIGUSR1, SIGTERM, SIGPIPE };

    sigemptyset(&block_mask);
    numsig = sizeof(csignal)/sizeof(*csignal);

    for (i = 0; i < numsig; i++) {
        for (j = 0; j < numsig; j++) {
            sigaddset(&block_mask, csignal[j]);
        }
        caction.sa_handler = control_daemon_sig_handler;
        caction.sa_mask = block_mask;
        caction.sa_flags = SA_RESTART;
        sigaction(csignal[i], &caction, NULL);
    } 
    return;
} 

/*-
 * control_daemon_prime()
 * just luff around, keeping files open.
 */
static void
control_daemon_prime() 
{
	/* dissociate */
	setsid();
	chdir("/");
	umask(0);

	/* define signal handler/action */
	control_daemon_installsigaction();

	/* existential nightmare, just wait to die... */
	while(1) {
		pause();
	};
	/* NOTREACHED */
}

main(argc, argv)
	int argc;
	char **argv;
{
	putenv("LANG=C");

	if (geteuid() != 0) {		/* WR16626 */
		exit(1);		/* WR16626 */
	}				/* WR16626 */
	control_daemon_prime();
	/* NOTREACHED */
}
