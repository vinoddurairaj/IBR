/**************************************************************************************

Module Name: Sftk_MM.C   
Author Name: Parag sanghvi
Description: All MM API definations are defined here
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_MM.h"

//
// Global Varialbe Definations
//
GMM_INFO	GMM_Info;

//
// MM_Start()
//
// Starts the Memory Manager.  
//
// Returns: TRUE on success, FALSE on error
//
// NOTE: It does Event Log Message registration on success or failuer, caller do not 
// need to do these.
// 
BOOLEAN
MM_Start()
{
	BOOLEAN							bret		= FALSE;
	ULONG							sizeOfmem, i, j, sizeToAllocate, numOfNodes, total_memSize;
	ULONG							returnBytes, numOfArray;
	DWORD							result		= NO_ERROR;
	PSET_MM_DATABASE_MEMORY			pMM_Db;
	MM_DATABASE_SIZE_ENTRIES		mmDbSizeEntries;
	PSET_MM_RAW_MEMORY				mm_raw_memory = NULL;
	PULONG							aPFNs	= NULL;
	ULONG							numberOfPagesAllocated, numberOfChunksAllocated;
		
	if (GMM_Info.MM_Initialized == TRUE)
	{
		DebugPrint( ("MM_Start: MM is Already Initialized !!! returning Success !!! \n"));
		return TRUE;	// True means Successed
	}

	result = MM_Init(&GMM_Info);
	if (result != NO_ERROR)
	{
		DebugPrint( ("MM_Start:: MM_Init() Failed with error %ld (0x%X) \n",
							result, result));
		
		return FALSE;	// failed
	}

#if 0	// Following code is not required !!! Commented out that later
	// If the cache is > 2GB then make sure 
	// HKEY_LOCAL_MACHINE\CurrentControlSet\Control\Session Manager\Memory Management\IoPageLockLimit
	#define	REG_SYSTEM_SESSION_MEMORY_MANAGER	"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\"
	// is not Zero.  Suggest setting it to 0xc0000000.
	if(GMM_Info.MaxAllocatPhysMemInMB > 2000) 
	{
		if(GMM_Info.ThreeGB) 
		{
			DWORD result, dwType, sizeofKey;
			result = RegOpenKeyEx(	HKEY_LOCAL_MACHINE, 
									REG_SYSTEM_SESSION_MEMORY_MANAGER, 
									0, KEY_ALL_ACCESS, &dwHandle);
			if (result == NO_ERROR) 
			{
				DWORD	IoPageLockLimit;
				sizeofKey = sizeof(IoPageLockLimit);
				
				result = RegQueryValueEx(	dwHandle, "IoPageLockLimit", NULL, &dwType, 
											(UCHAR *)&IoPageLockLimit, &sizeofKey);
				
				if (result == NO_ERROR) 
				{
					if(IoPageLockLimit < 0xc0000000) 
					{
						DebugPrint( ("MM_Start:: RegKey: %s IoPageLockLimit() Values 0x%08x < 0x%08x, AWE may failed !!! \n",
									REG_SYSTEM_SESSION_MEMORY_MANAGER, IoPageLockLimit, 0xc0000000));
						// Log Event 
						// MM_DeInit();
						// return FALSE;
						IoPageLockLimit = 0xc0000000;
						sizeofKey		= sizeof(IoPageLockLimit);
						dwType			= REG_DWORD;
						result = RegSetValueEx(	dwHandle, "IoPageLockLimit", NULL, dwType, 
												(UCHAR *)&IoPageLockLimit, sizeofKey);
					}
				}
				RegCloseKey(dwHandle);
			}
		} // if(GMM_Info.ThreeGB) 
	} // if(GMM_Info.MaxAllocatPhysMemInMB > 2000) 
#endif

	// Bump up processe working set, not sure needed or not, but it's better to be safe.

	if(!SetProcessWorkingSetSize(GetCurrentProcess(), 0x1000, 0xc0000000)) 
	{
		DebugPrint( ("MM_Start:: SetProcessWorkingSetSize(dwMinimumWorkingSetSize 0x%08x, dwMaximumWorkingSetSize 0x%08x ) failed with Error %d, 0x%08x !!! \n",
									0x1000, 0xc0000000, GetLastError(), GetLastError()));
	}

	// -Retrieve Kernel MM different sizeof Fixed memory allocation information
	// It passed array of GET_MM_DATABASE_SIZE, to get list of MM manager Database structure size 
	sizeOfmem = sizeof(mmDbSizeEntries);
	RtlZeroMemory( &mmDbSizeEntries, sizeOfmem);
	mmDbSizeEntries.AWEUsed					= GMM_Info.UsedAWE;
	mmDbSizeEntries.PageSize				= GMM_Info.PageSize;
	mmDbSizeEntries.VChunkSize				= GMM_Info.VAllocChunkSize;
	mmDbSizeEntries.MaxAllocatePhysMemInMB	= GMM_Info.MaxAllocatPhysMemInMB;
	mmDbSizeEntries.IncrementAllocationChunkSizeInMB	= GMM_Info.IncrementAllocationChunkSizeInMB;
	mmDbSizeEntries.NumberOfEntries			= MAX_MM_TYPE_ENTRIES;

	mmDbSizeEntries.AllocThreshold			= GMM_Info.AllocThreshold;
	mmDbSizeEntries.AllocIncrement			= GMM_Info.AllocIncrement;
	mmDbSizeEntries.AllocThresholdTimeout	= GMM_Info.AllocThresholdTimeout;
	mmDbSizeEntries.AllocThresholdCount		= GMM_Info.AllocThresholdCount;
	
	mmDbSizeEntries.FreeThreshold			= GMM_Info.FreeThreshold;
	mmDbSizeEntries.FreeIncrement			= GMM_Info.FreeIncrement;
	mmDbSizeEntries.FreeThresholdTimeout	= GMM_Info.FreeThresholdTimeout;
	mmDbSizeEntries.FreeThresholdCount		= GMM_Info.FreeThresholdCount;
	
	result = MM_DeviceIOControl( GMM_Info.HandleCtl, 
								IOCTL_GET_MM_START, // IOCTL_GET_MM_DATABASE_SIZE,
								&mmDbSizeEntries, sizeOfmem, 
								&mmDbSizeEntries, sizeOfmem, 
								&returnBytes);
	if (result != NO_ERROR)
	{
		DebugPrint( ("MM_Start:: MM_DeviceIOControl(%s) IOCTL_GET_MM_START Failed with error %ld (0x%X) \n",
							SFTK_CTL_USER_MODE_DEVICE_NAME, result, result));
		// TODO : Log Event message here
		goto done;
	}

	// -First Allocate Memory For MM_DB_ENTRIES these includes all strcutures required for MM to functions.
	// Let's use VitualAlloc for each Memory Index size needed 
	if (mmDbSizeEntries.NumberOfEntries == 0)
	{
		DebugPrint( ("MM_Start:: FIXME FIXME :: mmDbSizeEntries.NumberOfEntries = %d , FIXME FIXME nothing to do !!!!! \n",
							mmDbSizeEntries.NumberOfEntries, MAX_MM_TYPE_ENTRIES));
	}

	if (mmDbSizeEntries.NumberOfEntries > MAX_MM_TYPE_ENTRIES)
	{
		DebugPrint( ("MM_Start:: FIXME FIXME :: mmDbSizeEntries.NumberOfEntries = %d > MAX_MM_TYPE_ENTRIES %d , FIXME FIXME !!! \n",
							mmDbSizeEntries.NumberOfEntries, MAX_MM_TYPE_ENTRIES));
	
		mmDbSizeEntries.NumberOfEntries = MAX_MM_TYPE_ENTRIES;
	}

	RtlZeroMemory( &GMM_Info.MM_Db, sizeof(GMM_Info.MM_Db));

	for (	i=0, GMM_Info.MM_Db.NumberOfEntries = 0; 
			(i < mmDbSizeEntries.NumberOfEntries); 
			i++, GMM_Info.MM_Db.NumberOfEntries ++)
	{ // for : Allocate memory for MM database structures memory

		if (mmDbSizeEntries.MmDb[i].NodeSize == 0)
			continue; // nothing to allocate for this type

		if (mmDbSizeEntries.MmDb[i].PageSizeRepresenting == 0)
			continue;

		// Calculate Total memory size required to allocate current MM Index Total nodes
		if (mmDbSizeEntries.MmDb[i].PageSizeRepresenting == GMM_Info.PageSize)
		{ // Calculate Total number of nodes requires to holde PAGE_SIZE
			numOfNodes = (ULONG) ( ((DWORDLONG) GMM_Info.MaxAllocatPhysMemInMB * (DWORDLONG) ONEMILLION) / 
									(DWORDLONG) GMM_Info.PageSize );
		}
		else
		{
			numOfNodes = (ULONG) ( ((DWORDLONG) GMM_Info.MaxAllocatPhysMemInMB * (DWORDLONG) ONEMILLION) / 
									(DWORDLONG) mmDbSizeEntries.MmDb[i].PageSizeRepresenting );

		}

		total_memSize = (numOfNodes * mmDbSizeEntries.MmDb[i].NodeSize);
		ROUND_UP(total_memSize, GMM_Info.PageSize);
		
		// Calculate number of array nodes requires to hold these memory
		numOfArray = 1;
		if (total_memSize > GMM_Info.MaxAllocAllowedInBytes)
		{
			numOfArray = (total_memSize / GMM_Info.MaxAllocAllowedInBytes);
			if ((total_memSize % GMM_Info.MaxAllocAllowedInBytes) != 0)
				numOfArray ++;
		}

		// Now allocate SET_MM_DATABASE_MEMORY memory to pass it to driver
		sizeOfmem = sizeof(SET_MM_DATABASE_MEMORY) + (numOfArray * sizeof(VIRTUAL_MM_INFO));
		pMM_Db = (PSET_MM_DATABASE_MEMORY) VirtualAlloc(NULL,	sizeOfmem, MEM_COMMIT, PAGE_READWRITE);

		if (pMM_Db == NULL) 
		{
			DebugPrint( ("MM_Start:: VirtualAlloc( Size %d ) PSET_MM_DATABASE_MEMORY Index %d, Nodesize %d Failed with Eror %d (0x%08x)!!! \n",
							sizeOfmem, i, mmDbSizeEntries.MmDb[i].NodeSize, GetLastError(), GetLastError()));
	
			GetErrorText();
			goto done;
		}

		RtlZeroMemory(pMM_Db, sizeOfmem);
		GMM_Info.MM_Db.PtrMmDb[i]		= pMM_Db;	// set this global variable used for error recovery
		GMM_Info.MM_Db.NumberOfEntries++;			// set this global variable used for error recovery

		pMM_Db->MMIndex				= mmDbSizeEntries.MmDb[i].MMIndex;
		pMM_Db->NodeSize			= mmDbSizeEntries.MmDb[i].NodeSize;
		pMM_Db->TotalNumberOfNodes	= numOfNodes;
		pMM_Db->TotalMemorySize		= total_memSize;
		pMM_Db->NumberOfArray		= numOfArray;
		
		pMM_Db->ChunkSize = 0;	// Temporary used !!!

		for (j=0; j < numOfArray; j++)
		{ // For : Allocate  Virtual address in chunk for Memory index
			if ( (pMM_Db->ChunkSize + GMM_Info.MaxAllocAllowedInBytes) > pMM_Db->TotalMemorySize)
				sizeToAllocate = pMM_Db->TotalMemorySize - pMM_Db->ChunkSize;
			else
				sizeToAllocate = GMM_Info.MaxAllocAllowedInBytes;

			// Now allocate SET_MM_DATABASE_MEMORY memory to pass it to driver
			pMM_Db->VMem[j].Vaddr = (PULONG) VirtualAlloc(NULL,	sizeToAllocate, MEM_COMMIT, PAGE_READWRITE);

			if (pMM_Db->VMem[j].Vaddr == NULL) 
			{
				DebugPrint( ("MM_Start:: VirtualAlloc( Size %d ) for pMM_Db->VMem[j %d].Vaddr  Index %d, Nodesize %d Failed with Eror %d (0x%08x)!!! \n",
								sizeToAllocate, j, i, mmDbSizeEntries.MmDb[i].NodeSize, GetLastError(), GetLastError()));
				GetErrorText();
				goto done;
			}
			RtlZeroMemory(pMM_Db->VMem[j].Vaddr, sizeToAllocate);

			pMM_Db->VMem[j].SizeOfMem = sizeToAllocate;
			pMM_Db->ChunkSize += sizeToAllocate; // incrementallocated memory
		} // For : Allocate  Virtual address in chunk for Memory index

		GMM_Info.TotalMemoryAllocated += (ULONGLONG) pMM_Db->TotalMemorySize;

		pMM_Db->ChunkSize	= pMM_Db->TotalMemorySize;

		// Pass these memory to driver 
		result = MM_DeviceIOControl(	GMM_Info.HandleCtl, 
										IOCTL_SET_MM_DATABASE_MEMORY,
										pMM_Db, sizeOfmem, 
										NULL, 0, 
										&returnBytes);
		if (result != NO_ERROR)
		{
			DebugPrint( ("MM_Start:: MM_DeviceIOControl(%s) IOCTL_SET_MM_DATABASE_MEMORY Failed with error %ld (0x%X) \n",
								SFTK_CTL_USER_MODE_DEVICE_NAME, result, result));
			// TODO : Log Event message here
			goto done;
		}
	} // for : Allocate memory for MM database structures memory

	// Allocate First chunk of AWE/Or Virtual memory at MM startup. 
	// Later, Load balancing will increase/decrease these RAW Memory allocation/free through Shared memory IPC communications.
	if (GMM_Info.UsedAWE == TRUE)
	{ // if : AWE is supported
		numberOfPagesAllocated = 
			(GMM_Info.InitRawMemoryToAllocateInMB * ONEMILLION) / GMM_Info.PageSize;

		sizeOfmem = sizeof(SET_MM_RAW_MEMORY) + (numberOfPagesAllocated * sizeof(ULONG));
		mm_raw_memory = (PSET_MM_RAW_MEMORY) VirtualAlloc(NULL,	sizeOfmem, MEM_COMMIT, PAGE_READWRITE);
		if (mm_raw_memory == NULL) 
		{
			DebugPrint( ("MM_Start:: AWEUsed : VirtualAlloc( Size %d ) PSET_MM_RAW_MEMORY for numberofpages %d, Failed with Error %d (0x%08x)!!! \n",
							sizeOfmem, numberOfPagesAllocated, GetLastError(), GetLastError()));
			GetErrorText();
			goto done;
		}
		RtlZeroMemory(mm_raw_memory, sizeOfmem);
	
		bret = Allocate_AWE_Memory( GMM_Info.ProcessHandle,
									&numberOfPagesAllocated, 
									FALSE,
									&mm_raw_memory->ArrayOfMem );
		if (bret == FALSE)
		{ // failed
			DebugPrint( ("MM_Start: Allocate_AWE_Memory() Failed \n"));
			// TODO : Log Event Message here
			goto done;
		}

		mm_raw_memory->NumberOfArray	= numberOfPagesAllocated;
		mm_raw_memory->ChunkSize		= GMM_Info.PageSize;
	} // if : AWE is supported
	else
	{ // else : AWE is NOT supported
		numberOfChunksAllocated = 
		(GMM_Info.InitRawMemoryToAllocateInMB * ONEMILLION) / GMM_Info.VAllocChunkSize;
		//numberOfChunksAllocated = 
		//	(ULONG) ( (ULONGLONG)(GMM_Info.MaxAllocatPhysMemInMB * ONEMILLION) - GMM_Info.TotalMemoryAllocated);
		//numberOfChunksAllocated = numberOfChunksAllocated  - ONEMILLION;
		//numberOfChunksAllocated = numberOfChunksAllocated / GMM_Info.VAllocChunkSize;

		sizeOfmem = sizeof(SET_MM_RAW_MEMORY) + (numberOfChunksAllocated * sizeof(ULONG));
		mm_raw_memory = (PSET_MM_RAW_MEMORY) VirtualAlloc(NULL,	sizeOfmem, MEM_COMMIT, PAGE_READWRITE);
		if (mm_raw_memory == NULL) 
		{
			DebugPrint( ("MM_Start:: Non AWEUsed : VirtualAlloc( Size %d ) PSET_MM_RAW_MEMORY for numberOfChunksAllocated %d, Failed with Error %d (0x%08x)!!! \n",
							sizeOfmem, numberOfChunksAllocated, GetLastError(), GetLastError()));
			GetErrorText();
			goto done;
		}
		RtlZeroMemory(mm_raw_memory, sizeOfmem);

		// Test_GlobalMemoryStatusEx( &statex );
		bret = Allocate_Virtual_Memory_Pages(	&numberOfChunksAllocated, 
												GMM_Info.VAllocChunkSize,
												FALSE,
												&mm_raw_memory->ArrayOfMem );
		if (bret == FALSE)
		{ // failed
			DebugPrint( ("MM_Start: Allocate_Virtual_Memory_Pages() Failed \n"));
			// TODO : Log Event Message here
			goto done;
		}

		mm_raw_memory->NumberOfArray	= numberOfChunksAllocated;
		mm_raw_memory->ChunkSize		= GMM_Info.VAllocChunkSize;
	} // else : AWE is NOT supported

	mm_raw_memory->TotalMemorySize = (mm_raw_memory->NumberOfArray * mm_raw_memory->ChunkSize);
	// mm_raw_memory->ArrayOfMem = aPFNs;

	// Increment Global Total Mem Allocation counter
	GMM_Info.TotalMemoryAllocated += (ULONGLONG) mm_raw_memory->TotalMemorySize;
	
	// Pass this Allocated Init RAW Memory to driver 
	result = MM_DeviceIOControl(	GMM_Info.HandleCtl, 
									IOCTL_MM_INIT_RAW_MEMORY,
									mm_raw_memory, sizeOfmem, 
									NULL, 0, 
									&returnBytes);
	if (result != NO_ERROR)
	{ // Failed : IOCTL_MM_INIT_RAW_MEMORY 
		DebugPrint( ("MM_Start:: MM_DeviceIOControl(%s) IOCTL_MM_INIT_RAW_MEMORY Failed with error %ld (0x%X) \n",
								SFTK_CTL_USER_MODE_DEVICE_NAME, result, result));
		// TODO : Log Event message here

		// Do Cleanup
		if (GMM_Info.UsedAWE == TRUE)
		{ // if : AWE is supported
			bret = Free_AWE_Memory(	GMM_Info.ProcessHandle,
									&numberOfPagesAllocated, 
									numberOfPagesAllocated,	
									FALSE,		
									&mm_raw_memory->ArrayOfMem );
			if (bret == FALSE)
			{ // failed
				DebugPrint( ("Test_VirtualMemory_Alloc: Free_AWE_Memory(NumOfArray %d) Failed !!! \n", numberOfChunksAllocated));
			}
		}
		else
		{ // else : AWE is not Supported
			bret = Free_Virtual_Memory_Pages(	numberOfChunksAllocated,	
												FALSE,		
												&mm_raw_memory->ArrayOfMem );
			if (bret == FALSE)
			{ // failed
				DebugPrint( ("Test_VirtualMemory_Alloc: Free_AWE_Memory(NumOfArray %d) Failed !!! \n", numberOfChunksAllocated));
			}
		}

		GMM_Info.TotalMemoryAllocated -= (ULONGLONG) mm_raw_memory->TotalMemorySize;
		if ((LONGLONG) GMM_Info.TotalMemoryAllocated < (LONGLONG) 0)
		{
			DebugPrint( ("MM_DeInit():: *** FIXME FIXME **** GMM_Info.TotalMemoryAllocated %I64d != (ULONGLONG) 0 .!! \n",
										GMM_Info.TotalMemoryAllocated));
			GMM_Info.TotalMemoryAllocated = 0;
			// TODO :Log Event 
			// Put break point here for Debugging !!!
		}
		goto done;
	} // Failed : IOCTL_MM_INIT_RAW_MEMORY 

	GMM_Info.MM_Initialized = TRUE;

	if (GMM_Info.HThreadTerminateEvent)
		SetEvent(GMM_Info.HThreadTerminateEvent);

	bret = TRUE;	// success
	

done:

	if (bret == FALSE)
	{ // Failed so Cleanup Here
		MM_DeInit(&GMM_Info);
		GMM_Info.MM_Initialized = FALSE;

	}

	if (mm_raw_memory)
		VirtualFree(mm_raw_memory, 0, MEM_RELEASE);
	/*
	if (aPFNs)
		VirtualFree(aPFNs, 0, MEM_RELEASE);
	*/
	
	return bret;
} // MM_Start()

