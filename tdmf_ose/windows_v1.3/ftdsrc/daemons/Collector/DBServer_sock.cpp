/*
 * DBServer_sock.cpp -   Handles socket communications with clients and TDMF Agents 
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


//Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
//#include <afx.h>
#ifdef _DEBUG
#include <stdio.h>
#include <conio.h>
#endif
#include <time.h>
extern "C" {
#include "ftd_sock.h"
#include "sock.h"
#include "iputil.h"
}
#include "libmngt.h"
#include "libmngtmsg.h"
#include "DBServer.h"
#include <map>

#define DBSERVER_BROADCAST_TIMER_EVENT  "DBSERVER_BROADCAST_TIMER_EVENT\0"

//must be set to 1.  Using CreateThread() causes memory leaks.
#define USE_BEGINTHREADEX   1
#define MMP_MNGT_WATCHDOG_MUTEX     "MMP_MNGT_WATCHDOG_MUTEX\0"
#define SOCKET_RX_CHECK_TIMEOUT     500   // 1/2 sec timeout

//defined in WinService.cpp
extern HANDLE   ghTerminateEvent;

#ifdef _COLLECT_STATITSTICS_
#define INC_THRD_RUNNING InterlockedIncrement(&g_nNbThreadsRunning);g_nNbThrdCreated++; 
#define DEC_THRD_RUNNING InterlockedDecrement(&g_nNbThreadsRunning)
#else
#define INC_THRD_RUNNING
#define DEC_THRD_RUNNING
#endif

// ***********************************************************
// local definitions
/**
 * provided as context to the dbserver_sock_process_request function (thread)
 */
class ThreadContext
{
public:
    ThreadContext( CollectorConfig  *cfg, sock_t *request, mmp_mngt_header_t *hdr )
        :m_cfg(cfg)
        ,m_request(request)
        {
            if (hdr)
                m_mngtmsghdr = *hdr;
        };

    CollectorConfig     *m_cfg;
    sock_t              *m_request;
    mmp_mngt_header_t   m_mngtmsghdr;

};


