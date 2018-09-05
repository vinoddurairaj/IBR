/*
 * ftd_lg.c - FTD logical group interface
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

#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_dev.h"
#include "ftd_ioctl.h"
#include "ftd_ps.h"
#include "ftd_error.h"
#include "ftd_sock.h"
#include "ftd_lg_states.h"
#include "ftd_platform.h"
#include "ftd_throt.h"
#include "ftd_stat.h"
#include "misc.h"
#include "llist.h"
#include "ftdio.h"

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

int bDbgLogON =0;

static int ftd_lg_parse_state(ftd_lg_t *lgp, char *buf, int buflen);
static int ftd_lg_parse_state_value(char *key, char *value,
	ftd_tune_t *tunables, int verify, int set);
static int ftd_lg_journal_apply_done(ftd_lg_t *lgp);

static int ftd_lg_journal_chunk_apply(ftd_lg_t *lgp);
static int ftd_lg_journal_get_next_file(ftd_lg_t *lgp);

static int ftd_lg_add_group(ftd_lg_t *lgp,
	int hostid, int *state, int *regen_hrdb,
	int *grpstarted, int autostart);
static int ftd_lg_add_devs(ftd_lg_t *lgp,
	ftd_stat_t *lgstat, int regen, int autostart, int grpstarted);
static int ftd_lg_update_bitmaps(ftd_lg_t *lgp,
	ftd_stat_t *lgstat, int silent);
static int ftd_lg_rem_group(ftd_lg_t *lgp,
	int state, int autostart, int silent);
static int ftd_lg_rem_devs(ftd_lg_t *lgp,
	ftd_stat_t *lgstat, int autostart, int silent, int all);
static int ftd_lg_cleanup(ftd_lg_t *lgp);

/*
 * ftd_lg_create -- create a ftd_lg_t object
 */
ftd_lg_t *
ftd_lg_create(void)
{
	ftd_lg_t *lgp;

    /* Allocates ONE ftd_lg_t array in memory with elements initialized to 0. */
	if ((lgp = (ftd_lg_t*)calloc(1, sizeof(ftd_lg_t))) == NULL) {
		return NULL;
	}
	/*   */
	if ((lgp->tunables = (ftd_tune_t *)calloc(1, sizeof(ftd_tune_t))) == NULL) {
		goto errexit;
	}
	if ((lgp->db = (dirtybit_array_t *)calloc(1, sizeof(dirtybit_array_t))) == NULL) {
		goto errexit;
	}
	if ((lgp->cfgp = ftd_config_lg_create()) == NULL) {
		goto errexit;
	}
#if defined(_WINDOWS)
	if ((lgp->fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
		goto errexit;
	}
#else
	if ((lgp->fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) {
		goto errexit;
	}
#endif
	if ((lgp->isockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
		goto errexit;
	}
	if ((lgp->dsockp = ftd_sock_create(FTD_SOCK_INET)) == NULL) {
		goto errexit;
	}
	if ((lgp->devlist = CreateLList(sizeof(ftd_dev_t**))) == NULL) {
		goto errexit;
	}
	if ((lgp->throttles = CreateLList(sizeof(throttle_t))) == NULL) {
		goto errexit;
	}
	if ((lgp->statp = calloc(1, sizeof(ftd_lg_stat_t))) == NULL) {
		goto errexit;
	}
	if ((lgp->compp = comp_create(0)) == NULL) {
		goto errexit;
	}

	lgp->ctlfd = INVALID_HANDLE_VALUE;
	lgp->devfd = INVALID_HANDLE_VALUE;

#if defined(_WINDOWS)
	lgp->perffd = INVALID_HANDLE_VALUE;
	lgp->babfd = INVALID_HANDLE_VALUE;
#endif

	return lgp;

errexit:

	ftd_lg_delete(lgp);
		
	return NULL;
}

/*
 * ftd_lg_cleanup -- remove lg object state
 */
static int
ftd_lg_cleanup(ftd_lg_t *lgp)
{
	ftd_dev_t	**devpp;

	if (lgp == NULL || lgp->magicvalue != FTDLGMAGIC) 
		return 0;

	if (lgp->isockp)
		ftd_sock_delete(&lgp->isockp);

	if (lgp->dsockp)
		ftd_sock_delete(&lgp->dsockp);
	
	if (lgp->jrnp)
		ftd_journal_delete(lgp->jrnp);

	if (lgp->cfgp) 
		ftd_config_lg_delete(lgp->cfgp);

	if (lgp->fprocp)
		ftd_proc_delete(lgp->fprocp);

	if (lgp->devlist) {
		if (SizeOfLL(lgp->devlist) > 0) {
			ForEachLLElement(lgp->devlist, devpp) {
				ftd_dev_delete((*devpp));
			}
		}
		FreeLList(lgp->devlist);
	}
		
	if (lgp->tunables)
		free(lgp->tunables);

	if (lgp->cbuf)
		free(lgp->cbuf);

	if (lgp->buf)
		free(lgp->buf);

	if (lgp->recvbuf)
		free(lgp->recvbuf);

	if (lgp->db)
	{
		if (lgp->db->dbbuf)
			free(lgp->db->dbbuf);

		free(lgp->db);
	}

	if (lgp->ctlfd && lgp->ctlfd != INVALID_HANDLE_VALUE)
	{
		ftd_close(lgp->ctlfd);
		lgp->ctlfd = INVALID_HANDLE_VALUE;
	}

	ftd_lg_close(lgp);

	if (lgp->throttles)
		FreeLList(lgp->throttles);

	if (lgp->statp)
		free(lgp->statp);

#if !defined(_WINDOWS)
	if (lgp->sigpipe[STDIN_FILENO] > 0) {
		close(lgp->sigpipe[STDIN_FILENO]);
	}
	if (lgp->sigpipe[STDOUT_FILENO] > 0) {
		close(lgp->sigpipe[STDOUT_FILENO]);
	}
#endif

	if (lgp->compp)
		comp_delete(lgp->compp);

	return 0;
}

/*
 * ftd_lg_delete -- delete a ftd_lg_t object
 */
int
ftd_lg_delete(ftd_lg_t *lgp)
{

	if (lgp == NULL) {
		return -1;
	}

	if (ftd_lg_cleanup(lgp) < 0) {
		return 0;
	}

	free(lgp);

	return 0;
}

/*
 * ftd_lg_dev_to_list -- add a ftd_dev_t object to linked list
 */
int
ftd_lg_add_dev_to_list(LList *devlist, ftd_dev_t **devpp)
{

	AddToTailLL(devlist, devpp);

	return 0;
}

/*
 * ftd_lg_remove_dev_from_list -- remove a ftd_dev_t object from linked list
 */
int
ftd_lg_remove_dev_from_list(LList *devlist, ftd_dev_t **devpp)
{

	RemCurrOfLL(devlist, devpp);

	return 0;
}

/*
 * ftd_lg_init -- initialize the ftd_lg_t object
 */
int
ftd_lg_init(ftd_lg_t *lgp, int lgnum, int role, int startflag)
{
	ftd_dev_cfg_t	**devcfgpp, *devcfgp;
	ftd_dev_t		**devpp, *devp;
	int				dev_bsize;
	int				found, i, ndevs, num = 0, rc, size = -1;
#if defined(_WINDOWS)
	disk_stats_t	*devstat = NULL;
	HANDLE fd;
#endif

	if (lgp->magicvalue == FTDLGMAGIC) {
		ftd_lg_cleanup(lgp);
	}

	lgp->lgnum = lgnum;

#if !defined(_WINDOWS)
	if (pipe(lgp->sigpipe) < 0) {
		return -1;
	}
#endif

	if ((rc = ftd_config_read(lgp->cfgp, lgp->lgnum, role, startflag)) < 0) {
		ftd_config_lg_delete(lgp->cfgp);
		return rc;
	}

	if (lgp->cfgp->role == ROLEPRIMARY) {
		// open the master control device 
		if ((lgp->ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
			reporterr(ERRFAC, M_CTLOPEN, ERRCRIT,
				FTD_CTLDEV, ftd_strerror());
			goto errret;
		}

		FTD_CREATE_GROUP_NAME(lgp->devname, lgp->lgnum);

		// get actual BAB size
		dev_bsize = DEV_BSIZE;
		lgp->babsize = ftd_ioctl_get_bab_size(lgp->ctlfd, &dev_bsize);
		if (lgp->babsize < 0) {
			goto errret;
		}
		lgp->offset = 0;
		lgp->datalen = 0;
		lgp->devfd = INVALID_HANDLE_VALUE;
	}

#if defined(_WINDOWS)
	if ( (role == ROLEPRIMARY) && (startflag) ) {
		ndevs = SizeOfLL(lgp->cfgp->devlist);

		devstat = calloc(1, ndevs * sizeof(disk_stats_t));
		if (devstat == NULL) {
			reporterr(ERRFAC, M_MALLOC, (ndevs * sizeof(disk_stats_t)));
			goto errret;
		}

        if (ftd_ioctl_get_dev_stats(lgp->ctlfd, lgp->lgnum, -1, devstat, ndevs) < 0) {
			goto errret;
		}
	}

#endif

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		
		devcfgp = *devcfgpp;

		if ((devp = ftd_dev_create()) == NULL) {
			goto errret;
		}

 		if (role == ROLEPRIMARY) {
#if defined(_WINDOWS)
			if (startflag) {
				
				found = FALSE;

				// only if group has been started 
				// otherwise no sense in trying to get dev nums
				for (i = 0; i < ndevs; i++) {
					if (!strcmp(devcfgp->devname, devstat[i].devname + strlen(devstat[i].devname) - 2)) {
						// found it
						found = TRUE;
						break;
					}
				}

				if (!found) {
					// skip it
					continue;
				}

				num = devstat[i].localbdisk;
			}

			if (lgp->cfgp->chaining) {
 				fd = ftd_dev_lock(devcfgp->pdevname, lgp->lgnum);
				if (fd == INVALID_HANDLE_VALUE) {
					reporterr(ERRFAC, M_DEVLOCK, ERRCRIT, devcfgp->pdevname, ftd_strerror());

					goto errret;
				}

 				size = ftd_dev_locked_disksize(fd);

 				if (ftd_dev_unlock(fd) == 0)
 					ftd_close(fd);
 			} else {
 				size = disksize(devcfgp->pdevname);
			}
#else
 			size = disksize(devcfgp->pdevname);
#endif
			// num not used by UNIX
			ftd_dev_init(devp, devcfgp->devid,
				devcfgp->pdevname, devcfgp->devname, size, num);
 		} else {
#if defined(_WINDOWS)
			if (!startflag) {
 				fd = ftd_dev_lock(devcfgp->sdevname, lgp->lgnum);
				if (fd == INVALID_HANDLE_VALUE) {
					reporterr(ERRFAC, M_DEVLOCK, ERRCRIT, devcfgp->sdevname, ftd_strerror());

					goto errret;
				}

 			size = ftd_dev_locked_disksize(fd);
			} else {
			fd = NULL;
            size = 0; 
			}

 			if (ftd_dev_unlock(fd) == 0)
 				ftd_close(fd);
#else
 			size = disksize(devcfgp->sdevname);
#endif
			// num not used by UNIX
			ftd_dev_init(devp, devcfgp->devid,
				devcfgp->sdevname, devcfgp->sdevname, size, num);
		}

		if (ftd_lg_add_dev_to_list(lgp->devlist, &devp) < 0) {
			goto errret;
		}
	}
		
	lgp->magicvalue = FTDLGMAGIC;

#if defined(_WINDOWS)
	if (devstat) {
		free(devstat);
	}
#endif

	return 0;

errret:

#if defined(_WINDOWS)
	if (devstat) {
		free(devstat);
	}
#endif

	ForEachLLElement(lgp->devlist, devpp) {
		ftd_dev_delete(*devpp);
	}

	return -1;
}

int
ftd_lg_close(ftd_lg_t *lgp)
{

	if (lgp->devfd && lgp->devfd != INVALID_HANDLE_VALUE)
	{
		ftd_close(lgp->devfd);
		lgp->devfd = INVALID_HANDLE_VALUE;
	}

#if defined(_WINDOWS)

	// close these here since they were opened in lg_open

	if (lgp->babfd != INVALID_HANDLE_VALUE )
	{
		ftd_close(lgp->babfd);
		lgp->babfd = INVALID_HANDLE_VALUE ;
	}

	if (lgp->perffd)
	{
		ftd_close(lgp->perffd);
		lgp->perffd = NULL;
	}
#endif

	return 0;
}

/*
 * ftd_lg_open --
 * open the logical group control device 
 */
int
ftd_lg_open(ftd_lg_t *lgp)
{
	
	FTD_CREATE_GROUP_NAME(lgp->devname, lgp->lgnum);

	// open the group control device 
	lgp->devfd = ftd_open(lgp->devname, O_RDWR, 0);

	if (lgp->devfd == INVALID_HANDLE_VALUE) {
		reporterr(ERRFAC, M_LGDOPEN, ERRCRIT,
			lgp->devname, ftd_strerror());
		return -1;
	}

#if defined(_WINDOWS)
	{
		char event[_MAX_PATH];
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;

		// Initialize a security descriptor and assign it a NULL 
		// discretionary ACL to allow unrestricted access. 
		// Assign the security descriptor to a file. 
		if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
			return FALSE;
		if (!SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE))
			return FALSE;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = &sd;
		sa.bInheritHandle = FALSE;
		
		FTD_CREATE_GROUP_BAB_EVENT_NAME(event, lgp->lgnum);
		if ((lgp->babfd =
			OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, event)) == NULL)
		{
			lgp->babfd = INVALID_HANDLE_VALUE;

			reporterr(ERRFAC, M_OPENEVENT, ERRCRIT,
				event, ftd_strerror());
			return -1;
		}

		FTD_PERF_GROUP_GET_EVENT_NAME(event, lgp->lgnum);
		if ((lgp->perffd = CreateEvent(&sa, FALSE, FALSE, event)) == NULL) {
			lgp->perffd = INVALID_HANDLE_VALUE;

			reporterr(ERRFAC, M_CRTEEVENT, ERRCRIT,
				event, ftd_strerror());
			return -1;
		}
	}
#endif

	return 0;
}

/*
 * ftd_lg_backup_perf_file --
 * make a copy of the lg performance file
 */
static void
ftd_lg_backup_perf_file(ftd_lg_t *lgp)
{
	char perfpath[MAXPATHLEN], perfpathbackup[MAXPATHLEN];
	struct stat statbuf;

	sprintf(perfpath, "%s/p%03d.prf", PATH_RUN_FILES, lgp->lgnum);
	sprintf(perfpathbackup, "%s/p%03d.prf.1", PATH_RUN_FILES, lgp->lgnum);

	if (0 == stat(perfpath, &statbuf)) {
		if (0 == stat(perfpathbackup, &statbuf)) {
			(void)unlink(perfpathbackup);
		}
		(void)rename(perfpath, perfpathbackup);
	}

	return;
}

/*
 * ftd_lg_init_devs --
 * initialize the state of the internal logical group devices
 */
int
ftd_lg_init_devs(ftd_lg_t *lgp)
{
	ftd_dev_t	**devpp, *devp;
	int			csnum;

	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;
				
		if (devp->deltamap) {
			FreeLList(devp->deltamap);
			devp->deltamap = NULL;
		}
		if (devp->sumbuf) {
			free(devp->sumbuf);
			devp->sumbuf = NULL;
		}
		
		devp->deltamap = CreateLList(sizeof(ftd_dev_delta_t));
		csnum = (lgp->tunables->chunksize / CHKSEGSIZE) + 2;
		devp->sumbuflen = 2 * (csnum * DIGESTSIZE);
		devp->sumbuf = (char*)realloc(devp->sumbuf, devp->sumbuflen);
		devp->sumcnt = 2;
		devp->sumnum = csnum;

		devp->dbres = CHKSEGSIZE + 1;
		devp->rsyncoff = 0;
		devp->rsyncackoff = 0;
		devp->rsyncdelta = 0;
		devp->rsyncdone = 0;
		devp->rsynclen = 0;
		devp->rsyncbytelen = 0;
		devp->dirtyoff = 0;
		devp->dirtylen = 0;

		devp->statp->rsyncoff = 0;
		devp->statp->rsyncdelta = 0;
	}

	return 0;
}

