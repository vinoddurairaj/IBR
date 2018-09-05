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
/*
 * ftd_start.c - Start one or more logical groups
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * This module initializes a logical group. It reads the config file
 * for the group and then checks the persistent store for state information.
 * If the PS doesn't have any data for this group, we add the group 
 * and its devices to the PS and assume the group is new one. If a device
 * doesn't exist in the PS, we assume it is a new device and set the 
 * appropriate defaults for it. 
 *
 * After reading/setting up of the group/device info from the PS, we 
 * load the driver with the group and device information, including
 * state and dirty bit maps. The device files for the group and its
 * devices are also created at this time. The logical group is now
 * ready for action.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <signal.h>
#if !defined(linux)
#include <macros.h>
#else
#include <sys/param.h>
#define  min MIN
#define  max MAX
#include <linux/fs.h>
#endif /* !defined(linux) */

#include <sys/types.h>
#include <sys/wait.h>

#if defined(SOLARIS)
#include <sys/mkdev.h>
#endif

#include "cfg_intr.h"
#include "ps_intr.h"
#include "ps_migrate.h"
#include "ftdio.h"
#include "ftdif.h"
#include "pathnames.h"
#include "config.h"
#include "ftd_cmd.h"
#include "errors.h"
#include "platform.h"
#include "devcheck.h"
#include "aixcmn.h"
#include "common.h"

// Device configuration error codes
#define CONF_DEV_FAILED_GETTING_PSTORE_INFO -2
#define CONF_DEV_FAILED_MALLOC_FOR_PSTORE_DEV_ATTRIBUTES -3
#define CONF_DEV_FAILED_GETTING_LRDB_FROM_PSTORE -4
#define CONF_DEV_FAILED_GETTING_HRDB_FROM_PSTORE -5
#define CONF_DEV_FAILED_ADDING_DEVICE_TO_PSTORE -6
#define CONF_DEV_FAILED_MALLOC_FOR_LRDB -7
#define CONF_DEV_FAILED_MALLOC_FOR_HRDB -8
#define CONF_DEV_FAILED_SETTING_LRDB_TO_FF_IN_PSTORE -9
#define CONF_DEV_FAILED_SETTING_DEV_ATTRIBUTES_IN_PSTORE -10
#define CONF_DEV_FAILED_GETTING_DEV_ATTRIBUTES_FROM_PSTORE -11
#define CONF_DEV_FAILED_OPENING_DRIVER_HANDLE -12
#define CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES -13
#define CONF_DEV_FAILED_SETTING_HRDB_TO_FF_IN_PSTORE -14
#define CONF_DEV_FAILED_MALLOC_FOR_DRIVER_ADJUSTED_HRDB -15

static char *progname = NULL;
static char configpaths[MAXLG][32];

static int autostart;
static int grp_started;

// Proportional HRDB Tracking Resolution structure
static tracking_resolution_info     HRDB_tracking_resolution;

// To keep the Pstore size for each group and avoid recalculating it when
// several groups share the same Pstore
static long long Pstore_size_KBs[FTD_MAX_GROUPS];

extern int ps_get_tracking_resolution_info( tracking_resolution_info *tracking_res_info_ptr );

extern char *paths;
extern int dbarr_ioc(int fd, int ioc, dirtybit_array_t *db, ftd_dev_info_t *dev);
extern void create_reboot_autostart_file(int group);

char *argv0;

static int start_group(int group, int force, int autostart);
static int conf_lg(char *ps_name, char *group_name, int group,
                   unsigned int hostid, int *state, int *regen_hrdb, long long Pstore_size);
static int conf_dev(char *ps_name, char *group_name, int group, int regen_hrdb,
                    ftd_dev_info_t *devp, sddisk_t *sddisk );

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -a | -g <group#> [-b]\n", progname);
    fprintf(stderr, "\t<group#> is " GROUPNAME " group number. (0 - %d)\n", MAXLG-1);
}

/* this assumes 32-bits per int */ 
#define WORD_BOUNDARY(x)  ((x) >> 5) 
#define SINGLE_BIT(x)     (1 << ((x) & 31))
#define TEST_BIT(ptr,bit) (SINGLE_BIT(bit) & *(ptr + WORD_BOUNDARY(bit)))
#define SET_BIT(ptr,bit)  (*(ptr + WORD_BOUNDARY(bit)) |= SINGLE_BIT(bit))

/* use natural integer bit ordering (bit 0 = LSB, bit 31 = MSB) */
#define START_MASK(x)     (((unsigned int)0xffffffff) << ((x) & 31))
#define END_MASK(x)       (((unsigned int)0xffffffff) >> (31 - ((x) & 31)))

// Get the shift count representing the MSBit of an integer
static int
get_shift_count(unsigned int value)
{
    int i = 0;

    while( value != 0 )
	{
	    value >>= 1;
		i++;
	}
	return( i-1 );
}

static void
set_bits(unsigned int *ptr, int x1, int x2)
{
    unsigned int *dest;
    unsigned int mask, word_left, n_words;

    word_left = WORD_BOUNDARY(x1);
    dest = ptr + word_left;
    mask = START_MASK(x1);
    if ((n_words = (WORD_BOUNDARY(x2) - word_left)) == 0) {
        mask &= END_MASK(x2);
        *dest |= mask;
        return;
    }
    *dest++ |= mask;
    while (--n_words > 0) {
        *dest++ = 0xffffffff;
    }
    *dest |= END_MASK(x2);
    return;
}

static int
get_lrdb(char *ps_name, char *dev_name, unsigned int size,
    int **lrdb, unsigned int *lrdb_bits)
{
    int  ret;
    int *buffer;

    /* allocate a buffer */
    if ((buffer = (int *)malloc(size)) == NULL) {
        return PS_MALLOC_ERROR;
    }
    ret = ps_get_lrdb(ps_name, dev_name, (caddr_t)buffer, size, lrdb_bits);
    if (ret != PS_OK) {
        free(buffer);
        return ret;
    }
    *lrdb = buffer;
    
    return PS_OK;
}

static int
get_hrdb(char *ps_name, char *dev_name, unsigned int size,
    int **hrdb, unsigned int *hrdb_bits)
{
    int  ret;
    int *buffer;

    /* allocate a buffer */
    if ((buffer = (int *)malloc(size)) == NULL) {
        return PS_MALLOC_ERROR;
    }
    ret = ps_get_hrdb(ps_name, dev_name, (caddr_t)buffer, size, hrdb_bits);
    if (ret != PS_OK) {
        free(buffer);
        return ret;
    }
    *hrdb = buffer;
    
    return PS_OK;
}

/*
 * For a pstore, determine the maximum devices it can support
 */
int get_max_ps_devices(char *ps_name, unsigned long long *perdevsize)
{
    unsigned long long  size = 0, err;
    char                raw_name[MAXPATHLEN];
    u_longlong_t        dsize;
    int                 maxdevs, hrt_type = FTD_HS_NOT_SET, ctlfd;

#if defined(HPUX)
    if (is_logical_volume(ps_name)) {
        convert_lv_name(raw_name, ps_name, 1);
    } else {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    if ((dsize = disksize(raw_name)) < 0) {
        reporterr(ERRFAC, M_STAT, ERRCRIT, raw_name, strerror(errno));
        return -1;
    }

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
    sprintf( debug_msg, "<<< Pstore size = %lld >>>\n", dsize ); // <<< delete
    reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg); // <<< delete
#endif

    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
	    dtc_open_err_check_driver_config_file_and_log_error_message(errno);
        return -1;
    }
	// Get the HRDB type from the driver
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
    close(ctlfd);

    /* the size of one device assuming one device per group */
    if (hrt_type == FTD_HS_LARGE)
        size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE_LARGE + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;
    else if (hrt_type == FTD_HS_SMALL)
        size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE_SMALL + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;
    else if (hrt_type == FTD_HS_PROPORTIONAL)
	{
	  // NOTE: this calculation is meant to determine the maximum number of devices for
      //  this Pstore, but also to return the Pstore space required to add a device, which will
      //  be used in the error message if there is not enough space to add devices (unused otherwise).
      //  In the case of Proportional HRDB, the HRDB size can vary from one device to another;
	  //  here, we will return the size assuming the maximum HRDB size at the chosen tracking
	  //  resolution level, BUT for the maximum NUMBER of devices, we set,
	  //  for the fixed size records (Pstore header, device and group tables, device and group attributes, LRDBs
	  //  and devices HRDB info table) enough space to support the absolute maximum number of devices 
	  //  (hard coded constant FTD_MAX_DEVICES). 
	  //  TODO ? In a second phase, we could try to come up with a more precise algorithm; for instance we could
	  //  scan the group configuration files to determine the disk size of each device associated with the
	  //  PStore we are working on, and then make provision for some more devices and get a max_dev from that.
      size = FTD_PS_LRDB_SIZE + HRDB_tracking_resolution.max_HRDB_size_KBs + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;
	}

    *perdevsize = size;

    /* Note: DEV_BSIZE is of different value on different OSes.
     * HPUX 1 KB, others 512 B
     */
    size = size / DEV_BSIZE;

    // Here dsize becomes the maximum number of devices.
#if defined(_AIX) || defined(linux)
    if (hrt_type == FTD_HS_LARGE)
        dsize = (daddr_t)((dsize - 32 -
               (FTD_PS_LRDB_SIZE+FTD_PS_HRDB_SIZE_LARGE)/DEV_BSIZE ) / size);
    else if (hrt_type == FTD_HS_SMALL)
        dsize = (daddr_t)((dsize - 32 -
               (FTD_PS_LRDB_SIZE+FTD_PS_HRDB_SIZE_SMALL)/DEV_BSIZE ) / size);
     else if (hrt_type == FTD_HS_PROPORTIONAL)
	 {
         dsize = FTD_MAX_DEVICES;
	 }
#else
     if (hrt_type == FTD_HS_PROPORTIONAL)
	 {
         dsize = FTD_MAX_DEVICES;
	 }
	 else
	 {
         dsize = (daddr_t)((dsize - 32) / size);
	 }
#endif
    if (dsize > FTD_MAX_DEVICES) {
        dsize = FTD_MAX_DEVICES;
    }
    maxdevs = dsize;
    return(maxdevs);
}

/* WR PROD4607
 * Function for checking that there are no duplicate source devices 
 * nor dtc devices (depending on argument passed) in the same group configuration,
 * or across groups depending on the cfg_path passed.
 * Return: 0 if no duplicate.
 */