// ***********************************************************
// local prototypes
//static DWORD WINAPI dbserver_sock_BroadcastThread(void* Context);
#if USE_BEGINTHREADEX
static unsigned int __stdcall   dbserver_sock_process_request(void* context);
#else   
static DWORD WINAPI             dbserver_sock_process_request(void* context);
#endif
#ifdef TDMF_ENABLE_BROADCAST
static void         dbserver_sock_manage_broadcast_req  (CollectorConfig* cfg, sock_t *request );
#endif
static void         dbserver_sock_registration_key_req  (CollectorConfig* cfg, sock_t* request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_registration_key_from_agent(sock_t* request);
static void         dbserver_sock_manage_broadcast_response(CollectorConfig* cfg, sock_t *agentResponse);
static int          dbserver_sock_sendmsg_to_TdmfAgent  (unsigned long rip, unsigned long rport, const char* senddata, int sendlen, char* rcvdata, int rcvsize);
static void         dbserver_sock_get_lg_cfg_req        (CollectorConfig* cfg, sock_t *request );
static void         dbserver_sock_get_tdmf_file_req     (CollectorConfig* cfg, sock_t *request );
static void         dbserver_sock_set_lg_cfg_req        (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_sendfile_req          (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_tdmf_cmd_req          (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_get_all_devices       (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_recv_alert            (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_recv_status           (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_recv_perf             (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_set_perf_cfg          (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_agent_general_config  (CollectorConfig* cfg, sock_t *request, int iMngtMsgType);
static bool         dbserver_sock_recv_agent_alive      (sock_t* request);
static void         dbserver_sock_recv_group_state      (CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr);
static void         dbserver_sock_recv_group_monitoring (sock_t *request);
static void         dbserver_sock_recv_tdmfcommontool_registration(sock_t *request);
static void         dbserver_sock_recv_collector_params (bool isSet, CollectorConfig* cfg, sock_t *request);
static void         dbserver_sock_recv_agent_state      (sock_t *request);

static int          dbserver_sock_sendmsg_recv_cfg(unsigned long rip, unsigned long rport, 
                                                   const char* senddata, int sendlen, 
                                                   mmp_mngt_ConfigurationMsg_t ** ppRcvdata );

static int          dbserver_sock_sendmsg_recv_file(unsigned long rip, unsigned long rport, 
                                                    const char* senddata, int sendlen, 
                                                    mmp_mngt_FileMsg_t ** ppRcvdata );

static bool         dbserver_get_agent_ip_port(const char *szAgentId, unsigned long *agentIP, unsigned int *agentPort);
static void         dbserver_get_agent_hostid(/*in*/const char *szAgentId, /*out*/char *szHostId);

static bool         dbserver_sock_send_ftd_header(sock_t *request);
static void         dbserver_mmp_dump( const char* data, int size, bool bMsgRcvd, bool bAgentIsPeer, const char* szHostId, const char *szMMPmsgType );

// ***********************************************************
static CollectorConfig* g_cfg;

static void HandleIncomingRequest(sock_t* listener, CollectorConfig* cfg)
{
	sock_t* request = sock_create();
	bool bRequestSocketUsed = false;
	int  r;

	r = sock_accept(listener,request); DBGPRINTCOND(r<0,(stderr,"sock_accept failed (err=%d)!",r));
	if ( r >= 0 )
	{
		int magicNumber;
		//read message header: first the check management type 
		r = mmp_mngt_sock_recv(request,(char*)&magicNumber,sizeof(int));
		if ( r == sizeof(int) )
		{
			magicNumber = ntohl(magicNumber);

			if ( magicNumber == MNGT_MSG_MAGICNUMBER )
			{   //management protocol 

				mmp_mngt_header_t   hdr = {0,0,0,0};
				//read remainder of header 
				r = mmp_mngt_sock_recv(request,(char*)(&hdr.magicnumber + 1), sizeof(hdr) - sizeof(hdr.magicnumber) );

				mmp_convert_mngt_hdr_ntoh(&hdr);//to host byte order
				hdr.magicnumber = MNGT_MSG_MAGICNUMBER;//to host byte order (could not be init. above)

				//dispatch this request to a dedicated thread
				//pContext memory released within thread ( dbserver_sock_process_request() )
				ThreadContext *pContext = new ThreadContext( cfg, request, &hdr );
				unsigned int tid,cnt=0;
				HANDLE hThread;
				DWORD LastError = 0;
				int   iErrno = errno;

				cnt = 0;
				hThread = 0;
				while( !hThread && ++cnt < 5)
				{
					hThread = (HANDLE)_beginthreadex(NULL,0,dbserver_sock_process_request,pContext,0,&tid);
					iErrno  = errno;
					if (!hThread)
					{
						TraceWrn("WARNING! _beginthreadex() failed (0x%08x), retrying...!\n",errno);
						Sleep(5);
					}
				}

				if (!hThread)
				{
#ifdef _DEBUG
					TraceAll("*******************************************************\n");
					TraceAll("*****Unable to start thread because 0x%08x!\n",errno);
					TraceAll("*******************************************************\n");
#else
					TraceErr("**** Unable to start thread (0x%08x)\n",errno);
#endif
					_ASSERT(0);
				}
				else
				{
#ifdef _COLLECT_STATITSTICS_
					INC_THRD_RUNNING;
#endif
					::CloseHandle(hThread);
					bRequestSocketUsed = true;
				}
			}
			else
			{   
				TraceCrit("Collector Socket: Msg header check, bad MNGT_MSG_MAGICNUMBER ");                            
				//_ASSERT(0);
			}
		}
	}
	if ( !bRequestSocketUsed )
	{   //if socket not used, just delete it.
		sock_delete(&request);
	}
}

// ***********************************************************
// Function name    : dbserver_sock_dispatch_io
// Description      : main listener socket of TDMF Collector.
//                    This functions exposes socket port 
//                    and receives ALL management protocol messages,
//                    from Applications and TDMF Agents.  
// Return type      : int 
// 
// ***********************************************************
void 
dbserver_sock_dispatch_io( CollectorConfig* cfg )
{
    bool bNoError;
    bool bRequestSocketUsed;
    int  r;

    //init required network components
    if ( sock_startup() < 0 )
        return;

    dbserver_agent_monit_initialize();
    AgentAliveSocketInitialize();

	HANDLE hWatchdogMutex = CreateMutex(0,0,MMP_MNGT_WATCHDOG_MUTEX);

    g_cfg = cfg;//g_cfg used only by dbserver_dispatch_accepted_connection()

    //create listener sockets
    sock_t* listener = sock_create();
	sock_t* listener2 = sock_create();

    r = sock_init( listener, NULL, NULL, cfg->ulIP, 0, SOCK_STREAM, AF_INET, 1, 0);
	if (r >= 0)
	{
		r = sock_init( listener2, NULL, NULL, cfg->ulIP, 0, SOCK_STREAM, AF_INET, 1, 0);
	}
	TraceCrit("Collector Socket: Initialization(IP:%d.%d.%d.%d)", 
                  (cfg->ulIP & 0x000000ff),((cfg->ulIP & 0x0000ff00)>>8),
                  ((cfg->ulIP & 0x00ff0000)>>16), ((cfg->ulIP & 0xff000000)>>24));

    if ( r < 0 ) 
    {
        DBGPRINT((stderr,"Unable to init listener socket (err=%d)!",r));
        //cleanup
        sock_delete(&listener);
		sock_delete(&listener2);
        return; 
    }

    r = sock_listen(listener, cfg->iPort);
    TraceCrit("Collector Socket: Listen(Port %d)\n", cfg->iPort);
    if ( r >= 0 ) 
    {
		// listen on byte inverted port
		int nPort = htons(cfg->iPort);
	    r = sock_listen(listener2, nPort);
		TraceCrit("Collector Socket: Listen(Port %d)\n", nPort);
	}

    if ( r >= 0 ) 
    {
        bNoError = true;

#if DBG
        OutputDebugString("** [Collector DispatchIO starting] **\n");
#endif
        //////////////////////////////////////////
        //
        if (SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_LOWEST ))
        {
#if DBG
            OutputDebugString("Set DispatchIO Thread priority to THREAD_PRIORITY_LOWEST (-2)\n");
#endif
        }
        else
        {
#if DBG
            OutputDebugString("Set DispatchIO Thread priority to THREAD_PRIORITY_LOWEST failed!\n");
#endif
        }


        //listen for incoming connection from clients
        while( !gbDBServerQuit && bNoError )
        {
			bool bNewMessage = false;

            // Maintains the Collector Statistics
            AccumulateCollectorStats();

#if 0
            if (g_nNbMsgWaiting > 1000)
			{
                // give our quanta to someeone else
                //Sleep(0);
                // wait a little while
                Sleep(10*g_nNbMsgWaiting/1000);
			}
#endif
            if ( sock_check_recv(listener, SOCKET_RX_CHECK_TIMEOUT) ) //one sec time-out for connections
            {
                sock_t* request = sock_create();
                bRequestSocketUsed = false;

                r = sock_accept(listener,request); DBGPRINTCOND(r<0,(stderr,"sock_accept failed (err=%d)!",r));
                if ( r >= 0 )
                {
                    int magicNumber;
                    //read message header: first the check management type 
                    r = mmp_mngt_sock_recv(request,(char*)&magicNumber,sizeof(int));
                    if ( r == sizeof(int) )
                    {
                        magicNumber = ntohl(magicNumber);

                        if ( magicNumber == MNGT_MSG_MAGICNUMBER )
                        {   //management protocol 

                            mmp_mngt_header_t   hdr = {0,0,0,0};
                            //read remainder of header 
                            r = mmp_mngt_sock_recv(request,(char*)(&hdr.magicnumber + 1), sizeof(hdr) - sizeof(hdr.magicnumber) );

                            mmp_convert_mngt_hdr_ntoh(&hdr);//to host byte order
                            hdr.magicnumber = MNGT_MSG_MAGICNUMBER;//to host byte order (could not be init. above)

                            //dispatch this request to a dedicated thread
                            //pContext memory released within thread ( dbserver_sock_process_request() )
                            ThreadContext *pContext = new ThreadContext( cfg, request, &hdr );
                            unsigned int tid,cnt=0;
                            HANDLE hThread;
                            DWORD LastError = 0;
                            int   iErrno = errno;

                            cnt = 0;
                            hThread = 0;
                            while( !hThread && ++cnt < 5)
                            {
                                hThread = (HANDLE)_beginthreadex(NULL,0,dbserver_sock_process_request,pContext,0,&tid);
                                iErrno  = errno;
                                if (!hThread)
            {
                                    TraceWrn("WARNING! _beginthreadex() failed (0x%08x), retrying...!\n",errno);
                                    Sleep(5);
                                }
            }

                            if (!hThread)
			{
#ifdef _DEBUG
                                TraceAll("*******************************************************\n");
                                TraceAll("*****Unable to start thread because 0x%08x!\n",errno);
                                TraceAll("*******************************************************\n");
#else
                                TraceErr("**** Unable to start thread (0x%08x)\n",errno);
#endif
                                _ASSERT(0);
                            }
                            else
                            {
#ifdef _COLLECT_STATITSTICS_
                                INC_THRD_RUNNING;
#endif
                                ::CloseHandle(hThread);
                                bRequestSocketUsed = true;
                            }
                        }
                        else
                        {   
                             TraceCrit("Collector Socket: Msg header check, bad MNGT_MSG_MAGICNUMBER ");                            
                            //_ASSERT(0);
                        }
                    }
                }
                if ( !bRequestSocketUsed )
                {   //if socket not used, just delete it.
                    sock_delete(&request);
                }
			}

            gbDBServerQuit = ( WAIT_OBJECT_0 == WaitForSingleObject(ghTerminateEvent,0) );

        }//while
    }
    else
    {
        bNoError = false;
    }

    AgentAliveSocketClose();
	CloseHandle(hWatchdogMutex);

#ifdef _DEBUG
    DBGPRINT((stderr,"About to exit dbserver_sock_dispatch_io"));
#endif
    sock_delete(&listener);
    sock_cleanup();
}


// ***********************************************************
// Function name    : dbserver_sock_process_request
// Description      : Process message received from either Applications or TDMF Agents.
//                    This function is called within a thread dedicated to serve this request.
//                    The response, if any, should be sent to requester using the provided 
//                    STREAM (TCP) socket.
// Argument         : void* context
// 
// ***********************************************************
#if USE_BEGINTHREADEX
static unsigned int __stdcall  
#else
static DWORD WINAPI
#endif
dbserver_sock_process_request(void* context)
{
    ThreadContext* threadContext = (ThreadContext*)context;
    sock_t *request              = threadContext->m_request;
    CollectorConfig* cfg         = threadContext->m_cfg;
    mmp_mngt_header_t *hdr       = &threadContext->m_mngtmsghdr;
    bool bDeleteSocket           = true;
    DWORD tid                    = GetCurrentThreadId();

    TraceAll("Main Msg Socket: IN  tid=0x%08x, type=%s",tid, mmp_mngt_getMsgTypeText(hdr->mngttype) );

    if (g_bKeyValid ||
        (hdr->mngttype == MMP_MNGT_MONITORING_DATA_REGISTRATION) ||  // Only accept this msg if key is not valid
        (hdr->mngttype == MMP_MNGT_TDMFCOMMONGUI_REGISTRATION) ||
        (hdr->mngttype == MMP_MNGT_GET_DB_PARAMS) ||
        (hdr->mngttype == MMP_MNGT_REGISTRATION_KEY))
    {
    switch(hdr->mngttype)
    {
    case MMP_MNGT_SET_LG_CONFIG:      // receiving configuration data from client
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_SET_LG_CONFIG++;
#endif
		if ( hdr->sendertype == SENDERTYPE_TDMF_CLIENTAPP )
			dbserver_sock_set_lg_cfg_req(cfg,request,hdr);
		else 
			_ASSERT(0);
		break;

	case MMP_MNGT_GET_LG_CONFIG:        // request to get configuration data of a TDMF Agent
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_GET_LG_CONFIG++;
#endif
		dbserver_sock_get_lg_cfg_req(cfg,request);
		break;

#ifdef TDMF_ENABLE_BROADCAST
#pragma message(">>>>>>>>  Collector Broadcast functionality ENABLED  <<<<<<<<")
	case MMP_MNGT_AGENT_INFO_REQUEST:       // request for host information: IP, listener socket port, ...
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_AGENT_INFO_REQUEST++;
#endif
		if ( hdr->sendertype == SENDERTYPE_TDMF_CLIENTAPP )
			dbserver_sock_manage_broadcast_req(cfg,request);
		else 
			_ASSERT(0);//????
		break;
#endif
            
	case MMP_MNGT_AGENT_INFO:       // response from Agent to a MMP_MNGT_AGENT_INFO_REQUEST message
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_AGENT_INFO++;
#endif
		if ( hdr->sendertype == SENDERTYPE_TDMF_AGENT )
			dbserver_sock_manage_broadcast_response(cfg,request);
		else 
			_ASSERT(0);
		break;

	case MMP_MNGT_REGISTRATION_KEY:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_REGISTRATION_KEY++;
#endif
		if ( hdr->sendertype == SENDERTYPE_TDMF_CLIENTAPP )
			dbserver_sock_registration_key_req(cfg,request,hdr);
		else if ( hdr->sendertype == SENDERTYPE_TDMF_AGENT )
			//Agent PUSH its license key 
			dbserver_sock_registration_key_from_agent(request);
		else 
			_ASSERT(0);//????
		break;
            
	case MMP_MNGT_TDMF_CMD://from GUI
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_TDMF_CMD++;
#endif
		dbserver_sock_tdmf_cmd_req(cfg,request,hdr);
		break;

	case MMP_MNGT_SET_AGENT_GEN_CONFIG:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG++;
#endif
	case MMP_MNGT_GET_AGENT_GEN_CONFIG:
#ifdef _COLLECT_STATITSTICS_
		if (hdr->mngttype == MMP_MNGT_GET_AGENT_GEN_CONFIG)
			g_TdmfMessageStates.Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG++;
#endif
		dbserver_sock_agent_general_config(cfg,request,hdr->mngttype);
		break;

	case MMP_MNGT_SET_ALL_DEVICES:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_SET_ALL_DEVICES++;
#endif
		_ASSERT(0);
		break;

	case MMP_MNGT_GET_ALL_DEVICES://from GUI
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_GET_ALL_DEVICES++;
#endif
		dbserver_sock_get_all_devices(cfg,request,hdr);
		break;

	case MMP_MNGT_ALERT_DATA://from Agent
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_ALERT_DATA++;
#endif
		dbserver_sock_recv_alert(cfg,request,hdr);
		break;

	case MMP_MNGT_STATUS_MSG:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_STATUS_MSG++;
#endif
		dbserver_sock_recv_status(cfg,request,hdr);
		break;

	case MMP_MNGT_PERF_MSG:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_PERF_MSG++;
#endif
		dbserver_sock_recv_perf(cfg,request,hdr);
		break;

	case MMP_MNGT_PERF_CFG_MSG:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_PERF_CFG_MSG++;
#endif
		dbserver_sock_set_perf_cfg(cfg,request,hdr);
		break;

		// From GUI
	case MMP_MNGT_MONITORING_DATA_REGISTRATION:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION++;
#endif
		//this function returns only when socket is disconnected
		dbserver_agent_monit_monitorDataConsumer(request);
		break;

	case MMP_MNGT_AGENT_ALIVE_SOCKET:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_AGENT_ALIVE_SOCKET++;
#endif
		//this socket is kept opened as long as Agent is connected.
		if ( hdr->sendertype == SENDERTYPE_TDMF_AGENT )
			bDeleteSocket = dbserver_sock_recv_agent_alive(request);
		break;

	case MMP_MNGT_GROUP_STATE:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_GROUP_STATE++;
#endif
		dbserver_sock_recv_group_state(cfg,request,hdr);
		break;

	case MMP_MNGT_GROUP_MONITORING:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_GROUP_MONITORING++;
#endif
		dbserver_sock_recv_group_monitoring(request);
		break;

	case MMP_MNGT_TDMFCOMMONGUI_REGISTRATION:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION++;
#endif
		dbserver_sock_recv_tdmfcommontool_registration(request);
		break;

	case MMP_MNGT_SET_DB_PARAMS:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_SET_DB_PARAMS++;
#endif
	case MMP_MNGT_GET_DB_PARAMS:
#ifdef _COLLECT_STATITSTICS_
		if (hdr->mngttype == MMP_MNGT_GET_DB_PARAMS)
			g_TdmfMessageStates.Nb_MMP_MNGT_GET_DB_PARAMS++;
#endif
		dbserver_sock_recv_collector_params (hdr->mngttype == MMP_MNGT_SET_DB_PARAMS, cfg, request);
		break;

	case MMP_MNGT_AGENT_STATE:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_AGENT_STATE++;
#endif
		TraceWrn("Collector Process request: MMP_MNGT_AGENT_STATE msg received, cannot process!");
		break;

	case MMP_MNGT_GUI_MSG:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_MMP_MNGT_GUI_MSG++;
#endif
		dbserver_agent_monit_fowardGuiMsg(request);
		break;

	case MMP_MNGT_TDMF_SENDFILE:
		if ( hdr->sendertype == SENDERTYPE_TDMF_CLIENTAPP )
			dbserver_sock_sendfile_req(cfg,request,hdr);
		else 
			_ASSERT(0);
		break;

	case MMP_MNGT_TDMF_GETFILE:   // request to get TDMF File data of a TDMF Agent
		dbserver_sock_get_tdmf_file_req(cfg,request);
		break;

	default:
#ifdef _COLLECT_STATITSTICS_
		g_TdmfMessageStates.Nb_default++;
#endif
		TraceCrit("Collector Process request: Unexpected MMP_MNGT type %d !",hdr->mngttype);//_ASSERT(0);
		break;

	}
    }

    if ( bDeleteSocket )
        sock_delete(&request);

    delete threadContext;//dynamically allocated by caller

#ifdef _COLLECT_STATITSTICS_
    DEC_THRD_RUNNING;
#endif

    TraceAll("Main Msg Socket: OUT tid=0x%08x",tid);
#if USE_BEGINTHREADEX
    _endthreadex(0);
#endif
    return 0;
}



// ***********************************************************
// Function name    : dbserver_sock_registration_key_req
// Description      : Handles a Get/Set registration key message request from an Application. 
//                    Send request to TDMF Agent, receive its response
//                    and update this Agent's registration key in TDMF database.
// Return type      : void 
// 
// ***********************************************************
void dbserver_sock_registration_key_req(CollectorConfig* cfg, sock_t* request, mmp_mngt_header_t *hdr)
{
    int r;
    mmp_TdmfRegistrationKey data;
    //read message data
    r = mmp_mngt_sock_recv(request,(char*)&data,sizeof(data));
    if ( r == sizeof(data) )
    {
        mmp_mngt_RegistrationKeyMsg_t  msg;
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;

        if (strcmp(data.szServerUID, "Collector") == 0)
        {
            dbserver_db_SetCollectorRegistrationKey(data.szRegKey);

            //send back answer
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = MMP_MNGT_REGISTRATION_KEY;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
            mmp_convert_mngt_hdr_hton(&msg.hdr);
            msg.keydata = data;

            r = mmp_mngt_sock_send(request, (char*)&msg, sizeof(msg)); _ASSERT(r == sizeof(msg));
        }
        else
        {
            dbserver_get_agent_hostid(data.szServerUID, data.szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
            dbserver_get_agent_ip_port(data.szServerUID,&agentIP,&agentPort);
            
            if ( agentIP != 0 )
            {
                //initializations common to both situations below 
                msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
                msg.hdr.mngttype    = MMP_MNGT_REGISTRATION_KEY;
                msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
                msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
                mmp_convert_mngt_hdr_hton(&msg.hdr);
                
                if ( data.szRegKey[0] == 0 )
                {   //this is a REQUEST to RETREIVE the EXISTING RegKey.
                    //request TDMF Agent for its registration key
                    strcpy( msg.keydata.szServerUID, data.szServerUID );
                    //providing empty key = request TDMF Agent for its existing key
                    msg.keydata.szRegKey[0] = 0;
                    //send msg to TDMF Agent.  Wait for response on this socket.
                    //use msg as input/output buffer
                }
                else 
                {   //this is a REQUEST to SET a RegKey.
                    //send key to TDMF Agent 
                    msg.keydata = data;//same data as received
                    //use msg as input/output buffer
                }
                
                int recv = dbserver_sock_sendmsg_to_TdmfAgent(agentIP, agentPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg) );//_ASSERT(recv == sizeof(msg));
                if ( recv == sizeof(msg) )
                {   //update TDMF DB
                    //empty key means error processing request on Server/Agent side    
                    if ( msg.keydata.szRegKey[0] != 0 )
                    {   //update TDMF DB
                        int iKeyExpN = msg.keydata.iKeyExpirationTime;
                        //to host bytes order for use by dbserver_db_update_key()
                        msg.keydata.iKeyExpirationTime = ntohl(msg.keydata.iKeyExpirationTime);
                        dbserver_db_update_key(&msg.keydata);
                        //back to network bytes order before sending response
                        msg.keydata.iKeyExpirationTime = iKeyExpN;
                        
                        msg.hdr.mngtstatus = MMP_MNGT_STATUS_OK;
                    }
                    else
                    {
                        msg.hdr.mngtstatus = MMP_MNGT_STATUS_ERR_BAD_OR_MISSING_REGISTRATION_KEY;
                    }
                }
                else
                {
                    msg.hdr.mngtstatus = -recv;
                }
                
                //send answer received from TDMF Agent back to requester using same socket.
                msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
                msg.hdr.mngttype    = MMP_MNGT_REGISTRATION_KEY;
                msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
                mmp_convert_mngt_hdr_hton(&msg.hdr);
                //keep msg.keydata as received from Agent or empty if failure to receive from Agent
                r = mmp_mngt_sock_send(request, (char*)&msg, sizeof(msg)); _ASSERT(r == sizeof(msg));
                
            }
            else
            {
                TraceWrn("****Error, wrong TDMF agent IP (%d.%d.%d.%d) or Port (%d)!",(agentIP & 0x000000ff),(agentIP & 0x0000ff00)>>8,(agentIP & 0x00ff0000)>>16,(agentIP & 0xff000000)>>24,agentPort);
                
                //re-use msg 
                msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
                msg.hdr.mngttype    = MMP_MNGT_REGISTRATION_KEY;
                msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
                msg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
                mmp_convert_mngt_hdr_hton(&msg.hdr);
                //send back msg.keydata.szServerUID msg.keydata.szRegKey as received    
                r = mmp_mngt_sock_send(request, (char*)&msg, sizeof(msg)); _ASSERT(r == sizeof(msg));
            }
        }
    }
    else
    {
        TraceWrn("***Error, incomplete regkey msg data (rcvd %d vs %d) received from %.8s !", r, sizeof(data), data.szServerUID);
    }
}

static 
void dbserver_sock_registration_key_from_agent(sock_t* request)
{
    int r;
    mmp_mngt_RegistrationKeyMsg_t msg;
    //read message data
    r = mmp_mngt_sock_recv(request,(char*)&msg.keydata,sizeof(msg.keydata));
    if ( r == sizeof(msg.keydata) )
    {
        //update TDMF DB
        //to host bytes order for use by dbserver_db_update_key()
        msg.keydata.iKeyExpirationTime = ntohl(msg.keydata.iKeyExpirationTime);
        dbserver_db_update_key(&msg.keydata);
    }
    else
    {
        TraceWrn("***Error, incomplete regkey msg data (rcvd %d vs %d) received from %.8s !", r, sizeof(msg.keydata), msg.keydata.szServerUID );
    }
}

#ifdef TDMF_ENABLE_BROADCAST
// ***********************************************************
// Function name    : dbserver_sock_manage_broadcast_req
// Description      : Broadcasts a MMP_MNGT_AGENT_INFO_REQUEST message on network.
//                    Wait until timeout expires before completing the request.
//                    Keep request socket opened in order to 
//                    return to requester when TDMF DB is updated
//                    with all broadcast responses from Agents
// Return type      : void 
// Argument         : sock_t *request
// 
// ***********************************************************
void dbserver_sock_manage_broadcast_req(CollectorConfig* cfg, sock_t *request)
{
    int r;

    bool bCreator = false;
    HANDLE evRechargeBroadcastTimer = ::CreateEvent(NULL,TRUE,0,DBSERVER_BROADCAST_TIMER_EVENT);_ASSERT(evRechargeBroadcastTimer != NULL);
    if ( evRechargeBroadcastTimer != NULL  && ::GetLastError() != ERROR_ALREADY_EXISTS )
    {   //only one requester broadcasts the request. 
        //avoid sending multiple broadcasts simultaneously.

        //broadcast on network the Broadcast request message to TDMF Agents 
        //create a DATAGRAM socket to Broadcast request message to TDMF Agents 
        sock_t* brdcst = sock_create();
        r = sock_init( brdcst, NULL, NULL, cfg->ulIP, cfg->ulBroadcastIP, SOCK_DGRAM, AF_INET, 1, 0);_ASSERT(r>=0);
        if ( r >= 0 ) 
        {
            int n = 1;
            r = sock_set_opt(brdcst, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(int));_ASSERT(r>=0);
            if ( r >= 0 )
            {
                r = sock_bind_to_port( brdcst, TDMF_BROADCAST_PORT );_ASSERT(r>=0);
                if ( r >= 0 ) 
                {
                    mmp_mngt_BroadcastReqMsg_t  msg;
                    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
                    msg.hdr.mngttype    = MMP_MNGT_AGENT_INFO_REQUEST;
                    msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
                    mmp_convert_mngt_hdr_hton(&msg.hdr);
                    //indicate to Agent on which Server port to send response
                    msg.data.iBrdcstResponsePort= htonl( cfg->iPort );

                    DBGPRINT( (stderr,"BroadcastThread: %d   now broadcasting agent info request ...",time(0)) );
                    r = sock_sendto( brdcst, (char*)&msg, sizeof(msg) );_ASSERT(r == sizeof(msg));
                    DBGPRINTCOND(r != sizeof(msg),(stderr,"BroadcastThread: Error broadcasting the Broadcast request to Agents !"));
                }
            }
        }
        sock_delete(&brdcst);//no need to disconnect a SOCK_DGRAM socket ?
    }

    //wait for a period of iBroadcastResponsesTimeout without receiving responses before returning to requester
    while( WAIT_TIMEOUT != WaitForSingleObject(evRechargeBroadcastTimer, cfg->iBroadcastResponsesTimeout * 1000 ) )
    {
        //a TDMF Agent as responded to broadcast.  loop back and wait again...
    }
    DBGPRINT( (stderr,"BroadcastThread: %d, timeout occured waiting for agents responses",time(0)) );
    ::CloseHandle(evRechargeBroadcastTimer);
    //request socket disconnect will occur in calling function, signaling the initial requester that 
    //TDMF DB can now be accessed for updated agent information.
    return;
}
#endif //#ifdef TDMF_ENABLE_BROADCAST


// ***********************************************************
// Function name    : dbserver_sock_manage_broadcast_response
// Description      : Handles a MMP_MNGT_AGENT_INFO response received from a TDMF Agent.
//                    TDMF Agent sends this msg as a response to a MMP_MNGT_AGENT_INFO_REQUEST msg.
//                    Agent information is saved in TDMF database (t_ServerInfo record)
// Return type      : void 
// Argument         : CollectorConfig* cfg
// Argument         : sock_t *agentResponse
// 
// ***********************************************************
void dbserver_sock_manage_broadcast_response(CollectorConfig* cfg, sock_t *agentResponse)
{
    int r;
    mmp_TdmfServerInfo data;

    r = mmp_mngt_sock_recv(agentResponse,(char*)&data,sizeof(data));
    if ( r == sizeof(data) )
    {
        mmp_convert_TdmfServerInfo_ntoh(&data);

        if (!strcmp(data.szIPRouter, MMP_NAT_ENABLED))         // WR32867, NAT or Router is used, take translated address from IP header
            {
            ip_to_ipstring(agentResponse->rip, data.szIPRouter );   //WR32867 Router IP translation, rip is the real address
            }



        //DBGPRINT((stderr,"BroadcastThread: response from %s :\n  hostid=%s\n  name=%s\n  ip[0]=%s\n  port=%d\n  domain=%s",
        //    data.szIPAgent[0],data.szServerUID,data.szMachineName,data.szIPAgent[0],data.iPort,data.szTDMFDomain));
        TraceInf("Presence signal from %s :\n  hostid=%s\n  name=%s\n  ip[0]=%s\n  port=%d\n  domain=%s",
            data.szIPAgent[0],data.szServerUID,data.szMachineName,data.szIPAgent[0],data.iPort,data.szTDMFDomain);
    
        //TDMF Agent did his work. thanks for calling, bye bye !
        //sock_disconnect(agentResponse); 

        //update TDMF DB 
        dbserver_db_update_serverInfo(&data);

		int iHostId = strtoul(data.szServerUID, NULL, 16);
		dbserver_agent_monit_UpdateAgentBABSizeAllocated(&iHostId, HOST_ID_NUMERIC, data.iBABSizeAct, data.iBABSizeReq);

#ifdef TDMF_ENABLE_BROADCAST
        //special case to manage response to original the Broadcast requester
        //It is only after a timeout elapsed after the last agent response received,
        //that a response is sent to requester.
        //Signal indicates to wait thread to refill its timeout timer
        HANDLE evRechargeBroadcastTimer = ::OpenEvent(EVENT_ALL_ACCESS,0,DBSERVER_BROADCAST_TIMER_EVENT);
        if ( evRechargeBroadcastTimer != NULL )
        {
            ::PulseEvent(evRechargeBroadcastTimer);//release all threads waiting on evRechargeBroadcastTimer
            ::CloseHandle(evRechargeBroadcastTimer);
        }
#endif  //#ifdef TDMF_ENABLE_BROADCAST

    }
    else
    {
        if ( r >= sizeof(data.szServerUID) )
        {
            data.szServerUID[64] = 0;//just in case ....
            TraceInf("Warning! Possible MMP version mismatch with Agent %s !",data.szServerUID);
        }
    }
}

// ***********************************************************
// Function name    : dbserver_sock_get_lg_cfg_req
// Description      : Handles an Application request to retreive the current
//                    logical group configuration, in the form of tyhe content 
//                    of a .cfg file.
//                    A request is sent to the specified TDMF Agent; the response
//                    is relayed to the application.
//                    The response is sent back to Application using the 
//                    already opened request socket.
// Return type      : void  
// Argument         : CollectorConfig* cfg
// Argument         : sock_t *request
// 
// ***********************************************************
void    dbserver_sock_get_lg_cfg_req(CollectorConfig* cfg, sock_t *request)
{
    int r,toread,towrite;
    mmp_mngt_ConfigurationMsg_t msg;
    mmp_mngt_header_t *hdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_ConfigurationMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(hdr+1),toread);
    if ( r == toread )
    {
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;

        dbserver_get_agent_hostid(msg.szServerUID, msg.szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
        dbserver_get_agent_ip_port(msg.szServerUID,&agentIP,&agentPort);

        if ( agentIP != 0 )
        {
            //send configuration request to TDMF Agent
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = MMP_MNGT_GET_LG_CONFIG;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
            mmp_convert_mngt_hdr_hton(&msg.hdr);

            //other 'msg' fields were filled above by mmp_mngt_sock_recv(). 
            //send them as received.

            //send msg to TDMF Agent.  Wait for response on same socket.
            //expected response is a mmp_mngt_ConfigurationMsg_t.
            mmp_mngt_ConfigurationMsg_t *pRcvdCfgData;
            r = dbserver_sock_sendmsg_recv_cfg( agentIP, agentPort, 
                                                (const char*)&msg, sizeof(msg), 
                                                &pRcvdCfgData );
            bool bRcvdCfgData;
            int  iDataSize; 
            if ( pRcvdCfgData != 0 )
            {   //data received from TDMF Agent
                //relay Tdmf Agent response back to requester
                //send almost as received, do only necessary modifications/conversions below
                iDataSize = pRcvdCfgData->data.uiSize;
                pRcvdCfgData->data.iType        = htonl(pRcvdCfgData->data.iType);
                pRcvdCfgData->data.uiSize       = htonl(pRcvdCfgData->data.uiSize);
                //note : cfg data files are not written to DB by TDMF Collector
                //       so no DB access here.
                bRcvdCfgData = false;

                pRcvdCfgData->hdr.mngtstatus  = ntohl(pRcvdCfgData->hdr.mngtstatus);
            }
            else
            {   
                bRcvdCfgData = true;
                pRcvdCfgData = new mmp_mngt_ConfigurationMsg_t;
                //hdr filled below
                strcpy(pRcvdCfgData->szServerUID, msg.szServerUID);
                strcpy(pRcvdCfgData->data.szFilename,msg.data.szFilename);//as request received
                pRcvdCfgData->data.iType        = msg.data.iType;//as request received, already as network byte order
                pRcvdCfgData->data.uiSize       = 0;//no cfg received from Agent
                iDataSize                       = 0;
                
                pRcvdCfgData->hdr.mngtstatus    = -r;//report error status to requester
                
            }

            //
            //respond to requester
            //
            pRcvdCfgData->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
            pRcvdCfgData->hdr.mngttype       = MMP_MNGT_SET_LG_CONFIG;    //send a SET config message to requester 
            pRcvdCfgData->hdr.sendertype     = SENDERTYPE_TDMF_SERVER;
            mmp_convert_mngt_hdr_hton(&pRcvdCfgData->hdr);

            towrite = sizeof(mmp_mngt_ConfigurationMsg_t) + iDataSize;
            r = mmp_mngt_sock_send(request, (char*)pRcvdCfgData, towrite); _ASSERT(r == towrite);

            if ( !bRcvdCfgData )
            {   //release memory allocated by dbserver_sock_sendmsg_recv_cfg()
                mmp_mngt_free_cfg_data_mem(&pRcvdCfgData);
            }
            else
            {   //release memory allocated locally
                delete pRcvdCfgData;
            }
        }
        else
        {
            DBGPRINT((stderr,"****Error, wrong agent IP (%d.%d.%d.%d) or Port (%d)!",(agentIP & 0x000000ff),(agentIP & 0x0000ff00)>>8,(agentIP & 0x00ff0000)>>16,(agentIP & 0xff000000)>>24,agentPort));

            //respond with a unknown Agent status
            //re-use some fields of the received message
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = MMP_MNGT_SET_LG_CONFIG;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
            mmp_convert_mngt_hdr_hton(&msg.hdr);
            //msg.szServerUID field is already ok
            msg.data.uiSize     = htonl(0);

            towrite = sizeof(mmp_mngt_ConfigurationMsg_t) + 0;//only the mmp_mngt_ConfigurationMsg_t struct.
            r = mmp_mngt_sock_send(request, (char*)&msg, towrite); _ASSERT(r == towrite);
        }
    }
}

// ***********************************************************
// Function name    : dbserver_sock_get_tdmf_file_req
// Description      : Handles an Application request to retreive the current
//                    TDMF file (script file).
//                    A request is sent to the specified TDMF Agent; the response
//                    is relayed to the application.
//                    The response is sent back to Application using the 
//                    already opened request socket.
// Return type      : void  
// Argument         : CollectorConfig* cfg
// Argument         : sock_t *request
// 
// ***********************************************************

void    dbserver_sock_get_tdmf_file_req(CollectorConfig* cfg, sock_t *request)
{
    int r,toread,towrite;
    mmp_mngt_FileMsg_t msg;
    mmp_mngt_header_t *hdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get TDMF file message into msg structure
    toread = sizeof(mmp_mngt_FileMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(hdr+1),toread);
    if ( r == toread )
    {
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;

        dbserver_get_agent_hostid(msg.szServerUID, msg.szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
        dbserver_get_agent_ip_port(msg.szServerUID,&agentIP,&agentPort);

        if ( agentIP != 0 )
        {
            //send configuration request to TDMF Agent
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = MMP_MNGT_TDMF_GETFILE;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
            mmp_convert_mngt_hdr_hton(&msg.hdr);

            mmp_mngt_FileMsg_t *pRcvdFileData;
            r = dbserver_sock_sendmsg_recv_file ( agentIP, agentPort, 
                                                (const char*)&msg, sizeof(msg), 
                                                &pRcvdFileData );
            bool bRcvdFileData;
            int  iDataSize; 
            if ( pRcvdFileData != 0 )
            {   //data received from TDMF Agent
                //relay Tdmf Agent response back to requester
                //send almost as received, do only necessary modifications/conversions below
                iDataSize = pRcvdFileData->data.uiSize;
                pRcvdFileData->data.iType        = htonl(pRcvdFileData->data.iType);
                pRcvdFileData->data.uiSize       = htonl(pRcvdFileData->data.uiSize);
                //note : cfg data files are not written to DB by TDMF Collector
                //       so no DB access here.
                bRcvdFileData = false;

                pRcvdFileData->hdr.mngtstatus  = ntohl(pRcvdFileData->hdr.mngtstatus);
            }
            else
            {   
                bRcvdFileData = true;
                pRcvdFileData = new mmp_mngt_FileMsg_t;
                //hdr filled below
                strcpy(pRcvdFileData->szServerUID, msg.szServerUID);
                strcpy(pRcvdFileData->data.szFilename,msg.data.szFilename);//as request received
                pRcvdFileData->data.iType        = msg.data.iType;//as request received, already as network byte order
                pRcvdFileData->data.uiSize       = 0;//no cfg received from Agent
                iDataSize                        = 0;
                
                pRcvdFileData->hdr.mngtstatus    = -r;//report error status to requester
                
            }

            //
            //respond to requester
            //
            pRcvdFileData->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
            pRcvdFileData->hdr.mngttype       = MMP_MNGT_TDMF_SENDFILE;//send a SENDFILE message to requester 
            pRcvdFileData->hdr.sendertype     = SENDERTYPE_TDMF_SERVER;
            mmp_convert_mngt_hdr_hton(&pRcvdFileData->hdr);

            towrite = sizeof(mmp_mngt_FileMsg_t) + iDataSize;
            r = mmp_mngt_sock_send(request, (char*)pRcvdFileData, towrite); _ASSERT(r == towrite);

            if ( !bRcvdFileData )
            {   //release memory allocated by dbserver_sock_sendmsg_recv_file()
                mmp_mngt_free_file_data_mem(&pRcvdFileData);
            }
            else
            {   //release memory allocated locally
                delete pRcvdFileData;
            }
        }
        else
        {
            DBGPRINT((stderr,"****Error, wrong agent IP (%d.%d.%d.%d) or Port (%d)!",(agentIP & 0x000000ff),(agentIP & 0x0000ff00)>>8,(agentIP & 0x00ff0000)>>16,(agentIP & 0xff000000)>>24,agentPort));

            //respond with a unknown Agent status
            //re-use some fields of the received message
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = MMP_MNGT_TDMF_SENDFILE;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
            mmp_convert_mngt_hdr_hton(&msg.hdr);
            //msg.szServerUID field is already ok
            msg.data.uiSize     = htonl(0);

            towrite = sizeof(mmp_mngt_FileMsg_t) + 0;
            r = mmp_mngt_sock_send(request, (char*)&msg, towrite); _ASSERT(r == towrite);
        }
    }
}

// ***********************************************************
// configuration received from client has to be sent to Agent 
void    dbserver_sock_set_lg_cfg_req(CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr ) 
{
    int r;
    mmp_mngt_ConfigurationMsg_t    *pCfgData = 0; 

    //receive all configuration data from request socket
    mmp_mngt_recv_cfg_data( request, hdr, &pCfgData ); 

    if ( pCfgData != 0 )
    {
        //relay request to TDMF Agent
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;
        mmp_mngt_ConfigurationStatusMsg_t   response;

        dbserver_get_agent_hostid(pCfgData->szServerUID, pCfgData->szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
        dbserver_get_agent_ip_port(pCfgData->szServerUID,&agentIP,&agentPort);

        //dbserver_mmp_dump( pCfgData, sizeof(mmp_mngt_ConfigurationMsg_t) + pCfgData->data.uiSize, true, false, pCfgData->szServerUID, "MMP_MNGT_SET_LG_CONFIG" );

        if ( agentIP != 0 )
        {
            //send configuration request to TDMF Agent
            //send message almost as received, do only necessary modifications
            pCfgData->hdr.sendertype    =   SENDERTYPE_TDMF_SERVER;
            mmp_convert_mngt_hdr_hton(&pCfgData->hdr);
            //other fields of msg were filled above by mmp_mngt_recv_cfg_data()

            int towrite = sizeof(mmp_mngt_ConfigurationMsg_t) + pCfgData->data.uiSize;

            pCfgData->data.iType        = htonl(pCfgData->data.iType);
            pCfgData->data.uiSize       = htonl(pCfgData->data.uiSize);

            //send msg to TDMF Agent.  Wait for response, mmp_mngt_ConfigurationStatusMsg_t, on same socket.
            r = dbserver_sock_sendmsg_to_TdmfAgent(agentIP, agentPort, (const char*)pCfgData, towrite, (char*)&response, sizeof(response));
            if ( r == sizeof(response) )
            {   //success
                //relay Tdmf Agent response back to requester (below)
                response.hdr.mngtstatus  = ntohl(response.hdr.mngtstatus);
            }
            else
            {   //eror, could not get response from TDMF Agent
                //send back requester an error status
                response.hdr.mngtstatus  = -r;//mngt error status
                response.iStatus = htonl(1);// 1 = rx/tx error
            }
            //relay Tdmf Agent response back to requester
            //send message almost as received. do only necessary modifications
        }
        else
        {
            DBGPRINT((stderr,"****Error,  unknown agent IP (0x%08x) or Port (%d)!",agentIP,agentPort));

            //relay Tdmf Agent response back to requester
            response.hdr.mngtstatus     = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
        }

        response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
        response.hdr.mngttype       = MMP_MNGT_SET_CONFIG_STATUS;
        response.hdr.sendertype     = SENDERTYPE_TDMF_SERVER;
        mmp_convert_mngt_hdr_hton(&response.hdr);
        r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); _ASSERT(r == sizeof(response));

        //release memory obtained from mmp_mngt_recv_cfg_data()
        mmp_mngt_free_cfg_data_mem(&pCfgData);
    }
}

// ***********************************************************
void dbserver_sock_tdmf_cmd_req(CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr ) 
{
    int r,toread;
    mmp_mngt_TdmfCommandMsg_t msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfCommandMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread);
    if ( r == toread )
    {
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;
        bool                        bError = true;//assume error

        dbserver_get_agent_hostid(msg.szServerUID, msg.szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
        dbserver_get_agent_ip_port(msg.szServerUID,&agentIP,&agentPort);

        if ( agentIP != 0 )
        {
            //send configuration request to TDMF Agent
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = MMP_MNGT_TDMF_CMD;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
            mmp_convert_mngt_hdr_hton(&msg.hdr);
            //other fields of msg were filled above by mmp_mngt_sock_recv()
            //send other fields just as received (network byte order)

            //send msg to TDMF Agent.  Wait for response, mmp_mngt_TdmfCommandStatusMsg_t, on same socket.
            //no data transmit expected from peer.
            mmp_mngt_TdmfCommandStatusMsg_t response;
            /*
            r = dbserver_sock_sendmsg_to_TdmfAgent(agentIP, agentPort, (const char*)&msg, sizeof(msg), (char*)&response, sizeof(response));
            if ( r == sizeof(response) )
            {   //relay Tdmf Agent response back to requester (below)
            }
            else
            {   //error, could not get response from Agent
                //send Tdmf Agent response back to requester
                response.hdr.mngtstatus = htonl( -r );
                response.iStatus = htonl(MMP_MNGT_TDMF_CMD_STATUS_ERR_COULD_NOT_REACH_TDMF_AGENT);
            }
            r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); _ASSERT(r == sizeof(response));
            */
            int r;
            sock_t * agent = sock_create();
            r = sock_init( agent, NULL, NULL, 0, agentIP, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
            if ( r >= 0 ) 
            {
                r = sock_connect(agent, agentPort); 
                if ( r >= 0 ) 
                {   //first send ftd_header_t to TDMF Agent
                    if ( dbserver_sock_send_ftd_header(agent) )
                    {   //second send MMP_MNGT_TDMF_CMD request
                        r = mmp_mngt_sock_send(agent, (char*)&msg, sizeof(msg)); //_ASSERT(r == sizeof(msg));
                        if ( r == sizeof(msg) )
                        {   //receive mmp_mngt_TdmfCommandStatusMsg_t from Agent
                            r = mmp_mngt_sock_recv(agent, (char*)&response, sizeof(response), MMP_MNGT_TIMEOUT_DMF_CMD); //_ASSERT(r == sizeof(response)); // ardev 030110
                            if ( r == sizeof(response) )
                            {   
                                bError = false;//if we get here everything is now ok .

                                //relay mmp_mngt_TdmfCommandStatusMsg_t back to requester
                                r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); //_ASSERT(r == sizeof(response));
                                //blindly relay any additionnal data (the output msg string) to requester
                                char outputmsg[1024];
                                //read from Agent
                                while( (r = mmp_mngt_sock_recv(agent, outputmsg, sizeof(outputmsg))) > 0 )
                                {   //send to requester
                                    int r2 = mmp_mngt_sock_send(request, outputmsg, r); //_ASSERT(r2 == r);
                                }
                            }
                        }
                    }
                }
                sock_disconnect(agent);
            }
            sock_delete(&agent);
        }

        if ( bError )
        {
            DBGPRINT((stderr,"****Error, wrong agent IP (%d.%d.%d.%d) or Port (%d)!",(agentIP & 0x000000ff),(agentIP & 0x0000ff00)>>8,(agentIP & 0x00ff0000)>>16,(agentIP & 0xff000000)>>24,agentPort));
            //report error to requester
            mmp_mngt_TdmfCommandStatusMsg_t response;
            response.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            response.hdr.mngttype    = MMP_MNGT_TDMF_CMD;
            response.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            response.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT;
            mmp_convert_mngt_hdr_hton(&response.hdr);

            strcpy( response.szServerUID, msg.szServerUID);//as received from requester     
            response.iSubCmd = msg.iSubCmd;            //as received from requester - already in network byte order     
            response.iLength = 0;
            //send back to requester an error status 
            r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); //_ASSERT(r == sizeof(response));
        }
    }
}


// ***********************************************************
// configuration received from client has to be sent to Agent 
void dbserver_sock_agent_general_config(CollectorConfig* cfg, sock_t *request, int iMngtMsgType)
{
    int r,toread;
    mmp_mngt_TdmfAgentConfigMsg_t msg;
    mmp_mngt_TdmfAgentConfigMsg_t response;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfAgentConfigMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread);
    if ( r == toread )
    {
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;

        dbserver_get_agent_hostid(msg.data.szServerUID, msg.data.szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
        dbserver_get_agent_ip_port(msg.data.szServerUID,&agentIP,&agentPort);

        if ( agentIP != 0 && agentPort != 0 )
        {
            //send general configuration request to TDMF Agent

            //send configuration request to TDMF Agent
            //send message as it is received. only hdr is re-built
            msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            msg.hdr.mngttype    = iMngtMsgType;
            msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
            mmp_convert_mngt_hdr_hton(&msg.hdr);

            r = dbserver_sock_sendmsg_to_TdmfAgent(agentIP, agentPort, (const char*)&msg, sizeof(msg), (char*)&response, sizeof(response));
            //relay Tdmf Agent response back to requester (below) with minimum of modifications
            if ( r == sizeof(response) )
            {   response.hdr.sendertype  = htonl(SENDERTYPE_TDMF_SERVER);
            }
            else
            {   //error, could not get response from Agent
                //send Tdmf Agent response back to requester
                response.hdr.sendertype  = htonl(SENDERTYPE_TDMF_SERVER);
                response.hdr.mngtstatus  = htonl( -r );
            }

            //if this was a MMP_MNGT_SET_AGENT_GEN_CONFIG AND the Port value was modified, 
            //then Collector must update its internal look-up-table for this Agent
            if ( iMngtMsgType == MMP_MNGT_SET_AGENT_GEN_CONFIG      && 
                 0 == response.hdr.mngtstatus /* successful call to Agent */)
            {
                int requestedAgentPort = ntohl(msg.data.iPort);
                if ( agentPort != requestedAgentPort )
                {   
                    dbserver_db_update_server_port( msg.data.szServerUID, requestedAgentPort );
                }
            }
        }
        else
        {
            DBGPRINT((stderr,"****Error, wrong agent IP (%d.%d.%d.%d) or Port (%d)!",(agentIP & 0x000000ff),(agentIP & 0x0000ff00)>>8,(agentIP & 0x00ff0000)>>16,(agentIP & 0xff000000)>>24,agentPort));

            //send configuration request to TDMF Agent
            response.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            response.hdr.mngttype    = iMngtMsgType;
            response.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            response.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
            mmp_convert_mngt_hdr_hton(&response.hdr);

            strcpy( response.data.szServerUID, msg.data.szServerUID);//as received from requester       
            // 'data' portion not relevant in this case. do not fill it.
        }

        //send back response to requester
        r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); //_ASSERT(r == sizeof(response));
    }
}

// ***********************************************************
//
static void         
dbserver_sock_get_all_devices(CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr)
{
    int r,toread,towrite;
    mmp_mngt_TdmfAgentDevicesMsg_t msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread);
    if ( r == toread )
    {
        mmp_mngt_TdmfAgentDevicesMsg_t *pTdmfDevicesResponseMsg = 0;
        unsigned long                   agentIP = 0;
        unsigned int                    agentPort = 0;
        int                             mngtstatus;

        dbserver_get_agent_hostid(msg.szServerUID, msg.szServerUID);
        dbserver_get_agent_ip_port(msg.szServerUID,&agentIP,&agentPort);

        if ( agentIP != 0 && agentPort != 0 )
        {
            //send GET all device request to TDMF Agent

            //create socket on Agent
            int r;
            sock_t * agent_sock = sock_create();
            r = sock_init( agent_sock, NULL, NULL, 0, agentIP, SOCK_STREAM, AF_INET, 1, 0); //_ASSERT(r>=0);
            if ( r >= 0 ) 
            {
                r = sock_connect(agent_sock, agentPort);
                if ( r >= 0 ) 
                {
                    mngtstatus = MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;//assume error

                    //first send ftd_header_t to TDMF Agent
                    if ( dbserver_sock_send_ftd_header(agent_sock) )
                    {   
                        //send request and pTdmfDevicesResponseMsg is set to received response.
                        r = mmp_mngt_request_alldevices( 0, 0, agent_sock, 
                                                         SENDERTYPE_TDMF_SERVER, msg.szServerUID, true,
                                                         &pTdmfDevicesResponseMsg );
                        if ( pTdmfDevicesResponseMsg != 0 )
                        {   
                            mngtstatus = MMP_MNGT_STATUS_OK;//success
                        }
                        else
                        {
                            switch(r)
                            {
                            case -1:    
                                mngtstatus = MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT;  
                                break;
                            case -2:    
                            case -3:    
                            case -4:    
                            case -5:    
                                mngtstatus = MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;  
                                break;
                            }
                        }
                    }
                }
                else
                {
                    mngtstatus = MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT;
                }
                sock_disconnect(agent_sock);
            }
            else
            {
                mngtstatus = MMP_MNGT_STATUS_ERR_COLLECTOR_INTERNAL_ERROR;
            }
            sock_delete(&agent_sock);


            if ( pTdmfDevicesResponseMsg == 0 )
            {   //an error occured, no result avaiulable
                pTdmfDevicesResponseMsg = new mmp_mngt_TdmfAgentDevicesMsg_t;
                pTdmfDevicesResponseMsg->iNbrDevices = 0;
            }
            //prepare response message 
            towrite = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t) + pTdmfDevicesResponseMsg->iNbrDevices*sizeof(mmp_TdmfDeviceInfo);

            pTdmfDevicesResponseMsg->hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            pTdmfDevicesResponseMsg->hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            pTdmfDevicesResponseMsg->hdr.mngttype    = MMP_MNGT_SET_ALL_DEVICES;
            pTdmfDevicesResponseMsg->hdr.mngtstatus  = mngtstatus;
            mmp_convert_mngt_hdr_hton(&pTdmfDevicesResponseMsg->hdr);

            strcpy(pTdmfDevicesResponseMsg->szServerUID, msg.szServerUID);

            int iNbrDevices = pTdmfDevicesResponseMsg->iNbrDevices;
            pTdmfDevicesResponseMsg->iNbrDevices = htonl(pTdmfDevicesResponseMsg->iNbrDevices);

            //convert ntoh all device info structures received
            mmp_TdmfDeviceInfo *devInfo = (mmp_TdmfDeviceInfo *) (pTdmfDevicesResponseMsg + 1);
            for (int i=0; i<iNbrDevices; i++ , devInfo++ )
            {
                mmp_convert_TdmfAgentDeviceInfo_ntoh(devInfo);
            }


            //send back response to requester
            r = mmp_mngt_sock_send(request, (char*)pTdmfDevicesResponseMsg, towrite); _ASSERT(r == towrite);

            if ( mngtstatus == MMP_MNGT_STATUS_OK )
            {   //only case where mem was allocated within mmp_mngt_request_alldevices()
                mmp_mngt_free_alldevices_data(&pTdmfDevicesResponseMsg);
            }
            else
            {
                delete pTdmfDevicesResponseMsg;
            }
        }
        else
        {
            DBGPRINT((stderr,"****Error, wrong agent IP (%d.%d.%d.%d) or Port (%d)!",(agentIP & 0x000000ff),(agentIP & 0x0000ff00)>>8,(agentIP & 0x00ff0000)>>16,(agentIP & 0xff000000)>>24,agentPort));

            mmp_mngt_TdmfAgentDevicesMsg_t response;
            //send configuration request to TDMF Agent
            response.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
            response.hdr.mngttype    = MMP_MNGT_SET_ALL_DEVICES;
            response.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
            response.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
            mmp_convert_mngt_hdr_hton(&response.hdr);

            strcpy( response.szServerUID, msg.szServerUID);//as received from requester     
            response.iNbrDevices = 0;

            //send back response to requester
            r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); _ASSERT(r == sizeof(response));
        }
    }
}



// ***********************************************************
// Alert message received from TDMF Agent.
static void         
dbserver_sock_recv_alert(CollectorConfig* cfg, sock_t *s, mmp_mngt_header_t *hdr)
{
    int r,toread;
    mmp_mngt_TdmfAlertMsg_t msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfAlertMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(s,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        char *pData = 0, *szModuleName = 0, *szAlertMessage = 0;

        msg.iLength = ntohl(msg.iLength); _ASSERT(msg.iLength>=2);//a minimum of two '\0' char expected (two empty strings).
        mmp_convert_TdmfAlert_ntoh(&msg.data);

        //now we know how much data follows for the two strings
        if ( msg.iLength > 0 )
        {
            pData = new char [msg.iLength];
            toread = msg.iLength;
            r = mmp_mngt_sock_recv(s,pData,toread);_ASSERT(r == toread);
            if ( r == toread )
            {
                szModuleName    = pData;
                szAlertMessage  = szModuleName + strlen(szModuleName) + 1;
            }
        }

        // retreive Agent Host ID
        unsigned long ulHostID = strtoul(msg.data.szServerUID, NULL, 16);
        // Update Alive Socket TIMESTAMP 
        AgentAliveSetTimeStamp(ulHostID, time(0));


        //all alert data available in msg.data
        //write to TDMF db 
        dbserver_db_add_alert( &msg.data, szAlertMessage );

        delete [] pData;
    }
    else
    {
        TraceWrn("Wrong size (%d vs %d) of Tdmf Alert message (MMP_MNGT_ALERT_DATA) received.", r+sizeof(mmp_mngt_header_t), toread+sizeof(mmp_mngt_header_t) );
    }
}

// ***********************************************************
// Status message received from a TDMF Agent.
static void         
dbserver_sock_recv_status(CollectorConfig* cfg, sock_t *s, mmp_mngt_header_t *hdr)
{
    int r,toread;
    mmp_mngt_TdmfStatusMsgMsg_t msg;
    mmp_mngt_TdmfStatusMsgMsg_t *pmsg = &msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfStatusMsgMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(s,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        mmp_convert_TdmfStatusMsg_ntoh(&msg.data);

        if ( msg.data.iLength > 0 && msg.data.iLength < 0x40000 ) /* allow valid text message to be 256 KB max */
        {
            char *pszMessage = new char [msg.data.iLength];

            toread = msg.data.iLength;
            r = mmp_mngt_sock_recv(s,pszMessage,toread); _ASSERT(r == toread);//bad msg.data.iLength value RX ?
            if ( r == toread )
            {   
                // retreive Agent Host ID
                unsigned long ulHostID = strtoul(msg.data.szServerUID, NULL, 16);
                // Update Alive Socket TIMESTAMP 
                AgentAliveSetTimeStamp(ulHostID, time(0));
                
                //all data available in msg.data, write to TDMF db
                dbserver_db_add_status( &msg.data, pszMessage );
            }

            delete [] pszMessage;
        }
        else
        {
            TraceWrn("A bad Tdmf Status message (MMP_MNGT_STATUS_MSG) received from HostID %.8s, reporting a msg length of %d bytes.", msg.data.szServerUID, msg.data.iLength);
        }
    }
    else
    {
        TraceWrn("Wrong size (%d vs %d) of Tdmf Status message (MMP_MNGT_STATUS_MSG) received.", r+sizeof(mmp_mngt_header_t), toread+sizeof(mmp_mngt_header_t) );
    }
}

// ***********************************************************
// Performance message received from a TDMF Agent.
static void         
dbserver_sock_recv_perf(CollectorConfig* cfg, sock_t *s, mmp_mngt_header_t *hdr)
{
    int r,toread;
    mmp_mngt_TdmfPerfMsg_t msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfPerfMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(s,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        mmp_convert_TdmfPerfData_ntoh(&msg.data);

        dbserver_mmp_dump( (char*)&msg.data, sizeof(msg.data), true, true, msg.data.szServerUID, "MMP_MNGT_PERF_MSG" );

        // retreive Agent Host ID
        unsigned long ulHostID = strtoul(msg.data.szServerUID, NULL, 16);
        // Update Alive Socket TIMESTAMP 
        AgentAliveSetTimeStamp(ulHostID, time(0));

        //validate the number of perf. data bytes 
        if ( msg.data.iPerfDataSize % sizeof(ftd_perf_instance_t) == 0 )
        {
            int iNbrPerfData = msg.data.iPerfDataSize / sizeof(ftd_perf_instance_t);
            ftd_perf_instance_t *pPerfData = new ftd_perf_instance_t [iNbrPerfData];

            //read from socket a vector of performance data (ftd_perf_instance_t)
            toread = msg.data.iPerfDataSize;
            r = mmp_mngt_sock_recv(s,(char*)pPerfData,toread); _ASSERT(r == toread);
            if ( r == toread )
            {   
                //convert performance data from network bytes to host bytes order
                ftd_perf_instance_t *pWk  = pPerfData;
                ftd_perf_instance_t *pEnd = pPerfData + iNbrPerfData;
                //optimization: convert Host ID string to numerical once before calling dbserver_agent_monit_UpdateAgentGroupPerf()
                unsigned long ulHostID = strtoul(msg.data.szServerUID,0,16);_ASSERT(ulHostID!=0);
                for( ; pWk < pEnd ; pWk++ )
                {
                    mmp_convert_ntoh(pWk);
                    TracePerf("PERF DATA from %08x (%s):lgnum=%d, devid=%d, drvmode=%d, connection=%d, pctdone=%d ", 
                        ulHostID,dbserver_db_GetNameFromHostID((int)ulHostID),pWk->lgnum,pWk->devid,pWk->drvmode, pWk->connection, pWk->pctdone );
                    //  update ONLY latest GROUP perf data , not Device data
                    if ( pWk->lgnum == -100 && pWk->devid >= 0 )
                    {
                        dbserver_agent_monit_UpdateAgentGroupPerf(&ulHostID, HOST_ID_NUMERIC, pWk);
                    }
                }

                //all data available in msg.data, write to TDMF db
                dbserver_db_add_perfdata( &msg.data, pPerfData );
            }

            delete [] pPerfData;
        }
        else
        {   //TDMF Agent and TDMF Collector do not use identical ftd_perf_instance_t structure !!!!!!!
            TraceErr("* Bad performance data size received in a MMP_MNGT_PERF_MSG msg from HostID=%.8s \n  ==>> Possible ftd_perf_instance_t structure mismatch: expecting sizeof(ftd_perf_instance_t)=%d .", msg.data.szServerUID, sizeof(ftd_perf_instance_t) );
        }
    }
}

// ***********************************************************
// Performance configuration message received from a TDMF Client.
static void         
dbserver_sock_set_perf_cfg(CollectorConfig* cfg, sock_t *s, mmp_mngt_header_t *hdr)
{
    int r,toread;
    mmp_mngt_TdmfPerfCfgMsg_t msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //read the remainder of the get cfg message into msg structure
    toread = sizeof(mmp_mngt_TdmfPerfCfgMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(s,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        mmp_convert_TdmfPerfConfig_ntoh(&msg.data);

        //save info to DB, in t_NVP table
        dbserver_db_update_NVP_PerfCfg( &msg.data );

        //get list of all TDMF Agents Server Id (unique host id)
        std::list<CString>  listServerUID;
        dbserver_db_get_list_servers( listServerUID );

        //send message as it is received. only hdr has been re-built
        msg.hdr             = *hdr;
        msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
        mmp_convert_mngt_hdr_hton(&msg.hdr);
        mmp_convert_TdmfPerfConfig_hton(&msg.data);

        //send same message to all TDMF Agents registered in DB
        std::list<CString>::iterator it = listServerUID.begin();
        while( it != listServerUID.end() )
        {
            unsigned long   agentIP = 0;
            unsigned int    agentPort = 0;
            dbserver_get_agent_ip_port( (LPCTSTR)(*it) , &agentIP,&agentPort);
            if ( agentIP != 0 && agentPort != 0 )
            {
                //send performance configuration to TDMF Agent
                //no response expected. Agent might not be actually on-line.
                dbserver_sock_sendmsg_to_TdmfAgent(agentIP, agentPort, (const char*)&msg, sizeof(msg), 0, 0);
            }
            
            it++;
        }
    }
}

static bool 
dbserver_sock_recv_agent_alive(sock_t* request)
{
    bool bDeleteSocket = true;
    int r,toread;
    mmp_mngt_TdmfAgentAliveMsg_t msg;
    mmp_mngt_header_t *phdr = (mmp_mngt_header_t *)&msg;

    //mmp_mngt_header_t already read from socket
    //read the remainder of the message 
    toread = sizeof(msg.szServerUID) ;
    r = mmp_mngt_sock_recv(request,(char*)msg.szServerUID,toread); _ASSERT(r == toread);
    if ( r == toread )
    {   //attempt to recv msg.iMsgLength , optional ..
        msg.iMsgLength = 0;//clear just in case it won't be recvd.
        mmp_mngt_sock_recv(request,(char*)&msg.iMsgLength,sizeof(msg.iMsgLength)); 

        //retreive Agent Host ID
        int iHostID = strtoul(msg.szServerUID, NULL, 16);
        if ( iHostID != 0 )
        {   
            AgentAliveSocketAdd(request,iHostID,&msg);
            bDeleteSocket = false;//success. Socket must be kept alive.
        }
    }
   
    return bDeleteSocket;
}

// ***********************************************************
// Replication Group state data recvd from a TDMF Agent
static void         
dbserver_sock_recv_group_state(CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr)
{
    int r,toread;
    mmp_mngt_TdmfGroupStateMsg_t msg;

    //mmp_mngt_header_t already read from socket
    //read the remainder of the message 
    toread = sizeof(msg.szServerUID);
    r = mmp_mngt_sock_recv(request,(char*)msg.szServerUID,toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        mmp_TdmfGroupState grpstate;
        //retreive Agent Host ID
        unsigned long ulHostID = strtoul(msg.szServerUID, NULL, 16);

        // Update Alive Socket TIMESTAMP 
        AgentAliveSetTimeStamp(ulHostID, time(0));

        //read all Group states available on socket
        toread = sizeof(grpstate);
        do
        {
            r = mmp_mngt_sock_recv(request,(char*)&grpstate,toread); _ASSERT(r == toread);
            if ( r == toread )
            {
                mmp_convert_ntoh(&grpstate);

                dbserver_mmp_dump( (char*)&grpstate, toread, true, true, msg.szServerUID, "MMP_MNGT_GROUP_STATE" );

                if ( grpstate.sRepGrpNbr >= 0 )
                    dbserver_agent_monit_UpdateAgentGroupState(&ulHostID, HOST_ID_NUMERIC, &grpstate);
                else
                    break;//end of TX flag received 
            }

        } while( r == toread );
    }
}

//MMP_MNGT_TDMFCOMMONGUI_REGISTRATION
// ***********************************************************
// Function name    : dbserver_sock_recv_tdmfcommontool_registration
// Description      : This msg is received from the tdmf gui app.
//                    On first rx, it requests the exclusive right among tdmf gui app
//                    to access the TDMF DB. (only one tdmf gui app. should be accessing the TDMF DB)
//                    When granted, this msg must be  periodically sent, as a watchdog,
//                    to retain the exclusive right on the TDMF DB.
//                    The maximum period between msgs to retain the exclusive right is 60 seconds.
// ***********************************************************
#define WATCHDOG_MAX_PERIOD     60  //60 seconds    
#define WATCHDOG_MAX_NB_USER    4

struct WatchdogInfo
{
    unsigned long ulOwnerIP;
    time_t        ulLastMsgTime;
};
static std::map<CString, WatchdogInfo> gmapWatchdogOwner;

static void         
dbserver_sock_recv_tdmfcommontool_registration(sock_t *request)
{
    int r,toread;
    mmp_mngt_TdmfCommonGuiRegistrationMsg_t msg;
    mmp_mngt_header_t *phdr = &msg.hdr;

    //mmp_mngt_header_t already read from socket
    //read the remainder of the message 
    toread = sizeof(mmp_mngt_TdmfCommonGuiRegistrationMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r != toread )
        return;//socket error

    //protect access on global variables
    HANDLE h = OpenMutex(SYNCHRONIZE,0,MMP_MNGT_WATCHDOG_MUTEX);
    if ( h != NULL )
        WaitForSingleObject(h,1000);

    time_t now  = time(0);

    bool bCallerIsOwner = false;
    std::map<CString, WatchdogInfo>::iterator it = gmapWatchdogOwner.find(msg.szClientUID);
    if ((it != gmapWatchdogOwner.end()) && (it->second.ulOwnerIP == request->rip))
    {
        bCallerIsOwner = true;
    }

    bool bOwnershipGrantedToCaller;

    if ( msg.bRequestOwnership == 0 )
    {   //caller-owner requests to release to its ownership over the TDMF system
        if ( bCallerIsOwner )
        {
            gmapWatchdogOwner.erase(it);

            bOwnershipGrantedToCaller = false;//as requested
        }
        else
        {
            bOwnershipGrantedToCaller = false;//cannot release when not the owner
        }
    }
    else
    {
        if (bCallerIsOwner)
        {
            bOwnershipGrantedToCaller = true;
        }
        else if (gmapWatchdogOwner.size() < WATCHDOG_MAX_NB_USER)
        {
            bOwnershipGrantedToCaller = true;
        }
        else // max user nb reached, check if some of them have expired
        {
            bOwnershipGrantedToCaller = false;

            std::map<CString, WatchdogInfo>::iterator it;
            for (it = gmapWatchdogOwner.begin(); it != gmapWatchdogOwner.end(); it++)
            {
                if (now > it->second.ulLastMsgTime + WATCHDOG_MAX_PERIOD)
                {
                    gmapWatchdogOwner.erase(it);
                    bOwnershipGrantedToCaller = true;
                    break;  // only find the first expired one
                }
            }
        }

        if ( bOwnershipGrantedToCaller )
        {
            struct WatchdogInfo Info;
            Info.ulLastMsgTime = now;
            Info.ulOwnerIP     = request->rip; _ASSERT(request->rip != 0 && request->rip != 0xcccccccc);
            gmapWatchdogOwner[msg.szClientUID] = Info;
        }
    }

    //release access on global variables
    if ( h != NULL )
    {
        ReleaseMutex(h);
        CloseHandle(h);
    }

    //respond to caller
    //prepare msg header
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngttype    = MMP_MNGT_TDMFCOMMONGUI_REGISTRATION;
    msg.hdr.sendertype  = SENDERTYPE_TDMF_SERVER;
    msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //send back response in  msg.szClientUID : send all '1' for ok, send all '0' for no.
    memset(msg.szClientUID, bOwnershipGrantedToCaller ? '1' : '0', sizeof(msg.szClientUID));
    int towrite = sizeof(msg);
    r = mmp_mngt_sock_send(request,(char*)&msg,towrite); _ASSERT(r == towrite);
}


// ***********************************************************
// Function name    : dbserver_sock_recv_group_monitoring
// Description      : recv a MMP_MNGT_GROUP_MONITORING msg from a TDMF Agent 
// Return type      : int 
// 
// ***********************************************************
static void 
dbserver_sock_recv_group_monitoring(sock_t *request)
{
    int r,toread;
    mmp_mngt_TdmfReplGroupMonitorMsg_t msg;
    mmp_mngt_header_t *phdr = &msg.hdr;

    //mmp_mngt_header_t already read from socket
    //read the remainder of the message 
    toread = sizeof(mmp_mngt_TdmfReplGroupMonitorMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r != toread )
        return;//socket error

    mmp_convert_ntoh(&msg.data);
    // retreive Agent Host ID
    unsigned long ulHostID = strtoul(msg.data.szServerUID, NULL, 16);
    // Update Alive Socket TIMESTAMP 
    AgentAliveSetTimeStamp(ulHostID, time(0));
    dbserver_agent_monit_UpdateAgentGroupMonitoring(&msg.data, request->rip);
}


// ***********************************************************
static void         
dbserver_sock_recv_collector_params (bool isSet, CollectorConfig* cfg, sock_t *request)
{
    int r,toread;
    mmp_mngt_TdmfCollectorParamsMsg_t msg;
    mmp_mngt_header_t *phdr = &msg.hdr;

    if ( isSet )
    {   //MMP_MNGT_SET_DB_PARAMS received.  
        //mmp_mngt_header_t already read from socket
        //read the remainder of the message 
        toread = sizeof(mmp_mngt_TdmfCollectorParamsMsg_t)-sizeof(mmp_mngt_header_t);
        r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread); _ASSERT(r == toread);
        if ( r != toread )
            return;//socket error

        mmp_convert_ntoh(&msg.data);

        dbserver_mmp_dump( (char*)(phdr+1), toread, true, false, "\0", "MMP_MNGT_SET_DB_PARAMS" );

        r = dbserver_db_write_sysparams(&msg.data, cfg);
        //prepare response, reuse available buffer
        phdr->magicnumber = MNGT_MSG_MAGICNUMBER;
        phdr->mngttype    = MMP_MNGT_SET_DB_PARAMS;
        phdr->sendertype  = SENDERTYPE_TDMF_SERVER;
        if ( r == 0 )
            phdr->mngtstatus  = MMP_MNGT_STATUS_OK;
        else
            phdr->mngtstatus  = MMP_MNGT_STATUS_ERR_ERROR_ACCESSING_TDMF_DB;

        mmp_convert_mngt_hdr_hton(phdr);

        int towrite = sizeof(mmp_mngt_header_t);
        r = mmp_mngt_sock_send(request,(char*)phdr,towrite); _ASSERT(r == towrite);
    }
    else
    {   //MMP_MNGT_GET_DB_PARAMS received.  only the mmp_mngt_header_t is tx for this request.
        //get latest values
        r = dbserver_db_read_sysparams(&msg.data,cfg);
        //prepare response, reuse available buffer
        phdr->magicnumber = MNGT_MSG_MAGICNUMBER;
        phdr->mngttype    = MMP_MNGT_GET_DB_PARAMS;
        phdr->sendertype  = SENDERTYPE_TDMF_SERVER;
        if ( r == 0 )
            phdr->mngtstatus  = MMP_MNGT_STATUS_OK;
        else
            phdr->mngtstatus  = MMP_MNGT_STATUS_ERR_ERROR_ACCESSING_TDMF_DB;

        dbserver_mmp_dump( (char*)&msg, sizeof(mmp_mngt_TdmfCollectorParamsMsg_t), false, false, "\0", "MMP_MNGT_GET_DB_PARAMS" );

        mmp_convert_mngt_hdr_hton(phdr);
        mmp_convert_hton(&msg.data);

        int towrite = sizeof(mmp_mngt_TdmfCollectorParamsMsg_t);
        r = mmp_mngt_sock_send(request,(char*)&msg,towrite); _ASSERT(r == towrite);
    }
}


// ***********************************************************
// Function name    : dbserver_sock_recv_agent_state
// Description      : recv a MMP_MNGT_AGENT_STATE msg from a TDMF Agent 
// Return type      : none
// 
// ***********************************************************
static void         
dbserver_sock_recv_agent_state      (sock_t *request)
{
    int r,toread;
    mmp_mngt_TdmfAgentStateMsg_t msg;
    mmp_mngt_header_t *phdr = &msg.hdr;

    //mmp_mngt_header_t already read from socket
    //read the remainder of the message 
    toread = sizeof(mmp_mngt_TdmfAgentStateMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(request,(char*)(phdr+1),toread); _ASSERT(r == toread);
    if ( r != toread )
        return;//socket error

    mmp_convert_ntoh(&msg.data);

    dbserver_mmp_dump( (char*)(phdr+1), toread, true, true, msg.data.szServerUID, "MMP_MNGT_AGENT_STATE" );

    //update DB with server state
    dbserver_db_update_server_state(&msg.data,request->rip);
}

 
// ***********************************************************
// Function name    : dbserver_sock_sendmsg_to_TdmfAgent
// Description      : sends a msg to TDMF Agent and, if required, receive its response message 
// Return type      : int 
// Argument         : unsigned long rip
// Argument         : const char* senddata
// Argument         : int sendlen
// Argument         : char* rcvdata
// Argument         : int *rcvsize
// 
// ***********************************************************
int dbserver_sock_sendmsg_to_TdmfAgent(unsigned long rip, unsigned long rport, const char* senddata, int sendsize, char* rcvdata, int rcvsize) 
{
    int r;

    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, rport); 
        if ( r >= 0 ) 
        {
            //first send ftd_header_t to TDMF Agent
            if ( dbserver_sock_send_ftd_header(s) )
            {
                r = mmp_mngt_sock_send(s, (char*)senddata, sendsize); //_ASSERT(r == sendsize);
                //r = bytes sent or error status
                if ( r == sendsize )
                {   
                    if ( rcvdata != NULL )
                    {
                        r = mmp_mngt_sock_recv(s, rcvdata, rcvsize); //_ASSERT(r == rcvsize);
                        //r = bytes received or error status
                        if ( r != rcvsize)
                        {
                            r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                        }
                    }
                }
                else
                {
                    r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                }
            }
            else
            {
                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
            }
        }
        else
        {
            r = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT;
        }
        sock_disconnect(s);
    }
    sock_delete(&s);
    return r;
}




// ***********************************************************
// Function name    : dbserver_sock_sendmsg_recv_cfg
// Description      : sends a request for config. data to TDMF Agent 
//                    and manages the reception of this cfg data. 
// Return type      : int 0 if cfg received from Agent,
//                        < 0 if error :  
//                        -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT ,
//                        -MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT             
// Argument         : unsigned long rip
// Argument         : unsigned long rport
// Argument         : const char* senddata
// Argument         : int sendlen
// Argument         : char ** ppRcvdata : addr. of a ptr to be assigned by this function.
//                                        Upon success, ptr assigned to a valid mmp_mngt_ConfigurationMsg_t message.
//                                        Otherwise, ptr assigned to NULL.
//
// ***********************************************************
int dbserver_sock_sendmsg_recv_cfg(unsigned long rip, unsigned long rport, const char* senddata, int sendlen, mmp_mngt_ConfigurationMsg_t ** ppRcvdata) 
{
    int r;

    *ppRcvdata  = 0;

    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, rport);
        if ( r >= 0 ) 
        {
            //first send ftd_header_t to TDMF Agent
            if ( dbserver_sock_send_ftd_header(s) )
            {
                r = mmp_mngt_sock_send(s, (char*)senddata, sendlen); _ASSERT(r == sendlen);
                if ( r == sendlen )
                {
                    //mmp_mngt_ConfigurationMsg_t agentCfg;
                    mmp_mngt_header_t hdr;

                    //first read only the mngt msg header, store into agentCfg.
                    r = mmp_mngt_sock_recv(s, (char*)&hdr, sizeof(hdr)); _ASSERT(r == sizeof(hdr));
                    if ( r == sizeof(hdr) )
                    {
                        mmp_convert_mngt_hdr_ntoh(&hdr);

                        _ASSERT(hdr.mngttype == MMP_MNGT_SET_LG_CONFIG);

                        if ( hdr.mngttype == MMP_MNGT_SET_LG_CONFIG )
                        {   
                            mmp_mngt_recv_cfg_data( s, &hdr, ppRcvdata); 
                            if ( *ppRcvdata != 0 )
                            {
                                 r = 0;//success
                            }
                            else
                            {
                                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                            }
                        }
                        else
                        {
                            r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                        }
                    }
                    else
                    {
                        r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                    }
                }
                else
                {
                    r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                }
            }
            else
            {
                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
            }
        }
        else
        {
            r = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT;
        }
        sock_disconnect(s);
    }
    sock_delete(&s);

    return r;
}

// ***********************************************************
// Function name    : dbserver_sock_sendmsg_recv_file
// Description      : sends a request for TDMF file data to TDMF Agent 
//                    and manages the reception of this file data. 
// Return type      : int 0 if file received from Agent,
//                        < 0 if error :  
//                        -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT ,
//                        -MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT             
// Argument         : unsigned long rip
// Argument         : unsigned long rport
// Argument         : const char* senddata
// Argument         : int sendlen
// Argument         : char ** ppRcvdata : addr. of a ptr to be assigned by this function.
//                                        Upon success, ptr assigned to a valid mmp_mngt_FileMsg_t message.
//                                        Otherwise, ptr assigned to NULL.
//
// ***********************************************************
int dbserver_sock_sendmsg_recv_file(unsigned long rip, unsigned long rport, const char* senddata, int sendlen, mmp_mngt_FileMsg_t ** ppRcvdata) 
{
    int r;
    *ppRcvdata  = 0;
    mmp_mngt_FileMsg_t    *pFileData = 0;

    TraceInf("dbserver_sock_sendmsg_recv_file() to 0x%8x, port:0x%4x", rip, rport); 

    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, rport);
        if ( r >= 0 ) 
        {
            //first send ftd_header_t to TDMF Agent
            if ( dbserver_sock_send_ftd_header(s) )
            {
                r = mmp_mngt_sock_send(s, (char*)senddata, sendlen); _ASSERT(r == sendlen);
                if ( r == sendlen )
                {
                    //mmp_mngt_FileMsg_t agentCfg;
                    mmp_mngt_header_t hdr;

                    //first read only the mngt msg header, store into agentCfg.
                    r = mmp_mngt_sock_recv(s, (char*)&hdr, sizeof(hdr)); _ASSERT(r == sizeof(hdr));
                    if ( r == sizeof(hdr) )
                    {
                        mmp_convert_mngt_hdr_ntoh(&hdr);

                        _ASSERT(hdr.mngttype == MMP_MNGT_TDMF_SENDFILE);

                        if ( hdr.mngttype == MMP_MNGT_TDMF_SENDFILE )
                        {   
                            mmp_mngt_recv_file_data( s, &hdr, ppRcvdata); 

                            if ( *ppRcvdata != 0 )
                            {
                                 //pFileData = &ppRcvdata[0];
                                 TraceInf("dbserver_sock_sendmsg_recv_file(0x%8x) Rx: %s %d %d \n", 
                                           rip,
                                           ppRcvdata[0]->data.szFilename,                                          
                                           ppRcvdata[0]->data.iType,
                                           ppRcvdata[0]->data.uiSize); 
                                 r = 0;//success
                            }
                            else
                            {
                                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                            }
                        }
                        else
                        {
                            r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                        }
                    }
                    else
                    {
                        r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                    }
                }
                else
                {
                    r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
                }
            }
            else
            {
                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT;
            }
        }
        else
        {
            r = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT;
        }
        sock_disconnect(s);
    }
    sock_delete(&s);

    if ( r )
        TraceInf("dbserver_sock_sendmsg_recv_file(0x%8x) Error 0x%x\n", rip, r );
    
    return r;
}

