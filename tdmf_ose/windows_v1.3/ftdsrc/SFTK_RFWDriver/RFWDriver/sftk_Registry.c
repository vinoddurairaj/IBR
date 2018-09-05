/**************************************************************************************

Module Name: sftk_Registry.C   
Author Name: Parag sanghvi 
Description: Registry APIS
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

//
// ------------------ Registry access APIS ------------------------------------
//
NTSTATUS 
sftk_IsRegKeyPresent(	CONST PWCHAR KeyPathName, 
						CONST PWCHAR KeyValueName)
{
    OBJECT_ATTRIBUTES   attributes;     // attributes for the registry key
    HANDLE              keyHandle;      // handle into the registry
    PVOID               pInformation;   // the information structure
    ULONG               size, resultLength;   // the amount of return data
    NTSTATUS            status;         // intermediate result
	UNICODE_STRING		keyPathName;
	UNICODE_STRING		keyValueName;


	// OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);

	RtlInitUnicodeString(&keyPathName, KeyPathName);
	RtlInitUnicodeString(&keyValueName, KeyValueName);
	
	InitializeObjectAttributes(	&attributes, 
								&keyPathName, 
								OBJ_CASE_INSENSITIVE, 
								NULL, 
								NULL);

	// open the key
    status = ZwOpenKey( &keyHandle, KEY_READ, &attributes);
    
    // if we succeeded, check for a value key
    if (NT_SUCCESS(status)) 
	{ // success
		size = sizeof(KEY_VALUE_BASIC_INFORMATION) + ((keyValueName.Length + 1) * sizeof(WCHAR)) ;

		// DEFAULT_POOL_TAG is defined in project file FcEngineDriver.dsp.
		pInformation = OS_AllocMemory(	PagedPool, size);
        if (!pInformation) 
		{
			DebugPrint((DBG_ERROR, "sftk_IsRegKeyPresent:: OS_AllocMemory(size %d) Failed returning status %x, KeyPathName %S KeyValueName %S! \n", 
									size, STATUS_INSUFFICIENT_RESOURCES, KeyPathName, KeyValueName));
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // query the registry for the value key
        status = ZwQueryValueKey(	keyHandle, 
									&keyValueName, 
									KeyValueBasicInformation, 
									pInformation,
									size, 
									&resultLength);

		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_IsRegKeyPresent:: ZwQueryValueKey() Failed with status %x, KeyPathName %S KeyValueName %S! \n", 
									status, KeyPathName, KeyValueName));
		}
 
        ZwClose(keyHandle);

		if (!pInformation) 
			OS_FreeMemory(pInformation);

	}
    return status;
} // sftk_IsRegKeyPresent()

NTSTATUS 
sftk_CheckRegKeyPresent( CONST PWCHAR KeyPathName)	// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
{ 
	// OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);
	return RtlCheckRegistryKey(	RTL_REGISTRY_SERVICES, KeyPathName); 
} // sftk_CheckRegKeyPresent()

NTSTATUS 
sftk_CreateRegistryKey(	CONST PWCHAR KeyPathName )
{ 
	// OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);
	return RtlCreateRegistryKey(	RTL_REGISTRY_SERVICES, KeyPathName);
} // sftk_CreateRegistryKey()


NTSTATUS 
sftk_RegistryKey_DeleteValue(	CONST PWCHAR KeyPathName, 
								CONST PWCHAR KeyValueName)
{ 
	// OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);
	return RtlDeleteRegistryValue(	RTL_REGISTRY_SERVICES, 
									KeyPathName, 
									KeyValueName); 
} // sftk_RegistryKey_DeleteValue()

NTSTATUS 
sftk_RegistryKey_DeleteKey(		CONST PWCHAR KeyPathName)	// L"sftk_block\\Parameters\\LGNum%08d"	// its Key
{ 
	NTSTATUS			status	  = STATUS_SUCCESS;
	HANDLE				keyHandle = NULL;
	OBJECT_ATTRIBUTES   attributes;     // attributes for the registry key
	UNICODE_STRING		keyPathName;

	OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);

	RtlInitUnicodeString(&keyPathName, KeyPathName);
	
	InitializeObjectAttributes(	&attributes, 
								&keyPathName, 
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 
								NULL, 
								NULL);

	// open the key
    status = ZwOpenKey( &keyHandle, KEY_ALL_ACCESS, &attributes);
    
    // if we succeeded, check for a value key
    if (!NT_SUCCESS(status)) 
	{ // success
		DebugPrint((DBG_ERROR, "sftk_RegistryKey_DeleteKey:: ZwOpenKey() Failed with status %x, KeyPathName %S ! \n", 
							status, KeyPathName));
		goto done;
	}

	status = ZwDeleteKey(keyHandle);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_RegistryKey_DeleteKey:: ZwDeleteKey() Failed with status %x, KeyPathName %S ! \n", 
							status, KeyPathName));
		goto done;
	}
done:

	if (keyHandle != NULL)
		ZwClose(keyHandle);
	return status;
} // sftk_RegistryKey_DeleteKey()

NTSTATUS 
sftk_GetRegKey( CONST	PWCHAR	KeyPathName,		// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
				CONST	PWCHAR	KeyValueName,		// like L"LGNumber"
						ULONG	ValueType,			// REG_DWORD, REG_SZ, etc..
						PVOID	ValueData,			// Actual Data
						ULONG	ValueLength)		// sizeof(ULONG), etc..	Data size
{
	RTL_QUERY_REGISTRY_TABLE paramTable[2];

	// OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);

	RtlZeroMemory(&paramTable[0], 2 * sizeof(RTL_QUERY_REGISTRY_TABLE));

	paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | 
								  RTL_QUERY_REGISTRY_REQUIRED;
	if (ValueType == REG_BINARY)
	{
		paramTable[0].Flags         = RTL_QUERY_REGISTRY_REQUIRED;
		paramTable[0].QueryRoutine  = QueryRegistryValue;
		paramTable[0].Name          = KeyValueName;
		paramTable[0].EntryContext  = &ValueLength;
		paramTable[0].DefaultType   = ValueType;
		paramTable[0].DefaultData   = NULL;
		paramTable[0].DefaultLength = ValueLength;

		paramTable[1].QueryRoutine  = NULL;
		paramTable[1].Name          = NULL;

		return RtlQueryRegistryValues(	RTL_REGISTRY_SERVICES,
										KeyPathName,
										&paramTable[0],
										ValueData,
										NULL);
	} // if (ValueType == REG_BINARY)
	else
	{ // for all other type of value
		paramTable[0].DefaultType   = ValueType;
		paramTable[0].DefaultData   = NULL;
		paramTable[0].DefaultLength = ValueLength;
		paramTable[0].EntryContext  = ValueData;
		paramTable[0].Name          = KeyValueName;

		paramTable[1].QueryRoutine  = NULL;
		paramTable[1].Name          = NULL;

		return RtlQueryRegistryValues(	RTL_REGISTRY_SERVICES,
										KeyPathName,
										&paramTable[0],
										NULL,
										NULL);
	}
} // sftk_GetRegKey()

NTSTATUS 
QueryRegistryValue(	PWSTR	ValueName, 
					ULONG	ValueType,
					PVOID	ValueData,
					ULONG	ValueLength,
					PVOID	Context,
					PVOID	EntryContext)
{
	PUCHAR	pData = (PUCHAR) Context;
	PUCHAR	pValue = (PUCHAR) ValueData;

	ULONG dataLen=*((ULONG *) EntryContext);

	if(ValueType==REG_SZ || ValueType==REG_MULTI_SZ)
	{
		ULONG i;

		if(dataLen<ValueLength / 2)
		{
			*((ULONG *) EntryContext)=0;
			return STATUS_BUFFER_TOO_SMALL;
		}

		for(i=0; i<ValueLength/2; i++)
			pData[i] = pValue[i*2];

		*(ULONG *) EntryContext = ValueLength / 2;
		return STATUS_SUCCESS;
	}

	if(dataLen < ValueLength)
	{
		*((ULONG *) EntryContext)=0;
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlCopyMemory(pData, pValue, ValueLength);
	* (ULONG *) EntryContext = ValueLength;
	
	return STATUS_SUCCESS;
} // QueryRegistryValue

NTSTATUS 
sftk_SetRegKey( CONST	PWCHAR	KeyPathName,		// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
				CONST	PWCHAR	KeyValueName,		// like L"LGNumber"		
						ULONG	ValueType,			// REG_DWORD, REG_SZ, etc..
						PVOID	ValueData,			// Actual Data
						ULONG	ValueLength)		// sizeof(ULONG), etc..	Data size
{
	// OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL);
	return RtlWriteRegistryValue(	RTL_REGISTRY_SERVICES, 
									KeyPathName,		//	L"FSTState\\RunningConfiguration", 
									KeyValueName,		// like L"LGNumber",
									ValueType,			
									ValueData, 
									ValueLength);
} // sftk_SetRegKey()


NTSTATUS
sftk_lg_Create_RegKey(PSFTK_LG Sftk_Lg)
{
	NTSTATUS	status				= STATUS_SUCCESS;
	BOOLEAN		bCreatedNewKey		= FALSE;
	BOOLEAN		bTotalLgCountUpdated= FALSE;
	ULONG		totalLg				= 0;
	ULONG		lastShutdown		= TRUE;	// success at create time by default since Pstore file will be there 
	PSFTK_DEV	pSftkDev			= NULL;
	PLIST_ENTRY	plistEntry			= NULL;
	WCHAR		lgNumPathKey[80], lgNumKey[30];

	
	// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
	if (Sftk_Lg->Role.CreationRole == PRIMARY)
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Lg->LGroupNumber);
	else
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Lg->LGroupNumber);
#else
	swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Lg->LGroupNumber);
#endif
	status = sftk_CheckRegKeyPresent(lgNumPathKey);
	if (!NT_SUCCESS(status))
	{ // create LG root key
		status = sftk_CreateRegistryKey(lgNumPathKey);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_CreateRegistryKey(%S) Failed  with status %08x! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, status));
			return status;	// nothing to cleanup at this stage
		}
		bCreatedNewKey = TRUE;
	}

	// create and update each required values under LG Reg key
	// REG_KEY_LGNumber
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_LGNumber, REG_DWORD, 
							 &Sftk_Lg->LGroupNumber,sizeof(Sftk_Lg->LGroupNumber));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_LGNumber, Sftk_Lg->LGroupNumber, status));
		goto done;
	}

#if TARGET_SIDE
	// REG_KEY_CreationRole
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_CreationRole, REG_DWORD, 
							 &Sftk_Lg->Role.CreationRole, sizeof(Sftk_Lg->Role.CreationRole));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_CreationRole, Sftk_Lg->Role.CreationRole, status));
		goto done;
	}
	// REG_KEY_PreviouseRole
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_PreviouseRole, REG_DWORD, 
							 &Sftk_Lg->Role.PreviouseRole,sizeof(Sftk_Lg->Role.PreviouseRole));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_PreviouseRole, Sftk_Lg->Role.PreviouseRole, status));
		goto done;
	}
	// REG_KEY_CurrentRole
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_CurrentRole, REG_DWORD, 
							 &Sftk_Lg->Role.CurrentRole,sizeof(Sftk_Lg->Role.CurrentRole));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_CurrentRole, Sftk_Lg->Role.CurrentRole, status));
		goto done;
	}
	// REG_KEY_FailOver
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_FailOver, REG_DWORD, 
							 &Sftk_Lg->Role.FailOver,sizeof(Sftk_Lg->Role.FailOver));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_FailOver, Sftk_Lg->Role.FailOver, status));
		goto done;
	}
	// REG_KEY_JEnable
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_JEnable, REG_DWORD, 
							 &Sftk_Lg->Role.JEnable,sizeof(Sftk_Lg->Role.JEnable));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JEnable, Sftk_Lg->Role.JEnable, status));
		goto done;
	}
	// REG_KEY_JApplyRunning
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_JApplyRunning, REG_DWORD, 
							 &Sftk_Lg->Role.JApplyRunning,sizeof(Sftk_Lg->Role.JApplyRunning));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JApplyRunning, Sftk_Lg->Role.JApplyRunning, status));
		goto done;
	}
	// REG_KEY_JPath
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_JPath, REG_SZ, 
							 Sftk_Lg->Role.JPathUnicode.Buffer,(Sftk_Lg->Role.JPathUnicode.Length + 2));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %S) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JPath, Sftk_Lg->Role.JPathUnicode.Buffer, status));
		goto done;
	}
#endif // #if TARGET_SIDE

	// REG_KEY_TotalDev
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_TotalDev, REG_DWORD, 
							 &Sftk_Lg->LgDev_List.NumOfNodes,sizeof(Sftk_Lg->LgDev_List.NumOfNodes));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_TotalDev, Sftk_Lg->LgDev_List.NumOfNodes, status));
		goto done;
	}
	// REG_KEY_PstoreFile
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_PstoreFile, REG_SZ, 
							 Sftk_Lg->PStoreFileName.Buffer,(Sftk_Lg->PStoreFileName.Length + 2));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %S) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_PstoreFile, Sftk_Lg->PStoreFileName.Buffer, status));
		goto done;
	}

	// REG_KEY_LastShutdown
	lastShutdown = TRUE;
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_LastShutdown, REG_DWORD, 
							 &lastShutdown,sizeof(lastShutdown));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_LastShutdown, lastShutdown, status));
		goto done;
	}

	// Now Create Each and every Src Device Information List Reg Keys under current LG 
	for( plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
		 plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every Devs under logical group list 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		status = sftk_dev_Create_RegKey( pSftkDev );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_dev_Create_RegKey(DevNum %d %s) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, pSftkDev->cdev, pSftkDev->Vdevname, status));
			goto done;
		}
	} // for :scan thru each and every Devs under logical group list 

	if (bCreatedNewKey == TRUE)
	{ // update Total LG counter
		// REG_KEY_TotalLG
#if TARGET_SIDE
		if (Sftk_Lg->Role.CreationRole == PRIMARY)
			status = sftk_GetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, 
									REG_DWORD, &totalLg, sizeof(totalLg));
		else
			status = sftk_GetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, 
									REG_DWORD, &totalLg, sizeof(totalLg));
#else
		status = sftk_GetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, 
									REG_DWORD, &totalLg, sizeof(totalLg));
#endif
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %s Value %d) Failed with status %08x! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, "TotalLG Counter Value", totalLg, status));
		}

		totalLg ++;
#if TARGET_SIDE
		if (Sftk_Lg->Role.CreationRole == PRIMARY)
			status = sftk_SetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, REG_DWORD, 
										&totalLg, sizeof(totalLg));
		else
			status = sftk_SetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, REG_DWORD, 
										&totalLg, sizeof(totalLg));
#else
		status = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, REG_DWORD, 
								 &totalLg, sizeof(totalLg));
#endif
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %s %d) Failed with status %08x! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, "TotalLG Counter Value", totalLg, status));
			goto done;
		}
		bTotalLgCountUpdated = TRUE;
	} // update Total LG counter

	status = STATUS_SUCCESS;

done:
	if (!NT_SUCCESS(status))
	{ // Error: Failed, do cleanup here
		NTSTATUS	tmpStatus;	// for debugging only

		// if (bCreatedNewKey == TRUE)
		{ // Delete LG Key Which will delete its all child Src DEvice key too.
			WCHAR		pathKey[350], lgNumKey2[100];
			// Delete LG Key Which will delete its all child Src DEvice key too.

			// REG_KEY_LGNum_KEY_ROOT L"LGNum%08d"				// its Key
			wcscpy( pathKey, REG_SFTK_BLOCK_REGISTRY_DIRECT_PARAMETERS); // REG_SFTK_BLOCK_DIRECT_PARAMETERS
			wcscat( pathKey, REG_SFTK_SLASH);
#if TARGET_SIDE
			if (Sftk_Lg->Role.CreationRole == PRIMARY)
				swprintf( lgNumKey2, REG_KEY_LGNum_KEY_ROOT,  REG_PRIMARY_UNICODE, Sftk_Lg->LGroupNumber);
			else
				swprintf( lgNumKey2, REG_KEY_LGNum_KEY_ROOT,  REG_SECONDARY_UNICODE, Sftk_Lg->LGroupNumber);
#else
			swprintf( lgNumKey2, REG_KEY_LGNum_KEY_ROOT,  Sftk_Lg->LGroupNumber);
#endif
			wcscat( pathKey, lgNumKey2);

			// pathkey = L"SYSTEM\\CurrentControlSet\\Services\\sftkblk\\Parameters\\LGNum%08d"
			tmpStatus = sftk_RegistryKey_DeleteKey( pathKey);
			if (!NT_SUCCESS(tmpStatus))
			{
				DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: LG Num %d sftk_RegistryKey_DeleteKey(%S ) Failed with tmpStatus %08x! \n", 
								Sftk_Lg->LGroupNumber, pathKey, tmpStatus));
			}
		} // Delete LG Key Which will delete its all child Src DEvice key too.

		if(bTotalLgCountUpdated == TRUE)
		{
			totalLg = 0;
			// REG_KEY_TotalLG
#if TARGET_SIDE
			if (Sftk_Lg->Role.CreationRole == PRIMARY)
				tmpStatus = sftk_GetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, 
											REG_DWORD, &totalLg, sizeof(totalLg) );
			else
				tmpStatus = sftk_GetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, 
											REG_DWORD, &totalLg, sizeof(totalLg) );
#else
			tmpStatus = sftk_GetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, 
										REG_DWORD, &totalLg, sizeof(totalLg) );
#endif
			if (NT_SUCCESS(tmpStatus))
			{ // success
				if (totalLg > 0)
					totalLg --;
#if TARGET_SIDE
			if (Sftk_Lg->Role.CreationRole == PRIMARY)
				tmpStatus = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, REG_DWORD, 
											&totalLg, sizeof(totalLg));
			else
				tmpStatus = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, REG_DWORD, 
											&totalLg, sizeof(totalLg));
#else
				tmpStatus = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, REG_DWORD, 
											&totalLg, sizeof(totalLg));
#endif
				if (!NT_SUCCESS(tmpStatus))
				{
					DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %s Value %d) Failed with tmpStatus %08x! \n", 
										Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, "TotalLG Counter Value", totalLg, tmpStatus));
				}
			} // success
			else
			{ // failed
				DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %s Value %d) Failed with tmpStatus %08x! \n", 
									Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, "REG_KEY_TotalLG", totalLg, tmpStatus));
			}
		}
	} // Error: Failed, do cleanup here
	
	return status;
} // sftk_lg_Create_RegKey()

NTSTATUS
sftk_lg_Delete_RegKey(PSFTK_LG Sftk_Lg)
{
	NTSTATUS	status				= STATUS_SUCCESS;
	ULONG		totalLg				= 0;
	WCHAR		pathKey[350], lgNumKey[100];

	// Delete LG Key Which will delete its all child Src DEvice key too.

	// REG_KEY_LGNum_KEY_ROOT L"LGNum%08d"				// its Key
	wcscpy( pathKey, REG_SFTK_BLOCK_REGISTRY_DIRECT_PARAMETERS);
	wcscat( pathKey, REG_SFTK_SLASH);
#if TARGET_SIDE
	if (Sftk_Lg->Role.CreationRole == PRIMARY)
		swprintf( lgNumKey, REG_KEY_LGNum_KEY_ROOT,  REG_PRIMARY_UNICODE, Sftk_Lg->LGroupNumber);
	else
		swprintf( lgNumKey, REG_KEY_LGNum_KEY_ROOT,  REG_SECONDARY_UNICODE, Sftk_Lg->LGroupNumber);
#else
	swprintf( lgNumKey, REG_KEY_LGNum_KEY_ROOT,  Sftk_Lg->LGroupNumber);
#endif
	wcscat( pathKey, lgNumKey);

	// pathkey = L"SYSTEM\\CurrentControlSet\\Services\\sftkblk\\Parameters\\LGNum%08d"
	status = sftk_RegistryKey_DeleteKey( pathKey);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Delete_RegKey:: LG Num %d sftk_RegistryKey_DeleteKey(%S ) Failed with status %08x! \n", 
						Sftk_Lg->LGroupNumber, pathKey, status));
		return status;
	}
	/*
	status = sftk_RegistryKey_DeleteValue( REG_SFTK_BLOCK_DRIVER_PARAMETERS, lgNumKey);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Delete_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_RegistryKey_DeleteValue(%S : KeyValue to Delete %S) Failed with status %08x! \n", 
						Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, lgNumKey, status));
		return status;
	}
	*/
	
	// REG_KEY_TotalLG
