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
 * $Id: ftd_linux.c,v 1.109 2018/02/11 02:22:01 paulclou Exp $
 */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
#include <generated/autoconf.h> // WR PROD11894 SuSE 11.2
#else
#include <linux/autoconf.h>
#endif

/*
 NOTE concerning SuSE 11.2 driver port, WR PROD11894
 To apply changes specifically to the drivers for SuSE 11.2, the following preprocessor directive was used:
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
 specific code for SuSE 11.2
 #endif

 An other approach would have been to use preprocessor directives based on the existence of the new implemented
 kernel mechanisms, such as HAVE_UNLOCKED_IOCTL, but doing so would have had impacts on our driver provided in
 already supported Linux releases, where these mechanisms started to exist in parallel with previous mechanisms
 which we kept using. This would have implied redoing test cycles to capture possible side effects.
 For this reason, the kernel version macro and preprocessor directives have been used.
 In a future release, we may want to use the preprocessor directives based on the mechanism names instead of
 kernel version numbers, provided that test cycles are possible to capture any side effects on new builds of
 already provided drivers.
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gfp.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/poll.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
// PROD00013379: porting for RHEL 7.0
#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#endif

/*
 NOTE concerning SuSE 11.2 driver port, WR PROD11894
 The Big Kernel Lock feature (function calls lock_kernel() and unlock_kernel()) does not exist anymore.
 This change has caused, among other things, the removal of the include file smp_lock.h, which we included in ftd_linux.c.
 The replacing approach is now to use localized per-driver mutex locks.
 Our driver used the Big Kernel Lock in one function: ftd_thrd_daemon(), which is invoked at the creation of a new logical group.
 A recommended solution is to use, for instance:
 #if HAVE_UNLOCKED_IOCTL
     #include <linux/mutex.h>
 #else
     #include <linux/smp_lock.h>
 #endif

 . 

 #if HAVE_UNLOCKED_IOCTL
    mutex_lock(&fs_mutex);
 #else
    lock_kernel();
 #endif
  However, as mentionned previously, using the preprocesor directive HAVE_UNLOCKED_IOCTL would cause driver behavior
 changes in our driver already provided in earlier releases where both mechanisms (big kernel lock and unlocked_ioctl
 with mutexes) existed in parallel and our driver kept using the big kernel lock. This would have necessitated new
 test cycles to capture possible side effects in already provided and tested drivers. For this reason, the preprocessor
 directives we have used are based on the kernel version number instead of the mechanism name.
 Thus, this change only applies to our driver for SuSE 11.2 at this point.
  Also, the ioctl API has changed and now the unlocked_ioctl must be used, which has less arguments than our previous ioctl,
 in the same fashion as compat_ioctl.
*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
#include <linux/smp_lock.h>
#else
#include <linux/mutex.h>  // WR PROD11894 SuSE 11.2
#endif

#include <linux/ioctl.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <stdbool.h>

#include <linux/if.h>
#include <linux/cdev.h>
#include "ftdio.h"
#include "ftd_linux_proc_fs.h"

// WI_338550 December 2017, implementing RPO / RTT
#include <linux/time.h>

atomic_t pending_ios = ATOMIC_INIT(0);
atomic_t pending_writes = ATOMIC_INIT(0);
atomic_t pending_io = ATOMIC_INIT(0);
atomic_t pending_lrdb_writes = ATOMIC_INIT(0);
atomic_t lrdb_writes = ATOMIC_INIT(0);

#define MAJOR_NR ftd_bmajor		  /* force definitions on in blk.h */
static int ftd_bmajor;			  /* declared before including blk.h */
static int ftd_cmajor;			  /* declared before including blk.h */

#define DEVICE_NR(device) MINOR(device)	  /* sbull has no partition bits */
#define DEVICE_NAME "ftd"		  /* name for messaging */
#define DEVICE_INTR ftd_intrptr		  /* pointer to the bottom half */
#define DEVICE_NO_RANDOM		  /* no entropy to contribute */
#define DEVICE_REQUEST ftd_request_fn
#define DEVICE_OFF(d)			  /* do-nothing */

#include <linux/blkpg.h>
#include <linux/mm.h>

/* ftd_ddi.h must be included after all the system stuff to be ddi compliant */
#include "ftd_linux.h"
#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"
#include "ftd_bits.h"
#include "ftd_pending_ios_monitoring.h"
#include "ftd_dynamic_activation.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
#include <linux/devfs_fs_kernel.h>
#endif


#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/list.h>

#define BIO_DATA_FROM_BAB  // Local bio to take data directly from the BAB for the physical IO

#if defined(CONFIG_COMPAT) && !defined(HAVE_COMPAT_IOCTL)

#if defined(LINUX260)
#include <linux/ioctl32.h>
#include <linux/syscalls.h> // For sys_ioctl()
#else // LINUX240

#if defined(CONFIG_X86_64)
#include <asm-x86_64/ioctl32.h>
typedef int (*ioctl_trans_handler_t)(unsigned int, unsigned int, unsigned long, struct file *);
#endif // defined(CONFIG_X86_64)

#endif // LINUX260

// Registering a NULL ioctl_trans_handler_t function will use the 64bits ioctl handler.
static struct {
	unsigned int cmd;
	ioctl_trans_handler_t func;
} ftd_ioctl32_cmd[] = {
    { FTD_GET_CONFIG,           NULL },
    { FTD_NEW_DEVICE,           NULL },
    { FTD_NEW_LG,               NULL },
    { FTD_DEL_DEVICE,           NULL },
    { FTD_DEL_LG,               NULL },
    { FTD_CTL_CONFIG,           NULL },
    { FTD_GET_DEV_STATE_BUFFER, NULL },
    { FTD_GET_LG_STATE_BUFFER,  NULL },
    { FTD_SEND_LG_MESSAGE,      NULL },
    { FTD_OLDEST_ENTRIES,       NULL },
    { FTD_MIGRATE,              NULL },
    { FTD_GET_LRDBS,            NULL },
    { FTD_GET_HRDBS,            NULL },
    { FTD_SET_LRDBS,            NULL },
    { FTD_SET_HRDBS,            NULL },
    { FTD_GET_LRDB_INFO,        NULL },
    { FTD_GET_HRDB_INFO,        NULL },
    { FTD_GET_DEVICE_NUMS,      NULL },
    { FTD_SET_DEV_STATE_BUFFER, NULL },
    { FTD_SET_LG_STATE_BUFFER,  NULL },
    { FTD_START_LG,             NULL },
    { FTD_GET_NUM_DEVICES,      NULL },
    { FTD_GET_NUM_GROUPS,       NULL },
    { FTD_GET_DEVICES_INFO,     NULL },
    { FTD_GET_GROUPS_INFO,      NULL },
    { FTD_GET_DEVICE_STATS,     NULL },
    { FTD_GET_GROUP_STATS,      NULL },
    { FTD_SET_GROUP_STATE,      NULL },
    { FTD_UPDATE_DIRTYBITS,     NULL },
    { FTD_CLEAR_BAB,            NULL },
    { FTD_CLEAR_HRDBS,          NULL },
    { FTD_CLEAR_LRDBS,          NULL },
    { FTD_GET_GROUP_STATE,      NULL },
    { FTD_GET_BAB_SIZE,         NULL },
    { FTD_UPDATE_LRDBS,         NULL },
    { FTD_UPDATE_HRDBS,         NULL },
    { FTD_SET_SYNC_DEPTH,       NULL },
    { FTD_SET_IODELAY,          NULL },
    { FTD_SET_SYNC_TIMEOUT,     NULL },
    { FTD_CTL_ALLOC_MINOR,      NULL },
    { FTD_INIT_STOP,            NULL },
    { FTD_GET_GROUP_STARTED,    NULL },
    { FTD_SET_MODE_TRACKING,    NULL },
    { FTD_SET_DEVICE_SIZE,      NULL },
    { FTD_OPEN_NUM_INFO,        NULL },
    { FTD_SET_JLESS,            NULL },
    { FTD_SET_LRDB_MODE,        NULL },
    { FTD_SET_LRDB_BITS,        NULL },
    { FTD_HRDB_TYPE,            NULL },
    { FTD_OVERRIDE_GROUP_STATE, NULL },
    { FTD_BACKUP_HISTORY_BITS,  NULL },
    { FTD_GET_SYSCHK_COUNTERS,  NULL },
    { FTD_CLEAR_SYSCHK_COUNTERS, NULL},
    { FTD_CAPTURE_SOURCE_DEVICE_IO, NULL},
    { FTD_RELEASE_CAPTURED_SOURCE_DEVICE_IO, NULL},
    { 0, NULL }
};

static int max_ioctl32 = -1;

static int ftd_register_ioctl32_cmds(void);
static int ftd_unregister_ioctl32_cmds(void);

#endif // defined(CONFIG_COMPAT) && !defined(HAVE_COMPAT_IOCTL)

/* some global state varibles */
unsigned int ftd_debug_flags = DEBUG_BITS(request)  |
                               DEBUG_BITS(device) /*|
			       DEBUG_BITS(ioctl)    |
                               DEBUG_BITS(delete)   |
                               DEBUG_BITS(open)     |
                               DEBUG_BITS(iodone) */;

extern ftd_lg_map_t     Started_LG_Map;
extern ftd_uchar_t      Started_LGS[];


/**
 * \todo Replace this global ftd_blkdev array by struct gendisk* entries directly within each device's softp.
 */
struct block_device *ftd_blkdev = NULL; 
                                         
struct cdev *ftd_cdev = NULL;

#define PRIVATE_MEMORY_POOL

#ifdef PRIVATE_MEMORY_POOL
/* This private memory pool is allocated at ftd_init() time; its purpose is to provide memory
   to data blocks that cannot be played directly from the BAB due to misalignment with block boundaries
   to respect for certain platforms, AND to avoid having to call dynamic kernel memory allocation on
   run time, thus avoiding potential memory shortages.
   NOTE: current prototype uses hard coding of page and total sizes; this should become dynamic based
         on BAB size, which is known at ftd_init() time.
   NOTE 2: this prototype has been written in a rush situation and, should it become permanently part
           of the product, should be improved in areas of error handling and messages, for instance.
*/
#define MEM_POOL_PAGE_SIZE ((int)(PAGE_SIZE))     // Use platform macro to determine page size
#define MEM_POOL_SIZE ((unsigned)(128*1024*1024)) // tests: 128 MB
#define MEM_POOL_NUM_PAGES (MEM_POOL_SIZE / MEM_POOL_PAGE_SIZE)
static ftd_int32_t  num_pool_pages_allocated = 0;
static ftd_int32_t  num_pool_pages_used = 0;

typedef struct mem_pool_struct
{
  struct  list_head  list;
  char    *mem_pool_page_ptr;
} mem_pool_ctl;

struct list_head free_pool_head;
struct list_head used_pool_head;
    
mem_pool_ctl  mem_pool_page_list[MEM_POOL_NUM_PAGES];

static spinlock_t  mem_pool_lck;

#endif // PRIVATE_MEMORY_POOL




#ifndef ROUND_UP
#define ROUND_UP(v, inc) ( ( (int) (((v) + (inc - 1)) / (inc)) ) * (inc) )
#endif

static inline int ftd_dispatch_io(ftd_dev_t *softp);
struct buf * ftd_dequeue_io(ftd_dev_t *softp);
void ftd_queue_io(ftd_dev_t *softp, struct buf * bio);
static int ftd_do_io(ftd_dev_t *softp, struct buf *bh);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define KMEM_CACHE_T kmem_cache_t
#else
#define KMEM_CACHE_T struct kmem_cache
#endif

static KMEM_CACHE_T *ftd_metabuf_cache = NULL;
static const char *ftd_metabuf_cache_name = DRIVERNAME "_metabuf";

static const size_t ftd_kmalloc_ceiling = 131071;
FTD_PUBLIC void kmem_free(void *addr, int size);
FTD_PUBLIC void *kmem_zalloc(int size, int flags);

FTD_PRIVATE ftd_void_t ftd_thread_lock( void *mutex_lock_ptr );    // WR PROD11894 SuSE 11.2
FTD_PRIVATE ftd_void_t ftd_thread_unlock( void *mutex_lock_ptr );

/*
 * these have to be allocated separately because external
 * subsystems want to have a pre-defined structure
 */

/*
 *  BAB size default is 2MB, this is tuned by insmod.
 */
int	chunk_size = 1048576;

#if 0
int	num_chunks = 2;
#else
int	num_chunks = 10;
#endif

#define FTDSTAT DRIVERNAME "stat"

/**
 *
 * @brief ftd_blkdev_get
 *
 * @internal Open the block device so that we can get the bdev's internal structure .
 *
 *
 */
int  ftd_blkdev_get(struct block_device * bdev, unsigned int flags )
{
	int err =0;

  #if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
    err = blkdev_get(bdev, FMODE_READ|FMODE_WRITE, flags);
  #elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
    err = blkdev_get(bdev, FMODE_READ|FMODE_WRITE, NULL);  // WR PROD11894 SuSE 11.2
  #else
    err = blkdev_get(bdev, FMODE_READ|FMODE_WRITE);
  #endif
    return err;

}
/**
 *
 * @brief ftd_blkdev_get
 *
 * @internal Release the block device structure.
 *
 */
void ftd_blkdev_put(struct block_device * bdev)
{
 #if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
	blkdev_put(bdev);
 #else
    blkdev_put(bdev, FMODE_READ|FMODE_WRITE);
 #endif
}



static inline int ftd_dispatch_io(ftd_dev_t *softp)
{
       struct buf *bio;
       int ret = 0;

       bio = ftd_dequeue_io(softp);

       if (bio) 
	   {
               ftd_do_io(softp, bio);
               ret = 1;
       }

       return(ret);
}

/*
 * Description:
 *      thread daemon on logical group
 *
 * Input(s):
 *      lgp     - logical group object
 *
 * Output(s):
 *      N/A
 *
 * Return(s):
 *      exit code
 */
static void ftd_thrd_func(ftd_lg_t *lgp)
{
        int cont = 1;
        ftd_dev_t *softp;
        ftd_context_t context;

        while (cont)
        {
                ACQUIRE_LOCK(lgp->lock, context);
                cont = 0;
                softp = lgp->devhead;
                while (softp)
                {
                        RELEASE_LOCK(lgp->lock, context);
                        cont |= ftd_dispatch_io(softp);
                        ACQUIRE_LOCK(lgp->lock, context);
                        softp = softp->next;
                }
                RELEASE_LOCK(lgp->lock, context);
        }
}