// ***********************************************************
// TDMF file received from client has to be sent to Agent 
void  dbserver_sock_sendfile_req(CollectorConfig* cfg, sock_t *request, mmp_mngt_header_t *hdr ) 
{
    int r;
    mmp_mngt_FileMsg_t    *pFileData = 0; 

    //receive all configuration data from request socket
    mmp_mngt_recv_file_data( request, hdr, &pFileData ); 

    TraceInf("dbserver_sock_sendfile_req to %s, file %s , type %d, Size %d", 
                                               pFileData->szServerUID, 
                                               pFileData->data.szFilename, 
                                               pFileData->data.iType, 
                                               pFileData->data.uiSize);

    if ( pFileData != 0 )
    {
        //relay request to TDMF Agent
        unsigned long               agentIP = 0;
        unsigned int                agentPort = 0;
        mmp_mngt_FileStatusMsg_t    response;

        dbserver_get_agent_hostid(pFileData->szServerUID, pFileData->szServerUID);//if required, translate szServerUID information from Host Name to Host ID.
        dbserver_get_agent_ip_port(pFileData->szServerUID,&agentIP,&agentPort);

        if ( agentIP != 0 )
        {
            //send configuration request to TDMF Agent
            //send message almost as received, do only necessary modifications
            pFileData->hdr.sendertype    =   SENDERTYPE_TDMF_SERVER;
            mmp_convert_mngt_hdr_hton(&pFileData->hdr);
            //other fields of msg were filled above by mmp_mngt_recv_file_data()

            int towrite = sizeof(mmp_mngt_FileMsg_t) + pFileData->data.uiSize;

            pFileData->data.iType        = htonl(pFileData->data.iType);
            pFileData->data.uiSize       = htonl(pFileData->data.uiSize);

            //send msg to TDMF Agent.  Wait for response, mmp_mngt_ConfigurationStatusMsg_t, on same socket.
            r = dbserver_sock_sendmsg_to_TdmfAgent(agentIP, agentPort, (const char*)pFileData, towrite, (char*)&response, sizeof(response));
            if ( r == sizeof(response) )
            {   //success
                //relay Tdmf Agent response back to requester (below)
                response.hdr.mngtstatus  = ntohl(response.hdr.mngtstatus);
            }
            else
            {   //eror, could not get response from TDMF Agent
                //send back requester an error status
                response.hdr.mngtstatus  = -r;//mngt error status
                response.iStatus = htonl(1);// 1 = rx/tx error
            }
            //relay Tdmf Agent response back to requester
            //send message almost as received. do only necessary modifications
        }
        else
        {
            DBGPRINT((stderr,"****Error, unknown agent IP (0x%08x) or Port (%d)!",agentIP,agentPort));

            //relay Tdmf Agent response back to requester
            response.hdr.mngtstatus     = MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT;
        }

        response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
        response.hdr.mngttype       = MMP_MNGT_TDMF_SENDFILE_STATUS;
        response.hdr.sendertype     = SENDERTYPE_TDMF_SERVER;
        mmp_convert_mngt_hdr_hton(&response.hdr);
        r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); _ASSERT(r == sizeof(response));

        //release memory obtained from mmp_mngt_recv__data()
        mmp_mngt_free_file_data_mem(&pFileData);
    }
}

