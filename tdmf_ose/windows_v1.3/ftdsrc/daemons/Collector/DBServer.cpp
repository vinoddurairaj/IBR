/*
 * DBServer.cpp - Handles management requests from clients such as 
 *               ConfigTool and MonitorTool
 *
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <direct.h> //_getcwd()
#include "DBServer.h"
#include <errors.h>
extern "C" {
#include "iputil.h"
#include "ftd_cfgsys.h"
#include "DbgDmp.h"
}


//**************************************************************
//declarations of global variables
//
bool volatile  gbDBServerQuit;

//defined in WinService.cpp
extern HANDLE   ghTerminateEvent;

static int   giFlagLog = LOG_COLLECTOR_PRINTF_FLAG | LOG_COLLECTOR_LOGFILE_FLAG;
static int   giLogLevel = 0;
static bool  gbShowMMPData = false;
static bool  gbShowStats = false;
static bool  gbShowPerf  = false;

//
// COLLECTOR STATISTICS DATA
//
long    g_nNbMsgWaiting;
long    g_nNbThreadsRunning;
long    g_nNbMsgSent;
long    g_nNbThrdCreated;
long    g_nNbAliveMsg;
long    g_nNbAliveAgents;
bool    g_bKeyValid = false;


#pragma message(__LOC__ "Including collector->gui stats send")
//
// Local copy of TDMF collector statistics
// 
static  mmp_TdmfCollectorState      g_TdmfCollectorState;
//
// Global copy of message numbers
//
mmp_TdmfMessagesStates      g_TdmfMessageStates;
//
// Global copy of the resource manager (re-branding)
//
CResourceManager  g_ResourceManager;

static void InitializeCollectorStateStruct(void);

#ifdef _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_
void SendCollectorStatisticsDataToCommonApplication(void);
void RetreiveCollectorStatisticsData(mmp_TdmfCollectorState *data);
#endif

//**************************************************************
DWORD WINAPI CollectorMain(void* notused)
{
    CollectorConfig     cfg;

    //read trace level and flag from registry 'TraceLevel' and 'TraceFlag'
    //uses braces so path[] disapears of the stack after we are done with it. 
    {
        char    path[512];
        char    tmp[128];
        int     level,flags;    
        if ( cfg_get_software_key_value("TraceStat", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            gbShowStats = ( atoi(tmp) != 0 );
        else
            gbShowStats = false;//disable
        if ( cfg_get_software_key_value("TraceMMP", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            gbShowMMPData = ( atoi(tmp) != 0 );
        else
            gbShowMMPData = false;//disable
        if ( cfg_get_software_key_value("TraceLevel", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            level = atoi(tmp);
        else
            level = 0 ;//disable
        if ( cfg_get_software_key_value("TraceFlag", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            flags = atoi(tmp);
        else
            flags = 0 ;//disable
        if ( cfg_get_software_key_sz_value("InstallPath", path, sizeof(path)) == CFG_OK )
        {
            DbgDmpSetFileDir ( path );
        }
        else
        {
            if ( NULL != _getcwd(path,sizeof(path)) )
            {
                DbgDmpSetFileDir ( path );
            }
        }

        setCollectorTraceParams( level, flags );
        DbgDmpSetFileName( "CollectorTraces.log" );
#ifdef _DEBUG
        DbgDmpSetFileMaxSize ( 65536*16 );//1 MB max 
#else
        DbgDmpSetFileMaxSize ( 65536*4 );//256 KB max 
#endif

        TraceCrit("++++++++++++++ Collector STARTS ++++++++++++++");
        if ( giLogLevel ) 
        {
            TraceCrit("giLogLevel: %d ", giLogLevel);
        }
        
        if( gbShowMMPData )
        {
            TraceCrit("gbShowMMPData active");
        }

        if ( gbShowStats )
        {
            TraceCrit("gbShowStats active");
        }
        if ( gbShowPerf )
        {
            TraceCrit("gbShowPerf active \n");
        }       

        if ( cfg_get_software_key_value("PriorityBoost", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
        {
            if ( atoi(tmp) != 0 )
            {
                //todo ...
            }
        }
    }

	// Init db
    if ( 0 == dbserver_db_initialize(&cfg) )
        return 1;

    gbDBServerQuit = false;

	// Clear collector statistics 
    InitializeCollectorStateStruct(); 
	// Collector Statistics Time Initialization
    g_TdmfCollectorState.CollectorTime = time(0);

	// Check license
	g_bKeyValid = dbserver_db_CheckRegistrationKey();

    //blocks until gbDBServerQuit flag is set
    dbserver_sock_dispatch_io( &cfg );

    dbserver_db_close();

    TraceCrit("-------------- Collector STOPS --------------\n\n"); 
    DbgDmpFileClose();

    return 0;
}


void CollectorTrace( FILE* pStreamPtr, const char* message, ...)
{
    va_list arglist;

    if ( giFlagLog )
    {
        va_start(arglist, message);
        DbgDmpPrint ( 3 /*CRIT*/, 0 /*fnct*/, (char*)message, &arglist );
        va_end(arglist);
    }
}

