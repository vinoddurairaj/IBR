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

#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/ioctl.h>

#include "platform.h"
#include "config.h"
#include "ipc.h"
#include "ftd_cmd.h"
#include "ps_intr.h"

#include <syslog.h>
#include "ftd_trace.h" 	/* PBouchard ++ */
#if defined (SOLARIS) && (SYSVERS >= 590)
#define EFI_SUPPORT
#endif

#if defined (SOLARIS)
#define HAVE_LIBZFS
#endif

#define TDMFIP_280_FREEFORALL_LICKEY "0HFW8G8HCW3HCWAGFO7F8@7F" // Real: G8A9G8PGHBXDI4XDI9H9XGI1 (rev -1)
#define LENGTH_TDMFIP_280_FREEFORALL_LICKEY 24

/*
#if (defined(SOLARIS) && (SYSVERS < 580)) || \
    (defined(HPUX) && (SYSVERS < 1123)) || \
    (defined(_AIX) && (SYSVERS < 520))
*/
//#define FTD_IPV4
//#endif 

//#if defined(linux)
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
//#define FTD_IPV4
//#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0) */
//#endif /* linux*/

//#if defined FTD_IPV4
//#ifndef INET_ADDRSTRLEN
//#define INET_ADDRSTRLEN 16
//#endif
//#define INET6_ADDRSTRLEN 46
//#define NI_MAXHOST      1025
//#endif /* defined FTD_IPV4 */ 

#if defined(linux)
#define DEV_BSHIFT      9
#endif /* defined(linux) */

#ifdef TDMF_TRACE
extern int getconfigs (char* ,char *,char paths[][32], int , int ) ;
#else
extern int getconfigs (char paths[][32], int, int);
#endif
extern void ftd_time_stamp (char *file, char *func, char *fmt, ...);

#define FTD_DBG_ALL     0xffff
#define FTD_DBG_NONE    0x0000
#define FTD_DBG_FLOW1   0x0001
#define FTD_DBG_FLOW2   0x0002
#define FTD_DBG_FLOW3   0x0004
#define FTD_DBG_FLOW4   0x0008
#define FTD_DBG_FLOW5   0x0010
#define FTD_DBG_FLOW6   0x0020
#define FTD_DBG_FLOW7   0x0040
#define FTD_DBG_FLOW8   0x0080
#define FTD_DBG_FLOW9   0x0100
#define FTD_DBG_FLOW10  0x0200
#define FTD_DBG_FLOW11  0x0400
#define FTD_DBG_FLOW12  0x0800
#define FTD_DBG_FLOW13  0x1000
#define FTD_DBG_FLOW14  0x2000
#define FTD_DBG_FLOW15  0x4000
#define FTD_DBG_FLOW16  0x4001
#define FTD_DBG_FLOW17  0x4002

#define FTD_DBG_ERROR   0x8000

#define FTD_DBG_DEFAULT FTD_DBG_ERROR
#define FTD_DBG_REFRESH FTD_DBG_FLOW5
#define FTD_DBG_THROTD  FTD_DBG_FLOW6
#define FTD_DBG_PMD_RMD FTD_DBG_FLOW7
#define FTD_DBG_SOCK    FTD_DBG_FLOW8
#define FTD_DBG_SYNC    FTD_DBG_FLOW9
#define FTD_DBG_IOCTL   FTD_DBG_FLOW10
#define FTD_DBG_IO      FTD_DBG_FLOW11
#define FTD_DBG_SEQCER	FTD_DBG_FLOW13
    
extern int ftd_debugFlag;
int ftd_debugFlagSaved;