static int
check_against_duplicate_source_devices(char *cfg_path, int check_also_dtc_devices)
{
    FILE *fp;
	int  duplicate_found = 0;
	char command[256], dev_name[128];

    /* The data-disk definition in the configuration files have the following format:
	   DATA-DISK:        /dev/rjfs2xx4
       The options specified in the command used in this function cause the following:
	   grep -s: silent, do not return error messages
	   grep -h: do not return the file name on the output line
       cut -d : -f 2: return the second substring of the line, using the ":" delimiter
	*/

    // Check data-disk definitions first.
	// NOTE: we eliminate any lines having data disk /dev/zero, which is used in Network analysis config files as dummy devices.
    sprintf(command, "/bin/grep -s -h DATA-DISK %s | /bin/grep -v /dev/zero | /usr/bin/cut -d : -f 2 | /bin/sort | /usr/bin/uniq -d | awk '{print $1}' 2> /dev/null", cfg_path);
    fp = popen(command, "r");
    if (fp == NULL)
    {
        /* popen() execution error */
        reporterr(ERRFAC, M_DUP_CHK_ERR, ERRINFO, cfg_path);
        return 0;
    }
    else
    {
        for ( ; fgets(dev_name, sizeof(dev_name), fp) != 0 ; )
        {
		    dev_name[strlen(dev_name)-1] = '\0';
			if( strstr(cfg_path, "*.cur") != NULL )
			{
                reporterr(ERRFAC, M_DUP_SRCDEV, ERRCRIT, dev_name);
			}
			else
			{
                reporterr(ERRFAC, M_DUP_SRCDEV_1CFG, ERRCRIT, dev_name, cfg_path);
			}
			duplicate_found = 1;
        }
        pclose(fp);
    }
	if( check_also_dtc_devices )
	{
        /* Check dtc-device definitions second (duplicates can also lead into problems) */
        sprintf(command, "/bin/grep -s -h DTC-DEVICE %s | /usr/bin/cut -d : -f 2 | /bin/sort | /usr/bin/uniq -d | awk '{print $1}' 2> /dev/null", cfg_path);
        fp = popen(command, "r");
        if (fp == NULL)
        {
            /* popen() execution error */
            reporterr(ERRFAC, M_DUP_CHK_ERR, ERRINFO, cfg_path);
            return 0;
        }
        else
        {
            for ( ; fgets(dev_name, sizeof(dev_name), fp) != 0 ; )
            {
		        dev_name[strlen(dev_name)-1] = '\0';
			    if( strstr(cfg_path, "*.cur") != NULL )
			    {
                    reporterr(ERRFAC, M_DUP_SRCDEV, ERRCRIT, dev_name);
			    }
			    else
			    {
                    reporterr(ERRFAC, M_DUP_SRCDEV_1CFG, ERRCRIT, dev_name, cfg_path);
			    }
			    duplicate_found = 1;
            }
            pclose(fp);
        }
	}
	return(duplicate_found);
}

/* WR PROD4388
 * Function for checking that all the source devices of a group can be opened at least Read-only
 * Failure of opening a device would cause problems later on in dtcstart, such as failure in getting
 * disk size with the brute force seek and read approach, which could put the group in a state where
 * it cannot be stopped properly.
 * Return: 0 if all devices OK.
 */
static int
check_open_of_source_devices(char *cfg_path)
{
    FILE *fp;
	int  fd;
	int  save_errno;
	int  device_open_error = 0;
	char command[128], dev_name[128];
	char *carriage_return = NULL;

    sprintf(command, "/bin/grep DATA-DISK %s | awk '{print $2}' 2> /dev/null", cfg_path);
    fp = popen(command, "r");
    if (fp == NULL)
    {
        /* popen() execution error; consider all devices ok by default */
        save_errno = errno;
        reporterr(ERRFAC, M_OPEN_CHK_ERR, ERRINFO, cfg_path, save_errno);
        return 0;
    }
    else
    {
        for ( ; fgets(dev_name, sizeof(dev_name), fp) != 0 ; )
        {
		    if( (carriage_return = strchr(dev_name, '\n')) != NULL )
		    {
			    /* Replace terminating '\n' by '\0' character if applicable */
		        *carriage_return = '\0';
		    }
			/* Check against device name "DATA-DISK", which could occur if the user has commented out
			   a device definition line and kept it in the file, with a ":" inserted before DATA-DISK */
		    if( strstr(dev_name, "DATA-DISK") != NULL )
			    continue;
            if( (fd = open(dev_name, O_RDWR))  < 0)
            {
                save_errno = errno;
                reporterr(ERRFAC, M_OPEN, ERRCRIT, dev_name, strerror(save_errno));
			    device_open_error = 1;
            }
            else
            {
                close (fd);
            }
        } 
        pclose(fp);
    }
	return(device_open_error);
}

/*
 * Get the descriptive conf_dev error string from specified error code
 */
char *get_conf_dev_error_string( int error_code )
{
    static char *msg_CONF_DEV_FAILED_GETTING_PSTORE_INFO = "failed getting Pstore info";
    static char *msg_CONF_DEV_FAILED_MALLOC_FOR_PSTORE_DEV_ATTRIBUTES = "failed allocating memory for device attributes";
    static char *msg_CONF_DEV_FAILED_GETTING_LRDB_FROM_PSTORE = "failed getting LRDB from Pstore";
    static char *msg_CONF_DEV_FAILED_GETTING_HRDB_FROM_PSTORE = "failed getting HRDB from Pstore";
    static char *msg_CONF_DEV_FAILED_ADDING_DEVICE_TO_PSTORE = "failed adding the device to the Pstore";
    static char *msg_CONF_DEV_FAILED_MALLOC_FOR_LRDB = "failed allocating memory for LRDB";
    static char *msg_CONF_DEV_FAILED_MALLOC_FOR_HRDB = "failed allocating memory for HRDB";
    static char *msg_CONF_DEV_FAILED_SETTING_LRDB_TO_FF_IN_PSTORE = "failed initializing LRDB to 0xff... in the Pstore";
    static char *msg_CONF_DEV_FAILED_SETTING_DEV_ATTRIBUTES_IN_PSTORE = "failed setting device attributes in the Pstore";
    static char *msg_CONF_DEV_FAILED_GETTING_DEV_ATTRIBUTES_FROM_PSTORE = "failed getting device attributes from the Pstore";
    static char *msg_CONF_DEV_FAILED_OPENING_DRIVER_HANDLE = "failed opening the driver control device";
    static char *msg_CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES = "failed adjusting Pstore device info after driver calculations";
    static char *msg_CONF_DEV_FAILED_SETTING_HRDB_TO_FF_IN_PSTORE = "failed initializing HRDB to 0xff... in the Pstore";
    static char *msg_CONF_DEV_FAILED_MALLOC_FOR_DRIVER_ADJUSTED_HRDB = "failed allocating new memory for driver adjusted HRDB";
	static char *default_error_string = "unknow error";

	switch( error_code )
	{
        case CONF_DEV_FAILED_GETTING_PSTORE_INFO:
		  return( msg_CONF_DEV_FAILED_GETTING_PSTORE_INFO );
		  break;
        case CONF_DEV_FAILED_MALLOC_FOR_PSTORE_DEV_ATTRIBUTES:
		  return( msg_CONF_DEV_FAILED_MALLOC_FOR_PSTORE_DEV_ATTRIBUTES );
		  break;
        case CONF_DEV_FAILED_GETTING_LRDB_FROM_PSTORE:
		  return( msg_CONF_DEV_FAILED_GETTING_LRDB_FROM_PSTORE );
		  break;
        case CONF_DEV_FAILED_GETTING_HRDB_FROM_PSTORE:
		  return( msg_CONF_DEV_FAILED_GETTING_HRDB_FROM_PSTORE );
		  break;
        case CONF_DEV_FAILED_ADDING_DEVICE_TO_PSTORE:
		  return( msg_CONF_DEV_FAILED_ADDING_DEVICE_TO_PSTORE );
		  break;
        case CONF_DEV_FAILED_MALLOC_FOR_LRDB:
		  return( msg_CONF_DEV_FAILED_MALLOC_FOR_LRDB );
		  break;
        case CONF_DEV_FAILED_MALLOC_FOR_HRDB:
		  return( msg_CONF_DEV_FAILED_MALLOC_FOR_HRDB );
		  break;
        case CONF_DEV_FAILED_SETTING_LRDB_TO_FF_IN_PSTORE:
		  return( msg_CONF_DEV_FAILED_SETTING_LRDB_TO_FF_IN_PSTORE );
		  break;
        case CONF_DEV_FAILED_SETTING_DEV_ATTRIBUTES_IN_PSTORE:
		  return( msg_CONF_DEV_FAILED_SETTING_DEV_ATTRIBUTES_IN_PSTORE );
		  break;
        case CONF_DEV_FAILED_GETTING_DEV_ATTRIBUTES_FROM_PSTORE:
		  return( msg_CONF_DEV_FAILED_GETTING_DEV_ATTRIBUTES_FROM_PSTORE );
		  break;
        case CONF_DEV_FAILED_OPENING_DRIVER_HANDLE:
		  return( msg_CONF_DEV_FAILED_OPENING_DRIVER_HANDLE );
		  break;
        case CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES:
		  return( msg_CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES );
		  break;
        case CONF_DEV_FAILED_SETTING_HRDB_TO_FF_IN_PSTORE:
		  return( msg_CONF_DEV_FAILED_SETTING_HRDB_TO_FF_IN_PSTORE );
		  break;
        case CONF_DEV_FAILED_MALLOC_FOR_DRIVER_ADJUSTED_HRDB:
		  return( msg_CONF_DEV_FAILED_MALLOC_FOR_DRIVER_ADJUSTED_HRDB );
		  break;
	    default:
	  	  return( default_error_string );
		  break;
	}
}

