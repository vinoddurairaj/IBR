/*
 * ftd_mngt_misc.c - ftd management miscelaneous functions
 *
 * This file was created to moved of ftd_mngt.cpp various
 * functions in order to avoid some link-time problems.
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
#include "ftd_ioctl.h"
}
#include "libmngtdef.h"
#include "libmngtmsg.h"
#include "errmsg_list.h"
#include "ftd_mngt_lg.h"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#else
#define ASSERT(exp)     ((void)0)
#endif
#define DBGPRINT(a)     ftd_mngt_tracef a 


TDMFAgentEmulator   gTdmfAgentEmulator  = { false };
mmp_TdmfPerfConfig  gTdmfPerfConfig     = {0,0};
int                 giTDMFCollectorIP   = 0;    //updated each time a msg is received from Collector
int                 giTDMFCollectorPort = TDMF_COLLECTOR_DEF_PORT;  //default values
bool				gbTDMFCollectorPresent = false;
bool				gbTDMFCollectorPresentWarning = false;


///////////////////////////////////////////////////////////////////////////////
//period at which the group monitoring (PStore and Journal size) are checked for any modification.
time_t  ftd_mngt_get_repl_grp_monit_upload_period()
{
    return gTdmfPerfConfig.iReplGrpMonitPeriod/10;//1/10 second to seconds
}

///////////////////////////////////////////////////////////////////////////////
int  ftd_mngt_send_lg_monit(int lgnum, bool isPrimary, ftd_mngt_lg_monit_t* monitp)
{
    mmp_mngt_TdmfReplGroupMonitorMsg_t *pmsg = (mmp_mngt_TdmfReplGroupMonitorMsg_t *)monitp->msg;
    
    if ( pmsg == NULL )
        return -1;

    //build msg
    if ( pmsg->hdr.magicnumber == 0 )
    {   //most of the message info. is static, so init it once and use it many times.
        //init. static data of msg only once
        pmsg->hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        pmsg->hdr.mngttype        = MMP_MNGT_GROUP_MONITORING;
        pmsg->hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
        pmsg->hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
        mmp_convert_mngt_hdr_hton(&pmsg->hdr);

        memset(&pmsg->data,0,sizeof(pmsg->data));
        pmsg->data.iReplGrpSourceIP = 0;//updated only if ROLESECONDARY
        pmsg->data.liDiskFreeSz     = 0;
        pmsg->data.liActualSz       = 0;
        ftd_mngt_getServerId( pmsg->data.szServerUID );
    }

    //init dynamic data of msg
    if ( isPrimary )
    {   //a ReplGroup Primary machine uses the PStore 
        pmsg->data.liDiskTotalSz    = monitp->pstore_disk_total_sz;
        pmsg->data.liDiskFreeSz     = monitp->pstore_disk_free_sz;   
        pmsg->data.liActualSz       = monitp->curr_pstore_file_sz;
        pmsg->data.isSource         = 1;
    }
    else
    {   //a ReplGroup Secondary machine uses the Journal
        pmsg->data.liDiskTotalSz    = monitp->journal_disk_total_sz;
        pmsg->data.liDiskFreeSz     = monitp->journal_disk_free_sz;   
        pmsg->data.liActualSz       = monitp->curr_journal_files_sz;
        pmsg->data.isSource         = 0;
        pmsg->data.iReplGrpSourceIP = monitp->iReplGrpSourceIP;
    }
    pmsg->data.sReplGrpNbr      = (short)lgnum;
    mmp_convert_hton(&pmsg->data);

    //send msg to Collector
    //open socket on Collector port ,send msg and close socket connection.
    int r, towrite = sizeof(mmp_mngt_TdmfReplGroupMonitorMsg_t);
    r = mmp_sendmsg(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pmsg,towrite,0,0);
    if ( r != 0 )
    {
        if ( r == -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR )
        {   //TDMF Collector not found !
            char ipstr[24];
            ip_to_ipstring(giTDMFCollectorIP,ipstr);
            DBGPRINT((2,"****Warning, Collector not found at IP=%s , Port=%d !\n",ipstr,giTDMFCollectorPort));
        }
        else
        {
            DBGPRINT((1,"****Error (%d) while sending ReplGroupMonitorMsg to Collector!\n",r));
        }
        r = -2;//failure
    }

    return r;
}

///////////////////////////////////////////////////////////////////////////////
void ftd_mngt_emulatorGetHostID(const char* szOriginServerUID, int iEmulatorIdx, /*out*/char * szAgentUniqueID, int size )
{
    //host ID
    sprintf(szAgentUniqueID, "%08x", iEmulatorIdx);
	//add an hidden information: the real host ID

	// 'szAgentUniqueID' '\0' ... 'OH=' 'szOriginServerUID' '\0'
	// 'OH' : Origin Hostid
	_ASSERT(strlen(szOriginServerUID) <= 8);
	_ASSERT(strlen(szAgentUniqueID)+1+3+strlen(szOriginServerUID)+1 <= size);
	//write to last characters of szAgentUniqueID, after the iEmulatorIdx information.
	sprintf(szAgentUniqueID+size-(3+8+1), "OH=%-8s", szOriginServerUID);
}

///////////////////////////////////////////////////////////////////////////////
void ftd_mngt_emulatorGetMachineName(int iEmulatorIdx, /*in*/const char *szMachineName, /*out*/char *szEmulatedMachineName, int size)
{
    _snprintf(szEmulatedMachineName, size, "%s_%d", szMachineName, iEmulatorIdx);
    szEmulatedMachineName[ size - 1 ] = 0;//ensure EOS...
}

///////////////////////////////////////////////////////////////////////////////
void    ftd_mngt_GetCollectorInfo(int *collectorIP, int *collectorPort)
{
    *collectorIP    = giTDMFCollectorIP;    
    *collectorPort  = giTDMFCollectorPort;  
}

///////////////////////////////////////////////////////////////////////////////
int    ftd_mngt_IfCollectorPresent()
{
    return (int)gbTDMFCollectorPresent;
}

///////////////////////////////////////////////////////////////////////////////
void    ftd_mngt_GetPerfConfig(mmp_TdmfPerfConfig *perfCfg)
{
    *perfCfg =  gTdmfPerfConfig;//as read from registry or lastest received from MMP_MNGT_PERF_CFG_MSG
}
