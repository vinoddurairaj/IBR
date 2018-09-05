/* #ident "@(#)$Id: ftd_mngt_get_all_devices.c,v 1.28 2003/11/13 02:48:21 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_GET_ALL_DEVICES_C_
#define _FTD_MNGT_GET_ALL_DEVICES_C_

#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/param.h>
#include <dirent.h>
#include <limits.h>

#if defined(SOLARIS)
#include <sys/dkio.h>
#include <sys/vtoc.h>
#elif defined(HPUX)
#include <sys/diskio.h>
#include <mntent.h>
#elif defined(_AIX)
#include <sys/lvdd.h>           /* LV_INFO ioctl usage */
#include <lvm.h>                /* LV_INFO ioctl usage */
#elif defined(linux)
#include <linux/fs.h>
#endif

#define STRUCT_INCR	10
#define VECTOR_SZ_INCR 	STRUCT_INCR * sizeof(mmp_TdmfDeviceInfo)

#if defined(HPUX) || defined(linux)
typedef	unsigned long long u_longlong_t;
#endif

#if defined(linux)
/* to make a part list  of active devices*/
struct part{
	long lpartsize;
	char pname[80];
};

struct part *p, *startp;
long maxpart;
static int
setvtocinfo(mmp_TdmfDeviceInfo **, int *, unsigned int *, char *);
#endif


#if defined(SOLARIS)
static int
setvtocinfo(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
    int                   fd;
    daddr_t               size;
    struct dk_cinfo       dkinfo;
    struct vtoc           dkvtoc;
    unsigned long         sectsiz; 
    u_longlong_t          tsize;
    u_longlong_t          tstart;
    char                  cmdline[256];
    char                  fstype[8];
    mmp_TdmfDeviceInfo    *pWk;
    mmp_TdmfDeviceInfo    *tmpp;
    FILE                  *f;

    memset(&dkinfo, 0, sizeof(dkinfo));
    memset(&dkvtoc, 0, sizeof(dkvtoc));

    if ((fd = open(fn, O_RDWR)) == -1) {
        sprintf(logmsg, "%s open failed. errno = %d\n", fn, errno);
        logout(11, F_setvtocinfo, logmsg);
        return -1;
    }
    if(FTD_IOCTL_CALL(fd, DKIOCINFO, &dkinfo) < 0) {
        logoutx(4, F_setvtocinfo, "ioctl DKIOCINFO failed", "errno", errno);
        close(fd);
        return -1;
    }
    /* Get the size of all partitions */
    if (FTD_IOCTL_CALL(fd, DKIOCGVTOC, &dkvtoc) < 0) {
        logoutx(4, F_setvtocinfo, "ioctl DKIOCGVTOC failed", "errno", errno);
        close(fd);
        return -1;
    }
    close(fd);

    if (dkvtoc.v_sectorsz > 0) {
        sectsiz = dkvtoc.v_sectorsz;
    } else {
        sectsiz = DEV_BSIZE;
    }
    tsize = (u_longlong_t) dkvtoc.v_part[dkinfo.dki_partition].p_size *
        (u_longlong_t) sectsiz;

	if (tsize <= 0) return -1;

    /* if output vector full, stretch it. */	
    if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfo) == *piVectorSzBytes) {
	tmpp = *ppDevicesVector;
        *ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
	if (*ppDevicesVector == NULL) {
            logoutx(19, F_setvtocinfo, "realloc failed", "errno", errno);
	    *ppDevicesVector = tmpp;
	    *piVectorSzBytes -= VECTOR_SZ_INCR;
            return -1;
	}
    }
    pWk = (*ppDevicesVector) + (*piNbrDevices);

    tstart = (u_longlong_t) dkvtoc.v_part[dkinfo.dki_partition].p_start *
        (u_longlong_t) sectsiz;
    lltoax(pWk->szStartOffset, tstart);
    lltoax(pWk->szLength, tsize);
    strcpy(pWk->szDrivePath, fn);
    pWk->sFileSystem = MMP_MNGT_FS_IGNORE;

    (*piNbrDevices)++;
    return 0;
}

#elif defined(HPUX)

/*
 * Return size of disk in DEV_BSIZE (1024) byte blocks.
 */
static int
setvtocinfo(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
    int			  fd;
    capacity_type	  cap;
    unsigned long	  sectors;
    /* u_longlong_t	  tsize; */
    unsigned long long    tsize;
    mmp_TdmfDeviceInfo    *pWk;
    mmp_TdmfDeviceInfo    *tmpp;

    if ((fd = open(fn, O_RDWR)) == -1) {
        return -1;
    }

    if (FTD_IOCTL_CALL(fd, DIOC_CAPACITY, &cap) < 0) {
        return -1;
    }
    close(fd);

    sectors = (unsigned long)(cap.lba);
    tsize = (u_longlong_t)sectors * (u_longlong_t)DEV_BSIZE;

	if (tsize <= 0) return -1;

    /* if output vector full, stretch it. */	
    if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfo) == *piVectorSzBytes) {
	tmpp = *ppDevicesVector;
        *ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
	if (*ppDevicesVector == NULL) {
            logoutx(4, F_setvtocinfo, "realloc failed", "errno", errno);
	    *ppDevicesVector = tmpp;
	    *piVectorSzBytes -= VECTOR_SZ_INCR;
            return -1;
	}
    }
    pWk = (*ppDevicesVector) + (*piNbrDevices);

    lltoax(pWk->szLength, tsize);
    strcpy(pWk->szDrivePath, fn);
    strcpy(pWk->szStartOffset, "0");
    pWk->sFileSystem = MMP_MNGT_FS_IGNORE;

    (*piNbrDevices)++;
    return 0;
}

