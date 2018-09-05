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
 * ftd_linux.h
 *
 * $Id: ftd_linux.h,v 1.18 2010/12/20 20:17:19 dkodjo Exp $
 *
 */
#ifndef _FTD_LINUX_H_
#define _FTD_LINUX_H_

#include <linux/version.h>
#include <linux/fs.h>
#include <linux/jbd.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

extern unsigned int ftd_debug_flags;
extern int ftd_debug;
enum debug_options {
	debug_ioctl,
	debug_request,
	debug_iodone,
	debug_open,
	debug_close,
	debug_release,
	debug_read,
	debug_write,
	debug_device,
	debug_dirty,
	debug_pstore,
	debug_module,
	debug_clone,
	debug_delete,
};
/*
 * be careful to add new BH flags, which may conflict with other
 * software modules. at least make sure greater than BH_JBDDirty.
 */
#define BH_TDMF_PrivateStart (BH_PrivateStart + 8)
enum ftd_state_bits {
	BH_TDMF_DUMMY = BH_TDMF_PrivateStart,
	BH_TDMF_READ,
	BH_TDMF_WRITE,
	BH_TDMF_READA,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
extern int ftd_size[];
extern int ftd_blocksizes[];
extern int ftd_hardsect_sizes[];
extern int ftd_maxreadahead[];

#define buf buffer_head
#define av_forw b_next_free
#define av_back b_prev_free
#define b_bcount b_size		//block size 
#define b_blkno b_rsector	//logical block number

#else

#include <linux/bio.h>
extern struct block_device *ftd_blkdev;

#define buf bio
#define b_bcount bi_size
#define b_blkno bi_sector
#define b_end_io bi_end_io
#define b_rdev bi_bdev->bd_dev
#define av_forw bi_next
#define b_count bi_cnt
#define b_private bi_private
#define b_data bi_io_vec->bv_page
#define b_state bi_flags

#endif /* < KERNEL_VERSION(2, 6, 0) */

#define b_flags b_state		// Is it correctly? (k-harada)
#define b_iodone b_end_io

#define DEV_BSHIFT 	9 	// b_size >> DEV_BSHIFT = num  of sectors
#define DEV_BSIZE	512

#define KM_NOSLEEP	GFP_ATOMIC
#define KM_SLEEP	GFP_KERNEL

#define NBBY		8

#define bzero(from, len)	memset(from, 0, len)
#define bcopy(from, to, len)	memcpy(to, from, len)

/* These macros must only be used in the driver code */

#include <linux/kdev_t.h>
typedef unsigned int minor_t;
typedef unsigned int major_t;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define getmajor(dev)	MAJOR(dev)
#define getminor(dev)	MINOR(dev)
#else
/* getminor and getmajor work on encoded dev_t values
#define getminor(dev)   ((unsigned int)((dev & 0xff) | ((dev >> 12) & 0xfff00)))
#define getmajor(dev)   ((unsigned int)((dev & 0xfff00) >> 8))
*/
#define getmajor(dev)	MAJOR(dev)
#define getminor(dev)	MINOR(dev)
/* FTDGETMAJOR and FTDGETMINOR work on 64 bit dev_t like values */
#define FTDGETMAJOR(dev) MAJOR(dev)
#define FTDGETMINOR(dev) MINOR(dev)
#endif /* < KERNEL_VERSION(2, 6, 0) */

#include <asm/uaccess.h>
#define copyout(from, to, len)	copy_to_user((to), (from), (len))

#include <asm/delay.h>
#define delay(usec)	udelay(usec)
#define drv_usectohz

#define FLIP_ERROR(err) ((err) > 0 ? -(err) : (err))
/*
 * debug
 */
#ifdef _DEBUG
#define _FTD_DEBUGF 1
#else
#define _FTD_DEBUGF 0
#endif
#define DEBUG_BITS(option) ((1ul) << debug_##option)
#define ftd_debugf(bits, a0, a1...) if (_FTD_DEBUGF && (ftd_debug_flags & DEBUG_BITS(bits))) {\
    printk(KERN_NOTICE " " #bits " %s:%d [%d:%d] " a0, __FILE__, __LINE__, current->pid, smp_processor_id(), ## a1); \
}

#define ftd_debug0(a0) if (_FTD_DEBUGF) {\
    printk(KERN_NOTICE "%s:%d [%d:%d] " a0, __FILE__, __LINE__, current->pid, smp_processor_id()); \
}
#define ftd_debug1(a0, a1) ftd_debugf(0, a0, a1)
#define ftd_debug2(a0, a1, a2) ftd_debugf(0, a0, a1, a2)
#define ftd_debug3(a0, a1, a2, a3) ftd_debugf(0, a0, a1, a2, a3)
#define ftd_debug4(a0, a1, a2, a3, a4) ftd_debugf(0, a0, a1, a2, a3, a4)

void ftd_gendisk_init(struct block_device *dtcbd, int index);
void ftd_blkdev_put(struct block_device * bdev);
int  ftd_blkdev_get(struct block_device * bdev, unsigned int flags );

#endif /* #ifndef _FTD_LINUX_H_ */
