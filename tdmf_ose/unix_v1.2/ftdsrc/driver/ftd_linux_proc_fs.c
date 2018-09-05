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
 * @brief @ref proc_fs implementation.
 *
 * @author Martin Proulx
 */

#include <linux/proc_fs.h>

#include "ftd_linux_proc_fs.h"
#include "ftd_pending_ios_monitoring.h"

// Temp fix for the fact that QNM isn't properly defined anymore.
// C.F. WR PROD00007051
#define QNM "dtc"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
static int group_pending_ios_read_proc(struct file *filp, char *buf, size_t count, loff_t *offp);
static struct file_operations ftd_procfs_fops =
{
	owner:              THIS_MODULE,
	read:               group_pending_ios_read_proc,
};
#endif


/**
 * @brief Publishes the pending_ios entry.
 *
 * We presently print each device's pending_ios count, following by the group's sum.
 * If a device is being waited on, we print a W at the right of its pending_ios count.
 *
 * Sample output:
 * Group  1 pending ios:   168W    2     1    28    22    29     8    58   191     0 :   507
 *
 * @param data is actually a ftd_lg_t* pointer.
 *
 * @attention Since devices are added at the head the the list in lgp->devhead, they are presented in reverse order.
 *
 * @ingroup proc_fs 
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
static int group_pending_ios_read_proc(struct file *filp, char *buf, size_t count, loff_t *offp)
{

    ftd_lg_t* lgp = PDE_DATA(file_inode(filp));
	char *p = buf;
	char local_buffer[64];
	int  length, cumulative_length;

    ftd_uint32_t total_pending_ios = 0;
    ftd_uint32_t device_pending_ios = 0;
    ftd_uint32_t device_is_being_waited_on = 0;
    ftd_dev_t* devp;

    static int entry_count = 0;
	static int printed_non_0_offset = 0;

    if( count <= 10 )
	{
        printk( "<<< Error in group_pending_ios_read_proc(), got count specification less than minimum data length; could cause buffer overflow.\n" );
		return( 0 );
	}

    // NOTE: with RHEL 7 Beta, our proc read function is called over and over in an infinite loop.
	// There used to be an *eof argument passed to our function to set to 1 to signal end of data but it is no longer there.
	// Also, we are always called with a reading offset of 0 (*offp) and always the same count to read,
	// so we have no way of detecting that this is a re-entry. For the moment, we use an entry count
	// and return 0 at first re-entry to signal end of data. A WR has been opened to come back to this
	// once RHEL 7 GA is available: WR PROD00015054.
	if( ++entry_count == 2 )
	{
	    entry_count = 0;
		return( 0 );
	}

	if( lgp == NULL )
	{
        printk( "<<< Error in group_pending_ios_read_proc(), got NULL lgp!\n" );
		return( 0 );
	}

    // If the read offset is not 0, return 0 to signal end of data, since we return all the info in a single call, so we should not be called
	// with a non-zero read offset.
    if( *offp != 0 )
	{
	    if( !printed_non_0_offset )
		{
            printk( "<<< in group_pending_ios_read_proc(), got non-zero offset; exiting.\n" );
		    printed_non_0_offset = 1; // To avoid filling up kernel logs if called repeatedly with non-zero offset
		}
		return( 0 );
	}

    cumulative_length = sprintf(local_buffer, "Group %2d pending ios: ", lgp->th_lgno);
	if( cumulative_length > count )
	{
	    // Exceeding requested data length; could cause buffer overflow
		length = sprintf(p, "Overflow\n" );
		return( length );
	}
	local_buffer[cumulative_length] = '\0';
    p += sprintf(p, local_buffer);

    for (devp = lgp->devhead; devp; devp = devp->next) {
        ftd_context_t device_context;
        
        ACQUIRE_LOCK(devp->lock, device_context);
        {
            device_pending_ios = pending_ios_monitoring_get_ios_pending(devp);
            device_is_being_waited_on = pending_ios_monitoring_is_device_being_waited_on_for_pending_ios_completion(devp);
        }
        RELEASE_LOCK(devp->lock, device_context);

        length = sprintf(local_buffer, "%5u%c", device_pending_ios, device_is_being_waited_on ? 'W' : ' ');
        cumulative_length += length;

		if( cumulative_length > count )
		{
		    // Exceeding requested data length; could cause buffer overflow
			p += sprintf(p-10, " Overflow\n" );
			return( (int)(p - buf) + 1 );
		}

		local_buffer[cumulative_length] = '\0';
	    p += sprintf(p, local_buffer);
        total_pending_ios += device_pending_ios;
    }
    
	if( cumulative_length+6 > count )
	{
	    // Not enough room to safely store the total for this group
        printk( "<<< group_pending_ios_read_proc(); not enough room to safely store the total for this group" );
	    return( (int)(p - buf) + 1 );
	}
    p += sprintf(p, ": %5u\n", total_pending_ios);
    
	return (int)(p - buf) + 1;
}

#else
static int group_pending_ios_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
    ftd_lg_t* lgp = data;    

    ftd_uint32_t total_pending_ios = 0;
    ftd_uint32_t device_pending_ios = 0;
    ftd_uint32_t device_is_being_waited_on = 0;
    ftd_dev_t* devp;
    p += sprintf(p, "Group %2d pending ios: ", lgp->th_lgno);
    
    for (devp = lgp->devhead; devp; devp = devp->next) {
        ftd_context_t device_context;
        
        ACQUIRE_LOCK(devp->lock, device_context);
        {
            device_pending_ios = pending_ios_monitoring_get_ios_pending(devp);
            device_is_being_waited_on = pending_ios_monitoring_is_device_being_waited_on_for_pending_ios_completion(devp);
        }
        RELEASE_LOCK(devp->lock, device_context);
        
        p += sprintf(p, "%5u%c", device_pending_ios, device_is_being_waited_on ? 'W' : ' ');
        total_pending_ios += device_pending_ios;
    }
    
    p += sprintf(p, ": %5u\n", total_pending_ios);
    
	*eof = 1;
	return (int)(p - page) + 1;
}
#endif


/** @todo What sort of error handling can/should we do on a failed /proc directory creation? */
void proc_fs_init_driver_entries(void)
{
    proc_mkdir("driver/"QNM, NULL);
}

