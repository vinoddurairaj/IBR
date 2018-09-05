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
/* #ident "@(#)$Id: ftd_mngt_get_all_devices.c,v 1.35 2014/11/10 16:41:42 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_GET_ALL_DEVICES_C_
#define _FTD_MNGT_GET_ALL_DEVICES_C_

#if defined (SOLARIS) && (SYSVERS >= 590)
#define EFI_SUPPORT
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/param.h>
#include <dirent.h>
#include <limits.h>

#if defined(SOLARIS)
#include <sys/dkio.h>
#include <sys/vtoc.h>
#ifdef EFI_SUPPORT
#include <sys/efi_partition.h>
#endif
#elif defined(HPUX)
#include <sys/diskio.h>
#include <mntent.h>
#elif defined(_AIX)
#include <sys/lvdd.h>           /* LV_INFO ioctl usage */
#include <lvm.h>                /* LV_INFO ioctl usage */
#include <sys/devinfo.h>        /* IOCINFO devinfo usage */ 
#include <sys/ioctl.h>          /* IOCINFO ioctl usage */  
#include "aixcmn.h"
#elif defined(linux)
#include <linux/fs.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define LVMDIR "/etc/lvmtab.d"
#else
#define LVMDIR "/etc/lvm/backup"
#endif
#endif

#include "common.h"
extern void force_dsk_or_rdsk(char *pathout, char *pathin, int to_char);
extern void close_part_list();

#ifdef HAVE_LIBZFS
#include "dynamic_libzfs.h"
static dynamic_libzfs_context_t* global_dynamic_libzfs_context;
#endif

#define STRUCT_INCR	10
#define VECTOR_SZ_INCR 	STRUCT_INCR * sizeof(mmp_TdmfDeviceInfoEx)

#if defined(_AIX)
u_longlong_t    major;
u_longlong_t    minor;
struct          stat statbuf;
#endif

static void log_for_analysis( char *function_name, char *message1, char *message2 )
{
    sprintf( logmsg, "%s %s; description: %s\n", function_name, message1, message2 );
    logoutdelta( logmsg );
	return;
}

static int
setvtocinfo(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn);

int Agnt_disksize(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes,char *fn)
{

    if (setvtocinfo(ppDevicesVector, piNbrDevices, piVectorSzBytes,fn) == -1) {
         return (-1);
    }
    return 0;
}

#if defined(SOLARIS)
#ifdef NOT_USE_LIBGEN 
/*
 * comment out this function, instead, use the one defined in common.c
 */
char * ftd_fgets(char *line, int size, int fd)
{
    int  i = 0, ret;
    static int offset = 0;
    char *buf;

    if (size <= 0) {
        return (NULL);
    }

    buf = (char *)malloc( size );
    memset(line, 0, size);

    if((ret = read(fd, buf, (size-1))) <= 0){
        free(buf);
        return (NULL);
    }

    do{
        line[i] = buf[i];
        if( buf[i] == '\n'|| buf[i] == '\0' ){
            break;
        }
        i++;
    }while( i < ret );

    if(i == ret){
        free(buf);
        return (line);
    }
    
    offset = ret - (i+1);
    lseek(fd, -offset, SEEK_CUR);
    free(buf);
    return (line);
}
#endif
#endif

#define SECTORS_PER_TERABYTE (1LL <<31)
#define FS_SIZE_UPPER_LIMIT  0x100000000000LL

