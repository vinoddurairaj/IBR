/*
 * DBServer_db.cpp - Handles all access to TDMF database
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
#include "stdafx.h"
#pragma warning(disable : 4786) //get rid of annoying STL warning
#include <map>      //using STL map 
#include <vector>   
using namespace std;

#include <process.h>
extern "C" {
#include "iputil.h"
#include "ftd_cfgsys.h"
#include "ftd_perf.h"
#include "ftd_mngt.h"
}
#include "libmngtmsg.h"
#include "DBServer.h"
#include "FsTdmfDb.h"
#include "FsTdmfRecNvpNames.h"
#include "license.h"
#include <errors.h>


static int dbserver_db_read_sysparams_collector_ip_port(CollectorConfig  *cfg);
static int dbserver_db_read_sysparams_db_check_period(FsTdmfDb* pTdmfDb);
static int dbserver_db_read_sysparams_db_perf_tbl_max_nbr(FsTdmfDb* pTdmfDb);
static int dbserver_db_read_sysparams_db_perf_tbl_max_days(FsTdmfDb* pTdmfDb);
static int dbserver_db_read_sysparams_db_alertstatus_tbl_max_nbr(FsTdmfDb* pTdmfDb);
static int dbserver_db_read_sysparams_db_alertstatus_tbl_max_days(FsTdmfDb* pTdmfDb);
static bool  localIsMessageWithoutText(const char* szMessage);
static short localIsGroupRelatedMessage(const char* szMessage);
static void  dbserver_db_write_version();
static void  LogKeyChange(FsTdmfDb* pTdmfDb, mmp_TdmfRegistrationKey *data);
static void dbserver_db_CheckRegistrationKey_request(FsTdmfDb* pTdmfDb, bool* pRetVal, HANDLE hEvent);
unsigned int dbserver_db_cleanup( FsTdmfDb* pTdmfDb );

static char EventViewerMsg[500];

#define MinuteElapsed   60  // 60 * 1 second!
#define HourElapsed     MinuteElapsed * 60
#define DayElapsed      HourElapsed * 24

////////////////////////////////////////

class CServerInfo 
{
public:
    int         iIP;
    int         iPort;
    int         iHostId;
    CString     cszServerName;//host name

    //default ctr
    CServerInfo(){};
    //copy ctr
    CServerInfo(const CServerInfo & aInfo)
    {
        assign(aInfo);
    };
    CServerInfo& operator=(const CServerInfo & aInfo)
    {
        assign(aInfo);
        return *this;
    };
    bool operator<(const CServerInfo & aInfo)
    {
        return iHostId < aInfo.iHostId;
    };
    void assign(const CServerInfo & aInfo)
    {
        iIP     = aInfo.iIP;
        iPort   = aInfo.iPort;
        iHostId = aInfo.iHostId;
        cszServerName = aInfo.cszServerName;
    };
};
typedef map<int/*host id*/,CServerInfo>    AgentUIDtoIP;
//
//TDMF Agent look-up table for which the Host ID (as a int value) is the key.
//
static  AgentUIDtoIP            gAllTDMFAgent;


////////////////////////////////////////
#define NOT_DAYS     0
#define NOT_NUMBER   0
#define MAX_NUMBER_OF_DB_DELETED_PER_ITERATION  250000
//
// This object performs the maintenance of one TDMF db table.
// Not multi-thread safe, must be called from the DB worker thread
class TableParams 
{
public:
    TableParams(const char *szTableShortName, const char *szTableName, FsTdmfDb *ptdmfDb, FsTdmfRec *ptdmfTable );

    void    ReadCheckPeriods();
    bool    CleanupTable();
    bool    CleanupTable(int type);

private:
    unsigned int        m_uiRequestedMaxNumber;  
    unsigned int        m_uiDynamicMaxNumber;  
    unsigned int        m_uiRequestedMaxDays;
    unsigned int        m_uiDynamicMaxDays;  
    unsigned long       m_uiLastCleanup;  
    unsigned int        m_uiCurrentDBSize;

    FsTdmfRec       *m_tdmfTable;
    FsTdmfDb        *m_tdmfDb;
    CString         m_cszTableName;
    CString         m_cszTableShortName;
};

TableParams::TableParams(const char *szTableShortName, const char *szTableName, FsTdmfDb *ptdmfDb, FsTdmfRec *ptdmfTable )
{
    m_tdmfTable         = ptdmfTable;
    m_tdmfDb            = ptdmfDb;
    m_cszTableName      = szTableName;
    m_cszTableShortName = szTableShortName;

    ReadCheckPeriods();
    m_uiDynamicMaxNumber = MAX_NUMBER_OF_DB_DELETED_PER_ITERATION;
    m_uiDynamicMaxDays   = m_uiRequestedMaxDays;
    m_uiCurrentDBSize    = 0;
}

void    TableParams::ReadCheckPeriods()
{
    if ( m_cszTableName == FsTdmfRecPerf::smszTblName )
    {   //performance table
        m_uiRequestedMaxNumber  = dbserver_db_read_sysparams_db_perf_tbl_max_nbr(m_tdmfDb);
        m_uiRequestedMaxDays    = dbserver_db_read_sysparams_db_perf_tbl_max_days(m_tdmfDb);
    }
    else if ( m_cszTableName == FsTdmfRecAlert::smszTblName )
    {   //alert status table
        m_uiRequestedMaxNumber = dbserver_db_read_sysparams_db_alertstatus_tbl_max_nbr(m_tdmfDb);
        m_uiRequestedMaxDays   = dbserver_db_read_sysparams_db_alertstatus_tbl_max_days(m_tdmfDb);
    }
    else
    {
        _ASSERT(0);
    }

    if ( m_uiRequestedMaxNumber <= 0 )
        m_uiRequestedMaxNumber = NOT_NUMBER;//disables this restriction

    if ( m_uiRequestedMaxDays <= 0 )
        m_uiRequestedMaxDays = NOT_DAYS;//disables this restriction
}

bool    TableParams::CleanupTable()
{
    if ( m_uiDynamicMaxNumber != NOT_NUMBER )
    {   //number criteria in effect
        if ( !CleanupTable(0) )
            return false;

    }   
    if ( m_uiDynamicMaxDays != NOT_DAYS )
    {   //expiration date in effect
        if ( !CleanupTable(1) )
            return false;
    }
    return true;
}

//
// This function is making the following assumptions:
//
// Errors in doing the delete are caused by timouts 
// Meaning that our delete is trying to delete too
// many records at once
//
// If this assumption is incorrect, we will be calling
// this function every 2 minutes for nothing!
//
//
bool    TableParams::CleanupTable(int type)
{
    BOOL b = TRUE;
    unsigned int uiRemoveValues;

    //use the m_uiDynamicMaxNumber criteria
    if ( type == 0 )
    {
        int recordCount;

        //
        // Because reading the size of the DB is long, we cache it until 
        // we are able to come back to acceptable size...
        // 
        // This is especially important if we are unable to delete
        // all the extra records in one pass!
        //
        if (m_uiCurrentDBSize)
        {
            recordCount = m_uiCurrentDBSize;
        }
        else
        {
            recordCount = m_tdmfTable->FtdCount();
            m_uiCurrentDBSize = recordCount;
        }

        //
        // Check against the maximum accepted value
        //
        if ( recordCount > (int)m_uiRequestedMaxNumber )
        {
            uiRemoveValues = recordCount - m_uiRequestedMaxNumber;

            //
            // We also have a dynamic range of removal
            //
            // This is in case the DB cannot remove the
            // number we request... Otherwise, we would 
            // end up never being able to remove the 
            // extra entries
            //
            if (uiRemoveValues > m_uiDynamicMaxNumber)
			{
                //
                // We remove by MAX_NUMBER_OF_DB_DELETED_PER_ITERATION
                // entries
                //
                uiRemoveValues = m_uiDynamicMaxNumber;
            }

            TraceAll( "DB maintenance: about to remove %d records from table %s ... ",uiRemoveValues , (LPCTSTR)m_cszTableName);
            b = m_tdmfTable->FtdDelete ( uiRemoveValues );
            if ( !b )
            {   //if SQL DELETE fails, it could be because of too large number of records to be deleted.
                //so next time, try with a smaller number
                m_uiDynamicMaxNumber /= 2;
                TraceAll( "DB maintenance: unable to cleanup table %s, too many records to cleanup. Retry later.", (LPCTSTR)m_cszTableName);
            }
            else 
            {   
#if DBG
                sprintf(EventViewerMsg,"DB Cleanup - Removed %d messages from DB\n",uiRemoveValues);
                OutputDebugString(EventViewerMsg);
#endif

                //
                // Substract the number from out count
                //
                m_uiCurrentDBSize -= uiRemoveValues;

                if (uiRemoveValues ==  (recordCount - m_uiRequestedMaxNumber))
                {
                    //
                    // if this is true, it means that we removed everything 
                    // that was over the cached size...
                    // so next time, re-read the size!
                    //
                    m_uiCurrentDBSize = 0;
				}
                else
                {
                    //
                    // We didn't remove everything this pass...
                    // make sure we get called again!
                    //
                    return false;
				}

                TraceAll( "DB maintenance: cleanup of table %s completed.", (LPCTSTR)m_cszTableName);
            }
        }
        else
        {
            //
            // Force reread of db size,
            // and reset max dynamic number of deleted records per iteration
            //
            m_uiCurrentDBSize = 0;
            m_uiDynamicMaxNumber = MAX_NUMBER_OF_DB_DELETED_PER_ITERATION;
        }
    }
    //use the TimeStamp criteria
    else if ( type == 1 )
    {
        CString     cszWhere;
        CTime       lastdaytokeep;

        lastdaytokeep = CTime(time(0)) - CTimeSpan( m_uiDynamicMaxDays, 0, 0, 0 );
        //delete records with ts up to lastdaytokeep.
        cszWhere.Format( " time_stamp < '%s' "
                         , (LPCTSTR)lastdaytokeep.Format("%Y-%m-%d") );

        TraceAll( "DB maintenance: about to remove records %s from table %s ... ", (LPCTSTR)cszWhere, (LPCTSTR)m_cszTableName);

        b = m_tdmfTable->FtdDelete ( cszWhere );//_ASSERT(b);
        if ( !b )
        {   //if SQL DELETE fails, it could be because of too large number of records to be deleted.
            //so next time, try with a date that is further back in time...
            m_uiDynamicMaxDays++;
            TraceAll( "DB maintenance: unable to cleanup table %s, too many records to cleanup. Retry later.", (LPCTSTR)m_cszTableName);
        }
        else
        {   
            //
            // Reset cached db size, we deleted some entries, so we need to re-read this again
            //
            m_uiCurrentDBSize = 0;

            if (m_uiDynamicMaxDays <= m_uiRequestedMaxDays)
            {
                //
                // We actually removed everything we wanted...
                //
                m_uiDynamicMaxDays = m_uiRequestedMaxDays;
			}
            else
            {
                //
                // We didn't remove everything, so lets readjust upwards to try and delete an extra day
                //
                m_uiDynamicMaxDays--;
                return false;
            }

            TraceAll( "DB maintenance: cleanup of table %s completed.", (LPCTSTR)m_cszTableName);
        }
    }

    return b == TRUE;
}

