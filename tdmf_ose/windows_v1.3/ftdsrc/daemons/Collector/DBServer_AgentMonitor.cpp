/*
 * DBServer_AgentMonitor.cpp - Management of all TDMF Agent status 
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
#include <afx.h>
#include <map>
#include <queue>
#ifdef _TEST_AGENT_MONITOR
#include <iostream>
#endif

extern "C" 
{
#include "iputil.h"
#include "sock.h"
#include "ftd_perf.h"
#include "ftdio.h"
}

#include "DBServer.h"
#include "libmngtmsg.h"
#include "TDMFNotificationSubscriptionMessage.h"
#include "libmngtnep.h"

using namespace std;

// ***********************************************************
/**
 * These objects contains the monitoring data filter for one consumer (GUI).
 * The filter controls the type of information sent back to consumer (GUI):
 * Server - status only
 * One Server Group - status only
 * One Server Group - full stats
 * The filter is set according to consumer requests messages.
 */
class GroupDataFilter
{
public:
    GroupDataFilter():  m_bStateStats(true)      //by default, receive a Group short status.
                       ,m_bFullStats(false) {};

    bool    m_bStateStats,  //Repl.Group State is requested
            m_bFullStats;   //Repl.Group full statistics are requested
};

class ServerDataFilter
{
public:
    ServerDataFilter() : m_nPrevBABAllocated(0), m_nPrevBABRequested(0) { RequestServerData(true); };//by default, receive any Servers status, registered or not.

    inline bool    NeedToSendAliveMsg()    { return m_cPrevAliveState != 1;}
    inline bool    NeedToSendNOTAliveMsg() { return m_cPrevAliveState != 0;}
    inline void    AgentIsAlive()          { m_cPrevAliveState = 1; }
    inline void    AgentIsNOTAlive()       { m_cPrevAliveState = 0; }

    inline void    RequestServerData(bool bRequest) { m_bStateStats = bRequest; m_cPrevAliveState = 2; }

    bool    m_bStateStats;      //Server/Agent State is requested by GUI ?

    char    m_cPrevAliveState;  //Last Alive state sent to consumer (GUI).  To Avoid resending same state over and over to GUI.
                                //0 == !Alive, 1 = Alive, 2 = not initialized

	int     m_nPrevBABAllocated;
	int     m_nPrevBABRequested;

    //each entry describes the data filter for one Replication Group
    std::map<int /*key = Group Nbr*/ ,GroupDataFilter>  m_mapGroup;
};

class ConsumerDataFilter : public TDMFNotificationSubscriptionMessage
{
public:
    //each entry describes the data filter for one TDMF Agent 
    std::map<int /*key = HostID*/ ,ServerDataFilter>  m_mapServer;
    /*
     * Override of base class methods.
     * Used when parsing subscription request
     */
    bool serverSubscription     ( int iHostID, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType );
    bool replgroupSubscription  ( int iHostID, int iGroupNumber, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType );
};
    
bool ConsumerDataFilter::serverSubscription     ( int iHostID, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType )
{
    ServerDataFilter & filterServer = m_mapServer[iHostID];
    _ASSERT( eType == TYPE_STATE );

    filterServer.RequestServerData( eAct == ACT_ADD );
    return true;
}

bool ConsumerDataFilter::replgroupSubscription  ( int iHostID, int iGroupNumber, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType )
{
    GroupDataFilter & filterGroup = m_mapServer[iHostID].m_mapGroup[iGroupNumber];

    if ( eType == TYPE_STATE )
    {
        filterGroup.m_bStateStats = ( eAct == ACT_ADD );
        //printf("\n +++++  %s STATE grp %d for server %08x ", ( eAct == ACT_ADD ) ? "ADD" : "DEL" , iGroupNumber, iHostID);
    }
    else if ( eType == TYPE_FULL_STATS )
    {
        filterGroup.m_bFullStats = ( eAct == ACT_ADD );
        //printf("\n ++++++ %s FULL grp %d for server %08x ", ( eAct == ACT_ADD ) ? "ADD" : "DEL" , iGroupNumber, iHostID);
    }

    return true;
}

// ***********************************************************
static bool dbserver_agent_monit_send_perf_data(sock_t* monitorSocket, DWORD dwConsumerID, ConsumerDataFilter & filter, bool bForceSend = false );
static bool dbserver_agent_monit_send_status_mode_data(sock_t* monitorSocket, DWORD dwConsumerID, ConsumerDataFilter & filter, bool bForceSend = false );
static bool dbserver_agent_monit_send_gui_msg_data(sock_t* monitorSocket, DWORD dwTID);
static bool dbserver_agent_monit_send_collector_stat(sock_t* monitorSocket, DWORD dwTID);

//functions used to manage the status/mode data PUSH
//one Event per consumer (GUI) thread.
//each consumer (GUI) thread Event is signaled when one RG status/mode changes.
static void     all_consumers_add   (DWORD dwTID, HANDLE& hEventPush, HANDLE& hEventGuiMsg);
static void     all_consumers_remove(DWORD dwTID = GetCurrentThreadId());
static void     all_consumers_signal_new_data();
static bool     all_consumers_pop_gui_msg(mmp_TdmfGuiMsg& Msg, DWORD dwTID);

// ***********************************************************
#define MAX_CONSUMERS       32  //32 bits max in a DWORD
#define MAX_DATA_TYPES      4
#define INVALID_TID         -1

#define CONSUMER_BIT(c)     (1UL << c)
// various index to be used for parameter 'dwOneDataType' - value used as index in m_DataTypes[]
#define RAW_PERF_DATA_INDEX         0
#define STATUS_AND_MODE_DATA_INDEX  1

// Collector Statistics broadcast interval, in secs
#define COLLECTOR_STAT_INTERVAL 15

class RepGroupMonitorData
{
public:
    RepGroupMonitorData();
    RepGroupMonitorData(const RepGroupMonitorData & aRGMData);//copy ctr
    ~RepGroupMonitorData();

    RepGroupMonitorData& operator=(const RepGroupMonitorData & aRGMData);

    void setRawPerf(const ftd_perf_instance_t* pPerf, int iHostID);
    void setGroupState(const mmp_TdmfGroupState * pGrpState, int iHostID);
    void setGroupMonit(const mmp_TdmfReplGroupMonitor * pGrpMonit, int iHostID);

           void getRawPerf(ftd_perf_instance_t* pPerf);
    inline time_t getPerfTimeStamp() const;
    inline time_t getMostRecentTimeStamp() const;
    inline int  getConnection() const;
    inline int  getDrvMode() const;
    inline int  getRGNumber() const;
    inline int  getPctDone() const;
	inline bool isGroupStarted() const;
    inline bool isGroupInCheckPoint() const;

    //returns the bit reserved for this consumer in all m_DataTypes[] values
    static DWORD AddConsumer( DWORD tid = GetCurrentThreadId() );
    static void  DelConsumer( DWORD tid = GetCurrentThreadId() );

    //returns the bit reserved for this consumer in all m_DataTypes[] values
    //DWORD getConsumerID( DWORD dwTID );

    bool  isDataTypeSetForConsumer( DWORD dwOneDataType, DWORD dwConsumerID )
            { return ( m_DataTypes[dwOneDataType] & CONSUMER_BIT(dwConsumerID) ) != 0; };

