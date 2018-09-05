#if defined(_AIX)

/*-
 * like many things with _AIX kernel extensions,
 * need to help things along writing code that 
 * isn't needed for other OSs. 
 *
 * this module implements callout queue timers 
 * for use as synchronous mode watchdog timers.
 *
 * made attempts where possible to make the 
 * interfaces conform to timeout(9f) and untimeout(9f).
 * with _AIX, timeout(9f) and untimeout(9f) aren't
 * MP_SAFE. rather, they recommend talloc() and 
 * friends. these timers must be preallocated by
 * the driver in process context, say, at init time.
 */

#include "ftd_kern_ctypes.h"
#include "ftd_timers.h"
#include "ftd_kern_cproto.h"

/*-
 * module private state
 */
FTD_PRIVATE ftd_int32_t trbblk_inited = 0;	/* whether subsystem ready */
#if defined(DBG_TIMERTAB)
trbblk_t trbblk_tab[NTRBSLOTS];	/* table of trb slots */
#else /*  defined(DBG_TIMERTAB) */
FTD_PRIVATE trbblk_t *trbblk_tab = (trbblk_t *) 0;	/* table of trb slots */
#endif /*  defined(DBG_TIMERTAB) */
FTD_PRIVATE Simple_lock trbblk_lock;	/* table access lock */

/*-
 * init a block of trb's
 */
ftd_int32_t
ftd_timer_init ()
{
  ftd_int32_t i;

  /* whether inited */
  if (trbblk_inited)
    {
      FTD_ERR (FTD_WRNLVL, "ftd_timer_init: already inited.");
      return (-1);
    }

	/*-
	 * allocate a resource lock 
	 */
  ALLOC_LOCK (trbblk_lock, "ftd_timer lock");

	/*-
	 * allocate table slots
	 */
#if !defined(DBG_TIMERTAB)
  trbblk_tab = (trbblk_t *) kmem_zalloc (TRBMEMSIZ, KM_NOSLEEP);
  if (trbblk_tab == (trbblk_t *) 0)
    {
      DEALLOC_LOCK (trbblk_lock);
      FTD_ERR (FTD_WRNLVL, "ftd_timer_init: kmem_zalloc() failed.");
      return (-1);
    }
#endif /* !defined(DBG_TIMERTAB) */

	/*-
	 * init block slots
	 */
  for (i = 0; i < NTRBSLOTS; i++)
    {

      /* i be available */
      trbblk_tab[i].trb_stat = TRBSLOT_FREE;

      /* allocate a timer request block for each slot */
      trbblk_tab[i].trb_trbp = talloc ();
      if (trbblk_tab[i].trb_trbp == (struct trb *) 0)
	{
	  DEALLOC_LOCK (trbblk_lock);
#if !defined(DBG_TIMERTAB)
	  kmem_free (trbblk_tab, TRBMEMSIZ);
	  trbblk_tab = (trbblk_t *) 0;
#endif /* !defined(DBG_TIMERTAB) */
	  FTD_ERR (FTD_WRNLVL, "ftd_timer_init: talloc() failed.");
	  return (-1);
	}

      /* init callback func parms */
      trbblk_tab[i].trb_func = (ftd_void_t (*)())0;
      trbblk_tab[i].trb_arg = (caddr_t) 0;
#if defined(DBG_TIMERTAB)
      trbblk_tab[i].trb_nxt = &(trbblk_tab[i + 1]);
#endif /* defined(DBG_TIMERTAB) */

    }
#if defined(DBG_TIMERTAB)
  trbblk_tab[NTRBSLOTS - 1].trb_nxt = (trbblk_t *) 0;
#endif /* defined(DBG_TIMERTAB) */
  trbblk_inited = 1;

  return (0);
}

/*-
 * finish a block of timer request structs
 */
