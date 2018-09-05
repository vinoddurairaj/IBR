/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include <windows.h>
#include <stdio.h>

#include <winsock.h>
#include <crtdbg.h>

#include <winioctl.h>

#include "tdiinfo.h" // from recent NT4DDK "\ddk\src\network\inc\tdiinfo.h"
#include "smpletcp.h" // from recent NT4DDK "\ddk\src\network\wshsmple\smpletcp.h"

// Copyright And Configuration Management ----------------------------------
//
//                         TDI Query (TDIQ) Test Tool
//                                    For
//                          Windows 95 And Windows NT
//
//                     TDI Query Help Library - TDIQHelp.c
//
//    Copyright (c) 2000-2001, Printing Communications Associates, Inc.
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

static BOOLEAN     g_bIsWindows9X = FALSE;
static ULONG       g_nIPRouteEntrySize = sizeof( IPRouteEntry );
static HMODULE     g_hModule = NULL;

//
// TCP/IP Action Codes (Windows 9X)
//
#define WSCNTL_TCPIP_QUERY_INFO     0x00000000
#define WSCNTL_TCPIP_SET_INFO       0x00000001
#define WSCNTL_TCPIP_ICMP_ECHO      0x00000002
#define WSCNTL_TCPIP_TEST           0x00000003

typedef int (__stdcall *WsControlProc)(DWORD, DWORD, LPVOID, LPDWORD, LPVOID, LPDWORD);

int __stdcall WsControlNT(
   DWORD protocol,
   DWORD action,
   LPVOID pRequestInfo,
   LPDWORD pcbRequestInfoLen,
   LPVOID pResponseInfo,
   LPDWORD pcbResponseInfoLen
   )
{
   HANDLE   hDevice = INVALID_HANDLE_VALUE;
   BOOL     fResult;
   DWORD    nLastError;
   int      nResult = ERROR_SUCCESS;

   if( protocol == IPPROTO_TCP )
   {
      // 
      // Create DOS Device Symbolic Link
      // 
      fResult = DefineDosDevice(
                  DDD_RAW_TARGET_PATH,
                  "TCP",
                  "\\Device\\TCP"
                  );

      if( !fResult )
      {
         printf( "DefineDosDevice failed\n" );

         nResult = ERROR_BAD_DEVICE;
         WSASetLastError( nResult );

         goto RemoveDosDevice;
      }

      // 
      // Open TCP Device Handle
      // 
      hDevice = CreateFile(
                     "\\\\.\\TCP",
                     0, // Device Query Access
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     );

      if( hDevice == INVALID_HANDLE_VALUE )
      {
         printf( "CreateFile failed\n" );

         nResult = WSAGetLastError();
         WSASetLastError( nResult );

         goto RemoveDosDevice;
      }
   }
   else if( protocol == IPPROTO_UDP )
   {
      // 
      // Create DOS Device Symbolic Link
      // 
      fResult = DefineDosDevice(
                  DDD_RAW_TARGET_PATH,
                  "UDP",
                  "\\Device\\UDP"
                  );

      if( !fResult )
      {
         printf( "DefineDosDevice failed\n" );

         nResult = ERROR_BAD_DEVICE;
         WSASetLastError( nResult );

         goto RemoveDosDevice;
      }

      // 
      // Open UDP Device Handle
      // 
      hDevice = CreateFile(
                     "\\\\.\\UDP",
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     );

      if( hDevice == INVALID_HANDLE_VALUE )
      {
         printf( "CreateFile failed\n" );

         nResult = WSAGetLastError();
         WSASetLastError( nResult );

         goto RemoveDosDevice;
      }
   }
   else
   {
      printf( "Invalid Protocol\n" );

      nResult = WSAEPROTONOSUPPORT;
      WSASetLastError( nResult );

      goto RemoveDosDevice;
   }

   fResult = DeviceIoControl(
               hDevice,
               IOCTL_TCP_QUERY_INFORMATION_EX,
               pRequestInfo,
               *pcbRequestInfoLen,
               pResponseInfo,
               *pcbResponseInfoLen,
               pcbResponseInfoLen,	
               NULL        // Overlapped
               );

   if( !fResult )
   {
      nLastError = WSAGetLastError();

      return( nLastError );
   }

   CloseHandle( hDevice );

RemoveDosDevice:
   if( protocol == IPPROTO_TCP )
   {
      DefineDosDevice(
         DDD_RAW_TARGET_PATH|DDD_REMOVE_DEFINITION|DDD_EXACT_MATCH_ON_REMOVE,
         "TCP",
         "\\Device\\TCP"
         );
   }
   else if( protocol == IPPROTO_UDP )
   {
      DefineDosDevice(
         DDD_RAW_TARGET_PATH|DDD_REMOVE_DEFINITION|DDD_EXACT_MATCH_ON_REMOVE,
         "UDP",
         "\\Device\\UDP"
         );
   }

   return( nResult );
}

WsControlProc WsControl = NULL;