    void  clrDataTypeForConsumer( DWORD dwOneDataType, DWORD dwConsumerID )
            { m_DataTypes[dwOneDataType] &= ~CONSUMER_BIT(dwConsumerID); };

    void  markDataTypeUpdated(DWORD dwOneDataType)
            {   m_DataTypes[dwOneDataType] = ~0; };

#ifndef _DEBUG
private:
#endif
    inline  bool    Lock();
    inline  void    Unlock();

    //coordintation between reading and writing perf data to this object
    HANDLE              m_hMutex;

public:
    //latest RepGroup performance data
    ftd_perf_instance_t m_perf;
    time_t              m_perfts;

	//latest RepGroup state 
	mmp_TdmfGroupState	m_state;
    time_t              m_statets;
    //latest PStore and Journal related data
    __int64  m_iPStoreDiskTotalSz,
             m_iPStoreDiskFreeSz,
             m_iPStoreSz;
    __int64  m_iJournalDiskTotalSz,
             m_iJournalDiskFreeSz,
             m_iJournalSz;
    time_t   m_monitts;

    //keep track of a maximum of MAX_CONSUMERS consumers binary states 
    //one DWORD value corresponds to one thread id, a thread id identifying one consumer.
    //the index in the vector at which a consumer finds its thread id 
    //indicates which bit is reserved for it in each of the different m_DataFlags[] values.
    static DWORD    m_ConsumerIndexFromThreadID[MAX_CONSUMERS];
    static bool     m_bInitDone;

    //one DWORD value corresponds to a precise monitoring data type
    //each bit of a value is assigned to one Consumer (max of 32 consumers)
    //when a bit is set, it indicates to the targetted consumer that new data is available.
    //
    //this data organization allows to mark as updated one type of data in one, single operation, for all consumers
    //e.g. m_DataTypes[RAW_PERF_DATA_INDEX] = ~0;
    DWORD   m_DataTypes[MAX_DATA_TYPES];
};

DWORD RepGroupMonitorData::m_ConsumerIndexFromThreadID[MAX_CONSUMERS];
bool  RepGroupMonitorData::m_bInitDone = false;

// ***********************************************************
// ***********************************************************
typedef map<short /*lgnum*/ , RepGroupMonitorData>  LGMonitorMap;

// ***********************************************************
// ***********************************************************
//ONE Tdmf Agent monitoring data.
//Contains a map of all Rep.Group performance data.
class AgentMonitor
{
public:
    AgentMonitor() : m_nBABSizeAllocated(0), m_nBABSizeRequested(0)
        {
            m_bIsAlive  = FALSE;
        };
    AgentMonitor(const AgentMonitor& aAgent)
        {
            m_mapLG = aAgent.m_mapLG;
            m_bIsAlive = aAgent.m_bIsAlive;
#ifdef _DEBUG
            m_AgentID = aAgent.m_AgentID;
#endif
        }
    AgentMonitor& operator=(const AgentMonitor& aAgent)
        {
            m_mapLG = aAgent.m_mapLG;
            m_bIsAlive = aAgent.m_bIsAlive;
#ifdef _DEBUG
            m_AgentID = aAgent.m_AgentID;
#endif
			m_nBABSizeAllocated = aAgent.m_nBABSizeAllocated;
			m_nBABSizeRequested = aAgent.m_nBABSizeRequested;

            return *this;
        }

    //update a RepGroup with its latest perf. data
    void setData( const ftd_perf_instance_t * pLGPerf , int iHostID );
    //indicates if Tdmf Agent is alive or not
    void setData( bool bIsAlive , int iHostID );
    //update a RepGroup with its latest state
	void setData( const mmp_TdmfGroupState * pGrpState , int iHostID );
    void setData( const mmp_TdmfReplGroupMonitor * pGrpMonit , int iHostID );

#ifdef _DEBUG
    void setID( AgentIdentification & agentID ) { m_AgentID = agentID; } ;
    int  getHostID() const { return m_AgentID.getHostID(); } 
#endif

    bool    IsAlive() { return m_bIsAlive; }

#if 0
//#ifdef _TEST_AGENT_MONITOR
    operator CString()
        {
            char buf[32];
            CString tmp("Agent ");
            tmp += itoa(getHostID(),buf,16) ;
            tmp += ", monitor groups: ";

            LGMonitorMap::iterator it = m_mapLG.begin();
            while( it != m_mapLG.end() )
            {
                short lgnum = ((*it).second).m_perf.lgnum;
                //int   connection = ((*it).second).m_perf.connection;
                int   insert = ((*it).second).m_perf.insert;
                tmp += itoa(lgnum,buf,10) ;
                tmp += " insert=";
                tmp += itoa(insert,buf,10) ;
                tmp += ", ";
                it++;
            }
            return tmp;
        }
#endif

    //one ftd_perf_instance_t per Replication (Logical) Group
    LGMonitorMap    m_mapLG;

	int             m_nBABSizeAllocated;
	int             m_nBABSizeRequested;

private:
    //indicates if the TDMF Agent is Alive or not
    bool            m_bIsAlive;

#ifdef _DEBUG
    AgentIdentification m_AgentID;
#endif

};

// ***********************************************************
// ***********************************************************
typedef map<AgentIdentification,AgentMonitor>   AgentMonitorMap;
//Tdmf Agent monitoring data for ALL Agents
static AgentMonitorMap  gmapAgentMonitor;

// ***********************************************************
// ***********************************************************
RepGroupMonitorData::RepGroupMonitorData()
{
    if ( !m_bInitDone )
    {   //do only once, on first obj construction
        memset(m_ConsumerIndexFromThreadID,INVALID_TID,sizeof(m_ConsumerIndexFromThreadID));
        m_bInitDone = true;
    }
    m_hMutex = CreateMutex(0,0,0);

    memset(&m_perf,-1,sizeof(m_perf));
	memset(&m_state,0,sizeof(m_state));
    m_iPStoreDiskTotalSz = m_iPStoreDiskFreeSz = m_iPStoreSz = 
        m_iJournalDiskTotalSz = m_iJournalDiskFreeSz = m_iJournalSz = 0;

    m_perfts    = 0;
    m_statets   = 0;
}

//copy ctr
RepGroupMonitorData::RepGroupMonitorData(const RepGroupMonitorData & aRGMData)
{
    BOOL b = DuplicateHandle(
      GetCurrentProcess(),  // handle to source process
      aRGMData.m_hMutex,         // handle to duplicate
      GetCurrentProcess(),  // handle to target process
      &m_hMutex,      // duplicate handle
      0,        // requested access
      0,          // handle inheritance option
      DUPLICATE_SAME_ACCESS               // optional actions
    );_ASSERT(b);

    m_perf  =   aRGMData.m_perf;
	memset(&m_state,0,sizeof(m_state));//clear states


    m_iPStoreDiskTotalSz    = aRGMData.m_iPStoreDiskTotalSz;
    m_iPStoreDiskFreeSz     = aRGMData.m_iPStoreDiskFreeSz;
    m_iPStoreSz             = aRGMData.m_iPStoreSz;
    m_iJournalDiskTotalSz   = aRGMData.m_iJournalDiskTotalSz;
    m_iJournalDiskFreeSz    = aRGMData.m_iJournalDiskFreeSz;
    m_iJournalSz            = aRGMData.m_iJournalSz;

    for(int i=0 ; i<MAX_DATA_TYPES; i++) 
        m_DataTypes[i] = aRGMData.m_DataTypes[i];

    m_perfts    = 0;
    m_statets   = 0;
}

