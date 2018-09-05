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
/*
 * stop_group.c - Stop a logical group - used by override and stop
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
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
#include <signal.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

#if defined (SOLARIS)
#include <sys/mkdev.h>
#include <sys/mnttab.h>
#endif
#if defined(HPUX) || defined(linux)
#include <mntent.h>
#endif

#include "cfg_intr.h"
#include "ps_intr.h"

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif
#include "ftdio.h"
#include "ftd_cmd.h"
#include "errors.h"
#include "pathnames.h"
#include "config.h"
#include "platform.h"
#include "devcheck.h"
#include "stop_group.h"
#include "common.h"

extern char *argv0;


extern int dbarr_ioc(int fd, int ioc, dirtybit_array_t *db, ftd_dev_info_t *dev);

extern void remove_reboot_autostart_file(int group);  /* WR PROD6443 */

static void
backup_perf_file (int groupno)
{
    char perfpath[MAXPATHLEN];
    char perfpathbackup[MAXPATHLEN];
    struct stat statbuf;

    sprintf (perfpath, "%s/p%03d.prf", PATH_RUN_FILES, groupno);
    sprintf (perfpathbackup, "%s/p%03d.prf.1", PATH_RUN_FILES, groupno);
    if (0 == stat(perfpath, &statbuf)) {
        if (0 == stat(perfpathbackup, &statbuf)) {
            (void) unlink (perfpathbackup);
        }
        (void) rename (perfpath, perfpathbackup);
    }
    return;
}
    

int
ftd_stop_group(char *ps_name, int group, ps_version_1_attr_t *attr, int autostart, int clear_ODM)
{
    int              i, fd, lgfd, new_state, devfd, ioctlret;
    char             lg_name[MAXPATHLEN], cfg_path[32];
    char             full_cfg_path[MAXPATHLEN];
    char             rawname[MAXPATHLEN], blockname[MAXPATHLEN];
    char             startstr[32];
    sddisk_t         *temp;
    ftd_stat_t       gstat;
    ftd_state_t      state;
    struct stat      statbuf;
    stat_buffer_t    sb;
    ftd_lg_info_t    lg_info;
    ps_group_info_t  ps_ginfo;
    dirtybit_array_t db;
    int              err;
    char             pmdname[8];
    pid_t            pid;
    int              pcnt;
    sddisk_t         *sd;
    ftd_dev_info_t   devinfo;
    char             shrtname[MAXPATHLEN];
	unsigned int     hrdb_size, hrdb_offset;
#if defined(_AIX)
    char             cmd[MAXPATHLEN*2];
#endif

#if defined(SOLARIS)
    struct mnttab mnttable;
    FILE *mntfd;
#endif
#if defined(HPUX) || defined(linux)
    struct mntent *mntentp;
    FILE *mntfd;
#endif
    ftd_dev_t_t      grp = group;

    sigset_t deferred_signals;
    sigemptyset(&deferred_signals);

    sigaddset(&deferred_signals, SIGINT);
    sigaddset(&deferred_signals, SIGHUP);
    sigaddset(&deferred_signals, SIGTERM);
    
    FTD_CREATE_GROUP_NAME(lg_name, group);
    sprintf(cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, group);

    /* if PMD is running, tell them to kill it first */
    sprintf (pmdname, "PMD_%03d", group);
    if ((pid = getprocessid (pmdname, 1, &pcnt)) > 0) {
        reporterr(ERRFAC, M_STOPFAIL, ERRCRIT, group);
        return (-1);
    }

    /* open the config file */
    if (readconfig(1, 0, 0, cfg_path) < 0) 
    { /* Unable to open the group's ".cur" file; group already stopped. If -o option has been given
	     at dtcstop (clear_ODM parameter), don't remove this group's entries from ODM database (AIX) (pc080211) */
#if defined( _AIX )
        if( !clear_ODM )
		  update_ODM_entry( group, clear_ODM );
#endif
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT, argv0, cfg_path, strerror(errno));
        return -1;
    }

    /* Make sure we cleanup the flag file that dtcstart creates for this group if
	   we were autorestarting after a reboot (WR PROD6443) */
    remove_reboot_autostart_file(group);

    /* open the driver */
    if ((fd = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }
    /* open the logical group device */
    if ((lgfd = open(lg_name, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, lg_name, strerror(errno));
        close(fd);
        return -1;
    }
    memset(&sb, 0, sizeof(stat_buffer_t));

#if defined(HPUX) || defined(SOLARIS) || defined(linux) 
#if defined(linux)
#define DTC_MNTTAB "/etc/mtab"
#else
#define DTC_MNTTAB "/etc/mnttab"
#endif
    /* Make sure none of the dtc devices are mounted
       (normally the init_stop ioctl below would  take care of this,
        but for some reason our device sometimes appears closed even though 
	its mounted) */
#if defined(SOLARIS)
    if (!(mntfd = fopen(DTC_MNTTAB, "r"))) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, DTC_MNTTAB , strerror(errno));
        return -1;
    }
