/*HDR************************************************************************
 *                                                                         
 * Softek -                                     
 *
 *===========================================================================
 *
 * N dirparse.c
 * P Replicator 
 * S Boot
 * V Generic
 * A J. Christatos - jchristatos@softek.com
 * D 05.20.2004
 * O Parse directories at boot time
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 04.28.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: dirparse.c,v 1.2 2004/06/08 17:04:33 jcq40 Exp $"
 *
 *HDR************************************************************************/

#include <ntddk.h>
#include "common.h"
#include "mmgr_ntkrnl.h"

OS_NTSTATUS
  ZwCreateEvent(
    OUT PHANDLE  EventHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
    IN EVENT_TYPE  EventType,
    IN BOOLEAN  InitialState
    );

OS_NTSTATUS 
  ZwQueryDirectoryFile(
    IN HANDLE  FileHandle,
    IN HANDLE  Event  OPTIONAL,
    IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
    IN PVOID  ApcContext  OPTIONAL,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    OUT PVOID  FileInformation,
    IN ULONG  Length,
    IN FILE_INFORMATION_CLASS  FileInformationClass,
    IN BOOLEAN  ReturnSingleEntry,
    IN PUNICODE_STRING  FileName  OPTIONAL,
    IN BOOLEAN  RestartScan
    );

OS_NTSTATUS
  ZwWaitForSingleObject(
    IN HANDLE  Handle,
    IN BOOLEAN  Alertable,
    IN PLARGE_INTEGER  Timeout OPTIONAL
    );

OS_NTSTATUS
  ZwOpenDirectoryObject(
    OUT PHANDLE  DirectoryHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
    );

OS_NTSTATUS
	ZwQueryDirectoryObject(IN HANDLE DirectoryHandle,
	OUT PVOID Buffer,
	IN ULONG  BufferLength,
	IN BOOLEAN ReturnSingleEntry,
	IN BOOLEAN RestartScan,
	IN OUT PULONG Context,
	OUT PULONG ReturnLength);

/* from ntifs.h*/
typedef struct _FILE_BOTH_DIR_INFORMATION {
  ULONG  NextEntryOffset;
  ULONG  FileIndex;
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  LARGE_INTEGER  EndOfFile;
  LARGE_INTEGER  AllocationSize;
  ULONG  FileAttributes;
  ULONG  FileNameLength;
  ULONG  EaSize;
  CCHAR  ShortNameLength;
  WCHAR  ShortName[12];
  WCHAR  FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;


typedef enum
{
	OBJ_DEVICE    = 1,
	OBJ_SYMBLINK  = 2,
	OBJ_DIR       = 3

} dir_entry_e;


/*B**************************************************************************
 * ExpandSymLink -
 *
 * Expand symbolic Link uns_link to full name uns_expand.
 * Callers needs to free memory uns_expand.Buffer !!
 *
 *E==========================================================================
 */
MMG_PRIVATE
OS_NTSTATUS
_ExpandSymLink(IN  UNICODE_STRING  *p_uns_link,
 			   IN  HANDLE          *pDirHandle,
			   OUT UNICODE_STRING  *p_uns_expand)

/**/
{
	OS_NTSTATUS				 ret = STATUS_SUCCESS;
	HANDLE					 linkHandle;
	OBJECT_ATTRIBUTES		 obj_attrib;
	IO_STATUS_BLOCK			 IoStatusBlock;
	ULONG					 ReturnedLength;

	MMGDEBUG(MMGDBG_LOW, ("Entering _ExpandSymLink \n"));

	InitializeObjectAttributes(	&obj_attrib,
								p_uns_link,
								OBJ_KERNEL_HANDLE,
								*pDirHandle,	
								NULL            /* no ACL */
								);

	ret = ZwOpenSymbolicLinkObject(	&linkHandle,
								    GENERIC_ALL,
									&obj_attrib    );
	if (!OS_NT_SUCCESS(ret))
	{
		MMGDEBUG(MMGDBG_LOW, ("ZwOpenSymbolicLinkObject failed %x \n", ret));
		return(ret);
	}

	RtlZeroMemory(p_uns_expand , sizeof(UNICODE_STRING));

	ret = ZwQuerySymbolicLinkObject( linkHandle,
									 p_uns_expand,
									 &ReturnedLength     );

	if (ret == STATUS_BUFFER_TOO_SMALL)
	{
		p_uns_expand->Buffer = OS_ExAllocatePoolWithTag(NonPagedPool, 
											            sizeof(WCHAR) * ReturnedLength,
														'1gat' );
		ASSERT(p_uns_expand->Buffer);
		p_uns_expand->Length = 0;
		p_uns_expand->MaximumLength = sizeof(WCHAR) * ReturnedLength;

		ret = ZwQuerySymbolicLinkObject( linkHandle,
									     p_uns_expand,
									     &ReturnedLength     );
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("ZwQuerySymbolicLinkObject failed %x \n", ret));
			return(ret);
		}
	} else	if (!OS_NT_SUCCESS(ret))
	{
		MMGDEBUG(MMGDBG_LOW, ("ZwQuerySymbolicLinkObject failed %x \n", ret));
		return(ret);
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _ExpandSymLink (%x) \n", ret));

return(ret);
} /* _ExpandSymLink */