RepGroupMonitorData::~RepGroupMonitorData()
{
    if ( m_hMutex != NULL ) 
        CloseHandle(m_hMutex);
    m_hMutex = NULL;
}

RepGroupMonitorData& RepGroupMonitorData::operator=(const RepGroupMonitorData & aRGMData)
{
    BOOL b = DuplicateHandle(
      GetCurrentProcess(),  // handle to source process
      aRGMData.m_hMutex,         // handle to duplicate
      GetCurrentProcess(),  // handle to target process
      &m_hMutex,      // duplicate handle
      0,        // requested access
      0,          // handle inheritance option
      DUPLICATE_SAME_ACCESS               // optional actions
    );_ASSERT(b);

    m_perf              = aRGMData.m_perf;
	m_state             = aRGMData.m_state; 


    m_iPStoreDiskTotalSz    = aRGMData.m_iPStoreDiskTotalSz;
    m_iPStoreDiskFreeSz     = aRGMData.m_iPStoreDiskFreeSz;
    m_iPStoreSz             = aRGMData.m_iPStoreSz;
    m_iJournalDiskTotalSz   = aRGMData.m_iJournalDiskTotalSz;
    m_iJournalDiskFreeSz    = aRGMData.m_iJournalDiskFreeSz;
    m_iJournalSz            = aRGMData.m_iJournalSz;

    for(int i=0 ; i<MAX_DATA_TYPES; i++) 
        m_DataTypes[i] = aRGMData.m_DataTypes[i];

    return *this;
}

bool  RepGroupMonitorData::Lock()
{
    if ( m_hMutex != NULL )
        return WAIT_OBJECT_0 == WaitForSingleObject( m_hMutex, 60*1000 );
    return true;
}

void  RepGroupMonitorData::Unlock()
{
    if ( m_hMutex != NULL ) {
        BOOL b = ReleaseMutex( m_hMutex );_ASSERT(b);
    }
}

DWORD RepGroupMonitorData::AddConsumer( DWORD tid )
{
    for(DWORD i=0;i<MAX_CONSUMERS; i++)
    {
        if ( m_ConsumerIndexFromThreadID[i] == INVALID_TID )
        {   //assign this slot to the consumer
            m_ConsumerIndexFromThreadID[i] = tid;
            return i;
        }
    }
    return -1;
}

void RepGroupMonitorData::DelConsumer( DWORD tid )
{
    for(DWORD i=0;i<MAX_CONSUMERS; i++)
    {
        if ( m_ConsumerIndexFromThreadID[i] == tid )
        {   //free this slot 
            m_ConsumerIndexFromThreadID[i] = INVALID_TID;
            return;
        }
    }
}
    
void RepGroupMonitorData::setRawPerf(const ftd_perf_instance_t* pPerf, int iHostID)
{
    bool bNewData = false;
    int  drvmode = pPerf->drvmode;

    Lock();

    // some Unix agent drvmode can be FTD_MODE_FULLREFRESH: 
    // this has to be translated to FTD_MODE_REFRESH.
    if ( drvmode == FTD_MODE_FULLREFRESH )
        drvmode = FTD_MODE_REFRESH;

    if ( m_perf.connection != pPerf->connection ||
         m_perf.drvmode    != drvmode           ||
         m_perf.pctdone    != pPerf->pctdone    )
    {   
        TraceStats("RX STATS, Raw Perf: HostID=%08x (%s) lgnum=%3d : OLD c=%d , m=0x%02x . NEW c=%d , m=0x%02x , pctdone=%3u , bytesread=0x%016I64x.\n",
            iHostID, dbserver_db_GetNameFromHostID(iHostID), pPerf->lgnum, m_perf.connection, m_perf.drvmode, pPerf->connection, drvmode, pPerf->pctdone,pPerf->bytesread);
        markDataTypeUpdated(STATUS_AND_MODE_DATA_INDEX);
        bNewData = true;
    }
    //copy new values
    m_perf = *pPerf;
    m_perf.drvmode = drvmode;// translated drvmode
    m_perfts = time(0);


    markDataTypeUpdated(RAW_PERF_DATA_INDEX);
    Unlock();

    if ( bNewData )
    {   //signal all consumer of monitoring data that a change of status/mode as occured 
        all_consumers_signal_new_data();
    }
}

void RepGroupMonitorData::setGroupState(const mmp_TdmfGroupState * pGrpState, int iHostID)
{
    bool bNewData = false;

    Lock();
    if ( m_state.sState != pGrpState->sState )
    {   
        TraceStats("RX STATS, Group State: HostID=%08x (%s) lgnum=%d : OLD state=0x%02hx   NEW state=0x%02hx .\n",
            iHostID, dbserver_db_GetNameFromHostID(iHostID), m_state.sRepGrpNbr, m_state.sState, pGrpState->sState );
		//group state is part of status-mode data
        markDataTypeUpdated(STATUS_AND_MODE_DATA_INDEX);
        bNewData = true;
    }
    m_state = *pGrpState;
    m_statets = time(0);
    Unlock();

    if ( bNewData )
    {   //signal all consumer of monitoring data that a change of status/mode as occured 
        all_consumers_signal_new_data();
    }
}

void RepGroupMonitorData::setGroupMonit(const mmp_TdmfReplGroupMonitor * pGrpMonit, int iHostID)
{
    __int64 newTotal,newFree,newActual;

    newTotal    = pGrpMonit->liDiskTotalSz;
    newFree     = pGrpMonit->liDiskFreeSz;
    newActual   = pGrpMonit->liActualSz;

    Lock();
    if ( pGrpMonit->isSource )
    {   //msg comes from the SOURCE server, PStore data 
        TraceStats("RX STATS, Group Monit: HostID=%08x (%s) lgnum=%d : PStore: size=%-I64d KB, old=%-I64d KB. Disk Total=%-I64d KB, old=%-I64d KB. Disk Free=%-I64d KB, old=%-I64d KB\n",
            iHostID, dbserver_db_GetNameFromHostID(iHostID), (int)pGrpMonit->sReplGrpNbr, 
            newActual/1024,m_iPStoreSz/1024,
            newTotal/1024, m_iPStoreDiskTotalSz/1024,
            newFree/1024,  m_iPStoreDiskFreeSz/1024
            );

        m_iPStoreDiskTotalSz    = newTotal;
        m_iPStoreDiskFreeSz     = newFree;
        m_iPStoreSz             = newActual;
    }
    else
    {   //msg comes from the TARGET server, Journal data 
        TraceStats("RX STATS, Group Monit: HostID=%08x (%s) lgnum=%d : Journal: size=%-I64d KB, old=%-I64d KB. Disk Total=%-I64d KB, old=%-I64d KB. Disk Free=%-I64d KB, old=%-I64d KB\n",
            iHostID, dbserver_db_GetNameFromHostID(iHostID), (int)pGrpMonit->sReplGrpNbr, 
            newActual/1024,  m_iJournalSz/1024,
            newTotal/1024,   m_iJournalDiskTotalSz/1024,
            newFree/1024,    m_iJournalDiskFreeSz/1024 
            );

        m_iJournalDiskTotalSz   = newTotal;
        m_iJournalDiskFreeSz    = newFree;
        m_iJournalSz            = newActual;
    }
    m_monitts = time(0);

	//group monitoring data is part of status-mode data
    markDataTypeUpdated(STATUS_AND_MODE_DATA_INDEX);
    Unlock();

    //signal all consumer of monitoring data that a change of status/mode as occured 
    all_consumers_signal_new_data();
}

    
void RepGroupMonitorData::getRawPerf(ftd_perf_instance_t* pPerf) 
{
    Lock();

    *pPerf = m_perf;

    Unlock();
}