// ***********************************************************
bool         
dbserver_sock_send_ftd_header(sock_t *request)
{
    ftd_header_t ftdhdr;
    //only a few fields are required to be init in order for TDMF Agent to decode this ftd_header_t.
    //refer to ftd_master.c
    //todo : network byte order ???
    ftdhdr.cli = (HANDLE)1;//to be interpreted as a FTD_CON_IPC message
    ftdhdr.msgtype = FTDCMANAGEMENT;

    int r = mmp_mngt_sock_send(request, (char*)&ftdhdr, sizeof(ftdhdr)); _ASSERT(r == sizeof(ftdhdr));
    return r == sizeof(ftdhdr);
}

// ***********************************************************
bool dbserver_get_agent_ip_port(const char *szAgentId, unsigned long *agentIP, unsigned int *agentPort)
{
    *agentPort = *agentIP = 0;

    dbserver_db_get_agent_ip_port(szAgentId, agentIP, agentPort);

    //debug only 
    //_ASSERT(*agentIP != 0);_ASSERT(*agentPort != 0);

    return *agentIP != 0 && *agentPort != 0;
}

// ***********************************************************
void dbserver_get_agent_hostid(/*in*/const char *szAgentId, /*out*/char *szHostId)
{
    char *pBeginID;
    char szAgentIdCopy[256];

    strcpy(szAgentIdCopy,szAgentId);
    pBeginID = szAgentIdCopy; 

    if( strncmp( szAgentIdCopy, HOST_ID_PREFIX, HOST_ID_PREFIX_LEN ) == 0 )
    {   //this is a host id
        pBeginID += HOST_ID_PREFIX_LEN;
        /*
        //find out if host id is a hex or decimal number
        int base = 16;
        pBegin = pWk;
        //continue
        while( 0 != isxdigit( *pWk ) && base == 16)
        {
            if ( 0 == isdigit( *pWk ) )
            {   //hexadecimal but not decimal, so it has to be a decimal number
                base = 10;//set base and exit loop
            }
            pWk++;
        }
        strcpy(szHostId,pBegin);
        */

        //skip leading non-digits
        while( isspace( *pBeginID ) ) //returns a non-zero value on a white-space character (0x09 - 0x0D or 0x20)
            pBeginID++;
        strcpy(szHostId,pBeginID);
    }
    else
    {   //this is a machine name
        //get its host id from DB
        if ( !dbserver_db_get_agent_hostid(szAgentIdCopy, szHostId) )
        {
        }
    }
}

