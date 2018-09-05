/*
 * ftd_mngt_perf.cpp - ftd management message handlers
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
#pragma warning(disable:4786)
#include <map>
#include <time.h>
extern "C" 
{
#include "ftd_mngt.h"
#include "sock.h"
//#include "ftd_perf.h"
}
#include "libmngtdef.h"
#include "libmngtmsg.h"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#define DBGPRINT(a)     ((void)0)   //printf a
#else
#define ASSERT(exp)     ((void)0)
#define DBGPRINT(a)     ((void)0)
#endif

/////////////////////////////////////////////////////////////////////////////
static ftd_perf_instance_t *pPrevDeviceInstanceData = 0;
static ftd_perf_instance_t *pPrevDeviceInstanceDataEnd = 0;
static ftd_perf_instance_t *pWkDeviceInstanceData = 0;
static ftd_perf_instance_t *pWkDeviceInstanceDataEnd = 0;
static int  gEmulatorStatMsgCntr = 0;
static char gszServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];
static CRITICAL_SECTION g_csLockRepGrpState;
static bool gbCSInit = false;
static bool gbForceTxAllGrpState;
static bool gbForceTxAllGrpPerf; 
static HANDLE ghEvForceStatAcquisition;
static short    gsMode;
static time_t   gtLastRawPerfTx ;
static time_t   guiTdmfPerf_FastRateSendEndTime;
// Saumya Tripathi 06/23/04
// BofA Temporary Fix:: Tried on BofA system on 06/02/04
static int iResendCollector = 0;
static bool bResendCollector = false;


/////////////////////////////////////////////////////////////////////////////
static bool     ftd_mngt_performance_is_time_to_upld(time_t now, time_t deltat);
static time_t   ftd_mngt_get_force_fast_send_end_time();


/////////////////////////////////////////////////////////////////////////////
class RepGrpState
{
public:
    RepGrpState()
        {
            m_state.sState = 0;
            m_state.sRepGrpNbr = -1;

            m_time_last_perf    = 0;
            m_prev_actual       = 0;
            m_prev_effective    = 0;
            m_prev_bytesread    = 0;
            m_prev_byteswritten = 0;

        };
    RepGrpState(short sGrpNbr,bool bStarted)
        {
            m_state.sState = 0;
            setGroupNbr(sGrpNbr);
			setStartState(bStarted);

            m_time_last_perf    = 0;
            m_prev_actual       = 0;
            m_prev_effective    = 0;
            m_prev_bytesread    = 0;
            m_prev_byteswritten = 0;
        };
    ~RepGrpState()
        {
        };

	bool operator<(const RepGrpState & grpstate) const
	{
		return grpstate.m_state.sRepGrpNbr < m_state.sRepGrpNbr;
	} 
	//bit 0 indicates if group is started (1) or stopped (0)
    void setStartState(bool bNewStartState) 
    	{
            short prevState = m_state.sState;
    		m_state.sState  = (m_state.sState & 0xfffe) | (bNewStartState ? 0x1 : 0x0) ;//set or clear bit 0 
            //preserve a 'true' m_bStateChanged until hasGroupStateChanged() is called
            m_bStateChanged = m_bStateChanged || (prevState != m_state.sState);
    	};
	//bit 1 indicates if group is in checkpoint mode (1) or not (0)
    void setCheckpointState(bool bIsInCheckpoint) 
    	{ 
            short prevState = m_state.sState;
    		m_state.sState  = (m_state.sState & 0xfffd) | (bIsInCheckpoint ? 0x2 : 0x0) ;//set or clear bit 1
            //preserve a 'true' m_bStateChanged until hasGroupStateChanged() is called
            m_bStateChanged = m_bStateChanged || (prevState != m_state.sState);
    	};
	void setGroupNbr(short sGrpNbr)
		{
			m_state.sRepGrpNbr = sGrpNbr;
		};
	short getGroupNbr(void) const
		{
			return m_state.sRepGrpNbr;
		};
	const mmp_TdmfGroupState getGroupState() const 
		{ 
			return m_state; 
		};
	bool  isGroupStarted() const 
		{ 
			return m_state.sState & 0x1;//bit 0 = group started/stopped 
		};
	bool  hasGroupStateChanged() 
		{ 
            bool ret = m_bStateChanged;                            
            m_bStateChanged = false;//auto-reset 
			return ret;
		};

    inline void  setRateDeltaTime()
        {
            time_t now = time(0);
            m_deltatime_perf = now - m_time_last_perf;
            m_time_last_perf = now;//prepare for next call
        };

    inline __int64 getRate( __int64 actVal, __int64 prevVal )
        {   //1024 -> in kilo Bytes per second
            if ( m_deltatime_perf == 0 )
                m_deltatime_perf = 1;
            if ( actVal > prevVal )
                //return (actVal - prevVal) / 1024 / m_deltatime_perf;
                return (actVal - prevVal) / m_deltatime_perf; // ardev 030203
            else
                return 0;
        };


	mmp_TdmfGroupState	m_state;
    bool                m_bStateChanged;


    DWORD       m_time_last_perf,//time in second 
                m_deltatime_perf;
    __int64     m_prev_actual,
                m_prev_effective,
                m_prev_bytesread,
                m_prev_byteswritten;

};
//element in map means group is started; 
//map index is group number
static std::map<short,RepGrpState>  gmapRepGrpState;

/////////////////////////////////////////////////////////////////////////////
// group state sent to Collector by StatThread()
// called from another thread than ftd_mngt_performance_send_data()
void ftd_mngt_performance_set_group_cp(short sGrpNumber, int bIsCheckpoint)
{
	RepGrpState & grpstate = gmapRepGrpState[ sGrpNumber ];
	grpstate.setGroupNbr(sGrpNumber);
    if ( gbCSInit ) EnterCriticalSection(&g_csLockRepGrpState);    
    grpstate.setCheckpointState(bIsCheckpoint != 0 ? true : false);
    if ( gbCSInit ) LeaveCriticalSection(&g_csLockRepGrpState);
}


/////////////////////////////////////////////////////////////////////////////
/*
 * This function is called within the StatThread() 
 */