#define MAX(a,b)    ((a)>(b)?(a):(b)) 
inline time_t RepGroupMonitorData::getMostRecentTimeStamp() const
{
    return MAX( MAX(m_perfts, m_statets) , m_monitts );
}

inline time_t RepGroupMonitorData::getPerfTimeStamp() const
{
    return m_perfts;
}

inline int RepGroupMonitorData::getConnection() const
{
    return m_perf.connection;
}

inline int RepGroupMonitorData::getDrvMode() const
{
    return m_perf.drvmode;
}

inline int RepGroupMonitorData::getRGNumber() const
{
    return m_perf.lgnum;
}

inline int RepGroupMonitorData::getPctDone() const
{
    return m_perf.pctdone;
}

inline bool RepGroupMonitorData::isGroupStarted() const
{
	return (m_state.sState & 0x1) == 0x1 ;
}

inline bool RepGroupMonitorData::isGroupInCheckPoint() const
{
	return (m_state.sState & 0x2) == 0x2 ;
}

// ***********************************************************
// ***********************************************************
//this version of setData() replaces 
//the ftd_perf_instance_t data of one Replication group (logical group)
void AgentMonitor::setData( const ftd_perf_instance_t * pLGPerf , int iHostID )
{   //refresh LG perf data
    //logical group bucket added in map if not already existing 
    _ASSERT( pLGPerf->lgnum == -100 && pLGPerf->devid >= 0 );

    if ( ! (pLGPerf->lgnum == -100 && pLGPerf->devid >= 0) )
        return;

    ftd_perf_instance_t perf = *pLGPerf;

    perf.lgnum = pLGPerf->devid;//group stats: lgnum = -100 , devid = lg number.
    perf.devid = -1;//devid = -1 => groups stats => does not refer to a specific device

    m_mapLG[ perf.lgnum ].setRawPerf( &perf , iHostID);
}

void AgentMonitor::setData( bool bIsAlive , int iHostID )
{
    bool bNewData = false;

    bNewData = ( m_bIsAlive != bIsAlive );
    m_bIsAlive = bIsAlive;
	if ( bNewData && (bIsAlive == false) ) // signal now only if it's a not alive msg otherwise wait for updated info
    {   //signal all consumers of monitoring data that a change of status/mode as occured 
        all_consumers_signal_new_data();
    }
}

void AgentMonitor::setData( const mmp_TdmfGroupState * pGrpState , int iHostID )
{   //refresh LG state data
    if ( pGrpState->sRepGrpNbr < 0 )
        return;
    //logical group bucket added to m_mapLG if not already existing 
    m_mapLG[ pGrpState->sRepGrpNbr ].setGroupState( pGrpState , iHostID );
}

void AgentMonitor::setData( const mmp_TdmfReplGroupMonitor * pGrpMonit , int iHostID )
{   //refresh LG state data
    if ( pGrpMonit->sReplGrpNbr < 0 )
        return;
    //logical group bucket added to m_mapLG if not already existing 
    m_mapLG[ pGrpMonit->sReplGrpNbr ].setGroupMonit( pGrpMonit , iHostID );
}

// ***********************************************************
// Function name	: dbserver_agent_monit_AgentAlive
// Description	    : Report if TDMF Agent is alive or not.
// Return type		: void 
// Argument         : bool bIsAlive
// 
// ***********************************************************
void dbserver_agent_monit_UpdateAgentAlive(bool bIsAlive, void* pvAgentID, enum AgentIDType type)
{
    AgentIdentification agentId( pvAgentID, type );

    _ASSERT(agentId.IsValid());
    if ( agentId.IsValid() )
    {   //Add agent in map if it is not already existing.
        //Then, value (pLGPerf) is updated for this agent.
#ifdef _DEBUG
        gmapAgentMonitor[ agentId ].setID( agentId );
#endif
 		if (bIsAlive == false)
 		{
 			gmapAgentMonitor[ agentId ].setData( bIsAlive , agentId.getHostID() );
 		}
    }
}


// ***********************************************************
// Function name	: dbserver_agent_monit_UpdateAgentGroupPerf
// Description	    : Updates one Replication Group data
//                    of one TDMF Agent.
// Return type		: void 
// Argument         : const void* pvAgentID : TDMF Agent identification
// Argument         : enum AgentIDType type : type 
// Argument         : const ftd_perf_instance_t * pLGPerf : performance data of one Replication Group.
// 
// ***********************************************************
void dbserver_agent_monit_UpdateAgentGroupPerf(const void* pvAgentID, enum AgentIDType type, const ftd_perf_instance_t * pLGPerf)
{
    AgentIdentification agentId( pvAgentID, type );

    _ASSERT(agentId.IsValid());
    if ( agentId.IsValid() )
    {   //Add agent in map if it is not already existing.
        //Then, value (pLGPerf) is updated for this agent.
#ifdef _DEBUG
        gmapAgentMonitor[ agentId ].setID( agentId );
#endif
        gmapAgentMonitor[ agentId ].setData( pLGPerf , agentId.getHostID() );

#ifdef _TEST_AGENT_MONITOR
        cout << (LPCTSTR)((CString)gmapAgentMonitor[ agentId ]) << endl;
#endif
    }
}

void dbserver_agent_monit_UpdateAgentBABSizeAllocated(const void* pvAgentID, enum AgentIDType type, int iBABSizeAct, int iBABSizeReq)
{
    AgentIdentification agentId( pvAgentID, type );

    _ASSERT(agentId.IsValid());
    if ( agentId.IsValid() )
    {   //Add agent in map if it is not already existing.
        //Then, value (pLGPerf) is updated for this agent.
#ifdef _DEBUG
        gmapAgentMonitor[ agentId ].setID( agentId );
#endif
		if (!gmapAgentMonitor[ agentId ].IsAlive() ||
			(gmapAgentMonitor[ agentId ].m_nBABSizeAllocated != iBABSizeAct) ||
			(gmapAgentMonitor[ agentId ].m_nBABSizeRequested != iBABSizeReq))
		{
			gmapAgentMonitor[ agentId ].setData( true, agentId.getHostID() );
			gmapAgentMonitor[ agentId ].m_nBABSizeAllocated = iBABSizeAct;
			gmapAgentMonitor[ agentId ].m_nBABSizeRequested = iBABSizeReq;
			all_consumers_signal_new_data();
		}

#ifdef _TEST_AGENT_MONITOR
        cout << (LPCTSTR)((CString)gmapAgentMonitor[ agentId ]) << endl;
#endif
    }
}