#if TARGET_SIDE
	if (Sftk_Lg->Role.CreationRole == PRIMARY)
		status = sftk_GetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, 
							REG_DWORD, &totalLg, sizeof(totalLg));
	else
		status = sftk_GetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, 
							REG_DWORD, &totalLg, sizeof(totalLg));
#else
	status = sftk_GetRegKey(	REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, 
								REG_DWORD, &totalLg, sizeof(totalLg));
#endif
	if (NT_SUCCESS(status))
	{ // success
		if (totalLg > 0)
			totalLg --;
#if TARGET_SIDE
		if (Sftk_Lg->Role.CreationRole == PRIMARY)
			status = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, REG_DWORD, 
									&totalLg, sizeof(totalLg));
		else
			status = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, REG_DWORD, 
									&totalLg, sizeof(totalLg));
#else
		status = sftk_SetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, REG_DWORD, 
									&totalLg, sizeof(totalLg));
#endif
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Delete_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %s Value %d) Failed with status %08x! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, "REG_KEY_TotalPLG", totalLg, status));
			return status;
		}
	} // success
	else
	{ // failed
		DebugPrint((DBG_ERROR, "sftk_lg_Delete_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %s Value %d) Failed with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, REG_SFTK_BLOCK_DRIVER_PARAMETERS, "REG_KEY_TotalPLG", totalLg, status));
	}
	
	return status;
} // sftk_lg_Delete_RegKey()

NTSTATUS
sftk_lg_update_lastshutdownKey(PSFTK_LG Sftk_Lg, ULONG LastShutDown)
{
	NTSTATUS	status				= STATUS_SUCCESS;
	WCHAR		lgNumPathKey[80];

	// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
	if (Sftk_Lg->Role.CreationRole == PRIMARY)
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Lg->LGroupNumber);
	else
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Lg->LGroupNumber);
#else
	swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Lg->LGroupNumber);