#else
    if (!(mntfd = setmntent(DTC_MNTTAB, "r")) ) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, DTC_MNTTAB , strerror(errno));
        return -1;
    }
#endif
    
#if defined(SOLARIS)
    while ((!getmntent(mntfd, &mnttable)))
#else
    while ((mntentp = getmntent(mntfd))) 
#endif
    {
     	/* loop thru the list of devices in the configuration file */
   	 for (temp = mysys->group->headsddisk, i=0; 
		temp != NULL; temp = temp->n, i++) {
            force_dsk_or_rdsk(blockname, temp->sddevname, 0);
#if defined(SOLARIS)
            if (strcmp(mnttable.mnt_special, blockname)==0) {
#else	
            if (strcmp(mntentp->mnt_fsname, blockname)==0) {
#endif
                reporterr(ERRFAC, M_DEVOPEN, ERRCRIT, group);
                close(lgfd);
                close(fd);
                fclose(mntfd);
                return -1;
            } 
        }
    }
    fclose(mntfd);
#endif

    /* make sure all ftd devices are closed and prevent opens */ 
    if (FTD_IOCTL_CALL(lgfd, FTD_INIT_STOP, NULL) < 0) {
        if (errno == EBUSY) {
            reporterr(ERRFAC, M_DEVOPEN, ERRCRIT, group);
            reporterr(ERRFAC, M_INIT_STOP_BUSY, ERRCRIT);
            close(lgfd);
            close(fd);
            return -1;
        } else {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to initiate stop", strerror(errno));
            close(lgfd);
            close(fd);
            return -1;
        }
    }
    /* get the group stats */
    sb.lg_num = group;
    sb.dev_num = 0;
    sb.len = sizeof(gstat);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&gstat;

    /* Get the info from the driver */
    if((err = FTD_IOCTL_CALL(fd, FTD_GET_GROUP_STATS, &sb)) < 0) {
        EXIT(EXITANDDIE);
    }


    /* check the pstore to see what state the group is in */
    ps_ginfo.name = NULL;
    if ((err = ps_get_group_info(ps_name, lg_name, &ps_ginfo)) != PS_OK) {
        if (err != PS_GROUP_NOT_FOUND)
            reporterr(ERRFAC, M_PSERROR, ERRCRIT, lg_name);
        return -1;
    }

    /* if group is in BACKFRESH mode, we're done. */
#if 0

    if (gstat.state == FTD_MODE_BACKFRESH) {
        backup_perf_file (group);
        return 0;
    }
#endif

    /* clear autostart flag in pstore */
    if (autostart) {
        strcpy(startstr, "no");
        err = ps_set_group_key_value(mysys->pstore, mysys->group->devname,
            "_AUTOSTART:", startstr);
        if (err != PS_OK) {
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, mysys->group->devname, mysys->pstore);
            return -1;
        }
    }

    /* get the info on this group */
    sb.len = sizeof(ftd_lg_info_t);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lg_info;
    sb.lg_num = group;
    if (FTD_IOCTL_CALL(fd, FTD_GET_GROUPS_INFO, &sb) < 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get group info", 
            strerror(errno));
        close(fd);
        return -1;
    }

 

    new_state = gstat.state;

    /* if we're in REFRESH mode, update the LRDB and HRDB with the 
       journal. Set mode to TRACKING. */
    if (gstat.state == FTD_MODE_REFRESH || gstat.state == FTD_MODE_TRACKING) {
        FTD_IOCTL_CALL(fd, FTD_UPDATE_LRDBS, &grp);
        FTD_IOCTL_CALL(fd, FTD_UPDATE_HRDBS, &grp);
        new_state = FTD_MODE_TRACKING;

    /* if we're in NORMAL mode, see if we have journal entries. 
       If so, update dirtybits and set mode to TRACKING */
    } else if (gstat.state == FTD_MODE_NORMAL) {
        sb.lg_num = group;
        sb.addr = (ftd_uint64ptr_t)(unsigned long)&gstat;
        FTD_IOCTL_CALL(fd, FTD_GET_GROUP_STATS, &sb);
        if (gstat.bab_used > 0) {
            FTD_IOCTL_CALL(fd, FTD_CLEAR_LRDBS, &grp);
            FTD_IOCTL_CALL(fd, FTD_CLEAR_HRDBS, &grp);
            FTD_IOCTL_CALL(fd, FTD_UPDATE_LRDBS, &grp);
            FTD_IOCTL_CALL(fd, FTD_UPDATE_HRDBS, &grp);
            new_state = FTD_MODE_TRACKING;
        } else {
            FTD_IOCTL_CALL(fd, FTD_CLEAR_LRDBS, &grp);
            FTD_IOCTL_CALL(fd, FTD_CLEAR_HRDBS, &grp);

        }
    }

    /* loop thru the list of devices in the configuration file */
    for (temp = mysys->group->headsddisk, i=0; temp != NULL; temp = temp->n, i++) {
        dev_t tempdev[2];

        /* get the device number */
        strcpy(rawname, temp->sddevname);
        if (stat(rawname, &statbuf) != 0) {
            continue;
        }
        force_dsk_or_rdsk(blockname, rawname, 0);

        /* Make sure we've stopped capturing IO to the source device before tearing everything down. */
        {
            ftd_dev_t_t passdev=statbuf.st_rdev;
            
            if (FTD_IOCTL_CALL(fd, FTD_RELEASE_CAPTURED_SOURCE_DEVICE_IO, &passdev) != 0) {
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "FTD_CAPTURE_SOURCE_DEVICE_IO called to release the source device.", strerror(errno));
                close(lgfd);
                close(fd);
                return -1;
            }
        }
        
        /* if we're in PASSTHRU mode, ignore the LRDB and HRDB */
        if (gstat.state != FTD_MODE_PASSTHRU) {
            db.numdevs = 1;
            db.dblen32 = attr->lrdb_size / 4;
            tempdev[0] = statbuf.st_rdev;
            db.devs = (dev_t *)&tempdev[0];
            db.state_change = 0;
            if ((db.dbbuf = (ftd_uint64ptr_t)(unsigned long)malloc(db.dblen32*4))
				== (ftd_uint64ptr_t)NULL) {
                reporterr(ERRFAC, M_MALLOC, ERRCRIT, db.dblen32*4);
                close(lgfd);
                close(fd);
                return -1;
            }

            /* get the LRDB for the device */
            devinfo.lrdbsize32 = db.dblen32;
            if (dbarr_ioc(lgfd, FTD_GET_LRDBS, &db, &devinfo) < 0) {
                free((char *)(unsigned long)db.dbbuf);
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get lrdb",
                    strerror(errno));
                close(lgfd);
                close(fd);
                return -1;
            }

            /* put the LRDB into the persistent store */
            if (ps_set_lrdb(ps_name, rawname, (char *)(unsigned long)db.dbbuf, db.dblen32*4) != PS_OK) {
                free((char *)(unsigned long)db.dbbuf);
                reporterr(ERRFAC, M_PSWTLRDB, ERRCRIT, rawname, ps_name);
                close(lgfd);
                close(fd);
                return -1;
            }

            free((char *)(unsigned long)db.dbbuf);

            if( attr->hrdb_type == FTD_HS_PROPORTIONAL )
			{
                if( ps_get_device_hrdb_info(ps_name, rawname, &hrdb_size, &hrdb_offset, NULL, NULL, NULL, NULL, NULL, NULL ) != PS_OK )
			    {
                    reporterr(ERRFAC, M_ERR_HRDBINFO, ERRCRIT, rawname, group, ps_name);
                    close(lgfd);
                    close(fd);
                    return -1;
			    }
			}
			else  // Non-Proportional HRDB (Small or Large HRT)
			{
			    hrdb_size = attr->Small_or_Large_hrdb_size;
			}
            /* get the HRDB for the device */
            db.state_change = 0;
            db.dblen32 = hrdb_size / 4;
            if ((db.dbbuf = (ftd_uint64ptr_t)(unsigned long)malloc(db.dblen32*4))
				== (ftd_uint64ptr_t)NULL) {
                reporterr(ERRFAC, M_MALLOC, ERRCRIT, db.dblen32*4);
                close(lgfd);
                close(fd);
                return -1;
            }
    
            devinfo.hrdbsize32 = db.dblen32;
            if (dbarr_ioc(lgfd, FTD_GET_HRDBS, &db, &devinfo) < 0) {
                free((char *)(unsigned long)db.dbbuf);
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get hrdb",
                    strerror(errno));
                close(lgfd);
                close(fd);
                return -1;
            }

            /* put the HRDB into the persistent store */
            if ((err = ps_set_hrdb(ps_name, rawname, (char *)(unsigned long)db.dbbuf, db.dblen32*4)) != PS_OK) {
                free((char *)(unsigned long)db.dbbuf);
                reporterr(ERRFAC, M_PSWTHRDB, ERRCRIT, rawname, ps_name, err, ps_get_pstore_error_string(err));
                close(lgfd);
                close(fd);
                return -1;
            }
            free((char *)(unsigned long)db.dbbuf);
        }

        sb.lg_num = group;
        sb.dev_num = statbuf.st_rdev; /* device id of our device */
        sb.len = attr->dev_attr_size;
        if ((sb.addr = (ftd_uint64ptr_t)(unsigned long)malloc(sb.len)) == (ftd_uint64ptr_t)NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, sb.len);
            close(lgfd);
            close(fd);
            return -1;
        }

        /* get the attributes */
        if (FTD_IOCTL_CALL(fd, FTD_GET_DEV_STATE_BUFFER, &sb) < 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get dev stats", 
                strerror(errno));
            free((char *)(unsigned long)sb.addr);
            close(lgfd);
            close(fd);
            return -1;
        }

        /* put the attributes into the persistent store */
        if (ps_set_device_attr(ps_name,
                               rawname,
                               (char *)(unsigned long)sb.addr,
                               sb.len) != PS_OK) {
            free((char *)(unsigned long)sb.addr);
            reporterr(ERRFAC, M_PSWTDATTR, ERRCRIT, rawname, ps_name);
            close(lgfd);
            close(fd);
            return -1;
        }
        free((char *)(unsigned long)sb.addr);

        /* delete the device from the group */
        // Deleting the device and the its device nodes need to be made atomic in order to be able to resume an interrupted dtcstop.
        // C.F. PROD00008341.
        sigprocmask(SIG_BLOCK, &deferred_signals, NULL);
        {
            ftd_dev_t_t passdev=statbuf.st_rdev;
            if (FTD_IOCTL_CALL(fd, FTD_DEL_DEVICE, &passdev) < 0) {
                reporterr(ERRFAC, M_DRVDELDEV, ERRCRIT, rawname, lg_name, strerror(errno));
                close(lgfd);
                close(fd);
                // Let any blocked signal be handled now.
                sigprocmask(SIG_UNBLOCK, &deferred_signals, NULL);
                return -1;
            }

            /* delete the devices */
            unlink(blockname);
            unlink(rawname);
#if defined(_AIX)
            //If dynamic activation is not active then will need to register DTC driver to AIX ODM
            if (mysys->group->capture_io_of_source_devices == 0)
            {
                ftd_rm_aix_links(group, temp->devid);
                /* NOTE: update ODM database if applicable (based on flag passed in argument (pc080211) */
                if( !clear_ODM )
                {
                    if (ftd_odm_delete(group, temp->devid, shrtname) < 0)
                        reporterr(ERRFAC, M_ODMDELERR, ERRWARN, shrtname);
                }
            }
#endif
        }
        // Let any blocked signal be handled now.
        sigprocmask(SIG_UNBLOCK, &deferred_signals, NULL);

    } /*...end of block: for (temp = mysys->group->headsddisk... */

    close(lgfd);

    /* stuff the state into pstore */
    ps_set_group_state(ps_name, lg_name, new_state);