static int ftd_thrd_daemon(void *arg)
{
	ftd_lg_t *lgp = arg;
	ftd_context_t context;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
void *thrd_mutex_ptr = (void *)(&lgp->thrd_mutex);  // WR PROD11894 SuSE 11.2
#else
void *thrd_mutex_ptr = NULL;
#endif

	ftd_thread_lock( thrd_mutex_ptr );   // WR PROD11894 SuSE 11.2
        
	sprintf(current->comm, "TH_LG%d", lgp->th_lgno);
	/* Kernels >= 3.8 don't need daemonize() since we now use kthread_create(); daemonize() is deprecated at kernel >= 3.8 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 7, 0)
	daemonize(current->comm);
#endif

	ftd_thread_unlock( thrd_mutex_ptr );  // WR PROD11894 SuSE 11.2

    while (test_and_clear_bit(FTD_BITX_LG_STOP, &lgp->flags) == 0) 
	{
                ftd_thrd_func(lgp);
                /*
                 * set 1 sec timeout to prevent a thread from hanging
                 * in this sleep-on forever.
                 */
		          ftd_thread_lock( thrd_mutex_ptr );
                  interruptible_sleep_on_timeout(&lgp->th_sleep, HZ);
	              ftd_thread_unlock( thrd_mutex_ptr );
                  __set_current_state(TASK_RUNNING);
                  yield();

    }
        complete_and_exit(&lgp->th_exit, 0);
        return (0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
// PROD00013379: porting for RHEL 7.0
// The kernel function kernel_thread is effectively deprecated as of RHEL 7; here is the new API adjustment
FTD_PUBLIC void ftd_thrd_create(ftd_lg_t *lgp)
{
	static struct task_struct *lg_thread_task_struct;
	char lg_thread_string[32];
    // NOTE: the thread pid is set by kthread_create in task_struct	structure

        memset( lg_thread_string, 0, sizeof(lg_thread_string) );
        sprintf( lg_thread_string, "TH_LG%d", lgp->th_lgno );

        init_waitqueue_head(&lgp->th_sleep);
        init_completion(&lgp->th_exit);

		lgp->th_pid = -1;

        lg_thread_task_struct = kthread_create( ftd_thrd_daemon, lgp, lg_thread_string );

	    if((lg_thread_task_struct))
	    {
			lgp->th_pid = lg_thread_task_struct->pid;
	        wake_up_process(lg_thread_task_struct);
	    }

        if (lgp->th_pid < 0)
        {
            FTD_ERR(FTD_WRNLVL, "Failed to fork thread on LG%d, error = %d", lgp->th_lgno, lgp->th_pid);
            return;
        }

        ftd_debugf(ioctl, "ftd_thrd_create(%p): created TH_LG%d[%d]\n",lgp, lgp->th_lgno, lgp->th_pid);
}
#else
FTD_PUBLIC void ftd_thrd_create(ftd_lg_t *lgp)
{
        init_waitqueue_head(&lgp->th_sleep);
        init_completion(&lgp->th_exit);
        lgp->th_pid = kernel_thread(ftd_thrd_daemon, lgp, CLONE_FS | CLONE_FILES| CLONE_SIGHAND);

        if (lgp->th_pid < 0)
        {
                FTD_ERR(FTD_WRNLVL, "Failed to fork thread on LG%d, error = %d", lgp->th_lgno, lgp->th_pid);
                return;
        }

        ftd_debugf(ioctl, "ftd_thrd_create(%p): created TH_LG%d[%d]\n",lgp, lgp->th_lgno, lgp->th_pid);
}
#endif

/*
 * Description:
 *      wake up the waiting thread.
 *
 * Input(s):
 *      lgp     - logical group object
 *
 * Output(s):
 *      N/A
 *
 * Return(s):
 *      N/A
 */
static inline void ftd_thrd_wakeup(ftd_lg_t *lgp)
{
        wake_up_interruptible(&lgp->th_sleep);
}

/*
 * Description:
 *      destroy a specified logical group thread
 *
 * Input(s):
 *      lgp     - logical group object					 
 *
 * Output(s):
 *      N/A
 *
 * Return(s):
 *      N/A
 */
FTD_PUBLIC void ftd_thrd_destroy(ftd_lg_t *lgp)
{
    ftd_context_t context;

	if (lgp->th_pid > 0) 
	{
        ftd_debugf(delete, "ftd_thrd_destroy(%p): killing TH_LG%d[%d]\n", lgp, lgp->th_lgno, lgp->th_pid);
        set_bit(FTD_BITX_LG_STOP, &lgp->flags);
        ftd_thrd_wakeup(lgp);
        wait_for_completion(&lgp->th_exit);
    }
}

 
/*
 * Description:
 *
 *
 * Input(s):
 *
 *
 * Output(s):
 *
 *
 * Return(s):
 *
 */
dev_t ftd_makedevice(struct inode *in, unsigned int *major, unsigned int *minor)
{
    dev_t dev;
    *major = imajor(in);
    *minor = iminor(in);

    dev = MKDEV(*major, *minor);
    return(dev);
}


/**
 *
 * @brief ftd_blk_open
 *
 * @internal Open the block device that we want to replicate.
 *
 * @todo - Use the private_data to simplify intenal logic.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static int ftd_blk_open(struct inode *inode, struct file *filp)
#else
static int ftd_blk_open(struct block_device *block_dev, fmode_t mode)
#endif
{
    minor_t  minor=0;
    major_t  major=0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
    unsigned int  flags = filp->f_flags;
#else
    unsigned int  flags = 0;

    struct inode *inode = block_dev->bd_inode;
	if ( inode == NULL)
	{
        return FLIP_ERROR(ENXIO);
	}
    
#endif
    
    dev_t dev = ftd_makedevice(inode, &major, &minor);

    ftd_dev_t *softp;
    
struct block_device *bdev;
    struct inode *dev_inode;
    mm_segment_t old_fs;
	int err = 0;

   
    softp = ddi_get_soft_state(ftd_dev_state, U_2_S32(minor));

    if (softp) 
    {
        bdev = bdget(softp->localbdisk);

        ftd_blkdev_get (bdev, flags);

        if (!bdev->bd_disk->fops || !bdev->bd_disk->fops->open)
        {
            err = ENOTTY;
        }
        else 
        {
            old_fs = get_fs();
            dev_inode = bdev->bd_inode;
            ftd_debugf(open, "ftd_blk_open: bdev->inode %p i_rdev %x i_count %d\n", dev_inode, dev_inode->i_rdev, atomic_read(&dev_inode->i_count));
            err = ftd_dev_open(dev);
            if (!err) 
            {
                set_fs(KERNEL_DS);

  #if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
		err = bdev->bd_disk->fops->open(dev_inode, filp);
  #else
		err = bdev->bd_disk->fops->open(bdev ,mode);
  #endif	 
                set_fs(old_fs);
            }
        }

        ftd_blkdev_put(bdev);

    } 
    else
    {
        err = ENXIO;
    }
    return FLIP_ERROR(err);
}

static int ftd_char_open (struct inode *inode, struct file *filp)
{
	ftd_uint32_t major, minor;
	dev_t dev = ftd_makedevice(inode, &major, &minor);

	int err = 0;

	if (minor == FTD_CTL) {
		ftd_debugf(open, "ftd_char_open(%p, %p): ftd_ctl_open: i_rdev %x i_count %d\n",
            inode, filp, inode->i_rdev, atomic_read(&inode->i_count));
		err = ftd_ctl_open();
	} else {
		ftd_debugf(open, "ftd_char_open(%p, %p): ftd_lg_open(%x, %p)\n",
            inode, filp, (unsigned)dev, filp);
		err = ftd_lg_open(dev,filp);
	}
    return FLIP_ERROR(err);
}


/**
 *
 * @brief ftd_blk_release
 *
 * @internal Close the block device that we want to replicate.
 *
 * @todo - Use the private_data to simplify intenal logic.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static int ftd_blk_release(struct inode *inode, struct file *filp)
#else
static int ftd_blk_release(struct gendisk *gendisk, fmode_t mode)
#endif
{
    unsigned int major, minor;	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
    dev_t dev = ftd_makedevice(inode, &major, &minor);
#else
    
    struct block_device  *bdev1 = gendisk->private_data;

    major = bdev1->bd_disk->major;
    minor = bdev1->bd_disk->first_minor;

	dev_t dev = MKDEV(major, minor);

#endif

    ftd_dev_t *softp;
    struct block_device *bdev;
    struct inode *dev_inode;
    mm_segment_t old_fs;
	int err = 0;


    softp = ddi_get_soft_state(ftd_dev_state, U_2_S32(minor));

    if (softp) 
    {
        bdev = bdget(softp->localbdisk);
        ftd_blkdev_get(bdev,  0);

        if (!bdev->bd_disk->fops || !bdev->bd_disk->fops->release)
        {

            err = ENOTTY;
        }
        else 
        {
            dev_inode = bdev->bd_inode;
            ftd_debugf(open, "ftd_blk_release: bdev->inode %p i_rdev %x i_count %d\n",dev_inode, dev_inode->i_rdev, atomic_read(&dev_inode->i_count));

		#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
            err = bdev->bd_disk->fops->release(dev_inode, NULL);
		#else
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
			    // Kernels >= 3.8 return void at release() call
			    err = 0;
	            bdev->bd_disk->fops->release(bdev->bd_disk, mode);
			#else
	            err = bdev->bd_disk->fops->release(bdev->bd_disk, mode);
			#endif
		#endif
        }

        err = ftd_dev_close(dev);

        ftd_blkdev_put(bdev);

    } 
    else
    {
        err = ENXIO;
    }

    return FLIP_ERROR(err);
}

static int ftd_char_release (struct inode *inode, struct file *filp)
{
	ftd_uint32_t major, minor;
	dev_t dev = ftd_makedevice(inode, &major, &minor);
	int err;

	if( minor == FTD_CTL ) {
		ftd_debugf(open, "ftd_char_release(%p, %p): ftd_ctl_close i_count = %d\n",
            inode, filp, atomic_read(&inode->i_count));
		err = ftd_ctl_close();
	} 
	else 
	{
		//ftd_debugf(open, "ftd_char_release(%p, %p): ftd_lg_close(%x)\n",inode, filp, (unsigned)dev);
		err = ftd_lg_close(dev);
	}
    return FLIP_ERROR(err);
}

FTD_PUBLIC ftd_int32_t ftd_ctl_get_device_nums (dev_t dev, ftd_intptr_t arg, ftd_int32_t flag)
{
	ftd_devnum_t *devnum;

	devnum = (ftd_devnum_t *) arg;
	devnum->c_major = ftd_cmajor;
	devnum->b_major = ftd_bmajor;
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)
static int ftd_getgeo( struct block_device *bdev, struct hd_geometry *geo)
{
    ftd_dev_t *softp;
    ftd_uint32_t major = bdev->bd_disk->major;
    ftd_uint32_t minor = bdev->bd_disk->first_minor;
    int err;

    ftd_debugf( ioctl, "ftd_getgeo: major %u minor %u\n", major, minor );

    if((softp = ddi_get_soft_state(ftd_dev_state, U_2_S32(minor))) == NULL)
    {
	return -ENXIO;
    }
    bdev = bdget(softp->localbdisk);

    ftd_blkdev_get(bdev, O_RDWR|O_SYNC);


    if (!bdev->bd_disk->fops || !bdev->bd_disk->fops->getgeo) 
    {

        ftd_blkdev_put(bdev);

        return -ENOTTY;
    }
	
    err = bdev->bd_disk->fops->getgeo( bdev, geo );

    ftd_blkdev_put(bdev);

    ftd_debugf(ioctl, "ftd_getgeo: return %d\n", err);
    return err;
}
#endif

/*
 * The ioctl() implementation
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static int ftd_blk_common_ioctl (struct block_device *bdev, dev_t dev, struct file *filp, unsigned cmd, unsigned long arg)
#else
static int ftd_blk_common_ioctl (struct block_device *bdev, dev_t dev, fmode_t mode, unsigned cmd, unsigned long arg)
#endif
{
    int intval;
    ftd_debugf( ioctl, "blk_common_ioctl: dev %lx cmd %u\n", (long)dev, cmd );
    switch (cmd) 
    {
    case BLKBSZSET:
        /* set the logical block size */
        if (!capable (CAP_SYS_ADMIN))
            return -EACCES;
        if (!dev || !arg)
            return -EINVAL;
        if (get_user (intval, (int *) arg))
            return -EFAULT;
        if (intval > PAGE_SIZE || intval < 512 || (intval & (intval - 1)))
            return -EINVAL;

/*
WR PROD11894 SuSE 11.2
From Linux technical notes:
* blkdev_get() is extended to include exclusive access management.
  @holder argument is added and, if is @FMODE_EXCL specified, it will
  gain exclusive access atomically w.r.t. other exclusive accesses.

* blkdev_put() is similarly extended.  It now takes @mode argument and
  if @FMODE_EXCL is set, it releases an exclusive access.  Also, when
  the last exclusive claim is released, the holder/slave symlinks are
  removed automatically.

* bd_claim/release() and close_bdev_exclusive() are no longer
  necessary and either made static or removed.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
		if (!(mode & FMODE_EXCL) &&
		    blkdev_get(bdev, mode | FMODE_EXCL, &bdev) < 0)
 			return -EBUSY;
        set_blocksize (bdev, intval);
 		if (!(mode & FMODE_EXCL))
			blkdev_put(bdev, mode | FMODE_EXCL);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
        if (bd_claim(bdev, filp) < 0)
            return -EBUSY;
#else
        if (bd_claim(bdev, &bdev) < 0)
            return -EBUSY;
#endif
        set_blocksize (bdev, intval);
        bd_release(bdev);
#endif

        break;

    case BLKRASET:

        if(!capable(CAP_SYS_ADMIN))
            return -EACCES;
        if(arg > 0xff)
            return -EINVAL;
        break;

    case BLKFLSBUF:

        /*
         * flush dtc device buffers
         */
        if (!capable(CAP_SYS_ADMIN))
            return -EACCES;
        fsync_bdev(bdev);

    #if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
        invalidate_bdev(bdev, 0);
    #else
        invalidate_bdev(bdev);
    #endif
        break;

    }

    ftd_debugf( ioctl, "blk_common_ioctl: return 0\n" );
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static int ftd_blk_ioctl (struct inode *inode, struct file *filp, unsigned cmd, unsigned long arg)
#else
static int ftd_blk_ioctl (struct block_device *blockdev, fmode_t mode,  unsigned cmd, unsigned long arg)
#endif
{
    ftd_dev_t *softp;
    struct block_device *bdev;
    
    ftd_uint32_t major, minor;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
#else
    struct inode *inode = blockdev->bd_inode;    
#endif
    
    dev_t dev = ftd_makedevice(inode, &major, &minor);
    
    ftd_int32_t err = 0;

    if((softp = ddi_get_soft_state(ftd_dev_state, U_2_S32(minor))) == NULL)
    {
    	return -ENXIO;
    }


    bdev = bdget(softp->localbdisk);

    ftd_blkdev_get(bdev,  O_RDWR|O_SYNC);

    if (!bdev->bd_disk->fops)
    {

      ftd_blkdev_put(bdev);

        ftd_debugf(ioctl, "ftd_blk_ioctl: blkdev:fops null\n" );
        return -ENOTTY;
    }


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
    err = ftd_blk_common_ioctl(bdev, dev, filp, cmd, arg);
#else
    err = ftd_blk_common_ioctl(bdev, dev, mode, cmd, arg);
#endif

    ftd_debugf(ioctl, "ftd_blk_common_ioctl: err=%d  bdev %p  \n", err , bdev);

    if (err == 0)
        err = ioctl_by_bdev( bdev, cmd, arg );

    ftd_debugf(ioctl, "ioctl_by_bdev: bdev %p err=%d \n", bdev , err );

    ftd_blkdev_put(bdev);

    err = FLIP_ERROR(err);
    ftd_debugf(ioctl, "ftd_blk_ioctl: return %d\n", err);
    return err;
}

#if defined(CONFIG_COMPAT) && defined(HAVE_COMPAT_IOCTL)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static long ftd_blk_compat_ioctl (struct file *filp, unsigned cmd, unsigned long arg)
#else
static long ftd_blk_compat_ioctl (struct block_device * blk_dev, fmode_t mode, unsigned cmd, unsigned long arg)
#endif
{
    ftd_dev_t *softp;
    long err, err1;
    struct block_device *bdev;

    unsigned int bdev_flags = O_RDWR|O_SYNC;

	struct file fake_file = {};
	struct dentry fake_dentry = {};
	mode_t bdev_mode = FMODE_READ|FMODE_WRITE;


	#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
		struct inode *inode = filp->f_dentry->d_inode;
	#else
		struct inode *inode = blk_dev->bd_inode;
	#endif

    ftd_uint32_t major, minor;

    dev_t dev = ftd_makedevice(inode, &major, &minor);

    ftd_debugf(ioctl, "ftd_blk_compat_ioctl(%x, %lx): dev %x\n", cmd, arg, dev);

    if((softp = ddi_get_soft_state(ftd_dev_state, U_2_S32(minor))) == NULL)
	return -ENXIO;

    #if defined (LINUX260)

    bdev = bdget(softp->localbdisk);

    ftd_blkdev_get(bdev, O_RDWR|O_SYNC);

	#else
		#error "!LINUX260 and HAVE_COMPAT_IOCTL are not expected together."
	#endif /* defined (LINUX260) */

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
		err = ftd_blk_common_ioctl(bdev, dev, filp, cmd, arg);
	#else
		err = ftd_blk_common_ioctl(bdev, dev, mode, cmd, arg);
	#endif

    if (err == 0) 
    {
	if (bdev->bd_disk->fops->compat_ioctl)
	{

		#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)

	    fake_dentry.d_inode = bdev->bd_inode;
	    fake_file.f_mapping = bdev->bd_inode->i_mapping;
	    fake_file.f_dentry = &fake_dentry;
	    fake_file.f_flags = bdev_flags;
	    fake_file.f_mode  = bdev_mode;

	    err1 = bdev->bd_disk->fops->open(bdev->bd_inode, &fake_file);
		#else
	    err1 = bdev->bd_disk->fops->open(bdev , mode);
		#endif
	    if (err1) 
        {
		    FTD_ERR (FTD_WRNLVL, "compat_ioctl open failed dev %x, err %d", dev, err);
	    }

		#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
	    err = bdev->bd_disk->fops->compat_ioctl(&fake_file, cmd, arg);
    	err1 = bdev->bd_disk->fops->release(bdev->bd_inode, &fake_file);

		#else
        err = bdev->bd_disk->fops->compat_ioctl(bdev, mode, cmd, arg);
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
			    // Kernels >= 3.8 return void at release() call
			    err1 = 0;
	            bdev->bd_disk->fops->release(bdev->bd_disk, mode);
			#else
   	            err1 = bdev->bd_disk->fops->release(bdev->bd_disk, mode);
			#endif
		#endif

			if (err1) 
			{
				FTD_ERR (FTD_WRNLVL, "compat_ioctl release failed dev %x, err %d", dev, err);
			}
    	}
    	else 
    	{
    		err = -ENOIOCTLCMD;
    	}
    }

    #if defined (LINUX260)

    ftd_blkdev_put(bdev);

	#else
		#error "!LINUX260 and HAVE_COMPAT_IOCTL are not expected together."
	#endif
    err = FLIP_ERROR(err);
    ftd_debugf(ioctl, "ftd_blk_compat_ioctl: return %ld\n", err);
    return err;
}
#endif