ftd_int32_t
ftd_timer_fini ()
{
  ftd_int32_t i;

  /* whether inited */
  if (!trbblk_inited)
    {
      FTD_ERR (FTD_WRNLVL, "ftd_timer_fini: not inited.");
      return (-1);
    }

  /* free timer request blocks */
  for (i = 0; i < NTRBSLOTS; i++)
    {

		/*-
		 * if this timer is allocated and running, 
		 * stop it and perform any specified callback.
		 */
      if ((trbblk_tab[i].trb_stat == TRBSLOT_ALLOC) &&
	  (trbblk_tab[i].trb_trbp->flags & T_ACTIVE))
	{
	  tstop (trbblk_tab[i].trb_trbp);
	  tfree (trbblk_tab[i].trb_trbp);
	  (*trbblk_tab[i].trb_func) (trbblk_tab[i].trb_arg);
	}
      else
	{
	  /* otherwise just give it up */
	  tfree (trbblk_tab[i].trb_trbp);
	}

    }

  /* release the slot table */
#if !defined(DBG_TIMERTAB)
  kmem_free (trbblk_tab, TRBMEMSIZ);
  trbblk_tab = (trbblk_t *) 0;
#endif /* !defined(DBG_TIMERTAB) */

  /* release the lock */
  DEALLOC_LOCK (trbblk_lock);

  trbblk_inited = 0;

  return (0);
}

/*-
 * allocate a timer request block
 * yeah, this is a linear table lookup.
 * this is also synch mode, still
 * worried about performance?
 */
struct trb *
ftd_timer_alloc (func, arg)
ftd_void_t (*func) ();
     caddr_t arg;
{
  ftd_int32_t i;
  ftd_context_t context;
  ftd_int32_t wrap;
  static ftd_int32_t lstndx = NTRBSLOTS - 1;
  struct trb *trbp = (struct trb *) 0;

	/*-
	 * find any first free slot.
	 *
	 * can't block, so make one and only one pass.
	 * failing this, recovery is to perform the
	 * indicated callback function, with its argument.
	 */

  /* whether inited */
  if (!trbblk_inited)
    {
      FTD_ERR (FTD_WRNLVL, "ftd_timer_alloc: not inited.");
      return ((struct trb *) 0);
    }

  ACQUIRE_LOCK (trbblk_lock, context);

  /* find a free slot  */
  wrap = 0;
  i = lstndx;
  do
    {

      /* loop control */
      i = ++i % NTRBSLOTS;
      if (wrap == NTRBSLOTS)
	{

	  /* pass complete, fail */
	  FTD_ERR (FTD_WRNLVL,
	    "ftd_timer_alloc: couldn't allocate a timer request block.");

	  RELEASE_LOCK (trbblk_lock, context);

	  return (trbp);
	}

      /* if this slot is free, grab it */
      if (trbblk_tab[i].trb_stat == TRBSLOT_FREE)
	{
	  trbblk_tab[i].trb_stat = TRBSLOT_ALLOC;
	  trbblk_tab[i].trb_func = func;
	  trbblk_tab[i].trb_arg = arg;
	  trbp = trbblk_tab[i].trb_trbp;
	  lstndx = i;
	  break;
	}
      wrap++;

    }
  while (1);

  RELEASE_LOCK (trbblk_lock, context);

#if defined(DBG_TIMERTAB) && 0
  FTD_ERR (FTD_WRNLVL, "ftd_timer_alloc: returning 0x%08x", trbp);
#endif /* defined(DBG_TIMERTAB) */

  return (trbp);

}

/*-
 * release a timer request block
 * yeah, this is a linear table lookup.
 * this is also synch mode, still
 * worried about performance?
 */
ftd_int32_t
ftd_timer_rele (trbp)
     struct trb *trbp;
{
  ftd_context_t context;
  ftd_int32_t i = 0;
  ftd_int32_t freed = 0;

  if (!trbblk_inited)
    {
      FTD_ERR (FTD_WRNLVL, "ftd_timer_rele: not inited.");
      return (-1);
    }

#if defined(DBG_TIMERTAB) && 0
  FTD_ERR (FTD_WRNLVL, "ftd_timer_rele: releasing 0x%08x", trbp);
#endif /* defined(DBG_TIMERTAB) */

  ACQUIRE_LOCK (trbblk_lock, context);

  /* find the slot this trb was allocated from... */
  for (i = 0; i < NTRBSLOTS; i++)
    {

      /* ...and mark it free */
      if (trbblk_tab[i].trb_trbp == trbp)
	{
	  if (trbblk_tab[i].trb_stat == TRBSLOT_FREE)
	    {
	      FTD_ERR (FTD_WRNLVL,
		       "ftd_timer_rele: freeing free trb: 0x%08x", trbp);
	      panic ("ftd_timer_rele(): freeing free trb.");
	    }
	  trbblk_tab[i].trb_stat = TRBSLOT_FREE;
	  trbblk_tab[i].trb_func = (ftd_void_t (*)())0;
	  trbblk_tab[i].trb_arg = (caddr_t) 0;
	  freed++;
	  break;
	}
    }