BOOLEAN
MM_Stop()
{
	DWORD	result		= NO_ERROR;
	ULONG	returnBytes;

	// Issue IOCTL to Driver to stop MM 
	if (GMM_Info.MM_Initialized == FALSE)
	{
		DebugPrint( ("MM_Stop: MM is Not Initialized Before !!! returning Success !!! \n"));
		return TRUE;	// True means Successed
	}

	result = MM_DeviceIOControl(GMM_Info.HandleCtl, 
								IOCTL_MM_STOP,
								NULL, 0, 
								NULL, 0, 
								&returnBytes);
	if (result != NO_ERROR)
	{
		DebugPrint( ("MM_Stop:: MM_DeviceIOControl(%s) IOCTL_MM_STOP Failed with error %ld (0x%X) \n",
							SFTK_CTL_USER_MODE_DEVICE_NAME, result, result));
		// TODO : Log Event message here
	}
	
	MM_DeInit(&GMM_Info);

	return TRUE;	// success
} // MM_Stop()

VOID
MM_Init_default( PGMM_INFO	PtrGMM_Info )
{
	MEMORYSTATUSEX	statex;
	SYSTEM_INFO		sysInfo;	
	BOOLEAN			bret;

	PtrGMM_Info->MM_Initialized		= FALSE;
	PtrGMM_Info->UsedAWE			= FALSE;
	PtrGMM_Info->ThreeGB			= FALSE;
	PtrGMM_Info->ProcessHandle		= GetCurrentProcess();
	PtrGMM_Info->VAllocChunkSize	= MM_DEFAULT_VAllocChunkSize;	

	PtrGMM_Info->MaxAvailPhysMemUsedInPercentage	= MM_DEFAULT_MaxAvailPhysMemUsedInPercentage;
	PtrGMM_Info->InitRawMemoryToAllocateInMB		= MM_DEFAULT_InitRawMemoryToAllocateInMB;
	PtrGMM_Info->MaxAllocAllowedInBytes				= MM_DEFAULT_MAX_ALLOC_SIZE_IN_BYTES;
	PtrGMM_Info->IncrementAllocationChunkSizeInMB	= MM_DEFAULT_IncrementAllocationChunkSizeInMB;
	PtrGMM_Info->TotalMemoryAllocated				= 0;

	PtrGMM_Info->AllocThreshold			= DEFAULT_MM_ALLOC_THRESHOLD;
	PtrGMM_Info->AllocIncrement			= DEFAULT_MM_ALLOC_INCREMENT;
	PtrGMM_Info->AllocThresholdTimeout	= DEFAULT_MM_ALLOC_THRESHOLD_TIMEOUT;
	PtrGMM_Info->AllocThresholdCount	= DEFAULT_MM_ALLOC_THRESHOLD_COUNT;

	PtrGMM_Info->FreeThreshold			= DEFAULT_MM_FREE_THRESHOLD;
	PtrGMM_Info->FreeIncrement			= DEFAULT_MM_FREE_INCREMENT;
	PtrGMM_Info->FreeThresholdTimeout	= DEFAULT_MM_FREE_THRESHOLD_TIMEOUT;
	PtrGMM_Info->FreeThresholdCount		= DEFAULT_MM_FREE_THRESHOLD_COUNT;
	
	GetSystemInfo(&sysInfo);
	PtrGMM_Info->PageSize			= sysInfo.dwPageSize;
	PtrGMM_Info->VAllocChunkSize	= sysInfo.dwAllocationGranularity;	// must be 64K 

	RtlZeroMemory( &statex, sizeof(statex) );
	statex.dwLength = sizeof(MEMORYSTATUSEX);
	bret = GlobalMemoryStatusEx(&statex);
	if (bret == FALSE)
	{ // Function Failed
		MEMORYSTATUS state;

		DebugPrint( ("MM_Init_default:: GlobalMemoryStatusEx () Failed with error %ld (0x%X) using GlobalMemoryStatus() calls\n",
						GetLastError(), GetLastError()));
		
		RtlZeroMemory(&state, sizeof(state));
		state.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&state);

		PtrGMM_Info->TotalPhysMemInMB		= (state.dwTotalPhys / ONEMILLION);
		PtrGMM_Info->AvailPhysMemInMB		= (state.dwAvailPhys / ONEMILLION);

		// Determine if we're running on a 3GB user virtual address space system
		PtrGMM_Info->ThreeGB = (state.dwTotalVirtual >= (DWORDLONG) 0x80000000) ? TRUE : FALSE;
	}
	else
	{
		PtrGMM_Info->TotalPhysMemInMB		= (ULONG) (statex.ullTotalPhys / (DWORDLONG) ONEMILLION);
		PtrGMM_Info->AvailPhysMemInMB		= (ULONG) (statex.ullAvailPhys / (DWORDLONG) ONEMILLION);

		// Determine if we're running on a 3GB user virtual address space system
		PtrGMM_Info->ThreeGB = (statex.ullTotalVirtual >= (DWORDLONG) 0x80000000) ? TRUE : FALSE;
	}

	PtrGMM_Info->MaxAllocatPhysMemInMB	= (PtrGMM_Info->AvailPhysMemInMB * PtrGMM_Info->MaxAvailPhysMemUsedInPercentage) / 100;

	return;	
} // MM_Init_default()