/**
 *
 * @brief ioctl handler for the character devices.
 *
 * @internal This code contains an automatic wrapper to copy the ioctl parameter between user and kernel space as
 *           this needs to be explicitely done for linux.
 *
 * @internal This mechanism is a replacement for the previous mechanism which was based on register_ioctl32_conversion() from <linux/ioctl32.h>.
 *           The previous mechanism was just a hack and didn't properly move the ioctl argument betwen user and kernel space when
 *           both had the same address size, or when the mechanism was not available (CONFIG_COMPAT undefined).
 *           Additionally, a new translation mechanism was added in release 2.6.11 (compat_ioctl) and the older <linux/ioctl32.h> compatibility
 *           mechanism was removed a few versions later.
 *
 * @internal This solution was chosen since it was a fast and safe upgrade path from the previous mechanism, but it still is currently not
 *           believed to be the proper way to do things.
 *
 * @todo     It seems that a better way to do things would be to reuse the fact that the actual ioctl implementations within ftd_ioctl.c
 *           already make a local copy of the argument, but with a space agnostic bcopy().
 *           Before taking this route, we must investigate and better understand:
 *           -) Why local copies of the arguments are made using bcopy()?
 *              Other unix OSes automatically take care of copying/mapping the argument into kernel space,
 *              Why the need for a local copy?
 *              (This was confirmed for HP-UX, investigate for AIX and SunOS.)
 *           -) Rethink about the #ifdef-MISALIGNED-binary-copy-otherwise-bcopy way to deal with the local copies.
 *           -) Would just mapping the argument into kernel space make more sense than copying?
 *           -) If local copies are not needed for all other OSes, then the actual mechanism might indeed be a valid way
 *              to do things.
 *
 * @bug The actual mechanism has to be aware that some ioctls implementation are linux specific and it must be ensured that
 *      either the mechanism is disabled for these ioctls, or that these ioctl implementations do not take take of copying
 *      the ioctl argument in and out of kernel space themselves.
 *      Here are the current linux specific ioctls: FTD_OPEN_NUM_INFO(*),
 *      The ones marked with an asterisk(*) were already calling copy_from/to_user() by themselves.
 *
 * @todo Investigate if FTD_OPEN_NUM_INFO is really needed and remove it if not.
 *       A quick look seems to show that FTD_OPEN_NUM_INFO is possibly never ever called.
 *
 */
static int
ftd_char_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{

	ftd_uint32_t major, minor;


	dev_t dev = ftd_makedevice(inode, &major, &minor);


	ftd_int32_t flag = 0;
	ftd_int32_t err;

    // For linux ioctls, we have to take care ourselves of bringing in and out of kernel space the ioctl argument.
    // It is automatically done here for all ioctls implemented within ftd_ctl/lg_ioctl.
    // See the above @internal comments.
    const size_t arg_size = _IOC_SIZE(cmd);
    void* kernel_copy_of_arg = NULL;

    // The following logic is considered a bug, see the above @bug documentation.
    const int need_to_make_local_copy = (cmd != FTD_OPEN_NUM_INFO);
    
    if(need_to_make_local_copy && arg_size)
    {
       kernel_copy_of_arg = kmalloc(arg_size, GFP_KERNEL);

       if(kernel_copy_of_arg == NULL)
       {
          err = -ENOMEM;
          goto cleanup;
       }

       if(copy_from_user(kernel_copy_of_arg, (void *)arg, arg_size) != 0)
       {
          ftd_debugf(ioctl,
                     "ftd_char_ioctl(%x, %lx, %p): copy_from_user(%p, %lx, %zd) failed\n",
                     cmd, arg, filp,
                     kernel_copy_of_arg, arg, arg_size);
          err = -EFAULT;
          goto cleanup;
       }
    }
    
	if (minor == FTD_CTL) 
	{
		err = ftd_ctl_ioctl (dev, cmd, (ftd_intptr_t)kernel_copy_of_arg, flag);
	} 
	else 
	{
		err = ftd_lg_ioctl (dev, cmd, (ftd_intptr_t)kernel_copy_of_arg, flag);
    }

    if(kernel_copy_of_arg != NULL &&
       (_IOC_DIR(cmd) & _IOC_READ))
    {
       if (copy_to_user((void *)arg, kernel_copy_of_arg, arg_size))
       {
          ftd_debugf(ioctl,
                     "ftd_char_ioctl(%x, %lx, %p): copy_to_user(%p, %p, %zd) failed\n",
                     cmd, arg, filp,
                     (void*)arg, kernel_copy_of_arg, arg_size);
          err = -EFAULT;
          goto cleanup;
       }
    }

 cleanup:
    if (kernel_copy_of_arg != NULL)
    {
       kfree(kernel_copy_of_arg);
    }
        
	return FLIP_ERROR(err);
}

#if defined(CONFIG_COMPAT) && defined(HAVE_COMPAT_IOCTL)
static long ftd_char_compat_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    return ftd_char_ioctl(filp->f_dentry->d_inode, filp, cmd, arg);
}
#endif

// WR PROD11894 SuSE 11.2
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
static long ftd_char_unlocked_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    return (long)(ftd_char_ioctl(filp->f_dentry->d_inode, filp, cmd, arg));
}
#endif

unsigned int ftd_poll(struct file *filp, poll_table *wait)
{
	ftd_lg_t *lgp = filp->private_data;
	unsigned int mask = 0;
	if ((lgp->state & FTD_M_JNLUPDATE) == 0 || lgp->wlentries) 
	{
		mask |= POLLIN | POLLRDNORM;
	}
	else {
		/* set the entry to the wait queue.           */
		/* that entry will be waked up by ftd_wakeup. */
		poll_wait(filp, &lgp->ph,  wait);
		/* mask is 0. meening there is no data to read. */
	}
	return mask;
}


static struct block_device_operations ftd_bdops =
{
	owner:              THIS_MODULE,
	open:               ftd_blk_open,
	release:            ftd_blk_release,
	ioctl:              ftd_blk_ioctl,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)
	getgeo:		    ftd_getgeo,
#endif
#if defined(CONFIG_COMPAT) && defined(HAVE_COMPAT_IOCTL)
	compat_ioctl:         ftd_blk_compat_ioctl,
#endif
	//check_media_change: ftd_check_change,
	//revalidate:         ftd_revalidate,
};

struct file_operations ftd_cdops =
{
	owner:              THIS_MODULE,
	//read:               short_read,
	//write:              short_write,
	poll:               ftd_poll,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	ioctl:              ftd_char_ioctl,
#else
	unlocked_ioctl:     ftd_char_unlocked_ioctl,    // WR PROD11894 SuSE 11.2
#endif
#if defined(CONFIG_COMPAT) && defined(HAVE_COMPAT_IOCTL)
	compat_ioctl:       ftd_char_compat_ioctl,
#endif
	open:               ftd_char_open,
	release:            ftd_char_release,
};

/*
 * ftd_get_softp - return soft state for the FTD device from which
 * this I/O originated.
 *
 * Parameters:
 *     struct buf *bp - pointer to cloned buffer.
 *
 */
FTD_PUBLIC ftd_dev_t *
ftd_get_softp (struct buf * bp)
{
	ftd_dev_t *softp;
	struct buf *userbp;
	minor_t minor;

	ftd_debugf(pstore, "ftd_get_softp: enter\n");
	userbp = BP_USER (bp);

	if (userbp == NULL)
	{
		ftd_debugf(pstore, "ftd_get_softp: BP_USER(%p) = NULL\n", bp);
		FTD_ERR (FTD_WRNLVL, "Got a bad bp! %p userbp NULL", bp);
		return NULL;
	}


        minor = FTDGETMINOR(BP_DEV (userbp));

    softp = ftd_dynamic_activation_get_replication_device_structure(BP_DEV(userbp));
	if (softp == NULL) {

		ftd_debugf(pstore, "ftd_get_softp: return NULL\n");
		FTD_ERR (FTD_WRNLVL, "Got a bad softp! my %x user %x dev %d",bp, userbp, BP_DEV (userbp));

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
		bio_endio(userbp, userbp->bi_size, 1);
#  else
		bio_endio(userbp, 1);
#  endif

		return NULL;
	}
	ftd_debugf(pstore, "ftd_get_softp: return %p\n", softp);
	return softp;
}


/*-
 * ftd_biodone()
 *
 * called to biodone buffers returned from the driver below.
 */
FTD_PUBLIC ftd_void_t ftd_biodone (struct buf * bp, int ioerror)
{
	ftd_debugf(iodone, "ftd_biodone(%p)\n", bp);


#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	bio_endio(bp, bp->bi_size, ioerror);
#  else
	bio_endio(bp, ioerror);
#  endif

}


/*-
 * ftd_finish_io()
 *
 * this routine is called when we want to finish up an I/O that has
 * completed.  dodone is true if we're supposed to biodone the user's
 * buf.
 *
 * once we finish up mybp and biodone the users' bp, we then see if we
 * have things to do (softp->qhead).  if we do, then we remove the
 * head of the queue and start that I/O.  otherwise, we place mybp
 * back on the free list.
 *
 * the purpose of this routine is to make sure that we do all of
 * these things every time we're finishing up an I/O.  since we have
 * two iodone routines that can be called, it makes sense to have the
 * common code here.
 */
FTD_PRIVATE ftd_void_t ftd_finish_io (ftd_dev_t * softp, struct buf *bh, ftd_int32_t ioerror)
{
	ftd_context_t context;
	struct buf *userbp;

	ftd_debugf(iodone, "ftd_finish_io(%p, %p, %d)\n", softp, bh, ioerror);

	/* driver and kernel buffer references */
	userbp = BP_USER (bh);

	ACQUIRE_LOCK(softp->lgp->lock, context);
    if (BP_RW(bh) == WRITE)
        {
        //##@@
        softp->lgp->ulWriteDoneByteCount += userbp->b_bcount;
        }
	RELEASE_LOCK(softp->lgp->lock, context);
    atomic_dec(&pending_ios);
    if (BP_RW(bh) == WRITE)
        atomic_dec(&pending_writes);

		/*-
		 * if we're acking the I/O to the upper layers, then do so
		 * now. if we have an I/O delay, then wait that long before
		 * doing so do help throttle the applications.  don't hold
		 * locks around calls to biodone.
		 */
    if (!ioerror || !LG_DO_SYNCMODE (softp->lgp))
	ftd_biodone (userbp, ioerror);

    // It is important to decrement the device's pending_ios counter only when it is safe to delete the softp device.
    // This is because this counter is used in determining if it is safe or not to stop (and later on delete) a device
    // by the dynamic activation code while processing the FTD_INIT_STOP ioctl.
    // See additional comments within captured_io_monitoring_ensure_release_is_safe_and_init_release().
    pending_ios_monitoring_unregister_pending_io(softp);
    
	/*-
	 * free up the driver's buffer.
	 */
    kmem_cache_free(ftd_metabuf_cache, bh);
}

/* Modules to obtain / release private static memory pool pages */
#ifdef PRIVATE_MEMORY_POOL
static void  *obtain_mem_pool_page(int length)
{
  mem_pool_ctl *next_free;
  char  *allocated_page;
  ftd_context_t context;


  ACQUIRE_LOCK( mem_pool_lck, context );
  if( length > MEM_POOL_PAGE_SIZE )
  {
     printk( "<<< Mem pool: page size required (%d) exceeds page size of mem pool (%d)\n", length, MEM_POOL_PAGE_SIZE );
     RELEASE_LOCK( mem_pool_lck, context );
     return( 0 );
  }
  if( num_pool_pages_used == num_pool_pages_allocated )
  {
     if( printk_ratelimit() )
     {
         printk( "<<< Mem pool allocation request: all mem pool pages are used (%d)\n", num_pool_pages_allocated );
     }
     RELEASE_LOCK( mem_pool_lck, context );
     return( 0 );
  }

  next_free = (mem_pool_ctl *)free_pool_head.next;
  allocated_page = next_free->mem_pool_page_ptr;

  // Move the control record of the allocated page to the used list
  list_move_tail( ((struct list_head *)next_free), (&used_pool_head) );
  num_pool_pages_used++;

#ifdef TRACE_PRIVATE_MEMORY_POOL
  printk( "<<< Allocated page from memory pool at address %p; num of used pages = %d\n", allocated_page, num_pool_pages_used );
#endif
  RELEASE_LOCK( mem_pool_lck, context );
  return( (void *)allocated_page ); 
}

static int  release_mem_pool_page(char *page_to_release)
{
  ftd_context_t context;
  mem_pool_ctl *next_used;
  char         *allocated_page;
  int          j;
  int          result;
  int          found;

  result = 0;   // Will be set to -1 if error
  found = 0;

  ACQUIRE_LOCK( mem_pool_lck, context );
  if( num_pool_pages_used == 0 )
  {
     printk( "<<< ERROR: release_mem_pool_page() called while no mem page used\n" );
     result = -1;
  }
  else
  {
     // Find the control record of the page to release
     for( j = 0, next_used = (mem_pool_ctl *)used_pool_head.next;
          j < (int)num_pool_pages_used; 
          j++, next_used = (mem_pool_ctl *)(next_used->list.next) )
     {
	if( next_used->mem_pool_page_ptr == page_to_release )
	{  // Found page control record; move it to free list
           found = 1;
           list_move_tail( ((struct list_head *)next_used), (&free_pool_head) );
           --num_pool_pages_used;
#ifdef TRACE_PRIVATE_MEMORY_POOL
           printk( "<<< Released page from memory pool at address %p; num of used pages = %d\n", page_to_release, num_pool_pages_used );
#endif
	   break;
	}
     }
     if( !found )
     {
         printk( "<<< ERROR: memory pool page to release (adress %p) not found in used list\n", page_to_release );
         result = -1;
     }
  }
  RELEASE_LOCK( mem_pool_lck, context );
  return( result );
}

static void show_free_mem_pool_list( int all )
{
  ftd_context_t context;
  int j, show_page;
  mem_pool_ctl *mem_pool_record_ptr;

  ACQUIRE_LOCK( mem_pool_lck, context );
  printk("<<< Private memory pool list:\n" );
  for( j = 0, mem_pool_record_ptr = (mem_pool_ctl *)free_pool_head.next;
       j < num_pool_pages_allocated && mem_pool_record_ptr != 0; 
       j++, mem_pool_record_ptr = (mem_pool_ctl *)(mem_pool_record_ptr->list.next) )
  {
     show_page = all || (j == 0) || (j == num_pool_pages_allocated - 1);
     if( show_page )
     {
       printk( "<<< Memory pool page %d at adress %p\n", j, mem_pool_record_ptr->mem_pool_page_ptr );
     }
  }
  printk( "<<< Number of pages allocated = %d, requested = %d\n", num_pool_pages_allocated, MEM_POOL_NUM_PAGES );
  RELEASE_LOCK( mem_pool_lck, context );
}
 