// ***********************************************************
// Function name    : dbserver_db_cleanup
// Description      : Resposible for garbage collection : removing
//                    older records from some Tdmf DB tables.
// Return type      : unsigned int
// Argument         : pointer to TdmfDb
// 
// ***********************************************************
unsigned int dbserver_db_cleanup( FsTdmfDb* pTdmfDb )
{
    static TableParams tblPerf("Perf", pTdmfDb->mpRecPerf->smszTblName, pTdmfDb, pTdmfDb->mpRecPerf );
    static TableParams tblAlertStatus("AlertStatus", pTdmfDb->mpRecAlert->smszTblName, pTdmfDb, pTdmfDb->mpRecAlert );
    
    int                 period;
    static unsigned int uiThreadWorkPeriod  = HourElapsed;
    
    //
    // By default next run time should be in uiThreadWorkPeriod minutes
    //
    unsigned int        ReturnValue;
#if DBG
    OutputDebugString("DataBase Cleanup!\n");
#endif
    //
    //read configuration parameters at each pass allows dynamic modifications of these parameters.
    //
    tblPerf.ReadCheckPeriods();
    tblAlertStatus.ReadCheckPeriods();
    if ( (period = dbserver_db_read_sysparams_db_check_period(pTdmfDb)) > 0 )
    {
        uiThreadWorkPeriod = (unsigned int)period * MinuteElapsed;
    }

    ReturnValue = uiThreadWorkPeriod;
    
    //and other threads might be waiting to access the DB
    if (!tblPerf.CleanupTable())
    {
        //
        // We were unable to delete everything because of a timeout???
        //
        // Set next cleanup in 2 minutes
        //
        ReturnValue = 2 * MinuteElapsed;
#if DBG
        sprintf(EventViewerMsg,"DB Cleanup - Performance Table - Rescheduling cleanup in 2 minutes\n");
        OutputDebugString(EventViewerMsg);
#endif
    }
#if DBG
    else
    {
        sprintf(EventViewerMsg,"DB Cleanup - Performance Table - Completed");
        error_syslog_DirectAccess((LPSTR)g_ResourceManager.GetFullProductName(),EventViewerMsg);
        sprintf(EventViewerMsg,"DB Cleanup - Performance Table - Completed\n");
        OutputDebugString(EventViewerMsg);
    }
#endif

    //
    // Give other threads some breathing room
    //
    Sleep(0);
    
    //
    // Cleanup AlertStatus table
    //
    if (!tblAlertStatus.CleanupTable())
    {
        //
        // We were unable to delete everything because of a timeout???
        //
        // Set next cleanup in 2 minutes
        //
        ReturnValue = 2 * MinuteElapsed;
#if DBG
        sprintf(EventViewerMsg,"DB Cleanup - Alert Status Table - Rescheduling cleanup in 2 minutes\n");
        OutputDebugString(EventViewerMsg);
#endif
    }
#if DBG
    else
    {

        //
        // Only display message if a cleanup is not rescheduled in 2 minutes!
        //
        if (ReturnValue != 2 * MinuteElapsed)
        {
            sprintf(EventViewerMsg,"DB Cleanup - Alert Status Table - Completed");
            error_syslog_DirectAccess((LPSTR)g_ResourceManager.GetFullProductName(),EventViewerMsg);
            sprintf(EventViewerMsg,"DB Cleanup - Alert Status Table - Completed\n");
            OutputDebugString(EventViewerMsg);
        }
    }
#endif

    // RE-Check Registration Key
    dbserver_db_CheckRegistrationKey_request(pTdmfDb, NULL, NULL);

    return ReturnValue;
}


///////////////////////////////////////////////////////////
// Working thread (the one and only who can access the db)

HANDLE g_hWorkingThreadReady = NULL;
UINT   g_nThreadID = 0;

#define INC_MSG_WAITING InterlockedIncrement(&g_nNbMsgWaiting)
#define DEC_MSG_WAITING InterlockedDecrement(&g_nNbMsgWaiting)

enum ThreadMessage
{
    TM_FIRST = WM_USER,
    TM_INIT_ALLAGENT = TM_FIRST,
    TM_STOP_THREAD,
    TM_DB_UPDATE_KEY,
    TM_UPDATE_SERVERINFO,
    TM_UPDATE_SERVER_STATE,
    TM_UPDATE_SERVER_STATE_WITH_HOSTID,
    TM_ADD_ALERT,
    TM_ADD_PERFDATA,
    TM_UPDATE_NVP_PERFCFG,
    TM_WRITE_SYSPARAMS,
    TM_WRITE_VERSION,
    TM_READ_SYSPARAMS_COLLECTOR_IP_PORT,
    TM_DBSERVER_DB_READ_SYSPARAMS,
    TM_CHECK_REGISTRATION_KEY,
    TM_WRITE_COLLECTOR_KEY,
    TM_LAST
};

// Params packing structures
struct sysparams_params
{
    mmp_TdmfCollectorParams* dbparams;
    CollectorConfig*         cfg;
    int*                     pnRetVal;
    HANDLE                   hEvent;
};


int dbserver_db_initialize_AllAgent()
{
    int nRetCode  = 1;
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (PostThreadMessage(g_nThreadID, TM_INIT_ALLAGENT, (WPARAM)hEvent, (LPARAM)&nRetCode) != 0)
    {
        INC_MSG_WAITING;

        WaitForSingleObject(hEvent, INFINITE);
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }

    CloseHandle(hEvent);

    return nRetCode;
}

void dbserver_db_stop_working_thread()
{
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (PostThreadMessage(g_nThreadID, TM_STOP_THREAD, (WPARAM)hEvent, 0) != 0)
    {
        INC_MSG_WAITING;

        WaitForSingleObject(hEvent, INFINITE);
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }

    CloseHandle(hEvent);
}

