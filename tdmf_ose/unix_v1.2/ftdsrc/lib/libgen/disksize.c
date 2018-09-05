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
 * disksize -- returns the size of the disk.
 *
 * (c) Copyright 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the high level functionality for the PMD
 *
 ***************************************************************************
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "common.h"

#if defined(SOLARIS)

#include <sys/dkio.h>
#include "errors.h"
#include <sys/vtoc.h>

#ifdef EFI_SUPPORT
#include <sys/efi_partition.h>
#endif

#elif defined(HPUX)

#include <sys/diskio.h>
#include <sys/param.h>

#elif defined(_AIX)

#include <sys/lvdd.h>           /* LV_INFO ioctl usage */
#include <sys/devinfo.h>        /* IOCINFO devinfo usage */
#include <sys/ioctl.h>          /* IOCINFO ioctl usage */
#include <lvm.h>                /* LV_INFO ioctl usage */
#include "aixcmn.h"
#include "errors.h"

#endif

#if defined (linux)
#include <linux/fs.h> // For BLKGETSIZE
#endif

#define SECTORS_PER_TERABYTE (1LL << 31)
#define FS_SIZE_UPPER_LIMIT  0x100000000000LL

// Enable workaround when compiling for 32 bits on zLinux
// When compiling for 64 bits, s390x is defined.
// Another possible 32/64 bits trigger would have been the size/maximum value the unsigned long type.
#if defined (linux) && defined (__s390__) && ! defined(s390x)
#warning "Overriding a gcc on s390 bug.  See comments for details."
/**
 *
 * When compiling for 32 bits in the following environment:
 * Red Hat Enterprise Linux AS release 4 (Nahant Update 3 and 6) 
 * gcc (GCC) 3.4.5 20051201 (Red Hat 3.4.5-2)
 *
 * We'd get the following gcc error:
 *
 * disksize.c: In function `brute_force_get_device_size':
 * disksize.c:131: error: insn does not satisfy its constraints:
 * (insn 362 57 115 10 disksize.c:81 (set (reg:DI 2 %r2)
 * (const_int 4294967296 [0x100000000])) 52 {*movdi_31} (nil)
 * (nil))
 * disksize.c:131: internal compiler error: in reload_cse_simplify_operands, at postreload.c:391
 * Please submit a full bug report,
 * with preprocessed source if appropriate.
 * See <URL:http://bugzilla.redhat.com/bugzilla> for instructions.
 * Preprocessed source stored into /tmp/ccmkIQBo.out file, please attach this to your bugreport.
 *
 * There were two possible workarounds. 1, to turn off optimization. 2, to change the value of the incremental size
 * to something much lower that would put the problem aside.
 *
 **/

#define DB_OFFSET_INCREMENT_SIZE  (1024ULL * 100ULL)
#else
#define DB_OFFSET_INCREMENT_SIZE  (SECTORS_PER_TERABYTE * 2)
#endif

/*
 * @todo Isn't there a better way than brute force to obtain this kind of information?
 *       What about just llseeking to SEEK_END to obtain the device size?
 *       Check out implementations of df or fdisk to see how they do it for block devices.
 *
 * @internal I doubdt that the comments referring to the sizes of 1TB/16, 16TB and so on are still valid.
 */
