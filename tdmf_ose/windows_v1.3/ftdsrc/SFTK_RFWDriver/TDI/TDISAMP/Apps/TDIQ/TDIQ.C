/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include <windows.h>
#include <stdio.h>

#include <winsock.h>
#include <crtdbg.h>

#include <winioctl.h>   // PCAUSA

#include "tdiinfo.h" // from recent NT4DDK "\ddk\src\network\inc\tdiinfo.h"
#include "smpletcp.h" // from recent NT4DDK "\ddk\src\network\wshsmple\smpletcp.h"

#include "TDIQHelp.h"

// Copyright And Configuration Management ----------------------------------
//
//                         TDI Query (TDIQ) Test Tool
//                                    For
//                          Windows 95 And Windows NT
//
//                        MAIN Entry Point - TDIQ.c
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

#define  TDIQ_VERSION "1.00.00.01"

#define  SHOW_IPCONFIG  0x01
#define  SHOW_IPROUTE   0x02
#define  SHOW_IPSTAT    0x04
#define  SHOW_ATTABLE   0x08

ULONG    g_ShowFlags = 0;

//
// An Unsuccessful Experment...
//
UCHAR g_AtTable[ 2048 ];

#pragma pack(push,1)      // x86, MS compiler; MIPS, MIPS compiler

typedef
struct _ATTableEntry
{
   ULONG atIfIndex;     // Interface Index
   ULONG ate_type;      // Interface Type??? 6 -> Ethernet...
   ULONG ate_3;
   ULONG ate_4;
   ULONG atNetAddress;  // IP Address
   ULONG ate_6;         // So far always 0x00000003
}
   ATTableEntry, *PATTableEntry;

#pragma pack(pop)                  // x86, MS compiler; MIPS, MIPS compiler

extern int
TDIQ_GetAtTable(
   PUCHAR pQueryBuffer,
   PULONG pQueryBufferSize
   );

VOID
DisplayATTableEntry(
   PUCHAR   pQueryBuffer,
   ULONG    nQueryBufferSize
   )
{
   DWORD nEntryCount;
   PATTableEntry  pATTableEntry;
   struct in_addr inadDest;
   char szDestIp[128];
   int   i;

   if( !nQueryBufferSize )
   {
      printf( "AT Table Is Empty\n" );
      return;
   }

   nEntryCount = nQueryBufferSize/sizeof( ATTableEntry );
   pATTableEntry = (PATTableEntry )pQueryBuffer;

   for( i = 0; i < nEntryCount; ++i )
   {
      inadDest.s_addr = pATTableEntry->atNetAddress;
      strcpy(szDestIp, inet_ntoa(inadDest));

      printf( "%d %d 0x%8.8X 0x%8.8X %s 0x%8.8X\n",
         pATTableEntry->atIfIndex,
         pATTableEntry->ate_type,
         pATTableEntry->ate_3,
         pATTableEntry->ate_4,
         szDestIp,
         pATTableEntry->ate_6
         );

      ++pATTableEntry;
   }
}


