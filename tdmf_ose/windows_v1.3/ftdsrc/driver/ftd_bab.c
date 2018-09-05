/*
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

#include "ftd_ddi.h" 
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_klog.h"

/* size of each memory chunk */
FTD_PRIVATE int          chunk_size;

/* maximum number of buffers any single manager can have in use */
FTD_PRIVATE int          max_bab_buffers_in_use; 

/* the list of free buffers */
FTD_PRIVATE int          num_free_bab_buffers;
FTD_PRIVATE bab_buffer_t *free_bab_buffers;

#ifndef _ERESOURCE_SPINS_
FTD_PRIVATE KSPIN_LOCK   bab_buffer_lock;
#else
FTD_PRIVATE ERESOURCE    bab_buffer_lock;
#endif

FTD_PRIVATE void         bab_buffer_free _ANSI_ARGS((bab_buffer_t *buf));
FTD_PRIVATE bab_buffer_t *bab_buffer_alloc _ANSI_ARGS((void));

//
// PROBLEM!
//
// We use exallocatepool to allocate chunks and the headers for them
// but kmem_free to free them...
// It really would be nice to see use of the same type of function on both sides...
// i.e. kmem_alloc/kmem_free OR exallocatepool/exfreepool
//

/*
 * Called at driver install time.
 *    size: size of each chunk in bytes
 *    num:  number of chunks 
 */
unsigned int
ftd_bab_init(int size, int num, ULONG maxmem)
{
    int             i;
    bab_buffer_t    *buf = NULL;
    unsigned int    bablen;
    int             j;
    int             giveback;
    ULONG           physicalAddress = 1024 * 1024 * maxmem;

    IN_FCT(ftd_bab_init)

    //
    // Translate the size we got into 64bit sizes (b-cause of UNIX)
    //
    /* create the buffers and add them to the free list */
    chunk_size = size / sizeof(ftd_uint64_t);

    ALLOC_LOCK(bab_buffer_lock, "Dtc bab specific lock");

    //
    // We have no chunks allocated
    //
    num_free_bab_buffers = 0;

    //
    // we have no available chunks
    //
    free_bab_buffers = NULL;

    bablen = 0;

    /* do a bunch of mallocs */
    for (i = 0; i < num; i++) 
    {
        //
        // Each chunk gets a header allocated in non paged pool
        //
        buf = (bab_buffer_t *) ExAllocatePool(NonPagedPool, sizeof(bab_buffer_t));
    
        if (!buf)
        {
            //
            // We cannot allocate the header of the buffer! We have a problem!
            // Don't even try to allocate the buffer...
            // 
            FTD_ERR(FTD_WRNLVL, "BAB problem - could not allocate header\n");
            break;
        }
        else
        {
            if (maxmem) 
            {
                buf->start = (ftd_uint64_t *) physicalAddress;

                physicalAddress += chunk_size * sizeof(ftd_uint64_t);
            } 
            else 
            {
            //
            // The chunk itself is actually allocated in paged pool!!
            //
                buf->start = (ftd_uint64_t *) ExAllocatePool(PagedPool, chunk_size * sizeof(ftd_uint64_t));
            }
        }

        //
        // Whoa something did not allocate!
        //
        if (NULL == buf->start) 
        {
            //
            // free header!
            //
            kmem_free(buf, sizeof(bab_buffer_t));
            FTD_ERR(FTD_WRNLVL, "BAB problem - Unable to allocate all requested BAB memory\n");
            break;
        }

        //
        // The size of the current chunk is added to the BAB's length
        //
        bablen += chunk_size * sizeof(ftd_uint64_t);

        //
        // The buffers end is the buffers's start + the chunk size! DOH!
        //
        buf->end = buf->start + chunk_size;

        //
        // the allocated part points to the next free area of memory in the current chunk
        // of course since this is empty, everything points to the begining of the chunk!
        //
        buf->alloc = buf->free = buf->start;

        //
        // This does some extra initialization
        //
        /* add the buffer to the free list */
        bab_buffer_free(buf);
    }

    //
    // num_free_bab_buffers is ++'d in bab_buffer_free
    //
    max_bab_buffers_in_use = num_free_bab_buffers;

    OUT_FCT(ftd_bab_init)

    return (bablen);
}

/*
 * Waste all resources we allocated at init.
 */
