/*
 * ftd_stop.c - Stop one or more logical groups
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"

#if defined(_DEBUG)
#include <conio.h>
#endif
#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_config.h"
#include "ftd_proc.h"
#include "ftd_ps.h"
#include "ftd_lg.h"
#include "ftd_sock.h"
#include "ftd_devlock.h"
#include "ftd_mngt.h"


//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);

static int autostart;

static void
usage(char **argv)
{
    fprintf(stderr, "Usage: %s [-a | -g group_number] [-s]\n", argv[0]);
    fprintf(stderr, "OPTIONS: -a => all groups\n");
    fprintf(stderr, "         -g => group number\n");
    fprintf(stderr, "         -s => don't clear autostart in pstore\n");
}

int 
main(int argc, char *argv[])
{
    int     all, group, lgcnt;
    char  ch;
    ftd_lg_cfg_t    **cfgpp, *cfgp;
    ftd_lg_t        *lgp;
    LList           *cfglist = NULL;

////------------------------------------------
    void ftd_util_force_to_use_only_one_processor();
    ftd_util_force_to_use_only_one_processor();
//------------------------------------------

    if (argc < 2) 
    {
        goto usage_error;
    }

    all = 0;
    group = -1;
    autostart = 1;

    /* process all of the command line options */
    while ((ch = getopt(argc, argv, "asg:")) != -1) 
    {
        switch(ch) 
        {
            case 'a':
                if (group != -1) 
                    goto usage_error;

                all = 1;
                break;
            case 'g':
                if (all != 0) 
                    goto usage_error;

                group = strtol(optarg, NULL, 0);
                break;
            case 's':
                autostart = 0; 
                break;
            default:
                goto usage_error;
        }
    }

    if ((all == 0) && (group < 0)) 
    {
        goto usage_error;
    }

    if (ftd_sock_startup() == -1) 
    {
        error_tracef( TRACEERR, "Stop:main()", "Calling ftd_sock_startup()" );
        goto errexit;
    }

    if (ftd_dev_lock_create() == -1) 
    {
        error_tracef( TRACEERR, "Stop:main()", "Calling ftd_dev_lock_create()" );
        goto errexit;
    }


    if (ftd_init_errfac( "Replicator", argv[0], NULL, NULL, 0, 1) == NULL) 
    {
        error_tracef( TRACEERR, "Stop:main()", "Calling ftd_init_errfac()" );
        goto errexit;
    }

    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);

    // create config file list
    if ((cfglist = ftd_config_create_list()) == NULL) 
    {
        error_tracef( TRACEERR, "Stop:main()", "Calling ftd_config_create_list()" );
        goto errexit;
    }

    // get all primary config files
    if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) 
    {
        error_tracef( TRACEERR, "Stop:main()", "Calling ftd_config_get_primary_started()" );
        goto errexit;
    }

    // if none exist then exit  
    if ((lgcnt = SizeOfLL(cfglist)) == 0) 
    {
        reporterr(ERRFAC, M_NOGROUP, ERRWARN);
        goto errexit;
    }

    if (all != 0) 
    {
        ForEachLLElement(cfglist, cfgpp) 
        {
            cfgp = *cfgpp;

            if ((lgp = ftd_lg_create()) == NULL) 
            {
                error_tracef( TRACEERR, "Stop:main()", "Calling ftd_lg_create()" );
                goto errexit;
            }

//          if (ftd_lg_init(lgp, cfgp->lgnum, ROLEPRIMARY, 1) < 0) 
			if (ftd_lg_init(lgp, cfgp->lgnum, ROLEPRIMARY, 2) < 0) // Verify Drive Locking
            {
                ftd_lg_delete(lgp);
                goto errexit;
            }

            if (ftd_lg_rem(lgp, autostart, 0) < 0) 
            {
                reporterr(ERRFAC, M_STOPGRP, ERRCRIT, cfgp->lgnum);
                ftd_lg_delete(lgp);
                goto errexit;
            }
            ftd_lg_delete(lgp);
        }
    } 
    else 
    {
        if ((lgp = ftd_lg_create()) == NULL) 
        {
            error_tracef( TRACEERR, "Stop:main()", "Calling ftd_lg_create()" );
            goto errexit;
        }

//      if (ftd_lg_init(lgp, group, ROLEPRIMARY, 1) < 0) 
		if (ftd_lg_init(lgp, group, ROLEPRIMARY, 2) < 0) // Verify Drive Locking
        {
            ftd_lg_delete(lgp);
            error_tracef( TRACEERR, "Stop:main()", "Calling ftd_lg_init()" );
            goto errexit;
        }

        if (ftd_lg_rem(lgp, autostart, 0) < 0) 
        {
            reporterr(ERRFAC, M_STOPGRP, ERRCRIT, group);
            ftd_lg_delete(lgp);
            goto errexit;
        }
        ftd_lg_delete(lgp);
    }

    if (cfglist) 
    {
        ftd_config_delete_list(cfglist);
    }

    SendAllPendingMessagesToCollector();

    ftd_delete_errfac();
    ftd_dev_lock_delete();
    ftd_sock_cleanup();

    exit(0);
    return 0; /* for stupid compiler */

usage_error:
    usage(argv);

errexit:

    if (cfglist) 
    {
        ftd_config_delete_list(cfglist);
    }

    SendAllPendingMessagesToCollector();

    ftd_delete_errfac();
    ftd_dev_lock_delete();
    ftd_sock_cleanup();

    exit(1);
    return 0; /* for stupid compiler */
}
