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
 *   08/05/1997 - Steve Wahl   - original code
 *   04/06/2001 - Danny Turrin - Removed un-necessary raw device
 *                               validation in procedure convert_lv_name
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
#include "common.h"
#if defined(linux)
#endif
#include "pathnames.h"
#ifdef NEED_SYS_MNTTAB
#include "ftd_mount.h"
#else
#include <sys/mnttab.h>
#endif

#ifdef HAVE_LIBZFS
#include "dynamic_libzfs.h"
static dynamic_libzfs_context_t* global_dynamic_libzfs_context;
#endif

#if defined(SOLARIS) /* 1LG-ManyDEV */ 
int dummyfd_mnt;
#endif

extern u_longlong_t fdisksize (int fd); 
extern u_longlong_t disksize (char* devicepath); 

#if defined(linux)
struct part *startp = NULL;
long maxpart;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define LVMDIR "/etc/lvmtab.d"
#else
#define LVMDIR "/etc/lvm/backup"
#endif /* < KERNEL_VERSION(2, 6, 0) */
#endif

#if defined(_AIX)
#  define OUTFMTSTR "{%s %d %.2f %s / %llu SECT %lu %ld %s} "
#else
#  define OUTFMTSTR "{%s %d %.2f %s / %llu SECT %s} "
#endif

/****************************************************************************
 * force_dsk_or_rdsk_path -- force a path to have either /rdsk or /dsk in it
 *                           and add it to the end if it isn't there
 ***************************************************************************/
#if defined(_AIX) || defined(HPUX)
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
          
          /*
           * Code added to check if the path is a directory or not.
           * If it is a directory for character devices then 
           * setting the variables accordingly to convert from 
           * character to block or viceversa
           */

             if (S_ISDIR(sb.st_mode)) {
                 if (strstr(pathin,"/rdsk")!= NULL) {
                           is_char = 1;
                    }  
                 else if (strstr(pathin,"/dsk")!= NULL) {
                           is_char = 0;
                    }
                 }
                else

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
	/* We do not process raw devices on linux */
        strcpy (pathout, pathin);
}
#else /* defined(_AIX) || defined(HPUX)*/
void
force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag)
{
    int i, len, foundflag;
	struct stat sb;

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
		char *suffix = tordskflag ? "/rdsk" : "/dsk";
		char *src, *dest;
		if (stat(pathin, &sb) == 0 &&
			(sb.st_mode & (S_IFCHR|S_IFBLK)) &&
			(src = strrchr(pathin, '/'))) {
			/*
			 * Block or char special device:  perform needed (tordskflag) name
			 * transformation on the last pathname component.
			 */
			len = (src - pathin) + 1;
			strncpy(pathout, pathin, len);
			src++;				/* skip over '/' */
			dest = &pathout[len];
			if (sb.st_mode & S_IFBLK && tordskflag) {
				/* <blk dev name> => r<blk dev name> */
				*dest++ = 'r';
				suffix = NULL;
			} else if (sb.st_mode & S_IFCHR && !tordskflag) {
				if (*src == 'r') {
					/* r<blk dev name> => <blk dev name> */
					suffix = NULL;
					src++;
				}
			} else
				/*
				 * device is the right device type, keep the original name
				 */
				suffix = NULL;
		} else {
			/*
			 * Look for device name with raw/block suffix (/rdsk or /dsk)
			 */
			dest = pathout;
			src = pathin;
		}
        strcpy (dest, src);
		if (suffix) {
			dest = &pathout[strlen(pathout) - 1];
			while (dest > pathout && *dest == '/')
				dest--;
			strcpy(dest, suffix);
		}
    }
}
#endif /* !defined(_AIX) !defined(HPUX)*/

#if defined(HPUX)

#include <sys/diskio.h>
#include <sys/param.h>
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

void
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
#if defined(HPUX) && (SYSVERS >= 1131)
   /* HPUX 11.31 has vgdisplay command in /usr/sbin; soft link from /etc not necessarily there at install (pc071029) */
    char command[] = "/usr/sbin/vgdisplay -v";
