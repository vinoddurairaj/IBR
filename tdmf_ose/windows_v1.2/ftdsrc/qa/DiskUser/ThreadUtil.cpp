#include <stdafx.h>
#include <process.h>
#include "ThreadUtil.h"

static ThreadStruc      ThreadList[MAX_THREADS];

//
// Interlocked variables
//
static volatile long    iNumThreadsExecuting    = 0; 
#if _DEBUG
static volatile long    iNumReadsSuccessful     = 0;
static volatile long    iNumReadsUnsuccessful   = 0;
static volatile long    iNumWritesSuccessful    = 0;
static volatile long    iNumWritesUnsuccessful  = 0;
#endif

static unsigned int     uiRandGen;
static char             TempBuffer[] =  "the quick brown fox jumps over the lazy dogs back\r\n" \
                                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n" \
                                        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\r\n" \
                                        "THE quick brown FOX JUMPS OVER THE LAZY DOGS BACK\r\n" \
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\r\n" \
                                        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD\r\n" \
                                        "No one knows where they went, no one cares!\r\n" \
                                        "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\r\n" \
                                        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\r\n" \
                                        "What is the question to the universe and everything?\r\n" \
                                        "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG\r\n" \
                                        "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH\r\n" \
                                        "Why did the chicken cross the road?\r\n" \
                                        "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\r\n" \
                                        "JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ\r\n" \
                                        "To get to the other side!!\r\n" \
                                        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK\r\n" \
                                        "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n" \
                                        "Who is the most big of the bigest mosts?\r\n" \
                                        "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\r\n" \
                                        "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN\r\n" \
                                        "The bigest most of course! Who did you think it was?\r\n" \
                                        "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\r\n" \
                                        "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP\r\n" \
                                        "I thought I saw a puzzy cat... But it was a bear!\r\n" \
                                        "QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ\r\n" \
                                        "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\r\n" \
                                        "AAAAAAAAAAAaa Not another meeting I hope!\r\n" \
                                        "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\r\n" \
                                        "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT\r\n" \
                                        "There is no place like home, there is no place like it\r\n" \
                                        "UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU\r\n" \
                                        "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\r\n" \
                                        "This text is not copyrighted. Please copy it as much\r\n" \
                                        "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW\r\n" \
                                        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n" \
                                        "as you want! P.s. All your base are belong to us!\r\n" \
                                        "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\r\n" \
                                        "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ\r\n";

#define BUFFER_SIZE 20480 // multiples of 2048
static char             WriteBuffer[BUFFER_SIZE];
static char             ReadBuffer[BUFFER_SIZE];

unsigned int            gBytesPerThread    = 0;
unsigned int            guiReadWriteRatio  = 0;
unsigned int            guiSleepMultiplier = 0;

void TU_InitThreadList(unsigned int uiReadWriteRatio, unsigned int uiSleepMultiplier)
{
    iNumThreadsExecuting    = 0;
    guiReadWriteRatio       = uiReadWriteRatio;
    guiSleepMultiplier      = uiSleepMultiplier;
    uiRandGen               = (unsigned)time( NULL );

    ZeroMemory(ThreadList,sizeof(ThreadList));
    for (int i = 0; i < sizeof(WriteBuffer);i+=sizeof(TempBuffer))
    {
        if ((i + sizeof(TempBuffer)) < sizeof(WriteBuffer))
        {
            CopyMemory(WriteBuffer+i,TempBuffer,sizeof(TempBuffer));
        }
        else
        {
            CopyMemory(WriteBuffer+i,TempBuffer,(sizeof(WriteBuffer) - i));
        }
    }
}

