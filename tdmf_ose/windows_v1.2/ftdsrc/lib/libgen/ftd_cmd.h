#ifndef _FTD_CMD_H_
#define _FTD_CMD_H_
/*
 * ftd_cmd.h - parameters for all ftd commands
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* 
 * This is how we create a group name stored in the pstore. It is also
 * the device name for the logical group control device. 
 */
#define FTD_CREATE_GROUP_NAME(group_name, group_num) \
                      sprintf(group_name, "/dev/" QNM "/lg%d/ctl", group_num);

#define FTD_CREATE_GROUP_DIR(dir_name, group_num) \
                      sprintf(group_name, "/dev/" QNM "/lg%d", group_num);

/* the maximum number of key/value pairs of tunables */
#define FTD_MAX_KEY_VALUE_PAIRS 1024

/* this string is used to set the default tunables for a new logical group */
#define FTD_DEFAULT_TUNABLES "CHUNKSIZE: 256\nCHUNKDELAY: 0\nSYNCMODE: off\n\
SYNCMODEDEPTH: 1\nSYNCMODETIMEOUT: 30\nIODELAY: 0\nNETMAXKBPS: -1\nSTATINTERVAL: 10\n\
MAXSTATFILESIZE: 64\nLOGSTATS: on\n\
TRACETHROTTLE: off\nCOMPRESSION: off\nBABLOW: 10\nBABHIGH: 20\n\
_MODE: ACCUMULATE\n_AUTOSTART: no"


#if defined(HPUX)
#define FTD_SYSTEM_CONFIG_FILE PATH_CONFIG "/" QNM ".conf"
#elif defined(SOLARIS)
#define FTD_SYSTEM_CONFIG_FILE "/usr/kernel/drv/" QNM ".conf"
#elif defined(_AIX)
#define FTD_SYSTEM_CONFIG_FILE "/usr/lib/drivers/" QNM ".conf"
#endif

/*
 *
 */
#define FTD_MAX_DEVICES         1024
#define FTD_MAX_GROUPS          512

/*
 * Default sizes for persistent store and logical groups
 */
#define FTD_PS_LRDB_SIZE        8*1024
#define FTD_PS_HRDB_SIZE        128*1024
#define FTD_PS_GROUP_ATTR_SIZE  4*1024
#define FTD_PS_DEVICE_ATTR_SIZE 4*1024

#define FTD_DRIVER_CHUNKSIZE 1024*1024*1
#endif /* _FTD_CMD_H_ */
