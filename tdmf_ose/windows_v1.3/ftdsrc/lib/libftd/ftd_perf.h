/*
 * ftd_perf.h - FTD performance monitor interface
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

#ifndef _FTD_PERF_H_
#define _FTD_PERF_H_

#ifdef __cplusplus
extern "C"{ 
#endif

//#include "llist.h"    AC,2002-05-02 : no linked list used here

// DTurrin - Nov 7th, 2001
// If you are using Windows 2000 or later,
// you need to add the Global\ prefix so that
// client session will use kernel objects in
// the server's global name space.
// #define NTFOUR
#ifdef NTFOUR
#pragma message(">>>> ftd_perf.h : Building for NT 4 <<<<<")
#define FTD_PERF_GROUP_GET_EVENT_NAME(group_name, group_num) \
	sprintf(group_name, "DTClg%dperfget", group_num);
#define FTD_PERF_GET_EVENT_NAME ("DTCperfget")
#define FTD_PERF_SET_EVENT_NAME ("DTCperfset")
#else // #ifdef NTFOUR
#define FTD_PERF_GROUP_GET_EVENT_NAME(group_name, group_num) \
	sprintf(group_name, "Global\\DTClg%dperfget", group_num);
#define FTD_PERF_GET_EVENT_NAME ("Global\\DTCperfget")
#define FTD_PERF_SET_EVENT_NAME ("Global\\DTCperfset")
#endif // #ifdef NTFOUR

#define FTDPERFMAGIC				0xBADF00D6	
#define MAX_SIZEOF_INSTANCE_NAME    48  //13	// PXXX ==> G:
#define MAX_SIZEOF_PERF_EVENT_NAME	13  // lg000perfset

#define SHARED_MEMORY_ITEM_COUNT    (FTD_MAX_GROUPS * FTD_MAX_DEVICES + 1) 
#define SHARED_MEMORY_OBJECT_SIZE   (SHARED_MEMORY_ITEM_COUNT * sizeof(ftd_perf_instance_t))
#ifdef NTFOUR
#define SHARED_MEMORY_OBJECT_NAME   (TEXT("DTC_PERF_DATA"))
#else // #ifdef NTFOUR
#define SHARED_MEMORY_OBJECT_NAME   (TEXT("Global\\DTC_PERF_DATA"))
#endif // #ifdef NTFOUR

#ifdef NTFOUR
#define SHARED_MEMORY_MUTEX_NAME    (TEXT("DTC_PERF_DATA_MUTEX"))
#else // #ifdef NTFOUR
#define SHARED_MEMORY_MUTEX_NAME    (TEXT("Global\\DTC_PERF_DATA_MUTEX"))
#endif // #ifdef NTFOUR
#define SHARED_MEMORY_MUTEX_TIMEOUT ((DWORD)1000L)
// #undef NTFOUR

//MMP : pack ftd_perf_instance_t structure to be sent over socket 
#pragma pack(push, 1)   
typedef struct ftd_perf_instance_s {
	__int64	actual;
	__int64	effective;
	__int64	bytesread;
	__int64	byteswritten;
	int		connection;	/* 0 - pmd only, 1 - connected, -1 - accumulate */
	int		drvmode;	/* driver mode                          */
    int		lgnum;		// group number
    int		insert;		// gui list insert
	int		devid;		/* device id */
	int		rsyncoff;	/* rsync sectors done */
	int		rsyncdelta;	/* rsync sectors changed */
	int		entries;	/* # of entries in bab */
	int		sectors;	/* # of sectors in bab */
	int		pctdone;
	int		pctbab;
    WCHAR   wcszInstanceName[MAX_SIZEOF_INSTANCE_NAME]; // SZ instance name
    DWORD	Reserved1;		// unused
    DWORD	Reserved2;		// unused
	char	role;
    char    padding[3 + 32];// to align structure on a 8 byte boundary.
} ftd_perf_instance_t;
#pragma pack(pop)   


typedef struct ftd_perf_s {
	int						magicvalue;		/* so we know it's been initialized	*/
	HANDLE					hSharedMemory;
	HANDLE					hMutex;
	HANDLE					hGetEvent;
	HANDLE					hSetEvent;
	ftd_perf_instance_t		*pData;
} ftd_perf_t;

extern BOOL ftd_perf_init(ftd_perf_t *perfp, BOOL bReadOnlyAccess);
extern ftd_perf_t *ftd_perf_create(void);
extern BOOL ftd_perf_delete(ftd_perf_t *perfp);

#ifdef __cplusplus 
}
#endif

#endif //_FTD_PERF_H_