int
TDIQ_GetAtTable(
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize
   )
{
   int   result = ERROR_SUCCESS;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   ULONG tcpRequestBufSize = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = AT_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = AT_MIB_ADDRXLAT_ENTRY_ID;

   result = WsControl(
               IPPROTO_TCP,
               WSCNTL_TCPIP_QUERY_INFO,
               &tcpRequestQueryInfoEx,
               &tcpRequestBufSize,
               pQueryBuffer,
               pQueryBufferSize
               );

   if( result )
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n",
         __FILE__,
         __LINE__,
         result
         );

      return( result );
   }

   return( result );
}

/////////////////////////////////////////////////////////////////////////////
//// TDIQ_GetIPInterfaceInfoForAddr
//
// Purpose
// Fetch the IPInterface information corresponding to the specified IP
// address.
//
// Parameters
//
// Return Value
// 
// Remarks
// You should expect this function to fail in certain situations. For example,
// a PPP adapter that is not connected will not have a IPInterface entry.
//

int
TDIQ_GetIPInterfaceInfoForAddr(
   IPAddr theIPAddr,
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize // IPInterfaceInfo size + MAX_PHYSADDR_SIZE for address bytes
   )
{
   int   result = ERROR_SUCCESS;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   ULONG tcpRequestBufSize = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = CL_NL_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = IP_INTFC_INFO_ID;

   memcpy(
      &tcpRequestQueryInfoEx.Context,
      &theIPAddr,
      sizeof( ULONG )
      );

   result = WsControl(
               IPPROTO_TCP,
               WSCNTL_TCPIP_QUERY_INFO,
               &tcpRequestQueryInfoEx,
               &tcpRequestBufSize,
               pQueryBuffer,
               pQueryBufferSize
               );

   if( result )
   {
#ifdef ZNEVER
      // This is an ordinary occurance for Win98 PPP
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n",
         __FILE__,
         __LINE__,
         result
         );
#endif

      return( result );
   }

   return( result );
}


int
TDIQ_GetIFEntryForInstance(
   ULONG nInstance,
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize // IFEntry size byte plus MAX_IFDESCR_LEN for description
   )
{
   int   result = ERROR_SUCCESS;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   ULONG tcpRequestBufSize = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = IF_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = nInstance;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = IF_MIB_STATS_ID;

   result = WsControl(
               IPPROTO_TCP,
               WSCNTL_TCPIP_QUERY_INFO,
               &tcpRequestQueryInfoEx,
               &tcpRequestBufSize,
               pQueryBuffer,
               pQueryBufferSize
               );

   if( result )
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n",
         __FILE__,
         __LINE__,
         result
         );

      return( result );
   }

   return( result );
}

int
TDIQ_GetIPSNMPInfo(
   IPSNMPInfo  *pIPSnmpInfo,
   PULONG      pInfoSize
   )
{
   int   result = ERROR_SUCCESS;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   ULONG tcpRequestBufSize = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = CL_NL_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_STATS_ID;

   result = WsControl(
               IPPROTO_TCP,
               WSCNTL_TCPIP_QUERY_INFO,
               &tcpRequestQueryInfoEx,
               &tcpRequestBufSize,
               pIPSnmpInfo,
               pInfoSize
               );

   if( result )
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n",
         __FILE__,
         __LINE__,
         result
         );

      return( result );
   }

   return( result );
}


int
TDIQ_GetIFEntryForIFIndex(
   ULONG nIFIndex,
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize // IFEntry size byte plus MAX_IFDESCR_LEN for description
   )
{
   ULONG       nInstance = 0;
   ULONG       nSize;
   int         nResult = ERROR_SUCCESS;
   IFEntry     *pIFEntry;

   //
   // Walk The List Of IFEntries By Instance
   //
   while( !nResult )
   {
      nSize = *pQueryBufferSize;

      nResult = TDIQ_GetIFEntryForInstance(
                  nInstance++,
                  pQueryBuffer,
                  &nSize
                  );

      if( nResult )
      {
         break;
      }

      //
      // Check For Match On Index
      //
      pIFEntry = (IFEntry * )pQueryBuffer;

      if( pIFEntry->if_index == nIFIndex )
      {
         nResult = ERROR_SUCCESS;
         *pQueryBufferSize = nSize;
         break;
      }
   }

   return( nResult );
}

int
TDIQ_GetIPRouteTable(
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize
   )
{
   int   result = ERROR_SUCCESS;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   ULONG tcpRequestBufSize = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = CL_NL_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

   result = WsControl(
               IPPROTO_TCP,
               WSCNTL_TCPIP_QUERY_INFO,
               &tcpRequestQueryInfoEx,
               &tcpRequestBufSize,
               pQueryBuffer,
               pQueryBufferSize
               );

   if( result )
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n",
         __FILE__,
         __LINE__,
         result
         );

      return( result );
   }

   return( result );
}