ULONG
MM_Init_from_registry(PGMM_INFO	PtrGMM_Info)
{
	ULONG				result	= NO_ERROR;
	HKEY				dwHandle= 0;
	DWORD				keyULong, sizeofKey, dwType;

	// ---- Call Registry and retrieve information
	result = RegOpenKeyEx(	HKEY_LOCAL_MACHINE, 
							REG_SYSTEM_MM_PARAMETERS_ASCII, 
							0, 
							KEY_ALL_ACCESS, 
							&dwHandle);

	if (result != NO_ERROR) 
	{ // Failed
		DebugPrint( ("MM_Init_from_registry: RegOpenKeyEx(%s) Failed With LastError %d (0x%08x) returning error !!! \n", 
							REG_SYSTEM_MM_PARAMETERS_ASCII, GetLastError(), GetLastError() ));
		// TODO Log Event Message
		return NO_ERROR;
	}
	
	// ---- Retrieve Values from registry and initialized it.

	// Get MaxAvailPhysMemUsedInPercentage using "MMAllocPercentage"
	keyULong  = 0;
	sizeofKey = sizeof(keyULong);
	result = RegQueryValueEx(	dwHandle, 
								REG_KEY_MM_SIZE_IN_PERCENTAGE, 
								NULL, 
								&dwType, 
								(UCHAR *)&keyULong, 
								&sizeofKey);
	
	if (result != NO_ERROR) 
	{ // Failed
		keyULong = 0;
	}
	
	if ( (keyULong == 0) || (keyULong > 90) )
	{	// first time running Cache Module, cacheSizeInMB is set to zero, so 
		// calculate apprpriate percentage of RAM usage and set this value 
		// in registery.
#if 0
		BOOLEAN	win2k = FALSE;

		// Query OS Version
		// Try calling GetVersionEx using the OSVERSIONINFOEX structure,
		// If that fails, try using the OSVERSIONINFO structure.
		ZeroMemory(&ver, sizeof(OSVERSIONINFOEX));
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if( !(GetVersionEx ((OSVERSIONINFO *) &ver)) )	
		{ // failed
			// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
			ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
			if (GetVersionEx( (OSVERSIONINFO *) &ver))
			{ // successed
				win2k = (ver.dwMajorVersion == 5) ? TRUE : FALSE;
			}
		}
		else	// successed
			win2k = (ver.dwMajorVersion == 5) ? TRUE : FALSE;

		// first time running Cache Module, cacheSizeInMB is set to zero, so 
		// calculate apprpriate percentage of RAM usage and set this value 
		// in registery.

		// todo :	IF user changes size of Physical RAM on system after 
		//			installation and running of this cache module
		//			we need to change updated registry cache percentage 
		//			value each time when this module runs, so just commented
		//			out followinf if condition and run code inside if statement
		//	.		all time !!!! Requires more thoughts though !!

		//	bump cache mem down for win2k - it seems to need more memory 
		//	for the system
		//	First time since installation.  
		//	these values are somewhat arbitrary
		//	cacheSizeInMB is now actually the Percentage of 
		//	Physical Memory to use for the cache
		if(PtrGMM_Info->TotalPhysMemInMB <= 64)
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 10 : 20;		
		else if(PtrGMM_Info->TotalPhysMemInMB <= 128)
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 30 : 40;
		else if(PtrGMM_Info->TotalPhysMemInMB <= 256)
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 40 : 50;
		else if(PtrGMM_Info->TotalPhysMemInMB <= 512)
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 50 : 60;
		else if(PtrGMM_Info->TotalPhysMemInMB <= 1024)
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 65 : 75;
		else if(PtrGMM_Info->TotalPhysMemInMB <= 2048)
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 70 : 80;
		else 	// > 2GB of RAM
			PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = (win2k) ? 80 : 90;
#endif
		// Set Values in REG_KEY_MM_SIZE_IN_PERCENTAGE
		keyULong		= PtrGMM_Info->MaxAvailPhysMemUsedInPercentage;
		sizeofKey		= sizeof(keyULong);
		dwType			= REG_DWORD;

		RegSetValueEx(	dwHandle, REG_KEY_MM_SIZE_IN_PERCENTAGE, 0, dwType, 
						(UCHAR *)&keyULong, sizeofKey);
	} // first time running Cache Module, cacheSizeInMB is set to zero, so 
	else
	{	// valid value
		PtrGMM_Info->MaxAvailPhysMemUsedInPercentage = keyULong;
	}

	// Get REG_KEY_MM_INIT_RAW_MEM_TO_ALLOCATE using "InitRawMemoryAllocate"
	keyULong  = 0;
	sizeofKey = sizeof(keyULong);
	result = RegQueryValueEx(	dwHandle, 
								REG_KEY_MM_INIT_RAW_MEM_TO_ALLOCATE, 
								NULL, 
								&dwType, 
								(UCHAR *)&keyULong, 
								&sizeofKey);
	
	if (result == NO_ERROR) 
	{
		if ( (keyULong > 0) && (keyULong < PtrGMM_Info->MaxAllocatPhysMemInMB) )
			PtrGMM_Info->InitRawMemoryToAllocateInMB = keyULong;
	}

	// Get REG_KEY_MM_INIT_RAW_MEM_TO_ALLOCATE using "InitRawMemoryAllocate"
	keyULong  = 0;
	sizeofKey = sizeof(keyULong);
	result = RegQueryValueEx(	dwHandle, 
								REG_KEY_MM_INCREMENT_ALLOC_CHUNK_SIZE, 
								NULL, 
								&dwType, 
								(UCHAR *)&keyULong, 
								&sizeofKey);
	
	if (result == NO_ERROR) 
	{
		if ( (keyULong > 0) && (keyULong < (PtrGMM_Info->MaxAllocatPhysMemInMB - PtrGMM_Info->InitRawMemoryToAllocateInMB)) )
		{
			PtrGMM_Info->IncrementAllocationChunkSizeInMB = keyULong;
			PtrGMM_Info->AllocIncrement	= (PtrGMM_Info->IncrementAllocationChunkSizeInMB * ONEMILLION);
			PtrGMM_Info->FreeIncrement	= (PtrGMM_Info->IncrementAllocationChunkSizeInMB * ONEMILLION);
		}
	}

	// TODO : ADD Registry information for Alloc/Free Dynamic Threhold and other tuning parameters

#if 0
	// Query REG_KEY_MM_CHUNK_SIZE_ALLOCATION in VAllocChunkSize
	keyULong  = 0;
	sizeofKey = sizeof(keyULong);
	result = RegQueryValueEx(	dwHandle, 
								REG_KEY_MM_CHUNK_SIZE_ALLOCATION, 
								NULL, 
								&dwType, 
								(UCHAR *)&keyULong, 
								&sizeofKey);
	
	if (result != NO_ERROR) 
	{ // Failed
		keyULong = MM_DEFAULT_VAllocChunkSize;
	}
	PtrGMM_Info->VAllocChunkSize = keyULong;
#endif

	// ---- Set Values in Registry

	// set REG_KEY_MM_AVAILABLE_PHY_MEM
	keyULong		= PtrGMM_Info->AvailPhysMemInMB;
	sizeofKey		= sizeof(keyULong);
	dwType			= REG_DWORD;
	RegSetValueEx(	dwHandle, REG_KEY_MM_AVAILABLE_PHY_MEM, 0, dwType, 
					(UCHAR *)&keyULong, sizeofKey);

	// Calculate and Update in registry total maximum Memory to be used for our memory manager in MB
	PtrGMM_Info->MaxAllocatPhysMemInMB	= 
				(PtrGMM_Info->AvailPhysMemInMB * PtrGMM_Info->MaxAvailPhysMemUsedInPercentage) / 100;

	// ROUND_UP( PtrGMM_Info->MaxAllocatPhysMemInMB, PtrGMM_Info->PageSize);

	// set REG_KEY_MM_ALLOCATED_MEM
	keyULong		= PtrGMM_Info->MaxAllocatPhysMemInMB;
	sizeofKey		= sizeof(keyULong);
	dwType			= REG_DWORD;
	RegSetValueEx(	dwHandle, REG_KEY_MM_ALLOCATED_MEM, 0, dwType, 
					(UCHAR *)&keyULong, sizeofKey);
	goto done;

done:
	if (dwHandle)
		RegCloseKey(dwHandle);

	return result;
} // MM_Init_from_registry()

//
// return NO_ERROR == 0 if Successed else GetLastError()
//
ULONG
MM_Init( PGMM_INFO	PtrGMM_Info)
{
	ULONG				result	= NO_ERROR;
	HKEY				dwHandle= 0;
	
	RtlZeroMemory(PtrGMM_Info, sizeof(GMM_INFO));

	PtrGMM_Info->HandleCtl = INVALID_HANDLE_VALUE;

	// initlaized with default values.
	MM_Init_default( PtrGMM_Info );

	// Check can we use AWE 
	PtrGMM_Info->UsedAWE = MM_is_AWESupported();

	// Retrieves values from registry and initialized it
	MM_Init_from_registry( PtrGMM_Info );

	// Open the device
	PtrGMM_Info->HandleCtl = CreateFile(	SFTK_CTL_USER_MODE_DEVICE_NAME,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ|FILE_SHARE_WRITE,
    									NULL,
    									OPEN_EXISTING,
    									FILE_ATTRIBUTE_NORMAL, // |FILE_FLAG_NO_BUFFERING,
    									NULL);

	if ( PtrGMM_Info->HandleCtl == INVALID_HANDLE_VALUE )
	{
		result = GetLastError();	// failed
		DebugPrint( ("MM_Start:: CreateFile(%s) Failed with error %ld (0x%X) \n",
							SFTK_CTL_USER_MODE_DEVICE_NAME, GetLastError(), GetLastError()));
		goto done;
		// TODO : Log Event here
	}

#if 0 // SM_IPC_SUPPORT
	// Create Shared Memory map for IPC Communications with driver
	PtrGMM_Info->OpenedSMHandle = FALSE;
	result = MM_SharedIPC_Open(PtrGMM_Info);
	if (result != NO_ERROR)
	{ // Failed
		DebugPrint( ("MM_Init:: MM_SharedIPC_Open() Failed with GetLastError %ld (0x%X), Can not continue.. returning error \n",
						GetLastError(), GetLastError()));
		// TODO : Log Event Message here
		goto done;
	}
#endif
	PtrGMM_Info->PtrSM_Cmd = (PSM_CMD) 
				VirtualAlloc(NULL, SFTK_SHAREDMEMORY_SIZE, MEM_COMMIT, PAGE_READWRITE);
	if (PtrGMM_Info->PtrSM_Cmd == NULL) 
	{
		result = GetLastError();
		DebugPrint( ("MM_Init() : VirtualAlloc(%d) Failed to allocate on heap. GetLastError - %d (0x%x)\n",
					SFTK_SHAREDMEMORY_SIZE, GetLastError(), GetLastError()));
		goto done;
	}

	// Create thread which handles IPC communications
	PtrGMM_Info->TerminateThread		= FALSE;

	PtrGMM_Info->HThreadTerminateEvent	=  CreateEvent(NULL, FALSE, FALSE, NULL);
	if (PtrGMM_Info->HThreadTerminateEvent == NULL)
	{ // Error Failed
		result = GetLastError();
		DebugPrint( ("MM_Init:: CreateEvent(For Thread) Failed with GetLastError %ld (0x%X), Can not continue.. returning error \n",
						GetLastError(), GetLastError()));
		// TODO : Log Event Message here
		goto done;
	}

#if 0 // SM_IPC_SUPPORT
	PtrGMM_Info->ThreadWaitMilliseconds	= DEFAULT_SM_THREAD_WAIT_IN_MILLI_SECONDS;	// 30 seconds
	PtrGMM_Info->ThreadSleepMilliseconds = DEFAULT_SM_THREAD_EVENT_SIGNALLED_DELAY_SLEEP;	// 15 seconds

	PtrGMM_Info->HThread = CreateThread(	NULL,	0,
											(LPTHREAD_START_ROUTINE) SM_Thread,
											PtrGMM_Info,
											0,
											NULL);
#endif

	PtrGMM_Info->HThread = CreateThread(	NULL,	0,
											(LPTHREAD_START_ROUTINE) SM_IOCTL_Thread,
											PtrGMM_Info,
											0,
											NULL);
	if (PtrGMM_Info->HThread == NULL)
	{ // Error Failed
		result = GetLastError();
		DebugPrint( ("MM_Init:: CreateThread(SM_Thread) Failed with GetLastError %ld (0x%X), Can not continue.. returning error \n",
						GetLastError(), GetLastError()));
		

		if (PtrGMM_Info->HThreadTerminateEvent)
			CloseHandle(PtrGMM_Info->HThreadTerminateEvent);
		// TODO : Log Event Message here
		goto done;
	}

	result = NO_ERROR; // success

done:
	if (result != NO_ERROR)
	{ // failed
		MM_DeInit(PtrGMM_Info);
	}

	return result;
} // MM_Init()

