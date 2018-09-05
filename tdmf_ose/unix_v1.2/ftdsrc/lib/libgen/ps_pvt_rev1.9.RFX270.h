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
 * ps_pvt_rev1.9.RFX270.h - Persistent store private interface as of release 2.7.0. 
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

#include "ps_intr.h"

#define PS_VERSION_1_MAGIC		0xBADF00D5
#define PS_VERSION_1_MAGIC_RFX270	PS_VERSION_1_MAGIC
#define PS_VERSION_1_MAGIC_OLD1		0xBADF00D1
#define PS_VERSION_1_MAGIC_OLD2		0xBADF00D2
#define PS_VERSION_1_MAGIC_OLD3		0xBADF00D3
#define PS_VERSION_1_MAGIC_OLD4		0xBADF00D4
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

/*
 * An entry in our device table
 */
typedef struct _ps_dev_entry_ {
    char         path[MAXPATHLEN];
    int          pathlen;
    int          state;
    unsigned int num_lrdb_bits;
    unsigned int num_hrdb_bits;
    unsigned int num_sectors;
    u_longlong_t ackoff;
    
    /* for future use */
#if defined(_LP64) || _FILE_OFFSET_BITS == 32 
    int          pad[7];
#elif _FILE_OFFSET_BITS == 64
    int          pad[3];
#endif

} ps_dev_entry_t;

#endif /* _PS_PVT_H_ */

