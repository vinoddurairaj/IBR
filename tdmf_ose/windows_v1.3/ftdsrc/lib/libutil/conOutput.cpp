/*
 * ftd_mngt.c - ftd management message handlers
 *
 * Copyright (c) 2002 Fujitsu SoftTek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include <windows.h>
#include <stdio.h>
extern "C" 
{
#include "conOutput.h"
}

#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#define DBGPRINT(a)     printf a 
#else
#define ASSERT(exp)    ((void)0)
#define DBGPRINT(a)     ((void)0)
#endif

#define INT_MIN     0x80000000

///////////////////////////////////////////////////////////////////////////////
typedef struct __ConsoleOutputCtrl
{
    HANDLE          hOutputRead,    //provide this handle to child process
                    hOutputWrite,   //provide this handle to child process
                    hErrorWrite;    //provide this handle to child process
    char            *pData;
    unsigned int    iSize, 
                    iTotalDataRead;

} ConsoleOutputCtrl;

///////////////////////////////////////////////////////////////////////////////
/* local prototypes */
static bool consoleOutput_init   (ConsoleOutputCtrl *ctrl);
static bool consoleOutput_read   (ConsoleOutputCtrl *ctrl);
static void consoleOutput_delete (ConsoleOutputCtrl *ctrl);


///////////////////////////////////////////////////////////////////////////////
int consoleOutput_Launch( const char* szAppName, const char* szAppCmdLine, const char* szAppCurrentDir, char** ppszAppOutput, int *piAppExitCode )
{
    BOOL                b;
    DWORD               dwCreationFlags;
    STARTUPINFO         sInfo;
    PROCESS_INFORMATION pInfo;
    ConsoleOutputCtrl   conctrl;

    if ( ppszAppOutput == NULL )
        return -1;//bad param.
    if ( szAppName == NULL && szAppCmdLine == NULL )
        return -1;//bad param.

    *ppszAppOutput = NULL;
    *piAppExitCode = INT_MIN;

    consoleOutput_init(&conctrl);

    //launch process command 
    memset(&sInfo,0,sizeof(sInfo));
    sInfo.cb = sizeof(sInfo);
    dwCreationFlags  = CREATE_NO_WINDOW; // Windows NT/2000/XP: This flag is valid only when starting a console application. If set, the console application is run without a console window 
    sInfo.dwFlags    = STARTF_USESTDHANDLES;
    sInfo.hStdError  = conctrl.hErrorWrite;
    sInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    sInfo.hStdOutput = conctrl.hOutputWrite;

    DBGPRINT(("Launching <%s> , options = <%s>...\n",szAppName,szAppCmdLine));
    b =   CreateProcess(szAppName,            // name of executable module
                        (char*)szAppCmdLine,         // command line string
                        NULL,                 // SD
                        NULL,                 // SD
                        TRUE,                 // handle inheritance option
                        dwCreationFlags,      // creation flags
                        NULL,                 // new environment block
                        szAppCurrentDir,      // current directory name
                        &sInfo,               // startup information
                        &pInfo                // process information
                        );
    if ( b )
    {
        DBGPRINT(("  Waiting for cmd process to end ...\n"));

        //accumulate all console output into pData buffer
        consoleOutput_read(&conctrl);
#ifdef _DEBUG
        //printf("\nConsole output:>>>>\n%s",conctrl.pData);
#endif

        //wait until app process has completed and exits
        if ( WAIT_OBJECT_0 == WaitForSingleObject(pInfo.hProcess,INFINITE) )
        {
            //Get Process exit code
            if ( GetExitCodeProcess(pInfo.hProcess,(DWORD*)piAppExitCode) )
            {
#ifdef _DEBUG
                //printf("\n<<<<Process ended.  Return code = %d\n",*piAppExitCode);
#endif
                if ( *piAppExitCode == 0 )
                {   //success.  
                }
            }

            //fill output pointer 
            if ( conctrl.iTotalDataRead > 0 )
            {
                *ppszAppOutput = (char*)malloc( conctrl.iTotalDataRead );
                memcpy( *ppszAppOutput, conctrl.pData, conctrl.iTotalDataRead ); 
            }
        }
    }
    else
    {
        char bigbuf[1024];
        DWORD err = GetLastError();
        int len = _snprintf( bigbuf, sizeof(bigbuf),"consoleOutput_Launch() : Error (%lu , 0x%08x) launching app (%s %s).", err, err, szAppName, szAppCmdLine );
        //fill output pointer 
        *ppszAppOutput = (char*)malloc(len);
        memcpy(*ppszAppOutput,bigbuf,len); 
        *piAppExitCode = INT_MIN;
    }

    CloseHandle(pInfo.hProcess);
    CloseHandle(pInfo.hThread);

    consoleOutput_delete(&conctrl);
    
    return 0;
}



