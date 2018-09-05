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
/*-
 * ftd_bab.c 
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#if defined(linux)
#include "ftd_linux.h"
#endif

#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"
#if defined(_AIX)
#include <sys/uio.h>
#endif

/* size of each memory chunk */
#if !defined (LINUX260)
FTD_PRIVATE ftd_int32_t chunk_size;
#else
static ftd_int32_t chunk_size;
#endif

/* maximum number of buffers any single manager can have in use */
FTD_PRIVATE ftd_int32_t max_bab_buffers_in_use;

/* the list of buffer managers */
FTD_PRIVATE bab_mgr_t *bab_mgrs;

/* the list of free buffers */
#if defined(SOLARIS)
static kmutex_t bab_lck;
#elif defined(HPUX)
static lock_t *bab_lck;
#elif defined(_AIX)
static Simple_lock bab_lck;
#elif defined(linux)
static spinlock_t  bab_lck;
#endif

FTD_PRIVATE ftd_int32_t num_free_bab_buffers;
FTD_PRIVATE bab_buffer_t *free_bab_buffers;

#if defined(HPUX) && defined(STATIC_BAB_MALLOC)
/* one big ass buffer */
extern ftd_uint64_t big_ass_buffer[];
extern ftd_int32_t big_ass_buffer_size;
#endif

#if defined(FTD_DEBUG) && 0
/*-
 * ftd_bab_dump_free_buffers()
 * 
 * debugging diagnostics.
 */
FTD_PUBLIC ftd_void_t
ftd_bab_dump_free_buffers ()
{
  bab_buffer_t *temp;

  FTD_ERR (FTD_DBGLVL, "number of free buffers = %d\n", num_free_bab_buffers);
  temp = free_bab_buffers;
  while (temp)
    {
      FTD_ERR (FTD_DBGLVL, "BUF (0x%x): 0x%x -> 0x%x\n",
	       (ftd_int32_t) temp, (ftd_int32_t) temp->start, (ftd_int32_t) temp->end);
      temp = temp->next;
    }
}
#endif /* FTD_DEBUG */

/*-
 * ftd_bab_init()
 *
 * called at driver install time.
 *    size: size of each chunk in bytes
 *    num:  number of chunks 
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_init (ftd_int32_t size, ftd_int32_t num)
{
  ftd_int32_t i;
  bab_buffer_t *buf;
  ftd_int32_t bablen;
  ftd_int32_t j;
  ftd_int32_t giveback;

  Enter (int, FTD_BAB_INIT, 0, size, num, 0, 0);

  ALLOC_LOCK (bab_lck, QNM " driver mutex");

  bab_mgrs = NULL;

  /* create the buffers and add them to the free list */
  chunk_size = size / 8;

  num_free_bab_buffers = 0;
  free_bab_buffers = NULL;

#if defined(HPUX) && defined(STATIC_BAB_MALLOC)
  bablen = big_ass_buffer_size;
  /* break up our BSS buffer into a bunch of chunks */
  for (i = 0; i <= ((big_ass_buffer_size / 8) - chunk_size); i += chunk_size)
    {
      buf = (bab_buffer_t *) & big_ass_buffer[i];

#elif defined(SOLARIS) || defined(_AIX) || defined(HPUX) || defined(linux)
      bablen = 0;
      /* do a bunch of mallocs */
      for (i = 0; i < num; i++)
	{
#if defined(linux)
	  buf = (bab_buffer_t *) __get_free_pages(GFP_KERNEL,
			get_order(chunk_size * sizeof (ftd_uint64_t)));
#elif defined(HPUX)
	  buf = (bab_buffer_t *) kmem_zalloc (chunk_size * sizeof (ftd_uint64_t), KM_NOSLEEP);
#else
	  buf = (bab_buffer_t *) kmem_alloc (chunk_size * sizeof (ftd_uint64_t), KM_SLEEP);
#endif
	  if (buf == NULL)
	    {
	      if (i < 2)
		{
		  FTD_ERR (FTD_WRNLVL,
		    "%s: Not enough memory for minimum bab.\n", DRIVERNAME);
		  Return (0);
		}
	      FTD_ERR (FTD_NTCLVL, "%s: Only allocated %d of %d requested bytes.",
		       DRIVERNAME, bablen, size * num);
	      break;
	    }
	  bablen += chunk_size * sizeof (ftd_uint64_t);
#endif
	  buf->start = (ftd_uint64_t *) (buf + 1);
	  buf->end = ((ftd_uint64_t *) buf) + chunk_size;
	  buf->alloc = buf->free = buf->start;
	  /* add the buffer to the free list */
	  bab_buffer_free (buf);
#if defined(HPUX) && defined(STATIC_MALLOC)
	}
#elif defined(SOLARIS) || defined(_AIX) || defined(HPUX) || defined(linux)
    }
