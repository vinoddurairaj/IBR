NOTE: this file is unused
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
/*
 * ftd_thread.h:
 * Author       : D.Kotrappa.(d.kotrappa@pst.fujitsu.com)
 * Date         : 2003/08/01 18:35 JST
 * Description  : Data structures for ftd_threads used for buffer_head
 *                queue processing.
 */
#include <linux/version.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <asm/bitops.h>
#include <linux/locks.h>
#include <linux/slab.h>


#include "ftd_linux.h"
#include "ftd_def.h"

#ifndef _FTD_THREAD_H
#define _FTD_THREAD_H

typedef wait_queue_head_t ftd_wait_queue_head_t;

typedef struct ftd_thread_s {
        void                    (*run) (void *data);
        void                    *data;
        ftd_wait_queue_head_t    wqueue;
        unsigned long           flags;
        struct completion       *event;
        struct task_struct      *tsk;
        const char              *name;
} ftd_thread_t;

#define FTD_THREAD_WAKEUP  0

static inline void ftd_flush_signals (void)
{
        spin_lock(&current->sighand->siglock);
        flush_signals(current);
        spin_unlock(&current->sighand->siglock);
}

static inline void ftd_init_signals (void)
{
        current->exit_signal = SIGCHLD;
        siginitsetinv(&current->blocked, sigmask(SIGKILL));
}

#define ftd_init_waitqueue_head init_waitqueue_head
#define ftd_signal_pending signal_pending
#define FTD_DECLARE_WAITQUEUE(w,t) DECLARE_WAITQUEUE((w),(t))

#define  FTD_BUG(x...) { printk("ftd: bug in file %s, line %d\n", __FILE__, __LINE__); } 

/* ftd_thread.c prototypes */
extern  ftd_thread_t *ftd_register_thread(void (*run) (void *), void *data, const char *name);
extern  void ftd_interrupt_thread(ftd_thread_t *thread);
extern  void ftd_unregister_thread(ftd_thread_t *thread);
extern  void ftd_wakeup_thread(ftd_thread_t *thread);
extern  int ftd_thread(void * arg);
extern  void ftd_task(void *data );
#endif /* #ifndef _FTD_THREAD_H_ */
