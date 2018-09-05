// Simple driver that demonstrates dynamically loading and unloading

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "KSTcpEx.h"
#include "KSUtil.h"

#define NT_DEVICE_NAME_W     L"\\Device\\TDIQtest"
#define DOS_DEVICE_NAME_W    L"\\DosDevices\\TDIQtest"

#define TL_INSTANCE  0

#define MAX_FAST_ENTITY_BUFFER ( sizeof(TDIEntityID) * 20 )
#define MAX_FAST_ADDRESS_BUFFER ( sizeof(IPAddrEntry) * 10 )

TDI_PROVIDER_INFO          g_TdiProviderInfo;
TDI_PROVIDER_STATISTICS    g_TdiProviderStatistics;

#define ADDR_TAB_COUNT 20
IPAddrEntry                g_IPAddressTable[ ADDR_TAB_COUNT ];

IPSNMPInfo                 g_SnmpInfo;


NTSTATUS
TDIQtestOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
TDIQtestClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
TDIQtestUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
DoQueryProviderInfoTest( void )
{
   NTSTATUS          Status;

   //
   // Query Tcp Provider Information
   //
   Status = KS_QueryProviderInfo(
               TCP_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         TCP_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

   //
   // Query Udp Provider Information
   //
   Status = KS_QueryProviderInfo(
               UDP_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         UDP_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

   //
   // Query An Example RawIp Provider Information
   //
   Status = KS_QueryProviderInfo(
               L"\\Device\\RawIp\\11",
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         L"\\Device\\RawIp\\11",
         &g_TdiProviderInfo
         );
   }

   //
   // Query AtalkDdp Provider Information
   //
   Status = KS_QueryProviderInfo(
               ATALK_DDP_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         ATALK_DDP_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

   //
   // Query AtalkAdsp Provider Information
   //
   Status = KS_QueryProviderInfo(
               ATALK_ADSP_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         ATALK_ADSP_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

   //
   // Query AtalkAspClient Provider Information
   //
   Status = KS_QueryProviderInfo(
               ATALK_ASP_CLIENT_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         ATALK_ASP_CLIENT_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

   //
   // Query AtalkAspServer Provider Information
   //
   Status = KS_QueryProviderInfo(
               ATALK_ASP_SERVER_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         ATALK_ASP_SERVER_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

   //
   // Query AtalkPap Provider Information
   //
   Status = KS_QueryProviderInfo(
               ATALK_PAP_DEVICE_NAME_W,
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         ATALK_PAP_DEVICE_NAME_W,
         &g_TdiProviderInfo
         );
   }

#ifdef ZNEVER
   //
   // Query PCAUSA-Specific Protocols
   //
   Status = KS_QueryProviderInfo(
               L"\\Device\\Nbf_PCASIMMP2",
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         L"\\Device\\Nbf_PCASIMMP2",
         &g_TdiProviderInfo
         );
   }

   Status = KS_QueryProviderInfo(
               L"\\Device\\NetBT_PCASIMMP2",
               &g_TdiProviderInfo
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         L"\\Device\\NetBT_PCASIMMP2",
         &g_TdiProviderInfo
         );
   }
#endif // ZNEVER
}


VOID
DoQueryProviderStatisticsTest( void )
{
   NTSTATUS          Status;

   //
   // Query Tcp Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               TCP_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         TCP_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

   //
   // Query Udp Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               UDP_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         UDP_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

   //
   // Query An Example RawIp Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               L"\\Device\\RawIp\\11",
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         L"\\Device\\RawIp\\11",
         &g_TdiProviderStatistics
         );
   }

   //
   // Query AtalkDdp Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               ATALK_DDP_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         ATALK_DDP_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

   //
   // Query AtalkAdsp Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               ATALK_ADSP_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         ATALK_ADSP_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

   //
   // Query AtalkAspClient Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               ATALK_ASP_CLIENT_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         ATALK_ASP_CLIENT_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

   //
   // Query AtalkAspServer Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               ATALK_ASP_SERVER_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         ATALK_ASP_SERVER_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

   //
   // Query AtalkPap Provider Statistics
   //
#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               ATALK_PAP_DEVICE_NAME_W,
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         ATALK_PAP_DEVICE_NAME_W,
         &g_TdiProviderStatistics
         );
   }

