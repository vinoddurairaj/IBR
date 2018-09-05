/*
 * ftd_mngt_sys.cpp - ftd management system shudown related functionalities
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
#include "ftd_mngt.h"
#include <windows.h>
#include <winreg.h>

#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#else
#define ASSERT(exp)     ((void)0)
#endif
#define DBGPRINT(a)     error_tracef a 

//force to link with this Win32 library
#pragma comment(lib,"advapi32.lib")

///////////////////////////////////////////////////////////////////////////////
/* global variables */


///////////////////////////////////////////////////////////////////////////////
#ifndef MIN
#define MIN(a,b)    (a<b?a:b)
#endif

///////////////////////////////////////////////////////////////////////////////
/* local prototypes */

///////////////////////////////////////////////////////////////////////////////
/* local variables */
static char gcMngtSysNeedToRestart;

///////////////////////////////////////////////////////////////////////////////
void ftd_mngt_sys_initialize()
{
    gcMngtSysNeedToRestart = 0;
}

///////////////////////////////////////////////////////////////////////////////
void ftd_mngt_sys_restart_system()
{
	HANDLE hToken;              // handle to process token 
	TOKEN_PRIVILEGES tkp;       // pointer to token structure 
 
	BOOL fResult;               // system shutdown flag 

	// Get the current process token handle so we can get shutdown 
	// privilege. 
 
	OpenProcessToken(GetCurrentProcess(), 
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
 
	// Get the LUID for shutdown privilege. 
 
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
 
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
	// Get shutdown privilege for this process. 
 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES) NULL, 0); 
 
	// Do not display the shutdown dialog box and start the shutdown without a countdown. 
	fResult = InitiateSystemShutdown( 
		NULL,                                  // shut down local computer 
		NULL,                                  // message to user 
		0,                                     // time-out period : 0 = computer shuts down without displaying dialog box
		TRUE,                                  // apps are forcibly closed
		TRUE);                                 // reboot after shutdown 
 
	// Disable shutdown privilege. 
 
	tkp.Privileges[0].Attributes = 0; 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
			(PTOKEN_PRIVILEGES) NULL, 0); 
 
}

///////////////////////////////////////////////////////////////////////////////
//0 = no need to restart, otherwise need to restart TDMF Agent FTD Master Thread
int     ftd_mngt_sys_need_to_restart_system()
{
    return (int)gcMngtSysNeedToRestart;
}

///////////////////////////////////////////////////////////////////////////////
//called when critical values like BAB size, TCP Port or TCP Window size have been modified
void    ftd_mngt_sys_request_system_restart()
{
    gcMngtSysNeedToRestart = 1;
}
