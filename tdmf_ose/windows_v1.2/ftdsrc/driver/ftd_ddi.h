/*
 * ftd_ddi.h 
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
#ifndef _FTD_DDI_H_
#define _FTD_DDI_H_

#include "ftd_def.h"

#define bzero(ptr,size)				RtlZeroMemory(ptr, size)
#define ddi_copyin(a,b,c,d)			copyin(a,b,c)
#define ddi_copyout(a,b,c,d)		copyout(a,b,c)

#define KM_SLEEP         1
#define KM_NOSLEEP       0

#define DDI_SUCCESS  0
#define DDI_FAILURE -1

extern int copyout(void *Destination, void *Source, unsigned long Length) ;
extern int copyin(void *Destination, void *Source, unsigned long Length) ;

/* 
 * DDI emulation functions 
 */
int ddi_soft_state_init _ANSI_ARGS((void **state, size_t size, size_t nitems));
void ddi_soft_state_fini _ANSI_ARGS((void **state));
void *ddi_get_soft_state _ANSI_ARGS((void *state, int item));
void ddi_soft_state_free _ANSI_ARGS((void *state, int item));
int ddi_soft_state_zalloc _ANSI_ARGS((void *state, int item, void *ptr));
int ddi_get_free_soft_state(void *state);

/*
 * More Solaris emulation functions
 */
void delay (long mSecs);
void *kmem_alloc(size_t size, int flag);
void *kmem_zalloc(size_t size, int flag);
void kmem_free(void *addr, size_t size);

/*
 * Finish up the biodone call
 */
void ftd_biodone_finish(struct buf *);

#endif /* _FTD_DDI_H_ */
