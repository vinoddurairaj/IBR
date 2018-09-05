#if !defined(FTD_BUF_H)
#define  FTD_BUF_H

#include <sys/buf.h>

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
#if defined(HPUX)
#define FTD_MAX_BUF	(128)  /* Workaround for HP Hang see dvr/442 */
#else
#define FTD_MAX_BUF     (64)   
#endif
#endif

/* a priori assumption of maximum number of configured devices */
#ifndef FTD_MAX_DEV
#define FTD_MAX_DEV     (256)  
#endif

/* types */

/*-
 * since we head another driver,
 * don't use struct buf fields to record our data...
 * _AIX uses this for all of its buffers, the others
 * use it for the LRT map buf header...
 */
typedef struct ftd_metabuf {
	struct buf  m_buf;		/* buffer itself */
	struct buf *m_mbp;		/* clone bp */
	struct buf *m_ubp;		/* cloned bp */
	void *m_private;		/* whatever */
	struct wlheader_s *m_wlhp;	/* write log header */
	void *m_softp;			/* associated ftd device */
} ftd_metabuf_t;

/* accessors */

#if defined(HPUX)

#define BP_PRIVATE(bp) ((bp)->b_s7)
#define BP_DEV(bp) ((bp)->b_dev)
#define BP_PCAST (struct buf *)
#define BIOD_CAST(f) ((int(*)())(f))
#define BP_USER(bp) BP_PRIVATE((bp))
#define BP_WLHDR(bp) BP_PRIVATE((bp))
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))

#elif defined(SOLARIS)

#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)
#define BP_DEV(bp) ((bp)->b_edev)
#define BP_PCAST (caddr_t)
#define BIOD_CAST(f) ((int(*)())(f))
#define BP_USER(bp) BP_PRIVATE((bp))
#define BP_WLHDR(bp) ((*(ftd_metabuf_t **)&(bp))->m_wlhp)
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))

#elif defined(_AIX)

#define BP_DEV(bp) ((bp)->b_dev)
#define BP_PCAST (struct buf *)
#define BIOD_CAST(f) ((void(*)())(f))
#define BP_PRIVATE(bp) ((*(ftd_metabuf_t **)&(bp))->m_private)
#define BP_USER(bp) ((*(ftd_metabuf_t **)&(bp))->m_ubp)
#define BP_WLHDR(bp) ((*(ftd_metabuf_t **)&(bp))->m_wlhp)
#define BP_SOFTP(bp) (*(ftd_dev_t **)&((*(ftd_metabuf_t **)&(bp))->m_softp))

#else
NOT_IMPLEMENTED
#endif

/* exports */

/* buffer pool */
#if defined(_AIX)
extern struct buf *ftd_buf_pool;
extern size_t ftd_buf_pool_size;
#else  /* defined(_AIX) */
extern struct buf *ftd_buf_pool;
extern int ftd_buf_pool_size;
#endif /* defined(_AIX) */

/* protos */

/* ftd_buf.c */
extern struct buf *ftd_init_pvt_buf_pool(void);
extern void ftd_fini_pvt_buf_pool(void);
extern struct buf *getpvtrbuf(int flag);
extern void freepvtrbuf(struct buf *bp);

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
#if defined(_AIX) || defined(SOLARIS)
#define GETRBUF getpvtrbuf
#define FREERBUF freepvtrbuf
#define FTD_INIT_BUF_POOL ftd_init_pvt_buf_pool
#define FTD_FINI_BUF_POOL ftd_fini_pvt_buf_pool
#else /* defined(_AIX) || defined(SOLARIS) */
#define GETRBUF getpvtrbuf
#define FREERBUF freepvtrbuf
#define FTD_INIT_BUF_POOL ftd_init_buf_pool
#define FTD_FINI_BUF_POOL ftd_fini_pvt_buf_pool
#endif /* defined(_AIX) || defined(SOLARIS) */

#endif /* !defined(FTD_BUF_H) */
