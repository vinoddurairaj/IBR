/*
 * DBServer_AgentAliveSocket.cpp -   Handles communications initiated by TDMF Agents (PUSH data) 
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
#include <map>
#include <process.h>
extern "C" 
{
#include "sock.h"
}
#include "DBServer.h"

using namespace std;

#ifdef _COLLECT_STATITSTICS_
#define INC_ALIVE_AGENTS InterlockedIncrement(&g_nNbAliveAgents); 
#define DEC_ALIVE_AGENTS InterlockedDecrement(&g_nNbAliveAgents);
#else
#define INC_ALIVE_AGENTS
#define DEC_ALIVE_AGENTS
#endif


#define INVALID_HOST_ID        0
#define ALIVE_TIMEOUT          (MMP_MNGT_ALIVE_TIMEOUT * 3)      // MULTIPLY BY 3, in secs
#define ALIVE_THREAD_TIMEOUT   (MMP_MNGT_ALIVE_TIMEOUT * 1000)   // in milliseconds
#define NO_TIMEOUT             0

// ***********************************************************
// class definition
// 2 map of host id and socket, to be able to search both ways.

struct oneAgentSocket
{
    sock_t      *s;
    int         iHostID;
    WSAEVENT    hWSAEv;//signaled when socket is closed
    WSAEVENT    hWSAEvRx;//signaled when data has been TX by perr (Repl.Server) on socket 
    time_t      tLastAliveRx;
};

static map<int /*host id*/, struct oneAgentSocket> g_mapPushSocket;
static bool     g_bListOfWSAEventsChanged;
static HANDLE   g_hEvAgentAliveSocketThreadEnd;
static HANDLE   g_hAgentAliveSocketThread;
static HANDLE   g_hMutexMap;

static unsigned int g_uiAliveTimeout         = ALIVE_TIMEOUT;
static unsigned int g_uiAliveThreadTimeout   = ALIVE_THREAD_TIMEOUT;

// ***********************************************************
// local definitions
#define LOCK_PUSH_SOCKET_MAP    WaitForSingleObject(g_hMutexMap,INFINITE);
#define UNLOCK_PUSH_SOCKET_MAP  ReleaseMutex(g_hMutexMap);


// ***********************************************************
// local prototypes
static unsigned int __stdcall AgentAliveSocketThread(void* pContext);

// ***********************************************************
// Function name	: 
// Description	    :   
// Return type		: 
// 
// ***********************************************************
void AgentAliveSetAliveTimeout(unsigned int Timeout)
{
    g_uiAliveTimeout        = Timeout;   
}

void AgentAliveSetThreadAliveTimeout( unsigned int Timeout)
{
    g_uiAliveThreadTimeout  = Timeout;
}

void AgentAliveSocketInitialize()
{
    g_hEvAgentAliveSocketThreadEnd = CreateEvent(0,0,0,0);
    g_hMutexMap = CreateMutex(0,0,0);
    unsigned int tid;
    g_hAgentAliveSocketThread = (HANDLE)_beginthreadex(NULL,0,AgentAliveSocketThread,0,0,&tid);_ASSERT(g_hAgentAliveSocketThread!=0);
}

void AgentAliveSocketClose()
{
    SetEvent(g_hEvAgentAliveSocketThreadEnd);
    WaitForSingleObject(g_hAgentAliveSocketThread,10000);
    CloseHandle(g_hMutexMap);
    CloseHandle(g_hEvAgentAliveSocketThreadEnd);
    CloseHandle(g_hAgentAliveSocketThread);
}

void AgentAliveSetTimeStamp(int iHostID, time_t tmMostRecentTimeStamp)
{
    map<int /*host id*/, struct oneAgentSocket>::iterator it,end;

    LOCK_PUSH_SOCKET_MAP
    it = g_mapPushSocket.find(iHostID);
    if ( it != g_mapPushSocket.end() )
    {
        struct oneAgentSocket & agentSocket = (*it).second;

		if ( agentSocket.tLastAliveRx <  tmMostRecentTimeStamp)
		{
			//update timestamp
			agentSocket.tLastAliveRx = tmMostRecentTimeStamp;
		}
    }
	else
	{	//the code below allows Common Console to report EMULATED Agents as Alive.
		/*
		if ( (unsigned)iHostID <= (unsigned)0x0fff )
		{	//this is from an EMULATED TDMF Agent.
			//EMULATED TDMF Agents do not have a Alive Socket ...
			//Notifications : signal that Agent is alive.
			dbserver_agent_monit_UpdateAgentAlive(true, &iHostID, HOST_ID_NUMERIC);
			dbserver_db_update_server_state(iHostID, AGENT_STATE_ALIVE);
		}
		*/
    }
    UNLOCK_PUSH_SOCKET_MAP
}

