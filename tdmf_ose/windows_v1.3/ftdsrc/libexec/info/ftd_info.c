/*
 * ftd_info.c - display logical group info on stdout 
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
#include "ftd_port.h"
#include "ftd_config.h"
#include "ftd_cfgsys.h"
#include "ftd_lg.h"
#include "ftdio.h"
#include "ftd_ps.h"
#include "ftd_error.h"
#include "ftd_ioctl.h"
#include "llist.h"
#include "ftd_sock.h"

#define ERROR_CODE  1
#define OK_CODE     0

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

#if defined(_WINDOWS)
//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);
#endif

#define TRUE    1
#define FALSE   0

static void printbabinfo(HANDLE master);
static void printdriver(ftd_lg_cfg_t *cfgp, ftd_stat_t info);
static void printpstore(ps_group_info_t *ginfo, ftd_stat_t info);
static void printother(ftd_stat_t Info);
static void printlocaldisk(disk_stats_t DiskInfo, ftd_lg_cfg_t *cfgp, ftd_dev_cfg_t *devp);
static void printbab(disk_stats_t DiskInfo);
static void printgroup(ftd_lg_t *lgp, int Other);

extern volatile char qdsreleasenumber[];

/* This is how the program is used */
static void
usage(char **argv)
{
    fprintf(stderr,"Usage: %s <options>\n\n",  argv[0]);
    fprintf(stderr,"\
    One of the following three options is mandatory:\n\
        -g <logical_group_num> : Display info for a logical group\n\
        -a                     : Display info for all groups\n\
        -v                     : Print Software Version and Build Sequence Number\n\
        -h                     : Help\n");

    exit(ERROR_CODE);
}

int main(int argc, char **argv)
{
    HANDLE          ctlfd = INVALID_HANDLE_VALUE;
#if defined(_AIX)
    int             ch;
#else  /* defined(_AIX) */
    char            ch;
#endif /* defined(_AIX) */
    int             all, group, lgcnt, Other, Version;
    ftd_lg_t        *lgp;
    LList           *cfglist;
    ftd_lg_cfg_t    **cfgpp;

////------------------------------------------
#if defined(_WINDOWS)
    void ftd_util_force_to_use_only_one_processor();
    ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------

    if (argc < 2) {
        goto usage_error;
    }

    all = 0;
    group = -1;
    Other = Version = 0;

    /* Determine what options were specified */
    while ((ch =getopt(argc, argv, "ahovg:")) != -1) {
        switch (ch) {
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
        case 'o':
            /* Display Other Info ( Not for Customer Use ) */
            Other = 1;
            break;
        case 'v':
            /* Display Version */
            Version = 1;
            break;
        case 'h':
        default:
            goto usage_error;
            break;
        }
    }

    if (Version)
        fprintf(stderr, "\n%s\n\n", qdsreleasenumber);

    if ((all == 0) && (group < 0)) {
        if (Version)
            goto errexit;
        else
            goto usage_error;
    }

#if defined(_WINDOWS)
    if (ftd_sock_startup() == -1) {
        error_tracef( TRACEERR, "info::main()", "Calling ftd_sock_startup()" );
        goto errexit;
    }
    if (ftd_dev_lock_create() == -1) {
        error_tracef( TRACEERR, "info::main()", "Calling ftd_dev_lock_create()" );
        goto errexit;
    }
#endif

    if (ftd_init_errfac("Replicator", argv[0], NULL, NULL, 0, 1) == NULL) {
        goto errexit;
    }

#if defined(_WINDOWS)
    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);
#endif

    // open the master control device 
    ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0);
    if (ctlfd == INVALID_HANDLE_VALUE) {
        reporterr(ERRFAC, M_CTLOPEN, ERRCRIT, ftd_strerror());
        goto errexit;
    }

    /* print out the bab data */
    printbabinfo(ctlfd);

    FTD_CLOSE_FUNC(__FILE__,__LINE__,ctlfd);

    // create config file list
    if ((cfglist = ftd_config_create_list()) == NULL) {
        error_tracef( TRACEERR, "info::main()", "Calling ftd_config_create_list()" );
        goto errexit;
    }
    // get all primary config files
    if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) {
        error_tracef( TRACEERR, "info::main()", "Calling ftd_config_get_primary_started()" );
        goto errexit;
    }
    lgcnt = SizeOfLL(cfglist);

    if (lgcnt == 0) {
        fprintf(stderr,"        No Logical Groups have been started.\n");
        goto errexit;
    }

    if (all != 0) {
        ForEachLLElement(cfglist, cfgpp) {
            if ((lgp = ftd_lg_create()) == NULL) {
                error_tracef( TRACEERR, "info::main()", "Calling ftd_lg_create()" );
                goto errexit;
            }
            if (ftd_lg_init(lgp, (*cfgpp)->lgnum, ROLEPRIMARY, 1) < 0) {
                error_tracef( TRACEERR, "info::main()", "Calling ftd_lg_init()" );
                ftd_lg_delete(lgp);
                goto errexit;
            }
            printgroup(lgp, Other);

            ftd_lg_delete(lgp);
        }
    } else {
            if ((lgp = ftd_lg_create()) == NULL) {
                error_tracef( TRACEERR, "info::main()", "Calling ftd_lg_create()" );
                goto errexit;
            }
            if (ftd_lg_init(lgp, group, ROLEPRIMARY, 1) < 0) {
                error_tracef( TRACEERR, "info::main()", "Calling ftd_lg_init()" );
                ftd_lg_delete(lgp);
                goto errexit;
            }
            printgroup(lgp, Other);

            ftd_lg_delete(lgp);
    }
            
    ftd_delete_errfac();

