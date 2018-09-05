/*
 * ftd_ps_pvt.h - Persistent store private interface 
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _FTD_PS_PVT_H_
#define _FTD_PS_PVT_H_

#include "ftd_ps.h"

#define PS_VERSION_1_MAGIC 0xBADF00D1

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
        char dummy[DEV_BSIZE - sizeof(unsigned int)];
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
    int          shutdown; /* TRUE, if group was shutdown properly */
    int          autostart; /* TRUE, if group is to be automatically started at boot*/
    int          checkpoint; /* TRUE, if group is in checkpoint mode */
    /* for future use */
    int          pad[4];
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
    /* for future use */
    int          pad[11];
} ps_dev_entry_t;

#endif /* _FTD_PS_PVT_H_ */

