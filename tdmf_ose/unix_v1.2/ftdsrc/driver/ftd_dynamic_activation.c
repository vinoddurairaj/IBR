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
 * @brief  Implementation of the dynamic activation.
 *
 * This file actually implements a collection of APIs:
 *
 * - @ref swap_table
 * - @ref captured_device_table
 * - @ref dynamic_activation
 * - @ref captured_io_monitoring
 *
 * @note   It would make sense to move the @ref swap_table and @ref captured_device_table to their own file, 
 *         but the impact would probably be to loose the possibility to inline their calls.
 *
 * @author Bradley Musolff (original code from tdmf)
 * @author Martin Proulx (replicator adaptations and enhancements)
 *
 * @todo   In doing so (implement for linux), fix the currently buggy implementation of minor_t, major_t, getminor() and getmajor().
 *
 */

#include "ftd_dynamic_activation.h"
#include "ftd_dynamic_activation_os.h"
#include "ftd_kern_cproto.h"
#include "ftd_klog.h"
#include "ftd_ddi.h"
#include "ftd_all.h"
#include "ftd_hash_table.h"
#include "ftd_pending_ios_monitoring.h"

/**
 * @brief Used to track our major numbers
 *
 * This is needed so that we are able to easily know wether a request to a given device
 * was done through our dtc device or through a dynamically activated one.
 *
 */
ftd_devnum_t ftd_major_num = {0,0};

/********************************************************************************************************************//**
 *
 * @defgroup swap_table swap_table API
 *
 * The swap table's job is to associate the captured #swap_table_key_t with
 *
 * - The pointer to the original strategy/read/write routine.
 * - How many dtc devices are capturing the IO through our replacement routines.
 *
 * This table is used to know when we need to install or uninstall the ftd_dynamic_activation_get_replacement_routines().
 *
 * @internal The current implementation is based on the @ref hash_table.
 *
 ************************************************************************************************************************/

/**
 * @brief Structure used to bind together the platform specific swapped routine pointers and a usage count.
 *
 * @ingroup swap_table
 */
typedef struct swapentry {
    /** @brief Remembers the original routines. */
    io_function_ptrs_t original_routines;
    
    /** @brief Remembers how many dtc devices are capturing their IO by having replaced the original routines. */
    int usage_count;
} swap_entry_t;

/**
 * @brief The hash table used to implement the @ref swap_table.
 *
 * @ingroup swap_table
 */
static hash_table_t swap_table_hash_table;

/**
 * @brief Lock used to protect concurrent external accesses to the @ref swap_table.
 *
 * @pre swap_table_init() must have been previously called before using the lock.
 *
 * @todo Evaluate if there's too much contention on this lock especially by
 *       ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device()
 *       and ftd_dynamic_activation_captured_driver_generic_replacement_routine() and find a solution if so.
 *
 * @ingroup swap_table
 */
static ftd_lock_t swap_table_lock;

/** 
 * @brief Initializes the @ref swap_table.
 *
 * @return 0 if succesful, or a standard errno code otherwise.
 *
 * @ingroup swap_table
 */
static inline int swap_table_init(void)
{
    // We're expecting to only hold a handful of entries in the hash table, so we're not expecting
    // collisions even when using a relatively small amount of slots.
    static const hash_table_size_t hash_table_size = 64;

    ALLOC_LOCK(swap_table_lock, QNM " swap table api implementation lock.");

    return hash_table_init(hash_table_size, &swap_table_hash_table);
}

/** 
 * @brief Terminates the @ref swap_table.
 *
 * @note We currently do not walk through the hash table to delete any remaining entries.
 *
 * @ingroup swap_table
 */
static inline void swap_table_finish(void)
{
    hash_table_finish(&swap_table_hash_table);
    
    DEALLOC_LOCK(swap_table_lock);
}

/**
 * @brief Allocates space to store pointers to the original io routines for the given key.
 *
 * @attention The pointers need to be remembered permanently and cannot be forgotten through any API.
 *            This is to avoid possible problems that may otherwise arise if we are releasing a captured
 *            device while a replacement routine is already entered, possibly for IO on another non replicated
 *            device of the same driver.
 *
 * @param swap_key[in] The key associated to the routines that we'll soon be asked to remember.
 *
 * @pre There must not already be registered routines.
 * @post swap_table_set_original_io_routines must be called to register valid entries.
 *
 * @return 0 if successful.
 *         ENOMEM if we cannot allocate the memory needed to store the new value.
 *         EEXIST if the entry was already present.
 *
 * @bug  Realize that the memory allocated here will never be freed, even when the driver is unloaded, as there's no way to do so.
 *
 * @internal The idea of splitting the work done by swap_table_alloc_space_for_original_io_routines() and
 *           swap_table_set_original_io_routines in two is because one one side we need to atomically be able, using an
 *           interrupt context safe lock object, to swap the io routines and register them in the swap table.
 *           On the other side, we need to allocate memory, which we can't do reliably on all platforms while holding an
 *           lock.
 *
 *           An easier alternative would have been to simply obtain and register the original_io_routines before swapping them,
 *           but this sadly cannot be achieved on AIX as there's no way to obtain the current routines without replacing them.
 *
 * @note Must be called from user context, because of the memory allocations performed.
 *
 * @bug  Realize that the memory allocated here will never be freed, even when the driver is unloaded, as there's no way to do so.
 * 
 * @ingroup swap_table
 */
static int swap_table_alloc_space_for_original_io_routines(swap_table_key_t swap_key)
{
    int rc = 0;
    swap_entry_t* new_entry = kmem_zalloc(sizeof(swap_entry_t), KM_SLEEP);

    if (new_entry)
    {
        rc = hash_table_add(&swap_table_hash_table, (hash_table_key_t)swap_key, new_entry);

        if(rc != 0)
        {
            kmem_free(new_entry, sizeof(swap_entry_t));
        }
    }
    else
    {
        rc = ENOMEM;
    }
    return rc;
}

/**
 * @brief Stores pointers to the original io routines for the given key.
 *
 * @attention The pointers need to be remembered permanently and cannot be forgotten through any API.
 *            This is to avoid possible problems that may otherwise arise if we are releasing a captured
 *            device while a replacement routine is already entered, possibly for IO on another non replicated
 *            device of the same driver.
 *
 * @param swap_key[in] The key for the routines that we are asked to remember.
 * @param original_io_routines[in] The original routines which will be remembered.
 *
 * @pre The #swap_table_lock should probably be held in most circumstances.
 * @pre swap_table_alloc_space_for_original_io_routines() should have been previously called.
 * @post Following calls to swap_table_get_original_io_routines for the same key will return original_io_routines.
 *
 * @return 0 if successful.
 *         ENOENT if the entry does not exist.
 *
 * @ingroup swap_table
 */
