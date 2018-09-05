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
 * @brief Dynamic activation related declarations.
 *
 * @author Bradley Musolff (original code from tdmf)
 * @author Martin Proulx (replicator adaptations and enhancements)
 *
 */
#ifndef _FTD_DYNAMIC_ACTIVATION_H_
#define _FTD_DYNAMIC_ACTIVATION_H_

#include "ftd_kern_ctypes.h"
#include "ftd_def.h"

extern ftd_devnum_t ftd_major_num;

/**
 * @brief Quick check that allows to find if a device is our dtc device.
 *
 * @pre dynamic_activation_init() must have previously been called.
 *
 * @todo Check if there's already something equivalent so we could get rid of this.
 *       Read the comments of captured_driver_replacement_strategy() for a way to get rid of this and
 *       #ftd_major_num at the same time.
 */
#ifdef linux
  // On linux, a given driver might end up with different char and block major device numbers.
  // Because of this, we must only look at our driver's block major device number, as otherwise,
  // there's a chance that our char major number will match another device's block major number.
  #define ISDTCDEV(dev) ((getmajor(dev) == ftd_major_num.b_major))
#else
  /** @bug the problem documented above for linux most likely applies to HP-UX as well. */    
  #define ISDTCDEV(dev) ((getmajor(dev) == ftd_major_num.c_major) || (getmajor(dev) == ftd_major_num.b_major))
#endif

/**
 * @brief Initiates IO to a possibly captured device using the appropriate mechanism.
 *
 * This function either directly calls a captured device's strategy buffer, or relies on
 * a BDEVSTRAT() call depending if the target device's strategy function has been
 * captured or not.
 *
 * @attention Initiating the IO to a captured source device using BDEVSTRAT instead of this function
 *            would end up recursing infinitely:
 *            - BDEVSTRAT would end up calling our captured_driver_replacement_strategy().
 *            - Our captured_driver_replacement_strategy() would rely on ftd_strategy().
 *            - ftd_strategy() would end up relaying the IO by using BDEVSTRAT.
 *            - And so on...
 *
 * @param bp  The IO buffer for which we should initiate the IO.
 *
 * @return    The strategy function of BDEVSTRAT's return code.
 *
 * @internal  This replaces previous direct calls to BDEVSTRAT from within ftd_do_io() and ftd_flush_lrdb().
 *
 *            From within ftd_flush_lrdb(), it is true that directly calling BDEVSTRAT would still work properly,
 *            unless the pstore device is also a source dtc device.  In the normal case and when the pstore device
 *            is on a captured driver, captured_driver_replacement_strategy() would be called, it would realize that
 *            we don't have any associated dtc device, and would call the original strategy like done here.
 *
 *            The question is, what happens if for some reason the pstore device is also improperly configured as
 *            a source device?
 *
 *            Being called by ftd_flush_lrdb() shields ourselves from such configuration errors, which
 *            we do not have control over.  If it can be proved that nothing will ever go wrong in those
 *            circumstances, then ftd_flush_lrdb() could be changed back to directly calling BDEVSTRAT().
 *
 *            From within ftd_do_io(), we must always go through this function, since the proper way to initiate the io
 *            depends if the target device is on a captured driver or not.
 */
FTD_PUBLIC
int ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device(struct buf *bp);

/** 
 * @brief Returns the proper ftd_dev_t device structure for a given dtc or captured device number.
 *
 * The devno can either directly be a dtc device, or a captured device.
 *
 * @param devno The captured or dtc source device for which we want to obtain the associated ftd_dev_t.
 *
 * @return The registered ftd_dev_t pointer associated with devno, or NULL if devno wasn't previously registered.
 *
 * @attention Must be called from within ftd_strategy() or AIX's ftd_strat().
 *
 * @internal Should replace any previous calls to ddi_get_soft_state() where the device can now possibly be a captured device.
 */
FTD_PUBLIC
ftd_dev_t* ftd_dynamic_activation_get_replication_device_structure(dev_t devno);

/**
 * @brief Globally initializes the support for dynamic activation.
 *
 * Should be called only once from the equivalent of the driver's attach function.
 *
 * @param dev The device number of the /dev/dtc/ctl device.
 *
 * @return 0 if succesful, or a standard errno code otherwise.
 */
FTD_PUBLIC
int ftd_dynamic_activation_init(dev_t dev);

/**
 * @brief Globally terminates the support for dynamic activation.
 *
 * Should be called only once from the equivalent of the driver's detach function.
 *
 */
FTD_PUBLIC
void ftd_dynamic_activation_finish(void);