void    dbserver_db_update_key(mmp_TdmfRegistrationKey *data)
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfRegistrationKey* pDataCopy = new mmp_TdmfRegistrationKey;
    memcpy(pDataCopy, data, sizeof(mmp_TdmfRegistrationKey));

    if (PostThreadMessage(g_nThreadID, TM_DB_UPDATE_KEY, (WPARAM)pDataCopy, 0) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

void    dbserver_db_update_serverInfo(mmp_TdmfServerInfo *data)
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfServerInfo * pDataCopy = new mmp_TdmfServerInfo;
    memcpy(pDataCopy, data, sizeof(mmp_TdmfServerInfo));

    if (PostThreadMessage(g_nThreadID, TM_UPDATE_SERVERINFO, (WPARAM)pDataCopy, 0) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

void    dbserver_db_update_server_state(mmp_TdmfAgentState *agentState, int iAgentIP)
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfAgentState* pDataCopy = new mmp_TdmfAgentState;
    memcpy(pDataCopy, agentState, sizeof(mmp_TdmfAgentState));

    if (PostThreadMessage(g_nThreadID, TM_UPDATE_SERVER_STATE, (WPARAM)pDataCopy, (LPARAM)iAgentIP) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

void    dbserver_db_update_server_state(int iHostID, enum tdmf_agent_state eState)
{
    if (PostThreadMessage(g_nThreadID, TM_UPDATE_SERVER_STATE_WITH_HOSTID, (WPARAM)iHostID, (LPARAM)eState) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

void    dbserver_db_add_alert(const mmp_TdmfAlertHdr *alertData, const char* szAlertMessage )
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfAlertHdr* pDataCopy = new mmp_TdmfAlertHdr;
    memcpy(pDataCopy, alertData, sizeof(mmp_TdmfAlertHdr));
    char* pszCopy = strdup(szAlertMessage);

    if (PostThreadMessage(g_nThreadID, TM_ADD_ALERT, (WPARAM)pDataCopy, (LPARAM)pszCopy) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

void dbserver_db_add_perfdata( mmp_TdmfPerfData *perfHdr, ftd_perf_instance_t *perfData )
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfPerfData* pDataCopy = new mmp_TdmfPerfData;
    memcpy(pDataCopy, perfHdr, sizeof(mmp_TdmfPerfData));
    int iNbrPerfData = perfHdr->iPerfDataSize / sizeof(ftd_perf_instance_t);
    ftd_perf_instance_t* pPerfDataCopy = new ftd_perf_instance_t[iNbrPerfData];
    memcpy(pPerfDataCopy, perfData, sizeof(ftd_perf_instance_t)*iNbrPerfData);

    if (PostThreadMessage(g_nThreadID, TM_ADD_PERFDATA, (WPARAM)pDataCopy, (LPARAM)pPerfDataCopy) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

void    dbserver_db_update_NVP_PerfCfg(const mmp_TdmfPerfConfig * perfCfg)
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfPerfConfig* pDataCopy = new mmp_TdmfPerfConfig;
    memcpy(pDataCopy, perfCfg, sizeof(mmp_TdmfPerfConfig));

    if (PostThreadMessage(g_nThreadID, TM_UPDATE_NVP_PERFCFG, (WPARAM)pDataCopy, 0) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

int dbserver_db_write_sysparams(const mmp_TdmfCollectorParams *sysparams, CollectorConfig  *cfg)
{
    // Copy data.  Will be deleted after the processing of the msg in the working thread
    mmp_TdmfCollectorParams* pDataCopy = new mmp_TdmfCollectorParams;
    memcpy(pDataCopy, sysparams, sizeof(mmp_TdmfCollectorParams));
    CollectorConfig* pCfgCopy = new CollectorConfig;
    memcpy(pCfgCopy, cfg, sizeof(CollectorConfig));

    if (PostThreadMessage(g_nThreadID, TM_WRITE_SYSPARAMS, (WPARAM)pDataCopy, (LPARAM)pCfgCopy) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }

    // TODO: return real code
    return 0;
}

static void dbserver_db_write_version()
{
    if (PostThreadMessage(g_nThreadID, TM_WRITE_VERSION, 0, 0) != 0)
    {
        INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

static int dbserver_db_read_sysparams_collector_ip_port(CollectorConfig  *cfg)
{
    int nPort = 0;
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (PostThreadMessage(g_nThreadID, TM_READ_SYSPARAMS_COLLECTOR_IP_PORT, (WPARAM)cfg, (LPARAM)hEvent) != 0)
    {
        INC_MSG_WAITING;
        
        WaitForSingleObject(hEvent, INFINITE);

        nPort = cfg->iPort;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }

    CloseHandle(hEvent);

    return nPort;
}

int dbserver_db_read_sysparams(mmp_TdmfCollectorParams * dbparams, CollectorConfig  *cfg)
{
    int nRetVal = 0;
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    struct sysparams_params params;
    params.dbparams = dbparams;
    params.cfg      = cfg;
    params.pnRetVal = &nRetVal;
    params.hEvent   = hEvent;

    if (PostThreadMessage(g_nThreadID, TM_DBSERVER_DB_READ_SYSPARAMS, (WPARAM)&params, (LPARAM)hEvent) != 0)
    {
        INC_MSG_WAITING;
        
        WaitForSingleObject(hEvent, INFINITE);
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }

    CloseHandle(hEvent);

    return nRetVal;
}

bool dbserver_db_CheckRegistrationKey()
{
    bool bValid = false;
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (PostThreadMessage(g_nThreadID, TM_CHECK_REGISTRATION_KEY, (WPARAM)&bValid, (LPARAM)hEvent) != 0)
    {
        INC_MSG_WAITING;
        
        WaitForSingleObject(hEvent, INFINITE);
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }

    CloseHandle(hEvent);

    return bValid;
}

void dbserver_db_SetCollectorRegistrationKey(char* szKey)
{
    char* pszCopy = strdup(szKey);

    if (PostThreadMessage(g_nThreadID, TM_WRITE_COLLECTOR_KEY, (WPARAM)pszCopy, 0) != 0)
    {
       INC_MSG_WAITING;
    }
    else // Error
    {
        DWORD dwErr = GetLastError();
        TraceErr("PostThreadMessage Fail with error code: %d", dwErr);
    }
}

///////////////////////////////////////////////////////////////////
// request functions

void dbserver_db_initialize_AllAgent_request(FsTdmfDb* pTdmfDb, HANDLE hEvent, int* nRetCode)
{
    //fill gAllTDMFAgent Map with info on all Agents.
    //for faster access to Agent IP/Port information.
    CString cszWhere;
    cszWhere.Format("%s != 0", (LPCTSTR)FsTdmfRecSrvInf::smszFldHostId );//bogus criteria to get ALL servers 
    if ( pTdmfDb->mpRecSrvInf->FtdFirst(cszWhere) )
    {
        do
        {   //build Map 
            CServerInfo     info;
            CString         cszTmp;
            string          strTmp;

            cszTmp     = pTdmfDb->mpRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldSrvIp1);
            info.iIP   = atoi(cszTmp);
            cszTmp     = pTdmfDb->mpRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldIpPort);
            info.iPort = atoi(cszTmp);
            cszTmp     = pTdmfDb->mpRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldHostId);
            info.iHostId = strtol(cszTmp,0,10);_ASSERT(info.iHostId != 0);
            info.cszServerName = pTdmfDb->mpRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldName);

            gAllTDMFAgent[ info.iHostId ] = info;
        }
        while ( pTdmfDb->mpRecSrvInf->FtdNext() );
    }

    *nRetCode = 0;

    SetEvent(hEvent);
}

void    dbserver_db_update_key_request(FsTdmfDb* pTdmfDb, mmp_TdmfRegistrationKey *data)
{
    TraceWrn("dbserver_db_update_key... : write key to db, %s , %s \n",data->szServerUID,data->szRegKey);

    int iHostId = strtoul(data->szServerUID, NULL, 16);
    if ( !pTdmfDb->mpRecSrvInf->FtdPos(iHostId) )//or ServerName or SrvId ??
    {   //this TDMF Collector is not found in DB. add it
        // should never happen but , ...
        if ( !pTdmfDb->mpRecSrvInf->FtdNew(iHostId) )
        {
            CString lszMsg;
            lszMsg.Format (
                "dbserver_db_update_agentConfiguration(): "
                "Cant access <%s> , 0x%x = %d\n<%s>",
                data->szServerUID, iHostId, iHostId, pTdmfDb->FtdGetErrMsg()
            );
            TraceWrn("**** dbserver_db_update_key: Error while adding a new Server Info record in db! %s \n", pTdmfDb->FtdGetErrMsg());
            pTdmfDb->FtdErrReset();
            return;
        } 
    }

    //update t_ServerInfo fields
    CString oldKey = pTdmfDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldKey );
    //backup actual current key and then update with the recvd key value
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldKeyOld,      oldKey );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldKey,         data->szRegKey );
    if ( data->iKeyExpirationTime < 0 )
        data->iKeyExpirationTime = 0;
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldKeyExpire,   CTime(data->iKeyExpirationTime).Format("%Y-%m-%d %H:%M:%S") );

    ///////////////////////////////////////////////////////////////////////////
    // Log key change (if it has changed)
    LogKeyChange(pTdmfDb, data);
}

void LogKeyChange(FsTdmfDb* pTdmfDb, mmp_TdmfRegistrationKey *data)
{
    CString szHostname;
    bool    bLog    = false;
    int     iHostId = 0;

    if (strcmp(data->szServerUID, "Collector") == 0)
    {
        szHostname = "Collector";
		
		// Read host id from db
        if ( pTdmfDb->mpRecNvp->FtdPos(TDMF_NVP_NAME_COLLECTOR_HOST_ID) )
        {
			CString cstrHostId = pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal);
			char* pszEnd;
			iHostId = strtol(cstrHostId, &pszEnd, 16);
		}

        bLog = true;
    }
    else
    {
        iHostId = strtoul(data->szServerUID, NULL, 16);
        if ( pTdmfDb->mpRecSrvInf->FtdPos(iHostId) )
        {  
            szHostname = pTdmfDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldName );
                        
            CString szWhere;
            szWhere.Format( "%s = '%s'", pTdmfDb->mpRecKeyLog->smszFldHostname, szHostname );
            
            if (!pTdmfDb->mpRecKeyLog->FtdFirst( szWhere ) )
            {
                bLog = true;
            }
            else
            {
                CString oldKey = pTdmfDb->mpRecKeyLog->FtdSel( FsTdmfRecKeyLog::smszFldKey );
                if (oldKey != data->szRegKey)
                {
                    bLog = true;
                }
            }
        }
    }

    // Log key change (if it has changed)
    if (bLog)
    {
        // Date Server NewKey ExpirationDate
        CString szDate    = CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S");
        CString szExpDate = CTime(data->iKeyExpirationTime).Format("%Y-%m-%d %H:%M:%S");
                
        if ( !pTdmfDb->mpRecKeyLog->FtdNew(szDate, szHostname, iHostId, data->szRegKey, szExpDate) )
        {
            TraceWrn("**** LogKeyChange: Error while adding a new Key Log record in db! %s \n", pTdmfDb->FtdGetErrMsg());
            pTdmfDb->FtdErrReset();
            return;
        } 
    }
}

void    dbserver_db_update_serverInfo_request(FsTdmfDb* pTdmfDb, mmp_TdmfServerInfo *data)
{
    bool bNewServer;


    TraceAll("dbserver_db_update_serverInfo... write agent info to db,"
             "machinename=%s , hostid=%s , ip=%s , port=%d, domain=%s \n"
            ,data->szMachineName,data->szServerUID,data->szIPAgent[0],data->iPort,data->szTDMFDomain
            );

    int iHostId = strtoul(data->szServerUID, NULL, 16);
    if ( !pTdmfDb->mpRecSrvInf->FtdPos(iHostId) )
    {   //this TDMF Server/Agent is not found in DB. add it
        //position on Domain record, create it if necessary 
        if ( !pTdmfDb->mpRecDom->FtdPos(data->szTDMFDomain) )
        {   //this TDMF Server/Agent is not found in DB. add it
            if ( !pTdmfDb->mpRecDom->FtdNew(data->szTDMFDomain) )
            {
                CString lszMsg;
                lszMsg.Format (
                    "dbserver_db_update_serverInfo(): "
                    "Can\'t access <%s>\n<%s>",
                    data->szTDMFDomain, pTdmfDb->FtdGetErrMsg()
                );
                TraceWrn("**** dbserver_db_update_serverInfo: Error while adding a new Server Info record in db! %s \n");
                pTdmfDb->FtdErrReset();
                return;
            } 
        }

        if ( !pTdmfDb->mpRecSrvInf->FtdNew(iHostId) )
        {
            CString lszMsg;
            lszMsg.Format (
                "dbserver_db_update_serverInfo(): "
                "Cant access host id %d (0x%x)\n<%s>",
                iHostId, iHostId, pTdmfDb->FtdGetErrMsg()
            );
            TraceWrn("Error while adding a new Server Info record in db! %s \n");
            pTdmfDb->FtdErrReset();
            return;
        } 
        bNewServer = true;
        TraceAll("dbserver_db_update_serverInfo() : %s (%s , 0x%08x) is NEW in DB",data->szMachineName,data->szServerUID,iHostId);
    }
    else
    {   //Server already exists in DB.  Not updating its Domain, therefore mpRecDom position is not relevant.
        bNewServer = false;
        TraceAll("dbserver_db_update_serverInfo() : %s (%s , 0x%08x) already known in DB",data->szMachineName,data->szServerUID,iHostId);
    }

    ///////////////////////////////////////////////////////////////////////////
    //update t_ServerInfo fields
    ///////////////////////////////////////////////////////////////////////////
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,       data->iPort );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldName,         data->szMachineName );

    unsigned long ipv4,ipv4_rtr;
    switch( data->ucNbrIP )
    {
    case 4:
        ipstring_to_ip(data->szIPAgent[3],&ipv4);
        pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvIp4,   ipv4 );
        //intentionnaly without break statement
    case 3:
        ipstring_to_ip(data->szIPAgent[2],&ipv4);
        pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvIp3,   ipv4 );
        //intentionnaly without break statement
    case 2:
        ipstring_to_ip(data->szIPAgent[1],&ipv4);
        pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvIp2,   ipv4 );
        //intentionnaly without break statement
    case 1 :
        ipstring_to_ip(data->szIPAgent[0],&ipv4);
        pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvIp1,   ipv4 );
        break;
    }

    //add to Map for next time it is requested
    CServerInfo      info;

    //WR32867++: Important: A router could translate the IP addr, so we need to use it by priority, 
    //dbserver_sock_manage_broadcast_response() overwrote the Agent's ->szIPRouter field
    ipstring_to_ip(data->szIPRouter,&ipv4_rtr);
    
    if (ipv4_rtr != 0 && ipv4 != ipv4_rtr)
        {
        // use the IP address provided by router
        ipv4 = ipv4_rtr;        
        }
    //WR32867--

    info.iIP     =   ipv4;    // the address to be used to communicate later on
    info.iPort   =   data->iPort;
    info.iHostId =   iHostId;_ASSERT(info.iHostId != 0);
    info.cszServerName = data->szMachineName;
    gAllTDMFAgent[ iHostId ] = info;

    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldRtrIp1,       ipv4_rtr); //WR32867
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldRtrIp2,       0 );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldRtrIp3,       0 );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldRtrIp4,       0 );

    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldBabSizeReq,      data->iBABSizeReq );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldBabSizeAct,      data->iBABSizeAct );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldTcpWinSize,   data->iTCPWindowSize );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsType,       data->szOsType );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsVersion,    data->szOsVersion );
	// Filter out unecessary info from TdmfVersion
	CString cstrTdmfVersion = data->szTdmfVersion;
	int nIndex = cstrTdmfVersion.FindOneOf("0123456789");
	if (nIndex > -1)
	{
		cstrTdmfVersion = cstrTdmfVersion.Mid(nIndex);
	}
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldTdmfVersion,  cstrTdmfVersion );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldNumberOfCpu,  data->iNbrCPU );
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldRamSize,      data->iRAMSize );

    //update some values only if server is ADDED to DB, not for an update
    if ( bNewServer )
    {
        CString cszDomainFk = pTdmfDb->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );
        pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldDomFk,    cszDomainFk );

        BOOL b1,b2;
        b1 = pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldDefaultJournalVol, data->szTdmfJournalPath );
        b2 = pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldDefaultPStoreFile, data->szTdmfPStorePath );
        TraceAll("dbserver_db_update_serverInfo() : pstore path (%s) and journal path (%s) : update %s ", data->szTdmfPStorePath, data->szTdmfJournalPath, (b1 && b2) ? "successful" : "*** ERROR ***" );
    }
    else
    {
        TraceAll("dbserver_db_update_serverInfo() : pstore path (%s) and journal path (%s) NOT updated because server already appears in DB", data->szTdmfPStorePath, data->szTdmfJournalPath );
    }
}

