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
#if !defined(FTD_BUF_H)
#define  FTD_BUF_H

#ifndef linux
#include <sys/buf.h>
#endif

/*-
 * driver private buffer pool.
 * the pool is preallocated at driver init time
 * so that buffers can be had without making
 * calls that might sleep. also, this relieves
 * the driver of depending on struct buf fields
 * for driver private reference usage. these
 * fields might be used by drivers in other
 * layers of our stack.
 */

/* number of buffers that we allocate per device.  */
#ifndef FTD_MAX_BUF
#define FTD_MAX_BUF     (64)
#endif

/* a priori assumption of maximum number of configured devices */
#ifndef FTD_MAX_DEV
#define FTD_MAX_DEV     (1024)
#endif

#if defined (linux)
#define dynamic_buf_pool
#endif

/* types */

/*-
 * since we head another driver,
 * don't use struct buf fields to record our data...
 * _AIX uses this for all of its buffers, the others
 * use it for the LRT map buf header...
 */
typedef struct ftd_metabuf
  {
    struct buf m_buf;		/* buffer itself */
    struct buf *m_mbp;		/* clone bp */
    struct buf *m_ubp;		/* cloned bp */
    ftd_void_t *m_private;	/* whatever */
    struct wlheader_s *m_wlhp;	/* write log header */
    ftd_void_t *m_softp;	/* associated ftd device */
#if defined(linux)
    int m_rw;                   /* read/write flag      */
    int m_dirty;		/* lrdb dirty, need flush */
#endif
  }
ftd_metabuf_t;

#if defined(linux)
typedef struct ftd_metabuf2
  {
    struct buf m_buf1;		/* for first 4KB of LRT  */
    struct buf m_buf2;		/* for second 4KB of LRT  */
    int rw;			/* read/write flag      */
  }
ftd_metabuf2_t;   
#endif

/* accessors */

#if defined(HPUX)

#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)
#define BP_DEV(bp) ((bp)->b_dev)
#define BP_PCAST (caddr_t)
#define BIOD_CAST(f) ((ftd_int32_t(*)())(f))
#define BP_USER(bp) BP_PRIVATE((bp))
#define BP_WLHDR(bp) ((*(ftd_metabuf_t **)&(bp))->m_wlhp)
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))

#elif defined(SOLARIS)

#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)
#define BP_DEV(bp) ((bp)->b_edev)
#define BP_PCAST (caddr_t)
#define BIOD_CAST(f) ((ftd_int32_t(*)())(f))
#define BP_USER(bp) BP_PRIVATE((bp))
#define BP_WLHDR(bp) ((*(ftd_metabuf_t **)&(bp))->m_wlhp)
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))

#elif defined(_AIX)

#define BP_DEV(bp) ((bp)->b_dev)
#define BP_PCAST (struct buf *)
#define BIOD_CAST(f) ((ftd_void_t(*)())(f))
#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)
#define BP_USER(bp) ((*(ftd_metabuf_t **)&(bp))->m_ubp)
#define BP_WLHDR(bp) ((*(ftd_metabuf_t **)&(bp))->m_wlhp)
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))

#elif defined(linux)

#define FTD_READAHEAD vm_max_readahead
#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define BP_DEV(bp) ((bp)->b_dev)
#else
#define BP_DEV(bp) ((bp)->bi_bdev->bd_dev)
#endif /* < KERNEL_VERSION(2, 6, 0) */

#define BIOD_CAST(f) ((ftd_int32_t(*)())(f))
#define BP_USER(bp) ((*(ftd_metabuf_t **)&(bp))->m_ubp)
#define BP_WLHDR(bp) ((*(ftd_metabuf_t **)&(bp))->m_wlhp)
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))
#define BP_RW(bp) (((ftd_metabuf_t *)bp)->m_rw)
#define BP_DIRTY(bp) (((ftd_metabuf_t *)bp)->m_dirty)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
// not completed (thonda)
#define BP_DEV(bp) ((bp)->b_dev)
#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)
#endif /* < KERNEL_VERSION(2, 6, 0) */

#else
NOT_IMPLEMENTED
#endif

/*- 
 * portability i/f 
 * 
 * _AIX and SOLARIS use the driver private 
 * pool, someday HPUX too. not currently
 * doing this on HPUX since the buffer that
 * we pass on is the user buffer, in which
 * case the private fields in the metabuffer
 * can't be passed and used for reference 
 * at iodone time.
 *
 * getpvtrbuf() freepvtrbuf() are interfaces
 * to the private pool. getrbuf() and freerbuf()
 * are dki(9) system buffer pool interfaces.
 */
#define GETRBUF(sleep_code) getpvtrbuf(sleep_code)
#define FREERBUF freepvtrbuf
#define FTD_INIT_BUF_POOL ftd_init_pvt_buf_pool
#define FTD_FINI_BUF_POOL ftd_fini_pvt_buf_pool

#endif /* !defined(FTD_BUF_H) */