/*
 * ftd_lg_add_devs --
 * add devices state to driver and pstore
 */
static int
ftd_lg_add_devs(ftd_lg_t *lgp,
	ftd_stat_t *lgstat, int regen, int autostart, int grpstarted)
{
	ftd_dev_cfg_t	**devcfgpp, *devcfgp;
	ftd_dev_t		*devp;
	ftd_dev_info_t	*devinfop = NULL;
	int				devidx, devadd = FALSE;
	char			buf[MAXPATH];
#if defined(_WINDOWS)	
	part_t			part, *partp;
	LList			*partlist;
	BOOL			bDismounted = FALSE;
#endif	

	if ((devinfop = (ftd_dev_info_t*)
		calloc(1, ((FTD_MAX_GROUPS*FTD_MAX_DEVICES) * sizeof(ftd_dev_info_t)))) == NULL)
	{
		reporterr(ERRFAC, M_MALLOC, ERRCRIT,
			(FTD_MAX_GROUPS*FTD_MAX_DEVICES*sizeof(ftd_dev_info_t)));
		return -1;
	}

#if defined(_WINDOWS)

	if ((partlist = CreateLList(sizeof(part_t))) == NULL) {
		goto errret;
	}

	// loop thru the list of devices and unmount - save symbolic name 
	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		
		devcfgp = *devcfgpp;
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			goto errret;
		}
	
		if (!QueryDosDevice(devcfgp->pdevname, devcfgp->vdevname, MAXPATHLEN)) {
			sprintf(buf, "can't get symbolic name for %s", devcfgp->pdevname);
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, buf, ftd_strerror());
			goto errret;
		}
		
		devp->devsize = disksize(devcfgp->pdevname);
		if (devp->devsize == -1) {
			char	buf[128];
			sprintf(buf, "can't get disk size for %s", devcfgp->pdevname);
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, buf, ftd_strerror());
			goto errret;
		}

		do {
			bDismounted = DeleteAVolume(devcfgp->pdevname);
		} while ( !bDismounted && (ftd_lockmsgbox(devcfgp->pdevname) == 0) );

		if (!bDismounted) {
			reporterr(ERRFAC, M_DISMOUNT, ERRCRIT,
					devcfgp->devname, lgp->devname);
			goto errret;
		}

		// save the names
		strcpy(part.pdevname, devcfgp->pdevname);
		strcpy(part.symname, devcfgp->vdevname);

		AddToTailLL(partlist, &part);
	}
#else
	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		
		devcfgp = *devcfgpp;
		
		if (stat(devcfgp->pdevname, &sb)) {
			reporterr(ERRFAC, M_STAT, ERRCRIT, devcfgp->devname, ftd_strerror());
			goto errret;
		}
	}
#endif
	// create the devices
	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			goto errret;
		}

		// add it
		if (ftd_dev_add(lgp->ctlfd,
			lgp->devfd, 
			lgp->cfgp->pstore,
			lgp->devname,
			lgp->lgnum,
#if defined(_WINDOWS)
			(*devcfgpp)->vdevname,
#else
			(*devcfgpp)->pdevname,
#endif
			(*devcfgpp)->devname,
			grpstarted,
			regen,
			&devidx,
			devp->devsize,
			devinfop) < 0)
		{
			goto errret;
		}

		devadd = TRUE;

		// initialize it
		if (ftd_dev_init(devp,
			(*devcfgpp)->devid,
			(*devcfgpp)->pdevname,
			(*devcfgpp)->devname,
			devinfop[devidx].disksize,
			devinfop[devidx].bdev) < 0)
		{
			goto errret;
		}
	}

#if defined(_WINDOWS)
	// recreate the partitions
	ForEachLLElement(partlist, partp) {
		CreateAVolume(partp->pdevname, partp->symname);
	}
	FreeLList(partlist);
#endif

	if (devinfop) {
		free(devinfop);
	}

	return 0;

errret:

#if defined(_WINDOWS)
	// recreate the partitions
	ForEachLLElement(partlist, partp) {
		CreateAVolume(partp->pdevname, partp->symname);
	}
	FreeLList(partlist);
#endif

	if (devadd) {
		// get the stats for the group - will be different than when passed in
		if (ftd_ioctl_get_group_stats(lgp->ctlfd, lgp->lgnum, lgstat, 1) < 0)
		{
			goto errret;
		}
		// some devs may have been added - remove them
		ftd_lg_rem_devs(lgp, lgstat, autostart, 1, 1);
	}

	if (devinfop) {
		free(devinfop);
	}

	return -1;
}

/*
 * ftd_lg_add_group --
 * add the logical group state to the driver and pstore
 */
static int
ftd_lg_add_group(ftd_lg_t *lgp, int hostid, int *state, int *regen_hrdb,
	int *grpstarted, int autostart)
{
	char                *buffer = NULL, *psname, *groupname;
	ftd_lg_info_t       info;
	int                 rc;
	ps_group_info_t     group_info;
	HANDLE			    fd;

#if defined(HPUX) || defined(_WINDOWS)
	char                raw_name[MAXPATHLEN];
#if defined(HPUX)
	ftd_devnum_t        dev_num;
#endif
	dev_t               dev;
#else 
	struct stat         statbuf;
#endif

#if defined(_WINDOWS)
	char				szPartition[MAXPATHLEN];
#endif

	*state = 0;
	*regen_hrdb = 0;
	*grpstarted = 0;

	psname = lgp->cfgp->pstore;
	groupname = lgp->devname;

	if ((buffer = (char *)calloc(1, FTD_PS_GROUP_ATTR_SIZE)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, FTD_PS_GROUP_ATTR_SIZE);
		goto errret;
	}

	// get the group info to see if this group exists
	group_info.name = NULL;

	rc = ps_get_group_info(psname, groupname, &group_info);
	
	if (rc == PS_GROUP_NOT_FOUND) {
		
		// if attributes don't exist, add the group to the persistent store 
		group_info.name = groupname;
		group_info.hostid = hostid;
		
		// set default attributes 
		strcpy(buffer, FTD_DEFAULT_TUNABLES);
		
		rc = ps_create_group(psname, &group_info,
			buffer, FTD_PS_GROUP_ATTR_SIZE, FTD_MODE_PASSTHRU);
		
		if (rc != PS_OK) {
			// we're hosed
			reporterr(ERRFAC, M_PSADDGRP, ERRCRIT, groupname, psname);
			goto errret;
		}
		
		*state = FTD_MODE_PASSTHRU;

	} else if (rc == PS_OK) {
		
		// see if the group was shutdown properly 
		if (group_info.shutdown != 1) {

			// shutdown isn't needed for PASSTHRU mode 
			// BACKFRESH doesn't add anything to the driver 

			if (group_info.state == FTD_MODE_BACKFRESH) {
				//free(buffer);

				*state = FTD_MODE_BACKFRESH;

				//return 0;
			} else {
				// regenerate the high res dirty bits 
				*regen_hrdb = 1;

				// mode must be set to TRACKING 
				*state = FTD_MODE_TRACKING;

				ps_set_group_state(psname, groupname, FTD_MODE_TRACKING);
			}
		} else {
			*state = group_info.state;
		}
	} else {
		// other error. we're hosed. 
		reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, groupname, psname);
		goto errret;
	}


#if defined(_WINDOWS)

#ifdef OLD_PSTORE
	force_dsk_or_rdsk(raw_name, psname, 1);
#else
	/* Touch the PStore file only. */
	fd = ftd_open(psname, O_RDWR|O_SYNC, 0);
	ftd_close(fd);

	sprintf(raw_name, "%s", psname);  
	sprintf(psname, "\\??\\%s", raw_name);
	info.vdevname = psname;
#endif

#else
	if (stat(psname, &statbuf) != 0) {
		reporterr(ERRFAC, M_PSSTAT, ERRCRIT, psname);
		goto errret;
	}
#endif

#if defined(_AIX)
/* 
 * _AIX (struct stat *)p->st_dev is the device of 
 * the parent directory of this node: "/". ouch. 
 */
	info.persistent_store = statbuf.st_rdev;
#elif !defined(_WINDOWS) /* defined(_AIX) */
	info.persistent_store = statbuf.st_rdev;
#endif /* defined(_AIX) */

#if defined(HPUX)

	if (ftd_ioctl_get_device_nums(lgp->ctlfd, &dev_num) != 0) {
		goto errret;
	}

	ftd_make_group(group, dev_num.c_major, &dev);

	info.lgdev = dev;        // both the major and minor numbers 
#elif defined(SOLARIS) || defined(_AIX)
	{
		struct stat csb;
		fstat(lgp->ctlfd, &csb);
		info.lgdev = makedev(major(csb.st_rdev), lgp->lgnum | FTD_LGFLAG);
	}
#elif defined(_WINDOWS)
	ftd_make_group(lgp->lgnum, 0, &dev);
	info.lgdev = dev;
#endif

	info.statsize = FTD_PS_GROUP_ATTR_SIZE; 

	if (ftd_ioctl_new_lg(lgp->ctlfd, &info, 1)) {
		if (ftd_errno() == EADDRINUSE) {
			*grpstarted = 1;
		} else {
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "NEW_LG", 
				strerror(ftd_errno()));
			goto errret;
		}
	}

#if defined(SOLARIS) || defined(_AIX)
	ftd_make_group(lgp->lgnum, info.lgdev);
#endif

//#if defined(NEW_PSTORE)
sprintf(lgp->cfgp->pstore, "%s", raw_name);  
//#endif


	// get the attributes of the group 
	ftd_lg_get_pstore_state(lgp, 0);

	// add the attributes to the driver 
	if (ftd_lg_set_driver_state(lgp) < 0) {
		goto errret;
	}

	if (buffer) {
		free(buffer);
	}

	return 0;

errret:
	
	if (buffer) {
		free(buffer);
	}

	ftd_lg_rem_group(lgp, *state, autostart, 1);

	return -1;
}

/*
 * ftd_lg_rem_devs --
 * remove group's devices from the driver and pstore
 */
static int
ftd_lg_rem_devs(ftd_lg_t *lgp,
	ftd_stat_t *lgstat, int autostart, int silent, int all)
{
	int				*savedbbufp, i, found;
	char			buf[MAXPATH];
	u_char			*lrdb = NULL, *hrdb = NULL;
	ftd_dev_cfg_t	**devcfgpp, *devcfgp;
	ftd_dev_info_t	*dip = NULL;
	disk_stats_t	devstat;
#if defined(_WINDOWS)
	part_t			part, *partp;
	LList			*partlist;
	BOOL			bDismounted = FALSE;
#else
	struct stat		statbuf;
	char			pmdname[8];
	pid_t			pid;
	int				pcnt, index;
#endif

	if ((dip = (ftd_dev_info_t*)calloc(lgstat->ndevs, sizeof(ftd_dev_info_t))) == NULL) {
		reporterr(ERRFAC, M_MALLOC, (lgstat->ndevs * sizeof(ftd_dev_info_t)));
		goto errret;
	}
	
	if ((lrdb = (u_char*)calloc(lgstat->ndevs, FTD_PS_LRDB_SIZE)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, (lgstat->ndevs*FTD_PS_LRDB_SIZE));
		goto errret;
	}
	
	if ((hrdb = (u_char*)calloc(lgstat->ndevs, FTD_PS_HRDB_SIZE)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, (lgstat->ndevs*FTD_PS_HRDB_SIZE));
		goto errret;
	}
	
	savedbbufp = lgp->db->dbbuf;

	// get the LRDB's for the group 
	lgp->db->dbbuf = (int*)lrdb;
	if (ftd_ioctl_get_lrdbs(lgp->ctlfd, lgp->devfd, lgp->lgnum,
		lgp->db, dip) != 0)
	{
		goto errret;
	}
			
	// get the HRDB's for the group 
	lgp->db->dbbuf = (int*)hrdb;
	if (ftd_ioctl_get_hrdbs(lgp->ctlfd, lgp->devfd, lgp->lgnum,
		lgp->db, dip) != 0)
	{
		goto errret;
	}

	// get devices currently in driver for the group
	if (ftd_ioctl_get_devices_info(lgp->ctlfd, lgp->lgnum, dip) != 0) {
		goto errret;
	}

	if ((partlist = CreateLList(sizeof(part_t))) == NULL) {
		goto errret;
	}

	// loop thru group devices 
	for (i = 0; i < lgstat->ndevs; i++) {
		
		found = FALSE;

		// get devname
		if (ftd_ioctl_get_dev_stats(lgp->ctlfd,
			lgp->lgnum, dip[i].bdev, &devstat, 1) < 0)
		{
			goto errret; 
		}

		ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
			devcfgp = *devcfgpp;
			if (!strcmp(devcfgp->devname, devstat.devname + strlen(devstat.devname) - 2)) {
				// found it
				found = TRUE;
				break;
			}
		}
		
		if (!all && found) {
			// skip it
			continue;
		}

#if defined(_WINDOWS)
		if (!QueryDosDevice(devcfgp->devname, part.symname, MAXPATHLEN) ) {
			sprintf(buf, "can't get symbolic name for %s", devcfgp->devname);
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, buf, ftd_strerror());
			goto errret;
		}
				
		do {
			bDismounted = DeleteAVolume(devcfgp->devname);
		} while ( !bDismounted && (ftd_lockmsgbox(devcfgp->devname) == 0) );

		if (!bDismounted) {
			reporterr(ERRFAC, M_DISMOUNT, ERRCRIT,
					devcfgp->devname, lgp->devname);
			goto errret;
		}
#else
		// UNIX needs to do something to test lock here ?
#endif
		// save the names
		strcpy(part.devname, devcfgp->devname);

		// get the device number 
#if !defined(_WINDOWS)
		strcpy(buf, part.devname);
		if (stat(buf, &statbuf) != 0) {
			continue;
		}
		part.dev_num = statbuf.st_rdev;
#else
		part.dev_num = dip[i].bdev;
#endif // !defined(_WINDOWS) 

		AddToTailLL(partlist, &part);
	}

	// loop thru the list of devices and remove from group 
	ForEachLLElement(partlist, partp) {
		
#if !defined(_WINDOWS)
		strcpy(buf, partp->devname);
#else
		FTD_CREATE_GROUP_DIR(buf, lgp->lgnum);
		strcat(buf, "/");
		strcat(buf, partp->devname);
#endif // !defined(_WINDOWS) 
		if (ftd_dev_rem(lgp->ctlfd,
			lgp->devfd,
			lgp->lgnum,
			lrdb,
			hrdb,
			lgp->cfgp->pstore,
			lgp->devname,
			buf, // pstore name not devname
			partp->dev_num,
			lgstat->state,
			lgstat->ndevs,
			dip,
			silent) < 0)
		{
			goto errret;
		}
	}

#if defined(_WINDOWS)
	// recreate the partitions
	ForEachLLElement(partlist, partp) {
		CreateAVolume(partp->devname, partp->symname);
	}
	FreeLList(partlist);
#endif

	if (dip) {
		free(dip);
	}

	if (lrdb) {
		free(lrdb);
	}

	if (hrdb) {
		free(hrdb);
	}

	lgp->db->dbbuf = savedbbufp;

	return 0;

errret:

#if defined(_WINDOWS)
	// recreate the partitions
	ForEachLLElement(partlist, partp) {
		CreateAVolume(partp->devname, partp->symname);
	}
	FreeLList(partlist);
#endif

	if (dip) {
		free(dip);
	}

	if (lrdb) {
		free(lrdb);
	}

	if (hrdb) {
		free(hrdb);
	}

	lgp->db->dbbuf = savedbbufp;

	return -1;
}

/*
 * ftd_lg_update_bitmaps --
 * update the device bitmaps for the group
 */