bool TU_StartDiskThread(unsigned int uiThreadID, unsigned int KbToUse, char cDiskName)
{
    gBytesPerThread = KbToUse;

    if (uiThreadID > MAX_THREADS)
    { 
        return false;
    }

    if (ThreadList[uiThreadID].ThreadStatus & THREAD_STARTED)
    {
        return true;
    }

    //
    // Start the thread
    //
    ThreadList[uiThreadID].ThreadHandle = (HANDLE)_beginthreadex(NULL,0,DiskUserThread,&ThreadList[uiThreadID],0,0);
    ThreadList[uiThreadID].ThreadID     = uiThreadID;
    ThreadList[uiThreadID].cDiskName    = cDiskName;

    if (!ThreadList[uiThreadID].ThreadHandle)
    {
        //
        // problem creating the thread!
        //
        OutputDebugString("OOOOOOPPPSS!\n");
    }
    else
    {
        ThreadList[uiThreadID].bAlive       = true;
        InterlockedIncrement(&iNumThreadsExecuting); 
    }
 
    return true;
}

void TU_StopDiskThreads(void)
{
    for (int i = 0; i< MAX_THREADS;i++)
    {
        ThreadList[i].bAlive = FALSE;
    }
}

int TU_GetNumThreadsExecuting(void)
{
    return iNumThreadsExecuting;
}

#if _DEBUG
int TU_GetNumReadsOK(void)
{
    return iNumReadsSuccessful;
}

int TU_GetNumReadsNOK(void)
{
    return iNumReadsUnsuccessful;
}

int TU_GetNumWritesOK(void)
{
    return iNumWritesSuccessful;
}

int TU_GetNumWritesNOK(void)
{
    return iNumWritesUnsuccessful;
}
#endif



// ***********************************************************
// Function name	: DiskUserThread 
// Description	    : The sole purpose of this thread is to 
//                    access the disk
// 
// ***********************************************************
static unsigned int __stdcall DiskUserThread(void* pContext)
{
    ThreadStruc *   CurrentThread = (ThreadStruc *)pContext;
    unsigned int    SleepValue;
    HANDLE          hFileHandle = 0;
    char            DbgMsg[256];

    //
    // Generate random sleep time (in ms)
    //
    srand(uiRandGen);
    SleepValue = 10 * guiSleepMultiplier + int(90.0 * guiSleepMultiplier * rand()/(RAND_MAX+1.0));
    uiRandGen+=SleepValue;

    sprintf(DbgMsg,"Thread %ld will sleep for %ld\n",CurrentThread->ThreadID, SleepValue);
    OutputDebugString(DbgMsg);

    hFileHandle = TU_OpenFile(CurrentThread->ThreadID,CurrentThread->cDiskName);

    while( CurrentThread->bAlive )
    {
        Sleep(SleepValue);
    
        if (hFileHandle)
        {
            TU_ReadWriteToFile(hFileHandle);
        }


    }//forever

    if (hFileHandle)
    {
        TU_CloseFile(hFileHandle);
    }

    sprintf(DbgMsg,"Thread %ld Died!\n",CurrentThread->ThreadID);
    OutputDebugString(DbgMsg);

    InterlockedDecrement(&iNumThreadsExecuting); 

    return 0;
}


//
// Try to read the disk extents using GetDiskFreeSpaceEx
//  
//
// If we have an error, return 0
//
//

void TU_GetDiskFreeSpaceEx( CString csDriveName, ULARGE_INTEGER * ReturnValue)
{
    ULARGE_INTEGER          liBytesAvailable;
    ULARGE_INTEGER          liTotalBytes;
    ULARGE_INTEGER          liTotalFreeBytes;
    ULARGE_INTEGER          liReturnValue;

    liReturnValue.QuadPart = 0;

    if ( GetDiskFreeSpaceEx(    csDriveName,            // directory name
                                &liBytesAvailable,      // bytes available to caller
                                &liTotalBytes,          // bytes on disk
                                &liTotalFreeBytes    ) )// free bytes on disk
    {
        
    }
    else
    {
        liTotalFreeBytes.QuadPart = 0;
    }

    ReturnValue->QuadPart = liTotalFreeBytes.QuadPart;
}


