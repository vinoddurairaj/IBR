/*
 * ftd_dev.c - FTD device interface
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
#include "ftd_devio.h"
#include "ftd_sock.h"
#include "ftd_dev.h"
#include "ftd_error.h"
#include "ftd_ps.h"
#include "ftdio.h"
#include "ftd_ioctl.h"
#include "disksize.h"
#include "ftd_platform.h"
#if defined(_WINDOWS)
#include "ftd_lg.h"
#include "ftd_devlock.h"
#include "volmntpt.h"
#endif

/*
 * ftd_dev_create -- create a ftd_dev_t object
 */
ftd_dev_t *
ftd_dev_create(void)
{
	ftd_dev_t *devp;

	if ((devp = (ftd_dev_t*)calloc(1, sizeof(ftd_dev_t))) == NULL)
	{
		reporterr(ERRFAC, M_MALLOC, ERRCRIT);
		return NULL;
	}

	if ((devp->statp =
		(ftd_dev_stat_t*)calloc(1, sizeof(ftd_dev_stat_t))) == NULL)
	{
		reporterr(ERRFAC, M_MALLOC, ERRCRIT);
		free(devp);
		return NULL;
	}

	devp->devfd = INVALID_HANDLE_VALUE;
	devp->ftddevfd = INVALID_HANDLE_VALUE;

	return devp;
}

/*
 * ftd_dev_delete -- delete a ftd_dev_t object
 */
int
ftd_dev_delete(ftd_dev_t *devp)
{
	
	if (devp->devfd && devp->devfd != INVALID_HANDLE_VALUE) {
		ftd_dev_close(devp);
		devp->devfd = INVALID_HANDLE_VALUE;
	}

	if (devp->ftddevfd && devp->ftddevfd != INVALID_HANDLE_VALUE) {
		ftd_dev_close(devp);
		devp->ftddevfd = INVALID_HANDLE_VALUE;
	}

	free(devp->statp);

	if (devp->deltamap) {
		FreeLList(devp->deltamap);
	}

	if (devp->sumbuf)
		free(devp->sumbuf);

	free(devp);

	return 0;
}

/*
 * ftd_dev_init -- initialize the ftd_dev_t object
 */
int
ftd_dev_init(ftd_dev_t *devp, int devid, char *rdevname,
	char *devname, int size, int num)
{
	ftd_dev_stat_t	*savestatp = devp->statp;
	LList			*savedeltamap = devp->deltamap;
	char			*savesumbuf = devp->sumbuf;
	char			*saversyncbuf = devp->rsyncbuf;
	unsigned char	*savedb = devp->db;
	HANDLE			savedevfd = devp->devfd;
	HANDLE			saveftddevfd = devp->ftddevfd;
	int				saveno0write = devp->no0write;

	memset(devp, 0, sizeof(ftd_dev_t));

	devp->statp = savestatp;
	devp->deltamap = savedeltamap;
	devp->sumbuf = savesumbuf;
	devp->rsyncbuf = saversyncbuf;
	devp->rsyncbuf = saversyncbuf;
	devp->db = savedb;
	devp->devfd = savedevfd;
	devp->ftddevfd = saveftddevfd;
	devp->no0write = saveno0write;

	devp->devid = devid;
	devp->devsize = size;

	devp->num = num;
	devp->ftdnum = num;

	return 0;
}

/*
 * ftd_dev_meta --
 * set meta flag if device is a Solaris meta device
 */
int
ftd_dev_meta(ftd_dev_cfg_t *devcfgp, ftd_dev_t *devp)
{
#if defined(SOLARIS)
	struct dk_cinfo	dkinfo;
	struct vtoc		dkvtoc;

	do {
#ifdef PURE
		memset(&dkinfo, 0, sizeof(dkinfo));
		memset(&dkvtoc, 0, sizeof(dkvtoc));
#endif
		if (ioctl(devp->devfd, DKIOCINFO, &dkinfo) < 0) {
			break;
		}
		if (ioctl(devp->devfd, DKIOCGVTOC, &dkvtoc) < 0) {
			break;
		}
		if (dkinfo.dki_ctype != DKC_MD
			&& dkvtoc.v_part[dkinfo.dki_partition].p_start == 0)
		{
			error_tracef( TRACEINF, "ftd_dev_meta():Sector 0 write dissallowed for device: %s", devcfgp->devname );
			devp->no0write = 1;
		}
	} while (0);	

#endif /* SOLARIS */

	return 0;
}