void
ftd_bab_fini(ULONG maxmem)
{
    FTD_IRQL    oldIrql;
    bab_buffer_t *temp;

    PREACQUIRE_LOCK

    IN_FCT(ftd_bab_fini)

    ACQUIRE_LOCK(bab_buffer_lock, oldIrql); 

    //
    // Get rid of all the buffer headers...
    //
    while (free_bab_buffers != NULL) 
    {
        temp = free_bab_buffers->next;

        //
        // Release the chunk... 
        //
        if (!maxmem)
            kmem_free(free_bab_buffers->start, chunk_size * sizeof(ftd_uint64_t));

        //
        // Release the header
        //
        kmem_free(free_bab_buffers, sizeof(bab_buffer_t));

        //
        // set pointer to next buffer 
        //
        free_bab_buffers = temp;
    }

    //
    // No more buffers!
    //
    num_free_bab_buffers = 0;

    RELEASE_LOCK(bab_buffer_lock, oldIrql);
    DEALLOC_LOCK(bab_buffer_lock);

    OUT_FCT(ftd_bab_fini)

    return;
}
/*
 * Put a buffer back into the free list
 */
FTD_PRIVATE void
bab_buffer_free(bab_buffer_t *buf)
{
    FTD_IRQL    oldIrql;
    PREACQUIRE_LOCK

    IN_FCT(bab_buffer_free)

    ACQUIRE_LOCK(bab_buffer_lock, oldIrql); 
    
    //
    // We add the chunks to the free_bab_buffers in reverse..
    // First we set the next to the previous buffer
    // (in the first case, this is NULL, so the last buffer will have ->next to NULL)
    //
    buf->next = free_bab_buffers;
    //
    // We set the free_bab_buffers to the last allocated buffer!
    // Or to the last free'd buffer
    // This means that the same buffer is going to be continuasly re-used if it is not filled completly...
    // which is cool
    //
    free_bab_buffers = buf;
    //
    // We add this to the list of available buffers
    //
    num_free_bab_buffers++;

    RELEASE_LOCK(bab_buffer_lock, oldIrql);

    OUT_FCT(bab_buffer_free)
}

/*
 * Allocate a buffer from the free list
 */
FTD_PRIVATE bab_buffer_t *
bab_buffer_alloc()
{
    FTD_IRQL    oldIrql;
    bab_buffer_t *buf;

    PREACQUIRE_LOCK

    IN_FCT(bab_buffer_alloc)

    ACQUIRE_LOCK(bab_buffer_lock, oldIrql); 

    //
    // free_bab_buffers is the pointer to the list of chunks available
    //
    if (free_bab_buffers != NULL) 
    {
        //
        // Get next available chunk 
        //
        buf = free_bab_buffers;
        //
        // set free_bab_buffers to point to the next available chunk
        //
        free_bab_buffers = free_bab_buffers->next;

        //
        // The chunk we give back will not know about the other available chunks!
        //
        buf->next = NULL;

        //
        // We have one less chunk available
        //
        num_free_bab_buffers--;

        //
        // Set everything to point to the begining of the chunk
        //
        buf->alloc = buf->free = buf->start;

        RELEASE_LOCK(bab_buffer_lock, oldIrql);
    
        OUT_FCT(bab_buffer_alloc)

        //
        // there is your chunk! Ready for use!
        //
        return buf;
    }

    RELEASE_LOCK(bab_buffer_lock, oldIrql);
    
    //
    // No buffers for you (none left)
    //
    OUT_FCT(bab_buffer_alloc)
    return NULL;
}

//
// Maybe this comment made sense in UNIX?
// We don't search trough any array... 
// We just allocate a new manager...
//
/*
 * Allocate a bab manager from the array of free managers.
 */
bab_mgr_t *
ftd_bab_alloc_mgr(ULONG maxmem)
{
    bab_mgr_t *mgr;

    IN_FCT(ftd_bab_alloc_mgr)

    //
    // Allocate a manager! Non-paged-pool, and "zero'd memory"
    //
    /* search thru the array looking for an unused manager */
    mgr = (bab_mgr_t *)kmem_zalloc(sizeof(bab_mgr_t), KM_NOSLEEP); 
    if (mgr == NULL) 
    {
        //
        // No manager... this is probably a big problem!
        //
        OUT_FCT(ftd_bab_alloc_mgr)
        return NULL;
    }

    //
    // not used... so don't try it ;)
    //
    if (maxmem)
    {
        mgr->flags |= FTD_BAB_PHYSICAL;
    }

    OUT_FCT(ftd_bab_alloc_mgr)
    return mgr;
}

