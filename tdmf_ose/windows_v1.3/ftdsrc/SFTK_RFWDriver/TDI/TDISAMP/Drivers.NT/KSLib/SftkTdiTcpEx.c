/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"
#include "SftkTdiUtil.h"

// Copyright And Configuration Management ----------------------------------
//
//                  TCP Extension IOCTL Support - KSTcpEx.c
//
//                  PCAUSA TDI Client Samples For Windows NT
//
//      Copyright (c) 1999-2000 Printing Communications Associates, Inc.
//                                - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//// NOTES
//
//

/////////////////////////////////////////////////////////////////////////////
//// STRUCTURES

#define TL_INSTANCE 0


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//
// ATTENTION!!!
// ------------
// Extend to support setting:
//
//   TCP-Level Options
//   UDP-Level Options
//   IP-Level Options
//   Socket-Level Options
//
// Account for the fact that some options must be set on Address file
// objects and others must be set on Connection file objects.
//
//

NTSTATUS
SFTK_TDI_TCPSetInformation(
   PFILE_OBJECT   pFileObject,
   ULONG          Entity,
   ULONG          Class,
   ULONG          Type,
   ULONG          Id,
   PVOID          pValue,
   ULONG          ValueLength
   )
{
   PIRP                 pIrp;
   IO_STATUS_BLOCK      IoStatus;
   PIO_STACK_LOCATION   pIrpSp;
   KEVENT               SyncEvent;
   NTSTATUS             Status;
   PTCP_REQUEST_SET_INFORMATION_EX pSetInfoEx;
   ULONG                Length;
   PDEVICE_OBJECT       pDeviceObject;

   //
   // Allocate Memory For Private Completion Context
   //
   Length = sizeof( TCP_REQUEST_SET_INFORMATION_EX ) + ValueLength;

   Status = NdisAllocateMemory(
                  &pSetInfoEx,
                  Length + sizeof( ULONG ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( Status ) )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   pSetInfoEx->ID.toi_entity.tei_entity = Entity;
   pSetInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
   pSetInfoEx->ID.toi_class = Class;
   pSetInfoEx->ID.toi_type = Type;
   pSetInfoEx->ID.toi_id = Id;

   NdisMoveMemory( pSetInfoEx->Buffer, (PUCHAR )pValue, ValueLength );
   pSetInfoEx->BufferSize = ValueLength;

   //
   // Create notification event object to be used to signal the
   // request completion.
   //
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   KeInitializeEvent(&SyncEvent, NotificationEvent, FALSE);

   //
   // Build the synchronous request to be sent to the Tcp driver
   //
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   pDeviceObject = IoGetRelatedDeviceObject(
                     pFileObject
                     );

   pIrp = IoBuildDeviceIoControlRequest(
            IOCTL_TCP_SET_INFORMATION_EX,
            pDeviceObject,
            pSetInfoEx,       // Input Buffer
            Length,          // Input Buffer Size
            NULL,       // Output Buffer
            0,          // Output Buffer Size
            FALSE,
            &SyncEvent,
            &IoStatus
            );

   if( pIrp == NULL )
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   pIrpSp = IoGetNextIrpStackLocation( pIrp );
   pIrpSp->FileObject = pFileObject;

   //
   // Pass request to Tcp driver and wait for request to complete.
   //
   Status = IoCallDriver( pDeviceObject, pIrp );

   if( (Status == STATUS_SUCCESS) || (Status == STATUS_PENDING) )
   {
      KeWaitForSingleObject(&SyncEvent, Executive, KernelMode, FALSE, NULL);

      NdisFreeMemory( pSetInfoEx, Length + sizeof( ULONG ), 0 );

      return( IoStatus.Status );
   }

   NdisFreeMemory( pSetInfoEx, Length + sizeof( ULONG ), 0 );

   return( Status );
}

NTSTATUS
SFTK_TDI_TdiCall ( 
   IN PIRP pIrp,
   IN PDEVICE_OBJECT pDeviceObject,
   IN OUT PIO_STATUS_BLOCK pIoStatusBlock
   )
{
   KEVENT kEvent;                                                    // signaling event
   NTSTATUS dStatus = STATUS_INSUFFICIENT_RESOURCES;                 // local status

   KeInitializeEvent ( &kEvent, NotificationEvent, FALSE );          // reset notification event
   pIrp->UserEvent = &kEvent;                                        // pointer to event
   pIrp->UserIosb = pIoStatusBlock;                               // pointer to status block
   dStatus = IoCallDriver ( pDeviceObject, pIrp );                   // call next driver
   if ( dStatus == STATUS_PENDING )                                  // make all request synchronous
   {
      (void)KeWaitForSingleObject ( 
                     (PVOID)&kEvent,                                 // signaling object
                     Executive,                                      // wait reason
                     KernelMode,                                     // wait mode
                     TRUE,                                           // alertable
                     NULL );                                         // timeout
   }
   dStatus = pIoStatusBlock->Status;                           
   return ( dStatus );                                               // return with status
}


NTSTATUS
SFTK_TDI_TCPQueryDeviceControl( 
   IN PFILE_OBJECT pFileObject,
   IN PVOID InputBuffer,
   IN ULONG InputBufferSize,
   IN OUT PVOID OutputBuffer,
   IN ULONG OutputBufferSize,
   OUT PULONG pdReturn
   )
{
   PIRP pIrp;                                   // local i/o request
   PIO_STACK_LOCATION pIoStack;                 // I/O Stack Location
   PDEVICE_OBJECT pDeviceObject;                // local device object
   IO_STATUS_BLOCK IoStatusBlock;               // return status
   NTSTATUS dStatus = STATUS_INVALID_PARAMETER; // default return status

   if ( pFileObject )
   {
      pDeviceObject = IoGetRelatedDeviceObject ( pFileObject );

      pIrp = IoBuildDeviceIoControlRequest(
                  IOCTL_TCP_QUERY_INFORMATION_EX,
                  pDeviceObject,
                  InputBuffer,
                  InputBufferSize,
                  OutputBuffer,
                  OutputBufferSize,
                  FALSE,
                  NULL,    // pointer to event
                  NULL     // pointer to return buffer
                  );

      if ( pIrp == NULL )
      {
         DbgPrint ( "ERROR: IoBuildDeviceIoControlRequest\n" );
         dStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
      else
      {
         pIoStack = IoGetNextIrpStackLocation( pIrp );   // get the iostack
         pIoStack->DeviceObject = pDeviceObject;         // store current device object
         pIoStack->FileObject = pFileObject;             // store file object in the stack

         dStatus = SFTK_TDI_TdiCall( pIrp, pDeviceObject, &IoStatusBlock );

         if ( pdReturn )                                 // requested by user?
            *pdReturn = IoStatusBlock.Information;       // return information size
      }
   }

   return ( dStatus );     // return with status
}


NTSTATUS
SFTK_TDI_TCPQueryInformation(
   PFILE_OBJECT   pFileObject,
   ULONG          Entity,
   ULONG          Instance,
   ULONG          Class,
   ULONG          Type,
   ULONG          Id,
   PVOID          pOutputBuffer,
   PULONG         pdOutputLength
   )
{
   TCP_REQUEST_QUERY_INFORMATION_EX QueryInfo;  // local query information

   RtlZeroMemory( &QueryInfo, sizeof( TCP_REQUEST_QUERY_INFORMATION_EX ) ); // input buffer length

   QueryInfo.ID.toi_entity.tei_entity = Entity;
   QueryInfo.ID.toi_entity.tei_instance = Instance;
   QueryInfo.ID.toi_class = Class;
   QueryInfo.ID.toi_type = Type;
   QueryInfo.ID.toi_id = Id;

   return( SFTK_TDI_TCPQueryDeviceControl(                        // send down device control call
               pFileObject,                                 // current transport/connection object
               &QueryInfo,                                  // input buffer
               sizeof ( TCP_REQUEST_QUERY_INFORMATION_EX ), // input buffer length
               pOutputBuffer,                               // output buffer
               *pdOutputLength,                             // output buffer length
               pdOutputLength                               // return information
               )
            );
}

NTSTATUS
SFTK_TDI_TCPGetIPSNMPInfo(
   PFILE_OBJECT   pFileObject,
   IPSNMPInfo     *pIPSnmpInfo,
   PULONG         pBufferSize
   )
{
   NTSTATUS Status;

   Status = SFTK_TDI_TCPQueryInformation( 
               pFileObject,         // control object
               CL_NL_ENTITY,        // entity
               TL_INSTANCE,         // instance
               INFO_CLASS_PROTOCOL, // class
               INFO_TYPE_PROVIDER,  // type
               IP_MIB_STATS_ID,     // id
               pIPSnmpInfo,         // output buffer
               pBufferSize          // output buffer size
               );

   return( Status );
}


NTSTATUS
SFTK_TDI_TCPGetIPAddrTable(
   PFILE_OBJECT   pFileObject,
   IPAddrEntry    *pIPAddrTable,
   PULONG         pBufferSize
   )
{
   NTSTATUS Status;

   Status = SFTK_TDI_TCPQueryInformation( 
               pFileObject,               // control object
               CL_NL_ENTITY,              // entity
               TL_INSTANCE,               // instance
               INFO_CLASS_PROTOCOL,       // class
               INFO_TYPE_PROVIDER,        // type
               IP_MIB_ADDRTABLE_ENTRY_ID, // id
               pIPAddrTable,              // output buffer
               pBufferSize                // output buffer size
               );

   return( Status );
}


NTSTATUS
SFTK_TDI_TCPGetIPRouteTable(
   PFILE_OBJECT   pFileObject,
   IPRouteEntry   *pIPRouteEntry,
   PULONG         pBufferSize
   )
{
   NTSTATUS Status;

   Status = SFTK_TDI_TCPQueryInformation( 
               pFileObject,               // control object
               CL_NL_ENTITY,              // entity
               TL_INSTANCE,               // instance
               INFO_CLASS_PROTOCOL,       // class
               INFO_TYPE_PROVIDER,        // type
               IP_MIB_RTTABLE_ENTRY_ID,   // id
               pIPRouteEntry,             // output buffer
               pBufferSize                // output buffer size
               );

   return( Status );
}


NTSTATUS
SFTK_TDI_TCPGetIPInterfaceInfoForAddr(
   PFILE_OBJECT      pFileObject,
   IPAddr            theIPAddr,
   PUCHAR            pQueryBuffer,// IPInterfaceInfo size + MAX_PHYSADDR_SIZE for address bytes
   PULONG            pBufferSize
   )
{
   NTSTATUS Status;
   TCP_REQUEST_QUERY_INFORMATION_EX QueryInfo;  // local query information

   RtlZeroMemory( &QueryInfo, sizeof( TCP_REQUEST_QUERY_INFORMATION_EX ) ); // input buffer length

   QueryInfo.ID.toi_entity.tei_entity = CL_NL_ENTITY;
   QueryInfo.ID.toi_entity.tei_instance = TL_INSTANCE;
   QueryInfo.ID.toi_class = INFO_CLASS_PROTOCOL;
   QueryInfo.ID.toi_type = INFO_TYPE_PROVIDER;
   QueryInfo.ID.toi_id = IP_INTFC_INFO_ID;

   RtlCopyMemory(
      &QueryInfo.Context,
      &theIPAddr,
      sizeof( ULONG )
      );

   Status = SFTK_TDI_TCPQueryDeviceControl(                    // send down device control call
               pFileObject,                              // current transport/connection object
               &QueryInfo,                               // input buffer
               sizeof ( TCP_REQUEST_QUERY_INFORMATION_EX ), // input buffer length
               pQueryBuffer,                             // output buffer
               *pBufferSize,                             // output buffer length
               pBufferSize                               // return information
               );

   return( Status );
}