/*
    Start one group
	RETURN: -1 indicates an error but no need to call ftd_stop_group for cleanup
	        -2: indicates error AND NEED TO CALL ftd_stop_group for cleanup
*/
static int
start_group(int group, int force, int autostart)
{
    int ret, ctlfd, i, state, regen;
    int n;  
    char copybuf[BUFSIZ];
    char cfg_path[32];
    char cur_cfg_path[32];
    char full_cfg_path[MAXPATHLEN];
    char full_cur_cfg_path[MAXPATHLEN];
	char all_cur_cfg_path[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    char ps_name[MAXPATHLEN];
    char * strp;
    char bdevstr[MAXPATHLEN];
    char startstr[32];
    sddisk_t *temp;
    ftd_state_t st;
    struct stat statbuf;
    char tunesetpath[MAXPATHLEN];
    char **setargv;
    pid_t childpid;
    int chldstat;
    int tempfd;
    int err;
    int infile, outfile;
    int64_t max_dev; 
    ps_version_1_attr_t attr;

    ftd_dev_info_t	*devp;
    int hrt_type = FTD_HS_NOT_SET;
    int j, numdtcs, psmaxdevs = 0, pscurdevs, newdtcs = 0;
	int need_to_create_Pstore = 0;
    char *dtcdevs, tmpdtc[MAXPATHLEN], dtclist[FTD_MAX_DEVICES][MAXPATHLEN];
    unsigned long long perdevsize;

    char error_log_file[MAXPATHLEN];
    unsigned int hrdb_base_offset;
/*
 * Added a function call to check the pstore version.
 * If an old version is found, ask the user to either migrate it or initialize it.
 * If the answer is yes then initialize the pstore else return.
 */
    int version; 
    int ans = -1;
    char in;

    if (GETPSNAME(group, ps_name) != 0) {
        fprintf(stderr, "Unable to retrieve pstore location from CFG file for group %d\n", group);
        return (-1);
    }


    need_to_create_Pstore = 0;	 // Assume the Pstore has been created

    /* Read the Pstore attributes */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);

    // Check if this is an old Pstore not supported at this stage of RFX 2.7.2
	// or if it does not exist.
	// NOTE: error messages are logged in ps_get_version_1_attr if applicable (argument 3 == 1).
    if( (ret == PS_INVALID_PS_VERSION) || (ret == PS_BOGUS_PS_NAME) )
	{
		return( -1 );
	}

	need_to_create_Pstore = (ret == PS_BOGUS_HEADER);

   // If we are using Proportional HRDBs, get the Tracking resolution information
   // for the tracking level defined in the Pstore (note: different Pstores
   // can use different tracking resolution levels).
   if( (!need_to_create_Pstore) && (attr.hrdb_type == FTD_HS_PROPORTIONAL) )
   {
     sprintf(error_log_file, "%s/" QNM "error.log", PATH_RUN_FILES);

     HRDB_tracking_resolution.level = attr.tracking_resolution_level;
     if( ps_get_tracking_resolution_info( &HRDB_tracking_resolution  ) != 0 )
     {
       fprintf( stderr, "An error occurred while attempting to get the Tracking resolution information.\n" );
       fprintf( stderr, "Please consult the error log file %s for details.\n", error_log_file );
       return( -1 );
     }
   }

    dtcdevs = (char *)ftdmalloc(MAXPATHLEN);
	/* read cfg file and get the dtc devices defined in it */
    if (GETNUMDTCDEVICES(group, &numdtcs, &dtcdevs) != 0) {
        fprintf(stderr, "Unable to determine the number of devices from CFG file for group %d\n", group);
        return (-1);
    }
	/* get the maximum number of devices the pstore can support as well as the
	 * the number of devices it currently holds */
    pscurdevs = ps_get_num_device(ps_name, &psmaxdevs);
	/* if the pstore does not hold a valid header then calculate the maximum
	 * number of devices it can support based on the Pstore size */
    if (psmaxdevs == 0) {
        psmaxdevs = get_max_ps_devices(ps_name, &perdevsize);
        pscurdevs = 0;
    }
    else
        get_max_ps_devices(ps_name, &perdevsize);

	/* get the devices present in the pstore and compare it with the devices from
	 * the cfg file. if the device from cfg file is not in pstore then it is
	 * a new device to be added and hence increment "newdtcs" */
    if (ps_get_device_list(ps_name, dtclist, FTD_MAX_DEVICES) == PS_OK) {
    	for (i = 0; i < numdtcs; i++) {
            if (i == 0)
            	strcpy(tmpdtc, dtcdevs);
            else
            	strcpy(tmpdtc, dtcdevs+(i*MAXPATHLEN)+1);
            for (j = 0; j < pscurdevs; j++) {
	    	  if (!strcmp((char *)&dtclist[j][0], tmpdtc))
        	    break;
	    }
	    if (j == pscurdevs)
		newdtcs++;
    	}
    } else
	newdtcs = numdtcs;
        /* if there are new devices to be added and pstore has reached its maximum
         * device count or if new devices are more than the available space then
	 * throw an error and exit */
	if ((newdtcs && (pscurdevs == psmaxdevs)) || newdtcs > (psmaxdevs - pscurdevs) || newdtcs > psmaxdevs) {
        reporterr(ERRFAC, M_SMALLPS, ERRWARN, ps_name, psmaxdevs, group, (unsigned long long)(newdtcs-(psmaxdevs-pscurdevs))*perdevsize);
        free(dtcdevs);
        return -1;
    }
    free(dtcdevs);

    /* pXXX.cfg is the user created version of the configuration, while
       pXXX.cur is the private copy of the configuration that will be used
       by future readconfigs so that other utilities will be working with
       the same version of the configuration that we start with here.  
       This allows the user to edit the pXXX.cfg files without affecting 
       anything until a subsequent start is run. */
    
    sprintf(cfg_path, "%s%03d.cfg", PMD_CFG_PREFIX, group);
    sprintf(cur_cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, group);
    sprintf(full_cfg_path, "%s/%s", PATH_CONFIG, cfg_path);
    sprintf(full_cur_cfg_path, "%s/%s", PATH_CONFIG, cur_cfg_path);

    /* unlink the old .cur file in case we starting after an unorderly 
       shutdown.  (ftdstop should remove this file otherwise.) */
    unlink(full_cur_cfg_path);

	/* PROD4607: check that no source device is duplicate in the config file */
	if(check_against_duplicate_source_devices(full_cfg_path, 1) != 0)
	{
		 return -1;
	}

    /* PROD4388: the following device verification was exclusive to HPUX; we now make it general
       for all platforms: on HPUX, it was a fix for bug involving MC/ServiceGuard installs for instance,
       local devs might	not be in an available state at boot time or any time after a lvchange or
       vgchange; then continuing on here would get into a bad state. Now we check that all the devices of
       the group being started can at least be opened read-only */
    if(check_open_of_source_devices(full_cfg_path) != 0)
	{
		 return -1;
	}

    /* now copy the .cfg file for this group to .cur */
    if ((infile = open (full_cfg_path, O_RDONLY, 0)) != -1) {
        if ((outfile = creat (full_cur_cfg_path,  S_IRUSR | S_IWUSR)) != -1) {
            while (( n = read(infile, copybuf, BUFSIZ)) > 0) {
                if (write (outfile, copybuf, n) != n) {
                    fprintf(stderr, 
                            "Failed copy of %s to %s - write failed [%s].\n", 
                            cfg_path, cur_cfg_path, strerror(errno));
                    close(outfile);
                    unlink(full_cur_cfg_path);
                    exit (-1);
                }
            }
            fchmod(outfile, S_IRUSR | S_IRGRP);
            close (outfile);
        } else {
            fprintf(stderr, "Failed copy of %s to %s - couldn't create %s\n", 
                    cfg_path, cur_cfg_path, cur_cfg_path);
            exit (-1);
        }
        close (infile);
    } else {
        fprintf(stderr, "Failed copy of %s to %s - couldn't open %s\n", 
                    full_cfg_path, full_cur_cfg_path, full_cfg_path);
	return (-1);
	
    }
    
	/* WR PROD4607: now check against duplicate source devices accross started groups; pass 0 as argument
	   to prevent checking also dtc device names so that we do not affect one-to-many configurations */
    sprintf(all_cur_cfg_path, "%s/p*.cur", PATH_CONFIG);
	if(check_against_duplicate_source_devices(all_cur_cfg_path, 0) != 0)
	{
        unlink(full_cur_cfg_path);
	    return -1;
	}

    /* Read and parse the cfg file and save configuration information, and also determine
       devices sizes */
    if ((ret = readconfig(1, 0, 0, cur_cfg_path)) < 0) {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg_path);
        unlink(full_cur_cfg_path);
        return (-1);
    }

    /* create the group name */
    FTD_CREATE_GROUP_NAME(group_name, group);

    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
        unlink(full_cur_cfg_path);
	    dtc_open_err_check_driver_config_file_and_log_error_message(errno);
        return -1;
    }
    // Get the HRDB type from the driver
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
	// WARNING: the ctlfd handle is closed further down; do not insert a return point before
	//         it is closed without closing it.

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
	sprintf( debug_msg, "<<< HRT type from the driver: %d >>>\n", hrt_type ); // <<< delete
    reporterr( ERRFAC, M_GENMSG, ERRINFO, debug_msg ); // <<< delete