  RELEASE_LOCK (trbblk_lock, context);

  if (!freed)
    {
      FTD_ERR (FTD_WRNLVL,
	       "ftd_timer_rele: couldn't release timer request block for timer 0x%08x", trbp);
      return (-1);
    }
  return (0);

}

/*-
 * ftd_timeout
 * roughly made equivalent to behavior of timeout(9f).
 */
struct trb *
ftd_timeout (func, arg, ticks)
ftd_void_t (*func) ();
     caddr_t arg;
     ftd_int32_t ticks;
{
  struct timestruc_t ts;
  struct itimerstruc_t its;

  struct trb *trbp;

  /* allocate a timer request block ... */
  trbp = ftd_timer_alloc (func, arg);
  if (trbp == (struct trb *) 0)
    {
      FTD_ERR (FTD_WRNLVL,
	       "ftd_timeout: ftd_timer_alloc() failed.");

		/*-
 	 	 * panicked when called during failure to allocate 
 	 	 * a timer block. in this case, trbp will be nil
 	 	 * and we croak here. just check for this case,
 	 	 * and do what ftd_synctimo() would do.
 	 	 * (for _AIX, ftd_synctimo()'s argument is 
 	 	 * (struct trb *), not (struct wlheader_s *)).
 	 	 */
      {
	struct wlheader_s *hp = (struct wlheader_s *) trbp->t_func_addr;
	struct buf *bp;

	if (hp && (bp = hp->bp))
	  ftd_biodone (bp);
      }

      return;
    }

  /* ... setup misc trb field ... */
  trbp->flags = T_INCINTERVAL | T_LOWRES;
  trbp->id = -1;
  trbp->timerid = -1;
  trbp->eventlist = -1;
  trbp->ipri = INTTIMER;

  /* ... and the trb callback setups ... */
  trbp->func = (ftd_void_t (*)(ftd_void_t *)) func;
  trbp->t_func_addr = (caddr_t) arg;


  /* ...and the trb callback relative timerval timeout... */
  ts.tv_sec = ticks / HZ;
  ts.tv_nsec = 0;
  its.it_interval = ts;
  its.it_value = ts;
  trbp->timeout = its;

  /* ...then start the timer. */
  tstart (trbp);

  return (trbp);

}

/*-
 * ftd_aixuntimeout
 * roughly made equivalent to behavior of untimeout(9f).
 */
ftd_void_t
ftd_aixuntimeout (trbp)
     struct trb *trbp;
{

  /* if running, stop the timer... */
  if (trbp->flags & T_ACTIVE)
    tstop (trbp);

  /* ...and free the trb. */
  ftd_timer_rele (trbp);

  return;
}

#endif /* defined(_AIX) */

#if defined(HPUX)

#include "ftd_kern_ctypes.h"
#include <limits.h>
#include "ftd_timers.h"
#include "ftd_kern_cproto.h"

#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_var.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftdio.h"

/* global variables */
extern struct ftd_ctl *ftd_global_state;
extern ftd_int32_t ftd_debug;

ftd_int32_t timer = 1 * HZ;	/* hard coded timer value - for now */
FTD_PRIVATE ftd_int32_t disarmTimer = 0;	/* flag to disarm timer */

ftd_void_t
ftd_timer_enqueue (ftd_lg_t * lgp, wlheader_t * wl)
{
  wl->syncTimeComplete = lbolt;	/* time stamp entry */
  if (!lgp->syncHead && !lgp->syncTail)
    {				/* 1st entry */
      lgp->syncHead = wl;
      lgp->syncTail = wl;
      wl->syncForw = wl->syncBack = NULL;
    }
  else
    {				/* all other entry is appended to syncTail */
      lgp->syncTail->syncForw = wl;
      wl->syncForw = NULL;
      wl->syncBack = lgp->syncTail;
      lgp->syncTail = wl;
    }

}				/* ftd_timer_enqueue */

