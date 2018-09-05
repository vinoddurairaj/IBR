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
 * @brief Declarations of platform specific parts of the @ref dynamic_activation_os.
 *
 * @author Martin Proulx
 */
#ifndef _FTD_DYNAMIC_ACTIVATION_LINUX_H_
#define _FTD_DYNAMIC_ACTIVATION_LINUX_H_

// --------------------------------------------------------------------------------------
//                                     I N C L U D E S
// --------------------------------------------------------------------------------------

#include <linux/blkdev.h>

#include "ftd_def.h"

// --------------------------------------------------------------------------------------
//                        C O N S T A N T S   A N D   T Y P E S
// --------------------------------------------------------------------------------------

typedef struct io_function_ptrs {
    make_request_fn* make_request_fn;
} io_function_ptrs_t;

/** We swap based on the captured device's request queue. */
typedef struct request_queue* swap_table_key_t;

typedef struct captured_io_parameters
{
    struct request_queue* queue;
    struct bio *bio;
} captured_io_parameters_t ;

// --------------------------------------------------------------------------------------
//                 G L O B A L   V A R I A B L E   R E F E R E N C E S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//               G L O B A L   F U N C T I O N   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//     I N L I N E S,  M A C R O S,  A N D   T E M P L A T E   D E F I N I T I O N S
// --------------------------------------------------------------------------------------

#define SWAP_KEYS_DESCRIPTION "request queue: %p"

extern int ftd_make_request(struct request_queue *queue, struct bio *bh);

// See ftd_dynamic_activation.c for the documentation.
// Declared here just for the linux implementation of captured_device_table_get_partition_io();
ftd_dev_t* captured_device_table_get (dev_t devno);

// See the .h for the documentation.
static inline swap_table_key_t ftd_dynamic_activation_get_swap_key_from_io_request(struct bio* bio)
{
    return bdev_get_queue(bio->bi_bdev);
}

// See the .h for the documentation.
/**
 * @internal The linux implementation is the only implementation that needs to rely on the opaque io_params argument.
 *           Luckilly, we can safely pierce the opacity because there's only one type parameters that are captured and packaged
 *           in this structure on linux.
 */
static inline swap_table_key_t ftd_dynamic_activation_get_swap_key_from_io_params(dev_t dev, const captured_io_parameters_t* io_params)
{
    return bdev_get_queue(io_params->bio->bi_bdev);
}

// See the .h for the documentation.
static inline void ftd_dynamic_activation_get_swap_keys_from_replication_device(ftd_dev_t *devp,
                                                                         swap_table_key_t* block_swap_key,
                                                                         swap_table_key_t* char_swap_key)
{
    *block_swap_key = bdev_get_queue(devp->bd);
    *char_swap_key = bdev_get_queue(devp->bd);
}

/**
 * @brief Linux implementation of the BDEVSTRAT function, which is used to submit an IO request to a non captured device.
 *
 * @todo Should be renamed to something like send_io_to_a_non_captured_device() and unix implementations/macros currently in ftd_all.h
 *       should be moved within their respective platform's dynamic activation implementations.
 */
static inline int BDEVSTRAT(struct bio *bio)
{
    generic_make_request(bio);
    return 0;
}

/**
 * @brief Obtains the dtc logical information associated with the partition device which an IO request falls into.
 *
 * On linux, blk_partition_remap() is always called by generic_make_request() before an IO is submitted
 * to a queue's make_request_fn.
 * 
 * Because of this, anytime the make_request_fn of a queue is captured, we'll capture all IOs targeting
 * both the whole device and its partitions.
 * 
 * Upon reception, all of the IO requests will have already been remapped to target the whole device
 * rather than one of its partition.
 *
 * Since each partition appears as a different block device and that we typically want to replicate IOs issued to partitions
 * only, we need to figure out which partition device corresponds to a given IO request remapped to the whole device
 * and modify the request accordingly so that we can 1st detect the fact that we need to process it and then process it accordingly.
 *
 * @param dev            The device number of the partitioned device.
 * @param io_params[in]  The IO request targeting the partitioned device but possibly containing a possibly replicated partition.
 * @param io_params[out] If the IO falls within the range of a replicated partition device,
 *                       the IO request modified to now target the proper partition of the captured device.
 *
 * @return The pointer to dtc logical device information associated with the proper partition if found, NULL otherwise.
 *         In the case the IO requests is an empty one (length of 0 sectors) NULL will be returned.
 *
 * @note In the case a given IO request falls into the range of a partition, we have no way to know if the IO was actually
 *       issued to a partition and has been remapped or was actually issued directly to the device containing partitions.
 *
 * @pre Captured_device_table_get() should normally have been called to check if we are replicating the partitioned device
 *      before verifying if we replicate a partition.
 *
 * @note IO requests directly issued to a partitioned device but that cross over the end boundaries of a partition are
 *       not considered as falling into this partition and are never replicated.
 *       Only a warning about the situation is issued. C.F. http://9.29.84.134:8080/browse/RFX-152
 *
 * @ingroup captured_device_table
 */
ftd_dev_t* captured_device_table_get_partition_io(dev_t dev, const captured_io_parameters_t *io_params);

/**
 * @brief Changes an IO request's offset and device members so that the IO is remapped to its containing device, if any.
 *
 * Ensures that any IO request targeting a partition is remapped into an equivalent request targeting the partition's containing device.
 *
 * @attention When an IO targeting a partition is sent to a captured device, this always needs to be used, because generic_make_request()
 *            will not be called and so the make_request_fn() already expects the request to target
 *            the device containing partitions.
 *
 * @param bio[in]  The IO request possibly targeting a partition.
 * @param bio[out] The IO request modified to now target the containing device of the partition, if needed.
 *
 */
void map_io_to_containing_device(struct bio* bio);



#endif // _FTD_DYNAMIC_ACTIVATION_LINUX_H_
