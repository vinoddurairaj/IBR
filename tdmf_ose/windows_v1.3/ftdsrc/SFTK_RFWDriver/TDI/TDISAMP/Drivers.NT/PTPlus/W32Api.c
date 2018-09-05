/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"
#include "TDIHApi.h" // In ..\..\Include directory
#include "KSUtil.h"
#include "addr.h"

// Copyright And Configuration Management ----------------------------------
//
//               TDI Filter Driver Win32 API Dispatch - W32Api.c
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


/////////////////////////////////////////////////////////////////////////////
//                      W I N 3 2  A P I  D E V I C E                      //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// W32API_InitDeviceExtension
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
W32API_InitDeviceExtension(
   PTDIH_DeviceExtension pTDIH_DeviceExtension
   )
{
   NdisZeroMemory( pTDIH_DeviceExtension, sizeof( TDIH_DeviceExtension ) );

   pTDIH_DeviceExtension->NodeIdentifier.NodeType = TDIH_NODE_TYPE_W32API_DEVICE;
   pTDIH_DeviceExtension->NodeIdentifier.NodeSize = sizeof( TDIH_DeviceExtension );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// W32API_Initialize
//
// Purpose
// Initialize the "Win32 API" device.
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
W32API_Initialize(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
{
   NTSTATUS                status;
   UNICODE_STRING          uniNtNameString;
   UNICODE_STRING          uniWin32NameString;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   PDEVICE_OBJECT          pFilterDeviceObject = NULL;
   PDEVICE_OBJECT          pTargetDeviceObject = NULL;
   PFILE_OBJECT            pTargetFileObject = NULL;
   PDEVICE_OBJECT          pLowerDeviceObject = NULL;

   KdPrint(("PCATDIH: W32API_Initialize Entry...\n"));

   //
   // Create counted string version of our Win32 API device name.
   //
   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   RtlInitUnicodeString( &uniNtNameString, TDIH_W32API_DEVICE_NAME );

   //
   // Create The Win32 API Device Object
   // ----------------------------------
   // IoCreateDevice zeroes the memory occupied by the object.
   //
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
   status = IoCreateDevice(
               DriverObject,
               sizeof( TDIH_DeviceExtension ),
               &uniNtNameString,
               FILE_DEVICE_UNKNOWN,
               0,                     // No standard device characteristics
               FALSE,                 // This isn't an exclusive device
               &pFilterDeviceObject
               );

   if( !NT_SUCCESS(status) )
   {
      KdPrint(("PCATDIH: Couldn't create the W32API Device Object\n"));

      return( status );
   }

   //
   // Initialize The Extension For The Win32 API Device Object
   //
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )( pFilterDeviceObject->DeviceExtension );

   W32API_InitDeviceExtension( pTDIH_DeviceExtension );

   //
   // Create counted string version of our Win32 device name.
   //
   RtlInitUnicodeString( &uniWin32NameString, TDIH_W32API_DOS_DEVICE_NAME );

   //
   // Create a link from our device name to a name in the Win32 namespace.
   //
   status = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );

   if (!NT_SUCCESS(status))
   {
      KdPrint( ("PCATDIH: Couldn't create the PCATDIH symbolic link\n") );

      IoDeleteDevice( DriverObject->DeviceObject );
   }
   else
   {
      KdPrint( ("PCATDIH: Win32 API Device Initialized!\n") );
   }

   return status;
}


/////////////////////////////////////////////////////////////////////////////
//// W32API_TestOpenAddress
//
// Purpose
// Test W32API DeviceIoControl mechanism and test handling of calls
// to TDI that are initiated from within the TDI Filter driver itself.
//
// Parameters
//
// Return Value
// 
// Remarks
// This function can be invoked by running the TDIHTest.EXE Win32 console
// application.
//

