/*
 * newbab.c - replacement for bab management
 */

#include "common.h"

#ifdef   WIN32
#ifdef   _KERNEL_
#include "mmgr_ntkrnl.h"
#else    /* !_KERNEL_*/
#include <windows.h>
#include <stdio.h>
#include "mmgr_ntusr.h"
#endif   /* _KERNEL_ */
#endif   /* _WINDOWS */

#include "slab.h"
#include "dtb.h"
#include "mmg.h"

//#include "ntddk.h"
#include "assert.h"


MMG_PUBLIC ULONG MmgDebugLevel;

/* Not in the w2k ntddk.h ?*/
#define SEC_COMMIT        0x8000000 

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    );

#define NT_DEVICE_NAME  L"\\Device\\newbab"
#define DOS_DEVICE_NAME L"\\DosDevices\\newbab"

#define IOCTL_MAPMEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x001, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _dev_control_ 
{

	PVOID BuffAddr;

} dev_control;


/*
 * The device extension
 */

typedef struct _dev_ext_ 
{
	int signature;
#define SIGNATURE (0xcafecafe)

	int size;

	// Try to allocate a bunch of memory
	PVOID MemoryBuffer;
	PVOID SystemBuffer;

} dev_ext;

BOOLEAN ExceptionFilter(EXCEPTION_POINTERS *pExceptionInfo,
						ULONG              ExceptionCode)

/**/
{

return(EXCEPTION_EXECUTE_HANDLER);
} /* ExceptionFilter */

NTSTATUS NewBabOpen(IN DEVICE_OBJECT *p_devobj,
					IN IRP *p_irp)

/**/
{
	KdPrint(("NewBabOpen"));

	p_irp->IoStatus.Status = STATUS_SUCCESS;
	p_irp->IoStatus.Information = 0;

	IoCompleteRequest(p_irp, IO_NO_INCREMENT);

return(STATUS_SUCCESS);
} /* NewBabOpen */

NTSTATUS NewBabClose(IN DEVICE_OBJECT *p_devobj,
					 IN IRP *p_irp)

/**/
{
	KdPrint(("NewBabClose"));

	p_irp->IoStatus.Status = STATUS_SUCCESS;
	p_irp->IoStatus.Information = 0;

	IoCompleteRequest(p_irp, IO_NO_INCREMENT);

return(STATUS_SUCCESS);
} /* NewBabClose */

/*
 * newbabMapMemory -
 *
 */
newbabMapMemory(IN PDEVICE_OBJECT DeviceObject,
				IN PVOID          LogicalAddress,
				IN ULONG          Lengthtomap,
				OUT PVOID         *OutBuffer
				)

/**/
{
	PHYSICAL_ADDRESS  Phys;
	SIZE_T            length;
	UNICODE_STRING    PhysName;
	OBJECT_ATTRIBUTES ObjAttr;
	HANDLE            PhysHdl;
	PVOID             PhysMemSection;
	NTSTATUS          ntstatus;
	PHYSICAL_ADDRESS  viewBase = {0};
	PVOID	          VirtBase;
	LARGE_INTEGER     secsz;


	KdPrint(("newbabMapMemory"));

	secsz.QuadPart = 0;
	secsz.LowPart  = Lengthtomap;


	ntstatus = ZwCreateSection(&PhysHdl,
							   SECTION_ALL_ACCESS,
							   NULL,
							   &secsz,
							   PAGE_READWRITE,
							   SEC_RESERVE,
							   NULL 
		                       );

	if (!NT_SUCCESS(ntstatus))
	{
		KdPrint(("Cannot  ZwCreateSection : %x \n", ntstatus));
		return(ntstatus);
	}

	ntstatus = ObReferenceObjectByHandle(PhysHdl, 
		                                 SECTION_ALL_ACCESS, 
										 (POBJECT_TYPE) NULL,
										 KernelMode, 
										 &PhysMemSection, 
										 (POBJECT_HANDLE_INFORMATION)NULL);
	if (!NT_SUCCESS(ntstatus))
	{
		KdPrint(("Cannot  ObReferenceObjectByHandle : %x \n",ntstatus));
		return(ntstatus);
	}

	VirtBase = 0;//LogicalAddress;
	viewBase = MmGetPhysicalAddress(LogicalAddress);
	length   = 0;

	ntstatus = ZwMapViewOfSection(PhysHdl, 
								  NtCurrentProcess(),
								  &VirtBase,
								  0L,
								  Lengthtomap,
								  &viewBase,
								  &length,
								  ViewShare,
								  SEC_RESERVE, //0,
								  PAGE_READWRITE);
	if (!NT_SUCCESS(ntstatus))
	{
		KdPrint(("Cannot  ZwMapViewOfSection : %x \n",ntstatus));
		return(ntstatus);
	} 

	*OutBuffer = VirtBase;

return(STATUS_SUCCESS);
} /* newbabMapMemory */

