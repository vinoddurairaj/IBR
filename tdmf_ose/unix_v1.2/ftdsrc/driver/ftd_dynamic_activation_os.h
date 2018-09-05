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
/***************************************************************************************
 *
 * LICENSED MATERIALS / PROPERTY OF IBM
 *
 * (c) Copyright IBM Corp.  2001-2010.  All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 ***************************************************************************************/

/**
 * @file
 * @brief Dynamic activation private API declarations which each OS needs to declare or implement.
 *
 * @author Martin Proulx (replicator adaptations and enhancements)
 *
 * @todo   Check out if we could make our AIX and HP compilers recognize the inline keyword.
 *         Enabling C99 support should do it.
 *
 * @todo   Clean up the compiler warnings seen on non linux platforms and caused by the non linux static implementations
 *         at the bottom of this file.
 */
#ifndef _FTD_DYNAMIC_ACTIVATION_OS_H_
#define _FTD_DYNAMIC_ACTIVATION_OS_H_

/********************************************************************************************************************//**
 *
 * @defgroup dynamic_activation_os dynamic activation private platform specific API.
 *
 * A platform specific API that needs to be implemented or used by each and every platform.
 *
 ************************************************************************************************************************/

// --------------------------------------------------------------------------------------
//                                     I N C L U D E S
// --------------------------------------------------------------------------------------

/**
 * @typedef strat_func_ptr
 * @brief Just a typedef to ease the manipulation of a pointer to a strategy routine.
 */

/**
 * @typedef write_func_ptr
 * @brief Just a typedef to ease the manipulation of a pointer to a platform specific write routine.
 */

/**
 * @typedef read_func_ptr
 * @brief Just a typedef to ease the manipulation of a pointer to a platform specific read routine.
 */

/**
 * @typedef io_function_ptrs_t
 * @brief Holds together the strategy and read write functions on unix systems, or the make_request_fn on linux.
 */

/**
 * @typedef swap_table_key_t
 * @brief Platform specific key over which the #io_function_ptrs_t routines are swapped.
 */

/**
 * @typedef captured_io_parameters
 * @brief Encapsulates all io parameters tied to a platform's specific strategy, read, write, aread, write
 *        or make request function.
 *
 * This is needed so that the dynamic_activation_captured_driver_generic_replacement_routine can be used regardless
 * of the type of IO we have just captured.
 *
 * An internal union is used to provide the different io parameters depending on the exact io we're encapsulating.
 * It's not C++ but is still better than relying on a void*.
 */

/**
 * The following platform specific include should define at least io_function_ptrs_t and captured_io_parameters.
 * The SWAP_KEYS_DESCRIPTION may also be defined if needed.
 */
#if defined(linux)
  #include "ftd_dynamic_activation_linux.h"
#elif defined(_AIX)
  #include "ftd_dynamic_activation_aix.h"
#elif defined(__sun)
  #include "ftd_dynamic_activation_sun.h"
#elif defined(__hpux)
  #include "ftd_dynamic_activation_hpux.h"
#else
  #error "Unsupported platform"
#endif

#ifndef SWAP_KEYS_DESCRIPTION
  // Default description:
  #define SWAP_KEYS_DESCRIPTION "block major device: %d"
#endif

#include "ftd_kern_ctypes.h"
#include "ftd_def.h"

// --------------------------------------------------------------------------------------
//                        C O N S T A N T S   A N D   T Y P E S
// --------------------------------------------------------------------------------------

/** @brief Platform independant signature used to forward a captured IO request to a platform specific function. */
typedef int (*generic_process_io_func_ptr) (const captured_io_parameters_t *io_params);
/** @brief Platform independant signature used to pass a captured IO request through in a platform specific manner. */
typedef int (*generic_pass_io_through_func_ptr) (const captured_io_parameters_t *io_params, const io_function_ptrs_t* original_io_routines);

// --------------------------------------------------------------------------------------
//                 G L O B A L   V A R I A B L E   R E F E R E N C E S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//               G L O B A L   F U N C T I O N   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//     I N L I N E S,  M A C R O S,  A N D   T E M P L A T E   D E F I N I T I O N S
// --------------------------------------------------------------------------------------

/**
 * @brief Obtains the replacement routines specific to a given platform.
 *
 * This needs to be implemented for each platform.
 *
 * \return A static pointer to the platform's replacements.
 */
const io_function_ptrs_t* ftd_dynamic_activation_get_replacement_routines(void);

/**
 * @brief Platform specific function that will replace the currently installed routines by
 *        the given routines for the given device.
 *
 * @param block_swap_key       The block device key for which we must replace the strategy routine.
 * @param block_devno          The block device number for which we must replace the strategy routine.
 * @param char_swap_key        The char device key for which we must replace the strategy routine.
 * @param replacement_routines The new routines to install for devno.
 *
 * @return A structure containing pointers to the io routines that we have just replaced.
 *         If the swap of any of these routines have failed, a pointer to NULL will be set in the structure.
 *
 * @note HP-UX does have different major numbers for both the block and character devices.
 *       It is for this platform that we must pass both block and char device numbers.
 *
 * @note AIX's swap key is technically the major device number, but the devswchg() API does require the
 *       full block device identifier.  It is for this reason only that we also need the block_devno parameter.
 *
 * @todo Understand the comments in the HP implementation about alignment and atomic swapping,
 *       and take action on it.  At first sight, it seems to me as if it's a given that
 *       function pointers will always be aligned to word boundaries on every platform.
 *
 * @todo Abstract the per platform key/device information that's really required.
 */