#ifdef TDMF_TRACE

	#ifdef TDMF_TRACE_MAKER
		int ftd_ioctl_trace(char *, char *, int );
	#else
		extern int ftd_ioctl_trace(char *, char *, int );
	#endif

	#define DPRINTF(...) 			    {fprintf(stderr,__VA_ARGS__);syslog(LOG_NOTICE,__VA_ARGS__);}	
	#define FTD_IOCTL_CALL(a,b,...)	    (ftd_ioctl_trace((char *)__FILE__,(char *)__func__,(int)b) || ioctl(a,b,__VA_ARGS__))
	#define GETPSNAME(a,b) 				getpsname((char*)__FILE__,(char*)__func__,a,b)
	#define GETCONFIGS(a,b,c) 			getconfigs((char*)__FILE__,(char*)__func__,a,b,c)
        #define GETNUMDTCDEVICES(a,b,c)                 getnumdtcdevices((char*)__FILE__,(char*)__func__,a,b,c)
    #define FTD_TIME_STAMP(traceFlow, ...) \
            {if (traceFlow & ftd_debugFlag) {ftd_time_stamp((char *)__FILE__, (char *)__func__, __VA_ARGS__);}}

#else
	#if defined(linux)
		#define DPRINTF(...)
	#else
		#define DPRINTF
	#endif /* defined(linux) */
	#define FTD_IOCTL_CALL(a,b,c)		ioctl(a,b,c)
	#define GETPSNAME(a,b) 				getpsname(a,b)
	#define GETCONFIGS(a,b,c) 			getconfigs(a,b,c)
        #define GETNUMDTCDEVICES(a,b,c)                 getnumdtcdevices(a,b,c)
    #define FTD_TIME_STAMP(traceFlow, ...)
#endif

#ifdef TDMF_TRACE
    #define ftd_trace_flow(flow, ...)   \
    {                                   \
        if (((flow) & ftd_debugFlag))       \
        {                               \
            DPRINTF("%d<-%d %s/%s: ", getpid(), getppid(), __FILE__, __func__);\
            DPRINTF(__VA_ARGS__);       \
        }                               \
    }
#else
    #define ftd_trace_flow(flow, ...)
#endif

					 /* PBouchard -- */

#if defined(DEBUG)
    #if defined(linux)
    #define ftd_debugf(parms, fmt, args...) (printf("%s:%s.%d[%d]: %s(" parms "): " fmt "\n", argv0, __FILE__, __LINE__, getpid(), __func__, ## args), fflush(stdout))
    #else
    #define ftd_debugf(...)
    #endif

    #define UNLINK(path) (printf("%s:%s.%d[%d]: %s -> unlink(%s)\n", argv0, __FILE__, __LINE__, getpid(), __func__, path), fflush(stdout), unlink(path))
#else
    #if defined(linux)
    #define ftd_debugf(parms, fmt, args...) (0 && (printf("%s:%s.%d[%d]: %s(" parms "): " fmt "\n", argv0, __FILE__, __LINE__, getpid(), __func__, ## args), fflush(stdout)))
    #else
    #define ftd_debugf(...)
    #endif

    #define UNLINK(path) unlink(path)
#endif

extern char  debug_msg[128];   /* For general tracing with reporterr, using M_GENMSG message code */

typedef union {
	u_char uc[4];
	u_long ul;
} encodeunion;


typedef struct _statestr {
    int stval;
    const char *str;
    const char *desc;
} statestr_ts;

typedef int EntryFunc(wlheader_t*, char*);

/*
 * FTD error codes
 */
#define FTD_ERR         -1
#define FTD_OK          0
#define FTD_NO_OP       1

/*
 * flag for nuke_journals()
 */
#define DELETE_ALL              0
#define DELETE_JOURNAL_ALL      1
#define DELETE_JOURNAL_CO       2
#define DELETE_JOURNAL_INCO     3