/**
 *
 * @brief Sets things up to capture and redirect all IO from the source device associated with the given dtc logical device.
 *
 * After this has been called, any IO targetting the source device will be replicated by us in a matter identical as if
 * the IO was targeting the associated logical device.
 *
 * @param devp[in]  The localbdisk member of the structure is used to find out the associated source device.
 * @param devp[out] The flags member of the structure will have bit FTD_LOCAL_DISK_IO_CAPTURED set if succesful.
 *
 * @return 0 if succesful, or a standard errno code otherwise.
 *
 * @post After a successful call, we need to call ftd_dynamic_activation_init_captured_device_release() first
 *       and then ftd_dynamic_activation_release_captured_device() before the dtc logical device is deleted.
 *
 * @internal This is called from the ftd_ctl_capture_source_device_io() implementation of the FTD_CAPTURE_SOURCE_DEVICE_IO ioctl.
 *
 */
FTD_PUBLIC
int ftd_dynamic_activation_capture_device(ftd_dev_t *devp);

/**
 * @brief Prepares the system to properly behave for the upcoming release of a captured device.
 *
 * Calling this function allows us to install a mechanism to ensure that there will not be any captured IOs still being processed
 * when ftd_dynamic_activation_release_captured_device() is called.
 *
 * The current mechanism checks if there are captured IOs in flight and waits for IOs being processed to terminate before returning.
 *
 * It also makes sure than any further IOs received by the capture strategy/read/write routines
 * will not be replicated, only forwarded to the captured devices.
 *
 * If wanted, the mechanism could be modified to busy wait for all pending IOs to terminate rather
 * than for EBUSY to be returned after some time.
 *
 * @param devp[in] The captured device which will soon be released.
 * @param devp[out] The flags member of the structure will have bit FTD_CAPTURED_DEVICE_RELEASE_INITIATED set if 0 or EBUSY is returned.
 *
 * @pre ftd_dynamic_activation_capture_device() should have been previously called for the given devp.
 *
 * @post IOs received by the capture routines after a call to this function will not be replicated anymore.
 *       ftd_dynamic_activation_release_captured_device needs to be called after this function returns succesfully.
 *
 * @return 0 if succesful
 *         ENOENT if the device wasn't previously properly captured.
 *         EBUSY if we couldn't obtain the proper conditions after a few seconds.
 *               In this case, further captured IOs are not replicated anymore, as FTD_CAPTURED_DEVICE_RELEASE_INITIATED
 *               has already been set.
 *
 * @internal This is called from the ftd_ctl_init_stop implementation of the FTD_INIT_STOP ioctl.
 *
 * @internal The behavior of this function could probably have been implemented within
 *           ftd_dynamic_activation_release_captured_device() as well, but aborting a device stop during a call to
 *           FTD_INIT_STOP is the proper and safe thing to do.
 *
 * @todo     In the case where the delays waiting sequentially for IOs to quiet down for each device is found out to be too slow,
 *           we could break this API in two.  The 1st part would be used to set the FTD_CAPTURED_DEVICE_RELEASE_INITIATED flag on all
 *           devices of the group, and the 2nd call would perform the wait.  This should quiet down the pending IOs on all captured devices
 *           in parallel.
 */
FTD_PUBLIC
int ftd_dynamic_activation_init_captured_device_release(ftd_dev_t *devp);

/**
 * @brief Deconstruct the mechanism used to capture and redirect all IO from the source device associated with the given
 * dtc logical device.
 *
 * @param devp[in] The localbdisk member is looked at in order to obtain the associated captured device.
 * @param devp[out] The flags member of the structure will have bit FTD_LOCAL_DISK_IO_CAPTURED unset if succesful.
 *
 * @pre ftd_dynamic_activation_capture_device() should have been previously called for the given devp.
 *
 * @return 0 or ENOENT if the device wasn't previously properly captured.
 *
 * @internal This is called from the ftd_ctl_capture_source_device_io() implementation of the
 *           FTD_RELEASE_CAPTURED_SOURCE_DEVICE_IO ioctl.
 */
FTD_PUBLIC
int ftd_dynamic_activation_release_captured_device(ftd_dev_t *devp);

/**
 * @brief Fail-safe mechanism to ensure that a captured device has been released before it gets deleted.
 *
 * This was added to prevent panics in cases where the replacement routines are seen to handle deleted devices which cause a panic.
 * See WR PROD00007744 for an example.
 *
 * Error messages are logged if the device was still captured.
 *
 * @param devp The captured device which is about to be deleted.
 *
 * @return 0 If the device wasn't captured or if it could be released successfully.
 *         A standard errno otherwise, coming from trying to release the device.
 */
int ftd_dynamic_activation_pre_device_delete_capture_release_check(ftd_dev_t *devp);

#endif // _FTD_DYNAMIC_ACTIVATION_H_