io_function_ptrs_t ftd_dynamic_activation_swap_io_routines(swap_table_key_t block_swap_key,
                                                           dev_t block_devno,
                                                           swap_table_key_t char_swap_key,
                                                           const io_function_ptrs_t* replacement_routines);

/**
 * @brief Platform specific way to map an IO request to the swap table key.
 *
 * @param bp A block IO request.
 *
 * @return The swap table key associated with the request.
 */
swap_table_key_t ftd_dynamic_activation_get_swap_key_from_io_request(struct buf* bp);

/**
 * @brief Platform specific way to obtain the swap key based on the target device of a given IO or its IO params.
 *
 * @param dev      The device that the IO is targeting.
 * @param io_param The opaque IO parameters associated with the request.
 *
 * @return The swap table key associated with the device or params.
 *
 * @internal The dev parameter is actually an easy workaround on the unix platforms to cover the fact that there's no
 *           embedded IO type in the captured_io_parameters_t.  Some kind of typing or extraction mechanism would
 *           be required to extract the device from the io_params according to the actual type of IO.
 */
swap_table_key_t ftd_dynamic_activation_get_swap_key_from_io_params(dev_t dev, const captured_io_parameters_t* io_params);

/**
 * @brief Platform specific way to map an IO request to the swap table key.
 *
 * @param devp[in]             Our replicated device's structure.
 * @param block_swap_key[out]  The swap key to use for IO to the block device.
 * @param char_swap_key[out]   The swap key to use for IO to the char device.
 *                             Needed on HP-UX only, should be the same as the block_swap_key on all other platforms.
 *
 */
void ftd_dynamic_activation_get_swap_keys_from_replication_device(ftd_dev_t* devp,
                                                                  swap_table_key_t* block_swap_key,
                                                                  swap_table_key_t* char_swap_key);


/**
 * @brief Dynamic activation generic replacement routine
 *
 * Generic logic that calls our processing function if the device is owned by us or passes through to the original routine if not.
 *
 * @param dev                        The device targetted by the captured IO.
 * @param io_params                  The OS and capture routine specific opaque IO parameters.  This parameter is regiven as is either
 *                                   to the process_captured_io or pass_through_captured_io functions.
 * @param process_captured_io        Function that is called to process the IO whenever the IO is targetting a replicated device.
 * @param pass_through_captured_io   Function that is called whenever we should not process the IO and just need to let it pass through to its
 *                                   original capture routine.  Note that the usage of pass through here has nothing to do with our PASSTHRU
 *                                   mode, in which we actually process the io.
 *
 * @return Directly the return codes of either the process_captured_io or pass_through_captured_io, which have various significances
 *         depending on the platform and captured function.  In any case, the return code should be as expected for the native captured function.
 *
 * @attention When installed, this function ends up being in the IO path of all IO requests on all minor devices
 *            of the captured driver.  Because of this, attention needs to be paid to efficiency.
 *
 * @attention It must be guaranteed that we will always be able to complete any IO given to this function,
 *            even if we may be in the process of capturing or releasing replication devices simultaneously.
 * 
 * @pre This is indirectly installed by calling install_capture_routines_if_needed().
 *      The real installed routines are OS specific, the ones returned by dynamic_activation_get_replacement_routines()
 *      and are implemented in each OS's DynamicActivation file, but rely on this generic one.
 *
 *      The routines  are the real replacement routines
 *
 * @note It may make sense to only process the captured io on write requests, but it would
 *       remove our ability to gather statistics on read IO requests.
 *
 * @ingroup dynamic_activation
 */
int ftd_dynamic_activation_captured_driver_generic_replacement_routine(dev_t dev,
                                                                       const captured_io_parameters_t *io_params,
                                                                       generic_process_io_func_ptr process_captured_io,
                                                                       generic_pass_io_through_func_ptr pass_through_captured_io);

#ifndef linux /* Common non Linux implementations */

#if defined(HPUX) || defined(_AIX)
  // The compiler tested on sjbhp11v didn't recognize the inline keyword.
  // The compiler tested on sjbimax61m1 didn't recognize the inline keyword.
  #define inline
#endif

// See above for the documentation.
static inline swap_table_key_t ftd_dynamic_activation_get_swap_key_from_io_request(struct buf* bp)
{
    return getmajor(BP_DEV(bp));
}

// See above for the documentation.
/**
 * @internal Luckilly, the unix (non linux) implementation only needs the dev parameter and not the opaque io_params.
 *           Otherwise, we'd need to add some mechanism to known which kind of IO params we're given, or how to obtain the device from it.
 */
static inline swap_table_key_t ftd_dynamic_activation_get_swap_key_from_io_params(dev_t dev, const captured_io_parameters_t* io_params)
{
    return getmajor(dev);
}

// See above for the documentation.
static inline void ftd_dynamic_activation_get_swap_keys_from_replication_device(ftd_dev_t *devp,
                                                                                swap_table_key_t* block_swap_key,
                                                                                swap_table_key_t* char_swap_key)
{
    *block_swap_key = getmajor(devp->localbdisk);
    *char_swap_key = getmajor(devp->localcdisk);
}

#endif /* ndef linux */

#endif // _FTD_DYNAMIC_ACTIVATION_OS_H_