#endif
  max_bab_buffers_in_use = num_free_bab_buffers;

  Return (bablen);
  /* NOTREACHED */
  Exit;
}

/*-
 * ftd_bab_fini()
 *
 * waste all resources we allocated at init.
 */
FTD_PUBLIC ftd_void_t
ftd_bab_fini ()
{
  bab_buffer_t *temp;

  Enterv (FTD_BAB_FINI, 0, 0, 0, 0, 0);

  /* make sure all bab_mgrs are wasted. */
  while (bab_mgrs != NULL)
    {
      if (ftd_bab_free_mgr (bab_mgrs) != 0)
	{
	  break;
	}
    }

#if defined(SOLARIS) || defined(_AIX) || (defined(HPUX) && !defined(STATIC_BAB_MALLOC)) || defined(linux)
  while (free_bab_buffers != NULL)
    {
      temp = free_bab_buffers->next;
#if defined(HPUX)
      FREE (free_bab_buffers, M_IOSYS);
#elif defined(linux)
      free_pages((unsigned long)free_bab_buffers, get_order(chunk_size * sizeof (ftd_uint64_t)));
#else
      kmem_free (free_bab_buffers, chunk_size * sizeof (ftd_uint64_t));
#endif
      free_bab_buffers = temp;
    }
#endif

  num_free_bab_buffers = 0;
  DEALLOC_LOCK (bab_lck);

  Returnv;
  /* NOTREACHED */
  Exitv;
}

/*
 * return the free BAB size
 */
ftd_int32_t
ftd_bab_free_size(void)
{
	return (num_free_bab_buffers * chunk_size * sizeof(ftd_uint64_t));
}

/*
 * put a buffer back into the head of free list
 */
FTD_PRIVATE ftd_void_t
bab_buffer_free(bab_buffer_t *buf)
{
#if defined(HPUX) || defined(_AIX) || defined(linux)
	ftd_context_t	context;
#endif

	ACQUIRE_LOCK(bab_lck, context);

	buf->next = free_bab_buffers;
	free_bab_buffers = buf;
	num_free_bab_buffers++;

	RELEASE_LOCK(bab_lck, context);

	return;
}

/*
 * allocate a buffer from the free list
 */
FTD_PRIVATE bab_buffer_t *
bab_buffer_alloc()
{
	bab_buffer_t	*buf;
#if defined(HPUX) || defined(_AIX)  || defined(linux)
	ftd_context_t	context;
#endif

	ACQUIRE_LOCK(bab_lck, context);

	if (free_bab_buffers == NULL) {
		RELEASE_LOCK(bab_lck, context);
		return (NULL);
	}

	buf = free_bab_buffers;
	free_bab_buffers = free_bab_buffers->next;
	num_free_bab_buffers--;
	buf->next = NULL;
	buf->alloc = buf->free = buf->start;

	RELEASE_LOCK(bab_lck, context);
	return (buf);
}

/*-
 * ftd_bab_alloc_mgr()
 *
 * allocate a bab manager from the array of free managers.
 */
FTD_PUBLIC bab_mgr_t *
ftd_bab_alloc_mgr ()
{
  bab_mgr_t *mgr;

  /* search thru the array looking for an unused manager */
  mgr = (bab_mgr_t *) kmem_zalloc (sizeof (bab_mgr_t), KM_NOSLEEP);
  if (mgr == NULL)
    {
      return NULL;
    }

  /* add it to the list of managers */
  mgr->next = bab_mgrs;
  bab_mgrs = mgr;

  return (mgr);
}

/*
 * free up a buffer manager
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_free_mgr(bab_mgr_t *mgr)
{
	bab_mgr_t	**temp;
	bab_buffer_t	*buf;

	/*
	 * if any buffers are in use, release forcely!
	 */
	while ((buf = mgr->in_use_head) != NULL) {
		mgr->in_use_head = buf->next;
		bab_buffer_free(buf);
	}

	/*
	 * remove the manager in the linked list
	 */
	temp = &bab_mgrs;
	while (*temp != NULL && *temp != mgr) {
		temp = &(*temp)->next;
	}
	if (*temp == NULL) {
		return (-1);
	}
	*temp = mgr->next;

	kmem_free(mgr, sizeof(bab_mgr_t));

	/*
	 * XXX: fixme.
	 * adjust the bab_mgr limits now that we have one less to deal with
	 */

	return (0);
}