int ftd_mngt_performance_init()
{
    gEmulatorStatMsgCntr = 0;

    pPrevDeviceInstanceData = new ftd_perf_instance_t[SHARED_MEMORY_ITEM_COUNT];
    if (pPrevDeviceInstanceData != 0 )
    {
        memset(pPrevDeviceInstanceData,0,sizeof(ftd_perf_instance_t)*SHARED_MEMORY_ITEM_COUNT);
        pPrevDeviceInstanceDataEnd = pPrevDeviceInstanceData + SHARED_MEMORY_ITEM_COUNT;
    }
    pWkDeviceInstanceData = new ftd_perf_instance_t[SHARED_MEMORY_ITEM_COUNT];
    if (pWkDeviceInstanceData != 0 )
    {
        pWkDeviceInstanceDataEnd = pWkDeviceInstanceData + SHARED_MEMORY_ITEM_COUNT;
    }

    ftd_mngt_getServerId( gszServerUID );//acquire only once   
    DBGPRINT(("\n\n   gszServerUID=<%s>\n",gszServerUID));

    if ( !gbCSInit )
    {
        InitializeCriticalSection(&g_csLockRepGrpState);
        gbCSInit = true;
    }

    gmapRepGrpState.clear();

    gbForceTxAllGrpState  = true;
    gbForceTxAllGrpPerf   = true; 
    gtLastRawPerfTx       = 0;
    guiTdmfPerf_FastRateSendEndTime = 0;

    ghEvForceStatAcquisition = CreateEvent(0,0,0,0);

    return pPrevDeviceInstanceData == 0 ? 0 : 1;
}

void ftd_mngt_performance_end()
{
    delete [] pPrevDeviceInstanceData;
    pPrevDeviceInstanceData = 0;

    delete [] pWkDeviceInstanceData;
    pWkDeviceInstanceData = 0;

    CloseHandle(ghEvForceStatAcquisition);
    ghEvForceStatAcquisition = NULL;

    gmapRepGrpState.clear();

    if (gbCSInit)
        DeleteCriticalSection(&g_csLockRepGrpState);
}

//immediat request to statd to acquire statistics
void ftd_mngt_performance_force_acq()
{
    if ( ghEvForceStatAcquisition )
        SetEvent(ghEvForceStatAcquisition);
}

HANDLE ftd_mngt_performance_get_force_acq_event()
{
    return ghEvForceStatAcquisition;
}

/*
 * Make sure to update Collector with ALL the LATEST information.
 */
void ftd_mngt_performance_send_all_to_Collector()
{
    gbForceTxAllGrpState  = true;
    gbForceTxAllGrpPerf   = true; 
}

/*
 * This function is called within the StatThread() 
 */