#elif defined(_AIX)

/* return DEV_BSIZE byte blocks of an lv */
static int
setvtocinfo(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
	int             fd;
	int             iocerr;
	daddr_t         size;
	u_longlong_t	tsize;
	int             bsiz_ratio;
	struct lv_info  lvi;
	mmp_TdmfDeviceInfo    *pWk;
	mmp_TdmfDeviceInfo    *tmpp;

	if ((fd = open(fn, O_RDWR)) == -1) {
		return (-1);
        }

	/* get logical volume info */
	iocerr = FTD_IOCTL_CALL(fd, LV_INFO, &lvi);
	if (iocerr)
		return (-1);

	/* lv_info.num_blocks is in 512b units, chk for scale */
	if (DEV_BSIZE != 512) {
		bsiz_ratio = DEV_BSIZE / 512;
		size = lvi.num_blocks / bsiz_ratio;
	} else {
		size = lvi.num_blocks;
	}
	close(fd);

	tsize = (u_longlong_t)size * (u_longlong_t)DEV_BSIZE;

	if (tsize <= 0) return -1;

	/* if output vector full, stretch it. */	
	if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfo) == *piVectorSzBytes) {
		tmpp = *ppDevicesVector;
		*ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
		if (*ppDevicesVector == NULL) {
			logoutx(4, F_setvtocinfo, "realloc failed", "errno", errno);
			*ppDevicesVector = tmpp;
			*piVectorSzBytes -= VECTOR_SZ_INCR;
			return -1;
		}
	}
	pWk = (*ppDevicesVector) + (*piNbrDevices);

	lltoax(pWk->szLength, tsize);
	strcpy(pWk->szDrivePath, fn);
	strcpy(pWk->szStartOffset, "0");
	pWk->sFileSystem = MMP_MNGT_FS_IGNORE;

	(*piNbrDevices)++;
	return (0);
}

/* lookup lv, pv properties (informational, currently not used) */
static int
lvprop(int fd)
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
	iocerr = FTD_IOCTL_CALL(fd, LV_INFO, &lvi);
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

#elif defined(linux)

static int
setvtocinfo(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
	int fd;
	int openvalue = 0;
	daddr_t          blksize,sectsize;
	u_longlong_t     tsize;
    mmp_TdmfDeviceInfo    *pWk;
    mmp_TdmfDeviceInfo    *tmpp;

#if 0
	/* get the number  open for a block device */
	openvalue = opened_num(fn);
	if (openvalue  > 0) return -1;	/* inuse */
	else if (openvalue < 0) return -1;
#endif

	if((fd = open(fn, O_RDWR)) == -1) {
		return -1;
	}
	/* Get #blocks for /dev/<device name> */
	if(FTD_IOCTL_CALL(fd, BLKGETSIZE, &blksize) < 0) {
		close(fd);
		return -1;
	}
	/* Get sector size in bytes for /dev/<device name> */
	/* I hope /dev/<device-name> should correspond to physical device*/
	if (FTD_IOCTL_CALL(fd, BLKSSZGET, &sectsize) < 0) {
		close(fd);
		return -1;
	}
	close(fd);

	if (sectsize <= 0) sectsize = DEV_BSIZE;
	tsize = (u_longlong_t)sectsize * (u_longlong_t) blksize;

	if (tsize <= 0) return -1;

	/* if output vector full, stretch it. */	
	if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfo) == *piVectorSzBytes) {
		tmpp = *ppDevicesVector;
		*ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
		if (*ppDevicesVector == NULL) {
			logoutx(4, F_setvtocinfo, "realloc failed", "errno", errno);
			*ppDevicesVector = tmpp;
			*piVectorSzBytes -= VECTOR_SZ_INCR;
			return -1;
		}
	}
    pWk = (*ppDevicesVector) + (*piNbrDevices);

	lltoax(pWk->szLength, tsize);
	strcpy(pWk->szDrivePath, fn);
	strcpy(pWk->szStartOffset, "0");
	pWk->sFileSystem = MMP_MNGT_FS_IGNORE;

    (*piNbrDevices)++;
    return 0;
}

#endif

/****************************************************************************
 * force_dsk_or_rdsk_path -- force a path to have either /rdsk or /dsk in it
 *                           and add it to the end if it isn't there
 ***************************************************************************/
#if defined(_AIX)
/*-
 * for AIX, force_dsk_or_rdsk() is functionally equivalent
 * to HPUX convert_lv_name(), with some additional paranoia. 
 *
 * the HPUX vers of convert_lv_name() tests whether a device
 * node name is S_ISCHR by parsing the name. 
 * for AIX, stat(2) the in-parameter device node name to make 
 * this determination. 
 * this will hopefully avoid confused behavior in cases where
 * device node names start with 'r'.
 */