// ***********************************************************
// Function name	: 
// Description	    :   
// Return type		: 
// 
// ***********************************************************
static void AgentAliveReadAliveMsg(sock_t *s, int iHostID, const mmp_mngt_TdmfAgentAliveMsg_t *pmsg)
{
    //read information tx by Agent, if any. allow 0.5 second to rx info
    //some earlier Agent version might not send any data.
    int iMsgSize = ntohl(pmsg->iMsgLength);

    if ( iMsgSize <= 0 || iMsgSize > 4096 )//just make sure we get a valid number ...
        return;

    //read number of bytes in msg
    int iRecvd;
    char *pMsgData = new char [ iMsgSize ];

    if ( (iRecvd = sock_recv(s,pMsgData,iMsgSize)) <= 0 )
    {
        DBGPRINT( (stdout,"\nAlive Socket : Error in Alive msg rx from host 0x08x : expecting %d bytes, recvd %d bytes.\n",iHostID,iMsgSize,iRecvd) );
        delete [] pMsgData;
        return;
    }

    //try to find any of the known tag-value pairs ...
    char *p;
    if ( (p=strstr(pMsgData, MMP_MNGT_ALIVE_TAG_MMPVER)) != NULL )
    {   //version of the MMP message protocol, increments at each modification of the protocol
        int mmp_version = atoi( p + strlen(MMP_MNGT_ALIVE_TAG_MMPVER) );
        if ( mmp_version != MMP_PROTOCOL_VERSION )
        {
            TraceWrn("********************************************************************");
            TraceWrn("MMP protocol version mismatch: TDMF Replication Server HostID=0x%08x, <%s>, reports version %d, this Collector uses %d.", 
                iHostID, s->rhostname == NULL ? "" : s->rhostname, mmp_version, MMP_PROTOCOL_VERSION);
            TraceWrn("TDMF Replication Server version should match TDMF Collector version.");
            TraceWrn("********************************************************************");
        }
    }

    delete [] pMsgData;
}

// ***********************************************************
// Function name	: 
// Description	    :   
// Return type		: 
// 
// ***********************************************************
void AgentAliveSocketAdd(sock_t* s, int iHostID, const mmp_mngt_TdmfAgentAliveMsg_t *pmsg)
{
    LOCK_PUSH_SOCKET_MAP
    if ( g_mapPushSocket.find( iHostID ) == g_mapPushSocket.end() )
    {   //not found, Agent is connecting for the first time
        struct oneAgentSocket agentSocket;  
        int r;

        AgentAliveReadAliveMsg(s,iHostID,pmsg);

        agentSocket.hWSAEv   = WSACreateEvent();
        //agentSocket.hWSAEvRx = WSACreateEvent();
        agentSocket.s        = s;
        agentSocket.iHostID  = iHostID;
        agentSocket.tLastAliveRx = time(0);//now
        //to have agentSocket.hWSAEv signaled when socket closes
        r = WSAEventSelect( s->sock, agentSocket.hWSAEv, FD_CLOSE | FD_READ );
        //to have agentSocket.hWSAEvRx signaled when data recvd on socket 
        //r = WSAEventSelect( s->sock, agentSocket.hWSAEvRx, FD_READ );
        
        //Notifications : signal that Agent is alive.
        dbserver_agent_monit_UpdateAgentAlive(true, &iHostID, HOST_ID_NUMERIC);
        dbserver_db_update_server_state(iHostID, AGENT_STATE_ALIVE);

        g_mapPushSocket[ iHostID ] = agentSocket;
        //to have thread rebuild its event vector
        g_bListOfWSAEventsChanged = true;

        TraceInf("Alive Socket : Add TDMF Client 0x%08x (%s)...",iHostID,dbserver_db_GetNameFromHostID(iHostID));
    }

    UNLOCK_PUSH_SOCKET_MAP

    INC_ALIVE_AGENTS
}