/* function prototypes */
extern char *ftdmalloc(int);
extern void *ftdvalloc(size_t);
extern void *ftdmemalign(size_t, size_t);
extern char *ftdcalloc(int, int);
extern char *ftdrealloc(char*, int);
extern void write_signal(int, int);
extern int opendevs(int);
extern void closedevs(int);
extern char *cfgpathtoname(char*);
extern int cfgpathtonum (char*);
extern int cfgtonum(int);
extern int numtocfg(int);
extern pid_t getprocessid(char*, int, int*);
extern int compress(u_char *source, u_char *dest, int len);
extern int decompress(u_char *source, u_char *dest, int len);
extern int is_parent_master(void);
extern sddisk_t *get_lg_dev(group_t *group, int devid);
extern sddisk_t *get_lg_rdev(group_t *group, u_longlong_t rdev);
extern sddisk_t *getdevice(rsync_t *rsync);
extern int get_started_groups_list (int* list, int* size);
extern int clearmode(int lgnum, int reset_RPO_stats);
extern int setmode(int lgnum, int mode, int doing_reboot_autostart);
extern void encodeauth (time_t ts, char* hostname, u_long hostid, u_long ,
    int* encodelen, char* encode);
extern void decodeauth (int encodelen, char* encodestr, time_t ts, 
    u_long hostid, char* hostname, size_t hostname_len, u_long );
extern void setmaxfiles(void);
extern int ftd_strtol(char *);
extern void syncgroup(int force);
extern int dev_mounted(char *devname, char *mountp);
extern void ftdfree(char *ptr);
extern int get_driver_mode(int lgnum);
extern int ftd_get_lg_group_stat(int lgnum, stat_buffer_t *statBuf, int display);
extern int update_lrdb(int lgnum);
extern int showtunables (tunable_t *tunables);
extern int ftd_getidlist(pid_t *pidlist);
extern int get_pstore_mode(int lgnum);

#if defined(SOLARIS)
extern int ftd_feof(int fd);
extern char * ftd_fgets(char *line, int size, int fd);
extern int get_license_manydev (int pmdflag, char ***lickeyaddr);
#endif

/*
 * ftdio.c (here because of file dependencies)
 */
extern int traverse_chunk(char *chunk, int *length, EntryFunc *func);
extern int apply_journal(group_t *group);

#if !defined(UNSIGNED_LONG_LONG_MAX)
#if defined (HPUX)
#define UNSIGNED_LONG_LONG_MAX   ULONG_LONG_MAX
#elif defined (_AIX) && (SYSVERS < 520)
#define UNSIGNED_LONG_LONG_MAX   ULONGLONG_MAX
#else
#define UNSIGNED_LONG_LONG_MAX   ULLONG_MAX
#endif
#endif

#endif

// WI_338550 December 2017, implementing RPO / RTT
extern ftd_uint64_t     ftd_lg_get_oldest_inconsistent_timestamp(group_t *lgp);
extern ftd_uint64_t     ftd_lg_get_last_consistency_point_timestamp(group_t *lgp);
extern void             ftd_lg_set_consistency_point_reached(group_t *lgp);
extern void             ftd_lg_set_consistency_point_lost(group_t *lgp);
extern void             ftd_lg_set_consistency_point_invalid(group_t *lgp);
extern void             ftd_lg_update_consistency_timestamps(group_t *lgp);
extern int              give_RPO_stats_to_driver( group_t *group );

// WI_338550 December 2017, implementing RPO / RTT
extern queue_t* Chunk_Timestamp_Queue_Create();
extern void Chunk_Timestamp_Queue_Destroy(queue_t* pQueue);
extern void Chunk_Timestamp_Queue_Init(queue_t* pQueue, u_long Size);
extern void Chunk_Timestamp_Queue_Push(queue_t* pQueue, chunk_timestamp_queue_entry_t* pData);
extern void Chunk_Timestamp_Queue_Pop(queue_t* pQueue);
extern chunk_timestamp_queue_entry_t* Chunk_Timestamp_Queue_Head(queue_t* pQueue);
extern chunk_timestamp_queue_entry_t* Chunk_Timestamp_Queue_Tail(queue_t* pQueue);
extern void Chunk_Timestamp_Queue_Clear(queue_t* pQueue);
extern int Chunk_Timestamp_Queue_IsEmpty(queue_t* pQueue);

