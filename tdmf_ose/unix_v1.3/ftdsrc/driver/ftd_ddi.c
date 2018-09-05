/*-
 * ddi.c - DDI emulation layer for HP-UX 10.20 
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
#include "ftd_kern_ctypes.h"

#if defined(HPUX) && (SYSVERS >= 1100)
extern int ftd_debug;
#endif

#if !defined(SOLARIS)

#include "ftd_ddi.h"
#include "ftd_klog.h"

/* #define _ANSI_ARGS(x) () */
#define _ANSI_ARGS(x) x

typedef struct _ddi_state_
  {
    size_t state_size;
    ftd_int32_t num_states;
    caddr_t *states;
  }
ddi_state_t;

/* turn on tracing, until release */
#define DRV_DEBUG 1

/***************************************************************************
 * Half-assed DDI emulation routines, implemented so our code will more or 
 * less follow the structure and appearance of the Solaris driver.
 * 
 * See Solaris 2.5 man pages for more details on these functions.
 ***************************************************************************/

#define DDI_MAX_ITEMS 1024

/*-
 * ddi_soft_state_init()
 *
 * Create a table of pointers to keep track of the instances. Nothing
 * earth shattering happens here - we just allocate a table of pointers
 * for future use. Note that we ignore the nitems value and preallocate
 * DDI_MAX_ITEMS entries. This eliminates the need to reallocate the array 
 * if the driver creates more instances than were preallocated, which is 
 * supported in the Solaris code. 
 */
ftd_int32_t
ddi_soft_state_init (ftd_void_t ** state, size_t size, size_t nitems)
{
  ddi_state_t *temp;

#if defined(FTD_DEBUG) && 0
  FTD_ERR (FTD_DBGLVL, "init soft state: %d %d\n", size, nitems);
#endif /* defined(FTD_DEBUG) */
  /* always allocate the max! */

  temp = (ddi_state_t *) kmem_zalloc (sizeof (ddi_state_t), KM_SLEEP);
  if (temp == NULL)
    {
#if defined(FTD_DEBUG)
      FTD_ERR (FTD_DBGLVL, "bogus zalloc\n");
#endif /* defined(FTD_DEBUG) */
      return (EINVAL);
    }
  temp->state_size = size;
  temp->num_states = nitems = DDI_MAX_ITEMS;

  /* create a table of pointers to instances. */
  temp->states = (caddr_t *) kmem_zalloc (nitems * sizeof (caddr_t), KM_SLEEP);
  if (temp->states == NULL)
    {
      kmem_free (temp, sizeof (ddi_state_t));
#if defined(FTD_DEBUG)
      FTD_ERR (FTD_DBGLVL, "bogus zalloc\n");
#endif /* defined(FTD_DEBUG) */
      return (EINVAL);
    }

#if defined(FTD_DEBUG) && 0
  FTD_ERR (FTD_DBGLVL, "STATE: 0x%x 0x%x %d %d\n", temp,
	   temp->states, temp->num_states, temp->state_size);
#endif /* defined(FTD_DEBUG) */
  *state = (ftd_void_t *) temp;
  return (0);
}

/*-
 * ddi_soft_state_fini()
 * 
 * free up all of the unfreed states as well as the state structures
 */
ftd_void_t
ddi_soft_state_fini (ftd_void_t ** state)
{
  caddr_t *table;
  ftd_int32_t i;
  ddi_state_t *temp = (ddi_state_t *) * state;

  if (temp == NULL)
    {
      return;
    }
  table = temp->states;

  /* just in case the driver didn't free up the instances... */
  for (i = 0; i < temp->num_states; i++)
    {
      /* don't know what to do with private, since we don't know the size */
      if (table[i] != NULL)
	{
	  kmem_free (table[i], temp->state_size);
	}
    }
  kmem_free (table, temp->num_states * sizeof (caddr_t));
  kmem_free (temp, sizeof (ddi_state_t));

  *state = NULL;
  return;
}