void    dbserver_db_update_server_state_request(FsTdmfDb* pTdmfDb, mmp_TdmfAgentState *agentState, int iAgentIP)
{
    //position on a ServInfo record
    if ( agentState->szServerUID[0] != 0 )
    {
        int iHostId = strtoul(agentState->szServerUID, NULL, 16);
        if ( !pTdmfDb->mpRecSrvInf->FtdPos(iHostId) )
        {   //no record found for this Server/Agent
            CString lszMsg;
            lszMsg.Format (
                "dbserver_db_update_server_state(): "
                "Can\'t find Server record for host id <%s> , 0x%x\n<%s>",
                agentState->szServerUID, iHostId, pTdmfDb->FtdGetErrMsg()
            );
            TraceWrn((LPCTSTR)lszMsg);
            pTdmfDb->FtdErrReset();
            return;
        }
    }
    else
    {   //Server unique ID not provided. Try to find its record in DB according to its IP address
        CString cszWhere;
        cszWhere.Format( " %s = %d ", (LPCTSTR)FsTdmfRecSrvInf::smszFldSrvIp1 , iAgentIP );
        if ( !pTdmfDb->mpRecSrvInf->FtdFirst( cszWhere ) )
        {   //no record found for this Server/Agent 
            CString lszMsg;
            char    str[32];
            ip_to_ipstring(iAgentIP,str);
            lszMsg.Format (
                "dbserver_db_update_server_state(): "
                "Can\'t find Server record with IP1=%s\n<%s>",
                str, pTdmfDb->FtdGetErrMsg()
            );
            TraceWrn((LPCTSTR)lszMsg);
            pTdmfDb->FtdErrReset();
            return;        
        }
    }
    //mpRecSrvInf is positonned on the Server record
    pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldState, agentState->iState );
}

void    dbserver_db_update_server_state_request(FsTdmfDb* pTdmfDb, int iHostID, enum tdmf_agent_state eState)
{
    //position on a ServInfo record
    if ( !pTdmfDb->mpRecSrvInf->FtdPos(iHostID) )
    {   //no record found for this Server/Agent
        CString lszMsg;
        lszMsg.Format (
            "dbserver_db_update_server_state(): "
            "Can\'t find Server record for host id 0x%x\n<%s>",
            iHostID, pTdmfDb->FtdGetErrMsg()
        );
        TraceWrn((LPCTSTR)lszMsg);
        pTdmfDb->FtdErrReset();
    }
    else
    {   //ok, update Server state
        pTdmfDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldState, (int)eState );
    }
}



void    dbserver_db_add_alert_request(FsTdmfDb* pTdmfDb, const mmp_TdmfAlertHdr *alertData, const char* szAlertMessage )
{
    //position on a ServInfo record
    int iHostId = strtoul(alertData->szServerUID, NULL, 16);

    short grpnbr;
    if ( alertData->sLGid < 0 )
        //try to find out the group number from the message content
        grpnbr = localIsGroupRelatedMessage(szAlertMessage);
    else
        grpnbr = alertData->sLGid;

	// NOTE: This code has not been written by me ;-)
	if ( grpnbr != -1 )
	{
		if (pTdmfDb->mpRecSrvInf->FtdPos(iHostId))
		{
			CString lszSrvId = pTdmfDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );

			// SELECT * FROM t_Logicalgroup WHERE (LgGroupId = grpnbr) AND (Source_Fk = lszSrvId) OR (LgGroupId = grpnbr) AND (Target_Fk = lszSrvId)
			CString cstrWhere;
			cstrWhere.Format("(%s = %d) AND (%s = %s) OR (%s = %d) AND (%s = %s)", // The WHERE clause has been changed 
							 FsTdmfRecLgGrp::smszFldLgGrpId, grpnbr,
							 FsTdmfRecLgGrp::smszFldSrcFk, lszSrvId,
							 FsTdmfRecLgGrp::smszFldLgGrpId, grpnbr,
							 FsTdmfRecLgGrp::smszFldTgtFk, lszSrvId);

			if (pTdmfDb->mpRecGrp->FtdFirst(cstrWhere))
			{
				CString lszSourceSvrId = pTdmfDb->mpRecGrp->FtdSel(FsTdmfRecLgGrp::smszFldSrcFk);
				// If the source server's Id (read in the group that we have found) is not the same as the one in the msg,
				// that means that this is a msg from a remote server.  Log the message under the source server
				if (lszSourceSvrId != lszSrvId)
				{
					// Find the HostId associated with lszSourceSvrId
					CString cstrWhere;
					cstrWhere.Format("%s = %d", FsTdmfRecSrvInf::smszFldKey, lszSourceSvrId);
					if (pTdmfDb->mpRecSrvInf->FtdFirst(cstrWhere))
					{
						iHostId = atoi(pTdmfDb->mpRecGrp->FtdSel(FsTdmfRecSrvInf::smszFldHostId));
					}
				}
			}
		}
	}

    //add new record in t_Alert table
    char *pszType;
    switch( alertData->cType )
    {
    case MMP_MNGT_MONITOR_TYPE_INFORMATION:
        pszType = "INFO\0";
        break;
    case MMP_MNGT_MONITOR_TYPE_WARNING:
        pszType = "WARNING\0";
        break;
    case MMP_MNGT_MONITOR_TYPE_ERROR:
        pszType = "FATAL\0";
        break;
    default:
        pszType = "UNKNOWN!\0";
        break;
    }

    CString cszTimeStampFromMsg;
    int iOffset = 0;
    //if message begins with date-time, remove it from msg and use it as thr new record time stamp
    //expected date-time format is : [YYYY/MM/DD HH:MM:SS]
    if ( *szAlertMessage == '[' && atoi(szAlertMessage + 1) > 1990 )
    {   //validate with a valid year number 
        iOffset = 1;
        while( *(szAlertMessage+iOffset) != ']' &&  iOffset < 32 )//over 32, there is a format problem in msg
            iOffset++;
        if ( iOffset != 32 )
        {
            char szTmpTS[40];
            memcpy(szTmpTS, szAlertMessage + 1, iOffset - 1 );
            szTmpTS[ iOffset - 1 ] = 0; 
            cszTimeStampFromMsg = szTmpTS;
            iOffset++;//increment so cszMsg starts just after ']'
        }
        else
        {
            iOffset = 0;//unexpected msg format, do not extract date-time from it.
        }
    }


    CString cszMsg = szAlertMessage + iOffset;
    //replace non-visible chars that causes problems when viewing the column text from within database
    cszMsg.Replace( '\n', ' ' );
    cszMsg.Replace( '\r', ' ' );
    cszMsg.Replace( '\'', ' ' );

    //
    // SVG 17 july 2003 - set message timestamp as collector time
    //
    if ( !pTdmfDb->mpRecAlert->FtdNew(  cszMsg,
                                        iHostId,
                                        grpnbr,
                                        alertData->sDeviceId,
                                        time(0),
                                        pszType,
                                        (short)alertData->cSeverity ) ) 
    {
        _ASSERT(0);
    }
}

