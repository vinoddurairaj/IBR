/****************************************************************
 * Name:  perf.h                                            *
 *                                                              *
 * Function:  Constants, structures, and function               *
 *            declarations specific to this DLL.                *
 *                                                              *
 ****************************************************************/
#ifndef __PERF__
#define __PERF__

//
// Header files
//
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <winioctl.h>
#include <winperf.h>

#include "counters.h"
#include "..\..\LogMsg.h"
#include "data.h"

//
// Various constants
//
#define FTD_NUM_OBJECT_TYPES         1

//
// Error-logging verbosity levels
//
#define LOG_LEVEL_NONE                  0
#define LOG_LEVEL_NORMAL                1
#define LOG_LEVEL_DEBUG_MIN             2
#define LOG_LEVEL_DEBUG_MAX             3

#define  DEFAULT_LOG_LEVEL      LOG_LEVEL_DEBUG_MAX

//
// Collection query-types
//
#define PERF_QUERY_TYPE_COSTLY          1
#define PERF_QUERY_TYPE_FOREIGN         2
#define PERF_QUERY_TYPE_GLOBAL          3
#define PERF_QUERY_TYPE_ITEMS           4

//
// Prototypes for globally defined functions...
//
//
// eventlog.c
//
VOID
PerfOpenEventLog(
	VOID
	);

VOID
PerfCloseEventLog(
	VOID
	);

VOID
PerfLogInformation(
	IN DWORD dwMessageLevel,
	IN DWORD dwMessage
	);

VOID
PerfLogError(
	IN DWORD dwMessageLevel,
	IN DWORD dwMessage
	);

VOID
PerfLogErrorWithData(
	IN DWORD        dwMessageLevel,
	IN DWORD        dwMessage,
	IN LPDWORD      lpdwDumpData,
	IN DWORD        cbDumpDataSize
	);

//
// parse.c
//
DWORD
PerfGetQueryType(
    IN LPWSTR lpValue
	);

BOOL
PerfIsNumberInList(
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
	);

#endif // __PERF__