void ftd_mngt_performance_send_data(const ftd_perf_instance_t *pDeviceInstanceData, int iNPerfData)
{
    mmp_mngt_TdmfPerfMsg_t  		perfmsg;
    const ftd_perf_instance_t       *pWkEnd;
    ftd_perf_instance_t             *pWkConv;
    BOOL                            MessageSent = FALSE;

    //better safe than sorry ...
    if( iNPerfData > SHARED_MEMORY_ITEM_COUNT )
    {
        ASSERT(0);
        iNPerfData = SHARED_MEMORY_ITEM_COUNT;
    }

    ///////////////////////////////////////////////////
    if ( iNPerfData > 0 )
    {   //make local work copy of data.  local copy can be modified ( hton ) at will.
        //pDeviceInstanceData content MUST NOT modified.
        memcpy( pWkDeviceInstanceData, pDeviceInstanceData, iNPerfData*sizeof(ftd_perf_instance_t) );
	} 
    //position pWkEnd at end of valid working data
    pWkEnd = pWkDeviceInstanceData + iNPerfData;

    ///////////////////////////////////////////////////
    int towrite,collectorIP,collectorPort;
    ftd_mngt_GetCollectorInfo(&collectorIP, &collectorPort);
	// By Saumya Tripathi 03/25/04 
	// Fixing WR 32920
	// do not try to talk to collector if collector not present.
    if ( collectorIP == 0 || collectorPort == 0 || ( ftd_mngt_IfCollectorPresent() == 0) )
    {
		return;
	}

    ///////////////////////////////////////////////////
    //Verify if any of the Repl. Group state have changed. If so, send it.
    //
    bool bGrpStateHdrSent = false;
    int  r;
    sock_t * s = sock_create();
  	r = sock_init( s, NULL, NULL, 0, collectorIP, SOCK_STREAM, AF_INET, 1, 0); ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, collectorPort);
        if ( r >= 0 ) 
        {
            {   //send vector of group states. 
                bool bForceTxAllGrpState = gbForceTxAllGrpState;//local copy = multi-thread protection
                bool bStateChange = false;//flag indicating if at least one group changed state

				//add started groups to gmapRepGrpState[] map
		        for( pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd ; pWkConv++ )
		        {   //group stats : lgnum == -100 , devid = lg number
					if ( pWkConv->lgnum == -100 && pWkConv->devid >= 0 )
					{	//a group in pWkDeviceInstanceData vector means it is started.
						RepGrpState & grpstate = gmapRepGrpState[ pWkConv->devid ];
                        EnterCriticalSection(&g_csLockRepGrpState);
						grpstate.setStartState(true);
                        LeaveCriticalSection(&g_csLockRepGrpState);
						grpstate.setGroupNbr(pWkConv->devid);
					}
		        }
				//send all group state to Collector
				std::map<short,RepGrpState>::iterator it = gmapRepGrpState.begin();
				std::map<short,RepGrpState>::iterator end = gmapRepGrpState.end();
				while( it != end )
				{
					bool bGroupStopped = true;
					RepGrpState & grpstate = (*it).second;
					//now check for groups that have been stopped
					if ( grpstate.isGroupStarted() )
					{	//verify if group is still active - started    
				        for( pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd ; pWkConv++ )
				        {   //group stats : lgnum == -100 , devid = lg number
					        if ( pWkConv->lgnum == -100 && pWkConv->devid >= 0 )
                            {
							    if ( grpstate.getGroupNbr() == pWkConv->devid )
                                {
								    bGroupStopped = false;//group still started
                                    break;//leave for() loop
                                }
                            }
						}
						if ( bGroupStopped )
						{	//group in map is not found in pWkDeviceInstanceData -> group has been stopped
                            EnterCriticalSection(&g_csLockRepGrpState);
							grpstate.setStartState(false);
                            LeaveCriticalSection(&g_csLockRepGrpState);
						}
					}

					////////////////////////////////////
					//send this group state to Collector, if required
                    if ( (bResendCollector = grpstate.hasGroupStateChanged()) || bForceTxAllGrpState || (iResendCollector > 0) )
                    {
						// Saumya Tripathi 06/23/04
						// BofA Temporary Fix:: Tried on BofA system on 06/02/04
						if ( bResendCollector )
						{
							iResendCollector = 25;
						}
						else
						{
							if ( iResendCollector > 0 )
							{
								iResendCollector--;
							}
						}

                        // now that we know we have to send a msg, make sure the header has been sent once.
                        if ( !bGrpStateHdrSent )
                        {   
	                        mmp_mngt_TdmfGroupStateMsg_t	statemsg;
	                        statemsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
	                        statemsg.hdr.mngttype       = MMP_MNGT_GROUP_STATE;
	                        statemsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
	                        statemsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
	                        //convert to network byte order now so it is ready to be sent on socket
	                        mmp_convert_mngt_hdr_hton(&statemsg.hdr);
	                        //get host name
	                        strcpy( statemsg.szServerUID, gszServerUID );
			                //write msg hdr 
                            towrite = sizeof(statemsg);
                            r = mmp_mngt_sock_send(s, (char*)&statemsg, towrite);
                            if ( r == towrite )
							{
                                bGrpStateHdrSent = true;//send hdr only once
							}
                        }

                        mmp_TdmfGroupState state;
						short tmpState = 0;
						short tmpGrpNumber = 0;
		                towrite = sizeof(state);
				        state   = grpstate.getGroupState();
						tmpState = state.sState;
						tmpGrpNumber = state.sRepGrpNbr;
	        	        mmp_convert_hton(&state);
		                r = mmp_mngt_sock_send(s, (char*)&state, towrite);
						// Saumya Tripathi 06/23/04
						// check for socket write error and log into the event log
						if ( r != towrite )
						{
                           	error_syslog(ERRFAC,LOG_WARNING,"****Warning, Group State: %d for Group Number: %d could not be sent to the Collector!!\n", tmpState, tmpGrpNumber );
						}
                        
                        MessageSent = TRUE;

                    }

					it++;
				}

                if ( bGrpStateHdrSent )
                {   ////////////////////////////////////
				    //send the last one that acts as a end of TX flag
					short tmpState = 0;
					short tmpGrpNumber = 0;
	                towrite = sizeof(mmp_TdmfGroupState);
				    mmp_TdmfGroupState grpstate = {-1,-1};
					tmpState = grpstate.sState;
					tmpGrpNumber = grpstate.sRepGrpNbr;
        		    mmp_convert_hton(&grpstate);
	                r = mmp_mngt_sock_send(s, (char*)&grpstate, towrite);
					// Saumya Tripathi 06/23/04
					// check for socket write error and log into the event log
					if ( r != towrite )
					{
						error_syslog(ERRFAC,LOG_WARNING,"****Warning, Group State: %d for Group Number: %d could not be sent to the Collector!!\n", tmpState, tmpGrpNumber );					}

                    MessageSent = TRUE;
                }
                //reset flag
                gbForceTxAllGrpState = false;
            }
        }
        sock_disconnect(s);//marks end of MMP_MNGT_GROUP_STATE msg TX
    }


    ///////////////////////////////////////////////////
    time_t now = time(0);
    ///////////////////////////////////////////////////
    //check if any groups are in REFRESH mode.
    //if so, make sure the stat update period remains at one second .
    gsMode = FTD_MODE_NORMAL;
    for( pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd ; pWkConv++ )
    {   //group stats : lgnum == -100 , devid = lg number
	    if ( pWkConv->lgnum == -100 && pWkConv->devid >= 0 )
        {
            if ( (pWkConv->drvmode & FTD_MODE_REFRESH) == FTD_MODE_REFRESH      ||
                 (pWkConv->drvmode & FTD_MODE_BACKFRESH) == FTD_MODE_BACKFRESH  )
            {   //make sure the stat update period remains at one second for another second.
                //ftd_mngt_performance_reduce_stat_upload_period(2,0);
                gsMode = FTD_MODE_REFRESH;
                break;
            }
        }
    }
    
    ///////////////////////////////////////////////////////////
    //is it time to send performance data to TDMF Collector ??
    //
    if ( ftd_mngt_performance_is_time_to_upld(now, now - gtLastRawPerfTx) || gbForceTxAllGrpPerf )
    {   //yes

        ///////////////////////////////////////////////////
        //send performance data to TDMF Collector
        //
        //in NORMAL mode avoid to resend same perf msg.
        //in EMULATOR mode, send systematically, without comparing.
        /*
        if ( !EMULATOR_MODE )
        {   
            if ( 0 == memcmp( pDeviceInstanceData, pPrevDeviceInstanceData, iNPerfData*sizeof(ftd_perf_instance_t) )
                 && !gbForceTxAllGrpPerf )//do not return here if must send Grp Perf data 
            {           
                //all data is identical to last transmission data, so no need to resend this again to Collector
                //printf("\nDEBUG only ! : no stat msg reduction...");
                sock_delete(&s);
                return;
            }

            //update local buffer for next call
            memcpy( pPrevDeviceInstanceData, pDeviceInstanceData, iNPerfData*sizeof(ftd_perf_instance_t) );
        }
        */

        if ( pWkEnd != pWkDeviceInstanceData )
        {
            ///////////////////////////////////////////////////
            //init some message members only once
            perfmsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
            perfmsg.hdr.mngttype       = MMP_MNGT_PERF_MSG;
            perfmsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
            perfmsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
            //convert to network byte order now so it is ready to be sent on socket
            mmp_convert_mngt_hdr_hton(&perfmsg.hdr);
            //get host name
            strcpy( perfmsg.data.szServerUID, gszServerUID );

            //transfer to TDMF collector
            perfmsg.data.iPerfDataSize = (char*)pWkEnd - (char*)pWkDeviceInstanceData;
            //convert to network byte order before sending on socket
            //note : data within pWkDeviceInstanceData is modified by hton .
            mmp_convert_TdmfPerfData_hton(&perfmsg.data);

            //compute rate values
		    for( pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd ; pWkConv++ )
		    {   //GROUP stats (not device stats) : lgnum == -100 , devid = lg number
				if ( pWkConv->lgnum == -100 && pWkConv->devid >= 0 )
				{	
                    __int64 rate;
                    RepGrpState & grpstate = gmapRepGrpState[ pWkConv->devid /*grp nbr*/ ];
                    grpstate.setRateDeltaTime();

                    rate                    = grpstate.getRate( pWkConv->actual, grpstate.m_prev_actual );
                    grpstate.m_prev_actual  = pWkConv->actual;
                    pWkConv->actual         = rate;

                    rate                    = grpstate.getRate( pWkConv->effective, grpstate.m_prev_effective );
                    grpstate.m_prev_effective  = pWkConv->effective;
                    pWkConv->effective         = rate;

                    rate                    = grpstate.getRate( pWkConv->bytesread, grpstate.m_prev_bytesread );
                    grpstate.m_prev_bytesread  = pWkConv->bytesread;
                    pWkConv->bytesread         = rate;

                    rate                    = grpstate.getRate( pWkConv->byteswritten, grpstate.m_prev_byteswritten );
                    grpstate.m_prev_byteswritten = pWkConv->byteswritten;
                    pWkConv->byteswritten        = rate;
                }
            }

            //convert all ftd_perf_instance_t values to network byte order
            for( pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd ; pWkConv++ )
            {
                mmp_convert_hton(pWkConv);
            }

            //
            //the following section of code is done ONLY ONCE when EMULATOR_MODE == false.
            //otherwise, more iterations are performed.
            //
            int iEmulatorIdx = gTdmfAgentEmulator.iAgentRangeMin;
            do
            {
                //section begin
                //this section of code is done only once when not emulating servers - EMULATOR_MODE == false
                r = sock_init( s, NULL, NULL, 0, collectorIP, SOCK_STREAM, AF_INET, 1, 0); ASSERT(r>=0);
                if ( r >= 0 ) 
                {
                    r = sock_connect(s, collectorPort);
                    if ( r >= 0 ) 
                    {
                        towrite = sizeof(mmp_mngt_TdmfPerfMsg_t);
                        r = mmp_mngt_sock_send(s, (char*)&perfmsg, towrite);

                        if ( r == towrite )
                        {   //send vector of ftd_perf_instance_t 
                            towrite = (char*)pWkEnd - (char*)pWkDeviceInstanceData;
                            mmp_mngt_sock_send(s, (char*)pWkDeviceInstanceData, towrite);
                            //reset flag on first successful tx
                            gbForceTxAllGrpPerf = false;
                        }

                        MessageSent = TRUE;
                    }
                    sock_disconnect(s);
                }
                //section end

                if (EMULATOR_MODE)
                {   //change host id
                    ftd_mngt_emulatorGetHostID(gszServerUID, iEmulatorIdx, perfmsg.data.szServerUID, sizeof(perfmsg.data.szServerUID));
                    //use 'insert' as a message counter. This allows to detect missed message.
                    for( pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd ; pWkConv++ )
                        pWkConv->insert = htonl( gEmulatorStatMsgCntr++ );
                }

            }while( EMULATOR_MODE && iEmulatorIdx++ <= gTdmfAgentEmulator.iAgentRangeMax );

            gtLastRawPerfTx = now;

            DBGPRINT(("\nRAW PERF sent at %ld",now));    
        }// if ( pWkEnd != pWkDeviceInstanceData )

    }// if ( ftd_mngt_performance_is_time_to_upld() )

    //
    // If we sent a message in this pass, update the alive message time stamp
    // (this means the alive message will wait +10secs before being sent)
    //
    if (MessageSent)
    {
        ftd_mngt_UpdateAliveMsgTime();
    }
    ///////////////////////////////////////////////////
    sock_delete(&s);
}

