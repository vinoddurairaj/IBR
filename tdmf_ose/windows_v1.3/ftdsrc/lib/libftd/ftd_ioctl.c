/*
 * ftd_ioctl.c - FTD driver interface  
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include "ftd_port.h"
#include "ftdio.h"
#include "ftd_ps.h"
#include "ftd_devio.h"
#include "ftd_ioctl.h"
#include "ftd_error.h"

/*
 * ftd_ioctl_get_bab_size --
 * gets the bab size in bytes.
 */
int
ftd_ioctl_get_bab_size(HANDLE ctlfd, int *size)
{
	int	rc;

	rc = ftd_ioctl(ctlfd, FTD_GET_BAB_SIZE, size, sizeof(int));
	if (rc != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_BAB_SIZE", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_update_lrdbs -- update the LRDBs in the driver for the group
 */
int
ftd_ioctl_update_lrdbs(HANDLE ctlfd, int lgnum, int silent)
{
	int	rc, num;

	num = lgnum;
	if ((rc = ftd_ioctl(ctlfd, FTD_UPDATE_LRDBS, &num, sizeof(int))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, "DRVERR", ERRWARN, "UPDATE_LRDBS", 
				strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_update_hrdbs -- update the HRDBs in the driver for the group
 */
int
ftd_ioctl_update_hrdbs(HANDLE ctlfd, int lgnum, int silent)
{
	int	rc, num;

	num = lgnum;
	if ((rc = ftd_ioctl(ctlfd, FTD_UPDATE_HRDBS, &num, sizeof(int))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, "DRVERR", ERRWARN, "UPDATE_HRDBS", 
				strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_set_group_state -- sets the group state in the driver
 */
int
ftd_ioctl_set_group_state(HANDLE ctlfd, int lgnum, int state)
{
    ftd_state_t	sb;
	int			rc;

	memset(&sb, 0, sizeof(sb));
	sb.lg_num = lgnum;
	sb.state = state;

    if ((rc = ftd_ioctl(ctlfd, FTD_SET_GROUP_STATE, &sb, sizeof(ftd_state_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRWARN,
            "SET_GROUP_STATE", strerror(ftd_errno()));
        return rc;
    }

    return 0;
}

/*
 * ftd_ioctl_get_group_state -- returns the group state from the driver
 */
int
ftd_ioctl_get_group_state(HANDLE ctlfd, int lgnum)
{
	ftd_state_t	lgstate;
	int			rc;

    lgstate.lg_num = lgnum;
    if ((rc = ftd_ioctl(ctlfd, FTD_GET_GROUP_STATE, &lgstate, sizeof(ftd_state_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRWARN,
            "GET_GROUP_STATE", strerror(ftd_errno()));
        return rc;
    }

    return lgstate.state;
}

/*
 * ftd_ioctl_get_group_stats -- returns the group stats from the driver
 */
int
ftd_ioctl_get_group_stats(HANDLE ctlfd, int lgnum, ftd_stat_t *lgp,
	int silent)
{
	stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
	sb.len = sizeof(ftd_stat_t);
	sb.addr = (char*)lgp;

	if ((rc = ftd_ioctl(ctlfd, FTD_GET_GROUP_STATS, &sb, sizeof(stat_buffer_t))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
				"GET_GROUP_STATE", strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_set_lrdbs --
 * sets the lrdb's in the driver for this group.
 */
int
ftd_ioctl_set_lrdbs
(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip, int ndevs)
{
	int	len, rc, i;

	if ((rc = ftd_ioctl_get_devices_info(ctlfd, lgnum, dip)) != 0) {
		return -1;
	}
	db->devs = (dev_t*)realloc(db->devs, sizeof(dev_t) * ndevs);
	len = 0;
	for (i = 0; i < ndevs; i++) {
		db->devs[i] = dip[i].cdev;
		len += dip[i].hrdbsize32;
	} 
	db->numdevs = ndevs;
	db->dblen32 = len;
	db->dbbuf = (int*)realloc(db->dbbuf, len * sizeof(int));

	memset((unsigned char*)db->dbbuf, 0xff, len*sizeof(int));
	
	if ((rc = ftd_ioctl(fd, FTD_SET_LRDBS, db, sizeof(dirtybit_array_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT,
			"SET_LRDBS", strerror(ftd_errno()));
        return rc;
    }
	
	return 0;
}

/*
 * ftd_ioctl_get_lrdbs --
 * gets the lrdb's from the driver for this group.
 */
int
ftd_ioctl_get_lrdbs
(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip)
{
	int			len, i, rc;
	ftd_stat_t	lgstat;

	// get the info for the group
	if ((rc = ftd_ioctl_get_group_stats(ctlfd, lgnum, &lgstat, 0)) < 0) {
		return -1;
	}

	if ((rc = ftd_ioctl_get_devices_info(ctlfd, lgnum, dip)) != 0) {
		return -1;
	}

	db->devs = (dev_t*)realloc(db->devs, sizeof(dev_t) * lgstat.ndevs);
	len = 0;
	for (i = 0; i < lgstat.ndevs; i++) {
		db->devs[i] = dip[i].cdev;
		len += dip[i].hrdbsize32;
	} 
	db->numdevs = lgstat.ndevs;
	db->state_change = 0;

    if ((rc = ftd_ioctl(fd, FTD_GET_LRDBS, db, sizeof(dirtybit_array_t))) != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_LRDBS", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_set_hrdbs --
 * sets the hrdb in the driver for this group.
 */
int
ftd_ioctl_set_hrdbs
(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip, int ndevs)
{
	int	len, rc, i;

	if ((rc = ftd_ioctl_get_devices_info(ctlfd, lgnum, dip)) != 0) {
		return -1;
	}
	db->devs = (dev_t*)realloc(db->devs, sizeof(dev_t) * ndevs);
	len = 0;
	for (i = 0; i < ndevs; i++) {
		db->devs[i] = dip[i].cdev;
		len += dip[i].hrdbsize32;
	} 
	db->numdevs = ndevs;
	db->dblen32 = len;
	db->dbbuf = (int*)realloc(db->dbbuf, len*sizeof(int));
	
	memset((unsigned char*)db->dbbuf, 0xff, len*sizeof(int));
	
	if ((rc = ftd_ioctl(fd, FTD_SET_HRDBS, db, sizeof(dirtybit_array_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT,
			"SET_LRDBS", strerror(ftd_errno()));
        return rc;
    }
	
	return 0;
}

/*
 * ftd_ioctl_set_hrdbs_from_buf --
 * sets the hrdb's in the driver for this device.
 */
int
ftd_ioctl_set_hrdbs_from_buf
(HANDLE ctlfd, HANDLE fd, int lgnum, dev_t *dev, int *hrdb, int len)
{
	int	rc;
	dirtybit_array_t db;

	memset(&db, 0, sizeof(dirtybit_array_t));
	db.devs = dev;
	db.numdevs = 1;
	db.dblen32 = len;
	db.dbbuf = hrdb;

	if ((rc = ftd_ioctl(fd, FTD_SET_HRDBS, &db, sizeof(dirtybit_array_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT,
			"SET_HRDBS", strerror(ftd_errno()));
        return rc;
    }
	
	return 0;
}

/*
 * ftd_ioctl_set_lrdbs_from_buf --
 * sets the lrdb's in the driver for this device.
 */
int
ftd_ioctl_set_lrdbs_from_buf
(HANDLE ctlfd, HANDLE fd, int lgnum, dev_t *dev, int *lrdb, int len)
{
	int	rc;

	dirtybit_array_t db;

	memset(&db, 0, sizeof(dirtybit_array_t));
	db.devs = dev;
	db.numdevs = 1;
	db.dblen32 = len;
	db.dbbuf = lrdb;

	if ((rc = ftd_ioctl(fd, FTD_SET_LRDBS, &db, sizeof(dirtybit_array_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT,
			"SET_LRDBS", strerror(ftd_errno()));
        return rc;
    }
	
	return 0;
}

/*
 * ftd_ioctl_get_hrdbs --
 * gets the hrdb's from the driver for this group.
 */
int
ftd_ioctl_get_hrdbs
(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip)
{
	int	len, i, rc;
	ftd_stat_t	lgstat;

	// get the info for the group
	if ((rc = ftd_ioctl_get_group_stats(ctlfd, lgnum, &lgstat, 0)) < 0) {
		return -1;
	}

	if ((rc = ftd_ioctl_get_devices_info(ctlfd, lgnum, dip)) != 0) {
		return -1;
	}

	db->devs = (dev_t*)realloc(db->devs, sizeof(dev_t) * lgstat.ndevs);
	len = 0;
	for (i = 0; i < lgstat.ndevs; i++) {
		db->devs[i] = dip[i].cdev;
		len += dip[i].hrdbsize32;
	} 

	db->numdevs = lgstat.ndevs;
	db->state_change = 0;
	
    if ((rc = ftd_ioctl(fd, FTD_GET_HRDBS, db,
		sizeof(dirtybit_array_t))) != 0)
	{
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"GET_HRDBS", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_clear_bab --
 * clears out the entries in the writelog.  Tries up to 10 
 * times to make this happen if EAGAIN is being returned.
 */
int
ftd_ioctl_clear_bab(HANDLE ctlfd, int lgnum)
{
	int	save_errno, i, rc;

    for (i = 0; i < 10; i++) {
        rc = ftd_ioctl(ctlfd, FTD_CLEAR_BAB, &lgnum, sizeof(int));
        if ((rc < 0 && ftd_errno() != EAGAIN) || rc == 0) {
            break;
        }
		save_errno = ftd_errno();
        sleep(1);
    }
	if (rc != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"CLEAR_BAB", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_clear_lrdbs --
 * clears out the lrdbs.
 */
int
ftd_ioctl_clear_lrdbs(HANDLE ctlfd, int lgnum, int silent)
{
	int	rc;

	rc = ftd_ioctl(ctlfd, FTD_CLEAR_LRDBS, &lgnum, sizeof(int));
	if (rc != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN, "CLEAR_LRDBS", strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_clear_hrdbs --
 * clears out the hrdbs.
 */
int
ftd_ioctl_clear_hrdbs(HANDLE ctlfd, int lgnum, int silent)
{
	int	rc;

	rc = ftd_ioctl(ctlfd, FTD_CLEAR_HRDBS, &lgnum, sizeof(int));
	if (rc != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN, "CLEAR_HRDBS", strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_set_iodelay --
 * Sets the iodelay in the driver for this logical group.
 */
int
ftd_ioctl_set_iodelay(HANDLE ctlfd, int lgnum, int delay)
{
	ftd_param_t	pa[1];
	int			rc;    

	pa->lgnum = lgnum;
	pa->value = delay;
    
	if ((rc = ftd_ioctl(ctlfd, FTD_SET_IODELAY, pa, sizeof(ftd_param_t))) != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"SET_IODELAY", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * set_sync_depth --
 * Sets the sync depth for the logical group in question.
 * To turn off sync mode, set the depth to -1.
 */
int
ftd_ioctl_set_sync_depth(HANDLE ctlfd, int lgnum, int sync_depth)
{
    ftd_param_t pa[1];
	int			rc;
    
    pa->lgnum = lgnum;
    pa->value = sync_depth;
    
	if ((rc = ftd_ioctl(ctlfd, FTD_SET_SYNC_DEPTH, pa, sizeof(ftd_param_t))) != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"SET_SYNC_DEPTH", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * set_sync_timeout --
 * Sets the sync timeout for the logical group. Zero indicates no timeout.
 */
int
ftd_ioctl_set_sync_timeout(HANDLE ctlfd, int lgnum, int sync_timeout)
{
	ftd_param_t	pa[1];
	int			rc;
    
    pa->lgnum = lgnum;
    pa->value = sync_timeout;
    
	if ((rc = ftd_ioctl(ctlfd, FTD_SET_SYNC_TIMEOUT, pa, sizeof(ftd_param_t))) != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"SET_SYNC_TIMEOUT", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * set_trace_level --
 * Sets the driver diagnostic trace level.  
 * In Windows, traces are directed to the System Event Log (System Log).
 */
int
ftd_ioctl_set_trace_level(HANDLE ctlfd, int level)
{
	int			rc;
    
	if ((rc = ftd_ioctl(ctlfd, FTD_SET_TRACE_LEVEL, &level, sizeof(int))) != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"SET_TRACE_LEVEL", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_set_lg_state_buffer --
 * Set the group state from the driver.
 */
int
ftd_ioctl_set_lg_state_buffer(HANDLE ctlfd, int lgnum, char *buf)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
	sb.len = FTD_PS_GROUP_ATTR_SIZE;
	sb.addr = buf;

    // set these tunables in driver memory 
	if ((rc = ftd_ioctl(ctlfd, FTD_SET_LG_STATE_BUFFER,
		&sb, sizeof(stat_buffer_t))) != 0)
	{
		reporterr(ERRFAC, M_DRVERR, ERRWARN,
			"SET_LG_STATE_BUFFER", strerror(ftd_errno()));
        return rc;
    }

	return 0;
}

/*
 * ftd_ioctl_get_lg_state_buffer --
 * Get the group state from the driver.
 */
int
ftd_ioctl_get_lg_state_buffer(HANDLE ctlfd, int lgnum,
	char *buf, int silent)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
	sb.len = FTD_PS_GROUP_ATTR_SIZE;
	sb.addr = buf;

	/* read from driver buffer */
	if ((rc = ftd_ioctl(ctlfd, FTD_GET_LG_STATE_BUFFER, &sb, sizeof(stat_buffer_t))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
				"GET_LG_STATE_BUFFER", strerror(ftd_errno()));
		}
        return rc;
    }

	return 0;
}

/*
 * ftd_ioctl_get_dev_stats -- returns the devices stats from the driver
 */
int
ftd_ioctl_get_dev_stats(HANDLE ctlfd, int lgnum, int devnum, disk_stats_t *DiskInfo, int nDiskStats)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 

	sb.lg_num = lgnum;
    sb.dev_num = devnum;
    sb.len = nDiskStats;
    sb.addr = (char *)DiskInfo;

    if ((rc = ftd_ioctl(ctlfd, FTD_GET_DEVICE_STATS, &sb, sizeof(stat_buffer_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRWARN,
            "GET_GROUP_STATE", strerror(ftd_errno()));
        return rc;
    }

    return 0;
}

/*
 * ftd_ioctl_set_dev_state_buffer --
 * Set the device state in the driver.
 */
int
ftd_ioctl_set_dev_state_buffer(HANDLE ctlfd, int lgnum, int devnum,
	int buflen, char *buf, int silent)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
	sb.dev_num = devnum;
	sb.len = buflen;
	sb.addr = buf;

    // set the tunables in driver memory 
	if ((rc = ftd_ioctl(ctlfd, FTD_SET_DEV_STATE_BUFFER,
		&sb, sizeof(stat_buffer_t))) != 0)
	{
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
				"SET_DEV_STATE_BUFFER", strerror(ftd_errno()));
		}
        return rc;
    }

	return 0;
}

/*
 * ftd_ioctl_get_dev_state_buffer --
 * Get the device state from the driver.
 */
int
ftd_ioctl_get_dev_state_buffer(HANDLE ctlfd, int lgnum, int devnum,
	int buflen, char *buf, int silent)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
	sb.dev_num = devnum;
	sb.len = buflen;
	sb.addr = buf;

	/* read from driver buffer */
	if ((rc = ftd_ioctl(ctlfd, FTD_GET_DEV_STATE_BUFFER,
		&sb, sizeof(stat_buffer_t))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
			    "GET_DEV_STATE_BUFFER", strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

// SAUMYA_FIX_INITIALIZATION_MODULE
#if 0
int
sftk_ioctl_config_begin()
{
	int	rc, num;

	num = lgnum;
	if ((rc = ftd_ioctl(ctlfd, FTD_CONFIG_BEGIN, &num, sizeof(int))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, "DRVERR", ERRWARN, "CONFIG_BEGIN", 
				strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

int
sftk_ioctl_config_end()
{
	int	rc, num;

	num = lgnum;
	if ((rc = ftd_ioctl(ctlfd, FTD_CONFIG_END, &num, sizeof(int))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, "DRVERR", ERRWARN, "CONFIG_END", 
				strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

int
sftk_ioctl_add_connections()
{
	return 1;
}

int
sftk_ioctl_remove_connections()
{
	return 1;
}

int
sftk_ioctl_enable_connections()
{
	return 1;
}

int
sftk_ioctl_disable_connections()
{
	return 1;
}

int
sftk_ioctl_launch_pmd()
{
	return 1;
}

int
sftk_ioctl_stop_all_connections()
{
	return 1;
}

int
sftk_ioctl_query_connections()
{
	return 1;
}

#endif // SAUMYA_FIX_INITIALIZATION_MODULE


/*
 * ftd_ioctl_get_groups_info --
 * Get the groups state from the driver.
 */
int
ftd_ioctl_get_groups_info(HANDLE ctlfd, int lgnum, ftd_lg_info_t *lgp, int silent)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
    sb.len = sizeof(ftd_lg_info_t);
	sb.addr = (char*)lgp;

	/* read from driver buffer */
	if ((rc = ftd_ioctl(ctlfd, FTD_GET_GROUPS_INFO, &sb, sizeof(stat_buffer_t))) != 0) {
        if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
		        "GET_GROUPS_INFO", strerror(ftd_errno()));
        }
		return rc;
    }

	return 0;
}

/*
 * ftd_ioctl_get_devices_info --
 * Get the device state from the driver.
 */
int
ftd_ioctl_get_devices_info(HANDLE ctlfd, int lgnum, ftd_dev_info_t *dip)
{
    stat_buffer_t       sb;
	int					rc;

	memset(&sb, 0, sizeof(stat_buffer_t)); 
	
	sb.lg_num = lgnum;
	sb.addr = (char*)dip;

	/* read from driver buffer */
	if ((rc = ftd_ioctl(ctlfd, FTD_GET_DEVICES_INFO, &sb, sizeof(stat_buffer_t))) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRWARN,
            "GET_LG_STATE_BUFFER", strerror(ftd_errno()));
        return rc;
    }

	return 0;
}

/*
 * ftd_ioctl_oldest_entries --
 * Get the oldest entries in the BAB.
 */
int
ftd_ioctl_oldest_entries(HANDLE devfd, oldest_entries_t *oe)
{
	int					rc;

	/* read from driver buffer */
	if ((rc = ftd_ioctl(devfd, FTD_OLDEST_ENTRIES,
		oe, sizeof(oldest_entries_t))) != 0) {
		if (ftd_errno() != EINVAL) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
				"GET_OLDEST_ENTRIES", strerror(ftd_errno()));
		}
        return rc;
    }

	return 0;
}

/*
 * ftd_ioctl_send_lg_message --
 * Insert a sentinel into the BAB.
 */
int
ftd_ioctl_send_lg_message(HANDLE devfd, int lgnum, char *sentinel)
{
	stat_buffer_t	sb;
    char			msgbuf[DEV_BSIZE];
    int				maxtries, rc, i, sent, errnum;

    memset(msgbuf, 0, sizeof(msgbuf));
    strcpy(msgbuf, sentinel); 
    memset(&sb, 0, sizeof(stat_buffer_t));
    
	sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = msgbuf;

    maxtries = 50;
	sent = 0;
    
    for (i = 0; i < maxtries; i++) 
    {
        rc = ftd_ioctl(devfd, FTD_SEND_LG_MESSAGE, &sb, sizeof(stat_buffer_t));
        if (rc != 0) 
        {
			errnum = ftd_errno();
            if (errnum == EAGAIN) 
            {
                sleep(50);
                continue;
            }
            if (errnum == ENOENT)
            {
                reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, "BAB FULL - cannot allocate entry\n");
                return -1;
            }
            else
            {
            reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, strerror(errnum));
            return -1;
            }
        } 
        else 
        {
            sent = 1;
			break;
        }
    }

    if (!sent) 
    {
        reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, "BAB busy clearing memory - cannot allocate entry\n");
		return -1;
	}

	return 0;
}

/*
 * ftd_ioctl_del_lg --
 * delete the group from the driver
 */
int
ftd_ioctl_del_lg(HANDLE ctlfd, int lgdev, int silent)
{
	int	rc;

	/* delete the lg from the driver */
	if ((rc = ftd_ioctl(ctlfd, FTD_DEL_LG, &lgdev, sizeof(int))) != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN, "DEL_LG", strerror(ftd_errno()));
		}
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_del_device --
 * delete the device from the driver
 */
int
ftd_ioctl_del_device(HANDLE ctlfd, int devnum)
{
	int	rc;

	/* delete the device from the driver */
	if ((rc = ftd_ioctl(ctlfd, FTD_DEL_DEVICE, &devnum, sizeof(int))) != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN, "DEL_DEVICE", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_get_device_nums --
 * get the device numbers for the target device.
 */
int
ftd_ioctl_get_device_nums(HANDLE ctlfd, ftd_devnum_t *devnum)
{
	int	rc;

	rc = ftd_ioctl(ctlfd, FTD_GET_DEVICE_NUMS, devnum, sizeof(ftd_devnum_t));
	if (rc != 0) {
		reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_DEVICE_NUMS", strerror(ftd_errno()));
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_new_lg --
 * create a new lg.
 */
int
ftd_ioctl_new_lg(HANDLE ctlfd, ftd_lg_info_t *info, int silent)
{
	int	rc, errnum;

	rc = ftd_ioctl(ctlfd, FTD_NEW_LG, info, sizeof(ftd_lg_info_t));
	errnum = ftd_errno();
	if (rc != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
				"NEW_LG", strerror(errnum));
		}
		errno = errnum;
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_new_device --
 * create a new device.
 */
int
ftd_ioctl_new_device(HANDLE ctlfd, ftd_dev_info_t *info, int silent)
{
	int	rc, errnum;

	rc = ftd_ioctl(ctlfd, FTD_NEW_DEVICE, info, sizeof(ftd_dev_info_t));
	errnum = ftd_errno();
	if (rc != 0) {
		if (!silent) {
			reporterr(ERRFAC, M_DRVERR, ERRWARN,
				"NEW_DEVICE", strerror(errnum));
		}
		errno = errnum;
		return rc;
	}

	return 0;
}

/*
 * ftd_ioctl_migrate --
 * migrate entries out of the BAB.
 */
int
ftd_ioctl_migrate(HANDLE devfd, int lgnum, int bytes)
{
	migrated_entries_t	mig;
	int					rc;

	if (bytes & 0x7) {                           /* DUMP CORE! */
		*(char*) 0 = 1;
	}
	mig.bytes = bytes;

	rc = ftd_ioctl(devfd, FTD_MIGRATE, &mig, sizeof(migrated_entries_t));
	if (rc != 0) {
		reporterr(ERRFAC, M_MIGRATE, ERRCRIT,
			lgnum, strerror(ftd_errno())); 
		return -1; 
    }

	return 0;
}

/*
 * ftd_ioctl_init_stop --
 * make sure group devs are closed and prevent opens.
 */
int
ftd_ioctl_init_stop(HANDLE devfd, int lgnum, int silent)
{
	stat_buffer_t	sb;

	memset(&sb, 0, sizeof(stat_buffer_t));

	if (ftd_ioctl(devfd, FTD_INIT_STOP, &sb, sizeof(sb)) != 0) 
	{
		if (ftd_errno() == EBUSY) 
		{
			if (!silent) 
			{
				reporterr(ERRFAC, M_DEVOPEN, ERRCRIT, lgnum);
			}
			return -1;
		} 
		else 
		{
			if (!silent) 
			{
				reporterr(ERRFAC, M_DRVERR, ERRCRIT, "INIT_STOP", strerror(ftd_errno()));
			}
			return -1;
		}
	}

	return 0;
}

/*
 *  Function: 		
 *		ULONG	Sftk_Get_TotalLgCount()
 *
 *  Arguments: 	
 * 				...
 * Returns: returns Total Number of LG configured in driver
 *
 * Description:
 *		sends FTD_GET_NUM_GROUPS to driver and retrieves Total Num of LG configured in driver
 */
ULONG
Sftk_Get_TotalLgCount(HANDLE ctlfd)
{
	DWORD	status;
	DWORD	retLength = 0;
	ULONG	totalLg = 0;

	status = sftk_DeviceIoControl(	NULL, 
									ctlfd,
									FTD_GET_NUM_GROUPS,
									&totalLg,				// LPVOID lpInBuffer,
									sizeof(totalLg),		// DWORD nInBufferSize,
									&totalLg,				// LPVOID lpOutBuffer,
									sizeof(totalLg),		// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_NUM_GROUPS", FTD_CTLDEV);
		return 0;
	}      

	return totalLg;
} // Sftk_Get_TotalLgCount()

/*
 *  Function: 		
 *		ULONG	Sftk_Get_TotalDevCount()
 *
 *  Arguments: 	
 * 				...
 * Returns: returns Total Number of Dev configured for all LG in driver
 *
 * Description:
 *		sends FTD_GET_DEVICE_NUMS to driver and retrieves Total Num of Devices exist for all LG in driver
 */
ULONG
Sftk_Get_TotalDevCount(HANDLE ctlfd)
{
	DWORD			status;
	ftd_devnum_t	devNum;
	DWORD			retLength = 0;
	ULONG			totalDev = 0;

	RtlZeroMemory(&devNum, sizeof(devNum));
	status = sftk_DeviceIoControl(	NULL, ctlfd,FTD_GET_DEVICE_NUMS, 
									&devNum,	sizeof(devNum),			// Input info 
									&devNum,	sizeof(devNum),			// OutPut Info
									&retLength );			
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n","FTD_GET_DEVICE_NUMS", FTD_CTLDEV);
		return 0;
	}      
	totalDev = devNum.b_major;

	return totalDev;
} // Sftk_Get_TotalDevCount()

/*
 *  Function: 		
 *		ULONG	Sftk_Get_TotalLgDevCount()
 *
 *  Arguments: 	
 * 				...
 * Returns: returns Total Number of Devices for specified LG configured in driver
 *
 * Description:
 *		sends FTD_GET_NUM_GROUPS to driver and retrieves Total Num of LG configured in driver
 */
ULONG
Sftk_Get_TotalLgDevCount(HANDLE ctlfd, ULONG LgNum)
{
	DWORD	status;
	DWORD	retLength = 0;
	ULONG	totalDev = LgNum;

	status = sftk_DeviceIoControl(	NULL, 
									ctlfd,
									FTD_GET_NUM_DEVICES,
									&totalDev,				// LPVOID lpInBuffer,
									sizeof(totalDev),		// DWORD nInBufferSize,
									&totalDev,				// LPVOID lpOutBuffer,
									sizeof(totalDev),		// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_NUM_DEVICES", FTD_CTLDEV);
		return 0;
	}      

	return totalDev;
} // Sftk_Get_TotalLgDevCount()

/*
 *  Function: 		
 *		DWORD	sftk_DeviceIoControl(...)
 *
 *  Arguments: 	HandleDevice : Valid Handle else pass NULL or INVALID_HANDLE_VALUE
 *				DeviceName	 : Pass string to which IOControlCode need to send
 * 				...
 * Returns: Execute IOCTL on specify device
 *
 * Description:
 *		return NO_ERROR on success else returns SDK GetLastError() 
 */
DWORD
sftk_DeviceIoControl(	IN	HANDLE	HandleDevice,
						IN	PCHAR	DeviceName,
						IN	DWORD	IoControlCode,
						IN	LPVOID	InBuffer,
						IN	DWORD	InBufferSize,
						OUT LPVOID	OutBuffer,
						IN	DWORD	OutBufferSize,
						OUT LPDWORD	BytesReturned)
{
	HANDLE	handle = HandleDevice;
	BOOL	bret;
	DWORD	status = NO_ERROR;

	if ( (HandleDevice == INVALID_HANDLE_VALUE) || (HandleDevice == NULL))
	{ // Open the device locally
		handle = CreateFile(	DeviceName,
								GENERIC_READ | GENERIC_WRITE,
								(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE),
    							NULL,
    							OPEN_EXISTING,
    							0,		// FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
    							NULL);

		if(handle==INVALID_HANDLE_VALUE)
		{ // failed !!
			status = GetLastError();
			printf("Error : CreateFile( Device : '%s') Failed with SDK Error %d \n", 
						DeviceName, status);
			//GetErrorText();
			goto done;
		}
	}
	else
		handle = HandleDevice;

	bret = DeviceIoControl(		handle,
								IoControlCode,
								InBuffer,
								InBufferSize,
								OutBuffer,
								OutBufferSize,
								BytesReturned,
								NULL);
	if (!bret) 
	{ // Zero indicated Failuer
		status = GetLastError(); 
		printf("Error : DeviceIoControl( Device:'%s', IOCTL %d (0x%08x)) Failed with SDK Error %d \n", 
						DeviceName, IoControlCode, IoControlCode,status);
		//GetErrorText();
		goto done;
	}
	
	status = NO_ERROR;	// success
done:
	if ( !((HandleDevice == INVALID_HANDLE_VALUE) || (HandleDevice == NULL)) )
	{
		if ( !((handle == INVALID_HANDLE_VALUE) || (handle == NULL)) )
			CloseHandle(handle);
	}

	return status;
} // sftk_DeviceIoControl()

/*
 *  Function: 		
 *		DWORD
 *		Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size )
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and All_LgStats has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_ALL_STATS_INFO to driver and retrieves All LG and their Devices Stats info from driver
 */
DWORD
Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size )
{
	DWORD			status;
	DWORD			retLength = 0;

	status = sftk_DeviceIoControl(	NULL, FTD_CTLDEV,FTD_GET_ALL_STATS_INFO, 
									All_LgStats,	Size,			// Input info 
									All_LgStats,	Size,			// OutPut Info
									&retLength );			
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. LastError %d\n",
							"FTD_GET_ALL_STATS_INFO", FTD_CTLDEV, GetLastError());
		//GetErrorText();
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_AllStatsInfo()


