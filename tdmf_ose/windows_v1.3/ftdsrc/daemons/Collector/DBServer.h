/*
 * DBServer.h -   definitions global to all DBServer source files
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
#ifndef __DBSERVER_H
#define __DBSERVER_H

#pragma warning( disable : 4786 )   //disable annoying STL debug warning msg
#include <list>     //STL
//#include <string>   //STL
#include <afx.h>   

#include "libmngtmsg.h"
#include "../../lib/libResMgr/ResourceManager.h"

//**************************************************************
// DBServer global definitions
//
#define  WIN32_LEAN_AND_MEAN    //avoid including rarely use stuff
#include <windows.h>
#include <crtdbg.h>
#ifdef _DEBUG
#include <stdio.h>
#define DBGPRINT(a)             CollectorTrace a
#define DBGPRINTCOND(exp,a)     if( exp ) DBGPRINT(a)
#else
#define DBGPRINT(a)             CollectorTrace a
#define DBGPRINTCOND(exp,a)     
#endif

extern long     g_nNbMsgWaiting;
extern long     g_nNbThreadsRunning;
extern long     g_nNbMsgSent;
extern long     g_nNbThrdCreated;
extern long     g_nNbAliveMsg;
extern long     g_nNbAliveAgents;
extern bool     g_bKeyValid;

extern  mmp_TdmfCollectorState      g_TdmfCollectorState;

extern  CResourceManager            g_ResourceManager;


#define _COLLECT_STATITSTICS_
#define _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_

#ifdef _COLLECT_STATITSTICS_

#define COLLECTOR_STATS_MESSAGE_WINDOW_NAME     "Collector Statistics"
#define COLLECTOR_MESSAGE_STRING                "COLLECTOR_STATS_MSG_STRING"
#define COLLECTOR_MAGIC_NUMBER                  0xFECEDEAE
#define COLLECTOR_STATS_BUFFER_VERSION          0x01000001

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "
#define __LOC2__ __FILE__ "("__STR1__(__LINE__)") : "

extern mmp_TdmfMessagesStates   g_TdmfMessageStates;

#endif

//**************************************************************
// DBServer structures 
//
typedef struct __CollectorConfig
{
    int             iPort;      //listener socket port number
    unsigned long   ulIP;       //TDMF Collector IP addr.
} CollectorConfig;


// ***********************************************************
/**
 * This object is used to identify an Agent by converting
 * one of its identification to its Host ID.
 * This object is used as a unique key in map.
 */
enum AgentIDType 
{
    IP_STRING,
    IP_NUMERIC,
    HOST_ID_NUMERIC,
    HOST_ID_STRING,
    HOST_NAME_STRING
};

class AgentIdentification
{
public:
    AgentIdentification() {m_IsValid = 0;m_iHostID = 0;};
    AgentIdentification( const void *pvAgentID, enum AgentIDType type);
    AgentIdentification( const AgentIdentification& AI)
    {
        m_IsValid = AI.m_IsValid;    
        m_iHostID = AI.m_iHostID;
    }
    //AgentIdentification( const char *szAgentID, enum AgentIDType type);
    //AgentIdentification( int iAgentID, enum AgentIDType type);

    bool operator< (const AgentIdentification & aAgent) const
    {
        return m_iHostID < aAgent.m_iHostID ;
    }

    bool    IsValid() const { return m_IsValid; }

    int     getHostID() const { return m_iHostID; }

private:
    bool    m_IsValid;    
    int     m_iHostID;
};

//**************************************************************
// DBServer global variables
//
//indicates to dbserver_sock_dispatch_io() it is time to quit.
extern volatile bool gbDBServerQuit;

//extern CollectorLog*    gpClctrLog


//**************************************************************
// function prototypes
//

//DBServer.cpp
void CollectorTrace( FILE* pStreamPtr, const char* message, ...);
void TraceCrit( const char* message, ...);
void TraceErr( const char* message, ...);
void TraceWrn( const char* message, ...);
void TraceInf( const char* message, ...);
void TraceAll( const char* message, ...);
void TraceMMP( const char* message, ...);
void TraceStats( const char* message, ...);
void TracePerf( const char* message, ...);
void ColDebugTrace(char* pcpMsg, ... );
//0 = disable traces, 1 = error, 2 = warning, 3 = info
void setCollectorTraceLevel(int level);
#define LOG_COLLECTOR_LOGFILE_FLAG  0x1
#define LOG_COLLECTOR_PRINTF_FLAG   0x2
#define LOG_COLLECTOR_PERF_FLAG     0x10

void setCollectorTraceFlags(int flags);
void setCollectorTraceParams(int level,int flags);

#ifdef _COLLECT_STATITSTICS_
void AccumulateCollectorStats(void);
void SendCollectorStatisticsData(void);
#endif

