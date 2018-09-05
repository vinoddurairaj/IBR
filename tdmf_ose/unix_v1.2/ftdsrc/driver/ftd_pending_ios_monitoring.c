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
/**
 * @file
 * @brief @ref pending_ios_monitoring implementation
 *
 * @author Martin Proulx
 */

/********************************************************************************************************************//**
 *
 * @defgroup pending_ios_monitoring Pending IOs monitoring API
 *
 * A pending IO is an IO request which has been received by our driver, but for which its associated read or write
 * request on the physical disk isn't completed yet.
 *
 * Tracking the pending IOs is needed in the following scenarios:
 *
 * - Before launching a full or smart refresh, the PMD must be assured that there are no pending IOs, as otherwise
 *   it is possible that the PMD reads data from disk which can shortly be overwritten by a pending write request.
 *
 * - Before uncapturing a dynamically activated device while stopping a group, where we must wait that all
 *   captured IOs are completed.
 *
 * It's very well possible that at least some of the above needs could be fulfilled by other simpler or better mechanisms
 * than this one, but this remains to be determined.
 *
 * The current implementation relies on a simple pending_ios_monitoring_pending_ios member variable in the ftd_dev_t
 * structure, combined with a group level critical section used exclusively by the pending_ios_monitoring API.
 * 
 * @note We currently monitor both read and write pending IOs and do not make any disctinction between them.
 *
 * In the event that it is noticed that waiting for the completion of both read and write requests is exceedingly inneficient
 * when we actually only need to wait for the completion of writes, both read and writes should be registered separately,
 * so it is then possible to wait only for the completion of write requests.
 *
 * @bug The name 'pending' IO is somewhat of a misnomer, referring to a 'being processed' IO would sound more exact.
 *      The concept of pending may refer to a specific period during the time the IO is 'being processed', which
 *      is only the time during we loose control of the IO after submitting it to the local device, until we
 *      regain control upon IO completion.  We should decide if the IOs are pending according to our own treatment, or
 *      to the application's view.
 *
 * @bug The current implementation does not hold water on AIX, as its strategy function must be written to be able to run
 *      in the interrupt handler execution environment, which prohibits the current sleeping implementation of
 *      enter_pending_ios_monitoring_critical_section(). See more detail here:
 *      http://publib.boulder.ibm.com/infocenter/systems/index.jsp?topic=/com.ibm.aix.kerneltechref/doc/ktechrf1/ddstrategy.htm
 *
 * @todo Figure out a replacement implementation that works on all platforms.
 *       Something based on two counters (pending and pending_while_waiting), a marker in the io request which states
 *       which counter should be decremented, an atomic switch from one counter to the other during the wait and a proper
 *       wait of first pending_while_waiting and then pending should work.
 *
 * @todo Figure out if only waiting for the submission of IOs being processed rather than their completion would be adequate.
 *       
 * @todo Figure out what's the reason to be of the pending_io counter within the _bab_mgr structure and merge this one in
 *       as well if possible.
 *
 * @todo Check out the platform specific pending io counters as well and see if they just couldn't all share this one.
 *       (Linux atomic_t pending_xxx for example).
 *
 ************************************************************************************************************************/

#include "ftd_pending_ios_monitoring.h"

/**
 *
 * @brief Enters the group's monitoring critical section.
 *
 * Only a single thread can enter the critical section of the given group at once.
 * If the critical section is already busy, any additional calling thread will wait for it to
 * be available before entering it.
 *
 * This critical section is what allows the following calls to be synchronized together:
 *
 * - pending_ios_monitoring_register_pending_io()
 * - pending_ios_monitoring_wait_for_pending_ios_completion()
 * - pending_ios_monitoring_wait_for_group_pending_ios_completion()
 *
 * @param lgp The group for which we want to enter the critical section.
 *
 * @post exit_pending_ios_monitoring_critical_section() must absolutely be called when done with the processing
 *       needed in the critical section.
 *
 * @internal The implemenation relies on a combination of the groups's pending_ios_monitoring_lock
 * and pending_ios_monitoring_critical_section_busy variable.  An implementation using our simple locks
 * cannot be used since we need to be able to sleep while holding the critical_section.  Of course, it
 * may be the case that each OS has a native and possible more efficient way to achieve our needs, but
 * this implementation was just simple and works for all.
 *
 */
