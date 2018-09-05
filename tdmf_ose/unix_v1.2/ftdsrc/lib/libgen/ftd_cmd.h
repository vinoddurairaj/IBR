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

#define FTD_CREATE_GROUP_DIR(group_dir, group_num) \
                      sprintf(group_dir, "/dev/" QNM "/lg%d", group_num);

/* the maximum number of key/value pairs of tunables */
#define FTD_MAX_KEY_VALUE_PAIRS 1024

/* this string is used to set the default tunables for a new logical group */
/* Modified by Watanabe
 *   Problem: In use dtcset, ON/OFF changing of TRACETHROTTLE cannot be done.
 *   Reason : The character comparison is failed, because the define of
 *            TRACETHROTTLE is wrong.
 *//*set default value of maxstatfilesize to 1024 Kb*/
#if BRANDTOKEN == 'M'
#define FTD_DEFAULT_TUNABLES "CHUNKSIZE: 2048\nCHUNKDELAY: 0\nSYNCMODE: off\n\
SYNCMODEDEPTH: 1\nSYNCMODETIMEOUT: 30\nNETMAXKBPS: -1\nSTATINTERVAL: 10\n\
MAXSTATFILESIZE: 1024\nTRACETHROTTLE: off\nCOMPRESSION: off\nJOURNAL: off\n\
_MODE: ACCUMULATE\n_AUTOSTART: no\nLRT: on"
#elif BRANDTOKEN == 'R'
#define FTD_DEFAULT_TUNABLES "CHUNKSIZE: 2048\nCHUNKDELAY: 0\nSYNCMODE: off\n\
SYNCMODEDEPTH: 1\nSYNCMODETIMEOUT: 30\nNETMAXKBPS: -1\nSTATINTERVAL: 10\n\
MAXSTATFILESIZE: 1024\nTRACETHROTTLE: off\nCOMPRESSION: off\nJOURNAL: on\n\
_MODE: ACCUMULATE\n_AUTOSTART: no\nLRT: on"
#else
You need to define a new string for your new brand in ftd_cmd.h
#endif

#if defined(linux)
#define FTD_SYSTEM_CONFIG_FILE PATH_DRIVER_FILES "/" QNM ".conf"
#endif /* defined(linux) */

#if defined(HPUX)
#define FTD_SYSTEM_CONFIG_FILE PATH_CONFIG "/" QNM ".conf"
#elif defined(SOLARIS)
#define FTD_SYSTEM_CONFIG_FILE "/usr/kernel/drv/" QNM ".conf"
#elif defined(_AIX)
#define FTD_SYSTEM_CONFIG_FILE "/usr/lib/drivers/" QNM ".conf"
#endif

#ifndef ROUND_UP
#define ROUND_UP(v, inc) ( ( (int) (((v) + (inc - 1)) / (inc)) ) * (inc) )
#endif
/*
 *
 */
#define FTD_MAX_DEVICES         1024
#define FTD_MAX_GROUPS          512

/*
 * Default sizes for persistent store and logical groups
 */
#define FTD_PS_LRDB_SIZE        (8*1024)
#define FTD_PS_HRDB_SIZE_SMALL  FTD_MAXIMUM_HRDB_SIZE_SMALL
#define FTD_PS_HRDB_SIZE_LARGE  FTD_MAXIMUM_HRDB_SIZE_LARGE
#define FTD_PS_GROUP_ATTR_SIZE  (4*1024)
#define FTD_PS_DEVICE_ATTR_SIZE (4*1024)

#if defined(linux)
/*
 * on linux, the kernel virtual space may get fragmented. allcating 1MB
 * as a unit is likely to fail. A workaround is to reduce the allocating
 * unit to a smaller size.
 */
#define FTD_DRIVER_CHUNKSIZE (1024*256)
#else
#define FTD_DRIVER_CHUNKSIZE (1024*1024)
#endif
#endif /* _FTD_CMD_H_ */
