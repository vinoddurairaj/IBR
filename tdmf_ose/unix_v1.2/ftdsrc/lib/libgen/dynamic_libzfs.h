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
 * @brief Declaration of the @ref dynamic_libzfs API.
 *
 * @author Martin Proulx
 *
 */

#ifndef _DYNAMIC_LIBZFS_H_
#define _DYNAMIC_LIBZFS_H_

/********************************************************************************************************************//**
 *
 * @defgroup dynamic_libzfs Dynamically loaded libzfs API
 *
 * This API solves these problems: 
 *
 * - ZFS's libzfs api is only available after the 6/06 Solaris 10 release.  Previous releases of solaris 10 do not
 *   have support for zfs and its libzfs library.  Because of this, we need to dynamically look for and load
 *   the libzfs library.  This API presents a simplified approach to this problem.
 *
 * - The API has evolved between the 6/06, 08/05 and Open Solaris releases.
 *   The signature of some functions have even changed.  So this API presents the smallest common denominator
 *   of what can be relied upon.
 *
 * - It offers a few higher level functionnalities, just to make some tasks easier.
 * 
 * @sa http://opensolaris.org/os/community/zfs/source/
 *     http://src.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/lib/libzfs/
 *
 * @{
 ************************************************************************************************************************/

/** @brief Opaque context handle used by the @ref dynamic_libzfs. */
typedef struct dynamic_libzfs_context dynamic_libzfs_context_t;

/**
 * @brief Dynamically loads the libzfs library and initializes it.
 *
 * @return A pointer to an allocated opaque context required by all the other functions.
 *         NULL is returned if libzfs could not be loaded or is missing critical functions.
 *
 * @post   The allocated handle must be freed by calling dynamic_libzfs_finish().
 */
dynamic_libzfs_context_t* dynamic_libzfs_init();

/**
 * @brief Terminates the usage of the library and dynamically unloads the library.
 *
 * @param dynamic_libzfs_context A context previously allocated by a call to dynamic_libzfs_init() or NULL.
 * 
 */
void dynamic_libzfs_finish(dynamic_libzfs_context_t* dynamic_libzfs_context);

/**
 * @brief Verifies if a given device is used by a zpool, and returns state information if so.
 * 
 * @param dynamic_libzfs_context  The context obtained from dynamic_libzfs_init().
 * @param file_descriptor         The open file descriptor of the device or file which we want to test for membership in a zpool.
 * @param zpool_name[in]          A pointer to a char* which will be updated upon return.
 * @param zpool_name[out]         The updated char* passed by reference now set to the pool's name.
 *                                The char* value must explicitely be freed by a call to free.
 *
 * @return 1  If the device is in the zpool, 0 otherwise.
 *
 */
int dynamic_libzfs_zpool_in_use(const dynamic_libzfs_context_t* dynamic_libzfs_context,
                                int file_descriptor,
                                char** zpool_name);

/**
 * @brief Verifies if a given device is used by a zpool, and returns state information if so.
 * 
 * @param dynamic_libzfs_context  The context obtained from dynamic_libzfs_init().
 * @param device_path             The path to a device or file which we want to test for membership in a zpool.
 * @param zpool_name[in]          A pointer to a char* which will be updated upon return.
 * @param zpool_name[out]         The updated char* passed by reference now set to the pool's name.
 *                                The char* value must explicitely be freed by a call to free.
 *
 * @return 1  If the device is in the zpool, 0 otherwise.
 *
 */
int dynamic_libzfs_zpool_device_path_in_use(const dynamic_libzfs_context_t* dynamic_libzfs_context,
                                            const char* device_path,
                                            char** zpool_name);

/**
 * @}
 */

#endif /* _DYNAMIC_LIBZFS_H_ */