static inline int swap_table_set_original_io_routines(swap_table_key_t swap_key, io_function_ptrs_t* original_io_routines)
{
    int rc = ENOENT;
    
    swap_entry_t* entry = hash_table_get(&swap_table_hash_table, (hash_table_key_t)swap_key);
    
    if(entry)
    {
        entry->original_routines = *original_io_routines;
        rc = 0;
    }

    return rc;
}

/**
 * @brief Reobtains a structure containing the pointers to the original io routines for the given key.
 *
 * @param swap_key[in] The swap key of the remembered routines which we are asked to obtain.
 *
 * @return A pointer to the remembered #io_function_ptrs_t structure containing pointers to the remembered io routines
 *         associated with the given key, or a pointer to NULL, if no routines were registered for this number.
 *
 * @pre The #swap_table_lock should probably be held in most circumstances.
 *  
 * @ingroup swap_table
 */ 
static inline const io_function_ptrs_t* swap_table_get_original_io_routines(swap_table_key_t swap_key)
{
    io_function_ptrs_t* original_routines = NULL;

    swap_entry_t* entry = hash_table_get(&swap_table_hash_table, (hash_table_key_t)swap_key);

    if(entry)
    {
        original_routines = &(entry->original_routines);
    }
   
    return original_routines;
}

/**
 * @brief Reobtains a structure containing the pointers to the original io routines for the given key, while
 *        automatically grabbing the lock.
 *
 * @param swap_key[in] The key of the remembered routines which we are asked to obtain.
 *
 * @return A pointer to the remembered #io_function_ptrs_t structure containing pointers to the remembered io routines
 *         associated with the given key, or a pointer to NULL, if no routines were registered for this number.
 *
 * @pre The #swap_table_lock should not already be held.
 *  
 * @ingroup swap_table
 */ 
static inline const io_function_ptrs_t* swap_table_get_original_io_routines_through_lock(swap_table_key_t swap_key)
{
    ftd_context_t context;
    const io_function_ptrs_t* original_io_routines = NULL;
    
    ACQUIRE_LOCK(swap_table_lock, context);
    {
        original_io_routines = swap_table_get_original_io_routines(swap_key);
    }
    RELEASE_LOCK(swap_table_lock, context);

    return original_io_routines;
}

/**
 * @brief Obtains the usage count of how many dtc devices are relying on captured IO for the given key.
 *
 * @param swap_key[in] The key for which we want to obtain the usage count.
 *
 * @return The usage count value.
 *         0 is returned if swap_table_set_original_io_routines() has never been called for the given key.
 *
 * @pre The #swap_table_lock should probably be held in most circumstances.
 *
 * @ingroup swap_table
 */
static inline unsigned int swap_table_get_usage_count(swap_table_key_t swap_key)
{
    unsigned int usage_count = 0;
    
    swap_entry_t* entry = hash_table_get(&swap_table_hash_table, (hash_table_key_t)swap_key);

    if(entry)
    {
        usage_count = entry->usage_count;
    }
    
    return usage_count;
}

/**
 * @brief Increments the usage count of how many dtc devices are relying on captured IO for the given key.
 *
 * @param swap_key[in] The key for which we increment the usage count.
 *
 * @return The incremented usage count value.
 *
 * @pre swap_table_set_original_io_routines() must have been called with key before.
 * @pre The #swap_table_lock should probably be held in most circumstances.
 *
 * @ingroup swap_table
 */
static inline unsigned int swap_table_increment_usage_count(swap_table_key_t swap_key)
{
    unsigned int new_usage_count = 0;
    
    swap_entry_t* entry = hash_table_get(&swap_table_hash_table, (hash_table_key_t)swap_key);

    if(entry)
    {
        new_usage_count = ++(entry->usage_count);
    }
    
    return new_usage_count;
}

/**
 * @brief Increments the usage count of how many dtc devices are relying on captured IO for both a block and char swap key.
 *
 * The char swap key is only dealt with if it is different than the block swap key.
 * This is a helper function to deal with the fact that on HP-UX, block and char swap keys can be different.
 *
 * @param block_swap_key[in] The block key for which we increment the usage count.
 * @param char__swap_key[in] The char key for which we increment the usage count.
 *
 * @pre swap_table_set_original_io_routines() must have been called with key before.
 * @pre The #swap_table_lock should probably be held in most circumstances.
 *
 * @ingroup swap_table
 */
static void swap_table_increment_block_and_swap_usage_count(swap_table_key_t block_swap_key, swap_table_key_t char_swap_key)
{

    swap_table_increment_usage_count(block_swap_key);
    
    if(block_swap_key != char_swap_key)
    {
        // On HP-UX, drivers may be assigned different block and char major device numbers.
        // For ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device() to function properly, the usage count must be
        // maintained for both major number in these cases.
        swap_table_increment_usage_count(char_swap_key);
    }
}

/**
 * @brief Decrements the usage count of how many dtc devices are relying on captured IO for the given key.
 *
 * @param swap_key[in] The key for which we decrement the usage count.
 *
 * @return The decremented usage count value.
 *
 * @pre swap_table_set_original_io_routines() must have been called with key before.
 * @pre The #swap_table_lock should probably be held in most circumstances.
 *
 * @ingroup swap_table
 */
static inline unsigned int swap_table_decrement_usage_count(swap_table_key_t swap_key)
{
    unsigned int new_usage_count = 0;
    
    swap_entry_t* entry = hash_table_get(&swap_table_hash_table, (hash_table_key_t)swap_key);

    if(entry)
    {
        new_usage_count = --(entry->usage_count);
    }
    
    return new_usage_count;
}


/********************************************************************************************************************//**
 *
 * @defgroup captured_device_table captured_device_table API
 *
 * The captured device table's job is to associate a captured device number with the associated logical device
 * through a pointer to the ftd_dev_t entry.
 *
 ************************************************************************************************************************/

/**
 * @brief The hash table used to implement the @ref captured_device_table.
 *
 * @ingroup captured_device_table
 */
static hash_table_t captured_device_table_hash_table;

/** 
 * @brief Initializes the implementation of the @ref captured_device_table.
 *
 * @return 0 if succesful, or a standard errno code otherwise.
 *
 * @ingroup captured_device_table
 */