static void enter_pending_ios_monitoring_critical_section(ftd_lg_t* lgp)
{
    ftd_context_t monitoring_context;
    int critical_section_busy = 0;
    
 try_again:
    ACQUIRE_LOCK(lgp->pending_ios_monitoring_lock, monitoring_context);
    {
#if defined(_AIX)
        // Things are currently arranged so that we never get to wait for the critical section on AIX,
        // as the actual implementation relying on a sleep (FTD_DELAY) cannot work.
        // The arrangement is that the critical_section_busy variable's value is always 0.
        // See @bug and @todo comments in at the @ref pending_ios_monitoring level.
#else
        critical_section_busy = lgp->pending_ios_monitoring_critical_section_busy;
#endif
        
        if(critical_section_busy == 0)
        {
            // The critical section is available, we grab it.
            lgp->pending_ios_monitoring_critical_section_busy = 1;
        }
    }
    RELEASE_LOCK(lgp->pending_ios_monitoring_lock, monitoring_context);

    if(critical_section_busy)
    {
        // We could not enter the critical section. We'll sleep and retry until it's available.
        
        // Using a shorter period is probably useless on most platforms as it appears that the various implementations
        // of FTD_DELAY cannot delay for very small periods anyways.
        // This was clearly noticed on Solaris, where the actual timeout period was noticed to be much longer than the
        // expected timeout value with shorter delay periods.
        static const unsigned int delay_period_us = 10000; // 1/100th of a second.

        FTD_DELAY(delay_period_us);
        goto try_again;
    }
}

/**
 *
 * @brief Exit the group's monitoring critical section.
 *
 * This liberates the critical section so that another thread can enter it.
 *
 * @param lgp The group for which we want to exit the critical section.
 *
 * @pre enter_pending_ios_monitoring_critical_section() must have previously been called.
 *
 */