void
force_dsk_or_rdsk(char *pathout, char *pathin, int to_char)
{
	char	*rdsk, *dsk;
	char	*src, *dest;
	int	len;
	int	is_char;
	struct	stat	sb;

	if (stat(pathin, &sb) == 0) {
		/*
		 * name exists, and node type is determined
		 */
		is_char = S_ISCHR(sb.st_mode) ? 1 : 0;
		if (is_char == 1) {
			if (to_char == 1) {	/* check for trivial case */
				strcpy(pathout, pathin);
				return;
			}
			/*
			 * if there is "/rdsk" characters, it is dtc device.
			 * if not, it is normal device on AIX. -- e.g. rlv00p
			 */
			if ((src = strstr(pathin, "/rdsk")) == NULL) {
				src = strrchr(pathin, '/');
			}
		} else {
			if (to_char == 0) {	/* check for trivial case */
				strcpy(pathout, pathin);
				return;
			}
			/*
			 * if there is "/dsk" characters, it is dtc device.
			 * if not, it is normal device on AIX. -- e.g. lv00p
			 */
			if ((src = strstr(pathin, "/dsk")) == NULL) {
				src = strrchr(pathin, '/');
			}
		}
	} else {
		/*
		 * name doesn't exist, thpe isn't known with certainly
		 * since it hasn't been created yet.
		 * just parse the name ala HP-UX...
		 */
		if ((src = strrchr(pathin, '/')) == NULL) {
			/* cannot happen -- maybe */
			strcpy(pathout, pathin);
			return;
		}
		rdsk = strstr(pathin, "/rdsk");
		dsk = strstr(pathin, "/dsk");
		if (rdsk || dsk) {
			if (rdsk) {
				src = rdsk;
				is_char = 1;
			} else {
				src = dsk;
				is_char = 0;
			}
		} else {
			is_char = (*(src + 1) == 'r') ? 1 : 0;
		}
		/* check for trivial cases */
		if ((is_char && to_char) || (!is_char && !to_char)) {
			strcpy(pathout, pathin);
			return;
		}
	}

	if (is_char) {
		/*
		 * conversion routine from CHR to BLK
		 */
		len = (src - pathin) + 1;
		strncpy(pathout, pathin, len);
		dest = pathout + len;
		src++;
		strcpy(dest, src + 1);
		return;
	} else {
		/*
		 * conversion routine from BLK to CHR
		 */
		len = (src - pathin) + 1;
		strncpy(pathout, pathin, len);
		dest = pathout + len;
		*dest++ = 'r';		/* add an 'r' */
		strcpy(dest, src + 1);
		return;
	}
}
#elif defined(linux)
void
force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag)
{
	strcpy(pathout, pathin);
}
#else /* defined(_AIX), defined(linux) */

static void
force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag)
{
    int i, len, foundflag;

    foundflag = 0;
    len = strlen(pathin);
    for (i=len-4; i>3; i--) {
        if (pathin[i] == '/') {
            if (0 == strncmp(&pathin[i+1], "rdsk/", 5)) {
                if (tordskflag) {
                    strcpy (pathout, pathin);
                } else {
                    strncpy (pathout, pathin, i+1);
                    strncpy (&pathout[i+1], "dsk/", 4);
                    strncpy (&pathout[i+5], &pathin[i+6], (len-i)-6);
                    pathout[len-1] = '\0';
                }
                foundflag = 1;
                break;
            }
            if (0 == strncmp(&pathin[i+1], "dsk/", 4)) {
                if (!tordskflag) {
                    strcpy (pathout, pathin);
                } else {
                    strncpy (pathout, pathin, i+1);
                    strncpy (&pathout[i+1], "rdsk/", 5);
                    strncpy (&pathout[i+6], &pathin[i+5], (len-i)-5);
                    pathout[len+1] = '\0';
                }
                foundflag = 1;
                break;
            }
        }
    }
    if (!foundflag) {
        strcpy (pathout, pathin);
        len--;
        while (pathout[len] == '/')
            len--;
        strcpy (&pathout[len+1], (tordskflag) ? "/rdsk": "/dsk");
    }
}
#endif /* !defined(_AIX) */

#if defined(HPUX)

#include <sys/diskio.h>
#include <sys/pstat.h>

/*****************************************************************
 * This procedure converts the block device path name
 * to it's corresponding raw device path name and vice-versa.
 *
 * - When to_char is equal to 0, pathin is the block device
 *   path name.
 * - When to_char is equal to 1, pathin is a raw device path
 *   name.
 *****************************************************************
 */

static void
convert_lv_name (char *pathout, char *pathin, int to_char)
{
    int len;
    char *src, *dest;

    if ((src = strrchr(pathin, '/')) != NULL) {
        len = (src - pathin) + 1;
        strncpy(pathout, pathin, len);
        dest = pathout + len;
        if (to_char) {
            *dest++ = 'r';
        } else { 
            src++; /* skip the 'r' */
        }
        strcpy(dest, src+1);
    } else { /* can't happen */
        strcpy(pathout, pathin);
    }
}

/*
 * Return TRUE if devname is a logical volume.
 */
static int 
is_logical_volume(char *devname)
{
    int         lvm_major;
    struct stat statbuf;

    if (stat(devname, &statbuf) != 0) {
        return 0;
    }

    /* this is hard coded for now, but we really should find it dynamically */
    lvm_major = 64;

    if ((statbuf.st_rdev >> 24) == lvm_major) {
        return 1;
    }
    return 0;
}

/*
 * Given a block device name, get the size of the device. The character
 * device must be inquired for this information, so we convert the name.
 */