static int captured_device_table_init(void)
{
    static const hash_table_size_t hash_table_size = 1024; // 1024 seems like an arbitrary value.
    return hash_table_init(hash_table_size, &captured_device_table_hash_table);
}

/** 
 * @brief Terminates the implementation of the @ref captured_device_table.
 *
 * @ingroup captured_device_table
 */
static void captured_device_table_finish(void)
{
    hash_table_finish(&captured_device_table_hash_table);
}

/** 
 *
 * @brief Add a device entry in the captured device table.
 *
 * @param devno The captured source device.
 * @param devp  A pointer to the associated dtc logical device's information.
 *
 * @return 0 if successful.
 *         ENOMEM if we cannot allocate the new entry,
 *         EEXIST if the entry was already present.
 *
 * @ingroup captured_device_table
 */
static int captured_device_table_add(dev_t devno, ftd_dev_t *devp)
{
    return hash_table_add(&captured_device_table_hash_table, devno, devp);
}

/** 
 *
 * @brief Unregisters a device entry in the captured device table.
 *
 * This makes subsequent searches of the device in the device table fail to find the device, but doesn't free the associated memory within the table.
 *
 * This is only needed by captured_io_monitoring_ensure_release_is_safe_and_init_release() so that the device can be forgotten about while holding
 * a spinlock, as freeing memory (calling captured_device_table_del()) isn't allowed on AIX.
 * 
 * @param devno The captured source device.
 *
 * @return 0 if successful.
 *         ENOENT if the devno wasn't previouly registered.
 * @ingroup captured_device_table
 */
static int captured_device_table_unregister(dev_t devno)
{
    return hash_table_update(&captured_device_table_hash_table, devno, NULL);
}

/**
 *
 * @brief  Delete a device entry in the captured device table.
 *
 * @param devno The captured source device for which we want to forget about its dtc logical device's information.
 *
 * @return 0 If the deletion was succesful, ENOENT if the devno wasn't previouly registered.
 *
 * @pre captured_device_table_init() must have previously been called.
 *
 * @ingroup captured_device_table
 */
static int captured_device_table_del (dev_t devno)
{
    return hash_table_del(&captured_device_table_hash_table, devno);
}

/**
 *
 * @brief  Obtains the dtc logical information associated with the captured device.
 *
 * @param devno The captured source device for which we want to obtain the associated dtc logical device information.
 *
 * @return The pointer to associated dtc logical device information if found, NULL otherwise.
 *
 * @pre captured_device_table_init() must have previously been called.
 *
 * @ingroup captured_device_table
 */
ftd_dev_t* captured_device_table_get (dev_t devno)
{
    return hash_table_get(&captured_device_table_hash_table, devno);
}

/********************************************************************************************************************//**
 *
 * @defgroup captured_io_monitoring Captured IO monitoring API.
 *
 * An API used to track how many captured IO are currently being replicated.
 *
 * This is currently needed to ensure that the dtc device associated
 * with the captured device cannot be stopped while captured IO is going by.
 *
 * @internal The devp->flags FTD_BITX_STOP_IN_PROGRESS bit could have been reused instead of
 *           FTD_BITX_CAPTURED_DEVICE_RELEASE_INITIATED, but this would have required to break
 *           the dynamic activation code encapsulation.
 *
 ************************************************************************************************************************/

/**
 * @brief Lock used to ensure that deciding to process a captured IO and initializing the capture release will both be done atomically.
 *
 * @ingroup captured_io_monitoring
 */
static ftd_lock_t release_init_lock;

/** 
 * @brief Initializes the @ref captured_io_monitoring.
 *
 * @ingroup captured_io_monitoring
 */
static inline void captured_io_monitoring_init(void)
{
    ALLOC_LOCK(release_init_lock, QNM " captured io monitoring api implementation lock.");
}

/** 
 * @brief Terminates the @ref captured_io_monitoring.
 *
 * @ingroup captured_io_monitoring
 */
static inline void captured_io_monitoring_finish(void)
{
    DEALLOC_LOCK(release_init_lock);
}

/**
 * @brief Tests if the device is a captured device and increments the number of requests being processed if so.
 * 
 *        The device isn't considered captured anymore if its release has been initiated by
 *        captured_io_monitoring_ensure_release_is_safe_and_init_release().
 *
 * @param devno    The device number of the device we are lookiing up.
 * @param io_params The additional IO parameters asociated with a request. Needed on Linux only.
 *
 * @return The pointer to the associated dtc logical device information if the device is captured, NULL otherwise.
 *
 * @post If the device is captured, captured_io_monitoring_decrement_requests_being_processed() must eventually be called.
 */
ftd_dev_t* captured_io_monitoring_increment_requests_being_processed_if_captured_device(dev_t devno, const captured_io_parameters_t* io_params)
{
    ftd_dev_t* devp = NULL;

    ftd_context_t release_init_context;

    // Atomically checking the presence of the device in the device table and incrementing the local_disk_captured_requests_being_processed counter
    // is required in order to make sure that a concurrent call to captured_io_monitoring_ensure_release_is_safe_and_init_release() will either
    // make sure that an already captured IO will complete before the release is safe or that an IO that hasn't been looked up yet will not be captured.
    ACQUIRE_LOCK(release_init_lock, release_init_context);
    {
        // 1st check if we're dealing with a captured device.
        devp = captured_device_table_get(devno);

#ifdef linux
        // On linux, any IO targeting a partition will have already been remapped to the containing device before we receive it.
        // In the case we replicate a partition but not the containing device, we need to find out if the remapped IO was actually
        // targetting a partition and map it back to the captured partition device if so.
        // See the documentation of get_device_of_partition() for all details.
        
        if (devp == NULL)
        {
            devp = captured_device_table_get_partition_io(devno, io_params);
        }
#endif /* linux */
        
        if(devp)
        {
            int release_initiated = 0;
            ftd_context_t device_context;
            ACQUIRE_LOCK(devp->lock, device_context);
            {
                // We'll only replicate the IO if the release of the said captured device hasn't been initiated yet.
                if(devp->flags & FTD_CAPTURED_DEVICE_RELEASE_INITIATED)
                {
                    release_initiated = 1;
                }
                else
                {
                    devp->local_disk_captured_requests_being_processed++;
                }
            }
            RELEASE_LOCK(devp->lock, device_context);

            if (release_initiated)
            {
                devp = NULL;
            }
        }
    }
    RELEASE_LOCK(release_init_lock, release_init_context);

    return devp;
}

