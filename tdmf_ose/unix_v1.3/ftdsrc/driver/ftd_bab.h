/*
 * ftd_bab.h 
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
#ifndef _FTD_BAB_H_
#define _FTD_BAB_H_

#include "ftd_def.h"

#if defined(HPUX)

#ifdef _KERNEL
#include "../h/types.h"
#else
#include <sys/types.h>
#endif

#elif defined(SOLARIS)

#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#endif

#include "ftdio.h"

typedef struct _bab_buffer_
  {
    struct _bab_buffer_ *next;
    ftd_uint64_t *start;	/* the first word of the buffer */
    ftd_uint64_t *end;		/* just past the end of the buffer */
    ftd_uint64_t *alloc;	/* we allocate space from this pointer */
    ftd_uint64_t *free;		/* and add free space at this one */
  }
bab_buffer_t;

/*
 * The bab manager holds a linked list of buffers containing data. Data
 * is allocated from tail->alloc and freed at head->free. Presumably,
 * one or more processes are allocating memory and adding data and another
 * process is removing data and freeing the memory. 
 * 
 * The manager divides the space between head->free and tail->alloc into
 * two regions, pending and committed, using an intermediate pointer
 * "pending". The data between head->free and pending_buf->pending
 * is "committed" and the data between pending_buf->pending and
 * tail->alloc is "pending". 
 */

#define MAX_FRAGS (64)

typedef struct _bab_mgr_
  {
    struct _bab_mgr_ *next;
    ftd_int32_t flags;
    ftd_int32_t num_in_use;	/* number of buffers in "in use" list */
    bab_buffer_t *in_use_tail;	/* the "in use" list */
    bab_buffer_t *in_use_head;	/* the "in use" list */
    bab_buffer_t *pending_buf;
    ftd_uint64_t *pending;
    ftd_int32_t num_frags;          /* number of spans */
    ftd_uint64_t *from[MAX_FRAGS];  /* starting address of each bab location */
    ftd_int32_t count[MAX_FRAGS];   /* number of bytes to be transfered */
    ftd_int64_t pending_io;         /* count of number of I/Os pending */
  }
bab_mgr_t;

#define FTD_BAB_DONT_USE        (1)

#define DEFAULT_MEMCHUNK_SIZE 6*MAXPHYS

/*
 * mgr is the bab_mgr_t structure pointer. 
 * buf is the buffer pointer.
 * block is the pointer to the block of data.
 */
#define BAB_MGR_FIRST_BLOCK(mgr, buf, block) \
{                                            \
    (buf) = (mgr)->in_use_head;              \
    (block) = (buf)->free;                   \
}

#define BAB_WL_LEN(hp) FTD_WLH_QUADS(hp)

/*
 This macro is NOT used in the 4.4 Release - 
All occurances of this macro - 3 instances - once each
in files: ftd_bits.c, ftd_ioctl.c and ftd_all.c are replaced
by the code itself - later , will need to be changed into a
function or macro.
 */
#define BAB_MGR_NEXT_BLOCK(block, hp, buf)                              \
{                                                                       \
    (block) += BAB_WL_LEN(hp);                                          \
    if ((block) == (buf)->alloc) {                                      \
        if (((buf) = (buf)->next) == NULL) {                            \
            (block) = NULL;                                             \
        } else {                                                        \
            (block) = (buf)->free;                                      \
        }                                                               \
    }                                                                   \
}

#endif /* _FTD_BAB_H_ */
