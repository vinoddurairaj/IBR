/*
 * ftd_devlock.c - FTD device lock interface
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
#include "ftd_devlock.h"
#include "llist.h"

typedef struct ftd_dev_lock_s {
	char devname[4];
	int	lgnum;
	int	disksize;
	HANDLE fd;
	int refcount;
} ftd_dev_lock_t;


static LList *locklist;
static CRITICAL_SECTION locklistSec;
static void ftd_dev_unlock_all(void);

int
ftd_dev_lock_create(void)
{
	if ( (locklist = CreateLList(sizeof(ftd_dev_lock_t**))) == NULL)
		return -1;

	InitializeCriticalSection(&locklistSec);

	return 0;
}

void
ftd_dev_lock_delete(void)
{
	if (locklist) {
		ftd_dev_unlock_all();

		FreeLList(locklist);

		locklist = NULL;
	}
	
	DeleteCriticalSection(&locklistSec);
}

int
ftd_dev_locked_disksize(HANDLE fd)
{
	ftd_dev_lock_t	**lockpp;
	int				size = -1;

	EnterCriticalSection(&locklistSec);

	ForEachLLElement(locklist, lockpp) {
		if ((*lockpp)->fd == fd) {
			size = (*lockpp)->disksize;
		}
	}

	LeaveCriticalSection(&locklistSec);

	return size;
}

HANDLE
ftd_dev_lock(char *devname, int lgnum)
{
	ftd_dev_lock_t	**lockpp, *lockp;
	HANDLE			fd;
	int				size;
	int				devlock_retry = 3; //rddev 021213

	EnterCriticalSection(&locklistSec);

	ForEachLLElement(locklist, lockpp) {
		if (!strcmp((*lockpp)->devname, devname)) {
			if ((*lockpp)->lgnum != lgnum) {
				LeaveCriticalSection(&locklistSec);

			    SetLastError (ERROR_DRIVE_LOCKED);
			
				return INVALID_HANDLE_VALUE;
			}

			(*lockpp)->refcount++;

			fd = (*lockpp)->fd;
			LeaveCriticalSection(&locklistSec);

			return fd;
		}
	}

	size = (int)disksize(devname);

	do 
	{
	fd = LockAVolume(devname);
	}
	while((fd == INVALID_HANDLE_VALUE) && (devlock_retry--));

	if (fd == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&locklistSec);
	
		return fd;
	}

	lockp = (ftd_dev_lock_t*) calloc(1, sizeof(ftd_dev_lock_t));
	if (lockp == NULL)
	{
		if (MountVolume(fd))
			CloseHandle(fd);

		LeaveCriticalSection(&locklistSec);

	    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
		
		return INVALID_HANDLE_VALUE;
	}

	lockp->fd = fd;
	lockp->lgnum = lgnum;
	lockp->disksize = size;
	strcpy(lockp->devname, devname);
	lockp->refcount = 1;

	AddToTailLL(locklist, &lockp);

	LeaveCriticalSection(&locklistSec);

	return fd;
}

int
ftd_dev_unlock(HANDLE fd)
{
	ftd_dev_lock_t	**lockpp;
	int				count = 0;

	EnterCriticalSection(&locklistSec);

	ForEachLLElement(locklist, lockpp) {
		if ((*lockpp)->fd == fd) {
			(*lockpp)->refcount--;

			count = (*lockpp)->refcount;
			if (count == 0)
			{
				RemCurrOfLL(locklist, lockpp);

				if ( !DismountVolume((*lockpp)->fd) ||
					!MountVolume((*lockpp)->fd) ) {
					free((*lockpp));
	
					LeaveCriticalSection(&locklistSec);

					return -1;
				}

				free((*lockpp));

				break;
			}
		}
	}

	LeaveCriticalSection(&locklistSec);

	return count;
}

static void
ftd_dev_unlock_all(void)
{
	ftd_dev_lock_t	**lockpp;

	ForEachLLElement(locklist, lockpp) {
		while(ftd_dev_unlock((*lockpp)->fd));
			
		CloseHandle((*lockpp)->fd);
	}
}

/*
 * DTurrin - Oct 26th, 2001
 *
 * ftd_dev_sync --
 * This file sinchronizes a device by flushing the
 * file HANDLE buffer.
 */
int
ftd_dev_sync(char *devname, int lgnum)
{
	ftd_dev_lock_t	**lockpp;

	EnterCriticalSection(&locklistSec);

	ForEachLLElement(locklist, lockpp)
	{
       // Find the correct file HANDLE for the Volume
		if (strcmp((*lockpp)->devname, devname) == 0)
		{
		    if ( !FlushFileBuffers((*lockpp)->fd) )
		    {
		        // Unable to Flush the file buffer
		    	LeaveCriticalSection(&locklistSec);
	    		return 0;
			}

			break;
		}
	}

	LeaveCriticalSection(&locklistSec);

	return 1;
}