/*
 * Free up a buffer manager
 */
int
ftd_bab_free_mgr(bab_mgr_t *mgr)
{
    IN_FCT(ftd_bab_free_mgr)

    //
    // You cannot free a manager that has chunks in it...
    //
    /* if any buffers are in use, fail! */
    if (mgr->num_in_use != 0) 
    {
        //
        // PROBLEM!
        //
        // If the manager has memory in use, you cannot free it!
        //
        OUT_FCT(ftd_bab_free_mgr)
        return -1;
    }

    //
    // Free manager memory
    //
    kmem_free(mgr,sizeof(bab_mgr_t));


    OUT_FCT(ftd_bab_free_mgr)
    return 0;
}

/*
 *  Allocate a chunk of memory from the manager.
 *  len64 is in 64-bit words.
 */
ftd_uint64_t *
ftd_bab_alloc_memory(bab_mgr_t *mgr, int len64)
{
    bab_buffer_t *buf, *newbuf;
    ftd_uint64_t *ret;
    unsigned int total;
    FTD_IRQL context;

#ifdef _DEBUG_SHOW_IO_SIZES
    static unsigned int     iMedian     = 0x0;
    static unsigned int     iMaximum    = 0x0;
    static unsigned int     iMinimum    = 0xFFFFFFFF;
    static unsigned int     iCounter    = 0x0;
    static unsigned int     imax_counter= 5000;
#endif

    IN_FCT(ftd_bab_free_mgr)

#ifdef _DEBUG_SHOW_IO_SIZES
    //
    // Static counters
    //
    // Moyenne: median size of the last 500 IO's
    // Maximum: maximum size of the last 500 IO's
    // Minimum: minimum size of the last 500 IO's
    //
    iMedian += len64;
    if ((unsigned int)len64 > iMaximum)
    {
        iMaximum = (unsigned int) len64 * sizeof(ftd_uint64_t);
    }

    if (iMinimum > (unsigned int) len64)
    {
        iMinimum = (unsigned int) len64 * sizeof(ftd_uint64_t);
    }

    if (iCounter >= imax_counter)
    {
        DbgPrint("Memory allocation counters: (every %ld allocs)\n",imax_counter);
        DbgPrint("Maximum = %ld bytes\n",iMaximum);
        DbgPrint("Minimum = %ld bytes\n",iMinimum);
        DbgPrint("Median = %ld bytes\n",(iMedian/iCounter));
        
        iMedian = 0;
        iMaximum = 0;
        iMinimum = 0xFFFFFFFF;
        iCounter = 0;
    }
    else
    {
        iCounter++;
    }
#endif


    //
    // PROBLEM!
    //
    // What overhead? The manager itself is in separate memory!
    // What are you talking about?
    //
    /* the overhead of a chunk is the bab_mgr_t structure */

    //
    // If the size you want to write is larger than the maximum size of the chunk
    // PROBLEM!
    // we can't help you!! This is a BIG problem in W2K3 where sizes can vary
    //
    if (    (mgr->flags & FTD_BAB_DONT_USE) 
        ||
            len64 > (chunk_size - (int)sizeof_64bit(bab_mgr_t) )   ) 
    {

#ifdef TRACE_LARGE_WRITES_TO_BAB
        //
        // LARGESIZE
        //
        // check for len64 > (chunk_size - (int)sizeof_64bit(bab_mgr_t)
        //
        // to see if it was actually a request for a very large size
        //
        if (len64 > (chunk_size - (int)sizeof_64bit(bab_mgr_t)))
        {
            DbgPrint("Impossible to allocate BAB memory, size requested larger than max size\nRequested size [%8ld]\nMaximum Size   [%8ld]\n-----------------------------\n", len64 * 8, (chunk_size - (int)sizeof_64bit(bab_mgr_t)) * 8);
        }
#endif
        OUT_FCT(ftd_bab_free_mgr)
        return NULL;
    }

    //
    // Does this manager have any chunks associated to it?
    //
    if (mgr->num_in_use == 0) 
    {
        //
        // NO? get a chunk!
        //
        if ((buf = bab_buffer_alloc()) == NULL) 
        {
            //
            // No chunks? Whoa!
            //
            OUT_FCT(ftd_bab_free_mgr)
            return NULL;
        }

        //
        // Initialize the data, tail = head = tha buffer ;)
        //
        mgr->in_use_tail = mgr->in_use_head = buf;

        //
        // This is the first chunk allocated, so we are at 1
        //
        mgr->num_in_use = 1;
        //
        // Allow this manager's memory to be used by new IO for this logical group
        //
        mgr->flags &= ~FTD_BAB_DONT_USE;

        //
        // Points to the next chunk in this managaer (none yet)
        //
        // PROBLEM!? This was already done in bab_buffer_alloc, why again?
        // 
        buf->next = NULL;

        /* initialize the pending pointers */
        //
        // Pending points to the first buffer that has non-commited data in it.
        //
        // Data is not commited in the writes until it has been actually written
        // to disk. 
        //
        mgr->pending_buf = buf;
        //
        // This points to the area in the pending buffer where the pending data starts
        //
        mgr->pending = buf->free;
    }

    //
    // in_use_tail points to header of the current chunk we are writing to
    //
    /* we always allocate from the tail buffer. */
    buf = mgr->in_use_tail;

    //
    // Total = the amount of free memory in this buffer
    //
    // buf->end   points to the end of the chunk
    // buf->alloc points to the end of the allocated memory of this chunk
    //
    total = buf->end - buf->alloc;

    //
    // if the length of the buffer being written is less than the amount of memory available
    // update the pointers, and give back the memory...
    //
    if ((unsigned int)len64 <= total) 
    {
        //
        // the area of memory where you can write to!
        //
        ret = buf->alloc;
        //
        // Update the allocated pointer of the chunk
        //
        buf->alloc += len64;

        OUT_FCT(ftd_bab_free_mgr)
        return ret;
    } 

    //
    // There was no space in the current chunk to put your stuff!!
    //

    //
    // If we still have available chunks
    //
    if (        (mgr->num_in_use < max_bab_buffers_in_use) 
            && 
    //
    // And we can get a chunk
    //
                ((newbuf = bab_buffer_alloc()) != NULL)     ) 
    {
        /*
         * all of the memory in the last buffer was committed,
         * then bump up the pending pointers, since they point to
         * unallocated space. 
         */

        //
        // if the current buffer is the pending buffer, 
        // and the pending pointer points 
        // to the end of this buffer
        //
        // adjust the pointers
        //
        if ((mgr->pending_buf == buf) && (mgr->pending == buf->alloc)) 
        {
            //
            // We just allocated a new chunk!
            //
            //
            // Ok, now the new chunk is the latest pending chunk
            //
            mgr->pending_buf = newbuf;
            //
            // and the pending pointer points to the start of this buffer!
            //
            mgr->pending = newbuf->free;
        }
        
        //
        // PROBLEM!? This was already done in bab_buffer_alloc, why again?
        // 
        //
        newbuf->next = NULL;

        //
        // the previous chunk points to this one as it's next
        //
        buf->next = newbuf;
        //
        // The "in use tail" or the chunk we now write to is the last allocated chunk
        //
        mgr->in_use_tail = newbuf;
        //
        // Yep, we now have one more chunk in use
        //
        mgr->num_in_use++;

        //
        // Since the buffer is sure to be able to accomodate the size of the write 
        // (remember we checked that in the begining)
        // just update the pointers
        //
        // chunk->alloc points to the non-used portion of the chunk
        //
        ret = newbuf->alloc;
        //
        // update the non-used portion of the chunk to point to behind what you just gave back
        //
        newbuf->alloc += len64;
        OUT_FCT(ftd_bab_free_mgr)
        return ret;
    }

    /* ain't no mo' buffers */
    OUT_FCT(ftd_bab_free_mgr)
    return NULL;
}