/*
All write requests that comes in the NORMAL mode are stored
in the BAB temporarily until the PMDs drain it out to the
secondary or the mirror system. This is the routine that
is called when anything needs to be put in the BAB.
The input to this routine is the length of the write request
(which is the length in bytes/8). The function of the routine
is to check if the request is not too large i.e. there is 
enof BAB memory available at this time- it returns 0 on failure
and 1 on success. But the main function is - it sets the
structure members num_frags, count and from[]. 'from' points
to the address in the BAB where the write will be stored and
count is the corresponding number of bytes. depending on the
size of the request and the size of a BAB chunk, we might
need more than 1 occurance of from and count -   ftd_do_write()
reads these values and does the copyin/bcopy from the user
buffer to the BAB buffer using the from and count values --
since ftd_do_write is protected by the LG LOCK, we don't worry
about it here.
*/
/*-
 * ftd_bab_alloc_memory()
 *
 * allocate a chunk of memory from the manager.
 * len64 is in 64-bit words.
 */
FTD_PUBLIC ftd_int32_t 
ftd_bab_alloc_memory (bab_mgr_t * mgr, ftd_int32_t len64)
{
  bab_buffer_t *buf, *newbuf = NULL;
  ftd_uint64_t *ret;
  ftd_int32_t total, i, tag;
#if defined(HPUX) || defined(_AIX)
  ftd_context_t context;
#endif /* defined(HPUX) */
  bab_buffer_t *save_pending_buf;
  ftd_uint64_t *save_pending;    /* save the 2 pointers */

  /* the overhead of a chunk is the bab_buffer_t structure */
  if ((mgr->flags & FTD_BAB_DONT_USE) ||
      len64 > (chunk_size - sizeof_64bit (bab_buffer_t))* num_free_bab_buffers) {
    return 0;
  }
  mgr->num_frags = 1;
  i = 0;

/* save the pending pointers, just in case we return error after allocating a buffer */

  save_pending_buf = mgr->pending_buf;
  save_pending = mgr->pending;

  if (mgr->num_in_use == 0) {
    if ((buf = bab_buffer_alloc ()) == NULL) {
      return 0;
    }
    mgr->in_use_tail = mgr->in_use_head = buf;
    mgr->num_in_use = 1;
    mgr->flags = 0;
    buf->next = NULL;

    /* initialize the pending pointers */
    mgr->pending_buf = buf;
    mgr->pending = buf->free;
  }

  /* we always allocate from the tail buffer. */
  buf = mgr->in_use_tail;
  total = buf->end - buf->alloc;
  if (len64 <= total) {
    ret = buf->alloc;
    buf->alloc += len64;
    mgr->from[0] = ret + sizeof_64bit(wlheader_t);
    mgr->count[0] = (len64 - sizeof_64bit(wlheader_t)) * 8;
    return 1;
  }

  tag = 1;
  /* come here only if span data in 2 or more chunks */
  if (total > MIN_BAB_SPACE_REQUIRED) {
    len64 = len64 - total;
    mgr->from[i] = buf->alloc + sizeof_64bit(wlheader_t) ;
    buf->alloc = buf->alloc + total;
    mgr->count[i] = (total- sizeof_64bit(wlheader_t)) * 8;
    tag = 0;
    i++;
  }

  while (len64 > 0) {
    /* WR35600:
     * Add mgr->num_in_use == max_bab_buffers_in_use as an error case too.
     * This case is currently guarded by checking in the beginning of this
     * funtion for remaining bab space. Also add it here to make checking
     * complete.
     */
    if ((mgr->num_in_use <= max_bab_buffers_in_use) &&
        (newbuf = bab_buffer_alloc()) == NULL) {
      mgr->pending_buf = save_pending_buf;
      mgr->pending = save_pending;
  
      /* we're failing the allocation, so undo changes to buf->alloc */
      if (tag==0) 
        buf->alloc -= total;

      return 0;
    }
    if ((mgr->pending_buf == buf) && (mgr->pending == buf->alloc) && (i == 0)) {
      mgr->pending_buf = newbuf;
      mgr->pending = newbuf->free;
    }

    mgr->num_frags++;
    newbuf->next = NULL;
    buf->next = newbuf;
    mgr->in_use_tail = newbuf;
    mgr->num_in_use++;
    buf = newbuf;

    if (i == 0)
      mgr->from[i] = buf->alloc + sizeof_64bit(wlheader_t);
    else
      mgr->from[i] = buf->alloc;
      
    if (len64 <= (chunk_size - sizeof_64bit(bab_buffer_t))) {
      buf->alloc = buf->alloc + len64;
      if (i == 0)
        mgr->count[i] = (len64 - sizeof_64bit(wlheader_t)) * 8;
      else
        mgr->count[i] = len64 * 8;
      if (tag) 
          mgr->num_frags--;
      tag = 1;
      return 1;
    }
    buf->alloc = buf->alloc + (chunk_size - sizeof_64bit(bab_buffer_t));
    /*here buf->alloc should be = buf->end ! */
    if (i == 0)
      mgr->count[i] = ((chunk_size - sizeof_64bit(bab_buffer_t)) - sizeof_64bit(wlheader_t)) * 8;
    else
      mgr->count[i] = (chunk_size - sizeof_64bit(bab_buffer_t)) * 8;
    len64 = len64 - (chunk_size - sizeof_64bit(bab_buffer_t));
    i++;
  } /* end while */
  /* this part of the code is unreachable, just in case.. */
  mgr->pending_buf = save_pending_buf;
  mgr->pending = save_pending;
  return 0;
}