//////////////////////////////////////////////////////////////////////////////
static bool consoleOutput_init(ConsoleOutputCtrl *ctrl)
{
    HANDLE hOutputReadTmp;
    SECURITY_ATTRIBUTES sa;
    // Set up the security attributes struct.
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    ctrl->iSize = 1024;
    ctrl->pData = new char [ctrl->iSize];
    ctrl->iTotalDataRead = 0;

    // Create the child process output pipe.
    if (!CreatePipe(&hOutputReadTmp,&ctrl->hOutputWrite,&sa,0))
    {
        ASSERT(0);
        return false;
    }
    // Create a duplicate of the output write handle for the std error
    // write handle. This is necessary in case the child application
    // closes one of its std output handles.
    if (!DuplicateHandle(GetCurrentProcess(), ctrl->hOutputWrite,
                         GetCurrentProcess(),&ctrl->hErrorWrite,
                         0,TRUE,
                         DUPLICATE_SAME_ACCESS))
    {
        ASSERT(0);
        return false;
    }

    // Create new output read handle and the input write handles. Set
    // the Properties to FALSE. Otherwise, the child inherits the
    // properties and, as a result, non-closeable handles to the pipes
    // are created.
    if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
                         GetCurrentProcess(),&ctrl->hOutputRead, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
    {
        ASSERT(0);
        return false;
    }

    // Close inheritable copies of the handles you do not want to be
    // inherited.
    if (!CloseHandle(hOutputReadTmp)) 
    {
        ASSERT(0);
        return false;
    }

    return true;
}

static bool consoleOutput_read(ConsoleOutputCtrl *ctrl)
{
    if ( ctrl->pData == 0 )
    {
        ASSERT(0);
        return false;
    }
    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    if (!CloseHandle(ctrl->hOutputWrite)) 
    {
        ASSERT(0);
        return false;
    }
    if (!CloseHandle(ctrl->hErrorWrite)) 
    {
        ASSERT(0);
        return false;
    }

    DWORD read = 0;
    ctrl->iTotalDataRead = 0;
    while ( ReadFile(ctrl->hOutputRead, 
                     ctrl->pData + ctrl->iTotalDataRead, 
                     ctrl->iSize - ctrl->iTotalDataRead, 
                     &read,0) )
    {
        ctrl->iTotalDataRead += read;
        if ( ctrl->iTotalDataRead == ctrl->iSize )
        {   //enlarge buffer by 1KB and continue reading from child output stream Pipe
            char *pNewData = new char [ ctrl->iSize + 1024 ];
            memmove(pNewData, ctrl->pData, ctrl->iSize );
            ctrl->iSize += 1024;
            delete [] ctrl->pData;
            ctrl->pData = pNewData;
        }
    }
    //printf("\n\nNo more data received from child process.");

    //because console output data is text-based, append '\0' at end of data received
    if ( ctrl->iTotalDataRead == ctrl->iSize )
    {   //enlarge buffer by 1 byte to ensure EOS
        char *pNewData = new char [ ctrl->iSize + 1 ];
        memmove(pNewData, ctrl->pData, ctrl->iSize );
        ctrl->iSize += 1;
        delete [] ctrl->pData;
        ctrl->pData = pNewData;
    }
    ctrl->pData[ ctrl->iTotalDataRead ] = 0;//ensure end of text string
    ctrl->iTotalDataRead++;

    return true;
}


static void consoleOutput_delete(ConsoleOutputCtrl *ctrl)
{
    if (!CloseHandle(ctrl->hOutputRead)) 
    {
        ASSERT(0);
    }
  
    delete [] ctrl->pData;
    ctrl->iSize = 0;
    ctrl->pData = 0;
    ctrl->iTotalDataRead = 0;
}
 