/*-
 * ddi_get_soft_state()
 * 
 * find the structure with a matching minor number.
 */
ftd_void_t *
ddi_get_soft_state (ftd_void_t * state, ftd_int32_t item)
{
  ddi_state_t *temp = (ddi_state_t *) state;

  if ((temp == NULL) || (temp->states == NULL) ||
      (item < 0) || (item >= temp->num_states))
    {
#if defined(FTD_DEBUG)
      FTD_ERR (FTD_DBGLVL, "BAD state: 0x%x %d\n", temp, item);
#endif /* defined(FTD_DEBUG) */
      return (NULL);
    }
#if 0
#if defined(FTD_DEBUG)
  FTD_ERR (FTD_DBGLVL, "GSS: 0x%x 0x%x %d\n", temp, temp->states, item);
#endif /* defined(FTD_DEBUG) */
#endif /* 0 */
  return ((ftd_void_t *) (temp->states[item]));
}

/*-
 * ddi_soft_state_free()
 *
 * release a softp structure.
 */
ftd_void_t
ddi_soft_state_free (ftd_void_t * state, ftd_int32_t item)
{
  ddi_state_t *temp = (ddi_state_t *) state;
  caddr_t *table;

  if ((temp == NULL) || (temp->states == NULL))
    {
      return;
    }
  table = temp->states;
  if ((item >= 0) && (item < temp->num_states) && (table[item] != NULL))
    {
      kmem_free (table[item], temp->state_size);
      table[item] = NULL;
    }
  return;
}

/*-
 * ddi_soft_state_zalloc()
 *
 * allocate kmem and index for a softp struct
 */
ftd_int32_t
ddi_soft_state_zalloc (ftd_void_t * state, ftd_int32_t item)
{
  ddi_state_t *temp = (ddi_state_t *) state;
  caddr_t ptr;

  if ((temp == NULL) || (temp->states == NULL) || (item < 0) ||
      (item >= temp->num_states) || (temp->states[item] != NULL))
    {
#if defined(FTD_DEBUG)
      FTD_ERR (FTD_DBGLVL, "BAD state zalloc: 0x%x %d\n", temp, item);
#endif /* defined(FTD_DEBUG) */
      return (DDI_FAILURE);
    }

  /* call kmem_zalloc */
  if ((ptr = (caddr_t) kmem_zalloc (temp->state_size, KM_SLEEP)) == NULL)
    {
#if defined(FTD_DEBUG)
      FTD_ERR (FTD_DBGLVL, "BAD state zalloc: 0x%x %d\n", temp, item);
#endif /* defined(FTD_DEBUG) */
      return (DDI_FAILURE);
    }
  temp->states[item] = ptr;
#if defined(FTD_DEBUG) && 0
  FTD_ERR (FTD_DBGLVL, "GOOD state zalloc: 0x%x 0x%x 0x%x %d %d\n",
	   temp, temp->states, ptr, temp->state_size, item);
#endif /* defined(FTD_DEBUG) */

  return (DDI_SUCCESS);
}

#if !defined(_AIX)
/*-
 * ftd_delay_timeout()
 *
 * wakeup(9f) wrapper
 */
FTD_PRIVATE ftd_int32_t
ftd_delay_timeout (caddr_t arg)
{
  wakeup (arg);
  return (0);
}
#endif /* !defined(_AIX) */

#if defined(_AIX)

#include <sys/malloc.h>
#include <sys/m_param.h>

/*-
 * kmem_alloc()
 *
 * _AIX xmalloc wrapper
 */
ftd_void_t *
kmem_alloc (size_t size, ftd_int32_t flag)
{
  ftd_void_t *result;

  /* Must not be on an interrupt */
  assert (getpid () != -1);

  result = xmalloc (size, 0, pinned_heap);

  return (result);
}

/*-
 * kmem_zalloc()
 *
 * _AIX xmalloc/bzero wrapper
 */
