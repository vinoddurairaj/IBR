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
/*
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

#if defined(HPUX)

/*
 * jch: can't seem to get this to work with aCC
*/
#undef _STDC_32_MODE__
#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <mntent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/diskio.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "errors.h"
#include "bigints.h"
#include "platform.h"
#include "ftd_mount.h"
#include "ftd_cmd.h"
#include "ftdio.h"

static int makedirs(char *path, int start, char *temp, mode_t mode);

/*
 * 64-bit lseek. Doesn't work.
 */
offset_t
llseek(int filedes, offset_t offset, int whence)
{
    return(lseek64(filedes, offset, whence));
}

/*
 * search /etc/mnttab (referenced by fp) for an entry that
 * matches the properties passed in mpref. For now, we only
 * support searching for a matching name (mnt_special). This
 * function starts at the current file position and iterates
 * through the file until a match or EOF is found. If EOF is
 * found, then it returns EOF. See manpage on Solaris for
 * full description of this function.
 */
int
getmntany(FILE *fp, struct mnttab *mp, struct mnttab *mpref)
{
    struct mntent *ent;
    int count, found;
    static char timebuf[16];

    /* check for stupidity */
    if ((mpref == NULL) || (mp == NULL)) {
        return EOF;
    }
    count = 0;
    if (mpref->mnt_special != NULL) count++;
    if (mpref->mnt_mountp != NULL) count++;
    if (mpref->mnt_fstype != NULL) count++;
    if (mpref->mnt_mntopts != NULL) count++;
    if (mpref->mnt_time != NULL) count++;

    while ((ent = getmntent(fp)) != NULL) {
        found = 0;
        if ((mpref->mnt_special != NULL) &&
            (strcmp(ent->mnt_fsname, mpref->mnt_special) == 0)) {
            found++;
        }
        if ((mpref->mnt_mountp != NULL) && 
            (strcmp(ent->mnt_dir, mpref->mnt_mountp) == 0)) {
            found++;
        }
        if ((mpref->mnt_fstype != NULL) && 
            (strcmp(ent->mnt_type, mpref->mnt_fstype) == 0)) {
            found++;
        }
        if ((mpref->mnt_mntopts != NULL) && 
            (strcmp(ent->mnt_opts, mpref->mnt_mntopts) == 0)) {
            found++;
        }
        if ((mpref->mnt_time != NULL) && 
            ((atol(mpref->mnt_time) == ent->mnt_time) == 0)) {
            found++;
        }
        if (found == count) {
            mp->mnt_special = ent->mnt_fsname;
            mp->mnt_mountp = ent->mnt_dir;
            mp->mnt_fstype = ent->mnt_type;
            mp->mnt_mntopts = ent->mnt_opts;
            sprintf(timebuf, "%ld", ent->mnt_time);
            mp->mnt_time = timebuf; 
            return 0;
        }
    }
    return EOF;
}

/*
 * Recursively traverse a path, creating all of directories in the path.
 * Directories are created with the passed in mode. It is assumed the
 * umask has been properly set before calling this function. The passed
 * in path should not begin with '/'. The "temp" argument is temporary
 * storage (we could have used a stack variable, but why burn up stack
 * space). "start" is the beginning index into "path" to begin parsing.
 */
static int
makedirs(char *path, int start, char *temp, mode_t mode)
{
    int i, ret;

    for (i = start; i < strlen(path); i++) {
        if (path[i] == '/') {
            strncpy(temp, path, i);
            temp[i] = 0;
            ret = mkdir(temp, mode);
            if ((ret == -1) && (errno != EEXIST)) {
                return -1;
            } 
            makedirs(path, i+1, temp, mode);
            return 0;
        }
    }
    /* done. hit the end of the path */
    return 0;
}

/*
 * Given a raw device path, create two new device special files 
 * (a raw device and a block device) using the major numbers and minor 
 * number passed in. The directories preceding the devices are created, too.
 * Function returns the device numbers for the raw and block devices.
 *
 * The access mode for the devices is: rw-r-----
 */
