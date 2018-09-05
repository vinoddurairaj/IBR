// File Object Info driver - peek protected kernel memory to get some
// information about file objects
//

#include "TDMFANALYZER.h"
#include "stdlib.h"

// Definition for ObQueryNameString call
//
NTSYSAPI
NTSTATUS
NTAPI ObQueryNameString(/*POBJECT*/PVOID, PUNICODE_STRING, ULONG, PULONG);


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:
    This routine is the entry point for the driver.  It is responsible
    for setting the dispatch entry points in the driver object and creating
    the device object.  Any resources such as ports, interrupts and DMA
    channels used must be reported.  A symbolic link must be created between
    the device name and an entry in \DosDevices in order to allow Win32
    applications to open the device.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    STATUS_SUCCESS if the driver initialized correctly, otherwise an error
    indicating the reason for failure.

--*/

{
    PLOCAL_DEVICE_INFO pLocalInfo;  // Device extension:
                                    //      local information for each device.
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    // Initialize the driver object dispatch table.
    // NT sends requests to these routines.

    DriverObject->MajorFunction[IRP_MJ_CREATE]          = GpdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = GpdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = GpdDispatch;
    DriverObject->DriverUnload                          = GpdUnload;

    // Create our device.
    Status = GpdCreateDevice(
                    GPD_DEVICE_NAME,
                    GPD_TYPE,
                    DriverObject,
                    &DeviceObject
                    );

    if ( NT_SUCCESS(Status) )
    {
        // Initialize the local driver info for each device object.
        pLocalInfo = (PLOCAL_DEVICE_INFO)DeviceObject->DeviceExtension;

        pLocalInfo->DeviceObject    = DeviceObject;
        pLocalInfo->DeviceType      = GPD_TYPE;
    }
    return Status;
}

NTSTATUS
GpdCreateDevice(
    IN   PWSTR              PrototypeName,
    IN   DEVICE_TYPE        DeviceType,
    IN   PDRIVER_OBJECT     DriverObject,
    OUT  PDEVICE_OBJECT     *ppDevObj
    )

/*++

Routine Description:
    This routine creates the device object and the symbolic link in
    \DosDevices.
    A symbolic link must be created between the device name and an entry
    in \DosDevices in order to allow Win32 applications to open the device.

Arguments:

    PrototypeName - Name base, # WOULD be appended to this.
    DeviceType - Type of device to create
    DriverObject - Pointer to driver object created by the system.
    ppDevObj - Pointer to place to store pointer to created device object

Return Value:

    STATUS_SUCCESS if the device and link are created correctly, otherwise
    an error indicating the reason for failure.

--*/


{
    NTSTATUS Status;                        // Status of utility calls
    UNICODE_STRING NtDeviceName;
    UNICODE_STRING Win32DeviceName;


    // Get UNICODE name for device.

    RtlInitUnicodeString(&NtDeviceName, PrototypeName);

    Status = IoCreateDevice(                             // Create it.
                    DriverObject,
                    sizeof(LOCAL_DEVICE_INFO),
                    &NtDeviceName,
                    DeviceType,
                    0,
                    FALSE,                      // Not Exclusive
                    ppDevObj
                    );

    if (!NT_SUCCESS(Status))
        return Status;             // Give up if create failed.

    // Clear local device info memory
    RtlZeroMemory((*ppDevObj)->DeviceExtension, sizeof(LOCAL_DEVICE_INFO));

    // Create links
    RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);

    Status = IoCreateSymbolicLink( &Win32DeviceName, &NtDeviceName );

    if (!NT_SUCCESS(Status))    // If we we couldn't create the link then
    {                           //  abort installation.
        IoDeleteDevice(*ppDevObj);
    }

    return Status;
}


NTSTATUS
GpdDispatch(
    IN    PDEVICE_OBJECT pDO,
    IN    PIRP pIrp
    )