/*-
 * ftd_bab_free_memory()
 * 
 * free up some memory from the manager. We can free memory up to the
 * last committed memory in its buffers.
 *
 * len64 is in 64-bit words
 * returns the number of words not freed. 
 *
 * remember, the oldest data is at the head; the newest is at the tail.
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_free_memory (bab_mgr_t * mgr, ftd_int32_t len64)
{
  bab_buffer_t *temp;
  ftd_int32_t buflen;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */


#if 0
#if defined(FTD_DEBUG)
  FTD_ERR (FTD_DBGLVL, "freeing up %d words\n", len64);
#endif /* defined(FTD_DEBUG) */
#endif /* 0 */

  /* start at the head buffer and move on until we free up "len" words */
  while ((len64 > 0) && (mgr->in_use_head != NULL))
    {

      /* This is kinda ugly. We have to handle the last part of the last
       * committed buffer very carefully, because not all of the memory 
       * may be committed. Presumably, we are freeing up only the committed 
       * memory, but the caller could be a bogon and ask to do more than 
       * that. Anyway, if all of the memory in the buffer is committed, 
       * then we can it free up. 
       */

      /* if this is the last committed buffer, go no further */
      if (mgr->pending_buf == mgr->in_use_head)
	{
	  buflen = mgr->pending - mgr->in_use_head->free;

	  /* free up part of committed memory */
	  if (len64 < buflen)
	    {
	      mgr->in_use_head->free += len64;
	      len64 = 0;
	      /* if all memory is committed, waste the buffer and list. */
	    }
	  else if (mgr->pending == mgr->in_use_head->alloc)
	    {
	      /* this can happen only when this is the last buffer! */
	      bab_buffer_free (mgr->in_use_head);
	      mgr->pending = NULL;
	      mgr->pending_buf = NULL;
	      mgr->in_use_tail = NULL;
	      mgr->in_use_head = NULL;
	      mgr->num_in_use = 0;
	      len64 -= buflen;
	      /* uncommitted memory exists, so just free up the committed stuff */
	    }
	  else
	    {
	      mgr->in_use_head->free = mgr->pending;
	      len64 -= buflen;
	    }
	  break;
	}

      /* we haven't made it to the last committed buffer, yet */
      buflen = mgr->in_use_head->alloc - mgr->in_use_head->free;
      if (len64 < buflen)
	{
#if 0
#if defined(FTD_DEBUG)
	  FTD_ERR (FTD_DBGLVL, "incrementing free\n");
#endif /* defined(FTD_DEBUG) */
#endif /* 0 */
	  mgr->in_use_head->free += len64;
	  len64 = 0;
	  break;
	}
      else
	{
#if 0
#if defined(FTD_DEBUG)
	  FTD_ERR (FTD_DBGLVL, "wrapping\n");
#endif /* defined(FTD_DEBUG) */
#endif /* 0 */
	  len64 -= buflen;
	  temp = mgr->in_use_head->next;
	  bab_buffer_free (mgr->in_use_head);
	  mgr->in_use_head = temp;
	  mgr->num_in_use--;
	}
    }

  return (len64);
}