// ***********************************************************
// Function name	: dbserver_agent_monit_UpdateAgentGroupState
// Description	    : Updates one Replication Group data
//                    of one TDMF Agent.
// Return type		: void 
// Argument         : const void* pvAgentID : TDMF Agent identification
// Argument         : enum AgentIDType type : type 
// Argument         : const ftd_perf_instance_t * pLGPerf : performance data of one Replication Group.
// 
// ***********************************************************
void dbserver_agent_monit_UpdateAgentGroupState(const void* pvAgentID, enum AgentIDType type, const mmp_TdmfGroupState * pGrpState)
{
    AgentIdentification agentId( pvAgentID, type );

    _ASSERT(agentId.IsValid());
    if ( agentId.IsValid() )
    {   //Add agent in map if it is not already existing.
        //Then, value (pLGPerf) is updated for this agent.
#ifdef _DEBUG
        gmapAgentMonitor[ agentId ].setID( agentId );
#endif
        gmapAgentMonitor[ agentId ].setData( pGrpState , agentId.getHostID() );
    }
}

void dbserver_agent_monit_UpdateAgentGroupMonitoring(const mmp_TdmfReplGroupMonitor * pGrpMonit, int iAgentIP)
{
    void *pvAgentID;
    enum AgentIDType type;

    if ( pGrpMonit->isSource )
    {   //sender is a the SOURCE TDMF Server of this group
        pvAgentID = (void*)pGrpMonit->szServerUID;
        type      = HOST_ID_STRING;
    }
    else
    {   //sender is a the TARGET TDMF Server of this group
        //special case for loopback mode
        if ( pGrpMonit->iReplGrpSourceIP == 0x0100007f )    //127.0.0.1 = loopback
            pvAgentID = (void*)&iAgentIP;//IP of host (host = SOURCE and TARGET) from which this information is received.
        else
            pvAgentID = (void*)&pGrpMonit->iReplGrpSourceIP;
        type      = IP_NUMERIC;
    }

    AgentIdentification agentId( pvAgentID, type );
    _ASSERT(agentId.IsValid());
    if ( agentId.IsValid() )
    {   //Add agent in map if it is not already existing.
        //Then, value (pGrpMonit) is updated for this agent.
#ifdef _DEBUG
        gmapAgentMonitor[ agentId ].setID( agentId );
#endif
        gmapAgentMonitor[ agentId ].setData( pGrpMonit , agentId.getHostID() );
    }
}

// ***********************************************************
// Function name	: dbserver_agent_monit_SetConnectState
// Description	    : Report if TDMF Agent is alive or not.
// Return type		: void 
// Argument         : bool bIsConnect
// 
// ***********************************************************
void dbserver_agent_monit_SetConnectState(bool bIsStarted, void* pvAgentID, enum AgentIDType type)
{
	mmp_TdmfGroupState thisGrpState;
	unsigned long ulHostID;
    AgentIdentification agentId( pvAgentID, type );
	
    _ASSERT(agentId.IsValid());
    if ( agentId.IsValid() )
    { 
		AgentMonitorMap::iterator it;
		ulHostID = agentId.getHostID();
		// Scan thru gmapAgentMonitor list to point to proper AgentID info list
		for (it = gmapAgentMonitor.begin(); it != gmapAgentMonitor.end(); it++)
		{
			AgentIdentification ThisAgentId = it->first;
			// If HostId match, convert all active LGs states
			if ( ThisAgentId.getHostID() == ulHostID )
			{
				AgentMonitor AgentMon = it->second;

				LGMonitorMap::iterator itLG;
				for (itLG = AgentMon.m_mapLG.begin(); itLG != AgentMon.m_mapLG.end(); itLG++)
				{
					RepGroupMonitorData RGData = itLG->second;
					thisGrpState.sRepGrpNbr = RGData.getRGNumber();
					thisGrpState.sState     = bIsStarted; //Not started

					TraceInf("SetConnectState() ID = %08x  LG%d  State:%d\n", ulHostID, thisGrpState.sRepGrpNbr, bIsStarted);
					dbserver_agent_monit_UpdateAgentGroupState(&ulHostID, HOST_ID_NUMERIC, &thisGrpState);
				}
			}	
		}
	}
}

// ***********************************************************
typedef struct {
    DWORD   dwThreadId;
    HANDLE  hEvent;
	// GUI Msg forwarding
	HANDLE  hEventGuiMsg;
	std::queue<mmp_TdmfGuiMsg> queueGuiMsg;
} MonitoringDataConsumers;
static MonitoringDataConsumers gAllConsumers[MAX_CONSUMERS];

void all_consumers_add(DWORD dwTID, HANDLE& hEventPush, HANDLE& hEventGuiMsg)
{
    MonitoringDataConsumers *pConsumer = gAllConsumers;
    for( int i=0;i < MAX_CONSUMERS; i++, pConsumer++)
    {
        if ( pConsumer->dwThreadId == INVALID_TID )
        {   
            pConsumer->dwThreadId   = dwTID;
            pConsumer->hEvent       = CreateEvent(0,0,0,0);
			// GUI Msg forwarding
			pConsumer->hEventGuiMsg = CreateEvent(0,0,0,0);

            hEventPush   = pConsumer->hEvent;
			hEventGuiMsg = pConsumer->hEventGuiMsg;

			return;
        }
    }
}

void all_consumers_remove(DWORD dwTID)
{
    MonitoringDataConsumers *pConsumer = gAllConsumers;
    for( int i=0;i < MAX_CONSUMERS; i++, pConsumer++)
    {
        if ( pConsumer->dwThreadId == dwTID )
        {   
            pConsumer->dwThreadId = INVALID_TID;
            CloseHandle( pConsumer->hEvent );
			// GUI Msg forwarding
			CloseHandle( pConsumer->hEventGuiMsg );
            return;
        }
    }
}

void all_consumers_signal_new_data()
{
    MonitoringDataConsumers *pConsumer = gAllConsumers;
    for( int i=0;i < MAX_CONSUMERS; i++, pConsumer++)
    {
        if ( pConsumer->dwThreadId != INVALID_TID )
        {   
            SetEvent( pConsumer->hEvent );
        }
    }
}

// GUI Msg forwarding
void all_consumers_add_gui_msg(mmp_TdmfGuiMsg Msg)
{
    MonitoringDataConsumers *pConsumer = gAllConsumers;
    for( int i=0;i < MAX_CONSUMERS; i++, pConsumer++)
    {
        if ( pConsumer->dwThreadId != INVALID_TID )
        {   
            pConsumer->queueGuiMsg.push(Msg);
			SetEvent( pConsumer->hEventGuiMsg );
        }
    }
}

bool all_consumers_pop_gui_msg(mmp_TdmfGuiMsg& Msg, DWORD dwTID)
{
    MonitoringDataConsumers *pConsumer = gAllConsumers;
    for( int i=0;i < MAX_CONSUMERS; i++, pConsumer++)
    {
        if ( pConsumer->dwThreadId == dwTID )
        {   
			Msg = pConsumer->queueGuiMsg.front();
            pConsumer->queueGuiMsg.pop();

			return true;
        }
    }

	return false;
}