/*
 * ftd_dev_open --
 * open target device and set initial state
 */
int
ftd_dev_open(ftd_dev_cfg_t *cfgp, int lgnum, ftd_dev_t *devp, int modebits, int permisbits, int role)
{
	char	*devname;
	char    dosname[_MAX_PATH];
	char    szVolumeName[MAX_PATH];

    //
    // SVG2004
    // Added:
    // If we have a valid handle, and we are a secondary,
    // get out of here!!
    //
    if (devp->devfd != INVALID_HANDLE_VALUE)
    {
        if (role != ROLEPRIMARY)
        {
            // SVG 2004
            //
            // This is a secondary attempting an open on something already open!!
            //
            // get out of here!
            return 0;
        }

    }

	devp->devfd = INVALID_HANDLE_VALUE;

	if (role == ROLEPRIMARY) 
	{
		devname = cfgp->pdevname;

		if (modebits & O_EXCL) 
		{ /* flag to tell me dev_lock and share */
			devp->devfd = ftd_dev_lock(devname, lgnum);
		} 
		else 
		{
			if ( strlen( devname ) == 2)     /* Drive Letter  */
			{
				sprintf(dosname, "\\\\.\\%s", devname);
				devname = dosname;
				devp->devfd = ftd_open(devname, modebits, permisbits);
			}
			else                             /* Mount Point   */
			{
				memset(szVolumeName, 0, MAX_PATH);
				if (!getVolumeNameForVolMntPt( devname,   /* ex H:\MountPoint1\ */
											   szVolumeName,  
											   MAX_PATH ))
					return -1;

				szVolumeName[48] = 0;
				devp->devfd = ftd_open(szVolumeName, modebits, permisbits);
			}
		}
	} 
	else 
	{
		devname = cfgp->sdevname;
		devp->devfd = ftd_dev_lock(devname, lgnum);
	}

	if (devp->devfd == INVALID_HANDLE_VALUE) 
	{
		return -1;
	}

	return 0;
}

/*
 * ftd_dev_ftd_open --
 * open target ftd device and set initial state
 */
int
ftd_dev_ftd_open(ftd_dev_cfg_t *cfgp, ftd_dev_t *devp,
	int modebits, int permisbits, int role)
{
	char	*devname;
	char    dosname[_MAX_PATH];
	char    szVolumeName[MAX_PATH];

	if (role != ROLEPRIMARY) 
	{
		return 0;
	}

	devp->ftddevfd = INVALID_HANDLE_VALUE;
	devname = cfgp->devname;

	if ( strlen( devname ) == 2)     /* Drive Letter  */
	{
		sprintf(dosname, "\\\\.\\%s", devname);
		devname = dosname;
		devp->ftddevfd = ftd_open(devname, modebits, permisbits);
	}
	else                             /* Mount Point   */
	{
		memset(szVolumeName, 0, MAX_PATH);
		if (!getVolumeNameForVolMntPt( devname,   /* ex H:\MountPoint1\ */
									   szVolumeName,  
									   MAX_PATH ))
			return -1;

		szVolumeName[48] = 0;
		devp->ftddevfd = ftd_open(szVolumeName, modebits, permisbits);
	}

	if (devp->ftddevfd == INVALID_HANDLE_VALUE) 
	{
		return -1;
	}
	return 0;
}

/*
 * ftd_dev_close --
 * close target device
 */
int ftd_dev_close(ftd_dev_t *devp)
{
	
	if (devp->devfd && devp->devfd != INVALID_HANDLE_VALUE)
	{
#if defined(_WINDOWS)
        ftd_dev_unlock(devp->devfd);
#else
		FTD_CLOSE_FUNC(__FILE__,__LINE__,devp->devfd);
#endif
		devp->devfd = INVALID_HANDLE_VALUE;
	}
    
	return 0;
}