#else
    char command[] = "/etc/vgdisplay -v";
#endif
 
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
                    size = size / 1024.0;
                    if (size >= 1.0) {
                        sprintf (outbuf, "{%s %d %.2f TB / %lu SECT %s} ",
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
#include "ftd_mount.h"

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
	long            major;
	long            minor;
	int             lvmerr;
	int             vgndx;
	int             lvndx;
	unsigned long long csize;	/* for over 2GB volume */
	double          fsize;
	char           *units;
	int             inuseflag;
	unsigned long long   sectcnt;
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
		major   = qvgs->vgs[vgndx].major_num;

		/* enumerate each logical volume */
		for (lvndx = 0; lvndx < num_lvs; lvndx++) {

			lvmerr = lvm_querylv(&qvgp->lvs[lvndx].lv_id,
					     &qlv, (char *) NULL);
			if (lvmerr)
				return (lvmerr);

			minor = qvgp->lvs[lvndx].lv_id.minor_num;
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
			csize = (unsigned long long)qlv->currentsize * (2LLU << (qlv->ppsize - 1));

			sectcnt = (csize >> DEV_BSHIFT);
			fsize = (double) csize;
			fsize = fsize / 1024.0;
			if (fsize >= 1.0 && fsize < 1024.0) {
				units = "KB";
			} else {
			    fsize = fsize / 1024.0;
			    if (fsize >= 1.0 && fsize < 1024.0) {
				units = "MB";
			    } else {
				fsize = fsize / 1024.0;
			        if (fsize >= 1.0 && fsize < 1024.0) {
				    units = "GB";
				} else {
				    fsize = fsize / 1024.0;
				    units = "TB";
				}
			    }
			}
			sprintf(&pbuf[0], OUTFMTSTR, cdevice, inuseflag,
			    fsize, units, sectcnt, major, minor, mntedon);
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


#if defined(linux)

/* execute while exiting configtool */
void
close_part_list()
{
    if (startp != NULL){
        free(startp);
        startp = NULL;
    }
}


/* If the inquired string of device name
   is present the global , returns 1 else 0
*/
int
ismatchinlist( char *inpstr )
{
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
make_part_list( )
{
    FILE *fh;
    char tmp[80], size[80], name[80];
    int ret;
    int i;
    long l;
    struct part *p;

    fh = fopen("/proc/partitions", "r+");
    if (fh == NULL) {
        printf("make_part_list(): fopen(/proc/partitions) fails %s(%d)\n",
			strerror(errno), errno);
        return -1;
    }

    /* count total partitions exists */
    do {
        ret = fscanf( fh, "%79s %79s %79s %79s",
                      (char *)&tmp, (char *)&tmp,  (char *)&size, (char *)&name);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
        i=11;
        while (i--)
            ret = fscanf( fh, "%79s", (char *)&tmp);
#endif

        maxpart++;

    } while ((ret > 0) && ( ret != EOF ));

    /* allocate mem to list parttions and its size */
    startp = (struct part*) malloc(sizeof(*startp) * maxpart);
    if ((startp) == NULL) {
        printf("Error:%s\n", strerror(errno));
        return -1;
    }

    p = startp;
    rewind(fh);

    /* make list of partitions */
    for (l=0; l < maxpart; l++) {
        ret = fscanf( fh, "%79s %79s %79s %79s",
                      (char *)&tmp, (char *)&tmp,  (char *)&size, (char *)&name);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
        i=11;
        /* ignore 11 infos for block devices */
        while (i--)
            ret = fscanf( fh, "%79s", (char *)&tmp);
#endif

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

static int
dev_swapped(char *devname)
{
	FILE	*fp;
	char	*tok, line[128];
	int	found = 0;

	if ((fp = fopen("/proc/swaps", "r")) == NULL)
		return(found);

	while (fgets(line, sizeof line, fp) != NULL) {
		if ((tok = strtok(line, "\t ")) != NULL) {
			if (strcmp(tok, devname) == 0) {
				found = 1;
				break;
			}
		}
	}
	fclose(fp);

	return(found);
}

/* function to identify whether device is mounted or in use as swap
   Return 0->not in use
   1 - in use
   -1  -> unable to get contrl device
*/
int
ismountorswap(char *dname)
{
    char	mountp[MAXPATH];
	/*
	 * in case the ftd driver is not loaded, check if the device
	 * is mounted as a file system or a swap device.
	 * well, there may be some better approach available to detect
	 * whether a device is in use.
	 */
    return (dev_swapped(dname) ? 1 : dev_mounted(dname, mountp));
}
#endif /* partition active devices Linux */

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
    unsigned long long sectors;
    unsigned long long tsectors;    
    char mntpnt[PATH_MAX];
    char templine[256];
    long double size; 
    char *tdevice;	/* holds cdevice or bdevice name */
    char *units;	/* device display storage units */
#if defined(linux)
    int inuse = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
    char buffer[PATH_MAX];
    char *displaydevname[4];
#endif /* >= KERNEL_VERSION(2, 6, 0) */
#endif /* defined(linux) */
#if defined(SOLARIS)
    int  tmpfd;
#endif  /* defined(SOLARIS) */
#if defined(_AIX)
    int  cmajor, cminor;	/* major and minor device numbers */
#endif  /* defined(_AIX) */

#if defined(linux)
    if (((DIR*)NULL) == (dfd = opendir (bdevpath))) return;
#else
    if (((DIR*)NULL) == (dfd = opendir (cdevpath))) return;
#endif /* defined(linux) */

#if defined(linux)
    f = fopen("/etc/mtab", "r");
#elif defined(SOLARIS)
    tmpfd = fcntl(dummyfd_mnt, F_DUPFD, 0);
    close(dummyfd_mnt);
    f = fopen ("/etc/mnttab", "r");
#else
    f = fopen ("/etc/mnttab", "r");
#endif /* defined(linux) */

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
#endif /* defined(linux) */

        errno = 0;
        inuseflag = 0;
        mntpnt[0] = '\0';
        templine[0] = '\0';
        sprintf (bdevice, "%s/%s", bdevpath, dent->d_name);
#if !defined(linux)     
        sprintf (cdevice, "%s/%s", cdevpath, dent->d_name);
        if (0 != stat (cdevice, &cstatbuf)) continue;
        if (!(S_ISCHR(cstatbuf.st_mode))) continue;
        if(strcmp(cdevice, bdevice) == 0){
            sprintf (bdevice, "%s/%s", bdevpath, &dent->d_name[1]);
        }
        if (0 != stat (bdevice, &bstatbuf)) continue;
        if (!(S_ISBLK(bstatbuf.st_mode))) continue;
        tsectors = (unsigned long long) disksize (cdevice);
#  if defined(_AIX)
	cmajor = bmajor(cstatbuf.st_rdev);
	cminor = minor(cstatbuf.st_rdev);
#  endif
#else
        if (0 != stat (bdevice, &bstatbuf)) continue;
        if (!(S_ISBLK(bstatbuf.st_mode))) continue;
        tsectors = (unsigned long long) disksize (bdevice);
#endif /* !defined(linux) */	
        sectors = (unsigned long long) tsectors;
        if ( 0L == sectors || 0xFFFFFFFFFFFFFFFFULL == sectors ) {
            continue;
        }
#if defined(linux)
       /* get the number  open for a block device */
        inuse = ismountorswap(bdevice);
        if (inuse  > 0) inuseflag = 1;
           else if (inuse < 0) continue;
#endif /* defined(linux) */

#if defined(linux)
        if (inuseflag && f != (FILE*)NULL) {
#else
        if (f != (FILE*)NULL) {
#endif /* defined(linux) */
            rewind(f);
            mpref.mnt_special = bdevice;
            mpref.mnt_mountp = (char*)NULL;
            mpref.mnt_fstype = (char*)NULL;
            mpref.mnt_mntopts = (char*)NULL;
            mpref.mnt_time = (char*)NULL;
            if (0 == getmntany (f, &mp, &mpref)) {
            	inuseflag = 1;
                if ((char*)NULL != mp.mnt_mountp && 0 < strlen(mp.mnt_mountp)) {
                    strcpy (mntpnt, mp.mnt_mountp);
                }
            }
        }
        /* calculate the size in gigabytes. */
#if !defined(linux)
        tdevice = cdevice;
#else
        tdevice = bdevice;
#endif

#ifdef HAVE_LIBZFS
        if(global_dynamic_libzfs_context &&
           strlen(mntpnt) == 0)
        {
            char* zpool_name = NULL;
            
            if(dynamic_libzfs_zpool_device_path_in_use(global_dynamic_libzfs_context, tdevice, &zpool_name))
            {
                snprintf(mntpnt, sizeof(mntpnt), "member of zpool: %s", zpool_name);
                free(zpool_name);
            }
        }
#endif
        size = ((long double)sectors *(long double)DEV_BSIZE) / 1024.0;
        if (size >= 1.0 && size < 1024.0) {
            units = "KB";
        } else {
            size = size / 1024.0;
            if  (size >= 1.0 && size < 1024.0) {
                units = "MB";
            } else {
                size = size / 1024.0;
               if (size >= 1.0 && size < 1024.0) { 
                units = "GB";
              } else {   
                size = size / 1024.0; 
                units = "TB"; 
              }
            }
        }
#if defined(linux)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
        strcpy(buffer,tdevice);
        displaydevname[0] = strtok(buffer, "/");
        displaydevname[1] = strtok(NULL, "/");
        if (strcmp(displaydevname[1],"mapper") == 0) {
            displaydevname[2] = strtok(NULL, "-");
            displaydevname[3] = strtok(NULL, "\0");
	    if (displaydevname[2] == NULL || displaydevname[3] == NULL) {
		strcpy(buffer,tdevice);
	    } else {
            sprintf(buffer,"/dev/%s/%s",displaydevname[2],displaydevname[3]);
	    }
        } else {
            strcpy(buffer,tdevice);
        }
        sprintf (templine, OUTFMTSTR, buffer,
              inuseflag, (double)size, units, sectors, mntpnt);
#else
          sprintf (templine, OUTFMTSTR, tdevice,
                inuseflag, (double)size, units, sectors, mntpnt);
#endif /* >= KERNEL_VERSION(2, 6, 0) */
#elif defined(_AIX)
          sprintf (templine, OUTFMTSTR, tdevice,
                inuseflag, (double)size, units, sectors, cmajor, cminor, mntpnt);
#else
          sprintf (templine, OUTFMTSTR, tdevice,
                inuseflag, (double)size, units, sectors, mntpnt);

#endif /* defined(linux) */
        write (fd, (void*) templine, strlen(templine));
    }
#if defined(SOLARIS)
    if (f != (FILE*)NULL) fclose(f);
    dup2(tmpfd, dummyfd_mnt);
    close(tmpfd);
#else
    if (f != (FILE*)NULL) fclose(f);
#endif
}

/****************************************************************************
 * capture_AIX_rhdiskX_devs -- reports on all rhdisk devices in /dev
 ***************************************************************************/
#if defined(_AIX)
static void
capture_AIX_rhdiskX_devs (int fd)
{
    char cdevice[PATH_MAX];
    DIR* dfd;
    struct dirent* dent;
    struct stat cstatbuf;
    FILE* f;
    struct mnttab mp;
    struct mnttab mpref;
    int inuseflag; 
    unsigned long long sectors;
    unsigned long long tsectors;    
    char mntpnt[PATH_MAX];
    char templine[256];
    long double size; 
    char *tdevice;	/* holds cdevice or bdevice name */
    char *units;	/* device display storage units */

    int  cmajor, cminor;	/* major and minor device numbers */

    if (((DIR*)NULL) == (dfd = opendir ("/dev"))) return;

    f = fopen ("/etc/mnttab", "r");

    while (NULL != (dent = readdir(dfd))) {
        if (strcmp(dent->d_name, ".") == 0) continue;
        if (strcmp(dent->d_name, "..") == 0) continue;


        errno = 0;
        inuseflag = 0;
        mntpnt[0] = '\0';
        templine[0] = '\0';
        sprintf (cdevice, "/dev/%s", dent->d_name);

        if( strstr( cdevice, "/dev/rhdisk" ) == NULL ) continue;
        if (0 != stat (cdevice, &cstatbuf)) continue;
        if (!(S_ISCHR(cstatbuf.st_mode))) continue;

        tsectors = (unsigned long long) disksize (cdevice);
	    cmajor = bmajor(cstatbuf.st_rdev);
	    cminor = minor(cstatbuf.st_rdev);
        sectors = (unsigned long long) tsectors;
        if ( 0L == sectors || 0xFFFFFFFFFFFFFFFFULL == sectors ) {
            continue;
        }

        if (f != (FILE*)NULL) {
            rewind(f);
            mpref.mnt_special = cdevice;
            mpref.mnt_mountp = (char*)NULL;
            mpref.mnt_fstype = (char*)NULL;
            mpref.mnt_mntopts = (char*)NULL;
            mpref.mnt_time = (char*)NULL;
            if (0 == getmntany (f, &mp, &mpref)) {
            	inuseflag = 1;
                if ((char*)NULL != mp.mnt_mountp && 0 < strlen(mp.mnt_mountp)) {
                    strcpy (mntpnt, mp.mnt_mountp);
                }
            }
        }
        /* calculate the size in gigabytes. */
        tdevice = cdevice;

        size = ((long double)sectors *(long double)DEV_BSIZE) / 1024.0;
        if (size >= 1.0 && size < 1024.0) {
            units = "KB";
        } else {
            size = size / 1024.0;
            if  (size >= 1.0 && size < 1024.0) {
                units = "MB";
            } else {
                size = size / 1024.0;
               if (size >= 1.0 && size < 1024.0) { 
                units = "GB";
              } else {   
                size = size / 1024.0; 
                units = "TB"; 
              }
            }
        }
          sprintf (templine, OUTFMTSTR, tdevice,
                inuseflag, (double)size, units, sectors, cmajor, cminor, mntpnt);
        write (fd, (void*) templine, strlen(templine));
    }
    if (f != (FILE*)NULL) fclose(f);
}
#endif

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

        if (0 == strcmp(dent->d_name, ".nodes"))
        {
            continue;
        }


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
#if !defined(linux)
        if (0 == strcmp(dent->d_name, "rdsk")) continue;
#endif /* !defined(linux) */
        if (0 == strcmp(dent->d_name, "dsk")) continue;
#if !defined(linux)
        sprintf (cdevdirpath, "%s/%s/rdsk", rootdir, dent->d_name);
#endif /* !defined(linux) */
        sprintf (bdevdirpath, "%s/%s/dsk", rootdir, dent->d_name);
        /* -- open the character device directory */
#if !defined(linux)
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath))) continue;
        (void) closedir (cdevfd);
#endif /* !defined(linux) */
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath))) continue;
        (void) closedir (bdevfd);
        capture_devs_in_dir (fd, cdevdirpath, bdevdirpath);
    }
    (void) closedir (dfd);
}