void TraceCrit( const char* message, ...)
{
    va_list arglist;
    if ( giFlagLog )
    {
        va_start(arglist, message);
        DbgDmpPrint ( 0, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}

void TraceErr( const char* message, ...)
{
    va_list arglist;
    if ( giFlagLog && giLogLevel >= 1 )
    {
        va_start(arglist, message);
        DbgDmpPrint ( 1, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}
void TraceWrn( const char* message, ...)
{
    va_list arglist;
    if ( giFlagLog && giLogLevel >= 2 )
    {
        va_start(arglist, message);
        DbgDmpPrint ( 2, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}
void TraceInf( const char* message, ...)
{
    va_list arglist;
    if ( giFlagLog && giLogLevel >= 3 )
    {
        va_start(arglist, message);
        DbgDmpPrint ( 3, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}
void TraceAll( const char* message, ...)
{
    va_list arglist;
    if ( giFlagLog && giLogLevel >= 4 )
    {
        va_start(arglist, message);
        DbgDmpPrint ( 4, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}
void TraceMMP( const char* message, ...)
{
    if ( gbShowMMPData )
    {
        if ( giFlagLog && giLogLevel >= 3 )
        {
            va_list arglist;
            va_start(arglist, message);
            DbgDmpPrint ( 3, 0 /*no fnct string*/, (char*)message, &arglist );
            va_end(arglist);
        }
    }
}
void TraceStats( const char* message, ...)
{
    if ( giFlagLog && gbShowStats )
    {
        va_list arglist;
        va_start(arglist, message);
        DbgDmpPrint ( 2, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}

void TracePerf( const char* message, ...)
{
    if ( gbShowPerf )
    {
        va_list arglist;
        va_start(arglist, message);
        DbgDmpPrint ( 0, 0 /*no fnct string*/, (char*)message, &arglist );
        va_end(arglist);
    }
}

void setCollectorTraceParams(int level,int flags)
{
    setCollectorTraceLevel( level );

    if ( flags == 0 && level != 0 ) {
        flags = LOG_COLLECTOR_LOGFILE_FLAG ;//force logging to file
    }

    setCollectorTraceFlags( flags );
}
void setCollectorTraceLevel(int level)
{
    if( level & LOG_COLLECTOR_PERF_FLAG )
    {
        gbShowPerf = 1;
    }
        
    giLogLevel = level & ~LOG_COLLECTOR_PERF_FLAG;
}
void setCollectorTraceFlags(int flags)
{
    giFlagLog = flags;
    //to printf or not to printf ...
    DbgDmpSetToScreen ( giFlagLog & LOG_COLLECTOR_PRINTF_FLAG ? 1 : 0 );
    DbgDmpSetToFile   ( giFlagLog & LOG_COLLECTOR_LOGFILE_FLAG ? 1 : 0 );
}

// Use with DebugView utility
void ColDebugTrace(char* pcpMsg, ... )
{
    va_list     lVaLst;
    static char lscaMsg [128] = {0};

    va_start ( lVaLst, pcpMsg );
    _vsnprintf ( lscaMsg, 128, pcpMsg, lVaLst );
    va_end   ( lVaLst );

    OutputDebugString(lscaMsg);
}

//
// Dispatch character input received by collector
//
// Please add any new commands to the following list here:
//
// case 1-2-3-4-5:      set trace level to specified level
//
// case 'f':            set dump to file ON/OFF
//
// case 'g':            display agent monitor output map
//
// case 'p':            set output to console ON/OFF
//
// case 'm':            show mmp data
//
// case 'q':            display number of waiting DB messages
//
// case 'r':            reset trace flags to none
//
//
void WINAPI DispatchCommand(int nCh)
{
    char    OutputMessage[256];
    bool    bDisplayMsgBox      = false;

    switch(nCh)
    {
        //
        // Set trace level
        //
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        {
            DWORD TraceLevel = nCh - '0';
            giLogLevel = TraceLevel;
            sprintf(OutputMessage,"Collector tracelevel now %0ld",TraceLevel);
            bDisplayMsgBox = true;
            break;
        }

        //
        // Dump to file ON/OFF
        //
        case 'f':
        case 'F':
        {
            bDisplayMsgBox = true;
            if ( giFlagLog & LOG_COLLECTOR_LOGFILE_FLAG ) 
            {
                sprintf(OutputMessage,"Collector will now stop loging output to file");
                giFlagLog &= ~LOG_COLLECTOR_LOGFILE_FLAG;
            }
            else
            {
                sprintf(OutputMessage,"Collector will now log output to file");
                giFlagLog |= LOG_COLLECTOR_LOGFILE_FLAG;
            }
            DbgDmpSetToScreen ( giFlagLog & LOG_COLLECTOR_PRINTF_FLAG ? 1 : 0 );
            DbgDmpSetToFile   ( giFlagLog & LOG_COLLECTOR_LOGFILE_FLAG ? 1 : 0 );

            break;
        }

        case 'g':
        case 'G':
        {
            dbserver_agent_monit_output_map();
            break;
        }

        //
        //
        //
        case 'p':
        case 'P':
        {
            bDisplayMsgBox = true;
            if ( giFlagLog & LOG_COLLECTOR_PRINTF_FLAG ) 
            {
                sprintf(OutputMessage,"Collector will now stop outputing traces in console");
                giFlagLog &= ~LOG_COLLECTOR_PRINTF_FLAG;
            }
            else
            {
                sprintf(OutputMessage,"Collector will now output traces in console");
                giFlagLog |= LOG_COLLECTOR_PRINTF_FLAG;
            }
            DbgDmpSetToScreen ( giFlagLog & LOG_COLLECTOR_PRINTF_FLAG ? 1 : 0 );
            DbgDmpSetToFile   ( giFlagLog & LOG_COLLECTOR_LOGFILE_FLAG ? 1 : 0 );


            break;
        }

        //
        // show mmp data
        //
        case 'm':
        case 'M':
        {
            sprintf(OutputMessage,"Collector will now display mmp data");
            bDisplayMsgBox = true;
            gbShowMMPData   = true;
            break;
        }

        case 'q':
        case 'Q':
        {
            dbserver_db_output_nb_msg_waiting();
            break;
        }

        //
        // Reset to defaults
        //
        case 'r':
        case 'R':
        {
            sprintf(OutputMessage,"Collector console reset to no loging");
            bDisplayMsgBox = true;
            gbShowStats     = false;
            gbShowMMPData   = false;
            giLogLevel      = 0;
            giFlagLog       = 0;
            DbgDmpSetToScreen ( giFlagLog & LOG_COLLECTOR_PRINTF_FLAG ? 1 : 0 );
            DbgDmpSetToFile   ( giFlagLog & LOG_COLLECTOR_LOGFILE_FLAG ? 1 : 0 );
            break;
        }

    }

    if (bDisplayMsgBox)
    {
        MessageBox(NULL,OutputMessage,"INFORMATION",MB_OK);
    }
}   

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/////
/////   COLLECTOR STATISTICS HANDLING
/////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef _COLLECT_STATITSTICS_

#define ONE_MINUTE 60
#define ONE_HOUR   ONE_MINUTE * 60
#define MAX_PENDING_DB_MSG      9000

static void InitializeCollectorStateStruct(void)
{
    ZeroMemory(&g_TdmfCollectorState,sizeof(mmp_TdmfCollectorState));
    ZeroMemory(&g_TdmfMessageStates,sizeof(mmp_TdmfMessagesStates));
}

//
// This function sends all statistics to the correct place
//
void SendCollectorStatisticsData(void)
{
#ifdef _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_
    //
    // We send the data to the application we have 
    //
    SendCollectorStatisticsDataToCommonApplication();
#endif
}

//
// This function collects all statistics relevant to the collector
//
// It also collects all the data per minute (approximativly)
//                               per hour   (approximativly)
//
void AccumulateCollectorStats(void)
{
    // This lets us accumulate and send the sum of all values during
    // the first hour (the value will fluctuate and get larger)
    // after which we have our first hour value, and we only
    // change this value over time
    static  bool        bFirstHour      = true;
    // DB Pending messages alarm 
    static  bool        bPDBMsg         = false;
    // Since every minute we reset these values to 0
    // we accumulate them over an hour so we can send
    // them
    static  DWORD       CummulatedThrd  = 0;
    static  DWORD       CummulatedMsg   = 0;
    static  DWORD       CummulatedAlive = 0;
    // This checks to see how many seconds have passed between successive calls
    // it accumulates all the time until 1 hour is reached (3600 seconds)
    static  DWORD       CummulatedTime  = 0;
    // This gives us the time this function was last called
    // it gives us the possibility to collect information with a time range
    static  time_t      PreviousTime    = time(0);
    // Current time inside this function
            time_t      CurrentTime     = time(0);
    // difference between current time and previous time
            DWORD       TimeDifference  = CurrentTime - PreviousTime;

    //
    // Set directly managed values (current values)
    //
    g_TdmfCollectorState.NbPDBMsg               =   g_nNbMsgWaiting;

	if ( g_nNbMsgWaiting >= MAX_PENDING_DB_MSG )
	{
		if ( !bPDBMsg ) 
		{
			error_syslog_DirectAccess((LPSTR)g_ResourceManager.GetFullProductName(),"COLLECTOR MAX DB PENDING MSGs IS REACHED");
			bPDBMsg = true;
		}
	}
	else
		bPDBMsg = false;

    g_TdmfCollectorState.NbThrdRng              =   g_nNbThreadsRunning;
    g_TdmfCollectorState.NbAgentsAlive          =   g_nNbAliveAgents;

    //
    // Get per minute stats
    //
    if (TimeDifference > ONE_MINUTE)
    {
        CummulatedTime += TimeDifference;
		//
		// Register the current time at Statistic calculations
		//
		g_TdmfCollectorState.CollectorTime = time(0); 		

        g_TdmfCollectorState.NbPDBMsgPerMn      =   g_nNbMsgSent;
        CummulatedMsg                           +=  g_nNbMsgSent;
        g_nNbMsgSent                            =   0;

        g_TdmfCollectorState.NbThrdRngPerMn     =   g_nNbThrdCreated;
        CummulatedThrd                          +=  g_nNbThrdCreated;
        g_nNbThrdCreated                        =   0;

        g_TdmfCollectorState.NbAliveMsgPerMn    =   g_nNbAliveMsg;
        CummulatedAlive                         +=  g_nNbAliveMsg;
        g_nNbAliveMsg                           =   0;

        //
        // During first hour show variables growing
        //
        if (bFirstHour)
        {
            g_TdmfCollectorState.NbPDBMsgPerHr  =   CummulatedMsg;
            g_TdmfCollectorState.NbThrdRngHr    =   CummulatedThrd;
            g_TdmfCollectorState.NbAliveMsgPerHr=   CummulatedAlive;
        }

        //
        // Get local message states
        //
        memcpy(&g_TdmfCollectorState.TdmfMessagesStates,&g_TdmfMessageStates,sizeof(mmp_TdmfMessagesStates ));
        //
        // Reset local message states
        //
        ZeroMemory(&g_TdmfMessageStates,sizeof(mmp_TdmfMessagesStates ));

        //
        // Get per hour stats
        // 
        if (CummulatedTime > ONE_HOUR)
        {
            //
            // Once fist hour has passed, always update values every hour only
            //
            bFirstHour                          =   false;
            g_TdmfCollectorState.NbPDBMsgPerHr  =   CummulatedMsg;
            CummulatedMsg                       =   0;
            g_TdmfCollectorState.NbThrdRngHr    =   CummulatedThrd;
            CummulatedThrd                      =   0;
            g_TdmfCollectorState.NbAliveMsgPerHr=   CummulatedAlive;
            CummulatedAlive                     =   0;
			// Reset the HOUR timebase
            CummulatedTime = 0;
        }

        //
        // One minute has passed, validate new time value
        //
        PreviousTime = CurrentTime;
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/////
/////   COLLECTOR STATISTICS SENDING TO APPLICATION
/////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_

//
// This is for common window message, and common send message parameters
//
// #include "..\..\Collector_Stats\statistics.h"
// Defines now included in DBServer.h
//

#pragma message(__LOC__ "Including collector->application stats send")

static HANDLE           g_hMMFile;
static char *           g_pMMFile;

const UINT    wm_CollMsg = RegisterWindowMessage( COLLECTOR_MESSAGE_STRING );

static bool     DestroyMapFile(HANDLE H_MMFile, char * p_MMFile);
static char *   ReturnRW_MapedViewOfFile(HANDLE h_MMFile);
static bool     CreateMapFile (LPCSTR cszFileName, DWORD size);
static void     FlushMapFile(char * p_MMFile, DWORD size);

void InitializeCommonApplicationMemory(void)
{
    if (CreateMapFile(COLLECTOR_MESSAGE_STRING,4096))
    {
        g_pMMFile = ReturnRW_MapedViewOfFile(g_hMMFile);
    }
}

void DeleteCommonApplicationMemory(void)
{
    if (g_hMMFile)
    {
        DestroyMapFile(g_hMMFile,g_pMMFile);
    }
}


void SendCollectorStatisticsDataToCommonApplication(void)
{
    HWND                        WindowHandle;
    mmp_TdmfCollectorState*     pTempDataStruct;

    //
    // Initialise global copy of datastructure, then copy it 
    // to global memory
    //
    if (g_hMMFile)
    {
        WindowHandle = ::FindWindow(0, COLLECTOR_STATS_MESSAGE_WINDOW_NAME );

        if (WindowHandle)
        {
            pTempDataStruct = (mmp_TdmfCollectorState*)g_pMMFile;

            //
            // Copy golbal structure into common memory
            //
            memcpy(pTempDataStruct,&g_TdmfCollectorState,sizeof(mmp_TdmfCollectorState));

            ::SendMessage(WindowHandle,wm_CollMsg,COLLECTOR_MAGIC_NUMBER,0 );
        }
    }
}

void RetreiveCollectorStatisticsData(mmp_TdmfCollectorState *pTempDataStruct)
{
	memcpy(pTempDataStruct,&g_TdmfCollectorState,sizeof(mmp_TdmfCollectorState));
}

static bool CreateMapFile (LPCSTR cszFileName, DWORD size)
{
    if (g_hMMFile)
    {
        //
        // There is already a handle! Get out of here
        //
        return false;
    }

    //
    // Try to open file mapping
    //
    g_hMMFile = OpenFileMapping(FILE_MAP_WRITE, 
                                FALSE, 
                                cszFileName);

    //
    // Allocate at least 4K
    //
    if (size < 4096)
    {
        size = 4096;
    }

    //
    // If file mapping did not work, create it
    //
    if (!g_hMMFile)
    {
        g_hMMFile = CreateFileMapping ( (HANDLE)0xFFFFFFFF,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        size,
                                        cszFileName);
    }

    if (g_hMMFile)
    {
        return true;
    }

    return false;
}

static char * ReturnRW_MapedViewOfFile(HANDLE h_MMFile)
{
    char * p_MMFile;

    p_MMFile = (char *)MapViewOfFile (h_MMFile, 
                                      FILE_MAP_WRITE, 
                                      0, 
                                      0, 
                                      0);
    
    

    return p_MMFile;
}

static bool DestroyMapFile(HANDLE H_MMFile, char * p_MMFile)
{
    if (!H_MMFile)
    {
        return FALSE;
    }

    if (p_MMFile)
    {
        UnmapViewOfFile (p_MMFile);
    }

    CloseHandle(H_MMFile);

    return true;
}

static void FlushMapFile(char * p_MMFile, DWORD size)
{
    if (g_hMMFile)
    {
        FlushViewOfFile(p_MMFile,size);
    }
}


#endif