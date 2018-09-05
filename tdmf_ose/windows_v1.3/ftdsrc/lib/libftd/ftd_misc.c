/*
 * ftd_misc.c -- FTD miscellaneous routines
 *
 * (c) Copyright 1999 Legato Systems, Inc. All Rights Reserved
 *
 */

#if defined(SOLARIS)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <unistd.h>

daddr_t fdisksize(HANDLE fd)
{
    daddr_t          size;
    struct dk_cinfo  dkinfo;
    struct vtoc      dkvtoc;

#ifdef PURE
    /* Yow!  We find a pure bug with the following IOCTLs. */
    size = 0;
    memset(&dkinfo, 0, sizeof(dkinfo));
    memset(&dkvtoc, 0, sizeof(dkvtoc));
#endif
    if(ioctl(fd, DKIOCINFO, &dkinfo) < 0) {
        return -1;
    }

    /* Get the size of all partitions */
    if (ioctl(fd, DKIOCGVTOC, &dkvtoc) < 0) {
        return -1;
    }
    size = dkvtoc.v_part[dkinfo.dki_partition].p_size;
    return size;
}
#elif defined(HPUX)

#include <sys/types.h>
#include <sys/diskio.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Return size of disk in DEV_BSIZE (1024) byte blocks.
 */
daddr_t fdisksize(HANDLE fd)
{
    capacity_type cap;

    if (ioctl(fd, DIOC_CAPACITY, &cap) < 0) {
        return -1;
    }
    return (cap.lba);
}
#elif defined(_AIX)

#include <fcntl.h>		/* open(2) usage */
#include <sys/types.h>		/* daddr_t usage */
#include <sys/lvdd.h>		/* LV_INFO ioctl usage */
#include <lvm.h>		/* LV_INFO ioctl usage */
#include "aixcmn.h"

/* return DEV_BSIZE byte blocks of an lv */
daddr_t
fdisksize(HANDLE fd)
{
	int             iocerr;
	daddr_t         size;
	int             bsiz_ratio;
	struct lv_info  lvi;

	/* get logical volume info */
	iocerr = ioctl(fd, LV_INFO, &lvi);
	if (iocerr)
		return (-1);

	/* lv_info.num_blocks is in 512b units, chk for scale */
	if (DEV_BSIZE != 512) {
		bsiz_ratio = DEV_BSIZE / 512;
		size = lvi.num_blocks / bsiz_ratio;
	} else
		size = lvi.num_blocks;

	return (size);
}

/* lookup lv, pv properties (informational, currently not used) */
int
lvprop(HANDLE fd)
{
	struct lv_info  lvi;
	int             iocerr;
	int             lvmqerr;
	struct queryvg *qvgp;
	struct querylv *qlvp;
	struct querypv *qpvp;
	struct lv_id    lvid;
	long            num_lvs;
	long            num_pvs;
	struct lv_array *lvap;
	struct pv_array *pvap;
	int             cur_lv;
	int             cur_pv;
	int             size;

	/* get logical volume info */
	iocerr = ioctl(fd, LV_INFO, &lvi);
	if (iocerr)
		return (-1);

	/* query volume group */
	lvmqerr = lvm_queryvg(&lvi.vg_id, &qvgp, (char *) NULL);
	if (lvmqerr)
		return (-1);

	/* rumble through lv's */
	num_lvs = qvgp->num_lvs;
	lvap = &qvgp->lvs[0];
	for (cur_lv = 0; cur_lv < num_lvs; cur_lv++) {
		/* query this lv */
		lvmqerr = lvm_querylv(&lvap->lv_id, &qlvp, (char *) NULL);
		lvap = &qvgp->lvs[cur_lv];
	}

	/* rumble through pv's */
	size = 0;
	num_pvs = qvgp->num_pvs;
	pvap = &qvgp->pvs[0];
	for (cur_pv = 0; cur_pv < num_pvs; cur_pv++) {
		/* query this pv */
		lvmqerr = lvm_querypv(&lvi.vg_id, &pvap->pv_id, &qpvp, (char *) NULL);
		/* accumulate size, likely different than returned by lvinfo */
		size += qpvp->alloc_ppcount * (2 << (qpvp->ppsize - 1));
		pvap = &qvgp->pvs[cur_pv];
	}

	return (0);
}

#elif defined(_WINDOWS)

#include "ftdPort.h"

//// Mike Pollett need for windows .h files
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif

#include <winioctl.h>

int ftd_get_dev_bsize(HANDLE fd)
{
	DISK_GEOMETRY geo;
	DWORD dwBytesReturned=0;
	int ByteShiftToSector = 9, BytesPerSector = 512;

	if (!DeviceIoControl(fd,
				IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
				&geo, sizeof(DISK_GEOMETRY), &dwBytesReturned, 
				(LPOVERLAPPED) NULL )) {
		return -1;
	}

	BytesPerSector = geo.BytesPerSector;

    switch ( BytesPerSector ) {

        case 128: {

            ByteShiftToSector = 7;
            break;
        }

        case 256: {

            ByteShiftToSector = 8;
            break;
        }

        case 512: {

            ByteShiftToSector = 9;
            break;
        }

        case 1024: {

            ByteShiftToSector = 10;
            break;
        }

        default: {

            ByteShiftToSector = 9;
            break;
        }

    }

    return ByteShiftToSector;
}

/*
 * Return size of disk in DEV_BSIZE byte blocks.
 */
long fdisksize(HANDLE fd)
{
	PARTITION_INFORMATION ppi;
	DWORD dwBytesReturned=0;
    int ftd_dev_bsize;

	if (!DeviceIoControl(fd,
				IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
				&ppi, sizeof(PARTITION_INFORMATION), &dwBytesReturned, 
				(LPOVERLAPPED) NULL )) {
		return -1;
	}

	if ( (ftd_dev_bsize = ftd_get_dev_bsize(fd)) == -1)
        return -1;

	return (ppi.PartitionLength.QuadPart >> ftd_dev_bsize);
}

daddr_t disksize(char *fn)
{
    long	    size;
	HANDLE		fd;
	DWORD		err;
	char        szVolumeName[8];       

	sprintf(szVolumeName, TEXT(fn), fn[0]);
	fd = CreateFile(   szVolumeName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,                               0,
		NULL );

	if( fd == INVALID_HANDLE_VALUE )	{
		err = GetLastError();

		return -1;
	}

    size = fdisksize(fd);

    CloseHandle(fd);
    
	return size;
}

#endif

#if !defined(_WINDOWS)

daddr_t disksize(char *fn)
{
    daddr_t size;
    int     fd;

    if ((fd = open(fn, O_RDWR)) == -1) {
        return -1;
    }

    size = fdisksize(fd);
    close(fd);
    return size;
}
#endif /* !_WINDOWS */