static int
get_char_device_info(char *dev_name, int is_lv, unsigned long *sectors)
{
    capacity_type cap;
    disk_describe_type describe;
    char raw_name[MAXPATHLEN];
    int fd;

    if (is_lv) {
        convert_lv_name(raw_name, dev_name, 1);
    } else  {
        force_dsk_or_rdsk(raw_name, dev_name, 1);
    }

    if ((fd = open(raw_name, O_RDWR | O_EXCL)) == -1) { 
        return -1;
    }
    if (FTD_IOCTL_CALL(fd, DIOC_DESCRIBE, &describe) < 0) {
        close(fd);
        return -2;
    }
    /* if the device is a CD-ROM, blow it off */
    if (describe.dev_type == CDROM_DEV_TYPE) {
        close(fd);
        return -3;
    }
    if (FTD_IOCTL_CALL(fd, DIOC_CAPACITY, &cap) < 0) {
        close(fd);
        return -4;
    }

    *sectors = cap.lba;

    close(fd);
    return 0;
}

/****************************************************************************
 * dev_info -- return information about a specific character device
 * This is a bit different from the Solaris version. HPUX allows access to
 * the underlying disk devices owned by the logical volume manager. Logical
 * volumes can have any name underneath the /dev tree, as long as they are
 * unique, so we can't rely on any naming convention or location for logical 
 * volumes. We know the device is a logical volume because it's major number
 * matches the major number of the lvm driver (64). lvm uses minor numbers to
 * uniquely identify the logical volumes. The lvm driver always uses 64 as
 * it's major number as far as we know. The character device file for logical
 * volumes is stored in the same directory as the block device, but it is
 * prefaced with an 'r' (e.g. /dev/vg00/rlvol1 vs. /dev/vg00/lvol1).
 ***************************************************************************/
static int
dev_info(char *devname, int *inuseflag, u_long *sectors, char *mntpnt)
{
    struct stat statbuf;
    struct pst_lvinfo lv;
    struct pst_diskinfo disk;
    char block_name[MAXPATHLEN];
    unsigned int device_num, lvm_major;
    int idx, status;

    FILE* fd;
    struct mnttab mp;
    struct mnttab mpref;

    *inuseflag = -1;
    *sectors = 0;
    *mntpnt = 0;
    if (stat(devname, &statbuf) != 0) {
        return -1;
    }
    device_num = statbuf.st_rdev;

    /* this is hard coded for now, but we really should find it dynamically */
    lvm_major = 64;

    if ((device_num >> 24) == lvm_major) {
        if (!S_ISBLK(statbuf.st_mode)) {
            convert_lv_name(block_name, devname, 0);
        } else {
            strcpy(block_name, devname);
        }
        status = get_char_device_info(block_name, 1, sectors);
        if (status < 0) {
            return status;
        }
        *inuseflag = 0; 

        /* search all logical volumes for a matching minor number */
        idx = 0;
        while ((status = pstat_getlv(&lv, sizeof(lv), 1, idx)) > 0) {
            if (lv.psl_dev.psd_minor == (device_num & 0xffffff)) {
                *inuseflag = 1; 
                break;
            }
            idx++;
        }
    } else {
        /* force the device name to be a block device name */
        if (!S_ISBLK(statbuf.st_mode)) {
            force_dsk_or_rdsk(block_name, devname, 0);

            /* we need the device number for the block device */
            if (stat(block_name, &statbuf) != 0) {
                return -1;
            }
            device_num = statbuf.st_rdev;
        } else {
            strcpy(block_name, devname);
        }
        /* hideous bug found in HPUX: the disk table is sometimes bogus and
           an entry may be missing. Assume it's in use just to be safe. */
        *inuseflag = -1;

        status = get_char_device_info(block_name, 0, sectors);
        if (status < 0) {
            return status;
        }

        /* search the disk devices for a matching device number */
        idx = 0;
        while ((status = pstat_getdisk(&disk, sizeof(disk), 1, idx)) > 0) {
            if ((disk.psd_dev.psd_minor == (device_num & 0xffffff)) &&
                (disk.psd_dev.psd_major == (device_num >> 24))) {
                *inuseflag = disk.psd_status;
                break;
            }
            idx++;
        }
    }
    /* check the mount table for the mount directory, if its in use */
    *mntpnt = 0;
    if (*inuseflag) {
		if ((FILE*)NULL != (fd = setmntent(MNT_MNTTAB, "rt"))) {
			mpref.mnt_mountp = (char*)NULL;
			mpref.mnt_fstype = (char*)NULL;
			mpref.mnt_mntopts = (char*)NULL;
			mpref.mnt_time = (char*)NULL;
			if (0 == getmntany (fd, &mp, &mpref)) {
				strcpy (mntpnt, mpref.mnt_mountp);
			}
			endmntent(fd) ;
		}
    }
    return 0;
}

/****************************************************************************
 * capture_logical_volumes -- return a buffer of all device information for 
 *                     all logical volumes
 ***************************************************************************/
