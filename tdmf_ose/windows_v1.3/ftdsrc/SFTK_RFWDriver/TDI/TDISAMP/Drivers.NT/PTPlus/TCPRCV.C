/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"

#include "inetinc.h"
#include "KSUtil.h"

#include "addr.h"
#include "tcpconn.h"
#include "tcprcv.h"

// Copyright And Configuration Management ----------------------------------
//
//                 TCP Receive Function Filters - TCPRcv.c
//         Transport Data Interface (TDI) Filter For Windows NT
//
//                Transport Data Interface (TDI) Filter Samples
//                                    For
//                                 Windows NT
//
//     Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------


NDIS_STATUS
TDIH_AllocatePacket(
   PTDIH_PACKET *hTDIH_Packet,
   AddrObj      *pAddrObj,
   TCPConn      *pTCPConn
   )
{
   NDIS_STATUS    nNdisStatus;
   PTDIH_PACKET   pTDIH_Packet;

   *hTDIH_Packet = NULL;

   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   nNdisStatus = NdisAllocateMemory(
                  &pTDIH_Packet,
                  sizeof( TDIH_PACKET ),
                  0,            // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( nNdisStatus != NDIS_STATUS_SUCCESS )
   {
      return( nNdisStatus );
   }

   ASSERT( pTDIH_Packet );
   if( !pTDIH_Packet )
   {
      return( NDIS_STATUS_RESOURCES );
   }

   NdisZeroMemory( pTDIH_Packet, sizeof( TDIH_PACKET ) );

   //
   // Save Associated Address Object And Connection
   //
   pTDIH_Packet->pAddrObj = pAddrObj;
   pTDIH_Packet->pTCPConn = pTCPConn;

   *hTDIH_Packet = pTDIH_Packet;

   return( NDIS_STATUS_SUCCESS );
}

VOID
TDIH_FreePacket( PTDIH_PACKET pTDIH_Packet )
{
   // ATTENTION!!! Free anything else???

   if( pTDIH_Packet->Head )
   {
      NdisFreeBuffer( pTDIH_Packet->Head );
   }

   pTDIH_Packet->Head = NULL;

   NdisFreeMemory( pTDIH_Packet, sizeof( TDIH_PACKET ), 0 );
}


VOID
TDIH_AssignBufferToPacket(
   PTDIH_PACKET pTDIH_Packet,
   PNDIS_BUFFER pNdisBuffer
   )
{
   ASSERT( !pNdisBuffer->Next ); // This ImplementationDoesn't Support

   ASSERT( !pTDIH_Packet->Head );   // Should Be Empty...
   ASSERT( !pTDIH_Packet->Tail );   // Should Be Empty...

   pTDIH_Packet->Head = pNdisBuffer;
   pTDIH_Packet->Tail = pNdisBuffer;

   NdisQueryBuffer(
      pNdisBuffer,
      &pTDIH_Packet->Data,
      &pTDIH_Packet->ContigSize
      );

   pTDIH_Packet->Position = 0;

   pTDIH_Packet->BufferCount = 1;
}


VOID
TDIH_IndicateReceivedPackets( TCPConn *pTCPConn )
{
   PTDIH_PACKET   pTDIH_Packet;
   AddrObj        *pAddrObj;

   KdPrint(( "TDIH_IndicateReceivedPackets: Entry...\n" ));

   ASSERT( pTCPConn );

   pAddrObj = pTCPConn->tc_ao;

   if( !pAddrObj )
   {
      //
      // Handle DisAssociated Connection
      // -------------------------------
      // This condition can occur if a private receive is completed, usually
      // with an unsuccessful status, during the disconnect process.
      //
      return;
   }

   if( IsListEmpty( &pTCPConn->tc_rcv_pkt_q ) )
   {
      KdPrint(( "TDIH_IndicateReceivedPackets: No Data To Indicate!!!\n" ));
      return;  // Nothing To Do!!!
   }

   //
   // Indicate To Client By Satisfying Any Pending TdiReceive
   // -------------------------------------------------------
   // If client has any pending receives, satisfy them in preference to
   // calling the receive event handler (if any).
   //
   if( !IsListEmpty( &pTCPConn->tc_rcv_pkt_q )
      && !IsListEmpty( &pTCPConn->tc_rcv_q )
      )
   {
      while( !IsListEmpty( &pTCPConn->tc_rcv_pkt_q )
         && !IsListEmpty( &pTCPConn->tc_rcv_q )
         )
      {
         PLIST_ENTRY    pLinkage;
         PIRP           Irp = NULL;
         ULONG          BytesCopied = 0;
         ULONG          BytesRemaining;
         PUCHAR         pVA;
         UINT           nVARemaining;
         PNDIS_BUFFER   pNdisBuffer;
         PTDI_REQUEST_KERNEL_RECEIVE   p;
         PIO_STACK_LOCATION            IrpSp = NULL;

         //
         // Fetch The Oldest Pending TdiReceive Request
         // -------------------------------------------
         // These requests were enqueued from the TdiReceive function.
         //
         pLinkage = NdisInterlockedRemoveHeadList(
                        &pTCPConn->tc_rcv_q,
                        &pTCPConn->tc_rcv_q_lock
                        );

         if( pLinkage && pLinkage != &pTCPConn->tc_rcv_q )
         {
            KdPrint(( "TDIH_IndicateReceivedPackets: Processing Queued TdiReceive IRP\n" ));

            Irp = CONTAINING_RECORD(
                        pLinkage,
                        IRP,
                        Tail.Overlay.ListEntry
                        );

            IoSetCancelRoutine( Irp, NULL );

            // Get the current I/O stack location.
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            ASSERT(IrpSp);

            //
            // Locate First TDIH_PACKET
            //
            pTDIH_Packet = (PTDIH_PACKET )pTCPConn->tc_rcv_pkt_q.Flink;
            ASSERT( pTDIH_Packet && (pTDIH_Packet != (PTDIH_PACKET )&pTCPConn->tc_rcv_pkt_q ) );

            //
            // Handle Special Cases
            //
            if( (pTDIH_Packet->IoStatus != STATUS_SUCCESS)
               || !pTDIH_Packet->ContigSize
               )
            {
               //
               // Setup Return Status And Byte Count
               //
               Irp->IoStatus.Status = pTDIH_Packet->IoStatus;
               Irp->IoStatus.Information = 0;

               //
               // Finished With Current TDIH_PACKET
               //
               RemoveEntryList( &pTDIH_Packet->ListElement );

               TDIH_FreePacket( pTDIH_Packet );

               //
               // Call The Client's Completion Routine
               //
               Irp->IoStatus.Status = STATUS_SUCCESS;
               IoCompleteRequest( Irp, IO_NO_INCREMENT );

               break;
            }

            //
            // Locate The Receive Request
            // --------------------------
            // The TDI_REQUEST_KERNEL_RECEIVE structure contains additional
            // information:
            //        p->ReceiveFlags
            //        p->ReceiveLength
            //
            p = (PTDI_REQUEST_KERNEL_RECEIVE )&IrpSp->Parameters;

            BytesRemaining = p->ReceiveLength;

            KdPrint(( "  Bytes Requested/Available: %d/%d\n",
               BytesRemaining, pTCPConn->TotalSize ));

            if( BytesRemaining > pTCPConn->TotalSize )
            {
               BytesRemaining = pTCPConn->TotalSize;
            }

            ASSERT( pTCPConn->TotalSize && pTDIH_Packet->ContigSize );

            //
            // Understand Where The Data Will Go
            // ---------------------------------
            // See the TdiBuildReceive MACRO in the TDIKRNL.H header file.
            //
            // Irp->MdlAddress is the FIRST MDL in a possible chain of MDLS
            // that describe the memory to be filled with received data.
            //
            // Since MDLs are identical to NDIS_BUFFERs on the Windows NT platform,
            // PCAUSA uses functions such as NdisQueryBuffer, etc.
            // to access the MDL fields instead of MmXYZ functions. This is
            // just a preference. However, it provides for (somewhat) greater
            // protability of code fragments between the Windows 9X VxD environment
            // and the Windows NT kernel mode environment.
            //
            // The memory passed to this routine should have already been probed
            // and locked by the caller.
            //

            pNdisBuffer = Irp->MdlAddress;   // First Buffer In Chain

            //
            // Access The NDIS_BUFFER Virtual Memory Address And Length
            //
            NdisQueryBuffer(
               pNdisBuffer,
               NULL,
               &nVARemaining
               );

            if( nVARemaining > BytesRemaining )
            {
               nVARemaining = BytesRemaining;
            }

            pVA = MmGetSystemAddressForMdl( Irp->MdlAddress );

            //
            // Loop To Fill NDIS_BUFFER(s) From TDIH_PACKET(s)
            // -----------------------------------------------
            // Most often there will be one(1) of each involved in the
            // copy. However, the general case must accomodate NDIS_BUFFER
            // chains as well as emptying multiple TDIH_PACKET(s) to
            // satisfy one request.
            //
            while( pTDIH_Packet && pNdisBuffer )
            {
               ULONG          BytesToCopy;

               ASSERT( BytesRemaining );

               //
               // Determine Amount To Copy In This Iteration
               //
               BytesToCopy = BytesRemaining;

               if( BytesToCopy > nVARemaining )
               {
                  BytesToCopy = nVARemaining;
               }

               if( BytesToCopy > pTDIH_Packet->ContigSize )
               {
                  BytesToCopy = pTDIH_Packet->ContigSize;
               }

               //
               // Copy The Data
               //
               NdisMoveMemory(
                  pVA,
                  &pTDIH_Packet->PacketBuffer[ pTDIH_Packet->Position ],
                  BytesToCopy
                  );

               BytesRemaining -= BytesToCopy;
               BytesCopied += BytesToCopy;
               pTCPConn->TotalSize -= BytesToCopy;

               //
               // Adjust Our TDIH_PACKET Chain For Bytes Taken
               //
               if( BytesToCopy < pTDIH_Packet->ContigSize )
               {
                  //
                  // Advance Position In Current TDIH_PACKET
                  //
                  pTDIH_Packet->ContigSize -= BytesToCopy;
                  pTDIH_Packet->Position += BytesToCopy;
                  pTDIH_Packet->Data = &pTDIH_Packet->PacketBuffer[ pTDIH_Packet->Position ];
               }
               else if( BytesToCopy == pTDIH_Packet->ContigSize )
               {
                  //
                  // Finished With Current TDIH_PACKET
                  //
                  RemoveEntryList( &pTDIH_Packet->ListElement );

                  TDIH_FreePacket( pTDIH_Packet );

                  if( !IsListEmpty( &pTCPConn->tc_rcv_pkt_q ) )
                  {
                     //
                     // Locate Next TDIH_PACKET
                     //
                     pTDIH_Packet = (PTDIH_PACKET )pTCPConn->tc_rcv_pkt_q.Flink;
                     ASSERT( pTDIH_Packet && (pTDIH_Packet != (PTDIH_PACKET )&pTCPConn->tc_rcv_pkt_q ) );
                  }
                  else
                  {
                     //
                     // No More TDIH_PACKETs
                     //
                     pTDIH_Packet = NULL;

                     //
                     // Start Another Private Receive On Connection
                     //
                     TDIH_TdiStartPrivateReceive( pTCPConn );
                  }
               }

               //
               // Adjust Client's NDIS_BUFFER Chain For Bytes Taken
               //
               if( BytesToCopy < nVARemaining )
               {
                  //
                  // Advance Position In Current NDIS_BUFFER
                  //
                  pVA += BytesToCopy;
                  nVARemaining -= BytesToCopy;
               }
               else if( BytesToCopy == nVARemaining )
               {
                  //
                  // Finished With Current NDIS_BUFFER
                  //
                  NdisGetNextBuffer( pNdisBuffer, &pNdisBuffer );

                  if( pNdisBuffer )
                  {
                     NdisQueryBuffer(
                        pNdisBuffer,
                        NULL,
                        &nVARemaining
                        );

                     if( nVARemaining > BytesRemaining )
                     {
                        nVARemaining = BytesRemaining;
                     }

                     pVA = MmGetSystemAddressForMdl( Irp->MdlAddress );
                  }
                  else
                  {
                     //
                     // No More NDIS_BUFFERs
                     //
                  }
               }
            }

            Irp->IoStatus.Information = BytesCopied;

            KdPrint(( "  Bytes Available/Copied: %d/%d\n",
               pTCPConn->TotalSize, Irp->IoStatus.Information ));

            //
            // Call The Client's Completion Routine
            //
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
         }
      }

      return;
   }

   //
   // Indicate To Clients ReceiveEvent Handler
   // ----------------------------------------
   // Only do this if there were no TdiReceive request pending.
   //
   if( pTCPConn->TotalSize && (pTCPConn->ao_rcv || pAddrObj->ao_rcv) )
   {
	   NTSTATUS    Status;
	   PIRP        Irp;
      ULONG       BytesTaken = 0;

      //
      // Locate First TDIH_PACKET
      //
      pTDIH_Packet = (PTDIH_PACKET )pTCPConn->tc_rcv_pkt_q.Flink;
      ASSERT( pTDIH_Packet && (pTDIH_Packet != (PTDIH_PACKET )&pTCPConn->tc_rcv_pkt_q ) );

      //
      // Pass To Receive Event Handler Set On The Address Object
      // -------------------------------------------------------
      // Actually we call a copy of the receive event handler pointer that
      // was taken when the address object was associated with the
      // connection object.
      //
      if( pTCPConn->ao_rcv )
      {
	      Status = pTCPConn->ao_rcv(
                     pTCPConn->ao_rcvcontext,
		               pTCPConn->tc_context,
                     (TDI_RECEIVE_NORMAL|TDI_RECEIVE_ENTIRE_MESSAGE),
		               pTDIH_Packet->ContigSize,
		               pTCPConn->TotalSize,
                     &BytesTaken,
		               pTDIH_Packet->Data,  // Tsdu
                     &Irp                 // TdiReceive IRP if STATUS_MORE_PROCESSING_REQUIRED
		               );

	      KdPrint(( "TDIH_IndicateReceivedPackets(1): Status: 0x%8.8X; Taken: %d\n",
            Status, BytesTaken ));
      }
      else
      {
	      Status = pAddrObj->ao_rcv(
                     pAddrObj->ao_rcvcontext,
		               pTCPConn->tc_context,
                     (TDI_RECEIVE_NORMAL|TDI_RECEIVE_ENTIRE_MESSAGE),
		               pTDIH_Packet->ContigSize,
		               pTCPConn->TotalSize,
                     &BytesTaken,
		               pTDIH_Packet->Data,  // Tsdu
                     &Irp                 // TdiReceive IRP if STATUS_MORE_PROCESSING_REQUIRED
		               );

	      KdPrint(( "TDIH_IndicateReceivedPackets(2): Status: 0x%8.8X; Taken: %d\n",
            Status, BytesTaken ));
      }


      if( Status == STATUS_DATA_NOT_ACCEPTED )
      {
         BytesTaken = 0;
      }

      ASSERT( BytesTaken <= pTCPConn->TotalSize );
      if( BytesTaken > pTCPConn->TotalSize )
      {
         BytesTaken = pTCPConn->TotalSize;
      }

      //
      // Adjust For Bytes Taken
      //
      while( BytesTaken && pTDIH_Packet )
      {
         if( BytesTaken < pTDIH_Packet->ContigSize )
         {
            //
            // Advance Position In Current TDIH_PACKET
            //
            pTCPConn->TotalSize -= BytesTaken;
            pTDIH_Packet->ContigSize -= BytesTaken;
            pTDIH_Packet->Position += BytesTaken;
            pTDIH_Packet->Data = &pTDIH_Packet->PacketBuffer[ pTDIH_Packet->Position ];
            BytesTaken = 0;
         }
         else if( BytesTaken >= pTDIH_Packet->ContigSize )
         {
            //
            // Finished With Current TDIH_PACKET
            //
            pTCPConn->TotalSize -= pTDIH_Packet->ContigSize;
            BytesTaken -= pTDIH_Packet->ContigSize;

            RemoveEntryList( &pTDIH_Packet->ListElement );

            TDIH_FreePacket( pTDIH_Packet );

            pTDIH_Packet = NULL;

            if( !IsListEmpty( &pTCPConn->tc_rcv_pkt_q ) )
            {
               //
               // Locate Next TDIH_PACKET
               //
               pTDIH_Packet = (PTDIH_PACKET )pTCPConn->tc_rcv_pkt_q.Flink;
               ASSERT( pTDIH_Packet && (pTDIH_Packet != (PTDIH_PACKET )&pTCPConn->tc_rcv_pkt_q ) );
            }
            else
            {
               //
               // Start Another Private Receive On Connection
               //
               TDIH_TdiStartPrivateReceive( pTCPConn );
            }
         }
      }

//      ASSERT( pTDIH_Packet ); // Queue For Later???

      if( Status == STATUS_MORE_PROCESSING_REQUIRED && Irp )
      {
         ULONG                         BytesCopied = 0;
         ULONG                         BytesRemaining;
         PNDIS_BUFFER                  pNdisBuffer;
         PUCHAR                        pVA;
         UINT                          nVARemaining;
         PTDI_REQUEST_KERNEL_RECEIVE   p;
         PIO_STACK_LOCATION            IrpSp = NULL;

         // Get the current I/O stack location.
         IrpSp = IoGetCurrentIrpStackLocation(Irp);
         ASSERT(IrpSp);

         //
         // Locate The Receive Request
         // --------------------------
         // The TDI_REQUEST_KERNEL_RECEIVE structure contains additional
         // information:
         //        p->ReceiveFlags
         //        p->ReceiveLength
         //
         p = (PTDI_REQUEST_KERNEL_RECEIVE )&IrpSp->Parameters;

         BytesRemaining = p->ReceiveLength;

         KdPrint(( "  Bytes Requested/Available: %d/%d\n",
            BytesRemaining, pTCPConn->TotalSize ));

         if( BytesRemaining > pTCPConn->TotalSize )
         {
            BytesRemaining = pTCPConn->TotalSize;
         }

         //
         // Understand Where The Data Will Go
         // ---------------------------------
         // See the TdiBuildReceive MACRO in the TDIKRNL.H header file.
         //
         // Irp->MdlAddress is the FIRST MDL in a possible chain of MDLS
         // that describe the memory to be filled with received data.
         //
         // Since MDLs are identical to NDIS_BUFFERs on the Windows NT platform,
         // PCAUSA will probably use functions such as NdisQueryBuffer, etc.
         // to access the MDL fields instead of MmXYZ functions. This is
         // just a preference. However, it provides for (somewhat) greater
         // protability of code fragments between the Windows 9X VxD environment
         // and the Windows NT kernel mode environment.
         //
         // The memory passed to this routine should have already been probed
         // and locked by the caller.
         //

         pNdisBuffer = Irp->MdlAddress;   // First Buffer In Chain

         //
         // Access The NDIS_BUFFER Virtual Memory Address And Length
         //
         NdisQueryBuffer(
            pNdisBuffer,
            &pVA,
            &nVARemaining
            );

         if( nVARemaining > BytesRemaining )
         {
            nVARemaining = BytesRemaining;
         }

         //
         // Loop To Fill NDIS_BUFFER(s) From TDIH_PACKET(s)
         // -----------------------------------------------
         // Most often there will be one(1) of each involved in the
         // copy. However, the general case must accomodate NDIS_BUFFER
         // chains as well as emptying multiple TDIH_PACKET(s) to
         // satisfy one request.
         //
         while( BytesRemaining && pTDIH_Packet && pNdisBuffer )
         {
            ULONG          BytesToCopy;

            //
            // Determine Amount To Copy In This Iteration
            //
            BytesToCopy = BytesRemaining;

            if( BytesToCopy > nVARemaining )
            {
               BytesToCopy = nVARemaining;
            }

            if( BytesToCopy > pTDIH_Packet->ContigSize )
            {
               BytesToCopy = pTDIH_Packet->ContigSize;
            }

            //
            // Copy The Data
            //
            NdisMoveMemory(
               pVA,
               &pTDIH_Packet->PacketBuffer[ pTDIH_Packet->Position ],
               BytesToCopy
               );

            BytesRemaining -= BytesToCopy;
            BytesCopied += BytesToCopy;
            pTCPConn->TotalSize -= BytesToCopy;

            //
            // Adjust Our TDIH_PACKET Chain For Bytes Taken
            //
            if( BytesToCopy < pTDIH_Packet->ContigSize )
            {
               //
               // Advance Position In Current TDIH_PACKET
               //
               pTDIH_Packet->ContigSize -= BytesToCopy;
               pTDIH_Packet->Position += BytesToCopy;
               pTDIH_Packet->Data = &pTDIH_Packet->PacketBuffer[ pTDIH_Packet->Position ];
            }
            else if( BytesToCopy == pTDIH_Packet->ContigSize )
            {
               //
               // Finished With Current TDIH_PACKET
               //
               RemoveEntryList( &pTDIH_Packet->ListElement );

               TDIH_FreePacket( pTDIH_Packet );

               if( !IsListEmpty( &pTCPConn->tc_rcv_pkt_q ) )
               {
                  //
                  // Locate Next TDIH_PACKET
                  //
                  pTDIH_Packet = (PTDIH_PACKET )pTCPConn->tc_rcv_pkt_q.Flink;
                  ASSERT( pTDIH_Packet && (pTDIH_Packet != (PTDIH_PACKET )&pTCPConn->tc_rcv_pkt_q ) );
               }
               else
               {
                  //
                  // No More TDIH_PACKETs
                  //
                  pTDIH_Packet = NULL;

                  //
                  // Start Another Private Receive On Connection
                  //
                  TDIH_TdiStartPrivateReceive( pTCPConn );
               }
            }

            //
            // Adjust Client's NDIS_BUFFER Chain For Bytes Taken
            //
            if( BytesToCopy < nVARemaining )
            {
               //
               // Advance Position In Current NDIS_BUFFER
               //
               pVA += BytesToCopy;
               nVARemaining -= BytesToCopy;
            }
            else if( BytesToCopy == nVARemaining )
            {
               //
               // Finished With Current NDIS_BUFFER
               //
               NdisGetNextBuffer( pNdisBuffer, &pNdisBuffer );

               if( pNdisBuffer )
               {
                  NdisQueryBuffer(
                     pNdisBuffer,
                     &pVA,
                     &nVARemaining
                     );

                  if( nVARemaining > BytesRemaining )
                  {
                     nVARemaining = BytesRemaining;
                  }
               }
               else
               {
                  //
                  // No More NDIS_BUFFERs
                  //
               }
            }
         }

         Irp->IoStatus.Information = BytesCopied;

         KdPrint(( "  Bytes Available/Copied: %d/%d\n",
            pTCPConn->TotalSize, Irp->IoStatus.Information ));

         //
         // Call The Higher Level Protocol
         // ------------------------------
         // Setup the flags field, then call the callback.
         //
#ifdef ZNEVER
         ASSERT( EvBuffer.erb_flags );

         if( EvBuffer.erb_flags )
         {
            *EvBuffer.erb_flags = (TDI_RECEIVE_NORMAL|TDI_RECEIVE_ENTIRE_MESSAGE);
         }

         ASSERT( EvBuffer.erb_rtn );
#endif // ZNEVER

         //
         // Call The Client's Completion Routine
         //
         IoCompleteRequest( Irp, IO_NO_INCREMENT );
      }

      return;
   }

   KdPrint(( "TDIH_IndicateReceivedPackets: No Way To Indicate!!!\n" ));
}

VOID
TDIH_ReceiveTimerProc(
   PVOID SystemSpecific1,
   PVOID FunctionContext,
   PVOID SystemSpecific2,
   PVOID SystemSpecific3
   )
{
   TCPConn     *pTCPConn = (TCPConn * )FunctionContext;

   KdPrint(( "TDIH_ReceiveTimerProc: Entry...\n" ));

   //
   // Call Common Routine To Attempt To Indicate Packets Upwards
   //
   TDIH_IndicateReceivedPackets( pTCPConn );
}


NTSTATUS
TDIH_TdiPrivateReceiveComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   ULONG                   nByteCount = Irp->IoStatus.Information;
   PTDIH_PACKET            pTDIH_Packet;
   AddrObj                 *pAddrObj;
   TCPConn                 *pTCPConn;

#if DBG
   KdPrint(( "TDIH_TdiPrivateReceiveComplete: Final Status: 0x%8.8X; Bytes Transfered: %d\n",
      Status, nByteCount ));
#endif

   pTDIH_Packet = (PTDIH_PACKET )Context;

   ASSERT( pTDIH_Packet );
   if( !pTDIH_Packet )
   {
      IoFreeIrp( Irp );    // It's Ours. Free it up

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   pAddrObj = pTDIH_Packet->pAddrObj;

   ASSERT( pAddrObj );
   if( !pAddrObj )
   {
      TDIH_FreePacket( pTDIH_Packet );

      IoFreeIrp( Irp );    // It's Ours. Free it up

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   pTCPConn = pTDIH_Packet->pTCPConn;
   ASSERT( pTCPConn );

   if( !pTCPConn )
   {
      TDIH_FreePacket( pTDIH_Packet );

      IoFreeIrp( Irp );    // It's Ours. Free it up

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   pTDIH_DeviceExtension = pAddrObj->ao_DeviceExtension;

   ASSERT( pTDIH_DeviceExtension );
   if( !pTDIH_DeviceExtension )
   {
      TDIH_FreePacket( pTDIH_Packet );

      IoFreeIrp( Irp );    // It's Ours. Free it up

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Save I/O Status
   //
   pTDIH_Packet->IoStatus = Status;

   //
   // Adjust Packet Size
   //
   pTDIH_Packet->ContigSize = nByteCount;

   //
   // PROCESS RECEIVED DATA HERE (2)
   // ------------------------------
   // Process data that is indicated when TDIH_TdiReceiveEventHandler
   // fill out an EventRcvBuffer to be filled in.
   //
   // Data is at pTDIH_Packet->PacketBuffer.
   //

   //
   // Add Packet To Connections Received Packet List
   //
   InsertTailList(
      &pTCPConn->tc_rcv_pkt_q,
      &pTDIH_Packet->ListElement
      );

   pTCPConn->TotalSize += pTDIH_Packet->ContigSize;

   //
   // Call Common Routine To Attempt To Indicate Packets Upwards
   // ----------------------------------------------------------
   // Don't call TDIH_IndicateReceivedPackets directly. Instead, 
   // use an NDIS timer. When the NDIS timer expires it will call 
   // TDIH_ReceiveTimerProc which simply calls TDIH_IndicateReceivedPackets
   // to attempt attempt to satisfy the TdiReceive request later.
   //
   // Using a timeout value of zero(0) causes the TDIH_ReceiveTimerProc
   // callback to be called "later", but as soon as possible.
   //
   NdisSetTimer( &pTCPConn->tc_rx_timer, 0 );

ROE_Exit:
   UTIL_DecrementLargeInteger(
      pTDIH_DeviceExtension->OutstandingIoRequests,
      (unsigned long)1,
      &(pTDIH_DeviceExtension->IoRequestsSpinLock)
      );

   // If the outstanding count is 0, signal the appropriate event which will
   // allow any pending detach to proceed.
   UTIL_IsLargeIntegerZero(
      CanDetachProceed,
      pTDIH_DeviceExtension->OutstandingIoRequests,
      &(pTDIH_DeviceExtension->IoRequestsSpinLock)
      );

   if (CanDetachProceed)
   {
      // signal the event object. Note that this is simply an
      // advisory check we do here (to wake up a sleeping thread).
      // It is the responsibility of the thread performing the detach to
      // ensure that no operations are truly in progress.
      KeSetEvent(&(pTDIH_DeviceExtension->IoInProgressEvent), IO_NO_INCREMENT, FALSE);
   }

   // Although the success return value is hard-coded here, you can
   // return an appropriate value (either success or more-processing-reqd)
   // based upon what it is that you wish to do in your completion routine.

   IoFreeIrp( Irp );    // It's Ours. Free it up
   return( STATUS_MORE_PROCESSING_REQUIRED );
}


NTSTATUS
TDIH_TdiStartPrivateReceive(
   TCPConn *pTCPConn
   )
{
	NTSTATUS                Status;
	AddrObj                 *pAddrObj;
   PIO_STACK_LOCATION      IrpSp = NULL;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   PTDIH_PACKET            pTDIH_Packet;
   NDIS_STATUS             nNdisStatus;
   PNDIS_BUFFER            pNdisBuffer;
   PIRP                    pReceiveIrp;

   KdPrint(("TDIH_TdiStartPrivateReceive: Entry...\n") );

   if( !pTCPConn )
   {
      return( STATUS_DATA_NOT_ACCEPTED );
   }

   pAddrObj = pTCPConn->tc_ao;
   ASSERT( pAddrObj );

   if( !pAddrObj )
   {
      return( STATUS_DATA_NOT_ACCEPTED );
   }

   pTDIH_DeviceExtension = pAddrObj->ao_DeviceExtension;

   //
   // Allocate A TDIH_PACKET
   //
   nNdisStatus = TDIH_AllocatePacket( &pTDIH_Packet, pAddrObj, pTCPConn );
   ASSERT( (nNdisStatus == NDIS_STATUS_SUCCESS) && pTDIH_Packet );

   if( !pTDIH_Packet || (nNdisStatus != NDIS_STATUS_SUCCESS) )
   {
      return( STATUS_DATA_NOT_ACCEPTED );
   }

   //
   // Allocate A NDIS_BUFFER
   // ----------------------
   // Allocate size for all available data.
   //
   NdisAllocateBuffer(
      &nNdisStatus,
      &pNdisBuffer,
      g_hTDIH_BufferPool,
      pTDIH_Packet->PacketBuffer,
      TDIH_PACKET_BUF_SIZE
      );

   ASSERT( (nNdisStatus == NDIS_STATUS_SUCCESS) && pNdisBuffer );
   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      TDIH_FreePacket( pTDIH_Packet );

      return( STATUS_DATA_NOT_ACCEPTED );
   }

   //
   // Assign NDIS_BUFFER Buffer To TDIH_PACKET
   //
   TDIH_AssignBufferToPacket( pTDIH_Packet, pNdisBuffer );

   //
   // Ask Transport To Provide All Available Data
   // -------------------------------------------
   // If we get to this point, then Available > Indicated. To fetch
   // all available information we setup the EventRcvBuffer and return
   // status of TDI_MORE_PROCESSING. When the transport satisfies this
   // request the TDIH_ReceiveOnEventComplete callback will be called.
   //
   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   pReceiveIrp = IoAllocateIrp(
                     (pTDIH_DeviceExtension->LowerDeviceObject)->StackSize,
                     FALSE
                     );

   ASSERT( pReceiveIrp );

   if( !pReceiveIrp )
   {
      TDIH_FreePacket( pTDIH_Packet );

      return( STATUS_DATA_NOT_ACCEPTED );
   }

   ASSERT( pTDIH_Packet->ContigSize == TDIH_PACKET_BUF_SIZE );

   TdiBuildReceive(
      pReceiveIrp,            // IRP
      pTDIH_DeviceExtension->LowerDeviceObject,
      pTCPConn->tc_FileObject,
      TDIH_TdiPrivateReceiveComplete,     // CompletionRoutine
      pTDIH_Packet,                       // Context
      pNdisBuffer,                        // MdlAddress
      0,                                  // ReceiveFlags
      pTDIH_Packet->ContigSize            // ReceiveLength
      );

   UTIL_IncrementLargeInteger(
      pTDIH_DeviceExtension->OutstandingIoRequests,
      (unsigned long)1,
      &(pTDIH_DeviceExtension->IoRequestsSpinLock)
      );

   Status = IoCallDriver(
               pTDIH_DeviceExtension->LowerDeviceObject,
               pReceiveIrp
               );

   KdPrint(( "StartPrivateReceive IoCallDriver Status: 0x%8.8X\n", Status ));

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiChainedReceiveEventHandler
//
// Purpose
// This is the hook for TDI_EVENT_CHAINED_RECIEVE event.
//
// Parameters
// See NTDDK documentation for TDI_EVENT_CHAINED_RECIEVE.
//
// Return Value
// See NTDDK documentation for TDI_EVENT_CHAINED_RECIEVE.
//
// Remarks
// This hook is called by the TCP device to indicate a packet to a
// TDI client who has set a TDI_EVENT_CHAINED_RECEIVE event handler on the
// Address Object.
//
// It is defined in the NT DDK. However, its first actual use seems to
// be in Windows 2000.
// 
// The receive event handling path should be as efficient and quick as
// possible. If too much time is spent in the receive event handler,
// packets can be lost. This is especially true of the receive DG event
// handler, where even verbose debug print output can cause packets
// to be lost.
//

NTSTATUS
TDIH_TdiChainedReceiveEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG ReceiveLength,        // length of client data in TSDU
   ULONG StartingOffset,       // offset of start of client data in TSDU
   PMDL  Tsdu,                 // TSDU data chain
   PVOID TsduDescriptor        // for call to TdiReturnChainedReceives
   )
{
	NTSTATUS    Status;
	AddrObj     *pAddrObj;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   TCPConn     *pTCPConn;

   KdPrint(("TDIH_TdiChainedReceiveEventHandler: Entry...\n") );

   KdPrint(("  ReceiveLength: %d; StartingOffset: %d; Flags: 0x%8.8x\n",
      ReceiveLength, StartingOffset, ReceiveFlags));

   //
   // EventContext Points To Our Address Object Record
   //
   ASSERT( TdiEventContext );
	pAddrObj = (AddrObj * )TdiEventContext;

   if( !pAddrObj )
   {
	   KdPrint(( "TDIH_TdiChainedReceiveEventHandler: Caller Event Handler NULL!!!\n" ));

      return( STATUS_DATA_NOT_ACCEPTED );
   }

   //
   // Map Caller's ConnectionContext To Our TCP Connection Record
   //
   ASSERT( ConnectionContext );
	pTCPConn = TDIH_GetConnFromConnectionContext( ConnectionContext );

   if( !pTCPConn )
   {
      KdPrint(("TDIH_TdiChainedReceiveEventHandler: TCPConn Record Not Found!!!\n") );
   }

   //
   // Sanity Check On Caller's Receive Event Handler
   //
   if( !pAddrObj->ao_chainedrcv )
   {
	   KdPrint(( "TDIH_TdiChainedReceiveEventHandler: Caller Event Handler NULL!!!\n" ));

      return( STATUS_DATA_NOT_ACCEPTED );
   }

   pTDIH_DeviceExtension = pAddrObj->ao_DeviceExtension;

   //
   // Pass To Receive Event Handler Set On The Address Object
   //
	Status = pAddrObj->ao_chainedrcv(
               pAddrObj->ao_chainedrcvcontext,
		         ConnectionContext,
               ReceiveFlags,
               ReceiveLength,    // length of client data in TSDU
               StartingOffset,   // offset of start of client data in TSDU
               Tsdu,             // TSDU data chain
               TsduDescriptor    // for call to TdiReturnChainedReceives
		         );

	KdPrint(( "TDIH_TdiChainedReceiveEventHandler: Status: 0x%8.8X\n",
      Status ));

   return( Status );
}


VOID
TDIH_CancelTdiReceive( PDEVICE_OBJECT pDeviceObject, PIRP pIrp )
{
#if DBG
   DbgPrint( "TDIH_CancelTdiReceive: Entry...\n" );
#endif

   // ATTENTION!!! Add code to do the cancel!!!
}

/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiReceive
//
// Purpose
// This is the hook for TdiReceive
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiReceive(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   TCPConn                       *pTCPConn = (TCPConn * )NULL;

   KdPrint(( "TDIH_TdiReceive: Entry...\n" ));

   //
   // Locate TCP Connection Object
   //
   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );
   ASSERT( pTCPConn );

   if( !pTCPConn )
	{
      // ATTENTION!!! Must deal with this fatal error...
      return( STATUS_INVALID_CONNECTION );
   }

   IoSetCancelRoutine( Irp, TDIH_CancelTdiReceive );

   IoMarkIrpPending( Irp );
   Irp->IoStatus.Status = STATUS_PENDING;

   //
   // Queue The IRP In The Connection's Pending TdiReceive List
   //
   NdisInterlockedInsertTailList(
      &pTCPConn->tc_rcv_q,
      &Irp->Tail.Overlay.ListEntry,
      &pTCPConn->tc_rcv_q_lock
      );

   //
   // Use An NDIS Timer To Indicate Packets To Higher Level Client
   // ------------------------------------------------------------
   // It would seem logical that in some cases the TdiReceive operation
   // could be completed with success directly from this function. It
   // data was immediately available, why not.
   //
   // However, it was found that some higher level clients simply could
   // not cope with immediate completion of a TdiReceive.
   //
   // Here we use an NDIS timer to force a delay so we can return
   // TDI_PENDING from here. When the NDIS timer expires it will call 
   // TDIH_ReceiveTimerProc which simply calls TDIH_IndicateReceivedPackets
   // to attempt attempt to satisfy the TdiReceive request later.
   //
   // Using a timeout value of zero(0) causes the TDIH_ReceiveTimerProc
   // callback to be called "later", but as soon as possible.
   //
   NdisSetTimer( &pTCPConn->tc_rx_timer, 0 );

   return( STATUS_PENDING );
}