#endif

	// REG_KEY_LastShutdown
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_LastShutdown, REG_DWORD, 
							 &LastShutDown,sizeof(LastShutDown));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_update_lastshutdownKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_LastShutdown, LastShutDown, status));
	}

	return status;
} // sftk_lg_update_lastshutdownKey

NTSTATUS
sftk_lg_update_PstoreFileNameKey(PSFTK_LG Sftk_Lg, PWCHAR PstoreFileName)
{
	NTSTATUS	status				= STATUS_SUCCESS;
	WCHAR		lgNumPathKey[80];
	UNICODE_STRING	unicode_string;

	// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
	if (Sftk_Lg->Role.CreationRole == PRIMARY)
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Lg->LGroupNumber);
	else
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Lg->LGroupNumber);
#else
	swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Lg->LGroupNumber);
#endif

	RtlInitUnicodeString( &unicode_string, PstoreFileName);
	// REG_KEY_LastShutdown
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_PstoreFile, REG_SZ,
							unicode_string.Buffer,(unicode_string.Length + 2));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_update_PstoreFileNameKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %S, Length %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_PstoreFile, 
							unicode_string.Buffer, unicode_string.Length, status));
	}

	return status;
} // sftk_lg_update_PstoreFileNameKey

#if TARGET_SIDE
NTSTATUS
sftk_lg_update_RegRoleAndSecondaryInfo( PSFTK_LG Sftk_Lg)
{
	NTSTATUS	status	= STATUS_SUCCESS;
	WCHAR		lgNumPathKey[80];

	// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
	if (Sftk_Lg->Role.CreationRole == PRIMARY)
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Lg->LGroupNumber);
	else
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Lg->LGroupNumber);
#else
	swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Lg->LGroupNumber);
#endif

	/*
	// REG_KEY_CreationRole
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_CreationRole, REG_DWORD, 
							 &Sftk_Lg->Role.CreationRole, sizeof(Sftk_Lg->Role.CreationRole));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_CreationRole, Sftk_Lg->Role.CreationRole, status));
		goto done;
	}
	*/
	// REG_KEY_PreviouseRole
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_PreviouseRole, REG_DWORD, 
							 &Sftk_Lg->Role.PreviouseRole,sizeof(Sftk_Lg->Role.PreviouseRole));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_PreviouseRole, Sftk_Lg->Role.PreviouseRole, status));
		goto done;
	}
	// REG_KEY_CurrentRole
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_CurrentRole, REG_DWORD, 
							 &Sftk_Lg->Role.CurrentRole,sizeof(Sftk_Lg->Role.CurrentRole));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_CurrentRole, Sftk_Lg->Role.CurrentRole, status));
		goto done;
	}
	// REG_KEY_FailOver
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_FailOver, REG_DWORD, 
							 &Sftk_Lg->Role.FailOver,sizeof(Sftk_Lg->Role.FailOver));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_FailOver, Sftk_Lg->Role.FailOver, status));
		goto done;
	}
	// REG_KEY_JEnable
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_JEnable, REG_DWORD, 
							 &Sftk_Lg->Role.JEnable,sizeof(Sftk_Lg->Role.JEnable));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JEnable, Sftk_Lg->Role.JEnable, status));
		goto done;
	}
	// REG_KEY_JApplyRunning
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_JApplyRunning, REG_DWORD, 
							 &Sftk_Lg->Role.JApplyRunning,sizeof(Sftk_Lg->Role.JApplyRunning));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JApplyRunning, Sftk_Lg->Role.JApplyRunning, status));
		goto done;
	}
	// REG_KEY_JPath
	status = sftk_SetRegKey( lgNumPathKey, REG_KEY_JPath, REG_SZ, 
							 Sftk_Lg->Role.JPathUnicode.Buffer,(Sftk_Lg->Role.JPathUnicode.Length + 2));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_SetRegKey(%S : %S, Value %S) Failed  with status %08x! \n", 
							Sftk_Lg, Sftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JPath, Sftk_Lg->Role.JPathUnicode.Buffer, status));
		goto done;
	}

done:
	return status;
} // sftk_lg_update_RegRoleAndSecondaryInfo
#endif