/*
 * ftd_dev_ftd_close --
 * close target ftd device
 */
int ftd_dev_ftd_close(ftd_dev_t *devp)
{
	
	if (devp->ftddevfd && devp->ftddevfd != INVALID_HANDLE_VALUE)
	{
#if defined(_WINDOWS)
        ftd_dev_unlock(devp->ftddevfd);
#else
		FTD_CLOSE_FUNC(__FILE__,__LINE__,devp->ftddevfd);
#endif
		devp->ftddevfd = INVALID_HANDLE_VALUE;
	}
    
	return 0;
}

int
ftd_dev_write(ftd_dev_t *devp, char *buf, ftd_uint64_t offset, int length,
	char *devname)
{
	if (offset == 0 && devp->no0write) 
	{
		error_tracef( TRACEINF, "ftd_dev_write():skipping sector 0 write: %s", devname );
		offset += DEV_BSIZE;
		buf += DEV_BSIZE;
		length -= DEV_BSIZE;
	}

	if (length > 0)
	{
		//DPRINTF((ERRFAC,LOG_INFO," dev_write: fd-%d, devname-%s: %d bytes @ offset %llu\n",			devp->devfd, devname, length, offset));

		if (ftd_llseek(devp->devfd, offset, SEEK_SET) == (ftd_uint64_t)-1) 
		{
			reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
				devname,
				offset,
				ftd_strerror());
			return -1;
		}

		if (ftd_write(devp->devfd, buf, length) != length) 
		{
			DebugTrace("ftd_write(0x%8x), Name:%s  ID:%d OFF:0x%8x L:0x%8x\n", devp->devfd, devname, 
				        devp->devid, offset, length);
			reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
				devname,
				devp->devid,
				offset,
				length,
				ftd_strerror()); 
			return -1;
		}
	}

	return 0;
}

static int
ftd_dev_get_lrdb(char *ps_name, char *dev_name, unsigned int size,
    int **lrdb, unsigned int *lrdb_bits)
{
	int	ret, *buffer;

	/* allocate a buffer */
	if ((buffer = (int *)malloc(size)) == NULL) {
		return PS_MALLOC_ERROR;
	}
	
	ret = ps_get_lrdb(ps_name, dev_name, (caddr_t)buffer, size, lrdb_bits);
    
	if (ret != PS_OK) {
		free(buffer);
		return ret;
	}
	*lrdb = buffer;
    
	return PS_OK;
}

static int
ftd_dev_get_hrdb(char *ps_name, char *dev_name, unsigned int size,
    int **hrdb, unsigned int *hrdb_bits)
{
    int  ret;
    int *buffer;

    // allocate a buffer
    if ((buffer = (int *)malloc(size)) == NULL) {
        return PS_MALLOC_ERROR;
    }
    ret = ps_get_hrdb(ps_name, dev_name, (caddr_t)buffer, size, hrdb_bits);
    if (ret != PS_OK) {
        free(buffer);
        return ret;
    }
    *hrdb = buffer;
    
    return PS_OK;
}

static void
ftd_dev_set_bits(unsigned int *ptr, int x1, int x2)
{
    unsigned int *dest;
    unsigned int mask, word_left, n_words;

    word_left = WORD_BOUNDARY(x1);
    dest = ptr + word_left;
    mask = START_MASK(x1);
    if ((n_words = (WORD_BOUNDARY(x2) - word_left)) == 0) {
        mask &= END_MASK(x2);
        *dest |= mask;
        return;
    }
    *dest++ |= mask;
    while (--n_words > 0) {
        *dest++ = 0xffffffff;
    }
    *dest |= END_MASK(x2);
    
	return;
}

/*
 * ftd_dev_regen_hrdb - 
 * re-generate the hrdb from the lrdb
 */