static map<int /*host id*/, struct oneAgentSocket>::iterator 
AgentAliveSocketRemove(map<int /*host id*/, struct oneAgentSocket>::iterator it)
{
    map<int /*host id*/, struct oneAgentSocket>::iterator itRetValue;
    LOCK_PUSH_SOCKET_MAP

    if ( it != g_mapPushSocket.end() )
    {
        map<int /*host id*/, struct oneAgentSocket>::iterator next_it;
        struct oneAgentSocket & agentSocket = (*it).second;
        int iHostID = agentSocket.iHostID;

        //Notifications : signal that Agent is gone.
        dbserver_agent_monit_UpdateAgentAlive(false, &iHostID, HOST_ID_NUMERIC);
        dbserver_agent_monit_SetConnectState(false, &iHostID, HOST_ID_NUMERIC);
        dbserver_db_update_server_state(iHostID, AGENT_STATE_NOT_ALIVE);


        BOOL b = WSACloseEvent( agentSocket.hWSAEv );_ASSERT(b);
        sock_disconnect(agentSocket.s);
        sock_delete(&agentSocket.s);

        next_it = g_mapPushSocket.erase( it );
        //to have AgentAliveSocketThread() thread rebuild its event vector
        g_bListOfWSAEventsChanged = true;

        TraceInf("Alive Socket : Remove TDMF Client 0x%08x (%s)...",iHostID,dbserver_db_GetNameFromHostID(iHostID));

        itRetValue = next_it;
    }
    else
    {
        itRetValue = g_mapPushSocket.end();
    }

    UNLOCK_PUSH_SOCKET_MAP

    DEC_ALIVE_AGENTS

    return itRetValue;
}

void AgentAliveSocketRemove(int iHostID)
{
    map<int /*host id*/, struct oneAgentSocket>::iterator it;

    LOCK_PUSH_SOCKET_MAP
    it = g_mapPushSocket.find( iHostID );
    if ( it != g_mapPushSocket.end() )
    {
        AgentAliveSocketRemove(it);
    }
    UNLOCK_PUSH_SOCKET_MAP
}


static bool 
AgentAliveSocketCheckTimeStamp(void)
{
	time_t now = time(0);
    map<int /*host id*/, struct oneAgentSocket>::iterator it,end;
	bool bOneAgentDisconnected = false;

#if USE_ALIVE_TRACE
	ColDebugTrace("AgentAliveSocketCheckTimeStamp\n");
#endif

    ////////////////////////////////////////////////////
    //checking all Repl.Servers Alive timestamp
    ////////////////////////////////////////////////////
    //TraceAll("Alive socket: checking all Repl.Servers Alive timestamp...");
    LOCK_PUSH_SOCKET_MAP
    end = g_mapPushSocket.end();
    it  = g_mapPushSocket.begin();
    while( it != end )
    {
        struct oneAgentSocket & agentSocket = (*it).second;
        if ( now - agentSocket.tLastAliveRx > (int)g_uiAliveTimeout )
        {   //this repl. server looks to be down.
            TraceAll("CheckTimeStamp: Repl.Server HostID=0x%08x (%s) has not reported for %d seconds." ,agentSocket.iHostID,dbserver_db_GetNameFromHostID(agentSocket.iHostID), now - agentSocket.tLastAliveRx );
            it = AgentAliveSocketRemove(it) ;
			bOneAgentDisconnected = true;
        }
        else
            it++;
    }
    UNLOCK_PUSH_SOCKET_MAP

	return bOneAgentDisconnected;
}

/**
 * read any Alive msg message received from peer (a Repl.Server) 
 */