/*++

Routine Description:
    This routine is the dispatch handler for the driver.  It is responsible
    for processing the IRPs.

Arguments:

    pDO - Pointer to device object.

    pIrp - Pointer to the current IRP.

Return Value:

    STATUS_SUCCESS if the IRP was processed successfully, otherwise an error
    indicating the reason for failure.

--*/

{
    PLOCAL_DEVICE_INFO pLDI;
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;

    //  Initialize the irp info field.
    //      This is used to return the number of bytes transfered.

    pIrp->IoStatus.Information = 0;
    pLDI = (PLOCAL_DEVICE_INFO)pDO->DeviceExtension;    // Get local info struct

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    //  Set default return status
    Status = STATUS_NOT_IMPLEMENTED;

    // Dispatch based on major fcn code.

    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            // We don't need any special processing on open/close so we'll
            // just return success.
            Status = STATUS_SUCCESS;
            break;

        case IRP_MJ_DEVICE_CONTROL:
            //  Dispatch on IOCTL
            switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
            {
            case IOCTL_FOI_RESOLVE:
                Status = GpdIoctlResolve(pLDI, pIrp, pIrpStack,
                            pIrpStack->Parameters.DeviceIoControl.IoControlCode
                            );
                break;
            }
            break;
    }

    // We're done with I/O request.  Record the status of the I/O action.
    pIrp->IoStatus.Status = Status;

    // Don't boost priority when returning since this took little time.
    IoCompleteRequest(pIrp, IO_NO_INCREMENT );

    return Status;
}


NTSTATUS
GpdIoctlResolve(
    IN PLOCAL_DEVICE_INFO pLDI,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpStack,
    IN ULONG IoctlCode  )


/*++

Routine Description:
    This routine processes the IOCTLs which read from the ports.

Arguments:

    pLDI        - our local device data
    pIrp        - IO request packet
    IrpStack    - The current stack location
    IoctlCode   - The ioctl code from the IRP

Return Value:
    STATUS_SUCCESS           -- OK

    STATUS_INVALID_PARAMETER -- passed parameters (input/output buffers)
                                are invalid

    STATUS_ACCESS_VIOLATION  -- An illegal address/pointer was given.

--*/

