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
 * ps_migrate.h --> All the old pstore structures are defined here.
 */

#define MAXPSFILESIZE    157286400
#define PS_TMP_FILE_PATH "/tmp/"

#define PS_ERR		 -1

typedef struct _ps_dev_ver1_ {
    char         path[MAXPATHLEN];
    int          pathlen;
    int          state;
    unsigned int num_lrdb_bits;
    unsigned int num_hrdb_bits;
    unsigned int num_sectors;
    int          pad[11];
} ps_dev_ver1_t;

typedef struct _ps_dev_ver3_ {
    char         path[MAXPATHLEN];
    int          pathlen;
    int          state;
    unsigned int num_lrdb_bits;
    unsigned int num_hrdb_bits;
    unsigned int num_sectors;
    off_t        ackoff;
    /* for future use */
#if defined(_LP64) || _FILE_OFFSET_BITS == 32
    int          pad[7];
#elif _FILE_OFFSET_BITS == 64
    int          pad[3];
#endif

} ps_dev_ver3_t;

//================== RFX 2.7.1 specific definitions ===============
// On AIX, the size of this structure is (running legacy RFX 2.7.0 code):
// DTC: [INFO / GENMSG]: Device entry size (sizeof(ps_dev_entry_t)) = 1056
// For backward compatibility with pre-RFX271 Pstore format, the field offsets of this
// structure must remain unchanged.
typedef struct _ps_dev_entry_RFX271_ {
    char         path[MAXPATHLEN]; // 1 KB + 1 on AIX (PATH_MAX+1), so should be 1028 (padding)
    int          pathlen;          // 4 bytes
    int          state;            // 4 bytes <<< this is unused; renamed later as unused_32bits for future use
    unsigned int num_lrdb_bits;	   // 4 bytes
    unsigned int num_hrdb_bits;	   // 4 bytes
	// The following field was previously num_sectors, now replaced by ps_dev_HRDB_info_entry.num_sectors_64bits
	// We must keep a 32-bit field here for backward compatibility with RFX2.7.0 Pstores
    unsigned int unused_32bits;	   // 4 bytes
    unsigned long long ackoff;		   // 8 bytes
	// Total size of used fields: 1056 bytes
	// WARNING: so the padding below (provision for future use was not even compiled in on AIX)
    /* Comment said: "for future use": */
#if defined(_LP64) || _FILE_OFFSET_BITS == 32
    int          pad[7];
#elif _FILE_OFFSET_BITS == 64
    int          pad[3];
#endif

} ps_dev_entry_RFX271_t;

/*
 * An entry in our device HRDB information table
 * Note: this is physically	written to the Pstore
 */
typedef struct _ps_dev_entry_2_RFX271_t_ { // Was _ps_dev_HRDB_info_entry_ in RFX271 but renamed to _ps_dev_entry_2_t_ in RFX272
    char         path[MAXPATHLEN];
    int          pathlen;
	unsigned int hrdb_resolution_KBs_per_bit;
	unsigned int hrdb_size;               // In bytes
	unsigned int dev_HRDB_offset_in_KBs;  // Device's hrdb offset
	int          has_reusable_HRDB;       // To detect that a deleted device left HRDB space unused
    // The num_sectors field of struct ps_dev_entry_t is not OK for TB devices; it was previously unused,
    // but now we need this information and it needs 64 bits; since
	// we must stay compatible with RFX 2.7.0 Pstore format, we cannot change its size in the old
	// structure; this is why the following field is ceated here (PROD8682)
	unsigned long long num_sectors_64bits;
	unsigned long long orig_num_sectors_64bits;	// Original num of sectors before device expansion
    unsigned long long limitsize_multiple; // Device size fudge factor for dev expansion provision
	unsigned int unused[4];                // Future use 
} ps_dev_entry_2_RFX271_t;


//================== RFX 2.7.0 definitions ===============
/*
 * Version 1 persisent store header 
 */
typedef struct _ps_version_1_hdr_RFX270_ {
    unsigned int max_dev;
    unsigned int max_group;
    unsigned int dev_attr_size;
    unsigned int group_attr_size;
    unsigned int dev_table_entry_size;
    unsigned int group_table_entry_size;
    unsigned int lrdb_size;
    unsigned int hrdb_size;

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
    unsigned int hrdb_offset;
    unsigned int last_block;   /* last block in the store */
} ps_version_1_hdr_RFX270_t;

/*
 * If we add a new version, just stuff it into the union. The magic
 * number will tell us which structure to access.
 */
typedef struct _ps_hdr_RFX270_ {
    unsigned int magic;
    union {
        ps_version_1_hdr_RFX270_t ver1;
    } data;
} ps_hdr_RFX270_t;

/*
 * An entry in our group table
 */
typedef struct _ps_group_entry_RFX270_ {
    char         path[MAXPATHLEN];
    int          pathlen;
    int          state;
    unsigned int hostid;
    int          shutdown;      /* TRUE, if group was shutdown properly */
    int          checkpoint;    /* TRUE, if group is in checkpoint state */
    /* for future use */
    int          pad[11];
} ps_group_entry_RFX270_t;

/*
 * An entry in our device table
 */
typedef struct _ps_dev_entry_RFX270_ {
    char         path[MAXPATHLEN];
    int          pathlen;
    int          state;
    unsigned int num_lrdb_bits;
    unsigned int num_hrdb_bits;
    unsigned int num_sectors;
    unsigned long long  ackoff;
    
    /* for future use */
#if defined(_LP64) || _FILE_OFFSET_BITS == 32 
    int          pad[7];
#elif _FILE_OFFSET_BITS == 64
    int          pad[3];
#endif

} ps_dev_entry_RFX270_t;