// Enable workaround when compiling for 32 bits on zLinux
// When compiling for 64 bits, s390x is defined.
// Another possible 32/64 bits trigger would have been the size/maximum value the unsigned long type.
#if defined (linux) && defined (__s390__) && ! defined(s390x)
#warning "Overriding a gcc on s390 bug.  See comments for details."
/**
 *
 * When compiling for 32 bits in the foolowing environment:
 * Red Hat Enterprise Linux AS release 4 (Nahant Update 3 and 6)
 * gcc (GCC) 3.4.5 20051201 (Red Hat 3.4.5-2)
 *
 * We'd get the following gcc error:
 *
 * ftd_mngt_get_all_devices.c: In function `Agnt_brute_force_get_device_size':
 * ftd_mngt_get_all_devices.c:243: error: insn does not satisfy its constraints:
 * (insn 328 51 105 10 ftd_mngt_get_all_devices.c:207 (set (reg:DI 2 %r2)
 *         (const_int 4294967296 [0x100000000])) 52 {*movdi_31} (nil)
 *             (nil))
 * ftd_mngt_get_all_devices.c:243: internal compiler error: in reload_cse_simplify_operands, at postreload.c:391
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


/**
 *
 * @todo Isn't there a better way than brute force to obtain this kind of information?
 *       What about just llseeking to SEEK_END to obtain the device size?
 *       Check out implementations of df or fdisk to see how they do it for block devices.
 *
 * @internal I doubdt that the comments referring to the sizes of 1TB/16, 16TB and so on are still valid.
 *
 * @todo Use the libgen/disksize.c implementation of the same method!
 *
*/
u_longlong_t Agnt_brute_force_get_device_size(int fd, char *device_name)
{
    u_longlong_t    min_fail = 0;
    u_longlong_t        max_succeed = 0;
    u_longlong_t        cur_db_off;
    char                buf[DEV_BSIZE];

    /*
     * First, see if we can read the device at all, just to
     * eliminate errors that have nothing to do with the
     * device's size.
     */

    if (((llseek(fd, (offset_t)0, SEEK_SET)) == -1) ||
        ((read(fd, buf, DEV_BSIZE)) == -1))
    {
	    log_for_analysis( "Agnt_brute_force_get_device_size(), failed accessing block 0 of", device_name, strerror(errno) );
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
        cur_db_off += DB_OFFSET_INCREMENT_SIZE) {
        if (((llseek(fd, (offset_t)(cur_db_off * DEV_BSIZE),
            SEEK_SET)) == -1) ||
            ((read(fd, buf, DEV_BSIZE)) != DEV_BSIZE))
                min_fail = cur_db_off;
        else
                max_succeed = cur_db_off;
    }
    if (min_fail == 0)
    {
	    log_for_analysis( "Agnt_brute_force_get_device_size(), failed determining size of", device_name, strerror(errno) );
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

    while (min_fail - max_succeed > 1) {
        cur_db_off = max_succeed + (min_fail - max_succeed)/2;
        if (((llseek(fd, (offset_t)(cur_db_off * DEV_BSIZE),
            SEEK_SET)) == -1) ||
            ((read(fd, buf, DEV_BSIZE)) != DEV_BSIZE))
                min_fail = cur_db_off;
        else
                max_succeed = cur_db_off;
    }

    /* the size is the last successfully read sector offset plus one */
    return (max_succeed + 1);
}

/*------------------------------------------------------------------------------------------------------------- */
/* The following setvtocinfo_2_5_2() functions, one per platform, are used as temporary fix for problems
   encountered with the Agnt_brute_force_get_device_size() on systems using IBM SDD and vpath devices. They use
   the logic that was in place in release 2.5.2 (pc080111) */
#if defined(linux)
/* to make a part list  of active devices*/
struct part{
        long lpartsize;
        char pname[80];
};

struct part *p, *startp;
long maxpart;
static int
setvtocinfo_2_5_2(mmp_TdmfDeviceInfoEx **, int *, unsigned int *, char *);
#endif


#if defined(SOLARIS)
static int
setvtocinfo_2_5_2(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
    int                   fd;
    u_longlong_t          size;
    struct dk_cinfo       dkinfo;
    struct vtoc           dkvtoc;
    unsigned long         sectsiz;
    u_longlong_t          tsize;
    u_longlong_t          tstart;
    char                  cmdline[256];
    char                  fstype[8];
    mmp_TdmfDeviceInfoEx    *pWk;
    mmp_TdmfDeviceInfoEx    *tmpp;
    FILE                  *f;
    int32_t               ret;
    int32_t               newvtoc = 0;
    int32_t               idx = 0;
#ifdef EFI_SUPPORT
    struct dk_gpt         *efip = NULL;
#endif

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
    if ((ret = read_vtoc(fd, &dkvtoc)) >= 0)
    {
      sectsiz = (unsigned long)(dkvtoc.v_sectorsz > 0) ? dkvtoc.v_sectorsz : DEV_BSIZE;
      tsize = (u_longlong_t) ((uint32_t)dkvtoc.v_part[dkinfo.dki_partition].p_size *
              (u_longlong_t) sectsiz);
      size = (u_longlong_t) ((u_longlong_t)tsize / (u_longlong_t) 512);
      tstart = (u_longlong_t) dkvtoc.v_part[dkinfo.dki_partition].p_start *
        (u_longlong_t) sectsiz;
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
        tstart = (u_longlong_t) efip->efi_parts[0].p_start * (u_longlong_t)sectsiz ;
      }
      else
      {
        return -1;
      }
    }
#endif

        if (tsize <= 0) return -1;

    /* if output vector full, stretch it. */
    if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
       tmpp = *ppDevicesVector;
       *ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
       if (*ppDevicesVector == NULL) {
          logoutx(19, F_setvtocinfo, "realloc failed", "errno", errno);
          *ppDevicesVector = tmpp;
          *piVectorSzBytes -= VECTOR_SZ_INCR;
          return -1;
       }
    }
    pWk = (*ppDevicesVector) + (*piNbrDevices);


    strcpy(pWk->szDrivePath, fn);

    pWk->sFileSystem = MMP_MNGT_FS_IGNORE;
    pWk->szFileSystemSpecificText[0] = 0x0;

    pWk->liDriveId = UINT64_MAX;
    
    pWk->liStartOffset = tstart;
    pWk->liLength = tsize;

    pWk->liDeviceMajor = UINT64_MAX;
    pWk->liDeviceMinor = UINT64_MAX;

