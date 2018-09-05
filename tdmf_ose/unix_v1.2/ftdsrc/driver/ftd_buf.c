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
#if defined(linux)
#include <linux/types.h>
#include "ftd_linux.h"
#else
#include <sys/types.h>
#endif
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

/**
 * Here is some background about the management of the ftd_metabuf structure.
 *
 * Incoming IO requests are received as a 'struct buf' requests.
 * This is the standard unix kernel structure where an IO request is described.
 *
 * The IO requests we receive are relayed to the source device using a modified cloned (copied) request.
 *
 * This cloned request lives in a ftd_metabuf structure, which holds both the cloned struct buf request,
 * as well as additional bookeeping information for our own purposes.
 *
 * In order to obtain the ftd_metabuf structures, a layered memory management system is used.
 *
 * Here are these layers.
 * 
 * 1-) An on demand global pool of ftd_metabuf allocation mechanism.
 *
 * This is given by the GETRBUF() and FREERBUF() API and is implemented within ftd_buf.c
 * This actually manages a reusable ftd_metabuf pool, which is increased as needed by chunks of FTD_MAX_BUF (64) ftd_metabuf structures.
 *
 * 2-) A device specific pool of ftd_metabuf.
 *
 * Each device initially obtains FTD_MAX_BUF (64) ftd_metabuf structures from the lower layer to form its private pool.
 * These ftd_metabuf are linked within the device's freelist member, through the private member pointers of the ftd_metabuf.
 * 
 * When needing to clone an incoming IO request, the first structure on the freelist will be used ( ftd_strategy()->ftd_do_io()->ftd_bioclone() ).
 * When the relayed IO has completed, the structure will be put back on the freelist ( ftd_finish_io() ).
 * 
 * 3-) An on demand additional allocation mechanism when the last element of the device specific pool has been assigned.
 * 
 * When ftd_bioclone() detects that it is using the last element of the freelist, it dynamically requests an additional
 * ftd_metabuf for the device from the 1st layer.
 * 
 * If successful, this guarantees 1 available ftd_metabuf on the freelist.
 *
 * The first ftd_metabuf obtained in this manner are permanently added to the device specific pool up to a limit of FTD_MAX_BUF new elements.
 * Additional ftd_metabuf allocated on demand are otherwise returned to the 1st layer when the IO completes ( ftd_finish_io() ).
 * The per device variable (bufcnt) is used to track these additional requests.
 * 
 * 4-) An IO deferring mechanism.
 * 
 * In the event that the freelist has no available ftd_metabuf (out of metabuf_t globally), the processing of the incoming IOs
 * is deferred until a ftd_metabuf becomes available ( ftd_finish_io() ).
 * 
 * To do this, the incoming IOs are queued into the device's qtail queue, which acts as a postponed IO queue.
 * The IO requests' av_forw fields are used to link together the postponed IOs in this queue.
 * When a relayed IO completes, its ftd_metabuf is reused to submit the head of the postponed queue from the completion routine of this relayed IO.
 * 
 * In order to maintain write ordering, any new incoming IO will always be postponed at the end of the queue (qtail) as long as there are IOs on this queue.
 *
 */

size_t ftd_buf_pool_size=0;
size_t ftd_buf_pool_count = 0;
struct buf *ftd_buf_pool = (struct buf *)0;
extern struct ftd_ctl *ftd_global_state;

#if !defined(_AIX)
struct buf *ftd_buf_pool_list[FTD_MAX_DEV+1];
int ftd_buf_pool_list_index = 0;
size_t ftd_buf_pool_size_per_dev;
#endif

/* private buffer pool locks */
#if defined(HPUX)
static lock_t *bufpool_lck;
#elif defined(SOLARIS)
static kmutex_t bufpool_lck;
#elif defined(_AIX)
static Simple_lock bufpool_lck;
#elif defined(linux)
static spinlock_t bufpool_lck;
#endif


#if !defined (LINUX260) 
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
#if defined(_AIX)
  ftd_buf_pool_count = FTD_MAX_DEV * (FTD_MAX_BUF + 1);
#else
  ftd_buf_pool_count = FTD_MAX_BUF + 1;
#endif
  ftd_buf_pool_size = ftd_buf_pool_count * sizeof (ftd_metabuf_t);

  /* get pool kmem */
  ftd_buf_pool = (struct buf *) kmem_zalloc (ftd_buf_pool_size, KM_NOSLEEP);
  if (ftd_buf_pool == (struct buf *) 0)
    {
      return ((struct buf *) 0);
    }
#if !defined(_AIX)
  /* keep address for kmem_free */
  ftd_buf_pool_list[0] = ftd_buf_pool;
  ftd_buf_pool_size_per_dev = ftd_buf_pool_size;
#endif

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
#if !defined(_AIX)
  int i;
#endif

#if defined(_AIX)
  kmem_free (ftd_buf_pool, ftd_buf_pool_size);