/*-
 * ftd_bab_commit_memory()
 * 
 * commit some memory from the manager. We can increment the pending pointer
 * up to the end of allocated memory.
 *
 * len64 is in 64-bit words
 * Returns the number of words not committed. 
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_commit_memory (bab_mgr_t * mgr, ftd_int32_t len64)
{
  ftd_int32_t buflen;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(FTD_DEBUG) && 0
  FTD_ERR (FTD_DBGLVL, "committing %d words\n", len64);
#endif /* defined(FTD_DEBUG) */

  /* start at the tail buffer and move on until we free up "len" bytes */
  while ((len64 > 0) && (mgr->pending_buf != NULL))
    {

      /* get the amount of uncommitted memory in this buffer */
      buflen = mgr->pending_buf->alloc - mgr->pending;

      /* commit only part of it */
      if (len64 < buflen)
	{
#if defined(FTD_DEBUG) && 0
	  FTD_ERR (FTD_DBGLVL, "incrementing pending by %d\n", buflen);
#endif /* defined(FTD_DEBUG) */
	  mgr->pending += len64;
	  len64 = 0;
	  break;
	  /* commit the rest of the buffer and presumably go to the next one */
	}
      else
	{
	  len64 -= buflen;
	  /* wrap to the next buffer */
	  if (mgr->pending_buf->next != NULL)
	    {
	      mgr->pending_buf = mgr->pending_buf->next;
	      mgr->pending = mgr->pending_buf->free;
	      /* no more buffers, all memory is now committed. */
	    }
	  else
	    {
	      mgr->pending = mgr->pending_buf->alloc;
	      break;		/* len64 should be 0 now, but bogons do exist. */
	    }
	}
    }

  return (len64);
}

/*-
 * ftd_bab_get_free_length()
 * 
 * return the number of 64-bit words available.
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_get_free_length (bab_mgr_t * mgr)
{
  ftd_int32_t len;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  len = num_free_bab_buffers * (chunk_size - sizeof_64bit (bab_buffer_t));
  if (mgr->in_use_tail != NULL)
    {
      len += mgr->in_use_tail->end - mgr->in_use_tail->alloc;
    }

  return (len);
}

/*-
 * ftd_bab_get_used_length()
 * 
 * DEBUG: not needed in the product.
 * return the exact amount of memory allocated in the buffer manager. don't
 * include wasted space. do include uncommitted space.
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_get_used_length (bab_mgr_t * mgr)
{
  ftd_int32_t len;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */
  bab_buffer_t *temp;

  len = 0;
  for (temp = mgr->in_use_head; temp != NULL; temp = temp->next)
    {
      len += temp->alloc - temp->free;
    }

  return len;
}

/*-
 * ftd_bab_get_committed_length()
 * 
 * return the exact amount of memory allocated in the buffer manager. don't
 * include wasted space and uncommitted space.
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_get_committed_length (bab_mgr_t * mgr)
{
  ftd_int32_t len;
  bab_buffer_t *temp;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  len = 0;
  for (temp = mgr->in_use_head; temp != NULL; temp = temp->next)
    {
      if (temp == mgr->pending_buf)
	{
	  len += mgr->pending - temp->free;
	  break;
	}
      else
	{
	  len += temp->alloc - temp->free;
	}
    }


  return len;
}

/*-
 * ftd_bab_copy_out()
 * 
 * returns the number of words it didn't copy.
 *
 * offset64 is the starting 64-bit word offset from the beginning.
 * len64 is the number of 64-bit words to copy
 *
 * pLock and pContext are the lock and associated context that must have already been acquired before being called.
 * We need to have access to them in order to temporarilly release the lock while copyout() is called.
 *
 * Using the lock internally here somewhat breaks the bab API, which shouldn't have to know about it,
 * but that's an easy solution to make sure that the problems documented in WR #45366 aren't seen again.
 */
