//  The following sample program illustrates the Address Windowing Extensions for Windows 2000. 

//  c:\>globalex
//  78 percent of memory is in use.
//  There are   65076 total Kbytes of physical memory.
//  There are   14248 free Kbytes of physical memory.
//  There are  150960 total Kbytes of paging file.
//  There are   88360 free Kbytes of paging file.
//  There are  1fff80 total Kbytes of virtual memory.
//  There are  1fe770 free Kbytes of virtual memory.
//  There are       0 free Kbytes of extended memory.

#define _WIN32_WINNT 0x0500

#include "sftk_MM.h"

//
// Globale Variable Definations
//

//
// Function Prototype defined in this file
//
VOID 
main(int argc, char *argv[]);

BOOLEAN 
Test_AWE();

BOOLEAN
Test_VirtualMemory_Alloc(ULONG MaxNumberOfPages);

BOOLEAN
Test_VirtualMemory_Alloc(ULONG MaxNumberOfPages);

BOOLEAN 
Test_GlobalMemoryStatusEx(MEMORYSTATUSEX *statex);

BOOLEAN 
Test_GetSystemInfo(SYSTEM_INFO *SysInfo);

BOOLEAN 
Test_GlobalMemoryStatus(MEMORYSTATUS *state);

VOID	
DisplayErrorText();

extern GMM_INFO	GMM_Info;