static int
ftd_lg_update_bitmaps(ftd_lg_t *lgp, ftd_stat_t *lgstat, int silent)
{

	if (lgstat->state == FTD_MODE_REFRESH
		|| lgstat->state == FTD_MODE_TRACKING)
	{
		if (ftd_ioctl_update_lrdbs(lgp->ctlfd, lgp->lgnum, silent) < 0) {
			return -1;
		}
		if (ftd_ioctl_update_hrdbs(lgp->ctlfd, lgp->lgnum, silent) < 0) {
			return -1;
		}

	} else if (lgstat->state == FTD_MODE_NORMAL) {
		
		if (ftd_ioctl_clear_lrdbs(lgp->ctlfd, lgp->lgnum, silent) < 0) {
			return -1;
		}
		if (ftd_ioctl_clear_hrdbs(lgp->ctlfd, lgp->lgnum, silent) < 0) {
			return -1;
		}

		if (lgstat->bab_used > 0) {

			if (ftd_ioctl_update_lrdbs(lgp->ctlfd,
				lgp->lgnum, silent) < 0)
			{
				return -1;
			}
			if (ftd_ioctl_update_hrdbs(lgp->ctlfd,
				lgp->lgnum, silent) < 0)
			{
				return -1;
			}
		}
	}

	return 0;
}

/*
 * ftd_lg_rem_group --
 * remove group state from driver and pstore
 */
static int
ftd_lg_rem_group(ftd_lg_t *lgp, int state, int autostart, int silent)
{
	int					rc;
	ps_version_1_attr_t	attr;
	ftd_lg_info_t		lginfo;
	ps_group_info_t		ps_lginfo;
	char				buf[MAXPATHLEN];
#if defined(_WINDOWS)
	BOOL				bDismounted = FALSE;
#else
	struct stat			statbuf;
	char				pmdname[8];
	pid_t				pid;
	int					pcnt, index;
#endif

	// check the pstore to see what state the group is in 
	if (ps_get_version_1_attr(lgp->cfgp->pstore, &attr) != PS_OK) {
		if (!silent) {
			reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, lgp->cfgp->pstore);
		}
		goto errret;
	}
	ps_lginfo.name = NULL;
	
	if ((rc = ps_get_group_info(lgp->cfgp->pstore,
		lgp->devname, &ps_lginfo)) != PS_OK)
	{
		if (rc != PS_GROUP_NOT_FOUND) {
			if (!silent) {
				reporterr(ERRFAC, M_PSERROR, ERRCRIT, lgp->devname);
			}
		}
		goto errret;
	}
	
	// clear autostart flag in pstore 
	if (autostart) {
		rc = ps_set_group_autostart(lgp->cfgp->pstore, lgp->devname, 0);
		if (rc != PS_OK) {
			if (!silent) {
				reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT,
					lgp->devname, lgp->cfgp->pstore);
			}
			goto errret;
		}
	}
	
	if (ftd_ioctl_get_groups_info(lgp->ctlfd,
		lgp->lgnum, &lginfo, silent) < 0)
	{
		goto errret;
	}
	
	// stuff the state into pstore 
	ps_set_group_state(lgp->cfgp->pstore, lgp->devname, state);

	// delete the group
	if (ftd_ioctl_del_lg(lgp->ctlfd, lginfo.lgdev, silent) < 0) {
		ftd_close(lgp->ctlfd);
		goto errret;
	}

	// write out the shutdown flag for the group 
	ps_set_group_shutdown(lgp->cfgp->pstore, lgp->devname, 1);

#if !defined(_WINDOWS)
	// waste the devices directory
	ftd_delete_group(lgp->lgnum);
	// move the .prf file out of the way so monitortool shows it gone
	ftd_lg_backup_perf_file(lgp);
#endif

	// remove .cur file
	if (autostart) {
		sprintf(buf, "%s/p%03d.cur", PATH_CONFIG, lgp->lgnum);
		unlink(buf);
	}

	return 0;

errret:

	return -1;
}


/*
 * ftd_lg_rem --
 * remove group state from driver and pstore
 */
int
ftd_lg_rem(ftd_lg_t *lgp, int autostart, int silent)
{
	ftd_stat_t	lgstat;
	int			tstate, lgopenflag = FALSE, ctlopenflag = FALSE;

	if (lgp->devfd == INVALID_HANDLE_VALUE) {
		if (ftd_lg_open(lgp) < 0) {
			return -1;
		}
		lgopenflag = TRUE;
	}

	if (lgp->ctlfd == INVALID_HANDLE_VALUE) {
		if ((lgp->ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) < 0) {
			reporterr(ERRFAC, M_CTLOPEN, ERRCRIT,
				FTD_CTLDEV, ftd_strerror());
			goto errret;
		}
		ctlopenflag = TRUE;
	}

	// init stop for the group
	if (ftd_ioctl_init_stop(lgp->devfd, lgp->lgnum, silent) < 0) {
		goto errret;
	}

	// get the stats for the group
	if (ftd_ioctl_get_group_stats(lgp->ctlfd, lgp->lgnum,
		&lgstat, silent) < 0)
	{
		goto errret;
	}

	if (ftd_lg_update_bitmaps(lgp, &lgstat, silent) < 0) {
		goto errret;
	}
	
	if (ftd_lg_rem_devs(lgp, &lgstat, autostart, 0, 1) < 0) {
		goto errret;
	}

	tstate = lgstat.state;
	
	if (tstate == FTD_MODE_REFRESH
		|| (tstate == FTD_MODE_NORMAL && lgstat.bab_used > 0))
	{
			tstate = FTD_MODE_TRACKING;
	}

	// must close the lg device before we remove it in 
	// ftd_lg_rem_group

	ftd_lg_close(lgp);
	lgopenflag = FALSE;

	if (ftd_lg_rem_group(lgp, tstate, autostart, silent) < 0)
	{
		goto errret;
	}

	if (ctlopenflag) {
		ftd_close(lgp->ctlfd);
	}

	return 0;

errret:

	if (lgopenflag) {
		ftd_lg_close(lgp);
	}

	if (ctlopenflag) {
		ftd_close(lgp->ctlfd);
	}

	return -1;
}

/*
 * ftd_lg_add --
 * add the logical group state to the pstore and driver
 */
int
ftd_lg_add(ftd_lg_t *lgp, int autostart)
{
	ftd_dev_info_t	*devp = NULL;
	ftd_proc_t		*procp;
	ftd_stat_t		lgstat;
	int				state, regen, rc, max_dev, grpstarted;
	char			*psname, *groupname, tunesetpath[MAXPATH];
	struct stat		statbuf;
	ps_version_1_attr_t attr;

	psname = lgp->cfgp->pstore;
	groupname = lgp->devname;

	// has this pstore been created? 
	if (ps_get_version_1_attr(psname, &attr) != PS_OK) {
		// no, so create it 
		rc = ps_create(psname, &max_dev);
		if (rc == 0) {
			reporterr(ERRFAC, M_PSCREAT_SUCCESS, ERRINFO,
				psname, lgp->lgnum, max_dev);     
		} else {
			reporterr(ERRFAC, M_PSCREATE, ERRCRIT, psname, lgp->lgnum);
			return -1;
		}
	}

	// add group
	if (ftd_lg_add_group(lgp, 0,
		&state, &regen, &grpstarted, autostart) < 0)
	{
		goto errret;
	}

	// open the logical group device
	if (ftd_lg_open(lgp) < 0) {
		goto errret;
	}

	// get the stats for the group
	if (ftd_ioctl_get_group_stats(lgp->ctlfd, lgp->lgnum,
		&lgstat, 0) < 0)
	{
		goto errret;
	}

	if (grpstarted) {
		// remove devices that are no longer in the config file
		if (ftd_lg_rem_devs(lgp, &lgstat, autostart, 0, 0) < 0) {
			// if some of the deleted devs get removed then that's ok
			goto errret;
		}
	}

	// add devices
	if (ftd_lg_add_devs(lgp, &lgstat, regen, autostart, grpstarted) < 0) {
		// if something went wrong in add_devs then devs that had been
		// successfully added are removed - so we just need to remove
		// the group
		if (!grpstarted) {
			// no devs in group
			ftd_lg_rem_group(lgp, state, autostart, 1);
		}
		goto errret;
	}

	if (ftd_ioctl_set_group_state(lgp->ctlfd, lgp->lgnum, state) < 0) {
		ftd_lg_rem(lgp, autostart, 1);
		goto errret;
	}
	
	// set the shutdown state of the group to FALSE 
	ps_set_group_shutdown(lgp->cfgp->pstore, lgp->devname, 0);

	/*
	 * configtool may not have been able to set the tunables in 
	 * the pstore because start had not been run, so it stores
	 * them in a temp file that we'll read in and delete now.
	 */
	sprintf(tunesetpath, "%s/settunables%d.tmp",PATH_CONFIG, lgp->lgnum);

	if (stat(tunesetpath, &statbuf) != -1) {
#if defined(_WINDOWS)
		procp = ftd_proc_create(PROCTYPE_THREAD);
#else		
		procp = ftd_proc_create(PROCTYPE_PROC);
#endif	
		if (procp == NULL) {
			ftd_lg_rem(lgp, autostart, 1);
			goto errret;
		}
		if (ftd_proc_exec(tunesetpath, 1) < 0) {
			ftd_lg_rem(lgp, autostart, 1);
			goto errret;
		}
		
		unlink(tunesetpath);
		ftd_proc_delete(procp);
	}

	// get tunables from driver 
	if (ftd_lg_get_driver_state(lgp, 0) != 0) {
		ftd_lg_rem(lgp, autostart, 1);
		goto errret;
	}
	
	// init the tunables in the driver 
	if (ftd_lg_set_driver_state(lgp) != 0) {
		ftd_lg_rem(lgp, autostart, 1);
		goto errret;
	}

	// init group stats in driver 
	if (ftd_stat_init_driver(lgp) < 0) {
		ftd_lg_rem(lgp, autostart, 1);
		goto errret;
	}

	// set autostart flag in pstore  
	if (!autostart) {
		rc = ps_set_group_autostart(lgp->cfgp->pstore, lgp->devname, 1);
		if (rc != 0) {
			ftd_lg_rem(lgp, autostart, 1);
			goto errret;
		}
	}

	if (devp) {
		free(devp);
	}

	ftd_lg_close(lgp);

	return 0;

errret:

	if (devp) {
		free(devp);
	}

	ftd_lg_close(lgp);

	return -1;
}

/*
 * ftd_lg_sigusr1 --
 * handle a SIGUSR1 signal
 */
int
ftd_lg_sigusr1(ftd_lg_t *lgp) 
{
	int	state = GET_LG_STATE(lgp->flags);

	if (lgp->cfgp->role == ROLEPRIMARY) {

		switch(state) {
		case FTD_SNORMAL:
			break;
		case FTD_SREFRESH:
		case FTD_SREFRESHC:
		case FTD_SREFRESHF:
			reporterr(ERRFAC, M_REFRTERM, ERRINFO, LG_PROCNAME(lgp));
			
			// reset group state
			
			// pstore -> ACCUMULATE
			ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, -1);
			
			// driver -> TRACKING
			ftd_lg_set_driver_run_state(lgp->lgnum, FTD_MODE_TRACKING);

			break;
		case FTD_SBACKFRESH:
			reporterr(ERRFAC, M_BACKTERM, ERRINFO, LG_PROCNAME(lgp));

			// reset group state

			// pstore -> ACCUMULATE
			ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, -1);
			
			// driver -> PASSTHRU
			ftd_lg_set_driver_run_state(lgp->lgnum, FTD_MODE_PASSTHRU);

			break;
		default:
			break;
		}

		// reset group stats
		ftd_stat_init_driver(lgp);
	}

	// force an exit from state machine
	return FTD_CINVALID;
}

/*
 * ftd_lg_sigterm --
 * handle a SIGTERM signal
 */
int
ftd_lg_sigterm(ftd_lg_t *lgp) 
{
	
	if (lgp->cfgp->role == ROLEPRIMARY) {
		// reset driver stats
		ftd_stat_init_driver(lgp);
	}

	// force an exit
	return FTD_CINVALID;
}

/*
 * ftd_lg_sigpipe --
 * handle a SIGPIPE signal
 */
int
ftd_lg_sigpipe(ftd_lg_t *lgp) 
{

	// force an exit
	return FTD_LG_NET_BROKEN;
}

/*
 * ftd_lg_kill --
 * handle a CKILL msg to rmd 
 */
int
ftd_lg_kill(ftd_lg_t *lgp) 
{
	ftd_header_t	header;
	int				rc;

	memset(&header, 0, sizeof(header));

	header.msgtype = FTDACKKILL;
	header.msg.lg.lgnum = lgp->lgnum;

	// tell pmd to die 
	if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_kill",lgp->dsockp, &header)) < 0) {
		return rc;
	}

	return 0;
}

/*
 * ftd_lg_set_driver_state -- set group state in driver
 */
int
ftd_lg_set_driver_state(ftd_lg_t *lgp)
{
	char	buf[FTD_PS_GROUP_ATTR_SIZE];
	int		rc;

	// set sync mode depth in driver 
	if (lgp->tunables->syncmode != 0) {
		if (ftd_ioctl_set_sync_depth(lgp->ctlfd, lgp->lgnum,
			lgp->tunables->syncmodedepth)!=0) {
			return -1;
		}
	} else {
		if (ftd_ioctl_set_sync_depth(lgp->ctlfd, lgp->lgnum, -1) != 0) {          
			return -1;
		}
	}
	// set iodelay in driver 
	if (ftd_ioctl_set_iodelay(lgp->ctlfd, lgp->lgnum, lgp->tunables->iodelay) != 0) {
		return -1;
	}
	// set syncmodetimeout in driver 
	if (ftd_ioctl_set_sync_timeout(lgp->ctlfd, lgp->lgnum,
		lgp->tunables->syncmodetimeout)!=0) {
		return -1;
	}

	memcpy(buf, lgp->tunables, sizeof(ftd_tune_t));
	if ((rc = ftd_ioctl_set_lg_state_buffer(lgp->ctlfd,
		lgp->lgnum, buf)) < 0)
	{
		return -1;
	}

	return 0;
}

/*
 * ftd_lg_get_driver_state -- get group state from driver
 */
int
ftd_lg_get_driver_state(ftd_lg_t *lgp, int silent)
{
	char	buf[FTD_PS_GROUP_ATTR_SIZE];

	if (ftd_ioctl_get_lg_state_buffer(lgp->ctlfd,
		lgp->lgnum, buf, silent) < 0)
	{
		return -1;
	}
	memcpy(lgp->tunables, buf, sizeof(ftd_tune_t));

	if (lgp->compp && lgp->compp->magicvalue == COMPMAGIC) {
		// set socket object compression algorithm 
		lgp->compp->algorithm = lgp->tunables->compression;
	}

	return 0;
}

/*
 * ftd_lg_set_pstore_state -- set group state in pstore
 */
int
ftd_lg_set_pstore_state(ftd_lg_t *lgp, char *buf, int buflen)
{
	int	rc;

	/* dump new buffer to the ps */
	rc = ps_set_group_attr(lgp->cfgp->pstore, lgp->devname, buf, buflen);
	if (rc != PS_OK) {
		return -1;
	}

	return 0;
}

/*
 * ftd_lg_get_pstore_state -- get group state from pstore
 */
int
ftd_lg_get_pstore_state(ftd_lg_t *lgp, int silent)
{
	ps_version_1_attr_t attr;
	int					rc;
	char				*buf;

	/* get header info */
	if ((rc = ps_get_version_1_attr(lgp->cfgp->pstore, &attr))!=PS_OK) {
		if (!silent) {
			reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT,
				lgp->devname, lgp->cfgp->pstore);
		}
		return -1;
	}
	if ((buf = (char*)malloc(attr.group_attr_size * sizeof(char))) == NULL) {
		return -1;
	}
	/* get the tunables */
	if (ps_get_group_attr(lgp->cfgp->pstore, lgp->devname, buf,
		attr.group_attr_size) != PS_OK) {
		if (!silent) {
			reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT,
				lgp->devname, lgp->cfgp->pstore);
		}
		free(buf);
		return -1;
	}
	/* parse buffer into tunables */
	if (ftd_lg_parse_state(lgp, buf, attr.group_attr_size) < 0) {
		free(buf);
		return -1;
	}

	if (lgp->compp && lgp->compp->magicvalue == COMPMAGIC) {
		// set socket object compression algorithm
		lgp->compp->algorithm = lgp->tunables->compression;
	}

	free(buf);

	return 0;
}