//
// Free len64 quadwords of memory!
//
/*
 * Free up some memory from the manager. We can free memory up to the
 * last committed memory in its buffers.
 *
 * len64 is in 64-bit words
 * Returns the number of words not freed. 
 *
 * Remember, the oldest data is at the head; the newest is at the tail.
 */
int
ftd_bab_free_memory(bab_mgr_t *mgr, int len64)
{
    bab_buffer_t *temp;
    int          buflen;
    FTD_IRQL    context;

    IN_FCT(ftd_bab_free_memory)

    //
    // in_use_head = pointer to the oldest available chunk of memory
    //


    //
    // Until there are no more chunk managers available or there 
    // is no more memory to be freed, go trough this list
    //
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

        //
        // If the following is true, it means that 
        // we are in the last "commited" buffer
        // everything past this is still pending
        //
         
        /* if this is the last committed buffer, go no further */
        if (mgr->pending_buf == mgr->in_use_head) 
        {
            //
            // We cannot free memory past the pending pointer.
            // that memory is uncommited memory, and we cannot free 
            // it until it has been commited so:
            //
            // Of course this math only works because we are in the
            // pending_buf...
            //
            // The "free" or "commited" area of memory is whatever is 
            // in front of the pending pointer
            //
            buflen = mgr->pending - mgr->in_use_head->free;

            //
            // if the lenght is smaller than what you just asked for,
            // cool!
            //
            /* free up part of committed memory */
            if (len64 < buflen) 
            {
                //
                // 
                //
                mgr->in_use_head->free += len64;
                len64 = 0;
                
            } /* if all memory is committed, waste the buffer and list. */
            //
            // if the pending pointer points to the head's alloc pointer
            // it means that there is nothing pending. Everything is 
            // empty, so release this buffer completly!
            //
            else if (mgr->pending == mgr->in_use_head->alloc) 
            {
                //
                // Reset everything to NULL!
                //
                /* this can happen only when this is the last buffer! */
                bab_buffer_free(mgr->in_use_head);
                mgr->pending = NULL;
                mgr->pending_buf = NULL;
                mgr->in_use_tail = NULL;
                mgr->in_use_head = NULL;
                mgr->num_in_use = 0;
                len64 -= buflen;
                
            } /* uncommitted memory exists, so just free up the committed stuff */
            //
            // There is still uncomited memory in this mgr's memory
            //
            // depending on where the call to bab_free came to, it may make sense
            //
            // PROBLEM!? Are we sure we can get into this else statement?
            //
            else 
            {
                //
                // set the free area to the pending area. Everything before the
                // pending area is now available.
                //
                // PROBLEM?! If we come here, this means there is pending data 
                // in the BAB. Which is not good in some cases
                // i.e. when we attempt to completly free the BAB!
                //
                mgr->in_use_head->free = mgr->pending;
                len64 -= buflen;
            }

            //
            // Get out of the while loop
            //
            // PROBLEM?! If we released less than requested, what happens?
            //
            break;
        } 

        //
        // Okay, we are not in the pending buffer
        // so we must be in a "commited" or "migrated" area
        // hopefully migrated! Unless we are overflowed, 
        // then we will just waste the BAB anyway!
        //
        /* we haven't made it to the last committed buffer, yet */
        buflen = mgr->in_use_head->alloc - mgr->in_use_head->free;

        //
        // If we are asking for less than the area available in this buffer
        //
        if (len64 < buflen) 
        {
            //
            // adjust the free pointer to advance in the buffer
            //
            mgr->in_use_head->free += len64;
            len64 = 0;
            break;
        } 
        else 
        {
            //
            // if the lenght to be freed is larger than this 
            // it means this chunk is emtpy, and we can 
            // zap it!
            //
            // Hey we may want to free more than just this area, so 
            // substract and go on
            //
            len64 -= buflen;

            //
            // Zap the chunk
            //
            temp = mgr->in_use_head->next;
            bab_buffer_free(mgr->in_use_head);
            mgr->in_use_head = temp;
            mgr->num_in_use--;
        }
    }

    //
    // This will return the amount of memory that was not released...
    // 

    OUT_FCT(ftd_bab_free_memory)
    return len64;
}