//
// Function Definations
//
VOID 
main(int argc, char *argv[])
{
	SYSTEM_INFO			sSysInfo;
	MEMORYSTATUSEX		statex;
	BOOLEAN				bret;	
	ULONG				i, returnBytes, result;
	int					ch;

	i = result = returnBytes = 0;

	DebugPrint( ("main : ----- MM Service Testing AWE/Virtual, Maximum Available Physical/Virtual Memory %d percentage \n", 
							MM_DEFAULT_MaxAvailPhysMemUsedInPercentage));

	Test_GetSystemInfo(&sSysInfo);     // populate the system information structure
	Test_GlobalMemoryStatusEx( &statex );

	bret = MM_Start();
	if (bret == FALSE)
	{ // Function Failed
		DebugPrint( ("main : MM_Start() Failed with error %ld (0x%X) \n",GetLastError(), GetLastError()));
		goto done;
	}
	DebugPrint( ("\n\n MaxAllocatPhysMemInMB %d MB, GMM_Info.TotalMemoryAllocated %I64d ( %d MB)!! \n\n",
				GMM_Info.MaxAllocatPhysMemInMB, GMM_Info.TotalMemoryAllocated,
				(ULONG)(GMM_Info.TotalMemoryAllocated / ONEMILLION)));
/*
#if 0
	goto stop;

	// Test : using IOCTL Alloc Memory Dynamically from kerenl to service 
	for (i = 0; i < 3; i++)
	{
		result = MM_DeviceIOControl( NULL, 
									IOCTL_MM_ALLOC_RAW_MEM, // IOCTL_GET_MM_DATABASE_SIZE,
									NULL, 0,
									NULL, 0,
									&returnBytes);
		if (result != NO_ERROR)
		{
			DebugPrint( ("main : i %d, MM_DeviceIOControl(%s) IOCTL_MM_ALLOC_RAW_MEM Failed with error %ld (0x%X) \n",
								i, FTD_CTLDEV, result, result));
			break;
		}
	}

	Sleep(180000);	// in miiliseconds
	DebugPrint( ("\n\n After Alloc : MaxAllocatPhysMemInMB %d MB, GMM_Info.TotalMemoryAllocated %I64d ( %d MB)!! \n\n",
				GMM_Info.MaxAllocatPhysMemInMB, GMM_Info.TotalMemoryAllocated,
				(ULONG)(GMM_Info.TotalMemoryAllocated / ONEMILLION)));
	goto stop;
	
	Test_GlobalMemoryStatusEx( &statex );
	DebugPrint( ("\n"));

	// Test : using IOCTL Free Memory Dynamically from kerenl to service 
	for (i = 0; i < 3; i++)
	{
		result = MM_DeviceIOControl( NULL, 
									IOCTL_MM_FREE_RAW_MEM, // IOCTL_GET_MM_DATABASE_SIZE,
									NULL, 0,
									NULL, 0,
									&returnBytes);
		if (result != NO_ERROR)
		{
			DebugPrint( ("main : i %d, MM_DeviceIOControl(%s) IOCTL_MM_FREE_RAW_MEM Failed with error %ld (0x%X) \n",
								i, FTD_CTLDEV, result, result));
			break;
		}
	}
	Sleep(60000);	// in miiliseconds
	goto stop;
#endif
stop:
*/
	// now press 'X' or 'x' Terminate Programs ::
	printf("\n");
	printf("Press 'X' or 'x' key to Terminate MM Program..... :\n");
	ch = 0;
	while((ch != 'X') && (ch != 'x'))
	{
		//printf("Press 'X' or 'x' key to Terminate MM Program..... :\n");
		ch = _getch();
	}

	DebugPrint( ("\n\n before Stop :GMM_Info.TotalMemoryAllocated %I64d ( %d MB)!! \n\n",
				GMM_Info.TotalMemoryAllocated, (ULONG)(GMM_Info.TotalMemoryAllocated / ONEMILLION)));
	
	Test_GlobalMemoryStatusEx( &statex );
	DebugPrint( ("\n"));

	bret = MM_Stop();
	if (bret == FALSE)
	{ // Function Failed
		DebugPrint( ("main : MM_Stop() Failed with error %ld (0x%X) \n",GetLastError(), GetLastError()));
		goto done;
	}

	DebugPrint( ("\n\n After Stop, GMM_Info.TotalMemoryAllocated %I64d ( %d MB)!! \n\n",
				GMM_Info.TotalMemoryAllocated, (ULONG)(GMM_Info.TotalMemoryAllocated / ONEMILLION)));
	
	Test_GlobalMemoryStatusEx( &statex );
	DebugPrint( ("\n"));
#if 0
	// Test_GlobalMemoryStatus( &state );
	DebugPrint( ("main : ----- Testing AWE/Virtual mem/free APIS to Allocate Maximum Available Physical/Virtual Memory %d percentage \n", 
							PERCENTAGE_AVAIL_MEMORY));
	bret = Test_AWE();
	if (bret == FALSE)
	{ // Function Failed
		DebugPrint( ("main : Test_AWE : Failed with error %ld (0x%X) \n",
					GetLastError(), GetLastError()));
	}
#endif

done:
	DebugPrint( ("\n\n main : ----- MM Is Terminating... Press Any Key To Terminate Program .......\n", 
							PERCENTAGE_AVAIL_MEMORY));
	_getch();
	return;
} // main()