static void
AliveSocketReadMsg(int iHostID) 
{
    time_t now = time(0);
    map<int /*host id*/, struct oneAgentSocket>::iterator it,end;

    LOCK_PUSH_SOCKET_MAP
    it = g_mapPushSocket.find(iHostID);
    if ( it != g_mapPushSocket.end() )
    {
        struct oneAgentSocket & agentSocket = (*it).second;
        int r;

        //1. read MMP header
        int magicNumber;
        //read message header: first the check management type 
        r = sock_recv(agentSocket.s,(char*)&magicNumber,sizeof(int));
        if ( r == sizeof(int) )
        {
            if ( ntohl(magicNumber) == MNGT_MSG_MAGICNUMBER )
            {   //management protocol 
                mmp_mngt_header_t   hdr = {0,0,0,0};
                //read remainder of header 
                r = sock_recv(agentSocket.s,(char*)(&hdr.magicnumber + 1), sizeof(hdr) - sizeof(int) );
                mmp_convert_mngt_hdr_ntoh(&hdr);//to host byte order
                //hdr.magicnumber = MNGT_MSG_MAGICNUMBER;//to host byte order (could not be init. above)

                //2. read specific msg received through Alive Socket...
                if ( hdr.mngttype == MMP_MNGT_AGENT_ALIVE_SOCKET )
                {   //Repl.Server just sends its periodic Alive msg.
                    mmp_mngt_TdmfAgentAliveMsg_t    alivemsg;
                    mmp_mngt_header_t               *phdr = (mmp_mngt_header_t*)&alivemsg;
                    r = sock_recv(agentSocket.s,(char*)(phdr + 1), sizeof(mmp_mngt_TdmfAgentAliveMsg_t) - sizeof(mmp_mngt_header_t) );
                    //update timestamp
                    agentSocket.tLastAliveRx = now;
                    TraceAll("Alive socket: ReplServer HostID=0x%08x (%s) reports at t=%d." ,agentSocket.iHostID,dbserver_db_GetNameFromHostID(agentSocket.iHostID),now);
#ifdef _COLLECT_STATITSTICS_
                    g_nNbAliveMsg++;
#endif
                }
            }
        }

        ////////////////////////////////////////////////////
        //empty socket : make sure to remove any bytes left unread
        if ( sock_check_recv(agentSocket.s, 0)  )
        {   //empty the socket read buffer by chunks of 128 bytes or less...
            char garbage[128];
            int read   = 0;
            int toread = (int)tcp_get_numbytes(agentSocket.s->sock);
            TraceAll("Emptying Alive socket RX buffer of HostID=0x%x (%s)",agentSocket.iHostID,dbserver_db_GetNameFromHostID(agentSocket.iHostID));
            do
            {
                read = mmp_mngt_sock_recv(agentSocket.s,garbage, (toread > 128 ? 128 : toread) );
                TraceAll("Alive socket RX buffer of HostID=0x%x (%s) , rx %d bytes" ,agentSocket.iHostID,dbserver_db_GetNameFromHostID(agentSocket.iHostID),read);
                if ( read > 0 )
                    toread -= read;
            }
            while( toread > 0 && read > 0 );
            TraceAll("Alive socket RX buffer of HostID=0x%x (%s) now empty." ,agentSocket.iHostID,dbserver_db_GetNameFromHostID(agentSocket.iHostID));
        }

        WSAResetEvent( agentSocket.hWSAEv );
    }

    ////////////////////////////////////////////////////
    //checking all Repl.Servers Alive timestamp
    ////////////////////////////////////////////////////
    //TraceAll("Alive socket: checking all Repl.Servers Alive timestamp...");
//    end = g_mapPushSocket.end();
//    it  = g_mapPushSocket.begin();
//    while( it != end )
//    {
//        struct oneAgentSocket & agentSocket = (*it).second;
//        if ( now - agentSocket.tLastAliveRx > g_uiAliveTimeout )
//        {   //this repl. server looks to be down.
//            TraceAll("Alive socket: Repl.Server HostID=0x%08x has not reported for %d seconds." ,agentSocket.iHostID, now - agentSocket.tLastAliveRx );
//            it = AgentAliveSocketRemove(it) ;
//        }
//        else
//            it++;
//    }

    UNLOCK_PUSH_SOCKET_MAP
}

