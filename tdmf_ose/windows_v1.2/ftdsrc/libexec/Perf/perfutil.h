/*++

Module Name:

    perfutil.h  

Abstract:

    This file supports routines used to parse and
    create Performance Monitor Data Structures.
    It actually supports Performance Object types with
    multiple instances

Author:


Revision History:


--*/
#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//
#define DWORD_MULTIPLE(x)    (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
#define QUADWORD_MULTIPLE(x) (((x+2*sizeof(DWORD)-1)/(2*sizeof(DWORD)))*(2*sizeof(DWORD)))

//    (assumes dword is 4 bytes long and pointer is a dword in size)
#define ALIGN_ON_DWORD(x)    ((VOID *)( ((DWORD) x & 0x00000003) ? ( ((DWORD) x & 0xFFFFFFFC) + 4 ) : ( (DWORD) x ) ))
#define ALIGN_ON_QUADWORD(x) ((VOID *)( ((DWORD) x & 0x00000007) ? ( ((DWORD) x & 0xFFFFFFF0) + 8 ) : ( (DWORD) x ) ))

//
// The definition of the only routine of perfutil.c, It builds part of a 
// performance data instance (PERF_INSTANCE_DEFINITION) as described in 
// winperf.h
//

// from PerfUtil.C

BOOL MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    LPWSTR  Name
);

#endif  //_PERFUTIL_H_
