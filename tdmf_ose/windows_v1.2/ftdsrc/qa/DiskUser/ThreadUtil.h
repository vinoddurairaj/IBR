#define MAX_THREADS         1000
#define THREAD_STARTED      0x00000001
#define THREAD_ACTIVE       0x00000010

typedef struct __ThreadStruc
{
    unsigned int    ThreadStatus;
    HANDLE          ThreadHandle;
    unsigned int    ThreadID;
    unsigned int    KbDiskUse;
    bool            bAlive;
    char            cDiskName;
} ThreadStruc;

void TU_InitThreadList(unsigned int uiReadWriteRatio = 50, unsigned int uiSleepMultiplier = 10);
bool TU_StartDiskThread(unsigned int uiThreadID, unsigned int KbToUse = 50*1024, char cDiskName = 'C');
void TU_StopDiskThreads(void);
int  TU_GetNumThreadsExecuting(void);
static unsigned int __stdcall DiskUserThread(void* pContext);

void            TU_GetDiskFreeSpaceEx( CString csDriveName, ULARGE_INTEGER * ReturnValue);
HANDLE          TU_OpenFile(unsigned int FileID,char DiskName);
void            TU_CloseFile(HANDLE HFile);
void            TU_ReadWriteToFile(HANDLE HFile);

#if _DEBUG
int TU_GetNumReadsOK(void);
int TU_GetNumReadsNOK(void);
int TU_GetNumWritesOK(void);
int TU_GetNumWritesNOK(void);
#endif