#if defined(_WINDOWS)
    ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

    exit(OK_CODE);
    return 0; /* for stupid compiler */

usage_error:
    usage(argv);
    
errexit:

ftd_delete_errfac();

#if defined(_WINDOWS)
    ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

    exit(Version ? OK_CODE : ERROR_CODE);
}

static void
printgroup(ftd_lg_t *lgp, int Other)
{
    ps_group_info_t group_info;
    ftd_stat_t      lgstat;
    disk_stats_t    DiskInfo;
    ftd_dev_cfg_t   **devpp, *devp;
    int             dev_num;
#if !defined(_WINDOWS)
    stat_buffer_t   sb;
    struct stat     statbuf;
#endif
#if defined(_WINDOWS)
    int             i, ndevs, found;
    disk_stats_t    *devstat = NULL;
#endif
    int rc;

    printf("\nLogical Group %d (%s -> %s)\n",
        lgp->lgnum, lgp->cfgp->phostname,
        lgp->cfgp->shostname);
        
    group_info.name = NULL;
    if ((rc = ps_get_group_info(lgp->cfgp->pstore,
        lgp->devname, &group_info)) != PS_OK) {
        if (rc == PS_GROUP_NOT_FOUND) {
            fprintf(stderr,"group doesn't exist in pstore: %d ", rc);
        } else {
            fprintf(stderr,"PSTORE error: %d ", rc);
        }
        fprintf(stderr,"%s, %s\n", lgp->cfgp->pstore, lgp->devname);
        return;
    }
                        
    /* Get the info for the group */
    if ((rc = ftd_ioctl_get_group_stats(lgp->ctlfd, lgp->lgnum,
        &lgstat, 0)) < 0) {
        return;
    }
            
    /* print driver logical group info */
    printdriver(lgp->cfgp, lgstat);
        
    /* print pstore logical group info */
    printpstore(&group_info, lgstat);
        
    ndevs = SizeOfLL(lgp->cfgp->devlist);

    devstat = calloc(1, ndevs * sizeof(disk_stats_t));
    if (devstat == NULL) {
        reporterr(ERRFAC, M_MALLOC, (ndevs * sizeof(disk_stats_t)));
        return;
    }

    if (ftd_ioctl_get_dev_stats(lgp->ctlfd, lgp->lgnum, -1, devstat, ndevs) < 0) {
        free(devstat);
        return;
    }

    ForEachLLElement(lgp->cfgp->devlist, devpp) {
        devp = *devpp;
    
        printf("\n    Device %s:\n", devp->devname);


        found = FALSE;

        // only if group has been started 
        // otherwise no sense in trying to get dev nums
        for (i = 0; i < ndevs; i++) 
        {
            if (!strcmp(devp->devname, strchr(devstat[i].devname, ':') - 1 ))
            {
                // found it
                found = TRUE;
                break;
            }
        }

        if (!found) {
            // skip it
            continue;
        }

        dev_num = devstat[i].localbdisk;


        ftd_ioctl_get_dev_stats(lgp->ctlfd, lgp->lgnum, dev_num, &DiskInfo, 1);
        
        printlocaldisk(DiskInfo, lgp->cfgp, devp);
        printbab(DiskInfo);
    }

#if defined(_WINDOWS)
    free(devstat);
#endif

    return;
}