ftd_void_t
ftd_timer_dequeue (ftd_lg_t * lgp, wlheader_t * wl)
{
  wlheader_t *tmp;

  if (!lgp->syncHead)
    {
      FTD_ERR (FTD_WRNLVL, "ftd_timer_dequeue: no head.");
      return;
    }

  if (wl->syncForw)
    {
      if (wl->syncBack)
	{			/* this must be a lg_migrate call */
          tmp = (wlheader_t *)wl->syncBack;
          tmp->syncForw = (wlheader_t *)wl->syncForw;
          tmp = (wlheader_t *)wl->syncForw;
          tmp->syncBack = (wlheader_t *)wl->syncBack;
	}
      else
	{			/* this entry must be at the head */
	  lgp->syncHead = wl->syncForw;
          tmp = (wlheader_t *)wl->syncForw;
          tmp->syncBack = (wlheader_t *)NULL;
	}
    }
  else
    {				/* must be the tail entry */
      if (wl->syncBack)
	{
	  lgp->syncTail = wl->syncBack;
          tmp = (wlheader_t *)wl->syncBack;
          tmp->syncForw = (wlheader_t *)NULL;
	}
      else
	{			/* this entry must be the last entry */
	  lgp->syncHead = lgp->syncTail = NULL;
	  wl->syncForw = wl->syncBack = NULL;
	}
    }

}				/* ftd_timer_dequeue */


/*-
 * Walk each logical group and biodone entries that have expired
 */
ftd_void_t
ftd_finish_sync_io ()
{
  ftd_lg_t *lgp;
  wlheader_t *wl;
  ftd_context_t context;
  ftd_int32_t curTick = lbolt;	/* get current time tick */
  ftd_int32_t deltaTm;
  struct buf *sv_ubp;

  ftd_int32_t cnt = 0;		/* use for debug */

  for (lgp = ftd_global_state->lghead; lgp; lgp = lgp->next)
    {
      ACQUIRE_LOCK (lgp->lock, context);

      /* sync mode ?? */
      if (lgp->sync_timeout <= 0)
	{
	  RELEASE_LOCK (lgp->lock, context);
	  continue;
	}

      /* entries waiting to expire */
      while (lgp->syncHead)
	{
	  wl = lgp->syncHead;
	  if (lbolt < wl->syncTimeComplete)
	    deltaTm = (ftd_int32_t) ((INT_MAX - wl->syncTimeComplete + lbolt) / HZ);
	  else
	    deltaTm = (ftd_int32_t) ((lbolt - wl->syncTimeComplete) / HZ);
	  if (deltaTm >= lgp->sync_timeout)
	    {
	      sv_ubp = wl->bp;
	      wl->bp = (struct buf *) 0;
	      if (sv_ubp)
		{
#ifdef DBG_TIMER
		  FTD_ERR (FTD_WRNLVL, "ftd_finish_sync_io: ftd_biodone %d %d.", ++cnt, deltaTm);
#endif
		  if (ftd_debug == 5)
		     printf("ftd_finish_sync_io: ftd_biodone %d %d.", ++cnt, deltaTm);
		  ftd_biodone (sv_ubp);
		}
#ifdef DBG_TIMER
	      FTD_ERR (FTD_WRNLVL, "ftd_finish_sync_io: ftd_timer_dequeue %d.", cnt);
#endif
              if (ftd_debug == 5)
		     printf("ftd_finish_sync_io: ftd_timer_dequeue %d.", cnt);
	      ftd_timer_dequeue (lgp, wl);
	    }
	  else
	    {			/* we're done walking this logical group */
	      break;
	    }
	}			/* end while */
      RELEASE_LOCK (lgp->lock, context);
    }				/* end for lgp */

  /* re-arm ftd_timer */
  if (!disarmTimer)
	ftd_timer (timer);

}				/* ftd_finish_sync_io */

/*-
 * Timer function for syncronous mode.
 * This function initiates a timeout of X seconds on the 1st instance, it
 * re-arm itself.  On subsequence timeout, if in sync mode, it will
 * call function to walk the BAB and finish_io.
 */
ftd_void_t
ftd_timer (ftd_int32_t tm)
{
  /* re-arm timer */
  mp_timeout (ftd_finish_sync_io, (caddr_t) 0, timer);

}				/* ftd_timer */

/*
 * function to set disarm flag
 */
ftd_void_t
ftd_disarm_timer (ftd_void_t)
{
  disarmTimer = 1;
  untimeout(ftd_finish_sync_io, NULL);
}

#endif /* HPUX */