/*
 * DirSearch - 
 *
 */
OS_NTSTATUS
DirSearch(IN HANDLE          *DirHandle,
		  IN PUNICODE_STRING p_uns)

/**/
{
	OS_NTSTATUS               ret = STATUS_SUCCESS;
	IO_STATUS_BLOCK			  IoStatusBlock;
	FILE_BOTH_DIR_INFORMATION *pDirInfo = NULL;
	HANDLE					  Event;

	MMGDEBUG(MMGDBG_LOW, ("Searching %.*S\n", 
		p_uns->Length, 
		p_uns->Buffer));

	ret = ZwCreateEvent(&Event, GENERIC_ALL, 0, NotificationEvent, FALSE);
	if (!OS_NT_SUCCESS(ret))
	{
		MMGDEBUG(MMGDBG_LOW, ("KeInitializeEvent %x \n", ret));
		return(ret);
	}

	pDirInfo = OS_ExAllocatePoolWithTag(NonPagedPool,
									    sizeof(FILE_BOTH_DIR_INFORMATION)+
										sizeof(WCHAR)*256,
										'pfid');
	ASSERT(pDirInfo);

	/* 
	 * Iterate the directory for our filename p_uns
	 */
	ret = ZwQueryDirectoryFile(
				*DirHandle,
				NULL, //Event,
				0, 
				0,
				&IoStatusBlock,
				pDirInfo,
				sizeof(FILE_BOTH_DIR_INFORMATION)+sizeof(WCHAR)*256,
				FileBothDirectoryInformation,
				FALSE,
				p_uns,
				TRUE    );

	if (ret == STATUS_PENDING)
	{
		ret = ZwWaitForSingleObject(
				*DirHandle,
				TRUE,
				NULL);

		ret = IoStatusBlock.Status;
	}

	if (!OS_NT_SUCCESS(ret))
	{
		MMGDEBUG(MMGDBG_LOW, ("ZwQueryDirectoryFile %x \n", ret));
		return(ret);
	}

	MMGDEBUG(MMGDBG_LOW, ("found %.*S\n", 
		pDirInfo->ShortNameLength, 
		pDirInfo->ShortName));

	OS_ExFreePool(pDirInfo);

return(ret);
} /* DirSearch */

typedef struct _DIRECTORY_BASIC_INFORMATION
{
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectTypeName;

} DIRECTORY_BASIC_INFORMATION;

/*
 * DirSearch - 
 *
 */
OS_NTSTATUS
ObjDirSearch(IN HANDLE          DirHandle,
		     IN PUNICODE_STRING p_uns,
		     OUT dir_entry_e    *p_entryType)

