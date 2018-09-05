/*
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

#include "ftd_ddi.h"
#include "ftd_klog.h"

/* #define _ANSI_ARGS(x) () */
#define _ANSI_ARGS(x) x

typedef struct _ddi_state_ {
    size_t  state_size;
    int     num_states;
    caddr_t *states;
} ddi_state_t;

/* turn on tracing, until release */
#define DRV_DEBUG 1 

/***************************************************************************
 * Half-assed DDI emulation routines, implemented so our code will more or 
 * less follow the structure and appearance of the Solaris driver.
 * 
 * See Solaris 2.5 man pages for more details on these functions.
 ***************************************************************************/


#define DDI_MAX_ITEMS 1024

/*
 * Create a table of pointers to keep track of the instances. Nothing
 * earth shattering happens here - we just allocate a table of pointers
 * for future use. Note that we ignore the nitems value and preallocate
 * DDI_MAX_ITEMS entries. This eliminates the need to reallocate the array 
 * if the driver creates more instances than were preallocated, which is 
 * supported in the Solaris code. 
 */
int
ddi_soft_state_init(void **state, size_t size, size_t nitems)
{
    ddi_state_t *temp;

    IN_FCT(ddi_soft_state_init)

    /* always allocate the max! */

    temp = (ddi_state_t *)kmem_zalloc(sizeof(ddi_state_t), KM_SLEEP);
    if (temp == NULL) 
    {
#if defined(FTD_DEBUG)
        FTD_ERR(FTD_DBGLVL, "bogus zalloc\n");
#endif /* defined(FTD_DEBUG) */
        OUT_FCT(ddi_soft_state_init)
        return EINVAL;
    }
    temp->state_size = size;
    temp->num_states = nitems = DDI_MAX_ITEMS;

    /* create a table of pointers to instances. */
    temp->states = (caddr_t *)kmem_zalloc(nitems*sizeof(caddr_t), KM_SLEEP);
    if (temp->states == NULL) 
    {
        kmem_free(temp, sizeof(ddi_state_t));
#if defined(FTD_DEBUG)
        FTD_ERR(FTD_DBGLVL, "bogus zalloc\n");
#endif /* defined(FTD_DEBUG) */
        OUT_FCT(ddi_soft_state_init)
        return EINVAL;
    }

    *state = (void *)temp;
    OUT_FCT(ddi_soft_state_init)
    return 0;
}

/*
 * free up all of the unfreed states as well as the state structures
 */
void
ddi_soft_state_fini(void **state)
{
    caddr_t     *table;
    int         i;
    ddi_state_t *temp = (ddi_state_t *)*state;
    
    IN_FCT(ddi_soft_state_fini)

    if (temp == NULL) 
    {
        OUT_FCT(ddi_soft_state_fini)
        return;
    }
    table = temp->states;

    /* just in case the driver didn't free up the instances... */
    for (i = 0; i < temp->num_states; i++) 
    {
        /* don't know what to do with private, since we don't know the size */
        if (table[i] != NULL) 
        {
            kmem_free(table[i], temp->state_size);
        }
    }
    kmem_free(table, temp->num_states*sizeof(caddr_t));
    kmem_free(temp, sizeof(ddi_state_t));

    *state = NULL;
    OUT_FCT(ddi_soft_state_fini)
    return;
}

/*
 * find the structure with a matching minor number.
 */
void *
ddi_get_soft_state(void *state, int item)
{
    ddi_state_t *temp = (ddi_state_t *)state;
    void * RetValue;

    IN_FCT(ddi_get_soft_state)

    if ((temp == NULL) || (temp->states == NULL) || 
        (item < 0) || (item >= temp->num_states)) 
    {
#if defined(FTD_DEBUG)
        FTD_ERR(FTD_DBGLVL,"BAD state: 0x%x %d\n", temp, item);
#endif /* defined(FTD_DEBUG) */
        OUT_FCT(ddi_get_soft_state)
        return NULL;
    }

    RetValue = (void *)(temp->states[item]);
    OUT_FCT(ddi_get_soft_state)
    return (RetValue);
}

void
ddi_soft_state_free(void *state, int item)
{
    ddi_state_t *temp = (ddi_state_t *)state;
    caddr_t *table;

    IN_FCT(ddi_soft_state_free)
        
    if ((temp == NULL) || (temp->states == NULL)) 
    {
        OUT_FCT(ddi_soft_state_free)
        return;
    }
    table = temp->states;
    if ((item >= 0) && (item < temp->num_states) && (table[item] != NULL)) 
    {
        table[item] = NULL;
    }

    OUT_FCT(ddi_soft_state_free)
    return;
}

