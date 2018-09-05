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
#ifndef _PS_PVT_H_
#define _PS_PVT_H_
/*
 * ps_pvt.h - Persistent store private interface 
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

/* -------------------------------- Pstore structure -----------------------------
- Unused space (start of volume): 16K
- Pstore header: 1K
    See structures ps_hdr_t and ps_version_1_hdr_t
- Device table: see structure ps_dev_entry_t
- Group table: see structure ps_group_entry_t
- Device attributes: 4KB per device; device statistics, structure devstat_t
- Group attributes: 4KB per group; these are the dtcset tunables
- Some padding to align on 8KB boundary
- LRDB (low resolution dirty bits) space: 8KB per device
- Device HRDB information table, structure ps_dev_entry_2_t
- HRDB space
    Legacy approach
      Small HRT: 128 KB per device
      Large HRT: 12 MB per device
    New Proportional HRDB approach: desired Tracking resolution level set by dtcinit (or default of MEDIUM)
      and device size determine the HRDB size, up to a maximum, and adjusted if necessary so that the tracking
	  resolution be a power of 2.
*/

#include "ps_intr.h"

#define PS_VERSION_1_MAGIC		          0xBADF00D7 // Starts at RFX TUIP 2.7.2
#define PS_VERSION_1_MAGIC_OLD1		      0xBADF00D1
#define PS_VERSION_1_MAGIC_OLD2		      0xBADF00D2
#define PS_VERSION_1_MAGIC_OLD3		      0xBADF00D3
#define PS_VERSION_1_MAGIC_OLD4		      0xBADF00D4
#define PS_VERSION_1_MAGIC_RFX26X_RFX270  0xBADF00D5 // Most recent release prior to Proportional HRDB
#define PS_VERSION_1_MAGIC_RFX271         0xBADF00D6 // Support of Proportional HRDB
/*
 * Version 1 persisent store header 
 */
typedef struct _ps_version_1_hdr_ {
    unsigned int max_dev;
    unsigned int max_group;
    unsigned int dev_attr_size;
    unsigned int group_attr_size;
    unsigned int dev_table_entry_size;
    unsigned int group_table_entry_size;
    unsigned int lrdb_size;
    // The following field is valid only in Legacy Small or Large HRT modes, but is compatible
	// with both new and pre-RFX2.7.1 Pstore formats (field just renamed, not moved):
    unsigned int Small_or_Large_hrdb_size;

    /* indices to help with table buffer allocation */
    unsigned int num_device;
    unsigned int num_group;
    int          last_device;
    int          last_group;

    /* 1K sector offsets to data */
    unsigned int dev_attr_offset;
    unsigned int group_attr_offset;
    unsigned int dev_table_offset;
    unsigned int group_table_offset;
    unsigned int lrdb_offset;
    unsigned int hrdb_offset;  // This is the base offset of the global HRDB area
    unsigned int last_block;   /* last block in the store */

	// The following fields are added in RFX 2.7.1 for support of Poportional HRDB. Since 1 KB is allocated
	// for this structure, adding these fields still preserves compatibility with previous Pstore format.
	// These fields must NOT be accessed in the case of a product upgrade from RFX 2.7.0 or earlier which would 
	// want to preserve an old Pstore format, in which case this upgade would keep its current HRDB type (Small or Large).
    unsigned int hrdb_type;
    unsigned int dev_HRDB_info_table_offset;  // Offset to devices' HRDB info table (also in KBs)
    unsigned int tracking_resolution_level;   // Applicable only to Proportional HRDB type (high, medium, low)
    unsigned int max_HRDB_size_KBs;           // Maximum HRDB size associated with the Tracking resolution level
	unsigned int next_available_HRDB_offset;  // Offset to next HRDB free space (in KBs)
	unsigned int dev_HRDB_info_entry_size;    // Size of an entry in the device HRDB info table

} ps_version_1_hdr_t;

/*
 * If we add a new version, just stuff it into the union. The magic
 * number will tell us which structure to access.
 */
typedef struct _ps_hdr_ {
    unsigned int magic;
    union {
        ps_version_1_hdr_t ver1;
    } data;
} ps_hdr_t;

/*
 * An entry in our group table
 */
