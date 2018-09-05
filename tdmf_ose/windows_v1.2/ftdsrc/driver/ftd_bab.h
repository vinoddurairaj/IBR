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
#include "ftdio.h"

typedef struct _bab_buffer_ {
    struct _bab_buffer_ *next;
    ftd_uint64_t            *start;    /* the first word of the buffer */ 
    ftd_uint64_t            *end;      /* just past the end of the buffer */
    ftd_uint64_t            *alloc;    /* we allocate space from this pointer */
    ftd_uint64_t            *free;     /* and add free space at this one */
} bab_buffer_t;

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
typedef struct _bab_mgr_ {
	struct _bab_mgr_ *next;
	/*
	 * need to synchronize access to this struct. apparently, there is a
	 * race condition involving threads reading fields of the struct in
	 * ftd_ctl_get_group_stats() and writers of these fields in other
	 * contexts, resulting in panic(). rather than use the lg struct
	 * lock, which is already heavily used, allocate one here, bear in
	 * mind this introduces a locking hierarchy including this and the lg
	 * struct lock. 
	 */
	int             flags;
	int             num_in_use;	/* number of buffers in "in use" list */
	bab_buffer_t   *in_use_tail;	/* the "in use" list */
	bab_buffer_t   *in_use_head;	/* the "in use" list */
	bab_buffer_t   *pending_buf;
	ftd_uint64_t   *pending;

} bab_mgr_t;

#define FTD_BAB_DONT_USE        (1)
#define FTD_BAB_PHYSICAL        (2)

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
 *
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

void       ftd_bab_dump_free_buffers _ANSI_ARGS((void));

unsigned int	ftd_bab_init _ANSI_ARGS((int size, int num, ULONG maxmem));
void			ftd_bab_fini _ANSI_ARGS((ULONG maxmem));
bab_mgr_t		*ftd_bab_alloc_mgr _ANSI_ARGS((ULONG maxmem));
int				ftd_bab_free_mgr _ANSI_ARGS((bab_mgr_t *mgr));

ftd_uint64_t	*ftd_bab_alloc_memory _ANSI_ARGS((bab_mgr_t *mgr, int len64));
int				ftd_bab_free_memory _ANSI_ARGS((bab_mgr_t *mgr, int len64));
int				ftd_bab_commit_memory _ANSI_ARGS((bab_mgr_t *mgr, int len64));
int				ftd_bab_get_free_length _ANSI_ARGS((bab_mgr_t *mgr));
int				ftd_bab_get_used_length _ANSI_ARGS((bab_mgr_t *mgr));
int				ftd_bab_get_committed_length _ANSI_ARGS((bab_mgr_t *mgr));
int				ftd_bab_copy_out _ANSI_ARGS((bab_mgr_t *mgr, int offset64, 
                                     ftd_uint64_t *addr, int len64));
ftd_uint64_t	*ftd_bab_get_ptr _ANSI_ARGS((bab_mgr_t *mgr, int offset64, 
                                      bab_buffer_t **buf));
ftd_uint64_t	*ftd_bab_get_pending _ANSI_ARGS((bab_mgr_t *mgr));

PVOID			ftd_bab_map_memory(PVOID address, ULONG length);

VOID			ftd_bab_unmap_memory(PVOID address, ULONG length);
#endif /* _FTD_BAB_H_ */