int
ftd_make_devices(struct stat *cbuf, struct stat *bbuf, char *raw, char *block, dev_t cdev, dev_t bdev) /* SS/IS activity */
{
    int         ret;
    char        temp[MAXPATHLEN];
    mode_t      mode, old_mask;

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    /* create the directories above the devices */
    if ((makedirs(raw, 1, temp, mode) == 0) &&
        (makedirs(block, 1, temp, mode) == 0)) {

        unlink(raw);
        mknod(raw, cbuf->st_mode, cdev);
        chown (raw, cbuf->st_uid, cbuf->st_gid); /* SS/IS activity */
            
        unlink(block);
        mknod(block, bbuf->st_mode, bdev);
        chown (block, bbuf->st_uid, bbuf->st_gid); /* SS/IS actvity */

    }
    umask(old_mask);
    return 0;
}

/*
 * Given a logical group number and the major number of the device,
 * create the device file. Return the device number (major and minor)
 * for the device file.
 */
int
ftd_make_group(int lgnum, dev_t major_num, dev_t *dev_out)
{
    char        group_name[MAXPATHLEN], temp[MAXPATHLEN];
    dev_t       dev;
    mode_t      mode, old_mask;
    struct stat statbuf;

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    dev = makedev(major_num, lgnum | FTD_LGFLAG);
    *dev_out = dev;

    if (makedirs(group_name, 1, temp, mode) == 0) {
        mode = S_IWUSR | S_IRUSR | S_IRGRP;

        /* if device doesn't exist ... */ 
        if (stat(group_name, &statbuf) != 0) {
            mknod(group_name, _S_IFCHR | mode, dev);
        /* if device id is wrong ... */
        } else if (statbuf.st_rdev != dev) {
            /* waste it */
            unlink(group_name);
            mknod(group_name, _S_IFCHR | mode, dev);
        }
    }
    umask(old_mask);
    return 0;
}

#elif defined(SOLARIS)  || defined(linux)

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>

#include "platform.h"
#include "ftd_cmd.h"
#include "ftdio.h"

/*
 * Recursively traverse a path, creating all of directories in the path.
 * Directories are created with the passed in mode. It is assumed the
 * umask has been properly set before calling this function. The passed
 * in path should not begin with '/'. The "temp" argument is temporary
 * storage (we could have used a stack variable, but why burn up stack
 * space). "start" is the beginning index into "path" to begin parsing.
 */
static int
makedirs(char *path, int start, char *temp, mode_t mode)
{
    int i, ret;

    for (i = start; i < strlen(path); i++) {
        if (path[i] == '/') {
            strncpy(temp, path, i);
            temp[i] = 0;
            ret = mkdir(temp, mode);
            if ((ret == -1) && (errno != EEXIST)) {
                return -1;
            } 
            makedirs(path, i+1, temp, mode);
            return 0;
        }
    }
    /* done. hit the end of the path */
    return 0;
}

/*
 * Given a raw device path, the group number, and the minor number of
 * the device, create two new pathnames for the raw and block devices
 * and link them to the devices created by the driver.
 * The directories preceding the symbolic links are created, too.
 *
 * The access mode for the devices is: rw-r-----
 */
void
ftd_make_devices(struct stat *cbuf, struct stat *bbuf, char *raw, char *block, dev_t dev)
 /* SS/IS activity */ 
{
    char        temp[MAXPATHLEN];
    mode_t      mode, old_mask;

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if ((makedirs(raw, 1, temp, mode) == 0) &&
        (makedirs(block, 1, temp, mode) == 0)) {

#if !defined(linux)
        unlink(raw);
        mknod(raw, cbuf->st_mode, dev);
        chown (raw, cbuf->st_uid, cbuf->st_gid);   /* SS/IS activity */
#endif
        unlink(block);
        mknod(block, bbuf->st_mode, dev);
        chown (block, bbuf->st_uid, bbuf->st_gid); /* SS/IS actvity */
#if defined(linux)
 {
	char *p;
	int lgnum = -1, dtcnum = -1;
	/* Make symbolic links in /dev for Veritas to find these */
	p = strstr(block, "lg");
	if (p) {
		sscanf(p, "lg%d", &lgnum);
		p = strstr(p, "dtc");
		if (p)	sscanf(p, "dtc%d", &dtcnum);
	}
	if (lgnum != -1 && dtcnum != -1) {
		sprintf(temp, "/dev/dtc%d-%d", lgnum, dtcnum);
		symlink(block, temp);
        }
 }
#endif
    }

    umask(old_mask);
}

/*
 * Given a logical group number, create a link to the device file
 * generated by the driver.
 */
