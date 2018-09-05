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
 * ftd_thread.c
 * Description 	: Registering/unregistering ftd_thread's and ftd_thread itself.
 */
#include "ftd_linux.h"
#include "ftd_thread.h"
#include "ftd_kern_cproto.h"

ftd_thread_t *
ftd_register_thread(void (*run) (void *), void *data, const char *name)
{
	ftd_thread_t *thread;
	struct completion event;

	ftd_debug1("ftd_register_thread: registering thread for %s\n", name);
	thread = (ftd_thread_t *)kmalloc(sizeof(ftd_thread_t), GFP_KERNEL);
	if (!thread)
		return NULL;

	memset(thread, 0, sizeof(ftd_thread_t));
	ftd_init_waitqueue_head(&thread->wqueue);

	init_completion(&event);
	thread->event = &event;
	thread->run = run;
	thread->data = data;
	thread->name = name;
	if (kernel_thread(ftd_thread, thread, 0) < 0) {
		kfree(thread);
		return NULL;
	}
	wait_for_completion(&event);
	return thread;
}

void
ftd_interrupt_thread(ftd_thread_t *thread)
{
	if (!thread->tsk) {
		FTD_BUG();
		return;
	}
	ftd_debug1("interrupting FTD-thread pid %d\n", thread->tsk->pid);
	send_sig(SIGKILL, thread->tsk, 1);
}

void
ftd_unregister_thread(ftd_thread_t *thread)
{
	struct completion event;

	init_completion(&event);

	thread->event = &event;
	thread->run = NULL;
	thread->name = NULL;
	ftd_interrupt_thread(thread);
	wait_for_completion(&event);
	kfree(thread);
}

void
ftd_wakeup_thread(ftd_thread_t *thread)
{
	ftd_debug1("ftd: waking up FTD thread %p.\n", thread);
	set_bit(FTD_THREAD_WAKEUP, &thread->flags);
	wake_up(&thread->wqueue);
}

int
ftd_thread(void * arg)
{
	ftd_thread_t *thread = arg;

	lock_kernel();

	/* Detach thread */
#if defined(linux)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
        daemonize();
#else
        daemonize(thread->name);
#endif /* < KERNEL_VERSION(2, 6, 0) */
#else
	daemonize();
#endif /* defined(linux) */

	sprintf(current->comm, thread->name);
	ftd_init_signals();
	ftd_flush_signals();
	thread->tsk = current;

	unlock_kernel();

	complete(thread->event);
	while (thread->run) {
		void (*run)(void *data);
		DECLARE_WAITQUEUE(wait, current);

		add_wait_queue(&thread->wqueue, &wait);
		set_task_state(current, TASK_INTERRUPTIBLE);
		if (!test_bit(FTD_THREAD_WAKEUP, &thread->flags)) {
			ftd_debug1("ftd: thread %x went to sleep.\n", thread);
			schedule();
			ftd_debug1("ftd: thread %x woke up.\n", thread);
		}
		current->state = TASK_RUNNING;
		remove_wait_queue(&thread->wqueue, &wait);
		clear_bit(FTD_THREAD_WAKEUP, &thread->flags);

		run = thread->run;
		if (run) {
			run(thread->data);
			run_task_queue(&tq_disk);
		}
		if (ftd_signal_pending(current))
			ftd_flush_signals();
	}
	complete(thread->event);
	return 0;
}

void
ftd_task(void *data)
{
	ftd_dev_t               *softp;
	struct  completion      event, event2;
	struct  buf             *buf, *buf2;
	char                    *lrdbmap;

	softp = (ftd_dev_t *)data;

	/* for first 4KB */
	buf = softp->lrdbbp;
	lrdbmap = (char *)softp->lrdb.map;

	init_completion(&event);
	init_buffer(buf, biodone_psdev, &event);
	bcopy(lrdbmap, buf->b_data, buf->b_bcount);

	/* for second 4KB */
	buf2 = buf + 1;
	lrdbmap += buf->b_bcount;

	init_completion(&event2);
	init_buffer(buf2, biodone_psdev, &event2);
	bcopy(lrdbmap, buf2->b_data, buf2->b_bcount);

	/* make IO request */
	generic_make_request(WRITE, buf);
	run_task_queue(&tq_disk);
	wait_for_completion(&event);
	/* test_bit(BH_Uptodate, &(buf->b_state)); */

	generic_make_request(WRITE, buf2);
	run_task_queue(&tq_disk);
	wait_for_completion(&event2);
	/* test_bit(BH_Uptodate, &(buf2->b_state)); */
}