#endif

    /* NOTE: at very beginning, dtcstart gets the HRT type from the driver; if the type is FTD_HS_NOT_SET,
	         it means that no Pstore has been initialized (dtcinit not run with -s/-l/-r) OR we have rebooted.
			 In this case, dtcstart scans the config files and checks the Pstores to get the Pstores type to give
			 it to the driver. If no formated Pstore is found, dtcstart gives the default hrt type to the driver:
			 FTD_HS_PROPORTIONAL.
       Possibilities here:
	   1) No Pstore was initialized, then we got a PS_BOGUS_HEADER status from ps_get_version_1_attr() and 
	      the driver gave us the default HRDB type given to it by dtcstart at the beginning; in this case we create
	      the Pstore with default format i.e. Proportional HRDB with default tracking resolution level.
	   2) The Pstores were initialized before but this Pstore has been broken or zeroed (dtcinit -p <Pstore name>):
	      we must take the HRDB format returned to us by the driver, as the driver knows only about one and only one common
		  Pstore format.
	   3) The Pstore was initialized before but with a format different than that given to us by the driver; keeping
	      the legacy code design, the driver's format specification has priority; so we must recreate the Pstore
	      based on the type specified by the driver; this should not occur since the driver would have gotten a 
	      new format specification from dtcinit -s/-l/-r which would have reformatted the Pstores accordingly,
	      but we keep the logic as per legacy code to be able to cope with that.
	   4) This Pstore has been created before with the same format as that known by the driver: we can proceed 
	      to start the group and add it to the Pstore if it is not already there.

	   NOTE: we assume here that dtcinit -r (proportional HRDB) will not be allowed on a single Pstore when other
	         Pstores exist with Small or Large HRT, again because the driver knows only about a unique Pstore format.
			 This is to preserve the legacy code approach.
			 To change Pstore format from Small/Large HRT to Proportional HRT, dtcinit -r must be run without
			 specifying a single Pstore (i.e. be applied to all Pstores).
	*/

    if( need_to_create_Pstore )
    {
		if( hrt_type == FTD_HS_NOT_SET )
		{
            // The driver does not know the Pstore format, so we did not find any formatted Pstore:
            // take default format of Proportional HRDB with default tracking resolution level.
		    hrt_type = FTD_HS_PROPORTIONAL;
			// Give the HRDB type to the driver
            FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
		}
		// else (hrt_type != FTD_HS_NOT_SET): keep hrt_type specified by the driver for Pstore creation.
    }
    else if( (attr.hrdb_type != hrt_type) && (hrt_type != FTD_HS_NOT_SET) )
    {
	    // The Pstore has been initialized but the driver specifies a different Pstore type now.
		// We must reformat the Pstore according to the driver's specification (keeping legacy behavior),
		// UNLESS the HRDB type given to us by the driver is FTD_HS_NOT_SET.
		need_to_create_Pstore = 1;
    }

    // Close the handle opened to talk to the driver for HRDB type     
    close(ctlfd);

	if( need_to_create_Pstore )
    {
        if( hrt_type == FTD_HS_PROPORTIONAL )
		{
           HRDB_tracking_resolution.level = PS_DEFAULT_TRACKING_RES;
           sprintf(error_log_file, "%s/" QNM "error.log", PATH_RUN_FILES);

           if( ps_get_tracking_resolution_info( &HRDB_tracking_resolution  ) != 0 )
           {
             fprintf( stderr, "An error occurred while attempting to get the Tracking resolution information.\n" );
             fprintf( stderr, "Please consult the error log file %s for details.\n", error_log_file );
             unlink(full_cur_cfg_path);
	         return( -1 );
           }
		}

        ret = create_ps(ps_name, &max_dev, &HRDB_tracking_resolution);
        if (ret == 0)
        {
		  if( hrt_type == FTD_HS_PROPORTIONAL )
		  {
            reporterr( ERRFAC, M_PSCREAT_VARHRDB, ERRINFO, ps_name, 
                       ps_get_tracking_resolution_string(HRDB_tracking_resolution.level), (unsigned int)max_dev );
		  }
		  else
		  {
            reporterr(ERRFAC, M_PSCREAT_SUCCESS, ERRINFO, ps_name, max_dev);     
		  }
        }
        else
        {
		  // Error occurred
		  if( hrt_type == FTD_HS_PROPORTIONAL )
		  {
            reporterr( ERRFAC, M_PSCREAT_FAILED, ERRCRIT, ps_name );
		  }
		  else
		  {
            reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, group);
		  }
          unlink(full_cur_cfg_path);
          exit(-1);
        }
    }

    // Get the Pstore size for this group if not already known
	if( Pstore_size_KBs[group] == 0	)
	{
      if ((Pstore_size_KBs[group] = ps_get_pstore_size_KBs(ps_name)) <= 0)
      {
        unlink(full_cur_cfg_path);
        return(-1);
      }
	}

    // If the Pstore was already created before, we still want to check that
	// its size is large enough (Pstore may have been formatted with an earlier
	// revision of the product, which we have seen in test cycles that ran
	// different revisions) (PROD9808)
	// NOTE: if some error occurs getting the HRDB base offset, we continue and assume
	// that the Pstore is big enough; if it is not big enough, it will be caught further down
	// (it is just that the error message will not be as clear then)
	if( !need_to_create_Pstore )
	{
        ret = ps_get_hrdb_base_offset( ps_name, &hrdb_base_offset );
		if( ret == PS_OK )
	    {
			if( Pstore_size_KBs[group] <= (long long)hrdb_base_offset )
			{
		        reporterr( ERRFAC, M_PSTOOSMALL, ERRCRIT, ps_name, Pstore_size_KBs[group], hrdb_base_offset );
                unlink(full_cur_cfg_path);
				return( -1 );
			}
		}
	}

    /* open the driver and create the group and devices! */
    /* create the group */
    if ((ret = conf_lg( ps_name, group_name, group, mysys->hostid, &state, &regen, Pstore_size_KBs[group] )) < 0)
    {
	    if( grp_started )
		{
		    // This group is already running
		    reporterr(ERRFAC, M_ALREADY_STARTED, ERRINFO, group);
            return (-1);
		}
		else
		{
            return (-2); // -2 status indicates that whatever was completed must be undone
		}
    }

    /* backfresh mode means we don't add anything to the driver */
    if (state == FTD_MODE_BACKFRESH) {
        /* don't do anything here */
    }

	devp = (ftd_dev_info_t*)malloc(FTD_MAX_DEVICES*sizeof(ftd_dev_info_t));

    /* create the devices */
    for (temp = mysys->group->headsddisk, i=0; temp != NULL; temp = temp->n, i++) {
#if defined(HPUX)
	/*-
	 * deprecate the f*king control daemon.
	 * here, open each local device. 
	 * on return from this function, we'll fork(2).
	 * the forked process will persist until kill(2)'ed
	 * by the stop program...
	 */

        /* First, build the block dev name from the raw dev */
	force_dsk_or_rdsk(bdevstr, temp->devname, 0);

	if( (temp->devid = open(bdevstr, O_RDONLY, 0666))  < 0 ) {
		reporterr(ERRFAC, M_OPEN, ERRCRIT, bdevstr, strerror(errno));
            	return (-2);
        }
#endif /* defined(HPUX) */
        if ((ret = conf_dev(ps_name, group_name, group, regen, devp, temp )) < 0) {
            if( ret != -1 )
			{
			    // conf_dev returned a specific error code, not generic -1; get the related error string
                reporterr(ERRFAC, M_CONFDEVER, ERRCRIT, get_conf_dev_error_string(ret));
			}
			free(devp);
            return (-2);
        }
    }

	free(devp);

    if(mysys->group->capture_io_of_source_devices)
    {
        reporterr(ERRFAC, M_CAPTSRCDEVSIO, ERRINFO, group);
    }
    
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
	    dtc_open_err_check_driver_config_file_and_log_error_message(errno);
        return (-2);
    }

    st.lg_num = group;
    st.state = state;
    if (FTD_IOCTL_CALL(ctlfd, FTD_SET_GROUP_STATE, &st) < 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group state", 
            strerror(errno));
        return (-2) ;
    }

    /* set the shutdown state of the group to FALSE */
    ps_set_group_shutdown(ps_name, group_name, 0);

    /* configtool may not have been able to set the tunables in 
       the pstore because start had not been run, so it stores
       them in a temp file that we'll read in and delete now. */
    sprintf(tunesetpath, PATH_CONFIG "/settunables%d.tmp",group);
    if (stat(tunesetpath, &statbuf) != -1) {
        setargv = (char **) ftdmalloc (sizeof(char *) * 2);
        setargv[0] = (char *) ftdmalloc (sizeof(tunesetpath));
        strcpy (setargv[0], tunesetpath);
        setargv[1] = (char *) NULL;
        childpid = fork ();
        switch (childpid) {
        case 0 : 
            /* child */
            execv(tunesetpath, setargv);
        default:
            wait(&chldstat);
            unlink(tunesetpath);
            free (setargv[0]);
            free (setargv);
        }
    }

    /* get tunables that driver needs to know about. */
    if (gettunables(group_name, 1, 0) < 1) {
        fprintf(stderr, "Couldn't retrieve tunable paramaters.\n");
        return -2;
    }   
    
    /* set the tunables in the driver */
    if (setdrvrtunables(group, &sys[0].tunables)!=0) {
        return -2;
    }
    /* set autostart flag in pstore */ 
    if (!autostart) {
        strcpy(startstr, "yes");
        err = ps_set_group_key_value(mysys->pstore, mysys->group->devname,
            "_AUTOSTART:", startstr);
        if (err != PS_OK) {
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, mysys->group->devname, ps_name);
            return -2;
        }
    }
	else
	{
		/* Create a flag file so that the PMD knows we are doing autostart after reboot */
        create_reboot_autostart_file(group);
	}
    return (0);
}

static int
conf_lg(char *ps_name, char *group_name, int group, unsigned int hostid,
        int *state, int *regen_hrdb, long long Pstore_size)
{
    char                *buffer;
    ftd_lg_info_t       info;
    int                 ctlfd, ret;
    stat_buffer_t       sb;
    ps_group_info_t     group_info;
    ps_version_1_attr_t attr;
    struct stat         statbuf;
#if defined(HPUX)
    ftd_devnum_t        dev_num;
    dev_t               dev;
#endif

    *regen_hrdb = 0;
    grp_started = 0;

    /* get the header to find out the size of the attribute buffer */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);
    if (ret != PS_OK) {
        reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        return -1;
    }

    if ((buffer = (char *)calloc(attr.group_attr_size, 1)) == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.group_attr_size);
        return -1;
    }

    /* get the group info to see if this group exists */
    bzero((void *)&group_info, sizeof(ps_group_info_t));
    ret = ps_get_group_info(ps_name, group_name, &group_info);

    if (ret == PS_GROUP_NOT_FOUND) {
        /* if attributes don't exist, add the group to the persistent store */
        group_info.name = group_name;
        group_info.hostid = hostid;
        ret = ps_add_group(ps_name, &group_info);
        if (ret != PS_OK) {
            /* we're hosed */
            reporterr(ERRFAC, M_PSADDGRP, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }

        /* set default attributes */
        strcpy(buffer, FTD_DEFAULT_TUNABLES);

        ftd_trace_flow(FTD_DBG_FLOW1, "Set PS %s %s default tunables\n", 
                       ps_name, group_name);

        ret = ps_set_group_attr(ps_name, group_name, buffer, attr.group_attr_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_group(ps_name, group_name, group, 0);
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }

        /* set the group state, too */
        ret = ps_set_group_state(ps_name, group_name, FTD_MODE_PASSTHRU);

        ftd_trace_flow(FTD_DBG_FLOW1, "Set PS %s %s group as passthru\n", 
                       ps_name, group_name);

        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_group(ps_name, group_name, group, 0);
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }
        *state = FTD_MODE_PASSTHRU;

        /* set the group checkpoint state to default */
        if ((ret = ps_set_group_checkpoint(ps_name, group_name, 0)) != PS_OK) {
            /* we're hosed */
            ps_delete_group(ps_name, group_name, group, 0);
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }
    } 
    else if (ret == PS_OK) 
    {
        /* see if the group was shutdown properly */
        if (group_info.shutdown != 1) 
        {
            /* Unclean shutdown (crash or hard reboot) */

            /* BACKFRESH doesn't add anything to the driver */
            if (group_info.state == FTD_MODE_BACKFRESH) 
            {
                *state = FTD_MODE_BACKFRESH;
            }
            /* shutdown isn't needed for PASSTHRU mode */
            else if(group_info.state == FTD_MODE_PASSTHRU) 
            {
                 *state = FTD_MODE_PASSTHRU;
            }
            else 
            {

                gettunables (group_name, 1, 1);

                if (sys[0].tunables.use_lrt == 1)
                {
                   /* LRT ON: we can rely on the pstore LRDB. Regenerate the high res dirty bits from pstore LRDB */
                   *regen_hrdb = 1;

                   /* The original code used to set the state to FTD_MODE_TRACKING but this lead to checksum mismatches
                      due to incorrect state transitions at start. FTD_MODE_REFRESH fixes this, but still an analysis of
                      the "state machine" is necessary to clarify how it should behave and why it fails. */
                   *state = FTD_MODE_TRACKING;
                }
                else
                {
                   /* LRT OFF: cannot rely on pstore LRDB; need Full Refresh */
                   *regen_hrdb = 0;
                   *state = FTD_MODE_TRACKING;
                   ps_set_group_key_value(mysys->pstore, mysys->group->devname, "_MODE:", "FULL_REFRESH");
                }
                 ps_set_group_state(ps_name, group_name, *state);

            }
        } 
        else 
        {
            *state = group_info.state;
        }
    } 
    else 
    {
        /* other error. we're hosed. */
        reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name, ret);
        free(buffer);
        return -1;
    }

    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
	    dtc_open_err_check_driver_config_file_and_log_error_message(errno);
        free(buffer);
        return -1;
    }

    if (stat(ps_name, &statbuf) != 0) {
        reporterr(ERRFAC, M_PSSTAT, ERRCRIT, ps_name);
        close(ctlfd);
        free(buffer);
        return(-1);
    }

#if defined(_AIX)
    /* 
     * _AIX (struct stat *)p->st_dev is the device of 
     * the parent directory of this node: "/". ouch. 
     */
    info.persistent_store = statbuf.st_rdev;
#else /* defined(_AIX) */
    info.persistent_store = statbuf.st_rdev;
#endif /* defined(_AIX) */
    
#if defined(HPUX)

    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICE_NUMS, &dev_num) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get device numbers", 
            strerror(errno));
        close(ctlfd);
        free(buffer);
        return -1;
    }

    ftd_make_group(group, dev_num.c_major, &dev);
    info.lgdev = dev;        /* both the major and minor numbers */
#elif defined(SOLARIS) || defined(_AIX)
    {
        struct stat csb;
        fstat(ctlfd, &csb);
        info.lgdev = makedev(major(csb.st_rdev), group | FTD_LGFLAG);
    }
#elif defined(linux)
    {
        ftd_devnum_t        dev_num;
        if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICE_NUMS, &dev_num) != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get device numbers", 
                strerror(errno));
            close(ctlfd);
            free(buffer);
            return -1;
        }

        /*
         * Use ftd character device major number
         */
        info.lgdev = makedev(dev_num.c_major, group);
    }
#endif

    info.statsize = attr.group_attr_size; 
    if (FTD_IOCTL_CALL(ctlfd, FTD_NEW_LG, &info)) {
        if (errno == EADDRINUSE){
            grp_started = 1;
            close(ctlfd);
            free(buffer);
            return -1;
        } else {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't add group", 
                strerror(errno));
            close(ctlfd);
            free(buffer);
            return -1;
        }
    }

#if defined(SOLARIS) || defined(_AIX)  || defined(linux)
    ftd_make_group(group, info.lgdev);