static int
capture_logical_volumes(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
{
    FILE *f;
    int status;
    char command[] = "/etc/vgdisplay -v";
    char tempbuf[MAXPATHLEN];
    char raw_name[MAXPATHLEN];
    char mountpath[MAXPATHLEN];
    char		  *ptr, *end;
    u_long		  sectors;
    /* u_longlong_t	  size; */
    unsigned long long	  size;
    mmp_TdmfDeviceInfo    *pWk;
    mmp_TdmfDeviceInfo    *tmpp;
    u_longlong_t          tsize;
    int			  inuseflag;
    char		  *mntpnt;

    mntpnt = mountpath;
    if ((f = popen(command, "r")) != NULL) {
        while (fgets(tempbuf, MAXPATHLEN, f) != NULL) {
            if ((ptr = strstr(tempbuf, "LV Name")) != NULL) {
                ptr += 8;
                while (*ptr && isspace(*ptr)) ptr++;
                end = ptr;
                while (*end && !isspace(*end)) end++;
                *end = 0;
                convert_lv_name (raw_name, ptr, 1);

		if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfo) == *piVectorSzBytes) {
			tmpp = *ppDevicesVector;
			*ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
			if (*ppDevicesVector == NULL) {
				logoutx(4, F_capture_logical_volumes, "realloc failed", "errno", errno);
				*ppDevicesVector = tmpp;
				*piVectorSzBytes -= VECTOR_SZ_INCR;
				pclose(f);
				return -1;
			}
		}
		pWk = (*ppDevicesVector) + (*piNbrDevices);

                /* -- get sundry information about the device */
                if (0 <= dev_info (raw_name, &inuseflag, &sectors, mntpnt)) {
                    size = ((u_longlong_t)sectors * (u_longlong_t)DEV_BSIZE);
                } else {
                    size = (u_longlong_t)0;
                }
		lltoax(pWk->szLength, size);
		strcpy(pWk->szDrivePath, raw_name);
		strcpy(pWk->szStartOffset, "0");
		pWk->sFileSystem = MMP_MNGT_FS_UNKNOWN;
    		(*piNbrDevices)++;
            }
        }
        status = pclose(f);
    } else {
		logoutx(4, F_capture_logical_volumes, "popen failed", "errno", errno);
	}
    return 0;
}

#endif /* HPUX */

#if defined(_AIX)
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sysconfig.h>

/*-
 * for AIX, things are, well, *real* different.
 * 
 * for one, all process access to physical DAD 
 * storage is through the logical volume driver. 
 * this simplifies the implementation of the
 * AIX vers of this module. however...
 *
 * the disk driver node naming convention bears 
 * no semblance to that for SOLARIS or HPUX.
 * here, rather than rely on pattern matching
 * device node names, liblvm subroutines are used 
 * to enumerate all of the lv's available to the
 * system. 
 * 
 * here, interesting properties for each found lv 
 * are determined, and output as with other vers.
 *
 */

/* format of output string */
#define OUTFMTSTR "{%s %d %.2f %s / %lu SECT %s} "

/*-
 * get the address of the logical volume
 * driver ddconfig entry point. this is
 * used with liblbm subr's.
 */
static mid_t
hd_config_addr()
{
	struct cfg_load cfg_ld;
	char            hddrvnm[64];

	memset(&cfg_ld, 0, sizeof(struct cfg_load));

	cfg_ld.kmid = 0;
	cfg_ld.path = "/etc/drivers/hd_pin";
	cfg_ld.libpath = NULL;

	if (sysconfig(SYS_QUERYLOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
		return (0);
	}
	return (cfg_ld.kmid);
}

/*-
 * use liblvm subr's to enumerate all available 
 * lv's configured in the system. for each found 
 * lv: determine some interesting properties,
 * format and write output to the descriptor
 * given by the fd parm.
 */
static char            bdevice[128];
static char            cdevice[128];

static int
queryvgs(mid_t addr, mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
{
	struct queryvgs *qvgs;
	struct queryvg  *qvgp;
	struct querylv  *qlv;
	long            num_vgs;
	long            num_lvs;
	int             lvmerr;
	int             vgndx;
	int             lvndx;
	unsigned long long csize;	/* for over 2GB volume */
	unsigned long   sectcnt;
	mmp_TdmfDeviceInfo    *pWk;
	mmp_TdmfDeviceInfo    *tmpp;
	u_longlong_t          tsize;

	
	/* query for all available volume groups */
	lvmerr = lvm_queryvgs(&qvgs, addr);
	if (lvmerr)
		return (lvmerr);
	num_vgs = qvgs->num_vgs;

	/* enumerate each volume group */
	for (vgndx = 0; vgndx < num_vgs; vgndx++) {

		lvmerr = lvm_queryvg(&qvgs->vgs[vgndx].vg_id, &qvgp, NULL);
		if (lvmerr)
			return (lvmerr);
		num_lvs = qvgp->num_lvs;

		/* enumerate each logical volume */
		for (lvndx = 0; lvndx < num_lvs; lvndx++) {

			lvmerr = lvm_querylv(&qvgp->lvs[lvndx].lv_id,
					     &qlv, (char *) NULL);
			if (lvmerr)
				return (lvmerr);

			/*
			 * presume the logical volume was created with 
			 * a default path name, that it lives in "/dev/".
			 */
			/* sprintf(bdevice, "/dev/%s", qlv->lvname); */
			sprintf(cdevice, "/dev/r%s", qlv->lvname);

    			/* if output vector full, stretch it. */	
			if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfo) == *piVectorSzBytes) {
				tmpp = *ppDevicesVector;
				*ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
				if (*ppDevicesVector == NULL) {
					logoutx(4, F_queryvgs, "realloc failed", "errno", errno);
					*ppDevicesVector = tmpp;
					*piVectorSzBytes -= VECTOR_SZ_INCR;
					return -1;
				}
			}
			pWk = (*ppDevicesVector) + (*piNbrDevices);

			/*
			 * compute sizes of things, format and write output
			 */
			csize = (unsigned long long)qlv->currentsize * (2 << (qlv->ppsize - 1));
			sectcnt = (csize >> DEV_BSHIFT);
    			tsize = sectcnt * (u_longlong_t)DEV_BSIZE;
			lltoax(pWk->szLength, tsize);
			strcpy(pWk->szDrivePath, cdevice);
			strcpy(pWk->szStartOffset, "0");
			pWk->sFileSystem = MMP_MNGT_FS_UNKNOWN;
    			(*piNbrDevices)++;
		}
	}
}