void dbserver_db_add_perfdata_request( FsTdmfDb* pTdmfDb, mmp_TdmfPerfData *perfHdr, ftd_perf_instance_t *perfData )
{
    int iTimeStamp = time(0);
    int iPrevLgnum = -1;//minimize DB access

    //support for emulated TDMF agents:
    //emulated agents dreporet their 'real' agent HostID with a hidden flag at the end of the szServerUID

    int iHostId = strtoul(perfHdr->szServerUID, NULL, 16);
    
    //emulated host id value must be between 0x0000 and 0x0fff 
    if ( 0x00001000 > (unsigned)iHostId )
    {   //extract host id of origin TDMF agent
        char* pszHiddenInfo = &perfHdr->szServerUID[ sizeof(perfHdr->szServerUID)-(8+3+1) ];
        if ( *(pszHiddenInfo+0) == 'O' && 
             *(pszHiddenInfo+1) == 'H' &&
             *(pszHiddenInfo+2) == '=' )
        {
            //use the origin server host id.  all database access (LG groups) below will now work.
            iHostId = strtoul(pszHiddenInfo+3, NULL, 16);
        }
        else
        {   //??? from host id value, this should have been an 'emulated' host
            _ASSERT(0);
        }
    }


    if ( pTdmfDb->mpRecSrvInf->FtdPos(iHostId) )//or ServerName or SrvId ??
    {
        //a ServerInfo record is now selected
        int iNbrPerfData = perfHdr->iPerfDataSize / sizeof(ftd_perf_instance_t);
        for( int i = 0; i < iNbrPerfData; i++, perfData++ )
        {
            bool bGroupStats = false;

            if ( perfData->lgnum == -100 )
            {   //this is a cumulative Logical Group stats structure
                bGroupStats = true;
                perfData->lgnum = perfData->devid;//reference: ftd_statd.c 
                //borrow some of the following device stats (there has to be at least one) information
                //to ensure a valid positioning on a mpRecPair record
                perfData->devid = (perfData + 1)->devid;//get a valid devid
            }

            if ( iPrevLgnum != perfData->lgnum )//minimize access to DB 
            {   //position on the logical group record
                if ( !pTdmfDb->mpRecGrp->FtdPos(perfData->lgnum) )
                {   //no record found for this LG ?????
                    TraceWrn("dbserver_db_add_perfdata, no t_LG record found for lgnum=%hd, hostID=%08x\n",perfData->lgnum,iHostId);
                    continue;
                }
            }
            iPrevLgnum = perfData->lgnum;
            //update some Replication Group values in t_Logical_Group table
            pTdmfDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldConnection, perfData->connection );
            pTdmfDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldStateTimeStamp, CTime(iTimeStamp).Format("%Y-%m-%d %H:%M:%S") );


            /*
            //position on the repl. pair record
            if ( !pTdmfDb->mpRecPair->FtdPos(perfData->devid) )
            {   //no record found for this rep.pair ?????
                TraceWrn("dbserver_db_add_perfdata, no t_RecPair record found for devid=%d of lgnum=%hd, hostID=%08x\n",
                    perfData->devid,perfData->lgnum,iHostId));
                continue;
            }
            */

            //write to t_Perf table.
            //Server,Logical Group and RecPair db cursor are now preset.
            //Create new record in t_Performance table
            char szInstName[MAX_SIZEOF_INSTANCE_NAME];
            wcstombs( szInstName, perfData->wcszInstanceName, MAX_SIZEOF_INSTANCE_NAME );
            if ( szInstName[0] == 0 )
            {   //avoid empty string
                szInstName[0] = ' ';
                szInstName[1] = 0;
            }
            char role = perfData->role;
            if ( role != 'p' && role != ' ' )
                role = '?';

            if( !pTdmfDb->mpRecPerf->FtdNew( iTimeStamp, 
                                              perfData->actual,
                                              perfData->effective,
                                              perfData->bytesread,
                                              perfData->byteswritten,
                                              role,
                                              perfData->connection,
                                              perfData->drvmode,
                                              perfData->lgnum,
                                              perfData->insert,
                                              //device id = -1 indicates Group Stats, otherwise Device stats
                                              bGroupStats ? -1 : perfData->devid,
                                              perfData->rsyncoff,
                                              perfData->rsyncdelta,
                                              perfData->entries,
                                              perfData->sectors,
                                              perfData->pctdone,
                                              perfData->pctbab,
                                              szInstName 
                                              ) ) 
            {   //error
                CString lszMsg;
                lszMsg.Format (
                    "dbserver_db_add_perfdata(): "
                    "Error saving new perf record, hostid=%x, group=%hd\n<%s>",
                    iHostId, perfData->lgnum, pTdmfDb->FtdGetErrMsg()
                );
                TraceWrn((LPCTSTR)lszMsg);
                pTdmfDb->FtdErrReset();
            }

        }//for
    }
    else
    {
        TraceWrn("dbserver_db_add_perfdata, no t_ServerInfo record found for hostid=<%s>,0x%x\n",perfHdr->szServerUID,iHostId);    
    }
}

void    dbserver_db_update_NVP_PerfCfg_request(FsTdmfDb* pTdmfDb, const mmp_TdmfPerfConfig * perfCfg)
{
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp(TDMF_NVP_NAME_PERF_UPLD_PERIOD, perfCfg->iPerfUploadPeriod) )
    {
        char value[32];
        if ( !pTdmfDb->mpRecNvp->FtdNew(TDMF_NVP_NAME_PERF_UPLD_PERIOD, CString(itoa(perfCfg->iPerfUploadPeriod,value,10)) ) )
        {
            CString lszMsg;
            lszMsg.Format (
                "dbserver_db_update_NVP_PerfCfg(): "
                "Cant access <%s>\n<%s>",
                 TDMF_NVP_NAME_PERF_UPLD_PERIOD,
                 pTdmfDb->FtdGetErrMsg()
            );
            TraceWrn("Error while adding a new NVP record in db! %s \n");
            pTdmfDb->FtdErrReset();
            return;
        } 
    }
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp(TDMF_NVP_NAME_REPLGRP_MONIT_UPLD_PERIOD, perfCfg->iReplGrpMonitPeriod) )
    {
        char value[32];
        if ( !pTdmfDb->mpRecNvp->FtdNew(TDMF_NVP_NAME_REPLGRP_MONIT_UPLD_PERIOD, CString(itoa(perfCfg->iReplGrpMonitPeriod,value,10)) ) )
        {
            CString lszMsg;
            lszMsg.Format (
                "dbserver_db_update_NVP_PerfCfg(): "
                "Cant access <%s>\n<%s>",
                 TDMF_NVP_NAME_REPLGRP_MONIT_UPLD_PERIOD,
                 pTdmfDb->FtdGetErrMsg()
            );
            TraceWrn("Error while adding a new NVP record in db! %s \n");
            pTdmfDb->FtdErrReset();
            return;
        } 
    }
}

int dbserver_db_write_sysparams_request(FsTdmfDb* pTdmfDb, const mmp_TdmfCollectorParams *sysparams, CollectorConfig  *cfg)
{
    CString tmp;
    char value[128];

#pragma message (__LOC__ "REMOVED PORT UPDATE FROM WRITE_SYSPARAMS_REQUEST")

    //append 'minutes' to the number
    tmp  = itoa(sysparams->iDBCheckPeriodMinutes,value,10);
    tmp += CString(" minutes");
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_DB_CHECK_PERIOD, tmp ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_CHECK_PERIOD, tmp ) )
        {
            return -1;
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Period at which tables restrictions are checked and enforced. Value can be specified with <minutes> or <hours> units. Default value = 60 minutes .") );
    }

    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_DB_PERF_TBL_MAX_NUMBER,sysparams->iDBPerformanceTableMaxNbr ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_DB_PERF_TBL_MAX_NUMBER),CString(itoa(sysparams->iDBPerformanceTableMaxNbr,value,10)) ) )
        {
            return -1;
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Limit the number of records in the DB t_Performance table to this value. A value of 0 (zero) disables this restriction.") );
    }

    //append 'days' to the number
    tmp  = itoa(sysparams->iDBPerformanceTableMaxDays,value,10);
    tmp += CString(" days");
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_DB_PERF_TBL_MAX_DAYS, tmp ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_DB_PERF_TBL_MAX_DAYS),tmp ) )
        {
            return -1;
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Limit the number of records in the DB t_Performance table based on record time_stamp. An empty field disables this resctriction.") );
    }

    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_NUMBER,sysparams->iDBAlertStatusTableMaxNbr ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_NUMBER),CString(itoa(sysparams->iDBAlertStatusTableMaxNbr,value,10)) ) )
        {
            return -1;
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Limit the number of records in the DB t_Alert_Status table to this value. A value of 0 (zero) disables this restriction.") );
    }

    //append 'days' to the number
    tmp  = itoa(sysparams->iDBAlertStatusTableMaxDays,value,10);
    tmp += CString(" days");
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_DAYS,tmp ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_DAYS),tmp ) )
        {
            return -1;
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Limit the number of records in the DB t_Alert_Status table based on record time_stamp. An empty field disables this resctriction.") );
    }

    return 0;
}

static void dbserver_db_write_version_request(FsTdmfDb* pTdmfDb)
{
    CString cszVersion, cszBuild;

    //get value from resource
    char szFileName[_MAX_PATH];
    GetModuleFileName(NULL, szFileName, _MAX_PATH);
    DWORD dw;
    DWORD size = GetFileVersionInfoSize(szFileName, &dw);
    char* pVer = new char[size];
    if( GetFileVersionInfo(szFileName, 0, size, pVer) )
    {
        LPVOID pBuff;
        UINT cbBuff;
        //assuming English as default res language ...
        VerQueryValue(pVer, "\\StringFileInfo\\040904B0\\ProductVersion", &pBuff, &cbBuff); 
        cszVersion = (LPTSTR)pBuff;

		VerQueryValue(pVer, "\\StringFileInfo\\040904B0\\SpecialBuild", &pBuff, &cbBuff); 
        cszBuild = (LPTSTR)pBuff;
    }
    delete [] pVer;


	cszVersion += _T(" Build ");
	cszVersion += cszBuild;

    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_COLLECTOR_VER, cszVersion ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_COLLECTOR_VER, cszVersion ) )
        {
            return ;//error
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Collector File Version.") );
    }

    //MMP Protocol version : MMP_PROTOCOL_VERSION from libmngtmsg.h.
    cszVersion.Format("%d",MMP_PROTOCOL_VERSION);
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_COLLECTOR_MMP_VER, cszVersion ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_COLLECTOR_MMP_VER, cszVersion ) )
        {
            return ;//error
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("MMP protocol version used by Collector.  MMP version must be matched with peers - Replication Servers.") );
    }

    //Gui Collector Protocol version : MMP_GUI_COLLECTOR_PROTOCOL_VERSION from libmngtmsg.h.
    cszVersion.Format("%d", GUI_COLLECTOR_PROTOCOL_VERSION);
    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_GUI_COLLECTOR_PROTOCOL_VER, cszVersion ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_GUI_COLLECTOR_PROTOCOL_VER, cszVersion ) )
        {
            return ;//error
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("GUI Collector protocol version used by Collector.") );
    }

	//Also write Collector's Hostid
	char szHostId[32];
    cszVersion = ftd_mngt_getServerId(szHostId);

    if ( !pTdmfDb->mpRecNvp->FtdUpdNvp( TDMF_NVP_NAME_COLLECTOR_HOST_ID, cszVersion ) )
    {   //update NVP value in db
        if ( !pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_COLLECTOR_HOST_ID, cszVersion ) )
        {
            return ;//error
        }
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Collector HostId.") );
    }
}

void dbserver_db_read_sysparams_collector_ip_port_request(FsTdmfDb* pTdmfDb, CollectorConfig  *cfg, HANDLE hEvent)
{
    if ( pTdmfDb->mpRecNvp->FtdPos( TDMF_NVP_NAME_COLLECTOR_PORT ) )
    {
        cfg->iPort = atoi( pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( cfg->iPort == 0 )
            cfg->iPort = TDMF_COLLECTOR_DEF_PORT; 
    }
    else
    {
        cfg->iPort = TDMF_COLLECTOR_DEF_PORT; 
        //add this NVP to DB
        char port[32];
        pTdmfDb->mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_COLLECTOR_PORT) , CString(itoa(cfg->iPort,port,10 )) );
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("TCP listener Port number opened by Collector.") );
    }

    if (hEvent)
    {
        SetEvent(hEvent);
    }
}

