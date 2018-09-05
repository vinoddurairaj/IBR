/*
 * ftd_pathnames.h - FTD path names
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

#ifndef _FTD_PATHNAMES_H_
#define _FTD_PATHNAMES_H_

#if defined(_WINDOWS)
#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\" PRODUCTNAME "\\CurrentVersion"

extern char* ftd_nt_path(char*);
#endif

/*
 * /var/opt or other place where we make our files parent directory
 */
#if defined(_WINDOWS)
#define PATH_VAR_OPT	"\\etc\\opt"
#else
#define PATH_VAR_OPT	"/var/opt"
#endif

/*
 * Where we put the runtime files (perf info, etc)
 */
#if !defined(_WINDOWS)
#define PATH_RUN_FILES	PATH_VAR_OPT "/" OEM QNM 
#define PATH_BASE			"/opt/" OEM QNM	
#define PATH_BIN_FILES		PATH_BASE "/bin"
#define PATH_LIBEXEC_FILES	PATH_BASE "/libexec"
#else
#define PATH_RUN_FILES	ftd_nt_path("InstallPath")
#define PATH_BASE	ftd_nt_path("Installed")
#define PATH_BIN_FILES		PATH_RUN_FILES
#define PATH_LIBEXEC_FILES	PATH_RUN_FILES
#endif

/*
 * Where we put the config files
 */
#if defined(_AIX)
#define PATH_CONFIG			
#elif !defined(_WINDOWS)  /* defined(_AIX) */
#define PATH_CONFIG			"/etc/opt/" OEM QNM	
#else
#define PATH_CONFIG			ftd_nt_path("InstallPath")
#endif /* defined(_AIX) */

/*
 * Suffix on our config files
 */
#define PATH_CFG_SUFFIX	"cfg"
#define PATH_STARTED_CFG_SUFFIX	"cur"

/*
 * master daemon 
 */
#if !defined(_WINDOWS)
#define FTD_MASTER		"in." QNM
#define FTD_MASTER_PATH	PATH_BIN_FILES "/" FTD_MASTER 
#else
#define FTD_MASTER		MASTERNAME
extern DWORD MasterThread(LPDWORD param);
#endif

/*
 * primary mirror daemon 
 */
#if !defined(_WINDOWS)
#define FTD_PMD			"in.pmd"
#define FTD_PMD_PATH	PATH_BIN_FILES "/" FTD_PMD 
#else
#define FTD_PMD			"Primary"
#define FTD_PMD_PATH	FTD_PMD
extern DWORD PrimaryThread(LPDWORD param);
#endif

/*
 * secondary mirror daemon 
 */
#if !defined(_WINDOWS)
#define FTD_RMD			"in.rmd"
#define FTD_RMD_PATH	PATH_BIN_FILES "/" FTD_RMD 
#else
#define FTD_RMD			"Remote"
#define FTD_RMD_PATH	FTD_RMD
extern DWORD RemoteThread(LPDWORD param);
#endif


/*
 * throttle evaluator daemon 
 */
#if !defined(_WINDOWS)
#define FTD_THROT		"throtd"
#define FTD_THROT_PATH	PATH_BIN_FILES "/" FTD_THROT 
#else
#define FTD_THROT		"statd"
#define FTD_THROT_PATH	FTD_THROT
extern DWORD StatThread(LPDWORD param);
#endif


#endif