//request that the perf upload period be 1 second for the next N seconds
//after that period, it falls back to the configured gTdmfPerfConfig.iPerfUploadPeriod.
void    ftd_mngt_performance_reduce_stat_upload_period(int iFastPerfPeriodSeconds, int bWakeUpStatThreadNow)
{
    time_t now = time(0);
    if ( guiTdmfPerf_FastRateSendEndTime > now )
    {   //add on top of remainding delay already requested
        guiTdmfPerf_FastRateSendEndTime = guiTdmfPerf_FastRateSendEndTime - now + iFastPerfPeriodSeconds;
    }
    else 
    {
        guiTdmfPerf_FastRateSendEndTime = now + iFastPerfPeriodSeconds;
    }
    if ( bWakeUpStatThreadNow )
        ftd_mngt_performance_force_acq();//immediat wake up of StatThread
}

//time at which the Agent may stop sending perf stats at highest frequency.
static time_t ftd_mngt_get_force_fast_send_end_time()
{
    return guiTdmfPerf_FastRateSendEndTime;
}

///////////////////////////////////////////////////////
//STATD thread wake-up period
//
//IMMEDIAT : timeout when a TDMF command has been received and fast perf stats/mode feedback is required.
#define IMMEDIAT_WAKEUP_PERIOD       1 //  1 second
//ACTION : timeout when at least one group is in REFRESH/BACKFRESH mode
#define ACTION_WAKEUP_PERIOD         3 //  3 seconds
//DEFAULT : timeout when nothing particular occurs on agent
#define DEFAULT_WAKEUP_PERIOD       10 // 10 seconds