void
ftd_make_group(int lgnum, dev_t dev)
{
    char        group_name[MAXPATHLEN], temp[MAXPATHLEN];
    mode_t      mode, old_mask;

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (makedirs(group_name, 1, temp, mode) == 0) {

        mode = S_IWUSR | S_IRUSR | S_IRGRP;

        unlink(group_name);
        mknod(group_name, S_IFCHR | mode, dev);
    }
    umask(old_mask);
}

#elif defined(_AIX)

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/mntctl.h>
#include <sys/vmount.h>

#include "ftd_mount.h"
#include "ftd_cmd.h"
#include "aixcmn.h"

/* don't free(3) these... */
static char     timebuf[16];
static char     mnt_special_buf[MAXPATHLEN];
static char     mnt_mountp_buf[MAXPATHLEN];
static char     mnt_fstype_buf[16];
static char     mnt_mntopts_buf[16];

int
getmntany(FILE * fp, struct mnttab * mp, struct mnttab * mpref)
{
	struct mntent  *ent;
	int             count, found;
	int             mcerr;
	int             vmdesc;
	struct vmount  *vma=(struct vmount  *)NULL;
	struct vmount  *vmp;
	char           *vmlim;
	char           *vmtobjp;
	char           *vmtstbp;
	int             i, maxentry;

	/* check for stupidity */
	if ((mpref == NULL) || (mp == NULL)) {
		return EOF;
	}
	count = 0;
	if (mpref->mnt_special != NULL)
		count++;
	if (mpref->mnt_mountp != NULL)
		count++;
	if (mpref->mnt_fstype != NULL)
		count++;
	if (mpref->mnt_mntopts != NULL)
		count++;
	if (mpref->mnt_time != NULL)
		count++;

	/* query mntctl in descriptor mode */
	mcerr = mntctl(MCTL_QUERY, sizeof(vmdesc), (char *) &vmdesc);
	if (mcerr < 0)
		return (-1);

	/* vmdesc is the size of the buffer to allocate */
	vma = (struct vmount *) malloc(vmdesc);
	mcerr = mntctl(MCTL_QUERY, vmdesc, (char *) &vma[0]);
	if (mcerr <= 0) {
		if(vma)
			free(vma);
		return (-1);
	}

	/* scan for matching entry */
	vmp = vma;
	maxentry = mcerr;
	for (i = 0; i < maxentry; i++) {

		found = 0;

		vmtobjp = vmt2dataptr(vmp, VMT_OBJECT);
		vmtstbp = vmt2dataptr(vmp, VMT_STUB);

		{
			char           *typp;

			strncpy(mnt_special_buf, vmtobjp, MAXPATHLEN - 1);
			strncpy(mnt_mountp_buf, vmtstbp, MAXPATHLEN - 1);

			/* not sure how to represent fs types here... */
			switch (vmp->vmt_gfstype) {
			case MNT_AIX:
				typp = "oaix";
			case MNT_NFS:
				typp = "nfs";
				break;
			case MNT_JFS:
				typp = "jfs";
				break;
			case MNT_CDROM:
				typp = "cdrom";
				break;
			default:
				typp = "unknown";
				break;
			}
			strcpy(mnt_fstype_buf, typp);

			/* not sure how to represent flags here... */
			sprintf(mnt_mntopts_buf, "0x%08x", &vmp->vmt_flags);
		}
		if ((mpref->mnt_special != NULL) &&
		    (strcmp(vmtobjp, mpref->mnt_special) == 0)) {
			found++;
		}
		if ((mpref->mnt_mountp != NULL) &&
		    (strcmp(vmtstbp, mpref->mnt_mountp) == 0)) {
			found++;
		}
		if ((mpref->mnt_fstype != NULL) &&
		    (strcmp(vmp->vmt_gfstype, mpref->mnt_fstype) == 0)) {
			found++;
		}
		if ((mpref->mnt_mntopts != NULL) &&
		    (strcmp(vmp->vmt_flags, mpref->mnt_mntopts) == 0)) {
			found++;
		}
		if ((mpref->mnt_time != NULL) &&
		    ((atol(mpref->mnt_time) == vmp->vmt_time) == 0)) {
			found++;
		}
		if (found == count) {
			mp->mnt_special = mnt_special_buf;
			mp->mnt_mountp = mnt_mountp_buf;
			mp->mnt_fstype = mnt_fstype_buf;
			mp->mnt_mntopts = mnt_mntopts_buf;
			sprintf(timebuf, "%ld", vmp->vmt_time);
			mp->mnt_time = timebuf;
			if(vma)
				free(vma);
			return 0;
		}
		vmp = (struct vmount *) ((char *) vmp + vmp->vmt_length);
	}
	if(vma)
		free(vma);
	return (EOF);
}