VOID
TDIQ_DisplayIPRouteTable(
   PUCHAR pIPRouteTableBuffer,
   IPSNMPInfo *pIPSNMPInfo,
   IPAddrEntry *pIPAddrTable
   )
{
   int i, j, result;
   IPRouteEntry *pIPRouteEntry;

   printf( "==========================================================================\n");
   printf( "Interface List:\n" );

   for( i = 0; i < pIPSNMPInfo->ipsi_numif; i++ )
   {
      ULONG IFEntryBufferSize = sizeof( IFEntry ) + MAX_IFDESCR_LEN;
      UCHAR IFEntryBuffer[ sizeof( IFEntry ) + MAX_IFDESCR_LEN ];
      IFEntry *pIFEntry = (IFEntry * )IFEntryBuffer;

      //
      // Get IFEntry For Address
      //
      result = TDIQ_GetIFEntryForIFIndex(
                  pIPAddrTable[pIPSNMPInfo->ipsi_numif - 1 - i].iae_index,
                  (PUCHAR )pIFEntry,
                  &IFEntryBufferSize
                  );

      if( result )
      {
         fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
            WSAGetLastError());
         continue;
      }

      printf( "0x%x ...",
         0xFF & pIPAddrTable[pIPSNMPInfo->ipsi_numif - 1 - i].iae_index
         );

      //
      // Display Physical Address
      //
      for( j = 0; j < pIFEntry->if_physaddrlen; ++j )
      {
         printf( "%2.2X", pIFEntry->if_physaddr[j] );

         if( j < pIFEntry->if_physaddrlen - 1 )
         {
            printf("-" );
         }
      }

      while( j++ < MAX_PHYSADDR_SIZE )
      {
         printf( "..." );
      }

      if( pIFEntry->if_physaddrlen )
      {
         printf( "." );    // Asthetics...
      }

      printf( " %*.*s\n",
         pIFEntry->if_descrlen,
         pIFEntry->if_descrlen,
         pIFEntry->if_descr
         );
   }
   printf( "==========================================================================\n");

   printf( "==========================================================================\n");
   printf( "Active Routes:\n" );
   printf( "Network Address          Netmask  Gateway Address        Interface  Metric\n");

   pIPRouteEntry = (IPRouteEntry * )pIPRouteTableBuffer;

   for( i = 0; i < pIPSNMPInfo->ipsi_numroutes; i++ )
   {
      IPAddrEntry *pIPAddrEntry = NULL;
      struct in_addr inadDest;
      struct in_addr inadMask;
      struct in_addr inadGateway;
      struct in_addr inadInterface;
      char szDestIp[128];
      char szMaskIp[128];
      char szGatewayIp[128];
      char szInterfaceIp[128];

      for( j = 0; j < pIPSNMPInfo->ipsi_numaddr; j++ )
      {
         if( pIPAddrTable[j].iae_index == pIPRouteEntry->ire_index )
         {
            pIPAddrEntry = &pIPAddrTable[j];
            break;
         }
      }

      if( !pIPAddrEntry )
      {
         //
         // Move To Next Route
         //
         pIPRouteEntry = TDIQ_GetNextIPRouteTableEntry( pIPRouteEntry );
         continue;
      }

      inadDest.s_addr = pIPRouteEntry->ire_dest;
      inadMask.s_addr = pIPRouteEntry->ire_mask;
      inadGateway.s_addr = pIPRouteEntry->ire_nexthop;
      inadInterface.s_addr = pIPAddrEntry->iae_addr;

      strcpy(szDestIp, inet_ntoa(inadDest));
      strcpy(szMaskIp, inet_ntoa(inadMask));
      strcpy(szGatewayIp, inet_ntoa(inadGateway));
      strcpy(szInterfaceIp, inet_ntoa(inadInterface));

      printf("%15s %16s %16s %16s %7d\n", 
            szDestIp, 
            szMaskIp, 
            szGatewayIp, 
            szInterfaceIp, 
            pIPRouteEntry->ire_metric1
            );

      //
      // Move To Next Route
      //
      pIPRouteEntry = TDIQ_GetNextIPRouteTableEntry( pIPRouteEntry );
   }
   printf( "==========================================================================\n");

   //
   // Newline After Displaying Route Information
   //
   printf( "\n" );
}