NTSTATUS
sftk_dev_Create_RegKey(PSFTK_DEV Sftk_Dev)
{
	NTSTATUS		status					= STATUS_SUCCESS;
	BOOLEAN			bCreatedNewKey			= FALSE;
	BOOLEAN			bTotalDevCounterUpdated = TRUE;
	ULONG			totalDev				= 0;
	WCHAR			devNumPathKey[128], unicodeBuffer[256], devlgNumKey[80];
	UNICODE_STRING	unicode_string;
	ANSI_STRING		ansi_string;
	PREG_DEV_INFO	pRegDevInfo = NULL;
	PUCHAR			pBuffer;
	ULONG			sizeOfBuffer;

	OS_ASSERT(Sftk_Dev->SftkLg != NULL);

	// REG_KEY_DevNum_STRING_ROOT L"sftkblk\\Parameters\\LGNum%08d\\DevNum%08d"	// its Key
#if TARGET_SIDE
	if (Sftk_Dev->SftkLg->Role.CreationRole == PRIMARY)
		swprintf( devNumPathKey, REG_KEY_DevNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
	else
		swprintf( devNumPathKey, REG_KEY_DevNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
#else
	swprintf( devNumPathKey, REG_KEY_DevNum_STRING_ROOT,  Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
#endif

	status = sftk_CheckRegKeyPresent(devNumPathKey);
	if (!NT_SUCCESS(status))
	{ // create LG root key
		status = sftk_CreateRegistryKey(devNumPathKey);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d DevNum %d sftk_CreateRegistryKey(%S) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, status));
			return status;	// nothing to do cleanup at this stage
		}
		bCreatedNewKey = TRUE;
	}
#if 1
	// Allocate Binary structures for each device 
	pRegDevInfo = OS_AllocMemory(PagedPool, 4096);
	if (pRegDevInfo == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d OS_AllocMemory(size %d) Failed for pRegDevInfo returning status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, 4096, status));
		goto done;
	}
	// Initialize this Binary structures
	RtlZeroMemory(pRegDevInfo, 4096);

	if(Sftk_Dev->SftkLg)
		pRegDevInfo->LgNum = Sftk_Dev->SftkLg->LGroupNumber;
	else
		pRegDevInfo->LgNum = Sftk_Dev->LGroupNumber;

	pRegDevInfo->cDev	  = Sftk_Dev->cdev;
	pRegDevInfo->Disksize = Sftk_Dev->Disksize;
	pRegDevInfo->Lrdbsize = Sftk_Dev->Lrdb.len32;
	pRegDevInfo->Hrdbsize = Sftk_Dev->Hrdb.len32;
	pRegDevInfo->Statsize = Sftk_Dev->statsize;

	pRegDevInfo->DevNameLength					= (USHORT) strlen(Sftk_Dev->Devname);
	pRegDevInfo->VDevNameLength					= (USHORT) strlen(Sftk_Dev->Vdevname);
	pRegDevInfo->strRemoteDeviceNameLength		= (USHORT) strlen(Sftk_Dev->strRemoteDeviceName);

	pRegDevInfo->bUniqueVolumeIdValid			= Sftk_Dev->bUniqueVolumeIdValid;
	pRegDevInfo->UniqueIdLength					= Sftk_Dev->UniqueIdLength;

	pRegDevInfo->bSignatureUniqueVolumeIdValid	= Sftk_Dev->bSignatureUniqueVolumeIdValid;
	pRegDevInfo->SignatureUniqueIdLength		= Sftk_Dev->SignatureUniqueIdLength;

	pRegDevInfo->bSuggestedDriveLetterLinkValid = Sftk_Dev->bSuggestedDriveLetterLinkValid;
	pRegDevInfo->SuggestedDriveLetterLinkLength = Sftk_Dev->SuggestedDriveLetterLinkLength;

	pBuffer = (PUCHAR) ((ULONG) pRegDevInfo + sizeof(REG_DEV_INFO));
	sizeOfBuffer = sizeof(REG_DEV_INFO);

	RtlCopyMemory( pBuffer, Sftk_Dev->Devname, pRegDevInfo->DevNameLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->DevNameLength);
	sizeOfBuffer += pRegDevInfo->DevNameLength;

	RtlCopyMemory( pBuffer, Sftk_Dev->Vdevname, pRegDevInfo->VDevNameLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->VDevNameLength);
	sizeOfBuffer += pRegDevInfo->VDevNameLength;

	RtlCopyMemory( pBuffer, Sftk_Dev->strRemoteDeviceName, pRegDevInfo->strRemoteDeviceNameLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->strRemoteDeviceNameLength);
	sizeOfBuffer += pRegDevInfo->strRemoteDeviceNameLength;

	RtlCopyMemory( pBuffer, Sftk_Dev->UniqueId, pRegDevInfo->UniqueIdLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->UniqueIdLength);
	sizeOfBuffer += pRegDevInfo->UniqueIdLength;

	RtlCopyMemory( pBuffer, Sftk_Dev->SignatureUniqueId, pRegDevInfo->SignatureUniqueIdLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->SignatureUniqueIdLength);
	sizeOfBuffer += pRegDevInfo->SignatureUniqueIdLength;

	RtlCopyMemory( pBuffer, Sftk_Dev->SuggestedDriveLetterLink, pRegDevInfo->SuggestedDriveLetterLinkLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->SuggestedDriveLetterLinkLength);
	sizeOfBuffer += pRegDevInfo->SuggestedDriveLetterLinkLength;

	// REG_KEY_DevInfo : Update Registry
	status = sftk_SetRegKey( devNumPathKey, REG_KEY_DevInfo, REG_BINARY, 
							 pRegDevInfo,sizeOfBuffer);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, REG_BINARY Value 0x%08x, size %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_DevInfo, pRegDevInfo, sizeOfBuffer, status));
		goto done;
	}