/****************************************************************************
 * capture_devlistfile -- write out all devices.
 *                        Defined output device on devconf file.
 ***************************************************************************/
void
capture_devlistfile (int fd)
{
    DIR* dfd;
    char cdevice[PATH_MAX];
    char bdevice[PATH_MAX];
    char devpath[PATH_MAX];
    char *devlist = (char *)NULL;
    char *listp;
    char configpath[FILE_PATH_LEN];
    char *rdskp;
#if defined(SOLARIS)
    int  cfgfd;
#else
    FILE *cfgfd;
#endif
    int  i;
    int  len;
    int  f_dup;
    int  listused = 0;
#if defined(HPUX)
    FILE *fp;
#if (SYSVERS >= 1131)
   /* HPUX 11.31 has vgdisplay command in /usr/sbin; soft link from /etc not necessarily there at install (pc071029) */
    char command[] = "/usr/sbin/vgdisplay";
#else
    char command[] = "/etc/vgdisplay";
#endif
 
    char tempbuf[MAXPATHLEN];
    char *ptr;
    char *vglist = (char *)NULL;
    char *vglistp;
    int  vgnum = 0;
#endif

#if defined(HPUX)
    if ((fp = popen(command, "r")) != NULL) {
        memset(tempbuf, 0, MAXPATHLEN);    
        while (fgets(tempbuf, MAXPATHLEN, fp) != NULL) {
            if ((ptr = strstr(tempbuf, "VG Name")) != NULL) {
                if ((vgnum % 4) == 0 ) {
                    vglist = (char *)ftdrealloc (vglist, PATH_MAX*4*(vgnum/4+1));
                }
                ptr += 8;
                while (*ptr && isspace(*ptr)) {
                    ptr++;
                }
                vglistp = vglist + (PATH_MAX * vgnum);
                memset(vglistp, 0, PATH_MAX);
                strcpy(vglistp, ptr);
                vgnum++;
            }
            memset(tempbuf, 0, MAXPATHLEN);
        }
    }
#endif

    /* -- get the user config devices -- */
    sprintf(configpath, "%s/devlist.conf", PATH_CONFIG);
#if defined(SOLARIS)
    if ((cfgfd = open(configpath, O_RDONLY)) != -1) {
        while (ftd_fgets(devpath, PATH_MAX, cfgfd) != NULL) {
#else
    if ((cfgfd = fopen(configpath, "r")) != NULL) {
        while (fgets(devpath, PATH_MAX, cfgfd) != NULL) {	
#endif
            if (devpath[0] != '/') {
                continue;
            }
            len = strlen(devpath);

            while (devpath[len-1] == '\n' || devpath[len-1] == ' ' || devpath[len-1] == '\t' || devpath[len-1] == '/') {
                devpath[len-1] = 0;
         	len--;
            }

            /* specified path check */
            if ((len == 8) && (strcmp(devpath,"/dev/dtc") == 0)) {
                continue;
            }
            if ((len >= 9) && (strncmp(devpath,"/dev/dtc/", 9) == 0)) {
                continue;
            }
#if defined(SOLARIS)
            if ((len == 4) && (strcmp(devpath,"/dev") == 0)) {
                continue;
            }
            if ((len == 7) && (strcmp(devpath,"/dev/vx") == 0)) {
                continue;
            }
            if ((len == 7) && (strcmp(devpath,"/dev/md") == 0)) {
                continue;
            }
            if ((len == 11) && (strcmp(devpath,"/dev/sfdisk") == 0)) {
                continue;
            }
#elif defined(_AIX) 
            if ((len == 4) && (strcmp(devpath,"/dev") == 0)) {   
                continue;    
            }
#elif defined(linux) 
            if ((len == 4) && (strcmp(devpath,"/dev") == 0)) {   
                continue;    
            }
            if ((len == 11) && (strcmp(devpath,"/dev/mapper") == 0)) {   
                continue;    
            }
            if ((len == 10) && (strcmp(devpath,"/dev/mpath") == 0)) {   
                continue;    
            }
#elif defined(HPUX)
            f_dup = 0;
            for (i=0; i<vgnum; i++) {
                vglistp = vglist + (PATH_MAX * i);    
                if ((strncmp(devpath, vglistp, len) == 0)) {
                    f_dup = 1;
                    break;
                }
            }
            if (f_dup == 1) {
                continue;
            }
#endif
            /* duplication path check */
            if ((listused % 8) == 0 ) {
                devlist = (char *)ftdrealloc (devlist, PATH_MAX*8*(listused/8+1));
            }
            f_dup = 0;
            for (i=0; i<listused; i++) {
                if (strcmp((devlist + (PATH_MAX * i)), devpath) == 0) {
                    f_dup = 1;
                    break;
                }
            }
            if (f_dup == 1) {
                continue;
            }

            listp = devlist + (PATH_MAX * listused);
            memset(listp, 0, PATH_MAX);
            strncpy(listp, devpath, len);
            listused++;

            memset(bdevice, 0x00, PATH_MAX);
            memset(cdevice, 0x00, PATH_MAX);

            if (((DIR*)NULL) == (dfd = opendir (devpath))) {
                continue;
            }
            (void) closedir (dfd);

	    /* process rdsk anywhere within the device pathname */
	    /* and use dsk instead of removing a leading r in the basename */

            if ((rdskp=strstr(devpath, "/rdsk")) == 0
	     || (*(rdskp+5) != '/' && *(rdskp+5) != '\0')) {
#if defined(SOLARIS)
                /* -- devices type : /AAAA/yyy/rdsk/xxx -- */
                walk_dirs_for_devs (fd, devpath);
                
                /* -- devices type : /AAAA/rdsk/yyy/xxx -- */
                sprintf(cdevice, "%s/rdsk", devpath); 
                if (((DIR*)NULL) != (dfd = opendir (cdevice))) {
                    (void) closedir (dfd);
                    walk_rdsk_subdirs_for_devs (fd, cdevice);
                }
                memset(cdevice, 0x00, PATH_MAX);
#endif
                /* -- devices type : /AAAA/xxx and /AAAA/rxxx-- */
                capture_devs_in_dir (fd, devpath, devpath);

                sprintf(cdevice, "%s/rdsk", devpath);
                sprintf(bdevice, "%s/dsk", devpath);
            } else {
                strncpy(cdevice, devpath, len);
                strncpy(bdevice, devpath, rdskp-devpath+1);
                strcpy(bdevice+(rdskp-devpath+1), rdskp+2 );
            }
            /* -- devices type : /AAAA( /AAA/rdsk )-- */
            if (((DIR*)NULL) != (dfd = opendir (cdevice))) {
                (void) closedir (dfd);    
                capture_devs_in_dir (fd, cdevice, bdevice);
            }
       }
#if defined(SOLARIS)
        close(cfgfd);
#else
        fclose(cfgfd);
#endif
        free(devlist);
    }
#if defined(HPUX)
    if (fp != NULL) {
        pclose(fp);
        free(vglist);
    }
#endif
}

/****************************************************************************
 * capture_all_devs -- write out all devices found on a system to
 *                     the specified output device
 ***************************************************************************/
void
capture_all_devs (int fd)
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

#ifdef HAVE_LIBZFS
    global_dynamic_libzfs_context = dynamic_libzfs_init();
#endif
    
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

    /* -- get the GDS(FUJITSU Global Disk Service) disk set devices */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/sfdsk"))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/sfdsk");
    }
    /* -- get the user config devices -- */
    capture_devlistfile (fd);