#else	/* defined(SOLARIS) || defined(HPUX) */
  for (i = 0; i <= ftd_buf_pool_list_index; i++)
    {
#if defined(HPUX)
        if (ftd_buf_pool_list[i] != NULL) FREE (ftd_buf_pool_list[i], M_IOSYS);
#else
        if (ftd_buf_pool_list[i] != NULL) kmem_free (ftd_buf_pool_list[i], ftd_buf_pool_size_per_dev);
#endif
        ftd_buf_pool_list[i] = NULL;
    }
#endif

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
getpvtrbuf( ftd_int32_t sleep_code )
{
  struct buf *bp;
  struct buf *new_entry_ptr;
  ftd_ctl_t *ctlp;
#if !defined(SOLARIS)
  ftd_context_t context;
#endif /* !defined(SOLARIS) */
#if !defined(_AIX)
  ftd_int32_t i;
  struct buf *bp1;
  ftd_metabuf_t *mbp;
#endif

  if ((ctlp = ftd_global_state) == NULL)
    panic ("FTD: Lost global state");

  ACQUIRE_LOCK (bufpool_lck, context);
  if (ctlp->ftd_buf_pool_headp)
    {
      bp = ctlp->ftd_buf_pool_headp;
#if !defined(_AIX)
      /* allocate entry check */
	  if( (bp->av_forw == NULL) && (ftd_buf_pool_list_index < FTD_MAX_DEV) )
	  {
	      if( sleep_code == KM_SLEEP )
	      {
	          // If sleep is requested in case of lack of memory available, we must release the lock
              RELEASE_LOCK (bufpool_lck, context);
	      }

	      new_entry_ptr = kmem_zalloc (ftd_buf_pool_size_per_dev, sleep_code);
	      
	      if( sleep_code == KM_SLEEP )
	      {
	          // Reacquire the lock if it was released and, if we successfully got memory,
	          // check if a buffer just became available
			  // or if ctlp->ftd_buf_pool_headp has changed while we had released the lock.
			  // If so, free the memory we just allocated and work from the new state of
			  // the buffer pool.
              ACQUIRE_LOCK (bufpool_lck, context);
			  if( new_entry_ptr != NULL )
			  {
    			  if( (bp = ctlp->ftd_buf_pool_headp) == NULL )
    			  {
                      kmem_free (new_entry_ptr, ftd_buf_pool_size_per_dev);
    				  new_entry_ptr = NULL;
    			  }
				  else
				  {
    			      if( bp->av_forw != NULL )
        			  {
                          kmem_free (new_entry_ptr, ftd_buf_pool_size_per_dev);
        				  new_entry_ptr = NULL;
        			  }
				  }
			  }
	      } // if( sleep_code...
	  } // if( (bp->av_forw == NULL...
  }	// if (ctlp->ftd_buf_pool_headp...

  if (ctlp->ftd_buf_pool_headp)
    {
      bp = ctlp->ftd_buf_pool_headp;
      if (bp->av_forw == NULL) {
          if (ftd_buf_pool_list_index < FTD_MAX_DEV) {
              /* Take new entry allocated above */

              if ((ftd_buf_pool_list[++ftd_buf_pool_list_index] = new_entry_ptr) == NULL)
              {
                  ftd_buf_pool_list_index--;
              } else {
                  ftd_buf_pool_count += (FTD_MAX_BUF + 1);
                  ftd_buf_pool_size += ftd_buf_pool_size_per_dev;
                  bp->av_forw = ftd_buf_pool_list[ftd_buf_pool_list_index];
                  mbp = (ftd_metabuf_t *)ftd_buf_pool_list[ftd_buf_pool_list_index];

                  /* cons a free list */
                  for (i = 0; i <= FTD_MAX_BUF; i++) {
                      mbp[i].m_mbp = &mbp[i].m_buf;
                      bp1 = (struct buf *) &(mbp[i].m_buf);
                      if (i == 0)
                        {
                          bp1->av_back = bp;
                        }
                      else
                        {
                          bp1->av_back = &(mbp[i - 1].m_buf);
                        }
                      if (i == FTD_MAX_BUF)
                        {
                          bp1->av_forw = NULL;
                        }
                      else
                        {
                          bp1->av_forw = &(mbp[i + 1].m_buf);
                        }
                  }
              }
          } /* ftd_buf_pool_list_index < FTD_MAX_DEV */
      } /* bp->av_forw == NULL */
#endif
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
#if !defined(SOLARIS)
  ftd_context_t context;
#endif /* !defined(SOLARIS) */ 

  if ((ctlp = ftd_global_state) == NULL)
    panic ("FTD: Lost global state");

  ACQUIRE_LOCK (bufpool_lck, context);
  bp->av_forw = ctlp->ftd_buf_pool_headp;
  ctlp->ftd_buf_pool_headp = bp;
  RELEASE_LOCK (bufpool_lck, context);

}
#endif /* !defined LINUX(260) */
