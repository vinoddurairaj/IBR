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
 * @brief @ref proc_fs public declarations.
 *
 * @author Martin Proulx
 */

#ifndef _FTD_LINUX_IOS_MONITORING_H_
#define _FTD_LINUX_IOS_MONITORING_H_

#include "ftd_def.h"

/********************************************************************************************************************//**
 *
 * @defgroup proc_fs Linux /proc filesystem API
 *
 * Allows the publishing of various driver information within the /proc/driver/QNM/ directory.
 *
 * @todo We could add per device entries under the group's directories.
 * 
 ************************************************************************************************************************/


/**
 * @brief Initializes the proc fs entries for the whole driver.
 *
 * This currently creates a /proc/driver/QMN directory.
 *
 * @post proc_fs_finish_driver_entries() must be called upon driver unloading.
 *
 * @ingroup proc_fs
 */
void proc_fs_init_driver_entries(void);

/**
 * @brief Removes any entries created by proc_fs_init_driver_entries() from the /proc filesystem.
 * 
 * @pre proc_fs_init_driver_entries() must have been previously called upon driver loading.
 *
 * @ingroup proc_fs
 */
void proc_fs_finish_driver_entries(void);

/**
 * @brief Registers files in the /proc/driver/QNM/lgX directory.
 *
 * @param lgp A pointer to the group's structure.
 * 
 * @post proc_fs_unregister_lg_proc_entries() must later be called when the group is deleted.
 *
 * @ingroup proc_fs 
 */
void proc_fs_register_lg_proc_entries(ftd_lg_t* lgp);

/**
 * @brief Unregisters all files in the /proc/driver/QNM/lgX directory.
 *
 * @param lgp A pointer to the group's structure.
 *
 * @pre proc_fs_register_lg_proc_entries() must have previously been called when the group was created.
 *
 * @ingroup proc_fs
 */
void proc_fs_unregister_lg_proc_entries(ftd_lg_t* lgp);

#endif /* _FTD_LINUX_IOS_MONITORING_H_ */
