/*
 * ftd_mngt_msgs.c - status msgs sent back to Collector
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

extern "C" 
{
#include "ftd_sock.h"
#include "iputil.h"
#include "ftd_cfgsys.h"
//#include "ftd_mngt.h"
char * ftd_mngt_getServerId( /*out*/ char *szHostId );
}

#include "libmngtdef.h"
#include "libmngtmsg.h"
#include "errmsg_list.h"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#else
#define ASSERT(exp)     ((void)0)
#endif
#define DBGPRINT(a)     error_tracef a 

extern StatusMsgList    gStatusMsgList;

// By Saumya 03/06/04
extern int giTDMFCollectorIP;

extern "C" void ftd_mngt_msgs_log(const char **argv, int argc);

/*
 */
char * ftd_mngt_getServerId( /*out*/ char *szHostId )
{
    //optimisation: since the HostID is constant, 
    //retreive it only once and keep a copy for futur requests.
    static unsigned long gulHostID = INVALID_HOST_ID;

    if ( gulHostID == INVALID_HOST_ID )
    {   
        gulHostID = GetHostId();
        //gulHostID = getMACAddr();
    }
    sprintf(szHostId, "%08lx", gulHostID );
    return szHostId;
}

///////////////////////////////////////////////////////////////////////////////
/*
 * Called by EXEs other than ReplServer.exe to send a status msg to Collector
 */
extern "C" void ftd_mngt_msgs_send_status_msg_to_collector(const char *szMessage, int iTimestamp)
{
    char tmp[32];
    int iTDMFCollectorIP,iTDMFCollectorPort;

    if ( cfg_get_software_key_value("DtcCollectorIP", tmp, CFG_IS_STRINGVAL) != CFG_OK )
        return;

    ipstring_to_ip(tmp,(unsigned long*)&iTDMFCollectorIP);

    if ( cfg_get_software_key_value("DtcCollectorPort", tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
        return ;

    iTDMFCollectorPort = atoi(tmp);


    //////////////////////////////////////////////////////
    //build Agent Info response msg
    //////////////////////////////////////////////////////
    int r,towrite;
    mmp_mngt_TdmfStatusMsgMsg_t *pMsg;
    int iSize   = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + 1024;
    char *pData = new char [iSize];
    pMsg        = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;

    pMsg->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    pMsg->hdr.mngttype       = MMP_MNGT_STATUS_MSG;
    pMsg->hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    pMsg->hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&pMsg->hdr);
    ftd_mngt_getServerId( pMsg->data.szServerUID );

    ///////////////////////////////////////////////////
    //prepare Status Msg to be sent to TDMF Collector
    ///////////////////////////////////////////////////
    pMsg->data.iLength    = strlen(szMessage) + 1;      //length of message string following this structure, including terminating '\0' character.

    if ( sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength >= iSize )
    {   //truncate msg
        pMsg->data.iLength = iSize - sizeof(mmp_mngt_TdmfStatusMsgMsg_t);
    }

    char *pMsgString      = (char*)(pMsg+1);
    pMsg->data.cPriority  = LOG_INFO;
    pMsg->data.cTag       = 0;
    pMsg->data.iTdmfCmd   = 0;
    pMsg->data.iTimeStamp = iTimestamp;
    strncpy( pMsgString, szMessage, pMsg->data.iLength );
    pMsgString[ pMsg->data.iLength - 1 ] = 0;//force EOS at end of string
    ///////////////////////////////////
    //send Status Msg to TDMF Collector
    ///////////////////////////////////
    towrite = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength;
    mmp_convert_TdmfStatusMsg_hton(&pMsg->data);
    r = mmp_sendmsg(iTDMFCollectorIP,iTDMFCollectorPort,(char*)pMsg,towrite,0,0);

    delete [] pData;
}


/*
 * Send all status message in gStatusMsgList to Collector.
 * List is then cleared of all messages.
 */
void ftd_mngt_msgs_send_status_msg_to_collector()
{
    char msg[1024];
    IterStatusMsg it  = gStatusMsgList.m_list.begin();
    IterStatusMsg end = gStatusMsgList.m_list.end();
    //scan all status msg in list
    while( it != end )
    {
        if ( (*it).getTag() == StatusMsg::TAG_MESSAGE )
        {
            strncpy( msg, (*it).getMessage(), 1024 );
            msg[sizeof(msg)-1] = 0;//just in case
            ftd_mngt_msgs_send_status_msg_to_collector(msg, (*it).getTimeStamp());
        }

        it++;
    }
    gStatusMsgList.m_list.clear();
}


void ftd_mngt_msgs_log(const char **argv, int argc)
{
    char szMessage[1024],szFname[MAX_PATH],szExt[MAX_PATH];
	struct tm	*tim;
	time_t		t;

	time(&t);
	tim = localtime(&t);
    _splitpath(argv[0],NULL,NULL,szFname,szExt);
	sprintf(szMessage,
		"[%04d/%02d/%02d %02d:%02d:%02d] %s: [%s:%s%s] [INFO / TOOL]  %s%s \n", 
		(1900+tim->tm_year), (1+tim->tm_mon), tim->tm_mday,
		tim->tm_hour, tim->tm_min, tim->tm_sec, 

#if defined( _TLS_ERRFAC )
		((errfac_t *)TlsGetValue( TLSIndex ))->facility,
		((errfac_t *)TlsGetValue( TLSIndex ))->hostname,
#else
		ERRFAC->facility,
		ERRFAC->hostname,
#endif

		szFname,szExt,
		szFname,szExt );	

    for(int i=1; i<argc; i++)
    {
        strcat(szMessage,argv[i]);
        strcat(szMessage," ");
    }
    //write to System Event Log and to gStatusMsgList
    error_syslog(ERRFAC,LOG_INFO,szMessage);
    //send messages in gStatusMsgList to Collector

	// By Saumya Tripathi
	// Fixing WR 32933
	if ( giTDMFCollectorIP == 0 )
	{
		char                tmp[32];
		//
		// get the ip of the collector
		//
		if ( cfg_get_software_key_value("DtcCollectorIP", tmp, CFG_IS_STRINGVAL) == CFG_OK )
		{
			ipstring_to_ip(tmp,(unsigned long*)&giTDMFCollectorIP);
		}
	}

	// By Saumya 03/03/04
	// With these modifications it will run in a collector less environment also
	// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
	// still work; all the CLIs will work too
	if ( giTDMFCollectorIP != 0 )
	{
    ftd_mngt_msgs_send_status_msg_to_collector();
}

}