/*
 * ftd_lg_parse_state --
 * parse state in buffer and assign to appropriate tunables
 */
static int
ftd_lg_parse_state(ftd_lg_t *lgp, char *lbuf, int buflen)
{
	char	key[255], *keyp, value[255], *valuep, buf[4096], *bufp;
	int		rc = 0;

	memcpy(buf, lbuf, buflen);

	keyp = key;
	valuep = value;
	bufp = buf;

	while (getbufline(&bufp, &keyp, &valuep, '\n')) {
		if (ftd_lg_parse_state_value(keyp, valuep, lgp->tunables, 0, 1)!=0) {
			rc = -1;
		}
	}

	return rc;
}

/*
 * ftd_lg_parse_state_value --
 * assign and/or verify state value in tunables structure
 */
static int
ftd_lg_parse_state_value(char *key, char *value,
	ftd_tune_t *tunables, int verify, int set)
{  
	int	intval;

	if (strcmp("CHUNKSIZE:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval < MIN_CHUNKSIZE || intval > MAX_CHUNKSIZE) {
				reporterr(ERRFAC, M_CHUNKSZ, ERRWARN, intval,
						MAX_CHUNKSIZE, MIN_CHUNKSIZE);
				return -1;
			}
		}
		if (set) {
			tunables->chunksize = intval * 1024;
		}
#if !defined(_WINDOWS)
	} else if (strcmp("STATINTERVAL:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval < MIN_STATINTERVAL || intval > MAX_STATINTERVAL) {
				reporterr(ERRFAC, M_STATINT, ERRWARN, intval,
						MAX_STATINTERVAL, MIN_STATINTERVAL);
				return -1;
			}
		}
		if (set) {
			tunables->statinterval = intval;
		}
	} else if (strcmp("MAXSTATFILESIZE:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval > MAX_MAXSTATFILESIZE || intval < MIN_MAXSTATFILESIZE ) {
				reporterr(ERRFAC, M_STATSIZE, ERRWARN, intval,
						MAX_MAXSTATFILESIZE, MIN_MAXSTATFILESIZE);
				return -1;
			}
		}
		if (set) {
			tunables->maxstatfilesize = intval * 1024;
		}
	} else if (strcmp("LOGSTATS:", key) == 0) {
		if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
			if (set) {
				tunables->stattofileflag = 1;
			}
		} else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
			if (set) {
				tunables->stattofileflag = 0;
			}
		} else {
			if (verify) {
				reporterr(ERRFAC, M_LOGSTATS, ERRWARN);
				return -1;
			} 
		}
#endif
	} else if (strcmp("SYNCMODEDEPTH:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval == 0 ) {
				/* DTurrin - Sept. 24th, 2001
				   99999999 is now the max value that can be entered
				   in the dialog box.                                     */
				reporterr(ERRFAC, M_SYNCDEPTH, ERRWARN, intval, 99999999, 1);
				return -1;
			}
		}
		if (set) {
			tunables->syncmodedepth = intval;
		}
	} else if (0 == strcmp("SYNCMODETIMEOUT:", key)) {
		intval = atoi(value);
		if (verify) {
			if (intval < 1 ) {
				/* DTurrin - Sept. 24th, 2001
				   99999999 is now the max value that can be entered
				   in the dialog box.                                     */
				reporterr(ERRFAC, M_SYNCTIMEOUT, ERRWARN, intval, 99999999, 1);
				return -1;
			}
		}
		if (set) {
			tunables->syncmodetimeout = intval;
		}
	} else if (strcmp("SYNCMODE:", key) == 0) {
		if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
			if (set) {
				tunables->syncmode = 1;
			}
		} else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
			if (set) {
				tunables->syncmode = 0;
			}
		} else {
			if (verify) {
				reporterr(ERRFAC, M_SYNCVAL, ERRWARN);
				return -1;
			}
		}
#if !defined(_WINDOWS)
	} else if (strcmp("TRACETHROTTLE:", key) == 0) {
		if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
			if (set) {
				tunables->tracethrottle = 1;
			}
		} else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
			if (set) {
				tunables->tracethrottle = 0;
			}
		} else {
			if (verify) {
				reporterr(ERRFAC, M_TRACEVAL, ERRWARN);
				return -1;
			}
		}
#endif
	} else if (strcmp("COMPRESSION:", key) == 0) {
		if (strcmp (value, "on") == 0
			|| strcmp (value, "ON")== 0
			|| strcmp (value, "1") == 0)
		{
			if (set) {
				tunables->compression = 1;
			}
		} else if (strcmp(value, "off") == 0
			|| strcmp(value, "OFF") == 0
			|| strcmp (value, "0") == 0)
		{
			if (set) {
				tunables->compression = 0;
			}
		} else {
			if (verify) {
				reporterr(ERRFAC, M_COMPRESSVAL, ERRWARN);
				return -1;
			}
		}
	} else if (strcmp("_MODE:", key) == 0) {
#if !defined(_WINDOWS)
	} else if (strcmp("NETMAXKBPS:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval <= MIN_NETMAXKBPS && intval != MAX_NETMAXKBPS) {
				reporterr(ERRFAC, M_NETMAXERR, ERRWARN, intval,
						MAX_NETMAXKBPS, MIN_NETMAXKBPS);
				return -1;
			}
		}
		if (set) {
			tunables->netmaxkbps = intval;
		}
	} else if (strcmp("IODELAY:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval < 0) {
				reporterr(ERRFAC, M_IODELAY, ERRWARN, intval, 500000, 0);
				return -1;
			}
			if (intval > 500000) {
				reporterr(ERRFAC, M_IODELAY, ERRWARN, intval, 500000, 0);
				return -1;
			}
		}
		if (set) {
			tunables->iodelay = intval;
		}
#endif
	} else if (strcmp("CHUNKDELAY:", key) == 0) {
		intval = atoi (value);
		if (verify) {
			if (intval < 0) {
				reporterr(ERRFAC, M_CHUNKDELAY, ERRWARN, intval, INT_MAX, 0);
				return -1;
			}
		}
		if (set) {
			tunables->chunkdelay = intval;
		}
	} else if (strcmp("REFRESHTIMEOUT:", key) == 0) {
		intval = atoi(value);
		if (verify) {
			if (intval < -1 || intval > INT_MAX) {
				reporterr(ERRFAC, M_REFRTIMEOUT, ERRWARN, intval, INT_MAX, -1);
				return -1;
			}
		}
		if (set) {
			tunables->refrintrvl = intval;
		}
	}  else { 
		return -1;
	}

	return 0;
}

/*
 * ftd_lg_set_pstore_state_value --
 * sets a state value in the pstore given a <KEY>: and value.
 */
int
ftd_lg_set_pstore_state_value(ftd_lg_t *lgp, char *inkey, char *invalue) 
{
	int rc;

	rc = ps_set_group_key_value(lgp->cfgp->pstore, lgp->devname, inkey, invalue);
	if (rc != PS_OK) {
	reporterr(ERRFAC, M_PSWTDATTR, ERRWARN,
			lgp->devname, lgp->cfgp->pstore);
	}

	return 0;
}

/*
 * ftd_lg_get_pstore_state_value --
 * gets a state value from the pstore given a <KEY>: and value.
 */
int
ftd_lg_get_pstore_state_value(ftd_lg_t *lgp, char *inkey, char *invalue) 
{
	int rc;

	rc = ps_get_group_key_value(lgp->cfgp->pstore, lgp->devname, inkey, invalue);
	if (rc != PS_OK) {
		reporterr(ERRFAC, M_PSRDGATTR, ERRWARN,
			lgp->devname, lgp->cfgp->pstore);
	}

	return 0;
}

/*
 * ftd_lg_dump_pstore_attr --
 * write tunable paramters to stdout
 */
int
ftd_lg_dump_pstore_attr(ftd_lg_t *lgp, FILE* fd) 
{
	char	*ps_key[FTD_MAX_KEY_VALUE_PAIRS];
	char	*ps_value[FTD_MAX_KEY_VALUE_PAIRS];
	char	*inbuf, *temp;
	int		rc, num_ps;

	/* allocate an input buffer and output buffer for the parameters */
	if ((inbuf = (char*)malloc(FTD_PS_GROUP_ATTR_SIZE)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, FTD_PS_GROUP_ATTR_SIZE);
		return -1;
	}
	rc = ps_get_group_attr(lgp->cfgp->pstore, lgp->devname,
		inbuf, FTD_PS_GROUP_ATTR_SIZE);
	if (rc != PS_OK) {
		reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT,
			lgp->devname, lgp->cfgp->pstore);
		free(inbuf);
	return -1;
	}

	/* dump the attributes */
	temp = inbuf;
	num_ps = 0;
	while (getbufline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
		if (ps_key[num_ps][0] != '_') {
			fprintf(fd, "%s %s\n", ps_key[num_ps], ps_value[num_ps]); 
		}
		num_ps++;
	}

	free(inbuf);

	return 0;
}

/*
 * ftd_lg_set_state_value --
 * sets a tunable paramter in the driver and pstore given a <KEY>: and value.
 */
int
ftd_lg_set_state_value(ftd_lg_t *lgp, char *inkey, char *invalue) 
{
	char	*inbuf, *temp, *outbuf;
	char	line[MAXPATHLEN];
	char	*ps_key[FTD_MAX_KEY_VALUE_PAIRS];
	char	*ps_value[FTD_MAX_KEY_VALUE_PAIRS];
	int		num_ps, i, j, found, linelen, len, rc;

	if (inkey[strlen(inkey) - 1] != ':') {
		reporterr(ERRFAC, M_BADKEY, ERRCRIT, inkey, "set_state_value");
		return -1;
	}

	/* get current driver state */
	if (ftd_lg_get_driver_state(lgp, 1) < 0) {
		return -1;
	}
	
	/* verify new value */
	rc = ftd_lg_parse_state_value(inkey, invalue, lgp->tunables, 1, 0);
	if (rc < 0) {
		return rc;
	}
	
	/* allocate an input buffer and output buffer for the parameters */
	if (((inbuf = (char*)malloc(FTD_PS_GROUP_ATTR_SIZE)) == NULL) ||
		((outbuf = (char*)calloc(1, FTD_PS_GROUP_ATTR_SIZE)) == NULL)) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, FTD_PS_GROUP_ATTR_SIZE);
		return -1;
	}
	rc = ps_get_group_attr(lgp->cfgp->pstore, lgp->devname,
		inbuf, FTD_PS_GROUP_ATTR_SIZE);
	if (rc != PS_OK) {
		reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT,
			lgp->devname, lgp->cfgp->pstore);
		return -1;
	}
	
	/* parse the attributes into key/value pairs */
	temp = inbuf;
	num_ps = 0;
	while (getbufline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
		num_ps++;
	}
	
	/* add/replace the key/value pairs */
	found = FALSE;
	for (j = 0; j < num_ps; j++) {
		if (strcmp(inkey, ps_key[j]) == 0) {
			/* replace value */
			ps_value[j] = invalue;
			found = TRUE;
			break;
		}
	}
	
	/* add key/value */
	if (!found) {
		ps_key[num_ps] = inkey;
		ps_value[num_ps] = invalue;
		num_ps++;
	}
	
	/* create a new buffer */
	len = 0;
	for (i = 0; i < num_ps; i++) {
		sprintf(line, "%s %s\n", ps_key[i], ps_value[i]);
		linelen = strlen(line);
		strncpy(&outbuf[len], line, linelen);
		len += linelen;
	}
	outbuf[len] = '\0';

	/* set the new lg state value in the tunables structure */
	if (ftd_lg_parse_state(lgp, outbuf, FTD_PS_GROUP_ATTR_SIZE) != 0) {
		return -1;
	}
	
	/* set the new lg state in the pstore */
	if (ftd_lg_set_pstore_state(lgp, outbuf, FTD_PS_GROUP_ATTR_SIZE) != 0) {
		return -1;
	}
	
	/* set the lg state in the driver */
	if (ftd_lg_set_driver_state(lgp) != 0) {
		return -1;
	}

	free(inbuf);
	free(outbuf);

	return 0; 
}

/*
 * ftd_lg_open_devs --
 * open all devices in the group
 */
int
ftd_lg_open_devs(ftd_lg_t *lgp, int mode, int permis, int tries)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;
	int				i, ret = 0;

	if (tries < 1) {
		tries = 1;
	} else if (tries > 5) {
		tries = 5;
	}

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			// dev not found
			ftd_lg_close_devs(lgp);
			return -1;
		}
		
		for (i=0;i<tries;i++) {
			
			if (ftd_dev_open((*devcfgpp), lgp->lgnum, devp, mode, permis, lgp->cfgp->role) < 0) {
				
				{
					char	*devname;
					int		errnum = ftd_errno();
					
					if (lgp->cfgp->role == ROLEPRIMARY) {
						devname = (*devcfgpp)->pdevname;
					} else {
						devname = (*devcfgpp)->sdevname;
					}
					
					if (FTD_SOCK_CONNECT(lgp->dsockp)) {
						if ((ret = ftd_sock_send_err(lgp->dsockp,
							ftd_get_last_error(ERRFAC))) < 0)
						{
							return ret;
						}
					}
#if defined(_WINDOWS)
					if (lgp->cfgp->role == ROLEPRIMARY) {
						if (ftd_lockmsgbox(devname) == -1) {
							ret = -1;
							break;
						}
					} else {
						if (i == (tries-1)) {
							// last try failed
							ret = -1;
							reporterr(ERRFAC, M_OPEN, ERRWARN, devname, strerror(errnum));
							continue;
						} else {
							sleep(1);
						}
					}
#else
					if (i == (tries-1)) {
						// last try failed
						ret = -1;
						reporterr(ERRFAC, M_OPEN, ERRWARN, devname, strerror(errnum));
						continue;
					} else {
						sleep(1);
					}
#endif
				}
			} else {
				break;
			}
		}

		ftd_dev_meta((*devcfgpp), devp);
	}

	
	return ret;
}

/*
 * ftd_lg_open_ftd_devs --
 * open all ftd devices in the group
 */
int
ftd_lg_open_ftd_devs(ftd_lg_t *lgp, int mode, int permis, int tries)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;
	int				i, ret = 0;

	if (tries < 1) {
		tries = 1;
	} else if (tries > 5) {
		tries = 5;
	}

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			// dev not found
			ftd_lg_close_ftd_devs(lgp);
			return -1;
		}

		for (i=0;i<tries;i++) {
			if (ftd_dev_ftd_open((*devcfgpp), devp, mode, permis, lgp->cfgp->role) < 0) {
				
				{
					int		errnum = ftd_errno();
					char	*devname = (*devcfgpp)->devname;
					
					reporterr(ERRFAC, M_OPEN, ERRWARN, devname, strerror(errnum));
					
					if (FTD_SOCK_CONNECT(lgp->dsockp)) {
						if ((ret = ftd_sock_send_err(lgp->dsockp,
							ftd_get_last_error(ERRFAC))) < 0)
						{
							return ret;
						}
					}

#if defined(_WINDOWS)
					if (lgp->cfgp->role == ROLEPRIMARY) {
						if (ftd_lockmsgbox(devname) == -1) {
							ret = -1;
							break;
						}
					} else {
						ret = -1;
						break;
					}
#else
					sleep(1);
#endif
				}
			} else {
				break;
			}
		}
	}

	return ret;
}

#if defined(_WINDOWS)

/*
 * ftd_lg_lock_devs --
 * lock all devices in the group
 */
int
ftd_lg_lock_devs(ftd_lg_t *lgp, int role)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;
	char			*devname;

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		if (role == ROLEPRIMARY) {
			devname = (*devcfgpp)->pdevname;
		} else {
			devname = (*devcfgpp)->sdevname;
		}
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			reporterr(ERRFAC, M_DEVLOCK, ERRCRIT, devname, ftd_strerror());
			// dev not found
			ftd_lg_unlock_devs(lgp);
			return -1;
		}
		if ((devp->devfd = ftd_dev_lock(devname, lgp->lgnum)) == INVALID_HANDLE_VALUE) {
			reporterr(ERRFAC, M_DEVLOCK, ERRCRIT, devname, ftd_strerror());
			return -1;
		}
	}

	return 0;
}