#endif

    /* get the attributes of the group */
    ret = ps_get_group_attr(ps_name, group_name, buffer, attr.group_attr_size);

    /* add the attributes to the driver */
    ftd_trace_flow(FTD_DBG_FLOW1,
                "Set drv %s %s default tunables, grp_attr_size: %d\n", 
                ps_name, group_name, attr.group_attr_size);
    if (ftd_debugFlag & FTD_DBG_FLOW1) {
        ps_dump_info(ps_name, group_name, buffer);
    }
    
    sb.lg_num = group; /* the minor number of the group */
    sb.dev_num = 0;
    sb.len = attr.group_attr_size;
    sb.addr = (ftd_uint64ptr_t)(unsigned long)buffer;
    if (FTD_IOCTL_CALL(ctlfd, FTD_SET_LG_STATE_BUFFER, &sb)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group stats", 
            strerror(errno));
        close(ctlfd);
        free(buffer);
        return -1;
    }

    free(buffer);
    close(ctlfd);
    return 0;
}

/* FIXME:  This code is a mess */
static int
conf_dev(char *ps_name, char *group_name, int lgnum, int regen_hrdb,
	 ftd_dev_info_t *devp, sddisk_t *sddisk)
{
    stat_buffer_t       sbuf;
    ps_version_1_attr_t attr;
    ps_dev_info_t       dev_info, check_dev_info;
    ps_dev_info_t       *dev_attributes_ptr;
    int                 ctlfd, lgfd, ret, found = 1;
    ftd_dev_info_t      info;
    struct stat         sb, sd;
    dev_t               cdev, bdev;
    char                *buffer;
    int                 *lrdb, *hrdb;
    unsigned int        hrdb_bits, lrdb_bits;
    dirtybit_array_t    dbarray;
    char                raw[MAXPATHLEN], block[MAXPATHLEN], raw_name[MAXPATHLEN],
                        block_name[MAXPATHLEN], bff[128];
    char		msg[MAXPATHLEN+128];
    char 		shortname[MAXPATHLEN]; 
    int			need_regen = 0;
    ftd_uint64_t        currentsize;/* Expand vol*/
    char		*devname   = sddisk->devname;
    char		*mirname   = sddisk->mirname;
    char		*sddevname = sddisk->sddevname;
    ftd_uint64_t      limitsize_multiple  = sddisk->limitsize_multiple;

    int			index;
	unsigned int hrdb_size, hrdb_offset, initial_hrdb_size, hrdb_res_KBs_per_bit, lrdb_resolution_sectors_per_bit;
	ftd_uint64_t num_sectors;
	int          store_active_num_bits_and_lrdb_res, result;

#if defined(HPUX) || defined(linux)
    ftd_devnum_t        dev_num;
#endif

	/* The legacy code (up to RFX 2.7.0 inclusive) made the driver believe that 
	   the device size is a factor * its real size;
	   this way, the driver used (1/factor) of the tracking bits since there is
	   no I/O to the fictitious sectors, corresponding to the tracking bits that were, hence, not used
	   unless device expansion was done. For the first phase of RFX 2.7.1, we keep this logic.
       If we want to turn off the fudging factor and use all of the tracking bits, it can be done in 
       the config files by setting the DISK-LIMIT-SIZE attribute to 1. 
       In this case, if device expansion occurs, we will need a mechanism to compress the dirty bits 
       so that they represent the new resolution for the expanded device. <<< TODO (dtcexpand)
	*/
    if (limitsize_multiple == 0) {
        limitsize_multiple = DEFAULT_LIMITSIZE_MULTIPLE;
    }

    dev_info.limitsize_multiple = limitsize_multiple; // To pass this info to ps_add_device

    /* make sure the device exists */
    if (stat(devname, &sb)) {
        reporterr(ERRFAC, M_STAT, ERRCRIT, devname, strerror(errno));
        return -1;
    }

    strcpy(raw_name,devname);
    force_dsk_or_rdsk(block_name,raw_name,0);
    
    if (stat(block_name, &sd) ){
        reporterr(ERRFAC, M_STAT, ERRCRIT,block_name, strerror(errno)); 
        return -1;
    }  

    /* get the header to find out the max size of the dirty bitmaps */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);
    if (ret != PS_OK) {
        /* we're hosed. */
        return CONF_DEV_FAILED_GETTING_PSTORE_INFO;
    }

    if ((buffer = (char *)calloc(attr.dev_attr_size, 1)) == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.dev_attr_size);
        return CONF_DEV_FAILED_MALLOC_FOR_PSTORE_DEV_ATTRIBUTES;
    }

    /* create the device names */
    strcpy(raw, sddevname);
    force_dsk_or_rdsk(block, raw, 0);

    /* get the attributes of the device */
    ret = ps_get_device_attr(ps_name, raw, buffer, attr.dev_attr_size);
    if (ret == PS_OK)
    {
 		// Get this device's HRDB size, offset and tracking resolution
        if( attr.hrdb_type == FTD_HS_PROPORTIONAL )
	    {
            if( ps_get_device_hrdb_info(ps_name, raw, &hrdb_size, &hrdb_offset, NULL, 
                     &dev_info.hrdb_resolution_KBs_per_bit, NULL, &num_sectors, NULL, NULL ) != PS_OK )
		    {
                reporterr(ERRFAC, M_ERR_HRDBINFO, ERRCRIT, raw, lgnum, ps_name);
                free(buffer);
                return -1;
            }
            // Check that the recorded device size in the Pstore matches the actual device size
            // to detect situation where an old Pstore is used after redefining devices in groups and
    	    // omitting to reformat the Pstore (PROD8564). If this group uses proportional HRDB,
    		// the HRDB size might not satisfy the new device definitions.
    		// NOTE: we cannot rely on num_sectors if the Pstore format is pre-RFX271, however.
    		if( num_sectors != sddisk->devsize )
    		{
                reporterr(ERRFAC, M_PS_DEVSIZE_ERR, ERRCRIT, num_sectors, sddisk->devsize, raw);
                reporterr(ERRFAC, M_PS_DEVSIZE_ER2, ERRCRIT);
                free(buffer);
                return -1;
    		}
	    }
	    else  // Non-Proportional HRDB (Small or Large HRT)
	    {
		    hrdb_size = attr.Small_or_Large_hrdb_size;
	    }
        if (get_lrdb(ps_name, raw, attr.lrdb_size, &lrdb, &lrdb_bits) != PS_OK) {
            /* we're hosed */
            free(buffer);
            return CONF_DEV_FAILED_GETTING_LRDB_FROM_PSTORE;
        }
        if (get_hrdb(ps_name, raw, hrdb_size, &hrdb, &hrdb_bits) != PS_OK) {
            /* we're hosed */
            free(lrdb);
            free(buffer);
            return CONF_DEV_FAILED_GETTING_HRDB_FROM_PSTORE;
        }
        /* if the hrdb is bogus, create a new one */
        if (regen_hrdb) {
			need_regen = 1;
        }
    } else if (ret == PS_DEVICE_NOT_FOUND) {
        /* if attributes don't exist, add the device to the persistent store */
        dev_info.name = raw;
	    found = 0;

        /* FIXME: how do we override/calculate these values? */
        dev_info.info_allocated_lrdb_bits = attr.lrdb_size * 8;
		// Note: dev_info.num_hrdb_bits	is set in ps_add_device.
		dev_info.num_sectors = sddisk->devsize;

        ret = ps_add_device(ps_name, &dev_info, &HRDB_tracking_resolution, Pstore_size_KBs[lgnum], attr.hrdb_type);
        if (ret != PS_OK) {
            reporterr(ERRFAC, M_PS_DEV_ADD_ERR, ERRCRIT, ps_get_pstore_error_string(ret));
            /* we're hosed */
            free(buffer);
            return CONF_DEV_FAILED_ADDING_DEVICE_TO_PSTORE;
        }
		hrdb_size = dev_info.hrdb_size;  // ps_add_device calculates the HRDB size

        /* set up default LRDB and HRDB */
        if ((lrdb = (int *)malloc(attr.lrdb_size)) == NULL) {
            free(buffer);
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.lrdb_size);
            return CONF_DEV_FAILED_MALLOC_FOR_LRDB;
        }
        // PROD8548: take note of initial hrdb_size in case of driver adjustments
        initial_hrdb_size = hrdb_size;
        if ((hrdb = (int *)malloc(hrdb_size)) == NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, hrdb_size);
            free(lrdb);
            free(buffer);
            return CONF_DEV_FAILED_MALLOC_FOR_HRDB;
        }
        memset((caddr_t)lrdb, 0xff, attr.lrdb_size);

        ret = ps_set_lrdb(ps_name, raw, (caddr_t)lrdb, attr.lrdb_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return CONF_DEV_FAILED_SETTING_LRDB_TO_FF_IN_PSTORE;
        }
        // Set the ackoff field for this device to 0
        ps_set_device_entry(ps_name, raw, 0);

        /* FIXME: set up default attributes, if any. */
        // sprintf(buffer, "DEFAULT_DEVICE_ATTRIBUTES: NONE\n");
        // WR PROD4059: this is not supported; this string 
        //       DEFAULT_DEVICE_ATTRIBUTES... must not be sent to the pstore
        //       nor to the driver (see FTD_SET_DEV_STATE_BUFFER ioctl call below).
        //       If sent, it overwrites some of the stats fields with the string characters.
        //       No string parsing is done in this driver ioctl nor in ps_set_device_attr().
        memset(buffer, 0, attr.dev_attr_size);
        ret = ps_set_device_attr(ps_name, raw, buffer, attr.dev_attr_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return CONF_DEV_FAILED_SETTING_DEV_ATTRIBUTES_IN_PSTORE;
        }
    } else {
        /* other error. we're hosed. */
        free(buffer);
        return CONF_DEV_FAILED_GETTING_DEV_ATTRIBUTES_FROM_PSTORE;
    }

    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
	    dtc_open_err_check_driver_config_file_and_log_error_message(errno);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return CONF_DEV_FAILED_OPENING_DRIVER_HANDLE;
    }

/* new code begin- for multiple pstores ... */

#ifdef _AIX
    if (sddisk->dd_minor >= 0) {
	index = sddisk->dd_minor;
    } else if (sddisk->md_minor >= 0) {
	index = sddisk->md_minor;
    } else {
	index = -1;
    }
    int minor_index = index;
#else
    index = -1;
#endif
   
    if(FTD_IOCTL_CALL(ctlfd, FTD_CTL_ALLOC_MINOR, &index) < 0) {
	printf("\n error getting the minor number ");
	free(lrdb);
	free(hrdb);
	free(buffer);
	return -1;
    } 
/* new code ends..... */

#if defined(_AIX)
    if (minor_index >= 0 && minor_index != index) {
	reporterr(ERRFAC, M_MINORMISMATCH, ERRWARN, sddevname, minor_index,
		  mirname, index);
    }
#endif

    lgfd = open(group_name, O_RDWR);
    if (lgfd < 0) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, group_name, strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(ctlfd);
        return -1;
    }

#if defined(HPUX) || defined(linux)

    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICE_NUMS, &dev_num) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get device numbers", 
            strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(lgfd);
        close(ctlfd);
        return -1;
    }
#if defined(HPUX)
    cdev = makedev(dev_num.c_major, index);
    bdev = makedev(dev_num.b_major, index);
                     