// ***********************************************************
// Function name	: dbserver_agent_monit_monitorDataConsumer
// Description	    : This function is called within a thread created 
//                    when a MMP_MNGT_MONITOR_DATA_REGISTER is received.
//                    The hdr portion of the MMP_MNGT_MONITORING_DATA_REGISTRATION message                    
//                    is already read at this point.
// Return type		: void 
// Argument         : sock_t *monitorSocket
// 
// ***********************************************************
enum monitor_states
{
    STATE_WAIT_FOR_EVENTS,
    STATE_READ_NOTIF_MSG,
    STATE_SEND_MONITORING_PERF_DATA_MSG,
    STATE_SEND_MONITORING_STATUS_MODE_DATA_MSG,
    STATE_PEER_IS_GONE,
	STATE_SEND_GUI_DATA_MSG,
	STATE_SEND_COLLECTOR_STAT_MSG           //rddev 030328
};

void dbserver_agent_monit_monitorDataConsumer( sock_t *monitorSocket )
{
    bool    bContinue = true;
    int     r;
    int     state;
    DWORD   timeout = 1000; //millisec
    time_t  timeoutsec,     //in seconds
	        stattimeoutsec; //in seconds
    DWORD   tid = GetCurrentThreadId();
    DWORD   dwConsumerID = RepGroupMonitorData::AddConsumer(tid);
    HANDLE  hPUSHEvent;
	HANDLE  hGuiMsgEvent;
    HANDLE  handles[3];
    DWORD   waitstatus;
    short   sConsecutiveTimeouts = 0;
    ConsumerDataFilter  filter;
    time_t  now, lastRawPerfSend = time(0); 
    time_t  lastCollectorStatSend = time(0);

    TraceInf("dbserver_agent_monit_monitorDataConsumer IN  tid=0x%08x \n",tid);

    mmp_TdmfMonitoringCfg   data;
    int toread = sizeof(data);
    r = mmp_mngt_sock_recv(monitorSocket, (char *)&data, toread);
    if ( r == toread )
    {
        timeoutsec = ntohl(data.iMaxDataRefreshPeriod);
        timeout = timeoutsec * 1000;//second to msec
		stattimeoutsec = COLLECTOR_STAT_INTERVAL;  

        //start by sending a full review
        if ( dbserver_agent_monit_send_status_mode_data(monitorSocket,dwConsumerID,filter,true) )
            state = STATE_WAIT_FOR_EVENTS;
        else
            state = STATE_PEER_IS_GONE; 

        if ( dbserver_agent_monit_send_perf_data(monitorSocket,dwConsumerID,filter,true) )
            state = STATE_WAIT_FOR_EVENTS;
        else
            state = STATE_PEER_IS_GONE; 

    } 
    else 
    {
        state = STATE_PEER_IS_GONE; 
    }

    monitorSocket->hEvent = (HANDLE)WSACreateEvent();
    r = sock_set_nonb(monitorSocket);_ASSERT(r == 0);

	all_consumers_add(tid, hPUSHEvent, hGuiMsgEvent);
    handles[0] = monitorSocket->hEvent;
    handles[1] = hPUSHEvent;
	handles[2] = hGuiMsgEvent;

	//Push Collector Stat to data consumer for the first time
	dbserver_agent_monit_send_collector_stat( monitorSocket,tid );

    //loop until data consumer closes the socket
    while( state != STATE_PEER_IS_GONE )
    {
        /////////////////////////////////
        //state machine
        //
        //tests sequence IS important !!
        /////////////////////////////////
        if ( state == STATE_WAIT_FOR_EVENTS )
        {   //check for data from peer OR a PUSH event...
            waitstatus = WaitForMultipleObjects(3,handles,FALSE,1000);//wake up each second to check for timeout (timeoutsec)

            now = time(0);

            if (waitstatus == WAIT_OBJECT_0)
            {   //monitorSocket->hEvent signaled = socket event from peer/consumer
                //request or config. msg received from peer
                int bytes = (int)tcp_get_numbytes(monitorSocket->sock);
                if ( bytes > 0 )
                    state = STATE_READ_NOTIF_MSG;//go read data 
                else
                    state = STATE_PEER_IS_GONE;
            }
            else if (waitstatus == WAIT_OBJECT_0 + 1)
            {   //new monitoring data available. PUSH it to consumer.
                state = STATE_SEND_MONITORING_STATUS_MODE_DATA_MSG;
            }
			else if (waitstatus == WAIT_OBJECT_0 + 2)
            {   //new monitoring data available. PUSH it to consumer.
                state = STATE_SEND_GUI_DATA_MSG;
            }
        }

        //////////////////////////////////////////////////////////////////
        //exclusive tests of various state values
        //////////////////////////////////////////////////////////////////
        if ( state == STATE_READ_NOTIF_MSG )
        {
            TDMFNotificationMessage rxMsg;
            bool b = rxMsg.Recv(monitorSocket, 30000 /*msec*/ );//_ASSERT(b);
            if ( b )
            {   
                if ( 0 == tcp_get_numbytes(monitorSocket->sock) )
                    WSAResetEvent(monitorSocket->hEvent);//clear a possibly signaled event...

                //valid notification request. now check data
                if ( rxMsg.getType() == NOTIF_MSG_SUBSCRIPTION )
                {
                    filter.setData(rxMsg.getData(), rxMsg.getLength(), false);
                    filter.parseData();
                    state = STATE_WAIT_FOR_EVENTS;
                }
                else
                {
                    state = STATE_PEER_IS_GONE;
                }
            }
            else
            {
                state = STATE_PEER_IS_GONE; 
            }
        }
        /////////////////////////////////
        else if ( state == STATE_SEND_MONITORING_PERF_DATA_MSG )
        {   //send only the performance data that changed since last call
            if ( dbserver_agent_monit_send_perf_data(monitorSocket,dwConsumerID,filter) )
                state = STATE_WAIT_FOR_EVENTS;
            else
                state = STATE_PEER_IS_GONE; 
        }
        /////////////////////////////////
        else if ( state == STATE_SEND_MONITORING_STATUS_MODE_DATA_MSG )
        {   //send only the performance data that changed since last call
            if ( dbserver_agent_monit_send_status_mode_data(monitorSocket,dwConsumerID,filter) )
                state = STATE_WAIT_FOR_EVENTS;
            else
                state = STATE_PEER_IS_GONE; 
        }
        /////////////////////////////////
        else if ( state == STATE_SEND_GUI_DATA_MSG )
        {   //send only the performance data that changed since last call
            if ( dbserver_agent_monit_send_gui_msg_data(monitorSocket, tid) )
                state = STATE_WAIT_FOR_EVENTS;
            else
                state = STATE_PEER_IS_GONE; 
        }
        else if ( state == STATE_SEND_COLLECTOR_STAT_MSG )
        {   //send Collector Statistics values
            if ( dbserver_agent_monit_send_collector_stat(monitorSocket,tid) )  //rddev 030328
                state = STATE_WAIT_FOR_EVENTS;
            else
                state = STATE_PEER_IS_GONE; 
        }

        //////////////////////////////////////////////////////////////////
        //make sure RAW perf are sent each time timeoutsec is reached
        //////////////////////////////////////////////////////////////////
        if ( state == STATE_WAIT_FOR_EVENTS )
        {   
            if( now - lastRawPerfSend >= timeoutsec )
            {   //peer-configured timeout expired. It is time to send performance data to peer.
                lastRawPerfSend = now;
                state = STATE_SEND_MONITORING_PERF_DATA_MSG;
            }

            else if( now - lastCollectorStatSend >= stattimeoutsec )
            {   //peer-configured timeout expired. It is time to send Collector Statistics to peer.
                lastCollectorStatSend = now;
                state = STATE_SEND_COLLECTOR_STAT_MSG;
            }
        }

    }//while

    RepGroupMonitorData::DelConsumer(tid);
    all_consumers_remove(tid);

    sock_disconnect(monitorSocket);
    sock_set_b(monitorSocket);
    WSACloseEvent(monitorSocket->hEvent);

    TraceInf("dbserver_agent_monit_monitorDataConsumer OUT tid=0x%08x \n\n",tid);
}

