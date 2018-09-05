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

/*
 * Where we put the config files
 */
#define PATH_CONFIG	"/%OURCFGDIR%"

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