#if defined(linux)
    sync();
#endif
    /* delete the group */
    /* if (FTD_IOCTL_CALL(fd, FTD_DEL_LG, &lg_info.lgdev) < 0) { */
    {
    ftd_dev_t_t passdev=lg_info.lgdev;
    if (FTD_IOCTL_CALL(fd, FTD_DEL_LG, &passdev) < 0) {
        reporterr(ERRFAC, M_DRVDELGRP, ERRCRIT, lg_name);
        close(fd);
        return -1;
    }
    }
    close(fd);

    /* write out the shutdown flag for the group */
    ps_set_group_shutdown(ps_name, lg_name, 1);

    /* waste the devices directory */
    ftd_delete_group(group);
#if defined(linux)
 {
   /* Remove symbolic links in /dev created for Veritas */
   for (temp = mysys->group->headsddisk, i=0; temp; temp = temp->n, i++) {
	sprintf(blockname, "/dev/dtc%d-%d", group, i);
	unlink(blockname);
    }
 }
#endif

    /* move the .prf file out of the way so monitortool shows it gone */
    backup_perf_file (group);

    
    /* remove .cur file */
    if (autostart) {
        sprintf(full_cfg_path, "%s/%s", PATH_CONFIG, cfg_path);
        unlink (full_cfg_path);
    }

    return 0;
}