void dbserver_db_read_sysparams_request(FsTdmfDb*                 pTdmfDb,
                                        mmp_TdmfCollectorParams * dbparams,
                                        CollectorConfig  *        cfg,
                                        int*                      pRetVal,
                                        HANDLE                    hEvent)
{
    dbserver_db_read_sysparams_collector_ip_port_request(pTdmfDb, cfg, NULL);
    dbparams->iCollectorTcpPort          = cfg->iPort;
    dbparams->iDBCheckPeriodMinutes      = dbserver_db_read_sysparams_db_check_period(pTdmfDb);
    dbparams->iDBPerformanceTableMaxNbr  = dbserver_db_read_sysparams_db_perf_tbl_max_nbr(pTdmfDb);
    dbparams->iDBPerformanceTableMaxDays = dbserver_db_read_sysparams_db_perf_tbl_max_days(pTdmfDb);
    dbparams->iDBAlertStatusTableMaxNbr  = dbserver_db_read_sysparams_db_alertstatus_tbl_max_nbr(pTdmfDb);
    dbparams->iDBAlertStatusTableMaxDays = dbserver_db_read_sysparams_db_alertstatus_tbl_max_days(pTdmfDb);
    dbparams->bValidKey                  = g_bKeyValid;

    *pRetVal = 0;

    SetEvent(hEvent);
}

void dbserver_db_CheckRegistrationKey_request(FsTdmfDb* pTdmfDb,
                                              bool*     pRetVal,
                                              HANDLE    hEvent)
{
    if (pTdmfDb->mpRecNvp->FtdPos(TDMF_NVP_NAME_REGISGTRATION_KEY))
    {
        CString cstrRegKey = pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal);
        
        char* rgszKeys[2];
        rgszKeys[0] = (char*)(LPCTSTR)cstrRegKey;
        rgszKeys[1] = NULL;
        
        if (check_key(rgszKeys, np_crypt_key_pmd, cstrRegKey.GetLength()) == LICKEY_OK)
        {
            if (pRetVal != NULL)
            {
                *pRetVal = true;
            }
            if (hEvent != NULL)
            {
                SetEvent(hEvent);
            }
            return;
        }
    }
    
    if (pRetVal != NULL)
    {
        *pRetVal = false;
    }

    // Log invalid key (but don't reprint it every time)
    static bool bLogged = false;
    if (!bLogged)
    {
        TraceCrit("Invalid collector registration key.");
        bLogged = true;
    }

    if (hEvent != NULL)
    {
        SetEvent(hEvent);
    }
}

void dbserver_db_SetCollectorRegistrationKey_request(FsTdmfDb* pTdmfDb, char* szKey)
{
    // Check key validity
    char* rgszKeys[2];
    rgszKeys[0] = szKey;
    rgszKeys[1] = NULL;
	license_data_t licenseData;

    if (check_key_r(rgszKeys, np_crypt_key_pmd, strlen(szKey), &licenseData) == LICKEY_OK)
    {
        g_bKeyValid = true;
    }
    else
    {
        // Log invalid key
        TraceCrit("Invalid collector registration key.");
    }

    // Log key change
    mmp_TdmfRegistrationKey data;
    strcpy(data.szServerUID, "Collector");
    strcpy(data.szRegKey, szKey);

	time_t exp_date = licenseData.expiration_date << 16;
	/* Force expiration hour to 13:00:00 */
	struct tm expires;
	expires = *localtime( &exp_date );  
	expires.tm_sec = 00;
	expires.tm_min = 00;
	expires.tm_hour = 13;
	exp_date = mktime(&expires);

    data.iKeyExpirationTime = exp_date;

    LogKeyChange(pTdmfDb, &data);

    // Save new key
    if (!pTdmfDb->mpRecNvp->FtdUpdNvp(TDMF_NVP_NAME_REGISGTRATION_KEY, szKey))
    {
        if (!pTdmfDb->mpRecNvp->FtdNew(TDMF_NVP_NAME_REGISGTRATION_KEY, szKey))
        {
            CString lszMsg;
            lszMsg.Format("dbserver_db_update_NVP_PerfCfg(): Cant access <%s>\n<%s>",
                           TDMF_NVP_NAME_REGISGTRATION_KEY,
                           pTdmfDb->FtdGetErrMsg());
            pTdmfDb->FtdErrReset();
        }
    }
}

//
// SVG 16 july 2003 PATCH for optimizing collector
//
static unsigned int     suiSkipRate             = 0;
static unsigned int     suiSkipedMsgs           = 0;
static unsigned int     suiNonSkipedMsgs        = 0;
static unsigned int     suiOptimizationActive   = 0;
static time_t           sMsgsTimeStamp          = 0;
#if DBG
static char DbgMsg[500];
#endif

//////////////////////////////////////////////////////////////////////////////
// The working thread (serializes the calls to the bd)