BOOLEAN 
Test_AWE()
{
	BOOLEAN			bret;					// generic Boolean value
	ULONG			MaxNumberOfPages;			// number of pages to request
	ULONG			numberOfPagesAllocated;		// initial number of pages requested
	PULONG			aPFNs = NULL;				// page info; holds opaque data
	SYSTEM_INFO		sSysInfo;					// useful system information
	MEMORYSTATUSEX	statex;
	ULONG			availPhysMemInMB, totalPhysMemInMB;	
	BOOLEAN			threeGB;	
	ULONG			cacheSizeInMB, saveCacheSizeInMB, percentage;
	HANDLE			processHandle = GetCurrentProcess();

	MaxNumberOfPages = numberOfPagesAllocated = 0;

	// GProcessHandle = GetCurrentProcess();

	Test_GetSystemInfo(&sSysInfo);     // populate the system information structure
	
	bret = Test_GlobalMemoryStatusEx( &statex );
	if (bret == FALSE)
		{ // Function Failed
		DebugPrint( ("Test_GlobalMemoryStatusEx : GlobalMemoryStatusEX() Failed with error %ld (0x%X) \n",
					GetLastError(), GetLastError()));
		GetErrorText();
		goto done;
		}

	// successed GlobalMemoryStatusEx()
	availPhysMemInMB = (ULONG) (statex.ullAvailPhys / (DWORDLONG) ONEMILLION);
	totalPhysMemInMB = (ULONG) (statex.ullTotalPhys / (DWORDLONG) ONEMILLION);

	// Determine if we're running on a 3GB user virtual address space system
	threeGB = (statex.ullTotalVirtual >= (DWORDLONG) 0x80000000) ? TRUE : FALSE;

	// now convert the percentage into Actual megabytes	cache value
	cacheSizeInMB = (availPhysMemInMB * PERCENTAGE_AVAIL_MEMORY)/100; // - ReservedFreeMemInMB;

	saveCacheSizeInMB = cacheSizeInMB;

	if( cacheSizeInMB > availPhysMemInMB) 
		cacheSizeInMB = availPhysMemInMB - RESERVE_MEMORY_INMB_KEEP_FREE;	// quietly bump cachesize down

	// Calculate the Total number of pages of memory to request.
	MaxNumberOfPages = (ULONG) (	((DWORDLONG) cacheSizeInMB * (DWORDLONG) ONEMILLION) / 
									((DWORDLONG) sSysInfo.dwPageSize) );
	// Calculate the size of the user PFN array, and allocate size Array OF Physical pages holder.
	// PFNArraySize = NumberOfPages * sizeof (ULONG);

	DebugPrint(("\n Allocating Physical Pages Info : \n"));
	DebugPrint(("\t totalPhysMemInMB          : %12d (0x%08x)\n", totalPhysMemInMB, totalPhysMemInMB));
	DebugPrint(("\t availPhysMemInMB          : %12d (0x%08x)\n", availPhysMemInMB, availPhysMemInMB));
	DebugPrint(("\t threeGB                   : %12d (0x%08x)\n", threeGB, threeGB));
	DebugPrint(("\t AllocatingSizeInMB        : %12d (0x%08x)\n", cacheSizeInMB, cacheSizeInMB));
	DebugPrint(("\t MaxNumberOfPages          : %12d (0x%08x)\n", MaxNumberOfPages, MaxNumberOfPages));
	DebugPrint(("\t sSysInfo.dwPageSize       : %12d (0x%08x)\n\n", sSysInfo.dwPageSize, sSysInfo.dwPageSize));

	
	percentage = ((MaxNumberOfPages * sSysInfo.dwPageSize) / ONEMILLION);
	percentage = ( (percentage * 100) / availPhysMemInMB);
	DebugPrint( ("Asking Allocated Pages : %12d (%12d MB Memory) (percentage of Available Phy Mem %3d ) \n", 
								MaxNumberOfPages, ((MaxNumberOfPages * sSysInfo.dwPageSize) / ONEMILLION),
								percentage) );

	// Allocate the physical memory.
	// First try to allocate AWE pages
	numberOfPagesAllocated = MaxNumberOfPages;
	
	DebugPrint( ("\n\n*********** Using AWE to Allocate Physical Pages Directly **************\n")); 
	bret = Allocate_AWE_Memory( processHandle,
								&numberOfPagesAllocated, 
								TRUE,
								&aPFNs );
	if (bret == FALSE)
	{ // failed
		DebugPrint( ("Test_AWE: Allocate_AWE_Memory() Failed \n"));
		goto done;
	}

	percentage = ((numberOfPagesAllocated * sSysInfo.dwPageSize) / ONEMILLION);
	percentage = ( (percentage * 100) / availPhysMemInMB);
	DebugPrint( ("Actual Allocated Pages : %12d (%12d MB Memory) (percentage of Available Phy Mem %3d \n", 
								numberOfPagesAllocated, ((numberOfPagesAllocated * sSysInfo.dwPageSize) / ONEMILLION),
								percentage));

	DebugPrint( ("\n Didpalying Memory status AFTER Physical Pages allocation:\n")); 	
	Test_GlobalMemoryStatusEx( &statex );
	DebugPrint( ("\n Freeing All Memory \n")); 	
	bret = Free_AWE_Memory(	processHandle,
							&numberOfPagesAllocated,	
							NUM_OF_PAGES_TO_FREE_AT_ATIME,	
							TRUE,		
							&aPFNs );
	if (bret == FALSE)
	{ // failed
		DebugPrint( ("Test_AWE: Free_AWE_Memory() Failed \n"));
		goto done;
	}

	DebugPrint( ("\n\n*********** Test Is Completed **************\n")); 
	
	// Reserve the virtual memory.
done:
	if (aPFNs)
		VirtualFree(aPFNs, 0, MEM_RELEASE);

  return TRUE;
} // Test_AWE()

