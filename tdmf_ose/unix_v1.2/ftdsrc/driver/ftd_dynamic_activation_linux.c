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
 * (c) Copyright IBM Corp.  2001-2009.  All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 ***************************************************************************************/

/**
 * @file @brief Implementation of platform specific parts of the @ref dynamic_activation_os.
 */

// --------------------------------------------------------------------------------------
//                                     I N C L U D E S
// --------------------------------------------------------------------------------------

#include "ftd_dynamic_activation_os.h"
#include "ftd_kern_cproto.h"

// --------------------------------------------------------------------------------------
//                        C O N S T A N T S   A N D   T Y P E S
// --------------------------------------------------------------------------------------
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
/** Opaque type allowing to obtain partition information. */
typedef struct partition_information
{
    struct gendisk* disk;
    dev_t containing_devno;
    int partno;
    
} partition_information_t;
#else // LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
typedef struct partition_information
{
    struct block_device* disk;
    
}  partition_information_t;
#endif // LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)

// --------------------------------------------------------------------------------------
//               G L O B A L   V A R I A B L E S   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//                 L O C A L   F U N C T I O N   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

/***************************************************************************
 * @brief Obtains opaque information which is later used to obtain information related to the partition.
 *
 * @param bd_disk           The containing disk object.
 * @param containing_devno  The device number of the containing disk.
 * @param partno            The 0 based index of the partition we're requesting, where 0 is the containing device and 1 is the 1st partition.
 *
 * @post put_partition_info() must be called on the resulting partition information if the return of partition_info_get_partition() is non NULL.
 * 
 ***************************************************************************/
static partition_information_t get_partition_info(struct gendisk *bd_disk, dev_t containing_devno, int partno);

/**
 * Frees up the opaque partition information.
 *
 * @param partition_info   Information obtained from get_partition_info();
 */
static void put_partition_info(const partition_information_t* partition_info);

/**
 * Obtains the hd_struct from the partition_info.
 *
 * @param partition_info   Information obtained from get_partition_info();
 *
 * @return A hd_pointer to an hd_struct or NULL if no partition information is available.
 */
static struct hd_struct* partition_info_get_partition(const partition_information_t*  partition_info);

/**
 * @brief Obtains the dev_t associated with a given partition.
 *
 * @param part               The partition structure.
 * @param containing_device  The dev_t of the containing device.
 * @param partno             The 0 based index of the partition we're requesting, where 0 is the containing device and 1 is the 1st partition.
 *
 * @return The device number of the partition or 0 if no partition information is available.
 *
 */
static dev_t partition_info_get_devt(const partition_information_t* partition_info);

// --------------------------------------------------------------------------------------
//                                       M A C R O S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//                                         C O D E
// --------------------------------------------------------------------------------------

// See the ftd_dynamic_activation.h for the documentation.
/** @ingroup dynamic_activation_os */
io_function_ptrs_t ftd_dynamic_activation_swap_io_routines(swap_table_key_t block_swap_key,
                                                           dev_t block_devno,
                                                           swap_table_key_t char_swap_key,
                                                           const io_function_ptrs_t* replacement_routines)
{
    io_function_ptrs_t old_routines = {0};
    struct request_queue* device_queue = block_swap_key;
    
    BUG_ON(replacement_routines->make_request_fn == NULL);

    BUG_ON(block_swap_key != char_swap_key);

    old_routines.make_request_fn = device_queue->make_request_fn;
    
    device_queue->make_request_fn = replacement_routines->make_request_fn;
        
    return old_routines;
}

/**
 * @brief Processes (replicates) an IO request received by our replacement captured queue make request function.
 *
 * @param io_params      The parameters of the captured IO request.
 *
 * @return The platform specific standard return codes for the make request function.
 *
 * @internal The signature needs to conform to the generic_process_io_func_ptr type.
 */
static int process_captured_queue_io(const captured_io_parameters_t* io_params)
{
    return ftd_make_request(io_params->queue, io_params->bio);
}

/**
 * @brief Forwards an io by directly calling the make_request_fn on a captured queue.
 *
 * @param io_params The parameters of the captured IO request.  @param
 * original_io_routines The IO routines that were replaced.
 *
 * @return A standard request function return code.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_queue_make_request_fn(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    original_io_routines->make_request_fn(io_params->queue, io_params->bio);
	return( 0 );   // PROD00013379: porting for RHEL 7.0
#else
    return original_io_routines->make_request_fn(io_params->queue, io_params->bio);
#endif
}

/**
 * @brief Our replacement make request function that receives all captured IOs.
 */