#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

#ifdef ZNEVER
   //
   // Query PCAUSA-Specific Protocols
   //
   Status = KS_QueryProviderStatistics(
               L"\\Device\\Nbf_PCASIMMP2",
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         L"\\Device\\Nbf_PCASIMMP2",
         &g_TdiProviderStatistics
         );
   }

#ifdef DBG
   RtlFillMemory(
      &g_TdiProviderStatistics,
      sizeof( TDI_PROVIDER_STATISTICS ),
      0xFF
      );
#endif // DBG

   Status = KS_QueryProviderStatistics(
               L"\\Device\\NetBT_PCASIMMP2",
               &g_TdiProviderStatistics
               );

   if(NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderStatistics(
         L"\\Device\\NetBT_PCASIMMP2",
         &g_TdiProviderStatistics
         );
   }
#endif // ZNEVER
}


VOID
DisplayIPAddrEntry(
   PFILE_OBJECT   pFileObject,
   IPAddrEntry    *pIPAddrEntry  // Defined in KSTcpEx.h
   )
{
   PUCHAR   pByte;
   NTSTATUS Status;
   UCHAR InterfaceInfo[ sizeof( IPInterfaceInfo ) + MAX_PHYSADDR_SIZE ];
   ULONG InterfaceInfoSize;
   IPInterfaceInfo *pIPInterfaceInfo = NULL;
   int   j;

   //
   // Get IPInterfaceInfo For Address
   //
   InterfaceInfoSize = sizeof( InterfaceInfo );

   Status = KS_TCPGetIPInterfaceInfoForAddr(
               pFileObject,
               pIPAddrEntry->iae_addr,
               InterfaceInfo,
               &InterfaceInfoSize
               );
   
   if( NT_SUCCESS( Status ) )
   {
      pIPInterfaceInfo = (IPInterfaceInfo * )InterfaceInfo;
   }

   //
   // Dump IP Address Index
   //
   KdPrint(( "[%d]  ", pIPAddrEntry->iae_index ));

   //
   // Dump IP Address
   //
   pByte = (PUCHAR )&pIPAddrEntry->iae_addr;

   KdPrint(( "%d.%d.%d.%d  ",
      pByte[ 0 ], pByte[ 1 ], pByte[ 2 ], pByte[ 3 ]
      ));

   //
   // Dump IP Address Mask
   //
   pByte = (PUCHAR )&pIPAddrEntry->iae_mask;

   KdPrint(( "%d.%d.%d.%d  ",
      pByte[ 0 ], pByte[ 1 ], pByte[ 2 ], pByte[ 3 ]
      ));

   //
   // Dump MAC/Physical Address
   //
   if( pIPInterfaceInfo && pIPInterfaceInfo->iii_addrlength )
   {
      for( j = 0; j < pIPInterfaceInfo->iii_addrlength; ++j )
      {
         KdPrint(( "%2.2X", pIPInterfaceInfo->iii_addr[j] ));

         if( j < pIPInterfaceInfo->iii_addrlength - 1 )
         {
            KdPrint(( "-" ));
         }
      }
   }
   else
   {
      KdPrint(( "XX-XX-XX-XX-XX-XX" ));
   }

   KdPrint(( "\n" ));
}


