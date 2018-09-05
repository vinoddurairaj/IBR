/*
 * ftd_stat.c - FTD logical group statistics interface
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
#include "ftd_ps.h"
#include "ftd_lg.h"
#include "ftd_error.h"
#include "ftd_ioctl.h"

#if !defined(_WINDOWS)
/*
 * ftd_stat_init_file --
 * initialize lg stat file
 */
int
ftd_stat_init_file(ftd_lg_t *lgp)
{
	FILE		*hdrfd;
	char		fil[MAXPATHLEN], fil2[MAXPATHLEN];
	char		filhdr[MAXPATHLEN], perfname[MAXPATHLEN];
#if defined(HPUX)
	char		procname[MAXPATHLEN];
#endif
	struct stat	statbuf;

	if (lgp->cfgp->role == ROLEPRIMARY) {
		// if primary doesn't want a file then skip it
		// could the design be symmetric ? ... no role ...whine
		if (lgp->tunables->stattofileflag == 0
		|| lgp->tunables->statinterval < 1) {
			return -1;
		}
	}
	if (0 != stat(PATH_VAR_OPT, &statbuf)) {
		(void)mkdir(PATH_VAR_OPT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	if (0 != stat(PATH_RUN_FILES, &statbuf)) {
		(void)mkdir(PATH_RUN_FILES, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	if (lgp->cfgp->role == ROLEPRIMARY) {
		sprintf(filhdr, "%s/%s%03d.phd",
			PATH_RUN_FILES, PRIMARY_CFG_PREFIX, lgp->lgnum);
		sprintf(perfname, "%s/%s%03d.prf",
			PATH_RUN_FILES, PRIMARY_CFG_PREFIX, lgp->lgnum);
    } else {
		sprintf(filhdr, "%s/%s%03d.phd",
			PATH_RUN_FILES, SECONDARY_CFG_PREFIX, lgp->lgnum);
		sprintf(perfname, "%s/%s%03d.prf",
			PATH_RUN_FILES, SECONDARY_CFG_PREFIX, lgp->lgnum);
    }
	sprintf(fil, "%s", perfname);
	sprintf(fil2, "%s.1", perfname);
	(void)unlink(fil2);
	(void)unlink(filhdr);
	(void)rename(fil, fil2);

	/* print a "table of contents" for performance file created */
	if ((hdrfd = fopen(filhdr, "w")) == NULL) {
		reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, ftd_strerror());
		return -1;
	}
	if (lgp->cfgp->role == ROLEPRIMARY) {
		fprintf (hdrfd, "\"ts\" x\n");
		fprintf (hdrfd, "\"dummy\" 0\n");
		fprintf (hdrfd, "\"Device\" n\n");
		fprintf (hdrfd, "\"Xfer Actual KBps\" y\n");
		fprintf (hdrfd, "\"Xfer Effective KBps\" y\n");
		fprintf (hdrfd, "\"Entries in BAB\" y\n");
		fprintf (hdrfd, "\"Sectors in BAB\" y\n");
		fprintf (hdrfd, "\"Percent Done\" y\n");
		fprintf (hdrfd, "\"Percent BAB Full\" y\n");
		fprintf (hdrfd, "\"Driver Mode\" 0\n");
		fprintf (hdrfd, "\"Read KBps\" y\n");
		fprintf (hdrfd, "\"Write KBps\" y\n");
	} else {
		fprintf (hdrfd, "\"ts\" x\n");
		fprintf (hdrfd, "\"dummy1\" 0\n");
		fprintf (hdrfd, "\"Device\" n\n");
		fprintf (hdrfd, "\"Xfer Actual KBps\" y\n");
		fprintf (hdrfd, "\"Xfer Effective KBps\" y\n");
		fprintf (hdrfd, "\"Entries per sec recvd\" y\n");
		fprintf (hdrfd, "\"Entry age recvd\" y\n");
		fprintf (hdrfd, "\"Entry age in journal\" y\n");
		fflush (hdrfd);
	}
	(void)fclose(hdrfd);
	
	if ((lgp->statp->statfd = fopen(fil, "w")) == NULL) {
		reporterr (ERRFAC, M_FILE, ERRCRIT, fil, ftd_strerror());
		return -1;
	}

	error_tracef( TRACEINF, "ftd_stat_init_file():opened stats performance file %s", fil );

/*
I don't think this is used
#if defined(HPUX)
	sprintf (procname, "%05d", (int) getpid());
	if ((procfd = open(procname, O_RDONLY)) == -1) {
		reporterr (ERRFAC, M_FILE, ERRCRIT, procname, ftd_strerror());
	}
#endif
*/

	return 0;
}
/*
 * ftd_stat_dump_file --
 * save lg stats to stat file
 */
int
ftd_stat_dump_file(ftd_lg_t *lgp)
{
	time_t			statts;
	struct tm		*tim;
	ftd_dev_cfg_t	*devcfgp;
	ftd_dev_t		**devpp, *devp;
	char			fil[MAXPATHLEN], fil2[MAXPATHLEN], perfname[MAXPATHLEN];
	double			pctbab;
	int				deltasec;

	if (lgp->cfgp->role == ROLEPRIMARY) {
		(void)time(&statts);
		tim = localtime(&statts);

		if (lgp->statp && lgp->statp->statfd == (FILE*)NULL) {
			return;
		}
		fprintf(lgp->statp->statfd, "%02d:%02d:%02d ",
			tim->tm_hour, tim->tm_min, tim->tm_sec);

		ForEachLLElement(lgp->devlist, devpp) {
			devp = *devpp;
			if (devp->statp->actualkbps <= 0.0) {
				devp->statp->actualkbps = 0.0;
			}
			if (devp->statp->effectkbps <= 0.0) {
				devp->statp->effectkbps = 0.0;
			}
			if (devp->statp->entries < 0) {
				devp->statp->entries = 0;
			}
			if (devp->statp->sectors < 0) {
				devp->statp->sectors = 0;
			}
#if defined(HPUX)
			if (isnanf(devp->statp->pctdone)) {
#elif defined(_AIX) || defined(SOLARIS)
			if (finite(devp->statp->pctdone) == 0) {
#else
			//if (IsNANorINF(devp->statp->pctdone)) {
#endif
				devp->statp->pctdone = 0.0;
			}
			if (devp->statp->pctdone <= 0.0) {
				devp->statp->pctdone = 0.0;
			}
			if (devp->statp->pctdone >100.0) {
				devp->statp->pctdone = 0.0;
			}
			pctbab = (double)devp->statp->pctbab;
#if defined(HPUX)
			if (isnanf(pctbab)) {
#elif defined(_AIX) || defined(SOLARIS)
			if (finite(pctbab) == 0) {
#else
			//if (IsNANorINF(pctbab)) {
#endif
				pctbab = 0.0;
			}
			if (pctbab < 0.0) {
				pctbab = 0.0;
			}
			if (pctbab > 100.0) {
				pctbab = 100.0;
			}

			if (devp->statp->local_kbps_read <= 0.0) {
				devp->statp->local_kbps_read = 0.0;
			}
			if (devp->statp->local_kbps_written <= 0.0) {
				devp->statp->local_kbps_written = 0.0;
			}

			if ((devcfgp =
				ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL)
			{
				reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
				return -1;
			}

			fprintf(lgp->statp->statfd,
				" || %s %6.2f %6.2f %d %d %6.2f %6.2f %d %6.2f %6.2f",
				devcfgp->devname,
				devp->statp->actualkbps,
				devp->statp->effectkbps,
				devp->statp->entries,
				devp->statp->sectors,
				devp->statp->pctdone,
				pctbab,
				lgp->statp->drvmode,
				devp->statp->local_kbps_read,
				devp->statp->local_kbps_written);
        
			fflush(lgp->statp->statfd);
		}
		fprintf(lgp->statp->statfd, "\n");
		fflush(lgp->statp->statfd);

		if (lgp->tunables->maxstatfilesize < ftell(lgp->statp->statfd)) {
			(void)fclose(lgp->statp->statfd);
			sprintf(perfname, "%s/%s%03d.prf",
				PATH_RUN_FILES, PRIMARY_CFG_PREFIX, lgp->lgnum);
			sprintf(fil, "%s", perfname);
			sprintf(fil2, "%s.1", perfname);
			(void)unlink(fil2);
			(void)rename(fil, fil2);
			if ((lgp->statp->statfd = fopen(fil, "w")) == NULL) {
				reporterr (ERRFAC, M_FILE, ERRCRIT, fil, ftd_strerror());
			}
		}
	} else {
		(void)time(&statts);
		deltasec = statts - lgp->statp->statts;

		if (deltasec < 10) {
			return;
		}
		tim = localtime(&statts);

		if (lgp->statp->statfd == (FILE*)NULL) {
			return -1;
		}
		fprintf(lgp->statp->statfd, "%02d:%02d:%02d ",
			tim->tm_hour, tim->tm_min, tim->tm_sec);

		ForEachLLElement(lgp->devlist, devpp) {
			devp = *devpp;
			devp->statp->actualkbps =
				((devp->statp->actual * 1.0) / (deltasec * 1.0)) / 1024.0;
			devp->statp->effectkbps =
				((devp->statp->effective * 1.0) / (deltasec * 1.0)) / 1024.0;
			devp->statp->entries =
				((devp->statp->entries * 1.0) / deltasec);

		    if ((devcfgp =
				ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL)
			{
				reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
				return -1;
			}
			fprintf(lgp->statp->statfd, " || %s %6.2f %6.2f %6.2f %d %d",
				devcfgp->sdevname,
				devp->statp->actualkbps,
				devp->statp->effectkbps,
				devp->statp->entries,
				devp->statp->entage,
				devp->statp->jrnentage);

			fflush(lgp->statp->statfd);

			devp->statp->actual = 0;
			devp->statp->effective = 0;
			devp->statp->entries = 0;
		}
		fprintf(lgp->statp->statfd, "\n");
		fflush(lgp->statp->statfd);

		//if (lgp->tunables->maxstatfilesize < ftell(lgp->statp->statfd)) {
		if (65536 < ftell(lgp->statp->statfd)) {
			(void)fclose(lgp->statp->statfd);
			sprintf(perfname, "%s/%s%03d.prf",
				PATH_RUN_FILES, SECONDARY_CFG_PREFIX, lgp->lgnum);
			sprintf(fil, "%s", perfname);
			sprintf(fil2, "%s.1", perfname);
			(void)unlink(fil2);
			(void)rename(fil, fil2);

			if ((lgp->statp->statfd = fopen(fil, "w")) == NULL) {
                reporterr (ERRFAC, M_FILE, ERRCRIT, fil, ftd_strerror());
            }
        }
		lgp->statp->statts = statts;
	}

	return 0;
}

#endif /* !_WINDOWS */
/*
 * ftd_stat_init_driver --
 * initialize lg stat in driver
 */
int
ftd_stat_init_driver(ftd_lg_t *lgp)
{
	ftd_dev_t		**devpp, *devp;
	ftd_dev_stat_t	*statp;
	char			devbuf[FTD_PS_DEVICE_ATTR_SIZE];
    int				rc, tconnection, tdevid;

    memset(devbuf, 0, sizeof(devbuf));

	ForEachLLElement(lgp->devlist, devpp) {
		
		devp = *devpp;
		
		statp = (ftd_dev_stat_t*)devbuf;
		
		// retain devid, connection in driver 
		statp->connection = tconnection = devp->statp->connection;
		statp->devid = tdevid = devp->ftdnum;
		rc = ftd_ioctl_set_dev_state_buffer(lgp->ctlfd, lgp->lgnum,
			devp->ftdnum, sizeof(devbuf), devbuf, 0);
		if (rc != 0) {
			return rc;
		}
		
		memset(devp->statp, 0, sizeof(ftd_dev_stat_t));
		
		// retain devid, connection in memory
		devp->statp->connection = tconnection;
		devp->statp->devid = tdevid;
	}

	return 0;
}

/*
 * ftd_stat_connection_driver --
 * initialize lg stat in driver
 */
int
ftd_stat_connection_driver(HANDLE ctlfd, int lgnum, int state)
{
	char			devbuf[FTD_PS_DEVICE_ATTR_SIZE];
    int				rc;
	ftd_stat_t		lgstat;
	disk_stats_t	*devtemp, *devstat;

	if (ctlfd == INVALID_HANDLE_VALUE)
		return -1;

	if (ftd_ioctl_get_group_stats(ctlfd, lgnum, &lgstat, 1) < 0) {
		return -1;
	}

	devstat = malloc((lgstat.ndevs + 1) * sizeof(disk_stats_t));
	if (devstat == NULL)
		return -1;

    memset(devstat, -1, (lgstat.ndevs + 1) * sizeof(disk_stats_t));
    if (ftd_ioctl_get_dev_stats(ctlfd, lgnum, -1, devstat, lgstat.ndevs + 1) < 0)
	{
        free(devstat);
		return -1;
	}
	
    devtemp = devstat;
    while(devtemp->localbdisk != -1) {
		ftd_dev_stat_t	*statp;

		rc = ftd_ioctl_get_dev_state_buffer(ctlfd, lgnum, devtemp->localbdisk, 
			sizeof(devbuf), devbuf, 0);
		if (rc != 0) {
			free(devstat);
			return rc;
		}

		statp = (ftd_dev_stat_t*)devbuf;
#if defined(_WINDOWS)
		statp->connection = state;
#endif

		rc = ftd_ioctl_set_dev_state_buffer(ctlfd, lgnum, devtemp->localbdisk, 
			sizeof(devbuf), devbuf, 0);
		if (rc != 0) {
			free(devstat);
			return rc;
		}

		devtemp++;
	}

	free(devstat);

	return 0;
}

/*
 * ftd_stat_set_group_connect --
 * set group connection state in memory and driver
 */
int
ftd_stat_set_connect(ftd_lg_t *lgp, int state)
{
	ftd_dev_t	**devpp;
	int rc;

	if ((rc = ftd_stat_connection_driver(lgp->ctlfd, lgp->lgnum, state)) < 0) {
		return rc;
	}

	ForEachLLElement(lgp->devlist, devpp) {
		if ((*devpp) && (*devpp)->statp) {
			(*devpp)->statp->connection = state;
		}
	}

	return 0;
}

/*
 * ftd_stat_dump_driver --
 * save group device stats to driver
 */
int
ftd_stat_dump_driver(ftd_lg_t *lgp)
{
// SAUMYA_FIX 
// we don't need this anymore. Driver will be updating this structure from now on
// as it has all the information it needs
#if 0
	char			devbuf[FTD_PS_DEVICE_ATTR_SIZE];
	ftd_dev_t		**devpp, *devp;
	ftd_dev_stat_t *devstatp, *statp;
	int				rc;

	ForEachLLElement(lgp->devlist, devpp) {
		
		devp = *devpp;
		
		rc = ftd_ioctl_get_dev_state_buffer(lgp->ctlfd,
			lgp->lgnum, devp->ftdnum, sizeof(devbuf), devbuf, 0);
		if (rc != 0) {
			return rc;
		}
		
		devstatp = (ftd_dev_stat_t*)devbuf;
		
		statp = devp->statp;

		// add stats to stats returned from driver 
		devstatp->devid = statp->devid;
		devstatp->actual += statp->actual;
		devstatp->effective += statp->effective;
		devstatp->entries += statp->entries;

		devstatp->rsyncoff = statp->rsyncoff;
		devstatp->rsyncdelta = statp->rsyncdelta;

#if 0 //ifdef TDMF_TRACE
		fprintf(stderr,"\n*** devstatp->devid = %d\n",devstatp->devid);
		fprintf(stderr,"\n*** devstatp->actual = %d\n",devstatp->actual);
		fprintf(stderr,"\n*** devstatp->effective = %d\n",
			devstatp->effective);
		fprintf(stderr,"\n*** devstatp->entries = %d\n",devstatp->entries);
		fprintf(stderr,"\n*** devstatp->rsyncdelta = %d\n",
			devstatp->rsyncdelta);
		fprintf(stderr,"\n*** devstatp->rsyncoff = %d\n",
			devstatp->rsyncoff);
#endif
		rc = ftd_ioctl_set_dev_state_buffer(lgp->ctlfd, lgp->lgnum,
			devp->ftdnum, sizeof(devbuf), devbuf, 0);
		if (rc != 0) {
			return -1;
		}
		
		// reset internal counters 
		
		statp->actual = 0;
		statp->effective = 0;
		statp->entries = 0;
	}
#endif


	return 0;
}

#if !defined(_WINDOWS)

#if !defined(SECTORSIZE)
#if defined(SOLARIS) || defined(_AIX)
#define SECTORSIZE 512
#elif defined(HPUX)
#define SECTORSIZE 1024
#endif
#endif /* !defined(SECTORSIZE) */

int
ftd_lg_get_dev_stats(ftd_lg_t *lgp, int deltasec)
{
	ftd_dev_t		**devpp, *devp;
	ftd_dev_cfg_t	*devcfgp;
	ftd_stat_t		lgstat;
	disk_stats_t	devstat;
	char			procfile[MAXPATHLEN] = "", procname[MAXPATHLEN] =  "";
	long			totalsects;
	char			devbuffer[FTD_PS_DEVICE_ATTR_SIZE];
	ftd_dev_stat_t	*statp;
	HANDLE			procfd;
	struct stat		sb;
	int				pcnt;
	pid_t			pmdpid;
#if defined(SOLARIS)
	struct prpsinfo		ps;
#elif defined(HPUX)
	struct pst_status	ps;
#endif
	double			dev_ptotread_sectors, dev_ptotwrite_sectors;

	if (ftd_ioctl_get_group_stats(lgp->ctlfd,
		lgp->lgnum, &lgstat, 1) < 0) {
		return 0;
	}

	lgp->statp->pctbab = (int) ((((double)lgstat.bab_used) * 100.0) / 
		(((double)lgstat.bab_used) + ((double)lgstat.bab_free)));
	lgp->statp->entries = lgstat.wlentries;
	lgp->statp->sectors = lgstat.bab_used / SECTORSIZE;

	sprintf(procname, "PMD_%03d", lgp->lgnum);
   
	// figure out whether pmd is running and get pid here
	if ((pmdpid = ftd_proc_get_pid(procname, 1, &pcnt)) <= 0) {
		lgp->statp->pmdup = FALSE;
		lgp->statp->pid = -1;
	} else {
		lgp->statp->pmdup = TRUE;
		lgp->statp->pid = pmdpid;

#if defined(SOLARIS)
		sprintf(procfile,"/proc/%05d", (int) pmdpid);
		if ((procfd = open(procfile, O_RDONLY))==-1) {
#ifdef DEBUG_THROTTLE
			fprintf(stderr, "Couldn't open proc file: %s",procfile);
#endif /* DEBUG_THROTTLE */
			lgp->statp->percentcpu = 0;
			close(procfd);
		} else {
			(void)ioctl(procfd, PIOCPSINFO, (char*)&ps);
			lgp->statp->percentcpu =
				(int)((((ps.pr_pctcpu && 0x0000ffff) * 1.0) / 327.68) + 0.5);
			close(procfd);
		}
#elif defined(HPUX)
		if (pstat_getproc(&ps, sizeof(ps), 0, pmdpid) == -1) {
#ifdef DEBUG_THOTTLE
			fprintf(stderr, "pstat_getproc failed for pid %d\n",pmdpid);
#endif /* DEBUG_THROTTLE */
			lgp->statp->percentcpu=0;
		} else {
			lgp->statp->percentcpu =
				(int)((((ps.pst_pctcpu && 0x0000ffff) * 1.0) / 327.68) + 0.5);
        }
#elif defined(_AIX)
{
	/*- 
	 * _AIX doesn't hand you the equivalent of %CPU. 
	 * so...compute it from various times...
	 */
	int ncpu;
	int pctcpu;
	unsigned long now;
	unsigned long ttime;
	unsigned long etime;
	struct procsinfo pb;

	pid_t pididx=pmdpid;

	struct fdsinfo *fb = (struct fdsinfo *) 0;

	if(getprocs(&pb, sizeof(pb), fb, 0, &pididx, 1) ==  1) {
		now = (unsigned long)time(0);
		ncpu = sysconf(_SC_NPROCESSORS_CONF);
		ttime = now - pb.pi_start;
		etime = pb.pi_utime + pb.pi_stime;
		pctcpu = (100 * etime) / ttime;
		pctcpu = pctcpu / ncpu;
		lgp->statp->percentcpu = pctcpu;
	}
	else {
		/* getprocs(3) bummer */
		lgp->statp->percentcpu = 0;
	}
}
#endif
    }
    /* We'll calculate the lg % done by the lowest device %, so init to 100 */
    lgp->statp->pctdone = 100;
    lgp->statp->actualkbps = 0;
    lgp->statp->effectkbps = 0;
    lgp->statp->local_kbps_read = 0.0;
    lgp->statp->local_kbps_written = 0.0;

    /* get stats for each device */
	ForEachLLElement(lgp->devlist, devpp) {

		devp = *devpp;

		if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
			continue;
		}
		if (ftd_ioctl_get_dev_state_buffer(lgp->ctlfd,
			lgp->lgnum, devp->ftdnum, sizeof(devbuffer), devbuffer, 1) < 0)
		{
			return 0; 
		}
		// stats from pmd are in statp
		statp = (ftd_dev_stat_t*)devbuffer;

		if (ftd_ioctl_get_dev_stats(lgp->ctlfd,
			lgp->lgnum, devp->ftdnum, &devstat, 1) < 0)
		{
			return 0; 
		}
		// calculate dev stats

		// device reads/writes --> sum to group reads/writes
		dev_ptotread_sectors = devp->statp->ctotread_sectors;
		dev_ptotwrite_sectors = devp->statp->ctotwrite_sectors;

		devp->statp->ctotread_sectors = devstat.sectorsread;
		devp->statp->ctotwrite_sectors = devstat.sectorswritten;

		devp->statp->local_kbps_read =
			((((double)(int)(devp->statp->ctotread_sectors -
				dev_ptotread_sectors)) *
					(double)DEV_BSIZE) / 1024.0) /
						(double)lgp->tunables->statinterval;

		devp->statp->local_kbps_written =
			((((double)(int)(devp->statp->ctotwrite_sectors -
				dev_ptotwrite_sectors)) *
					(double)DEV_BSIZE) / 1024.0) /
						(double)lgp->tunables->statinterval;

		lgp->statp->local_kbps_read += devp->statp->local_kbps_read;
		lgp->statp->local_kbps_written += devp->statp->local_kbps_written;

		//other per device statistics from driver
		devp->statp->sectors = devstat.wlsectors;
		devp->statp->entries = devstat.wlentries;

		devp->statp->pctbab =
			(devstat.wlsectors * SECTORSIZE * 100.0) /
			((lgstat.bab_used + lgstat.bab_free) * 1.0);
		devp->statp->actualkbps =
			((statp->actual * 1.0) / (deltasec * 1.0)) / 1024.0;
		devp->statp->effectkbps =
			((statp->effective * 1.0) / (deltasec * 1.0)) / 1024.0;

		// calculate percent done 
		totalsects = (long)devp->devsize;

		// reset the pmds devstat info since it has been read
		devp->statp->pctdone =
			((statp->rsyncoff * 1.0) / (totalsects * 1.0)) * 100.0;

        statp->actual = statp->effective = 0;
		if (ftd_ioctl_set_dev_state_buffer(lgp->ctlfd,
			lgp->lgnum, devp->ftdnum, sizeof(devbuffer), devbuffer, 0) < 0)
		{
			return 0; 
		}
		// lg % done should be equal to the lowest device percentage
		if (devp->statp->pctdone < lgp->statp->pctdone) {
			lgp->statp->pctdone = devp->statp->pctdone;
		}
		lgp->statp->actualkbps =
			lgp->statp->actualkbps + devp->statp->actualkbps;
		lgp->statp->effectkbps =
			lgp->statp->effectkbps + devp->statp->effectkbps;
	}

	return 1;
}

#endif /* !_WINDOWS */