#ifdef _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_
void SendCollectorStatisticsDataToCommonApplication(void);
void RetreiveCollectorStatisticsData(mmp_TdmfCollectorState *data);
void InitializeCommonApplicationMemory(void);
void DeleteCommonApplicationMemory(void);
#endif


//DBServer_sock.cpp
void    dbserver_sock_dispatch_io( CollectorConfig* cfg );


//DBServer_db.cpp
int     dbserver_db_initialize(CollectorConfig* cfg);
void    dbserver_db_close();
void    dbserver_db_update_key(mmp_TdmfRegistrationKey *data);
void    dbserver_db_update_serverInfo(mmp_TdmfServerInfo *data);
void    dbserver_db_add_alert(const mmp_TdmfAlertHdr *alertData, const char* szAlertMessage );
//void    dbserver_db_add_monitoring(mmp_TdmfMonitoringData *monitoringData);
void    dbserver_db_add_status(mmp_TdmfStatusMsg *statusData, char* szMessage);
#ifdef _FTD_PERF_H_
void    dbserver_db_add_perfdata( mmp_TdmfPerfData *perfHdr, ftd_perf_instance_t *perfData );
#endif
void    dbserver_db_get_agent_ip_port(const char* szAgentUID, unsigned long *agentIP, unsigned int *agentPort);
void    dbserver_db_update_NVP_PerfCfg(const mmp_TdmfPerfConfig * perfCfg);
void    dbserver_db_get_list_servers( std::list<CString> & listServerUID );
bool    dbserver_db_get_agent_hostid(const char* szServerName, char* szHostId);
bool    dbserver_db_get_agent_hostid(int iServerIP, char* szHostId);
void    dbserver_db_update_server_port( const char * szHostID, int iNewPort );
void    dbserver_db_update_server_port( int iHostID, int iNewPort );
void    dbserver_db_update_server_state(mmp_TdmfAgentState *agentState, int iAgentIP);
void    dbserver_db_update_server_state(int iHostID, enum tdmf_agent_state eState);

//
int     dbserver_db_read_sysparams (mmp_TdmfCollectorParams *sysparams, CollectorConfig  *cfg);
int     dbserver_db_write_sysparams(const mmp_TdmfCollectorParams *sysparams, CollectorConfig  *cfg);

bool    dbserver_db_CheckRegistrationKey();
void    dbserver_db_SetCollectorRegistrationKey(char* szKey);

LPCSTR  dbserver_db_GetNameFromHostID(int iHostID);

void    dbserver_db_output_nb_msg_waiting();


//DBServer_agentMonitor.cpp
void    dbserver_agent_monit_initialize();
#ifdef _SOCK_H
void    dbserver_agent_monit_monitorDataConsumer( sock_t *monitorSocket );
void    dbserver_agent_monit_fowardGuiMsg(sock_t *monitorSocket);
#endif
#ifdef _FTD_PERF_H_
void    dbserver_agent_monit_UpdateAgentGroupPerf(const void* pvAgentID, enum AgentIDType type, const ftd_perf_instance_t * pLGPerf);
void    dbserver_agent_monit_UpdateAgentBABSizeAllocated(const void* pvAgentID, enum AgentIDType type, int iBABSizeAct, int iBABSizeReq);
#endif
void    dbserver_agent_monit_UpdateAgentAlive(bool bIsAlive, void* pvAgentID, enum AgentIDType type);
void    dbserver_agent_monit_UpdateAgentGroupState(const void* pvAgentID, enum AgentIDType type, const mmp_TdmfGroupState * pGrpState);
void    dbserver_agent_monit_UpdateAgentGroupMonitoring(const mmp_TdmfReplGroupMonitor * pGrpMonit, int iAgentIP );
void    dbserver_agent_monit_SetConnectState(bool bIsStarted, void* pvAgentID, enum AgentIDType type);
void    dbserver_agent_monit_output_map();


//DBServer_AgentAliveSocket.cpp
void AgentAliveSocketInitialize();
void AgentAliveSocketClose();
#ifdef _SOCK_H
void AgentAliveSocketAdd(sock_t* s, int iHostID, const mmp_mngt_TdmfAgentAliveMsg_t *pmsg);
#endif
void AgentAliveSetTimeStamp(int iHostID, time_t tmMostRecentTimeStamp);
bool AgentAliveSocketCheckTimeStamp(void);

#ifdef _DEBUG
void    debug_test_brdcst (int argc, char *argv[], CollectorConfig *cfg , bool server);
void    debug_simul_client(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_regkeyreq(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_setregkey(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_setLGcfg(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_setTdmfFile(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_getLGcfg(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_sendtdmfcmd(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_getalldevices(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_getAgentCfg(int argc, char* argv[], CollectorConfig *cfg );
void    debug_simul_client_SetPerfCfg(int argc, char* argv[], CollectorConfig *cfg );
//void    debug_display_agent_lgcfg(char *szAgentId, mmp_TdmfLGCfgHdr *data);
#endif

#endif  //__DBSERVER_H