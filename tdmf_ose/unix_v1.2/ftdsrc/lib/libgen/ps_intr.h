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
#ifndef _PS_INTR_H_
#define _PS_INTR_H_
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif


/*
 * ps_intr.h - Persistent store interface
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

/* offset from beginning of slice to the pstore header in 1024 byte blocks */
#define PS_HEADER_OFFSET      16  /* 16K */

/* define the header versions */
#define PS_LARGE_HRDB                 0
#define PS_SMALL_HRDB                 1
#define PS_LATEST_HEADER              1
#define PS_OLD1_HEADER                2 // RFX 2.2.0.0 and 2.1
#define PS_OLD2_HEADER                3 // RFX 2.2 and 2.3
#define PS_OLD3_HEADER                4 // RFX 2.4 and 2.4.2.0 
#define PS_OLD4_HEADER                5 // RFX 2.5, 2.5.3.0	and 2.6.0
#define PS_LARGE_HRDB_PRE_RFX271      6 // Pstore formats up to RFX 2.7.0, still supported in RFX 2.7.1
#define PS_SMALL_HRDB_PRE_RFX271      7 // Pstore formats up to RFX 2.7.0, still supported in RFX 2.7.1
#define PS_PROPORTIONAL_HRDB_HEADER   8
#define PS_LARGE_HRDB_RFX271          9
#define PS_SMALL_HRDB_RFX271          10
#define PS_PROPORTIONAL_HRDB_RFX271   11

// Tracking Resolution codes for HRDB Resolution determination (Proportional HRDB)
#define PS_LOW_TRACKING_RES      1
#define PS_MEDIUM_TRACKING_RES   2
#define PS_HIGH_TRACKING_RES     3
#define NUMBER_OF_TRACKING_RESOLUTIONS  3

#define PS_DEFAULT_TRACKING_RES  PS_MEDIUM_TRACKING_RES  // Default Tracking Resolution is MEDIUM

// Default target (desired) HRDB Resolutions related to the different Tracking Resolution levels
// WARNING: these values must be power of 2
#define PS_LOW_RES_GRANULARITY     64 // KB per HRDB bit
#define PS_MEDIUM_RES_GRANULARITY  32
#define PS_HIGH_RES_GRANULARITY    8

// Default HRDB size maximum values related to the different Tracking Resolution levels
#define PS_LOW_MAX_HRDB_SIZE       4096 // KB
#define PS_MEDIUM_MAX_HRDB_SIZE    8192
#define PS_HIGH_MAX_HRDB_SIZE      16384

/* function return values, if anyone cares */
#define PS_OK                  0
#define PS_BOGUS_PS_NAME      -1
#define PS_BOGUS_GROUP_NAME   -2
#define PS_BOGUS_DEVICE_NAME  -3
#define PS_BOGUS_BUFFER_LEN   -4
#define PS_INVALID_PS_VERSION -5
#define PS_NO_ROOM            -6
#define PS_BOGUS_HEADER       -7
#define PS_SEEK_ERROR         -8
#define PS_MALLOC_ERROR       -9
#define PS_READ_ERROR         -10
#define PS_WRITE_ERROR        -11
#define PS_DEVICE_NOT_FOUND   -12
#define PS_GROUP_NOT_FOUND    -13
#define PS_BOGUS_PATH_LEN     -14
#define PS_BOGUS_CONFIG_FILE  -15
#define PS_KEY_NOT_FOUND      -16
#define PS_DEVICE_MAX         -17 
#define PS_NO_HRDB_SPACE      -18
#define PS_NO_PROP_HRDB       -19
#define PS_HRDB_TABLE_FULL    -20
#define PS_DEV_EXPANSION_EXCEEDED -21
/*
 * The relevant fields for a version 1 persistent store
 * NOTE: this is run time used information, it does not dictate the actual physical
 *       format in the Pstore itself, so it does not affect compatibility with previous
 *       Pstore version.
 */
typedef struct _ps_version_1_attr_ {
    unsigned int  max_dev;
    unsigned int  max_group;
    unsigned int  dev_attr_size;
    unsigned int  group_attr_size;
    unsigned int  lrdb_size;
    unsigned int  Small_or_Large_hrdb_size;  // This field is valid only in Legacy Small or Large HRT modes
    unsigned int  dev_table_entry_size;
    unsigned int  group_table_entry_size;
    unsigned int  num_device;
    unsigned int  num_group;
    int           last_device;
    int           last_group;
    unsigned int  hrdb_type;
    unsigned int  tracking_resolution_level; // Applicable only to Proportional HRDB type (high, medium, low)
    unsigned int  max_HRDB_size_KBs;         // Maximum HRDB size associated with the Tracking resolution level
	unsigned int  dev_HRDB_info_entry_size;	 // Size of an entry in the device HRDB info table
	u_longlong_t  next_available_HRDB_offset;
} ps_version_1_attr_t;