static void
enum_lvs(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
{
	queryvgs(hd_config_addr(), ppDevicesVector, piNbrDevices, piVectorSzBytes);
}
#endif  /* defined (_AIX) */

#if defined(linux)
/* execute while exiting configtool */
void close_part_list() {
	if (startp != NULL){
		free(startp);
		startp = NULL;
	}
}   

/* If the inquired string of device name
is present the global , returns 1 else 0
*/
int
ismatchinlist( char *inpstr ){
	int match;
	long l;
	struct part *p;

	/* check for match in the list */
	p = startp;
	match=0;

	for (l=1; l<maxpart; l++) {
		if (strcmp(p->pname, inpstr) == 0) {
			match = 1;
			break;
		}
		p++;
	}
	return match;
}

/* Make a list of Active devices
Execute while start of configtool or
Refresh of Device list
*/
int
make_part_list( ) {
	FILE *fh;
	char tmp[80], size[80], name[80];
	int ret;
	int i;
	long l;
	struct part *p;

	fh = fopen("/proc/partitions", "r+");
	if (fh == NULL)
	{
		sprintf(logmsg, "%s\n", strerror(errno));
		logout(4, F_make_part_list, logmsg);
		return -1;
	}

	/* count total partitions exists */
	do {
		ret = fscanf( fh, "%s %s %s %s",
				&tmp, &tmp,  &size, &name);

		i=11;
		while (i--)
			ret = fscanf( fh, "%s", &tmp);
		maxpart++;
	} while ((ret > 0) && ( ret != EOF ));

	/* allocate mem to list parttions and its size */
	startp = (struct part*) malloc(sizeof(*startp) * maxpart);
	if ((startp) == NULL){
		logoutx(4, F_make_part_list, "Error", "errno", errno);
		return -1;
	}
	p = startp;
	rewind(fh);

	/* make list of partitions */
	for (l=0; l < maxpart; l++) {
		ret = fscanf( fh, "%s %s %s %s",
					&tmp, &tmp,  &size, &name);
		i=11;
		/* ignore 11 infos for block devices */
		while (i--)
			ret = fscanf( fh, "%s", &tmp);

		p->lpartsize = atol(size);
		strcpy(p->pname, name);
		p++;
	}
 
	/* display the list- just for check */
	/* p = startp;
	   for (l=1; l<maxpart; l++, p++) 
	   printf("%s %ld\n", p->pname, p->lpartsize);
	*/ 
   
	fclose(fh);
 
	return 0;
}

/* function to identify the number of opens for agiven fine name. 
   Return 0   -> no fails opened
          +ve -> number of open times
          -1  -> unable to get contrl device
*/
int opened_num(char *dname)
{
	int fh;
	struct stat bstatbuf;
	ftd_open_num_t ftd_open_num;

	fh = open( FTD_CTLDEV, O_RDWR);
	if( fh == -1 ){
		logoutx(4, F_opened_num, "Error opening FTD_CTLDEV", "errno", errno);
		return -1;
	}

	stat (dname, &bstatbuf);
	ftd_open_num.dno = bstatbuf.st_rdev;
	if( FTD_IOCTL_CALL(fh, FTD_OPEN_NUM_INFO, &ftd_open_num)) {
		close(fh);
		return -1;
	}
	close(fh);
	return (ftd_open_num.open_num);
}
#endif /* partition active devices Linux */

/****************************************************************************
 * capture_devs_in_dir -- reports on all devices in the given directories
 ***************************************************************************/
static void
capture_devs_in_dir(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *cdevpath, char *bdevpath)
{
    char cdevice[PATH_MAX];
    char bdevice[PATH_MAX];
    DIR *dfd;
    struct dirent *dent;
    struct stat cstatbuf;
    struct stat bstatbuf;
    FILE* f;

#if defined(linux)
	if (((DIR*)NULL) == (dfd = opendir (bdevpath))) return;
#else
    if (((DIR*)NULL) == (dfd = opendir (cdevpath))) return;
#endif

    while (NULL != (dent = readdir(dfd))) {
        if (strcmp(dent->d_name, ".") == 0) continue;
        if (strcmp(dent->d_name, "..") == 0) continue;
#if defined(linux)
		if ((strcmp(bdevpath,"/dev") == 0) &&
				(!(strncmp(dent->d_name, QNM, strlen(QNM)) == 0))){
			if (startp) {
				if (!ismatchinlist(dent->d_name))
					continue;
			}
		}

#endif /* linux DK */


#if !defined(linux)
        sprintf (cdevice, "%s/%s", cdevpath, dent->d_name);
#endif
        sprintf (bdevice, "%s/%s", bdevpath, dent->d_name);

#if !defined(linux)
        if (0 != stat (cdevice, &cstatbuf)) continue;
        if (!(S_ISCHR(cstatbuf.st_mode))) continue;
#endif
        if (0 != stat (bdevice, &bstatbuf)) continue;
        if (!(S_ISBLK(bstatbuf.st_mode))) continue;

#if defined(linux)
        if (setvtocinfo(ppDevicesVector, piNbrDevices, piVectorSzBytes, bdevice) == -1) {
#else
        if (setvtocinfo(ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice) == -1) {
#endif
            continue;
		}
    }
    (void) closedir (dfd);
}

/****************************************************************************
 * walk_rdsk_subdirs_for_devs -- recursively walks a directory tree that
 *                               already has rdsk in it for their devices
 ***************************************************************************/
static void
walk_rdsk_subdirs_for_devs(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *rootdir)
{
    DIR* dfd;
    DIR* cdevfd;
    DIR* bdevfd;
    struct dirent* dent;
    char cdevdirpath[PATH_MAX];
    char bdevdirpath[PATH_MAX];

    if (((DIR*)NULL) == (dfd = opendir (rootdir))) return;
    while (NULL != (dent = readdir(dfd))) {
        if (0 == strcmp(dent->d_name, ".")) continue;
        if (0 == strcmp(dent->d_name, "..")) continue;
        sprintf (cdevdirpath, "%s/%s", rootdir, dent->d_name);
        force_dsk_or_rdsk (bdevdirpath, cdevdirpath, 0);
        /* -- open the character device directory */
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath))) continue;
        (void) closedir (cdevfd);
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath))) continue;
        (void) closedir (bdevfd);
		capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevdirpath, bdevdirpath);
        walk_rdsk_subdirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevdirpath);
    }
    (void) closedir (dfd);
}