/**
 * @brief Decreases the number of requests being processed for a given device.
 *
 * @param devp[in] A pointer to the dtc device for which we will decrement the number of requests being processed.
 *
 * @pre captured_io_monitoring_increment_requests_being_processed_if_captured_device() must have been previously called and returned
 *      the given devp.
 *
 * @ingroup captured_io_monitoring
 */
static void captured_io_monitoring_decrement_requests_being_processed(ftd_dev_t* devp)
{
    ftd_context_t context;
    ACQUIRE_LOCK(devp->lock, context);
    {
        devp->local_disk_captured_requests_being_processed--;
    }
    RELEASE_LOCK(devp->lock, context);
}

/**
 * @brief Checks if the conditions are safe to release the given captured device.
 *
 * It is considered safe to release a captured device when no IO is currently being processed by it.
 *
 * @param devp[in] The captured device we are checking the conditions for.
 *
 * @return 1 if safe, 0 otherwise.
 *
 * @pre The devp->lock and release_init_lock should be held.
 * @pre captured_io_monitoring_init() must have been called.
 *
 * @internal We actually need to look at two different things in order to find out if a captured device
 *           is currently busy or not.
 *
 *           - Are there any IOs that have just been captured and are currently being submitted for this device?
 *           - Are there any IOs that have already been submitted for this device but haven't completed yet?
 *
 *           In the case of block strategy function IOs, the 1st question is answered by looking at a device's
 *           local_disk_captured_requests_being_processed counter and the 2nd one is answered by looking at
 *           the device's pending_ios counter.
 *
 *           Note that the pending_ios counter isn't incremented at the very beginning of the ftd_strategy() function.
 *           This has no consequences for our usage because we're already tracking the fact that a captured IO is being
 *           submitted when calling ftd_strategy() from captured_driver_replacement_strategy().
 *           
 *           Be aware that the location where the device's pending_ios counter is decremented is crucial, as it is possible
 *           for a device to be deleted as soon as this counter reaches 0.
 *
 *           In the case of raw read/writes, both questions can be answered only by looking at the
 *           local_disk_captured_requests_being_processed counter.
 *           This is because ftd_rw() relies on physio which waits for IO completion before returning.
 *
 *           Of course, the pending_ios counter will also end up being maintained, but isn't otherwise needed for this case.
 *
 * @ingroup captured_io_monitoring
 */
static inline int captured_io_monitoring_is_release_safe(ftd_dev_t* devp)
{
    return (devp->local_disk_captured_requests_being_processed == 0) && (pending_ios_monitoring_get_ios_pending(devp) == 0);
}

/**
 * @copydoc ftd_dynamic_activation_init_captured_device_release()
 * @ingroup captured_io_monitoring
 */ 
static int captured_io_monitoring_ensure_release_is_safe_and_init_release(ftd_dev_t* devp)
{
    ftd_context_t device_context;
    ftd_context_t release_init_context;
    int rc = 0;
    int io_captured = 0;
    int safe = 0;
    unsigned int time_waited_us = 0;
    
    ACQUIRE_LOCK(devp->lock, device_context);
    {
        io_captured = (devp->flags & FTD_LOCAL_DISK_IO_CAPTURED);
    }
    RELEASE_LOCK(devp->lock, device_context);
    
    if(!io_captured)
    {
        return ENOENT;
    }

    if (captured_device_table_get(devp->localbdisk) == NULL &&
        captured_device_table_get(devp->localcdisk) == NULL)
    {
        // Then we've already been here and need not do anything, but we'll still log a warning.
        // This was most likely caused by someone hitting CTRL-C on dtcstop.  See WR PROD00008341.
        FTD_ERR(FTD_WRNLVL, "captured_io_monitoring_ensure_release_is_safe_and_init_release: Device %d:%d has already been unregistered from the device table.",
                getmajor(devp->bdev),
                getminor(devp->bdev));
        return 0;
    }
        
    // We 1st signal any new IO that the release has been initiated and immediately release the lock so that
    // IOs previously treated can terminate.
    // Since the advent of the release_init_lock, this flag only avoids additional IOs captured while we are waiting for safe conditions
    // to add to the congestion by being replicated.  See comments below around captured_device_table_del() explaining how this flag
    // might eventually be replaced.
    ACQUIRE_LOCK(devp->lock, device_context);
    {
        devp->flags |= FTD_CAPTURED_DEVICE_RELEASE_INITIATED;
    }
    RELEASE_LOCK(devp->lock, device_context);

    // We then wait for the proper conditions to release the captured device.
try_again:
    ACQUIRE_LOCK(release_init_lock, release_init_context);
    {
        ACQUIRE_LOCK(devp->lock, device_context);
        {
            safe = captured_io_monitoring_is_release_safe(devp);
        }
        RELEASE_LOCK(devp->lock, device_context);

        // Atomically detecting safe conditions and removing the device from the captured device table is required in order to make sure that anything
        // entering the captured routine will always go fully through captured_io_monitoring_increment_requests_being_processed_if_captured_device()
        // and atomically be replicated or just forwarded to the original IO routine.
        if (safe)
        {
            // Note that we cannot delete the device from the table unless the conditions are safe (I.E no IO is already being replicated for this device)
            // because the table is still relied upon at the entry of ftd_strategy().  If this is ever changed, than maybe the presence in the table could
            // replace the FTD_CAPTURED_DEVICE_RELEASE_INITIATED mechanism.
            rc = captured_device_table_unregister(devp->localbdisk);

            if (rc)
            {
                FTD_ERR(FTD_WRNLVL, "captured_io_monitoring_ensure_release_is_safe_and_init_release: captured_device_table_unregister() failed: %d.", rc);
            }
            
            if(devp->localbdisk != devp->localcdisk)
            {
                int unregister_char_rc = captured_device_table_unregister(devp->localcdisk);
                
                if (unregister_char_rc)
                {
                    FTD_ERR(FTD_WRNLVL, "captured_io_monitoring_ensure_release_is_safe_and_init_release: captured_device_table_unregister() failed for the char device: %d.",
                            unregister_char_rc);
                    
                    if (rc == 0)
                    {
                        rc = unregister_char_rc;
                    }
                }
            }
        }   
    }
    RELEASE_LOCK(release_init_lock, release_init_context);

    if (safe && rc == 0)
    {
        // Now that no lock is being held, take care of really deleting the entries from the table.
        rc = captured_device_table_del(devp->localbdisk);
        
        if (rc)
        {
            FTD_ERR(FTD_WRNLVL, "captured_io_monitoring_ensure_release_is_safe_and_init_release: captured_device_table_del() failed: %d.", rc);
        }
        
        if(devp->localbdisk != devp->localcdisk)
        {
            int del_char_rc = captured_device_table_del(devp->localcdisk);
            
            if (del_char_rc)
            {
                FTD_ERR(FTD_WRNLVL, "captured_io_monitoring_ensure_release_is_safe_and_init_release: captured_device_table_del() failed for the char device: %d.",
                        del_char_rc);
                
                if (rc == 0)
                {
                    rc = del_char_rc;
                }
            }
        }
    }   
    
    if(!safe)
    {
        static const unsigned int timeout_us = 5000000; // 5 secs.
        if(time_waited_us > timeout_us)
        {
            // Realize that since FTD_CAPTURED_DEVICE_RELEASE_INITIATED has been set, we already do not replicate any
            // new incoming IOs.
            // Because any new IO since this flag was set hasn't been replicated and was lost, we just have to live
            // with the fact that we cannot totally recover from this error and transparently restore the IO captures.
            rc = EBUSY;
        }
        else
        {
            // Using a shorter period is probably useless on most platforms as it appears that the various implementations
            // of FTD_DELAY cannot delay for very small periods anyways.
            // This was clearly noticed on Solaris, where the actual timeout period was noticed to be much longer than the
            // expected timeout value with shorter delay periods.
            static const unsigned int delay_period_us = 10000; // 1/100th of a second.
                
            FTD_DELAY(delay_period_us);
            time_waited_us += delay_period_us;
            goto try_again;
        }
    }
    
    return rc;
}