#endif // PRIVATE_MEMORY_POOL



/*-
 *
 * we have the driver below us call this routine when we're doing
 * simple things.  for all reads we go through here, as well as writes
 * that we're not journalling for some reason.  this routine is
 * supposed to be fairly simple.  we get the pointer to the user's bp,
 * modify some stats, adjust the resid(ual) for the buf and then call
 * ftp_finish_io to wrap things up.
 *
 * same as ftd_iodone_generic() for otherUNIX
 */

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
static int ftd_end_request_generic (struct bio *bh, unsigned int nums, int err)
#  else
static void ftd_end_request_generic (struct bio *bh, int err)
#  endif

{
	ftd_dev_t *softp;
	ftd_lg_t *lgp;
	ftd_context_t context;
	minor_t minor;
	int ioerror = 0;

	struct buf *myb = BP_USER(bh);
  int uptodate = test_bit(BIO_UPTODATE, &bh->bi_flags);

	/* bp -> device state */
	if (!(softp = ftd_get_softp (bh)))
	{
		ftd_debugf(iodone, "ftd_end_request_generic: no softp\n");
		/* user buffer release in ftd_get_softp */
		FTD_ERR (FTD_WRNLVL, "ftd_end_request_generic:Can't get softp! uptodate %d bh %p",uptodate,bh);
#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
		return 1;
#else
		return;
#endif
	} 

#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	if (bh->bi_size)
	{
	    FTD_ERR(FTD_WRNLVL, "ftd_iodone_generic: bi_size set");
	    ioerror = 1;
	    ftd_finish_io (softp, bh, ioerror);
	    return 0;
	}
#endif 
	ftd_debugf(iodone, "ftd_end_request_generic(%p, %d)\n", bh, uptodate);


	if (!uptodate && err)
	{
	    if (err == -EIO)
		FTD_ERR (FTD_WRNLVL, ":ftd_end_request_generic:Device I/O Error! uptodate %d", uptodate);
	    ioerror = err;
	}


	if (!uptodate && err)
	{
	    ftd_debugf(iodone, "ftd_end_request_generic: ftd_finish_io(%p, %d) with error\n",
            bh, uptodate);
	    ftd_finish_io (softp, bh, ioerror);
#if defined(LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	    return 0;
#else
	    return;
#endif
	}

	/* get logical group */
	lgp = softp->lgp;

	/* update per dev stats */
	switch (BP_RW(bh))
	{
	case READA:
	case READ:
		ftd_debugf(iodone, "ftd_end_request_generic: READ\n");
		softp->readiocnt++;

		softp->sectorsread += (myb->bi_size >> DEV_BSHIFT);

		break;
	case WRITE:
		ftd_debugf(iodone, "ftd_end_request_generic: WRITE\n");
		softp->writeiocnt++;

		softp->sectorswritten += (myb->bi_size >> DEV_BSHIFT);

		break;
	}

	ftd_finish_io (softp, bh, ioerror);
	ftd_debugf(iodone, "ftd_end_request_generic: return\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    return 0;
   #endif
}

#ifdef DO_CKSUMS
#define uint_fast32_t unsigned int
uint_fast32_t cksum_user_data_in_bio(struct bio *bh);
#endif


#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
static int ftd_end_request_journal (struct bio *bh, unsigned int nums, int err)
#  else
static void ftd_end_request_journal (struct bio *bh, int err)
#  endif

{
	ftd_int32_t length;
	wlheader_t *hp;
	ftd_lg_t *lgp;
	bab_mgr_t *mgr;
	ftd_dev_t *softp;
	ftd_int32_t dowakeup;
	ftd_int32_t ioerror = 0;
	struct buf *userbp, *mybp = bh;
	minor_t minor;
	ftd_context_t context;


  int uptodate = test_bit(BIO_UPTODATE, &bh->bi_flags);


#ifdef BIO_DATA_FROM_BAB
  // Should always be true once the hacked mechanism is completed.
  if(bh->bi_private != NULL)
    {
       // Arrange things so that mybp and bh can be used again to reobtain data within the ftd_metabuf_t.
       bh = bh->bi_private;
       bh->bi_size = mybp->bi_size; // This copy allows us to free the duplicated data bio immediately, as this was found to be the only data needed.
       bio_put(mybp);
       mybp = bh; // This allows not having to modify the following mix of bh and mybp usages.

       if(BP_PRIVATE(bh))
       {
          void** array_of_allocated_data_copies = BP_PRIVATE(bh);
          int i;
          
          for(i = 0; i < mybp->bi_vcnt; i++)
          {
             if(array_of_allocated_data_copies [i])
             {
#ifdef PRIVATE_MEMORY_POOL
                release_mem_pool_page(array_of_allocated_data_copies[i]);
#else
                kfree(array_of_allocated_data_copies[i]);
#endif // PRIVATE_MEMORY_POOL
             }
          }
          kfree(array_of_allocated_data_copies);
       }
    }
#endif
    
	/* bp -> device state */
	if (!(softp = ftd_get_softp (bh)))
	{
		ftd_debugf(iodone, "ftd_end_request_journal(%p): no softp\n", bh);
		FTD_ERR (FTD_WRNLVL, "ftd_end_request_journal:Can't get softp! uptodate %d bh %p",uptodate,bh);
#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
		return 1;
#else
		return;
#endif
	}

#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
  if (bh->bi_size)
  {
     FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: bi_size set");
     ioerror = 1;
     ftd_finish_io (softp, bh, ioerror);
     return 0;
  }
#endif 

	ftd_debugf(iodone, "ftd_end_request_journal(%p, %d): userbp %p\n",
        bh, uptodate, BP_USER(bh));

	/* write log header, group, bab mgr references */
	hp = (wlheader_t *) BP_WLHDR (mybp);
	if (hp->majicnum != DATASTAR_MAJIC_NUM)
		FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM");
	lgp = (ftd_lg_t *)(unsigned long) hp->group_ptr;
	mgr = lgp->mgr;

	ACQUIRE_LOCK (lgp->lock, context);

	/* test state consistency. */
	userbp = (struct buf *)(unsigned long)hp->bp;
	if (userbp == NULL)
	{
		panic ("ftd_end_request_journal: BAB entry corrupt bh %p hp %p\n", bh, hp);
	}

	/* header state transistion. pending -> complete */
	hp->bp = (ftd_uint64ptr_t)NULL;

	hp->complete = 1;
	mgr->pending_io--;
    atomic_dec(&pending_io);
	/* whether to wake the daemon */
	dowakeup = 0;

	/* update disk stats */
	softp->writeiocnt++;

	softp->sectorswritten += (userbp->bi_size >> DEV_BSHIFT);

	softp->wlentries++;
	softp->wlsectors += hp->length;

	/* update group stats */
	lgp->wlentries++;
	lgp->wlsectors += hp->length;

	/* complete the IO, according to error states and current driver mode */
        if (!uptodate && err)
	{
	    if (err == -EIO)
		FTD_ERR (FTD_WRNLVL, "ftd_end_request_journal:Device I/O Error! uptodate %d", uptodate);
	    ioerror = err;
	}
	else
	{
		/* asynch mode */
		hp->bp = (ftd_uint64ptr_t)NULL;
		hp->timoid = INVALID_TIMEOUT_ID;
		if (LG_DO_SYNCMODE (lgp) && lgp->sync_timeout > 0 &&
		    !(lgp->mgr->flags & FTD_BAB_DONT_USE)) {
			/* synch mode */
			unsigned long expire;
			struct timer_list *my_timer;
			hp->bp = (ftd_uint64ptr_t)(unsigned long) userbp;
			// in case if time not expires.
			ftd_debugf(iodone, "ftd_end_request_journal: arm ftd_synctimo(%p) timeout %u\n",
                hp, lgp->sync_timeout);
			expire = (lgp->sync_timeout * HZ) + jiffies;
			my_timer = &(*(wlheader_t **)&(hp))->timer;
			hp->timoid = (ftd_uint64ptr_t)(unsigned long)my_timer;
			init_timer(my_timer);
			my_timer->expires = expire;
			my_timer->data = (unsigned long) hp;
			my_timer->function = ftd_synctimo;
			add_timer(my_timer);
			ioerror = 1;
		}
	}

	/*-
	 * if this is the first pending request, commit it.
	 * the bab keeps two head pointers.  one is to the last I/O to
	 * complete, and the other is to the last I/O to have started (well,
	 * to have allocated space, which should almost be the same
	 * thing, in the absense of benign races).  here we commit the memory
	 * as long as we find headers that are complete.  sanity code
	 * could be added here (to check to make sure the length isn't too long,
	 * or that the magic numbers are in place, etc).
	 */
	ftd_debugf(iodone, "ftd_end_request_journal: pending check\n");
	if (ftd_bab_get_pending (mgr) == (ftd_uint64_t *) hp) {
		if(hp->majicnum != DATASTAR_MAJIC_NUM) {
			if(mgr->pending_io == 0) {
				FTD_ERR(FTD_WRNLVL, "Moving Pending#1");
				mgr->pending_buf = mgr->in_use_tail;
				mgr->pending     = mgr->pending_buf->alloc;
				dowakeup=1;
				goto manipulate_pending;
			} else
				FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM#1");
		}
		length = FTD_WLH_QUADS (hp);
		ftd_bab_commit_memory (mgr, length);

        // ##@@ Update stats
        lgp->ulCommittedByteCount += ((ftd_uint64_t)hp->length << DEV_BSHIFT);
		/* commit any completed requests past this one! */
		while ((hp = (wlheader_t *) ftd_bab_get_pending (mgr)) != NULL) {
			if (hp->majicnum != DATASTAR_MAJIC_NUM) {
				if(mgr->pending_io == 0) {
					FTD_ERR(FTD_WRNLVL, "Moving Pending#2");
					mgr->pending_buf = mgr->in_use_tail;
					mgr->pending     = mgr->pending_buf->alloc;
					dowakeup=1;
					goto manipulate_pending;
				} else
					FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM#2");
			}

			if (hp->complete == 1) {
				length = FTD_WLH_QUADS (hp);
				ftd_bab_commit_memory (mgr, length);
                // ##@@ Update stats
                lgp->ulCommittedByteCount += ((ftd_uint64_t)hp->length << DEV_BSHIFT);
			} else {
				break;
			}
		}
		dowakeup = 1;
	}

manipulate_pending:
	RELEASE_LOCK (lgp->lock, context);

	ftd_debugf(iodone, "ftd_end_request_journal: manipulate_pending: dodone %d dowakeup %d\n",
        ioerror, dowakeup);

#ifdef DO_CKSUMS
    {

       uint_fast32_t user = cksum_user_data_in_bio(bh);
       uint_fast32_t saved = (uint_fast32_t)bh->bi_private;
       if(user != saved)
       {
          printk(KERN_EMERG "cksum of source data changed (%u != %u) during io for: %llx %u\n",
                 user,
                 saved,
                 userbp->bi_sector * 512, userbp->bi_size);
       }

    }
#endif
    
	ftd_finish_io (softp, bh, ioerror);

	/*-
	 * we do the wakeup last to try to avoid some races with finish I/O
	 * and to give the upper layers a chance to complete.  it was moved
	 * here as part of the big bug hunt which ultimately was caused by
	 * the improper unmapping of I/O requests.
	 */
	if (dowakeup)
		ftd_wakeup (lgp);

	ftd_debugf(iodone, "ftd_end_request_journal: return\n");
#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
        return 0;
#endif /* defined (LINUX260) */
}

/**
    Commit buffer to BAB and update stats.
*/
void IOCommitToBAB(struct buf *bh) 
{
    ftd_int32_t length;
    wlheader_t *hp;
    ftd_lg_t *lgp;
    bab_mgr_t *mgr;
    ftd_dev_t *softp;
    ftd_int32_t dowakeup;
    ftd_int32_t ioerror = 0;
    struct buf *userbp, *mybp = bh;
    minor_t minor;
    ftd_context_t context;

    ftd_debugf(iodone, "IOCommitToBAB\n");

	/* bp -> device state */
	if (!(softp = ftd_get_softp (bh))) 
	{
		ftd_debugf(iodone, "IOCommitToBAB(%p): no softp\n", bh);
		FTD_ERR (FTD_WRNLVL, "IOCommitToBAB:Can't get softp! uptodate bh %p",bh);
		return;
	}

	ftd_debugf(iodone, "IOCommitToBAB(%p): userbp %p\n",bh, BP_USER(bh));

	/* write log header, group, bab mgr references */
	
	hp = (wlheader_t *) BP_WLHDR (mybp);
	
	if (hp->majicnum != DATASTAR_MAJIC_NUM)
       	FTD_ERR(FTD_WRNLVL, "IOCommitToBAB: BAB DATASTAR_MAJIC_NUM");

	lgp = (ftd_lg_t *)(unsigned long) hp->group_ptr;
	mgr = lgp->mgr;

	// Jacques.....  Remove Acquire lock that was causing the Crash.
       // Iocommit is inside FTD_DO_write which already do an Acquire
       // If you want to put IOcommit inside Journal_perf fonction then 
       // you need to uncomment the acquire/lock 
       // ACQUIRE_LOCK (lgp->lock, context);

	/* test state consistency. */
	userbp = (struct buf *)(unsigned long)hp->bp;
	if (userbp == NULL) 
	{
       	panic ("IOCommitToBAB: BAB entry corrupt bh  hp %p\n", hp);
	}

	// This means that the IO that this chunk refers to is completed
	// TODO: We should revise this since it does not necessarily hold TRUE with the performance improvements

	hp->complete = 1;
    // WI_338550 December 2017, implementing RPO / RTT
	mgr->pending_io--;
	atomic_dec(&pending_io);
	/* whether to wake the daemon */
	dowakeup = 0;

	/* update disk stats */
	softp->writeiocnt++;

	softp->sectorswritten += (userbp->bi_size >> DEV_BSHIFT);

	softp->wlentries++;
	softp->wlsectors += hp->length;

    /* update group stats */
	lgp->wlentries++;
	lgp->wlsectors += hp->length;

	hp->bp = (ftd_uint64ptr_t)NULL;
	hp->timoid = INVALID_TIMEOUT_ID;

	if (LG_DO_SYNCMODE (lgp) && lgp->sync_timeout > 0 && !(lgp->mgr->flags & FTD_BAB_DONT_USE)) 
	{
            /* synch mode */
            unsigned long expire;
            struct timer_list *my_timer;

            hp->bp = (ftd_uint64ptr_t)(unsigned long) userbp;
            // in case if time not expires.

            ftd_debugf(iodone, "ftd_end_request_journal: arm ftd_synctimo(%p) timeout %u\n",  hp, lgp->sync_timeout);

            expire = (lgp->sync_timeout * HZ) + jiffies;
            my_timer = &(*(wlheader_t **)&(hp))->timer;
            hp->timoid = (ftd_uint64ptr_t)(unsigned long)my_timer;

            init_timer(my_timer);
            my_timer->expires = expire;
            my_timer->data = (unsigned long) hp;
            my_timer->function = ftd_synctimo;

            add_timer(my_timer);
            ioerror = 1;
	}

    /*-
    * if this is the first pending request, commit it.
    * the bab keeps two head pointers.  one is to the last I/O to
    * complete, and the other is to the last I/O to have started (well,
    * to have allocated space, which should almost be the same
    * thing, in the absense of benign races).  here we commit the memory
    * as long as we find headers that are complete.  sanity code
    * could be added here (to check to make sure the length isn't too long,
    * or that the magic numbers are in place, etc).
    */
    ftd_debugf(iodone, "ftd_end_request_journal: pending check\n");

	if (ftd_bab_get_pending (mgr) == (ftd_uint64_t *) hp) 
	{
		if(hp->majicnum != DATASTAR_MAJIC_NUM) 
		{
			if(mgr->pending_io == 0) 
			{
				FTD_ERR(FTD_WRNLVL, "Moving Pending#1");

				mgr->pending_buf = mgr->in_use_tail;
				mgr->pending     = mgr->pending_buf->alloc;
				dowakeup=1;

				goto manipulate_pending;
			} 
			else
				FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM#1");
		}

		length = FTD_WLH_QUADS (hp);

		ftd_bab_commit_memory (mgr, length);
              
              lgp->ulCommittedByteCount += ((ftd_uint64_t)hp->length << DEV_BSHIFT);


		/* commit any completed requests past this one! */
		while ((hp = (wlheader_t *) ftd_bab_get_pending (mgr)) != NULL) 
		{
			if (hp->majicnum != DATASTAR_MAJIC_NUM) 
			{
				if(mgr->pending_io == 0) 
				{
					FTD_ERR(FTD_WRNLVL, "Moving Pending#2");

					mgr->pending_buf = mgr->in_use_tail;
					mgr->pending     = mgr->pending_buf->alloc;
					dowakeup=1;

					goto manipulate_pending;
				} 
				else
					FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM#2");
			}

			if (hp->complete == 1) 
			{
				length = FTD_WLH_QUADS (hp);
				ftd_bab_commit_memory (mgr, length);
                           lgp->ulCommittedByteCount += ((ftd_uint64_t)hp->length << DEV_BSHIFT);
			} 
			else 
			{
				break;
			}
		}
		dowakeup = 1;
	}

manipulate_pending:
	// Jacques.....  Remove Acquire lock that was causing the Crash.
    // Iocommit is inside FTD_DO_write which already do an Acquire
	//RELEASE_LOCK (lgp->lock, context);

	ftd_debugf(iodone, "ftd_end_request_journal: manipulate_pending: dodone %d dowakeup %d\n", ioerror, dowakeup);

    /*-
    * we do the wakeup last to try to avoid some races with finish I/O
    * and to give the upper layers a chance to complete.  it was moved
    * here as part of the big bug hunt which ultimately was caused by
    * the improper unmapping of I/O requests.
    */
	if (dowakeup)
		ftd_wakeup (lgp);

    ftd_debugf(iodone, "IOCommitToBAB: return\n");
}



#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
static int ftd_end_request_journal_perf (struct bio *bh, unsigned int nums, int err)
#  else
static void ftd_end_request_journal_perf (struct bio *bh, int err)
#  endif

{
    ftd_int32_t length;
    //wlheader_t *hp;
    ftd_lg_t *lgp;
    bab_mgr_t *mgr;
    ftd_dev_t *softp;
    ftd_int32_t ioerror = 0;
    struct buf *userbp, *mybp = bh;
    minor_t minor;
    ftd_context_t context;


    int uptodate = test_bit(BIO_UPTODATE, &bh->bi_flags);

    /* bp -> device state */
	if (!(softp = ftd_get_softp (bh))) 
	{
		ftd_debugf(iodone, "ftd_end_request_journal_perf(%p): no softp\n", bh);
		FTD_ERR (FTD_WRNLVL, "ftd_end_request_journal_perf:Can't get softp! uptodate %d bh %p",uptodate,bh);

#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
		return 1;
#else
		return;
#endif  
    }

#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    if (bh->bi_size) 
    {
        FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: bi_size set");
        ioerror = 1;
        ftd_finish_io (softp, bh, ioerror);
        return 0;
    }
#endif 

    ftd_debugf(iodone, "ftd_end_request_journal_perf(%p, %d): userbp %p\n", bh, uptodate, BP_USER(bh));

    /* test state consistency. */
    if (!uptodate && err) 
    {
        if (err == -EIO)
            FTD_ERR(FTD_WRNLVL, "iodone_journal:Error Incomplete I/O:");
 
    }

    //This is testing only, to validate that IOCommit/ftd_end_request_journal_perf works like the old call "ftd_end_request_journal"  
    //  IOCommitToBAB(mybp);


    /* complete the IO, according to error states and current driver mode */
    if (!uptodate && err) 
    {
        if (err == -EIO)
            FTD_ERR (FTD_WRNLVL, "ftd_end_request_journal_perf:Device I/O Error! uptodate %d", uptodate);
        ioerror = err;
    }

    ftd_debugf(iodone, "ftd_end_request_journal_perf: manipulate_pending: dodone %d\n", ioerror);

    ftd_finish_io (softp, bh, ioerror);

    ftd_debugf(iodone, "ftd_end_request_journal_perf: return\n");

#if defined (LINUX260) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    return 0;
#endif 

}

/*-
 * ftd_do_read()
 *
 * process a read to the device.  setup things so that we can call our
 * bdev_strategy with the buf and have it do the right thing.
 */
FTD_PRIVATE ftd_void_t ftd_do_read (ftd_dev_t *softp, struct buf *mybh)
{
	struct buf *passdownbh = mybh;
    struct block_device *bdev;

	ftd_debugf(read, "ftd_do_read(%p, %p)\n", softp, mybh);
	/* reads are trivial... */
	passdownbh->b_end_io = ftd_end_request_generic;
    bdev = bdget(softp->localbdisk);

    ftd_blkdev_get(bdev, O_RDWR|O_SYNC);

    passdownbh->bi_bdev = bdev;

    ftd_blkdev_put(bdev);


	ftd_debugf(read, "ftd_do_read: return\n");
}


#ifdef DO_CKSUMS
static uint_fast32_t const crctab[256] =
  {
    0x00000000,
    0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
    0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
    0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
    0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
    0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
    0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
    0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
    0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
    0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
    0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
    0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
    0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
    0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
    0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
    0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
    0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
    0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
    0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
    0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
    0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
    0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
    0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
    0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
    0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
    0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
    0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
    0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
    0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
    0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
    0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
    0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
    0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
    0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
    0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
  };

struct cksum_context
{
   uint_fast32_t crc;
   int total_length;
};
 
void cksum_init(struct cksum_context* context)
{
   context->crc = 0;
   context->total_length = 0;
}

void cksum_add(struct cksum_context* context, unsigned char* data, int length)
{   
   int bytesLeft = length;
   context->total_length += length;
   while (bytesLeft--)
   {
      context->crc = (context->crc << 8) ^ crctab[((context->crc >> 24) ^ *data++) & 0xFF];
   }
}

uint_fast32_t cksum_get(struct cksum_context* context)
{
   for (; context->total_length; context->total_length >>= 8)
      context->crc = (context->crc << 8) ^ crctab[((context->crc >> 24) ^ context->total_length) & 0xFF];
   
   context->crc = ~context->crc & 0xFFFFFFFF;
   
   return context->crc;
}

uint_fast32_t cksum(unsigned char* data, int length)
{
   uint_fast32_t crc = 0;
   int bytesLeft = length;
   
   while (bytesLeft--)
   {
      crc = (crc << 8) ^ crctab[((crc >> 24) ^ *data++) & 0xFF];
   }
   
   for (; length; length >>= 8)
      crc = (crc << 8) ^ crctab[((crc >> 24) ^ length) & 0xFF];
   
   crc = ~crc & 0xFFFFFFFF;
   
   return crc;
}

uint_fast32_t cksum_user_data_in_bio(struct bio *bh)
{
   struct cksum_context in_user;
   uint_fast32_t user = 0;
   struct bio_vec *bvec;
   int ind;
   cksum_init(&in_user);
   
   bio_for_each_segment(bvec, bh, ind) {
      char *data = NULL;
      struct page *bvpage = bio_iovec_idx(bh, ind)->bv_page;
      ftd_uint32_t bvlen, bvoffset = bio_iovec_idx(bh, ind)->bv_offset;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
      data = kmap_atomic(bvpage) + bvoffset;	// PROD00013379: porting for RHEL 7.0
#else
      data = kmap_atomic(bvpage, KM_USER0) + bvoffset;
#endif
      //data = kmap(bvpage) + bvoffset;
      bvlen = bio_iovec_idx(bh, ind)->bv_len;
      
      cksum_add(&in_user, data, bvlen);
      
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
      kunmap_atomic(data);	// PROD00013379: porting for RHEL 7.0
#else
      kunmap_atomic(data, KM_USER0);
#endif
   }
   return cksum_get(&in_user);
}

int cksum_matches(unsigned char* srcdata, unsigned char* tardata, int length)
{
   uint_fast32_t src_cksum = cksum(srcdata, length);
   uint_fast32_t tar_cksum = cksum(tardata, length);

   if(src_cksum != tar_cksum)
   {
      printk(KERN_EMERG "cksum mismatch: %u != %u\n", src_cksum, tar_cksum);   
   }
   
   return src_cksum == tar_cksum;
}

// This function is just there to make it easy to obtain the data through systemtap.
void log_cksum(const char* context, struct buf *userbp, long cksum)
{
   printk(KERN_DEBUG "log_cksum (%s): %llx %u %lu\n",
          context,
          userbp->bi_sector * 512,
          userbp->bi_size,
          cksum);
}
#endif  /* ... DO_CKSUMS */


/*-
 * ftd_do_write()
 *
 * copy the contents of the buffer to our memory log and/or manage the
 * high resolution and low resolution dirty bits.
 */
FTD_PRIVATE struct bio *ftd_do_write(ftd_dev_t *softp, struct buf *userbp, struct buf *mybp)
{
	wlheader_t *hp;
    ftd_context_t context;
	ftd_uint64_t *buf, *temp;
	ftd_int32_t size64, i=0;
	ftd_lg_t *lgp;
	struct buf *passdownbp = mybp;
	ftd_uint32_t lgnum;	  /* for Dynamic Mode Change */
	char *vfrom;
#ifdef DO_CKSUMS
    struct cksum_context in_user;
    struct cksum_context in_bab;
#endif

	struct bio_vec *bvec;
	ftd_uint32_t bvlen, bvoffset, baboff = 0, offset;
	struct page *bvpage;
	int ind;

    // WI_338550 December 2017, implementing RPO / RTT
    struct timeval tv;
    ftd_time_t current_time_seconds;

	// For return, in case we create a new local bio in addition to the clone (ex.: data tx from BAB)
	struct bio *duplicated_data_bio = NULL;

	ftd_debugf(write, "ftd_do_write(%p, %p, %p\n", softp, userbp, mybp);
	lgp = softp->lgp;
	passdownbp->b_end_io = ftd_end_request_generic;

    if (ISDTCDEV(BP_DEV (userbp) ) )
    {
        /** @todo Understand why the following is needed and why it caused hangs when tried on a captured device. */
        /** We need to obtain the bdev to replace the bi_bdev in the passdownbp, but why the need to open it? */
        struct block_device *bdev = bdget(softp->localbdisk);
        ftd_blkdev_get(bdev, O_RDWR|O_SYNC);
        passdownbp->bi_bdev = bdev;
        ftd_blkdev_put(bdev);
        BUG_ON(bdev != softp->bd);
    }

    atomic_inc(&pending_writes);

	/* bab_mgr always sees the world as 64-bit words! */
	/* XXX This code should use the FTD_WLH_QUADS() macro XXX */
	size64 = sizeof_64bit (wlheader_t) + (userbp->b_bcount / 8);

#ifdef notyet
	if (userbp->b_bcount & (DEV_BSIZE - 1))
		FTD_ERR(FTD_WRNLVL, "Not Multiple of DEV_BSIZE");
#endif
	lgnum = (ftd_uint32_t)(getminor(lgp->dev));
	if (0 <= lgnum && lgnum < MAXLG)
	{
		if (sync_time_out_flg[lgnum] == 1 && lgp->state != FTD_MODE_TRACKING)
		{
            ftd_int32_t old_state = lgp->state;
            ACQUIRE_LOCK(lgp->lock, context);
            lgp->state = FTD_MODE_TRACKING;
            RELEASE_LOCK(lgp->lock, context);
            FTD_ERR(FTD_WRNLVL,"ftd_do_write(%p, %p, %p): lg%d[%p] sync timeout: Transition [%x]->Tracking[%x]\n",softp, userbp, mybp, lgnum, lgp, old_state, lgp->state);
		}
	}

	/*-
	 * this is where journal memory is allocated,
	 * and buffer data is copied to it. if the
	 * allocation fails, a driver state transistion
	 * is triggered, FTD_M_JNLUPDATE -> FTD_M_TRACKING,
     * or FTD_M_REFRESH -> FTD_M_TRACKING.
	 *
	 * XXX
	 * this is a rotten bit of state machinery, but as it's
	 * the beloved heart and soul of the product, its likely
	 * to remain as is.
	 */
    ACQUIRE_LOCK(lgp->lock, context);
    // ##@@ Update stats
    lgp->ulWriteByteCount += userbp->b_bcount;
    
    // WI_338550 December 2017, implementing RPO / RTT
    do_gettimeofday(&tv);
    current_time_seconds = tv.tv_sec;
    lgp->PMDStats.LastIOTimestamp = tv.tv_sec;
    if( lgp->PMDStats.OldestInconsistentIOTimestamp == 0 )
    {
        lgp->PMDStats.OldestInconsistentIOTimestamp = tv.tv_sec;
    }

    if (lgp->state & FTD_M_JNLUPDATE && (ftd_bab_alloc_memory (lgp->mgr, size64) != 0))
	{
#ifdef  BIO_DATA_FROM_BAB
        // The number of io vectors in our allocated data bio is currently always the same as in the original one.
        duplicated_data_bio = bio_alloc(GFP_ATOMIC, userbp->bi_vcnt);
		if( duplicated_data_bio == NULL )
        {
           FTD_ERR (FTD_WRNLVL, "Cannot allocate bio structure (%d iovecs) for data transfers from BAB !\n", userbp->bi_vcnt);
           FTD_ERR (FTD_WRNLVL, "Will proceed with transfer from user space for this IO request.\n" );
        }
		else
		{
        duplicated_data_bio->bi_sector = passdownbp->bi_sector;
        duplicated_data_bio->bi_bdev = passdownbp->bi_bdev;
        duplicated_data_bio->bi_rw = passdownbp->bi_rw;
        duplicated_data_bio->bi_private = passdownbp;
		}
#endif
        
        temp = lgp->mgr->from[0] - sizeof_64bit(wlheader_t);
        ftd_debugf(write, "ftd_do_write: ftd_bab_alloc_memory from %p data %p len %d\n",
            lgp->mgr->from[0], temp, size64 * 8);
        hp = (wlheader_t *) temp;
        hp->complete = 0;
        hp->bp = (ftd_uint64ptr_t)(unsigned long)userbp;

        /* add a header and some data to the memory buffer */
        hp->majicnum = DATASTAR_MAJIC_NUM;
        hp->timoid = INVALID_TIMEOUT_ID;  /* bug 40: (see WR15928) */
        hp->offset = userbp->b_blkno;
        hp->length = userbp->b_bcount >> DEV_BSHIFT;
        hp->span = lgp->mgr->num_frags;
        hp->dev = softp->cdev;

        hp->diskdev = new_encode_dev(softp->localbdisk);

        hp->group_ptr = (ftd_uint64ptr_t)(unsigned long)lgp;
        hp->flags = 0;
        
        // WI_338550 December 2017, implementing RPO / RTT
        hp->timestamp = current_time_seconds;
        
        // Update stats
        lgp->ulPendingByteCount += userbp->b_bcount;
        /* now copy the data */
        offset = 0;
        
#ifdef DO_CKSUMS
        cksum_init(&in_user);
        cksum_init(&in_bab);
#endif

#ifdef BIO_DATA_FROM_BAB
        // There are various reasons why data copied in the bab may not be adequately laid out to be used as source data for our
        // duplicated data bio.  In such circumstances, the data is copied in temporary allocated memory, where it is
        // crystallized and from where it is added to the duplicated_data_bio.

        // Since we currently do not know before hand which segment of the bio request will be usable from the bab or not,
        // we always set up the array used to remember which segments were played from allocated memory, even if we may not
        // always need it.  A possible optimization might be to test ahead if we need to dynamically allocate memory and the array,
        // or not.
        if(duplicated_data_bio)
        {
           // If succesfully allocated, the BP_PRIVATE pointer points to an array of all the dynamically allocated copies that will later need to be freed
           const size_t array_size = userbp->bi_vcnt * sizeof(void*);
           BP_PRIVATE(mybp) = kmalloc(array_size, GFP_ATOMIC);
           if(BP_PRIVATE(mybp))
           {
              memset(BP_PRIVATE(mybp), 0x0, userbp->bi_vcnt * sizeof(void*));
           }
           else
           {
              FTD_ERR (FTD_WRNLVL, "Cannot allocate memory (%ld bytes) to track IO data outside of multiple bab fragments!\n", array_size);
              FTD_ERR (FTD_WRNLVL, "Will proceed with transfer from user space for this IO request.\n" );
              bio_put(duplicated_data_bio);
              duplicated_data_bio = NULL;
           }
        }
#endif
        i=0; // Reinitialized here just to avoid having to look for any possible usage of i above.
        bio_for_each_segment(bvec, userbp, ind) {
             char *data = NULL;
             bvpage = bio_iovec_idx(userbp, ind)->bv_page;
             bvoffset = bio_iovec_idx(userbp, ind)->bv_offset;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
             data = kmap_atomic(bvpage) + bvoffset;		// PROD00013379: porting for RHEL 7.0
#else
             data = kmap_atomic(bvpage, KM_USER0) + bvoffset;
#endif
             bvlen = bio_iovec_idx(userbp, ind)->bv_len;

#ifdef BIO_DATA_FROM_BAB
             void* target_bab_address_in_current_chunk = (caddr_t)(lgp->mgr->from[i])+baboff;
             unsigned int bab_chunk_space_left = lgp->mgr->count[i]-baboff;
             
             bool bio_segment_fits_in_a_single_bab_chunk = bvlen <= bab_chunk_space_left;
             bool bio_segment_fits_in_a_single_bab_page =
                 PAGE_ALIGN((intptr_t)target_bab_address_in_current_chunk) > ((intptr_t)target_bab_address_in_current_chunk + bvlen);

             // The actual rule to consider data lying in the bab to be safe to add to a bio is slightly too restrictive.
             // Bio segments split in two chunks or bio segments crossing page boundaries in the bab may actually be safe
             // and usable by calling bio_all_page on all the chunks/pages as long as each subpart meets the various
             // hardware sector size and other requirements.
             // Since such a case is believed to be extremely rare, we just rely on the oversimplified and easier logic.
             bool bio_data_from_bab_is_safe = bio_segment_fits_in_a_single_bab_chunk && bio_segment_fits_in_a_single_bab_page;
             
             if(duplicated_data_bio && bio_data_from_bab_is_safe == false)
             {
#ifdef PRIVATE_MEMORY_POOL
                void* allocated_bio_segment_data_copy = (void *)obtain_mem_pool_page(bvlen);
#else
                void* allocated_bio_segment_data_copy = kmalloc(bvlen, GFP_ATOMIC);
#endif // PRIVATE_MEMORY_POOL

                if(allocated_bio_segment_data_copy)
                {
                   void** array_of_allocated_data_copies = BP_PRIVATE(mybp);
                   array_of_allocated_data_copies[ind] = allocated_bio_segment_data_copy;

                   memcpy(allocated_bio_segment_data_copy, data, bvlen);
                   bio_add_page(duplicated_data_bio, virt_to_page(allocated_bio_segment_data_copy), bvlen, offset_in_page(allocated_bio_segment_data_copy));
                }
                else
                {
                   if( printk_ratelimit() )
                   {
                       FTD_ERR (FTD_WRNLVL, "Cannot allocate memory (%ld bytes) to fix IO data outside of multiple bab fragments!\n", bvlen);
                       FTD_ERR (FTD_WRNLVL, "Will proceed with data from user space for this IO request.\n" );
                   }
                   bio_add_page(duplicated_data_bio, virt_to_page(data), bvlen, offset_in_page(data));
                }
             }
#endif
             
#ifdef DO_CKSUMS
             cksum_add(&in_user, data, bvlen);
#endif
             
             // baboff represents the amount of data already copied within the current bab fragment so far.
             // offset represents the amount of data already copied from the current bio segment.
             while (i < lgp->mgr->num_frags) {
                 // If the amount of data left to copy from the current bio segment
                 // is smaller than the remaining space of the current bab fragment...
                 // (This happens when there are more than 1 bio segment and that we've used more than 1 bab chunk.)
                 if ((bvlen-offset) < ((ftd_uint32_t)(lgp->mgr->count[i])-baboff)) {
                         // We copy what's left of the bio segment.
                         void* bab_address = (caddr_t)(lgp->mgr->from[i])+baboff;
                         unsigned int bab_segment_length = (bvlen-offset);
                         bcopy((data+offset), bab_address, bab_segment_length);
#ifdef BIO_DATA_FROM_BAB
                         // @todo Validate this:
                         // Since we can only get here in the case where multiple bio segments are mapped in multiple bab fragments,
                         // it will never be safe to have the bio data taken from the bab, and the following code can be removed.
                         if( duplicated_data_bio && bio_data_from_bab_is_safe)
                         {
                             int result = bio_add_page(duplicated_data_bio,
                                                       virt_to_page(bab_address),
                                                       bab_segment_length,
                                                       offset_in_page(bab_address));
                             if( result != bab_segment_length )
                             {
                                 FTD_ERR (FTD_WRNLVL, "Cannot add page to BAB IO vector !\n" );
                                 FTD_ERR (FTD_WRNLVL, "Will proceed with transfer from user space for this IO request.\n" );
                                 bio_put(duplicated_data_bio);
                                 duplicated_data_bio = NULL;
                             }
                         }
                         
                         // If we have already fixed the data in memory allocated for the duplicated data bio,
                         // then we must make sure that the bab holds the very same data as well.
                         if ( duplicated_data_bio && !bio_data_from_bab_is_safe && BP_PRIVATE(mybp) )
                         {
                             void** array_of_allocated_data_copies = BP_PRIVATE(mybp);
                             void* allocated_bio_segment_data_copy = array_of_allocated_data_copies[ind];

                             if(allocated_bio_segment_data_copy)
                             {
                                 bcopy((allocated_bio_segment_data_copy+offset), bab_address, bab_segment_length);
                             }
                         }
#endif

#ifdef DO_CKSUMS
                         if (!cksum_matches((data+offset),
                                            (caddr_t)(lgp->mgr->from[i])+baboff,
                                            (bvlen-offset)))
                         {
                            // <<< can bab-bio and cksum cohabitate ? <<<
                            printk(KERN_EMERG "Copy cksum mismatches in 1st case. data: %p, offset: %d, ind: %d, i: %d, (lgp->mgr->from[i])+baboff: %p, bvlen: %d, lgp->mgr->num_frags: %d\n",
                                   data,
                                   offset,
                                   ind,
                                   i,
                                   (caddr_t)(lgp->mgr->from[i])+baboff,
                                   bvlen,
                                   lgp->mgr->num_frags
                               );
                            printk(KERN_EMERG "During io for: %llx %u\n", userbp->bi_sector * 512, userbp->bi_size);
                            
                            dump_data("Source", (data+offset), (bvlen-offset));
                            dump_data("Target", (caddr_t)(lgp->mgr->from[i])+baboff, (bvlen-offset));
                            
                         }
*/
                         cksum_add(&in_bab, (caddr_t)(lgp->mgr->from[i])+baboff, (bvlen-offset));
#endif
                         baboff = baboff + (bvlen-offset);
                         // We're done with this bio segment.
                         offset = 0;
                         break;
                 } else {
                         // Otherwise, we fit what we can from the bio within what's left of the bab fragment.
                         void* bab_address = (caddr_t)(lgp->mgr->from[i])+baboff;
                         unsigned int bab_segment_length = ((ftd_uint32_t)(lgp->mgr->count[i])-baboff);
                         bcopy((data+offset), bab_address, bab_segment_length);

#ifdef BIO_DATA_FROM_BAB
                         if( duplicated_data_bio && bio_data_from_bab_is_safe)
                         {
                             int result = bio_add_page(duplicated_data_bio,
                                                       virt_to_page(bab_address),
                                                       bab_segment_length,
                                                       offset_in_page(bab_address));
                             if( result != bab_segment_length )
                             {
                                 FTD_ERR (FTD_WRNLVL, "Cannot add page to BAB IO vector !\n" );
                                 FTD_ERR (FTD_WRNLVL, "Will proceed with transfer from user space for this IO request.\n" );
                                 bio_put(duplicated_data_bio);
                                 duplicated_data_bio = NULL;
                             }
                         }
                         
                         // If we have already fixed the data in memory allocated for the duplicated data bio,
                         // then we must make sure that the bab holds the very same data as well.
                         if ( duplicated_data_bio && !bio_data_from_bab_is_safe && BP_PRIVATE(mybp) )
                         {
                             void** array_of_allocated_data_copies = BP_PRIVATE(mybp);
                             void* allocated_bio_segment_data_copy = array_of_allocated_data_copies[ind];
                             
                             if(allocated_bio_segment_data_copy)
                             {
                                 bcopy((allocated_bio_segment_data_copy+offset), bab_address, bab_segment_length);
                             }
                         }
#endif
#ifdef DO_CKSUMS
/*
                         if (!cksum_matches((data+offset),
                                            (caddr_t)(lgp->mgr->from[i])+baboff,
                                            ((ftd_uint32_t)(lgp->mgr->count[i])-baboff)))
                         {
                            printk(KERN_EMERG "Copy cksum mismatches in 2nd case. data: %p, offset: %d, ind: %d, i: %d, (lgp->mgr->from[i])+baboff: %p, bvlen: %d, lgp->mgr->num_frags: %d, ((ftd_uint32_t)(lgp->mgr->count[i])-baboff): %d\n",
                                   data,
                                   offset,
                                   ind,
                                   i,
                                   (caddr_t)(lgp->mgr->from[i])+baboff,
                                   bvlen,
                                   lgp->mgr->num_frags,
                                   ((ftd_uint32_t)(lgp->mgr->count[i])-baboff)
                               );
                            printk(KERN_EMERG "During io for: %llx %u\n", userbp->bi_sector * 512, userbp->bi_size);
                            dump_data("Source", (data+offset), ((ftd_uint32_t)(lgp->mgr->count[i])-baboff));
                            dump_data("Target", (caddr_t)(lgp->mgr->from[i])+baboff, ((ftd_uint32_t)(lgp->mgr->count[i])-baboff));
                         }
*/
                         cksum_add(&in_bab, (caddr_t)(lgp->mgr->from[i])+baboff, ((ftd_uint32_t)(lgp->mgr->count[i])-baboff));
#endif

                          // If the amount of data left to copy from the current bio segment happened to be
                         // the amount of space that was available in the bab fragment...
                         if ((bvlen-offset) == ((ftd_uint32_t)(lgp->mgr->count[i])-baboff)) {
                            // We're done with both this bab fragment and this bio segment.
                            offset = 0;
                            baboff = 0;
                            i++;
                            break;
                         }
                         offset = offset + ((ftd_uint32_t)(lgp->mgr->count[i]) - baboff);
                         // We're done with this bab fragment.
                         baboff = 0;
                         i++;
                 }
           }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
           kunmap_atomic(data);	// PROD00013379: porting for RHEL 7.0
#else
           kunmap_atomic(data, KM_USER0);
#endif
       }
#ifdef DO_CKSUMS
        {
           uint_fast32_t user = cksum_get(&in_user);
           uint_fast32_t bab = cksum_get(&in_bab);
/* <<< HERE: USAGE OF BI_PRIVATE by checksum logic <<< */
           // We keep a copy of the cksum so that later calculations can be compared to what we have obtained here.
           // WARNING: ftd_write_lrdb() also makes use of this field.  If we want to track down changing data with this temp mechanism,
           // we must "dtcset LRT=off" in order to avoid obtaining false positives.
           // WARNING: The other end of this is currently only found in ftd_end_request_journal as the work was done in the RFXTUIP26x_p branch.
           // This code currently uses the ftd_end_request_journal_perf instead of ftd_end_request_journal so there is still some work to be done
           // in order to have these changes work in the current branch.
           passdownbp->bi_private = (void*)user;
           
           log_cksum("in_user", userbp, user);
           log_cksum("in_bab", userbp, bab);
        }
#endif


        BP_WLHDR (mybp) = hp;
        /*-
         * we use the iodone function that we set before to figure out
         * what to do now.
         */
        lgp->mgr->pending_io++;
        atomic_inc(&pending_io);
#ifdef BIO_DATA_FROM_BAB
        if( duplicated_data_bio )
        duplicated_data_bio->b_end_io = ftd_end_request_journal;
		else
           passdownbp->b_end_io  = ftd_end_request_journal;
#else
        passdownbp->b_end_io  = ftd_end_request_journal;
#endif
/* The next lines are de-activated (trial that was done at new IOCommitToBAB in the case of data corruption investigations)
        passdownbp->b_end_io  = ftd_end_request_journal_perf;
        IOCommitToBAB(mybp);
*/
	} else if (lgp->state == FTD_MODE_NORMAL) {

		ftd_debugf(write, "ftd_do_write: ftd_clear_hrdb\n");
		/* we just overflowed the journal */
		FTD_ERR (FTD_DBGLVL, "Transition: NORMAL->TRACKING mode.");
		//ftd_clear_hrdb (lgp);		 //RFW don't do this
		//ftd_compute_dirtybits (lgp, FTD_HIGH_RES_DIRTYBITS);
		lgp->state = FTD_MODE_TRACKING;
        lgp->KeepHistoryBits = FALSE;    // to prevent forever duplication
	} else if (lgp->state == FTD_MODE_REFRESH) {
		ftd_debugf(write, "ftd_do_write: FTD_MODE_REFRESH\n");

		/*-
		 * we just overflowed the journal again. since we've been
		 * keeping track of the dirty bits all along, there is
		 * little we need to do here except flush the bab. we
		 * can't recompute the dirty bits here because entries
		 * may have been migrated out of the bab in the interrum
		 * and we'd have no way of knowing.
		 */
		/*-
		 * XXX
		 * WARNING don't we want to clear the bab here because we
		 * know it is dirty?  yes, in theory we do, but we don't
		 * want to step on the PMD's world view, so we let it do it
		 * when it thinks we've gone down this code path
		 */
		FTD_ERR (FTD_DBGLVL, "Transition: REFRESH->TRACKING mode.");
		lgp->state = FTD_MODE_TRACKING;
        lgp->KeepHistoryBits = FALSE;    // to prevent forever duplication
    } else if ((lgp->state == FTD_MODE_TRACKING) ||
               (lgp->state == FTD_MODE_CHECKPOINT_JLESS)) {
		ftd_debugf(write, "ftd_do_write: FTD_MODE_TRACKING or FTD_MODE_CHECKPOINT_JLESS\n");
	} else {
		ftd_debugf(write, "ftd_do_write: ????\n");
                /*
                 * possible lgp->state to reach here
                 * FTD_MODE_PASSTHRU
                 * FTD_MODE_FULLREFRESH
                 * FTD_MODE_BACKFRESH
                 * FTD_MODE_SYNCTIMEO
                 * UNKNOWN states
                 */
		/*-
		 * XXX
		 * this `state' transistion is particularly rotten,
		 * as it suggests we're in an unknown state and that's
		 * ok. seems this would be a great place to panic().
		 */
	}
    RELEASE_LOCK(lgp->lock, context);

	/*
	 * As in Windows, the hrdb is now always updated.
	 */
    ftd_update_hrdb (softp, userbp);

	ftd_debugf(write, "ftd_do_write: return\n");
	return( duplicated_data_bio );
}


/***********************************************************************
 * ftd_bioclone
 *
 */

#if defined (LINUX260) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#define SLAB_NOIO GFP_NOIO
#endif

FTD_PUBLIC struct buf *
ftd_bioclone(ftd_dev_t *softp, struct buf *sbp, struct buf *dbp)
{
	ftd_debugf(clone, "ftd_bioclone(%p, %p, %p)\n", softp, sbp, dbp);
	while (!dbp) {
        dbp = (struct buf *)kmem_cache_alloc(ftd_metabuf_cache, SLAB_NOIO);
		if (!dbp) {
			__set_current_state(TASK_RUNNING);
			yield();
		}
	}
	memcpy(dbp, sbp, sizeof(*sbp));
	dbp->b_end_io = ftd_end_request_generic;
	dbp->av_forw =  NULL; 
	dbp->bi_flags |= 1 << BIO_CLONED;
#ifdef BIO_DATA_FROM_BAB
    dbp->bi_private = NULL;
    BP_PRIVATE(dbp) = 0x0;
#endif
	BP_USER(dbp) = sbp;
    BP_SOFTP(dbp) = softp;


	ftd_debugf(write, "ftd_bioclone: return %p\n", dbp);
	return(dbp);
}

static int ftd_do_io(ftd_dev_t *softp, struct buf *bh)
{
	ftd_context_t context;
	ftd_debugf(request, "ftd_do_io(%p, %p)\n", softp, bh)
    atomic_inc(&pending_ios);

	ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device(bh);
	return (0);
}

/*
 * Transfer a buffer directly, without going through the request queue.
 */
int ftd_make_request(struct request_queue *queue, struct bio *bh)
{
	ftd_dev_t *softp;
	ftd_uint32_t numblocks;
	ftd_context_t context;

    int minor = FTDGETMINOR(bh->b_rdev), changed = 0;
    struct bio *mybh;
    unsigned long rw;
        
	rw = bio_data_dir(bh);
    ftd_debugf(request, "ftd_make_request(%p, %lu, %p)\n", queue, rw, bh);

	/* test device number validity */
	softp = ftd_dynamic_activation_get_replication_device_structure(BP_DEV(bh));
	if (softp == NULL) {
		ftd_debugf(request, "ftd_make_request: no softp\n");

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
		bio_io_error(bh, bh->bi_size);
#  else
		bio_io_error(bh);
#  endif 

		return 0;
	}

	/* validate device block number */
	if (bh->b_blkno >= softp->disksize) 
    {
		ftd_debugf(request, "ftd_make_request: bad block number\n");

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
                bio_io_error(bh, bh->bi_size);
#  else
                bio_io_error(bh);
#  endif

		return 0;
	}

	/* truncate this request if it spans the end of the volume */
	numblocks = bh->b_bcount >> DEV_BSHIFT;
	if ((bh->b_blkno + numblocks) > softp->disksize) 
    {
		numblocks = softp->disksize - bh->b_blkno;
		bh->b_bcount = (numblocks << DEV_BSHIFT);
	}

	/* if transfer length is 0, this call is a noop */
	if (bh->b_bcount == 0) 
    {

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
                bio_endio(bh, bh->b_bcount, 0);
#  else
                bio_endio(bh, 0);
#  endif

		return (0);
	}

#if	0
	/*
	 * the bits setup is not used here anymore, which caused lots
	 * of grieves where a family of bit functions like test_bit,
	 * set_bit simply does not do a right work here.
	 */
	switch (rw) {
	case READ:
		bh->b_state |= (BH_TDMF_READ << 1);
		break;
	case READA:
		bh->b_state |= (BH_TDMF_READA << 1);
		break;
	case WRITE:
		bh->b_state |= (BH_TDMF_WRITE << 1);
		break;
	default:
		BUG();
		break;
	}
#endif

    pending_ios_monitoring_register_pending_io(softp);
    
	mybh = ftd_bioclone(softp, bh, NULL);


    BP_RW(mybh) = bio_data_dir(bh);
    switch (bio_data_dir(bh)) 
    {

	case READA:
	case READ:
		ftd_do_read(softp, mybh);
		ftd_do_io(softp, mybh);
        return(0);
		break;
	case WRITE:
#ifdef BIO_DATA_FROM_BAB // This scenario preserves write ordering between pstore update (first) and data IO (second)
	{
		struct bio  *new_private_bio;
		new_private_bio = ftd_do_write(softp, bh, mybh);

        if( new_private_bio )
		  mybh = new_private_bio;	 // We will do the user IO from a new private bio (such as case of direct BAB transfers)

        if (softp->lgp->lrt_mode) 
        {
           ACQUIRE_LOCK(softp->lock, context);
           if (ftd_update_lrdb(softp, bh)) 	// Check if flush_lrdb is needed
           {
           	 RELEASE_LOCK(softp->lock, context);
			 // If we take the data directly from the BAB we process the IO here with ftd_do_io, not from the pstore_end_io callback
		     if( ftd_flush_lrdb (softp, mybh, context) != 0 )  
		     {
                // If error occured in ftd_flush_lrdb, initiate the User IO from here; 
                // otherwise the user IO is queued from processing in pstore_end_io tp [reserve write order (pstore first)
                ftd_do_io(softp, mybh);
             }
			 return( 0 );
       	   }
		   else
           	 RELEASE_LOCK(softp->lock, context);
       	}
       	// Coming here means there was no need to update the lrdb, so out IO has neither been done nor queued 
        ftd_do_io(softp, mybh);
		break;
        }
        
#else	   // ...of #ifdef BIO_DATA_FROM_BAB; previous logic here

		ftd_do_write(softp, bh, mybh);
        if (softp->lgp->lrt_mode) {
           ACQUIRE_LOCK(softp->lock, context);
           if (ftd_update_lrdb(softp, bh)) {
           	RELEASE_LOCK(softp->lock, context);
		if (!ftd_flush_lrdb (softp, mybh, context) == 0)  {
                         ftd_do_io(softp, mybh);
                }
       	   } else {
                    RELEASE_LOCK(softp->lock, context);
                    ftd_do_io(softp, mybh);
           }
	} else {
		ftd_do_io(softp, mybh);
	}
		break;
#endif 
	}

	/*
	 * the I/O queuing mechanism wherein the bios are queued behind
	 * softp->qhead to initiate I/Os after LRDB I/Os are completed, 
	 * has been removed since it led to longer umount times for 
	 * dtc devices. Now we simply initiate the I/O after the LRDB
	 * is flushed.
	 * Checked-in on 12-March-2007 IST
	 */

	return(0);
}

static void
ftd_request_fn(struct request_queue *q)
{
    printk("ftd_request_fn(%p)\n", q);
    return;
}

/*
 *  ftd_queue_io
 *
 *  queue up a struct bio for a dtc device
 *
 */
void
ftd_queue_io(ftd_dev_t *softp, struct buf *bio)
{
         ftd_context_t context;
         struct buf *tempbio;

         bio->av_forw = NULL;
         ACQUIRE_LOCK(softp->lock, context);
         if (!softp->qhead) {
                softp->qhead = bio;
         } else {
                tempbio = softp->qhead;
                while (tempbio->av_forw != NULL)
                        tempbio = tempbio->av_forw;
                tempbio->av_forw = bio;
         }
         RELEASE_LOCK(softp->lock, context);
}

/*
 * ftd_dequeue_io
 *
 * dequeue a struct bio for a dtc device
 *
 */
struct buf *
ftd_dequeue_io(ftd_dev_t *softp)
{
        ftd_context_t context;
        struct buf *bio = NULL;

        ACQUIRE_LOCK(softp->lock, context);
        if (softp->qhead) {
            bio = softp->qhead;
            softp->qhead = bio->av_forw;
            bio->av_forw = NULL;
        }
        RELEASE_LOCK(softp->lock, context);

        return (bio);
}

/*
 * pstore_end_io
 *
 * dequeue a struct bio for a dtc device
 *
 */
    #  if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
static int pstore_end_io(struct buf * bio, unsigned int num, int err)
    #  else
static void pstore_end_io(struct buf * bio, int err)
    #  endif
{
    struct buf *cloneduserbio = bio->bi_private;
    ftd_dev_t *softp;

    if (cloneduserbio)
    {
      #ifdef BIO_DATA_FROM_BAB
        if(cloneduserbio->bi_private)
        {
            // We sent the bio owning the data.
            softp = BP_SOFTP(cloneduserbio->bi_private);
        }
        else
        {
            // We sent the cloned bio.
            softp = BP_SOFTP(cloneduserbio);
        }
      #else	 // Data from user space
        softp = cloneduserbio->bi_private;
      #endif

        // This is the user data IO; points to the BAB bio if data to be taken from the BAB
        ftd_queue_io(softp, cloneduserbio);
        ftd_thrd_wakeup(softp->lgp);
    }

    bio_put(bio);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    return 0;
#else 
    return;
#endif
}



static int bio_write_page(int off, void * page, dev_t pstore, struct buf *cloneduserbio)
{
    int error = 0;
    struct bio * bio ;
    struct block_device *pstbdev;

    bio = bio_alloc(GFP_ATOMIC, 1);

    if (!bio) 
    {
        return -ENOMEM;
    }
    bio->bi_rw = WRITE_SYNC;
    bio->bi_sector = off;

// A leak has been found on RHEL7: bio_alloc (above) already sets the bio usage (reference) count to 1;
// if we do bio_get the count goes to 2 and the buffer will not be released at bio_put, not decrementing to 0.
// This leak could also be on RHEL 5 and 6 but at this stage of the current release we make the fix only for RHEL 7.
// Apply the fix to all versions of RHEL now (defect 241404), TDMFIP 2.9.0
// #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    if( (int)(bio->bi_cnt.counter) == 0 )
	{
        bio_get(bio);
	}
// #else
//     bio_get(bio);
// #endif
    pstbdev = bdget(pstore);

    ftd_blkdev_get(pstbdev, O_RDWR|O_SYNC);

    bio->bi_bdev = pstbdev;

    ftd_blkdev_put(pstbdev);

    bio->av_forw = NULL;
    bio->bi_end_io = pstore_end_io;
    bio->bi_private = cloneduserbio;

    if (bio_add_page(bio, virt_to_page(page), PAGE_SIZE, 0) < PAGE_SIZE) 
    {
        bio_put(bio);
        printk("error adding page to LRT bio\n");
        error = -EFAULT;
        goto Done;
    }

         ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device(bio);
  Done:
         return error;
 }

/**
 * @bug The following code combined with bio_write_page() assume that PAGE_SIZE is a multiple of the pstore device's hardware sector size.
 *      This assumption just happens to work out of luck with the devices and PAGE_SIZE we have encountered so far.
 *      More intelligence should be added here if we want this to work the day we'll encounter a hardware device
 *      whose hardware block size is larger that PAGE_SIZE.
 *      We should also make sure that the pstore updates we do are within the device's maximum atomic I/O capabilities.
 */
FTD_PRIVATE int ftd_write_lrdb(ftd_dev_t *softp, struct buf *cloneduserbio)
{
    int ret = 0;
    /*
     * need to issue two requests, each with 4KB LRDB.
     * Why not just assemble a single bio with two pages?
     * Some drivers will not handle a multi-page bio yet.
     */
    /* Do not need to use the lrdbbp bio struct here, really just need
       sector, lrdb.map, and block device the lrdbbp struct is legacy
       from code that was attempting to use a single lrt bio for
       each dtc device */

#ifdef BIO_DATA_FROM_BAB
#else
    if (cloneduserbio)
        cloneduserbio->bi_private = softp;
#endif
    ret = bio_write_page(softp->lrdb_offset, softp->lrdb.map,softp->lgp->persistent_store, NULL);
    ret |= bio_write_page(softp->lrdb_offset+(PAGE_SIZE >> DEV_BSHIFT), softp->lrdb.map+PAGE_SIZE, softp->lgp->persistent_store, cloneduserbio);

    return (ret);

}

/*
 *  ftd_flush_lrdb
 *
 *  Write lrdb I/O to disk, schedule bio to be written upon LRDB completion
 *
 */
FTD_PUBLIC int ftd_flush_lrdb(ftd_dev_t *softp, void *bio, ftd_context_t context)
{
       struct buf *biop = bio;
       return (ftd_write_lrdb(softp, biop));
}


void ftd_gendisk_init(struct block_device *dtcbd, int index)
{
        dtcbd->bd_disk->major = ftd_bmajor;
        dtcbd->bd_disk->first_minor = index + 1;
        sprintf(dtcbd->bd_disk->disk_name, "ftd%d", index + 1);
        blk_queue_make_request(dtcbd->bd_disk->queue, ftd_make_request);
        dtcbd->bd_disk->fops = &ftd_bdops;
        dtcbd->bd_disk->private_data = dtcbd;
}

/**
 * @brief Arranges for our logical device to present the same attributes as the physical one.
 *
 * This is required so that the requests received on our queue can be passed as-is to the physical device's queue.
 *
 * Currently supported characteristics are:
 *
 * - Hardsector size (Not all devices use 512, for example, dasd disks on s390 can have 4096 hardsector size.)
 *
 * @param physical_block_device  The block device representing the physical device we're replicating.
 * @param logical_block_device   The block device of our logical device.
 *
 * @todo The following attribtues should also be considered:
 * - blk_queue_max_sectors
 * - blk_queue_max_phys_segments
 * - blk_queue_max_hw_segments
 * - blk_queue_max_segment_size
 * - blk_queue_segment_boundary
 * - void blk_queue_dma_alignment
 *
 * @todo Check out how the lvm driver (drivers/md/) deals with this.
 *
 */
FTD_PUBLIC void map_physical_device_attributes_to_logical_device(struct block_device* physical_block_device, struct block_device*  logical_block_device)
{
   struct request_queue *logical_block_device_queue = logical_block_device->bd_disk->queue;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   unsigned short physical_device_hardware_sector_size = bdev_hardsect_size(physical_block_device);

   ftd_debugf(device, "map_physical_device_attributes_to_logical_device: Setting up hardware sector size of logical %s to be of %d bytes like for the physical %s.\n",
              logical_block_device->bd_disk->disk_name,
              physical_device_hardware_sector_size,
              physical_block_device->bd_disk->disk_name);

   blk_queue_hardsect_size(logical_block_device_queue, physical_device_hardware_sector_size);


#else
   unsigned short physical_device_hardware_sector_size = bdev_logical_block_size(physical_block_device);

   ftd_debugf(device, "map_physical_device_attributes_to_logical_device: Setting up hardware sector size of logical %s to be of %d bytes like for the physical %s.\n",
              logical_block_device->bd_disk->disk_name,
              physical_device_hardware_sector_size,
              physical_block_device->bd_disk->disk_name);
   
   blk_queue_logical_block_size(logical_block_device_queue, physical_device_hardware_sector_size);

#endif
}


#ifdef FTD_DEBUG
/* display in kilo bytes */
#define K(x) ((x) << (PAGE_SHIFT - 10))
#endif

int ftd_init(void)
{
	int ret;
    int retc;
	int minor;
	ftd_ctl_t *ctlp;
    int j;

	int devno;
	dev_t cdev;

#ifdef FTD_DEBUG
	struct sysinfo i;
#endif
	printk("%s driver module init. (%s %s)\n", PRODUCTNAME, VERSION, VERSIONBUILD);

#if defined(CONFIG_COMPAT) && !defined(HAVE_COMPAT_IOCTL)
	ret = ftd_register_ioctl32_cmds();
	if (ret < 0)
	{
		printk(KERN_ALERT "ftd_init: Unable to register 32-bit ioctl cmds: %d\n", -ret);
		ftd_unregister_ioctl32_cmds();
		return (DDI_FAILURE);
	}
#endif

	ret = register_blkdev(ftd_bmajor, "ftd");

	if (ret < 0)
	{
		printk(KERN_ALERT "ftd_init: can't get major %d\n", ftd_bmajor);
		printk(KERN_ALERT "ftd_init: return value = %d\n", ret);
		return (DDI_FAILURE);
	}
	ftd_bmajor = ret;
	/* register the lg ctl as character device. */
	/* block and char using same major number.  */

	ftd_debugf(open, "ftd_init bmajor :  %d\n", ftd_bmajor);


	cdev = 0;
	retc = alloc_chrdev_region(&cdev, 0, MAXMIN+1, "ftd");


	if (retc < 0)
	{
		printk(KERN_ALERT "ftd_init: register char device = %d\n", retc);
		unregister_blkdev(ftd_bmajor,"ftd");
		return (DDI_FAILURE);
	}
	
	ftd_cdev = cdev_alloc();
	if (ftd_cdev == NULL) 
    {
		printk(KERN_ALERT "ftd_init: allocate char device failure\n");
		unregister_chrdev_region(cdev,MAXMIN+1);
		unregister_blkdev(ftd_bmajor,"ftd");
		return (DDI_FAILURE);
	}

    ftd_cdev->ops = &ftd_cdops;
	ftd_cdev->owner = THIS_MODULE;
	ftd_cmajor = MAJOR( cdev );

#ifdef CONFIG_DEVFS_FS
	#error "We do not support devfs.Check CONFIG_DEVFS_FS flag."
	devfs_dir    = devfs_mk_dir(NULL, QNM, NULL);
	if (! devfs_dir) return -EBUSY;		  /* problem */

	devfs_handle = devfs_register( devfs_dir, "ctl", DEVFS_FL_DEFAULT, ftd_bmajor, FTD_MAXMIN, S_IFBLK|S_IRUGO|S_IWUSR, &ftd_bdops,NULL);
#else

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
	devfs_mk_dir("ftd");
#endif

#endif

    proc_fs_init_driver_entries();
    
	/* forward all md request to md_make_request */

    ftd_blkdev = vmalloc(FTD_MAX_DEV*sizeof(struct block_device));
    if (ftd_blkdev == NULL) 
    {
            printk(KERN_ALERT "block_device alloc failed\n");
            return (-1);
    }
    memset(ftd_blkdev, 0, FTD_MAX_DEV*sizeof(struct block_device));


    /*
	 *    DDI_ATTACH is the only thing we know and grok.
	 */
	if (ftd_failsafe)
		return (DDI_FAILURE);

	/* Should NOT occur.
	 * For some reason, the driver is re-loaded/re-attached while the
	 * control structure is not release yet !!!  Free memory for this
	 * time and return error!
	 */
	/* From here is TDMF specific initialization. */
	if (ftd_global_state)
		goto error;

	ctlp = ftd_global_state = kmem_zalloc (sizeof (ftd_ctl_t), KM_NOSLEEP);

	if (ddi_soft_state_init (&ftd_dev_state, sizeof (ftd_dev_t), 16) != 0)
	{
		return DDI_FAILURE;
	}

	if (ddi_soft_state_init (&ftd_lg_state, sizeof (ftd_lg_t), 16) != 0)
	{
		if (ftd_global_state)
			kmem_free (ftd_global_state, sizeof (ftd_ctl_t));

		ddi_soft_state_fini (&ftd_dev_state);
		return DDI_FAILURE;
	}

#ifdef FTD_DEBUG
	si_meminfo(&i);
	printk("free RAM memory: %ld MBytes\n", K(i.freeram) >> 10);
	printk("chunk_size = %x num_chunks = %x \n", chunk_size, num_chunks);
#endif

	ctlp->chunk_size = chunk_size;
	ctlp->num_chunks = num_chunks;
	ctlp->bab_size = ftd_bab_init (ctlp->chunk_size, ctlp->num_chunks);
    ctlp->lghead = NULL;
    ctlp->hrdb_type = FTD_HS_NOT_SET;

	if  (ctlp->bab_size ==  0)
	{

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
        devfs_remove("ftd");
#endif

#ifdef CONFIG_DEVFS_FS
        devfs_unregister(devfs_dir);
#endif

        /* unregister character device */
	    cdev_del( ftd_cdev );
        unregister_chrdev_region(MKDEV(ftd_cmajor,0),MAXMIN+1);
        /* unregister block device */
        unregister_blkdev(ftd_bmajor, "ftd");
        vfree(ftd_blkdev);


#if defined(CONFIG_COMPAT) && !defined(HAVE_COMPAT_IOCTL)

        ftd_unregister_ioctl32_cmds();
#endif

		if (ftd_global_state)
			kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
		/* release memory allocated for stat structure */
		ddi_soft_state_fini (&ftd_dev_state);
		ddi_soft_state_fini (&ftd_lg_state);

		printk(KERN_ALERT "ftd_init: Can't init BAB.\n");
		return (DDI_FAILURE);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    ftd_metabuf_cache = kmem_cache_create(ftd_metabuf_cache_name,
                                          (int)ROUND_UP(sizeof(ftd_metabuf_t), sizeof(void *)),
                                          0,
                                          0,
                                          NULL,
                                          NULL);
#else
    ftd_metabuf_cache = kmem_cache_create(ftd_metabuf_cache_name,
                                          (int)ROUND_UP(sizeof(ftd_metabuf_t), sizeof(void *)),
                                          0,
                                          0,
                                          NULL);
#endif

    if (!ftd_metabuf_cache)
    {
        printk(KERN_ERR "ftd_init(): kmem_cache_create(%s, %d, ..) failed\n",ftd_metabuf_cache_name, (int)ROUND_UP(sizeof(ftd_metabuf_t), sizeof(void *)));
        goto error;
    }
	Started_LG_Map.lg_map = (ftd_uint64ptr_t) 0;
	Started_LG_Map.count=0;
	Started_LG_Map.lg_max=0;
	ftd_memset (Started_LGS, FTD_STOP_GROUP_FLAG, MAXLG);

	/* for Dynamic Mode Change */
	sync_time_out_flg = (ftd_int32_t *)kmem_zalloc(sizeof(ftd_int32_t) * MAXLG, KM_NOSLEEP);

	if (sync_time_out_flg == NULL)
	{
		goto error;
	}

#ifdef PRIVATE_MEMORY_POOL
	/*  Allocate private memory pool pages at init, for bio data that cannot be aligned properly in BAB
	    in case we transfer directly from BAB (BIO_DATA_FROM_BAB switch) */
	ALLOC_LOCK (mem_pool_lck, QNM " driver mem pool mutex");

	   /* Initialize linked list of private memory pool page pointers; at this stage
	      all pages are in the free list, none in the used list */
           INIT_LIST_HEAD( &free_pool_head );
           INIT_LIST_HEAD( &used_pool_head );
           printk( "<<< ftd_init(), static memory pool initialization: page size on this platform = %d\n", MEM_POOL_PAGE_SIZE );
	   for( j = 0; j < MEM_POOL_NUM_PAGES; j++ )
	   {
	     mem_pool_page_list[j].mem_pool_page_ptr = (char *)kmalloc(MEM_POOL_PAGE_SIZE, KM_NOSLEEP);
             if( mem_pool_page_list[j].mem_pool_page_ptr == 0 )
             {
                printk( "<<< ftd_init(): PRIVATE_MEMORY_POOL ERROR: cannot allocate private memory pool page number %d\n", j );
                printk( "<<< ftd_init(): PRIVATE_MEMORY_POOL ERROR: will proceed with memory pool of %d pages\n", num_pool_pages_allocated );
                // Zero the remaining pointers
                for( /* keep current j */; j < MEM_POOL_NUM_PAGES; j++ )
                {
                   mem_pool_page_list[j].mem_pool_page_ptr = (char *)0;
                }
                break;
             }
             else
             {
	        list_add_tail( (struct list_head *)&mem_pool_page_list[j], &free_pool_head );
                num_pool_pages_allocated++;
             }
	   }
#ifdef TRACE_PRIVATE_MEMORY_POOL
	   show_free_mem_pool_list( 0 );  // <-- pass 1 as argument to show all pages, 0 to show only first and last 
#endif
#endif // PRIVATE_MEMORY_POOL

    ret = ftd_dynamic_activation_init(MKDEV(ftd_cmajor, FTD_CTL));
    if (ret != 0)
    {
        printk(KERN_ALERT "ftd_init: Failed initializing dynamic activation: %d\n", ret);
        goto error;
    }
       
	/*
	 *     For a fixed-disk drive, read and verify the label
	 *     Don't worry if it fails, the drive may not be formatted.
	 */
	ALLOC_LOCK (ctlp->lock, QNM " driver mutex");

	retc = cdev_add( ftd_cdev, cdev, MAXMIN + 1);
	if (retc < 0) 
    {
		printk(KERN_ALERT "ftd_init: add char device = %d\n", retc);
		cdev_del( ftd_cdev );
		unregister_chrdev_region(cdev,MAXMIN+1);
		unregister_blkdev(ftd_bmajor,"ftd");
		goto error;
	}
    
	return 0;

error:
	/* release memory allocated for stat structure */
	ddi_soft_state_fini (&ftd_dev_state);
	ddi_soft_state_fini (&ftd_lg_state);
	/* free allocated BAB memory */
	ftd_bab_fini();

#if     0
	/* release private buffer pool */
	FTD_FINI_BUF_POOL ();
#endif

	if (ftd_global_state)
		kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
	if (sync_time_out_flg)
		kmem_free (sync_time_out_flg, sizeof(ftd_int32_t) * MAXLG);

	return (DDI_FAILURE);
}

void ftd_cleanup(void)
{
	ftd_context_t context;
	ftd_ctl_t *ctlp;
	int ret = 0;

	int devno;
	dev_t cdev;

    int j;

	printk("%s driver module cleanup\n", PRODUCTNAME);

	/*
	 * Before anything else, get rid of the timer functions.  Set the
	 * "usage" flag on each device as well, under lock, so that if the
	 * timer fires up just before we delete it, it will either complete
	 * or abort.  Otherwise we have nasty race conditions to worry about.
	 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
        devfs_remove("ftd");
#endif


#ifdef CONFIG_DEVFS_FS
	devfs_unregister(devfs_dir);
#endif

	/* unregister character device */
	cdev_del( ftd_cdev );
    unregister_chrdev_region(MKDEV(ftd_cmajor,0),MAXMIN+1);
	/* unregister block device */
    unregister_blkdev(ftd_bmajor, "ftd");

    /*  NAA See WR 41682 why this code was removed from here and the vfree moved
        to the end of this function.
        for (devno = 0; devno < FTD_MAX_DEV; devno++) {
           ftd_gendisk_cleanup(blkdev + devno);
        }
        vfree(blkdev);
*/

#if defined(CONFIG_COMPAT) && !defined(HAVE_COMPAT_IOCTL)

	ftd_unregister_ioctl32_cmds();
#endif
    
    proc_fs_finish_driver_entries();

#ifdef PRIVATE_MEMORY_POOL
    if( num_pool_pages_allocated )
    {
       for( j = 0; j < num_pool_pages_allocated; j++ )
       {
          if( mem_pool_page_list[j].mem_pool_page_ptr )
	     kmem_free( mem_pool_page_list[j].mem_pool_page_ptr, MEM_POOL_PAGE_SIZE );
       }
       num_pool_pages_allocated = 0;
    }
    DEALLOC_LOCK( mem_pool_lck );
#endif // PRIVATE_MEMORY_POOL
    
    /* FIXME: max_readahead and max_sectors */

	/* From here it is TDMF specific de-initialization. */

	if (ftd_failsafe) {
		ret = (DDI_FAILURE);
		goto error;
	}

	if ((ctlp = ftd_global_state) == NULL) {
		ret = (DDI_FAILURE);
		goto error;
	}

	/* ACQUIRE_LOCK (ctlp->lock, context); */

	/*
	 *     We can't detach while things are open.
	 */
	if (ftd_dev_n_open (ctlp)) {
		/* RELEASE_LOCK (ctlp->lock, context); */
		ret = (DDI_FAILURE);
	}

	/*
	 *     Kill them all!
	 */
	while (ctlp->lghead)
	{
		ftd_del_lg (ctlp->lghead, getminor (ctlp->lghead->dev));
	}

	/* RELEASE_LOCK (ctlp->lock, context); */
	DEALLOC_LOCK (ctlp->lock);

	/*
	 *     Remove other data structures allocated in attach()
	 */

        kmem_cache_destroy(ftd_metabuf_cache);

	ftd_bab_fini ();
	if (ftd_global_state) kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
	if (sync_time_out_flg) kmem_free (sync_time_out_flg, sizeof(ftd_int32_t) * MAXLG);

	if (ftd_dev_state) ddi_soft_state_fini (&ftd_dev_state);
	if (ftd_lg_state) ddi_soft_state_fini (&ftd_lg_state);

    ftd_dynamic_activation_finish();
    
#if	0
	/* release private buffer pool */
	FTD_FINI_BUF_POOL ();
#endif


    vfree(ftd_blkdev);

	return ;				  //SUCCESS.

error:
	if(ret < 0)
		printk(KERN_ALERT " Error in module_exit(ftd_cleanup) \n");
	return ;				  //FAILURE.
}


//layered open/close for local disk and persistent store disk.
FTD_PUBLIC ftd_int32_t ftd_layered_open(struct block_device **bdev, dev_t dev_num)
{
	int err;


	if(!(*bdev = bdget(dev_num)))
	{
		return ENOMEM;
	}

	err =  ftd_blkdev_get(*(bdev), 0);

	if(err)
	{
		return err;
	}


	return 0;
}


FTD_PUBLIC ftd_void_t ftd_layered_close(struct block_device *bdev, dev_t dev_num)
{
    ftd_debugf(delete, "ftd_layered_close(%p, %x)\n", bdev, (unsigned)dev_num);

    if(bdev)
    {
    	ftd_blkdev_put(bdev);
    }

}


FTD_PUBLIC ftd_void_t ftd_wakeup(ftd_lg_t * lgp)
{
	wake_up(&lgp->ph);
}

/*-
 * ftd_thread_lock(), ftd_thread_unlock()
 *
 * These replace the lock_kernel() and unlock_kernel() direct calls which are no longer available
 * in most recent releases of Linux (WR PROD11894 SuSE 11.2).
 */
FTD_PUBLIC ftd_void_t
ftd_thread_lock( void *mutex_lock_ptr )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
    mutex_lock( (struct mutex *)mutex_lock_ptr );
#else
    lock_kernel();
#endif
    return;
}

FTD_PUBLIC ftd_void_t
ftd_thread_unlock( void *mutex_lock_ptr )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
    mutex_unlock( (struct mutex *)mutex_lock_ptr );
#else
    unlock_kernel();
#endif
    return;

}