static void exit_pending_ios_monitoring_critical_section(ftd_lg_t* lgp)
{
    ftd_context_t monitoring_context;
    
    ACQUIRE_LOCK(lgp->pending_ios_monitoring_lock, monitoring_context);
    {
        lgp->pending_ios_monitoring_critical_section_busy = 0;
    }
    RELEASE_LOCK(lgp->pending_ios_monitoring_lock, monitoring_context);
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
void pending_ios_monitoring_register_pending_io(ftd_dev_t* devp)
{
    enter_pending_ios_monitoring_critical_section(devp->lgp);
    {
        ftd_context_t device_context;
        ACQUIRE_LOCK(devp->lock, device_context);
        {
            devp->pending_ios_monitoring_pending_ios++;
        }
        RELEASE_LOCK(devp->lock, device_context);
    }
    exit_pending_ios_monitoring_critical_section(devp->lgp);  
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
void pending_ios_monitoring_unregister_pending_io(ftd_dev_t* devp)
{
    ftd_context_t device_context;
    ACQUIRE_LOCK(devp->lock, device_context);
    {
        devp->pending_ios_monitoring_pending_ios--;
    }
    RELEASE_LOCK(devp->lock, device_context);  
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
ftd_uint32_t pending_ios_monitoring_get_ios_pending(ftd_dev_t* devp)
{
    return devp->pending_ios_monitoring_pending_ios;
}

/**
 * @brief Obtains the number of IOs pending for the given device, while automatically grabbing
 *        and releasing the device's lock.
 *
 * @param devp The device for which we want to know the number of pending IOs.
 *
 * @return The number of pending IOs registered on this device.
 * 
 */
static ftd_uint32_t get_ios_pending_using_device_lock(ftd_dev_t* devp)
{
    ftd_uint32_t ios_pending = 0;
    ftd_context_t device_context;
    
    ACQUIRE_LOCK(devp->lock, device_context);
    {
        ios_pending = pending_ios_monitoring_get_ios_pending(devp);
    }
    RELEASE_LOCK(devp->lock, device_context);
    
    return ios_pending;
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
ftd_uint32_t pending_ios_monitoring_get_group_ios_pending(ftd_lg_t* lgp)
{
    ftd_uint32_t ios_pending = 0;
    ftd_dev_t *devp;
    
    for (devp = lgp->devhead; devp; devp = devp->next) {
        ios_pending += get_ios_pending_using_device_lock(devp);
    }
    
    return ios_pending;
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
ftd_uint32_t pending_ios_monitoring_is_device_being_waited_on_for_pending_ios_completion(ftd_dev_t* devp)
{
    return devp->pending_ios_monitoring_waiting_for_no_pending_ios;
}

/**
 * @brief Registers the fact that someone is waiting or not for IO completion on the given device.
 *
 * @param devp The device for which we want to modify the waiting status.
 * @param waiting_value Set to 1 is we are waiting for pending IOs completion, 0 if not.
 */ 
static void set_device_waiting_for_pending_ios_completion(ftd_dev_t* devp, ftd_uint32_t waiting_value)
{
    ftd_context_t device_context;
    ACQUIRE_LOCK(devp->lock, device_context);
    {
        devp->pending_ios_monitoring_waiting_for_no_pending_ios = waiting_value;
    }
    RELEASE_LOCK(devp->lock, device_context);
}

/**
 * @brief Waits until all pending IOs for the device have completed, assuming the
 *        monitoring_critical_section critical section is already entered.
 *
 * @param devp The device for which we want to wait.
 *
 * @return 0 if succesful
 *         ETIME if a timeout value has been reached.
 *
 * @internal The current need is to actually wait for the completion of write pending IOs only.
 *           See additional comments about this in the description of the @ref pending_ios_monitoring.
 *
 * @internal An implementation based on some waitqueue mechanism would probably be more interesting, but
 *           would probably require a per platform implementation.
 *
 */
static ftd_int32_t wait_for_no_pending_ios(ftd_dev_t* devp)
{
    ftd_int32_t rc = 0;
    
    ftd_uint32_t ios_pending = 0;
    unsigned int time_waited_us = 0;

    set_device_waiting_for_pending_ios_completion(devp, 1);
 try_again:
    ios_pending = get_ios_pending_using_device_lock(devp);

    if(ios_pending > 0)
    {
        static const unsigned int timeout_us = 1800 * 1000000; // Was 600 secs, 10 minutes. Now set to 30 minutes.

        if(time_waited_us > timeout_us)
        {
            FTD_ERR(FTD_WRNLVL, "wait_for_no_pending_ios: Timeout of %d us reached for device: %d in group %d.",
                    timeout_us,
                    getminor(devp->bdev),
                    getminor(devp->lgp->dev) & ~FTD_LGFLAG);
            rc = ETIME;
        }
        else
        {
            // Using a shorter period is probably useless on most platforms as it appears that the various implementations
            // of FTD_DELAY cannot delay for very small periods anyways.
            // This was clearly noticed on Solaris, where the actual timeout period was noticed to be much longer than the
            // expected timeout value with shorter delay periods.
            static const unsigned int delay_period_us = 30000; // Was 10000, 1/100th of a second. Now set to 3/100th sec.

#if defined(FTD_DEBUG)
            FTD_ERR(FTD_DBGLVL,
                    "wait_for_no_pending_ios: Need to delay: ios_pending: %d, group: %d, device: %d, delay: %d, waited: %d, timeout: %d",
                    ios_pending,
                    getminor(devp->lgp->dev) & ~FTD_LGFLAG,
                    getminor(devp->bdev),
                    delay_period_us,
                    time_waited_us,
                    timeout_us);
#endif
            FTD_DELAY(delay_period_us);
            time_waited_us += delay_period_us;
            goto try_again;
        }
    }

    set_device_waiting_for_pending_ios_completion(devp, 0);
    
    return rc;
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
ftd_int32_t pending_ios_monitoring_wait_for_pending_ios_completion(ftd_dev_t* devp)
{
    ftd_int32_t rc = 0;
    
    enter_pending_ios_monitoring_critical_section(devp->lgp);
    {
        rc = wait_for_no_pending_ios(devp);
    }
    exit_pending_ios_monitoring_critical_section(devp->lgp);
    
    return rc;
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
ftd_int32_t pending_ios_monitoring_wait_for_group_pending_ios_completion(ftd_lg_t* lgp)
{
    ftd_int32_t rc = 0;
    
    enter_pending_ios_monitoring_critical_section(lgp);
    {
        ftd_dev_t *devp;
        
        for (devp = lgp->devhead; devp; devp = devp->next) {
            ftd_int32_t device_rc = wait_for_no_pending_ios(devp);

            if(device_rc != 0)
            {
                rc = device_rc;
            }
        }
    }
    exit_pending_ios_monitoring_critical_section(lgp);
    
    return rc;
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
void pending_ios_monitoring_init(ftd_lg_t* lgp)
{
    ALLOC_LOCK(lgp->pending_ios_monitoring_lock, QNM " pending io monitoring API implementation lock.");
    lgp->pending_ios_monitoring_critical_section_busy = 0;
}

// See the .h for the documentation.
/** @ingroup pending_ios_monitoring */
void pending_ios_monitoring_finish(ftd_lg_t* lgp)
{
    DEALLOC_LOCK(lgp->pending_ios_monitoring_lock);
}