/********************************************************************************************************************//**
 *
 * @defgroup dynamic_activation dynamic activation API.
 *
 * The API needed to make the dynamic activation happen.
 * Definition of some vocabulary:
 *
 * - dynamic activation  The business wording for the ability to replicate IO targetting existing, non dtc devices.
 *                       The value added is that none of the existing applications need to be reconfigured or restarted
 *                       in order to initiate the replication.
 * - captured driver     A driver (major device) for which all IO requests have been redirected to us.
 * - captured device     A device on which we have installed a mechanism enabling us to see all IO requests targetting it.
 * - original routines   The block IO strategy and char read and write routines that were called by the OS for the captured device
 *                       before we captured them.
 *
 * The dynamic activation implementation is mostly encapsulated within the @ref ftd_dynamic_activation.c file.
 * Only the implemementation of the OS specific ftd_dynamic_activation_swap_io_routines() is within external OS specific files.
 *
 * The core driver makes a relatively simple usage of this API.  To find out how this API is meant to be used, please refer to
 * the the public functions' documentation and search for their usage within the driver.
 *
 * From userspace, the replicated devices are captured and released through these IO controls:
 * FTD_CAPTURE_SOURCE_DEVICE_IO and FTD_RELEASE_CAPTURED_SOURCE_DEVICE_IO.
 *
 * @sa https://w3.tap.ibm.com/w3ki/download/attachments/3246702/driver+-+dynamic+activation+changes.uml for diagrams
 *     used in a briefing on the dynamic activation subject.
 *
 ************************************************************************************************************************/

/**
 * @brief Sets the mechanism up in order to capture the IO to the given device if not already done.
 *
 * A device is captured by calling ftd_dynamic_activation_swap_io_routines().
 * 
 * @param block_swap_key The block device key for which we will swap the routines if not already done.
 * @param block_devno    The block device number for which we will swap the routines if not already done.
 * @param char_swap_key  The char device key for which we will swap the routines if not already done.
 *
 * @note HP-UX does have different major numbers for both the block and character devices.
 *       It is for this platform that we must pass both block and char device keys.
 *
 * @note AIX's swap key is technically the major device number, but the devswchg() API does require the
 *       full block device identifier.  It is for this reason only that we also need the block_devno parameter.
 *
 * @return 0 if successful or EACCESS if the call to ftd_dynamic_activation_swap_io_routines() failed.
 *
 * @ingroup dynamic_activation
 */
FTD_PRIVATE
int install_capture_routines_if_needed(swap_table_key_t block_swap_key, dev_t block_devno, swap_table_key_t char_swap_key)
{
    int rc = 0;
    ftd_context_t context;
    
    // Proper locking is required in order to avoid problems in the case multiple
    // logical devices are started in parallel.
    // The full solution is implemented in the consolidated driver, but for the mean-time, we'll leave opened
    // the tiny potential for problems when multiple instances of dtcstart/dtcstop are run in parallel.
    // The odds of this happening is negligible and we can always have some control over it.
    //ACQUIRE_LOCK(swap_table_lock, context);
    {
        // Verifying the usage count for only the block_swap_key device is sufficient on HP-UX, even if the block and char
        // major device numbers may be different.
        // In the case that both numbers are different, they are still tied to a single driver, so the respective
        // block and char usage count have to be the same.
        if (swap_table_get_usage_count(block_swap_key) == 0)
        {
            rc = swap_table_alloc_space_for_original_io_routines(block_swap_key);
            
            if (rc == EEXIST)
            {
                // Be aware that since the original io routines are never forgotten,
                // we may end up setting the original io routines to values we already know
                // in some cases.
                // This is not an actual error.
                rc = 0;
            }

            if (rc == 0 &&
                block_swap_key != char_swap_key)
            {
                // On HP-UX, drivers may be assigned different block and char major device numbers.
                // We also need to remember the io routines that have been swapped for the char driver.
                rc = swap_table_alloc_space_for_original_io_routines(char_swap_key);
                
                if (rc == EEXIST)
                {
                    // Be aware that since the original io routines are never forgotten,
                    // we may end up setting the original io routines to values we already know
                    // in some cases.
                    // This is not an actual error.
                    rc = 0;
                }
            }

            if (rc == 0)
            {
                // This lock will not prevent against concurrent instances of dtcstart, but ensures that IOs seen while we are starting
                // will not cause problems as long as we swap + set + increment the usage count all while we're holding it.
                // We used to increment the usage count out of the lock and that caused problem described in WR PROD00008515.
                ACQUIRE_LOCK(swap_table_lock, context);
                {
                    io_function_ptrs_t original_routines = ftd_dynamic_activation_swap_io_routines(block_swap_key,
                                                                                                   block_devno,
                                                                                                   char_swap_key,
                                                                                                   ftd_dynamic_activation_get_replacement_routines());

#ifdef linux
                    if(original_routines.make_request_fn)
#else
                    if(original_routines.strategy &&
                       original_routines.read &&
                       original_routines.write)
#endif /* #ifdef linux */
                    {
                        FTD_ERR (FTD_NTCLVL, "Dynamic activation: now capturing IO of " SWAP_KEYS_DESCRIPTION ".",
                                 block_swap_key, char_swap_key);
                        rc = swap_table_set_original_io_routines(block_swap_key, &original_routines);
                        
                        if (rc == 0 &&
                            block_swap_key != char_swap_key)
                        {
                            // On HP-UX, drivers may be assigned different block and char major device numbers.
                            // We also need to remember the io routines that have been swapped for the char driver.
                            rc = swap_table_set_original_io_routines(char_swap_key, &original_routines);
                        }  

                        if (rc == 0)
                        {
                            // We count the usage so we know when we can swap out safely
                            swap_table_increment_block_and_swap_usage_count(block_swap_key, char_swap_key);
                        }
                    }
                    else
                    {
                        FTD_ERR(FTD_WRNLVL, "install_capture_routines_if_needed: ftd_dynamic_activation_swap_io_routines failed.");
                        rc = EACCES;
                    }
                }
                RELEASE_LOCK(swap_table_lock, context);
            }
        }
        else
        {
            // Already swapped, just need to increment the usage counts so we know when we can swap out safely.
            swap_table_increment_block_and_swap_usage_count(block_swap_key, char_swap_key);
        }
    }
    //RELEASE_LOCK(swap_table_lock, context);
    
    return rc;
}