typedef struct _ps_group_entry_ {
    char         path[MAXPATHLEN];
    int          pathlen;
    int          state;
    unsigned int hostid;
    int          shutdown;      /* TRUE, if group was shutdown properly */
    int          checkpoint;    /* TRUE, if group is in checkpoint state */
    /* for future use */
    int          pad[11];
} ps_group_entry_t;


// On AIX, the size of this structure is (running legacy RFX 2.7.0 code):
// DTC: [INFO / GENMSG]: Device entry size (sizeof(ps_dev_entry_t)) = 1056
// For backward compatibility with pre-RFX271 Pstore format, the field offsets of this
// structure must remain unchanged.
typedef struct _ps_dev_entry_ {
    char         path[MAXPATHLEN];        // 1 KB + 1 on AIX (PATH_MAX+1), so should be 1028 (padding)
    int          pathlen;                 // 4 bytes
    unsigned int ps_valid_lrdb_bits;      // 4 bytes (was unused "state" field, PROD8693); now used for valid number of LRDB bits (PROD10057)
    unsigned int ps_allocated_lrdb_bits;  // 4 bytes (total number of LRDB bits allocated in the Pstore for this device)
    unsigned int ps_allocated_hrdb_bits;  // 4 bytes (total number of HRDB bits allocated in the Pstore for this device)
	// The following field was previously num_sectors, now replaced by ps_dev_entry_2_t.num_sectors_64bits
	// We must keep a 32-bit field here for backward compatibility with RFX2.7.0 Pstores
    unsigned int ps_valid_hrdb_bits;      // Number of valid HRDB bits for this device (PROD10057)
    u_longlong_t ackoff;		          // 8 bytes
	// Total size of used fields: 1056 bytes
	// WARNING: so the padding below (provision for future use was not even compiled in on AIX)
    /* Comment said: "for future use": */
#if defined(_LP64) || _FILE_OFFSET_BITS == 32
    int          pad[7];
#elif _FILE_OFFSET_BITS == 64
    int          pad[3];
#endif

} ps_dev_entry_t;

/*
 * An entry in our device HRDB information table
 * Note: this is physically	written to the Pstore
 */
typedef struct _ps_dev_entry_2_t_ {
    char         path[MAXPATHLEN];
    int          pathlen;
	unsigned int lrdb_res_shift_count;
	unsigned int lrdb_res_sectors_per_bit;
	unsigned int hrdb_resolution_KBs_per_bit; // SEE WARNING BELOW and WR PROD10058
	unsigned int hrdb_size;               // In bytes
	unsigned int dev_HRDB_offset_in_KBs;  // Device's hrdb offset
	int          has_reusable_HRDB;       // To detect that a deleted device left HRDB space unused.
    // The num_sectors field of struct ps_dev_entry_t is not OK for TB devices; it was previously unused,
    // but now we need this information and it needs 64 bits; since
	// we must stay compatible with RFX 2.7.0 Pstore format, we cannot change its size in the old
	// structure; this is why the following field is ceated here (PROD8682)
	unsigned long long num_sectors_64bits;
	unsigned long long orig_num_sectors_64bits;	// Original num of sectors before device expansion
    unsigned long long limitsize_multiple; // Device size fudge factor for dev expansion provision
	unsigned int unused[4];                // Future use 
} ps_dev_entry_2_t;

/*
  WARNING: the field  hrdb_resolution_KBs_per_bit can happen to take on a value of 0 in the old Small
  and Large HRT modes, where the resolution can be of 512 bytes per bit for small devices.
  In Proporional HRT mode (default mode of RFX 2.7.2), this problem cannot occur because the minimum 
  resolution is 1 KB per bit. This field is used only for information purposes in the old Small and 
  Large HRT modes and dtcinfo is already fixed to cope with that (see ftd_info.c, function dump_devinfo()). 
  This field is used for computation purposes only in Proportional HRT mode, where the problem cannot occur. 
  IF YOU DECIDE TO USE THIS FIELD FOR COMPUTATION PURPOSES IN THE OLD SMALL OR LARGE HRT MODES IN A FUTURE RELEASE
  (after RFX 2.7.2), you need to take into account the possible value of 0 (resolution of 512 bytes per tracking bit).
  SEE WR PROD10058.
*/

#endif /* _PS_PVT_H_ */