#ifdef HAVE_LIBZFS
    if(global_dynamic_libzfs_context)
    {
        char* zpoolName = NULL;
        
        if(dynamic_libzfs_zpool_in_use(global_dynamic_libzfs_context, fd, &zpoolName))
        {
            strncpy(pWk->szFileSystemSpecificText, zpoolName, sizeof(pWk->szFileSystemSpecificText));
            pWk->szFileSystemSpecificText[sizeof(pWk->szFileSystemSpecificText)] = 0x0;
            pWk->sFileSystem = MMP_MNGT_FS_ZFS;
            free(zpoolName);
        }
    }
#endif
    
    close(fd);
    
    (*piNbrDevices)++;
    return 0;
}

#elif defined(HPUX)

/*
 * Return size of disk in DEV_BSIZE (1024) byte blocks.
 */
static int
setvtocinfo_2_5_2(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
    int                   fd;
    capacity_type         cap;
    unsigned long long  sectors;
    /* u_longlong_t       tsize; */
    unsigned long long    tsize;
    mmp_TdmfDeviceInfoEx    *pWk;
    mmp_TdmfDeviceInfoEx    *tmpp;

    if ((fd = open(fn, O_RDWR)) == -1) {
        return -1;
    }

    if (FTD_IOCTL_CALL(fd, DIOC_CAPACITY, &cap) < 0) {
        return -1;
    }
    close(fd);

    sectors = (unsigned long long)(cap.lba);
    tsize = (u_longlong_t)sectors * (u_longlong_t)DEV_BSIZE;

        if (tsize <= 0) return -1;

    /* if output vector full, stretch it. */
    if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
        tmpp = *ppDevicesVector;
        *ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
        if (*ppDevicesVector == NULL) {
            logoutx(4, F_setvtocinfo, "realloc failed", "errno", errno);
            *ppDevicesVector = tmpp;
            *piVectorSzBytes -= VECTOR_SZ_INCR;
            return -1;
        }
    }
    pWk = (*ppDevicesVector) + (*piNbrDevices);

    strcpy(pWk->szDrivePath, fn);

    pWk->sFileSystem = MMP_MNGT_FS_IGNORE;
    pWk->szFileSystemSpecificText[0] = 0x0;

    pWk->liDriveId = UINT64_MAX;

    pWk->liStartOffset = 0;
    pWk->liLength = tsize;

    pWk->liDeviceMajor = UINT64_MAX;
    pWk->liDeviceMinor = UINT64_MAX;
    
    (*piNbrDevices)++;
    return 0;
}

#elif defined(_AIX)

/* return DEV_BSIZE byte blocks of an lv */
static int
setvtocinfo_2_5_2(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
        int             fd;
        int             iocerr;
        daddr_t         size;
        u_longlong_t    tsize;
        u_longlong_t    major;
        u_longlong_t    minor;
        int             bsiz_ratio;
        struct          stat statbuf;
        struct          devinfo devi;
        mmp_TdmfDeviceInfoEx    *pWk;
        mmp_TdmfDeviceInfoEx    *tmpp;

        if ((fd = open(fn, O_RDWR)) == -1) {
                return (-1);
        }

        /* get logical volume info */
         iocerr = FTD_IOCTL_CALL(fd, IOCINFO, &devi);
        if (iocerr)
                return (-1);

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
        else {
            return -1;
       }

        tsize =  ((u_longlong_t)size);

        if (0 != stat (fn, &statbuf)) return;

        major =  (u_longlong_t) bmajor(statbuf.st_rdev);
        minor =  (u_longlong_t) minor(statbuf.st_rdev);

        close(fd);

        tsize = tsize * (u_longlong_t)DEV_BSIZE;

        if (tsize <= 0) return -1;

        /* if output vector full, stretch it. */
        if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
                tmpp = *ppDevicesVector;
                *ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
                if (*ppDevicesVector == NULL) {
                        logoutx(4, F_setvtocinfo, "realloc failed", "errno", errno);
                        *ppDevicesVector = tmpp;
                        *piVectorSzBytes -= VECTOR_SZ_INCR;
                        return -1;
                }
        }
        pWk = (*ppDevicesVector) + (*piNbrDevices);

        strcpy(pWk->szDrivePath, fn);

        pWk->sFileSystem = MMP_MNGT_FS_IGNORE;
        pWk->szFileSystemSpecificText[0] = 0x0;

        pWk->liDriveId = UINT64_MAX;

        pWk->liStartOffset = 0;
        pWk->liLength = tsize;

        pWk->liDeviceMajor = major;
        pWk->liDeviceMinor = minor;
        
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
setvtocinfo_2_5_2(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
        int fd;
        int openvalue = 0;
        daddr_t          blksize;
        u_longlong_t     tsize;
    mmp_TdmfDeviceInfoEx    *pWk;
    mmp_TdmfDeviceInfoEx    *tmpp;

#if 0
        /* get the number  open for a block device */
        openvalue = opened_num(fn);
        if (openvalue  > 0) return -1;  /* inuse */
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
        close(fd);

        tsize = (u_longlong_t) blksize * (u_longlong_t)DEV_BSIZE;

        if (tsize <= 0) return -1;

        /* if output vector full, stretch it. */
        if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
                tmpp = *ppDevicesVector;
                *ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
                if (*ppDevicesVector == NULL) {
                        logoutx(4, F_setvtocinfo, "realloc failed", "errno", errno);
                        *ppDevicesVector = tmpp;
                        *piVectorSzBytes -= VECTOR_SZ_INCR;
                        return -1;
                }
        }
    pWk = (*ppDevicesVector) + (*piNbrDevices);

    strcpy(pWk->szDrivePath, fn);

    pWk->sFileSystem = MMP_MNGT_FS_IGNORE;
    pWk->szFileSystemSpecificText[0] = 0x0;

    pWk->liDriveId = UINT64_MAX;
    
    pWk->liStartOffset = 0;
    pWk->liLength = tsize;

    pWk->liDeviceMajor = UINT64_MAX;
    pWk->liDeviceMinor = UINT64_MAX;
    
    (*piNbrDevices)++;
    return 0;
}