/**/
{
	OS_NTSTATUS                  ret   = STATUS_OBJECT_NAME_NOT_FOUND;
	IO_STATUS_BLOCK			     IoStatusBlock;
	DIRECTORY_BASIC_INFORMATION  *pDirInfo = NULL;
	ULONG                        Ctxt;
	ULONG                        ReturnedLength, i;
	UNICODE_STRING               uns_str[3];
	dir_entry_e                  typArr[3];


	pDirInfo = OS_ExAllocatePoolWithTag(NonPagedPool,
									    sizeof(DIRECTORY_BASIC_INFORMATION)+
										sizeof(WCHAR)*256,
										'pfit');
	ASSERT(pDirInfo);
	
	Ctxt = 0;
	ReturnedLength = 0;
	while (TRUE)
	{
		/*
		 * Ask for one entry at a time
		 */
		ret = ZwQueryDirectoryObject(DirHandle, 
									pDirInfo,
									sizeof(DIRECTORY_BASIC_INFORMATION)+
									sizeof(WCHAR)*256,
									TRUE, 
									FALSE, 
									&Ctxt, 
									&ReturnedLength);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("ZwQueryDirectoryObject %x \n", ret));
			return(ret);
		}

		*p_entryType = OBJ_DIR;

		MMGDEBUG(MMGDBG_LOW, (" Obj directory %.*S \n", 
				pDirInfo->ObjectName.Length, 
				pDirInfo->ObjectName.Buffer));
		MMGDEBUG(MMGDBG_LOW, ("type %.*S \n", 
			    pDirInfo->ObjectTypeName.Length, 
				pDirInfo->ObjectTypeName.Buffer));

		if (RtlCompareUnicodeString(&pDirInfo->ObjectName, p_uns, TRUE)==0)
		{
			MMGDEBUG(MMGDBG_LOW, ("found in directory %.*S \n", 
				p_uns->Length, 
				p_uns->Buffer));

			RtlInitUnicodeString(&uns_str[0], L"device");
			RtlInitUnicodeString(&uns_str[1], L"SymbolicLink");
			RtlInitUnicodeString(&uns_str[2], L"directory");
			typArr[0] = OBJ_DEVICE;
			typArr[1] = OBJ_SYMBLINK;
			typArr[2] = OBJ_DIR;

			for (i = 0; i < 3; i++)
			{
				if (RtlCompareUnicodeString(&pDirInfo->ObjectTypeName, &uns_str[i], TRUE)==0)
				{
					*p_entryType = typArr[i];
					i = 3;
					break;
				}
			}
			return(STATUS_SUCCESS);
		}
	}

	/* Search dir */

return(ret);
} /* ObjDirSearch */

/*B**************************************************************************
 * OpenBootDir -
 *
 * Open System root directory at boot time to find our configuration file. 
 *
 * We follow/expand symbolic links from "\\SystemRoot"  until we find the 
 * correct device pathname. We then open the Device pathname (pseudo mounting it) 
 * and append the remaining path to it.
 * 
 * We return this directory's handle to caller.
 *E==========================================================================
 */
MMG_PUBLIC
OS_NTSTATUS
OpenBootDir(OUT HANDLE *pResultHdl)