int
ftd_dev_regen_hrdb(HANDLE ctlfd, HANDLE devfd, int lgnum,
	ftd_dev_info_t *devp, dev_t cdev, int *lrdb, int *hrdb)
{
	int				i, j, k, num32, bitnum;
	unsigned int	value, expansion;
	
	if (ftd_ioctl_get_devices_info(ctlfd, lgnum, devp)) {
		goto errret;
	}

	for (i = 0; i < (FTD_MAX_GROUPS*FTD_MAX_DEVICES); i++) {
		if (cdev == devp[i].cdev) {
			break;
		}
	}

	if (i == (FTD_MAX_GROUPS*FTD_MAX_DEVICES)) {
		// this can't happen - we added the dev above 
		reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", 
			ftd_strerror());
		goto errret;
	}

	// this should be a power of 2 
	expansion = 1 << (devp[i].lrdb_res - devp[i].hrdb_res);
#if 0
	printf("\n$$$ disk size, lrdb_numbits, hrdb_numbits, lrdb_res, hrdb_res, expansion = %d, %d, %d, %d, %d, %d\n",
		devp[i].disksize,
		devp[i].lrdb_numbits,
		devp[i].hrdb_numbits,
		devp[i].lrdb_res,
		devp[i].hrdb_res, expansion); 
#endif
	// zero it out 
	memset(hrdb, 0, FTD_PS_HRDB_SIZE);

	// lrdb_numbits to words, att.hrdb_size (bytes) to words
	num32 = ((devp[i].lrdb_numbits / 32) + 1) < ((FTD_PS_HRDB_SIZE / 4) / (int)expansion) ?
		((devp[i].lrdb_numbits / 32) + 1): ((FTD_PS_HRDB_SIZE / 4) / (int)expansion);
	//num32 = min( (devp[i].lrdb_numbits / 32) + 1, (FTD_PS_HRDB_SIZE / 4) / expansion );

	// march thru bitmap one 32-bit word at a time
	for (j = 0; j < num32; j++) {
		// blow off empty words 
		if ((value = lrdb[j]) == 0) continue;

		// test each bit 
		bitnum = j * expansion * 32;
		for (k = 0; k < 32; k++) {
			if ((value >> k) & 1) {
				ftd_dev_set_bits((unsigned int*)hrdb, bitnum + k*expansion,
					bitnum + ((k+1)*expansion)-1);
			}
		} 
	}


	return 0;

errret:

	return -1;
}

/*
 * ftd_dev_add - 
 * create the device and add state to pstore and driver
 */