/*
 * ftd_lg_unlock_devs --
 * unlock all devices in the group
 */
int
ftd_lg_unlock_devs(ftd_lg_t *lgp)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			// dev not found
			return -1;
		}
		if (ftd_dev_unlock(devp->devfd) == 0) {
			ftd_close(devp->devfd);

			devp->devfd = INVALID_HANDLE_VALUE;
		}
	}

	return 0;
}

/*
 * DTurrin - Oct 26th, 2001
 *
 * ftd_lg_sync_prim_devs --
 * Synchronize all Logical Group PRIMARY devices
 */
int
ftd_lg_sync_prim_devs(ftd_lg_t *lgp)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;
  	char			*devname;

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp)
	{
		// Get PRIMARY device name
		devname = (*devcfgpp)->pdevname;

       // Verify that the ftd Device exists
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL)
		{
			// dev not found
			reporterr(ERRFAC, M_DEVSYNC, ERRCRIT, devname, ftd_strerror());
			return -1;
		}

       // Synchronize the device
		if (ftd_dev_sync(devname, lgp->lgnum) == 0)
		{
           // Failed to synchronize the device
			reporterr(ERRFAC, M_DEVSYNC, ERRCRIT, devname, ftd_strerror());
			return -1;
		}
   }

   return 0;
}

#endif

/*
 * ftd_lg_close_devs --
 * close all devices in the group
 */
int
ftd_lg_close_devs(ftd_lg_t *lgp)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			// dev not found
			continue;
		}
		if (ftd_dev_close(devp) < 0) {
			return -1;
		}
	}

	return 0;
}

/*
 * ftd_lg_close_ftd_devs --
 * close all ftd devices in the group
 */
int
ftd_lg_close_ftd_devs(ftd_lg_t *lgp)
{
	ftd_dev_cfg_t	**devcfgpp;
	ftd_dev_t		*devp;

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		if ((devp = ftd_lg_devid_to_dev(lgp, (*devcfgpp)->devid)) == NULL) {
			// dev not found
			continue;
		}
		if (ftd_dev_ftd_close(devp) < 0) {
			return -1;
		}
	}

	return 0;
}


/*
 * ftd_lg_devid_to_dev --
 * return the ftd_dev_t object for the given devid
 */
ftd_dev_t *
ftd_lg_devid_to_dev(ftd_lg_t *lgp, int devid)
{
	ftd_dev_t **devpp;

	ForEachLLElement(lgp->devlist, devpp) {
		if ((*devpp)->devid == devid) {
			return (*devpp);
		}
	}

	return NULL;
}

/*
 * ftd_lg_devid_to_devcfg --
 * return the ftd_dev_cfg_t object for the given device
 */
ftd_dev_cfg_t *
ftd_lg_devid_to_devcfg(ftd_lg_t *lgp, int devid)
{
	ftd_dev_cfg_t **devcfgpp;

	ForEachLLElement(lgp->cfgp->devlist, devcfgpp) {
		if ((*devcfgpp)->devid == devid) {
			return (*devcfgpp);
		}
	}

	return NULL;
}

/*
 * ftd_lg_minor_to_dev --
 * return the ftd_dev_t object for the given minor number
 */
ftd_dev_t *
ftd_lg_minor_to_dev(ftd_lg_t *lgp, int minor)
{
	ftd_dev_t **devpp;

	ForEachLLElement(lgp->devlist, devpp) {
		if ((*devpp)->num == (dev_t)minor) {
			return (*devpp);
		}
	}

	return NULL;
}

/*
 * ftd_lg_ftd_to_dev --
 * return the ftd_dev_t object for the given ftd number
 */
ftd_dev_t *
ftd_lg_ftd_to_dev(ftd_lg_t *lgp, int ftdnum)
{
	ftd_dev_t **devpp;

	ForEachLLElement(lgp->devlist, devpp) {
		if ((*devpp)->ftdnum == (dev_t)ftdnum) {
			return (*devpp);
		}
	}

	return NULL;
}

/*
 * ftd_lg_housekeeping --
 * do peripheral tasks: save stats, get tunables, eval net usage,
 * update lrdb's
 */
int
ftd_lg_housekeeping(ftd_lg_t *lgp, int tune)
{
	time_t	currts;
	int		deltatime;

	// time to update internal state ? 
	time(&currts);
	deltatime = currts - lgp->statp->statts;

	if (deltatime >= 1) {
		
		// since the smallest tunable interval is 1 - we dump at
		// this  interval since doing it any more frequently yields 
		// no gain and taxes the pmd too much - thread candidate

		if (lgp->cfgp->role == ROLEPRIMARY) {
			
			if (tune) {
				// get tunables
				ftd_lg_get_driver_state(lgp, 0);

				// chunksize changed
				if (lgp->tunables->chunksize > lgp->buflen) {
					// re-allocate data buffer 
					lgp->buflen = lgp->tunables->chunksize;
					lgp->buf = (char*)realloc(lgp->buf, lgp->buflen);
				}
			}

#if !defined(_WINDOWS)
			ftd_lg_eval_net_usage(lgp);
#endif
			// dump stats
			ftd_stat_dump_driver(lgp);
		} else {
#if defined(_WINDOWS)
			// nothing is done on secondary for NT
#else
			if (deltatime >= 10) {
				// since this is not settable - we do it every 10
				// so as not to tax the rmd too much - thread candidate
				ftd_stat_dump_file(lgp);
			}
#endif
		}
		lgp->statp->statts = currts;
	}

	if (lgp->cfgp->role == ROLESECONDARY) {
		return 0;
	}

	// time to update LRDBs ? 
	deltatime = currts - lgp->lrdbts;

	if (deltatime >= 360) {
		ftd_ioctl_update_lrdbs(lgp->ctlfd, lgp->lgnum, 0);
		lgp->lrdbts = currts;
	}	

	return 0;
}

/*
 * ftd_lg_get_bab_chunk --
 * copy BAB block from kernel to user space
 */
static int
ftd_lg_get_bab_chunk(ftd_lg_t *lgp, int force)
{
	oldest_entries_t	oe;
	int					errnum = 0, pstate, rc = 0;

	lgp->bufoff = 0;
	lgp->datalen = 0;

	memset(&oe, 0, sizeof(oe));

	if (lgp->buflen < lgp->tunables->chunksize) {
		lgp->buflen = lgp->tunables->chunksize;
		lgp->buf = (char*)realloc(lgp->buf, lgp->buflen);
	}

	oe.addr = (int*)lgp->buf;
	oe.offset32 = lgp->offset;
	oe.len32 = lgp->buflen / sizeof(int);

#ifdef TDMF_TRACE
	fprintf(stderr,"\n*** getchunk: oe.offset32 = %d\n",
		oe.offset32);
	fprintf(stderr,"\n*** getchunk: requested length = %d\n",
		oe.len32 * sizeof(int));
#endif
	if ((rc = ftd_ioctl_oldest_entries(lgp->devfd, &oe)) < 0) {
		errnum = ftd_errno();
		if (errnum != EINVAL) {
			reporterr (ERRFAC, M_BABGET, ERRCRIT,
				lgp->devname, errnum);
			return -1;
		}
	}
#ifdef TDMF_TRACE
	fprintf(stderr,"\n*** getchunk: get-oldest rc = %d\n",rc);
	fprintf(stderr,"\n*** getchunk: oe.retlen32 = %d\n",oe.retlen32);
	fprintf(stderr,"\n*** getchunk: oe.state = %d\n",oe.state);
#endif

	switch(oe.state) {
	case FTD_MODE_PASSTHRU:
		return FTD_CPASSTHRU;
	case FTD_MODE_TRACKING:

		if (force)
			break;

		if (GET_LG_STATE(lgp->flags) == FTD_STRACKING) {
			// pmd already in tracking mode 
			break;
		} else if (GET_LG_STATE(lgp->flags) == FTD_SNORMAL) {
			reporterr (ERRFAC, M_BABOFLOW, ERRWARN, LG_PROCNAME(lgp));
			
			time(&lgp->oflowts);

			if (lgp->tunables->refrintrvl == 0) {
				// don't even try a refresh
				return FTD_CREFTIMEO;
			}
		}

		return FTD_CTRACKING;
	case FTD_MODE_REFRESH:
		
		if (force)
			break;

		pstate = GET_LG_STATE(lgp->flags);
		
		if (ftd_lg_state_to_driver_state(pstate) != oe.state) {
			if (pstate == FTD_SBACKFRESH) {
				// not ok if in backfresh mode
				reporterr (ERRFAC, M_DRVREFR, ERRWARN);
				return FTD_CINVALID;
			}
			return FTD_CREFRESH;
		}
		
		break;
	case FTD_MODE_BACKFRESH:
		
		pstate = GET_LG_STATE(lgp->flags);
		
		if (ftd_lg_state_to_driver_state(pstate) != oe.state) {
			if (pstate == FTD_SNORMAL) {
				// ok if in normal mode
				return FTD_CBACKFRESH;
			}
			reporterr (ERRFAC, M_DRVBACK, ERRWARN);
			return FTD_CINVALID;
		}
		
		break;
	case FTD_MODE_NORMAL:
		
		if (force)
			break;

		pstate = GET_LG_STATE(lgp->flags);
		
		if (ftd_lg_state_to_driver_state(pstate) != oe.state) {
			
			// override to normal - in this case set rsync to done
			// so we quit the operation and go to normal 
			SET_LG_RFDONE(lgp->flags);
			
			// must also tell remote
			if ((rc = ftd_sock_send_rsync_end(lgp->dsockp, lgp, pstate)) < 0) {
				return rc;
			}
			
			return FTD_CNORMAL;
		}
		
		break;
	default:
		break;
	}

	// no BAB entries left to get 
	if (errnum == EINVAL) {
		return 0;
	}

	lgp->datalen = oe.retlen32*sizeof(int); /* bytes */
	lgp->bufoff = 0;

	if (oe.retlen32 == 0) {
		return 0;
	}

	return 0;
}

/*
 * ftd_lg_bab_has_entries -
 * return true if BAB contains entries
 */
int
ftd_lg_bab_has_entries(ftd_lg_t *lgp, int allowed)
{
	ftd_stat_t	lgstat;
	int			rc, byteoff;

	rc = ftd_ioctl_get_group_stats(lgp->ctlfd, lgp->lgnum, &lgstat, 0);
	if (rc != 0) {
		return -1;
	}

	lgp->babused = lgstat.bab_used;
	lgp->babfree = lgstat.bab_free;

	byteoff = lgp->offset * sizeof(int);

	if (lgp->babused <= (byteoff + allowed)) {
		return 0;
	} else {
		return 1;
	}

	/*
	if (lgstat.wlentries > 0 && lgstat.wlsectors > 0) {
		return 1;
	}
	*/
}

/*
 * ftd_lg_get_counts --
 * step thru chunk by entry and bump counter
 */
int
ftd_lg_get_counts(ftd_lg_t *lgp, wlheader_t *entry, char *dataptr) 
{
	ftd_dev_t	*devp;
	int			entrylen;

	if ((devp = ftd_lg_minor_to_dev(lgp, entry->diskdev)) == NULL) {
		reporterr(ERRFAC, M_BADDEVID, ERRCRIT, entry->diskdev);
		return -1;
	}
	entrylen = entry->length << DEV_BSHIFT;

	if (lgp->compp->algorithm != 0) {
		// estimate this - since no way of knowing per device actual 
		devp->statp->actual += (int)(lgp->cratio * entrylen);
	} else {
		devp->statp->actual += entrylen;
	}

	devp->statp->effective += entrylen;
	devp->statp->entries++;

	return 0;
}


/*
 * ftd_lg_start_journaling --
 * start journaling entries on secondary system.
 */
int
ftd_lg_start_journaling(ftd_lg_t *lgp, int jrnstate) 
{

	if (ftd_journal_create_next(lgp->jrnp, jrnstate) == NULL) {
		return -1;
	}

	ftd_lg_close_devs(lgp);
		
	if (ftd_lg_open_devs(lgp, O_RDONLY, 0, 5) < 0) {
		return -1;
	}

	return 0;
}

/*
 * ftd_lg_stop_journaling --
 * stop journaling entries on secondary system.
 */
int
ftd_lg_stop_journaling(ftd_lg_t *lgp, int jrnstate, int jrnmode) 
{
	ftd_journal_file_t	*jrnfp;
	int					rc;

	ftd_journal_set_state(lgp->jrnp, jrnstate);

	// depends on target journal mode	
	switch(jrnmode) {
	case FTDJRNMIR:	
		// apply any coherent journals while journaling 
		if ((jrnfp = ftd_journal_create_next(lgp->jrnp, jrnstate)) == NULL) {
			return -1;
		}
		if (!GET_LG_CPON(lgp->flags)) {
			// only apply if we are NOT in checkpoint 
			ftd_lg_close_devs(lgp);

			if (ftd_lg_open_devs(lgp, O_RDONLY, 0, 5) < 0) {
				return -1;
			}
			if ((rc = ftd_sock_send_start_apply(lgp->lgnum, lgp->isockp, 0)) < 0)
			{
				return rc;
			}
		}
		break;
	case FTDMIRONLY:
		// stop journaling and open mirrors 
		lgp->jrnp->cur = NULL;

		ftd_lg_close_devs(lgp);
			
		if (ftd_lg_open_devs(lgp, O_RDWR | O_EXCL, 0, 5) < 0) {
			return -1;
		}

		break;
	default:
		break;
	}

	SET_JRN_MODE(lgp->jrnp->flags, jrnmode);
	SET_JRN_STATE(lgp->jrnp->flags, jrnstate);

	return 0;
}

/*
 * ftd_lg_traverse_chunk --
 * step thru chunk by entry and do work on entries
 */