#elif defined(linux)

    bdev = makedev(dev_num.b_major, index);
    cdev = bdev;
#endif

#elif defined(SOLARIS) || defined(_AIX)
    {
        struct stat csb;
        fstat(ctlfd, &csb);
        cdev = makedev(major(csb.st_rdev), index);
        bdev = cdev;
    }
#endif

    info.lgnum = lgnum;
    info.cdev = cdev;
    info.bdev = bdev;
    info.localcdev = sb.st_rdev;
	currentsize = sddisk->devsize; // We have the device size since readconfig was called, and this size was validated

#if defined(linux)
    {
	int bfd = open(devname, O_RDWR);

	if (bfd >= 0) {
	    ret = ioctl(bfd, BLKFLSBUF, 0);
	    close(bfd);
	}
    }
#endif

    // The following applies a factor to the real device size to make the driver believe it is larger
	// than it really is; thus only a fraction of the dirty bits will be actually used in reality.
	// The purpose of this is to allow for future device expansion, legacy way of RFX 2.7.0.
    info.disksize = limitsize_multiple * currentsize;
    info.maxdisksize = limitsize_multiple * currentsize; 

    info.lrdbsize32 = attr.lrdb_size / 4;      /* number of 32-bit words */
    info.hrdbsize32 = hrdb_size / 4;           /* number of 32-bit words */
    info.statsize = attr.dev_attr_size;
	ret = ps_Pstore_supports_Proportional_HRDB(ps_name);
	if( ret <= 0 )
	{   // Error occurred or old Pstore format
        reporterr(ERRFAC, M_PS_VERSION_ERR, ERRCRIT, ps_name, ret);
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(lgfd);
        close(ctlfd);
        return (-1);
	}
    if( attr.hrdb_type == FTD_HS_PROPORTIONAL )
	{
	    // Driver requires the bit resolution as a shift count for Proportional HRDB;
		// for other HRDB modes (Small or Large) this value is set by the driver and returned
		// to us after
	    info.hrdb_res = get_shift_count( dev_info.hrdb_resolution_KBs_per_bit * 1024 );
	}

    ps_get_lrdb_offset(ps_name, raw, &info.lrdb_offset);

    if (FTD_IOCTL_CALL(ctlfd, FTD_NEW_DEVICE, &info)) {
        if (grp_started && errno == EADDRINUSE) {
        } else {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't add new device", 
                strerror(errno));
            free(lrdb);
            free(hrdb);
            free(buffer);
            close(lgfd);
            close(ctlfd);
            return -1;
            // <<< TODO: should we not delete the group from the Pstore if it was just partially created (if( !found ))
			// <<< See WR PROD10089
        }
    }
	if( found )
	{
        // The device was already in the Pstore; the hrdb_size found in the Pstore for this device
		// should not have been changed by the driver but we check against this because of cases seen
		// in the past where group config files had been changed (different devices) and the Pstore
		// had not been re-initialized.
        if( attr.hrdb_type == FTD_HS_PROPORTIONAL )
		{
	        if( (info.hrdb_res != get_shift_count( dev_info.hrdb_resolution_KBs_per_bit * 1024) ) ||
		        (info.hrdbsize32 != (hrdb_size / 4)) )
			{
                reporterr(ERRFAC, M_DRV_HRDB_ERR, ERRCRIT, raw); 
                reporterr(ERRFAC, M_DRV_HRDB_ER2, ERRCRIT, info.hrdbsize32*4, info.hrdb_res,
                          hrdb_size, get_shift_count( dev_info.hrdb_resolution_KBs_per_bit * 1024)); 
                reporterr(ERRFAC, M_PS_DEVSIZE_ER2, ERRCRIT);
                free(lrdb);
                free(hrdb);
                free(buffer);
                close(lgfd);
                close(ctlfd);
                return -1;
			}
		}
		// In case this Pstore has been migrated from 270 or 271 and this is the first dtcstart since its migration,
		// then some of the fields are not set; set them here
		check_dev_info.name = &msg[0];	// We use this string here for the time of the function call to avoid allocating another one
        ret = ps_get_device_info(ps_name, raw, &check_dev_info); // <<< seg faults in there
	    if (ret != PS_OK)
		{
            free(lrdb);
            if( hrdb != NULL ) free(hrdb);
            free(buffer);
            close(lgfd);
            close(ctlfd);
            return(CONF_DEV_FAILED_GETTING_PSTORE_INFO);
		}
		if( (check_dev_info.info_valid_lrdb_bits == 0) || (check_dev_info.info_valid_hrdb_bits == 0) )
		{
	        ret = ps_adjust_lrdb_and_hrdb_numbits( ps_name, raw, info.lrdb_numbits, info.hrdb_numbits, 0 );
		    if (ret != PS_OK)
			{
	            free(lrdb);
	            if( hrdb != NULL ) free(hrdb);
	            free(buffer);
	            close(lgfd);
	            close(ctlfd);
	            return(CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES);
			}
		    lrdb_resolution_sectors_per_bit = (1 << info.lrdb_res);
			hrdb_res_KBs_per_bit = (1 << info.hrdb_res) / 1024;
		    ret = ps_set_device_hrdb_info(ps_name, raw, 0, 0, hrdb_res_KBs_per_bit, lrdb_resolution_sectors_per_bit, info.lrdb_res, 0, 0, 1);
		    if (ret != PS_OK)
			{
	            free(lrdb);
	            if( hrdb != NULL ) free(hrdb);
	            free(buffer);
	            close(lgfd);
	            close(ctlfd);
	            return(CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES);
			}
		}
	}

    // TODO: the following code has become complicated because of the RFX 2.7.1 support of RFX 2.7.0 Pstores, among
	// other things, which necessitated having 2 device info tables instead of one.
	// If time permits, this should be cleaned up.
    if (found == 0)
    {
	    result = PS_OK; // Assume no error will occur in the following.

	    // We just added this device to the Pstore and in the driver.
        // It is possible that the driver has adjusted the HRDB size and bit resolution in the FTD_NEW_DEVICE call;
	    // the driver returns the info in the structure we passed to it, then we must adjust the user 
	    // space info and device entry info in the Pstore, and the Pstore next available HRDB offset (Proportional HRDB).
		// Also, the LRDB resolution is only calculated by the driver. We also store it in the Pstore.
	    dev_info.lrdb_res_sectors_per_bit = lrdb_resolution_sectors_per_bit = (1 << info.lrdb_res);

        if( attr.hrdb_type == FTD_HS_PROPORTIONAL )
		{
	        if( (info.hrdb_res != get_shift_count( dev_info.hrdb_resolution_KBs_per_bit * 1024) ) ||
		        (info.hrdbsize32 != (hrdb_size / 4)) )
		    {
				store_active_num_bits_and_lrdb_res = 0;
		        hrdb_size = info.hrdbsize32 * 4;
                if( hrdb_size != initial_hrdb_size )
				{
				    // HRDB size has changed; we need to reallocate memory.
                    free(hrdb);
                    if ((hrdb = (int *)malloc(hrdb_size)) == NULL)
                    {
                      reporterr(ERRFAC, M_MALLOC, ERRCRIT, hrdb_size);
					  result = CONF_DEV_FAILED_MALLOC_FOR_DRIVER_ADJUSTED_HRDB;
                    }
				}
				if( result == PS_OK )
				{
			        dev_info.hrdb_resolution_KBs_per_bit = (1 << info.hrdb_res) / 1024;

					ret = ps_adjust_device_info(ps_name, raw, hrdb_size,
	                       initial_hrdb_size, dev_info.hrdb_resolution_KBs_per_bit, dev_info.lrdb_res_sectors_per_bit,
	                       dev_info.dev_HRDB_offset_in_KBs, Pstore_size_KBs[lgnum], attr.hrdb_type, 1, sddisk->devsize, 
	                       dev_info.limitsize_multiple, info.lrdb_numbits, info.hrdb_numbits, info.lrdb_res);
	                if (ret != PS_OK)
					    result = CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES;
				}
			}
			else
			{
			    // Although the values calculated by dtcstart have not been changed by the driver, we still
				// want to store the active number of bits and the LRDB resolution.
				store_active_num_bits_and_lrdb_res = 1;
			}
		}
		// In the case of Small or Large HRT mode, the tracking bit resolution
		// and the EFFECTIVE number of actual tracking bits (might be less than the fixed amount allocated in the Pstore)
		// are calculated by the driver and returned to user space.
		// We save them in the Pstore device info (WR PROD10057). We do this also in Proportional mode if the above block did not execute.
        if( (attr.hrdb_type != FTD_HS_PROPORTIONAL) || store_active_num_bits_and_lrdb_res )
		{
		    hrdb_res_KBs_per_bit = (1 << info.hrdb_res) / 1024;
		    // Enter HRDB resolution in devices HRDB info table; the 8th argument in the call (only_resolution),
			// tells the function to set only the resolution field.
		    // NOTE: WR PROD10058 has a been opened to document a possible need to store the resolution in bytes per bit instead of KBs per bit,
		    // but this is not done right now in the context of RFX 2.7.1. It should be discussed for the next release.
		    ret = ps_set_device_hrdb_info(ps_name, raw, 0, 0, hrdb_res_KBs_per_bit, lrdb_resolution_sectors_per_bit, info.lrdb_res, 0, 0, 1);
			if( ret != PS_OK )
		    {
                reporterr(ERRFAC, M_PSERRINFO, ERRINFO, raw, ps_name, ret, ps_get_pstore_error_string(ret));
			}
			else
			{
			    // Now save the effective number of LRDB and HRDB bits in the Pstore.
				ret = ps_adjust_lrdb_and_hrdb_numbits( ps_name, raw, info.lrdb_numbits, info.hrdb_numbits, 0 );
				if( ret != PS_OK )
	                reporterr(ERRFAC, M_PSERRINFO, ERRINFO, raw, ps_name, ret, ps_get_pstore_error_string(ret));
			}
            if (ret != PS_OK)
			    result = CONF_DEV_FAILED_ADJUSTING_PSTORE_DEV_INFO_AFTER_DRIVER_CHANGES;
		}

        if (result != PS_OK)
        {
            // Error. Must cancel what was done.
            ps_delete_device(ps_name, raw);
            free(lrdb);
            if( hrdb != NULL ) free(hrdb);
            free(buffer);
            close(lgfd);
            close(ctlfd);
            return(result);
        }


        // Set all HRDB bits
        memset((caddr_t)hrdb, 0xff, info.hrdbsize32*4);
        ret = ps_set_hrdb(ps_name, raw, (caddr_t)hrdb, info.hrdbsize32*4);
        if (ret != PS_OK) {
            /* we're hosed */
            reporterr(ERRFAC, M_PSWTHRDB, ERRCRIT, raw, ps_name, ret, ps_get_pstore_error_string(ret));
            ps_delete_device(ps_name, raw);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return CONF_DEV_FAILED_SETTING_HRDB_TO_FF_IN_PSTORE;
        }
    }
/* Expand vol */
    if (info.disksize != currentsize) {
        info.disksize = currentsize;
        if (FTD_IOCTL_CALL(ctlfd, FTD_SET_DEVICE_SIZE, &info)) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set disk size", strerror(errno));
            free(lrdb);
            free(hrdb);
            free(buffer);
            close(lgfd);
            close(ctlfd);
            return -1;
        }
    }
/* Expand vol */

#if defined(HPUX)
    ftd_make_devices(&sb, &sd, raw, block, cdev, bdev);