/*
 * Info for each device
 * Note: this is runtime usage info and NOT what is written physically in the Pstore,
 *       so additional fields do not compromise backward compatibility on Pstore format.
 *       For what is physically in the Pstore, see ps_pvt.h.
 * PROD8693: the unused state field has now been removed from the structure
 */
typedef struct _ps_dev_info {
    char                *name;                    /* device name */
    unsigned int        info_allocated_lrdb_bits; /* number of allocated bits in LRDB */
    unsigned int        info_allocated_hrdb_bits; /* number of allocated bits in HRDB */
    unsigned int        info_valid_lrdb_bits;     /* number of valid bits in LRDB */
    unsigned int        info_valid_hrdb_bits;     /* number of valid bits in HRDB */
    unsigned long long  num_sectors;              /* number of sectors in device */
	unsigned int        lrdb_res_sectors_per_bit;
    unsigned int        lrdb_res_shift_count;     // resolution = 1 << lrdb_res_shift_count (calculated by the driver)
	unsigned int        hrdb_resolution_KBs_per_bit; // See warning below, and WR PROD10058
	unsigned int        hrdb_size;                // In bytes
	unsigned int        dev_HRDB_offset_in_KBs;   // Device's hrdb offset 
    unsigned long long  limitsize_multiple;       // Device size fudge factor for dev expansion provision
    unsigned long long  orig_num_sectors;         // Original number of sectors before any device expansion
} ps_dev_info_t;
/*
  WARNING: the field hrdb_resolution_KBs_per_bit can happen to take on a value of 0 in the old Small
  and Large HRT modes, where the resolution can be of 512 bytes per bit for small devices.
  In Proporional HRT mode (default mode of RFX 2.7.2), this problem cannot occur because the minimum 
  resolution is 1 KB per bit. This field is used only for information purposes in the old Small and 
  Large HRT modes and dtcinfo is already fixed to cope with that (see ftd_info.c, function dump_devinfo()). 
  This field is used for computation purposes only in Proportional HRT mode, where the problem cannot occur. 
  IF YOU DECIDE TO USE THIS FIELD FOR COMPUTATION PURPOSES IN THE OLD SMALL OR LARGE HRT MODES IN A FUTURE RELEASE
  (after RFX 2.7.2), you need to take into account the possible value of 0 (resolution of 512 bytes per tracking bit).
  SEE WR PROD10058.
*/

/*
 * info stored for each group
 */
typedef struct _ps_group_info {
    char         *name;         /* group name */
    int          state;         /* state of the group */
    unsigned int hostid;        /* host id that owns this group */
    int          shutdown;      /* proper shutdown flag */
    int          checkpoint;    /* checkpoint flag */
} ps_group_info_t;

/*
 * Replication profile structures for Proportional HRDB
 */
typedef struct _tracking_resolution_info {
    int          level;                      // Tracking resolution level numeric code to store in Pstore header (for low, medium, high)
	unsigned int bit_resolution_KBs_per_bit; // HRDB resolution in KBytes per bit
	unsigned int max_HRDB_size_KBs;          // Max HRDB size at this Tracking resolution level 
} tracking_resolution_info;

/* functions: */
int ps_get_lrdb_offset(char *ps_name, char *dev_name, unsigned int *offset);
int ps_get_lrdb(char *ps_name, char *dev_name, char *buffer, 
                int buf_len, unsigned int *num_bits);
int ps_get_hrdb(char *ps_name, char *dev_name, char *buffer, 
                int buf_len, unsigned int *num_bits);
int ps_set_lrdb(char *ps_name, char *dev_name, char *buffer, int buf_len);
int ps_set_hrdb(char *ps_name, char *dev_name, char *buffer, int buf_len);
int ps_get_group_key_value(char *ps_name, char *group_name, 
                           char *key, char *value);
int ps_set_group_key_value(char *ps_name, char *group_name, 
                           char *key, char *value);
int ps_get_group_attr(char *ps_name, char *group_name, 
                      char *buffer, int buf_len);
int ps_set_group_attr(char *ps_name, char *group_name, 
                      char *buffer, int buf_len);
int ps_get_device_attr(char *ps_name, char *group_name, 
                       char *buffer, int buf_len);
int ps_set_device_attr(char *ps_name, char *group_name, 
                       char *buffer, int buf_len);
#ifdef USE_PS_SET_DEVICE_SIZE
// The following function is currently unused:
int ps_set_device_size(char *ps_name, char *group_name, 
                       unsigned int num_sectors);
