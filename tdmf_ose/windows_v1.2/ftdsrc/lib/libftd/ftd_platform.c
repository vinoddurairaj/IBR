/*
 * ftd_platform.c - FTD platform specific functions  
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
#include "ftd_platform.h"

#if defined(HPUX)

/*
 * jch: can't seem to get this to work with aCC
 */
#undef _STDC_32_MODE__
#define _LARGEFILE64_SOURCE

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
            if ((ret == -1) && (ftd_errno() != EEXIST)) {
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
ftd_make_devices(char *raw, char *block, dev_t raw_major_num, 
                 dev_t block_major_num, int minor_num, dev_t *raw_dev_out, 
                 dev_t *block_dev_out)
{
    int         ret;
    char        temp[MAXPATHLEN];
    mode_t      mode, old_mask;
    dev_t       cdev, bdev;
    struct stat statbuf;

	/* always return these */
    bdev = makedev(block_major_num, minor_num);
    cdev = makedev(raw_major_num, minor_num);
    *block_dev_out = bdev;
    *raw_dev_out = cdev;

    /* clear out the umask so we can set the permissions */
    old_mask = umask(0);

    /* mode for directory creation */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    /* create the directories above the devices */
    if ((makedirs(raw, 1, temp, mode) == 0) &&
        (makedirs(block, 1, temp, mode) == 0)) {

        mode = S_IWUSR | S_IRUSR | S_IRGRP;

        /* if device doesn't exist ... */ 
        if (stat(raw, &statbuf) != 0) {
            mknod(raw, _S_IFCHR | mode, cdev);
        /* if device id is wrong ... */
        } else if (statbuf.st_rdev != cdev) {
            /* waste it */
            unlink(raw);
            mknod(raw, _S_IFCHR | mode, cdev);
        }

        /* if device doesn't exist ... */ 
        if (stat(block, &statbuf) != 0) {
            mknod(block, _S_IFBLK | mode, bdev);
        /* if device id is wrong ... */
        } else if (statbuf.st_rdev != bdev) {
            /* waste it */
            unlink(block);
            mknod(block, _S_IFBLK | mode, bdev);
        }
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

#elif defined(SOLARIS)

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
            if ((ret == -1) && (ftd_errno() != EEXIST)) {
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
ftd_make_devices(char *raw, char *block, dev_t dev)
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
        mknod(raw, S_IFCHR | mode, dev);
        unlink(block);
        mknod(block, S_IFBLK | mode, dev);
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
	vmlim = ((char *) vma) + vmdesc;
	while ((char *) vmp < vmlim) {

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
            if ((ret == -1) && (ftd_errno() != EEXIST)) {
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
ftd_make_devices(char *raw, char *block, dev_t dev)
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
        mknod(raw, S_IFCHR | mode, dev);
        unlink(block);
        mknod(block, S_IFBLK | mode, dev);
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

#elif defined(_WINDOWS)

#include "ftd_port.h"
#include "ftdio.h"

/*
 * Given a logical group number, create a link to the device file
 * generated by the driver.
 */
int
ftd_make_group(int lgnum, dev_t major_num, dev_t *dev_out)
{
    dev_t       dev;

    dev = lgnum | FTD_LGFLAG;
    *dev_out = dev;

    return 0;
}

int
ftd_make_group_event(int lgnum, dev_t major_num, dev_t *dev_out)
{
    char        group_name[MAXPATHLEN];
    dev_t       dev;

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    dev = lgnum | FTD_LGFLAG;
    *dev_out = dev;

    return 0;
}

int
ftd_make_devices(char *raw, char *block, dev_t raw_major_num, 
                 dev_t block_major_num, int minor_num, dev_t *raw_dev_out, 
                 dev_t *block_dev_out)
{
    dev_t       cdev, bdev;

	/* always return these */
    bdev = minor_num;
    cdev = minor_num;
    *block_dev_out = bdev;
    *raw_dev_out = cdev;

    return 0;
}

#endif

#if !defined(_WINDOWS)
/*
 * Waste the directories created for the group
 */
void
ftd_delete_group(int lgnum)
{
    char group_name[MAXPATHLEN];
    char dir_name[MAXPATHLEN];

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* waste the subdirs */
    FTD_CREATE_GROUP_DIR(group_name, lgnum);    
    sprintf(dir_name, "%s/dsk", group_name);
    unlink(dir_name);

    sprintf(dir_name, "%s/rdsk", group_name);
    unlink(dir_name);

    unlink(group_name);
}

#endif