BOOLEAN
Test_VirtualMemory_Alloc(ULONG MaxNumberOfPages)
{
	BOOLEAN			bret = FALSE;					// generic Boolean value
	PULONG			aPFNs = NULL;				// page info; holds opaque data
	ULONG			pageSize,percentage, availPhysMemInMB;
	ULONG			numberOfPagesAllocated;		// initial number of pages requested
	MEMORYSTATUSEX	statex;				// useful system information

	bret = Test_GlobalMemoryStatusEx( &statex );
	if (bret == FALSE)
		{ // Function Failed
		DebugPrint( ("Test_VirtualMemory_Alloc : GlobalMemoryStatusEX() Failed with error %ld (0x%X) \n",
					GetLastError(), GetLastError()));
		GetErrorText();
		goto done;
		}

	availPhysMemInMB = (ULONG) (statex.ullAvailPhys / (DWORDLONG) ONEMILLION);
	
	DebugPrint( ("\n\n*********** Using Virtual Memory to Allocate Physical Pages Directly **************\n")); 

	for (pageSize = (64 * ONE_K); pageSize <= (64 * ONE_K); pageSize *= 2)
	{
		// Allocate the physical memory.
		// First try to allocate AWE pages
		numberOfPagesAllocated = (MaxNumberOfPages * 4096)/pageSize;

		// Test_GlobalMemoryStatusEx( &statex );
		bret = Allocate_Virtual_Memory_Pages(	&numberOfPagesAllocated, 
												pageSize,
												TRUE,
												&aPFNs );
		if (bret == FALSE)
		{ // failed
			DebugPrint( ("Test_VirtualMemory_Alloc: Allocate_Virtual_Memory_Pages() Failed \n"));
			goto done;
		}

		percentage = ((numberOfPagesAllocated * pageSize) / ONEMILLION);
		percentage = ( (percentage * 100) / availPhysMemInMB);
		DebugPrint( ("pageSize: %12d,  Allocated Pages: %12d (%12d MB Memory) (percentage %3d of Available Phy Mem)\n", 
									pageSize, numberOfPagesAllocated, ((numberOfPagesAllocated * pageSize) / ONEMILLION),
									percentage));

#if 1
		// DebugPrint( ("\n Didpalying Memory status AFTER Physical Pages allocation:\n")); 	
		// Test_GlobalMemoryStatusEx( &statex );
		// DebugPrint( ("\n Freeing All Memory \n")); 	
		bret = Free_Virtual_Memory_Pages(	numberOfPagesAllocated,	
											TRUE,		
											&aPFNs );
		if (bret == FALSE)
		{ // failed
			DebugPrint( ("Test_VirtualMemory_Alloc: Free_AWE_Memory() Failed \n"));
			goto done;
		}
#else
		{
			ULONG	i, j;
			PVOID	pBuff;

			for (i=0; i < numberOfPagesAllocated; i++)
			{ // for : Do Pages at a time
				// DebugPrint( ("Index : %03d,  Physical Page 0x%08x \n", freePages, aPFNs[i]));
				if ( (ULONG *) aPFNs[i] == NULL)
					continue;

				for(j=0, pBuff = (PVOID) aPFNs[i];
					j < pageSize; 
					j += PAGE_SIZE, pBuff = (PVOID) ((ULONG) pBuff + (ULONG) PAGE_SIZE))
				{
					// bret = VirtualFree( (PVOID) pBuff , PAGE_SIZE, MEM_DECOMMIT);
					bret = VirtualFree( (PVOID) pBuff , PAGE_SIZE, MEM_FREE);
					if( bret != TRUE ) 
					{ // Failed
						DebugPrint((" Free_Virtual_Memory_Pages:: Failed: VirtualFree(Array Index %d : Memory 0x%08x) GetLastError - %d (0x%x) \n", 
																					i, aPFNs[i], GetLastError(),GetLastError() ));
						GetErrorText();
					}
					RtlZeroMemory(pBuff, PAGE_SIZE);

				}

				bret = VirtualFree( (PVOID) aPFNs[i], 0, MEM_RELEASE);
				if( bret != TRUE ) 
				{ // Failed
					DebugPrint((" Free_Virtual_Memory_Pages:: Failed: VirtualFree(Array Index %d : Memory 0x%08x) GetLastError - %d (0x%x) \n", 
																					i, aPFNs[i], GetLastError(),GetLastError() ));
					GetErrorText();
				}
				aPFNs[i] = (ULONG) NULL;
			} // for : Do Pages at a time	 
			if (aPFNs)
				VirtualFree( (PVOID) aPFNs , 0, MEM_RELEASE);
			aPFNs = NULL;
		}
#endif
	}

done:
	if (aPFNs)
		VirtualFree(aPFNs, 0, MEM_RELEASE);

	return TRUE;
} // Test_VirtualMemory_Alloc()