//************************************************************************
void dbserver_mmp_dump( const char* data, int size, bool bMsgRcvd, bool bAgentIsPeer, const char* szHostId, const char *szMMPmsgType )
{
    unsigned char        buf[4096],*pout,*poutbegin;
    const unsigned char  *pin = (unsigned char *)data;
    const unsigned char  *pinend = (unsigned char *)data + size;
    
    poutbegin = pout = buf;
    //try to avoid dynamic allocation for better performance.
    //each byte will be printed with 2 characters + 1 space = 3 chars
    //+1 for ending '\0'
    if ( ((size+2)/3 + 1 ) > sizeof(buf) )
    {
        poutbegin = pout = new unsigned char [ (size+1)*3 ];
    }

    while( pin < pinend )
    {
        pout = (unsigned char*)itoa( *pin, (char*)pout, 16) + 2;//char to hex  = 2 chars
        *pout = ' '; pout++;    //add space
        pin++;
    }
    *pout = '\0';

    //note : for TraceMMP() to log to CollectorTraces.log, these registry values are required:
    //          TraceFlag  has to be 1 or 3 (odd number)
    //          TraceLevel has to be >= 3 
    //          TraceMMP   has to be not 0 (1)
    if ( bMsgRcvd )
        //msg to or from Agent
        TraceMMP("BinDump: Msg RX from %8s, MsgType=%s, Msg data hex dump=%s", bAgentIsPeer ? szHostId : "GUI", szMMPmsgType,poutbegin);
    else
        //msg to or from GUI
        TraceMMP("BinDump: Msg TX to   %8s, MsgType=%s, Msg data hex dump=%s", bAgentIsPeer ? szHostId : "GUI", szMMPmsgType,poutbegin);


    if ( poutbegin != buf )
        delete [] poutbegin;
}