int
ftd_lg_traverse_chunk(ftd_lg_t *lgp)
{
	wlheader_t		*wlhead;
	ftd_journal_t	*jrnp;
	ftd_journal_file_t *jrnfp;
	char			*dataptr, *chunk;
	int				chunkoff = 0, entrylen, bytesleft, rc = 0;
	int				ret;
	ftd_dev_t		*devp;
	ftd_dev_cfg_t	*devcfgp;
	ftd_uint64_t	offset, lock_len;
#if !defined(_WINDOWS)
	ftd_uint64_t	unlock_len;
#endif

	if (lgp->cfgp->role == ROLESECONDARY) {

		jrnp = lgp->jrnp;
		jrnfp = jrnp->cur;

		if ((jrnfp && (jrnfp->fd != INVALID_HANDLE_VALUE))
			&& GET_JRN_MODE(jrnp->flags) == FTDJRNMIR)
		{
			// check for apply done 
			if (ftd_lg_journal_apply_done(lgp))
			{
				if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDMIRONLY) < 0)
				{
					return -1;
				}
				
				// next entry gets written to mirror directly
				
			} else {

				// we lock the journal file for the entire chunk
				// as opposed to locking it for each entry 

				// lock the journal file segment
				offset = ftd_llseek(jrnfp->fd, 0, SEEK_END);
				
				if (offset == (ftd_uint64_t)-1) {
					reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
						jrnfp->name,
						0,
						ftd_strerror());
					return -1;
				}

				jrnfp->size = offset-1;
				lock_len = lgp->datalen;
			
				rc = ftd_journal_file_lock(jrnfp, offset, lock_len);
				if (rc == -1) {
					return rc;
				} else {
					SET_JRN_LOCK(jrnfp->flags);
				}

			}
		}
	}

	chunk = lgp->buf;
	chunkoff = lgp->bufoff;

	while (1) {
		if (chunkoff >= lgp->datalen) {
			rc = 0;
			break;
		}
		// only have a partial header left in chunk 
		if ((lgp->datalen - chunkoff) <= sizeof(wlheader_t)) {
			lgp->datalen = chunkoff;
			rc = 0;
			break;
		}
		
		wlhead = (wlheader_t*)(chunk + chunkoff);

#ifdef TDMF_TRACE
		fprintf(stderr,"\n*** %s - trav_chunk: wlhead->majicnum = %08x\n",
			LG_PROCNAME(lgp), wlhead->majicnum);
		fprintf(stderr,"\n*** %s - trav_chunk: wlhead->offset = %d\n",
			LG_PROCNAME(lgp), wlhead->offset);
		fprintf(stderr,"\n*** %s - trav_chunk: wlhead->length = %d\n",
			LG_PROCNAME(lgp), wlhead->length);
#endif

		entrylen = wlhead->length << DEV_BSHIFT;
		bytesleft = lgp->datalen - chunkoff - sizeof(wlheader_t);

		// if partial-entry then adjust chunk size and return 
		if (bytesleft && bytesleft < entrylen) {
			lgp->datalen = chunkoff;
			rc = 0;
			break;
		} 
		if (wlhead->majicnum != DATASTAR_MAJIC_NUM) {
			reporterr(ERRFAC, M_BADHDR, ERRCRIT);
			return -1;
		}
		dataptr = ((char*)wlhead + sizeof(wlheader_t));

		// bump buf pointer past this seen entry
		lgp->bufoff += sizeof(wlheader_t) + entrylen;

		// look for special entries 
		if (wlhead->offset == -1) {
			if (!memcmp(dataptr, MSG_INCO, strlen(MSG_INCO))) {
				error_tracef( TRACEINF, "ftd_lg_traverse_chunk():[%s] got MSG_INCO = %s", LG_PROCNAME(lgp), MSG_INCO );
				dataptr += strlen(MSG_INCO)+1;
				if (lgp->cfgp->role == ROLESECONDARY) {
					rc = FTD_CREFRESH;	// transition to refresh state 
					goto retstate;
				}
			} else if (!memcmp(dataptr, MSG_CO, strlen(MSG_CO))) {
				error_tracef( TRACEINF, "ftd_lg_traverse_chunk():[%s] got MSG_CO = %s", LG_PROCNAME(lgp), MSG_CO );
				dataptr += strlen(MSG_CO)+1;
				if (lgp->cfgp->role == ROLESECONDARY) {
					rc = FTD_CNORMAL;	// transition to normal state 
					goto retstate;
				}
			} else if (!memcmp(dataptr, MSG_CPON, strlen(MSG_CPON))) {
				error_tracef( TRACEINF, "ftd_lg_traverse_chunk():[%s] got MSG_CPON = %s", LG_PROCNAME(lgp), MSG_CPON );
				if (lgp->cfgp->role == ROLESECONDARY) {
					dataptr += strlen(MSG_CPON)+1;
					rc = FTD_CCPPEND;
					goto retstate;
				}
			} else if (!memcmp(dataptr, MSG_CPOFF, strlen(MSG_CPOFF))) {
				error_tracef( TRACEINF, "ftd_lg_traverse_chunk():[%s] got MSG_CPOFF = %s", LG_PROCNAME(lgp), MSG_CPOFF );
				if (lgp->cfgp->role == ROLESECONDARY) {
					dataptr += strlen(MSG_CPOFF)+1;
					rc = FTD_CCPOFF;
					goto retstate;
				}
			}
		} else {
			// call appropriate entry func
			if (lgp->cfgp->role == ROLEPRIMARY) {
				rc = ftd_lg_get_counts(lgp, wlhead, dataptr);
				if (rc < 0) {
					return -1;
				}
			} else {
				if ((devp =
					ftd_lg_minor_to_dev(lgp, wlhead->diskdev)) == NULL)
				{
					// report err
				}	
				
				switch(GET_JRN_MODE(jrnp->flags)) {
				case FTDMIRONLY:
					offset = ((ftd_uint64_t)(wlhead->offset)) << DEV_BSHIFT;
				
					if ((devcfgp =
						ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL)
					{
						reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
						return -1;
					}	
					if (ftd_dev_write(devp, dataptr, offset, entrylen,
						devcfgp->sdevname) < 0)
					{
						return -1;
					}
					break;
				case FTDJRNMIR:
				case FTDJRNONLY:
					rc = ftd_journal_file_write(lgp->jrnp,
						lgp->lgnum,
						devp->devid,
						wlhead->offset,
						wlhead->length,
						dataptr);
						
					if (rc < 0) {
						if (rc == -2) {
							reporterr(ERRFAC, M_JRNSPACE, ERRCRIT, lgp->jrnp->cur->name);
						} else if (rc == -3) {
							{
								char *filename;

								if (lgp->jrnp == NULL
									|| lgp->jrnp->cur == NULL)
								{
									filename = "No journal file";
									offset = 0;
								} else {
									filename = lgp->jrnp->cur->name;
									offset = lgp->jrnp->cur->offset;
								}
								reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
									filename,
									0,
									offset,
									(wlhead->length << DEV_BSHIFT),
									ftd_strerror());
							}
						}
						return rc;
					}
					break;
				default:
					break;
				}
			}
		}
		chunkoff += sizeof(wlheader_t) + entrylen;
	}

	lgp->bufoff = 0;

retstate:

	ret = rc;

	if (lgp->cfgp->role == ROLESECONDARY
		&& jrnfp
		&& GET_JRN_LOCK(jrnfp->flags))
	{
		// unlock the journal file 
		// make sure chunk gets sync'd to disk before acking 
		// the primary

		if (jrnfp && (jrnfp->fd != INVALID_HANDLE_VALUE) ) {
			ftd_fsync(jrnfp->fd);
		
#if !defined(_WINDOWS)
			unlock_len = ~(jrnfp->locked_length);

			rc = ftd_journal_file_unlock( jrnfp,
				jrnfp->locked_offset + jrnfp->locked_length,
				unlock_len );
#else
			rc = ftd_journal_file_unlock(jrnfp,	jrnfp->locked_offset, jrnfp->locked_length);
#endif
			if (rc == -1) {
				reporterr(ERRFAC, "JRNUNLOCK", ERRWARN,
					jrnfp->name, ftd_strerror());
				return -1;
			}
		
			UNSET_JRN_LOCK(jrnfp->flags);
		}
	}

	return ret;
}

/*
 * ftd_lg_proc_bab --
 * copy BAB blocks from kernel to user space and send to remote server
 */
int
ftd_lg_proc_bab(ftd_lg_t *lgp, int force)
{
	time_t		lastts, lastlrdb;
	int			rc, savechunksize;

	lastts = lastlrdb = 0;

	savechunksize = lgp->tunables->chunksize;

	if (lgp->tunables->chunkdelay > 0) {
		sleep(lgp->tunables->chunkdelay);
	}	

tryagain:

	if ((rc = ftd_lg_get_bab_chunk(lgp, force)) != 0) {
		goto errret;
	}

	//DPRINTF((ERRFAC,LOG_INFO," ftd_lg_proc_bab: lgp->datalen = %d\n",  		lgp->datalen));

	if (lgp->datalen > 0) {
		
		// traverse chunk - get device counts - adjust length 
		if ((rc = ftd_lg_traverse_chunk(lgp)) != 0) {
			goto errret;
		}

		/*
		 * if lgp->datalen == 0 after 'traverse_chunk' then
		 * we could not get an entry into the chunk buffer
		 * need a bigger chunk buffer
		 */
		if (lgp->datalen == 0) {

			// temporarily change chunksize to accomodate 
			// don't bump offset and return 
			
			lgp->tunables->chunksize *= 2;
			goto tryagain;
		}

		// send the chunk to the secondary 
		if ((rc = ftd_sock_send_bab_chunk(lgp)) < 0) {
			goto errret;
		}

		lgp->offset += (lgp->datalen / sizeof(int));
	} 

	lgp->tunables->chunksize = savechunksize;

	return 0;

errret:

	lgp->tunables->chunksize = savechunksize;

	return rc;
}

/*
 * ftd_lg_proc_signal --
 * process the lg signal
 */
int
ftd_lg_proc_signal(ftd_lg_t *lgp)
{
	int	s, rc = 0;

#if !defined(_WINDOWS)
	if ((rc = read(lgp->sigpipe[STDIN_FILENO], &s, sizeof(int))) < 0) {
		return rc;
	}
#else
	ResetEvent(lgp->procp->hEvent);
	s = lgp->procp->nSignal;
#endif

	switch(s) {
	case FTDCSIGUSR1:
		// handle kill signal
		rc = ftd_lg_sigusr1(lgp);
		break;
	case FTDCSIGPIPE:
		// handle pipe signal
		rc = ftd_lg_sigpipe(lgp);
		break;
	case FTDCSIGTERM:
		// handle term signal
		rc = ftd_lg_sigterm(lgp);
		break;
	case FTDCKILL:
		// handle rmd kill signal
		rc = ftd_lg_kill(lgp);
		break;
	default:
		break;
	}

	return rc;
}

/*
 * ftd_lg_select_events --
 */
int
ftd_lg_select_events(ftd_lg_t *lgp)
{
	int				rc, nselect = 0;
	long			io_sleep;
	int				errnum;
	struct timeval	seltime;
	fd_set			read_set;
	time_t			now;

#if defined(_WINDOWS)
#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#endif

top:

	FD_ZERO(&read_set);

#if !defined(_WINDOWS)	
	if (lgp->cfgp->role == ROLEPRIMARY) {
		// add BAB to readset
		FD_SET(lgp->devfd, &read_set);
	}
	
	// add signal queue to readset
	FD_SET(lgp->sigpipe[STDIN_FILENO], &read_set);

	if (lgp->cfgp->role == ROLEPRIMARY) {
		nselect = max( max(FTD_SOCK_FD(lgp->isockp),
			FTD_SOCK_FD(lgp->dsockp)),
				max(lgp->sigpipe[STDIN_FILENO], lgp->devfd));
	} else {
		nselect = max( max(FTD_SOCK_FD(lgp->isockp),
			FTD_SOCK_FD(lgp->dsockp)),
				lgp->sigpipe[STDIN_FILENO]);
	}
#endif
	// add ipc connection to readset
	if (FTD_SOCK_FD(lgp->isockp) != (SOCKET)-1) {
		FD_SET((u_int)FTD_SOCK_FD(lgp->isockp), &read_set);
	}
	
	// add net connection to readset
	if (FTD_SOCK_FD(lgp->dsockp) != (SOCKET)-1) {
		FD_SET((u_int)FTD_SOCK_FD(lgp->dsockp), &read_set);
	}

	if (lgp->cfgp->role == ROLESECONDARY) {
		
		// secondary system

		if (GET_LG_STATE(lgp->flags) == FTD_SAPPLY) {
			io_sleep = 0;
		} else {
			io_sleep = 1;
		}
	} else {

		// primary system

		if ( (GET_LG_STATE(lgp->flags) == FTD_SREFRESH
				|| GET_LG_STATE(lgp->flags) == FTD_SREFRESHC
				|| GET_LG_STATE(lgp->flags) == FTD_SREFRESHF 
				|| GET_LG_STATE(lgp->flags) == FTD_SBACKFRESH)
			&& !GET_LG_RFDONE_ACKPEND(lgp->flags) )
		{
			// rsync - just want to return immediately
			// also, check for primary waiting for RFDONE sentinel ACK
			// from secondary - we don't want to spin.
			io_sleep = 0;
		} else {
			io_sleep = 1; 
		}
	}

	seltime.tv_sec = io_sleep;
	seltime.tv_usec = 0;

	rc = select(++nselect, &read_set, NULL, NULL, &seltime);

	if (rc == -1) {
		
		errnum = sock_errno();
		
		error_tracef( TRACEERR, "ftd_lg_select_events():select rc, errno = %d, %d", rc, errnum );
		
		if (errnum == EWOULDBLOCK || errnum == EINTR) {
			goto top;
		} else {
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "select",
				sock_strerror(errnum));
			return FTD_CINVALID;
		}
	
	} else if (rc == 0) {
		
		// idle 
		if (lgp->idlets == 0) {
			time(&lgp->idlets);
			now = lgp->idlets;
		} else {
			time(&now);
		}

		lgp->timeslept = now - lgp->idlets;

		if (FTD_SOCK_CONNECT(lgp->dsockp)) {
			
			if (lgp->timeslept > 0
				&& (lgp->timeslept % FTD_NET_MAX_IDLE) == 0)
			{
				if (ftd_sock_test_link(FTD_SOCK_LHOST(lgp->dsockp),
					FTD_SOCK_RHOST(lgp->dsockp),
					FTD_SOCK_LIP(lgp->dsockp),
					FTD_SOCK_RIP(lgp->dsockp),
					FTD_SOCK_PORT(lgp->dsockp),
					FTD_SOCK_CONNECT_SHORT_TIMEO) == 0)
				{
					// link down
					return FTD_LG_NET_TIMEO;
				}
			
			} else if (lgp->timeslept > 0
				&& (lgp->timeslept % (FTD_NET_SEND_NOOP_TIME)) == 0)
			{
				// send a NOOP to the peer 
				if ((rc = ftd_sock_send_noop(lgp->dsockp, lgp->lgnum, 1)) < 0) {
					return rc;
				}
			}
		}
	} else {
		// check ipc channel
		if (FD_ISSET(FTD_SOCK_FD(lgp->isockp), &read_set)) {
			// incoming IPC msg 
			if ((rc = ftd_sock_recv_lg_msg(lgp->isockp, lgp, 0)) != 0) {
				if (rc != FTD_LG_NET_NOT_READABLE) {
					return rc;
				}
			}
		}

#if !defined(_WINDOWS)
		// check signals
		if (FD_ISSET(lgp->sigpipe[STDIN_FILENO], &read_set)) {
			// incoming signal 
			if ((rc = ftd_lg_proc_signal(lgp)) != 0) {
				return rc;
			}
		}

		if (GET_LG_BAB_READY(lgp->flags) && FTD_SOCK_CONNECT(lgp->dsockp)) {
			// check bab ready 
			if (FD_ISSET(lgp->devfd, &read_set)) {
				// incoming BAB
				if ((rc = ftd_lg_proc_bab(lgp, 0)) != 0) {
					// function returns driver state change 
					return rc;
				}
			}
		}

#endif
		if (FTD_SOCK_CONNECT(lgp->dsockp)) {
			// check wire for incoming packets
			if (FD_ISSET(FTD_SOCK_FD(lgp->dsockp), &read_set)) {

				lgp->idlets = 0;

				while (TRUE) {
					// get them all
					if ((rc = ftd_sock_recv_lg_msg(lgp->dsockp, lgp, 0)) != 0) {
						if (rc == FTD_LG_NET_NOT_READABLE) {
							// no more to read
							break;
						} else {
							return rc;
						}
						ftd_lg_housekeeping(lgp, 0);
					} else {
						// returned 0
						if (lgp->cfgp->role == ROLESECONDARY) {
							break;
						}
					}
				}
			}
		}
		rc = 0;
	}

	return rc;
}

#if defined(_WINDOWS)

/*
 * ftd_lg_wait_all_events --
 */