{
                                
    ULONG InBufferSize;         // Amount of data avail. from caller.
    ULONG OutBufferSize;        // Max data that caller can accept.
    ULONG DataBufferSize;
    FOI_RESOLVE_INPUT *pI;      // pointer to input buffer
	FOI_RESOLVE_OUTPUT *pO;     // pointer to output buffer
    PFILE_OBJECT pFObj;
    PDEVICE_OBJECT pDObj;
    ULONG ul;

    // Size of buffer containing data from application
    InBufferSize  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    // Size of buffer for data to be sent to application
    OutBufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // NT copies inbuf here before entry and copies this to outbuf after
    // return, for METHOD_BUFFERED IOCTL's.
    pI = (FOI_RESOLVE_INPUT *)pIrp->AssociatedIrp.SystemBuffer;
    pO = (FOI_RESOLVE_OUTPUT *)pIrp->AssociatedIrp.SystemBuffer;

    // Check input & output buffer sizes
    //
    if ( InBufferSize != sizeof(FOI_RESOLVE_INPUT) ||
	     OutBufferSize != sizeof(FOI_RESOLVE_OUTPUT) )
    {
        return STATUS_INVALID_PARAMETER;
    }
    pFObj = (PFILE_OBJECT)(pI->FObjAddress);    // get file object pointer
    pO->fobj.isValid = FALSE;
    pO->dobj.isValid = FALSE;

    // try to access file and driver object...
    try {
        if (pFObj->Type != IO_TYPE_FILE || pFObj->Size != sizeof(FILE_OBJECT))
            return STATUS_INVALID_PARAMETER;
		// Access file object by given pointer and copy values to users buffer.
        pO->fobj.Type           = pFObj->Type;
        pO->fobj.Size           = pFObj->Size;
        pO->fobj.DeviceObject   = (ULONG)pFObj->DeviceObject;
        pO->fobj.LockOperation  = pFObj->LockOperation;
        pO->fobj.DeletePending  = pFObj->DeletePending;
        pO->fobj.ReadAccess     = pFObj->ReadAccess;
        pO->fobj.WriteAccess    = pFObj->WriteAccess;
        pO->fobj.DeleteAccess   = pFObj->DeleteAccess;
        pO->fobj.SharedRead     = pFObj->SharedRead;
        pO->fobj.SharedWrite    = pFObj->SharedWrite;
        pO->fobj.SharedDelete   = pFObj->SharedDelete;
        pO->fobj.Flags          = pFObj->Flags;
        pO->fobj.CurrentByteOffset.QuadPart = pFObj->CurrentByteOffset.QuadPart;
        pO->fobj.Waiters        = pFObj->Waiters;
        pO->fobj.Busy           = pFObj->Busy;
        ul = (pFObj->FileName.Length > (MAX_FOBJ_FILE_LEN*2)) ? (2*MAX_FOBJ_FILE_LEN) : pFObj->FileName.Length;
        RtlZeroMemory(pO->fobj.FileName, MAX_FOBJ_FILE_LEN*2);
		RtlCopyMemory(pO->fobj.FileName, pFObj->FileName.Buffer, ul);
        pO->fobj.isValid        = TRUE;

        // try to access the device object this file object belongs to.
		pDObj = pFObj->DeviceObject;
        if (pDObj->Type == IO_TYPE_DEVICE)
        {
            PUNICODE_STRING         fullUniName;
            ULONG                   actualLen;
            
			pO->dobj.Type           = pDObj->Type;
            pO->dobj.Size           = pDObj->Size;
            pO->dobj.ReferenceCount = pDObj->ReferenceCount;
            pO->dobj.DriverObject   = pDObj->DriverObject;
            pO->dobj.Flags          = pDObj->Flags;             // See above:  DO_..
            pO->dobj.Characteristics = pDObj->Characteristics;  // See ntioapi:  FILE_...
    		pO->dobj.DeviceType     = pDObj->DeviceType;

            RtlZeroMemory(pO->dobj.DeviceName, MAX_FOBJ_FILE_LEN*2);
    		fullUniName = ExAllocatePool(PagedPool, (MAX_FOBJ_FILE_LEN*2));
            if(fullUniName)
			{
				// undocumented: get device objects name...
                fullUniName->MaximumLength = MAX_FOBJ_FILE_LEN*2-(2*sizeof(ULONG));
                if(NT_SUCCESS(ObQueryNameString(pDObj, fullUniName, MAX_FOBJ_FILE_LEN-sizeof(ULONG), &actualLen)))
				{
        			ul = (fullUniName->Length > (MAX_FOBJ_FILE_LEN*2)) ? (2*MAX_FOBJ_FILE_LEN) : fullUniName->Length;
            		RtlCopyMemory(pO->dobj.DeviceName, fullUniName->Buffer, ul);
                }
                ExFreePool( fullUniName );
            }
            pO->dobj.isValid        = TRUE;
        }

    } except( EXCEPTION_EXECUTE_HANDLER )
	{
        return STATUS_ACCESS_VIOLATION;   // It was not legal.
    }

    // Indicate # of bytes read
    //
    pIrp->IoStatus.Information = sizeof(FOI_RESOLVE_OUTPUT);
    return STATUS_SUCCESS;
}


VOID
GpdUnload(
    PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:
    This routine prepares our driver to be unloaded.  It is responsible
    for freeing all resources allocated by DriverEntry as well as any
    allocated while the driver was running.  The symbolic link must be
    deleted as well.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/

{
    PLOCAL_DEVICE_INFO pLDI;
    CM_RESOURCE_LIST NullResourceList;
    BOOLEAN ResourceConflict;
    UNICODE_STRING Win32DeviceName;

    // Find our global data
    pLDI = (PLOCAL_DEVICE_INFO)DriverObject->DeviceObject->DeviceExtension;

    // Assume all handles are closed down.
    // Delete the things we allocated - devices, symbolic links

    RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&Win32DeviceName);

    IoDeleteDevice(pLDI->DeviceObject);
}