#endif


static int
setvtocinfo(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *fn)
{
    int                   fd;
    u_longlong_t          size;
    u_longlong_t          tsize;
    u_longlong_t          major = 0;
    u_longlong_t          minor = 0;
#if defined (_AIX)
    struct stat           statbuf;
#endif
    mmp_TdmfDeviceInfoEx    *pWk;
    mmp_TdmfDeviceInfoEx    *tmpp;
	int save_errno;

    /* The Agnt_brute_force_get_device_size() logic does not work with IBM SDD vpath devices. 
       Until it is debugged, in these cases, we use the old logic from release 2.5.2. setvtocinfo_2_5_2() will
       do everything that is done here so we just return after calling it (pc080109) */
    if( strstr( fn, "/vpath" ) != NULL )
       return( setvtocinfo_2_5_2( ppDevicesVector, piNbrDevices, piVectorSzBytes, fn ) );

    if ((fd = open(fn, O_RDWR)) == -1) {
	    save_errno = errno;
        sprintf(logmsg, "%s open failed. errno = %d\n", fn, errno);
        logout(11, F_setvtocinfo, logmsg);
	    log_for_analysis( "setvtocinfo() for dev size, failed opening device", fn, strerror(save_errno) );
        return -1;
    }

    size = Agnt_brute_force_get_device_size(fd, fn);
    tsize = (u_longlong_t)size * (u_longlong_t)DEV_BSIZE;

#if defined (_AIX)
    if (0 != stat (fn, &statbuf))
        return;

    major =  (u_longlong_t) bmajor(statbuf.st_rdev);
    minor =  (u_longlong_t) minor(statbuf.st_rdev);
#endif

    if (tsize <= 0)
	    log_for_analysis( "setvtocinfo() for dev size, error occured in Agnt_brute_force_get_device_size for ", fn, "returning -1" );
    
    if (tsize <= 0) return -1;

    /* if output vector full, stretch it. */
    if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
       tmpp = *ppDevicesVector;
       *ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
	   save_errno = errno;
       if (*ppDevicesVector == NULL) {
          logoutx(19, F_setvtocinfo, "realloc failed", "errno", errno);
          *ppDevicesVector = tmpp;
          *piVectorSzBytes -= VECTOR_SZ_INCR;
	    log_for_analysis( "setvtocinfo() device capture vector too small, and failed reallocation, now at device ", fn, strerror(save_errno) );
          return -1;
       }
    }
    pWk = (*ppDevicesVector) + (*piNbrDevices);

    strcpy(pWk->szDrivePath, fn);

    pWk->sFileSystem = MMP_MNGT_FS_IGNORE;
    pWk->szFileSystemSpecificText[0] = 0x0;

    pWk->liDriveId = UINT64_MAX;
    
    pWk->liStartOffset = 0;
    pWk->liLength = tsize;

    pWk->liDeviceMajor = major;
    pWk->liDeviceMinor = minor;
    
#ifdef HAVE_LIBZFS
    if(global_dynamic_libzfs_context)
    {
        char* zpoolName = NULL;
        
        if(dynamic_libzfs_zpool_in_use(global_dynamic_libzfs_context, fd, &zpoolName))
        {
            strncpy(pWk->szFileSystemSpecificText, zpoolName, sizeof(pWk->szFileSystemSpecificText));
            pWk->szFileSystemSpecificText[sizeof(pWk->szFileSystemSpecificText)] = 0x0;
            pWk->sFileSystem = MMP_MNGT_FS_ZFS;
            free(zpoolName);
        }
    }
#endif
    
    close(fd);
    
    (*piNbrDevices)++;
    return 0;
}

/****************************************************************************
 * force_dsk_or_rdsk_path -- force a path to have either /rdsk or /dsk in it
 *                           and add it to the end if it isn't there
 ***************************************************************************/