/**
 * @brief Takes care of releasing the captured device if no one still needs it to be captured.
 *
 * We rely on the @ref swap_table usage count to decide if we can release the device or not.
 *
 * @pre install_capture_routines_if_needed() with the same dev should have been previously called.
 *
 * @param block_dev   The block device key for which we will unswap the routines if needed.
 * @param block_devno The block device number for which we will unswap the routines if needed.
 * @param char_dev    The char device key for which we will unswap the routines if needed.
 *
 * @note HP-UX does have different major numbers for both the block and character devices.
 *       It is for this platform that we must pass both block and char device keys.
 *
 * @note AIX's swap key is technically the major device number, but the devswchg() API does require the
 *       full block device identifier.  It is for this reason only that we also need the block_devno parameter.
 *
 * @return 0 if successful, or ENOENT if there was no strategy captured for the device.
 *
 * @ingroup dynamic_activation
 */
FTD_PRIVATE
int uninstall_capture_routines_if_needed(swap_table_key_t block_swap_key, dev_t block_devno, swap_table_key_t char_swap_key)
{
    ftd_context_t context;
    
    // Proper locking is required in order to avoid problems in the case multiple
    // logical devices are stopped in parallel.
    ACQUIRE_LOCK(swap_table_lock, context);
    {
        if(swap_table_get_usage_count(block_swap_key) == 0)
        {
            return ENOENT;
        }
        
        if(swap_table_decrement_usage_count(block_swap_key) == 0)
        {
            const io_function_ptrs_t* original_io_routines = swap_table_get_original_io_routines(block_swap_key);

            if(block_swap_key != char_swap_key)
            {
                // On HP-UX, drivers may be assigned different block and char major device numbers.
                // For ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device() to function properly, the usage count must be
                // maintained for both major number in these cases.
                swap_table_decrement_usage_count(char_swap_key);
            }
            
            ftd_dynamic_activation_swap_io_routines(block_swap_key, block_devno, char_swap_key, original_io_routines);
            FTD_ERR (FTD_NTCLVL, "Dynamic activation: stopped capturing IO of " SWAP_KEYS_DESCRIPTION ".",
                     block_swap_key, char_swap_key);
            
            // We cannot ever forget the original io routines values in case that a captured_driver_replacement_strategy/read/write
            // routine was already entered before we end up being called in parallel.
        }
    }
    RELEASE_LOCK(swap_table_lock, context);
    
    return 0;
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
ftd_dev_t* ftd_dynamic_activation_get_replication_device_structure(dev_t devno)
{
    /* if this came in through the dtc device, get the pointer using ddi_get_soft_state. */
    if (ISDTCDEV(devno)) {
       minor_t minor = getminor(devno);
       return ddi_get_soft_state(ftd_dev_state, U_2_S32(minor));
    } else {
       return captured_device_table_get(devno);
    }
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
int ftd_dynamic_activation_init(dev_t dev)
{
    int rc = captured_device_table_init();

    if (rc == 0)
    {
        rc = swap_table_init();

        if (rc == 0)
        {
            captured_io_monitoring_init();
            
            // The flag parameter is unused on all implementations.
            ftd_ctl_get_device_nums(dev, (ftd_intptr_t)&ftd_major_num, 0);
        }
    }

    return rc;
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
void ftd_dynamic_activation_finish(void)
{
    captured_device_table_finish();
    swap_table_finish();
    captured_io_monitoring_finish();
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
int ftd_dynamic_activation_capture_device(ftd_dev_t *devp)
{
    // The reason to register the device in the table before installing the capture routines
    // is to avoid IO being sent to the capture routines before the captured_device_table holds the entry.
    int rc = captured_device_table_add(devp->localbdisk, devp);

    if(rc)
    {
        FTD_ERR(FTD_WRNLVL, "dynamic_activation_capture_device: captured_device_table_add() failed: %d.", rc);
    }

    if(rc == 0 &&
       devp->localbdisk != devp->localcdisk)
    {
        // On HP-UX, drivers may be assigned different block and char major device numbers,
        // so we also need to register the char device as a captured device.
        rc = captured_device_table_add(devp->localcdisk, devp);

        if(rc)
        {
            FTD_ERR(FTD_WRNLVL, "dynamic_activation_capture_device: captured_device_table_add() failed for the char device: %d.", rc);
            captured_device_table_del(devp->localbdisk);
        }
    }
    
    if (rc == 0)
    {
        ftd_context_t context;
        swap_table_key_t block_swap_key = 0;
        swap_table_key_t char_swap_key = 0;
        ftd_dynamic_activation_get_swap_keys_from_replication_device(devp, &block_swap_key, &char_swap_key);

        rc = install_capture_routines_if_needed(block_swap_key, devp->localbdisk, char_swap_key);
        if (rc)
        {
            FTD_ERR(FTD_WRNLVL, "dynamic_activation_capture_device: install_capture_routines_if_needed() failed: %d.", rc);
            captured_device_table_del(devp->localbdisk);

            if(devp->localbdisk != devp->localcdisk)
            {
                captured_device_table_del(devp->localcdisk);
            }
        }
        ACQUIRE_LOCK(devp->lock, context);
        devp->flags |= FTD_LOCAL_DISK_IO_CAPTURED;
        RELEASE_LOCK(devp->lock, context);
    }
    
    return rc;
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
int ftd_dynamic_activation_init_captured_device_release(ftd_dev_t *devp)
{
    return captured_io_monitoring_ensure_release_is_safe_and_init_release(devp);
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
int ftd_dynamic_activation_release_captured_device(ftd_dev_t *devp)
{
    int rc = 0;
    swap_table_key_t block_swap_key = 0;
    swap_table_key_t char_swap_key = 0;

    ftd_dynamic_activation_get_swap_keys_from_replication_device(devp, &block_swap_key, &char_swap_key);

    rc = uninstall_capture_routines_if_needed(block_swap_key, devp->localbdisk, char_swap_key);
    
    if (rc)
    {
        FTD_ERR(FTD_WRNLVL,
                "ftd_dynamic_activation_release_captured_device: uninstall_capture_routines_if_needed() failed: %d.",
                rc);
    }
    else
    {
        // Regardless of any del_block/char_rc errors, if we were able to uninstall the capture routines, we're not capturing the io anymore.
        ftd_context_t context;
        ACQUIRE_LOCK(devp->lock, context);
        devp->flags &= ~FTD_LOCAL_DISK_IO_CAPTURED;
        RELEASE_LOCK(devp->lock, context);
    }
    
    return rc;
}

// See the .h for the documentation.
/** @ingroup dynamic_activation */
FTD_PUBLIC
int ftd_dynamic_activation_pre_device_delete_capture_release_check(ftd_dev_t *devp)
{
    int rc = 0;
    int io_captured = 0;
    int release_initiated = 0;
    ftd_context_t context;
    
    ACQUIRE_LOCK(devp->lock, context);
    {
        io_captured = (devp->flags & FTD_LOCAL_DISK_IO_CAPTURED);
        release_initiated = (devp->flags & FTD_CAPTURED_DEVICE_RELEASE_INITIATED);
        
    }
    RELEASE_LOCK(devp->lock, context);
  
    if (io_captured)
    {
        int need_to_init_release = FALSE;
        FTD_ERR(FTD_WRNLVL, "ftd_dynamic_activation_pre_device_delete_capture_release_check: Device %d:%d from group %d is still capturing source device %d:%d. Trying to release it.",
                getmajor(devp->bdev),
                getminor(devp->bdev),
                getminor(devp->lgp->dev) & ~FTD_LGFLAG,
                getmajor(devp->localbdisk),
                getminor(devp->localbdisk));

        if(!release_initiated)
        {
            FTD_ERR(FTD_WRNLVL, "ftd_dynamic_activation_pre_device_delete_capture_release_check: Release has never been initiated. Initiating it.");
            need_to_init_release = TRUE;
        }
        else   
        {
            if(captured_device_table_get(devp->localbdisk) == NULL)
            {
                FTD_ERR(FTD_WRNLVL, "ftd_dynamic_activation_pre_device_delete_capture_release_check: Release has already been initiated successfully.");
            }
            else
            {
                FTD_ERR(FTD_WRNLVL, "ftd_dynamic_activation_pre_device_delete_capture_release_check: Release has already been initiated but unsuccessfully. Trying again.");
                need_to_init_release = TRUE;
            }
        }
            
        if (need_to_init_release)
        {
            rc = ftd_dynamic_activation_init_captured_device_release(devp);
            
            if (rc)
            {
                FTD_ERR(FTD_WRNLVL,
                        "ftd_dynamic_activation_pre_device_delete_capture_release_check: ftd_dynamic_activation_init_captured_device_release() failed: %d",
                        rc);
            }
        }
            
        if (rc == 0)
        {
            rc = ftd_dynamic_activation_release_captured_device(devp);
            
            if(rc != 0)
            {
                FTD_ERR(FTD_WRNLVL, "ftd_dynamic_activation_pre_device_delete_capture_release_check: ftd_dynamic_activation_release_captured_device() failed: %d", rc);
            }
        }
    }
    
    return rc;
}

#ifdef linux
  #ifndef WARN_ON_ONCE
  #define WARN_ON_ONCE(cond) WARN_ON(cond)
  #endif
#endif

/**
 * @brief Wrapper that works around the fact that the strategy function does not have a return value on all platforms.
 */
static inline int call_strategy_and_ensure_return_value(const io_function_ptrs_t* original_io_routines, struct buf *bp)
{
    int rc = 0;
#if defined(_AIX) || defined (HPUX)
    original_io_routines->strategy(bp);
#elif defined(linux)
    struct block_device* original_target_device = NULL;
    map_io_to_containing_device(bp);

    original_target_device = bp->bi_bdev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    original_io_routines->make_request_fn(bdev_get_queue(bp->bi_bdev), bp);  // PROD00013379: porting for RHEL 7.0
	rc = 0;
#else
    rc = original_io_routines->make_request_fn(bdev_get_queue(bp->bi_bdev), bp);
#endif
    // On linux (prior to RHEL 7), a make_request_fn() is allowed to modify the bio to be redirected to another device and to return 1, asking the
    // bio to be submited again. See the sources of generic_make_request() and ldd3 info about "stacking" drivers.
    // According to the information seen (ldd3 + sources), it seems a safe assumption that when this mechanism is used,
    // it implies that the request will have been modified to target a different device, in which case it is safe to resubmit
    // from scratch through generic_make_request().
    // If we ever encounter a case where we must resubmit to the same device, we'll log the fact in order to have a clue
    // if any problems related to resubmiting through generic_make_request() on the same device ever shows up for unforeseen cases.
	// NOTE: on RHEL 7, make_request_fn() returns void.
    if(rc)
    {
        WARN_ON_ONCE(bp->bi_bdev == original_target_device);
        generic_make_request(bp);
        rc = 0;
    }
    
#else
    rc = original_io_routines->strategy(bp);
#endif
    return rc;
}

// See the .h for the documentation.
/**
 * @ingroup dynamic_activation
 *
 * @internal  We cannot be holding the swap_table_lock while we are forwarding the IO, regardless of the mechanism used.
 *            Doing so opens the door to a recursive usage of the #swap_table_lock which was seen (WR 42836) when
 *            a captured virtual volume manager forwards its io to another captured driver.
 *            This is because the captured_driver_replacement_strategy() routine of the lower level captured driver also
 *            needs to grab the #swap_table_lock.
 *
 * @bug       Our current implementation relies on proper configuration in order to make sure that nothing goes wrong
 *            if other groups are started and stopped simultaneously.  See comments within the code explaining the
 *            repercussions of having to release the #swap_table_lock.
 *            The current implementation also needs to be reworked if we ever make it possible to dynamically capture
 *            the IOs of a device without having to stop and restart its replication group. 
 *
 * @todo      Find a way to be 100% safe, even in the misconfiguration situations described in the code's comments
 *            related to the repercussions of having to release the #swap_table_lock before forwarding the IO.
 *
 */
FTD_PUBLIC
int ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device(struct buf *bp)
{
    int rc;
    ftd_context_t context;
    const io_function_ptrs_t* original_io_routines = NULL;    

    ACQUIRE_LOCK(swap_table_lock, context);
    {
        swap_table_key_t swap_key = ftd_dynamic_activation_get_swap_key_from_io_request(bp);
        
        // Check if the io is targetting a captured device.
        if (swap_table_get_usage_count(swap_key) > 0)
        {
            // The request is for a captured device.
            // We must directly call the strategy function, as calling it through BDEVSTRAT would end calling
            // (our) captured_driver_replacement_strategy() again.
            original_io_routines = swap_table_get_original_io_routines(swap_key);
        }
    }
    RELEASE_LOCK(swap_table_lock, context);

    // Since we cannot be holding any lock while forwarding the IO, there is always a chance that the device for which
    // we are initiating the IO gets captured/uncaptured from now on, now that we have released the lock.
    // This can happen if replication groups are started stopped at the same time the current IO is being processed.

    // Examples:
    // -) We're replicating IO for a device in a non dynamically activated group and an other dynamically replicated
    //    group with devices managed by the same driver is started or stopped.
    // -) We are being called from ftd_flush_lrdb() and a group with a source on the same device as the pstore
    //    is being started or stopped simultaneously.

    // This opens the door to initiating the IO using a different mechanism than we would have otherwise chosen.
    // Two scenarios are possible.  Here we demonstrate that no harm is normally caused in both of them.
    //
    // -) captured -> uncaptured driver:
    //
    //    We will incorrectly call the original_io_routine->strategy() rather than BDEVSTRAT.
    //    For the now non captured device, BDEVSTRAT will end up calling the original strategy routine as well.
    //    The behavior is thus equivalent, but goes through a slightly different path.  No harm can be done.
    //
    // -) uncaptured -> captured driver:
    //
    //    We will incorrectly call BDEVSTRAT rather than the original_io_routine->strategy().
    //    Because the device is now captured, our captured_driver_replacement_strategy() will end up being called.
    //    One crucial fact to realize is that since the device for which we are forwarding the IO wasn't previously captured,
    //    we cannot currently be processing the IO of a captured device.
    //    Because of this, the captured_driver_replacement_strategy() will end up forwarding the IO
    //    by calling the original_io_routine->strategy().
    //    The behavior is thus equivalent, but goes through a slightly different path.  No harm can be done.
    // 
    //    Be warned that these deductions will not hold anymore in the future if we ever make it possible to
    //    dynamically capture a device without stopping and restarting its group.  If we ever do so, it would
    //    be possible to capture the device for which we are currently processing the IO, which would make it possible
    //    to call BDEVSTRAT for a captured device.  See the other comments about the infinite recursion that happens in
    //    this case.
    //
    //    Be also warned that improper configurations may make it possible as well to capture a device for which we
    //    are currently processing the IO when starting another group.
    //    Using the same source device in two groups is certainly such a misconfiguration scenario.
    //    Another possibility is to configure a device as both the pstore and source device in two different groups.
    //
    //    We currently have to live with these misconfiguration risks until a viable solution is found.
    //    Realize also that probably many other things will fail as well in these circumstances.
    
    if(original_io_routines)
    {
        rc = call_strategy_and_ensure_return_value(original_io_routines, bp);
    }
    else
    {
        // The request is targetting a non captured device.
        // We can safely initiate the IO using BDEVSTRAT.
        rc = BDEVSTRAT(bp);
    }  
    
    return rc;
}

// See the .h for the API documentation.
/**
 * @ingroup dynamic_activation_os
 *
 * @internal
 *       Be warned that the following comments were recopied verbatim from the previous comments of the captured_driver_replacement_strategy()
 *       routine that was replaced by this generic routine.
 *
 *       It's a waste that the dtc pointer obtained by a call to captured_device_table_get() is
 *       thrown away and immediately reobtained within ftd_strategy().  Rewriting ftd_strategy()
 *       in terms of something along the lines of ftd_strategy() which would only take care of obtaining the
 *       associated ftd_dev_t pointer using ddi_get_soft_state() and then call ftd_strategy_implementation() with the pointer,
 *       and do all the job would be a better way to go.  The captured device strategy would directly
 *       call the ftd_strategy_implementation().
 *
 *       As a bonus, doing this would allow us to get rid of ftd_dynamic_activation_get_replication_device_structure()
 *       and ISDTCDEV() !
 *
 *       Note that now that we have read and write capture strategies, ftd_rw would also need to be modified in order
 *       to properly call ftd_strategy or some kind of additional ftd_strategy_captured_io which would obtain its device pointer
 *       through captured_device_table_get.  To be able to do this, either ftd_rw's signature would need to be modified
 *       to take the additional parameter specifying where the IO is coming from (captured or dtc device),
 *       or ISDTCDEV would need to remain and be used for the decision to be taken.
 *
 *       With the additions of read/read captures, it's not clear anymore that the code would be much simplified by the
 *       above suggestion, so we'll leave things as they are for now.
 *
 */
int ftd_dynamic_activation_captured_driver_generic_replacement_routine(dev_t dev,
                                                                       const captured_io_parameters_t *io_params,
                                                                       generic_process_io_func_ptr process_captured_io,
                                                                       generic_pass_io_through_func_ptr pass_through_captured_io)
{
    int rc;
    ftd_dev_t* devp = NULL;
    
    // Check if a replication device is known to handle IO for this device.
    if ((devp = captured_io_monitoring_increment_requests_being_processed_if_captured_device(dev, io_params)))
    {
        rc = process_captured_io(io_params);
        captured_io_monitoring_decrement_requests_being_processed(devp);
    }
    else
    {
        // Since installing the capture routines and registering the original pointers in the swap_table cannot be atomic,
        // obtaining the original_io_routines through the the swap_table_lock is required.
        // We must and will always be able to obtain the original io routines here, as swapping and registering
        // is atomic with respect to the swap_table_lock.

        const io_function_ptrs_t* original_io_routines =
            swap_table_get_original_io_routines_through_lock(ftd_dynamic_activation_get_swap_key_from_io_params(dev, io_params));

        rc = pass_through_captured_io(io_params, original_io_routines);
    }

    return (rc);

}