BOOLEAN 
Test_GlobalMemoryStatusEx(MEMORYSTATUSEX *statex)
{
	BOOLEAN	bret;
	
	statex->dwLength = sizeof(MEMORYSTATUSEX);

	bret = GlobalMemoryStatusEx (statex);

	if (bret == FALSE)
		{ // Function Failed
		  DebugPrint( ("GlobalMemoryStatusEx () Failed with error %ld (0x%X) \n",
					GetLastError(), GetLastError()));
		  return FALSE;
		}

	DebugPrint( ("\nGlobalMemoryStatusEx()     :  \n"));
	DebugPrint( ("dwLength                    : %12d  \n",		statex->dwLength));
	DebugPrint( ("dwMemoryLoad                : %12d  \n",		statex->dwMemoryLoad));
	DebugPrint( ("ullTotalPhys                : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullTotalPhys, 
													(ULONG) (statex->ullTotalPhys / (ONE_K)),
													(ULONG) (statex->ullTotalPhys / (ONE_K * ONE_K))));
	DebugPrint( ("ullAvailPhys                : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullAvailPhys, 
													(ULONG) (statex->ullAvailPhys / (DWORDLONG) (ONE_K)),
													(ULONG) (statex->ullAvailPhys / (DWORDLONG) (ONE_K * ONE_K))));
	DebugPrint( ("ullTotalPageFile            : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullTotalPageFile, 
													(ULONG) (statex->ullTotalPageFile / (DWORDLONG) (ONE_K)),
													(ULONG) (statex->ullTotalPageFile / (DWORDLONG) (ONE_K * ONE_K))));
	DebugPrint( ("ullAvailPageFile            : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullAvailPageFile, 
													(ULONG) (statex->ullAvailPageFile / (DWORDLONG) (ONE_K)),
													(ULONG) (statex->ullAvailPageFile / (DWORDLONG) (ONE_K * ONE_K))));
	DebugPrint( ("ullTotalVirtual             : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullTotalVirtual, 
													(ULONG) (statex->ullTotalVirtual / (DWORDLONG) (ONE_K)),
													(ULONG) (statex->ullTotalVirtual / (DWORDLONG) (ONE_K * ONE_K))));
	DebugPrint( ("ullAvailVirtual             : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullAvailVirtual, 
													(ULONG) (statex->ullAvailVirtual / (DWORDLONG) (ONE_K)),
													(ULONG) (statex->ullAvailVirtual / (DWORDLONG) (ONE_K * ONE_K))));
	DebugPrint( ("ullAvailExtendedVirtual     : %12I64d (%10d KB) (%10d MB) \n",
													statex->ullAvailExtendedVirtual, 
													(ULONG) (statex->ullAvailExtendedVirtual / (DWORDLONG) (ONE_K)),
													(ULONG) (statex->ullAvailExtendedVirtual / (DWORDLONG) (ONE_K * ONE_K))));
	return TRUE;
} // Test_GlobalMemoryStatusEx()