VOID
DoTCPQueryTest( void )
{
   KS_CHANNEL        KS_Channel;
   NTSTATUS          Status;
   ULONG             nBufferSize, nNumAddr;
   int               j;

   //
   // Open A TDI Control Channel To The Transport
   //
   Status = KS_OpenControlChannel( TCP_DEVICE_NAME_W, &KS_Channel );

   if( !NT_SUCCESS(Status) )
   {
      DbgPrint( "Unable To Open TDI Control Channel\n" );
      return;
   }

   //
   // Query For SNMP Information
   // --------------------------
   // The IP SNMP information of most interest includes:
   // 
	//    ipsi_numif     - Number of interfaces
	//    ipsi_numaddr   - Number of IP addresses
	//    ipsi_numroutes - Number of active routes
   //
   nBufferSize = sizeof( g_SnmpInfo );

   Status = KS_TCPGetIPSNMPInfo(
               KS_Channel.m_pFileObject,
               &g_SnmpInfo,
               &nBufferSize
               );

   if( NT_SUCCESS( Status ) )
   {
      DbgPrint( "Got IP SNMP Information\n" );
   }

   nNumAddr = g_SnmpInfo.ipsi_numaddr;

   //
   // Query For The IP Address Table
   //
   nBufferSize = sizeof( IPAddrEntry ) * ADDR_TAB_COUNT;

   Status = KS_TCPGetIPAddrTable(
               KS_Channel.m_pFileObject,
               g_IPAddressTable,
               &nBufferSize
               );

   if( NT_SUCCESS( Status ) )
   {
      DbgPrint( "Bytes Returned: %d; Addresses Returned: %d\n",
         nBufferSize, nNumAddr );

      for ( j = 0; j < nNumAddr; j++ )
      {
         DisplayIPAddrEntry(
            KS_Channel.m_pFileObject,
            &g_IPAddressTable[ j ]
            );
      }
   }

   if ( !NT_SUCCESS ( Status ) )
      DbgPrint ( "ERROR: unable to fetch IP address table\n" );

   //
   // Close The TDI Control Channel
   //
   KS_CloseControlChannel( &KS_Channel );
};


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS Status;
    UNICODE_STRING uniNtNameString;
    UNICODE_STRING uniWin32NameString;

    KdPrint( ("TDIQTest: Entered the Load/Unload driver!\n") );

    //
    // Create counted string version of our device name.
    //
    RtlInitUnicodeString( &uniNtNameString, NT_DEVICE_NAME_W );

    //
    // Create the device object
    //
    Status = IoCreateDevice(
                 DriverObject,
                 0,                     // We don't use a device extension
                 &uniNtNameString,
                 FILE_DEVICE_UNKNOWN,
                 0,                     // No standard device characteristics
                 FALSE,                 // This isn't an exclusive device
                 &deviceObject
                 );

    if ( NT_SUCCESS(Status) )
    {
        //
        // Create dispatch points for create/open, close, unload.
        //
        DriverObject->MajorFunction[IRP_MJ_CREATE] = TDIQtestOpen;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = TDIQtestClose;
        DriverObject->DriverUnload = TDIQtestUnload;

        KdPrint( ("TDIQTest: just about ready!\n") );

        //
        // Create counted string version of our Win32 device name.
        //
        RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME_W );
    
        //
        // Create a link from our device name to a name in the Win32 namespace.
        //
        Status = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );

        if (!NT_SUCCESS(Status))
        {
            KdPrint( ("TDIQTest: Couldn't create the symbolic link\n") );

            IoDeleteDevice( DriverObject->DeviceObject );
        }
        else
        {
            KdPrint( ("TDIQTest: All initialized!\n") );

            DoQueryProviderInfoTest();
            DoQueryProviderStatisticsTest();
            DoTCPQueryTest();
        }
    }
    else
    {
        KdPrint( ("TDIQTest: Couldn't create the device\n") );
    }
    return Status;
}

NTSTATUS
TDIQtestOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // No need to do anything.
    //

    //
    // Fill these in before calling IoCompleteRequest.
    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    KdPrint( ("TDIQTest: Opened!!\n") );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

NTSTATUS
TDIQtestClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // No need to do anything.
    //

    //
    // Fill these in before calling IoCompleteRequest.
    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    KdPrint( ("TDIQTest: Closed!!\n") );

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID
TDIQtestUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNICODE_STRING uniWin32NameString;

    //
    // All *THIS* driver needs to do is to delete the device object and the
    // symbolic link between our device name and the Win32 visible name.
    //
    // Almost every other driver ever witten would need to do a
    // significant amount of work here deallocating stuff.
    //

    KdPrint( ("TDIQTest: Unloading!!\n") );
    
    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME_W );

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //
    
    IoDeleteSymbolicLink( &uniWin32NameString );

    //
    // Finally delete our device object
    //

    IoDeleteDevice( DriverObject->DeviceObject );
}