static void
printbabinfo(HANDLE master)
{
    char    buf[MAXPATHLEN];
    unsigned int actual, num_chunks, chunk_size, size, driverstate;

    /* get bab size from driver */
    ftd_ioctl_get_bab_size(master, &actual);

    num_chunks = chunk_size = 0;

    memset(buf, 0, sizeof(buf));
    cfg_get_driver_key_value("num_chunks", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
    num_chunks = strtol(buf, NULL, 0);

    memset(buf, 0, sizeof(buf));
    cfg_get_driver_key_value("chunk_size", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
    chunk_size = strtol(buf, NULL, 0);

    memset(buf, 0, sizeof(buf));
    cfg_get_driver_key_value("Start", FTD_DRIVER_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
    driverstate = strtol(buf, NULL, 0);

    size = chunk_size * num_chunks;

    printf("\nRequested BAB size ................ %u (~ %d MB)\n", size, size >> 20);
    printf("Actual BAB size ................... %u (~ %d MB)\n", actual, actual >> 20);

    if ( driverstate == 0 )
        printf("Driver state ...................... Loaded at Boot-Time \n");
    else
        printf("Driver state ...................... Not Loaded at Boot-Time (%d)\n", driverstate);

    return;
}

static void
printdriver(ftd_lg_cfg_t *cfgp, ftd_stat_t Info) 
{
    char *state;

    switch (Info.state) {
    case FTD_MODE_PASSTHRU:
        state = "Passthru";
        break;
    case FTD_MODE_NORMAL:
        state = "Normal";
        break;
   case FTD_MODE_CHECKPOINT_JLESS:
        state = "Checkpoint (Tracking when Journal-Less)";
        break;
    case FTD_MODE_TRACKING:
        state = "Tracking";
        break;
    case FTD_MODE_REFRESH:
        state = "Refresh";
        break;
    case FTD_MODE_FULLREFRESH:
        state = "FullRefresh";
        break;
    case FTD_MODE_BACKFRESH:
    default:
        state = "Backfresh";
        break;
    }

    printf("\n    Mode of operations.............. %s\n", state);
    printf("    Entries in the BAB.............. %lu\n", Info.wlentries);
    printf("    Sectors in the BAB.............. %lu\n", Info.wlsectors);
    if (Info.sync_depth != (unsigned int) -1) {
        printf("    Sync/Async mode................. Sync\n");
        printf("    Sync mode target depth.......... %u\n", Info.sync_depth);
        printf("    Sync mode timeout............... %u\n", Info.sync_timeout);
    } else {
        printf("    Sync/Async mode................. Async\n");
    }
//    printf("    I/O delay....................... %u\n", Info.iodelay);
    printf("    Persistent Store................ %s\n", cfgp->pstore);

    return;
}

static void
printpstore(ps_group_info_t *ginfo, ftd_stat_t Info)
{
    if (ginfo->checkpoint) 
    {
        if ( ( Info.state == FTD_MODE_TRACKING ) || ( Info.state == FTD_MODE_PASSTHRU ) )
        {
            printf("    Checkpoint State(not effective). %s\n", "Off");
        }
        else // New report message for WR17056
        {
            printf("    Checkpoint State................ %s\n", "On");
        }
    } 
    else 
    {
        printf("    Checkpoint State................ %s\n", "Off");
    }

    return;
}

static void
printother(ftd_stat_t Info)
{
    char *tb;
    time_t lt;

    lt = (time_t) Info.loadtimesecs;
    tb = ctime(&lt);
    tb[strlen(tb) - 1] = '\0';
    printf("\n    Load time....................... %s\n", tb);
    printf("    Load time system ticks.......... %lu\n", Info.loadtimesystics);

    printf("    Used (calc): %d Free (calc): %d\n", Info.bab_used, Info.bab_free);

    return;
}

static void
printlocaldisk(disk_stats_t DiskInfo, ftd_lg_cfg_t *cfgp, ftd_dev_cfg_t *devp)
{
    printf("\n        Local disk device number........ 0x%x\n", (int) DiskInfo.localbdisk);
    printf("        Local disk size (sectors)....... %lu\n", DiskInfo.localdisksize);
    printf("        Local disk name................. %s\n", devp->pdevname);
    printf("        Remote mirror disk.............. %s:%s\n", 
        cfgp->shostname, devp->sdevname);

    return;
}

static void 
printbab(disk_stats_t DiskInfo)
{
    printf("        Read I/O count.................. %llu\n", DiskInfo.readiocnt);
    printf("        Total # of sectors read......... %llu\n", DiskInfo.sectorsread);
    printf("        Write I/O count................. %llu\n", DiskInfo.writeiocnt);
    printf("        Total # of sectors written...... %llu\n", DiskInfo.sectorswritten);
    printf("        Entries in the BAB.............. %lu\n", DiskInfo.wlentries);
    printf("        Sectors in the BAB.............. %lu\n", DiskInfo.wlsectors);

    return;
}
