/*
 * ftd_ps.h - Persistent store interface
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
#ifndef _FTD_PS_H_
#define _FTD_PS_H_

#include "ftd_ps_pvt.h"

/*
 * Default sizes for persistent store and logical groups
 */
#define FTD_PS_LRDB_SIZE        8*1024
#define FTD_PS_HRDB_SIZE        128*1024
#define FTD_PS_GROUP_ATTR_SIZE  4*1024
#define FTD_PS_DEVICE_ATTR_SIZE 4*1024

/* offset from beginning of slice to the pstore header in 1024 byte blocks */
#define PS_HEADER_OFFSET      16  /* 16K */

#define FTD_MAX_KEY_VALUE_PAIRS	1024

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

/*
 * The relevant fields for a version 1 persistent store
 */
typedef struct _ps_version_1_attr_ {
    unsigned int  max_dev;
    unsigned int  max_group;
    unsigned int  dev_attr_size;
    unsigned int  group_attr_size;
    unsigned int  lrdb_size;
    unsigned int  hrdb_size;
    unsigned int  dev_table_entry_size;
    unsigned int  group_table_entry_size;
    unsigned int  num_device;
    unsigned int  num_group;
    int           last_device;
    int           last_group;
} ps_version_1_attr_t;

/*
 * info stored for each device
 */
typedef struct _ps_dev_info {
    char         *name;         /* device name */
    int          state;         /* state of the device */
    unsigned int num_lrdb_bits; /* number of valid bits in LRDB */
    unsigned int num_hrdb_bits; /* number of valid bits in HRDB */
    unsigned int num_sectors;   /* number of sectors in device */
} ps_dev_info_t;

/*
 * info stored for each group
 */
typedef struct _ps_group_info {
    char         *name;         /* group name */
    int          state;         /* state of the group */
    unsigned int hostid;        /* host id that owns this group */
    int          shutdown;      /* proper shutdown flag */
    int          autostart;     /* for start -b */
    int          checkpoint;    /* checkpoint flag */
} ps_group_info_t;

/* functions: */
extern int ps_get_lrdb_offset(char *ps_name, char *dev_name, unsigned int *offset);
extern int ps_get_lrdb(char *ps_name, char *dev_name, char *buffer, 
                int buf_len, unsigned int *num_bits);
extern int ps_get_hrdb(char *ps_name, char *dev_name, char *buffer, 
                int buf_len, unsigned int *num_bits);
extern int ps_set_lrdb(char *ps_name, char *dev_name, char *buffer, int buf_len);
extern int ps_set_hrdb(char *ps_name, char *dev_name, char *buffer, int buf_len);
extern int ps_get_group_key_value(char *ps_name, char *group_name, 
                           char *key, char *value);
extern int ps_set_group_key_value(char *ps_name, char *group_name, 
                           char *key, char *value);
extern int ps_get_group_attr(char *ps_name, char *group_name, 
                      char *buffer, int buf_len);
extern int ps_set_group_attr(char *ps_name, char *group_name, 
                      char *buffer, int buf_len);
extern int ps_get_device_attr(char *ps_name, char *group_name, 
                       char *buffer, int buf_len);
extern int ps_set_device_attr(char *ps_name, char *group_name, 
                       char *buffer, int buf_len);
extern int ps_set_device_size(char *ps_name, char *group_name, 
                       unsigned int num_sectors);
extern int ps_add_group(char *ps_name, ps_group_info_t *group_info);
extern int ps_delete_group(char *ps_name, char *group_name);
extern int ps_add_device(char *ps_name, ps_dev_info_t *dev_info);
extern int ps_delete_device(char *ps_name, char *dev_name);
extern int ps_create_version_1(char *ps_name, ps_version_1_attr_t *attr);
extern int ps_get_version_1_attr(char *ps_name, ps_version_1_attr_t *attr);
extern int ps_get_version(char *ps_name, int *version);
extern int ps_get_device_list(char *ps_name, char *buffer, int buf_len);
extern int ps_get_group_list(char *ps_name, char *buffer, int buf_len);
extern int ps_get_device_index(char *ps_name, char *dev_name, int *index);
extern int ps_get_group_info(char *ps_name, char *group_name, 
                      ps_group_info_t *group_info);
extern int ps_get_device_info(char *ps_name, char *dev_name, ps_dev_info_t *dev_info);
extern int ps_set_group_state(char *ps_name, char *group_name, int state);
extern int ps_set_group_shutdown(char *ps_name, char *group_name, int value);
extern int ps_set_group_autostart(char *ps_name, char *group_name, int autostart);
extern int ps_set_group_checkpoint(char *ps_name, char *group_name, int checkpoint);

extern int ps_create(char *ps_name, int *max_dev);

extern int ps_create_group(char *ps_name, ps_group_info_t *info,
	char *buffer, int buf_len, int state);
extern int ps_create_device(char *ps_name, ps_dev_info_t *info,
	int *lrdb, int lrdb_len, int *hrdb, int hrdb_len);

#endif /* _FTD_PS_H_ */