//
// Opens a file with name "File_FileID" (Where FileID = FileID)
// for read/write operations
//
HANDLE TU_OpenFile(unsigned int FileID,char DiskName)
{
    HANDLE  hFileHandle;
    char    cFileName[250];
#if _DEBUG
    char    DbgMsg[256];
#endif

    //
    //
    // TODO:
    //
    // Create directory if it does not exist (And ask user for confirmation)
    //
    // Delete all files in TestHere Directory (And ask user for confirmation)
    //

    sprintf(cFileName,"\\\\.\\%c:\\TestHere\\File_%ld.txt",DiskName,FileID);

#if _DEBUG
    sprintf(DbgMsg,"Opened file %s for read/write operations\n",cFileName);
    OutputDebugString(DbgMsg);
#endif

    hFileHandle = CreateFile(   cFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    return hFileHandle;

}

void TU_CloseFile(HANDLE HFile)
{
    CloseHandle(HFile);
}

//
// Reads/Writes in random areas of the disk
// on a file handle
//
void TU_ReadWriteToFile(HANDLE HFile)
{
    unsigned int    RandomValueSeek         = 0;

    DWORD           dwNumberOfBytesToDo;
	DWORD           dwNumberOfBytesDone     = 0; 
    DWORD           ret                     = 0;

    RandomValueSeek = (1 + int((1.0 * gBytesPerThread) * rand()/(RAND_MAX+1.0)));

    if (RandomValueSeek > (gBytesPerThread - sizeof(WriteBuffer)))
    {
        RandomValueSeek = gBytesPerThread - sizeof(WriteBuffer);
    }

    //
    // Make sure we are sector aligned! (let's assume 2048)
    //
    RandomValueSeek -= (RandomValueSeek%2048);

    //
    // Seek to somewhere in our file (randomly) 
    // (make sure we don't go past our buffer)
    //
    SetFilePointer(HFile,RandomValueSeek,NULL,FILE_BEGIN);

    //
    // Now read or write from the file from this location
    //
    if ( (1 + int(100.0 * rand()/(RAND_MAX+1.0))) > (int)guiReadWriteRatio )
    {
        dwNumberOfBytesToDo = (1 + int((1.0 * sizeof(WriteBuffer)) * rand()/(RAND_MAX+1.0) ));

        //
        // Our write buffer has to be at least 2048bytes!
        //
        ASSERT(sizeof(WriteBuffer)>2048);

        //
        // Sector align ourself (to do unbuffered read/writes)
        //
        if (dwNumberOfBytesToDo > 2048)
        {
            dwNumberOfBytesToDo -= (dwNumberOfBytesToDo%2048);
        }
        else
        {
            dwNumberOfBytesToDo += (2048 - (dwNumberOfBytesToDo%2048));
        }

        ret = WriteFile( HFile, 
                        (void *)WriteBuffer, 
                        dwNumberOfBytesToDo, // number of bytes to write! 
                        &dwNumberOfBytesDone, // pointer to number of bytes written
                        NULL );
#if _DEBUG
        if (ret)
        {
            InterlockedIncrement(&iNumWritesSuccessful);
        }
        else
        {
            InterlockedIncrement(&iNumWritesUnsuccessful);
        }
#endif
    }
    else
    {
        dwNumberOfBytesToDo = (1 + int((1.0 * sizeof(ReadBuffer)) * rand()/(RAND_MAX+1.0) ));

        //
        // Sector align ourself (to do unbuffered read/writes)
        //
        if (dwNumberOfBytesToDo > 2048)
        {
            dwNumberOfBytesToDo -= (dwNumberOfBytesToDo%2048);
        }
        else
        {
            dwNumberOfBytesToDo += (2048 - (dwNumberOfBytesToDo%2048));
        }


        ret = ReadFile( HFile,
                        (void *)ReadBuffer,
                        dwNumberOfBytesToDo,
                        &dwNumberOfBytesDone,
                        NULL );

#if _DEBUG
        if (ret)
        {
            InterlockedIncrement(&iNumReadsSuccessful);
        }
        else
        {
            InterlockedIncrement(&iNumReadsUnsuccessful);
        }
#endif

    }
        
}