int
ftd_dev_add(HANDLE ctlfd, HANDLE devfd, char *ps_name, char *group_name,
	int lgnum, char *rdevname, char *devname, int grpstarted,
	int regen_hrdb, int *devidx, unsigned int size, ftd_dev_info_t *devp)
{
	ps_version_1_attr_t	attr;
    ps_dev_info_t       dev_info;
    ftd_dev_info_t      info;
    dev_t               cdev, bdev;
    int                 index, ret, *lrdb = NULL, *hrdb = NULL, need_regen = 0;
    unsigned int        hrdb_bits, lrdb_bits;
    char                *buffer = NULL, raw[MAXPATHLEN];
#if defined(HPUX) || defined(_WINDOWS)
    ftd_devnum_t        dev_num;
#endif
#if defined(_WINDOWS)
	BOOL				bDismounted = FALSE;
#else 
	char				block[MAXPATHLEN];
    struct stat         sb;
#endif

	// create the device names 
#if !defined(_WINDOWS)
	strcpy(raw, devname);
	force_dsk_or_rdsk(block, raw, 0);
#else
	FTD_CREATE_GROUP_DIR(raw, lgnum);
	strcat(raw, "/");
	strcat(raw, devname);
#endif

	// get the header to find out the max size of the dirty bitmaps 
	ret = ps_get_version_1_attr(ps_name, &attr);
	
	if (ret != PS_OK) {
		goto errret;
	}
	
	if ((buffer = (char *)calloc(1, attr.dev_attr_size)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.dev_attr_size);
		goto errret;
	}

	// get the attributes of the device from the pstore
	ret = ps_get_device_attr(ps_name, raw, buffer, attr.dev_attr_size);
	
	if (ret == PS_OK) {
		if (ftd_dev_get_lrdb(ps_name, raw, attr.lrdb_size,
			&lrdb, &lrdb_bits) != PS_OK)
		{
			goto errret;
		}

		if (ftd_dev_get_hrdb(ps_name, raw, attr.hrdb_size,
			&hrdb, &hrdb_bits) != PS_OK)
		{
			goto errret;
		}

		// if the hrdb is bogus, create a new one 
		if (regen_hrdb) {
			need_regen = 1;
		}
	} else if (ret == PS_DEVICE_NOT_FOUND) {
		
		// if attributes don't exist, add the device to the persistent store 
		dev_info.name = raw;

		// FIXME: how do we override/calculate these values? 
		dev_info.num_lrdb_bits = attr.lrdb_size * 8;
		dev_info.num_hrdb_bits = attr.hrdb_size * 8;

		// set up default LRDB and HRDB
		if ((lrdb = (int *)malloc(attr.lrdb_size)) == NULL) {
			reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.lrdb_size);
			goto errret;
		}

		if ((hrdb = (int *)malloc(attr.hrdb_size)) == NULL) {
			reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.hrdb_size);
			goto errret;
		}
		
		memset((caddr_t)lrdb, 0xff, attr.lrdb_size);
		memset((caddr_t)hrdb, 0xff, attr.hrdb_size);
		
		ret = ps_create_device(ps_name, &dev_info,
			lrdb, attr.lrdb_size, hrdb, attr.hrdb_size);
		
		if (ret != PS_OK) {
			if (ret == PS_NO_ROOM) {
				reporterr(ERRFAC, M_PSNOROOM, ERRCRIT,
					ps_name, dev_info.name);
			}
			goto errret;
		}
		
	} else {
		// other error. we're hosed. 
		goto errret;
	}

	// get the unique index for this device 
	ret = ps_get_device_index(ps_name, raw, &index);
	if (ret != PS_OK) {
		goto errret;
	}

	if (ftd_ioctl_get_device_nums(ctlfd, &dev_num) != 0) {
		goto errret;
	}

	index = dev_num.b_major;
	cdev = index;
	bdev = index;

	info.lgnum = lgnum;
	info.cdev = cdev;
	info.bdev = bdev;

	info.vdevname = rdevname;
	info.devname = raw;

	info.disksize = size;
	info.lrdbsize32 = attr.lrdb_size / 4;      /* number of 32-bit words */
	info.hrdbsize32 = attr.hrdb_size / 4; /* number of 32-bit words */
	info.statsize = attr.dev_attr_size; 

	ps_get_lrdb_offset(ps_name, raw, &info.lrdb_offset);

	// set devid's and size in info_t struct
	*devidx = index;

	devp[index].bdev = bdev;
	devp[index].cdev = cdev;
	devp[index].disksize = info.disksize;

    if (ftd_ioctl_new_device(ctlfd, &info, 1) != 0) 
    {
		int errnum = ftd_errno();

        if (!(grpstarted && errnum == EADDRINUSE)) 
        {
			char	buf[256];

			sprintf(buf, "partition:%s, volume device:%s - %s",
                devname, info.vdevname, "Unable to insert our driver into the stack for this device\n");
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "NEW DEVICE", buf);
			goto errret;
		}
	}
#if defined(SOLARIS) || defined(_AIX)
	ftd_make_devices(raw, block, bdev);
#endif

	// add the attributes to the driver 
	if (ftd_ioctl_set_dev_state_buffer(ctlfd, lgnum, cdev,
		attr.dev_attr_size, buffer, 0) != 0)
	{
        goto errret;
	}

	// set the LRDB in the driver 
	if (ftd_ioctl_set_lrdbs_from_buf(ctlfd, devfd, lgnum,
		&cdev, lrdb, attr.lrdb_size))
	{
		goto errret;
    }

	// regen the HRDB 
	// we have bitsize set (by new_device) at this point 
    if (need_regen) 
    {
		if (ftd_dev_regen_hrdb(ctlfd, devfd, lgnum, devp, cdev,
			lrdb, hrdb) < 0)
		{
			goto errret;
		}
	}

	// set the HRDB in the driver 
	if (ftd_ioctl_set_hrdbs_from_buf(ctlfd, devfd, lgnum,
		&cdev, hrdb, attr.hrdb_size))
	{
		goto errret;
    }

	if (buffer) {
		free(buffer);
	}
	if (hrdb) {
		free(hrdb);
	}
	if (lrdb) {
		free(lrdb);
	}
    
	return 0;