void proc_fs_finish_driver_entries(void)
{
    remove_proc_entry("driver/"QNM, NULL);
}

/**
 * @brief Utility function used to format the path of a given group's subdirectory within the /proc fs.
 *
 * @param buffer A sufficiently large buffer (no error checking is performed) to hold the formatted information.
 * @param lgp    A pointer to a group's info.  Only used to get the group's number.
 *
 * @return The number of characters written in buffer.
 */
static int sprint_group_directory_name_in_proc_fs(char* buffer, ftd_lg_t* lgp)
{
    return sprintf(buffer, "driver/%s/lg%d", QNM, getminor(lgp->dev));
}

/** @todo What sort of error handling can/should we do on a failed /proc entry creation? */
void proc_fs_register_lg_proc_entries(ftd_lg_t* lgp)
{
    char buffer[32];
    char* end_of_lg_directory_name = &buffer[0] + sprint_group_directory_name_in_proc_fs(buffer, lgp);

    proc_mkdir(buffer, NULL);
    strcpy(end_of_lg_directory_name, "/pending_ios");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	struct proc_dir_entry *p_proc_dir_entry = NULL;
/*
 For kernels 3.10 and above, here is the interface for proc_create_data():
 struct proc_dir_entry *proc_create_data(const char *name, umode_t mode, struct proc_dir_entry *parent, const struct file_operations *proc_fops,void *data); 
	Where
	name: The name of the proc entry
	mode: The access mode for proc entry
	parent: The name of the parent directory under /proc
	proc_fops: The structure in which the file operations for the proc entry is defined.
	data: If any data needs to be passed to the proc entry.
*/ 
	p_proc_dir_entry = proc_create_data(buffer, 0444, NULL, &ftd_procfs_fops, lgp);

	if( p_proc_dir_entry == NULL )
        printk( "<<< Error: got NULL pointer returned by proc_create_data in proc_fs_register_lg_proc_entries()\n" );
#else
    create_proc_read_entry (buffer, 0, NULL, group_pending_ios_read_proc ,lgp);
#endif
}

/** @todo What sort of error handling can/should we do on a failed /proc entry creation? */
void proc_fs_unregister_lg_proc_entries(ftd_lg_t* lgp)
{
    char buffer[32];
    char* end_of_lg_directory_name = &buffer[0] + sprint_group_directory_name_in_proc_fs(buffer, lgp);

    strcpy(end_of_lg_directory_name, "/pending_ios");
    remove_proc_entry (buffer, NULL);

    *end_of_lg_directory_name = 0x0;
    remove_proc_entry (buffer, NULL);
}