/**/
{
	OS_NTSTATUS				 ret = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES		 obj_attrib;
	IO_STATUS_BLOCK			 IoStatusBlock;
	UNICODE_STRING           uns_name, 
		                     uns_link, 
							 uns_nextc,
							 uns_remain,
							 uns_open;
	WCHAR                    *p_scratchBuf;
	HANDLE					 FileHandle, 
							 DirHandle;
	dir_entry_e              entryType;
	ULONG					 index;


	MMGDEBUG(MMGDBG_LOW, ("Entering openBootDir \n"));

	try { /* finally */

	FileHandle  = NULL;
	DirHandle   = NULL;
	entryType   = OBJ_SYMBLINK;
	*pResultHdl = NULL;

	RtlZeroMemory(&uns_link, sizeof(UNICODE_STRING));
	RtlZeroMemory(&uns_nextc, sizeof(UNICODE_STRING));
	RtlZeroMemory(&uns_remain, sizeof(UNICODE_STRING));

	/*
	 * Try to find the remaining path for the config file
	 * from the registry otherwise use default value.
	 */
	RtlInitUnicodeString(&uns_open, L"system32\\drivers");
	//ExpandFromRegistry("");

	/*
	 * We allocate a scratch buffer for unicode strings
	 * expansions, at startup time so we have only one 
	 * free point at the end of the function.
	 * ASSUME: no strings used or returned by the system
	 * should be bigger that this buffer ! (2*MAX_PATH)
	 */

	p_scratchBuf = (WCHAR *)OS_ExAllocatePoolWithTag(NonPagedPool,
				                                     sizeof(WCHAR)*(2*256), 
													 'ghft');
	if (p_scratchBuf == NULL)
	{
		MMGDEBUG(MMGDBG_LOW, ("Cannot allocate scratch buffer \n"));
		ret = STATUS_INSUFFICIENT_RESOURCES;
		leave;
	}

	/*
	 * we use a trick, we put a "\" a start
	 * and we use the next wchar to insert
	 * the string so if we need a "\" first
	 * we can just decrement the buffer ptr
	 * one wchar away.
	 */
	p_scratchBuf[0] = L'\\';
	uns_name.Buffer = &p_scratchBuf[1];
	uns_name.Length = 0;
	uns_name.MaximumLength = sizeof(WCHAR)*(2*256);

	/* system root is an object manager symbolic link to the root path... */
	RtlAppendUnicodeToString(&uns_name, L"SystemRoot");

	OS_FsRtlDissectName(uns_name, &uns_nextc, &uns_remain);

	/*
	 * Phase One -
	 *
	 * We follow/expand directories and links in the 
	 * windows Object manager.
	 */
	while(entryType != OBJ_DEVICE)
	{
	
		/* Here we have:
		 * uns_name is the full pathname to be parse at iteration 1
         * uns_nextc is the next component to be analyze.
         * uns_remaining is the rest of the path after uns_nextc 
         * entryType is the type of nextc
         * DirHandle is the current directory handle if any.
		 */

		if (uns_link.Buffer != NULL)
		{
			OS_ExFreePool(uns_link.Buffer);
			RtlZeroMemory(&uns_link, sizeof(UNICODE_STRING));
		}

		if (DirHandle == NULL &&
			uns_nextc.Buffer[0] != L'\\')
		{
			uns_nextc.Length += sizeof(WCHAR);
			uns_nextc.MaximumLength += sizeof(WCHAR);
			uns_nextc.Buffer--;
			ASSERT(uns_nextc.Buffer[0] == L'\\');
		}

		/*
		 * If uns_nextc is a symbolic link
		 * expand it now.
		 * Create a new Unicode string
		 * with uns_link+uns_remain
		 * and restart the expansion from there.
		 */
		if (entryType == OBJ_SYMBLINK)
		{
			UNICODE_STRING uns_new;
			UNICODE_STRING uns_tmp;

			ret = _ExpandSymLink(&uns_nextc, &DirHandle, &uns_link);
			if (!OS_NT_SUCCESS(ret))
			{
				MMGDEBUG(MMGDBG_LOW, ("cannot expand symnlik %.*S (%x)\n", 
					uns_nextc.Length, uns_nextc.Buffer, ret));
				leave;
			}

			/*
			 * If the link starts with a '\'
			 * it is an absolute path name.
			 * we need to close the relative 
			 * directory handle.
			 */
			if (uns_link.Buffer[0] == L'\\' &&
				DirHandle != NULL)
			{
				ZwClose(DirHandle);
				DirHandle = NULL;
			}

			/*
			 * We are overwriting p_scratchBuf but
			 * uns_remain still points to it, so
			 * we must be careful not to scratch it
			 * if uns_link is greater
			 */
				
			/*
			 * First put in place the remain string
			 */
			if (uns_remain.Length != 0)
			{
				uns_tmp.Length = 0;
				uns_tmp.MaximumLength = sizeof(WCHAR)*(2*256);
				index = uns_link.Length+sizeof(WCHAR); // start at one
				index /= sizeof(WCHAR); // in WCHAR unit
				uns_tmp.Buffer = &p_scratchBuf[index];
				p_scratchBuf[index] = L'\\';
				if (uns_remain.Buffer[0] != L'\\')
				{
					uns_tmp.Length += sizeof(WCHAR);
					RtlAppendUnicodeStringToString(&uns_tmp, &uns_remain);
				} else {
					RtlCopyUnicodeString(&uns_tmp, &uns_remain);
				}
			}

			/*
			 * Create the new string
			 * uns_link is < 2*MAX_PATH
			 * we overid uns_nextc here.
			 */
			
			uns_new.MaximumLength = uns_remain.MaximumLength+uns_link.MaximumLength+2;
			uns_new.Length = 0;
			uns_new.Buffer = &p_scratchBuf[1];
			RtlCopyUnicodeString(&uns_new, &uns_link);

			/*
			 * replace that null character by '\' if remains exist.
			 * uns_link.Length is w/o the NULL
			 * so uns_link.Buffer[uns_link.Length/sizeof(WCHAR)] gives the NULL.
			 */
			index = (uns_link.Length)/sizeof(WCHAR);
			if (uns_link.Buffer[index] == 0 &&
				uns_remain.Length != 0)
			{
				uns_new.Buffer[index] = L'\\';
			}

			uns_new.Length += uns_remain.Length+sizeof(WCHAR);// the '\'

			/* Take the new identity */
			uns_remain = uns_new; 

			/*
			 * Get the next component to search for
			 */
			OS_FsRtlDissectName(uns_remain, &uns_nextc, &uns_remain);

			if (DirHandle == NULL &&
				uns_nextc.Buffer[0] != L'\\')
			{
				uns_nextc.Length += sizeof(WCHAR);
				uns_nextc.MaximumLength += sizeof(WCHAR);
				uns_nextc.Buffer--;
				ASSERT(uns_nextc.Buffer[0] == L'\\');
			}
		}   /* if symlink */

		MMGDEBUG(MMGDBG_LOW, ("Opening %.*S \n", uns_nextc.Length, uns_nextc.Buffer));

		/* 
		 * Now Open directory 'nextc' relative to DirHandle 
		 */
		InitializeObjectAttributes(	&obj_attrib,
									&uns_nextc,
									OBJ_KERNEL_HANDLE,
									DirHandle,	
									NULL         /* no ACL */
									);

		ret = ZwOpenDirectoryObject( &FileHandle, 
			                         DIRECTORY_ALL_ACCESS, 
									 &obj_attrib);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("ZwOpenDirectoryObject failed %x \n", ret));
			return(ret);
		}

		/* Get Next Component name */
		OS_FsRtlDissectName(uns_remain, &uns_nextc, &uns_remain);
		
		MMGDEBUG(MMGDBG_LOW, ("Searching %.*S \n", uns_nextc.Length, uns_nextc.Buffer));

		/* uns_remain mem leak ... ?*/

		/* end condition no more component */
		if (uns_nextc.Length == 0)
		{
			break;
		}

		/* Search nextc in current directory */
		ret = ObjDirSearch(FileHandle, &uns_nextc, &entryType);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("DirSearch failed %x \n", ret));
			return(ret);
		}
 
		ZwClose(DirHandle);
		DirHandle = FileHandle;
		
	} /* while */

	if (uns_link.Length == 0)
	{
		MMGDEBUG(MMGDBG_LOW, ("No Device found ! \n"));
		ret = STATUS_OBJECT_NAME_NOT_FOUND;
		leave;
	}

	/*
	 * Phase Two
	 *
	 * We just open the device with the name we just found.
	 */

	uns_name.Length = 0;
	if (uns_name.Length + uns_remain.Length + uns_link.Length >= uns_name.MaximumLength)
	{
		MMGDEBUG(MMGDBG_LOW, ("Not enough space left in string \n"));
		ret = STATUS_INSUFFICIENT_RESOURCES;
		leave;
	}

	/*
	 * To be sure not to overwrite 
	 * uns_remain we copy it first
	 * at the correct place.
	 */
	{
		UNICODE_STRING uns_tmp;
		uns_tmp.Length = 0;
		uns_tmp.MaximumLength = sizeof(WCHAR)*(2*256);
		index = uns_link.Length+sizeof(WCHAR); // start at one
		index /= sizeof(WCHAR); // in WCHAR unit
		uns_tmp.Buffer = &p_scratchBuf[index];
		if (uns_remain.Buffer[0] != L'\\')
		{
			uns_tmp.Length += sizeof(WCHAR);
			RtlAppendUnicodeStringToString(&uns_tmp, &uns_remain);
		} else {
			RtlCopyUnicodeString(&uns_tmp, &uns_remain);
		}
		RtlCopyUnicodeString(&uns_name, &uns_link);
		index = (uns_link.Length)/sizeof(WCHAR);
		if (uns_link.Buffer[index] == 0 &&
			uns_remain.Length != 0)
		{
			uns_name.Buffer[index] = L'\\';
		}
	}

	uns_name.Length += uns_remain.Length;

	
	//RtlAppendUnicodeStringToString(&uns_name, &uns_open);
	if (uns_link.Buffer != NULL)
	{
		OS_ExFreePool(uns_link.Buffer);
		RtlZeroMemory(&uns_link, sizeof(UNICODE_STRING));
	}

	MMGDEBUG(MMGDBG_LOW, ("Opening %.*S \n", uns_name.Length, uns_name.Buffer));

	DirHandle  = NULL;
    FileHandle = NULL;

	/* Open directory 'nextc' relative to DirHandle */
	InitializeObjectAttributes(	&obj_attrib,
								&uns_name,
								OBJ_KERNEL_HANDLE,
								NULL,	     /* no root handle absolute open */
								NULL         /* no ACL */
								);

	ret = ZwCreateFile(	&DirHandle,
						GENERIC_ALL|FILE_LIST_DIRECTORY,
						&obj_attrib,
						&IoStatusBlock,
						NULL,
						FILE_ATTRIBUTE_DIRECTORY,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_OPEN,
						0,
						NULL,
						0
						);

	if (!OS_NT_SUCCESS(ret))
	{
		MMGDEBUG(MMGDBG_LOW, ("ZwCreateFile failed %x \n", ret));
		leave;
	}

		/* Open directory 'nextc' relative to DirHandle */
	InitializeObjectAttributes(	&obj_attrib,
								&uns_open,
								OBJ_KERNEL_HANDLE,
								DirHandle,	     /* no root handle absolute open */
								NULL         /* no ACL */
								);

	ret = ZwCreateFile(	&FileHandle,
						GENERIC_ALL|FILE_LIST_DIRECTORY,
						&obj_attrib,
						&IoStatusBlock,
						NULL,
						FILE_ATTRIBUTE_DIRECTORY,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_OPEN,
						0,
						NULL,
						0
						);

	if (!OS_NT_SUCCESS(ret))
	{
		MMGDEBUG(MMGDBG_LOW, ("ZwCreateFile failed %x \n", ret));
		leave;
	}

	*pResultHdl = FileHandle;

	} finally {

		OS_ExFreePool(p_scratchBuf);
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving openBootDir(%x) \n", ret));

return(ret);
} /* OpenBootDir */

/*EOF*/