//
// This commits memory in the group memory manager's memory
// allowing it to be freed later.
//
// An IO goes trough : pending, commited, migrated, free
//
/*
 * commit some memory from the manager. We can increment the pending pointer
 * up to the end of allocated memory.
 *
 * len64 is in 64-bit words
 * Returns the number of words not committed. 
 */
int
ftd_bab_commit_memory(bab_mgr_t *mgr, int len64)
{
    int          buflen;
    FTD_IRQL    context;

    IN_FCT(ftd_bab_commit_memory)

    //
    // we commit memory until there is no more memory to be commited,
    // or there is no more pending buffers.
    //
    // PROBLEM?! why check mgr->pending_buf == NULL?
    // can that really happen?
    //
    // except if we get a commit_memory after we "destroyed" 
    // the bab?
    // 
    /* start at the tail buffer and move on until we free up "len" bytes */
    while ((len64 > 0) && (mgr->pending_buf != NULL)) 
    {

        //
        // Length of pending data in the current buffer is 
        // alloc - pending 
        //
        // Remember that pending_bug, and pending should always
        // be pointing in the same buffer!
        //
        /* get the amount of uncommitted memory in this buffer */
        buflen = mgr->pending_buf->alloc - mgr->pending;

        //
        // If the amount of commited memory is less than the amount of pending data
        //
        /* commit only part of it */
        if (len64 < buflen) 
        {
            //
            // adjust the pending pointer to be past the data we just commited
            //
            mgr->pending += len64;
            len64 = 0;
            break;
            
        } 
        else /* commit the rest of the buffer and presumably go to the next one */
        { 
            //
            // There is not enough memory in this buffer to satisfy this request,
            // so completly commit this buffer, and go to the next one!
            //
            len64 -= buflen; 
            /* wrap to the next buffer */
            if (mgr->pending_buf->next != NULL) 
            {
                //
                // set pending_buf to next buffer
                // and set pending to the start of the non-commited 
                // memory of that buffer.
                //
                // This should ALWAYS be the start of the next buffer
                //
                mgr->pending_buf = mgr->pending_buf->next;
                mgr->pending = mgr->pending_buf->free;
                
            } 
            else /* no more buffers, all memory is now committed. */ 
            {  
                //
                // There are no more pending buffers, so 
                // the pending pointer should be set to the
                // pending_bug's alloc pointer
                //
                // which means that there is no pending data...
                //
                mgr->pending = mgr->pending_buf->alloc;
                break;  
            } /* len64 should be 0 now, but bogons do exist. */

            // PROBLEM?!
            // yes, len64 should now be 0, because we should not
            // be trying to commit more memory than there is 
            // pending memory in the group's memory manager memory
            //
        }
    }

    OUT_FCT(ftd_bab_commit_memory)
    //
    // Return the amount of non-comited memory
    //
    return len64;
}

