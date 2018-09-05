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
 * @brief  @ref pending_ios_monitoring public declarations.
 *
 * @author Martin Proulx
 */

#ifndef _FTD_PENDING_IOS_MONITORING_H_
#define _FTD_PENDING_IOS_MONITORING_H_

#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"

/**
 * @brief Registers the current IO being processed as pending.
 *
 * The registration of the new pending IO can only performed when no one is waiting for pending IOs completion of the device.
 *
 * In the event that someone is currently calling pending_ios_monitoring_wait_for_device_pending_ios_completion() or
 * pending_ios_monitoring_wait_for_group_pending_ios_completion() for the same device or for the device's group,
 * the call automatically blocks until it is possible to register the new pending IO.
 *
 * @attention It is important that the pending IO is registered before any proecssing of importance is done to the newly
 *            received IO.  Of particular importance are the bitmaps update.
 *
 * @param devp The device for which the IO is being registered.
 *
 * @pre The devp->lock must not be locked.
 *
 * @post pending_ios_monitoring_unregister_pending_io() must be called when the IO is completed, wether succesfully or unsuccesfully.
 *
 * @internal The current implementation actually defers the registration of the new pending IO
 *           until no one in the group is waiting for pending IOs completion.
 */
void pending_ios_monitoring_register_pending_io(ftd_dev_t* devp);

/**
 * @brief Removes the current IO being processed from the set of pending IOs.
 *
 * @attention It is important that the pending IO is only unregistered after all processing is completed on both the IO
 *            and the device itself.  In the context of dynamic activation, unregistering the last pending IO of a device
 *            also implies that it is safe to delete the device.
 *
 * @param devp The device for which the IO is being processed.
 *
 * @pre The devp->lock must not be locked.
 *
 * @pre pending_ios_monitoring_register_pending_io() must have previously been called for the same IO.
 *
 */
void pending_ios_monitoring_unregister_pending_io(ftd_dev_t* devp);

/**
 * @brief Obtains the number of IOs pending for the given device.
 *
 * @param devp The device for which we want to know the number of pending IOs.
 *
 * @return The number of pending IOs registered on this device.
 *
 * @pre The devp->lock should be held.
 *
 */
ftd_uint32_t pending_ios_monitoring_get_ios_pending(ftd_dev_t* devp);

/**
 * @brief Checks is someone is waiting for pending IO completions on the given device.
 *
 * @param devp The device for which we are querying the wait status.
 *
 * @return 1 if someone is waiting for the IO completions of this device, 0 otherwise.
 *
 * @pre The devp->lock should be held.
 */
ftd_uint32_t pending_ios_monitoring_is_device_being_waited_on_for_pending_ios_completion(ftd_dev_t* devp);

/**
 * @brief Obtains the number of IOs pending amongst all devices of a group.
 *
 * @param lgp The group for which we want to know the number of IOs pending.
 *
 * @return The total number of pending IOs registered on all devices of the group.
 *
 * @pre The devp->lock should not be held.
 *
 * @internal Currently only used by ftd_del_lg(), which has a questionnable need for this.
 *           See comments around usage in ftd_del_lg().
 */
ftd_uint32_t pending_ios_monitoring_get_group_ios_pending(ftd_lg_t* lgp);

/**
 * @brief Waits until all pending IOs for the device have completed.
 *
 * @param devp The device for which we want to wait.
 *
 * @return 0 if succesful
 *         ETIME if a timeout value has been reached.
 *
 * @pre The devp->lock should not be held.
 *
 * @internal The current implementation waits for all pending ios regardless if they were read or write requests.
 *           Waiting for the completion of write pending IOs only would be sufficient for our current needs.
 *           See additional comments about this in the description of the @ref pending_ios_monitoring.
 */
ftd_int32_t pending_ios_monitoring_wait_for_pending_ios_completion(ftd_dev_t* devp);

/**
 * @brief Waits until all pending IOs for all devices of a group have completed.
 *
 * @param lgp The group for which we want to wait.
 *
 * @return 0 if succesful
 *         ETIME if a timeout value has been reached.
 *
 * @pre No devp->lock of the group's devices should be held.
 *
 * @internal The current implementation waits for all pending ios regardless if they were read or write requests.
 *           Waiting for the completion of write pending IOs only would be sufficient for our current needs.
 *           See additional comments about this in the description of the @ref pending_ios_monitoring.
 *
 * @internal All devices of the group are always waited on.  In the case we encounter errors,
 *           we return the last error we've seen.
 */
ftd_int32_t pending_ios_monitoring_wait_for_group_pending_ios_completion(ftd_lg_t* lgp);


/**
 * @brief Initializes the @ref pending_ios_monitoring support for a group.
 *
 * @param lgp The group for which we want to initialize the API support.
 *
 */
void pending_ios_monitoring_init(ftd_lg_t* lgp);

/**
 * @brief Terminates the @ref pending_ios_monitoring support for a group.
 *
 * @param lgp The group for which we want to termiate the API support.
 *
 */
void pending_ios_monitoring_finish(ftd_lg_t* lgp);

#endif // _FTD_PENDING_IOS_MONITORING_H_