/*
 * NewBabControl - 
 */
NTSTATUS NewBabControl(IN DEVICE_OBJECT *p_devobj,
					   IN IRP *p_irp)

/**/
{
	NTSTATUS ntstatus  = STATUS_SUCCESS;
	dev_ext  *p_devext = NULL;
	ULONG    controlcode;
	IO_STACK_LOCATION *p_iostack = NULL;

	ASSERT(p_irp);
	KdPrint(("NewBabClose"));

	try {
		try {

	p_iostack = IoGetCurrentIrpStackLocation(p_irp);
	controlcode = p_iostack->Parameters.DeviceIoControl.IoControlCode;
	p_devext = (dev_ext *)p_devobj->DeviceObjectExtension;

	switch(controlcode)
	{
	case IOCTL_MAPMEMORY:
		p_irp->UserBuffer = 0;

#ifdef NOP
		p_devext->MemoryBuffer = ExAllocatePool(PagedPoolCacheAligned, 1024*1024);
		if (p_devext->MemoryBuffer == NULL)
		{
			p_irp->IoStatus.Information = 0;
			p_irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			IoCompleteRequest(p_irp, IO_NO_INCREMENT);
			return(ntstatus);
		}
#endif
		
		ntstatus = newbabMapMemory(p_devobj, p_devext->MemoryBuffer, 
			                       1024*1024, &p_devext->SystemBuffer);

		if (!NT_SUCCESS(ntstatus))
		{
			p_irp->IoStatus.Information = 0;
			p_irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			IoCompleteRequest(p_irp, IO_NO_INCREMENT);
			return(ntstatus);
		}
		RtlFillMemory(p_devext->MemoryBuffer, 512, 'A');
		((dev_control *)(p_irp->UserBuffer))->BuffAddr = p_devext->SystemBuffer;

		break;

	default:
		ntstatus = STATUS_INVALID_PARAMETER;
		break;
	}

	p_irp->IoStatus.Information = 0;
	p_irp->IoStatus.Status = ntstatus;
	IoCompleteRequest(p_irp, IO_NO_INCREMENT);

	} 
		except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()))
		{
		}
	}
	finally 
	{
		
	}

return(ntstatus);
}	/* NewBabControl */

VOID NewBabUnload(IN DRIVER_OBJECT *p_drvobj)

/**/ 
{

	KdPrint(("NewBabUnload"));

} /* NewBabUnload */

/*
 * DriverEntry - 
 */

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
					 IN PUNICODE_STRING RegistryPath)

/**/
{
	UNICODE_STRING DriverName,
		           Win32Name;
	DEVICE_OBJECT  *p_devobj;
	NTSTATUS	   ntstatus = STATUS_SUCCESS;
	dev_ext        *p_ext   = NULL;
	ULONG          ret      = 0;

	try {
		try {

			KdPrint(("DriverEntry\n"));
			DbgBreakPoint();

			RtlInitUnicodeString(&DriverName, NT_DEVICE_NAME);
			ntstatus = IoCreateDevice(DriverObject,
									  sizeof(dev_ext),
									  &DriverName,
									  FILE_DEVICE_UNKNOWN,
									  0,
									  FALSE,
									  &p_devobj);

			if (!NT_SUCCESS(ntstatus))
			{
				KdPrint(("Cannot create device %x \n", ntstatus));
				leave;
			}

			DriverObject->MajorFunction[IRP_MJ_CREATE] = NewBabOpen;
			DriverObject->MajorFunction[IRP_MJ_CLOSE]  = NewBabClose;
			DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NewBabControl;
			DriverObject->DriverUnload = NewBabUnload;

			p_devobj->Flags |= DO_DIRECT_IO;
			p_ext = (dev_ext *)p_devobj->DeviceExtension;
			RtlZeroMemory(p_ext, sizeof(dev_ext));
			p_ext->signature = SIGNATURE;
			p_ext->size = 0;


			RtlInitUnicodeString(&Win32Name, DOS_DEVICE_NAME);
			ntstatus = IoCreateSymbolicLink(&Win32Name, &DriverName);
			if (!NT_SUCCESS(ntstatus))
			{
				KdPrint(("Cannot Create Symbolic link\n"));
				leave;
			}

		} 
		except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()))
		{
		}
	}
	finally 
	{
		if (!NT_SUCCESS(ntstatus))
		{
			IoDeleteDevice(p_devobj);
		}
	}

return (ntstatus);
} /* DriverEntry */


/* EOF */					