/*
 * Return the number of 64-bit words available.
 */
int
ftd_bab_get_free_length(bab_mgr_t *mgr)
{
    int len;
    FTD_IRQL context;
   
    IN_FCT(ftd_bab_get_free_length)

    //
    // the next comment is bogus
    // as we would always do the if
    // the calc is correct for the current group
    //
/* FIXME: base the available space upon the maxInUse buffers.
 * num_free = (max_in_use - mgr->num_in_use); 
 * if (num_free > num_free_bab_buffers) num_free = num_free_bab_buffers;
 * len = num_free * (chunk_size - sizeof_64bit(bab_mgr_t));
 */

    // PROBLEM!
    // In windows, we don't allocate the bab_mgr inside the chunk!
    // so we should not substract this value!
    //
    // Anyhow, the free memory available is
    //
    // the number of free unused chunks * sizeof the chunks
    //
    len = num_free_bab_buffers * (chunk_size - sizeof_64bit(bab_mgr_t));
    if (mgr->in_use_tail != NULL) 
    { 
        //
        // + whatever memory is not free in our current manager's
        // current chunk 
        //
        len += mgr->in_use_tail->end - mgr->in_use_tail->alloc;
    }

    OUT_FCT(ftd_bab_get_free_length)
    return len;
}

//
// Actually this next funcion is called in ftd_ioctl, so why is it commented
// as not needed in product?
//
/*
 * DEBUG: not needed in the product.
 * Return the exact amount of memory allocated in the buffer manager. Don't
 * include wasted space. Do include uncommitted space.
 */
int
ftd_bab_get_used_length(bab_mgr_t *mgr)
{
    int          len;
    FTD_IRQL context;
    bab_buffer_t *temp;

    IN_FCT(ftd_bab_get_used_length)

    //
    // This calculated the amount of memory that is used in each chunk
    //
    len = 0;
    for (temp = mgr->in_use_head; temp != NULL; temp = temp->next) 
    {
        //
        // looks ok
        //
        len += temp->alloc - temp->free;
    }

    OUT_FCT(ftd_bab_get_used_length)
    return len;
}

// 
// pretty benign function
//
/*
 * Return the exact amount of memory allocated in the buffer manager. Don't
 * include wasted space and uncommitted space.
 */