// ***********************************************************
// Function name	: AgentAliveSocketThread 
// Description	    : The sole purpose of this thread is to 
//                    perform specific actions when one of the 
//                    monitored sockets closes.
// Return type		: 
// 
// ***********************************************************
static unsigned int __stdcall AgentAliveSocketThread(void* pContext)
{
    bool        bAlive = true;    
    WSAEVENT    *phSocketEvents = 0, *phWork;
    struct oneAgentSocket   *pAgentSockets = 0, *pAgentWork;
    int         nSyncObjects;

#ifdef _COLLECT_STATITSTICS_
    g_nNbAliveMsg = 0;
#endif

    while( bAlive )
    {
        //just sit and wait until the map contains Agent(s)
        while( g_mapPushSocket.size() == 0 && bAlive)
        {
            bAlive = (WAIT_TIMEOUT == WaitForSingleObject(g_hEvAgentAliveSocketThreadEnd,NO_TIMEOUT));
            Sleep(1000);    
        }
        if (!bAlive)
            break;//exit main while loop.

        //
        // at this point, nSyncObjects > 0 
        //
        LOCK_PUSH_SOCKET_MAP

        nSyncObjects = g_mapPushSocket.size();
        //build a vector of WSAEVENTs and a vector of corresponding oneAgentSocket strcutures.
        phWork      = phSocketEvents  = new WSAEVENT[ nSyncObjects ];
        pAgentWork  = pAgentSockets   = new struct oneAgentSocket[ nSyncObjects ];

        map<int /*host id*/, struct oneAgentSocket>::iterator it  = g_mapPushSocket.begin();
        map<int /*host id*/, struct oneAgentSocket>::iterator end = g_mapPushSocket.end();
        while( it != end )
        {
            struct oneAgentSocket & agentSocket = (*it).second;
            *pAgentWork = agentSocket;
            *phWork     = agentSocket.hWSAEv;
            pAgentWork ++;
            phWork ++;
            it++;
        }
        //vector is up-to-date, reset rebuild flag
        g_bListOfWSAEventsChanged = false;
        UNLOCK_PUSH_SOCKET_MAP
        
        //wake-up periodically to make sure WSAEvent vector is still up-to-date
        DWORD status;
        bool bOneAgentDisconnected = false;
        do
        {   //wait until one of the sockets closes (or timeout occurs)
            status = WSAWaitForMultipleEvents(nSyncObjects,phSocketEvents,FALSE,g_uiAliveThreadTimeout,FALSE);

            if ( (status >= WSA_WAIT_EVENT_0) && (status < (WSA_WAIT_EVENT_0 + nSyncObjects)) )
            {
                WSANETWORKEVENTS NetworkEvents;
                pAgentWork = pAgentSockets + (status - WSA_WAIT_EVENT_0);
                if ( 0 == WSAEnumNetworkEvents(pAgentWork->s->sock,phSocketEvents[status - WSA_WAIT_EVENT_0],&NetworkEvents) )
                {
                    if ( NetworkEvents.lNetworkEvents & FD_READ )
                    {   //data available to be read on one Alive socket
                        AliveSocketReadMsg( pAgentWork->iHostID );
                        //remain in do - while loop unless AgentAliveSocketRemove() was called within AliveSocketReadMsg()
                    }
                    else 
                    if ( NetworkEvents.lNetworkEvents & FD_CLOSE )
                    {   //a peer has closed its Alive socket can't detect any remote failure.
                        AgentAliveSocketRemove( pAgentWork->iHostID );
                        //leave loop to process disconnection and reorganize Events and Data vectors
                        bOneAgentDisconnected = true;
                    }
                    else
                        ASSERT(0);
                }
                else
                    ASSERT(0);
            }
            
	        // Socket failure detected ??
	        if ( AgentAliveSocketCheckTimeStamp() ) 
	        {
                // No it's not the Shthate shthime shthamp
	            bOneAgentDisconnected = true;
	        }

            //check end thread event state
            bAlive = (WAIT_TIMEOUT == WaitForSingleObject(g_hEvAgentAliveSocketThreadEnd,0));
        } 
        while ( bOneAgentDisconnected == false      && 
                g_bListOfWSAEventsChanged == false  &&
                bAlive == true );

        //a socket has closed, rebuild list of sockets to check on.
        delete [] phSocketEvents;
        delete [] pAgentSockets;

    }//forever

    return 0;
}