/*
 *
 */
int
ddi_soft_state_zalloc(void *state, int item, void *ptr)
{
    ddi_state_t *temp = (ddi_state_t *)state;

    IN_FCT(ddi_soft_state_zalloc)

    if ((temp == NULL) || (temp->states == NULL) || (item < 0) || 
        (item >= temp->num_states) || (temp->states[item] != NULL)) 
    {
#if defined(FTD_DEBUG)
        FTD_ERR(FTD_DBGLVL, "BAD state zalloc: 0x%x %d\n", temp, item);
#endif /* defined(FTD_DEBUG) */
        OUT_FCT(ddi_soft_state_zalloc)
        return DDI_FAILURE;
    }

    temp->states[item] = ptr;
#if defined(FTD_DEBUG)
        FTD_ERR(FTD_DBGLVL, "GOOD state zalloc: 0x%x 0x%x 0x%x %d %d\n", 
                temp, temp->states, ptr, temp->state_size, item); 
#endif /* defined(FTD_DEBUG) */

    OUT_FCT(ddi_soft_state_zalloc)
    return DDI_SUCCESS;
}

int
ddi_get_free_soft_state(void *state)
{
    ddi_state_t *temp = (ddi_state_t *)state;
    caddr_t *table;
    int item = 0;

    IN_FCT(ddi_get_free_soft_state)

    if ((temp == NULL) || (temp->states == NULL)) 
    {
        OUT_FCT(ddi_get_free_soft_state)
        return -1;
    }
    table = temp->states;
    while ( !(table[item] == NULL) && (item < temp->num_states) ) 
    {
        item++;
    }

    if (item == temp->num_states) 
    {
        OUT_FCT(ddi_get_free_soft_state)
        return -1;
    }

    OUT_FCT(ddi_get_free_soft_state)
    return item;
}

void *kmem_alloc(size_t size, int flag)
{
    void *result;

#if _DEBUG
    FTD_CONTEXT currentIrql = KeGetCurrentIrql();

    IN_FCT(kmem_alloc)

    ASSERT(currentIrql <= DISPATCH_LEVEL);
#else
    IN_FCT(kmem_alloc)
#endif

    result = ExAllocatePool(NonPagedPool, size);

    OUT_FCT(kmem_alloc)
    return result;
}

void *kmem_zalloc(size_t size, int flag)
{
    void *result;
#if _DEBUG
    FTD_CONTEXT currentIrql;

    IN_FCT(kmem_zalloc)
    currentIrql = KeGetCurrentIrql();
    ASSERT(currentIrql <= DISPATCH_LEVEL);
#else
    IN_FCT(kmem_zalloc)
#endif

    if ((result = ExAllocatePool(NonPagedPool, size))) 
    {
        bzero(result, size);
    }

    OUT_FCT(kmem_zalloc)
    return result;
}

void kmem_free(void *addr, size_t size)
{
#if _DEBUG
    FTD_CONTEXT currentIrql;

    IN_FCT(kmem_free)
    currentIrql = KeGetCurrentIrql();
    ASSERT(currentIrql <= DISPATCH_LEVEL);
#else
    IN_FCT(kmem_free)
#endif

    ExFreePool(addr);

    OUT_FCT(kmem_free)
}

int copyout(void *Source, void *Destination, unsigned long Length) 
{
    IN_FCT(copyout)
    RtlCopyMemory(Destination,Source,Length); 
    OUT_FCT(copyout)
    return 0;
} 

int copyin(void *Source, void *Destination, unsigned long Length) 
{
    IN_FCT(copyin)
    RtlCopyMemory(Destination,Source,Length);
    OUT_FCT(copyin)
    return 0;
} 

/*
 * delay execution for a specified number of clock ticks. Use mp_timeout to
 * schedule the event. sleep() waits for the event to occur.
 */
void
delay (long mSecs)
{
    LARGE_INTEGER delayValue ;
    LARGE_INTEGER delayTrue ;
    IN_FCT(delay) 

    delayValue.QuadPart = 10 * 1000 * mSecs ; // units of 100 nanoseconds
    delayTrue.QuadPart = -(delayValue.QuadPart) ; // relative
   
    KeDelayExecutionThread (KernelMode, FALSE, &delayTrue);
    
    OUT_FCT(delay)
    return;
}


/*************************************************************************
 *      END OF DDI EMULATION 
 *************************************************************************/