#endif
int ps_add_group(char *ps_name, ps_group_info_t *group_info);
int ps_delete_group(char *ps_name, char *group_name, int group_number, int delete_devices_also);
int ps_add_device(char *ps_name, ps_dev_info_t *dev_info, tracking_resolution_info *HRDB_tracking_resolution,
                  long long Pstore_size_KBs, int hrdb_type);
int ps_delete_device(char *ps_name, char *dev_name);
int ps_create_version_1(char *ps_name, ps_version_1_attr_t *attr);
int ps_get_version_1_attr(char *ps_name, ps_version_1_attr_t *attr, int log_message);
int ps_get_version(char *ps_name, int *version);
int ps_get_device_list(char *ps_name, char (*buffer)[], int buf_len);
int ps_get_group_list(char *ps_name, char *buffer, int buf_len);
int ps_get_device_index(char *ps_name, char *dev_name, int *index);
int ps_get_group_info(char *ps_name, char *group_name, 
                      ps_group_info_t *group_info);
int ps_get_device_info(char *ps_name, char *dev_name, ps_dev_info_t *dev_info);
int ps_get_num_device(char *ps_name,int *cur_max_dev); 
int ps_set_group_state(char *ps_name, char *group_name, int state);
int ps_set_group_shutdown(char *ps_name, char *group_name, int value);
int ps_set_group_checkpoint(char *ps_name, char *group_name, int value);
int ps_dump_info(char *ps_name, char *group_name, char *inbuffer);

int create_ps(char *ps_name, int64_t *max_dev, tracking_resolution_info *HRDB_tracking_resolution);

/* FRF */
int ps_get_device_entry(char *ps_name, char *dev_name,u_longlong_t *ackoff);
int ps_set_device_entry(char *ps_name, char *dev_name,u_longlong_t ackoff);

// Dirty bit Tracking Resolution configuration for Proportional HRDB
int ps_get_tracking_resolution_info( tracking_resolution_info *tracking_res_info_ptr );

// Proportional HRDB size, offset and resolution get and set function
int ps_get_device_hrdb_info(char *ps_name, char *dev_name, unsigned int *hrdb_size,
	            unsigned int *hrdb_offset_in_KBs, unsigned int *lrdb_res_sectors_per_bit,
	            unsigned int *hrdb_resolution_KBs_per_bit, unsigned int *lrdb_res_shift_count,
	            unsigned long long *num_sectors, unsigned long long *limitsize_multiple,
	            unsigned long long *orig_num_sectors);
int	ps_set_device_hrdb_info(char *ps_name, char *dev_name, unsigned int hrdb_size,
                unsigned int hrdb_offset_in_KBs, unsigned int HRDB_resolution_KBs_per_bit, unsigned int LRDB_res_sectors_per_bit,
                unsigned int lrdb_res_shift_count, unsigned long long num_sectors, unsigned long long limitsize_multiple, int only_resolution);
int ps_set_device_limitsize_multiple(char *ps_name, char *dtc_dev_name, unsigned long long new_limitsize_multiple);
int ps_adjust_device_info(char *ps_name, char *dev_name, 
                   unsigned int hrdb_size, unsigned int previous_hrdb_size, unsigned int HRDB_resolution_KBs_per_bit, 
                   unsigned int LRDB_res_sectors_per_bit, unsigned int dev_hrdb_offset_KBs, long long Pstore_size_KBs, int hrdb_type,
                   int undo_next_Pstore_hrdb_offset, unsigned long long num_sectors, unsigned long long limitsize_multiple,
                   unsigned int lrdb_numbits, unsigned int hrdb_numbits, unsigned int lrdb_res_shift_count);

// Update the number of LRDB and HRDB bits for a specific device
int ps_adjust_lrdb_and_hrdb_numbits( char *ps_name, char *raw, unsigned int lrdb_numbits, unsigned int hrdb_numbits, int also_allocated_bits );

int	ps_update_num_sectors(char *ps_name, char *dev_name, unsigned long long new_dev_size);

int	ps_verify_expansion_provision(char *ps_name, char *dev_name, unsigned long long new_dev_size, unsigned long long *limitsize_factor_from_pstore);

int ps_clear_device_hrdb_entry(char *ps_name, char *dev_name);

long long ps_get_pstore_size_KBs(char *ps_name);

int ps_Pstore_supports_Proportional_HRDB(char *ps_name);

char *ps_get_tracking_resolution_string(int level);
char *ps_get_pstore_error_string(int error_code);

int  ps_get_hrdb_base_offset( char *ps_name, unsigned int *hrdb_base_offset );  // To get the base offset of the global HRDB area of a Pstore

#endif /* _PS_INTR_H_ */