/****************************************************************************
 * walk_dirs_for_devs -- walks a directory tree, seeing if rdsk/dsk 
 *                          subdirectories exist, and report on their
 *                          devices
 ***************************************************************************/
static void
walk_dirs_for_devs(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *rootdir)
{
    DIR* dfd;
    DIR* cdevfd;
    DIR* bdevfd;
    struct dirent* dent;
    char cdevdirpath[PATH_MAX];
    char bdevdirpath[PATH_MAX];

    if (((DIR*)NULL) == (dfd = opendir (rootdir))) return;
    while (NULL != (dent = readdir(dfd))) {
        if (0 == strcmp(dent->d_name, ".")) continue;
        if (0 == strcmp(dent->d_name, "..")) continue;
        /* -- create character and block device directory paths */
#if !defined(linux)
        if (0 == strcmp(dent->d_name, "rdsk")) continue;
#endif
        if (0 == strcmp(dent->d_name, "dsk")) continue;
#if !defined(linux)
        sprintf (cdevdirpath, "%s/%s/rdsk", rootdir, dent->d_name);
#endif
        sprintf (bdevdirpath, "%s/%s/dsk", rootdir, dent->d_name);
        /* -- open the character device directory */
#if !defined(linux)
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath))) continue;
        (void) closedir (cdevfd);
#endif
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath))) continue;
        (void) closedir (bdevfd);
		capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevdirpath, bdevdirpath);
    }
    (void) closedir (dfd);
}

/****************************************************************************
 * capture_all_devs -- write out all devices found on a system to
 *                     the specified output device
 ***************************************************************************/
static void
capture_all_devs(mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
{
    DIR* dfd;
#if defined(linux)
	char cdevice[PATH_MAX];
	char bdevice[PATH_MAX];
	char devpath[PATH_MAX];
	char tmppath[PATH_MAX];
	char configpath[FILE_PATH_LEN];
	char command[FILE_PATH_LEN];
	struct dirent* dent;
	FILE *cfgfd;
	int  lvmfd;
	int  len;
#endif
#if defined(SOLARIS)

    /* -- get the local disk devices */
    capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/rdsk", "/dev/dsk");

    /* -- get the VxVM disk group volumes */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
        (void) closedir (dfd);
        if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/rdsk"))) {
            (void) closedir (dfd);
	    walk_rdsk_subdirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/vx/rdsk");
        }
    }

    /* -- get the datastar devices -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/" QNM);
    }
  
    /* -- get the SDS disk set devices */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/md"))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/md");
    }
  
    /* -- get the non-diskset SDS devices */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/md/rdsk"))) {
        (void) closedir (dfd);
	capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/md/rdsk", "/dev/md/dsk");
    }

    /* -- get the GDS(FUJITSU Global Disk Service) disk set devices */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/sfdsk"))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/sfdsk");
    }
 
#endif /* SOLARIS */
  
#if defined(HPUX)
    (void) capture_logical_volumes (ppDevicesVector, piNbrDevices, piVectorSzBytes);
    /* -- get the datastar devices -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/" QNM);
    }
#endif /* HPUX */

#if defined(_AIX)
    (void) enum_lvs (ppDevicesVector, piNbrDevices, piVectorSzBytes);
    /* -- get the datastar devices here -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/" QNM);
    }
#endif /* defined(_AIX) */
	