#if defined(SOLARIS)
#ifdef NOT_USE_LIBGEN
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
#endif  /* NOT_USE_LIBGEN */
#endif /* defined(SOLAIRIS) */

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
get_char_device_info(char *dev_name, int is_lv, u_long *sectors)
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
            mpref.mnt_special = block_name;
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
capture_logical_volumes(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
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
    char mountpath[MAXPATHLEN];
    char		  *ptr, *end;
    u_long		  sectors;
    /* u_longlong_t	  size; */
    unsigned long long	  size;
    mmp_TdmfDeviceInfoEx    *pWk;
    mmp_TdmfDeviceInfoEx    *tmpp;
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

		if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
			tmpp = *ppDevicesVector;
			*ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
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

		strcpy(pWk->szDrivePath, raw_name);

		pWk->sFileSystem = MMP_MNGT_FS_UNKNOWN;
        pWk->szFileSystemSpecificText[0] = 0x0;

        pWk->liDriveId = UINT64_MAX;
        
        pWk->liStartOffset = 0;
		pWk->liLength = size;

        pWk->liDeviceMajor = UINT64_MAX;
        pWk->liDeviceMinor = UINT64_MAX;
        
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
queryvgs(mid_t addr, mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
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
	unsigned long long sectcnt;
	mmp_TdmfDeviceInfoEx    *pWk;
	mmp_TdmfDeviceInfoEx    *tmpp;
	u_longlong_t          tsize;
	u_longlong_t          major;
	u_longlong_t          minor;

	
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

			/*
			 * presume the logical volume was created with 
			 * a default path name, that it lives in "/dev/".
			 */
			/* sprintf(bdevice, "/dev/%s", qlv->lvname); */
			sprintf(cdevice, "/dev/r%s", qlv->lvname);

    			/* if output vector full, stretch it. */	
			if ((*piNbrDevices) * sizeof(mmp_TdmfDeviceInfoEx) == *piVectorSzBytes) {
				tmpp = *ppDevicesVector;
				*ppDevicesVector = (mmp_TdmfDeviceInfoEx *)realloc(*ppDevicesVector, *piVectorSzBytes += VECTOR_SZ_INCR);
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
			csize = (unsigned long long)qlv->currentsize * (2ULL << (qlv->ppsize - 1));
			sectcnt = (csize >> DEV_BSHIFT);
    			tsize = sectcnt * (u_longlong_t)DEV_BSIZE;
			minor = qvgp->lvs[lvndx].lv_id.minor_num;

			strcpy(pWk->szDrivePath, cdevice);

			pWk->sFileSystem = MMP_MNGT_FS_UNKNOWN;
            pWk->szFileSystemSpecificText[0] = 0x0;

            pWk->liDriveId = UINT64_MAX;
            
            pWk->liStartOffset = 0;
            pWk->liLength = tsize;

			pWk->liDeviceMajor = major;
			pWk->liDeviceMinor = minor;

    			(*piNbrDevices)++;
		}
	}
}

static void
enum_lvs(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
{
	queryvgs(hd_config_addr(), ppDevicesVector, piNbrDevices, piVectorSzBytes);
}
#endif  /* defined (_AIX) */

#if defined(linux)

#ifdef NOT_USE_LIBGEN
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
		i=11;
		while (i--)
			ret = fscanf( fh, "%s", &tmp);
#endif
		maxpart++;
	} while ((ret > 0) && ( ret != EOF ));

	/*
	 * WR 36971:: Agent Memory Leak
	 *            free 'startp' if its not NULL.
	 */
	if (startp != NULL) {
	    free(startp);
	    startp = NULL;
	}
	
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
		i=11;
		/* ignore 11 infos for block devices */
		while (i--)
			ret = fscanf( fh, "%s", &tmp);
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
#endif /* NOT_USE_LIBGEN */
#endif /* partition active devices Linux */

/****************************************************************************
 * capture_AIX_rhdiskX_devs -- reports on all AIX rhdisk devices
 ***************************************************************************/
