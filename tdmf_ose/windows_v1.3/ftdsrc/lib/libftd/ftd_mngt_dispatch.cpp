/*
 * ftd_mngt_dispatch.c - ftd management message dispatcher
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


//// Mike Pollett need for windows .h files
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif

extern "C" 
{
#include "sock.h"
#include "iputil.h"
#include "ftd_cfgsys.h"
#include "ftd_mngt.h"
}
#include "libmngtmsg.h"

//defined in ftd_mngt.cpp
extern int  giTDMFCollectorIP   ;    //updated each time a msg is received from Collector
extern int  giTDMFCollectorPort ;  //default values


#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#else
#define ASSERT(exp)     ((void)0)
#endif
#define DBGPRINT(a)     ftd_mngt_tracef a 


///////////////////////////////////////////////////////////////////////////////
//ftd_mngt.cpp
/* local prototypes */
static unsigned int __stdcall receiveMngtMsgThread(void* pContext);
errfac_t * gMngtErrfac;

///////////////////////////////////////////////////////////////////////////////
/*
 * A MMP message request has to be received and processed.
 *
 * Message is expected to be a MMP MANAGEMENT msg.
 * Receive message from a SOCK_STREAM connection.
 */
int 
ftd_mngt_recv_msg(ftd_sock_t *fsockp)
{
	// By Saumya Tripathi 03/05/04
	// With these modifications it will run in a collector less environment also
	// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
	// still work; all the CLIs will work too
	if( giTDMFCollectorIP != 0 )
	{
	//create a thread to process request ?
    //indicate to caller that socket has to remain alive : keepalive = TRUE
    fsockp->keepalive = TRUE;
    /*
    //threads created with CreateThread() can be the cause of memory leaks when they terminate.
    //see MSDN, Q104641
    DWORD tid;
    HANDLE hThread = CreateThread(0,0,receiveMngtMsgThread,(void*)fsockp,0,&tid);
    CloseHandle(hThread);
    */
    unsigned int tid;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL,0,receiveMngtMsgThread,fsockp,0,&tid);ASSERT(hThread!=0);
    ::CloseHandle(hThread);
	}

    return 0;
}

static unsigned int __stdcall 
receiveMngtMsgThread(void *pContext)
{
	// Mike Pollett modified for new masterthread
 //   ftd_sock_t *fsockp  = (ftd_sock_t *)pContext;
    ftd_sock_t *fsockp  = NULL;
    sock_t     *sockp   = fsockp->sockp;
    //////////////////////////////////
    int r;
    mmp_mngt_header_t   hdr;

	// Mike Pollett modified for new masterthread
        // create an ipc socket and connect it to the local master ipc listener
        if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
		    return -1;
		}
	// Mike Pollett modified for new masterthread
		fsockp->sockp->sock = (int)pContext;


    //////////////////////////////////
    //init thread specific value ERRFAC
#if defined( _TLS_ERRFAC )
	TlsSetValue( TLSIndex, gMngtErrfac );
#else
    ERRFAC = gMngtErrfac;
#endif

    //////////////////////////////////
    //ftd_header_t header is read, now read the mmp_mngt_header_t structure 
    r = mmp_mngt_sock_recv(sockp,(char*)&hdr,sizeof(hdr));ASSERT(r == sizeof(hdr));
    if ( r != sizeof(hdr) )
    {
        //thread ? : error, close socket 
	    ftd_sock_delete(&fsockp);
        return -1;
    }

    mmp_convert_mngt_hdr_ntoh(&hdr);
    //////////////////////////////////
    //analyze mmp_mngt_header_t content
    //
    //validate
    if ( hdr.magicnumber != MNGT_MSG_MAGICNUMBER )
    {
        ASSERT(0);
        //thread ? : error, close socket 
		ftd_sock_delete(&fsockp);
        return -1;
    }

    ASSERT( hdr.sendertype == SENDERTYPE_TDMF_SERVER );
    if ( hdr.sendertype == SENDERTYPE_TDMF_SERVER )
    {   //refresh local value used throughout mngt functions
        if ( giTDMFCollectorIP != (int)sockp->rip && sockp->rip != 0 )
        {   //save collector new IP to registry
            char tmp[32];
            giTDMFCollectorIP = sockp->rip;
            ip_to_ipstring(giTDMFCollectorIP,tmp);
            if ( cfg_set_software_key_value("DtcCollectorIP", tmp, CFG_IS_STRINGVAL) != CFG_OK )
            {
                DBGPRINT((2,"WARNING! Unable to write \'DtcCollectorIP\' configuration value !\n"));
            }
        }
    }

    //dispatch
    switch(hdr.mngttype)
    {
    case MMP_MNGT_SET_LG_CONFIG:           // receiving a logical group cfg file 
        r = ftd_mngt_set_config(sockp);
        break;
    case MMP_MNGT_GET_LG_CONFIG:	        // request to send back a logical group cfg file
        r = ftd_mngt_get_config(sockp);
        break;
    case MMP_MNGT_REGISTRATION_KEY:     // SET/GET registration key
        r = ftd_mngt_registration_key_req(sockp);
        break;
    case MMP_MNGT_TDMF_CMD:             // management cmd, specified by a tdmf_commands sub-cmd
        r = ftd_mngt_tdmf_cmd(sockp);
        break;

    case MMP_MNGT_SET_AGENT_GEN_CONFIG:
    case MMP_MNGT_GET_AGENT_GEN_CONFIG:
        r = ftd_mngt_agent_general_config(sockp,hdr.mngttype);
        break;

    case MMP_MNGT_GET_ALL_DEVICES:
        r = ftd_mngt_get_all_devices(sockp);
        break;

    case MMP_MNGT_AGENT_INFO_REQUEST:      // request for host information: IP, listener socket port, ...
        ASSERT(0);                  // this request can be received only from the broadcast listener socket.
        r = -1;
        break;

    case MMP_MNGT_PERF_CFG_MSG:
        r = ftd_mngt_get_perf_cfg(sockp);
        break;

    case MMP_MNGT_TDMF_SENDFILE:   // receiving a TDMF file 
        r = ftd_mngt_set_file(sockp);
        break;

    case MMP_MNGT_TDMF_GETFILE:    // send TDMF file(s)
        r = ftd_mngt_get_file(sockp);
        break;

    case MMP_MNGT_SET_ALL_DEVICES:
    case MMP_MNGT_AGENT_INFO:     // receiving a MMP_MNGT_AGENT_INFO response message ???
    default:
        ASSERT(0);
        r = -1;
        break;
    }

	ftd_sock_delete(&fsockp);
    return r;
}