/*
 * Recursively traverse a path, creating all of directories in the path.
 * Directories are created with the passed in mode. It is assumed the
 * umask has been properly set before calling this function. The passed
 * in path should not begin with '/'. The "temp" argument is temporary
 * storage (we could have used a stack variable, but why burn up stack
 * space). "start" is the beginning index into "path" to begin parsing.
 */
static int
makedirs(char *path, int start, char *temp, mode_t mode)
{
    int i, ret;

    for (i = start; i < strlen(path); i++) {
        if (path[i] == '/') {
            strncpy(temp, path, i);
            temp[i] = 0;
            ret = mkdir(temp, mode);
            if ((ret == -1) && (errno != EEXIST)) {
                return -1;
            } 
            makedirs(path, i+1, temp, mode);
            return 0;
        }
    }
    /* done. hit the end of the path */
    return 0;
}

/*
 * Given a raw device path, the group number, and the minor number of
 * the device, create two new pathnames for the raw and block devices
 * and link them to the devices created by the driver.
 * The directories preceding the symbolic links are created, too.
 *
 * The access mode for the devices is: rw-r-----
 */
void
ftd_make_devices(struct stat *cbuf, struct stat *bbuf, char *raw, char *block, dev_t dev)
 /* SS/IS activity */ 
{
    char        temp[MAXPATHLEN];
    mode_t      mode, old_mask;

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if ((makedirs(raw, 1, temp, mode) == 0) &&
        (makedirs(block, 1, temp, mode) == 0)) {
        
        unlink(raw);
        mknod(raw, cbuf->st_mode, dev);
        chown (raw, cbuf->st_uid, cbuf->st_gid);   /* SS/IS activity */

        unlink(block);
        mknod(block, bbuf->st_mode, dev);
        chown (block, bbuf->st_uid, bbuf->st_gid);  /* SS/IS activity */
    }

    umask(old_mask);
}

/*
 * Given a logical group number, create a link to the device file
 * generated by the driver.
 */
void
ftd_make_group(int lgnum, dev_t dev)
{
    char        group_name[MAXPATHLEN], temp[MAXPATHLEN];
    mode_t      mode, old_mask;

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (makedirs(group_name, 1, temp, mode) == 0) {

        mode = S_IWUSR | S_IRUSR | S_IRGRP;

        unlink(group_name);
        mknod(group_name, S_IFCHR | mode, dev);
    }
    umask(old_mask);
}

/* ftd_make_aix_links
 *
 * given a group # and dtc dev number, create a device name linked to the
 * raw and block paths to the dtc devices
 */
int ftd_make_aix_links(char *rawpath, char *blkpath, int lgnum, int dtcnum)
{
   char buf[MAXPATHLEN];
   int ret;

   sprintf(buf, "/dev/rlg%ddtc%d", lgnum, dtcnum);
   unlink(buf);
   ret = link(rawpath, buf);

   if (ret < 0) goto out;

   sprintf(buf, "/dev/lg%ddtc%d",lgnum,dtcnum);
   unlink(buf);
   ret = link(blkpath, buf);

out:
   return (ret);
}

/* Ftd_rm_aix_links
 *
 * given a group # and dtc dev number, create a device name linked to the
 * raw and block paths to the dtc devices
 */
void ftd_rm_aix_links(int lgnum, int dtcnum)
{
   char buf[MAXPATHLEN];

   sprintf(buf, "/dev/rlg%ddtc%d", lgnum, dtcnum);
   unlink(buf);

   sprintf(buf, "/dev/lg%ddtc%d",lgnum,dtcnum);
   unlink(buf);
}

/*
 * ftd_odm_add
 *
 * calls dtcupdateodm to add the short form (LVM compatible) device name
 * for our driver into odm
 *
 * Returns -1 on failure 0 on success
 */