int
ftd_bab_get_committed_length(bab_mgr_t *mgr)
{
    int          len;
    bab_buffer_t *temp;
    FTD_IRQL context; 

    IN_FCT(ftd_bab_get_committed_length)

    //
    // Does the exact same thing as the previous function,
    // but does not count "pending" memory
    //
    // So it only returns the amount of "commited" space
    //
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

    OUT_FCT(ftd_bab_get_committed_length)
    return len;
}

//
// copies our memory area into another memory area
// this means that the other memory are does not
// have to know about our size, we just copy
// what was requested 
//
/*
 * returns the number of words it didn't copy.
 *
 * offset64 is the starting 64-bit word offset from the beginning.
 * len64 is the number of 64-bit words to copy
 */
int
ftd_bab_copy_out(bab_mgr_t *mgr, int offset64, ftd_uint64_t *addr, int len64)
{
    int          templen;
    bab_buffer_t *temp;
    ftd_uint64_t     *start, *oldstart;
    FTD_IRQL context;
    PHYSICAL_ADDRESS   physicalAddressBase;

    IN_FCT(ftd_bab_copy_out)

    //
    // offset is an offset from the begining of the bab
    //
    if (offset64 > 0) 
    {
        start = ftd_bab_get_ptr(mgr, offset64, &temp);
    } 
    else 
    {
        temp = mgr->in_use_head; 
        /* degenerate case: */
        if (temp == NULL) 
        {
            OUT_FCT(ftd_bab_copy_out)
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

        if (mgr->flags & FTD_BAB_PHYSICAL) 
        {
            oldstart = start;

            start = ftd_bab_map_memory(start, templen * 8);
        }

        copyout(start, addr, templen * 8);

        if (mgr->flags & FTD_BAB_PHYSICAL) 
        {
            ftd_bab_unmap_memory(start, templen * 8);
            start = oldstart;
        }

        len64 -= templen;
        addr += (temp->alloc - start);

        /* go to the next buffer */
        temp = temp->next;

        /* degenerate case: */
        if (temp == NULL) 
        {
            break;
        }
        start = temp->free;
    }

    OUT_FCT(ftd_bab_copy_out)
    return len64;
}

//
// This actually returns an offset inside the 
// commited or pending data.
// 
/*
 * Seek to a specified offset in the allocated region.
 * offset64 specifies a 64-bit word offset.
 *
 * N.B.: call with mgr->bm_lck held.
 */
ftd_uint64_t *
ftd_bab_get_ptr(bab_mgr_t *mgr, int offset64, bab_buffer_t **buf)
{
    bab_buffer_t *temp;
    int          len;
    FTD_IRQL context;

    IN_FCT(ftd_bab_get_ptr)

    //
    // if we have a chunk in this manager,
    // and the offset is 0
    //
    if (((temp = mgr->in_use_head) != NULL) && (offset64 == 0)) 
    {
        //
        // then just return the head chunk's free pointer
        // remember that head is the oldest data
        //
        *buf = temp;
        OUT_FCT(ftd_bab_get_ptr)
        return temp->free;
    }

    //
    // do we have a chunk in this manager? and is the offset larger than 0?
    //
    while ((temp != NULL) && (offset64 > 0)) 
    {
        //
        // amount of commited/pending data available in this chunk
        // is alloc - free 
        // 
        len = temp->alloc - temp->free;
        //
        // if the lenght of this block is larger than the requested offset...
        //
        /* if it's in this block, we're done. */
        if (len > offset64) 
        {
            *buf = temp;
            OUT_FCT(ftd_bab_get_ptr)
            return (temp->free + offset64);
            
        } 
        //
        // if lenght is exactly equal to the offset, it means that
        //
        else if (len == offset64) /* if it's at the beginning of the next block, make sure it's there */
        {
            if ((temp = temp->next) != NULL) 
            {
                *buf = temp;
                OUT_FCT(ftd_bab_get_ptr)
                return temp->free;
            }
            
        }
        //
        // Somewhere in the next block...
        //
        else /* got at least one more block to go */
        {
            offset64 -= len;
            temp = temp->next;
        }
    }

    //
    // Either the length specified is larger than the actual amount of 
    // commited or pending memory in the 
    // manager, 
    // or there is no chunk in this manager... return NULL!
    //
    /* went off the deep end */
    *buf = NULL;

    OUT_FCT(ftd_bab_get_ptr)
    return NULL;
}

//
// returns pointer to "pending" which points to the 
// manager's pending chunk's pending memory area
//
/*
 * Return a pointer to the pending pointer, if it points to valid data.
 */
ftd_uint64_t *
ftd_bab_get_pending(bab_mgr_t *mgr)
{
    IN_FCT(ftd_bab_get_pending)

    //
    //
    //
    /* Two invalid cases: No buffers or all data is committed */
    /* Third case: No mgr pointer */
    /* Note: there might be other cases, too. */
    //          No pending chunk manager
    if (        (mgr->pending_buf == NULL) 
            || 
    //          No in_use_tail chunk manager
                (mgr->in_use_tail == NULL) 
            ||  
    //          pending pointer equal to tail's alloc -> all data is commited!
                (mgr->pending == mgr->in_use_tail->alloc)   ) 
    {
        //
        // This is here to show us which case we hit.
        //
#if 0//DBG
        if (mgr->pending_buf == NULL) 
        {
            DbgPrint("Get pending returned NULL - mgr->pending_buf == NULL\n");
        }
        if (mgr->in_use_tail == NULL)
        {
            DbgPrint("Get pending returned NULL - mgr->in_use_tail == NULL\n");
        }
        else if (mgr->pending == mgr->in_use_tail->alloc)
        {
            DbgPrint("Get pending returned NULL - mgr->pending == mgr->in_use_tail->alloc\n");
        }

#endif
        OUT_FCT(ftd_bab_get_pending)
        return NULL;
    }

    //
    // if the manager's pending pointer is equal to the pending chunk manager's alloc
    // and the manager's pending buffer next is not null
    //
    // this means, if the pending buffer points to the end of the allocated block of 
    // the current buffer, and there is a next buffer
    //
    // Why do this?
    //
    // Probably just to make sure that the pending buffer gets advanced to the next 
    // buffer area if the last one is completly commited.
    //
    // My theory: It is because the chunk is often not used completly.
    //
    // The reason is that there is most certanly a space in the buffer at the end
    // that contains no data. So if we don't advance the pending buffer to the next 
    // actual pending data, we are in for some problems.
    //
    
    // From Subin....
    if (        (mgr->pending == mgr->pending_buf->alloc) 
            && 
                (mgr->pending_buf->next != NULL)            )
    {
        //
        // Set the pending buffer chunk to point to the next chunk
        // and set the pending pointer to point to the start of the next chunk
        //
        // because chunk_manager->free == chunk_manager->start 
        //
        mgr->pending_buf = mgr->pending_buf->next;
        mgr->pending = mgr->pending_buf->free;
    }

    //
    // PROBLEM?!
    //
    // What happens when a new buffer comes in that thinks it should be in the current
    // pending buffer, but this has advanced to the next chunk???
    //
    // My theory: It is becasue the new buffer should be in the next chunk anyhow.
    // since the pending was pointing to the end of the last buffer anyhow!
    //

    OUT_FCT(ftd_bab_get_pending)
    return (mgr->pending);
}


//
// The rest is only used if we use the maxmem thingy
// which we don't!
// so skip it!!
//


PVOID 
ftd_bab_map_memory(PVOID address, ULONG length)
{
    ULONG               inIoSpace = 0;
    PHYSICAL_ADDRESS    physicalAddress = RtlConvertUlongToLargeInteger((ULONG)address);
    PHYSICAL_ADDRESS    physicalAddressBase;
    char                *virtualAddress = NULL;

    IN_FCT(ftd_bab_map_memory)

    if ( !HalTranslateBusAddress (0,    // interfaceType
                        0,              // busNumber
                        physicalAddress,
                        &inIoSpace,
                        &physicalAddressBase))
    {
        OUT_FCT(ftd_bab_map_memory)
        return virtualAddress;
    }

    virtualAddress = MmMapIoSpace(physicalAddressBase, length, FALSE);

    OUT_FCT(ftd_bab_map_memory)
    return virtualAddress;
}

VOID 
ftd_bab_unmap_memory(PVOID address, ULONG length)
{
    IN_FCT(ftd_bab_unmap_memory)
    MmUnmapIoSpace(address, length);
    OUT_FCT(ftd_bab_unmap_memory)
}