static int captured_queue_replacement_make_request_fn(struct request_queue *queue, struct bio* bio)
{
    captured_io_parameters_t io_params = {queue, bio};

    // We should never encounter linked bio requests.
    // C.F. RFX-153.
    BUG_ON(bio->bi_next != NULL);
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(bio->bi_bdev->bd_dev,
                                                                              &io_params,
                                                                              process_captured_queue_io,
                                                                              call_captured_queue_make_request_fn);
}

// See the ftd_dynamic_activation.h for the documentation.
/** @ingroup dynamic_activation_os */
const io_function_ptrs_t* ftd_dynamic_activation_get_replacement_routines(void)
{
    static const io_function_ptrs_t replacement_routines = {captured_queue_replacement_make_request_fn};
    
    return &replacement_routines;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
// Extract from http://lwn.net/Articles/303270/, which covers the API changes within 2.6.28:

// The handling of minor numbers has been changed, allowing disks to have an essentially unbounded number of partitions.
// The cost of this change is that minor numbers may be attached to a different major number, and they might not all be contiguous;
// for this reason, drivers must set the GENHD_FL_EXT_DEVT flag before the extended numbers will be used.
// See this article for more information on this change: http://lwn.net/Articles/290141/

/**
 * Starting with kernel 2.6.28, we should use disk_max_parts() to access the value of gendisk->minors.
 *
 * This is an implementation for older kernel versions.
 */
static inline int disk_max_parts(struct gendisk *disk)
{
    return disk->minors;
}

/**
 * Starting with kernel 2.6.28, the disk_partitionable() function exists and should be used to check if
 * a disk is partitionable.
 *
 * This is an implementation for older kernel versions.
 * NOTE: the function does not exist either on RHEL 7 (see addition below, PROD00013379)
 *
 * @bug We should really return a bool, but for some reason, this doesn't compile.
 */
static inline int disk_partitionable(struct gendisk *disk)
{
	return disk_max_parts(disk) > 1;
}

static inline partition_information_t get_partition_info(struct gendisk *bd_disk, dev_t containing_devno, int partno)
{
    partition_information_t info = { bd_disk, containing_devno, partno};
    BUG_ON(partno <= 0);
    return info;
}

static inline void put_partition_info(const partition_information_t* partition_info)
{
    // Nothing to do.
}

static inline struct hd_struct* partition_info_get_partition(const partition_information_t* partition_info)
{
    // Beware that the partition index 0 in the kernel API represents the containing device and index 1 represents the 1st partition.
    // Within the implementation, index 0 represents the 1st partition.  See add_partition() and delete_partition().
    return partition_info->disk->part[(partition_info->partno)-1];
}

static inline dev_t partition_info_get_devt(const partition_information_t* partition_info)
{
    return (partition_info->containing_devno) + (partition_info->partno);
}

#else
/***************************************************************************
 * The following code is for cases where !LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
 ***************************************************************************/

static inline partition_information_t get_partition_info(struct gendisk *bd_disk, dev_t containing_devno, int partno)
{
    partition_information_t partition_info = { bdget_disk(bd_disk, partno) };
    return partition_info;
}

static inline void put_partition_info(const partition_information_t* partition_info)
{
    bdput(partition_info->disk);
}

static inline struct hd_struct* partition_info_get_partition(const partition_information_t* partition_info)
{
    struct block_device* disk = partition_info->disk;
    struct hd_struct* partition = NULL;

    // It's possible that there doesn't exist a partition for a given index.
    // It's also possible that an existing partition hasn't been opened in which case, 
    // the partition information is left null and we can safely skip.
    if(disk != NULL && disk->bd_part != NULL)
    {
        partition = disk->bd_part;
    }
    
    return partition;
}

static inline dev_t partition_info_get_devt(const partition_information_t* partition_info)
{
    if(partition_info->disk == NULL)
    {
        return 0;
    }
    
    return partition_info->disk->bd_dev;
}

#endif // LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
// NOTE: function disk_partitionable() does not exist on RHEL 7, so it needs to be defined here (PROD00013379)
static inline int disk_partitionable(struct gendisk *disk)
{
	return disk_max_parts(disk) > 1;
}
#endif

/**
 * @brief Changes an IO request's offset and device members so that the IO now targets the captured (partition) device.
 *
 * In the case we have detected that a captured IO target one of our replicated device which is a partition, 
 * but that the IO has been remapped to the containing device, we need to map the IO back to the captured partition device
 * before processing it.
 *
 * @param devp           The captured device's information which will be used to remap the IO request.
 * @param io_params[in]  The IO request targeting the device containing a captured partition.
 * @param io_params[out] The IO request modified to now target the proper partition of the captured device.
 *
 */
static void map_io_to_captured_device(ftd_dev_t* devp, const captured_io_parameters_t *io_params)
{
    struct bio* bio = io_params->bio;
    bio->bi_bdev = devp->bd;
    bio->bi_sector -= devp->bd->bd_part->start_sect;
}

// See ftd_dynamic_activation_linux.h for the documentation.
/**
 * @internal Based on the code of linux's blk_partition_remap().
 */
void map_io_to_containing_device(struct bio* bio)
{
    struct block_device* bdev = bio->bi_bdev;
    
    if(bdev != bdev->bd_contains)
    {
        bio->bi_sector += bdev->bd_part->start_sect;
        bdev = bdev->bd_contains;
    }
}

/**
 * @todo Is it possible to distinguish an IO issued to a partition from one targeting the whole device?
 */
ftd_dev_t* captured_device_table_get_partition_io(dev_t dev, const captured_io_parameters_t *io_params)
{
    ftd_dev_t* bio_partition_devp = NULL;
    const struct bio* bio = io_params->bio;
    struct gendisk* bd_disk = bio->bi_bdev->bd_disk;

    BUG_ON(bio->bi_bdev->bd_dev != dev);

    // We totally skip processing requests that have a length of 0 sectors.
    if (disk_partitionable(bd_disk) && bio_sectors(bio))
    {
        int i = 1; // Partition index

        // For bdget_disk(), partition number 0 is the device representing the whole disk. (http://lxr.linux.no/#linux+v2.6.37/fs/block_dev.c#L756)
        // Real partitions are indexed starting from 1.
        // Since the check for the replication of the whole device must be performed before this function is called,
        // we can sefely skip verifying the whole disk partition 0.
        for(i = 1; i <= disk_max_parts(bd_disk); i++)
        {
            partition_information_t partition_info = get_partition_info(bd_disk, dev, i);
            struct hd_struct* partition = partition_info_get_partition(&partition_info);
            
            if(partition != NULL)
            {
                const sector_t start_sector_of_partition = partition->start_sect;
                const sector_t last_sector_of_partition = partition->start_sect + partition->nr_sects - 1;
                
                const sector_t first_bio_sector = bio->bi_sector;
                const sector_t last_bio_sector = bio->bi_sector + bio_sectors(bio) - 1;

                dev_t partition_dev = partition_info_get_devt(&partition_info);
                
                put_partition_info(&partition_info);

                // We figure that the bio is within the partition and should be handled only if
                // the complete range of the IO falls within the partition.
                if (first_bio_sector >= start_sector_of_partition &&
                    last_bio_sector <= last_sector_of_partition)
                {
                    bio_partition_devp = captured_device_table_get(partition_dev);

                    // We'll remap the io to the current partition only if this is a replicated partition.
                    if (bio_partition_devp)
                    {
                        map_io_to_captured_device(bio_partition_devp, io_params);
                    }
                    break; 
                }
                
                // If the request isn't fully within the partition, check if it partially falls within this replicated partition to issue warnings.
                if (first_bio_sector < start_sector_of_partition &&
                    last_bio_sector >= start_sector_of_partition &&
                    captured_device_table_get(partition_dev) != NULL)
                {
                    FTD_ERR(FTD_WRNLVL,
                            "Skipping the processing of a %s request overlapping the beginning of partition %d on %s. Has IO been sent to %s directly?",
                            (bio_data_dir(bio) == WRITE) ? "write" : "read",
                            i,
                            bd_disk->disk_name,
                            bd_disk->disk_name);
                }
                
                if (first_bio_sector <= last_sector_of_partition &&
                    last_bio_sector > last_sector_of_partition &&
                    captured_device_table_get(partition_dev) != NULL)
                {
                    FTD_ERR(FTD_WRNLVL,
                            "Skipping the processing of a %s request overlapping the end of partition %d on %s. Has IO been sent to %s directly?",
                            (bio_data_dir(bio) == WRITE) ? "write" : "read",
                            i,
                            bd_disk->disk_name,
                            bd_disk->disk_name);
                }
            }
        }
    }
 
    return bio_partition_devp;
}