VOID
MM_DeInit( PGMM_INFO	PtrGMM_Info)
{
	ULONG i,j;

	// Free All Allocated memory
	for (i=0; i < PtrGMM_Info->MM_Db.NumberOfEntries; i++)
	{
		if (PtrGMM_Info->MM_Db.PtrMmDb[i] == NULL)
			continue;

		for (j=0; j < PtrGMM_Info->MM_Db.PtrMmDb[i]->NumberOfArray; j++)
		{
			if (PtrGMM_Info->MM_Db.PtrMmDb[i]->VMem[j].Vaddr)
				VirtualFree(PtrGMM_Info->MM_Db.PtrMmDb[i]->VMem[j].Vaddr, 0, MEM_RELEASE);

			PtrGMM_Info->MM_Db.PtrMmDb[i]->VMem[j].Vaddr = NULL;
		}

		GMM_Info.TotalMemoryAllocated -= (ULONGLONG) PtrGMM_Info->MM_Db.PtrMmDb[i]->TotalMemorySize;

		if ((LONGLONG) GMM_Info.TotalMemoryAllocated < (LONGLONG) 0)
		{
			DebugPrint( ("MM_DeInit():: *** FIXME FIXME **** GMM_Info.TotalMemoryAllocated %I64d != (ULONGLONG) 0 .!! \n",
										GMM_Info.TotalMemoryAllocated));
			GMM_Info.TotalMemoryAllocated = 0;
			// TODO :Log Event 
			// Put break point here for Debugging !!!
		}

		VirtualFree(PtrGMM_Info->MM_Db.PtrMmDb[i], 0, MEM_RELEASE);
		PtrGMM_Info->MM_Db.PtrMmDb[i] = NULL;
	}

	// First Terminate Thread if its already created
	if (PtrGMM_Info->HThread)
	{ // Terminate Thread  
		if (PtrGMM_Info->TerminateThread == FALSE)
		{
			PtrGMM_Info->TerminateThread = TRUE;

			if (PtrGMM_Info->HThreadTerminateEvent)
				SetEvent(PtrGMM_Info->HThreadTerminateEvent);

			if (WaitForSingleObject( PtrGMM_Info->HThread, DEFAULT_SM_THREAD_WAIT_IN_MILLI_SECONDS) != WAIT_OBJECT_0) 
			{ // Thread did not respond to exit command within delay.
				DebugPrint( ("MM_DeInit():: Thread Termination WaitForSingleObject() TimeOut %d milliseconds occured, Thread did not respond to exit within delay.!! \n",
							DEFAULT_SM_THREAD_WAIT_IN_MILLI_SECONDS));
			 }
		}

		CloseHandle(PtrGMM_Info->HThread);

		if (PtrGMM_Info->HThreadTerminateEvent)
			CloseHandle(PtrGMM_Info->HThreadTerminateEvent);

		PtrGMM_Info->HThread				= NULL;	
		PtrGMM_Info->HThreadTerminateEvent	= NULL;
	} // Terminate Thread  

#if 0 // SM_IPC_SUPPORT
	MM_SharedIPC_Close(PtrGMM_Info);
#endif

	if (PtrGMM_Info->PtrSM_Cmd)
		VirtualFree( PtrGMM_Info->PtrSM_Cmd, 0, MEM_RELEASE);

	PtrGMM_Info->PtrSM_Cmd = NULL;

	if (GMM_Info.TotalMemoryAllocated != (ULONGLONG) 0)
	{
		DebugPrint( ("MM_DeInit():: *** FIXME FIXME **** GMM_Info.TotalMemoryAllocated %I64d != (ULONGLONG) 0 .!! \n",
									GMM_Info.TotalMemoryAllocated));
		// TODO :Log Event 
		// Put break point here for Debugging !!!
	}

	if ( PtrGMM_Info->HandleCtl != INVALID_HANDLE_VALUE )
		CloseHandle(PtrGMM_Info->HandleCtl);

	RtlZeroMemory(PtrGMM_Info, sizeof(GMM_INFO));

	return;
} // MM_DeInit()

//
// NO_ERROR means Success else Win32 SDK GetLastError() returns
//
DWORD 
MM_DeviceIOControl(	HANDLE	Handle,
					ULONG	OperationCode, 
					PVOID	InBuf, 
					ULONG	InLen, 
					PVOID	OutBuf, 
					ULONG	OutLen, 
					PULONG	PtrBytesReturned)
{
	HANDLE	hFile			= INVALID_HANDLE_VALUE;
	DWORD	status			= NO_ERROR;

	if ( (Handle == NULL) || (Handle == INVALID_HANDLE_VALUE) )
	{ // createfile
		// Open the device
		hFile = CreateFile( SFTK_CTL_USER_MODE_DEVICE_NAME,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
    						NULL,
    						OPEN_EXISTING,
    						FILE_ATTRIBUTE_NORMAL,	// |FILE_FLAG_NO_BUFFERING,
    						NULL);

		if ( hFile == INVALID_HANDLE_VALUE )
		{
			DebugPrint( ("MM_DeviceIOControl:: CreateFile(%s, Ops 0x%08x ) Failed with error %ld (0x%X) \n",
						SFTK_CTL_USER_MODE_DEVICE_NAME, OperationCode, GetLastError(), GetLastError()));
			status = GetLastError();
			return status;
		}
	}
	else
		hFile = Handle;

	status = DeviceIoControl (	hFile, 
								OperationCode, 
								InBuf, 
								InLen, 
								OutBuf,
								OutLen,
								PtrBytesReturned,
								NULL );
	if ( !status ) 
	{ // failed
		DebugPrint( ("MM_DeviceIOControl:: DeviceIoControl(%s, Ops 0x%08x ) Failed with error %ld (0x%X) \n",
						SFTK_CTL_USER_MODE_DEVICE_NAME, OperationCode, GetLastError(), GetLastError()));
		status = GetLastError();
	} 
	else
		status	= NO_ERROR;
	
	if ( (Handle == NULL) || (Handle == INVALID_HANDLE_VALUE) )
	{
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle( hFile );
	}

	return status;
} // MM_DeviceIOControl()


//
// TRUE means Success else Failed
//
ULONG
MM_SharedIPC_Open(PGMM_INFO	PtrGMM_Info)
{
	DWORD		length = 0;
	DWORD		dw = 0;
	BOOLEAN		bCreatedFile = FALSE;

	if(PtrGMM_Info->OpenedSMHandle == TRUE)
	{
		DebugPrint( ("MM_SharedIPC_Open:: FIXME FIXME (PtrGMM_Info->OpenedSMHandle == TRUE) Already opened !!! FIXME FIXME  \n"));
		return TRUE;
	}
	PtrGMM_Info->OpenedSMHandle = FALSE;
	PtrGMM_Info->HFileMap		= NULL;
	PtrGMM_Info->HFile			= NULL;
	PtrGMM_Info->PtrSM_Cmd		= NULL;

	// check for previously created user shared memory file
	// OPEN_EXISTING fails if file does not exist
	PtrGMM_Info->HFile = CreateFile(SFTK_SHAREDMEMORY_USER_PATHNAME,		//	"Sftk_SharedMemory"
									GENERIC_READ | GENERIC_WRITE,
									FILE_SHARE_READ | FILE_SHARE_WRITE,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL);

	//	dw = GetLastError();
	if (PtrGMM_Info->HFile == INVALID_HANDLE_VALUE)
	{
		// user shared memory file does not exist, check for kernel
		// shared memory section object (created by DCSCache)
		PtrGMM_Info->HFileMap = OpenFileMapping(PAGE_READWRITE, // FILE_MAP_ALL_ACCESS, // FILE_MAP_READ,
	  											0,
												SFTK_SHAREDMEMORY_USER_PATHNAME);
		// dw = GetLastError();
		if (PtrGMM_Info->HFileMap == NULL || PtrGMM_Info->HFileMap == INVALID_HANDLE_VALUE)
		{
			// neither user shared memory file nor kernel shared memory
			// section object exist, create user shared memory file
			PtrGMM_Info->HFile =	CreateFile(SFTK_SHAREDMEMORY_USER_PATHNAME,
											   GENERIC_READ | GENERIC_WRITE,
											   FILE_SHARE_READ | FILE_SHARE_WRITE,
											   NULL,
											   CREATE_NEW,
											   FILE_ATTRIBUTE_NORMAL,
											   NULL);		
			if ( (PtrGMM_Info->HFile == NULL) || (PtrGMM_Info->HFile == INVALID_HANDLE_VALUE) )
			{ // Failed
				DebugPrint( ("MM_SharedIPC_Open:: CreateFile(%s) Failed with error %ld (0x%X) \n",
									SFTK_SHAREDMEMORY_USER_PATHNAME, GetLastError(), GetLastError()));
				return GetLastError();
			}
			
 			// Have Handle (hFile) to shared memory section object. Open file mapping
			PtrGMM_Info->HFileMap = CreateFileMapping(	PtrGMM_Info->HFile, NULL, PAGE_READWRITE,
														0, SFTK_SHAREDMEMORY_SIZE,
														SFTK_SHAREDMEMORY_USER_PATHNAME);

			if ( (PtrGMM_Info->HFileMap == NULL) || (PtrGMM_Info->HFileMap == INVALID_HANDLE_VALUE) )
			{ // Failed
				DebugPrint( ("MM_SharedIPC_Open:: CreateFileMapping(%s) Failed with error %ld (0x%X) \n",
									SFTK_SHAREDMEMORY_USER_PATHNAME, GetLastError(), GetLastError()));
				CloseHandle(PtrGMM_Info->HFile);
				return GetLastError();
			}

			bCreatedFile = TRUE;

		} // end if (hFileMap == NULL || hFileMap == INVALID_HANDLE_VALUE)

	} // end if (PtrGMM_Info->HFile == INVALID_HANDLE_VALUE)
	else 
	{	// previously created user shared memory file opened successfully
		// Have Handle (hFile) to shared memory file. Open file mapping
		PtrGMM_Info->HFileMap = CreateFileMapping(	PtrGMM_Info->HFile, NULL, PAGE_READWRITE,
													0, SFTK_SHAREDMEMORY_SIZE,
													SFTK_SHAREDMEMORY_USER_PATHNAME);

		// dw = GetLastError();
		if (PtrGMM_Info->HFileMap == NULL)
		{
			DebugPrint( ("MM_SharedIPC_Open():: CreateFileMapping(%s) Failed with GetLastError %ld (0x%X), returning LastError!! \n",
						SFTK_SHAREDMEMORY_USER_PATHNAME, GetLastError(), GetLastError()));
			CloseHandle(PtrGMM_Info->HFile);
			return GetLastError();
		}
	}

	// Get pointer to shared memory file/section object, map entire
	// file, no offsets. Map as read/write to allow zeroing of storage
	PtrGMM_Info->PtrSM_Cmd = (PSM_CMD) MapViewOfFile(	PtrGMM_Info->HFileMap, 
														FILE_MAP_WRITE | FILE_MAP_READ, 
														0, 0, 0);

	if (PtrGMM_Info->PtrSM_Cmd == NULL)
	{
		CloseHandle(PtrGMM_Info->HFileMap);
		CloseHandle(PtrGMM_Info->HFile);

		PtrGMM_Info->OpenedSMHandle = FALSE;
		PtrGMM_Info->HFileMap		= NULL;
		PtrGMM_Info->HFile			= NULL;
		PtrGMM_Info->PtrSM_Cmd		= NULL;

		return GetLastError();
	}

	if (bCreatedFile == TRUE)
	{ // Since we created zero out memory
		RtlZeroMemory(PtrGMM_Info->PtrSM_Cmd, SFTK_SHAREDMEMORY_SIZE);
	}

	PtrGMM_Info->OpenedSMHandle		= TRUE;	// success
	

	PtrGMM_Info->H_SMNamedEvent = CreateEvent(	NULL,		// SD = NULL
												FALSE,		// bManualReset = FALSE, means Synchronization event, auto reset by system
												FALSE,		// bInitialOwner = FALSE, No Ownership 
												SFTK_SHAREDMEMORY_USER_EVENT);

	if (PtrGMM_Info->H_SMNamedEvent == NULL)
	{ // Error Failed
		ULONG	lastError = GetLastError();

		DebugPrint( ("MM_SharedIPC_Open():: CreateMutex(%s) Failed with GetLastError %ld (0x%X), returning LastError!! \n",
						SFTK_SHAREDMEMORY_USER_EVENT, GetLastError(), GetLastError()));
		
		if (PtrGMM_Info->PtrSM_Cmd)
			UnmapViewOfFile(PtrGMM_Info->PtrSM_Cmd);

		CloseHandle(PtrGMM_Info->HFileMap);
		CloseHandle(PtrGMM_Info->HFile);

		PtrGMM_Info->OpenedSMHandle = FALSE;
		PtrGMM_Info->HFileMap		= NULL;
		PtrGMM_Info->HFile			= NULL;
		PtrGMM_Info->PtrSM_Cmd		= NULL;

		return lastError;
	}
	
	return NO_ERROR;
} // MM_SharedIPC_Open()

