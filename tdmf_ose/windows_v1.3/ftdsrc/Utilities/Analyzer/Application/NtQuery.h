#if !defined(__NTQUERY_H__)
#define __NTQUERY_H__

#include <windows.h>

#pragma warning( disable : 4005)

namespace NTDDK {
#include <ntddk.h>
};


// =================================================================
// SYSTEM INFO CLASSES
// =================================================================

typedef enum _SYSTEMINFOCLASS
{
    SystemBasicInformation,             // 0x002C
    SystemProcessorInformation,         // 0x000C
    SystemPerformanceInformation,       // 0x0138
    SystemTimeInformation,              // 0x0020
    SystemPathInformation,              // not implemented
    SystemProcessInformation,           // 0x00C8+ per process
    SystemCallInformation,              // 0x0018 + (n * 0x0004)
    SystemConfigurationInformation,     // 0x0018
    SystemProcessorCounters,            // 0x0030 per cpu
    SystemGlobalFlag,                   // 0x0004
    SystemInfo10,                       // not implemented
    SystemModuleInformation,            // 0x0004 + (n * 0x011C)
    SystemLockInformation,              // 0x0004 + (n * 0x0024)
    SystemInfo13,                       // not implemented
    SystemPagedPoolInformation,         // checked build only
    SystemNonPagedPoolInformation,      // checked build only
    SystemHandleInformation,            // 0x0004  + (n * 0x0010)
    SystemObjectInformation,            // 0x0038+ + (n * 0x0030+)
    SystemPageFileInformation,          // 0x0018+ per page file
    SystemInstemulInformation,          // 0x0088
    SystemInfo20,                       // invalid info class
    SystemCacheInformation,             // 0x0024
    SystemPoolTagInformation,           // 0x0004 + (n * 0x001C)
    SystemInfo23,                       // 0x0000, or 0x0018 per cpu
    SystemDpcInformation,               // 0x0014
    SystemInfo25,                       // checked build only
    SystemLoadDriver,                   // 0x0018, set mode only
    SystemUnloadDriver,                 // 0x0004, set mode only
    SystemTimeAdjustmentInformation,    // 0x000C, 0x0008 writeable
    SystemInfo29,                       // checked build only
    SystemInfo30,                       // checked build only
    SystemInfo31,                       // checked build only
    SystemCrashDumpInformation,         // 0x0004
    SystemInfo33,                       // 0x0010
    SystemCrashDumpStateInformation,    // 0x0004
    SystemDebuggerInformation,          // 0x0002
    SystemThreadSwitchInformation,      // 0x0030
    SystemRegistryQuotaInformation,     // 0x000C
    SystemAddDriver,                    // 0x0008, set mode only
    SystemPrioritySeparationInformation,// 0x0004, set mode only
    SystemInfo40,                       // not implemented
    SystemInfo41,                       // not implemented
    SystemInfo42,                       // invalid info class
    SystemInfo43,                       // invalid info class
    SystemTimeZoneInformation,          // 0x00AC
    SystemLookasideInformation,         // n * 0x0020
    MaxSystemInfoClass
} SYSTEMINFOCLASS, *PSYSTEMINFOCLASS, **PPSYSTEMINFOCLASS;

// =================================================================
// SYSTEM INFO STRUCTURES
// =================================================================

// -----------------------------------------------------------------
// 05: SystemProcessInformation
//     see ExpGetProcessInformation()
//     see also ExpCopyProcessInfo(), ExpCopyThreadInfo()

typedef struct _SYSTEM_THREAD
{
    FILETIME	 qKernelTime;       // 100 nsec units
    FILETIME	 qUserTime;         // 100 nsec units
    FILETIME	 qCreateTime;       // relative to 01-01-1601
    DWORD        d18;
    PVOID        pStartAddress;
	NTDDK::CLIENT_ID    Cid;               // process/thread ids
    DWORD        dPriority;
    DWORD        dBasePriority;
    DWORD        dContextSwitches;
    DWORD        dThreadState;      // 2=running, 5=waiting
	NTDDK::KWAIT_REASON WaitReason;
    DWORD        dReserved01;
} SYSTEM_THREAD, *PSYSTEM_THREAD, **PPSYSTEM_THREAD;

#define SYSTEM_THREAD_ sizeof(SYSTEM_THREAD)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    DWORD          dNext;           // relative offset
    DWORD          dThreadCount;
    DWORD          dReserved01;
    DWORD          dReserved02;
    DWORD          dReserved03;
    DWORD          dReserved04;
    DWORD          dReserved05;
    DWORD          dReserved06;
    FILETIME       qCreateTime;     // relative to 01-01-1601
    FILETIME       qUserTime;       // 100 nsec units
    FILETIME       qKernelTime;     // 100 nsec units
	NTDDK::UNICODE_STRING usName;
	NTDDK::KPRIORITY      BasePriority;
    DWORD          dUniqueProcessId;
    DWORD          dInheritedFromUniqueProcessId;
    DWORD          dHandleCount;
    DWORD          dReserved07;
    DWORD          dReserved08;
	NTDDK::VM_COUNTERS    VmCounters;
    DWORD          dCommitCharge;   // bytes
    SYSTEM_THREAD  ast [];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION, **PPSYSTEM_PROCESS_INFORMATION;