BOOLEAN 
Test_GetSystemInfo(SYSTEM_INFO *SysInfo)
{
	GetSystemInfo( SysInfo );
	DebugPrint(("\nGetSystemInfo()            :  \n"));

	DebugPrint(("dwPageSize                  : %12d  (0x%08x) \n", SysInfo->dwPageSize, SysInfo->dwPageSize));
	DebugPrint(("lpMinimumApplicationAddress : %12d  (0x%08x) \n", SysInfo->lpMinimumApplicationAddress, SysInfo->lpMinimumApplicationAddress));
	DebugPrint(("lpMaximumApplicationAddress : %12d  (0x%08x) \n", SysInfo->lpMaximumApplicationAddress, SysInfo->lpMaximumApplicationAddress));
	DebugPrint(("dwActiveProcessorMask       : %12d  (0x%08x) \n", SysInfo->dwActiveProcessorMask, SysInfo->dwActiveProcessorMask));
	DebugPrint(("dwNumberOfProcessors        : %12d  (0x%08x) \n", SysInfo->dwNumberOfProcessors, SysInfo->dwNumberOfProcessors));
	DebugPrint(("dwProcessorType             : %12d  (0x%08x) \n", SysInfo->dwProcessorType, SysInfo->dwProcessorType));
	DebugPrint(("dwAllocationGranularity     : %12d  (0x%08x) \n", SysInfo->dwAllocationGranularity, SysInfo->dwAllocationGranularity));
	DebugPrint(("wProcessorLevel             : %12d  (0x%08x) \n", SysInfo->wProcessorLevel, SysInfo->wProcessorLevel));
	DebugPrint(("wProcessorRevision          : %12d  (0x%08x) \n", SysInfo->wProcessorRevision, SysInfo->wProcessorRevision));
	DebugPrint(("dwOemId                     : %12d  (0x%08x) \n", SysInfo->dwOemId, SysInfo->dwOemId));

	return TRUE;
} // Test_GetSystemInfo()


BOOLEAN 
Test_GlobalMemoryStatus(MEMORYSTATUS *state)
{
	state->dwLength = sizeof(MEMORYSTATUS);

	GlobalMemoryStatus( state);

	DebugPrint( ("The MemoryStatus structure is %ld bytes long.\n",
			state->dwLength));
	DebugPrint( ("It should be %d.\n", sizeof (MEMORYSTATUS)));

	DebugPrint( ("%ld percent of memory is in use.\n",
			state->dwMemoryLoad));

	DebugPrint( ("There are %*ld total %sbytes of physical memory.\n",
			7, state->dwTotalPhys/ONE_K, "K"));
	DebugPrint( ("There are %*ld free %sbytes of physical memory.\n",
			7, state->dwAvailPhys/ONE_K, "K"));

	DebugPrint( ("There are %*ld total %sbytes of paging file.\n",
			7, state->dwTotalPageFile/ONE_K, "K"));
	DebugPrint( ("There are %*ld free %sbytes of paging file.\n",
			7, state->dwAvailPageFile/ONE_K, "K"));

	DebugPrint( ("There are %*ld (0x%*lx) total %sbytes of virtual memory.\n",
			7, state->dwTotalVirtual/ONE_K, 
			7, state->dwTotalVirtual/ONE_K, 
			"K"));
	DebugPrint( ("There are %*ld (0x%*lx) free %sbytes of virtual memory.\n",
			7, state->dwAvailVirtual/ONE_K, 
			7, state->dwAvailVirtual/ONE_K, 
			"K"));
  
	// Show the amount of extended memory available.
	return TRUE;

} // Test_GlobalMemoryStatus