VOID
TDIQ_DisplayIPAddrTable(
   PUCHAR pIPRouteTableBuffer,
   IPSNMPInfo *pIPSNMPInfo,
   IPAddrEntry *pIPAddrTable
   )
{
   int i, j, result;

   printf( "IP Configuration:\n" );

   for( i = 0; i < pIPSNMPInfo->ipsi_numaddr; i++ )
   {
      ULONG IFEntryBufferSize;
      UCHAR IFEntryBuffer[ sizeof( IFEntry ) + MAX_IFDESCR_LEN ];
      IFEntry *pIFEntry;
      ULONG InterfaceInfoSize;
      UCHAR InterfaceInfo[ sizeof( IPInterfaceInfo ) + MAX_PHYSADDR_SIZE ];
      IPInterfaceInfo *pIPInterfaceInfo;
      struct in_addr inaddrScratch;
      IPRouteEntry *pIPRouteTableEntry;
      IPRouteEntry *pIPRouteEntry;

      //
      // Get IPInterfaceInfo For Address
      //
      InterfaceInfoSize = sizeof( InterfaceInfo );
      pIPInterfaceInfo = (IPInterfaceInfo * )InterfaceInfo;

      result = TDIQ_GetIPInterfaceInfoForAddr(
                  pIPAddrTable[i].iae_addr,
                  (PUCHAR )pIPInterfaceInfo,
                  &InterfaceInfoSize
                  );

      if( result )
      {
         pIPInterfaceInfo = NULL;
      }

      //
      // Get IFEntry For Address
      //
      IFEntryBufferSize = sizeof( IFEntryBuffer );
      pIFEntry = (IFEntry * )IFEntryBuffer;

      result = TDIQ_GetIFEntryForIFIndex(
                  pIPAddrTable[i].iae_index,
                  (PUCHAR )pIFEntry,
                  &IFEntryBufferSize
                  );

      if( result )
      {
         fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
            WSAGetLastError());
         continue;
      }

      //
      // Display Adapter Banner
      //
      switch( pIFEntry->if_type )
      {
         case IF_TYPE_OTHER:        // MIB_IF_TYPE_OTHER
            printf( "OTHER Type Adapter:\n\n" );
            break;

         case IF_TYPE_ETHERNET:     // MIB_IF_TYPE_ETHERNET
            printf( "Ethernet Adapter:\n\n" );
            break;

         case IF_TYPE_TOKENRING:    // MIB_IF_TYPE_TOKENRING
            printf( "Tokenring Adapter:\n\n" );
            break;

         case IF_TYPE_FDDI:         // MIB_IF_TYPE_FDDI
            printf( "FDDI Adapter:\n\n" );
            break;

         case IF_TYPE_PPP:          // MIB_IF_TYPE_PPP
            printf( "PPP Adapter:\n\n" );
            break;

         case IF_TYPE_LOOPBACK:     // MIB_IF_TYPE_LOOPBACK
            printf( "Loopback Adapter:\n\n" );
            break;

         case IF_TYPE_SLIP:         // MIB_IF_TYPE_SLIP
            printf( "SLIP Adapter:\n\n" );
            break;

         default:
            printf( "UNKNOWN Adapter Type:\n\n" );
            break;
      }

      //
      // Display Description
      //
      printf( "  Description..........: %*.*s\n",
         pIFEntry->if_descrlen,
         pIFEntry->if_descrlen,
         pIFEntry->if_descr
         );

      //
      // Display Physical Address
      //
      printf( "  Physical Address.....: " );

      for( j = 0; j < pIFEntry->if_physaddrlen; ++j )
      {
         printf( "%2.2X", pIFEntry->if_physaddr[j] );

         if( j < pIFEntry->if_physaddrlen - 1 )
         {
            printf("-" );
         }
      }

      printf( "\n" );

      //
      // Display IP Address
      //
      inaddrScratch.s_addr = pIPAddrTable[i].iae_addr;

      printf( "  IP Address...........: %s\n", inet_ntoa(inaddrScratch) );

      //
      // Display Subnet Mask
      //
      inaddrScratch.s_addr = pIPAddrTable[i].iae_mask;

      printf( "  Subnet Mask..........: %s\n", inet_ntoa(inaddrScratch) );

      //
      // Display Gateway
      //
      pIPRouteTableEntry = (IPRouteEntry * )pIPRouteTableBuffer;
      pIPRouteEntry = NULL;

      if( pIPInterfaceInfo )
      {
         for( j = 0; j < pIPSNMPInfo->ipsi_numroutes; j++ )
         {
            if( pIPAddrTable[i].iae_index == pIPRouteTableEntry->ire_index )
            {
               if( pIPRouteTableEntry->ire_dest == 0 )
               {
                  pIPRouteEntry = pIPRouteTableEntry;
                  break;
               }
            }

            //
            // Move To Next Route
            //
            pIPRouteTableEntry = TDIQ_GetNextIPRouteTableEntry( pIPRouteTableEntry );
         }
      }

      if( pIPRouteEntry )
      {
         inaddrScratch.s_addr = pIPRouteEntry->ire_nexthop;

         printf( "  Default Gateway......: %s\n", inet_ntoa(inaddrScratch) );
      }
      else
      {
         inaddrScratch.s_addr = pIPAddrTable[i].iae_addr;

         printf( "  Default Gateway......: %s\n", inet_ntoa(inaddrScratch) );
      }

      //
      // Display IP Interface Speed
      //
      if( pIPInterfaceInfo )
      {
         printf( "  Speed................: %d bits/second\n", pIPInterfaceInfo->iii_speed );
      }
      else
      {
         printf( "  Speed................: Not Available\n" );
      }

      //
      // Display MTU
      //
      if( pIPInterfaceInfo )
      {
         printf( "  MTU (Local Nodes)....: %d bytes\n", pIPInterfaceInfo->iii_mtu );
      }
      else
      {
         printf( "  MTU (Local Nodes)....: Not Available\n" );
      }

      //
      // Newline After Displaying Each Adapter's Information
      //
      printf( "\n" );
   }
}