u_longlong_t brute_force_get_device_size(int fd)
{
    u_longlong_t    min_fail = 0;
    u_longlong_t    max_succeed = 0;
    u_longlong_t    cur_db_off;
    char            buf[DEV_BSIZE];

    /*
     * First, see if we can read the device at all, just to
     * eliminate errors that have nothing to do with the
     * device's size.
     */

    if (((llseek(fd, (offset_t)0, SEEK_SET)) == -1) ||
        ((read(fd, buf, DEV_BSIZE)) == -1))
    {
      return (0);  /* can't determine size */
    }
   /*
    * Now, go sequentially through the multiples of 1TB/16
    * to find the first read that fails (this isn't strictly
    * the most efficient way to find the actual size if the
    * size really could be anything between 0 and 2**64 bytes.
    * We expect the sizes to be less than 16 TB for some time,
    * so why do a bunch of reads that are larger than that?
    * However, this algorithm *will* work for sizes of greater
    * than 16 TB.  We're just not optimizing for those sizes.)
    */
    for (cur_db_off = DB_OFFSET_INCREMENT_SIZE;
         min_fail == 0 && cur_db_off < FS_SIZE_UPPER_LIMIT;
         cur_db_off += DB_OFFSET_INCREMENT_SIZE)
    {
        if (((llseek(fd, (offset_t)(cur_db_off * DEV_BSIZE), SEEK_SET)) == -1) ||
            ((read(fd, buf, DEV_BSIZE)) != DEV_BSIZE))
        {
                min_fail = cur_db_off;
        }
        else
        {
                max_succeed = cur_db_off; 
        }
    }
    
    if (min_fail == 0)
    {
       return (0);
    }

    /*
     * We now know that the size of the device is less than
     * min_fail and greater than or equal to max_succeed.  Now
     * keep splitting the difference until the actual size in
     * sectors in known.  We also know that the difference
     * between max_succeed and min_fail at this time is
     * 4 * SECTORS_PER_TERABYTE, which is a power of two, which
     * simplifies the math below.
     */

    while (min_fail - max_succeed > 1) 
    {
        cur_db_off = max_succeed + (min_fail - max_succeed)/2;
    
        if (((llseek(fd, (offset_t)(cur_db_off * DEV_BSIZE), SEEK_SET)) == -1) ||
            ((read(fd, buf, DEV_BSIZE)) != DEV_BSIZE))
        {
                min_fail = cur_db_off;
        }
        else
        {
                max_succeed = cur_db_off; 
        }
    }
    
    /* fd is again set to the  beginning of the file */
    llseek(fd, (offset_t)0, SEEK_SET); 

    /* the size is the last successfully read sector offset plus one */
    return (max_succeed + 1);
}

/*--------------------------------------------------------------------------------------------------------*/
/* brute_force_get_device_size() called by fdisksize() does not work on IBM SDD vpath devices (pc080116) */
/* This function here is a temporary measure before we fix the brute_force_get_device_size() function. */
u_longlong_t  vpath_fdisksize(int fd) 
/*................... Solaris ...............*/
#if defined(SOLARIS)
{

    u_longlong_t     size; 
    struct dk_cinfo  dkinfo;
    struct vtoc      dkvtoc;
    unsigned long    sectsiz; 
    u_longlong_t     tsize;
    int32_t          ret; 
#ifdef EFI_SUPPORT
    struct dk_gpt    *efip = NULL; 
#endif
    int32_t          newvtoc = 0; 
    int32_t          idx = 0; 
#ifdef PURE
    /* Yow!  We find a pure bug with the following IOCTLs. */
    size = 0;
    memset(&dkinfo, 0, sizeof(dkinfo));
    memset(&dkvtoc, 0, sizeof(dkvtoc));
#endif
    if(FTD_IOCTL_CALL(fd, DKIOCINFO, &dkinfo) < 0) {
        return -1;
    }

    /* Get the size of all partitions */
    if ((ret = read_vtoc(fd, &dkvtoc)) >= 0)
    {
      sectsiz = (unsigned long)(dkvtoc.v_sectorsz > 0) ? dkvtoc.v_sectorsz : DEV_BSIZE;
      tsize = (u_longlong_t) ((uint32_t)dkvtoc.v_part[dkinfo.dki_partition].p_size *
              (u_longlong_t) sectsiz);
      size = (u_longlong_t) ((u_longlong_t)tsize / (u_longlong_t) 512);
    }
#ifdef EFI_SUPPORT
    else if (ret == VT_ENOTSUP)
    {
      if ((ret = efi_alloc_and_read(fd, &efip)) < 0)
      {
         return -1;
      }
      newvtoc = 1;
    }
#endif    
    else
    {
      return -1;
    }
#ifdef EFI_SUPPORT
    if (newvtoc > 0)
    {
      for (idx = 0; idx < efip->efi_nparts; idx++)
      {
        if (efip->efi_parts[idx].p_size == 0)
        {
          continue;
        }
        newvtoc++;
        sectsiz = (unsigned long) efip->efi_lbasize;
        tsize += (u_longlong_t) efip->efi_parts[idx].p_size *
                     (u_longlong_t) sectsiz;
      }
      if (newvtoc > 1)
      {
        size = (u_longlong_t) ((u_longlong_t)tsize / (u_longlong_t) 512);
      }
      else
      {
        return -1;
      }
    }
#endif
    return size;
}


/*................... Linux  ...............*/
#elif defined(linux)
{
   unsigned long nsectors=0; 
    
    /* Get #blocks (512B) for /dev/<device name> */
    if (FTD_IOCTL_CALL(fd, BLKGETSIZE, &nsectors) < 0)
        return(-1);
   return ((u_longlong_t)nsectors); 
}