errret:

	if (buffer) {
		free(buffer);
	}
	if (hrdb) {
		free(hrdb);
	}
	if (lrdb) {
		free(lrdb);
	}

	ps_delete_device(ps_name, raw);

	return -1;
}

/*
 * ftd_dev_rem - 
 * remove the device from pstore and driver
 */
int
ftd_dev_rem(HANDLE ctlfd, HANDLE lgfd, int lgnum, u_char *lrdb, u_char *hrdb,
	char *ps_name, char *groupname, char *devname, int dev_num,
	int state, int ndevs, ftd_dev_info_t *dip, int silent)
{
	char			blockname[MAXPATHLEN], *devbuf = NULL;
	u_char			*lrdbp, *hrdbp;
#if !defined(_WINDOWS)
	struct stat		statbuf;
	pid_t			pid;
	int				pcnt, index;
#endif
	int				lrdboff, hrdboff, i;
	ftd_dev_info_t	*mapp;
		
	// create the device names 
	force_dsk_or_rdsk(blockname, devname, 0);

	// if state PASSTHRU, ignore the LRDB and HRDB 
	if (state != FTD_MODE_PASSTHRU) {
		
		// find device in dbarray dev list
		lrdboff = hrdboff = 0;

		for (i = 0; i < ndevs; i++) {
			mapp = dip + i;
			if (mapp->cdev == (unsigned int)dev_num) {
				break;
			}
			lrdboff += mapp->lrdbsize32; 
			hrdboff += mapp->hrdbsize32; 
		}
			
		if (i == ndevs) {
			reporterr(ERRFAC, M_NOHRT, ERRCRIT,	devname, dev_num);
			goto errret;
		}
			
		lrdbp = lrdb + (lrdboff * sizeof(int));
		hrdbp = hrdb + (hrdboff * sizeof(int));

		// put the LRDB into the persistent store 
		if (ps_set_lrdb(ps_name, devname, (char*)lrdbp,
			(FTD_PS_LRDB_SIZE / 4)) != PS_OK)
		{
			reporterr(ERRFAC, M_PSWTLRDB, ERRCRIT, devname, ps_name);
			goto errret;
		}

		// put the HRDB into the persistent store
		if (ps_set_hrdb(ps_name, devname, (char*)hrdbp,
			(FTD_PS_HRDB_SIZE / 4)) != PS_OK)
		{
			reporterr(ERRFAC, M_PSWTHRDB, ERRCRIT, devname, ps_name);
			goto errret;
		}
	}

	if ((devbuf = (char*)calloc(1, FTD_PS_DEVICE_ATTR_SIZE)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, FTD_PS_DEVICE_ATTR_SIZE);
		goto errret;
	}
	
	// get the attributes 
	if (ftd_ioctl_get_dev_state_buffer(ctlfd, lgnum, dev_num,
		FTD_PS_DEVICE_ATTR_SIZE, devbuf, silent) < 0)
	{
		goto errret;
	}
	
	// put the attributes into the persistent store 
	if (ps_set_device_attr(ps_name,	devname, devbuf, FTD_PS_DEVICE_ATTR_SIZE) != PS_OK) {
		reporterr(ERRFAC, M_PSWTDATTR, ERRCRIT, devname, ps_name);
		goto errret;
	}

	// delete the device from the group 
	if (ftd_ioctl_del_device(ctlfd, dev_num) < 0) {
		reporterr(ERRFAC, M_DRVDELDEV, ERRCRIT,
			devname, groupname);
		goto errret;
	}

#if !defined(_WINDOWS) // this is done in the driver for NT
	// delete the devices
	unlink(block);
	unlink(devname);
#endif

	free(devbuf);
	
	return 0;

errret:

	if (devbuf) {
		free(devbuf);
	}

	return -1;
}

