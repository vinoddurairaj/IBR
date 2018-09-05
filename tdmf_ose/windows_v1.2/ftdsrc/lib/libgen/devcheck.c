/***************************************************************************
 * devcheck.c - FullTime Data Remote Mirroring Daemon disk device information
 *              functions
 *
 * (c) Copyright 1997 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements device listing and device interogation from
 * the RMD (in an inetd-able form) for ftdconfigtool drop down lists
 * and device validation
 *
 * History:
 *   08/05/97 - Steve Wahl - original code
 *
 ***************************************************************************
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <strings.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include "devcheck.h"

#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h"
#else
#include <sys/mnttab.h>
#endif

extern daddr_t fdisksize (int fd);
extern daddr_t disksize (char* devicepath); 

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
	int             len;
	char           *src, *dest;
	struct stat     sb;
	int             is_char;

	/* whether name is a char device node */
	if (!stat(pathin, &sb))
		is_char = S_ISCHR(sb.st_mode) ? 1 : 0;
	else
		is_char = -1;

	if (is_char >= 0) {

		/*-
		 * name exists, and node type is determined
		 */

		/* check for trivial cases */
		if ((is_char && to_char) ||
		    (!is_char && !to_char)) {
			strcpy(pathout, pathin);
			return;
		}

		/*-
		 * special case.
		 * look for /dsk and /rdsk 
		 * 
		 */
		{
			char *rdsk;
			char *dsk;
			rdsk = strstr(pathin, "/rdsk");
			dsk = strstr(pathin, "/dsk");
			if( dsk || rdsk) {
				if ( rdsk )
					src = rdsk;
				else
					src = dsk;
			}
		}

		/* conversion cases */
		src = strrchr(pathin, '/');
		len = (src - pathin) + 1;
		strncpy(pathout, pathin, len);
		dest = pathout + len;
		if (to_char) 
			*dest++ = 'r'; /* add an 'r' */
		else 
			src++;	/* skip an 'r' */
		strcpy(dest, src + 1);

		return;

	} else {

		/*-
		 * name doesn't exist, type isn't known with 
		 * certainty since it hasn't been created yet.
		 * just parse the name ala HPUX...
		 */
		if ((src = strrchr(pathin, '/')) != NULL) {

			/*-
			 * special case.
			 * look for /dsk and /rdsk 
			 * 
			 */
			{
				char *rdsk;
				char *dsk;
				rdsk = strstr(pathin, "/rdsk");
				dsk = strstr(pathin, "/dsk");
				if( dsk || rdsk) {
					if ( rdsk )
						src = rdsk;
					else
						src = dsk;
				}
			}
      
			if (*(src + 1) == 'r' && to_char != 0) {
				/*
				 * -- if the device is already a raw device,
				 * do nothing 
				 */
				strcpy(pathout, pathin);
				return;
			}
			if (*(src + 1) != 'r' && to_char == 0) {
				/*
				 * -- the device is already not a raw device,
				 * do nothing 
				 */
				strcpy(pathout, pathin);
				return;
			}
			len = (src - pathin) + 1;
			strncpy(pathout, pathin, len);
			dest = pathout + len;
			if (to_char) {
				*dest++ = 'r';
			} else {
				src++;	/* skip the 'r' */
			}
			strcpy(dest, src + 1);

		} else {	/* can't happen */
			strcpy(pathout, pathin);
		}

		return;
	}
}
#else /* defined(_AIX) */
void
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
#include <sys/param.h>
#include <sys/pstat.h>