#if defined(linux)
	if (startp == NULL) {
		if (make_part_list() == -1) {
			logout(7, F_capture_devs_in_dir, "Timeout is not effective.\n");
		}
	}
	/* -- get the IDE/SCSI/RAID devices -- */
	capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev", "/dev");
	/* -- get the LVM devices -- */
	if (((DIR*)NULL) != (dfd = opendir ("/etc/lvmtab.d"))) {
		while (NULL != (dent = readdir(dfd))) {
			if (strcmp(dent->d_name, ".") == 0) continue;
			if (strcmp(dent->d_name, "..") == 0) continue;
			/* create LVM path */
			sprintf(cdevice, "/dev/%s", dent->d_name);
			strcpy(bdevice,cdevice);
			capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice, bdevice);
			memset(bdevice, 0x00, PATH_MAX);
			memset(cdevice, 0x00, PATH_MAX);
		}
		(void) closedir (dfd);
	}
	/* -- get the user config devices -- */
	sprintf(configpath, "%s/devlist.conf", PATH_CONFIG);
	if ( (cfgfd = fopen(configpath, "r")) != NULL ){
		fclose(cfgfd);
		sprintf(command, "/bin/cat %s | /bin/sed \"s/\\/$//\" | /bin/sort | /usr/bin/uniq", configpath);
		if ((cfgfd = popen(command,"r")) != NULL)
		{
			while ( fgets(devpath, PATH_MAX, cfgfd) != NULL) {
				if ( devpath[0] != '/') continue;
				len = strlen(devpath);
				while (devpath[len-1] == '\n' || devpath[len-1] == ' ' || devpath[len-1] == '\t'){
					devpath[len-1] = 0;
					len--;
				}
				/* specified path check */
				if ((len == 4) && (strcmp(devpath,"/dev") == 0))
				{
					continue;
				}

				if ((len >= 8) && (strncmp(devpath,"/dev/dtc",8) == 0))
				{
					continue;
				}

				if ((len != 4) && (strncmp(devpath,"/dev",4) == 0))
				{
					/* convert path for VG check */
					strcpy(tmppath,"/etc/lvmtab.d");
					strcat(tmppath,&(devpath[4]));
					if ((lvmfd = open (tmppath,O_RDONLY)) != -1) {
						(void) close (lvmfd);
						continue;
					}
				}

				if (((DIR*)NULL) != (dfd = opendir (devpath))) {
					(void) closedir (dfd);
					strcpy(bdevice, devpath);
					strcpy(cdevice, devpath);
					capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice, bdevice);
				}
			}
			pclose(cfgfd);
		}
	}
	/* -- get the datastar devices here -- */
	if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
		(void) closedir (dfd);
		walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/" QNM);
	}
	close_part_list();
#endif /* defined(linux) */

    return;
}

/*
 * Manages MMP_MNGT_GET_ALL_DEVICES request
 */
static int
ftd_mngt_get_all_devices(sock_t *sockID)
{
    mmp_mngt_TdmfAgentDevicesMsg_t  msg;
    mmp_TdmfDeviceInfo              *pDevicesVector = NULL;
    int                             iNbrDevices = 0;
    unsigned int		    iVectorSzBytes = 0;
    int                             r, toread, towrite;
    char                            *pWk;

    pWk = (char *)&msg;
    pWk += sizeof(mmp_mngt_header_t);

    /*
     * at this point, mmp_mngt_header_t header is read.
     * now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure
     * don't care about it, just empty socket to be able to response
     */
    toread = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t) - sizeof(mmp_mngt_header_t);

    r = ftd_sock_recv(sockID, (char *)pWk, toread);
    if (r != toread) {
        return -1;
    }
    /* no data to convert from network byte order to host byte order */

    capture_all_devs(&pDevicesVector, &iNbrDevices, &iVectorSzBytes);

    /**************************/
    /* build response message */
    /**************************/
    msg.hdr.magicnumber	= MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngttype	= MMP_MNGT_SET_ALL_DEVICES;
    msg.hdr.sendertype	= SENDERTYPE_TDMF_AGENT;
    msg.hdr.mngtstatus	= MMP_MNGT_STATUS_OK;
    /* convert to network byte order before sending on socket */
    mmp_convert_mngt_hdr_hton(&msg.hdr);

    msg.szServerUID[0]	= 0; /* don't care */
    msg.iNbrDevices	= htonl(iNbrDevices);

    /* convert to network byte order before sending on socket */
    for (r = 0; r < iNbrDevices; r++) {
        mmp_convert_TdmfAgentDeviceInfo_hton(pDevicesVector + r);
    }

    /* 
     * respond using same socket. 
     *  first send mmp_mngt_TdmfAgentDevicesMsg_t than send pDevicesVector
     */
    towrite = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t);
    r = ftd_sock_send(sockID, (char*)&msg, towrite);
    if (r == towrite) {
        towrite = iNbrDevices * sizeof(mmp_TdmfDeviceInfo);
        r = ftd_sock_send(sockID, (char *)pDevicesVector, towrite);
        if (r != towrite) {
    	    if (pDevicesVector != 0) {
		free(pDevicesVector);
	    }
            return -1;
	}
    } else {
	if (pDevicesVector != 0) {
		free(pDevicesVector);
	}
        return -1;
    }
    /* cleanup */
    if (pDevicesVector != 0) {
        free(pDevicesVector);
    }
    return 0;
}
#endif /* _FTD_MNGT_GET_ALL_DEVICES_C_ */