int
ftd_lg_wait_all_events(ftd_lg_t *lgp, HANDLE *hEvents, int nEvent)
{
	int		rc, ret = 0;
	long	io_sleep;
	time_t	now;

    if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s", LG_PROCNAME(lgp) ); }

	if ( (GET_LG_STATE(lgp->flags) == FTD_SREFRESH
			|| GET_LG_STATE(lgp->flags) == FTD_SREFRESHC
			|| GET_LG_STATE(lgp->flags) == FTD_SREFRESHF
			|| GET_LG_STATE(lgp->flags) == FTD_SBACKFRESH)
		&& !GET_LG_RFDONE_ACKPEND(lgp->flags) )
	{
		// rsync - just want to return immediately
		// also, check for primary waiting for RFDONE sentinel ACK
		// from secondary - we don't want to spin.
		io_sleep = 0;
	} else {
		io_sleep = 1; 
	}

	do {
		rc = WaitForMultipleObjects(nEvent, (CONST HANDLE *)hEvents, FALSE, io_sleep * 1000);

		switch(rc)
		{
		case WAIT_OBJECT_0:
			
			ftd_stat_dump_driver(lgp);

			break;

		case WAIT_OBJECT_0 + 1:
			
			// check signal 
			if ((ret = ftd_lg_proc_signal(lgp)) != 0)
				return ret;
			break;

		case WAIT_OBJECT_0 + 2:
			
            if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+2", LG_PROCNAME(lgp) ); }

			if (FTD_SOCK_CONNECT(lgp->isockp)) {
				
				// incoming IPC msg 
				if ((ret = ftd_sock_recv_lg_msg(lgp->isockp, lgp, 0)) != 0) {
					if (ret != FTD_LG_NET_NOT_READABLE) {
						if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+2, ftd_sock_recv_lg_msg 1 ret=%d", LG_PROCNAME(lgp), ret ); }
						return ret;
					}
				}
			}

			break;

		case WAIT_OBJECT_0 + 3:
			
            if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+3", LG_PROCNAME(lgp) ); }

			if (FTD_SOCK_CONNECT(lgp->dsockp)) {
				lgp->idlets = 0;

				// check wire for incoming packets
				if ((ret = ftd_sock_recv_lg_msg(lgp->dsockp, lgp, 0)) != 0) {
					if (ret == FTD_LG_NET_NOT_READABLE) {
						// none to read
					} else {
						if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+3, ftd_sock_recv_lg_msg 2 ret=%d", LG_PROCNAME(lgp), ret ); }
						return ret;
					}
				}
				ftd_lg_housekeeping(lgp, 0);
			}
			
			break;
			
		case WAIT_OBJECT_0 + 4:

            if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+4", LG_PROCNAME(lgp) ); }
			// check BAB
			if (hEvents[4] == INVALID_HANDLE_VALUE) {
				break;
			}
			
			if (FTD_SOCK_CONNECT(lgp->dsockp)) {

				if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+4, Before ftd_lg_proc_bab",LG_PROCNAME(lgp) ); }

				if ((ret = ftd_lg_proc_bab(lgp, 0)) != 0) {

						if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+4, After ftd_lg_proc_bab, ret=%d", LG_PROCNAME(lgp), ret ); }
					return ret;
				}

						if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_OBJECT_0+4, After ftd_lg_proc_bab, ret=%d", LG_PROCNAME(lgp), ret ); }
			} else {
				// since we don't call 'get_oldest_entries' - which
				// sets the bab event to 'signaled', we must tell ourselves
				// to do it later when we have a connection
				SET_LG_BAB_READY(lgp->flags);
			}
			
			break;
		case WAIT_TIMEOUT:
			
			if (FTD_SOCK_CONNECT(lgp->dsockp)) {

				if (lgp->idlets == 0) {
					time(&lgp->idlets);
					now = lgp->idlets;
				} else {
					time(&now);
				}
			
				lgp->timeslept = now - lgp->idlets;
				
				if (lgp->timeslept > 0
					&& (lgp->timeslept % FTD_NET_MAX_IDLE) == 0)
				{
					if (ftd_sock_test_link(FTD_SOCK_LHOST(lgp->dsockp),
						FTD_SOCK_RHOST(lgp->dsockp),
						FTD_SOCK_LIP(lgp->dsockp),
						FTD_SOCK_RIP(lgp->dsockp),
						FTD_SOCK_PORT(lgp->dsockp),
						FTD_SOCK_CONNECT_SHORT_TIMEO) == 0)

					{
						// link down
						return FTD_LG_NET_TIMEO;
					}
				
				} else if (lgp->timeslept > 0
					&& (lgp->timeslept % (FTD_NET_SEND_NOOP_TIME)) == 0)
				{
					// send a NOOP to the peer 
					if ((rc = ftd_sock_send_noop(lgp->dsockp, lgp->lgnum, 1)) < 0) {
						return rc;
					}
				}
			}

            if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:WAIT_TIMEOUT", LG_PROCNAME(lgp) ); }
			return 0;
		
		case WAIT_FAILED:
		default:
			reporterr(ERRFAC, M_SYSERR, ERRCRIT, "WaitForMultipleObjects",
				ftd_strerror());
			return FTD_CINVALID;
		}

		// break out of here if we are in rsync mode and we
		// recvd the DONE ack from the secondary
		if (GET_LG_RFDONE(lgp->flags))
		{
			break;
		}

	} while (TRUE);

            if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_wait_all_events():%s:End Of Function", LG_PROCNAME(lgp) ); }

	return ret;
}

/*
 * ftd_lg_wait_secondary_events --
 */
int
ftd_lg_wait_secondary_events(ftd_lg_t *lgp, HANDLE *hEvents, int nEvent)
{
	int		rc, ret = 0;

	rc = WaitForMultipleObjects(nEvent, (CONST HANDLE *)hEvents, FALSE, 0);
	
	switch(rc)
	{
	case WAIT_OBJECT_0:
		// incoming signal 
		if ((ret = ftd_lg_proc_signal(lgp)) != 0) {
			return ret;
		}
		break;
	case WAIT_TIMEOUT:
		return 0;
	case WAIT_FAILED:
	default:
		{
			DWORD err = GetLastError();
			reporterr(ERRFAC, M_SYSERR, ERRCRIT, "WaitForMultipleObjects",
				ftd_strerror());
			return -1;
		}
	}

	return ret;
}

#endif

/*
 * ftd_lg_dispatch_io --
 * dispatch i/o events for the lg
 */
int
ftd_lg_dispatch_io(ftd_lg_t *lgp)
{
	int				rc;
	ftd_header_t	ack;
#if defined(_WINDOWS)
	HANDLE	hEvents[5];
	int		nEvent;
#endif
	int		state;

    if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_dispatch_io():%s", LG_PROCNAME(lgp) ); }


	if (lgp->cfgp->role == ROLESECONDARY
		&& GET_LG_STATE(lgp->flags) != FTD_SAPPLY)
	{

		// check for apply done 

		if (GET_JRN_MODE(lgp->jrnp->flags) == FTDJRNMIR
			&& ftd_lg_journal_apply_done(lgp))
		{
			if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDMIRONLY) < 0) {
				return -1;
			}
			
			// next entry gets written to mirror directly
		
		}

		if (lgp->bufoff) {
			if (lgp->bufoff == -1) {
				lgp->bufoff = 0;
			}

			// still have data in buffer to process - 
			// we recvd a state transition sentinel last time
			// we called traverse chunk - so, process buffer remains

			if ((rc = ftd_lg_traverse_chunk(lgp)) == 0) {

				memset(&ack, 0, sizeof(ack));

				// tell primary to migrate the chunk
				ack.msgtype = FTDACKCHUNK;
				ack.msg.lg.len = lgp->datalen;
				
				if (GET_LG_RFSTART(lgp->flags)) {
					SET_LG_RFSTART(ack.msg.lg.flags);
					UNSET_LG_RFSTART(lgp->flags);
				}

				if (GET_LG_RFDONE(lgp->flags)) {
					SET_LG_RFDONE(ack.msg.lg.flags);
					UNSET_LG_RFDONE(lgp->flags);
				}
				if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_dispatch_io",lgp->dsockp, &ack)) < 0) {
					return rc;
				}
			} else {
				return rc;
			}
		}
	}
	
	lgp->bufoff = 0;

	if (lgp->cfgp->role == ROLEPRIMARY) {

#if defined(_WINDOWS)			
		nEvent = 5;
#endif
		
		state = GET_LG_STATE(lgp->flags);

		switch(state) {
		case FTD_SREFRESH:
		case FTD_SREFRESHC:
		case FTD_SREFRESHF:
				//WR32435
				//Chnaged By veera This is not Reqd
//                if (!GET_LG_RFSTART_ACKPEND(lgp->flags)) 
                {
#if defined(_WINDOWS)			
				hEvents[4] = lgp->babfd;
#else
				SET_LG_BAB_READY(lgp->flags);
#endif
//                    break;
                }
				break;

			// dtop thru

		case FTD_STRACKING:
		case FTD_SREFOFLOW:

#if defined(_WINDOWS)			
			hEvents[4] = INVALID_HANDLE_VALUE;	// for fsm use
			nEvent = 4;
#else
			UNSET_LG_BAB_READY(lgp->flags);
#endif
			break;
		default:

#if defined(_WINDOWS)			
			hEvents[4] = lgp->babfd;
#else
			SET_LG_BAB_READY(lgp->flags);
#endif
			break;
		}
	}

#if !defined(_WINDOWS)

	// UNIX - query all io channels for events and dispatch accordingly

	rc = ftd_lg_select_events(lgp);

#else
	{
		if (lgp->cfgp->role == ROLEPRIMARY) {
			
			// primary system 

			hEvents[0] = lgp->perffd;
			hEvents[1] = lgp->procp->hEvent;
			hEvents[2] = FTD_SOCK_HEVENT(lgp->isockp);
			hEvents[3] = FTD_SOCK_HEVENT(lgp->dsockp);
			
			rc = ftd_lg_wait_all_events(lgp, hEvents, nEvent);
            if ( bDbgLogON )    { error_tracef( TRACEINF4, "ftd_lg_dispatch_io():%s:After ftd_lg_wait_all_events(), rc == %d", LG_PROCNAME(lgp), rc ); }
            //if ( bDbgLogON )    { DPRINTF((ERRFAC,LOG_INFO,"\n" )); }

		} else {
			
			// secondary system 
			// Events - signals 
			nEvent = 1;
			hEvents[0] = lgp->procp->hEvent;

			if ((rc = ftd_lg_wait_secondary_events(lgp, hEvents, nEvent)) != 0) {
				return rc;
			}
			
			// select - for all i/o 
			rc = ftd_lg_select_events(lgp);
		}
	}
#endif	

    if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_lg_dispatch_io():%s:End Of Function", LG_PROCNAME(lgp) ); }
	return rc;
}

#if defined(_WINDOWS)

/*
 * ftd_lg_check_signals --
 * dispatch signal event for the lg
 */
int
ftd_lg_check_signals(ftd_lg_t *lgp, int timeo)
{
	int		rc, ret = 0;

	int		nEvent = 1;
	HANDLE	hEvents[1];
	
	if (lgp && lgp->procp && lgp->procp->hEvent) {
		hEvents[0] = lgp->procp->hEvent;
	} else {
		return 0;
	}

	rc = WaitForMultipleObjects(nEvent, (CONST HANDLE *)hEvents, FALSE, timeo * 1000);
	
	switch(rc)
	{
	case WAIT_OBJECT_0:
		// incoming signal 
		if ((rc = ftd_lg_proc_signal(lgp)) == FTD_CINVALID) {
			ret = FTD_EXIT_DIE;
		}
		break;
	case WAIT_TIMEOUT:
		ret = 0;
		break;
	case WAIT_FAILED:
	default:
		{
			DWORD err = GetLastError();
			ret = -1;
		}
	}

	return ret;
}

#endif

/*
 * ftd_lg_eval_net_usage --
 * evaluate network usage for the group and throttle accordingly  
 */
int
ftd_lg_eval_net_usage(ftd_lg_t *lgp) 
{
	ftd_dev_t			**devpp;
#if !defined(_WINDOWS)
	struct timeval		seltime;
#endif
	time_t				currentts = 0;
    int					netmaxkbps, deltasec;
	ftd_int64_t			actual, effective;

	netmaxkbps = lgp->tunables->netmaxkbps;

	if (netmaxkbps < 0) {
		goto noeval;
	}
	time(&currentts);

	deltasec = currentts - lgp->kbpsts;

	if (deltasec <= 0) {
		goto noeval;
	}
	lgp->prev_kbps = lgp->kbps;

	actual = effective = 0;

	ForEachLLElement(lgp->devlist, devpp) {
		actual += (*devpp)->statp->actual;
		effective += (*devpp)->statp->effective;
	}

	// calculate kbps 

	error_tracef( TRACEINF4, "ftd_lg_eval_net_usage():%s:actual, deltasec = %d, %d", LG_PROCNAME(lgp), actual, deltasec );
	lgp->kbps = (float)((float)(actual * 1.0) /
		(float)((deltasec * 1.0)) / 1024.0);

	//DPRINTF((ERRFAC,LOG_INFO,"eval_netspeed: kbps, netmaxkbps = %6.2f, %6.2f",
	//	(float) lgp->kbps, 
	//	(float) netmaxkbps));

	if (lgp->kbps > netmaxkbps) {
		lgp->netsleep += 500000;
	} else if (lgp->kbps < netmaxkbps) {
		if (lgp->netsleep > 500000) {
			lgp->netsleep >>= 1;
		} else  {
			lgp->netsleep = 0;
		}
	}

	error_tracef( TRACEINF4, "ftd_lg_eval_net_usage():%s:kbps = %6.2f, sleeping for %d", LG_PROCNAME(lgp), lgp->kbps, lgp->netsleep );

#if !defined(_WINDOWS)	
	if (lgp->netsleep < 1000000) {
		seltime.tv_sec = 0;
		seltime.tv_usec = lgp->netsleep;
	} else {
		seltime.tv_sec = lgp->netsleep/1000000;
		seltime.tv_usec = lgp->netsleep%1000000;
	}
	select(0, NULL, NULL, NULL, &seltime);
#else
/*

NT doesn't have signals to interupt us so don't sleep here.
sleep in main pmd i/o loop so we can catch other events

	usleep(lgp->netsleep);
*/
#endif

noeval:

	lgp->kbpsts = currentts;

	return 0;
}

/*
 * ftd_lg_set_driver_run_state -- sets the lg run state in the driver
 */