FTD_PUBLIC void * kmem_zalloc(int alloc_size, int flg)
{
    void *p = NULL;
    int tries = 0;
    int gfp_mask = GFP_KERNEL;

    if (flg == KM_NOSLEEP)
        gfp_mask &= (~(__GFP_WAIT));

    if (alloc_size == 0)
        return NULL;
    do {
    	if (alloc_size > ftd_kmalloc_ceiling) {
           gfp_mask |= __GFP_HIGHMEM;
           p = __vmalloc (alloc_size, gfp_mask, PAGE_KERNEL);
        } else {
	   p = kmalloc(alloc_size, gfp_mask);
        }
        if (p || (flg & KM_NOSLEEP)) {
	   if (p)
              memset(p, 0, alloc_size);
    	   return p;
   	}
	if (++tries == 200) {
	    if (alloc_size > ftd_kmalloc_ceiling) {
               FTD_ERR (FTD_WRNLVL, "__vmalloc(%d, %#x, %s) fails in range [%lx, %lx].",
                alloc_size, (unsigned)gfp_mask, "PAGE_KERNEL",
                VMALLOC_START, VMALLOC_END);
	    } else {
               FTD_ERR (FTD_WRNLVL, "kmalloc(%d, %#x) fails.",
            	alloc_size, (unsigned)gfp_mask);
	    }
            tries = 0;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
  	blk_congestion_wait(1, HZ/50);
#else
  	congestion_wait(WRITE, HZ/50);
#endif
    } while (1);
}


#define IN_INTERVAL(v, l, u) (((unsigned long)(((unsigned long)(v)) - ((unsigned long)(l)))) <= ((unsigned long)(u)))

FTD_PUBLIC void kmem_free(void* p, int size)
{
    if (p == NULL)
        return;
    if (size > ftd_kmalloc_ceiling) 
    {
        if (IN_INTERVAL(p, VMALLOC_START, VMALLOC_END)) 
        {
            vfree(p);
        } 
        else 
        {
            if (0)
                panic("kmem_free(%p, %d): address not in [%#lx, %#lx]\n",p, size, VMALLOC_START, VMALLOC_END);
            else
                printk(KERN_ERR "kmem_free(%p, %d): address not in [%#lx, %#lx]\n",p, size, VMALLOC_START, VMALLOC_END);
        }
    } else
        kfree(p);
}

#if defined(CONFIG_COMPAT) && !defined(HAVE_COMPAT_IOCTL)

static int ftd_register_ioctl32_cmds(void)
{
	int result = 0;
	int i;
	for (i = 0; ftd_ioctl32_cmd[i].cmd; i++) 
	{
		result = register_ioctl32_conversion(ftd_ioctl32_cmd[i].cmd, ftd_ioctl32_cmd[i].func);
		if (result) {
			break;
		}
		max_ioctl32 = i;
	}
	return result;
}

static int ftd_unregister_ioctl32_cmds(void)
{
	int result = 0;
	int err;
	while (max_ioctl32 >= 0) 
	{
		err = unregister_ioctl32_conversion(ftd_ioctl32_cmd[max_ioctl32].cmd);
		if (err && !result)
			result = err;
		max_ioctl32--;
	}
	return result;
}

#endif

module_init(ftd_init);
module_exit(ftd_cleanup);

MODULE_LICENSE("Proprietary");


module_param(chunk_size, int, 0);
module_param(num_chunks, int, 0);