NTSTATUS
W32API_TestOpenAddress()
{
   NTSTATUS          Status = STATUS_SUCCESS;
   KS_ADDRESS        m_KS_Address;
   TA_IP_ADDRESS     m_LocalAddress;   // TDI Address
   AddrObjExInfo  m_AddrObjExInfo;

   //
   // Setup Local TDI Address
   //
   KS_InitIPAddress(
      &m_LocalAddress,
      INADDR_ANY,    // Any Local Address
      0              // Any Local Port
      );

   //
   // Special Handling For Address Objects Created In The TDI Filter
   // --------------------------------------------------------------
   // Understand that if you initiate TDI calls from within a filter
   // driver, your calls will be handled by your filter. In many cases
   // this may lead to undesirable results. Instead, you may want to
   // filter (or more specifically, NOT filter) your own TDI operations.
   //
   // The following mechanism provides a means to specially identify your
   // own call to KS_OpenTransportAddress when it is subsequently handled
   // in the TDIH_TdiOpenAddressComplete function (which see, in Addr.c).
   //
   // The AddrObjExInfo structure, build below and placed in a list, can
   // be recognized in the TDIH_TdiOpenAddressComplete function. In that
   // function information in the aox_data1 and aox_data2 fields is saved
   // in the AddrObj when it is created. In addition, the aox_valid field
   // is set to TRUE.
   //
   // Understand that most TDI operations of interest directly or indirectly
   // reference an AddrObj. For example, a TCPConn structure has a pointer
   // to the AddrObj that it is associated with. So, simply identifying
   // the AddrObj as "special (e.g., aox_valid == TRUE) is enough to
   // allow all filter functions to know that the address object and it
   // associated connection objects must be handled some special way.
   // The additional aox_data1 and aox_data2 fields (or more of your own
   // invention) can be used to control special processing that you may
   // need to do.
   //
   // In addition, the aox_ao field in your AddrObjExInfo will be filled
   // in with a pointer to the AddrObj structure that was created by
   // TDIH_TdiOpenAddressComplete.
   //
   // The PassThru sample driver doesn't actually use this information.
   // However, it was felt that providing a mechanism to accomodate this
   // situation would be beneficial.
   //
   m_AddrObjExInfo.aox_thread = PsGetCurrentThread();
   m_AddrObjExInfo.aox_data1 = 0x12345678L;
   m_AddrObjExInfo.aox_data2 = NULL;

   InsertTailList(
      &AddrObjExInfoList,
      &m_AddrObjExInfo.aox_q
      );

   //
   // Open Transport Address
   //
   Status = KS_OpenTransportAddress(
                  TCP_DEVICE_NAME_W,
                  (PTRANSPORT_ADDRESS )&m_LocalAddress,
                  &m_KS_Address
                  );

   //
   // Remove The AddrObjExInfo Structure
   //
   RemoveEntryList( &m_AddrObjExInfo.aox_q );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Address Object Could Not Be Created
      //
      KdPrint(("PCATDIH: W32API_TestOpenAddress could not open address\n"));

      return( Status );
   }

   KdPrint(("PCATDIH: W32API_TestOpenAddress opened address\n"));

   KS_CloseTransportAddress( &m_KS_Address );

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// W32API_Dispatch
//
// Purpose
// The "Win32 API Device" function dispatcher.
//
// Parameters
//
// Return Value
// 
// Remarks
// The "Win32 API Device" is a skeleton of a "traditional" NT device
// handler. It is intended that it be used to support an interface to
// Win32 applications or other NT devices. It is the device that could
// be opened using CreateFile from a Win32 application. The code would
// look something like this:
// 
//   hTest = CreateFile(
//             "\\\\.\\PCATDIH",
//             GENERIC_READ | GENERIC_WRITE,
//             0,
//             NULL,
//             OPEN_EXISTING,
//             FILE_ATTRIBUTE_NORMAL,
//             NULL
//             );
//
// This function is called from the TDIH_DefaultDispatch first-level
// dispatcher in PCATDIH.c.
//

NTSTATUS
W32API_Dispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PIO_STACK_LOCATION  irpStack;
   PVOID               ioBuffer;
   ULONG               inputBufferLength;
   ULONG               outputBufferLength;
   ULONG               ioControlCode;
   NTSTATUS            Status = STATUS_SUCCESS;

   //
   // Init to default settings- 
   //
   Irp->IoStatus.Status      = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   //
   // Get a pointer to the current location in the Irp. This is where
   //     the function codes and parameters are located.
   //
   irpStack = IoGetCurrentIrpStackLocation(Irp);

   //
   // Get the pointer to the input/output buffer and it's length
   //
   ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
   inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
   outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

   switch (irpStack->MajorFunction)
   {
      case IRP_MJ_CREATE:
         KdPrint(("PCATDIH: W32API_Dispatch Create\n"));
         break;

      case IRP_MJ_CLOSE:
         KdPrint(("PCATDIH: W32API_Dispatch Close\n"));
         break;

      case IRP_MJ_CLEANUP:
         KdPrint(("PCATDIH: W32API_Dispatch Cleanup\n"));
         break;

      case IRP_MJ_SHUTDOWN:
         KdPrint(("PCATDIH: W32API_Dispatch Shutdown\n"));
         break;

      case IRP_MJ_DEVICE_CONTROL:
         //
         // get control code from stack and perform the operation
         //
         ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
         switch (ioControlCode)
         {
            // This is where you would add your IOCTL handlers          
            case IOCTL_W32API_TEST:
               KdPrint(("PCATDIH: W32API_Dispatch TEST IOCTL\n"));
               Status = W32API_TestOpenAddress();
               break;

            default:
               KdPrint(("PCATDIH: W32API_Dispatch IOCTL: 0x%8.8X\n",
                  ioControlCode ));
               Status = STATUS_INVALID_PARAMETER;
               break;

         }
         break;

      default:
         KdPrint(("PCATDIH: W32API_Dispatch MJ FCN: 0x%8.8X\n",
            irpStack->MajorFunction ));
         Status = STATUS_UNSUCCESSFUL;
         break;
   }

   //
   // all requests complete synchronously; notify caller of status
   //
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = outputBufferLength;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return Status;
}


#ifdef DBG

/////////////////////////////////////////////////////////////////////////////
//// W32API_Unload
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
W32API_Unload(
   PDEVICE_OBJECT pDeviceObject
   )
{
   PTDIH_DeviceExtension pTDIH_DeviceExtension;
   BOOLEAN		NoRequestsOutstanding = FALSE;
   UNICODE_STRING uniWin32NameString;

   KdPrint( ("PCATDIH: W32API_Unload Entry...\n") );

   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )pDeviceObject->DeviceExtension;

   ASSERT( pTDIH_DeviceExtension );

   try
   {
      //
      // Create counted string version of our Win32 API device name.
      //
      ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
      RtlInitUnicodeString( &uniWin32NameString, TDIH_W32API_DOS_DEVICE_NAME );

      //
      // Delete the link from our device name to a name in the Win32 namespace.
      //
      IoDeleteSymbolicLink( &uniWin32NameString );

		// Delete our device object. But first, take care of the device extension.
	   pTDIH_DeviceExtension->NodeIdentifier.NodeType = 0;
	   pTDIH_DeviceExtension->NodeIdentifier.NodeSize = 0;

		// Note that on 4.0 and later systems, this will result in a recursive fast detach.
		IoDeleteDevice( pDeviceObject );

      KdPrint(("PCATDIH: W32API_Unload Finished\n"));
	}
   except (EXCEPTION_EXECUTE_HANDLER)
   {
		// Eat it up.
		;
	}

	return;
}

#endif // DBG