static UINT __stdcall dbserver_db_working_thread(void* context)
{
    MSG             msg;
    FsTdmfDb*       pTdmfDb             = NULL;
    bool            bSkip               = false;
    time_t          LastTimeCln         = time(0);
    time_t          CurrentTime         = time(0);
    unsigned int    uiThreadWorkPeriod  = HourElapsed; // by default 1 hour
    try
    {       
        //////////////////////////////////////////////
        // Force the creation of the thread msg queue
        PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
        
        ///////////////////////////////////////
        //create permanent link with TDMF DB.
        CString cszUser,cszPW,cszDBServer;
        char    tmp[128];
        if ( cfg_get_software_key_value("TDMFdbUser", tmp, CFG_IS_STRINGVAL) == CFG_OK )
            cszUser = tmp;
        else
            cszUser = "DtcCollector";
        if ( cfg_get_software_key_value("TDMFdbPW", tmp, CFG_IS_STRINGVAL) == CFG_OK )
            cszPW = tmp;
        else
            cszPW = "Dtc2003";
        if ( cfg_get_software_key_value("DtcDbServer", tmp, CFG_IS_STRINGVAL) == CFG_OK )
            cszDBServer = tmp;
        else
            cszDBServer = "DtcDBServer";
        
        pTdmfDb = new FsTdmfDb( cszUser, cszPW , cszDBServer, "DtcDb" );
        if (!pTdmfDb->FtdIsOpen())
        {
            TraceWrn("**** Unable to open database using: User=%s , PW=%s , DBServer=%s \n",(LPCTSTR)cszUser,(LPCTSTR)cszPW,(LPCTSTR)cszDBServer);
            delete pTdmfDb;
            pTdmfDb = 0;
            SetEvent(g_hWorkingThreadReady);
            return 1;//error
        }

        //
        //Start the timer responsible to maintain the TDMF database in good shape
        //Among other things, it limit the number of records in Perf and AlertStatus tables
        //as configured.
        //
        int period;
        if ( (period = dbserver_db_read_sysparams_db_check_period(pTdmfDb)) > 0 )
        {
            uiThreadWorkPeriod = (unsigned int)period * 60;
        }
    }
    catch(...)
    {
        SetEvent(g_hWorkingThreadReady);
        return 1;
    }

    /////////////////////////////////
    // We're now ready to receive request
    SetEvent(g_hWorkingThreadReady);

    //////////////////////////////////////////
    //
    if (SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_HIGHEST ))
    {
#if DBG
        OutputDebugString("Set DB Thread priority to THREAD_PRIORITY_HIGHEST (+2)\n");
#endif
    }
    else
    {
#if DBG
        OutputDebugString("Set DB Thread priority to THREAD_PRIORITY_HIGHEST failed!\n");
#endif
    }

    //
    // Initialize random stuff
    //
    srand( (unsigned)time( NULL ) );

    //////////////////////////////////////////
    // process requests
    while (GetMessage(&msg, NULL, 0, 0))
    {
        //
        // Patch to make sure cleanup runs at specified intervals
        //
        CurrentTime = time(0);
        if ( (unsigned int)(CurrentTime - LastTimeCln) > uiThreadWorkPeriod )
        {
            uiThreadWorkPeriod = dbserver_db_cleanup( pTdmfDb );
            LastTimeCln = time(0);
        }

        if ((msg.message >= TM_FIRST) &&  (msg.message < TM_LAST))
        {
            DEC_MSG_WAITING;
        }

        switch(msg.message)
        {
        case TM_INIT_ALLAGENT:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_initialize_AllAgent_request(pTdmfDb, (HANDLE)msg.wParam, (int*)msg.lParam);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_INIT_ALLAGENT);
                *((int*)msg.lParam) = 1;
                SetEvent((HANDLE)msg.wParam);
            }
            break;

        case TM_STOP_THREAD:
            // All msg received after this one will be lost
            try
            {
                delete pTdmfDb;
                SetEvent((HANDLE)msg.wParam);
                _endthread();
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_STOP_THREAD);
            }
            break;

        case TM_DB_UPDATE_KEY:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_update_key_request(pTdmfDb, (mmp_TdmfRegistrationKey*)msg.wParam);
                delete (mmp_TdmfRegistrationKey*)msg.wParam;
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_DB_UPDATE_KEY);
            }
            break;

        case TM_UPDATE_SERVERINFO:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_update_serverInfo_request(pTdmfDb, (mmp_TdmfServerInfo*)msg.wParam);
                delete (mmp_TdmfServerInfo*)msg.wParam;
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_UPDATE_SERVERINFO);
            }
            break;

        case TM_UPDATE_SERVER_STATE:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_update_server_state_request(pTdmfDb, (mmp_TdmfAgentState*)msg.wParam, msg.lParam);
                delete (mmp_TdmfAgentState*)msg.wParam;
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_UPDATE_SERVER_STATE);
            }
            break;

        case TM_UPDATE_SERVER_STATE_WITH_HOSTID:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_update_server_state_request(pTdmfDb, msg.wParam, (enum tdmf_agent_state)msg.lParam);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_UPDATE_SERVER_STATE_WITH_HOSTID);
            }
            break;

        case TM_ADD_ALERT:
            try
            {
                bSkip = false;

                //
                // SVG 16 july 2003
                //
                //
                // If we are more than 500 messages back in our queue, say bye bye to info messages!
                //
                if (g_nNbMsgWaiting > 500)
                {
                    mmp_TdmfAlertHdr * MyAlert = (mmp_TdmfAlertHdr *)msg.wParam;
                    char *              cSkippedMsgChar     = (char *)msg.lParam;

                    if ((MyAlert) && (MyAlert->cSeverity > 3))
                    {
                        //
                        // Skip this message
                        //
                        bSkip = true;
                    }
                }


                if (!bSkip)
                {
                    g_nNbMsgSent++;
                    dbserver_db_add_alert_request(pTdmfDb, (const mmp_TdmfAlertHdr*)msg.wParam, (const char*)msg.lParam);
                }
                delete (mmp_TdmfAlertHdr*)msg.wParam;
                free((void*)msg.lParam);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_ADD_ALERT);
            }
            break;

        case TM_ADD_PERFDATA:
            try
            {
                                bSkip       = false;
                unsigned int    uiSkipRate  = 0;

                //
                // If we optimized any messages, dump the stats now
                //
                if (suiOptimizationActive)
                {
                    if (!sMsgsTimeStamp)
                    {
                        sMsgsTimeStamp = time(0);
                    }

                    //
                    // If one hour elapsed, dump stats to event viewer
                    //
                    if ( (time(0) - sMsgsTimeStamp) > HourElapsed )
                    {
                        //
                        // Dump to event viewer the following:
                        //
                        // %of messages dumped in last hour
                        //
                        if (suiSkipedMsgs)
                        {
                            sprintf(EventViewerMsg,
                                    "Collector Optimized Messaging removed %d messages out of %d (%d percent) in last hour",
                                    suiSkipedMsgs, 
                                    (suiSkipedMsgs+suiNonSkipedMsgs),
                                    (100*suiSkipedMsgs/(suiSkipedMsgs+suiNonSkipedMsgs)) );
                            error_syslog_DirectAccess((LPSTR)g_ResourceManager.GetFullProductName(),EventViewerMsg);
#if DBG
                            OutputDebugString(EventViewerMsg);
#endif
                        }

                        sMsgsTimeStamp = time(0);
                        suiSkipedMsgs = 0;
                        suiNonSkipedMsgs = 0;
                    }

                }

                //
                // SVG Special optimize 
                //
                if (g_nNbMsgWaiting > 1000)
                {
                    //
                    // How many msgs are we back?
                    //
                    uiSkipRate = (int)(g_nNbMsgWaiting/1000);

                    suiOptimizationActive = 2;

                    //
                    // If there was a rate change!
                    //
                    if ((uiSkipRate*20) != suiSkipRate)
                    {
#if DBG
                        if (suiSkipedMsgs)
                        {
                            //
                            // Print out the following info
                            //
                            // [number of messages currently pending]
                            // Msgs Rate change from
                            // [current rate of removal]
                            // to
                            // [new rate of removal]
                            // -
                            // [previous msgs removed]
                            //  /
                            // [previous msgs]
                            // 
                            // [Previous exact% of removed messages]
                            // 
                            sprintf(DbgMsg,
                                    "[%04d] Msgs Rate change from [%d] to [%d] - [%d]/[%d] [%d%%]\n",
                                    g_nNbMsgWaiting,
                                    suiSkipRate,
                                    (uiSkipRate*20),
                                    suiSkipedMsgs,
                                    (suiSkipedMsgs + suiNonSkipedMsgs),
                                    ((100*suiSkipedMsgs/(suiSkipedMsgs+suiNonSkipedMsgs))));
                        }
                        else
                        {
                            //
                            // Print out the following info
                            //
                            // [number of messages currently pending]
                            // Msgs Rate change from
                            // [current rate of removal]
                            // to
                            // [new rate of removal]
                            //
                            sprintf(DbgMsg,
                                    "[%04d] Msgs Rate change from [%d] to [%d]\n",
                                    g_nNbMsgWaiting,
                                    suiSkipRate,
                                    (uiSkipRate*20));
                        }

                        OutputDebugString(DbgMsg);
#endif

                        suiSkipRate = uiSkipRate * 20;
                    }

                }
                else 
                {
                    if (suiOptimizationActive)
                    {
                        suiOptimizationActive--;
                    }

                    //
                    // If there is less than 1000 messages pending, and there is a current
                    // skiprate, we have to change it back to 0!
                    //
                    if (suiSkipRate)
                    {

#if DBG
                        if (suiSkipedMsgs)
                        {
                            //
                            // Print out the following info
                            //
                            // [number of messages currently pending]
                            // Msgs Rate change from
                            // [current rate of removal]
                            // to
                            // [new rate of removal]
                            // -
                            // [previous msgs removed]
                            //  /
                            // [previous msgs]
                            // 
                            // [Previous exact% of removed messages]
                            // 
                            sprintf(DbgMsg,
                                    "[%04d] Msgs Rate change from [%d] to [0] - [%d]/[%d] [%d%%]\n",
                                    g_nNbMsgWaiting,
                                    suiSkipRate,
                                    suiSkipedMsgs,
                                    (suiSkipedMsgs + suiNonSkipedMsgs),
                                    ((100*suiSkipedMsgs/(suiSkipedMsgs+suiNonSkipedMsgs))));

                            OutputDebugString(DbgMsg);
                        }
#endif

                        suiSkipRate = 0;
                    }
                }

                if (suiSkipRate)
                {
                    //
                    // We are doing some special stuff here so we don't skip allways in the same order...
                    //
                    if ((1 + (unsigned int)(100.0 * rand()/(RAND_MAX+1.0))) <= suiSkipRate)
                    {
                        bSkip = true;
                        suiSkipedMsgs++;
                    }
                    else
                    {
                        suiNonSkipedMsgs++;
                    }
                }

                if (!bSkip)
                {
                    g_nNbMsgSent++;
                    dbserver_db_add_perfdata_request(pTdmfDb, (mmp_TdmfPerfData*)msg.wParam, (ftd_perf_instance_t*)msg.lParam);
                }
                delete (mmp_TdmfPerfData*)msg.wParam;
                delete [] (ftd_perf_instance_t*)msg.lParam;
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_ADD_PERFDATA);
            }
            break;

        case TM_UPDATE_NVP_PERFCFG:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_update_NVP_PerfCfg_request(pTdmfDb, (const mmp_TdmfPerfConfig*)msg.wParam);
                delete (mmp_TdmfPerfConfig*)msg.wParam;
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_UPDATE_NVP_PERFCFG);
            }
            break;

        case TM_WRITE_SYSPARAMS:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_write_sysparams_request(pTdmfDb, (const mmp_TdmfCollectorParams*)msg.wParam, (CollectorConfig*)msg.lParam);
                delete (mmp_TdmfCollectorParams*)msg.wParam;
                delete (CollectorConfig*)msg.lParam;
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_WRITE_SYSPARAMS);
            }
            break;

        case TM_WRITE_VERSION:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_write_version_request(pTdmfDb);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_WRITE_VERSION);
            }
            break;

        case TM_READ_SYSPARAMS_COLLECTOR_IP_PORT:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_read_sysparams_collector_ip_port_request(pTdmfDb, (CollectorConfig*)msg.wParam, (HANDLE)msg.lParam);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_READ_SYSPARAMS_COLLECTOR_IP_PORT);
                SetEvent((HANDLE)msg.lParam);
            }
            break;

        case TM_DBSERVER_DB_READ_SYSPARAMS:
            try
            {
                struct sysparams_params* pParams = (struct sysparams_params*)msg.wParam;
                g_nNbMsgSent++;
                dbserver_db_read_sysparams_request(pTdmfDb, pParams->dbparams, pParams->cfg, pParams->pnRetVal, pParams->hEvent);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_READ_SYSPARAMS_COLLECTOR_IP_PORT);
                SetEvent((HANDLE)msg.lParam);
            }
            break;

        case TM_CHECK_REGISTRATION_KEY:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_CheckRegistrationKey_request(pTdmfDb, (bool*)msg.wParam, (HANDLE)msg.lParam);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_CHECK_REGISTRATION_KEY);
                SetEvent((HANDLE)msg.lParam);
            }
            break;

        case TM_WRITE_COLLECTOR_KEY:
            try
            {
                g_nNbMsgSent++;
                dbserver_db_SetCollectorRegistrationKey_request(pTdmfDb, (char*)msg.wParam);
            }
            catch(...)
            {
                TraceErr("Exception catched while processing message: %d\n", TM_WRITE_COLLECTOR_KEY);
            }
            break;

        case WM_TIMER:
            ((TIMERPROC)msg.lParam)(NULL, WM_TIMER, msg.wParam, (DWORD)pTdmfDb);
            break;

        }
    }

    return 0;
}

////////////////////////////////////////
int dbserver_db_initialize(CollectorConfig* cfg)
{
	
	


    g_hWorkingThreadReady = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (_beginthreadex(NULL, 0, dbserver_db_working_thread, NULL, 0, &g_nThreadID) > 0)
    {
        WaitForSingleObject(g_hWorkingThreadReady, INFINITE);
        CloseHandle(g_hWorkingThreadReady);

        dbserver_db_initialize_AllAgent();
        
        //TDMF Collector main listener socket port number. cfg->iPort will be set
        dbserver_db_read_sysparams_collector_ip_port(cfg);

		cfg->ulIP =  Get_Selected_DtcIP_from_Install();
		if (cfg->ulIP == 0)
			{TraceCrit("Collector registry is missing: DtcIP , no hostid is found. \n");}
		/*
	    if ( cfg_get_software_key_value("DtcCollectorIP", tmp, CFG_IS_STRINGVAL) == CFG_OK )
		    {
	        ipstring_to_ip(tmp,(unsigned long*)&lcfgIP);
			cfg->ulIP = lcfgIP;
	    	}
	    else 
	    	{
	        unsigned long   *localIPlist,count;

			count  = getnetconfcount();
	        localIPlist = new unsigned long[count];
			//get list of IP address
	        getnetconfs(localIPlist);

	        cfg->ulIP    = localIPlist[0]; //TDMF Collector IP addr.
	        delete [] localIPlist;
			}
		*/
		
        
        //save this exe version number to System Parameters table 
        dbserver_db_write_version();

        return 1;//ok
    }

    return 0;
}

void dbserver_db_close()
{
    dbserver_db_stop_working_thread();
}

#define FATAL_PREFIX_STR    "[FATAL /"
#define INFO_PREFIX_STR     "[INFO /"
#define WARNING_PREFIX_STR  "[WARNING /"
void    dbserver_db_add_status(mmp_TdmfStatusMsg *statusData, char* szMessage)
{
    //avoid adding to DB messages with no significative text (only \n or spaces, ...), or message 
    if ( localIsMessageWithoutText(szMessage) )
        return;

    TraceWrn("dbserver_db_add_status, write status msg info to db, msg=%s\n",szMessage == NULL ? "" : szMessage );

    mmp_TdmfAlertHdr alertData;
    alertData.cSeverity     = 5;//1=highest, 5=lowest

    if ( strstr(szMessage,          INFO_PREFIX_STR) ) 
    {
        alertData.cType = MMP_MNGT_MONITOR_TYPE_INFORMATION;
        alertData.cSeverity     = 3;
    } 
    else if ( strstr(szMessage,   WARNING_PREFIX_STR) ) 
    {
        alertData.cType = MMP_MNGT_MONITOR_TYPE_WARNING;
        alertData.cSeverity     = 2;
    } 
    else if ( strstr(szMessage,   FATAL_PREFIX_STR) ) 
    {
        alertData.cType = MMP_MNGT_MONITOR_TYPE_ERROR;
        alertData.cSeverity     = 1;
    } 
    //
    // Catch TRACEERR messages, and let them trough!
    //
    else if (strstr (szMessage, "ERR:"))
    {
        alertData.cSeverity      = 3;//
    }
    else 
    {
        alertData.cType = MMP_MNGT_MONITOR_TYPE_INFORMATION;
    }

    alertData.sDeviceId     = -1;//irrelevant for a status msg
    alertData.sLGid         = -1;
    strcpy( alertData.szServerUID, statusData->szServerUID );
    alertData.uiTimeStamp   = statusData->iTimeStamp;

    if ( strlen(szMessage) > 512 )
    {   //512 = max size of smszFldTxt column of t_Alert_Status table
        szMessage[511] = 0;//truncate message
    }

    dbserver_db_add_alert(&alertData, szMessage );
}