/*................... HPUX  ...............*/
#elif defined(HPUX)
/*
 * Return size of disk in DEV_BSIZE (1024) byte blocks.
 */
{
    capacity_type cap;

    if (FTD_IOCTL_CALL(fd, DIOC_CAPACITY, &cap) < 0) {
        return -1;
    }
    return (cap.lba);
}




/*................... AIX  ...............*/
#elif defined(_AIX)
/* return DEV_BSIZE byte blocks of an lv */
{
	int             iocerr;
	u_longlong_t    size;
	int             bsiz_ratio;
	struct devinfo devi;

	iocerr = FTD_IOCTL_CALL(fd, IOCINFO, &devi);
	if (iocerr)
		return (-1);

	/*  disk and logical volume */



#if defined(DS_VM) && defined(DS_LVZ)
/* LVM, VxVM, and other */
#define SUBTYPE_TEST (devi.devsubtype == DS_LV || devi.devsubtype == DS_VM || devi.devsubtype == DS_LVZ )
#elif defined(DS_VM)
/* LVM and VxVM support */
#   define SUBTYPE_TEST (devi.devsubtype == DS_LV || devi.devsubtype == DS_VM)
#else
#   define SUBTYPE_TEST (devi.devsubtype == DS_LV)
#endif

	if (devi.devtype == DD_DISK && SUBTYPE_TEST)
	{
#ifdef DF_LGDSK
	    if (devi.flags & DF_LGDSK)
	    {
	        size = (uint32_t)devi.un.dk64.hi_numblks;
                size <<= 32;
                size |= (uint32_t)devi.un.dk64.lo_numblks;
            }
	    else
#endif
	    {
                size = (u_longlong_t)(devi.un.dk.numblks);
	    }
	}
        else if (devi.devtype == DD_SCDISK && devi.devsubtype == DS_PV)
	{
#ifdef DF_LGDSK
	    if (devi.flags & DF_LGDSK)
	    {
	       size = (uint32_t)devi.un.scdk64.hi_numblks;
               size <<= 32;
               size |= (uint32_t)devi.un.scdk64.lo_numblks;
            }
	    else
#endif
	    {
               size = (u_longlong_t)(devi.un.scdk.numblks);
            }
	}
	else
	    return -1;

	return ((u_longlong_t)size);
}


#endif      /* <---------- AIX */
/*.................................................*/

/**
 * @todo Review the whole brute_force_get_device_size() and vpath_fdisksize() mechanisms to obtain the disk size for 2.6.4.
 *       When doing so, change the agent's code so that the same code and techniques are shared with it.
 *
 *       The main problem with the brute force approach is that it is inneficient and slow, especially on busy disks.
 *       Its main advantage is that it is a nearly generic approach.
 *       See vpath related comments in disksize() on where its genericity fails.
 *
 *       The vpath_fdisksize()'s problem is that it reportedly doesn't work on at least Solaris VxVM and
 *       Solaris volume manager volumes > 2TB.
 *       Its advantage is that it's efficient.
 */
u_longlong_t fdisksize(int fd)
{
        u_longlong_t    size = 0;

        size = brute_force_get_device_size(fd);
        return(size);
}
#if defined(linux)
u_longlong_t linux_fdisksize(int fd)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
   unsigned long nsectors=0; 
    
    /* Get #blocks (512B) for /dev/<device name> */
    if (FTD_IOCTL_CALL(fd, BLKGETSIZE, &nsectors) < 0)
        return(-1);
   return ((u_longlong_t)nsectors); 
#else
   unsigned long long nsectors=0; 
    
    /* Get #blocks (512B) for /dev/<device name> */
    if (FTD_IOCTL_CALL(fd, BLKGETSIZE64, &nsectors) < 0)
        return(-1);

   /* BLKGETSIZE64 returns in bytes, BLKGETSIZE in blocks */
   return ((u_longlong_t)nsectors/512); 
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0) */
}

#endif

/* Main entry point to get a device's size in number of blocks of DEV_BSIZE */
u_longlong_t disksize(char *fn) 
{
    u_longlong_t size; 
    int     fd;

    if ((fd = open(fn, O_RDWR)) == -1) {
        return -1;
    }

    if( strstr( fn, "/vpath" ) != NULL )
     { /* brute_force_get_device_size() called by fdisksize() does not work on IBM SDD vpath devices (pc071221) */
       /* This is a temporary measure before we fix the brute_force_get_device_size() function. */
       size = vpath_fdisksize(fd);
     }
    else
     {
#if defined(linux)
       size = linux_fdisksize(fd);
#else
       size = fdisksize(fd);
#endif

     }

    close(fd);
    return size;
}