#endif /* SOLARIS */
  
#if defined(HPUX)
    (void) capture_logical_volumes (fd);
   
   /* -- get the VxVM disk group volumes */ 
   if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
     (void) closedir (dfd);
     if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/rdsk"))) {
            (void) closedir (dfd);
            walk_rdsk_subdirs_for_devs(fd, "/dev/vx/rdsk");
       }
   }

    /* -- get the datastar devices -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/" QNM);
    }
    /* -- get the user config devices -- */
    capture_devlistfile (fd);
#endif /* HPUX */

#if defined(_AIX)
    (void) enum_lvs (fd);

    /* -- get the VxVM disk group volumes */
   if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
     (void) closedir (dfd);
     if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/rdsk"))) {
            (void) closedir (dfd);
            walk_rdsk_subdirs_for_devs(fd, "/dev/vx/rdsk");
       }
    }
    
    // Get /dev/rhdisk* for AIX bootvolume migration feature (defect 72110)     
    capture_AIX_rhdiskX_devs (fd);
    
    /* -- get the datastar devices here -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/" QNM);
    }
    /* -- get the user config devices -- */
    capture_devlistfile (fd);
#endif /* AIX */

#if defined(linux)
 if (startp == NULL) {
   if (make_part_list() == -1) {
      printf("Warning: Timeout is not effective\n");
   }
 }
    /* -- get the IDE/SCSI/RAID devices -- */
    capture_devs_in_dir (fd, "/dev", "/dev");
    /* -- get the LVM devices -- */
    if (((DIR*)NULL) != (dfd = opendir (LVMDIR))) {
        while (NULL != (dent = readdir(dfd))) {
            if (strcmp(dent->d_name, ".") == 0) continue;
            if (strcmp(dent->d_name, "..") == 0) continue;
                /* create LVM path */
                sprintf(cdevice, "/dev/%s", dent->d_name);
                strcpy(bdevice,cdevice);
                capture_devs_in_dir (fd, cdevice, bdevice);
                memset(bdevice, 0x00, PATH_MAX);
                memset(cdevice, 0x00, PATH_MAX);
        }
        (void) closedir (dfd);
    }
    capture_devs_in_dir (fd, "/dev/mapper", "/dev/mapper");

    if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
        (void) closedir (dfd);
        if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/dsk"))) {
            (void) closedir (dfd);
            walk_rdsk_subdirs_for_devs (fd, "/dev/vx/dsk");
        }
    }

    /* -- get the evms devices here -- */
    if (((DIR*) NULL) != (dfd = opendir("/dev/evms")))
    {
        (void) closedir(dfd);
        strcpy(bdevice, "/dev/evms");
        strcpy(cdevice, "/dev/evms");
        capture_devs_in_dir(fd, cdevice, bdevice);
        walk_rdsk_subdirs_for_devs(fd, "/dev/evms");
    }

    /* -- get the /dev/mpath devices here -- */
    if (((DIR*) NULL) != (dfd = opendir("/dev/mpath")))
    {
        (void) closedir(dfd);
        strcpy(bdevice, "/dev/mpath");
        strcpy(cdevice, "/dev/mpath");
        capture_devs_in_dir(fd, cdevice, bdevice);
        walk_rdsk_subdirs_for_devs(fd, "/dev/mpath");
    }

    /* -- get the datastar devices here -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
        walk_dirs_for_devs (fd, "/dev/" QNM);
    }

    /* -- get the user config devices -- */
    sprintf(configpath, "%s/devlist.conf", PATH_CONFIG);
    if ( (cfgfd = fopen(configpath, "r")) != NULL ){
        fclose(cfgfd);
        sprintf(command, "/bin/cat %s | /bin/sed \"s/\\/$//\" | /bin/sort | /usr/bin/uniq", configpath);
        if ((cfgfd = popen(command,"r")) != NULL)
        {
	    while ( fgets(devpath, PATH_MAX, cfgfd) != NULL) {
                if ( devpath[0] != '/')
                {
	            continue;
                }
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

                if ((len == 9) && (strncmp(devpath,"/dev/evms",9) == 0))
                {
                    continue;
                }

                if ((len != 4) && (strncmp(devpath,"/dev",4) == 0))
                {
                    /* convert path for VG check */
                    strcpy(tmppath,LVMDIR);
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
                    capture_devs_in_dir (fd, cdevice, bdevice);
	        }
	    }
	    pclose(cfgfd);
	}
    }

  close_part_list();
#endif /* defined(linux) */

#ifdef HAVE_LIBZFS
    dynamic_libzfs_finish(global_dynamic_libzfs_context);
#endif
  
    return;
}

/****************************************************************************
 * process_dev_info_request -- start processing a device list request
 ***************************************************************************/
int
process_dev_info_request (int fd, char* cmd)
{
    if (cmd != NULL && 0 == strncmp(cmd, "ftd get all devices", 19)) {
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
        EXIT(1);
    }
    process_dev_info_request (1, "ftd get all devices info");
    return 0;
}
#endif


/*-------------- end of file ---------------*/