#if defined(_AIX)
static void
capture_AIX_rhdiskX_devs(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
{
    char cdevice[PATH_MAX];
    DIR *dfd;
    struct dirent *dent;
    struct stat cstatbuf;
    FILE* f;

    if (((DIR*)NULL) == (dfd = opendir ("/dev")))
    {
	     if( errno != ENOTDIR )	log_for_analysis( "capture_AIX_rhdiskX_devs() could not opendir for /dev", strerror(errno) );
         return;
	}

    while (NULL != (dent = readdir(dfd))) {
        if (strcmp(dent->d_name, ".") == 0) continue;
        if (strcmp(dent->d_name, "..") == 0) continue;

        sprintf (cdevice, "/dev/%s", dent->d_name);

        if( strstr( cdevice, "/dev/rhdisk" ) == NULL ) continue;

        if (0 != stat (cdevice, &cstatbuf))
        {
	         log_for_analysis( "capture_AIX_rhdiskX_devs() could not stat", cdevice, strerror(errno) );
	         continue;
		}
        if (!(S_ISCHR(cstatbuf.st_mode)))
        {
	         log_for_analysis( "capture_AIX_rhdiskX_devs() for ", cdevice, "not a special character device; skipping it" );
	         continue;
		}

        if (Agnt_disksize(ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice) == -1) {
	        log_for_analysis( "capture_AIX_rhdiskX_devs() for ", cdevice, "could not determine device size; skipping it" );
            continue;
		}
    }
    (void) closedir (dfd);
}
#endif

/****************************************************************************
 * capture_devs_in_dir -- reports on all devices in the given directories
 ***************************************************************************/
static void
capture_devs_in_dir(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *cdevpath, char *bdevpath)
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
    if (((DIR*)NULL) == (dfd = opendir (cdevpath)))
    {
	     if( errno != ENOTDIR )	log_for_analysis( "capture_devs_in_dir() could not opendir for", cdevpath, strerror(errno) );
         return;
	}

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
        if (0 != stat (cdevice, &cstatbuf))
        {
	         log_for_analysis( "capture_devs_in_dir() could not stat", cdevice, strerror(errno) );
	         continue;
		}
        if (!(S_ISCHR(cstatbuf.st_mode)))
        {
	         log_for_analysis( "capture_devs_in_dir() for ", cdevice, "not a special character device; skipping it" );
	         continue;
		}

        if(strcmp(cdevice, bdevice) == 0){   
            sprintf (bdevice, "%s/%s", bdevpath, &dent->d_name[1]);
        }
#endif
        if (0 != stat (bdevice, &bstatbuf))
        {
	         log_for_analysis( "capture_devs_in_dir() could not stat", bdevice, strerror(errno) );
	         continue;
		}
        if (!(S_ISBLK(bstatbuf.st_mode))) continue;

#if defined(linux)
        if (Agnt_disksize(ppDevicesVector, piNbrDevices, piVectorSzBytes, bdevice) == -1) {
#else
        if (Agnt_disksize(ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice) == -1) {
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
walk_rdsk_subdirs_for_devs(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *rootdir)
{
    DIR* dfd;
    DIR* cdevfd;
    DIR* bdevfd;
    struct dirent* dent;
    char cdevdirpath[PATH_MAX];
    char bdevdirpath[PATH_MAX];

    if (((DIR*)NULL) == (dfd = opendir (rootdir)))
    {
        if( errno != ENOTDIR )	log_for_analysis( "walk_rdsk_subdirs_for_devs() could not opendir for", rootdir, strerror(errno) );
	    return;
	}
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
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath)))
        {
            if( errno != ENOTDIR )	log_for_analysis( "walk_rdsk_subdirs_for_devs() could not opendir for", cdevdirpath, strerror(errno) );
		    continue;
		}
        (void) closedir (cdevfd);
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath)))
        {
            if( errno != ENOTDIR )	log_for_analysis( "walk_rdsk_subdirs_for_devs() could not opendir for", bdevdirpath, strerror(errno) );
		    continue;
		}
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
walk_dirs_for_devs(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes, char *rootdir)
{
    DIR* dfd;
    DIR* cdevfd;
    DIR* bdevfd;
    struct dirent* dent;
    char cdevdirpath[PATH_MAX];
    char bdevdirpath[PATH_MAX];

    if (((DIR*)NULL) == (dfd = opendir (rootdir)))
    {
        if( errno != ENOTDIR )	log_for_analysis( "walk_dirs_for_devs() could not opendir for", rootdir, strerror(errno) );
	    return;
	}
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
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath)))
	    {
	        if( errno != ENOTDIR )	log_for_analysis( "walk_dirs_for_devs() could not opendir for", cdevdirpath, strerror(errno) );
		    continue;
		}
        (void) closedir (cdevfd);
#endif
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath)))
	    {
	        if( errno != ENOTDIR )	log_for_analysis( "walk_dirs_for_devs() could not opendir for", bdevdirpath, strerror(errno) );
		    continue;
		}
        (void) closedir (bdevfd);
		capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevdirpath, bdevdirpath);
    }
    (void) closedir (dfd);
}


#if !defined(linux)
/****************************************************************************
 * agent_capture_devlistfile -- write out all devices.
 *                              Defined output device on devconf file.
 * TODO: we should look into consolidate this function with same function
 *       defined in devcheck.c.
 ***************************************************************************/
void
agent_capture_devlistfile (
    mmp_TdmfDeviceInfoEx **ppDevicesVector,
    int *piNbrDevices,
    unsigned int *piVectorSzBytes)
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
                    vglist = (char *)realloc (vglist, PATH_MAX*4*(vgnum/4+1));
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
#elif defined(HPUX)
            f_dup = 0;
            for (i=0; i<vgnum; i++) {
                vglistp = vglist + (PATH_MAX * i);    
                if ((strncmp(devpath, vglistp, len) == 0)) {
                    f_dup = 1;
                    break;
                }
            }
            if ( f_dup == 1 ) {
                continue;
            }