#else
	/*
	// create and update each required values under LG Reg key
	// REG_KEY_DevNumber
	status = sftk_SetRegKey( devNumPathKey, REG_KEY_DevNumber, REG_DWORD, 
							 &Sftk_Dev->cdev,sizeof(Sftk_Dev->cdev));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_DevNumber, Sftk_Dev->cdev, status));
		goto done;
	}

	// REG_KEY_VolumeNumber
	if (Sftk_Dev->DevExtension != NULL)
	{
		status = sftk_SetRegKey( devNumPathKey, REG_KEY_VolumeNumber, REG_DWORD, 
								 &Sftk_Dev->DevExtension->DeviceInfo.VolumeNumber.VolumeNumber,
								 sizeof(Sftk_Dev->DevExtension->DeviceInfo.VolumeNumber.VolumeNumber));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
								REG_KEY_VolumeNumber, Sftk_Dev->DevExtension->DeviceInfo.VolumeNumber.VolumeNumber, status));
			goto done;
		}

		// REG_KEY_DiskVolumeName 
		RtlInitUnicodeString( &unicode_string, Sftk_Dev->DevExtension->DeviceInfo.DiskVolumeName);
		status = sftk_SetRegKey( devNumPathKey, REG_KEY_DiskVolumeName, REG_SZ, 
								 unicode_string.Buffer, unicode_string.Length );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %S, Length %d) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_DiskVolumeName, 
								unicode_string.Buffer, unicode_string.Length, status));
			// goto done;
		}

	} // if (Sftk_Dev->DevExtension != NULL)

	// REG_KEY_UniqueId
	RtlInitAnsiString( &ansi_string, Sftk_Dev->UniqueId);
	OS_ZeroMemory( unicodeBuffer, sizeof(unicodeBuffer));
	unicode_string.Buffer		= unicodeBuffer;
	unicode_string.MaximumLength= sizeof(unicodeBuffer);
	unicode_string.Length		= 0;

	status = RtlAnsiStringToUnicodeString( &unicode_string, &ansi_string, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d RtlAnsiStringToUnicodeString(%S : %S, Value %s, Length %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_UniqueId, ansi_string.Buffer, Sftk_Dev->UniqueIdLength, status));
		goto done;
	}
	status = sftk_SetRegKey( devNumPathKey, REG_KEY_UniqueId, REG_SZ, 
							 unicode_string.Buffer, unicode_string.Length);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %S, Length %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_UniqueId, unicode_string.Buffer, unicode_string.Length, status));
		goto done;
	}

	// REG_KEY_SignatureUniqueId
	RtlInitAnsiString( &ansi_string, Sftk_Dev->SignatureUniqueId);
	OS_ZeroMemory( unicodeBuffer, sizeof(unicodeBuffer));
	unicode_string.Buffer		= unicodeBuffer;
	unicode_string.MaximumLength= sizeof(unicodeBuffer);
	unicode_string.Length		= 0;

	status = RtlAnsiStringToUnicodeString( &unicode_string, &ansi_string, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d RtlAnsiStringToUnicodeString(%S : %S, Value %s, Length %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_UniqueId, ansi_string.Buffer, Sftk_Dev->UniqueIdLength, status));
		goto done;
	}

	status = sftk_SetRegKey( devNumPathKey, REG_KEY_SignatureUniqueId, REG_SZ, 
							 unicode_string.Buffer, unicode_string.Length);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %S, Length %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_SignatureUniqueId, unicode_string.Buffer, unicode_string.Length, status));
		goto done;
	}

	// REG_KEY_VDevName
	RtlInitAnsiString( &ansi_string, Sftk_Dev->Vdevname);

	OS_ZeroMemory(unicodeBuffer, sizeof(unicodeBuffer));

	unicode_string.Buffer		= unicodeBuffer;
	unicode_string.MaximumLength= sizeof(unicodeBuffer);
	unicode_string.Length		= 0;
	
	status = RtlAnsiStringToUnicodeString( &unicode_string, &ansi_string, FALSE);
	if (NT_SUCCESS(status))
	{	// REG_KEY_VDevName
		status = sftk_SetRegKey( devNumPathKey, REG_KEY_VDevName, REG_SZ, 
								 unicode_string.Buffer, unicode_string.Length);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %S, Length %d) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
								REG_KEY_VDevName, unicode_string.Buffer, unicode_string.Length, status));
			goto done;
		}
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d RtlAnsiStringToUnicodeString(Vdevname : %s) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, Sftk_Dev->Vdevname, status));
	}

	// REG_KEY_DevName
	RtlInitAnsiString( &ansi_string, Sftk_Dev->Devname);

	OS_ZeroMemory(unicodeBuffer, sizeof(unicodeBuffer));

	unicode_string.Buffer		= unicodeBuffer;
	unicode_string.MaximumLength= sizeof(unicodeBuffer);
	unicode_string.Length		= 0;
	
	status = RtlAnsiStringToUnicodeString( &unicode_string, &ansi_string, FALSE);
	if (NT_SUCCESS(status))
	{	// REG_KEY_DevName
		status = sftk_SetRegKey( devNumPathKey, REG_KEY_DevName, REG_SZ, 
								 unicode_string.Buffer, unicode_string.Length);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %S, Length %d) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
								REG_KEY_DevName, unicode_string.Buffer, unicode_string.Length, status));
			goto done;
		}
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d RtlAnsiStringToUnicodeString(Devname : %s) Failed  with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, Sftk_Dev->Devname, status));
	}

	// REG_KEY_DiskSize
	status = sftk_SetRegKey( devNumPathKey, REG_KEY_DiskSize, REG_DWORD, 
							 &Sftk_Dev->Disksize,sizeof(Sftk_Dev->Disksize));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_DiskSize, Sftk_Dev->Disksize, status));
		goto done;
	}

	// REG_KEY_Lrdbsize
	status = sftk_SetRegKey( devNumPathKey, REG_KEY_Lrdbsize, REG_DWORD, 
							 &Sftk_Dev->Lrdb.len32,sizeof(Sftk_Dev->Lrdb.len32));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_Lrdbsize, Sftk_Dev->Lrdb.len32, status));
		goto done;
	}

	// REG_KEY_Hrdbsize
	status = sftk_SetRegKey( devNumPathKey, REG_KEY_Hrdbsize, REG_DWORD, 
							 &Sftk_Dev->Hrdb.len32,sizeof(Sftk_Dev->Hrdb.len32));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d, DevNum %d sftk_SetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, 
							REG_KEY_Hrdbsize, Sftk_Dev->Hrdb.len32, status));
		goto done;
	}
	*/
#endif // # if 1 #else

	if (bCreatedNewKey == TRUE)
	{ // update Total Dev counter under LG Reg Key : REG_KEY_TotalDev
		// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
		if (Sftk_Dev->SftkLg->Role.CreationRole == PRIMARY)
			swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber);
		else
			swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber);
#else
		swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Dev->SftkLg->LGroupNumber);
#endif
		totalDev = 0;
		// REG_KEY_TotalDev
		status = sftk_GetRegKey( devNumPathKey, REG_KEY_TotalDev, 
								 REG_DWORD, &totalDev, sizeof(totalDev));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d DevNum %d sftk_GetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_TotalDev, totalDev, status));
		}

		totalDev ++;
		status = sftk_SetRegKey( devNumPathKey, REG_KEY_TotalDev, REG_DWORD, 
								 &totalDev, sizeof(totalDev));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d DevNum %d sftk_SetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_TotalDev, totalDev, status));
			goto done;
		}
		bTotalDevCounterUpdated = TRUE;
	} // update Total Dev counter under LG Reg Key : REG_KEY_TotalDev

	status = STATUS_SUCCESS;

done:

	if (!NT_SUCCESS(status))
	{ // Error: Failed, do cleanup here
		NTSTATUS	tmpStatus;	// for debugging only

		// if (bCreatedNewKey == TRUE)
		{ // Delete Dev Key Which will delete its all child Src DEvice key too.
			WCHAR	pathKey[350], lgNumKey[100];

			// Delete Dev Key Which will delete its all child Src DEvice key too.

			// REG_KEY_DevNum_LGNum_KEY_ROOT L"LGNum%S%08d\\DevNum%08d"	// its Key
			wcscpy( pathKey, REG_SFTK_BLOCK_REGISTRY_DIRECT_PARAMETERS);
			wcscat( pathKey, REG_SFTK_SLASH);
#if TARGET_SIDE
			if (Sftk_Dev->SftkLg->Role.CreationRole == PRIMARY)
				swprintf( lgNumKey, REG_KEY_DevNum_LGNum_KEY_ROOT,  REG_PRIMARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
			else
				swprintf( lgNumKey, REG_KEY_DevNum_LGNum_KEY_ROOT,  REG_SECONDARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
#else
			swprintf( lgNumKey, REG_KEY_DevNum_LGNum_KEY_ROOT,  Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
#endif
			wcscat( pathKey, lgNumKey);
			// pathkey = L"SYSTEM\\CurrentControlSet\\Services\\sftkblk\\Parameters\\LGNum%08d\\DevNum%08d"
			tmpStatus = sftk_RegistryKey_DeleteKey( pathKey);
			if (!NT_SUCCESS(tmpStatus))
			{
				DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d Dev %d sftk_RegistryKey_DeleteKey(%S ) Failed with tmpStatus %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, pathKey, tmpStatus));
			}
		} 

		if(bTotalDevCounterUpdated == TRUE)
		{
			totalDev = 0;

			// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
			if (Sftk_Dev->SftkLg->Role.CreationRole == PRIMARY)
				swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber);
			else
				swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber);
#else
			swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Dev->SftkLg->LGroupNumber);
#endif

			// REG_KEY_TotalDev
			tmpStatus = sftk_GetRegKey( devNumPathKey, REG_KEY_TotalDev, 
										REG_DWORD, &totalDev, sizeof(totalDev));
			if (NT_SUCCESS(tmpStatus))
			{ // success
				if (totalDev > 0)
					totalDev --;

				status = sftk_SetRegKey(	devNumPathKey, REG_KEY_TotalDev, REG_DWORD, 
											&totalDev, sizeof(totalDev));
				if (!NT_SUCCESS(tmpStatus))
				{
					DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d DevNum %d sftk_SetRegKey(%S : %S Value %d) Failed with tmpStatus %08x! \n", 
										Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_TotalDev, totalDev, tmpStatus));
				}
			} // success
			else
			{ // failed
				DebugPrint((DBG_ERROR, "sftk_dev_Create_RegKey:: LG Num %d DevNum %d sftk_GetRegKey(%S : %S Value %d) Failed with tmpStatus %08x! \n", 
									Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_TotalDev, totalDev, tmpStatus));
			}
		}
	} // Error: Failed, do cleanup here
	
	if(pRegDevInfo)
		OS_FreeMemory(pRegDevInfo);

	return status;
} // sftk_dev_Create_RegKey()

NTSTATUS
sftk_dev_Delete_RegKey(PSFTK_DEV Sftk_Dev)
{
	NTSTATUS	status				= STATUS_SUCCESS;
	ULONG		totalDev			= 0;
	WCHAR		devNumPathKey[128];
	WCHAR		pathKey[350], lgNumKey[100];

	// Delete Dev Key Which will delete its all child Src DEvice key too.

	// REG_KEY_DevNum_LGNum_KEY_ROOT L"LGNum%S%08d\\DevNum%08d"	// its Key
	wcscpy( pathKey, REG_SFTK_BLOCK_REGISTRY_DIRECT_PARAMETERS);
	wcscat( pathKey, REG_SFTK_SLASH);
#if TARGET_SIDE
	if (Sftk_Dev->SftkLg->Role.CreationRole == PRIMARY)
		swprintf( lgNumKey, REG_KEY_DevNum_LGNum_KEY_ROOT,  REG_PRIMARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
	else
		swprintf( lgNumKey, REG_KEY_DevNum_LGNum_KEY_ROOT,  REG_SECONDARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
#else			
	swprintf( lgNumKey, REG_KEY_DevNum_LGNum_KEY_ROOT,  Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev);
#endif
	wcscat( pathKey, lgNumKey);
	// pathkey = L"SYSTEM\\CurrentControlSet\\Services\\sftkblk\\Parameters\\LGNum%08d\\DevNum%08d"
	status = sftk_RegistryKey_DeleteKey( pathKey);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_dev_Delete_RegKey:: LG Num %d Dev %d sftk_RegistryKey_DeleteKey(%S ) Failed with status %08x! \n", 
						Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, pathKey, status));
		return status;
	}
	
	// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
	if (Sftk_Dev->SftkLg->Role.CreationRole == PRIMARY)
		swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber);
	else
		swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, Sftk_Dev->SftkLg->LGroupNumber);