#else
    ftd_make_devices(&sb, &sd, raw, block, bdev); /* SS/IS activity */
#endif
#if defined(_AIX)
    //If dynamic activation is not active then will need to register DTC driver to AIX ODM
    if (mysys->group->capture_io_of_source_devices == 0)
    {
        /* create /dev/rlgXXXdtcX links for AIX volume expansion/JFS */
        if (ftd_make_aix_links(raw, block, lgnum, sddisk->devid) < 0 )
        {
            reporterr(ERRFAC, M_LNKERR, ERRCRIT);
        }
        if (ftd_odm_add(block_name, lgnum, sddisk->devid, shortname) < 0)
        {
	    reporterr(ERRFAC, M_ODMADDERR, ERRCRIT, shortname);
        }
    }
#endif

    /* add the attributes to the driver; this comment is not accurate; in the case of an added device,
       this call just zeroes out the device stats buffer (content at *buffer was zeroed above). */
    sbuf.lg_num = lgnum; /* by luck: the minor number of the group */
    sbuf.dev_num = cdev; /* was: index */
    sbuf.len = attr.dev_attr_size;
    sbuf.addr = (ftd_uint64ptr_t)(unsigned long)buffer;
    if (FTD_IOCTL_CALL(ctlfd, FTD_SET_DEV_STATE_BUFFER, &sbuf)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group stats", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
#if 0
printf("\n$$$ start: lrdb = \n");
{
unsigned char lrdbc[1024];
int i;

memcpy(lrdbc, lrdb, 1024);
for(i=0;i<1024;i++) {
	printf("%02x",lrdbc[i]);
}
printf("\n");
}
#endif
    /* set the LRDB in the driver */
    dbarray.numdevs = 1;
    {
    dev_t tempdev[2];
    ftd_dev_info_t devinfo;
    tempdev[0] = cdev;
    tempdev[1] = 0x12345678;
    dbarray.devs = (dev_t *)&tempdev[0];
    dbarray.dblen32 = info.lrdbsize32;
    dbarray.dbbuf = (ftd_uint64ptr_t)(unsigned long)lrdb;
    devinfo.lrdbsize32 = dbarray.dblen32;
    if (dbarr_ioc(lgfd, FTD_SET_LRDBS, &dbarray, &devinfo)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set lrdb", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
    }

    /* set the HRDB in the driver */
	/* we have bitsize set (by new_device) at this point */
	if (need_regen) {
        unsigned int value, expansion;
        int          i, j, k, num32, bitnum;

		sbuf.lg_num = lgnum;
		sbuf.addr = (ftd_uint64ptr_t)(unsigned long)devp;
		if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICES_INFO, &sbuf) != 0) {
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", 
				strerror(errno));
			return -1;
		}
		for(i=0;i<FTD_MAX_DEVICES;i++) {
			if (cdev == devp[i].cdev) {
				break;
			}
		}
		if (i == FTD_MAX_DEVICES) {
			/* this can't happen - we added the dev above */
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", 
				strerror(errno));
			return -1;
		}

		/* this should be a power of 2 */
		expansion = 1 << (devp[i].lrdb_res - devp[i].hrdb_res);
#if 0
		printf("\n$$$ disk size, lrdb_numbits, hrdb_numbits, lrdb_res, hrdb_res, expansion = %d, %d, %d, %d, %d, %d\n",
			devp[i].disksize,
			devp[i].lrdb_numbits,
			devp[i].hrdb_numbits,
			devp[i].lrdb_res,
			devp[i].hrdb_res, expansion); 
#endif
		/* zero it out */
		memset(hrdb, 0, hrdb_size);

        /* lrdb_numbits to words + 1, hrdb_size (bytes) to words / expansion */
        num32 = min( (devp[i].lrdb_numbits / 32) + 1, (hrdb_size / 4) / expansion );

		/* march thru bitmap one 32-bit word at a time */
		for (j = 0; j < num32; j++) {
			/* blow off empty words */
			if ((value = lrdb[j]) == 0) continue;

			/* test each bit */
			bitnum = j * expansion * 32;
			for (k = 0; k < 32; k++) {
				if ((value >> k) & 1) {
					set_bits((unsigned int *)hrdb, bitnum + k*expansion,
						bitnum + ((k+1)*expansion)-1);
				}
			} 
		}
	}
#if 0
printf("\n$$$ start: hrdb = \n");
{
unsigned char lrdbc[1024];
int i;

memcpy(lrdbc, hrdb, 1024);
for(i=0;i<1024;i++) {
	printf("%02x",lrdbc[i]);
}
printf("\n");
}
#endif
    dbarray.numdevs = 1;
    {
    dev_t tempdev=cdev;
    ftd_dev_info_t devinfo;
    dbarray.devs = (dev_t *)&tempdev;
    dbarray.dblen32 = info.hrdbsize32;
    dbarray.dbbuf = (ftd_uint64ptr_t)(unsigned long)hrdb;
    devinfo.hrdbsize32 = dbarray.dblen32;
    if (dbarr_ioc(lgfd, FTD_SET_HRDBS, &dbarray, &devinfo)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set hrdb", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
    }

    if(mysys->group->capture_io_of_source_devices)
    {
        ftd_dev_t_t new_device = bdev;
        
        if (FTD_IOCTL_CALL(ctlfd, FTD_CAPTURE_SOURCE_DEVICE_IO, &new_device) != 0) {
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "FTD_CAPTURE_SOURCE_DEVICE_IO", strerror(errno));
            free(lrdb);
            free(hrdb);
            free(buffer);
            close(ctlfd);
            close(lgfd);
            return -1;
		}   
    }

    free(lrdb);
    free(hrdb);
    free(buffer);
    close(ctlfd);
    close(lgfd);

#if defined(linux)
    {
    int blkfd;
    int blksize = -1;

    blkfd = open(block, O_RDWR);
    if (blkfd < 0) {
        sprintf(msg, "failed to open source device %s", block);
        reporterr(ERRFAC, M_DRVERR, ERRWARN, msg, get_error_str(errno));
        return 0;
    }
    if (ioctl(blkfd, BLKBSZGET, &blksize) < 0 || ioctl(blkfd, BLKBSZSET, &blksize) < 0) {
        sprintf(msg, "failed to set blksize_size for dtc device %s", block);
        ftd_trace_flow(FTD_DBG_FLOW1,
                "Got driver error in %s %s\n", msg, get_error_str(errno));
        /*
         * Hard coding blksize to 1024 might lead to problems on devices where the
         * block size is not 1024, but till all the appropriate IOCTLs for
         * querying devices for the block size are found, this hard coding will
         * have to do
         */
        blksize = 1024;
    }
    close(blkfd);
    }
#endif
    return 0;
}

// Function to undo whatever has been left over from an unsuccessful start_group (PROD9808)
int undo_start_group( int group )
{
    char                  ps_name[MAXPATHLEN];
	int                   rc;
    ps_version_1_attr_t   attr;

    if (GETPSNAME(group, ps_name) != 0)
	{
        return -1;
	}
    /* get the attributes of the pstore */
    if (ps_get_version_1_attr(ps_name, &attr, 1) != PS_OK)
    {
        return -1;
    }
    reporterr(ERRFAC, M_UNDOING_START, ERRINFO, group);
    rc = ftd_stop_group(ps_name, group, &attr, 1, 0);
	return( rc );
}

// Function to verify if a group has leftover Network analysis configuration files (there should not be any,
// but we do that in case some error would have occured in network analysis mode that would have left files behind)
// Return: 1 if net analysis cfg file; 0 otherwise
static int check_if_network_analysis_cfg_file( int group )
{
    char full_cfg_path[MAXPATHLEN];
    char full_cur_cfg_path[MAXPATHLEN];
    char cmd[MAXPATHLEN+80];

    // Check if this group is a fictitious group created for network bandwidth analysis
	// If so, log a message prompting to cleanup such files and return TRUE status
    sprintf( full_cfg_path, "%s/p%03d.cfg", PATH_CONFIG, group );
    if( is_network_analysis_cfg_file( full_cfg_path ) )
	{
        reporterr( ERRFAC, M_ISNETANALCFG, ERRINFO, full_cfg_path );
		return( 1 );
	}
    return( 0 );
}

/*
 * starts the driver, if needed
 *
static int
start_driver(char *psname)
{
    struct stat sb;
    int ctlfd;
    
    if (stat(psname, &sb))
        return -1;
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return (-1);
    }
    if (FTD_IOCTL_CALL(ctlfd, FTD_SET_PERSISTENT_STORE, &sb.st_rdev) && errno != EBUSY) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, psname, strerror(errno));
        return -1;
    }
    return 0;
}*/

int 
main(int argc, char *argv[])
{
    int i, fd, group, all, force, lgcnt, pid, pcnt, chpid, verbose;
    int ch; 
    char **setargv;
    int exitcode;
#if defined(HPUX)
    struct stat statbuf; 
    char cmd[128];
    int bfd;
#endif
    int gflag = 0;		/* -g option flag */
    int ret;
    int hrt_type = FTD_HS_NOT_SET, ctlfd, grp;
    char pstore[MAXPATHLEN];
    ps_version_1_attr_t attr;
	int check_group, found_net_analysis_config;
	int some_old_pstores;
	int num_of_started_groups, num_of_autostart_groups;
	int time_elapsed, max_wait_time;
    FILE *fp;
	char command[256], buf[16];
	int  IsRHEL7;

    FTD_TIME_STAMP(FTD_DBG_FLOW1, "Start\n");

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }
    progname = argv[0];
    if (argc < 2) {
        goto usage_error;
    }
    argv0 = argv[0];
    all = 0;
    force = 0;
    group = -1;
    verbose = 0;
        
    setmaxfiles();
    autostart = 0;
    /* process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "afbvg:p")) != EOF) {
        switch(ch) {
        case 'a':
            if (gflag) {
                fprintf(stderr, "Only one of -a and -g options should be specified\n");
                goto usage_error;
            }
            all = 1;
            break;
        case 'f':
            force = 1;
            break;
        case 'g':
            if (all != 0) {
                fprintf(stderr, "Only one of -a and -g options should be specified\n");
                goto usage_error;
            }
            if (gflag) {
                fprintf(stderr, "-g options are multiple specified\n");
                goto usage_error;
            }
            group = ftd_strtol(optarg);
            if (group < 0 || group >= MAXLG) {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                goto usage_error;
            }
            gflag++;
            break;
        case 'b':
            autostart = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'p':  // This is a hidden option, to check compatibility with Pstore migrations to RFX 2.7.2
		    // Note: dtcpsmigrate272 calls dtcstart with this option to chceck its compatibility.
            fprintf(stderr, "This version of dtcstart is compatible with pstore migrations to the format of release 2.7.2.\n");
			exit( 0 );
            break;
        default:
            goto usage_error;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Invalid arguments\n");
        goto usage_error;
    }

    if ((all == 0) && gflag == 0) {
        fprintf(stderr, "At least, one of -a and -g options should be specified\n");
        goto usage_error;
    }
    if (initerrmgt(ERRFAC) != 0) {
       /* NO need to exit here because it causes HP-UX */
       /* replication devices' startup at boot-time to fail */
       /* PRN# 498       */
    }