// this function provides the statd timeout/wake-up value, 
// it is based on group states / mode.
int ftd_mngt_performance_get_wakeup_period()
{
    int timeout;

    //top priority
    if ( time(0) <= ftd_mngt_get_force_fast_send_end_time() )
    {
        return IMMEDIAT_WAKEUP_PERIOD;
        DBGPRINT(("\nRAW PERF : IMMEDIAT_WAKEUP_PERIOD"));    
    }
    else if ( gsMode != FTD_MODE_NORMAL )
    {
        timeout = ACTION_WAKEUP_PERIOD;
        DBGPRINT(("\nRAW PERF : ACTION_WAKEUP_PERIOD"));    
    }
    else 
    {
        timeout = DEFAULT_WAKEUP_PERIOD;
        DBGPRINT(("\nRAW PERF : DEFAULT_WAKEUP_PERIOD"));    
    }
    return timeout;
}


///////////////////////////////////////////////////////
//RAW PERF TX up period
//
#define IDLE_PERF_DATA_TX_WAKEUP_PERIOD       60
#define FOCUS_PERF_DATA_TX_WAKEUP_PERIOD      10
#define ACTION_PERF_DATA_TX_WAKEUP_PERIOD      3
#define IMMEDIATE_PERF_DATA_TX_WAKEUP_PERIOD   1
//
//period requested by Collector (from GUI)
static bool  ftd_mngt_performance_is_time_to_upld(time_t now, time_t deltat)
{
    extern mmp_TdmfPerfConfig gTdmfPerfConfig;

    //top priority
    if ( now <= ftd_mngt_get_force_fast_send_end_time() )
    {
        DBGPRINT(("\nRAW PERF mode : Fast Send forced"));
        return true;
    }
    //second priority
    else if ( gsMode != FTD_MODE_NORMAL )
    {   //action is going on here, (in REFRESH or other)
        DBGPRINT(("\nRAW PERF mode : NOT FTD_MODE_NORMAL"));
        return ( deltat >= ACTION_PERF_DATA_TX_WAKEUP_PERIOD );
    }
    else
    {   //mode NORMAL
        time_t requested_upld_period = gTdmfPerfConfig.iPerfUploadPeriod/10;//convert to seconds

        DBGPRINT(("\nRAW PERF mode : FTD_MODE_NORMAL"));
        return ( deltat >= requested_upld_period );
    }
}