#else
	swprintf( devNumPathKey, REG_KEY_LGNum_STRING_ROOT,  Sftk_Dev->SftkLg->LGroupNumber);
#endif

	// REG_KEY_TotalDev
	status = sftk_GetRegKey(	devNumPathKey, REG_KEY_TotalDev, 
								REG_DWORD, &totalDev, sizeof(totalDev));
	if (NT_SUCCESS(status))
	{ // success
		if (totalDev > 0)
			totalDev --;

		status = sftk_SetRegKey(	devNumPathKey, REG_KEY_TotalDev, REG_DWORD, 
									&totalDev, sizeof(totalDev));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_dev_Delete_RegKey:: LG Num %d DevNum %d sftk_SetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
								Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_TotalDev, totalDev, status));
			return status;
		}
	} // success
	else
	{ // failed
		DebugPrint((DBG_ERROR, "sftk_dev_Delete_RegKey:: LG Num %d DevNum %d sftk_GetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
							Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->cdev, devNumPathKey, REG_KEY_TotalDev, totalDev, status));
	}

	return status;
} // sftk_dev_Delete_RegKey()


NTSTATUS
sftk_configured_from_registry()
{
	NTSTATUS		status				= STATUS_SUCCESS;
	PSFTK_LG		pSftk_Lg			= NULL;
	PSFTK_DEV		pSftk_Dev			= NULL;
	ULONG			i, lgnum, j, devNum, totalLg, totalDev, lastShutdown;
	PLIST_ENTRY		plistEntry;
	WCHAR			lgNumPathKey[80], unicodeBuffer[256], unicodeBuffer1[256];
	CHAR			charPstoreFile[256], charJFile[256];
	ftd_lg_info_t	lg_info;
	ftd_dev_info_t  dev_info;
	UNICODE_STRING	unicode, unicode1;
	ANSI_STRING		ansiString, ansiString1;
	BOOLEAN			bCreatedOneDev, bvalidDevNum;
	
#if TARGET_SIDE
	BOOLEAN			bPrimaryLG = TRUE;
	UCHAR			bCounter = 1;
do 
	{
	totalLg = 0;

	if (bCounter == 1)
		bPrimaryLG = TRUE;
	else
		bPrimaryLG = FALSE;


	if (bPrimaryLG == TRUE)
	{ // first get total number of LG count 
		status = sftk_GetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, 
								REG_DWORD, &totalLg, sizeof(totalLg));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
									REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalPLG, totalLg, status));
			return STATUS_SUCCESS; // Nothing to do 
		}
	}
	else
	{ // Second time 'S' get total number of LG count 
		status = sftk_GetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, 
								REG_DWORD, &totalLg, sizeof(totalLg));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
									REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalSLG, totalLg, status));
			return STATUS_SUCCESS; // Nothing to do 
		}
	}
#else
	// first get total number of LG count 
	status = sftk_GetRegKey( REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, 
							REG_DWORD, &totalLg, sizeof(totalLg));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S Value %d) Failed with status %08x! \n", 
								REG_SFTK_BLOCK_DRIVER_PARAMETERS, REG_KEY_TotalLG, totalLg, status));
		return STATUS_SUCCESS; // Nothing to do 
	}
#endif

	for (lgnum=0, i=0; i < totalLg; lgnum++)
	{ // for : get each and every LG and configured it from registry

		// L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#if TARGET_SIDE
		if (bPrimaryLG == TRUE)
			swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_PRIMARY_UNICODE, lgnum);
		else
			swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  REG_SECONDARY_UNICODE, lgnum);
#else
		swprintf( lgNumPathKey, REG_KEY_LGNum_STRING_ROOT,  lgnum);
#endif

		status = sftk_CheckRegKeyPresent(lgNumPathKey);
		if (!NT_SUCCESS(status))
		{ // Check LG root key exist
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_CheckRegKeyPresent(%S) Failed  with status %08x! \n", 
									lgNumPathKey, status));
			// continue try other value...;
		}
		OS_ZeroMemory( &lg_info, sizeof(lg_info));

		// Get differnet information for LG
		// REG_KEY_LGNumber
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_LGNumber, REG_DWORD, 
								 &lg_info.lgdev,sizeof(lg_info.lgdev));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								 lgNumPathKey, REG_KEY_LGNumber, lg_info.lgdev, status));
			continue;
		}
		i++;	// bump up one counter 

		OS_ASSERT( lg_info.lgdev == lgnum );

		// REG_KEY_PstoreFile
		OS_ZeroMemory(charPstoreFile, sizeof(charPstoreFile));
		OS_ZeroMemory(unicodeBuffer, sizeof(unicodeBuffer));
		unicode.Buffer			= unicodeBuffer;
		unicode.Length			= 0;
		unicode.MaximumLength	= sizeof(unicodeBuffer);

		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_PstoreFile, REG_SZ, 
								 &unicode, unicode.MaximumLength);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S, Value %S) Failed  with status %08x! \n", 
								 lgNumPathKey, REG_KEY_PstoreFile, unicode.Buffer, status));
			continue;
		}

		// REG_KEY_LastShutdown
		lastShutdown = FALSE;
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_LastShutdown, REG_DWORD, 
								 &lastShutdown,sizeof(lastShutdown));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								 lgNumPathKey, REG_KEY_LastShutdown, lastShutdown, status));
			continue;
		}
		if (lastShutdown == FALSE)
		{ // Lastshutdown was not successful, Full recovery required
			// We may run this in PASSTRHU MODE, But its not needed causes  service starttime
			// we just create this LG and make it enable.
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d LastShutDown %d Was Unsuccessful, full recovery needed, ignoring this LG to create, at service startup time it will get create !!!\n", 
								 lgnum, lastShutdown));
			continue;
		}
		
		// REG_KEY_TotalDev
		totalDev = 0;
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_TotalDev, REG_DWORD, 
								 &totalDev,sizeof(totalDev));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								 lgNumPathKey, REG_KEY_TotalDev, totalDev, status));
			continue;
		}
		if (totalDev == 0)
		{ // Lastshutdown was not successful, Full recovery required
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d totalDev %d == 0, Nothing to track for this LG, ignoring this LG to create, at service startup time it will get create !!!\n", 
								 lgnum, totalDev));
			continue;
		}

		// Create LG here
		ansiString.Buffer			= charPstoreFile;
		ansiString.Length			= 0;
		ansiString.MaximumLength	= sizeof(charPstoreFile);

		status = RtlUnicodeStringToAnsiString( &ansiString, &unicode, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d RtlUnicodeStringToAnsiString(%S) Failed with status 0x%08x !!!\n", 
								 lgnum, unicode.Buffer, status));
			continue;
		}
		OS_ZeroMemory( lg_info.vdevname, sizeof(lg_info.vdevname));
		OS_RtlCopyMemory( lg_info.vdevname, ansiString.Buffer, ansiString.Length);
		lg_info.statsize = 4096;	// for time being keep this hard coded, TODO ::: 

#if TARGET_SIDE
		// REG_KEY_CreationRole
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_CreationRole, REG_DWORD, 
								 &lg_info.lgCreationRole, sizeof(lg_info.lgCreationRole));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: LG Num %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								lgnum, lg_info.lgCreationRole, lgNumPathKey, REG_KEY_CreationRole, lg_info.lgCreationRole, status));
			continue;
		}
		if ( (bPrimaryLG == TRUE) && (lg_info.lgCreationRole == SECONDARY) )
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: LG Num %d Failed with (bPrimaryLG == TRUE) && (lg_info.lgCreationRole == SECONDARY)! \n", 
								lgnum));
			OS_ASSERT(FALSE);
			lg_info.lgCreationRole = PRIMARY;
		}
		if ( (bPrimaryLG == FALSE) && (lg_info.lgCreationRole == PRIMARY) )
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: LG Num %d Failed with (bPrimaryLG == TRUE) && (lg_info.lgCreationRole == SECONDARY)! \n", 
								lgnum));
			OS_ASSERT(FALSE);
			lg_info.lgCreationRole = SECONDARY;
		}

		// REG_KEY_JPath
		OS_ZeroMemory(unicodeBuffer1, sizeof(unicodeBuffer1));
		unicode1.Buffer			= unicodeBuffer1;
		unicode1.Length			= 0;
		unicode1.MaximumLength	= sizeof(unicodeBuffer1);

		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_JPath, REG_SZ, 
								 &unicode1, unicode1.MaximumLength);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: sftk_GetRegKey(%S : %S, Value %S) Failed  with status %08x! \n", 
								 lgNumPathKey, REG_KEY_JPath, unicode1.Buffer, status));
			continue;
		}
		// Create LG here
		ansiString1.Buffer			= charJFile;
		ansiString1.Length			= 0;
		ansiString1.MaximumLength	= sizeof(charJFile);

		status = RtlUnicodeStringToAnsiString( &ansiString1, &unicode1, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d RtlUnicodeStringToAnsiString(%S) Failed with status 0x%08x !!!\n", 
								 lgnum, unicode1.Buffer, status));
			continue;
		}
		OS_RtlCopyMemory( lg_info.JournalPath, ansiString1.Buffer, ansiString1.Length);
