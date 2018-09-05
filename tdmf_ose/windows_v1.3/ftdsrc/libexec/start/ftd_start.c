/*
 * ftd_start.c - Start one or more logical groups
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * This module initializes a logical group. It reads the config file
 * for the group and then checks the persistent store for state information.
 * If the PS doesn't have any data for this group, we add the group 
 * and its devices to the PS and assume the group is new one. If a device
 * doesn't exist in the PS, we assume it is a new device and set the 
 * appropriate defaults for it. 
 *
 * After reading/setting up of the group/device info from the PS, we 
 * load the driver with the group and device information, including
 * state and dirty bit maps. The device files for the group and its
 * devices are also created at this time. The logical group is now
 * ready for action.
 *
 */

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"

#if defined(_DEBUG)
#include <conio.h>
#endif

#include "ftd_port.h"
#include "ftd_config.h"
#include "ftd_lg.h"
#include "ftdio.h"
#include "ftd_ps.h"
#include "ftd_error.h"
#include "ftd_sock.h"
#include "ftd_stat.h"
#include "ftd_devlock.h"
#include "ftd_mngt.h"

//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);
static int          autostart;
static int start_group(int group, int force, int autostart);

static void
usage(char **argv)
{
    fprintf(stderr, "Usage: %s [-a | -g group_number] [-f] [-b]\n", argv[0]);
    fprintf(stderr, "OPTIONS: -a => all groups\n");
    fprintf(stderr, "         -g => group number\n");
    fprintf(stderr, "         -f => force\n");
    fprintf(stderr, "         -b => don't set automatic start state\n");
}

static int
start_group(int group, int force, int autostart)
{
    int         infile, outfile, n;
    char        copybuf[BUFSIZ], cfg_path[32], cur_cfg_path[32];
    char        full_cfg_path[MAXPATHLEN], full_cur_cfg_path[MAXPATHLEN];
    ftd_lg_t    *lgp;

    /*  pXXX.cfg is the user created version of the configuration, while
        pXXX.cur is the private copy of the configuration that will be used
        by future readconfigs so that other utilities will be working with
        the same version of the configuration that we start with here.  
        This allows the user to edit the pXXX.cfg files without affecting 
        anything until a subsequent start is run. */
    
    sprintf(cfg_path, "%s%03d.cfg", PRIMARY_CFG_PREFIX, group);
    sprintf(cur_cfg_path, "%s%03d.cur", PRIMARY_CFG_PREFIX, group);
    sprintf(full_cfg_path, "%s/%s", PATH_CONFIG, cfg_path);
    sprintf(full_cur_cfg_path, "%s/%s", PATH_CONFIG, cur_cfg_path);

    /*
     * unlink the old .cur file in case we starting after an unorderly 
     * shutdown.  (ftdstop should remove this file otherwise.)
     *
     * DTurrin - Sept 13th, 2001
     * Changed from "unlink" command to "access". If the file
     * exists, return an error message because this might be
     * an attempt to start an already started group.
     */

    if ( ( _access( full_cur_cfg_path , 0 ) == 0 ) && !force ) 
    {
        fprintf(stderr, "%s still exists. Group %03d is already started.\n",
                cur_cfg_path, group);
        return -1;
    }

    if ((lgp = ftd_lg_create()) == NULL) 
    {
        error_tracef( TRACEERR, "start_group()", "Calling ftd_lg_create()" );
        goto errret;
    }
    
	if (ftd_lg_init(lgp, group, ROLEPRIMARY, 0) < 0) 
    {
        error_tracef( TRACEERR, "start_group()", "Calling ftd_lg_init()" );
        goto errret;
    }
    
    if (ftd_lg_add(lgp, autostart) < 0) 
    {
        error_tracef( TRACEERR, "start_group()", "Calling ftd_lg_add()" );
        goto errret;
    }
    
    // reset checkpoint state flags 
    UNSET_LG_CPON(lgp->flags);
    UNSET_LG_CPPEND(lgp->flags);
    UNSET_LG_CPSTART(lgp->flags);
    UNSET_LG_CPSTOP(lgp->flags);
    UNSET_LG_RFDONE(lgp->flags);

    // set checkpoint off in pstore
    if (ps_set_group_checkpoint(lgp->cfgp->pstore, lgp->devname, 0) < 0) 
    {
        goto errret;
    }

    ftd_stat_connection_driver(lgp->ctlfd, lgp->lgnum, -1);

    /* now copy the .cfg file for this group to .cur */
    infile = open(full_cfg_path, O_RDONLY, 0);
    if (infile != -1) 
    {
        if ((outfile = creat(full_cur_cfg_path,  S_IRUSR | S_IWUSR)) != -1) 
        {
            while ((n = read(infile, copybuf, BUFSIZ)) > 0) 
            {
                if (write(outfile, copybuf, n) != n) 
                {
                    fprintf(stderr, "Failed copy of %s to %s - write failed.\n", 
                            cfg_path, cur_cfg_path);
                    goto errret;
                }
            }
            close(outfile);
        } 
        else 
        {
            fprintf(stderr, "Failed copy of %s to %s - couldn't create %s\n", 
                    cfg_path, cur_cfg_path, cur_cfg_path);
            goto errret;
        }
        close(infile);
    } 
    else 
    {
        fprintf(stderr, "Failed copy of %s to %s - couldn't open %s\n", 
                full_cfg_path, full_cur_cfg_path, full_cfg_path);
    }
    
    ftd_lg_delete(lgp);

    return 0;

errret:

    ftd_lg_delete(lgp);

    return -1;
}


/*
 * main
 */
int 
main(int argc, char *argv[])
{
    char ch;
    int             all, force, group, exitcode = 0;
    char            cfgpath[MAXPATH];
    LList           *cfglist = NULL;
    ftd_lg_cfg_t    **cfgpp, *cfgp;

////------------------------------------------
    void ftd_util_force_to_use_only_one_processor();
    ftd_util_force_to_use_only_one_processor();
////------------------------------------------

    if (argc < 2) 
    {
        goto usage_error;
    }

    all = 0;
    force = 0;
    group = -1;
    autostart = 0;

    // process all of the command line options 
    while ((ch = getopt(argc, argv, "afbg:")) != EOF) 
    {
        switch(ch) 
        {
            case 'a':
                if (group != -1) 
                {
                    goto usage_error;
                }
                all = 1;
                break;
            case 'f':
                force = 1;
                break;
            case 'g':
                if (all != 0) 
                {
                    goto usage_error;
                }
                group = strtol(optarg, NULL, 0);
                break;
            case 'b':
                autostart = 1;
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
        error_tracef( TRACEERR, "Start:main()", "Calling ftd_sock_startup()" );
        exitcode = 1;
        goto errexit;
    }

    if (ftd_dev_lock_create() == -1) 
    {
        error_tracef( TRACEERR, "Start:main()", "Calling ftd_dev_lock_create()" );
        exitcode = 1;
        goto errexit;
    }

    if (ftd_init_errfac( "Replicator", argv[0], NULL, NULL, 0, 1) == NULL) 
    {
        error_tracef( TRACEERR, "Start:main()", "Calling ftd_init_errfac()" );
        exitcode = 1;
        goto errexit;
    }

    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);
        
    sprintf(cfgpath, "%s", PATH_CONFIG);

    // get all primary configs 
    cfglist = ftd_config_create_list();
    
    exitcode = 0;
    if (all != 0) 
    {
        if (autostart) 
        {
            // get paths of previously started groups 
            if ( ftd_config_get_primary_started( cfgpath, cfglist ) < 0 ) 
            {
                error_tracef( TRACEERR, "Start:main()", "Calling ftd_config_get_primary_started()" );
                exitcode = 1;
                goto errexit;
            }
            if ( SizeOfLL(cfglist) == 0 )  
            {
                error_tracef( TRACEERR, "Start:main()", "Calling SizeOfLL()" );
                exitcode = 1;
                goto errexit;
            }
        } 
        else 
        {
            // get paths of all groups 
            if ( ftd_config_get_primary( cfgpath, cfglist ) < 0 ) 
            {
                error_tracef( TRACEERR, "Start:main()", "Calling ftd_config_get_primary()" );
                exitcode = 1;
                goto errexit;
            }
            if (SizeOfLL(cfglist) == 0) 
            {
                reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
                exitcode = 1;
                goto errexit;
            }
        }

        ForEachLLElement(cfglist, cfgpp) 
        {
            cfgp = *cfgpp;
            if ( start_group( cfgp->lgnum, force, autostart ) !=0 ) 
            {
                reporterr(ERRFAC, M_STARTGRP, ERRCRIT, cfgp->lgnum);
                exitcode = 1;
            }
        }
    } 
    else 
    {
        if ( start_group( group, force, autostart ) !=0 ) 
        {
            reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group);
            exitcode = 1;
        }
    }

    if ( cfglist ) 
    {
        ftd_config_delete_list(cfglist);
    }

    SendAllPendingMessagesToCollector();

    ftd_delete_errfac();
    ftd_sock_cleanup();

    exit(exitcode); 

    return exitcode; // for stupid compiler 

usage_error:
    exitcode = 1;
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

    exit(exitcode); 
    return 0; /* for stupid compiler */
}