int
ftd_lg_set_driver_run_state(int lgnum, int dstate)
{
	HANDLE	ctlfd = INVALID_HANDLE_VALUE;

	error_tracef( TRACEINF4, "ftd_lg_set_driver_run_state():LG%d, state %d", lgnum, dstate);

	if ((ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
		error_tracef( TRACEERR, "ftd_lg_set_driver_run_state(%d): INVALID_HANDLE_VALUE error", lgnum);
		goto errret;
	}

	switch(dstate) {
	case FTD_MODE_NORMAL:
	case FTD_MODE_REFRESH:
	case FTD_MODE_BACKFRESH:
	case FTD_MODE_TRACKING:
	case FTD_MODE_PASSTHRU:
		break;
	default:
		dstate = ftd_lg_get_driver_run_state(lgnum);
		break;
	}

	if (ftd_ioctl_set_group_state(ctlfd, lgnum, dstate) < 0) {
		error_tracef( TRACEERR, "ftd_lg_set_driver_run_state(%d): ftd_ioctl_set_group_state error", lgnum);
		goto errret;
	}
    
	if (ctlfd != INVALID_HANDLE_VALUE) {
		ftd_close(ctlfd);
	}

	return 0;

errret:

	if (ctlfd != INVALID_HANDLE_VALUE) {
		ftd_close(ctlfd);
	}

	return -1;
}

/*
 * ftd_lg_get_driver_run_state -- returns the lg state from the driver
 */
int
ftd_lg_get_driver_run_state(int lgnum)
{
	HANDLE	ctlfd = INVALID_HANDLE_VALUE;
	int		state = -1;

	error_tracef( TRACEINF4, "ftd_lg_get_driver_run_state():LG%d ", lgnum);

	if ((ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
		error_tracef( TRACEERR, "ftd_lg_get_driver_run_state(%d): INVALID_HANDLE_VALUE error", lgnum);
		goto errret;
	}

	if ((state = ftd_ioctl_get_group_state(ctlfd, lgnum)) < 0) {
		error_tracef( TRACEERR, "ftd_lg_get_driver_run_state(): ftd_ioctl_get_group_state(LG%d) error", lgnum);
		goto errret;
	}
    
	if (ctlfd != INVALID_HANDLE_VALUE) {
		ftd_close(ctlfd);
	}

	return state;

errret:

	if (ctlfd != INVALID_HANDLE_VALUE) {
		ftd_close(ctlfd);
	}

	return -1;
}

/*
 * ftd_lg_set_pstore_run_state -- sets the lg run state in the pstore 
 */
int
ftd_lg_set_pstore_run_state(int lgnum, ftd_lg_cfg_t *cfgp, int state)
{
	char	*statestr, group_name[MAXPATHLEN];
	int		rc;
	
	FTD_CREATE_GROUP_NAME(group_name, lgnum);

/* JRL removed, reading the cur file was corrupting cfgp
	if (ftd_config_read(cfgp, lgnum, ROLEPRIMARY, 1) < 0) {
		ftd_config_lg_delete(cfgp);
		return -1;
	}
*/
	if (state == FTD_SNORMAL) {
		statestr = "NORMAL";
	} else if (state == FTD_SBACKFRESH) {
		statestr = "BACKFRESH";
	} else if (state == FTD_SREFRESH) {
		statestr = "REFRESH";
	} else if (state == FTD_SREFRESHC) {
		statestr = "CHECK_REFRESH";
	} else if (state == FTD_SREFRESHF) {
		statestr = "FULL_REFRESH";
	} else {
		statestr = "ACCUMULATE";
	}  

	if ((rc = ps_set_group_key_value(cfgp->pstore, group_name,
		"_MODE:", statestr)) != 0) {
		return -1;
	}

	error_tracef( TRACEINF4, "ftd_lg_set_pstore_run_state():statestr = %s", statestr );
	
	return 0;
}

/*
 * ftd_lg_get_pstore_run_state -- returns the lg state from the pstore 
 */
int
ftd_lg_get_pstore_run_state(int lgnum, ftd_lg_cfg_t *cfgp)
{
	char			statestr[32], group_name[MAXPATHLEN];
	int				rc, state = 0;
	
	FTD_CREATE_GROUP_NAME(group_name, lgnum);

	if (ftd_config_read(cfgp, lgnum, ROLEPRIMARY, 1) < 0) {
		ftd_config_lg_delete(cfgp);
		return -1;
	}

	if ((rc = ps_get_group_key_value(cfgp->pstore, group_name,
		"_MODE:", statestr)) != 0) {
		return -1;
	}

	error_tracef( TRACEINF4, "ftd_lg_get_pstore_run_state():statestr = %s", statestr );
	
	if (!strcmp(statestr, "NORMAL")) {
		state = FTD_SNORMAL;
	} else if (!strcmp(statestr, "BACKFRESH")) {
		state = FTD_SBACKFRESH;
	} else if (!strcmp(statestr, "REFRESH")) {
		state = FTD_SREFRESH;
	} else if (!strcmp(statestr, "CHECK_REFRESH")) {
		state = FTD_SREFRESHC;
	} else if (!strcmp(statestr, "FULL_REFRESH")) {
		state = FTD_SREFRESHF;
	} else if (!strlen(statestr)) {
		state = FTD_SNORMAL;
	}  

	return state;
}

/*
 * ftd_lg_driver_state_to_lg_state --
 * returns the equivalent lg state given the driver state
 */
int
ftd_lg_driver_state_to_lg_state(int dstate)
{

	switch(dstate) {
	case FTD_MODE_REFRESH:
		//return FTD_SREFRESH;
	case FTD_MODE_TRACKING:
		return FTD_STRACKING;
	case FTD_MODE_BACKFRESH:
		return FTD_SBACKFRESH;
	case FTD_MODE_NORMAL:
		return FTD_SNORMAL;
	case FTD_MODE_PASSTHRU:
		return FTD_SPASSTHRU;
	default:
		return -1;
	}

}

/*
 * ftd_lg_state_to_driver_state --
 * returns the equivalent driver state given the lg state
 */
int
ftd_lg_state_to_driver_state(int state)
{

	switch(state) {
	case FTD_SREFRESH:
	case FTD_SREFRESHC:
	case FTD_SREFRESHF:
		return FTD_MODE_REFRESH;
	case FTD_SBACKFRESH:
		return FTD_MODE_BACKFRESH;
	case FTD_SNORMAL:
		return FTD_MODE_NORMAL;
	case FTD_STRACKING:
		return FTD_MODE_TRACKING;
	default:
		return -1;
	}

}

/*
 * ftd_lg_journal_chunk_apply -- apply the next journal chunk to mirrors 
 */
static int
ftd_lg_journal_chunk_apply(ftd_lg_t *lgp)
{
	ftd_lg_header_t		*header;
	ftd_dev_t			*devp;
	ftd_dev_cfg_t		*devcfgp;
	ftd_journal_file_t	**jrnfpp;
	char				*datap, filename[MAXPATHLEN];
	struct stat			statbuf;
	ftd_uint64_t		offset, off64, bytes_remain, lock_len;
	int					chunksize, rc, found;

#if defined(_WINDOWS)
	sprintf(filename, "%s\\%s",
		lgp->jrnp->jrnpath, lgp->jrnp->cur->name);
#else
	sprintf(filename, "%s/%s",
		lgp->jrnp->jrnpath, lgp->jrnp->cur->name);
#endif

	offset = lgp->jrnp->cur->offset;
	lock_len = lgp->jrnp->cur->chunksize + 1;

	// process the journal 
#if !defined(_WINDOWS)
	if ((offset =
		ftd_llseek(lgp->jrnp->cur->fd, lgp->jrnp->cur->offset, SEEK_SET)) ==
			(ftd_uint64_t)-1)
	{
		reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			filename,
			lgp->jrnp->cur->offset, 
			ftd_strerror());
		return -1;
	}
#endif
	if ((rc = ftd_journal_file_lock(lgp->jrnp->cur,
		lgp->jrnp->cur->offset, lock_len)) < 0)
	{
		reporterr(ERRFAC, "JRNLOCK", ERRWARN,
			filename, offset, lock_len, ftd_strerror());
		return -1;
	}
	SET_JRN_LOCK(lgp->jrnp->cur->flags);

	// we have a lock on the section - get the size 
	if (stat(filename, &statbuf) == -1) {
		reporterr(ERRFAC, M_STAT, ERRCRIT,
			filename, ftd_strerror());
		return -1;
	}
	lgp->jrnp->cur->size = statbuf.st_size;

	bytes_remain = lgp->jrnp->cur->size - lgp->jrnp->cur->offset;
	chunksize = (int)(lgp->jrnp->cur->chunksize < bytes_remain ?
		lgp->jrnp->cur->chunksize: bytes_remain);

	if (chunksize <= sizeof(ftd_header_t)) {
			
		// file done
		
		found = 0;	
		rc = 0;
		
		// find file in file list
		ForEachLLElement(lgp->jrnp->jrnlist, jrnfpp) {
			if (*jrnfpp == lgp->jrnp->cur) {
				found = 1;
				break;
			}
		}
		
		if (found) {
			rc = ftd_journal_file_delete(lgp->jrnp, jrnfpp);
		}

		lgp->jrnp->cur = NULL;
		
		if (rc == 1) {
			// we have finished with the last coherent file 
			return rc;
		}
			
		return 0;
	}

	if (chunksize > lgp->jrnp->buflen) {
		// re-allocate data buffer 
		lgp->jrnp->buflen = chunksize;
		lgp->jrnp->buf = (char*)realloc(lgp->jrnp->buf, lgp->jrnp->buflen);
	}
	// read journal chunk 
	if ((rc = ftd_read(lgp->jrnp->cur->fd,
		lgp->jrnp->buf, chunksize)) != chunksize)
	{
		reporterr(ERRFAC, M_READERR, ERRCRIT,
			filename,
			offset,
			chunksize,
			ftd_strerror());
		return -1;
	}

	offset = 0;

	// process journal chunk entries 
	while (chunksize > 0) {
		if (chunksize < sizeof(ftd_header_t)) {
			break;
		}
		header = (ftd_lg_header_t*)(lgp->jrnp->buf + offset);
		offset += sizeof(ftd_lg_header_t);
		chunksize -= sizeof(ftd_lg_header_t);

		header->len <<= DEV_BSHIFT;

		//DPRINTF((ERRFAC,LOG_INFO,				" chunksize, offset, header->devid, header->offset, header->len = %d, %llu, %d, %d, %d\n",	chunksize, offset,	header->devid, header->offset, header->len));

		if (chunksize < header->len) {
			// done with this chunk 
			offset -= sizeof(ftd_lg_header_t);
			if (offset == 0) {
				// need bigger chunk 
				lgp->jrnp->cur->chunksize *= 2;
			}
			break;
		}
		datap = lgp->jrnp->buf + offset;

		// find target mirror device 
		if ((devp = ftd_lg_devid_to_dev(lgp, header->devid)) == NULL) {
			reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->devid);
			return -1;
		}
		if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, header->devid)) == NULL) {
			reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->devid);
			return -1;
		}

		off64 = header->offset;
		off64 <<= DEV_BSHIFT;

		// write the block to the mirror
		if (ftd_dev_write(devp, datap, off64, header->len,
			devcfgp->sdevname) < 0) {
			return -1;
		}
				
		// bump journal offset 
		offset += header->len;
		chunksize -= header->len;
	} 
	
	// unlock the locked journal section 
#if !defined(_WINDOWS)
	rc = ftd_journal_file_unlock(lgp->jrnp->cur,
		lgp->jrnp->cur->offset + lock_len, ~(lock_len));
#else
	rc = ftd_journal_file_unlock(lgp->jrnp->cur,
		lgp->jrnp->cur->offset, lock_len);
#endif
	if (rc == -1) {
		reporterr(ERRFAC, M_JRNUNLOCK, ERRWARN,
			filename, ftd_strerror());
		return -1;
	}
	UNSET_JRN_LOCK(lgp->jrnp->cur->flags);

	lgp->jrnp->cur->offset += offset;

	return 0;
}

/*
 * ftd_lg_journal_get_next_file -- open next journal file
 */
static int
ftd_lg_journal_get_next_file(ftd_lg_t *lgp)
{
	ftd_journal_file_t	**jrnfpp, *jrnfp;
	ftd_jrnheader_t		jrnheader;
	struct stat			statbuf;
	int					jrncnt, lnum, lstate, lmode, rc, n;
	char				filename[MAXPATHLEN];

	lgp->jrnp->cpnum = -1;

	if ((jrncnt = ftd_journal_get_all_files(lgp->jrnp)) == 0) {
		// no more journals - done 
		return 1;
	}

	jrnfpp = HeadOfLL(lgp->jrnp->jrnlist);
	jrnfp = *jrnfpp;
	lgp->jrnp->cur = jrnfp;

	ftd_journal_parse_name(lgp->jrnp->flags, jrnfp->name,
		&lnum, &lstate, &lmode);
       
	// if state == INCO - done 
	if (lstate == FTDJRNINCO) {
		return 1;
	}

	// if lnum > checkpoint journal num - done 
	if (lgp->jrnp->cpnum >= 0 && lnum >= lgp->jrnp->cpnum) {
		
		// set CPPEND flag to cause a notification to rmd to go ahead
		// with final transition to cp
		SET_LG_CPPEND(lgp->flags);

		return 1;
	}

#if defined(_WINDOWS)
	sprintf(filename, "%s\\%s", lgp->jrnp->jrnpath, jrnfp->name);
#else
	sprintf(filename, "%s/%s", lgp->jrnp->jrnpath, jrnfp->name);
#endif
	if (stat(filename, &statbuf) < 0) {
		return -1;
	}

	// if current journal file size is 0 - done 
	if (statbuf.st_size == 0) {
		return 1;
	}

	if ((jrnfp->fd = ftd_open(filename, O_RDWR, 0))
		== INVALID_HANDLE_VALUE)
	{
		reporterr(ERRFAC, M_FILE, ERRCRIT,
			filename, ftd_strerror()); 
		return -1;
	} 

	if (ftd_journal_file_lock_header(jrnfp) < 0)
	{
		reporterr(ERRFAC, "JRNLOCK", ERRWARN,
			filename, ftd_strerror());
		return -1;
	}
	
	SET_JRN_LOCK(jrnfp->flags);

	n = sizeof(ftd_jrnheader_t);
	if ((rc = ftd_read(jrnfp->fd, &jrnheader, n)) != n) {
		ftd_uint64_t	size = 0;

		ftd_journal_file_unlock_header(jrnfp);
		UNSET_JRN_LOCK(jrnfp->flags);

		reporterr(ERRFAC, M_READERR, ERRCRIT,
			filename,
			size,
			n,
			ftd_strerror());
		return -1;
	}
		
	if (ftd_journal_file_unlock_header(jrnfp) < 0)
	{
		reporterr(ERRFAC, "JRNUNLOCK", ERRWARN,
			filename, ftd_strerror());
		return -1;
	}
	
	UNSET_JRN_LOCK(jrnfp->flags);

	if (jrnheader.magicnum != FTDJRNMAGIC) {
		reporterr(ERRFAC, M_JRNMAGIC, ERRCRIT, filename); 
		return -1;
	}

	jrnfp->chunksize = JRN_CHUNKSIZE;
	jrnfp->offset = sizeof(ftd_jrnheader_t);
	jrnfp->size = -1;

	error_tracef( TRACEINF4, "ftd_lg_journal_get_next_file():%s: jrnoff = %llu", LG_PROCNAME(lgp), jrnfp->offset );

	return 0;
}

/*
 * ftd_lg_apply -- apply logical group journal file to mirrors
 */
int
ftd_lg_apply(ftd_lg_t *lgp)
{
	int					rc;

	// do we need to get the next file ?
	if (lgp->jrnp->cur == NULL) {

		rc = ftd_lg_journal_get_next_file(lgp);
		
		if (rc == 0) {
			return 0;
		} else if (rc == 1) {
			// done
			return FTD_LG_APPLY_DONE;
		} else {
			// error
			return FTD_CINVALID;
		}
	}
	
	rc = ftd_lg_journal_chunk_apply(lgp);

	if (rc == 0) {
		return 0;
	} else if (rc == 1) {
		// done
		return FTD_LG_APPLY_DONE;
	} else {
		// error
		return FTD_CINVALID;
	}

}

/*
 * ftd_lg_journal_apply_done --
 * test for shutting down journaling and going back to mirroring
 */
int
ftd_lg_journal_apply_done(ftd_lg_t *lgp)
{
	ftd_uint64_t		rc;
	ftd_journal_file_t	**jrnfpp;
	char				filename[MAXPATHLEN];
	int					found;

getfilelist:

	rc = ftd_journal_get_all_files(lgp->jrnp);
	
	if (rc < 0) {
		return -1;
	} else if (rc == 0) {
		return 1;
	}

	if (lgp->jrnp->cur->fd != INVALID_HANDLE_VALUE) {
		
		rc = ftd_llseek(lgp->jrnp->cur->fd, 0, SEEK_END);
		
		if (rc < 0) {
			return -1;
		} else if (rc == 0) {

			// have zero size file as first file  
			// rmda has truncated it and exited 

			// clobber file 

			// get list element for cur
			ForEachLLElement(lgp->jrnp->jrnlist, jrnfpp) {
				if (*jrnfpp == lgp->jrnp->cur) {
					found = 1;
					break;
				}
			}

			{
				int	i;

				if (found) {
						
					for(i=0;i<3;i++) {
						if (ftd_journal_file_delete(lgp->jrnp, jrnfpp) == 0) {
							break;
						}
						sleep(1);
					}

				} else {
					
					// do it manually, I guess
#if defined(_WINDOWS)					
					sprintf(filename, "%s\\%s", lgp->jrnp->jrnpath, lgp->jrnp->cur->name);
#else
					sprintf(filename, "%s/%s", lgp->jrnp->jrnpath, lgp->jrnp->cur->name);
#endif					
					ftd_journal_file_close(lgp->jrnp->cur);
					lgp->jrnp->cur = NULL;
					
					for(i=0;i<3;i++) {
						if (unlink(filename) == 0) {
							break;
						}
						sleep(1);
					}
				}
			}

			goto getfilelist;
			
		} else {
			return 0;
		}
	}

	return 0;
}

/*
 * ftd_lg_report_invalid_op --
 * report invalid operation and ack master
 */
int
ftd_lg_report_invalid_op(ftd_lg_t *lgp, short inchar)
{
	ftd_header_t	ack;
	char			*opstring = "";
	int				rc;

	switch(inchar) {
	case FTD_CREFRESH:
	case FTD_CREFRESHC:
	case FTD_CREFRESHF:
		opstring = "launchrefresh";
		break;
	case FTD_CNORMAL:
		opstring = "launchpmds";
		break;
	case FTD_CBACKFRESH:
		opstring = "launchbackfresh";
		break;
	default:
		break;
	}

	reporterr(ERRFAC, M_CMDIGNORE, ERRWARN,
		opstring, LG_PROCNAME(lgp), opstring);

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKCLD;
	ack.msg.lg.lgnum = lgp->lgnum;
	ack.msg.lg.data = lgp->cfgp->role;

	if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_report_invalid_op",lgp->isockp, &ack)) < 0) {
		return rc;
	}

	return 0;
}