FTD_PUBLIC ftd_int32_t
ftd_bab_copy_out (bab_mgr_t * mgr, ftd_int32_t offset64, ftd_uint64_t * addr, ftd_int32_t len64, ftd_lock_t *pLock, ftd_context_t* pContext)
{
  ftd_uint64_t templen; // This is the the amount of 8 byte chunks we can process within the current chunk.
  bab_buffer_t *temp;   // This is a pointer to the bab chunk we'll be copying out data from.
  ftd_uint64_t *start;  // This is the address within the above bab chunk where we'll copy out the data from.

  if (offset64 > 0)
    {
      start = ftd_bab_get_ptr (mgr, offset64, &temp);
    }
  else
    {
      temp = mgr->in_use_head;
      /* degenerate case: */
      if (temp == NULL)
	{
	  return len64;
	}
      start = temp->free;
    }

  /* porpoise thru the buffers until we get enough data to satisfy the
     request */
  while (len64 > 0)
    {
      templen = temp->alloc - start;
      if (templen > len64)
	{
	  templen = len64;
	}
      // We must release the lock before calling copyout(), as this cannot be done while holding spinlocks.
      RELEASE_LOCK ((*pLock), (*pContext));
      
#if defined(FTD_DEBUG) && 0
      FTD_ERR (FTD_DBGLVL, "   Copy out %d words\n", templen);
#endif /* defined(FTD_DEBUG) */
      copyout ((caddr_t)start, (caddr_t)addr, (size_t)(templen * 8));

      // When the lock is reacquired, a few things could have happened:
      //
      // -) Additional bab data has been allocated to store incoming IOs.
      //        This may change the value of the ->next pointer of the tail chunk previously in use if additional chunks were added.
      //        This would change the ->alloc pointer of the tail chunk previously in use and of the potentially additional chunks.
      //        We need to be careful when those are reused.
      //
      // -) Data has been committed.
      //        This would change the pending_buf and pending pointers of the mgr.
      //        We don't use them once ftd_bab_get_ptr() has been called.
      //
      // -) Bab data has been freed.
      //        This would most likely cause problems, but this can only happen through ioctl requests from the PMD, so we assume
      //        this will not happen while we're processing an FTD_OLDEST_ENTRIES ioctl.
      //        There used to be comments within ftd_lg_oldest_entries about this as well.
      ACQUIRE_LOCK ((*pLock), (*pContext));
      
      len64 -= templen;
      addr += templen;

      /* go to the next buffer */
      temp = temp->next;

      /* degenerate case: */
      if (temp == NULL)
	{
	  break;
	}
      start = temp->free;
    }

  return (len64);
}

/*-
 * ftd_bab_get_ptr()
 * 
 * seek to a specified offset in the allocated region.
 * offset64 specifies a 64-bit word offset.
 */
FTD_PRIVATE ftd_uint64_t *
ftd_bab_get_ptr (bab_mgr_t * mgr, ftd_int32_t offset64, bab_buffer_t ** buf)
{
  bab_buffer_t *temp;
  ftd_int32_t len;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if (((temp = mgr->in_use_head) != NULL) && (offset64 == 0))
    {
      *buf = temp;
      return (temp->free);
    }

  while ((temp != NULL) && (offset64 > 0))
    {
      len = temp->alloc - temp->free;
      /* if it's in this block, we're done. */
      if (len > offset64)
	{
#if defined(FTD_DEBUG) && 0
	  FTD_ERR (FTD_DBGLVL, "jumping %d words\n", offset64);
#endif /* defined(FTD_DEBUG) */
	  *buf = temp;
	  return (temp->free + offset64);
	  /* if it's at the beginning of the next block, make sure it's there */
	}
      else if (len == offset64)
	{
	  if ((temp = temp->next) != NULL)
	    {
	      *buf = temp;
	      return (temp->free);
	    }
	  /* got at least one more block to go */
	}
      else
	{
#if defined(FTD_DEBUG) && 0
	  FTD_ERR (FTD_DBGLVL, "jumping 1 buffer\n");
#endif /* defined(FTD_DEBUG) */
	  offset64 -= len;
	  temp = temp->next;
	}
    }

  /* went off the deep end */
  *buf = NULL;

  return (NULL);
}

/*-
 * ftd_bab_get_pending()
 * 
 * return a pointer to the pending pointer, if it points to valid data.
 */
FTD_PUBLIC ftd_uint64_t *
ftd_bab_get_pending (bab_mgr_t * mgr)
{
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  /* two invalid cases: no buffers or all data is committed */
  /* third case: no mgr pointer */
  /* note: there might be other cases, too. */
  if ((mgr->pending_buf == NULL) || (mgr->in_use_tail == NULL) ||
      (mgr->pending == mgr->in_use_tail->alloc))
    {
      return (NULL);
    }
  if ((mgr->pending == mgr->pending_buf->alloc) && (mgr->pending_buf->next != NULL))
   {
     mgr->pending_buf = mgr->pending_buf->next;
     mgr->pending = mgr->pending_buf->free;
     FTD_ERR(FTD_WRNLVL, "Moved the Pending to next Buf");
   }
  return (mgr->pending);
}
