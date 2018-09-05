/*
 * ftd_statd.c - ftd stat dumper daemon
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
#include "ftd_error.h"
#include "ftd_config.h"
#include "ftd_lg.h"
#include "ftd_dev.h"
#include "ftd_stat.h"
#include "ftd_ioctl.h"
#include "ftd_throt.h"
#include "ftd_ps.h"
#include "ftdio.h"

#define SECTORSIZE 512

#define ROUNDL( d ) ((long)((d) + ((d) > 0 ? 0.5 : -0.5)))

/*
 * StatThread - primary statistics thread
 */
DWORD 
StatThread(LPDWORD param)
{
	HANDLE			ctlfd = INVALID_HANDLE_VALUE;
	ftd_perf_t	    *perfp = NULL;
	ftd_perf_instance_t *pData, *lgdata, *devdata;
    BOOL			bFreeMutex;
	long			totalsects;
	HANDLE			hEvents[2], lgfd;
	ftd_stat_t		*lgstat = NULL;
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	char				event[_MAX_PATH];

	static int	firsttime = TRUE;

	ftd_proc_args_t *args = (ftd_proc_args_t *) param;
	proc_t *procp = args->procp;

	// Initialize a security descriptor and assign it a NULL 
	// discretionary ACL to allow unrestricted access. 
	// Assign the security descriptor to a file. 
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
		goto errexit;
	if (!SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE))
		goto errexit;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = FALSE;

	if (ftd_init_errfac(PRODUCTNAME, "statd", NULL, NULL, 1, 1) == NULL) {
		goto errexit;
	}
	
	while( (ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
		if (WaitForSingleObject(procp->hEvent, 10000) != WAIT_TIMEOUT)
			goto errexit;
	};

	if ((perfp = ftd_perf_create()) == NULL) {
		goto errexit;
	}

	if (ftd_perf_init(perfp, FALSE) ) {
		goto errexit;
	}

	if ((lgstat = malloc(33 * sizeof(ftd_stat_t))) == NULL) {
		goto errexit;
	}

	hEvents[0] = perfp->hGetEvent;
	hEvents[1] = procp->hEvent;

	// run forever dump stats
	while (1) {  
		ftd_stat_t *lgtemp;

		int rc = WaitForMultipleObjects(2, (CONST HANDLE *)hEvents, FALSE, INFINITE);

		if (rc != WAIT_OBJECT_0) {
			pData = perfp->pData;
			memset(pData, -1, SHARED_MEMORY_OBJECT_SIZE);
			goto errexit;
		}

		memset(lgstat, -1, 33 * sizeof(ftd_stat_t));
		
		if (ftd_ioctl_get_group_stats(ctlfd, -1, lgstat, 1) < 0) {
			goto errexit;
		}
		
		lgtemp = lgstat;
		while (lgtemp->lgnum != -1) {
			FTD_PERF_GROUP_GET_EVENT_NAME(event, lgtemp->lgnum);
			lgfd = OpenEvent(EVENT_ALL_ACCESS, FALSE, event);
			if (lgfd != NULL) {			
				SetEvent(lgfd);
				CloseHandle(lgfd);
			}

            lgtemp++;
		}

        // lock memory block
        if (perfp->hMutex != NULL) {
            if (WaitForSingleObject (perfp->hMutex, SHARED_MEMORY_MUTEX_TIMEOUT) == WAIT_FAILED) {
                // unable to obtain a lock
                bFreeMutex = FALSE;
            } else {
                bFreeMutex = TRUE;
            }
        } else {
            bFreeMutex = FALSE;
        }
		
		pData = perfp->pData;
		memset(pData, -1, SHARED_MEMORY_OBJECT_SIZE);

		lgtemp = lgstat;
		while (lgtemp->lgnum != -1) {
			disk_stats_t	*devtemp, *devstat;
					
			lgdata = pData++;
	
			memset(lgdata, 0, sizeof(lgdata));
			swprintf(lgdata->wcszInstanceName, L"p%03d", lgtemp->lgnum);

			lgdata->role = 'p';
			lgdata->drvmode = lgtemp->state;
			
			lgdata->lgnum = -100;
			lgdata->devid = lgtemp->lgnum;

			lgdata->pctdone = 100;
			lgdata->pctbab = ROUNDL((((double)lgtemp->bab_used) * 100.0) / 
				(((double)lgtemp->bab_used) + ((double)lgtemp->bab_free)));

			lgdata->entries = lgtemp->wlentries;
			lgdata->sectors = lgtemp->bab_used / SECTORSIZE;
			
			devstat = malloc((lgtemp->ndevs + 1) * sizeof(disk_stats_t));
			if (devstat == NULL)
				goto errexit;

            memset(devstat, -1, (lgtemp->ndevs + 1) * sizeof(disk_stats_t));
            if (ftd_ioctl_get_dev_stats(ctlfd, lgtemp->lgnum, -1, devstat, lgtemp->ndevs + 1) < 0) {
                free(devstat);
				goto errexit;
			}

            devtemp = devstat;
            while(devtemp->localbdisk != (dev_t)-1) {
				char devbuffer[FTD_PS_DEVICE_ATTR_SIZE];
				ftd_dev_stat_t	*statp;

				devdata = pData++;

				if (ftd_ioctl_get_dev_state_buffer(ctlfd,
					lgtemp->lgnum, devtemp->localbdisk, sizeof(devbuffer), devbuffer, 1) < 0)
					goto errexit;

				// stats from pmd are in statp
				statp = (ftd_dev_stat_t*)devbuffer;
#ifdef TDMF_TRACE
				fprintf(stderr,"\n*** lgstat->lgnum = %d\n",lgstat->lgnum);
				fprintf(stderr,"\n*** statp->devid = %d\n",statp->devid);
				fprintf(stderr,"\n*** statp->actual = %I64i\n",statp->actual);
				fprintf(stderr,"\n*** statp->effective = %I64i\n", statp->effective);
				fprintf(stderr,"\n*** statp->entries = %d\n",statp->entries);
				fprintf(stderr,"\n*** statp->rsyncdelta = %d\n",
					statp->rsyncdelta);
				fprintf(stderr,"\n*** statp->rsyncoff = %d\n",
					statp->rsyncoff);
#endif
				lgdata->connection = statp->connection;
				devdata->connection = statp->connection;

				swprintf(devdata->wcszInstanceName, L"p%03d ==> %S",
					lgtemp->lgnum, devtemp->devname + strlen(devtemp->devname) - 2);

				devdata->role = ' ';
				devdata->drvmode = lgtemp->state;

				//devdata->devid = devtemp->localbdisk;
				devdata->devid = statp->devid;
				devdata->lgnum = lgtemp->lgnum;

				// calculate percent done 
				totalsects = (long)devtemp->localdisksize;
				devdata->pctdone = ROUNDL(((statp->rsyncoff * 1.0) / (totalsects * 1.0)) * 100.0);

				devdata->pctbab = ROUNDL(((double)(devtemp->wlsectors * SECTORSIZE) * 100.0) /
					(((double)lgtemp->bab_used + (double)lgtemp->bab_free) * 1.0));
/*
				if (devdata->pctbab > 100)
					printf("wlsectors: %d, bab_used: %d, bab_free: %d\n",
						devtemp->wlsectors, lgtemp->bab_used, lgtemp->bab_free);
*/
				devdata->entries = devtemp->wlentries;
				devdata->sectors = devtemp->wlsectors;

				devdata->bytesread = devtemp->sectorsread * DEV_BSIZE;
				devdata->byteswritten = devtemp->sectorswritten * DEV_BSIZE;

				lgdata->bytesread += devtemp->sectorsread * DEV_BSIZE;
				lgdata->byteswritten += devtemp->sectorswritten * DEV_BSIZE;

				devdata->actual = statp->actual;
				devdata->effective = statp->effective;

				lgdata->actual += statp->actual;
				lgdata->effective += statp->effective;

				// lg % done should be equal to the lowest device percentage
				if (devdata->pctdone < lgdata->pctdone) {
					lgdata->pctdone = devdata->pctdone;
				}
            
				devtemp++;
            }

			free(devstat);

            lgtemp++;
        }
	    
		firsttime = FALSE;

		if (bFreeMutex) {
		    ReleaseMutex (perfp->hMutex);
		}

		SetEvent(perfp->hSetEvent);
	}

errexit:

	if (lgstat) {
		free(lgstat);
	}

    if (perfp) {
		ftd_perf_delete(perfp);
	}

	ftd_delete_errfac();

    return 0;
}