#if defined( _AIX )
/*================================= update_ODM_entry ==============================*/
/* If clear_ODM true (-o option), don't remove group device entries from AIX ODM. If
   clear_ODM false, remove group device entries in AIX ODM */
int  update_ODM_entry( int group, int clear_ODM )
{
  sddisk_t         *temp;
  char             cfg_path[MAXPATHLEN], shrtname[MAXPATHLEN];
  char	           cmd[MAXPATHLEN * 3];

  if( !clear_ODM )
  {
    sprintf(cfg_path, "%s%03d.cfg", PMD_CFG_PREFIX, group);
    /* open the config file and load the global mysys control structure */
    if (readconfig(1, 0, 0, cfg_path) < 0)
    {
        reporterr(ERRFAC, M_CFGFILE, ERRWARN, argv0, cfg_path, strerror(errno));
        return( -1 );
    }

    //If dynamic activation is not active then will need to register DTC driver to AIX ODM
    if (mysys->group->capture_io_of_source_devices == 0)
    {
        /* Loop through the list of devices from the configuration file */
        for (temp = mysys->group->headsddisk; temp != NULL; temp = temp->n)
        {
            if (ftd_odm_delete(group, temp->devid, shrtname) < 0)
            {
                reporterr(ERRFAC, M_ODMDELERR, ERRWARN, shrtname);
	            return( -1 );
            }
        }
    }
  }
  return( 0 );
}


/*================================= update_all_ODM_entries ==============================*/
/* Remove ALL group device entries from AIX ODM. If clear_ODM true, don't remove ODM entries */
void  update_all_ODM_entries( int clear_ODM )
{
  char configpaths[MAXLG][32];
  int  lgcnt, i, group;

  /* get paths of all groups */
  lgcnt = GETCONFIGS(configpaths, 1, 0);
  if (lgcnt == 0) 
  {	reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
    return;
  }

  for (i = 0; i < lgcnt; i++) 
  {	group = cfgpathtonum( configpaths[i] );
    update_ODM_entry( group, clear_ODM );
  }

  return;
}
#endif