/*
 * Added code to check whether pstore migration is in progress.
 * If the pstore migration utility is running then do not start any groups.
 */ 
    if (getprocessid(QNM "psmigrate", 0, &pcnt) > 0) {
	fprintf(stderr, "Pstore migration is in progress. Please start the groups once it is completed.\n");
	exit(1);
    }
    if (getprocessid(QNM "psmigrate272", 0, &pcnt) > 0) {
	fprintf(stderr, "Pstore migration is in progress. Please start the groups once it is completed.\n");
	exit(1);
    }

    log_command(argc, argv);   /* trace command in dtcerror.log */
  
#if defined(HPUX)
    if (stat(FTD_CTLDEV, &statbuf) != 0) {
    #if (SYSVERS >= 1123)
        /* <<< This <#if> section temporarily includes the 11.31 version of HP-UX, until mksf is debugged on 11.31.
           From the OS Release Notes, mksf is supported in 11.31 and is upgraded for the agile view. We have to find out
           why it does not work (pc071029) */
 
	sprintf(cmd, "/usr/bin/mkdir /dev/%s", QNM);
	system (cmd);
	sprintf(cmd, "/usr/sbin/mknod /dev/%s/%s c `/usr/sbin/lsdev -h -d %s | /usr/bin/awk '{ print $1 }' | /usr/bin/head -n 1` 0xffffff",
		QNM, "ctl", QNM);   /* Corrected from /dev/dtc/dtc to /dev/dtc/ctl (pc071024) */
        system (cmd);
        memset (cmd, 0, sizeof(cmd));
        sprintf(cmd, "/usr/bin/chmod 644 /dev/%s/%s", QNM, "ctl");
        system (cmd);
        sprintf(cmd, "/usr/bin/chown bin /dev/%s/%s", QNM, "ctl");
        system (cmd);
        sprintf(cmd, "/usr/bin/chgrp bin /dev/%s/%s", QNM, "ctl");
    #else
        sprintf(cmd, "/etc/mksf -d %s -m 0xffffff -r %s", QNM,FTD_CTLDEV);
    #endif
        system (cmd);
    }
#endif
    /* config.c bogusity */
    paths = (char*)configpaths;

    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
	    dtc_open_err_check_driver_config_file_and_log_error_message(errno);
        return (-1);
    }

    // Clear the vector of Pstore sizes
	for( i = 0; i < FTD_MAX_GROUPS; i++ )
	{
	    Pstore_size_KBs[i] = 0;
	}

    /*
	HRT type is global to all pstores on a server (Small, Large or Proportional); dtcstart asks the driver
	what is the type of HRT in use on the server; if the driver has just been loaded, such as after an install
	or reboot, it replies that it does not know the HRT type;
    
    then dtcstart scans all the pstores defined in the cfg files UNTIL it finds a good pstore from which it
    can deduce the HRT type; in this case, it gives this HRT type information to the driver and continues;
	
	if no valid pstore is found AND NO OLD-VERSION pstore is detected, which is a possible and valid scenario,
	dtcstart tells the driver that the HRT type will be the default type: Proportional HRT with Medium tracking
	resolution; then the groups started will have their pstores formatted with this HRT type; this was already in our previous releases;

    what is new for PROD12671 is: if no valid pstore is found AND SOME OLD-VERSION pstores are detected,
    which need to be migrated, then dtcstart must error out; it must not give a default HRT type to the driver,
    because once this is done, this HRT type will prevail, and this may break the migration of old pstores because
    the driver will disagree if we end up migrating a pstore which had Small HRT, for instance, and we told the driver
    that the HRT type would be Proportional medium. So dtcstart errors out with an error message.
    */

    // Get the HRT type from the driver
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
    /*
     * If driver does not have the information, read pstores to get the value
     * of HRT type. Update the driver with this value.
     */
    if (hrt_type == FTD_HS_NOT_SET) {
        lgcnt = GETCONFIGS(configpaths, 1, 0);
        if (lgcnt == 0) {
                reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
                close(ctlfd);
                exit(1);
        }
		some_old_pstores = 0; // To detect if pstore migration is needed
        for (i = 0; i < lgcnt; i++)
        {
            grp = cfgtonum(i);
            if (GETPSNAME(grp, pstore) != 0) {
                fprintf(stderr, "Unable to retrieve pstore location from CFG file for group %d\n", grp);
                close(ctlfd);
                exit(1);
            }
#if defined(linux)
	        IsRHEL7 = check_if_RHEL7();
            // If rebooting on RHEL7 Linux, check if Pstore not accessible yet; if so, wait max_wait seconds with retries (RHEL 7 defects)
            if( IsRHEL7 && autostart )
			{
		         int max_wait = 120;
		         int time_waited = 0;
		         int retry = 1;
				 int fd;

		         while( retry && (time_waited < max_wait) )
		         {  
                     if( (fd = open(pstore, O_RDWR | O_SYNC)) == -1 )
					 {
					     // Cannot open the Pstore. Retry with timeout.
						 sleep( 5 );
						 time_waited += 5;
					     sprintf(debug_msg, "Failed opening the pstore %s to get the HRT type; retrying (time_waited: %d seconds; max_wait = %d seconds)...\n", pstore, time_waited, max_wait);
					     reporterr (ERRFAC, M_GENMSG, ERRINFO, debug_msg);
					 }
					 else
					 {
					     close( fd );
					     retry = 0;
					 } 
				 } //...of while retry...
			}
#endif

            if ( (ret = ps_get_version_1_attr(pstore, &attr, 0)) == PS_OK)
            {
			    // PStore initialized OK; give HRT type to the driver
                hrt_type = attr.hrdb_type;
                FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
                break;
            }
			else if( ret == PS_INVALID_PS_VERSION )
			{
				some_old_pstores = 1;
			}
        }
    }
    if( (hrt_type == FTD_HS_NOT_SET) && some_old_pstores )
	{
	    // We did not find an initialized Pstore of this RFX release; if some old version Pstore has been found
		// and dtcstart has been run before pstore migration by mistake, we don't want to give the default HRT type
		// to the driver, which could break the Pstore migration.
		fprintf( stderr, "No initialized Pstore has been found and some old version pstores have been detected.\n" );
		fprintf( stderr, "These old Pstores must be migrated or reformatted before dtcstart is run.\n" );
        close(ctlfd);
		exit( 1 );
	}
    if (hrt_type == FTD_HS_NOT_SET && !autostart)
    {
	    // If the driver does not have the HRT type and no Pstore was initialized,
		// but no old-version pstore detected, set the default to PROPORTIONAL HRT
        hrt_type = FTD_HS_PROPORTIONAL;
        FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
        reporterr(ERRFAC, M_HRTSIZE, ERRINFO, ps_get_tracking_resolution_string(PS_DEFAULT_TRACKING_RES));
        reporterr(ERRFAC, M_HRTSIZE2, ERRINFO);
    }
    close(ctlfd);

    // Get paths of all groups to check the config files */
    lgcnt = GETCONFIGS(configpaths, 1, 0);
	// Check if there are any Network analysis configuration files; these groups cannot be launched
	// in real mode.
	found_net_analysis_config = 0;
    if (all != 0)
    {
        for (i = 0; i < lgcnt; i++)
	    {
	        check_group = cfgtonum(i);
	        if( check_if_network_analysis_cfg_file(check_group) == 1)
			    found_net_analysis_config = 1;
		}
	}
	else
	{
        found_net_analysis_config = (check_if_network_analysis_cfg_file(group) == 1);
	}
	if( found_net_analysis_config )
	{
	    if( all )
            reporterr(ERRFAC, M_STARTGRPS, ERRCRIT);
		else
            reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group, -1);
	    exit( -1 );
	}

    exitcode = 0;
    if (all != 0) {
        if (autostart) {
            /* get paths of previously started groups */
            lgcnt = GETCONFIGS(configpaths, 1, 2);
            if (lgcnt == 0) {
                exit(1);
            } else {
			    num_of_autostart_groups = lgcnt; // Take note of number of groups to check successful start	at reboot (defect 74194)
			}
        } else {
            /* get paths of all groups */
            lgcnt = GETCONFIGS(configpaths, 1, 0);
            if (lgcnt == 0) {
                reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
                exit(1);
            }
        }

        if (verbose)
			printf("Number of " CAPGROUPNAME " Group to start: %d\n", lgcnt);

        for (i = 0; i < lgcnt; i++) {
            group = cfgtonum(i);

			if (verbose)
				printf("Start " CAPGROUPNAME " Group no: %d\n", group);

            if ((ret = start_group(group, force, autostart))!=0) {
			    if( all && autostart )
			    {
			        --num_of_autostart_groups; // Decrement number of groups for which to check successful start
				}
			    if (ret == -2)
				{
                    undo_start_group( group );
				}
		        if (ret == 1) {
		           exitcode = 1;
		        } else {
                   if( !grp_started ) reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group, ret);
                   exitcode = -1;
		        }
            }
        }

		// If we are rebooting on RHEL7, check that groups are started (with timeout) (defect 74194)
#if defined(linux)
        IsRHEL7 = check_if_RHEL7();
	    if( IsRHEL7 && all && autostart && (num_of_autostart_groups > 0) )
	    {
		    max_wait_time = 300;  // 5-minute timeout
			time_elapsed = 0;
			num_of_started_groups = 0;

		    sprintf(command, "/opt/SFTKdtc/bin/dtcinfo -a | /usr/bin/grep Mode | wc -l 2> /dev/null");

			while( (num_of_started_groups <	num_of_autostart_groups) && (time_elapsed < max_wait_time) )
			{
			    fp = popen(command, "r");
			    if (fp == NULL)
			    {
			        /* popen() execution error */
				    sprintf( debug_msg, "dtcstart: popen error for checking start of groups at reboot..." );
					reporterr( ERRFAC, M_GENMSG, ERRWARN, debug_msg );
					break;
			    }
			    else
				{
					if (fgets(buf, sizeof(buf), fp) == NULL)
					{
					    sprintf( debug_msg, "dtcstart: fgets error for checking start of groups at reboot..." );
						reporterr( ERRFAC, M_GENMSG, ERRWARN, debug_msg );
						pclose(fp);
						break;
					}
					else
					{
						num_of_started_groups = atoi(buf);
						if( num_of_started_groups <	num_of_autostart_groups )
						{
						    sprintf( debug_msg, "dtcstart: %d groups (out of %d) completed start; %d seconds left before timeout...",
						             num_of_started_groups, num_of_autostart_groups, (max_wait_time - time_elapsed) );
							reporterr( ERRFAC, M_GENMSG, ERRWARN, debug_msg );
						    pclose(fp);
							sleep( 5 );
							time_elapsed += 5;
						}
					}
				}
			} // ...while
		} // ...IsRHEL7 && all && autostart
#endif
    } else {
		if (verbose)
			printf("Start " CAPGROUPNAME " Group no: %d\n", group);

        if ((ret = start_group(group, force, autostart))!=0) {
			if (ret == -2)
			{
                undo_start_group( group );
			}
        	if (ret == 1) {
        	    exitcode = 1;
        	} else {
               if( !grp_started ) reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group, ret);
               exitcode = -1;
        	}
        } 
    }
    exit(exitcode);
    return exitcode; /* for stupid compiler */
usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}

/* The Following 3 lines of code are intended to be a hack 
   only for HP-UX 11.0 . 
   HP-UX 11.0 compile complains about not finding the below
   function when they are staticaly linked - also we DON'T want
   to link it dynamically. So for the time being - ignore these
   functions while linking. */ 
#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif
