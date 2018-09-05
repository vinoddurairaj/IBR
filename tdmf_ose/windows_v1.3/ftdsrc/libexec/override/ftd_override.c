/*
 * ftd_override.c - override the state of the driver 
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


#if defined(_WINDOWS) && defined(_DEBUG)
#include <conio.h>
#endif
#if !defined(_WINDOWS)
#include <sys/sysmacros.h>
#endif
#if defined(SOLARIS)
#include <sys/mkdev.h>
#endif
#include "ftd_port.h"
#include "ftd_ps.h"
#include "ftd_lg.h"
#include "ftd_error.h"
#include "ftd_ioctl.h"
#include "ftd_sock.h"
#include "ftdio.h"

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

#if defined(_WINDOWS) 
//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc); //from 2.1.6 Merge
#endif

/* internal defines */
#define SOMETHING_BAB   1
#define SOMETHING_LRDB  2
#define SOMETHING_HRDB  3

static void
usage(char **argv)
{
    fprintf(stderr, "Usage: %s [-g group_number | -a] \n", argv[0]);
    fprintf(stderr, "          [ [clear [bab | LRT| HRT]] | \n");
    fprintf(stderr, "            [state [passthru|normal|refresh|tracking|backfresh]] ]\n");
}

static void
maybe_report_state_failure(int group, int state, int err)
{
    /* XXX Should be REPORT ERRORIFIED XXX */
    switch(err) {
    case -6:
        fprintf(stderr, "Can't get PSTORE value\n");
        break;
    case -5:
        fprintf(stderr, "Can't open CTL device: %s\n", ftd_strerror());
        break;
    case -4:
        fprintf(stderr, "PS Get attributes failed\n");
        break;
    case -3:
        fprintf(stderr, "Can't get PSTORE group info for group %d\n", group);
        break;
    case -2:
        fprintf(stderr, "PS Set Group State failed for group %d state %d\n",
            group, state);
        break;
    case -1:
        fprintf(stderr, "Driver error setting state to %d for group %d\n%s\n",
            state, group, ftd_strerror());
        break;
    }
}

/*
 * set_state - set state of group
 */
static int
set_state(int group, int state)
{
    int         rc;
    ps_group_info_t ginfo;
    ps_version_1_attr_t attr;
    ftd_lg_t    *lgp;

    if ((lgp = ftd_lg_create()) == NULL) {
        rc = -1;
        goto errexit;
    }
    if (ftd_lg_init(lgp, group, ROLEPRIMARY, 1) < 0) {
        rc = -1;
        goto errexit;
    }
    if (ps_get_version_1_attr(lgp->cfgp->pstore, &attr) != PS_OK) {
        rc = -4;
        goto errexit;
    }
    /* set the state in the pstore */
    ginfo.name = NULL;
    if (ps_get_group_info(lgp->cfgp->pstore, lgp->devname, &ginfo) != PS_OK) {
        rc = -3;
        goto errexit;
    }

    /* if we are in backfresh mode, change the state in the pstore 
        and that's it */
/*
    if (ginfo.state == FTD_MODE_BACKFRESH) {
        if (ps_set_group_state(lgp->cfgp->pstore, lgp->devname, state) != PS_OK) {
            rc = -2;
            goto errexit;
        }
        * set the shutdown state to 1 *
        ps_set_group_shutdown(lgp->cfgp->pstore, lgp->devname, 1);

        rc = 0;
        goto errexit;
    }
*/

    /* if we are going into backfresh mode, stop the driver,
       and load pertinent data into the pstore */
    if (state == FTD_MODE_BACKFRESH) {
        /* stop the driver (ala ftdstop)
        ftd_lg_rem(lgp, 1);
        rc = 0;
        goto errexit;
        */
        // just clear bits ?
        rc = ftd_ioctl_clear_lrdbs(lgp->ctlfd, lgp->lgnum, 0);
        rc = ftd_ioctl_clear_hrdbs(lgp->ctlfd, lgp->lgnum, 0);
    } 

    /* set the state in the pstore */
    if (ps_set_group_state(lgp->cfgp->pstore, lgp->devname, state) != PS_OK) {
        rc = -2;
        goto errexit;
    }
 
    rc = ftd_ioctl_set_group_state(lgp->ctlfd, lgp->lgnum, state);

    ftd_lg_delete(lgp);

    return rc;

errexit:

    ftd_lg_delete(lgp);

    return rc;
}

/*
 *
 */
static int
clear_something(int group, int state)
{
    HANDLE  fd;
    int     ret, count;

    if ((fd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, ftd_strerror());
        return -1;
    }
    group |= FTD_LGFLAG;

    if (state == SOMETHING_BAB) {
        /* retry at most 10 times */
        count = 0;
        while (((ret = ftd_ioctl_clear_bab(fd, group)) == EAGAIN) && 
                (count < 10)) {
            sleep(1);
            count++;
        }
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        if (ret != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "CLEAR_BAB", ftd_strerror());
            return -1;
        }
    } else if (state == SOMETHING_LRDB) {
        ret = ftd_ioctl_clear_lrdbs(fd, group, 0);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        if (ret != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "CLEAR_LRDBS",
                ftd_strerror());
            return -1;
        }
    } else if (state == SOMETHING_HRDB) {
        ret = ftd_ioctl_clear_hrdbs(fd, group, 0);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        if (ret != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "CLEAR_HRDBS",
                ftd_strerror());
            return -1;
        }
    } else {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return -1;        
    }

    
    return 0;
}

int 
main(int argc, char *argv[])
{
    LList           *cfglist;
    ftd_lg_cfg_t    *cfgp, **cfgpp;
    int             state, group, index, force = 0;
#if defined(_AIX)
    int             ch;
#else /* defined(_AIX) */
    char            ch;
#endif /* defined(_AIX) */
    ftd_lg_t        *lgp;

////------------------------------------------
#if defined(_WINDOWS)
    void ftd_util_force_to_use_only_one_processor();
    ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------

    if (argc < 4) {
        goto usage_error;
    }

    while ((ch = getopt(argc, argv, "ag:f")) != -1) {
        switch (ch) {
        case 'a': 
            group = -1;
            index = optind;
            break;
        case 'g': 
            group = strtol(optarg, NULL, 0);
            index = optind;
            if (argc < 4) {
                goto usage_error;
            }
            break;
        case 'f':       // undocumented 'force' switch 
            force = 1;
            break;
        default:
            goto usage_error;
        }
    }
   
    cfgp = NULL;

#if defined(_WINDOWS)
    if (ftd_sock_startup() == -1) {
        goto errexit;
    }
    if (ftd_dev_lock_create() == -1) {
        goto errexit;
    }
#endif

    if (ftd_init_errfac("Replicator", argv[0], NULL, NULL, 0, 1) == NULL) {
        exit(1);
    }

#if defined(_WINDOWS) 
    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc); //from 2.1.6 Merge
#endif

    /* get all primary configs */
    if ((cfglist = ftd_config_create_list()) == NULL) {
        goto errexit;
    }
    
    if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) {
        goto errexit;
    }

    if (SizeOfLL(cfglist) == 0) {
        reporterr(ERRFAC, M_NOGROUP, ERRWARN);
        goto errexit;
    }

    if (strcmp("clear", argv[index]) == 0) {
        if (strcmp("bab", argv[index+1]) == 0) {
            /* clear the bab for one or all groups */
            state = SOMETHING_BAB;
        } else if (strcmp("LRT", argv[index+1]) == 0) {
            /* clear the LRDB for one or all groups */
            state = SOMETHING_LRDB;
        } else if (strcmp("HRT", argv[index+1]) == 0) {
            /* clear the HRDB for one or all groups */
            state = SOMETHING_HRDB;
        } else {
            goto usage_error;
        }
        if (group == -1) {
            ForEachLLElement(cfglist, cfgpp) {

                // if logical group is busy (ie. the pmd is running)
                // then don't allow override - it causes a lot of
                // problems in the pmd

                if ((lgp = ftd_lg_create()) == NULL) {
                    goto errexit;
                }
				if (ftd_lg_init(lgp, (*cfgpp)->lgnum, ROLEPRIMARY, 1) < 0) {
                    goto errexit;
                }
                
                if (force || ftd_lg_open(lgp) == 0) {
                    clear_something((*cfgpp)->lgnum, state);
                }
                ftd_lg_delete(lgp);
            }
        } else {
            // if logical group is busy (ie. the pmd is running)
            // then don't allow override - it causes a lot of
            // problems in the pmd

            if ((lgp = ftd_lg_create()) == NULL) {
                goto errexit;
            }
			if (ftd_lg_init(lgp, group, ROLEPRIMARY, 1) < 0) {
                goto errexit;
            }
                
            if (force || ftd_lg_open(lgp) == 0) {
                clear_something(group, state);
            }
        }
    } else if (strcmp("state", argv[index]) == 0) {
        if (strcmp("passthru", argv[index+1]) == 0) {
            state = FTD_MODE_PASSTHRU;
        } else if (strcmp("normal", argv[index+1]) == 0) {
            state = FTD_MODE_NORMAL;
        } else if (strcmp("refresh", argv[index+1]) == 0) {
            state = FTD_MODE_REFRESH;
        } else if (strcmp("tracking", argv[index+1]) == 0) {
            state = FTD_MODE_TRACKING;
        } else if (strcmp("backfresh", argv[index+1]) == 0) {
            state = FTD_MODE_BACKFRESH;
        } else {
            goto usage_error;
        }

        if (group == -1) {
            ForEachLLElement(cfglist, cfgpp) {
                    
                // if logical group is busy (ie. the pmd is running)
                // then don't allow override - it causes a lot of
                // problems in the pmd

                if ((lgp = ftd_lg_create()) == NULL) {
                    goto errexit;
                }
				if (ftd_lg_init(lgp, (*cfgpp)->lgnum, ROLEPRIMARY, 1) < 0) {
                    goto errexit;
                }
                if (force || ftd_lg_open(lgp) == 0) {
                    maybe_report_state_failure((*cfgpp)->lgnum,
                        state, set_state((*cfgpp)->lgnum, state));
                }
                ftd_lg_delete(lgp);
            }
        } else {

            // if logical group is busy (ie. the pmd is running)
            // then don't allow override - it causes a lot of
            // headaches in the pmd

            if ((lgp = ftd_lg_create()) == NULL) {
                goto errexit;
            }
			if (ftd_lg_init(lgp, group, ROLEPRIMARY, 1) < 0) {
                goto errexit;
            }

            if (force || ftd_lg_open(lgp) == 0) {
                maybe_report_state_failure(group,
                    state, set_state(group, state));
            }
            ftd_lg_delete(lgp);
        }
    } else {
        goto usage_error;
    }

    ftd_delete_errfac();

#if defined(_WINDOWS)
    ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

    exit(0);
    return 0;

usage_error:
    usage(argv);

errexit:

    ftd_delete_errfac();

#if defined(_WINDOWS)
    ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

    exit(1);
    return 0; /* for stupid compiler */
}