int
TDIQ_GetIPAddrTable(
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize
   )
{
   int   result = ERROR_SUCCESS;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   ULONG tcpRequestBufSize = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = CL_NL_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

   result = WsControl(
               IPPROTO_TCP,
               WSCNTL_TCPIP_QUERY_INFO,
               &tcpRequestQueryInfoEx,
               &tcpRequestBufSize,
               pQueryBuffer,
               pQueryBufferSize
               );

   if( result )
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n",
         __FILE__,
         __LINE__,
         result
         );

      return( result );
   }

   return( result );
}

BOOLEAN
TDIQ_IsIPInstalled( VOID )
{
   int result = 0;
   TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
   DWORD       tcpRequestBufSize = sizeof(tcpRequestQueryInfoEx);
   DWORD       entityIdsBufSize;
   DWORD       i;
   TDIEntityID *entityIds;
   DWORD       entityCount;

   memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

   tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = GENERIC_ENTITY;
   tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;

   tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_GENERIC;
   tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
   tcpRequestQueryInfoEx.ID.toi_id = ENTITY_LIST_ID;

   entityIdsBufSize = MAX_TDI_ENTITIES * sizeof(TDIEntityID);
   entityIds = (TDIEntityID *)calloc(entityIdsBufSize, 1);

   result = WsControl(
                IPPROTO_TCP,
                WSCNTL_TCPIP_QUERY_INFO,
                &tcpRequestQueryInfoEx,
                &tcpRequestBufSize,
                entityIds,
                &entityIdsBufSize);

   if (result)
   {
      free( entityIds );

      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
         WSAGetLastError());
      return FALSE;
   }

   //...after the call we compute:
   entityCount = entityIdsBufSize / sizeof(TDIEntityID);

   //find the ip interface
   for (i = 0; i < entityCount; i++)
   {
      ULONG entityType;
      DWORD entityTypeSize;

      if (entityIds[i].tei_entity == CL_NL_ENTITY)
      {
         //get ip interface info
         memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));

         tcpRequestQueryInfoEx.ID.toi_entity = entityIds[i];
         tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_GENERIC;
         tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
         tcpRequestQueryInfoEx.ID.toi_id = ENTITY_TYPE_ID;

         entityType;
         entityTypeSize = sizeof(entityType);

         result = WsControl(
                     IPPROTO_TCP,
                     WSCNTL_TCPIP_QUERY_INFO,
                     &tcpRequestQueryInfoEx,
                     &tcpRequestBufSize,
                     &entityType,
                     &entityTypeSize
                     );

         if (result)
         {
            fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
               WSAGetLastError());
            return FALSE;
         }

         if (entityType == CL_NL_IP) // Entity implements IP.
         {
            return( TRUE );
         }
      }
   }

   return( FALSE );
}

/////////////////////////////////////////////////////////////////////////////
//// TDIQ_GetNextIPRouteTableEntry
//
// Purpose
// Return pointer to the next IPRouteEntry given a pointer to a current
// IPRouteEntry.
//
// Parameters
//
// Return Value
// 
// Remarks
// The size of an IPRouteEntry differs between the Windows 9X and Windows
// NT platforms. There are several different approaches that could be
// used to accomodate this difference. The method used in TDIQ is to
// use this function find the next entry by adding a platform-specific
// offset to the current entry.
//

IPRouteEntry *
TDIQ_GetNextIPRouteTableEntry(
   IPRouteEntry *pIPRouteEntry
   )
{
   PUCHAR pEntry = (PUCHAR )pIPRouteEntry;

   pEntry += g_nIPRouteEntrySize;

   return( (IPRouteEntry * )pEntry );
}

// Call AFTER TDIQ_Startup...
BOOLEAN
TDIQ_IsWindows95( VOID )
{
   return( g_bIsWindows9X );
}

int
TDIQ_Startup( VOID )
{
   int         result = ERROR_SUCCESS;
   WSADATA     WSAData;

   g_hModule = LoadLibrary("wsock32.dll");
   if( !g_hModule )
   {
      fprintf(stderr, "LoadLibrary failed for wsock32.dll (%ld)\n", GetLastError());
      return EXIT_FAILURE;
   }

   WsControl = (WsControlProc)GetProcAddress( g_hModule, "WsControl");
   if( WsControl )
   {
      g_bIsWindows9X = TRUE;

      g_nIPRouteEntrySize = sizeof( IPRouteEntry );
      g_nIPRouteEntrySize -= sizeof( ULONG );
   }
   else
   {
      WsControl = WsControlNT;
      g_nIPRouteEntrySize = sizeof( IPRouteEntry );
   }

   result = WSAStartup(MAKEWORD(1, 1), &WSAData);

   if (result)
   {
      fprintf(stderr, "WSAStartup failed (%ld)\n", result);
      FreeLibrary(g_hModule);
      g_hModule = NULL;
      return FALSE;
   }

   return( result );
}

VOID
TDIQ_Cleanup( VOID )
{
   WSACleanup();
   if( g_hModule )
   {
      FreeLibrary( g_hModule );
   }

   g_hModule = NULL;
}

//end of code