// ***********************************************************
// Function name	: dbserver_agent_monit_send_perf_data
// Description	    : 
// Return type		: int 
// Argument         : sock_t* monitorSocket
// 
// ***********************************************************
bool dbserver_agent_monit_send_perf_data(sock_t* monitorSocket, DWORD dwConsumerID, ConsumerDataFilter & filter, bool bForceSend )
{
    //mmp_mngt_TdmfMonitorDataMsg_t   msg;
    TDMFNotificationMessage         msg(NOTIF_MSG_RG_RAW_PERF_DATA);
    mnep_NotifMsgDataRepGrpRawPerf  msgData;

    //send performance data messages to consumer
    AgentMonitorMap::iterator agentit  = gmapAgentMonitor.begin();
    AgentMonitorMap::iterator agentend = gmapAgentMonitor.end();
    //scan each server
    while( agentit != agentend )
    {
        int iHostID = ((*agentit).first).getHostID();

        msgData.agentUID.iHostID = htonl( iHostID );
        //
        //scan each Rep.Group of one server
        //
        LGMonitorMap::iterator rgit     = ((*agentit).second).m_mapLG.begin();
        LGMonitorMap::iterator rgitend  = ((*agentit).second).m_mapLG.end();
        ServerDataFilter & filterServer = filter.m_mapServer[iHostID];
        while( rgit != rgitend )
        {
            //request for this Repl.Group full stats ?
            if ( filterServer.m_mapGroup[(*rgit).first].m_bFullStats )
            {
                //get a REFERENCE on the RepGroupMonitorData obj in the map
                RepGroupMonitorData & monitData = ((*rgit).second);
                //check if this RepGroup data has been updated since last call
                if ( monitData.isDataTypeSetForConsumer(RAW_PERF_DATA_INDEX,dwConsumerID) || 
                     bForceSend )
                {
                    //get latest performance data for this RG
                    monitData.getRawPerf(&msgData.perf);
		            msgData.agentUID.tStimeTamp = htonl( monitData.getMostRecentTimeStamp() );
                    //convert to network byte order
                    mmp_convert_hton(&msgData.perf);

                    msg.setDataExt(sizeof(msgData),(char*)&msgData);
                    if ( !msg.Send(monitorSocket) )
                        return false;
                    
                    TraceInf("RAW PERF msg for group %d , Agent %08x (%s) sent to GUI. tid=0x%08x \n",(*rgit).first,iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;

                    //indicate this consumer (thread , app.) has processed this data
                    monitData.clrDataTypeForConsumer(RAW_PERF_DATA_INDEX,dwConsumerID);
                }
                else
                {
                    TraceAll("Not sending RAW PERF for group %d because no update rx from Agent %08x (%s). tid=0x%08x \n",(*rgit).first,iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;
                }
            }
            else
            {
                TraceAll("Not sending RAW PERF for group %d, Agent %08x (%s) because not requested by GUI, tid=0x%08x \n",(*rgit).first,iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId() ) ;
            }

            rgit++;//next RepGroup of server/Agent
        }
        agentit++;//next Tdmf server/Agent
    }
    return true;
}

// ***********************************************************
// Function name	: dbserver_agent_monit_send_status_mode_data
// Description	    : 
// Return type		: int 
// Argument         : sock_t* monitorSocket
// 
// ***********************************************************
bool dbserver_agent_monit_send_status_mode_data(sock_t* monitorSocket, DWORD dwConsumerID, ConsumerDataFilter & filter, bool bForceSend )
{
    TDMFNotificationMessage             msg(NOTIF_MSG_RG_STATUS_AND_MODE);
    mnep_NotifMsgDataRepGrpStatusMode   msgData;
    mnep_NotifMsgDataServerInfo         msgServerInfo;//host id

    //send performance data message to consumer
    AgentMonitorMap::iterator agentit  = gmapAgentMonitor.begin();
    AgentMonitorMap::iterator agentend = gmapAgentMonitor.end();
    //scan each server
    while( agentit != agentend )
    {
        AgentMonitor & agent = (*agentit).second;
        int iHostID          = ((*agentit).first).getHostID();
        //consumer selection for this server
        ServerDataFilter & filterServer = filter.m_mapServer[iHostID];


        //when Agent is down (not alive) there is no point sending 
        //data for ALL RGroups.  just send one status msg for this TDMF Agent.
        if ( ! agent.IsAlive() )
        {
            //was State info requested for this Server ?
            if ( filterServer.m_bStateStats && filterServer.NeedToSendNOTAliveMsg() )
            {
                TDMFNotificationMessage msg(NOTIF_MSG_SERVER_NO_ACTIVITY);
                msgServerInfo.agentUID.iHostID = htonl(iHostID);
                msgServerInfo.agentUID.tStimeTamp = htonl( time(0) );
				msgServerInfo.nBABAllocated = htonl(agent.m_nBABSizeAllocated);
				msgServerInfo.nBABRequested = htonl(agent.m_nBABSizeRequested);
                msg.setDataExt(sizeof(msgServerInfo),(char*)&msgServerInfo);
                if ( !msg.Send(monitorSocket) )
                    return false;

                filterServer.AgentIsNOTAlive();

                TraceInf("NOT ALIVE msg for Agent %08x (%s) sent to GUI. tid=0x%08x\n",iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;
            }
            //process next TDMF server...
        }
        else
        {   
            //confirm this Server is alive, only if info was requested for this Server.
			if ( filterServer.m_bStateStats &&
				(filterServer.NeedToSendAliveMsg() || 
				 (msgServerInfo.nBABAllocated != filterServer.m_nPrevBABAllocated) ||
				 (msgServerInfo.nBABRequested != filterServer.m_nPrevBABRequested)))
            {
                TDMFNotificationMessage alivemsg(NOTIF_MSG_SERVER_ACTIVITY);
                msgServerInfo.agentUID.iHostID = htonl(iHostID);
                msgServerInfo.agentUID.tStimeTamp = htonl( time(0) );
				msgServerInfo.nBABAllocated = htonl(agent.m_nBABSizeAllocated);
				msgServerInfo.nBABRequested = htonl(agent.m_nBABSizeRequested);
                alivemsg.setDataExt(sizeof(msgServerInfo),(char*)&msgServerInfo);
                if ( !alivemsg.Send(monitorSocket) )
                    return false;

                filterServer.AgentIsAlive();
				filterServer.m_nPrevBABAllocated = msgServerInfo.nBABAllocated;
				filterServer.m_nPrevBABRequested = msgServerInfo.nBABRequested;

                TraceInf("ALIVE msg for Agent %08x (%s) sent to GUI. tid=0x%08x\n",iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;
            }

            //
            //scan each Rep.Group of one server
            //
            LGMonitorMap::iterator rgit     = agent.m_mapLG.begin();
            LGMonitorMap::iterator rgitend  = agent.m_mapLG.end();
            while( rgit != rgitend )
            {
                //was State info requested for this group ?
                if ( filterServer.m_mapGroup[(*rgit).first].m_bStateStats )
                {
                    //get a REFERENCE on the RepGroupMonitorData obj in the map
                    RepGroupMonitorData & monitData = ((*rgit).second);
                    //check if this RG data has been updated since last call
                    if ( monitData.isDataTypeSetForConsumer(STATUS_AND_MODE_DATA_INDEX,dwConsumerID) ||
                         bForceSend )
                    {
                        msgData.agentUID.iHostID = iHostID;
                        msgData.agentUID.tStimeTamp = monitData.getMostRecentTimeStamp();

                        //get latest performance data for this RG
                        msgData.sConnection     = monitData.getConnection();
                        msgData.sRepGroupNumber = monitData.getRGNumber();

                        msgData.sState          = monitData.getDrvMode();
                        
						if ( monitData.isGroupStarted() ) 
							msgData.sState |=  0x0100;//set group started flag
						else 
							msgData.sState &= ~0x0100;//clr group started flag
						if ( monitData.isGroupInCheckPoint() )
							msgData.sState |=  0x0200;//set group cp on flag
						else 
							msgData.sState &= ~0x0200;//clr group cp on flag

                        msgData.sPctDone        = monitData.getPctDone();

                        msgData.liJournalDiskFreeSz   = monitData.m_iJournalDiskFreeSz;
                        msgData.liJournalDiskTotalSz  = monitData.m_iJournalDiskTotalSz;
                        msgData.liJournalSz           = monitData.m_iJournalSz;
                        msgData.liPStoreDiskFreeSz    = monitData.m_iPStoreDiskFreeSz;
                        msgData.liPStoreDiskTotalSz   = monitData.m_iPStoreDiskTotalSz;
                        msgData.liPStoreSz            = monitData.m_iPStoreSz;

                        //convert to network byte order
                        mnep_convert_hton(&msgData);

                        msg.setDataExt(sizeof(msgData),(char*)&msgData);
                        if ( !msg.Send(monitorSocket) )
                            return false;

                        TraceInf("STATUS-MODE msg for group %d , Agent %08x (%s) sent to GUI. tid=0x%08x\n",(*rgit).first,iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;

                        //indicate this consumer (thread , app.) has processed this data
                        monitData.clrDataTypeForConsumer(STATUS_AND_MODE_DATA_INDEX,dwConsumerID);
                    }
                    else
                    {
                        TraceAll("Not sending STATUS-MODE for group %d , Agent %08x (%s) because no update rx from Agent. tid=0x%08x\n",(*rgit).first,iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;
                    }
                }
                else
                {
                    TraceAll("Not sending STATUS-MODE for group %d , Agent %08x (%s) because not requested by GUI. tid=0x%08x\n",(*rgit).first,iHostID,dbserver_db_GetNameFromHostID(iHostID),GetCurrentThreadId()) ;
                }
                rgit++;//next RG of current server
            }
        }
        agentit++;//next Tdmf server
    }
    return true;
}


void dbserver_agent_monit_initialize()
{
    MonitoringDataConsumers *pConsumer = gAllConsumers;
    for( int i=0;i < MAX_CONSUMERS; i++, pConsumer++)
    {
        pConsumer->dwThreadId   = INVALID_TID;
        pConsumer->hEvent       = NULL;
		// GUI Msg forwarding
		pConsumer->hEventGuiMsg = NULL;
    }
}

// GUI Msg forwarding
void dbserver_agent_monit_fowardGuiMsg(sock_t *monitorSocket)
{
    DWORD   tid = GetCurrentThreadId();
    TraceInf("dbserver_agent_monit_fowardGuiMsg IN  tid=0x%08x \n",tid);

    mmp_TdmfGuiMsg data;
    int toread = sizeof(data);
	int r  = mmp_mngt_sock_recv(monitorSocket, (char *)&data, toread);
    if ( r == toread )
    {
		// put gui msg in each thread msg queue
		all_consumers_add_gui_msg(data);
    }
}

bool dbserver_agent_monit_send_gui_msg_data(sock_t* monitorSocket, DWORD dwTID)
{
	mmp_TdmfGuiMsg GuiMsg;

	if (all_consumers_pop_gui_msg(GuiMsg, dwTID))
	{
		TDMFNotificationMessage msg(NOTIF_MSG_GUI_MSG);
		msg.setData(sizeof(GuiMsg),(char*)&GuiMsg);

		if ( !msg.Send(monitorSocket) )
		{
		    return false;
		}
	}

    return true;
}

bool dbserver_agent_monit_send_collector_stat(sock_t* monitorSocket, DWORD dwTID)  //rddev 030328
{
	mmp_TdmfCollectorState CollectorStateMsg;

	RetreiveCollectorStatisticsData(&CollectorStateMsg);

    TraceInf("dbserver_agent_monit_send_collector_stat tid=0x%08x Time:0x%08x \n", dwTID, CollectorStateMsg.CollectorTime);

	//	Don't send global structure if is only initialized 
	if (CollectorStateMsg.CollectorTime != 0)
	{
		CollectorStateMsg.CollectorTime = time(0); // Transmit the Collector Time instead of Collector Stats time		
		TDMFNotificationMessage msg(NOTIF_MSG_COLLECTOR_STAT);

		msg.setData(sizeof(CollectorStateMsg),(char *)&CollectorStateMsg);

		if ( !msg.Send(monitorSocket) )
		{
		    return false;
		}
   	}

    return true;
}

void dbserver_agent_monit_output_map()
{
	TraceInf("+++++  Agent Monitor Map  +++++\n");

	AgentMonitorMap::iterator it;
	for (it = gmapAgentMonitor.begin(); it != gmapAgentMonitor.end(); it++)
	{
		AgentIdentification AgentId = it->first;
		TraceInf("Agent ID = %08x    ", AgentId.getHostID());

		AgentMonitor AgentMon = it->second;
		TraceInf("Alive = %s \n", AgentMon.IsAlive() ? "Yes" : "No");

		LGMonitorMap::iterator itLG;
		for (itLG = AgentMon.m_mapLG.begin(); itLG != AgentMon.m_mapLG.end(); itLG++)
		{
			TraceInf("  Group Number = %d \n", itLG->first);

			RepGroupMonitorData RGData = itLG->second;

			//TraceInf("    Perf Data = ...\n");
			//ftd_perf_instance_t m_perf;
			//time_t              m_perfts;

			TraceInf("    Group      = %d \n", RGData.getRGNumber());
			TraceInf("    State      = %d \n", RGData.getDrvMode());
			TraceInf("    Connection = %d \n", RGData.getConnection());
			//mmp_TdmfGroupState	m_state;
			//time_t              m_statets;

			TraceInf("    Size = ...\n");
			//__int64  m_iPStoreDiskTotalSz,
			//	m_iPStoreDiskFreeSz,
			//	m_iPStoreSz;
			//__int64  m_iJournalDiskTotalSz,
			//	m_iJournalDiskFreeSz,
			//	m_iJournalSz;
		}
	}

	TraceInf("+++++++++++++++++++++++++++++++\n");
}
