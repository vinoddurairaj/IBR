#include <sys/types.h>
#include "ftd_kern_ctypes.h"
#include "ftd_buf.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_kern_cproto.h"

/*-
 * driver private buffer pool.
 * purpose is to relieve driver of dependence on 
 * buffer header fields for private references.
 * the private buffer pool consists of `metabuffers'.
 * these are simply struct buf's embedded in
 * a structure which includes fields for the
 * driver's private references. 
 * macros in ftd_buf.h are used to convert 
 * struct buf pointer references to such private 
 * data.
 *
 * N.B.: this was developed for use by _AIX,
 * and extended for use by SOLARIS. HPUX yet
 * uses something different, god bless the
 * child that's got his own...
 */
size_t ftd_buf_pool_size=0;
size_t ftd_buf_pool_count = 0;
struct buf *ftd_buf_pool = (struct buf *)0;
extern struct ftd_ctl *ftd_global_state;

/* private buffer pool locks */
#if defined(HPUX)
static lock_t *bufpool_lck;
#if !defined(dynamic_buf_pool)
ftd_char_t ftd_buf_pool_mem[BUF_POOL_MEM_SIZ];
#endif
#elif defined(SOLARIS)
static kmutex_t bufpool_lck;
#elif defined(_AIX)
static Simple_lock bufpool_lck;
#endif


/*-
 * ftd_init_pvt_buf_pool()
 *
 * allocate and init structs for a pool of buf(9s)
 * headers private to this driver.
 */
FTD_PUBLIC struct buf *
ftd_init_pvt_buf_pool (ftd_void_t)
{
  ftd_int32_t i;
  ftd_int32_t rc;
  struct buf *bp;
  ftd_metabuf_t *mbp;
  static ftd_int32_t once = 1;

  if (once)
    {
      ALLOC_LOCK (bufpool_lck, QNM " driver mutex");
      once = 0;
    }

	/*-
	 * assume: 
	 * a maximum of FTD_MAX_DEV devs
	 * FTD_MAX_BUF count per dev for the its free buffer pool
	 * 1 count per dev for its lrdb buf.
	 */
  ftd_buf_pool_count = FTD_MAX_DEV * (FTD_MAX_BUF + 1);
  ftd_buf_pool_size = ftd_buf_pool_count * sizeof (ftd_metabuf_t);

  /* get pool kmem */
#if defined(dynamic_buf_pool)
  ftd_buf_pool = (struct buf *) kmem_zalloc (ftd_buf_pool_size, KM_NOSLEEP);
  if (ftd_buf_pool == (struct buf *) 0)
    {
      return ((struct buf *) 0);
    }
#else /* defined(dynamic_buf_pool) */
  ftd_buf_pool = (struct buf *) &ftd_buf_pool_mem[0];
#endif /* defined(dynamic_buf_pool) */

  mbp = (ftd_metabuf_t *) ftd_buf_pool;

  /* cons a free list */
  for (i = 0; i < ftd_buf_pool_count; i++)
    {
      mbp[i].m_mbp = &mbp[i].m_buf;
      bp = (struct buf *) &(mbp[i].m_buf);
      if (i == 0)
	{
	  bp->av_back = NULL;
	}
      else
	{
	  bp->av_back = &(mbp[i - 1].m_buf);
	}
      if (i == ftd_buf_pool_count - 1)
	{
	  bp->av_forw = NULL;
	}
      else
	{
	  bp->av_forw = &(mbp[i + 1].m_buf);
	}
    }

  return (struct buf *) ftd_buf_pool;
}

/*-
 * ftd_fini_pvt_buf_pool()
 *
 * release any resources allocated in ftd_init_pvt_buf_pool.
 */
FTD_PUBLIC ftd_void_t
ftd_fini_pvt_buf_pool (ftd_void_t)
{

#if defined(dynamic_buf_pool)
#if defined(HPUX)
  FREE (ftd_buf_pool, M_IOSYS);
#else
  kmem_free (ftd_buf_pool, ftd_buf_pool_size);
#endif
#endif /* defined(dynamic_buf_pool) */

  ftd_buf_pool = (struct buf *) 0;

  DEALLOC_LOCK (bufpool_lck);

  return;
}


/*-
 * bufzero()
 *
 * zero-fill a buf(9s) struct
 */
FTD_PUBLIC ftd_void_t
bufzero (struct buf * bp)
{
  bzero ((caddr_t) bp, sizeof (struct buf));

  return;
}

/*-
 * getpvtrbuf()
 *
 * our version of getrbuf(9f).
 */
FTD_PUBLIC struct buf *
getpvtrbuf()
{
  struct buf *bp;
  ftd_ctl_t *ctlp;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if ((ctlp = ftd_global_state) == NULL)
    panic ("FTD: Lost global state");

  ACQUIRE_LOCK (bufpool_lck, context);
  if (ctlp->ftd_buf_pool_headp)
    {
      bp = ctlp->ftd_buf_pool_headp;
      ctlp->ftd_buf_pool_headp = bp->av_forw;
      bufzero (bp);
    }
  else
    {
      /* no more free buffers */
      bp = NULL;
    }
  RELEASE_LOCK (bufpool_lck, context);

  return (bp);
}

/*-
 * freepvtrbuf()
 *
 * our version of freerbuf(9f).
 */
FTD_PUBLIC ftd_void_t
freepvtrbuf (struct buf * bp)
{
  ftd_ctl_t *ctlp;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if ((ctlp = ftd_global_state) == NULL)
    panic ("FTD: Lost global state");

  ACQUIRE_LOCK (bufpool_lck, context);
  bp->av_forw = ctlp->ftd_buf_pool_headp;
  ctlp->ftd_buf_pool_headp = bp;
  RELEASE_LOCK (bufpool_lck, context);

}
