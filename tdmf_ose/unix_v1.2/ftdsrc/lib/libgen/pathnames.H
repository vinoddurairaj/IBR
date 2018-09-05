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
/***************************************************************************
 * pathnames.h - DataStar pathname definitions
 *
 * (c) Copyright 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * Contains definitions for the paths that we use in this program.
 *
 * History:
 *   10/14/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */
#if defined(linux)
// We used to explicitely include <linux/version.h> here.
// Look for LINUX_VERSION_CODE comments within Makefile.inc.Linux for other details.
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
/*
 * /var/opt or other place where we make our files parent directory
 */
#define PATH_VAR_OPT	"/%VAROPTDIR%"

/*
 * Where we put the runtime files (perf info, etc)
 */
#define PATH_RUN_FILES	"/%VAROPTDIR%/%PKGNM%"

/*
 * Where we put the runtime files (perf info, etc)
 */
#define PATH_BASE	"/%OPTDIR%/%PKGNM%"
#define PATH_BIN_FILES	"/%OPTDIR%/%PKGNM%/bin"
#define PATH_LIBEXEC_FILES	"/%OPTDIR%/%PKGNM%/libexec"
#if defined(linux)
#define PATH_DRIVER_FILES	"/%ETCOPTDIR%/%PKGNM%/driver"
#define MODULES_CONFIG_PATH	"/etc/modprobe.d/sftkdtc.conf"

#define MODULES_NAME		"%MODULENAME%"
#endif /* defined(linux) */

/*
 * Where we put the config files
 */
#define PATH_CONFIG	"/%OURCFGDIR%"
#define UNIQUE_LPARID_FILE      PATH_CONFIG "/" QNM "lparid.cfg"

/*
 * Prefix on our primary-system config files
 */
#define PMD_CFG_PREFIX	"p"

/*
 * Prefix on our secondary-system config files
 */
#define RMD_CFG_PREFIX	"s"

/*
 * Suffix on our config files
 */
#define PATH_CFG_SUFFIX	"cfg"
#define PATH_STARTED_CFG_SUFFIX	"cur"

/*
 * FTDD path 
 */
#define FTDD_PATH PATH_BIN_FILES "/in." QNM
#define FTDD "in." QNM

/*
 * PMD path 
 */
#define PMD_PATH PATH_BIN_FILES "/in.pmd" 

/*
 * RMD path 
 */
#define RMD_PATH PATH_BIN_FILES "/in.rmd"


/*
 * RFD path 
 */
#define RFD_PATH PATH_BIN_FILES "/" QNM "refresh"

/*
 * FTD domain socket used for IPC 
 */
#define FTD_SOCKPATH PATH_RUN_FILES "/" QNM "sock.msg"

/*
 * RMD domain socket used for IPC 
 */
#define RMD_SOCKPATH PATH_RUN_FILES "/" QNM "sock.msg1"

/*
 * Replication Profile configuration file, giving HRT resolutions for Proportional HRDB
 */
#define TRACKING_RESOLUTIONS_FILE PATH_CONFIG "/" QNM "Tracking_Resolutions.txt"

/*
 * SFTKdtc marker file to disable Replicator services and driver loading at boot time
 */
#define SFTKDTC_SERVICES_DISABLE_FILE PATH_CONFIG "/SFTKdtc_services_disabled"