int ftd_odm_add(char *blksrcpath, int lgnum, int dtcnum, char * blockshort)
{
   pid_t pid;
   int ret;

   sprintf(blockshort, "lg%ddtc%d", lgnum, dtcnum);

   blksrcpath=blksrcpath+5;  /* skip over "/dev/" portion of dev name */

   if (( pid = fork()) < 0)
      return (-1); /* fork error */
   else if ( pid == 0) {
      if (execl("/usr/dtc/libexec/dtcupdateodm","dtcupdateodm", "add", blockshort,
                blksrcpath, (char *) 0) < 0) {
          return (-1);
      }
   }
   if (waitpid(pid, NULL, 0) < 0)
   {
       return (-1);
   } else {
       return (0);
   }

}

/* ftd_odm_delete
 *
 * calls dtcupdateodm to delete the short form (LVM compatible) device name
 * for our driver from odm
 *
 * Returns -1 on failure 0 on success
 */
int ftd_odm_delete(int lgnum, int dtcnum, char * blockshort)
{
   pid_t pid;
   int ret;

   sprintf(blockshort, "lg%ddtc%d", lgnum, dtcnum);

   if (( pid = fork()) < 0)
      return (-1); /* fork error */
   else if ( pid == 0) {
      if (execl("/usr/dtc/libexec/dtcupdateodm","dtcupdateodm", "delete", blockshort,
                 (char *) 0) < 0) {
          return (-1);
      }
   }
   if (waitpid(pid, NULL, 0) < 0)
   {
       return (-1);
   } else {
       return (0);
   }

}

#endif

#if defined(linux)

/*
 * jch: can't seem to get this to work with aCC
*/
#undef _STDC_32_MODE__
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif

#include <sys/types.h>
#include <mntent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "errors.h"
#include "bigints.h"
#include "platform.h"
#include "ftd_mount.h"
#include "ftd_cmd.h"
#include "ftdio.h"

int
getmntany(FILE *fp, struct mnttab *mp, struct mnttab *mpref)
{
    struct mntent *ent;
    int count, found;
    static char timebuf[16];

    /* check for stupidity */
    if ((mpref == NULL) || (mp == NULL)) {
        return EOF;
    }
    count = 0;
    if (mpref->mnt_special != NULL) count++;
    if (mpref->mnt_mountp != NULL) count++;
    if (mpref->mnt_fstype != NULL) count++;
    if (mpref->mnt_mntopts != NULL) count++;
    if (mpref->mnt_time != NULL) count++;

    while ((ent = getmntent(fp)) != NULL) {
        found = 0;
        if ((mpref->mnt_special != NULL) &&
            (strcmp(ent->mnt_fsname, mpref->mnt_special) == 0)) {
            found++;
        }
        if ((mpref->mnt_mountp != NULL) && 
            (strcmp(ent->mnt_dir, mpref->mnt_mountp) == 0)) {
            found++;
        }
        if ((mpref->mnt_fstype != NULL) && 
            (strcmp(ent->mnt_type, mpref->mnt_fstype) == 0)) {
            found++;
        }
        if ((mpref->mnt_mntopts != NULL) && 
            (strcmp(ent->mnt_opts, mpref->mnt_mntopts) == 0)) {
            found++;
        }
        if ((mpref->mnt_time != NULL))  {  //&& 
	//member struct  mntent.mnt_time does'nt exist in R.H linux
          //  ((atol(mpref->mnt_time) == ent->mnt_time) == 0)) {
            found++;
        }
        if (found == count) {
            mp->mnt_special = ent->mnt_fsname;
            mp->mnt_mountp = ent->mnt_dir;
            mp->mnt_fstype = ent->mnt_type;
            mp->mnt_mntopts = ent->mnt_opts;
	//member struct  mntent.mnt_time does'nt exist in R.H linux
            //sprintf(timebuf, "%ld", ent->mnt_time);
            //mp->mnt_time = timebuf; 
            return 0;
        }
    }
    return EOF;
}

#endif /* defined(linux) */


/*
 * Waste the directories created for the group
 */
void
ftd_delete_group(int lgnum)
{
    char group_name[MAXPATHLEN];
    char dir_name[MAXPATHLEN];

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    unlink(group_name);

    /* waste the subdirs */
    FTD_CREATE_GROUP_DIR(group_name, lgnum);    
    sprintf(dir_name, "%s/dsk", group_name);
    rmdir(dir_name);
    sprintf(dir_name, "%s/rdsk", group_name);
    rmdir(dir_name);
    rmdir(group_name);
}