ftd_void_t *
kmem_zalloc (size_t size, ftd_int32_t flag)
{
  ftd_void_t *result;

  /* Must not be on an interrupt */
  assert (getpid () != -1);

  if ((result = xmalloc (size, 0, pinned_heap)))
    bzero (result, size);

#if defined(FTD_DEBUG)
  if (result == (ftd_void_t *) 0)
    FTD_ERR (FTD_WRNLVL, "kmem_zalloc: failed to alloc 0x%08x", size);
#endif /* defined(FTD_DEBUG) */

  return (result);
}

/*-
 * kmem_free()
 *
 * _AIX xmfree wrapper
 */
kmem_free (ftd_void_t * addr, size_t size)
{
  xmfree (addr, pinned_heap);
}

/*-
 * drv_usedtohz()
 *
 * _AIX drv_usectohz replacement
 */
clock_t
drv_usectohz (clock_t usec)
{
  return (usec / _CLKTICK);
}

/*-
 * ftd_minphys()
 * 
 * for _AIX: minphys() is documented as mincnt():
 * The mincnt parameter cannot modify any other fields  without  the
 * risk  of  error.  If the mincnt parameter determines that the buf
 * header cannot be supported by  the  target  device,  the  routine
 * should  return  a  nonzero return code. This stops the buf header
 * and any additional buf headers from being sent to the  ddstrategy
 * routine.
 */
ftd_int32_t
ftd_minphys (struct buf * bp, ftd_void_t * arg)
{
  return 0;
}

#endif /* defined(_AIX) */

#if defined(HPUX)

/*-
 * ftd_delay()
 *
 * delay execution for a specified number of clock ticks. Use mp_timeout to
 * schedule the event. sleep() waits for the event to occur.
 */
ftd_void_t
ftd_delay (ftd_int32_t ticks, caddr_t addr)
{
  mp_timeout (ftd_delay_timeout, addr, ticks);

  sleep (addr, (PZERO + 2) | PCATCH);
  untimeout (ftd_delay_timeout, 0);

  return;
}

/*-
 * drv_usedtohz()
 *
 * HPUX drv_usectohz replacement
 */
clock_t
drv_usectohz (clock_t usec)
{
  return (usec / USEC_PER_CLK_TCK);
}

/* XXX deprecate me? */
#if defined(notdef)
/*-
 * ftd_drv_getparm()
 * 
 * Emulation routine. Only supports two param types - LBOLT (number of clock
 * ticks since last system reboot), and TIME (number of seconds since the
 * Jan 1, 1970). 
 */
ftd_int32_t
ftd_drv_getparm (ftd_uint32_t param, ftd_uint32_t * val)
{
  if (param == LBOLT)
    {
      *val = ticks_since_boot;
    }
  else if (param == TIME)
    {
      *val = time.tv_sec;
    }
  else
    {
      return (-1);
    }
  return (0);
}
#endif /* defined(notdef) */

/*-
 * kmem_zalloc()
 *
 * HP-UX kmem_zalloc replacement.
 */
ftd_void_t *
kmem_zalloc (size_t size, ftd_int32_t flag)
{
  caddr_t ret;

  /* TWS:  change this to sys_memall() */
  /* ret = (caddr_t)sys_memall(size); */
  MALLOC (ret, caddr_t, size, M_IOSYS, flag);

  if (ret == NULL)
    {
      return (NULL);
    }

  bzero (ret, size);
  return ((ftd_void_t *) ret);
}

#endif /* defined(HPUX) */

/*-
 * bioerror()
 *
 * bioerror(9f) replacement.
 */
ftd_void_t
bioerror (struct buf * bp, ftd_int32_t error)
{
  if (error != 0)
    {
      bp->b_flags |= B_ERROR;
    }
  else
    {
      bp->b_flags &= ~B_ERROR;
    }
  bp->b_error = error;
}

#endif /* defined(HPUX) || defined(_AIX) */