VOID
TDIQ_DisplayIPStats( IPSNMPInfo *pIPSNMPInfo )
{
   printf( "IP Statistics:\n" );

   printf( "  dwForwarding       = %d\n", pIPSNMPInfo->ipsi_forwarding );
   printf( "  dwDefaultTTL       = %d\n", pIPSNMPInfo->ipsi_defaultttl );
   printf( "  dwInReceives       = %d\n", pIPSNMPInfo->ipsi_inreceives );
   printf( "  dwInHdrErrors      = %d\n", pIPSNMPInfo->ipsi_inhdrerrors );
   printf( "  dwInAddrErrors     = %d\n", pIPSNMPInfo->ipsi_inaddrerrors );
   printf( "  dwForwDatagrams    = %d\n", pIPSNMPInfo->ipsi_forwdatagrams );
   printf( "  dwInUnknownProtos  = %d\n", pIPSNMPInfo->ipsi_inunknownprotos );
   printf( "  dwInDiscards       = %d\n", pIPSNMPInfo->ipsi_indiscards );
   printf( "  dwInDelivers       = %d\n", pIPSNMPInfo->ipsi_indelivers );
   printf( "  dwOutRequests      = %d\n", pIPSNMPInfo->ipsi_outrequests );
   printf( "  dwRoutingDiscards  = %d\n", pIPSNMPInfo->ipsi_routingdiscards );
   printf( "  dwOutDiscards      = %d\n", pIPSNMPInfo->ipsi_outdiscards );
   printf( "  dwOutNoRoutes      = %d\n", pIPSNMPInfo->ipsi_outnoroutes );
   printf( "  dwReasmTimeout     = %d\n", pIPSNMPInfo->ipsi_reasmtimeout );
   printf( "  dwReasmReqds       = %d\n", pIPSNMPInfo->ipsi_reasmreqds );
   printf( "  dwReasmOks         = %d\n", pIPSNMPInfo->ipsi_reasmoks );
   printf( "  dwReasmFails       = %d\n", pIPSNMPInfo->ipsi_reasmfails );
   printf( "  dwFragOks          = %d\n", pIPSNMPInfo->ipsi_fragoks );
   printf( "  dwFragFails        = %d\n", pIPSNMPInfo->ipsi_fragfails );
   printf( "  dwFragCreates      = %d\n", pIPSNMPInfo->ipsi_fragcreates );
   printf( "  dwNumIf            = %d\n", pIPSNMPInfo->ipsi_numif );
   printf( "  dwNumAddr          = %d\n", pIPSNMPInfo->ipsi_numaddr );
   printf( "  dwNumRoutes        = %d\n", pIPSNMPInfo->ipsi_numroutes );

   //
   // Newline After Displaying Statistics
   //
   printf( "\n" );
}

//
// Usage Message
//
char Usage[] = "\
Usage: tdiq [-options]\n\
Options:\n\
  -ipconfig - Display IP configuration information\n\
  -iproute  - Display IP route information\n\
  -ipstat   - Display IP statistics information\n\
  -all      - Display all information\n\
";