//retreive Agent IP and Port from its HostID
void    dbserver_db_get_agent_ip_port(const char* szAgentUID, unsigned long *agentIP, unsigned int *agentPort)
{
    CServerInfo   info;
    AgentUIDtoIP::iterator  it;

    *agentIP    = 0;
    *agentPort  = 0;

    info.iHostId = strtoul(szAgentUID,0,16);_ASSERT(info.iHostId != 0);

    it = gAllTDMFAgent.find(info.iHostId);
    if ( it != gAllTDMFAgent.end() )
    {   //server found in list
        info        = ((*it).second);
        *agentIP    = info.iIP;
        *agentPort  = info.iPort;
    }
    else
    {
        ASSERT(0);  
    }
}

//retreive Host ID from ServerName (machine name)
bool    dbserver_db_get_agent_hostid(const char* szServerName, char* szHostId)
{
    CServerInfo             info;
    AgentUIDtoIP::iterator  it;

    it = gAllTDMFAgent.begin();
    while ( it != gAllTDMFAgent.end() )
    {   //server found in list
        info = ((*it).second);
        if (info.cszServerName == szServerName)
        {
            sprintf(szHostId,"%08x",info.iHostId);
            return true;
        }
        it++;
    }

    ASSERT(0);  

    return false;
}

//retreive Host ID from its IP address 
bool    dbserver_db_get_agent_hostid(int iServerIP, char* szHostId)
{
    CServerInfo             info;
    AgentUIDtoIP::iterator  it;

    it = gAllTDMFAgent.begin();
    while ( it != gAllTDMFAgent.end() )
    {   //server found in list
        info = ((*it).second);
        if (info.iIP == iServerIP)      // WR32867- won't work with router IP translation, so this function can not be used...
        {
            sprintf(szHostId,"%08x",info.iHostId);
            return true;
        }
        it++;
    }

    ASSERT(0);  

    return false;
}

// ***********************************************************
// Function name    : dbserver_db_get_list_servers
// Description      : Get the unique Host ID of all the TDMF Agent present in TDMF DB.
// Return type      : void  
// Argument         : std::list<CString> & listServerUID
//                          The content of the provided std::list (STL) object is first cleared and then populated.
// 
// ***********************************************************
void    dbserver_db_get_list_servers( std::list<CString> & listServerUID )
{
    AgentUIDtoIP::iterator  it;
#if 0
    char UIDstring[200];
#endif
    for (it = gAllTDMFAgent.begin(); it != gAllTDMFAgent.end(); it++)
    {
#if 0
        listServerUID.push_back( itoa(it->first,UIDstring,10) );
#else
		// Mike Pollett changed to get VC7 to compile
        listServerUID.push_back( (CString &)(it->first) );
        //listServerUID.push_back( it->first );
#endif
    }
}

// ***********************************************************
void    dbserver_db_update_server_port( const char * szHostID, int iNewPort )
{
    dbserver_db_update_server_port( strtoul(szHostID, NULL, 16) , iNewPort );
}

// ***********************************************************
void    dbserver_db_update_server_port( int iHostID, int iNewPort )
{
    AgentUIDtoIP::iterator  it = gAllTDMFAgent.find(iHostID);
    if ( it != gAllTDMFAgent.end() )
    {   //server found in list
        CServerInfo & info = ((*it).second);
        info.iPort  = iNewPort;
    }
}

// Not multi-thread safe, must be called from the DB worker thread
static int dbserver_db_read_sysparams_db_check_period(FsTdmfDb* pTdmfDb)
{
    int period = 0;
    if ( pTdmfDb->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_CHECK_PERIOD ) )
    {
        CString value = pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal);
        period = atoi(value);//get number
        if ( period > 0 )
        {
            value.MakeLower();
            if ( strstr(value,"hour") )
                period *= 60;//hour to minutes
        }
    }
    else
    {   //add this NVP to DB
        period = 60;
        pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_CHECK_PERIOD , CString("60 minutes") );
        CString desc;
        desc.Format("Period at which tables restrictions are checked and enforced. Value can be specified with <minutes> or <hours> units. Default value = 60 minutes .");
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, desc );
    }
    return period;
}

// Not multi-thread safe, must be called from the DB worker thread
static int dbserver_db_read_sysparams_db_perf_tbl_max_nbr(FsTdmfDb* pTdmfDb)
{
    int maxnbr;
    if ( pTdmfDb->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_PERF_TBL_MAX_NUMBER ) )
    {
        maxnbr = atoi( pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( maxnbr <= 0 )
            maxnbr = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        maxnbr = 2000000;
        pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_PERF_TBL_MAX_NUMBER , CString("2000000") );
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the DB t_Performance table to this value. A value of 0 (zero) disables this restriction." );
    }
    return maxnbr;
}

// Not multi-thread safe, must be called from the DB worker thread
static int dbserver_db_read_sysparams_db_perf_tbl_max_days(FsTdmfDb* pTdmfDb)
{
    int max;
    if ( pTdmfDb->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_PERF_TBL_MAX_DAYS ) )
    {
        max = atoi( pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( max <= 0 )
            max = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        max = 7;//default : 7 days
        pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_PERF_TBL_MAX_DAYS , CString("7 day") );
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the DB t_Performance table based on record time_stamp. An empty field disables this resctriction." );
    }
    return max;
}

// Not multi-thread safe, must be called from the DB worker thread
static int dbserver_db_read_sysparams_db_alertstatus_tbl_max_nbr(FsTdmfDb* pTdmfDb)
{
    int maxnbr;
    if ( pTdmfDb->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_NUMBER ) )
    {
        maxnbr = atoi( pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( maxnbr <= 0 )
            maxnbr = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        maxnbr = 1000000;
        pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_NUMBER , CString("1000000") );
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the DB t_Alert_Status table to this value. A value of 0 (zero) disables this restriction." );
    }
    return maxnbr;
}

// Not multi-thread safe, must be called from the DB worker thread
static int dbserver_db_read_sysparams_db_alertstatus_tbl_max_days(FsTdmfDb* pTdmfDb)
{
    int max;
    if ( pTdmfDb->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_DAYS ) )
    {
        max = atoi( pTdmfDb->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( max <= 0 )
            max = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        max = 7;//default : 7 days
        pTdmfDb->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_DAYS , CString("7 day") );
        pTdmfDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the DB t_Alert_Status table based on record time_stamp. An empty field disables this resctriction." );
    }
    return max;
}

// ***********************************************************
// ***********************************************************
static bool localIsMessageWithoutText(const char* szMessage)
{
    register const char *p = szMessage;
    register int iNonPrintableChars = 0;
    while( *p != 0 )
    {
        if ( isprint(*p) == 0 || *p == ' ' )//consider spaces as non-significative, non-text .
            iNonPrintableChars++;
        p++;
    }

    //return true if half (or more) the message is made of non printable characters
    return iNonPrintableChars >= ((p - szMessage)>>1);
}

// ***********************************************************
short localIsGroupRelatedMessage(const char *szMessage)
{
    char *p;
    short grp_nbr = -1;
    //look for [... :PMD_xyz] or [... : RMD_xyz] or [... : RMDA_xyz] 
    if ( p=strstr(szMessage,"PMD_") )
    {
        grp_nbr = atoi(p+4);
    }
    else if ( p=strstr(szMessage,"RMD_") )
    {
        grp_nbr = atoi(p+4);
    }
    else if ( p=strstr(szMessage,"RMDA_") )
    {
        grp_nbr = atoi(p+5);
    }
    return grp_nbr;
}

// ***********************************************************
// ***********************************************************
AgentIdentification::AgentIdentification( const void *pvAgentID, enum AgentIDType type )
{
    m_IsValid = false;

    if ( type == IP_STRING || type == IP_NUMERIC )
    {
        unsigned long ip;
        if ( type == IP_STRING )
            ipstring_to_ip( (char *)pvAgentID, &ip );
        else 
            ip = *((unsigned long *)pvAgentID);

        AgentUIDtoIP::iterator it = gAllTDMFAgent.begin();
        while ( it != gAllTDMFAgent.end() && !m_IsValid )
        {
            if ( ((*it).second).iIP == (int)ip )
            {
                m_iHostID = ((*it).second).iHostId;
                m_IsValid = true;
            }
            it++;
        }
    }
    else if ( type == HOST_ID_STRING )
    {
        unsigned long ip;
        unsigned int  notused;
        dbserver_db_get_agent_ip_port( (const char *)pvAgentID, &ip, &notused );
        if ( ip != 0 )
        {   //valid host id
            m_iHostID = (int)strtoul((char *)pvAgentID,0,16);
            m_IsValid = true;
        }
    }
    else if ( type == HOST_NAME_STRING )
    {
        AgentUIDtoIP::iterator it = gAllTDMFAgent.begin();
        while ( it != gAllTDMFAgent.end() && !m_IsValid )
        {   
            //CServerInfo info = ((*it).second);
            if ( ((*it).second).cszServerName == (const char *)pvAgentID )
            {
                m_iHostID = ((*it).second).iHostId;
                m_IsValid = true;
            }
            it++;
        }
    }
    else if ( type == HOST_ID_NUMERIC )
    {
        m_iHostID = *((const int *)pvAgentID);
        m_IsValid = true;
    }
}

//
// Dummy string in case we don't find the hostid in our list
//
static char NameNotFound[] = "Name Not Found";

// ***********************************************************
// Function name    : GetNameFromHostID
// Description      : Get the unique Name from Host ID from the global map
// Return type      : pointer to the string containing the name
// Argument         : int iHostID
// ***********************************************************
LPCSTR  dbserver_db_GetNameFromHostID(int iHostID)
{
    AgentUIDtoIP::iterator  it = gAllTDMFAgent.find(iHostID);
    if ( it != gAllTDMFAgent.end() )
    {   //server found in list
        CServerInfo & info = ((*it).second);
        return ((LPCSTR)info.cszServerName);
    }
    else
    {
        return NameNotFound;
    }
}

/////////////////////////////////////////////////////////////////////
// Output debug info
void dbserver_db_output_nb_msg_waiting()
{
#ifdef CHECK_MSG_QUEUE
    char buf[80];
    sprintf(buf,"Nb Msg Waiting = %d\n", g_nNbMsgWaiting);
    //TraceInf(buf);
    MessageBox(0,buf,"PostMsgThread Messages",0);
#else
    TraceInf("Nb Msg Waiting is not computed in this version.\n");
#endif
}