void
convert_lv_name (char *pathout, char *pathin, int to_char)
{
    int len;
    char *src, *dest;

    if ((src = strrchr(pathin, '/')) != NULL) {
        if (*(src+1) == 'r' && to_char != 0) {
            /* -- if the device is already a raw device, do nothing */
            strcpy (pathout, pathin);
            return;
        }
        if (*(src+1) != 'r' && to_char == 0) {
            /* -- the device is already not a raw device, do nothing */
            strcpy (pathout, pathin);
            return;
        }
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
int 
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
    if (ioctl(fd, DIOC_DESCRIBE, &describe) < 0) {
        close(fd);
        return -2;
    }
    /* if the device is a CD-ROM, blow it off */
    if (describe.dev_type == CDROM_DEV_TYPE) {
        close(fd);
        return -3;
    }
    if (ioctl(fd, DIOC_CAPACITY, &cap) < 0) {
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
int
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
        if ((FILE*)NULL != (fd = fopen ("/etc/mnttab", "r"))) 
        {
            mpref.mnt_special = block_name;
            mpref.mnt_mountp = (char*)NULL;
            mpref.mnt_fstype = (char*)NULL;
            mpref.mnt_mntopts = (char*)NULL;
            mpref.mnt_time = (char*)NULL;
            if (0 == getmntany (fd, &mp, &mpref)) {
                strcpy (mntpnt, mp.mnt_mountp);
            }
            fclose (fd);
        }
    }
    return 0;
}

/****************************************************************************
 * capture_logical_volumes -- return a buffer of all device information for 
 *                     all logical volumes
 ***************************************************************************/
int
capture_logical_volumes (int fd)
{
    FILE *f;
    int status;
    char command[] = "/etc/vgdisplay -v";
    char tempbuf[MAXPATHLEN];
    char raw_name[MAXPATHLEN];
    char *ptr, *end;
    int inuseflag;
    u_long sectors;
    char mntpnt[MAXPATHLEN];
    char outbuf[MAXPATHLEN];
    double size;

    if ((f = popen(command, "r")) != NULL) {
        while (fgets(tempbuf, MAXPATHLEN, f) != NULL) {
            if ((ptr = strstr(tempbuf, "LV Name")) != NULL) {
                ptr += 8;
                while (*ptr && isspace(*ptr)) ptr++;
                end = ptr;
                while (*end && !isspace(*end)) end++;
                *end = 0;
                convert_lv_name (raw_name, ptr, 1);
                /* -- get sundry information about the device */
                if (0 <= dev_info (raw_name, &inuseflag, &sectors, mntpnt)) {
                    size = ((double) sectors * (double) DEV_BSIZE) / 1024.0;
                    if (size >= 1.0 && size < 1024) {
                        sprintf (outbuf, "{%s %d %.2f KB / %lu SECT %s} ",
                                 raw_name, inuseflag, size, sectors, mntpnt);
                    }
                    size = size / 1024.0;
                    if (size >= 1.0 && size < 1024) {
                        sprintf (outbuf, "{%s %d %.2f MB / %lu SECT %s} ",
                                 raw_name, inuseflag, size, sectors, mntpnt);
                    }
                    size = size / 1024.0;
                    if (size >= 1.0 && size < 1024) {
                        sprintf (outbuf, "{%s %d %.2f GB / %lu SECT %s} ",
                                 raw_name, inuseflag, size, sectors, mntpnt);
                    }
                } else {
                    sprintf (outbuf, "{%s 0 0.00 MB / 0 SECT } ", raw_name);
                }
                write (fd, (void*) outbuf, strlen(outbuf));
            }
        }
        status = pclose(f);
    }
    return 0;
}

#endif /* HPUX */

#if defined(_AIX)
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <lvm.h>
#include <sys/types.h>
#include <sys/sysconfig.h>
#include "aixcmn.h"
#include "ftd_mnttab.h"

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
mid_t
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
static char            pbuf[1024];
static char            bdevice[128];
static char            cdevice[128];
static char            mntedon[1024];
int
queryvgs(int fd, mid_t addr)
{
	struct queryvgs *qvgs;
	struct queryvg *qvgp;
	struct querylv *qlv;
	struct lv_array *lva;
	long            num_vgs;
	long            num_lvs;
	int             lvmerr;
	int             vgndx;
	int             lvndx;
	long            csize;
	double          fsize;
	char           *units;
	int             inuseflag;
	unsigned long   sectcnt;
	int             tfd;
	struct mnttab   mp;
	struct mnttab   mpref;


	
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

			/*-
			 * presume the logical volume was created with 
			 * a default path name, that it lives in "/dev/".
			 */
			sprintf(bdevice, "/dev/%s", qlv->lvname);
			sprintf(cdevice, "/dev/r%s", qlv->lvname);

			/*-
			 * in the SOLARIS and HPUX vers, the following
			 * code is used to check whether some process
			 * is using the device directly. apparently there,
			 * if the device contains a mounted filesystem
			 * the open(2) fails, and in subsequent processing
			 * whether and where it is mounted is determined.
			 * well, for AIX, this open(2) always succeeds,
			 * regardless of whether the device is mounted.
			 * for AIX then, check whether the device is 
			 * mounted, regardless of what open(2) returns. 
			 * if so, then revise the notion of "in use"
			 * accordingly, presuming that the intended
			 * semantics of "in use" apply to devices mounted
			 * as filesystems...
			 */
			inuseflag = 0;
			if (-1 == (tfd = open(cdevice, O_RDWR | O_EXCL))) {
				inuseflag = 1;
			} else {
				(void) close(tfd);
			}
			if (-1 == (tfd = open(bdevice, O_RDWR | O_EXCL))) {
				inuseflag = 1;
			} else {
				(void) close(tfd);
			}

			/*-
			 * whether and where device contains mounted filesystem
			 */
			memset(&mp, 0, sizeof(mp));
			mpref.mnt_special = bdevice;
			mpref.mnt_mountp = (char *) NULL;
			mpref.mnt_fstype = (char *) NULL;
			mpref.mnt_mntopts = (char *) NULL;
			mpref.mnt_time = (char *) NULL;
			if ((0 == getmntany(0, &mp, &mpref)) &&
			    (NULL != mp.mnt_mountp) &&
			    (0 < strlen(mp.mnt_mountp))) {
				strcpy(mntedon, mp.mnt_mountp);
				inuseflag=1;
			}
			else
				mntedon[0] = 0;

			/*-
			 * compute sizes of things, format and write output
			 */
			csize = qlv->currentsize * (2 << (qlv->ppsize - 1));
			sectcnt = (csize >> DEV_BSHIFT);
			fsize = (double) csize;
			fsize = fsize / 1024.0;
			if (fsize >= 1.0 && fsize < 1024.0) {
				units = "KB";
				sprintf(&pbuf[0], OUTFMTSTR, cdevice,
				 inuseflag, fsize, units, sectcnt, mntedon);
			}
			fsize = fsize / 1024.0;
			if (fsize >= 1.0 && fsize < 1024.0) {
				units = "MB";
				sprintf(&pbuf[0], OUTFMTSTR, cdevice,
				 inuseflag, fsize, units, sectcnt, mntedon);
			}
			fsize = fsize / 1024.0;
			if (fsize >= 1.0) {
				units = "GB";
				sprintf(&pbuf[0], OUTFMTSTR, cdevice,
				 inuseflag, fsize, units, sectcnt, mntedon);
			}
			write(fd, &pbuf[0], strlen(&pbuf[0]));

		}
	}
}