#define SYSTEM_PROCESS_INFORMATION_ sizeof(SYSTEM_PROCESS_INFORMATION)

// -----------------------------------------------------------------
// 16: SystemHandleInformation
//
typedef struct _SYSTEM_HANDLE
{
    DWORD       dIdProcess;
    BYTE		bObjectType;    // OB_TYPE_*
    BYTE		bFlags;         // bits 0..2 HANDLE_FLAG_*
    WORD		wValue;         // multiple of 4
	PVOID       pObject;
	NTDDK::ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE, **PPSYSTEM_HANDLE;

#define SYSTEM_HANDLE_ sizeof(SYSTEM_HANDLE)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    DWORD         dCount;
    SYSTEM_HANDLE ash [];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION, **PPSYSTEM_HANDLE_INFORMATION;

#define SYSTEM_HANDLE_INFORMATION_ sizeof (SYSTEM_HANDLE_INFORMATION)


// =================================================================
// OBJECT TYPE CODES
// =================================================================
#if defined(WINVER) & (WINVER >= 0x500)
#define OB_TYPE_TYPE                        0x001
#define OB_TYPE_DIRECTORY                   0x002
//#define OB_TYPE_SYMBOLIC_LINK						//  3
//#define OB_TYPE_TOKEN								//  4
//#define OB_TYPE_PROCESS							//  5
#define OB_TYPE_THREAD								//  6
#define OB_TYPE_EVENT                       0x008	//  7 = 0x007
#define OB_TYPE_EVENT_PAIR                  0x009	//  8
#define OB_TYPE_MUTANT						0x00A	//  9 = 0x009
//#define OB_TYPE_SEMAPHORE							// 10
//#define OB_TYPE_TIMER								// 11
//#define OB_TYPE_PROFILE							// 12
#define	OB_TYPE_SEMGUARD					0x00C	// <new>
// ...
#define OB_TYPE_WINDOW_STATION              0x00F	// 13 = 0x00D
#define OB_TYPE_DESKTOP                     0x010	// 14 = 0x00E
#define OB_TYPE_SECTION                     0x011	// 15 = 0x00F
#define OB_TYPE_KEY                         0x012	// 16 = 0x010
#define OB_TYPE_PORT                        0x013	// 17 = 0x011
//#define OB_TYPE_ADAPTER							// 18
//#define OB_TYPE_CONTROLLER                  		// 19
//#define OB_TYPE_DEVICE                      		// 20
//#define OB_TYPE_DRIVER                      		// 21
//#define OB_TYPE_IO_COMPLETION               		// 22
#define OB_TYPE_FILE                        0x01A	// 23 = 0x017
#else	// NT 4 and below...
#define OB_TYPE_TYPE                         1
#define OB_TYPE_DIRECTORY                    2
#define OB_TYPE_SYMBOLIC_LINK                3
#define OB_TYPE_TOKEN                        4
#define OB_TYPE_PROCESS                      5
#define OB_TYPE_THREAD                       6
#define OB_TYPE_EVENT                        7
#define OB_TYPE_EVENT_PAIR                   8
#define OB_TYPE_MUTANT                       9
#define OB_TYPE_SEMAPHORE                   10
#define OB_TYPE_TIMER                       11
#define OB_TYPE_PROFILE                     12
#define OB_TYPE_WINDOW_STATION              13
#define OB_TYPE_DESKTOP                     14
#define OB_TYPE_SECTION                     15
#define OB_TYPE_KEY                         16
#define OB_TYPE_PORT                        17
#define OB_TYPE_ADAPTER                     18
#define OB_TYPE_CONTROLLER                  19
#define OB_TYPE_DEVICE                      20
#define OB_TYPE_DRIVER                      21
#define OB_TYPE_IO_COMPLETION               22
#define OB_TYPE_FILE                        23
#endif

// =================================================================
// API PROTOTYPES
// =================================================================

//NTSTATUS NTAPI
extern "C" DWORD __stdcall 
NtQuerySystemInformation (SYSTEMINFOCLASS sic, 
                          PVOID           pData,
                          DWORD           dSize,
                          PDWORD          pdSize);

// =================================================================
// Some classes to handle different NtQuerySysteminfo items
// =================================================================

// Base class to handle NtQuerySystemInformation calls
class NtSystemInformation
{
public:
	NtSystemInformation(SYSTEMINFOCLASS sic);	
	virtual ~NtSystemInformation();
	
	virtual BOOL Refresh();

protected:
	BOOL	m_Refresh();
	SYSTEMINFOCLASS m_sic;
	PVOID	m_pData;
};

class NtSystemHandleInformation : public NtSystemInformation
{
public:
	NtSystemHandleInformation(); 
	virtual ~NtSystemHandleInformation(); 

	BOOL Refresh();
	DWORD Count();
	PSYSTEM_HANDLE Get(int idx);

protected:
};

class NtSystemProcessInformation : public NtSystemInformation
{
public:
	NtSystemProcessInformation(); 
	virtual ~NtSystemProcessInformation(); 

	BOOL Refresh();
	DWORD Count();
	PSYSTEM_PROCESS_INFORMATION Find(DWORD dwProcessId);
	char* GetProcessName(DWORD dwProcessId);

protected:
	DWORD	m_dwCount;
};

BOOL EnablePrivilege (HANDLE hprocess, LPSTR privilege, BOOL flag);

#endif