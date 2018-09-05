#include <windows.h>
#include <stdio.h>

#include <winsock.h>
#include <crtdbg.h>

#include <winioctl.h>   // PCAUSA

#include "tdiinfo.h" // from recent NT4DDK "\ddk\src\network\inc\tdiinfo.h"
#include "smpletcp.h" // from recent NT4DDK "\ddk\src\network\wshsmple\smpletcp.h"


int
TDIQ_GetIPInterfaceInfoForAddr(
   IPAddr theIPAddr,
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize // IPInterfaceInfo size + MAX_PHYSADDR_SIZE for address bytes
   );

int
TDIQ_GetIFEntryForInstance(
   ULONG nInstance,
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize // IFEntry size byte plus MAX_IFDESCR_LEN for description
   );

int
TDIQ_GetIPSNMPInfo(
   IPSNMPInfo  *pIPSnmpInfo,
   PULONG      pInfoSize
   );

int
TDIQ_GetIFEntryForIFIndex(
   ULONG nIFIndex,
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize // IFEntry size byte plus MAX_IFDESCR_LEN for description
   );

int
TDIQ_GetIPRouteTable(
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize
   );

int
TDIQ_GetIPAddrTable(
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize
   );

BOOLEAN
TDIQ_IsIPInstalled( VOID );

IPRouteEntry *
TDIQ_GetNextIPRouteTableEntry(
   IPRouteEntry *pIPRouteEntry
   );

BOOLEAN
TDIQ_IsWindows95( VOID );

int
TDIQ_Startup( VOID );

VOID
TDIQ_Cleanup( VOID );