#endif
            /* duplication path check */
            if ((listused % 8) == 0 ) {
                devlist = (char *)realloc (devlist, PATH_MAX*8*(listused/8+1));
            }
            f_dup = 0;
            for (i=0; i<listused; i++) {
                if (strcmp((devlist + (PATH_MAX * i)), devpath) == 0) {
                    f_dup = 1;
                    break;
                }
            }
            if ( f_dup == 1 ) {
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

	    /* process rdsk anywhere within the device path name */ 
	    /* replace with dsk instead of removing leading r in the basename */

            if ((rdskp=strstr(devpath, "/rdsk")) == 0
	     || (*(rdskp+5) != '/' && *(rdskp+5) != '\0')) {
#if defined(SOLARIS)
                /* -- devices type : /AAAA/yyy/rdsk/xxx -- */
                walk_dirs_for_devs (ppDevicesVector, piNbrDevices,
                            piVectorSzBytes, devpath);
                
                /* -- devices type : /AAAA/rdsk/yyy/xxx -- */
                sprintf(cdevice, "%s/rdsk", devpath); 
                if (((DIR*)NULL) != (dfd = opendir (cdevice))) {
                    (void) closedir (dfd);
                    walk_rdsk_subdirs_for_devs (ppDevicesVector, piNbrDevices,
                            piVectorSzBytes, cdevice);
                }
                memset(cdevice, 0x00, PATH_MAX);
#endif
                /* -- devices type : /AAAA/xxx and /AAAA/rxxx-- */
                capture_devs_in_dir (ppDevicesVector, piNbrDevices,
                            piVectorSzBytes, devpath, devpath);

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
                capture_devs_in_dir (ppDevicesVector, piNbrDevices,
                            piVectorSzBytes, cdevice, bdevice);
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
#endif /* !defined(linux) */


/****************************************************************************
 * capture_all_devs -- write out all devices found on a system to
 *                     the specified output device
 ***************************************************************************/
static void
capture_all_devs(mmp_TdmfDeviceInfoEx **ppDevicesVector, int *piNbrDevices, unsigned int *piVectorSzBytes)
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

// We had 2 Solaris customer cases where the number of devices on the systems was very high
// and this caused a timeout on the DMC. We were asked then to modify the Agent to capture
// only the vx devices. If this happens again and you want to generate an Agent that captures
// only vx devices, undefine the switch: CAPTURE_ALL_TYPES_OF_DEVICES_NOT_ONLY_VX_DEVS.
// Ref.:  WRs PROD15733 and PROD10717.
// IMPORTANT: do not put this compile switch into another switch. It must be visible on all platforms.
#define CAPTURE_ALL_TYPES_OF_DEVICES_NOT_ONLY_VX_DEVS

#ifdef HAVE_LIBZFS
    global_dynamic_libzfs_context = dynamic_libzfs_init();
#endif  
    
#if defined(SOLARIS)

#ifdef CAPTURE_ALL_TYPES_OF_DEVICES_NOT_ONLY_VX_DEVS
    /* -- get the local disk devices */
    capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/rdsk", "/dev/dsk");
#endif  

    /* -- get the VxVM disk group volumes */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
        (void) closedir (dfd);
        if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/rdsk"))) {
            (void) closedir (dfd);
	    walk_rdsk_subdirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/vx/rdsk");
        }
		else
		{
            if( errno != ENOTDIR )	log_for_analysis( "capture_all_devs()", "could not opendir for /dev/vx/rdsk; ", strerror(errno) );
		}
    }
	else
	{
        if( errno != ENOTDIR )	log_for_analysis( "capture_all_devs()", "could not opendir for /dev/vx; ", strerror(errno) );
    }

    /* -- get the datastar devices -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/" QNM);
    }
	else
	{
        if( errno != ENOTDIR )	log_for_analysis( "capture_all_devs()", "did not opendir for /dev/dtc; ", strerror(errno) );
	}
  
#ifdef CAPTURE_ALL_TYPES_OF_DEVICES_NOT_ONLY_VX_DEVS
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
#endif  
 
#endif /* SOLARIS */
  
#if defined(HPUX)
    (void) capture_logical_volumes (ppDevicesVector, piNbrDevices, piVectorSzBytes);

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
#endif /* HPUX */

#if defined(_AIX)
    (void) enum_lvs (ppDevicesVector, piNbrDevices, piVectorSzBytes);

    /* -- get the VxVM disk group volumes */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
        (void) closedir (dfd);
        if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/rdsk"))) {
            (void) closedir (dfd);
            walk_rdsk_subdirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/vx/rdsk");
        }
    }
  
    /* -- get the datastar devices here -- */
    if (((DIR*)NULL) != (dfd = opendir ("/dev/" QNM))) {
        (void) closedir (dfd);
	walk_dirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/" QNM);
    }

    // Get /dev/rhdisk* for AIX bootvolume migration feature (defect 72110)     
    capture_AIX_rhdiskX_devs(ppDevicesVector, piNbrDevices, piVectorSzBytes);

#endif /* defined(_AIX) */

#if !defined(linux)
#ifdef CAPTURE_ALL_TYPES_OF_DEVICES_NOT_ONLY_VX_DEVS
    /* -- get the user config devices -- */
    agent_capture_devlistfile (ppDevicesVector,piNbrDevices,piVectorSzBytes);
#endif
#endif /* !defined(linux) */