void
enum_lvs(int fd)
{
	queryvgs(fd, hd_config_addr());
}
#endif  /* defined (_AIX) */

/****************************************************************************
 * capture_devs_in_dir -- reports on all devices in the given directories
 ***************************************************************************/
void
capture_devs_in_dir (int fd, char* cdevpath, char* bdevpath)
{
    char cdevice[PATH_MAX];
    char bdevice[PATH_MAX];
    DIR* dfd;
    struct dirent* dent;
    struct stat cstatbuf;
    struct stat bstatbuf;
    FILE* f;
    struct mnttab mp;
    struct mnttab mpref;
    int inuseflag; 
    unsigned long sectors;
    long tsectors;
    char mntpnt[PATH_MAX];
    char templine[256];
    double size;
    int tfd;

    if (((DIR*)NULL) == (dfd = opendir (cdevpath))) return;
    f = fopen ("/etc/mnttab", "r");
    while (NULL != (dent = readdir(dfd))) {
        if (strcmp(dent->d_name, ".") == 0) continue;
        if (strcmp(dent->d_name, "..") == 0) continue;
        sprintf (cdevice, "%s/%s", cdevpath, dent->d_name);
        sprintf (bdevice, "%s/%s", bdevpath, dent->d_name);
        errno = 0;
        inuseflag = 0;
        mntpnt[0] = '\0';
        templine[0] = '\0';

        if (0 != stat (cdevice, &cstatbuf)) continue;
        if (!(S_ISCHR(cstatbuf.st_mode))) continue;
        if (0 != stat (bdevice, &bstatbuf)) continue;
        if (!(S_ISBLK(bstatbuf.st_mode))) continue;
    
        tsectors = (long) disksize (cdevice);
        sectors = (unsigned long) tsectors;
        if (tsectors == -1 || 0L == sectors || 0xffffffff == sectors) {
            continue;
        }
        if (-1 == (tfd = open (cdevice, O_RDWR | O_EXCL))) {
            inuseflag = 1;
        } else {
            (void) close (tfd);
        }
        if (-1 == (tfd = open (bdevice, O_RDWR | O_EXCL))) {
            inuseflag = 1;
        } else {
            (void) close (tfd);
        }
        if (inuseflag && f != (FILE*)NULL) {
            rewind(f);
            mpref.mnt_special = bdevice;
            mpref.mnt_mountp = (char*)NULL;
            mpref.mnt_fstype = (char*)NULL;
            mpref.mnt_mntopts = (char*)NULL;
            mpref.mnt_time = (char*)NULL;
            if (0 == getmntany (f, &mp, &mpref)) {
                if ((char*)NULL != mp.mnt_mountp && 0 < strlen(mp.mnt_mountp)) {
                    strcpy (mntpnt, mp.mnt_mountp);
                }
            }
        }
        /* calculate the size in gigabytes. */
        size = ((double)sectors * (double)DEV_BSIZE) / 1024.0;
        if (size >= 1.0 && size < 1024.0) {
            sprintf (templine, "{%s %d %.2f KB / %lu SECT %s} ", cdevice,
                inuseflag, size, sectors, mntpnt);
        } 
        size = size / 1024.0;
        if  (size >= 1.0 && size < 1024.0) {
            sprintf (templine, "{%s %d %.2f MB / %lu SECT %s} ", cdevice,
                inuseflag, size, sectors, mntpnt);
        }
        size = size / 1024.0;
        if  (size >= 1.0) {
            sprintf (templine, "{%s %d %.2f GB / %lu SECT %s} ", cdevice,
                inuseflag, size, sectors, mntpnt);
        }
        write (fd, (void*) templine, strlen(templine));
    }
    if (f != (FILE*)NULL) fclose(f);
}