#endif
		// -- create a LG device and its relative threads 
		OS_ASSERT(GSftk_Config.DriverObject != NULL);
		pSftk_Lg = NULL;

		status = sftk_Create_InitializeLGDevice( GSftk_Config.DriverObject, 
												 &lg_info,
												 &pSftk_Lg,
												 FALSE,
												 FALSE);
		if ( !NT_SUCCESS(status) )
		{
			DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d sftk_Create_InitializeLGDevice(Pstore : %S) Failed with status 0x%08x !!!\n", 
								 lgnum, unicode.Buffer, status));
			continue;
		}

		// Do not change state here, first read and configured all devices than update state.
		// for default we changed the state to tracking in case 2nd or other device config fails. 
		OS_ASSERT(pSftk_Lg != NULL);

		pSftk_Lg->state					= SFTK_MODE_TRACKING;
		pSftk_Lg->LastShutDownUpdated	= FALSE;
		OS_SetFlag( pSftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE);

#if TARGET_SIDE
		// REG_KEY_PreviouseRole
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_PreviouseRole, REG_DWORD, 
								 &pSftk_Lg->Role.PreviouseRole,sizeof(pSftk_Lg->Role.PreviouseRole));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: Sftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								pSftk_Lg, pSftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_PreviouseRole, pSftk_Lg->Role.PreviouseRole, status));
			// goto deleteLG;
		}
		// REG_KEY_CurrentRole
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_CurrentRole, REG_DWORD, 
								 &pSftk_Lg->Role.CurrentRole,sizeof(pSftk_Lg->Role.CurrentRole));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: pSftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								pSftk_Lg, pSftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_CurrentRole, pSftk_Lg->Role.CurrentRole, status));
			// goto deleteLG;
		}
		// REG_KEY_FailOver
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_FailOver, REG_DWORD, 
								 &pSftk_Lg->Role.FailOver,sizeof(pSftk_Lg->Role.FailOver));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: pSftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								pSftk_Lg, pSftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_FailOver, pSftk_Lg->Role.FailOver, status));
			// goto deleteLG;
		}
		/*
		// REG_KEY_JEnable
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_JEnable, REG_DWORD, 
								 &pSftk_Lg->Role.JEnable,sizeof(pSftk_Lg->Role.JEnable));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: pSftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								pSftk_Lg, pSftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JEnable, pSftk_Lg->Role.JEnable, status));
			goto done;
		}
		// REG_KEY_JApplyRunning
		status = sftk_GetRegKey( lgNumPathKey, REG_KEY_JApplyRunning, REG_DWORD, 
								 &pSftk_Lg->Role.JApplyRunning,sizeof(pSftk_Lg->Role.JApplyRunning));
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_lg_Create_RegKey:: pSftk_Lg 0x%08x for LG Num %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
								pSftk_Lg, pSftk_Lg->LGroupNumber, lgNumPathKey, REG_KEY_JApplyRunning, pSftk_Lg->Role.JApplyRunning, status));
			goto done;
		}
		*/
#endif
		
		bCreatedOneDev = FALSE;
		for (devNum=0, j=0; j < totalDev; devNum++)
		{ // for : get each and every Src Devices under current LG and configured it from registry

			OS_ZeroMemory( &dev_info, sizeof(dev_info));
			bvalidDevNum = FALSE;

#if TARGET_SIDE
			status = sftk_Read_Registry_For_Dev( &dev_info, lg_info.lgCreationRole, lgnum, devNum, &bvalidDevNum);
#else
			status = sftk_Read_Registry_For_Dev( &dev_info, PRIMARY, lgnum, devNum, &bvalidDevNum);
#endif

			if ( bvalidDevNum == TRUE)
				j ++; // update Dev Counter which is used

			if ( !NT_SUCCESS(status) )
			{
				DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d DevNum %d sftk_Create_InitializeLGDevice() Failed with status 0x%08x, ignoring this DevNum, going for next !!!\n", 
									 lgnum, devNum, status));
				
				continue;
			}
			
			// create Dev under lg
			pSftk_Dev = NULL;
			status = sftk_CreateInitialize_SftekDev( &dev_info, &pSftk_Dev, FALSE, FALSE, TRUE);
			if (!NT_SUCCESS(status)) 
			{ // failed
				DebugPrint((DBG_ERROR, "sftk_configured_from_registry:: LgNum %d DevNum %d sftk_CreateInitialize_SftekDev() Failed with status 0x%08x, ignoring this DevNum, going for next !!!\n", 
									 lgnum, devNum, status));
				continue;
			}

			bCreatedOneDev = TRUE;
		} // for : get each and every Src Devices under current LG and configured it from registry
// deleteLG:
		if (bCreatedOneDev == FALSE)
		{ // Delete Lg since there is no any one device exist for it
			sftk_delete_lg( pSftk_Lg, TRUE );
		}
	} // for : get each and every LG and configured it from registry

#if TARGET_SIDE
		bCounter++;
	} while (bCounter <= 2);
#endif
	return STATUS_SUCCESS;
} // sftk_configured_from_registry()

NTSTATUS
sftk_Read_Registry_For_Dev( ftd_dev_info_t  *Dev_info, ROLE_TYPE CreationRole, ULONG LgNum, ULONG DevNum, PBOOLEAN	ValidDevNum)
{
	NTSTATUS		status		= STATUS_SUCCESS;
	WCHAR			devNumPathKey[128], unicodeBuffer[256];
	UNICODE_STRING	unicode;
	ANSI_STRING		ansiString;
	PREG_DEV_INFO	pRegDevInfo = NULL;
	PUCHAR			pBuffer;

	*ValidDevNum = FALSE;

	// REG_KEY_DevNum_STRING_ROOT L"sftkblk\\Parameters\\LGNum%08d\\DevNum%08d"	// its Key
#if TARGET_SIDE
	if (CreationRole == PRIMARY)
		swprintf( devNumPathKey, REG_KEY_DevNum_STRING_ROOT,  REG_PRIMARY_UNICODE, LgNum, DevNum);
	else
		swprintf( devNumPathKey, REG_KEY_DevNum_STRING_ROOT,  REG_SECONDARY_UNICODE, LgNum, DevNum);
#else
	swprintf( devNumPathKey, REG_KEY_DevNum_STRING_ROOT,  LgNum, DevNum);
#endif

	status = sftk_CheckRegKeyPresent(devNumPathKey);
	if (!NT_SUCCESS(status))
	{ // Check LG root key exist, if not return error
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: sftk_CheckRegKeyPresent(%S) Failed  with status %08x! \n", 
									devNumPathKey, status));
		goto done;
	}
	*ValidDevNum = TRUE;	// since Registry Key is present

	Dev_info->lgnum = LgNum;