// internal function used to convert SDK API GetLastError() into string format and returns error text string to caller
VOID	
DisplayErrorText()
{
    HMODULE hModule = NULL; // default to system source
    LPSTR MessageBuffer;
    DWORD dwBufferLength;
	DWORD dwLastError	= GetLastError();
	DWORD dwFormatFlags =	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_IGNORE_INSERTS |
							FORMAT_MESSAGE_FROM_SYSTEM ;

	// display error details
	printf("\tError Detailed Information: \n");	
	printf("\tGetLastError() = %d (0x%08x) \n", dwLastError, dwLastError);	

    
    // If dwLastError is in the network range, 
    //  load the message source.

    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx(
            TEXT("netmsg.dll"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system 
    //  or from the supplied module handle.

    if(dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        ))
    {
        // Output message string on stderr.
		printf("\tErrorString    = %s \n",MessageBuffer);	

        // Free the buffer allocated by the system.
        LocalFree(MessageBuffer);
    } 
	else
	{
		printf("\tErrorString    = Unknown Error \n");	
	}

    // If we loaded a message source, unload it.
    if(hModule != NULL)
        FreeLibrary(hModule);

} // DisplayErrorText()


//
// Globale Variable Definations
//

BOOLEAN 
Init_AWE()
{
	BOOLEAN			bret;					// generic Boolean value
	ULONG			MaxNumberOfPages;			// number of pages to request
	ULONG			numberOfPagesAllocated;		// initial number of pages requested
	PULONG			aPFNs = NULL;				// page info; holds opaque data
	SYSTEM_INFO		sSysInfo;					// useful system information
	MEMORYSTATUSEX	statex;
	ULONG			availPhysMemInMB, totalPhysMemInMB;	
	BOOLEAN			threeGB;	
	ULONG			cacheSizeInMB, saveCacheSizeInMB, percentage;
	HANDLE			processHandle		= INVALID_HANDLE_VALUE;

	MaxNumberOfPages = numberOfPagesAllocated = 0;

	processHandle = GetCurrentProcess();

	Test_GetSystemInfo(&sSysInfo);     // populate the system information structure
	
	bret = Test_GlobalMemoryStatusEx( &statex );
	if (bret == FALSE)
		{ // Function Failed
		DebugPrint( ("Test_GlobalMemoryStatusEx : GlobalMemoryStatusEX() Failed with error %ld (0x%X) \n",
					GetLastError(), GetLastError()));
		GetErrorText();
		goto done;
		}

	// successed GlobalMemoryStatusEx()
	availPhysMemInMB = (ULONG) (statex.ullAvailPhys / (DWORDLONG) ONEMILLION);
	totalPhysMemInMB = (ULONG) (statex.ullTotalPhys / (DWORDLONG) ONEMILLION);

	// Determine if we're running on a 3GB user virtual address space system
	threeGB = (statex.ullTotalVirtual >= (DWORDLONG) 0x80000000) ? TRUE : FALSE;

	// now convert the percentage into Actual megabytes	cache value
	cacheSizeInMB = (availPhysMemInMB * PERCENTAGE_AVAIL_MEMORY)/100; // - ReservedFreeMemInMB;

	saveCacheSizeInMB = cacheSizeInMB;

	if( cacheSizeInMB > availPhysMemInMB) 
		cacheSizeInMB = availPhysMemInMB - RESERVE_MEMORY_INMB_KEEP_FREE;	// quietly bump cachesize down

	// Calculate the Total number of pages of memory to request.
	MaxNumberOfPages = (ULONG) (	((DWORDLONG) cacheSizeInMB * (DWORDLONG) ONEMILLION) / 
									((DWORDLONG) sSysInfo.dwPageSize) );
	// Calculate the size of the user PFN array, and allocate size Array OF Physical pages holder.
	// PFNArraySize = NumberOfPages * sizeof (ULONG);

	DebugPrint(("\n Allocating Physical Pages Info : \n"));
	DebugPrint(("\t totalPhysMemInMB          : %12d (0x%08x)\n", totalPhysMemInMB, totalPhysMemInMB));
	DebugPrint(("\t availPhysMemInMB          : %12d (0x%08x)\n", availPhysMemInMB, availPhysMemInMB));
	DebugPrint(("\t threeGB                   : %12d (0x%08x)\n", threeGB, threeGB));
	DebugPrint(("\t AllocatingSizeInMB        : %12d (0x%08x)\n", cacheSizeInMB, cacheSizeInMB));
	DebugPrint(("\t MaxNumberOfPages          : %12d (0x%08x)\n", MaxNumberOfPages, MaxNumberOfPages));
	DebugPrint(("\t sSysInfo.dwPageSize       : %12d (0x%08x)\n\n", sSysInfo.dwPageSize, sSysInfo.dwPageSize));

	
	percentage = ((MaxNumberOfPages * sSysInfo.dwPageSize) / ONEMILLION);
	percentage = ( (percentage * 100) / availPhysMemInMB);
	DebugPrint( ("Asking Allocated Pages : %12d (%12d MB Memory) (percentage of Available Phy Mem %3d ) \n", 
								MaxNumberOfPages, ((MaxNumberOfPages * sSysInfo.dwPageSize) / ONEMILLION),
								percentage) );

	// Allocate the physical memory.
	// First try to allocate AWE pages
	numberOfPagesAllocated = MaxNumberOfPages;
	
	DebugPrint( ("\n\n*********** Using AWE to Allocate Physical Pages Directly **************\n")); 
	bret = Allocate_AWE_Memory(	processHandle,
								&numberOfPagesAllocated, 
								TRUE,
								&aPFNs );
	if (bret == FALSE)
	{ // failed
		DebugPrint( ("Test_MM: Allocate_AWE_Memory() Failed \n"));
		goto done;
	}

	percentage = ((numberOfPagesAllocated * sSysInfo.dwPageSize) / ONEMILLION);
	percentage = ( (percentage * 100) / availPhysMemInMB);
	DebugPrint( ("Actual Allocated Pages : %12d (%12d MB Memory) (percentage of Available Phy Mem %3d \n", 
								numberOfPagesAllocated, ((numberOfPagesAllocated * sSysInfo.dwPageSize) / ONEMILLION),
								percentage));

	DebugPrint( ("\n Didpalying Memory status AFTER Physical Pages allocation:\n")); 	
	Test_GlobalMemoryStatusEx( &statex );
	DebugPrint( ("\n Freeing All Memory \n")); 	
	bret = Free_AWE_Memory(	processHandle,
							&numberOfPagesAllocated,	
							NUM_OF_PAGES_TO_FREE_AT_ATIME,	
							TRUE,		
							&aPFNs );
	if (bret == FALSE)
	{ // failed
		DebugPrint( ("Test_MM: Free_AWE_Memory() Failed \n"));
		goto done;
	}

	DebugPrint( ("\n\n*********** Test Is Completed **************\n")); 
	
	// Reserve the virtual memory.
done:
	if (aPFNs)
		VirtualFree(aPFNs, 0, MEM_RELEASE);

  return TRUE;
} // Init_AWE()



