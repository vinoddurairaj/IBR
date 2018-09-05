/*
 * ftd_mngt_stat.c - ftd management message handlers
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
#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#define ASSERT(exp)     _ASSERT(exp)
#define DBGPRINT(a)     fprintf a 
#else
#define ASSERT(exp)     ((void)0)
#define DBGPRINT(a)     ((void)0)
#endif

#define COPY_PERF_DATA_TO_LOCAL_BUF  0

extern "C" 
{
//#include "ftd_cfgsys.h"
#include "ftd_mngt.h"
#include "sock.h"
//#include "iputil.h"
}
#include "libmngtdef.h"
#include "libmngtmsg.h"

/////////////////////////////////////////////////////////////////////////////
/*
 * This thread is responsible for 
 */
static DWORD WINAPI ftd_mngt_stat_Thread(PVOID notused)
{
    ftd_perf_t              *perfp;
    ftd_perf_instance_t     *pDeviceInstanceData, *pWk;
#if COPY_PERF_DATA_TO_LOCAL_BUF 
    ftd_perf_instance_t     *pEnd;
    int                     iNbrDeviceInstance;
#endif
    mmp_TdmfPerfConfig      perfCfg;
    mmp_mngt_TdmfPerfMsg_t  perfmsg;

    //allow 5 seconds to allow system (driver, StatThread, etc.) to stabilize
    //before beginning to request performance data.
    Sleep(5000);

	if ( (perfp = ftd_perf_create()) == NULL) {
        ASSERT(0);
		return 1;
	}
 	// this opens the shared memory and mutex for you
    if ( ftd_perf_init(perfp, TRUE) ) {
        ASSERT(0);
		return 1;
	}

    ///////////////////////////////////////////////////
    //init some message members only once
    perfmsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    perfmsg.hdr.mngttype       = MMP_MNGT_PERF_MSG;
    perfmsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    perfmsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    //convert to network byte order now so it is ready to be sent on socket
    mmp_convert_mngt_hdr_hton(&perfmsg.hdr);
    //get host name
    ftd_mngt_getServerId( perfmsg.data.szServerUID );


    ///////////////////////////////////////////////////
    //allocate buffer to be filled with performance data 
#if COPY_PERF_DATA_TO_LOCAL_BUF 
    iNbrDeviceInstance      = 50;
    pDeviceInstanceData     = new ftd_perf_instance_t[ iNbrDeviceInstance ];
    pEnd                    = pDeviceInstanceData + iNbrDeviceInstance;
#endif


    ///////////////////////////////////////////////////
    //todo : add event to exit thread ?
    while( true )
    {
	    BOOL			bFreeMutex;
	    int				rc;

        //new values could have been received
        ftd_mngt_GetPerfConfig(&perfCfg);
        //pace thread
        Sleep(perfCfg.iPerfUploadPeriod*100);

        ///////////////////////////////////////////////////
#if COPY_PERF_DATA_TO_LOCAL_BUF 
	    memset(pDeviceInstanceData, 0, sizeof(ftd_perf_instance_t)*iNbrDeviceInstance );
#endif
        //request stat from StatThread
 	    SetEvent(perfp->hGetEvent);
        //wait for stats to be available from StatThread
 	    if ((rc = WaitForSingleObject(perfp->hSetEvent, 1000)) != WAIT_OBJECT_0) {
 		    if (rc == WAIT_TIMEOUT) {
 			    //return -2;
		    }
		    //return -1;
            continue;
	    }
 
        ///////////////////////////////////////////////////
	    // ok now get the data
        if (perfp->hMutex != NULL) {
            if (WaitForSingleObject (perfp->hMutex, SHARED_MEMORY_MUTEX_TIMEOUT) == WAIT_FAILED) {
                // unable to obtain a lock
                bFreeMutex = FALSE;
            } else {
                bFreeMutex = TRUE;
            }
        } else {
            bFreeMutex = FALSE;
        }
 
        // point to the first instance structure in the shared buffer
        ftd_perf_instance_t *pThisDeviceInstanceData = perfp->pData;
 
#if COPY_PERF_DATA_TO_LOCAL_BUF 
        pWk = pDeviceInstanceData;
#else
        pWk = pDeviceInstanceData = perfp->pData;
#endif
        // process each of the instances in the shared memory buffer
        while (*((int*)pThisDeviceInstanceData) != -1){

#if COPY_PERF_DATA_TO_LOCAL_BUF 
            if ( pWk == pEnd )
            {   //destination buffer is not large enough
                //create a larger buffer
                ftd_perf_instance_t *pNewDeviceInstanceData = new ftd_perf_instance_t[ iNbrDeviceInstance + 25 ];
                //copy existing data to larger buffer
                memmove( pNewDeviceInstanceData, pDeviceInstanceData, (char*)pWk-(char*)pDeviceInstanceData );
                delete [] pDeviceInstanceData;
                //reposition pointers in larger buffer
                pDeviceInstanceData = pNewDeviceInstanceData;
                pEnd                = pDeviceInstanceData + iNbrDeviceInstance + 25;
                pWk                 = pDeviceInstanceData + iNbrDeviceInstance;
                iNbrDeviceInstance  += 25;
            }
            // set pointer to first counter data field
		    // set pDeviceInstanceData for each device
		    pWk->role       = pThisDeviceInstanceData->role;
		    pWk->connection = pThisDeviceInstanceData->connection;
		    pWk->drvmode    = pThisDeviceInstanceData->drvmode;
		    
		    pWk->devid      = pThisDeviceInstanceData->devid;
		    pWk->lgnum      = pThisDeviceInstanceData->lgnum;
		    
		    pWk->actual     = pThisDeviceInstanceData->actual;
		    pWk->effective  = pThisDeviceInstanceData->effective;
		    pWk->rsyncoff   = pThisDeviceInstanceData->rsyncoff;
		    pWk->rsyncdelta = pThisDeviceInstanceData->rsyncdelta;
		    pWk->entries    = pThisDeviceInstanceData->entries;
		    pWk->sectors    = pThisDeviceInstanceData->sectors;
		    pWk->pctdone    = pThisDeviceInstanceData->pctdone;
		    pWk->pctbab     = pThisDeviceInstanceData->pctbab;
		    pWk->bytesread  = pThisDeviceInstanceData->bytesread;
		    pWk->byteswritten = pThisDeviceInstanceData->byteswritten;
		    memcpy(pWk->wcszInstanceName, pThisDeviceInstanceData->wcszInstanceName, MAX_SIZEOF_INSTANCE_NAME * 2);
#endif
		    
		    pWk++;
            // setup for the next instance
            pThisDeviceInstanceData++;
        }

#if COPY_PERF_DATA_TO_LOCAL_BUF == 0
        // done with the shared memory so free the mutex if one was 
        // acquired
        if (bFreeMutex) {
            ReleaseMutex (perfp->hMutex);
        }
#endif

        ///////////////////////////////////////////////////
        //send performance data to TDMF Collector
        //
        if ( pWk != pDeviceInstanceData )
        {
            //data is available in pDeviceInstanceData.
            //transfer to TDMF collector
            perfmsg.data.iPerfDataSize = (char*)pWk - (char*)pDeviceInstanceData;
            //convert to network byte order before sending on socket
            mmp_convert_TdmfPerfData_hton(&perfmsg.data);

            int towrite,collectorIP,collectorPort;
            ftd_mngt_GetCollectorInfo(&collectorIP, &collectorPort);
            if ( collectorIP != 0 && collectorPort != 0 )
            {
                int r;
                sock_t * s = sock_create();
                r = sock_init( s, NULL, NULL, 0, collectorIP, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
                if ( r >= 0 ) 
                {
                    r = sock_connect(s, collectorPort); _ASSERT(r>=0);
                    if ( r >= 0 ) 
                    {
                        towrite = sizeof(mmp_mngt_TdmfPerfMsg_t);
                        r = mmp_mngt_sock_send(s, (char*)&perfmsg, towrite);
                        if ( r == towrite )
                        {   //send vector of ftd_perf_instance_t 
                            towrite = (char*)pWk - (char*)pDeviceInstanceData;
                            mmp_mngt_sock_send(s, (char*)pDeviceInstanceData, towrite);
                        }
                    }
                    sock_disconnect(s);
                }
                sock_delete(&s);
            }
        }
        ///////////////////////////////////////////////////

#if COPY_PERF_DATA_TO_LOCAL_BUF == 1
        // done with the shared memory so free the mutex if one was 
        // acquired
        if (bFreeMutex) {
            ReleaseMutex (perfp->hMutex);
        }
#endif

    }//infinite loop ...

    delete [] pDeviceInstanceData;
    ftd_perf_delete(perfp);
    return 0;
}


void    ftd_mngt_stat_initialize()
{
    DWORD tid;
    HANDLE hThread = CreateThread(0,0,ftd_mngt_stat_Thread,0,0,&tid);
    CloseHandle(hThread);
}