#if 1
	// Allocate Binary structures for each device 
	pRegDevInfo = OS_AllocMemory(NonPagedPool, 4096);
	if (pRegDevInfo == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d OS_AllocMemory(size %d) Failed for pRegDevInfo returning status %08x! \n", 
							LgNum, DevNum, 4096, status));
		goto done;
	}
	// Initialize this Binary structures
	RtlZeroMemory(pRegDevInfo, 4096);
	status = sftk_GetRegKey( devNumPathKey, REG_KEY_DevInfo, REG_BINARY, 
							 pRegDevInfo, 4096);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S, REG_BINARY Value 0x%08x, size %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_DevInfo, pRegDevInfo, 4096, status));
		goto done;
	}

	Dev_info->lgnum		= pRegDevInfo->LgNum;
	Dev_info->cdev		= Dev_info->bdev = Dev_info->localcdev = pRegDevInfo->cDev;
	Dev_info->disksize	= pRegDevInfo->Disksize;
	Dev_info->lrdbsize32= pRegDevInfo->Lrdbsize;
	Dev_info->hrdbsize32= pRegDevInfo->Hrdbsize;
	// Dev_info->lrdb_res = 
	// Dev_info->hrdb_res = 
	// Dev_info->lrdb_numbits = 
	// Dev_info->hrdb_numbits = 

	Dev_info->statsize = pRegDevInfo->Statsize;
	// Dev_info->lrdb_offset = 
	pBuffer = (PUCHAR) ((ULONG) pRegDevInfo + sizeof(REG_DEV_INFO));

	RtlCopyMemory( Dev_info->devname, pBuffer, pRegDevInfo->DevNameLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->DevNameLength);

	RtlCopyMemory( Dev_info->vdevname, pBuffer, pRegDevInfo->VDevNameLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->VDevNameLength);

	RtlCopyMemory( Dev_info->strRemoteDeviceName, pBuffer, pRegDevInfo->strRemoteDeviceNameLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->strRemoteDeviceNameLength);
	
	Dev_info->bUniqueVolumeIdValid	= pRegDevInfo->bUniqueVolumeIdValid;
	Dev_info->UniqueIdLength		= pRegDevInfo->UniqueIdLength; 
	RtlCopyMemory( Dev_info->UniqueId, pBuffer, pRegDevInfo->UniqueIdLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->UniqueIdLength);

	Dev_info->bSignatureUniqueVolumeIdValid = pRegDevInfo->bSignatureUniqueVolumeIdValid;
	Dev_info->SignatureUniqueIdLength		= pRegDevInfo->SignatureUniqueIdLength;
	RtlCopyMemory( Dev_info->SignatureUniqueId, pBuffer, pRegDevInfo->SignatureUniqueIdLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->SignatureUniqueIdLength);

	Dev_info->bSuggestedDriveLetterLinkValid = pRegDevInfo->bSuggestedDriveLetterLinkValid;
	Dev_info->SuggestedDriveLetterLinkLength = pRegDevInfo->SuggestedDriveLetterLinkLength;
	RtlCopyMemory( Dev_info->SuggestedDriveLetterLink, pBuffer, pRegDevInfo->SuggestedDriveLetterLinkLength);
	pBuffer = (PUCHAR) ((ULONG) pBuffer + pRegDevInfo->SuggestedDriveLetterLinkLength);

#else
	/*
	// Get differnet information for Dev
	// --- REG_KEY_DevNumber
	status = sftk_GetRegKey( devNumPathKey, REG_KEY_DevNumber, REG_DWORD, 
							 &Dev_info->cdev, sizeof(Dev_info->cdev));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_DevNumber, Dev_info->cdev, status));
		goto done;
	}
	*ValidDevNum = TRUE;

	Dev_info->localcdev = Dev_info->bdev = Dev_info->cdev;
	
	// --- REG_KEY_UniqueId
	// OS_ZeroMemory(Dev_info->UniqueId, sizeof(Dev_info->UniqueId));
	OS_ZeroMemory( unicodeBuffer, sizeof(unicodeBuffer));
	unicode.Buffer			= unicodeBuffer;
	unicode.Length			= 0;
	unicode.MaximumLength	= sizeof(unicodeBuffer);

	status = sftk_GetRegKey( devNumPathKey, REG_KEY_UniqueId, REG_SZ, 
							 &unicode, unicode.MaximumLength);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S Length %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_UniqueId, unicode.MaximumLength, status));
		goto done;
	}
	// store this into character !
	ansiString.Buffer			= Dev_info->UniqueId;
	ansiString.Length			= 0;
	ansiString.MaximumLength	= sizeof(Dev_info->UniqueId);
	status = RtlUnicodeStringToAnsiString( &ansiString, &unicode, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LgNum %d DevNum %d RtlUnicodeStringToAnsiString(%S) Failed with status 0x%08x !!!\n", 
								 LgNum, DevNum, unicode.Buffer, status));
		goto done;
	}
	Dev_info->bUniqueVolumeIdValid	= TRUE;
	Dev_info->UniqueIdLength		= ansiString.Length / 2;	// since it stores in binary !!!

	// --- REG_KEY_SignatureUniqueId
	// OS_ZeroMemory(Dev_info->SignatureUniqueId, sizeof(Dev_info->SignatureUniqueId));
	OS_ZeroMemory(unicodeBuffer, sizeof(unicodeBuffer));
	unicode.Buffer			= unicodeBuffer;
	unicode.Length			= 0;
	unicode.MaximumLength	= sizeof(unicodeBuffer);

	status = sftk_GetRegKey( devNumPathKey, REG_KEY_SignatureUniqueId, REG_SZ, 
							 &unicode, unicode.MaximumLength);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S Length %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_SignatureUniqueId, unicode.MaximumLength, status));
		goto done;
	}
	// store this into character !
	ansiString.Buffer			= Dev_info->SignatureUniqueId;
	ansiString.Length			= 0;
	ansiString.MaximumLength	= sizeof(Dev_info->SignatureUniqueId);
	status = RtlUnicodeStringToAnsiString( &ansiString, &unicode, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LgNum %d DevNum %d RtlUnicodeStringToAnsiString(%S) Failed with status 0x%08x !!!\n", 
								 LgNum, DevNum, unicode.Buffer, status));
		goto done;
	}
	Dev_info->bSignatureUniqueVolumeIdValid	= TRUE;
	Dev_info->SignatureUniqueIdLength		= ansiString.Length + 1;

	// --- REG_KEY_VDevName
	// OS_ZeroMemory(Dev_info->vdevname, sizeof(Dev_info->vdevname));
	OS_ZeroMemory(unicodeBuffer, sizeof(unicodeBuffer));
	unicode.Buffer			= unicodeBuffer;
	unicode.Length			= 0;
	unicode.MaximumLength	= sizeof(unicodeBuffer);

	status = sftk_GetRegKey( devNumPathKey, REG_KEY_VDevName, REG_SZ, 
							 &unicode, unicode.MaximumLength);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S Length %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_VDevName, unicode.MaximumLength, status));
		goto done;
	}
	// store this into character !
	ansiString.Buffer			= Dev_info->vdevname;
	ansiString.Length			= 0;
	ansiString.MaximumLength	= sizeof(Dev_info->vdevname);
	status = RtlUnicodeStringToAnsiString( &ansiString, &unicode, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LgNum %d DevNum %d RtlUnicodeStringToAnsiString(%S) Failed with status 0x%08x !!!\n", 
								 LgNum, DevNum, unicode.Buffer, status));
		goto done;
	}

	// --- REG_KEY_DevName
	// OS_ZeroMemory(Dev_info->vdevname, sizeof(Dev_info->vdevname));
	OS_ZeroMemory(unicodeBuffer, sizeof(unicodeBuffer));
	unicode.Buffer			= unicodeBuffer;
	unicode.Length			= 0;
	unicode.MaximumLength	= sizeof(unicodeBuffer);

	status = sftk_GetRegKey( devNumPathKey, REG_KEY_DevName, REG_SZ, 
							 &unicode, unicode.MaximumLength);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S Length %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_DevName, unicode.MaximumLength, status));
		goto done;
	}
	// store this into character !
	ansiString.Buffer			= Dev_info->devname;
	ansiString.Length			= 0;
	ansiString.MaximumLength	= sizeof(Dev_info->devname);
	status = RtlUnicodeStringToAnsiString( &ansiString, &unicode, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LgNum %d DevNum %d RtlUnicodeStringToAnsiString(%S) Failed with status 0x%08x !!!\n", 
								 LgNum, DevNum, unicode.Buffer, status));
		goto done;
	}

	Dev_info->statsize = 4096; // TODO : for time being let keep hard coded....

	// --- REG_KEY_DiskSize
	status = sftk_GetRegKey( devNumPathKey, REG_KEY_DiskSize, REG_DWORD, 
							 &Dev_info->disksize, sizeof(Dev_info->disksize));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_DiskSize, Dev_info->disksize, status));
		goto done;
	}
	// REG_KEY_Lrdbsize
	status = sftk_GetRegKey( devNumPathKey, REG_KEY_Lrdbsize, REG_DWORD, 
							 &Dev_info->lrdbsize32, sizeof(Dev_info->lrdbsize32));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_Lrdbsize, Dev_info->lrdbsize32, status));
		goto done;
	}

	// REG_KEY_Hrdbsize
	status = sftk_GetRegKey( devNumPathKey, REG_KEY_Hrdbsize, REG_DWORD, 
							 &Dev_info->hrdbsize32, sizeof(Dev_info->hrdbsize32));
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_Read_Registry_For_Dev:: LG Num %d, DevNum %d sftk_GetRegKey(%S : %S, Value %d) Failed  with status %08x! \n", 
							LgNum, DevNum, devNumPathKey, REG_KEY_Hrdbsize, Dev_info->hrdbsize32, status));
		goto done;
	}
	*/
#endif
	
	status = STATUS_SUCCESS;
done:
	if(pRegDevInfo)
		OS_FreeMemory(pRegDevInfo);

	return status;
} // sftk_Read_Registry_For_Dev()