#if defined(linux)
	if (startp == NULL) {
		if (make_part_list() == -1) {
			logout(7, F_capture_devs_in_dir, "Timeout is not effective.\n");
		}
	}
	/* -- get the IDE/SCSI/RAID devices -- */
	capture_devs_in_dir (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev", "/dev");
	/* -- get the LVM devices -- */
	if (((DIR*)NULL) != (dfd = opendir (LVMDIR))) {
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
	if (((DIR*)NULL) != (dfd = opendir ("/dev/vx"))) {
	    (void) closedir (dfd);
	    if (((DIR*)NULL) != (dfd = opendir ("/dev/vx/dsk"))) {
		(void) closedir (dfd);
		/* linux does not support raw devices */
		walk_rdsk_subdirs_for_devs (ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/vx/dsk");
	    }
	}


      /* -- get the evms devices here -- */
      if (((DIR*) NULL) != (dfd = opendir("/dev/evms")))
      {
     	(void) closedir(dfd);
     	strcpy(bdevice, "/dev/evms");
    	strcpy(cdevice, "/dev/evms");
       capture_devs_in_dir(ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice, bdevice);
       walk_rdsk_subdirs_for_devs(ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/evms");
      }

      /* -- get the /dev/mapper devices here -- */
      if (((DIR*) NULL) != (dfd = opendir("/dev/mapper")))
      {
     	(void) closedir(dfd);
     	strcpy(bdevice, "/dev/mapper");
    	strcpy(cdevice, "/dev/mapper");
       capture_devs_in_dir(ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice, bdevice);
       walk_rdsk_subdirs_for_devs(ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/mapper");
      }

      /* -- get the /dev/mpath devices here -- */
      if (((DIR*) NULL) != (dfd = opendir("/dev/mpath")))
      {
     	(void) closedir(dfd);
     	strcpy(bdevice, "/dev/mpath");
    	strcpy(cdevice, "/dev/mpath");
       capture_devs_in_dir(ppDevicesVector, piNbrDevices, piVectorSzBytes, cdevice, bdevice);
       walk_rdsk_subdirs_for_devs(ppDevicesVector, piNbrDevices, piVectorSzBytes, "/dev/mpath");
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

				if ((len == 9) && (strncmp(devpath,"/dev/evms",9) == 0))
				{
					continue;
				}

				if ((len == 11) && (strncmp(devpath,"/dev/mapper",11) == 0))
				{
					continue;
				}
				if ((len == 10) && (strncmp(devpath,"/dev/mpath",10) == 0))
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

#ifdef HAVE_LIBZFS
    dynamic_libzfs_finish(global_dynamic_libzfs_context);
#endif
    
    return;
}

/*
 * Manages MMP_MNGT_GET_ALL_DEVICES request
 */
static int
ftd_mngt_get_all_devices(sock_t *sockID)
{
    mmp_mngt_TdmfAgentDevicesMsg_t  requestHeader;
    mmp_mngt_TdmfAgentExDeviceInfoHeader_t replyHeader;
    mmp_TdmfDeviceInfoEx              *pDevicesVector = NULL;
    int                             iNbrDevices = 0;
    unsigned int		            iVectorSzBytes = 0;
    int                             r, toread, towrite;
    char                            *pWk;

    pWk = (char *)&requestHeader;
    pWk += sizeof(mmp_mngt_header_t);

    /*
     * at this point, mmp_mngt_header_t header is read.
     * now read the remainder of the mmp_mngt_TdmfCommandMsg2_t structure
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
    replyHeader.hdr.magicnumber	= MNGT_MSG_MAGICNUMBER;
    replyHeader.hdr.mngttype	= MMP_MNGT_EX_DEVICE_INFO;
    replyHeader.hdr.sendertype	= SENDERTYPE_TDMF_AGENT;
    replyHeader.hdr.mngtstatus	= MMP_MNGT_STATUS_OK;
    /* convert to network byte order before sending on socket */
    mmp_convert_mngt_hdr_hton(&replyHeader.hdr);

    replyHeader.hdrEx.szServerUID[0]	= 0; /* don't care */
    replyHeader.hdrEx.ulMessageSize     = iNbrDevices * sizeof(mmp_TdmfDeviceInfoEx);
    replyHeader.hdrEx.ulMessageVersion  = DEVICEINFOEX_VERSION;
    replyHeader.hdrEx.ulInstanceCount	= iNbrDevices;
    /* convert to network byte order before sending on socket */
    mmp_convert_mngt_hdrEx_hton(&replyHeader.hdrEx);
    
    /* convert to network byte order before sending on socket */
    for (r = 0; r < iNbrDevices; r++) {
        mmp_convert_TdmfAgentDeviceInfoEx_hton(pDevicesVector + r);
    }

    /* 
     * respond using same socket. 
     *  first send mmp_mngt_TdmfAgentExDeviceInfoHeader_t than send pDevicesVector
     */
    towrite = sizeof(replyHeader);
    r = ftd_sock_send(sockID, (char*)&replyHeader, towrite);
    if (r == towrite) {
        towrite = ntohl(replyHeader.hdrEx.ulMessageSize);
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

