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
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

#if defined (SOLARIS)
#include <sys/mkdev.h>
#endif

#include "cfg_intr.h"
#include "ps_intr.h"

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
ftd_stop_group(char *ps_name, int group, ps_version_1_attr_t *attr, int autostart)
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

    FTD_CREATE_GROUP_NAME(lg_name, group);
    sprintf(cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, group);

    /* if PMD is running, tell them to kill it first */
    sprintf (pmdname, "PMD_%03d", group);
    if ((pid = getprocessid (pmdname, 1, &pcnt)) > 0) {
        reporterr(ERRFAC, M_STOPFAIL, ERRCRIT, group);
        return (-1);
    }

    /* open the config file */
    if (readconfig(1, 0, 0, cfg_path) < 0) {
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT, argv0, cfg_path, strerror(errno));
        return -1;
    }

    /* open the driver */
    if ((fd = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }
    /* open the logical group device */
    if ((lgfd = open(lg_name, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, lg_name);
        close(fd);
        return -1;
    }
    memset(&sb, 0, sizeof(stat_buffer_t));
    /* make sure all ftd devices are closed and prevent opens*/
    if (FTD_IOCTL_CALL(lgfd, FTD_INIT_STOP, &sb) < 0) {
        if (errno == EBUSY) {
            reporterr(ERRFAC, M_DEVOPEN, ERRCRIT, group);
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
    sb.addr = (ftd_uint64ptr_t)&gstat;

    /* Get the info from the Device */
    if((err = FTD_IOCTL_CALL(fd, FTD_GET_GROUP_STATS, &sb)) < 0) {
        exit(1);
    }


    /* check the pstore to see what state the group is in */
    ps_ginfo.name = NULL;
    if ((err = ps_get_group_info(ps_name, lg_name, &ps_ginfo)) != PS_OK) {
        if (err != PS_GROUP_NOT_FOUND)
            reporterr(ERRFAC, M_PSERROR, ERRCRIT, lg_name);
        return -1;
    }
    /* if group is in BACKFRESH mode, we're done. */
    if (gstat.state == FTD_MODE_BACKFRESH) {
        backup_perf_file (group);
        return 0;
    }

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
    sb.addr = (ftd_uint64ptr_t)&lg_info;
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
    if (gstat.state == FTD_MODE_REFRESH) {
        FTD_IOCTL_CALL(fd, FTD_UPDATE_LRDBS, group);
        FTD_IOCTL_CALL(fd, FTD_UPDATE_HRDBS, group);
        new_state = FTD_MODE_TRACKING;

    /* if we're in NORMAL mode, see if we have journal entries. 
       If so, update dirtybits and set mode to TRACKING */
    } else if (gstat.state == FTD_MODE_NORMAL) {
        sb.lg_num = group;
        sb.addr = (ftd_uint64ptr_t)&gstat;
        FTD_IOCTL_CALL(fd, FTD_GET_GROUP_STATS, &sb);
        if (gstat.bab_used > 0) {
            FTD_IOCTL_CALL(fd, FTD_CLEAR_LRDBS, group);
            FTD_IOCTL_CALL(fd, FTD_CLEAR_HRDBS, group);
            FTD_IOCTL_CALL(fd, FTD_UPDATE_LRDBS, group);
            FTD_IOCTL_CALL(fd, FTD_UPDATE_HRDBS, group);
            new_state = FTD_MODE_TRACKING;
        } else {
            FTD_IOCTL_CALL(fd, FTD_CLEAR_LRDBS, group);
            FTD_IOCTL_CALL(fd, FTD_CLEAR_HRDBS, group);
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

        /* if we're in PASSTHRU mode, ignore the LRDB and HRDB */
        if (gstat.state != FTD_MODE_PASSTHRU) {
            db.numdevs = 1;
            db.dblen32 = attr->lrdb_size / 4;
            tempdev[0] = statbuf.st_rdev;
            db.devs = (dev_t *)&tempdev[0];
            db.state_change = 0;
            if ((db.dbbuf = (ftd_uint64ptr_t)malloc(db.dblen32*4)) == NULL) {
                reporterr(ERRFAC, M_MALLOC, ERRCRIT, db.dblen32*4);
                close(lgfd);
                close(fd);
                return -1;
            }

            /* get the LRDB for the device */
            {
	    ftd_dev_info_t devinfo;
            devinfo.lrdbsize32 = db.dblen32;
            if (dbarr_ioc(lgfd, FTD_GET_LRDBS, &db, &devinfo) < 0) {
                free((char *)db.dbbuf);
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get lrdb",
                    strerror(errno));
                close(lgfd);
                close(fd);
                return -1;
            }
            }

            /* put the LRDB into the persistent store */
            if (ps_set_lrdb(ps_name, rawname, (char *)db.dbbuf, db.dblen32*4) != PS_OK) {
                free((char *)db.dbbuf);
                reporterr(ERRFAC, M_PSWTLRDB, ERRCRIT, rawname, ps_name);
                close(lgfd);
                close(fd);
                return -1;
            }

            free((char *)db.dbbuf);

            /* get the HRDB for the device */
            db.state_change = 0;
            db.dblen32 = attr->hrdb_size / 4;
            if ((db.dbbuf = (ftd_uint64ptr_t)malloc(db.dblen32*4)) == NULL) {
                reporterr(ERRFAC, M_MALLOC, ERRCRIT, db.dblen32*4);
                close(lgfd);
                close(fd);
                return -1;
            }
    
            {
	    ftd_dev_info_t devinfo;
            devinfo.hrdbsize32 = db.dblen32;
            if (dbarr_ioc(lgfd, FTD_GET_HRDBS, &db, &devinfo) < 0) {
                free((char *)db.dbbuf);
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get hrdb",
                    strerror(errno));
                close(lgfd);
                close(fd);
                return -1;
            }
            }

            /* put the HRDB into the persistent store */
            if (ps_set_hrdb(ps_name, rawname, (char *)db.dbbuf, db.dblen32*4) != PS_OK) {
                free((char *)db.dbbuf);
                reporterr(ERRFAC, M_PSWTHRDB, ERRCRIT, rawname, ps_name);
                close(lgfd);
                close(fd);
                return -1;
            }
            free((char *)db.dbbuf);
        }

        sb.lg_num = group;
        sb.dev_num = statbuf.st_rdev; /* device id of our device */
        sb.len = attr->dev_attr_size;
        if ((sb.addr = (ftd_uint64ptr_t)malloc(sb.len)) == NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, sb.len);
            close(lgfd);
            close(fd);
            return -1;
        }

        /* get the attributes */
        if (FTD_IOCTL_CALL(fd, FTD_GET_DEV_STATE_BUFFER, &sb) < 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "Failed to get dev stats", 
                strerror(errno));
            free((char *)sb.addr);
            close(lgfd);
            close(fd);
            return -1;
        }

        /* put the attributes into the persistent store */
        {
        char *passaddr = (char *)sb.addr;
        if (ps_set_device_attr(ps_name, rawname, passaddr, sb.len) != PS_OK) {
            free((char *)sb.addr);
            reporterr(ERRFAC, M_PSWTDATTR, ERRCRIT, rawname, ps_name);
            close(lgfd);
            close(fd);
            return -1;
        }
        }
        free((char *)sb.addr);

        /* delete the device from the group */
        {
        ftd_dev_t_t passdev=statbuf.st_rdev;
        if (FTD_IOCTL_CALL(fd, FTD_DEL_DEVICE, &passdev) < 0) {
            reporterr(ERRFAC, M_DRVDELDEV, ERRCRIT, rawname, lg_name, strerror(errno));
            close(lgfd);
            close(fd);
            return -1;
        }
        }

        /* delete the devices */
        unlink(blockname);
        unlink(rawname);
    }
    close(lgfd);

    /* stuff the state into pstore */
    ps_set_group_state(ps_name, lg_name, new_state);

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

    /* move the .prf file out of the way so monitortool shows it gone */
    backup_perf_file (group);

    
    /* remove .cur file */
    if (autostart) {
        sprintf(full_cfg_path, "%s/%s", PATH_CONFIG, cfg_path);
        unlink (full_cfg_path);
    }

    return 0;
}