BOOL	
MM_SharedIPC_Close(PGMM_INFO	PtrGMM_Info)
{
	if(PtrGMM_Info->OpenedSMHandle == TRUE)
	{	
		if ( !(PtrGMM_Info->HFileMap == NULL || PtrGMM_Info->HFileMap == INVALID_HANDLE_VALUE) )
		{
			UnmapViewOfFile(PtrGMM_Info->PtrSM_Cmd);
			CloseHandle(PtrGMM_Info->HFileMap);

			if ( !(PtrGMM_Info->HFile == NULL || PtrGMM_Info->HFile == INVALID_HANDLE_VALUE) )
				CloseHandle(PtrGMM_Info->HFile);
		}
	}	

	PtrGMM_Info->OpenedSMHandle = FALSE;
	PtrGMM_Info->HFileMap		= NULL;
	PtrGMM_Info->HFile			= NULL;
	PtrGMM_Info->PtrSM_Cmd		= NULL;

	// Close Named Mutex
	if (PtrGMM_Info->H_SMNamedEvent)
		CloseHandle(PtrGMM_Info->H_SMNamedEvent);

	PtrGMM_Info->H_SMNamedEvent = NULL;

	return TRUE;
} // MM_SharedIPC_Close()

DWORD
SM_Thread( PGMM_INFO PtrGMM_Info)
{
	ULONG				status;
	HANDLE				pHandleArray[2];
	BOOLEAN				gotWork, bret;
	ULONG				numberOfPagesAllocated, numberOfChunksAllocated;
	ULONG				totalAllocate;
	PULONG				aPFNs	= NULL;
	ULONG				count	= 2;
	PSM_CMD				pSM_Cmd = PtrGMM_Info->PtrSM_Cmd;
	PSET_MM_RAW_MEMORY	pmm_raw_memory;


	pHandleArray[0] = PtrGMM_Info->HThreadTerminateEvent;	// Terminate Thread event signalled
	pHandleArray[1] = PtrGMM_Info->H_SMNamedEvent;			// Shared Memory map Named Mutex to get ownership

	while (PtrGMM_Info->TerminateThread == FALSE)
	{
		gotWork = FALSE;

		status = WaitForMultipleObjects(	count,			// nCount
											pHandleArray,	// HANDLE *lpHandles,
											FALSE,			// bWaitAll = FALSE
											PtrGMM_Info->ThreadWaitMilliseconds);   // five-second time-out interval
		switch (status) 
		{
			case WAIT_OBJECT_0+0:	// The Terminate Thread Event got signalled
									break; 
			case WAIT_OBJECT_0+1:	 // The Named Mutex Got signalled, Got Ownership to Shared memory map 
									gotWork = TRUE;
									break; 
			case WAIT_TIMEOUT:		// Timeout
			default:
									break;
		} // switch (status) 

		if (gotWork == FALSE)
			continue;	 // Either Timeout occured or TerminateThreadEvent triggered, either 
						// case we continue back to while loop

		if (pSM_Cmd->Executed == CMD_EXECUTE_COMPLETE)
		{
			DebugPrint( ("SM_Thread: pSM_Cmd->Executed %d == CMD_EXECUTE_COMPLETE, We got our command result back, Ignoring this and going for wait!! \n", pSM_Cmd->Executed));
			// signalled it again since its sychronization event, it got unsignalled
			bret = SetEvent(PtrGMM_Info->H_SMNamedEvent);
			if (bret == FALSE)
			{
				DebugPrint( ("SM_Thread():: FIXME FIXME :: SetEvent() Failed With LastError %d (0x%08x), FIXME FIXME !! \n",
													GetLastError(), GetLastError() ));
			}
			Sleep(PtrGMM_Info->ThreadSleepMilliseconds);
			continue;
		}

		// We reach here means We got Mutex (Shared Memory) Ownership, executes command
		switch(pSM_Cmd->Command)
		{ // Execute Driver specified Command
			case CMD_MM_ALLOC:	// Allocate Memory

					DebugPrint( ("SM_Thread: CMD_MM_ALLOC Recieved, Processing!! \n"));

					pSM_Cmd->Status		= SM_STATUS_SUCCESS;

					pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
					aPFNs			= (PULONG) &pmm_raw_memory->ArrayOfMem;

					// Calculate proper increments of allocations
					// totalAllocate = (ULONG) (PtrGMM_Info->IncrementAllocationChunkSizeInMB * ONEMILLION);
					totalAllocate = (ULONG) pmm_raw_memory->TotalMemorySize;

					if ( (PtrGMM_Info->TotalMemoryAllocated + (ULONGLONG) totalAllocate) > 
								(ULONGLONG) (PtrGMM_Info->MaxAllocatPhysMemInMB * ONEMILLION) )
					{
						totalAllocate = (ULONG) 
								( ((ULONGLONG) (PtrGMM_Info->MaxAllocatPhysMemInMB * ONEMILLION)) - 
										PtrGMM_Info->TotalMemoryAllocated);
					}
										
					if (PtrGMM_Info->UsedAWE == TRUE)
					{ // if : AWE is supported
						ROUND_DOWN(totalAllocate, PtrGMM_Info->PageSize);
 
						if (totalAllocate < PtrGMM_Info->PageSize)
						{ // We are done, can't allocate any more memory
							pSM_Cmd->Status = SM_ERR_MM_REACH_LIMIT;
							DebugPrint( ("SM_Thread: CMD_MM_ALLOC(totalAllocate %d ) Can't allocate any more pages, so returning error!! \n", totalAllocate));
							break;
						}
						numberOfPagesAllocated = totalAllocate / PtrGMM_Info->PageSize;

						RtlZeroMemory(aPFNs, numberOfPagesAllocated);
						
						bret = Allocate_AWE_Memory( PtrGMM_Info->ProcessHandle,
													&numberOfPagesAllocated, 
													FALSE,
													&aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_ALLOC_FAILED;
							DebugPrint( ("SM_Thread: Allocate_AWE_Memory(NumPages %d ) Failed, returning error %d\n", numberOfPagesAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						pmm_raw_memory->NumberOfArray	= numberOfPagesAllocated;
						pmm_raw_memory->ChunkSize		= PtrGMM_Info->PageSize;
					} // if : AWE is supported
					else
					{ // else : AWE is NOT supported
						ROUND_DOWN(totalAllocate, PtrGMM_Info->VAllocChunkSize);

						if (totalAllocate < PtrGMM_Info->VAllocChunkSize)
						{ // We are done, can't allocate any more memory
							pSM_Cmd->Status = SM_ERR_MM_REACH_LIMIT;
							DebugPrint( ("SM_Thread: CMD_MM_ALLOC(totalAllocate %d ) Can't allocate any more pages, so returning error!! \n", totalAllocate));
							break;
						}
						numberOfChunksAllocated = totalAllocate / PtrGMM_Info->VAllocChunkSize;

						RtlZeroMemory(aPFNs, numberOfChunksAllocated);
						
						// Test_GlobalMemoryStatusEx( &statex );
						bret = Allocate_Virtual_Memory_Pages(	&numberOfChunksAllocated, 
																PtrGMM_Info->VAllocChunkSize,
																FALSE,
																&aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_ALLOC_FAILED;
							DebugPrint( ("SM_Thread: Allocate_Virtual_Memory_Pages(NumPages %d ) Failed, returning error %d\n", numberOfChunksAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						pmm_raw_memory->NumberOfArray	= numberOfChunksAllocated;
						pmm_raw_memory->ChunkSize		= PtrGMM_Info->VAllocChunkSize;
					} // else : AWE is NOT supported

					pmm_raw_memory->TotalMemorySize = (pmm_raw_memory->NumberOfArray * pmm_raw_memory->ChunkSize);
					//pmm_raw_memory->ArrayOfMem		= aPFNs;

					// Increment Global Total Mem Allocation counter
					PtrGMM_Info->TotalMemoryAllocated += (ULONGLONG) pmm_raw_memory->TotalMemorySize;
					break;

			case CMD_MM_FREE: // Free memory

					DebugPrint( ("SM_Thread: CMD_MM_FREE Recieved, Processing!! \n"));

					pSM_Cmd->Status = SM_STATUS_SUCCESS;

					pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
					aPFNs			= (PULONG) &pmm_raw_memory->ArrayOfMem;

					if (pmm_raw_memory->TotalMemorySize != (pmm_raw_memory->NumberOfArray * pmm_raw_memory->ChunkSize))
					{
						DebugPrint( ("SM_Thread: FIXME FIXME :: pmm_raw_memory->TotalMemorySize %d != %d (pmm_raw_memory->NumberOfArray %d * pmm_raw_memory->ChunkSize %d) \n", 
										pmm_raw_memory->TotalMemorySize, (pmm_raw_memory->NumberOfArray * pmm_raw_memory->ChunkSize),
										pmm_raw_memory->NumberOfArray, pmm_raw_memory->ChunkSize));
					}

					if (PtrGMM_Info->UsedAWE == TRUE)
					{ // if : AWE is supported
						numberOfPagesAllocated = pmm_raw_memory->NumberOfArray;

						bret = Free_AWE_Memory(	GMM_Info.ProcessHandle,
												&numberOfPagesAllocated, 
												numberOfPagesAllocated,	
												FALSE,		
												&aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_FREE_FAILED;
							DebugPrint( ("SM_Thread: FIXME FIXME Free_AWE_Memory(NumPages %d ) Failed, returning error %d\n", numberOfPagesAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						RtlZeroMemory(aPFNs, numberOfPagesAllocated);

						if ( pmm_raw_memory->ChunkSize != PtrGMM_Info->PageSize)
						{
							DebugPrint( ("SM_Thread: FIXME FIXME :: pmm_raw_memory->ChunkSize %d != PtrGMM_Info->PageSize %d \n", pmm_raw_memory->ChunkSize, PtrGMM_Info->PageSize));
						}

						totalAllocate = (numberOfPagesAllocated * PtrGMM_Info->PageSize);
					} // if : AWE is supported
					else
					{ // else : AWE is NOT supported
						numberOfChunksAllocated = pmm_raw_memory->NumberOfArray;

						bret = Free_Virtual_Memory_Pages(	numberOfChunksAllocated,	
															FALSE,		
															&aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_FREE_FAILED;
							DebugPrint( ("SM_Thread: FIXME FIXME Free_Virtual_Memory_Pages(NumPages %d ) Failed, returning error %d\n", numberOfChunksAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						RtlZeroMemory(aPFNs, numberOfChunksAllocated);

						if ( pmm_raw_memory->ChunkSize != PtrGMM_Info->VAllocChunkSize)
						{
							DebugPrint( ("SM_Thread: FIXME FIXME :: pmm_raw_memory->ChunkSize %d != PtrGMM_Info->PageSize %d \n", pmm_raw_memory->ChunkSize, PtrGMM_Info->VAllocChunkSize));
						}

						totalAllocate = (numberOfChunksAllocated * PtrGMM_Info->VAllocChunkSize);
					} // else : AWE is NOT supported

					// Decrement Global Total Mem Allocation counter
					PtrGMM_Info->TotalMemoryAllocated -= (ULONGLONG) totalAllocate;
					if ((LONGLONG) PtrGMM_Info->TotalMemoryAllocated < (LONGLONG) 0)
					{
						DebugPrint( ("MM_DeInit():: *** FIXME FIXME **** GMM_Info.TotalMemoryAllocated %I64d != (ULONGLONG) 0 .!! \n",
													PtrGMM_Info->TotalMemoryAllocated));
						PtrGMM_Info->TotalMemoryAllocated = 0;
						// TODO :Log Event 
						// Put break point here for Debugging !!!
					}
					break;

			default:
					pSM_Cmd->Status = SM_ERR_INVALID_COMMAND;
					DebugPrint( ("SM_Thread():: INVALID Command %d (0x%08x) From Driver!!!  returning Error 0x%08x !! \n",
												pSM_Cmd->Command, pSM_Cmd->Status));
					break;
		} // switch(pSM_Cmd->Command)

		// ReleaseMutex(PtrGMM_Info->H_SMNamedEvent);
		pSM_Cmd->Executed	= CMD_EXECUTE_COMPLETE;	// command executed

		if ( (pSM_Cmd->Command == CMD_MM_FREE) || (pSM_Cmd->Command == CMD_MM_ALLOC) || (pSM_Cmd->Command == CMD_TERMINATE) )
		{
			DebugPrint( ("\n\n GMM_Info.TotalMemoryAllocated %I64d ( %d MB), MaxAllocatPhysMemInMB %d MB !! \n\n",
				PtrGMM_Info->TotalMemoryAllocated, (ULONG)(PtrGMM_Info->TotalMemoryAllocated / ONEMILLION), 
				PtrGMM_Info->MaxAllocatPhysMemInMB ) );
		}

		bret = SetEvent(PtrGMM_Info->H_SMNamedEvent);
		if (bret == FALSE)
		{
			DebugPrint( ("SM_Thread():: FIXME FIXME :: SetEvent() Failed With LastError %d (0x%08x), FIXME FIXME !! \n",
												GetLastError(), GetLastError() ));
		}

		Sleep(PtrGMM_Info->ThreadSleepMilliseconds);
	} // while (PtrGMM_Info->TerminateThread == FALSE)

	return NO_ERROR;
} // SM_Thread()

DWORD
SM_IOCTL_Thread( PGMM_INFO PtrGMM_Info)
{
	ULONG				result, returnBytes;
	ULONG				numberOfPagesAllocated, numberOfChunksAllocated;
	ULONG				totalAllocate;
	BOOLEAN				bret;
	PULONG				aPFNs;
	PSET_MM_RAW_MEMORY	pmm_raw_memory;
	PSM_CMD				pSM_Cmd = PtrGMM_Info->PtrSM_Cmd;
	HANDLE				handle = INVALID_HANDLE_VALUE;

	pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
	aPFNs			= (PULONG) &pmm_raw_memory->ArrayOfMem;

	handle = CreateFile(	SFTK_CTL_USER_MODE_DEVICE_NAME,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
    						NULL,
    						OPEN_EXISTING,
    						FILE_ATTRIBUTE_NORMAL, // |FILE_FLAG_NO_BUFFERING,
    						NULL);

	if ( handle == INVALID_HANDLE_VALUE )
	{
		result = GetLastError();	// failed
		DebugPrint( ("MM_Start:: CreateFile(%s) Failed with error %ld (0x%X) \n",
							SFTK_CTL_USER_MODE_DEVICE_NAME, GetLastError(), GetLastError()));
		// goto done;
		// TODO : Log Event here
	}
	WaitForSingleObject( PtrGMM_Info->HThreadTerminateEvent, INFINITE); 

	pSM_Cmd->Executed	= CMD_EXECUTE_COMPLETE;
	pSM_Cmd->Command	= CMD_NONE;	
	pSM_Cmd->Status		= SM_STATUS_SUCCESS;

	while (PtrGMM_Info->TerminateThread == FALSE)
	{
		result = MM_DeviceIOControl(	handle, 
										IOCTL_MM_CMD, // IOCTL_GET_MM_DATABASE_SIZE,
										pSM_Cmd, SFTK_SHAREDMEMORY_SIZE, 
										pSM_Cmd, SFTK_SHAREDMEMORY_SIZE, 
										&returnBytes);
		if (result != NO_ERROR)
		{
			DebugPrint( ("SM_IOCTL_Thread:: MM_DeviceIOControl(%s) IOCTL_MM_CMD Failed with error %ld (0x%X), Terminating Thread !!! \n",
								SFTK_CTL_USER_MODE_DEVICE_NAME, result, result));
			// TODO : Log Event message here
			break;
		}

		if(pSM_Cmd->Command == CMD_TERMINATE)
		{
			DebugPrint( ("SM_IOCTL_Thread: pSM_Cmd->Command %d == CMD_TERMINATE,  Terminating Thread!! \n", 
													pSM_Cmd->Command));
			break;
		}

		if (pSM_Cmd->Executed == CMD_EXECUTE_COMPLETE)
		{
			DebugPrint( ("SM_IOCTL_Thread: pSM_Cmd->Command %d,  pSM_Cmd->Executed %d == CMD_EXECUTE_COMPLETE, We got our command result back, Ignoring this and Sending IOCTL again!! \n", 
													pSM_Cmd->Command, pSM_Cmd->Executed));
			RtlZeroMemory(pSM_Cmd, sizeof(SM_CMD));
			pSM_Cmd->Command	= CMD_NONE;
			pSM_Cmd->Executed	= CMD_EXECUTE_COMPLETE;
			continue;
		}

		// Execute command
		pSM_Cmd->Status		= SM_STATUS_SUCCESS;
		pSM_Cmd->Executed	= CMD_EXECUTE_COMPLETE;

		switch(pSM_Cmd->Command)
		{ // Execute Driver specified Command
			case CMD_MM_ALLOC:	// Allocate Memory

					DebugPrint( ("SM_IOCTL_Thread: CMD_MM_ALLOC Recieved, Processing!! \n"));

					pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
					aPFNs			= (PULONG) &pmm_raw_memory->ArrayOfMem;

					// Calculate proper increments of allocations
					// totalAllocate = (ULONG) (PtrGMM_Info->IncrementAllocationChunkSizeInMB * ONEMILLION);
					totalAllocate = (ULONG) pmm_raw_memory->TotalMemorySize;

					if ( (PtrGMM_Info->TotalMemoryAllocated + (ULONGLONG) totalAllocate) > 
								(ULONGLONG) (PtrGMM_Info->MaxAllocatPhysMemInMB * ONEMILLION) )
					{
						totalAllocate = (ULONG) 
								( ((ULONGLONG) (PtrGMM_Info->MaxAllocatPhysMemInMB * ONEMILLION)) - 
										PtrGMM_Info->TotalMemoryAllocated);
					}
										
					if (PtrGMM_Info->UsedAWE == TRUE)
					{ // if : AWE is supported
						ROUND_DOWN(totalAllocate, PtrGMM_Info->PageSize);
 
						if (totalAllocate < PtrGMM_Info->PageSize)
						{ // We are done, can't allocate any more memory
							pSM_Cmd->Status = SM_ERR_MM_REACH_LIMIT;
							DebugPrint( ("SM_IOCTL_Thread: CMD_MM_ALLOC(totalAllocate %d ) Can't allocate any more pages, so returning error!! \n", totalAllocate));
							break;
						}
						numberOfPagesAllocated = totalAllocate / PtrGMM_Info->PageSize;

						RtlZeroMemory(aPFNs, numberOfPagesAllocated);
						
						bret = Allocate_AWE_Memory( PtrGMM_Info->ProcessHandle,
													&numberOfPagesAllocated, 
													FALSE,
													(PULONG *) aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_ALLOC_FAILED;
							DebugPrint( ("SM_IOCTL_Thread: Allocate_AWE_Memory(NumPages %d ) Failed, returning error %d\n", numberOfPagesAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						pmm_raw_memory->NumberOfArray	= numberOfPagesAllocated;
						pmm_raw_memory->ChunkSize		= PtrGMM_Info->PageSize;
					} // if : AWE is supported
					else
					{ // else : AWE is NOT supported
						ROUND_DOWN(totalAllocate, PtrGMM_Info->VAllocChunkSize);

						if (totalAllocate < PtrGMM_Info->VAllocChunkSize)
						{ // We are done, can't allocate any more memory
							pSM_Cmd->Status = SM_ERR_MM_REACH_LIMIT;
							DebugPrint( ("SM_IOCTL_Thread: CMD_MM_ALLOC(totalAllocate %d ) Can't allocate any more pages, so returning error!! \n", totalAllocate));
							break;
						}
						numberOfChunksAllocated = totalAllocate / PtrGMM_Info->VAllocChunkSize;

						RtlZeroMemory(aPFNs, numberOfChunksAllocated);
						
						// Test_GlobalMemoryStatusEx( &statex );
						bret = Allocate_Virtual_Memory_Pages(	&numberOfChunksAllocated, 
																PtrGMM_Info->VAllocChunkSize,
																FALSE,
																(PULONG *) aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_ALLOC_FAILED;
							DebugPrint( ("SM_IOCTL_Thread: Allocate_Virtual_Memory_Pages(NumPages %d ) Failed, returning error %d\n", numberOfChunksAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						pmm_raw_memory->NumberOfArray	= numberOfChunksAllocated;
						pmm_raw_memory->ChunkSize		= PtrGMM_Info->VAllocChunkSize;
					} // else : AWE is NOT supported

					pmm_raw_memory->TotalMemorySize = (pmm_raw_memory->NumberOfArray * pmm_raw_memory->ChunkSize);
					//pmm_raw_memory->ArrayOfMem		= aPFNs;

					// Increment Global Total Mem Allocation counter
					PtrGMM_Info->TotalMemoryAllocated += (ULONGLONG) pmm_raw_memory->TotalMemorySize;
					break;

			case CMD_MM_FREE: // Free memory

					DebugPrint( ("SM_IOCTL_Thread: CMD_MM_FREE Recieved, Processing!! \n"));

					pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
					aPFNs			= (PULONG) &pmm_raw_memory->ArrayOfMem;

					if (pmm_raw_memory->TotalMemorySize != (pmm_raw_memory->NumberOfArray * pmm_raw_memory->ChunkSize))
					{
						DebugPrint( ("SM_IOCTL_Thread: FIXME FIXME :: pmm_raw_memory->TotalMemorySize %d != %d (pmm_raw_memory->NumberOfArray %d * pmm_raw_memory->ChunkSize %d) \n", 
										pmm_raw_memory->TotalMemorySize, (pmm_raw_memory->NumberOfArray * pmm_raw_memory->ChunkSize),
										pmm_raw_memory->NumberOfArray, pmm_raw_memory->ChunkSize));
					}

					if (PtrGMM_Info->UsedAWE == TRUE)
					{ // if : AWE is supported
						numberOfPagesAllocated = pmm_raw_memory->NumberOfArray;

						bret = Free_AWE_Memory(	GMM_Info.ProcessHandle,
												&numberOfPagesAllocated, 
												numberOfPagesAllocated,	
												FALSE,		
												(PULONG *) aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_FREE_FAILED;
							DebugPrint( ("SM_IOCTL_Thread: FIXME FIXME Free_AWE_Memory(NumPages %d ) Failed, returning error %d\n", numberOfPagesAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						RtlZeroMemory(aPFNs, numberOfPagesAllocated);

						if ( pmm_raw_memory->ChunkSize != PtrGMM_Info->PageSize)
						{
							DebugPrint( ("SM_IOCTL_Thread: FIXME FIXME :: pmm_raw_memory->ChunkSize %d != PtrGMM_Info->PageSize %d \n", pmm_raw_memory->ChunkSize, PtrGMM_Info->PageSize));
						}

						totalAllocate = (numberOfPagesAllocated * PtrGMM_Info->PageSize);
					} // if : AWE is supported
					else
					{ // else : AWE is NOT supported
						numberOfChunksAllocated = pmm_raw_memory->NumberOfArray;

						bret = Free_Virtual_Memory_Pages(	numberOfChunksAllocated,	
															FALSE,		
															(PULONG *) aPFNs );
						if (bret == FALSE)
						{ // failed
							pSM_Cmd->Status = SM_ERR_MM_FREE_FAILED;
							DebugPrint( ("SM_IOCTL_Thread: FIXME FIXME Free_Virtual_Memory_Pages(NumPages %d ) Failed, returning error %d\n", numberOfChunksAllocated, pSM_Cmd->Status));
							// TODO : Log Event Message here
							break;
						}

						RtlZeroMemory(aPFNs, numberOfChunksAllocated);

						if ( pmm_raw_memory->ChunkSize != PtrGMM_Info->VAllocChunkSize)
						{
							DebugPrint( ("SM_IOCTL_Thread: FIXME FIXME :: pmm_raw_memory->ChunkSize %d != PtrGMM_Info->PageSize %d \n", pmm_raw_memory->ChunkSize, PtrGMM_Info->VAllocChunkSize));
						}

						totalAllocate = (numberOfChunksAllocated * PtrGMM_Info->VAllocChunkSize);
					} // else : AWE is NOT supported

					// Decrement Global Total Mem Allocation counter
					PtrGMM_Info->TotalMemoryAllocated -= (ULONGLONG) totalAllocate;
					if ((LONGLONG) PtrGMM_Info->TotalMemoryAllocated < (LONGLONG) 0)
					{
						DebugPrint( ("MM_DeInit():: *** FIXME FIXME **** GMM_Info.TotalMemoryAllocated %I64d != (ULONGLONG) 0 .!! \n",
													PtrGMM_Info->TotalMemoryAllocated));
						PtrGMM_Info->TotalMemoryAllocated = 0;
						// TODO :Log Event 
						// Put break point here for Debugging !!!
					}
					break;

			default:
					pSM_Cmd->Status = SM_ERR_INVALID_COMMAND;
					DebugPrint( ("SM_IOCTL_Thread():: INVALID Command %d (0x%08x) From Driver!!!  returning Error 0x%08x !! \n",
												pSM_Cmd->Command, pSM_Cmd->Status));
					break;
		} // switch(pSM_Cmd->Command)

		if ( (pSM_Cmd->Command == CMD_MM_FREE) || (pSM_Cmd->Command == CMD_MM_ALLOC) || (pSM_Cmd->Command == CMD_TERMINATE) )
		{
			DebugPrint( ("\n\n GMM_Info.TotalMemoryAllocated %I64d ( %d MB), MaxAllocatPhysMemInMB %d MB !! \n\n",
				PtrGMM_Info->TotalMemoryAllocated, (ULONG)(PtrGMM_Info->TotalMemoryAllocated / ONEMILLION), 
				PtrGMM_Info->MaxAllocatPhysMemInMB ) );
		}
	} // while (PtrGMM_Info->TerminateThread == FALSE)

	PtrGMM_Info->TerminateThread = TRUE;

	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);

	return NO_ERROR;
} // SM_IOCTL_Thread()


//
//	Function:	MM_is_AWESupported()
//	Parameters: 
//	Returns: TRUE means AWE is supported for current process by system else FALSE
//
BOOLEAN
MM_is_AWESupported()
{
	BOOLEAN	aweSupported	= FALSE;	// Deafault not supported
	HANDLE	processHandle	= GetCurrentProcess();
	BOOLEAN	bret;
	ULONG	numberOfPagesAllocated	= 10;	// only for testing AWE suport
	PULONG	aPFNs					= NULL;	// page info; holds opaque data

	// Enable the privilege.
	bret = LoggedSetLockPagesPrivilege( processHandle, TRUE );
	if( bret == FALSE) 
	{ // Failed
		DebugPrint( ("MM_is_AWESupported() : LoggedSetLockPagesPrivilege(0x%x) Failed. GetLastError - %d (0x%x)\n",
						processHandle, GetLastError(), GetLastError()));
		// GetErrorText();
		goto done;
	}

	bret = Allocate_AWE_Memory( processHandle,
								&numberOfPagesAllocated, 
								TRUE,
								&aPFNs );
	if (bret == FALSE)
	{ // failed
		DebugPrint( ("MM_is_AWESupported() : Allocate_AWE_Memory() Failed. GetLastError - %d (0x%x)\n",
						GetLastError(), GetLastError()));
		goto done;
	}

	bret = Free_AWE_Memory(	processHandle,
							&numberOfPagesAllocated,	
							1,							// free 1 pages at atime 
							TRUE,		
							&aPFNs );
	if (bret == FALSE)
	{ // failed
		DebugPrint( ("MM_is_AWESupported() : Free_AWE_Memory() Failed. GetLastError - %d (0x%x)\n",
						GetLastError(), GetLastError()));
		goto done;
	}

	aweSupported = TRUE;	// AWE is supported
done:
	if (aPFNs)
		VirtualFree(aPFNs, 0, MEM_RELEASE);

	return aweSupported;
} // MM_is_AWESupported()

//   LoggedSetLockPagesPrivilege: a function to obtain, if possible, or
//   release the privilege of locking physical pages.
//
//   Inputs:
//
//       HANDLE hProcess: Handle for the process for which the
//       privilege is needed
//
//       BOOL bEnable: Enable (TRUE) or disable?
//
//   Return value: TRUE indicates success, FALSE failure.
// 
//
BOOL
LoggedSetLockPagesPrivilege ( HANDLE	hProcess,
                              BOOL		bEnable)
{
	struct 
	{
		DWORD				Count;
		LUID_AND_ATTRIBUTES Privilege [1];
	} privilege_info;

	HANDLE		token;
	BOOL		result;

	// Open the token.
	result = OpenProcessToken ( hProcess,
								TOKEN_ADJUST_PRIVILEGES,
								&token);

	if( result != TRUE ) 
	{
		DebugPrint( ("LoggedSetLockPagesPrivilege () : OpenProcessToken ( ) Failed. GetLastError - %d (0x%x)\n",
				GetLastError(), GetLastError()));
		
		return FALSE;
	}

	// Enable or disable?
	privilege_info.Count = 1;

	if( bEnable ) 
		privilege_info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	else 
		privilege_info.Privilege[0].Attributes = 0;

	// Get the LUID.
	result = LookupPrivilegeValue ( NULL,
									SE_LOCK_MEMORY_NAME,
									&(privilege_info.Privilege[0].Luid));

	if( result != TRUE ) 
	{
		DebugPrint( ("LoggedSetLockPagesPrivilege () : LookupPrivilegeValue ( ) Failed. GetLastError - %d (0x%x)\n",
				GetLastError(), GetLastError()));
		
		CloseHandle( token );
		return FALSE;
	}

	// Adjust the privilege.
	result = AdjustTokenPrivileges ( token, FALSE,
                                   (PTOKEN_PRIVILEGES) &privilege_info,
                                   (DWORD) NULL, 
								   (PTOKEN_PRIVILEGES) NULL, 
								   (PDWORD) NULL);

	// Check the result.
	if( result != TRUE ) 
	{
		DebugPrint( ("LoggedSetLockPagesPrivilege () : AdjustTokenPrivileges ( ) Failed. GetLastError - %d (0x%x)\n",
				GetLastError(), GetLastError()));
		CloseHandle( token );
		return FALSE;
	}	 
	else 
	{
		if( GetLastError() != ERROR_SUCCESS ) 
		{
			DebugPrint( ("Cannot enable SE_LOCK_MEMORY privilege, please check the local policy.\n"));

			DebugPrint( ("LoggedSetLockPagesPrivilege () : AdjustTokenPrivileges ( ) Failed. GetLastError - %d (0x%x)\n",
						GetLastError(), GetLastError()));
			
			CloseHandle( token );
			return FALSE;
		}
	}

  CloseHandle( token );
  return TRUE;
} // LoggedSetLockPagesPrivilege ()

#ifdef _DEBUG
VOID
SwrDebugPrint(
    // ULONG DebugPrintLevel,
    CHAR *DebugMessage,
    ...
    )
/*++
Routine Description:
    Debug print for all Swr
Arguments:
    Debug print level between 0 and 3, with 3 being the most verbose.
Return Value:
    None
--*/
{
    // if ((DebugPrintLevel <= (SwrDebug & 0x0000ffff)) || ((1 << (DebugPrintLevel + 15)) & SwrDebug)) 
	// if (SwrDebug & DebugPrintLevel)	{
		va_list ap;
		// sprintf(DebugMessage, DbgBufferString);
		va_start(ap, DebugMessage);
		_vsnprintf(DbgBufferString, DEBUG_BUFFER_LENGTH, DebugMessage, ap);
        OS_DbgPrint(DbgBufferString);
		va_end(ap);
    // }
    
} // SwrDebugPrint()
#endif

//
//	Function:	Allocate_AWE_Memory()
//	Parameters: 
//		IN		HANDLE	ProcessHandle:		IN ProcessHandle used to Allocate AWE Pages
//		IN OUT	PULONG	PtrNumberOfPages:	IN Total Number of Pages Asked to Allocate,
//											OUT Total Number of Pages Actually Allocated
//		IN BOOLEAN AllocateArrayMem,		IN TRUE means Allocate Array Memory to hold Physical pages and 
//													retrun pointer of Array Memory into PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem 		IN	Pointer to Pointer of Array Of Physical Pages memory 
//												Valid memory if AllocateArrayMem == FALSE
//											OUT it Allocates this memory if AllocateArrayMem == TRUE and Returns 
//												Valid Pointer of memory in this parameter if API returns TRUE	
// Returns: TRUE means successed else FALSE
//
BOOLEAN
Allocate_AWE_Memory(IN HANDLE	ProcessHandle, 
					IN PULONG	PtrNumberOfPages, 
					IN BOOLEAN	AllocateArrayMem,
					PULONG		*PtrArrayOfMem )
{
	BOOLEAN		bret = FALSE;
	PULONG		aPFNs = NULL;
	ULONG		numberOfPages, numberOfPagesInitial; // number of pages to request and initial number of pages requested
	ULONG		pfnArraySize;

	numberOfPages = numberOfPagesInitial = *PtrNumberOfPages;
	
	if (numberOfPages == 0)
	{
		DebugPrint( ("Allocate_AWE_Memory() : Invalid Argument, *PtrNumberOfPages %d (0x%08x), returning error \n",
					*PtrNumberOfPages,*PtrNumberOfPages ));
		goto done;
	}
	
	pfnArraySize = numberOfPages * sizeof(ULONG);
	if (AllocateArrayMem == TRUE)
	{
		// aPFNs = (ULONG_PTR *) HeapAlloc (GetProcessHeap (), 0, PFNArraySize);
		aPFNs = (ULONG *) VirtualAlloc(NULL,pfnArraySize, MEM_COMMIT, PAGE_READWRITE);
		
		if (aPFNs == NULL) 
		{
			DebugPrint( ("Allocate_AWE_Memory() : VirtualAlloc(%d) Failed to allocate on heap. GetLastError - %d (0x%x)\n",
					pfnArraySize, GetLastError(), GetLastError()));
			GetErrorText();
			goto done;
		}
	}
	else
	{
		aPFNs = (PULONG) PtrArrayOfMem;
		if (aPFNs == NULL) 
		{
			DebugPrint( ("Allocate_AWE_Memory() : Invalid Argument, PtrArrayOfMem 0x%08x, *PtrArrayOfMem 0x%08x, returning error \n",
					PtrArrayOfMem, *PtrArrayOfMem));
			goto done;
		}
	}
	RtlZeroMemory(aPFNs, pfnArraySize);

	bret = AllocateUserPhysicalPages(	ProcessHandle,
										&numberOfPages,
										aPFNs );
    
	if( bret != TRUE ) 
	{
		DebugPrint( ("Allocate_AWE_Memory() : AllocateUserPhysicalPages(h %x, NoOfPages %d ) Failed. GetLastError - %d (0x%x)\n",
				ProcessHandle, numberOfPagesInitial,
				GetLastError(), GetLastError()));

		GetErrorText();
		goto done;
	}

	if( numberOfPagesInitial != numberOfPages ) 
	{
		DebugPrint( ("Allocate_AWE_Memory() : AllocateUserPhysicalPages() successed. Asking Pages %d, Got Pages %d, GetLastError - %d (0x%x)\n",
				numberOfPagesInitial, numberOfPages, GetLastError(), GetLastError()));
		GetErrorText();
	}

	*PtrNumberOfPages = numberOfPages;
	bret = TRUE;	// successed

done:
	if (bret == TRUE)
	{ // successed
		if (AllocateArrayMem == TRUE)
			*PtrArrayOfMem = aPFNs;
	}
	else
	{ // failed
		if (AllocateArrayMem == TRUE)
		{
			if (aPFNs)
				VirtualFree(aPFNs, 0, MEM_RELEASE);
			*PtrArrayOfMem = NULL;	// Failed
		}
	}
	return bret;
} // Allocate_AWE_Memory()

//
//	Function:	Free_AWE_Memory()
//	Parameters: 
//		IN		HANDLE	ProcessHandle:		IN ProcessHandle used to Free AWE Pages
//		IN OUT	PULONG	PtrNumberOfPages:	IN Total Number of Pages to be Free,
//											OUT Total Number of Pages got Free
//		IN		ULONG	FreePagesAtATime,	IN Number of Pages to be free at a time
//		IN		BOOLEAN	FreeArrayMem,		TRUE means free memory for PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem )	IN Pointer to Pointer of Array Of Physical Pages memory
//											OUT it frees this memory if FreeArrayMem == TRUE and Returns 
//											NULL in this pointer	
// Returns: TRUE means successed else FALSE
//
BOOLEAN
Free_AWE_Memory(	IN		HANDLE	ProcessHandle,
					IN OUT	PULONG	PtrNumberOfPages,	
					IN		ULONG	FreePagesAtATime,	
					IN		BOOLEAN	FreeArrayMem,		
					IN OUT	PULONG	*PtrArrayOfMem )	
{
	BOOLEAN		bret = FALSE;
	PULONG		aPFNs = NULL;
	ULONG		numberOfPages, numberOfPagesInitial; // number of pages to request and initial number of pages requested
	ULONG		freePages, freeAskingPages, i;

	numberOfPages = numberOfPagesInitial = *PtrNumberOfPages;
	
	if (numberOfPages == 0)
	{
		DebugPrint( ("Free_AWE_Memory() : Invalid Argument, *PtrNumberOfPages %d (0x%08x), returning error \n",
					*PtrNumberOfPages,*PtrNumberOfPages ));
		bret = TRUE;	// successed
		goto done;
	}
	
	aPFNs = (PULONG) PtrArrayOfMem;
	if (aPFNs == NULL) 
	{
		DebugPrint( ("Free_AWE_Memory() : Invalid Argument, PtrArrayOfMem 0x%08x, *PtrArrayOfMem 0x%08x, returning error \n",
				PtrArrayOfMem, *PtrArrayOfMem));
		goto done;
	}

	freePages = FreePagesAtATime;
	// DebugPrint( ("\nFreeing Pages at a time: %12d\n", freePages)); 
	numberOfPages = 0;

	for (i=0; i < numberOfPagesInitial; i += freePages)
	{ // for : Do Pages at a time
		
		if (freePages > (numberOfPagesInitial - (i)))
			freePages = numberOfPagesInitial - (i);

		freeAskingPages = freePages;
		// DebugPrint( ("Index : %03d,  Physical Page 0x%08x \n", freePages, aPFNs[i]));
		bret = FreeUserPhysicalPages(	ProcessHandle,
										&freePages,
										&aPFNs[i] );

		if(( bret != TRUE ) || (freePages != freeAskingPages))
		{
			DebugPrint((" Failed: FreeUserPhysicalPages(FreeAskingPages %d Actual freePages %d) GetLastError - %d (0x%x) \n", 
																	freeAskingPages, freePages, GetLastError(),GetLastError() ));
			GetErrorText();
		}

		numberOfPages += freePages;
		freePages = FreePagesAtATime;
	} // Free the physical pages.	

	*PtrNumberOfPages = numberOfPages;
	bret = TRUE;	// successed

done:
	if (bret == TRUE) 
	{ // successed
		if (FreeArrayMem == TRUE)
		{
			if (aPFNs)
				VirtualFree(aPFNs, 0, MEM_RELEASE);
			*PtrArrayOfMem = NULL;	// Failed
		}
	}

	return bret;
} // Free_AWE_Memory()


//
//	Function:	Allocate_Virtual_Memory_Pages()
//	Parameters: 
//		IN OUT	PULONG	PtrnumberOfChunks:	IN Total Number of Chunks Asked to Allocate,
//											OUT Total Number of Chunks Actually Allocated
//		IN ULONG	ChunkSize:				IN bytes of one chunksize 
//		IN BOOLEAN AllocateArrayMem,		IN if TRUE means Allocate Array Memory to hold Virtual memory and 
//											retrun pointer of Array Memory into PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem 		IN	Pointer to Pointer of Array Of Virtual memory 
//												Valid memory if AllocateArrayMem == FALSE
//											OUT it Allocates this memory if AllocateArrayMem == TRUE and Returns 
//												Valid Pointer of memory in this parameter if API returns TRUE	
// Returns: TRUE means successed else FALSE
//
BOOLEAN
Allocate_Virtual_Memory_Pages(	IN PULONG	PtrNumberOfChunks, 
								IN ULONG	ChunkSize,
								IN BOOLEAN	AllocateArrayMem,
								PULONG		*PtrArrayOfMem )
{
	BOOLEAN		bret	= FALSE;
	PULONG		aPFNs	= NULL;
	ULONG		numberOfChunksInitial; // number of pages to request and initial number of pages requested
	ULONG		i;
	ULONG		pfnArraySize;

	numberOfChunksInitial = *PtrNumberOfChunks;
	if (numberOfChunksInitial == 0)
	{
		DebugPrint( ("Allocate_Virtual_Memory_Pages() : Invalid Argument, *PtrNumberOfChunks %d (0x%08x), returning error \n",
					*PtrNumberOfChunks,*PtrNumberOfChunks ));
		goto done;
	}

	// Check Round up Allocation unit against total number of pages
	pfnArraySize = numberOfChunksInitial * sizeof(ULONG);

	if (AllocateArrayMem == TRUE)
		aPFNs = (ULONG *) VirtualAlloc(NULL,pfnArraySize, MEM_COMMIT, PAGE_READWRITE);
	else
		aPFNs = (PULONG) PtrArrayOfMem;

	if (aPFNs == NULL) 
	{
		DebugPrint( ("Allocate_Virtual_Memory_Pages() : VirtualAlloc(%d) *PtrArrayOfMem = 0x%08x Failed to allocate on heap or invalid argument. GetLastError - %d (0x%x)\n",
					pfnArraySize, *PtrArrayOfMem, GetLastError(), GetLastError()));
		GetErrorText();
		goto done;
	}

	RtlZeroMemory(aPFNs, pfnArraySize);

	for (i=0; i < numberOfChunksInitial; i++)
	{ // for : Allocate Chunksize at a time
		// Memory allocated by this function is automatically initialized to zero, unless MEM_RESET is specified. 
		aPFNs[i] = (ULONG) VirtualAlloc(NULL, ChunkSize, MEM_COMMIT, PAGE_READWRITE);
		
		if ( (ULONG *) aPFNs[i] == NULL) 
		{
			// DebugPrint( ("Allocate_Virtual_Memory_Pages() : VirtualAlloc(%d) Failed to allocate on heap. GetLastError - %d (0x%x)\n",
			//						memsize, GetLastError(), GetLastError()));
			//GetErrorText();
			break;	// Returned as much as allocated pages
		}
		// RtlZeroMemory( (PVOID)aPFNs[i], memsize); // not needed since VirtualAlloc() returns always zero out memory
	} // for : Allocate Pages at a time

	if (i == 0)
	{ // Not even one page is allocated, so just return error, Operation is failed
		DebugPrint( ("Allocate_Virtual_Memory_Pages() : Failed:. Asking Pages %d, Got Pages %d, GetLastError - %d (0x%x)\n",
				numberOfChunksInitial, i, GetLastError(), GetLastError()));
		goto done;
	}
/*
	if( numberOfChunksInitial != i ) 
	{
		// DebugPrint( ("Allocate_Virtual_Memory_Pages() : vIRTUALaLLOC() successed. Asking Pages %d, Got Pages %d, GetLastError - %d (0x%x)\n",
		//		numberOfChunksInitial, i, GetLastError(), GetLastError()));
		// GetErrorText();
	}
*/
	*PtrNumberOfChunks = i;
	bret = TRUE;	// successed

done:
	if (bret == TRUE)
	{ // successed
		if (AllocateArrayMem == TRUE)
			*PtrArrayOfMem = aPFNs;
	}
	else
	{ // failed
		if (AllocateArrayMem == TRUE)
		{
			if (aPFNs)
				VirtualFree(aPFNs, 0, MEM_RELEASE);
			*PtrArrayOfMem = NULL;	// Failed
		}
	}
	return bret;
} // Allocate_Virtual_Memory_Pages()

//
//	Function:	Free_Virtual_Memory_Pages()
//	Parameters: 
//		IN OUT	ULONG	NumberOfChunks:		IN Total Number of Chunks in array to be Free,
//		IN		BOOLEAN	FreeArrayMem,		TRUE means free memory for PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem )	IN Pointer to Pointer of Array Of Physical Pages memory
//											OUT it frees this memory if FreeArrayMem == TRUE and Returns 
//											NULL in this pointer	
// Returns: TRUE means successed else FALSE
//
BOOLEAN
Free_Virtual_Memory_Pages(	IN OUT	ULONG	NumberOfChunks,	
							IN		BOOLEAN	FreeArrayMem,
							IN OUT	PULONG	*PtrArrayOfMem )	
{
	BOOLEAN		bret	= FALSE;
	PULONG		aPFNs	= NULL;
	ULONG		i;

	if (NumberOfChunks == 0)
	{
		DebugPrint( ("Free_Virtual_Memory_Pages() : Invalid Argument, NumberOfChunks %d (0x%08x) == 0 !! returning error \n",
					NumberOfChunks,NumberOfChunks ));
		bret = TRUE;	// successed
		goto done;
	}
	
	aPFNs = (PULONG) PtrArrayOfMem;
	if (aPFNs == NULL) 
	{
		DebugPrint( ("Free_Virtual_Memory_Pages() : Invalid Argument, PtrArrayOfMem 0x%08x, *PtrArrayOfMem 0x%08x, returning error \n",
				PtrArrayOfMem, *PtrArrayOfMem));
		goto done;
	}

	for (i=0; i < NumberOfChunks; i++)
	{ // for : Do Pages at a time
		// DebugPrint( ("Index : %03d,  Physical Page 0x%08x \n", freePages, aPFNs[i]));
		if ( (ULONG *) aPFNs[i] == NULL)
			continue;

		bret = VirtualFree( (PVOID) aPFNs[i], 0, MEM_RELEASE);
		if( bret != TRUE ) 
		{ // Failed
			DebugPrint((" Free_Virtual_Memory_Pages:: Failed: VirtualFree(Array Index %d : Memory 0x%08x) GetLastError - %d (0x%x) \n", 
																		i, aPFNs[i], GetLastError(),GetLastError() ));
			GetErrorText();
		}
		aPFNs[i] = (ULONG) NULL;
	} // for : Do Pages at a time	 

	bret = TRUE;	// successed

done:
	if (bret == TRUE) 
	{ // successed
		if (FreeArrayMem == TRUE)
		{
			if (aPFNs)
				VirtualFree(aPFNs, 0, MEM_RELEASE);
			*PtrArrayOfMem = NULL;	// Failed
		}
	}
	return bret;
} // Free_Virtual_Memory_Pages()