int main(int argc, char **argv)
{
   int         result = ERROR_SUCCESS;
   BOOL        bResult;
   IPSNMPInfo  ipSnmpInfo;
   DWORD       ipSnmpInfoSize;
   DWORD       ipAddrTableSize;
   IPAddrEntry *pIPAddrTable = NULL;
   DWORD       ipRouteTableBufSize;
   PUCHAR      pIPRouteTableBuffer = NULL;
   DWORD       AtInfoSize;

	if( argc < 2 )
   {
      fprintf(stderr,Usage);
      exit( 0 );
   }

   while( *++argv )
   {
      char *s = *argv;

      if( *s == '-' || *s == '/' )
      {
         ++s;
      }

      if( !stricmp( s, "ipconfig" ) )
      {
         g_ShowFlags |= SHOW_IPCONFIG;
      }
      else if( !stricmp( s, "iproute" ) )
      {
         g_ShowFlags |= SHOW_IPROUTE;
      }
      else if( !stricmp( s, "ipstat" ) )
      {
         g_ShowFlags |= SHOW_IPSTAT;
      }
      else if( !stricmp( s, "xlat" ) )
      {
         g_ShowFlags |= SHOW_ATTABLE;
      }
      else if( !stricmp( s, "all" ) )
      {
         g_ShowFlags |= SHOW_IPCONFIG;
         g_ShowFlags |= SHOW_IPROUTE;
         g_ShowFlags |= SHOW_IPSTAT;
      }
   }

   if( !g_ShowFlags )
   {
      fprintf(stderr,Usage);
      exit( 0 );
   }

   result = TDIQ_Startup();

   if( result )
   {
      fprintf(stderr, "TDIQ_Startup failed (%ld)\n", result);
      return EXIT_FAILURE;
   }

   printf( "TDI Query (TDIQ) Test Tool for Windows %s - V%s\n",
      TDIQ_IsWindows95() ? "95" : "NT",
      TDIQ_VERSION
      );
   printf( "Copyright (c) 2000-2001 Printing Communications Assoc., Inc (PCAUSA)\n\n" );

   //
   // Confirm That TCP/IP Is Installed
   //
   bResult = TDIQ_IsIPInstalled();

   if( !bResult )
   {
      fprintf(stderr, "%s(%d) IP Not Installed\n", __FILE__, __LINE__ );

      TDIQ_Cleanup();
      return EXIT_FAILURE;
   }

   //get ip snmp info
   ipSnmpInfoSize = sizeof(ipSnmpInfo);

   result = TDIQ_GetIPSNMPInfo( &ipSnmpInfo, &ipSnmpInfoSize );

   if (result)
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
         WSAGetLastError());

      TDIQ_Cleanup();
      return EXIT_FAILURE;
   }

   //
   // Get IP Address Table
   //
   ipAddrTableSize = sizeof(IPAddrEntry) * ipSnmpInfo.ipsi_numaddr;
   pIPAddrTable = (IPAddrEntry *)calloc(ipAddrTableSize, 1);

   result = TDIQ_GetIPAddrTable(
               (PUCHAR )pIPAddrTable,
               &ipAddrTableSize
               );

   if (result)
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
         WSAGetLastError());

      TDIQ_Cleanup();
      return EXIT_FAILURE;
   }

   //
   // Get IP Route Table
   //
   ipRouteTableBufSize = sizeof(IPRouteEntry) * ipSnmpInfo.ipsi_numroutes;
   pIPRouteTableBuffer = (PUCHAR )calloc(ipRouteTableBufSize, 1);

   result = TDIQ_GetIPRouteTable(
               pIPRouteTableBuffer,
               &ipRouteTableBufSize
               );

   if (result)
   {
      fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
         WSAGetLastError());

      TDIQ_Cleanup();
      return EXIT_FAILURE;
   }

   //
   // Display IP Address Table
   //
   if( g_ShowFlags & SHOW_IPCONFIG )
   {
      TDIQ_DisplayIPAddrTable( pIPRouteTableBuffer, &ipSnmpInfo, pIPAddrTable );
   }

   //
   // Display Route Table
   //
   if( g_ShowFlags & SHOW_IPROUTE )
   {
      TDIQ_DisplayIPRouteTable( pIPRouteTableBuffer, &ipSnmpInfo, pIPAddrTable );
   }

   //
   // Display IP Statistics
   //
   if( g_ShowFlags & SHOW_IPSTAT )
   {
      TDIQ_DisplayIPStats( &ipSnmpInfo );
   }


   if( g_ShowFlags & SHOW_ATTABLE )
   {
      //get address translation table
      AtInfoSize = sizeof(g_AtTable);

      result = TDIQ_GetAtTable( g_AtTable, &AtInfoSize );

      if (result)
      {
         fprintf(stderr, "%s(%d) TdiQueryDeviceControl failed (%ld)\n", __FILE__, __LINE__,
            WSAGetLastError());

         AtInfoSize = 0;
      }
      else
      {
         DisplayATTableEntry( g_AtTable, AtInfoSize );
      }
   }

   TDIQ_Cleanup();
   return( EXIT_SUCCESS );
}

//end of code

