/*++ 

Module Name:

    perfutil.c

Abstract:

    This file implements the utility routines used to construct the
	common parts of a PERF_INSTANCE_DEFINITION (see winperf.h) and
    perform event logging functions.

Created:

Revision History:

--*/
//
//  include files
//
#include <windows.h>
#include <string.h>
#include <winperf.h>

#include "perfutil.h"

BOOL
MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    LPWSTR  Name
    )
/*++

    MonBuildInstanceDefinition  -   Build an instance of an object

        Inputs:

            pBuffer         -   pointer to buffer where instance is to
                                be constructed

            pBufferNext     -   pointer to a pointer which will contain
                                next available location, DWORD aligned

            ParentObjectTitleIndex
                            -   Title Index of parent object type; 0 if
                                no parent object

            ParentObjectInstance
                            -   Index into instances of parent object
                                type, starting at 0, for this instances
                                parent object instance

            UniqueID        -   a unique identifier which should be used
                                instead of the Name for identifying
                                this instance

            Name            -   Name of this instance
--*/
{
    DWORD NameLength = 0;
    WCHAR *pName;

    //
    //  Include trailing null in name size
    //

    NameLength = lstrlenW(Name) * sizeof(WCHAR);
    if (NameLength > 0) {
        NameLength += sizeof(WCHAR); // to include the terminating NULL
    }

    pBuffer->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                          QUADWORD_MULTIPLE(NameLength);

    pBuffer->ParentObjectTitleIndex = ParentObjectTitleIndex;
    pBuffer->ParentObjectInstance = ParentObjectInstance;
    pBuffer->UniqueID = UniqueID;
    pBuffer->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pBuffer->NameLength = NameLength;

    pName = (PWCHAR)&pBuffer[1];
    lstrcpyW (pName, Name);

    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);
    return TRUE;
}