/****************************************************************************
 * walk_rdsk_subdirs_for_devs -- recursively walks a directory tree that
 *                               already has rdsk in it for their devices
 ***************************************************************************/
void
walk_rdsk_subdirs_for_devs (int fd, char* rootdir)
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
        capture_devs_in_dir (fd, cdevdirpath, bdevdirpath);
        walk_rdsk_subdirs_for_devs (fd, cdevdirpath);
    }
    (void) closedir (dfd);
}

/****************************************************************************
 * walk_dirs_for_devs -- walks a directory tree, seeing if rdsk/dsk 
 *                          subdirectories exist, and report on their
 *                          devices
 ***************************************************************************/
void
walk_dirs_for_devs (int fd, char* rootdir)
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
        if (0 == strcmp(dent->d_name, "rdsk")) continue;
        if (0 == strcmp(dent->d_name, "dsk")) continue;
        sprintf (cdevdirpath, "%s/%s/rdsk", rootdir, dent->d_name);
        sprintf (bdevdirpath, "%s/%s/dsk", rootdir, dent->d_name);
        /* -- open the character device directory */
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath))) continue;
        (void) closedir (cdevfd);
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath))) continue;
        (void) closedir (bdevfd);
        capture_devs_in_dir (fd, cdevdirpath, bdevdirpath);
    }
    (void) closedir (dfd);
}

/****************************************************************************
 * capture_all_devs -- write out all devices found on a system to
 *                     the specified output device
 ***************************************************************************/
void
capture_all_devs (int fd)
{
    DIR* dfd;
#if defined(SOLARIS)

    /* -- get the local disk devices */
    capture_devs_in_dir (fd, "/dev/rdsk", "/dev/dsk");
    /* -- get the VxVM disk group volumes */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
        (void) closedir (dfd);
        if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/rdsk"))) {
            (void) closedir (dfd);
            walk_rdsk_subdirs_for_devs (fd, "/dev/vx/rdsk");
        }
    }

    /* -- get the datastar devices -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/" QNM);
    }
  
    /* -- get the SDS disk set devices */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/md"))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/md");
    }
  
    /* -- get the non-diskset SDS devices */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/md/rdsk"))) {
        (void) closedir (dfd);
        capture_devs_in_dir (fd, "/dev/md/rdsk", "/dev/md/dsk");
    }

  
#endif /* SOLARIS */
  
#if defined(HPUX)
    (void) capture_logical_volumes (fd);
    /* -- get the datastar devices -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/" QNM);
    }
#endif /* HPUX */

#if defined(_AIX)
    (void) enum_lvs (fd);
    /* -- get the datastar devices here -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/" QNM);
    }
#endif /* defined(_AIX) */
	
    return;
}

/****************************************************************************
 * process_dev_info_request -- start processing a device list request
 ***************************************************************************/
int
process_dev_info_request (int fd, char* cmd)
{
    if (0 == strncmp(cmd, "ftd get all devices", 19)) {
        write (fd, "1 ", strlen("1 "));
        capture_all_devs (fd);
        write (fd, "\n", strlen("\n"));
    }
    return (0);
}

/*=======================================================================*/
/*=======================================================================*/

#ifdef STANDALONE_DEBUG

int
main (int argc, char** argv)
{
    if (0 != geteuid()) {
        fprintf (stderr, "must be root to run this program\n");
        exit (1);
    }
    process_dev_info_request (1, argv[1]);
}

#endif /* STANDALONE_DEBUG */

/*-------------- end of file ---------------*/
