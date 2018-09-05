/*
 * mmp_API.cpp - object providing an interface to the TDMF Collector main functionalities
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
#include "stdafx.h"
#include <set>
//#include <string>
#include <list>
#include <comutil.h>
extern "C"
{
#include "iputil.h"
#include "sock.h"
}
#include "System.h"     // TDMF Object Model 
#include "FsTdmfDB.h"
#include "FsTdmfRecNvpNames.h"
#include "libmngt.h"
#include "libmngtmsg.h"
#include "libmngtnep.h"
#include "mmp_api.h"
#include "lgcfg_buf_obj.h"

#define INVALID_HOST_ID     0
#define INVALID_GRP_NBR     -1


//*****************************************************************************
static void         localReplacePYYYbySYYY(char * pCfgData, int iRepGroupNum);


//*****************************************************************************
//*****************************************************************************
/**
 *
 */
class NotifSubscriptionItem
{
public:
    NotifSubscriptionItem() 
        : m_iHostId(INVALID_HOST_ID)
        , m_iGroupNbr(INVALID_GRP_NBR)
        {};

    NotifSubscriptionItem(  int iHostId, 
                            int iGroupNbr, 
                            eSubscriptionAction     eAction,
                            eSubscriptionDataType   eType
                         )
        : m_iHostId(iHostId)
        , m_iGroupNbr(iGroupNbr)
        , m_eAction(eAction)
        , m_eType(eType)
        {};

    ~NotifSubscriptionItem() 
        {};

    /**
     * used by std::map<> for unique objects identification
     */
    bool operator< (const NotifSubscriptionItem & aItem) const
    {
        if ( m_iHostId > aItem.m_iHostId )
            return false;
        if ( m_iHostId < aItem.m_iHostId )
            return true;
        //same host id, now check group number
        if ( m_iGroupNbr < aItem.m_iGroupNbr )
            return true;
        return false;
    }

    operator std::string() const
    {
        std::string tmp;
        char buf[40];
        tmp = "HostId=0x";
        tmp += itoa(m_iHostId,buf,16);
        tmp += " GrpNbr=";
        tmp += itoa(m_iGroupNbr,buf,10);
        tmp += " Action=";
        tmp += (m_eAction == ACT_ADD ? "ADD" : "DEL");
        tmp += " Type=";
        tmp += (m_eType == TYPE_FULL_STATS ? "FULL" : "STATE");
        return tmp;
    }

    /* getters */
    inline int                      getHostId() const    { return m_iHostId; };
    inline int                      getGroupNbr() const  { return m_iGroupNbr; };
    inline eSubscriptionAction      getAction() const    { return m_eAction; };
    inline eSubscriptionDataType    getType() const      { return m_eType; };

private:
    //object unique identifiers and attributes
    int     m_iHostId;
    int     m_iGroupNbr;

    //object attributes
    eSubscriptionAction     m_eAction;
    eSubscriptionDataType   m_eType;
};

class NotifSubscriptionItemSet : public std::set<NotifSubscriptionItem>  
{
public:
    void Insert_Modify( NotifSubscriptionItem & item )
    {
        NotifSubscriptionItemSet::iterator t = find( item );
        if ( t != end() )
        {   //found it, update object values
            (*t) = item;
        }
        else
        {   //not found, insert it
            insert( item );
        }
    }
};

/*
void DbgDumpSet(NotifSubscriptionItemSet* set)
{
    NotifSubscriptionItemSet::iterator it   = set->begin();
    NotifSubscriptionItemSet::iterator end  = set->end();
    printf("\n\nSet contains %d items:",set->size());
    while( it != end )
    {
        printf("\n  item : %s", ((std::string)(*it)).c_str() );
        it++;
    }
}
*/

//*****************************************************************************
//*****************************************************************************
/**
 *
 */
MMPAPI_Error::MMPAPI_Error( ErrorCodes code )
{
    setCode( code );
}

void MMPAPI_Error::setCode( ErrorCodes code )  
{ 
    m_code = code; 
    m_cszMsg.Empty();
}


/**
 * Converts the error code to an integer
 */
MMPAPI_Error::operator CString()
{
    switch(m_code)
    {
    case OK:    
        return CString("OK");
    case ERR_INTERNAL_ERROR:
        return CString("Internal error.") + m_cszMsg;
    case ERR_INVALID_PARAMETER:
        return CString("Invalid parameter provided to function.") + m_cszMsg;
    case ERR_ILLEGAL_IP_OR_PORT_VALUE:
        return CString("Collector IP address or IP Port value not specified.") + m_cszMsg;
    case ERR_UNABLE_TO_CONNECT_TO_COLLECTOR:        //  unable to connect to TDMF Collector
        return CString("Unable to establish an TCP connection with Collector.") + m_cszMsg;
    case ERR_SENDING_DATA_TO_COLLECTOR:              //  error while sending data to TDMF Collector
        return CString("Error occured while sending data to Collector.") + m_cszMsg;
    case ERR_RECEIVING_DATA_FROM_COLLECTOR:          //  error while receiving data from TDMF Collector
        return CString("Error occured while receiving data from Collector.") + m_cszMsg;
    case ERR_UNKNOWN_TDMF_AGENT:                     //  unknown TDMF Agent (szAgentId)
        return CString("Unknown Agent specified by Host ID.") + m_cszMsg;
    case ERR_UNABLE_TO_CONNECT_TO_TDMF_AGENT:        //  unable to connect to TDMF Agent
        return CString("Unable to establish an TCP connection with Agent.") + m_cszMsg;
    case ERR_COMM_RUPTURE_WITH_TDMF_AGENT:           //  unexpected communication rupture while rx/tx data with TDMF Agent
        return CString("Unexpected communication rupture while rx/tx data with Agent.") + m_cszMsg;
    case ERR_COMM_RUPTURE_WITH_TDMF_COLLECTOR:       //  unexpected communication rupture while rx/tx data with TDMF Collector
        return CString("Unexpected communication rupture while rx/tx data with Collector.") + m_cszMsg;
    case ERR_SAVING_TDMF_AGENT_CONFIGURATION:        //  Agent could not save the received Agent config.  Continuing using current config.
        return CString("Agent could not save the received Agent config.  Continuing using current config.") + m_cszMsg;
    case ERR_ILLEGAL_TDMF_AGENT_CONFIGURATION_PROVIDED://provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.
        return CString("Provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.") + m_cszMsg;
    case ERR_DATABASE_RELATION_ERROR://provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.
        return CString("A data relation error was found in database. ") + m_cszMsg;
    case ERR_BAD_OR_MISSING_REGISTRATION_KEY:
        return CString("Bad registration key sent or unable to retreive key from Agent.") + m_cszMsg;
    case ERR_AGENT_PROCESSING_TDMF_CMD:
        return CString("The Agent was unable to process the requested command.") + m_cszMsg;
	case ERR_NO_CFG_FILE:
		return CString("No file(s) was(were) found on server.");
    default:
        {
            CString msg;
            msg.Format("Unexpected error code %d !!!!!\n", (int)m_code );
            return msg + m_cszMsg;
        }
    }
}


void MMPAPI_Error::setFromMMPCode(int r)
{
    switch(r)
    {
    case 0://0 = success
        setCode(OK); break;  
    case -2://-2  = unable to connect to TDMF Collector
        setCode(ERR_UNABLE_TO_CONNECT_TO_COLLECTOR); break;  
    case -3://-3  = error while sending data to TDMF Collector
        setCode(ERR_SENDING_DATA_TO_COLLECTOR);    break;
    case -4://-4  = error while receiving data from TDMF Collector
        setCode(ERR_RECEIVING_DATA_FROM_COLLECTOR);    break;
    case -6://-6  = No Cfg file found
        setCode(ERR_NO_CFG_FILE);    break;

    case -10://-10 = invalid argument
        setCode(ERR_INVALID_PARAMETER);    break;

    //must match values of   enum mngt_status  ( libmngtmsg.h )
    case -100://-100 = unable to connect to TDMF Collector
        setCode(ERR_UNABLE_TO_CONNECT_TO_COLLECTOR);   break;
    case -101://-101 = unknown TDMF Agent (szAgentId)
        setCode(ERR_UNKNOWN_TDMF_AGENT);   break;
    case -102://-102 = unable to connect to TDMF Agent
        setCode(ERR_UNABLE_TO_CONNECT_TO_TDMF_AGENT);  break;
    case -103://-103 = unexpected communication rupture while rx/tx data with TDMF Agent
        setCode(ERR_COMM_RUPTURE_WITH_TDMF_AGENT); break;
    case -104://-104 = unexpected communication rupture while rx/tx data with TDMF Collector
        setCode(ERR_COMM_RUPTURE_WITH_TDMF_COLLECTOR); break;
    case -105://-105 = Agent could not save the received Agent config.  Continuing using current config.
        setCode(ERR_SAVING_TDMF_AGENT_CONFIGURATION);  break;
    case -106://-106 = provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.
        setCode(ERR_ILLEGAL_TDMF_AGENT_CONFIGURATION_PROVIDED);    break;
    case -107://-107 = basic functionality failed ...
        setCode(ERR_INTERNAL_ERROR);   break;
    case -108:
        setCode(ERR_BAD_OR_MISSING_REGISTRATION_KEY);  break;
    default:
        _ASSERT(0);
        setCode(ERR_INTERNAL_ERROR);   break;
    }
}


//*****************************************************************************
//*****************************************************************************

MMP_API::MMP_API()
{
	m_pSystemLog   = NULL;

    initCollectorIPAddr(0);
    initCollectorIPPort(0);
    m_hMMP         = NULL;
    m_db           = NULL;
	m_hNotifThread = NULL;
    m_socket       = NULL;
    InitializeCriticalSection(&m_csNotifRequest);
    m_hevNotifDataReady = m_hevNotifDataProcessed = m_hevEndNotifThread = NULL;
    m_SubscriptionsRegisterSet = 0;

	sock_startup();
}
    
MMP_API::MMP_API(int iCollectorIPAddr, int iCollectorIPPort, FsTdmfDb *db)
{
	m_pSystemLog   = NULL;

    initCollectorIPAddr(iCollectorIPAddr);
    initCollectorIPPort(iCollectorIPPort);
    initDB(db);
    m_hMMP         = NULL;
	m_hNotifThread = NULL;
    m_socket       = NULL;
    InitializeCriticalSection(&m_csNotifRequest);
    m_hevNotifDataReady = m_hevNotifDataProcessed = m_hevEndNotifThread = NULL;
    m_SubscriptionsRegisterSet = 0;

	sock_startup();
}

MMP_API::~MMP_API()
{
    unregisterForNotifications();

    DeleteCriticalSection(&m_csNotifRequest);

    if ( m_hMMP )
    {
        mmp_Destroy(m_hMMP);
        m_hMMP = NULL;
    }

    sock_delete(&m_socket);

	sock_cleanup();
}

void MMP_API::Trace(unsigned short thislevel, const char* message, ...)
{
	va_list marker;
	va_start(marker, message);

	if (m_pSystemLog)
	{
		m_pSystemLog->TraceOut(thislevel, message, marker);
	}

	va_end(marker);
}

void MMP_API::initCollectorIPAddr(int iCollectorIPAddr)
{
    m_ip = iCollectorIPAddr;
    ip_to_ipstring(m_ip, m_ipstr);

    Trace( ONLOG_EXTENDED, ">> initCollectorIPAddr(%s)", m_ipstr );
}

void MMP_API::initCollectorIPAddr(const char * szCollectorIPAddr)//dotted decimal format expected, "a.b.c.d"
{
    int ip;
    Trace( ONLOG_EXTENDED, ">> initCollectorIPAddr(%s)", (char*)szCollectorIPAddr );

    if ( 0 == ipstring_to_ip((char *)szCollectorIPAddr, (unsigned long *)&ip) ) 
    {
        initCollectorIPAddr(ip);
    }
}

void MMP_API::initCollectorIPPort(int iCollectorIPPort)
{
    m_port = iCollectorIPPort;

    Trace( ONLOG_EXTENDED, ">> initCollectorIPPort(%d)", iCollectorIPPort );
}

void MMP_API::initDB(FsTdmfDb *db)
{
    m_db = db;

    Trace( ONLOG_EXTENDED, ">> setDB(%p)", db );
}

void MMP_API::initTrace(CSystem* pSystemLog)
{
    m_pSystemLog = pSystemLog;
}

MMPAPI_Error MMP_API::createMMPHandle()
{
    if ( m_hMMP == NULL )
    {
        if ( m_ip == 0 || m_port == 0 )
        {
            Trace( ONLOG_DEBUG, ">> createMMPHandle : *** ERROR, uninitialized ip or port values.***" );
            return MMPAPI_Error( MMPAPI_Error::ERR_ILLEGAL_IP_OR_PORT_VALUE );
        }

        m_hMMP = mmp_Create(m_ipstr, m_port);

        if ( m_hMMP == NULL )
            return MMPAPI_Error( MMPAPI_Error::ERR_INTERNAL_ERROR );

        Trace( ONLOG_DEBUG, ">> createMMPHandle : ok" );
    }

    return MMPAPI_Error( MMPAPI_Error::OK );
}

   
MMPAPI_Error  	MMP_API::requestTDMFOwnership(HWND hUID, /*out*/bool * pbOwnershipGranted)
{
	char szClientUID[32];
    return requestTDMFOwnership( ltoa( (long)hUID, szClientUID, 16), pbOwnershipGranted );
}

MMPAPI_Error  	MMP_API::requestTDMFOwnership(const char * szClientUID, /*out*/bool * pbOwnershipGranted)
{
	Trace( ONLOG_EXTENDED, ">> requestTDMFOwnership");

    MMPAPI_Error code;
    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;
	int r = mmp_TDMFOwnership( m_hMMP, 
				               szClientUID,
                               true,//request or renew ownership
							   pbOwnershipGranted );
    code.setFromMMPCode(r);
    return code;
}

MMPAPI_Error  	MMP_API::releaseTDMFOwnership(HWND hUID, /*out*/bool * pbOwnershipGranted)
{
	char szClientUID[32];
    return releaseTDMFOwnership( ltoa( (long)hUID, szClientUID, 16), pbOwnershipGranted );
}

MMPAPI_Error  	MMP_API::releaseTDMFOwnership(const char * szClientUID, /*out*/bool * pbOwnershipGranted)
{
	Trace( ONLOG_EXTENDED, ">> releaseTDMFOwnership");

    MMPAPI_Error code;
    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;
	int r = mmp_TDMFOwnership( m_hMMP, 
				               szClientUID,
                               false,//release ownership
							   pbOwnershipGranted );
    code.setFromMMPCode(r);
    return code;
}

/* ****************************** */
/* Actions on a Replication Group */
/* ****************************** */
/**
 * Add or Modify the specified RepGroup from the TDMF system (Agent and DB)
 * If active, RepGroup is stopped to perform the update.
 */
			// Cleanup
			  // Stop group (the old one) if it already in DB
			  // If GroupNumber has changed
			  // {
			  //   Erase old .cfg file on servers
			  // }
			  // If GroupNumber has changed
			  // {
			  //   Remove performance data
			  // }
			  // Remove pairs data
			// Save new info
			  // Save new group info
			  // Add new pairs
			  // Generate .cfg files

MMPAPI_Error    MMP_API::addRepGroup(CReplicationGroup & repGroup /*group def*/, long nOldGroupNumber, long nOldTargetHostId, std::string* pstrWarning, bool bUseTransaction, bool bDistributeCfgFiles)
{
    MMPAPI_Error code;

	if (pstrWarning)
	{
		pstrWarning->assign("");
	}

    CServer *psrcAgent = repGroup.m_pParent;
    CServer *ptgtAgent = repGroup.m_pServerTarget;

#ifdef _DEBUG
    _ASSERT( !IsBadWritePtr(psrcAgent,sizeof(CServer)) );
	if (ptgtAgent != NULL)
	{
		_ASSERT( !IsBadWritePtr(ptgtAgent,sizeof(CServer)) );
	}
#endif

	if ((bUseTransaction == false) || m_db->mpRecGrp->BeginTrans())
	{
		try
		{
			bool bNewGroup = false;
			
			long nNewGroupNumber = repGroup.m_nGroupNumber;
			Trace( ONLOG_EXTENDED, ">> addRepGroup : Group Id=%d , SrcHostId=0x%08x , TgtHostId=0x%08x",
						nNewGroupNumber, psrcAgent->m_nHostID,
						ptgtAgent ? ptgtAgent->m_nHostID : 0);

			m_db->mpRecSrvInf->FtdPos(psrcAgent->m_nHostID);

			m_db->mpRecGrp->FtdLock();
			m_db->mpRecPair->FtdLock();
			m_db->mpRecPerf->FtdLock();

			//////////////////////////////////////////////////////////////////
			// Cleanup
			//////////////////////////////////////////////////////////////////

			/////////////////////////////////////
			// Stop group (the old one) if it already in DB
			// Find out if the RepGroup already exists (in DB)
			CString cszTmp;
			//build a SQL WHERE clause
			cszTmp.Format(  " %s = %d "
							" AND %s = %d "
							, FsTdmfRecLgGrp::smszFldLgGrpId
							, nOldGroupNumber
							, FsTdmfRecLgGrp::smszFldSrcFk
							, psrcAgent->m_iDbSrvId);

			if ( m_db->mpRecGrp->FtdFirst(cszTmp) )
			{   //record found, so RepGroup do exist.
				// Stop RepGroup 
				// 30/01/2003 No real need to stop the group (changes applied at restart)
				//cszTmp.Format(" -g%03d ", nOldGroupNumber); 
				//tdmfCmd(psrcAgent->m_nHostID, MMP_MNGT_TDMF_CMD_STOP, cszTmp);

				// delete Old cfg file on old target server
				if ((nOldTargetHostId != INVALID_HOST_ID) && ((ptgtAgent == NULL) || (nOldTargetHostId != ptgtAgent->m_nHostID)))
				{
					int  r;
					char buf[16];

					r = mmp_setTdmfAgentLGConfig( m_hMMP, 
												  _itoa( nOldTargetHostId, buf, 16 ),
												  true, //bIsHostId,
												  (short)nOldGroupNumber,
												  false,//bPrimary,
												  NULL, //pCfgData,
												  0     //uiDataSize
												  );_ASSERT(r==0);
					if ( r != 0 )
					{
						Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, %d while deleting RepGroup %d from TARGET Agent 0x%08x ***", r, nOldGroupNumber, ptgtAgent->m_iDbSrvId );
						//todo :
						//restore previous Tgt LGConfig ??
						//restore previous Src LGConfig ??
						code = MMPAPI_Error( MMPAPI_Error::ERR_DELETING_TARGET_REP_GROUP );
					}
				}

				if (nOldGroupNumber != nNewGroupNumber)
				{
					// Erase old .cfg file on servers
					int  r;
					char buf[16];
					//
					// SOURCE server        
					//
					r = mmp_setTdmfAgentLGConfig( m_hMMP, 
												  _itoa( psrcAgent->m_nHostID, buf, 16 ),
												  true, //bIsHostId,
												  (short)nOldGroupNumber,
												  true, //bPrimary,
												  NULL, //pCfgData,
												  0     //uiDataSize
												  );_ASSERT(r==0);
					if ( r != 0 )
					{
						Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, %d while deleting RepGroup %d from SOURCE Agent 0x%08x ***", r, nOldGroupNumber, psrcAgent->m_iDbSrvId);
						//todo :
						//restore previous Src LGConfig ??
						code = MMPAPI_Error( MMPAPI_Error::ERR_DELETING_SOURCE_REP_GROUP );
					}

					/////////////////////////////////////
					// Remove performance data
					cszTmp.Format(  " %s = %d "
								    " AND %s = %d "
									,FsTdmfRecPerf::smszFldGrpFk
									,nOldGroupNumber
									,FsTdmfRecPerf::smszFldSrcFk 
									,ptgtAgent->m_iDbSrvId
									);
					if ( !m_db->mpRecPerf->FtdDelete( cszTmp ) )
					{
						Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error deleting DB Performance records: %s ***", (LPCTSTR)cszTmp );
						throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
					}
				}

				/////////////////////////////////////
				// Remove pairs data
				std::list<CReplicationPair>::iterator  it;
				for (it = repGroup.m_listReplicationPair.begin(); it != repGroup.m_listReplicationPair.end(); it++)
				{
					std::list<CReplicationPair>::const_reference refPair = *it;

					if (refPair.m_eObjectState == CReplicationPair::RPO_DELETED) 
					{
						// Save the iterator and go to next item (to continue after the deletion)
						std::list<CReplicationPair>::iterator itDelete = it;
						it++;
						
						// Remove all associated performance records from DB
						cszTmp.Format(  " %s = %d "
									    " AND %s = %d "
										" AND %s = %d "
										,FsTdmfRecPerf::smszFldPairFk
										,refPair.m_nPairNumber
										,FsTdmfRecPerf::smszFldGrpFk
										,nOldGroupNumber
										,FsTdmfRecPerf::smszFldSrcFk 
										,psrcAgent->m_iDbSrvId );
						if ( !m_db->mpRecPerf->FtdDelete( cszTmp ) )
						{
							Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error deleting DB Performance records: %s ***", (LPCTSTR)cszTmp );
							throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
						}
						
						// Remove Pair from DB
						CString cszWhere;
						cszWhere.Format("%s = %d", FsTdmfRecPair::smszFldIk, refPair.m_iDbIk);
						
						if (!m_db->mpRecPair->FtdDelete(cszWhere))
						{
							throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
						}
						
						// Also remove it from list
						repGroup.m_listReplicationPair.erase(itDelete);
					}
				}
			}
			else
			{
				if ( !m_db->mpRecGrp->FtdNew(nNewGroupNumber) )
				{
					Trace( ONLOG_EXTENDED, ">> addRepGroup : ***Error, cannot create a RepGroup with LgGroupId=%d ***", nNewGroupNumber );
					throw MMPAPI_Error( MMPAPI_Error::ERR_CREATING_DB_RECORD );
				}

				bNewGroup = true;
			}

			////////////////////////////////////////////////
			// Save new info
			////////////////////////////////////////////////
			
			/////////////////////////////////////
			//save RepGroup values to DB
			//
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldLgGrpId,         nNewGroupNumber    ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSrcFk,           psrcAgent->m_iDbSrvId ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (ptgtAgent != NULL)
			{
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtFk,       ptgtAgent->m_iDbSrvId ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,          repGroup.m_strPStoreFile.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldJournalVol,      repGroup.m_strJournalDirectory.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldChainning,       (int)(repGroup.m_bChaining != 0 ? 1 : 0) ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes,           repGroup.m_strDescription.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}

			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldThrottles,           repGroup.m_strThrottles.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}

			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldChunkDelay,      (int)repGroup.m_nChunkDelay ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldChunkSize,       (int)repGroup.m_nChunkSizeKB ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldEnableCompression, (int)repGroup.m_bEnableCompression ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSyncMode,        (int)(repGroup.m_bSync != 0 ? 1 : 0) ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSyncModeDepth,   (int)repGroup.m_nSyncDepth ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSyncModeTimeOut, (int)repGroup.m_nSyncTimeout ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldRefreshNeverTimeOut,  (int)(repGroup.m_bRefreshNeverTimeout != 0 ? 1 : 0) ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldRefreshTimeOut,  (int)repGroup.m_nRefreshTimeoutInterval ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldMaxFileStatSize,  (int)repGroup.m_nMaxFileStatSize ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNetUsageThreshold,  (int)repGroup.m_bNetThreshold))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNetUsageValue,  (int)repGroup.m_nNetMaxKbps ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldUpdateInterval,  (int)repGroup.m_nStatInterval ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldJournalLess,  (int)repGroup.m_bJournalLess ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldJournalLess);
					pstrWarning->append(cstrTmp);
				}
			}
         
            if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPrimaryDHCPNameUsed,  (int)repGroup.m_bPrimaryDHCPAdressUsed ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}

			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPrimaryEditedIPUsed,  (int)repGroup.m_bPrimaryEditedIPUsed ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			unsigned long ip = 0;
            if (repGroup.m_strPrimaryEditedIP.length() > 0)
	        {
                ipstring_to_ip((char*)repGroup.m_strPrimaryEditedIP.c_str(),&ip);
			}

			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPrimaryEditedIP,  (int)ip))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtDHCPNameUsed,  (int)repGroup.m_bTargetDHCPAdressUsed ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}

               
	        if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtEditedIPUsed,  (int)repGroup.m_bTargetEditedIPUsed))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (repGroup.m_pServerTarget != NULL)
			{
				ip = 0;
				if (repGroup.m_strTargetEditedIP.length() > 0)
				{
					ipstring_to_ip((char*)repGroup.m_strTargetEditedIP.c_str(),&ip);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtEditedIP,  (int)ip))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
			}

			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSymmetric, (int)(repGroup.m_bSymmetric ? 1 : 0) ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldSymmetric);
					pstrWarning->append(cstrTmp);
				}
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSymmetricGroupNumber, repGroup.m_nSymmetricGroupNumber ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldSymmetricGroupNumber);
					pstrWarning->append(cstrTmp);
				}
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSymmetricNormallyStarted, (int)(repGroup.m_bSymmetricNormallyStarted ? 1 : 0) ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldSymmetricNormallyStarted);
					pstrWarning->append(cstrTmp);
				}
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldFailoverInitialState, repGroup.m_nFailoverInitialState ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldFailoverInitialState);
					pstrWarning->append(cstrTmp);
				}
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSymmetricPStoreFile, repGroup.m_strSymmetricPStoreFile.c_str() ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldSymmetricPStoreFile);
					pstrWarning->append(cstrTmp);
				}
			}
			if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSymmetricJournalDirectory, repGroup.m_strSymmetricJournalDirectory.c_str() ))
			{
				if (pstrWarning)
				{
					if (pstrWarning->size() == 0)
					{
						pstrWarning->append("Incompatible Database version:\n\n");
					}
					CString cstrTmp;
					cstrTmp.Format("Cannot save '%s' field.\n", FsTdmfRecLgGrp::smszFldSymmetricJournalDirectory);
					pstrWarning->append(cstrTmp);
				}
			}

			/////////////////////////////////////
			// Add new and save pairs
			std::list<CReplicationPair>::iterator  it;
			for (it = repGroup.m_listReplicationPair.begin(); it != repGroup.m_listReplicationPair.end(); it++)
			{
				std::list<CReplicationPair>::const_reference refPair = *it;

				//if RecPair does not exists, create it.
				if ( !m_db->mpRecPair->FtdPos(refPair.m_nPairNumber) )
				{
					_ASSERT(refPair.m_eObjectState == CReplicationPair::RPO_NEW);
					if ( !m_db->mpRecPair->FtdNew(refPair.m_nPairNumber) )
					{
						Trace( ONLOG_EXTENDED, ">> addRepGroup : ***Error, cannot create a RepPair with PairId=%d ***", refPair.m_nPairNumber );
						throw MMPAPI_Error( MMPAPI_Error::ERR_CREATING_DB_RECORD );
					}
				}

				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldPairId,      refPair.m_nPairNumber ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldGrpFk,       nNewGroupNumber ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcFk,       psrcAgent->m_iDbSrvId ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldNotes,       refPair.m_strDescription.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldState,       refPair.m_nState ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcDisk,     refPair.m_DeviceSource.m_strPath.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcDriveId,  (refPair.m_DeviceSource.m_strDriveId.length() == 0) ? "0" : refPair.m_DeviceSource.m_strDriveId.c_str()) )
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcStartOff, (refPair.m_DeviceSource.m_strStartOff.length() == 0) ? "0" : refPair.m_DeviceSource.m_strStartOff.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcLength,   (refPair.m_DeviceSource.m_strLength.length() == 0) ? "0" : refPair.m_DeviceSource.m_strLength.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtDisk,     refPair.m_DeviceTarget.m_strPath.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtDriveId,  (refPair.m_DeviceTarget.m_strDriveId.length() == 0) ? "0" : refPair.m_DeviceTarget.m_strDriveId.c_str()) )
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtStartOff, (refPair.m_DeviceTarget.m_strStartOff.length() == 0) ? "0" : refPair.m_DeviceTarget.m_strStartOff.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtLength,   (refPair.m_DeviceTarget.m_strLength.length() == 0) ? "0" : refPair.m_DeviceTarget.m_strLength.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldFS,          refPair.m_strFileSystem.c_str() ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				
				// Set state to SAVED
				it->SetObjectState(CReplicationPair::RPO_SAVED);
			}

			// Commit changes
			if ((bUseTransaction == false) || (m_db->mpRecGrp->CommitTrans() == TRUE))
			{
				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("Replication Group - Save Properties for '%s - group# %d'", repGroup.m_pParent->m_strName.c_str(), repGroup.m_nGroupNumber);
				repGroup.m_pParent->m_pParent->m_pParent->LogUserAction(cstrLogMsg);

				// Advise other GUI
				SendTdmfObjectChangeMessage(TDMF_GROUP, bNewGroup ? TDMF_ADD_NEW : TDMF_MODIFY, psrcAgent->m_iDbDomainFk, psrcAgent->m_iDbSrvId, nNewGroupNumber);
			}
			else
			{
				code = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
			}

			// Copy info to target group
			CReplicationGroup* repGroupTarget = repGroup.GetTargetGroup();
			if (repGroupTarget != NULL)
			{
				repGroupTarget->Copy(&repGroup);
			}

			/////////////////////////////////////
			// Generate .cfg files
			
			if (repGroup.IsValid() == 0)
			{
				if (bDistributeCfgFiles)
				{
					//add/update RepGroup cfg (.cfg file) on both SOURCE and TARGET Servers
					code = SendCfgFileToAgent( &repGroup, psrcAgent->m_nHostID, ptgtAgent->m_nHostID );

					// Start symmetric group if...
					if (code.IsOK() && (repGroup.m_bSymmetric) && (repGroup.m_bSymmetricNormallyStarted))
					{
						CString cstrOptions;
						cstrOptions.Format("-g%03d", repGroup.m_nSymmetricGroupNumber);
						code = tdmfCmd(ptgtAgent->m_nHostID, MMP_MNGT_TDMF_CMD_START, cstrOptions);
						if (code.IsOK())
						{
							cstrOptions.Format("-g %03d state passthru", repGroup.m_nSymmetricGroupNumber);
							code = tdmfCmd(ptgtAgent->m_nHostID, MMP_MNGT_TDMF_CMD_OVERRIDE, cstrOptions);
						}
					}
				}
			}
			else
			{
				Trace(ONLOG_EXTENDED, "Replication Group not valid: Config files not generated");

				code = SendEmptyCfgFileToAgent( &repGroup, psrcAgent->m_nHostID, ptgtAgent ? ptgtAgent->m_nHostID : INVALID_HOST_ID);
			}
		}
		catch(MMPAPI_Error eErr)
		{
			code = eErr;

			if (bUseTransaction) 
			{
				// Rollback (save all or nothing)
				m_db->mpRecGrp->Rollback();
			}
		}
		
		//must unlock in reverse order
		m_db->mpRecPerf->FtdUnlock();
		m_db->mpRecPair->FtdUnlock();
		m_db->mpRecGrp->FtdUnlock();
	}
	else
	{
		code = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

	if (pstrWarning && (pstrWarning->size() > 0))
	{
		pstrWarning->append("\nPlease upgrade the Collector/Database component.\n");
	}

    return code;
}

/**
 * Remove the specified RepGroup from the TDMF system (on Agent and DB)
 * Rep.Group does for sure have a Source Server but may not have a valid Target Server.
 * So, support this case.
 */
MMPAPI_Error    MMP_API::delRepGroup(CReplicationGroup & repGroup /*group def*/, bool bDeleteCfgfiles )
{
    MMPAPI_Error    code;

    int iRepGroupNum;
    int iSrcFk;
    int iSrcHostId = INVALID_HOST_ID,
        iTgtHostId = INVALID_HOST_ID;

    CServer *psrcAgent = repGroup.m_pParent;
    CServer *ptgtAgent = repGroup.m_pServerTarget;
    //should have full access on this memory area ...
    _ASSERT( !IsBadWritePtr(psrcAgent,sizeof(CServer)) );

    iRepGroupNum = repGroup.m_nGroupNumber;
    iSrcHostId   = psrcAgent->m_nHostID;
    if ( ptgtAgent != NULL )
    {
        _ASSERT( !IsBadWritePtr(ptgtAgent,sizeof(CServer)) );
        iTgtHostId   = ptgtAgent->m_nHostID;
    }

    Trace( ONLOG_EXTENDED, ">> delRepGroup : Group Id=%d , SrcHostId=0x%08x , TgtHostId=0x%08x", iRepGroupNum, iSrcHostId, iTgtHostId );

    //1. get DB SrvID and HostID of SOURCE Agent
    iSrcFk = psrcAgent->m_iDbSrvId;

    //2. Find RepGroup record showing Source_Fk and LgGroupId
    //   position mpRecGrp on group.  will be used later in fnct.
    CString cszTmp;
    //build a SQL WHERE clause
    cszTmp.Format(  " %s = %d "
                    " AND %s = %d "
                    , FsTdmfRecLgGrp::smszFldLgGrpId
                    , iRepGroupNum
                    , FsTdmfRecLgGrp::smszFldSrcFk
                    , iSrcFk
                    );
    if ( !m_db->mpRecGrp->FtdFirst(cszTmp) )
    {
        Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, cannot find RepGroup id=%d , SrvId=%d ***", iRepGroupNum, iSrcFk );
        return MMPAPI_Error( MMPAPI_Error::ERR_UNKNOWN_REP_GROUP );
    }

    //4. Stop RepGroup on SOURCE
    //cszTmp.Format(" -g%03d ", iRepGroupNum); 
    //code = tdmfCmd(iSrcHostId , MMP_MNGT_TDMF_CMD_STOP, cszTmp);
    //if ( code.IsOK() == false )
    //{
    //    Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, tdmfstop %s on SOURCE 0x%08x failed: %s ***", (LPCTSTR)cszTmp, iSrcHostId, (LPCTSTR)((CString)code) );
    //}

    //5. ONLY if it stopped ok, 
	if (bDeleteCfgfiles)
	{
		SendEmptyCfgFileToAgent( &repGroup, psrcAgent->m_nHostID, ptgtAgent ? ptgtAgent->m_nHostID : INVALID_HOST_ID);
	}

    //6. ONLY if RepGroups were removed on both systems,
    //   remove the RepGroup record and all related RepPairs and Performance records
    //
    //6.1 Remove all Performance records related to the RepGroup removed
    	m_db->mpRecPair->FtdLock();
	m_db->mpRecGrp->FtdLock();

	if (m_db->mpRecGrp->BeginTrans())
	{
		try
		{
			//6.2 Remove all RecPairs records related to the RepGroup removed
			cszTmp.Format(  " %s = %d "
				" AND %s = %d "
				,FsTdmfRecPair::smszFldGrpFk
				,iRepGroupNum
				,FsTdmfRecPair::smszFldSrcFk 
				,iSrcFk
				);
			if ( !m_db->mpRecPair->FtdDelete( cszTmp ) )
			{
				Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error deleting DB RepPairs records: %s ***", (LPCTSTR)cszTmp );
				throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
			}
			
			//6.3 Remove the RecGroup record 
			cszTmp.Format(  " %s = %d "
				" AND %s = %d "
				,FsTdmfRecLgGrp::smszFldLgGrpId
				,iRepGroupNum
				,FsTdmfRecLgGrp::smszFldSrcFk 
				,iSrcFk
				);

			if ( !m_db->mpRecGrp->FtdDelete( cszTmp ) )
			{
				// Remove perf record first and then retry
				m_db->mpRecPerf->FtdLock();
				CString cstrPerf;
				cstrPerf.Format(  " %s = %d AND %s = %d "
 								,FsTdmfRecPerf::smszFldGrpFk
								,iRepGroupNum
								,FsTdmfRecPerf::smszFldSrcFk 
								,iSrcFk);
 				if ( !m_db->mpRecPerf->FtdDelete( cstrPerf ) )
 				{
 					Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error deleting DB Performance records: %s ***", (LPCTSTR)cszTmp );
					m_db->mpRecPerf->FtdUnlock();
 					throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
 				}
				m_db->mpRecPerf->FtdUnlock();

				if ( !m_db->mpRecGrp->FtdDelete( cszTmp ) )
				{
					Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error deleting DB RepGroup records: %s ***", (LPCTSTR)cszTmp );
					throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
				}
			}

			// Commit changes
			if (m_db->mpRecGrp->CommitTrans() == FALSE)
			{
				code = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
			}
			else
			{
				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("Replication Group - Delete '%s - group# %d'", repGroup.m_pParent->m_strName.c_str(), repGroup.m_nGroupNumber);
				repGroup.m_pParent->m_pParent->m_pParent->LogUserAction(cstrLogMsg);

				// Advise other GUI
				SendTdmfObjectChangeMessage(TDMF_GROUP, TDMF_REMOVE, psrcAgent->m_iDbDomainFk, psrcAgent->m_iDbSrvId, repGroup.m_nGroupNumber);
			}

		}
		catch(MMPAPI_Error eErr)
		{
			code = eErr;

			// Rollback (save all or nothing)
			m_db->mpRecGrp->Rollback();
		}
	}
	else
	{
		code = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

    m_db->mpRecGrp->FtdUnlock();
	m_db->mpRecPair->FtdUnlock();
	
    return code;
}

MMPAPI_Error    MMP_API::addScriptServerFileToDb(CScriptServer & ScriptServer )
{
    MMPAPI_Error code;

	if (m_db->mpRecScriptSrv->BeginTrans())
	{
		try
		{
		   m_db->mpRecScriptSrv->FtdLock();
		

		    //if RecScriptSrv does not exists, create it.
			if ( !m_db->mpRecScriptSrv->FtdPos(ScriptServer.m_iDbScriptSrvId) )
			{
				if ( !m_db->mpRecScriptSrv->FtdNew(ScriptServer.m_iDbSrvId , 
                                 ScriptServer.m_strFileName.c_str(), 
                                 ScriptServer.m_strType.c_str(), 
                                 ScriptServer.m_strExtension.c_str(),
                                 ScriptServer.m_strCreationDate.c_str(), 
                                 ScriptServer.m_strContent.c_str() ) )
				{
					Trace( ONLOG_EXTENDED, ">> addScriptServer : ***Error, cannot create a ScriptServer");
					throw MMPAPI_Error( MMPAPI_Error::ERR_CREATING_DB_RECORD );
				}
			}
		
		

			// Commit changes
			if (m_db->mpRecSrvInf->CommitTrans() == FALSE)
			{
				code = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
			}
			else
			{
				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("Script Server - %s - saved", ScriptServer.m_strFileName.c_str());
				ScriptServer.m_pServer->m_pParent->m_pParent->LogUserAction(cstrLogMsg);


                //  send message to collector to save script file on the server
			}

		
		
		}
		catch(MMPAPI_Error eErr)
		{
			code = eErr;

			// Rollback (save all or nothing)
			m_db->mpRecSrvInf->Rollback();
		}

		//must unlock in reverse order
		m_db->mpRecSrvInf->FtdUnlock();
	
	}
	else
	{
		code = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

    return code;
}


MMPAPI_Error    MMP_API::delScriptServerFileFromDb(CScriptServer* pScriptServer )
{
   MMPAPI_Error    code;

 
    //1. Find Script server position.  will be used later in fnct.
    CString cszTmp;
    //build a SQL WHERE clause
    cszTmp.Format(  " %s = %d "
                    , FsTdmfRecSrvScript::smszFldScriptSrvId
                    , pScriptServer->m_iDbScriptSrvId
                  );

    if ( !m_db->mpRecScriptSrv->FtdPos( pScriptServer->m_iDbScriptSrvId ))
    {
        Trace( ONLOG_EXTENDED, ">> delScriptServerFile : ***Error, cannot find ScriptServer id=%d ",pScriptServer->m_iDbScriptSrvId );
        return MMPAPI_Error( MMPAPI_Error::ERR_UNKNOWN_SCRIPT_SERVER_FILE );
    }

    m_db->mpRecScriptSrv->FtdLock();

	if (m_db->mpRecScriptSrv->BeginTrans())
	{
		try
		{
		   //Remove the ScriptServerFile record 
			cszTmp.Format(  " %s = %d "
                    , FsTdmfRecSrvScript::smszFldScriptSrvId
                    , pScriptServer->m_iDbScriptSrvId
                  );

			if ( !m_db->mpRecScriptSrv->FtdDelete( cszTmp ) )
			{
				Trace( ONLOG_EXTENDED, ">> delScriptServerFile : ***Error deleting DB ScriptServer records: %s ***", (LPCTSTR)cszTmp );
				throw MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
			}

			// Commit changes
			if (m_db->mpRecScriptSrv->CommitTrans() == FALSE)
			{
				code = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
			}
			else
			{
				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("Script Server File: Delete %s From Server %s", pScriptServer->m_strFileName.c_str(), pScriptServer->m_pServer->m_strName.c_str());
				pScriptServer->m_pServer->m_pParent->m_pParent->LogUserAction(cstrLogMsg);
               
     		}

		}
		catch(MMPAPI_Error eErr)
		{
			code = eErr;

			// Rollback (save all or nothing)
			m_db->mpRecScriptSrv->Rollback();
		}
	}
	else
	{
		code = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

   
	m_db->mpRecScriptSrv->FtdUnlock();

    return code;
}

/**
 * Import ScriptFile from Server
 * Upon succes, output params repgrp, strOtherHostName and bIsThisHostSource 
 * are updated according to information received (.cfg file).
 */
MMPAPI_Error    MMP_API::ImportAllScriptServerFiles( CServer & server,
                                                     char* strFileExtension,
                                                     long lFileType,
                                                     std::list<CScriptServer> & listScriptServer)
{
    MMPAPI_Error    code;
    int             r;
	char szAgentId[16];
    char            *pFileData   = 0;
    unsigned int    uiDataSize  = 0;
   
	listScriptServer.clear(); 

     //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;


	//retrieve content of ALL files on Server with the specified extension
	r = mmp_getTdmfServerFile(	/*in*/ m_hMMP,
							/*in*/ itoa(server.m_nHostID,szAgentId,16),
							/*in*/ true,
							/*in*/ strFileExtension,
                            /*in*/ lFileType,
							/*out*/	&pFileData,
							/*out*/ &uiDataSize);

   if ( r != 0 )
    {
        code.setFromMMPCode(r);  
        Trace( ONLOG_EXTENDED, ">> ImportAllScriptServerFiles: ***Error, %d while retreiving ALL Script Server Files from Server %s", r, server.m_strName.c_str() );
        return code;
    }

  	mmp_TdmfFileTransferData *pFileTxtData;
    char *pFileScriptData;

	unsigned int iBytesProcessed = 0;
    while( iBytesProcessed < uiDataSize )
    {
        pFileTxtData = (mmp_TdmfFileTransferData *)(pFileData + iBytesProcessed);
        mmp_convert_ntoh(pFileTxtData);

        pFileScriptData = (char *)(pFileTxtData+1);
        CString strContent(pFileScriptData,(int)pFileTxtData->uiSize);

        if ( ((int)pFileTxtData->uiSize) > 0 )
		{
			long lFileType;
			if(server.IsValidScriptServerFileName(pFileTxtData->szFilename,lFileType))
			{
		   		listScriptServer.push_back( CScriptServer() );
				CScriptServer&  SS = listScriptServer.back();
      
				SS.m_strFileName = pFileTxtData->szFilename;
				CString strType;
				strType.Format("%d",lFileType); // batch file
				SS.m_strType = strType;
				SS.m_strExtension = strFileExtension;
				SS.m_strContent = strContent;
				
			}
			   
		}
		else
		{
			CString strMsg;
 			strMsg.Format("The filename '%s' will not be imported from the server '%s' because it is empty.", 
			pFileTxtData->szFilename,
			server.m_strName.c_str());
			AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
		}

		iBytesProcessed += sizeof(mmp_TdmfFileTransferData) +pFileTxtData->uiSize;
		
	}
    delete [] pFileData;
    return code;
}

MMPAPI_Error    MMP_API::ImportOneScriptServerFile( CServer& server,
                                                    char* strFilename,
                                                    long lFileType,
                                                    CScriptServer& ScriptServer)
{
    MMPAPI_Error    code;
    int             r;
	char szAgentId[16];
    char            *pFileData   = 0;
    unsigned int    uiDataSize  = 0;

     //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;


 	//retrieve content of ALL files on Server with the specified extension
	r = mmp_getTdmfServerFile(	/*in*/ m_hMMP,
							/*in*/ itoa(server.m_nHostID,szAgentId,16),
							/*in*/ true,
							/*in*/ strFilename,
                            /*in*/ lFileType,
							/*out*/	&pFileData,
							/*out*/ &uiDataSize);

    if (r != 0)
    {
	    code.setFromMMPCode(r);  
        Trace( ONLOG_EXTENDED, ">> ImportOneScriptServerFile: ***Error, %d while retreiving the file %s from Server %s", r,strFilename, server.m_strName.c_str() );
        return code;
    }
    
    if ( ((int)uiDataSize) <= 0 )
    {
        _ASSERT(0);//invalid size received, don't know how much to skip to next structure so bail-out.
        code.setCode( code.ERR_UNKNOWN_SCRIPT_SERVER_FILE );
     }
    else
    {
        CString strContent(pFileData,(int)uiDataSize);
        long lFileType;
        if(server.IsValidScriptServerFileName(strFilename,lFileType))
        {
			if(strContent.GetLength() > 0)
			{
				ScriptServer.m_strFileName = strFilename;
				ScriptServer.ExtractTypeAndExtension();
				ScriptServer.m_strContent = strContent ;
			}
		
        }
        else
        {
            code.setCode( code.ERR_UNKNOWN_SCRIPT_SERVER_FILE );
        }
    }
    delete [] pFileData;
    return code;
}


/**
 * Import group configuration from Server
 * Upon succes, output params repgrp, strOtherHostName and bIsThisHostSource 
 * are updated according to information received (.cfg file).
 */
MMPAPI_Error    MMP_API::getRepGroupCfg(const CServer & server, short sGrpNumber, 
                                        CReplicationGroup & replGrp, 
                                        std::string & strHostName, 
                                        std::string & strOtherHostName, 
                                        bool & bIsThisHostSource )
{
    MMPAPI_Error    code;
    char            buf[32];
    char            *pCfgData   = 0;
    unsigned int    uiDataSize  = 0;
    bool            bSourceSrvr;
    int             r;

    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;

    //try to retrieve content of pXXX.cfg
    bSourceSrvr = true;
    r = mmp_getTdmfAgentLGConfig( m_hMMP, 
                                  _itoa( server.m_nHostID, buf, 16 ),
                                  true, //bIsHostId,
                                  sGrpNumber,
                                  bSourceSrvr, //bSourceSrvr,
                                  &pCfgData,
                                  &uiDataSize
                                );
    if ( r != 0 && r != -6 )
    {
        code.setFromMMPCode(r);  
        delete [] pCfgData;
        Trace( ONLOG_EXTENDED, ">> getRepGroupCfg : ***Error, %d while retreiving  RepGroup %d"
                    " from TARGET Agent 0x%08x ***", r, sGrpNumber, server.m_nHostID );
        return code;
    }

    if ( r == -6 )
    {   //requested group cfg (pXXX) does not exists on Server.  Try sXXX .
        //try to retrieve content of sXXX.cfg
        bSourceSrvr = false;
        r = mmp_getTdmfAgentLGConfig( m_hMMP, 
                                      _itoa( server.m_nHostID, buf, 16 ),
                                      true, //bIsHostId,
                                      sGrpNumber,
                                      bSourceSrvr, //bSourceSrvr,
                                      &pCfgData,
                                      &uiDataSize
                                      );
        if ( r != 0 && r != -6 )
        {
            code.setFromMMPCode(r);  
            delete [] pCfgData;
            Trace( ONLOG_EXTENDED, ">> getRepGroupCfg : ***Error, %d while retreiving  RepGroup %d"
                        " from TARGET Agent 0x%08x ***", r, sGrpNumber, server.m_nHostID );
            return code;
        }
    }

    if ( r == -6 )
    {   //neither pXXX.cfg nor sXXX.cfg were found. the requested group just does not exist on the server.
        code.setCode( code.ERR_INVALID_PARAMETER );//bad group number
        delete [] pCfgData;
        return code;
    }

    //analyze the content of the cfg file...
	bool bIsWindows = ( strstr(server.m_strOSType.c_str(),"Windows") != 0 ||
						strstr(server.m_strOSType.c_str(),"windows") != 0 ||
						strstr(server.m_strOSType.c_str(),"WINDOWS") != 0 );
    tdmf_fill_objects_from_lgcfgfile( pCfgData, uiDataSize, bSourceSrvr, bIsWindows, sGrpNumber, replGrp, strHostName, strOtherHostName );
    bIsThisHostSource = bSourceSrvr;

    delete [] pCfgData;
    return code;
}

MMPAPI_Error    MMP_API::getRepGroupCfg(const CServer & server, 
                                        std::list<CReplicationGroup> & listGrp, 
										std::list<std::string> & listPrimaryHostName ,
                                        std::list<std::string> & listTgtHostName )
{
    MMPAPI_Error    code;
    char            *pCfgData   = 0;
    char            buf[16];
    unsigned int    uiDataSize  = 0;
    bool            bSourceSrvr;
    int             r;
    short           sGrpNumber;

    listGrp.clear(); 
    listPrimaryHostName.clear(); 
    listTgtHostName.clear(); 

    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;

    //retrieve content of ALL p*.cfg files on Server
    bSourceSrvr = true;
    r = mmp_getTdmfAgentLGConfig( m_hMMP, 
                                  _itoa( server.m_nHostID, buf, 16 ),
                                  true, //bIsHostId,
                                  -1,   //sGrpNumber : get all groups 
                                  0,    //bSourceSrvr : don't care when sGrpNumber is -1
                                  &pCfgData,
                                  &uiDataSize
                                );
    if ( r != 0 )
    {
        code.setFromMMPCode(r);  
        delete [] pCfgData;
        Trace( ONLOG_EXTENDED, ">> getRepGroupCfg ALL: ***Error, %d while retreiving ALL RepGroup cfg"
                    " from TARGET Agent 0x%08x ***", r, server.m_nHostID );
        return code;
    }


    unsigned int iBytesProcessed = 0;
    while( iBytesProcessed < uiDataSize )
    {
		listGrp.push_back( CReplicationGroup() );
        CReplicationGroup&  replGrp = listGrp.back();
        std::string         strOtherHostName, strHostName;
        mmp_TdmfFileTransferData *pFileTxData;

        pFileTxData = (mmp_TdmfFileTransferData *)(pCfgData + iBytesProcessed);
        mmp_convert_ntoh(pFileTxData);
        //pFileTxData->szFilename   ->> name of .cfg file, WITHOUT path.
  	    //pFileTxData->iType;		        //enum tdmf_filetype
        //pFileTxData->uiSize;             //size of following data. 
        if ( ((int)pFileTxData->uiSize) <= 0 )
        {
            _ASSERT(0);//invalid size received, don't know how much to skip to next structure so bail-out.
            break;
        }

        sGrpNumber = (short)atoi( &pFileTxData->szFilename[1] );_ASSERT(sGrpNumber>=0);

        //analyze the content of the cfg file...
		bool bIsWindows = ( strstr(server.m_strOSType.c_str(),"Windows") != 0 ||
							strstr(server.m_strOSType.c_str(),"windows") != 0 ||
							strstr(server.m_strOSType.c_str(),"WINDOWS") != 0 );
        tdmf_fill_objects_from_lgcfgfile( (char*)(pFileTxData+1), pFileTxData->uiSize, true/*bSourceSrvr*/, bIsWindows, sGrpNumber, replGrp, strHostName, strOtherHostName );
        //bIsThisHostSource = bSourceSrvr;
        
		listPrimaryHostName.push_back( strHostName ); 
        listTgtHostName.push_back( strOtherHostName ); 

        iBytesProcessed += sizeof(mmp_TdmfFileTransferData)+pFileTxData->uiSize;
    }

    delete [] pCfgData;
    return code;
}

MMPAPI_Error    MMP_API::tdmfStart          (const CReplicationGroup & repGroup)
{
    char szOptions[32];
    sprintf( szOptions, "-g%03d", repGroup.m_nGroupNumber );
    return tdmfCmd( repGroup.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_START, szOptions );
}

MMPAPI_Error    MMP_API::tdmfStart          (const CServer & srcServer)
{
    return tdmfCmd( srcServer.m_nHostID, MMP_MNGT_TDMF_CMD_START, "-a" );
}

MMPAPI_Error    MMP_API::tdmfStop           (const CReplicationGroup & repGroup)
{
    char szOptions[32];
    sprintf( szOptions, "-g%03d", repGroup.m_nGroupNumber );
    return tdmfCmd( repGroup.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_STOP, szOptions );
}

MMPAPI_Error    MMP_API::tdmfStop           (const CServer & srcServer)
{
    return tdmfCmd( srcServer.m_nHostID, MMP_MNGT_TDMF_CMD_STOP, "-a" );
}

MMPAPI_Error    MMP_API::tdmfLaunchRefresh  (const CReplicationGroup & repGroup, bool bForce)
{
    char szOptions[32];
    sprintf( szOptions, "-g%03d %s", repGroup.m_nGroupNumber, bForce ? "-f" : " " );
    return tdmfCmd( repGroup.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_LAUNCH_REFRESH, szOptions );
}

MMPAPI_Error    MMP_API::tdmfKillPMD        (const CReplicationGroup & repGroup)
{
    char szOptions[32];
    sprintf( szOptions, "-g%03d", repGroup.m_nGroupNumber );
    return tdmfCmd( repGroup.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_KILL_PMD, szOptions );
}

MMPAPI_Error    MMP_API::tdmfKillPMD        (const CServer & srcServer)
{
    return tdmfCmd( srcServer.m_nHostID, MMP_MNGT_TDMF_CMD_KILL_PMD, "-a" );
}

// base tdmf tool method
MMPAPI_Error    MMP_API::tdmfCmd( int iHostID /*agent host id*/, enum tdmf_commands eCmd, const char *szOptions )
{
    MMPAPI_Error    code;


    Trace( ONLOG_EXTENDED, ">> tdmfCmd : HostID=0x%08x , eCmd=%d , Options=%s ***", iHostID, (int)eCmd, szOptions );

    if ( eCmd < FIRST_TDMF_CMD && eCmd > LAST_TDMF_CMD )
    {

        Trace( ONLOG_EXTENDED, ">> tdmfCmd : ***Error , illegal command parameter value (%d)***", (int)eCmd );
        return MMPAPI_Error( MMPAPI_Error::ERR_INVALID_PARAMETER );    
    }

    code = createMMPHandle();
    if ( code.IsOK() == false)
        return code;

    char buf[32], *pszCmdOutput = 0 ;
    int r = mmp_mngt_sendTdmfCommand( m_hMMP, 
                                      itoa(iHostID,buf,16), //szAgentId,
                                      true,                 //bIsHostId,
                                      eCmd,
                                      szOptions,
                                      &pszCmdOutput );
    if ( r == 0 )
    {   //tdmf cmd (tdmf...exe) performed by Agent returns success
        code.setCode( code.OK );
    }
    else if ( r > 0 )
    {   //tdmf cmd (tdmf...exe) performed by Agent returns an error
        if ( eCmd == MMP_MNGT_TDMF_CMD_STOP && (pszCmdOutput != NULL) && strstr(pszCmdOutput,"dtcstop.exe") && 
             strstr(pszCmdOutput,"WARNING") && strstr(pszCmdOutput,"Target groups not started") )
        {   //do not consider a failure when attemtping to STOP a non-started Group
            code.setCode( code.OK );
        }
        else
        {
            code.setCode( code.ERR_AGENT_PROCESSING_TDMF_CMD );
            code.setMessage( pszCmdOutput );
        }
    }
    else 
    {   //other problem processing this request
        code.setFromMMPCode(r);
        if ( code.IsOK() == false )
        {
            Trace( ONLOG_EXTENDED, ">> tdmfCmd : ***Error, command failed: %s ***", (LPCTSTR)((CString)code) );
        }
    }

    if (pszCmdOutput != 0)
        m_strTdmfCmdOutput = pszCmdOutput;
    else
        m_strTdmfCmdOutput.erase();

    delete [] pszCmdOutput;
    return code;
}

std::string     MMP_API::getTdmfCmdOutput()
{
    return m_strTdmfCmdOutput;
}

MMPAPI_Error MMP_API::assignDomain(CServer *pServer, CDomain *pDomain)
{
    int           iHostID       = pServer->m_nHostID;
    const char*   szDomainName  = pDomain->m_strName.c_str();

    Trace( ONLOG_EXTENDED, ">> assignDomain : HostID=0x%08x , Domain name=(%s) ", iHostID, szDomainName );

	CString cszDomainKa;

    if ( m_db->mpRecDom->FtdPos(szDomainName) == false )
    {
		// Create domain
		if ( !m_db->mpRecDom->FtdNew(szDomainName) )
		{   //error, could not create Domain!
			return MMPAPI_Error( MMPAPI_Error::ERR_UNKNOWN_DOMAIN_NAME );
		}
		cszDomainKa = m_db->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );

		pDomain->m_iDbK = atoi(cszDomainKa);

		// Advise other GUI
		SendTdmfObjectChangeMessage(TDMF_DOMAIN, TDMF_ADD_NEW, atoi(cszDomainKa), 0, 0);
    }
	else
	{
		cszDomainKa = m_db->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );
	}

    if ( m_db->mpRecSrvInf->FtdPos(iHostID) == false )
    {
        Trace( ONLOG_EXTENDED, ">> assignDomain : ***Error, unknown HostID (0x%08x)***", iHostID );
        return MMPAPI_Error( MMPAPI_Error::ERR_UNKNOWN_HOST_ID );
    }

    if ( m_db->mpRecSrvInf->FtdUpd(FsTdmfRecSrvInf::smszFldDomFk, cszDomainKa) == false )
    {
        Trace( ONLOG_EXTENDED, ">> assignDomain : ***Error while updating ServerInfo DB record***" );
        return MMPAPI_Error( MMPAPI_Error::ERR_INTERNAL_ERROR );
    }

	// Advise other GUI
	SendTdmfObjectChangeMessage(TDMF_SERVER, TDMF_MOVE, pServer->m_iDbDomainFk, pServer->m_iDbSrvId, atoi(cszDomainKa));

	pServer->m_iDbDomainFk = atoi(cszDomainKa);

    return MMPAPI_Error( MMPAPI_Error::OK );
}


MMPAPI_Error MMP_API::getDevices( const long nHostID,
                                 /*out*/std::list<CDevice> & listCDevice,
								  bool bInitInfoFull )
{
    MMPAPI_Error code;
    char buf[16];
    mmp_TdmfDeviceInfo *pTdmfDeviceInfoVector,*pDevInfo;
    unsigned int        uiNbrDeviceInfoInVector = 0;                                

    listCDevice.clear();

    Trace( ONLOG_EXTENDED, ">> getServerDevices : HostID=0x%08x ", nHostID );

    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;

    int r = mmp_getTdmfAgentAllDevices( m_hMMP, 
                                        itoa(nHostID,buf,16),   //szAgentId,
                                        true,                   //bIsHostId,
                                        &pTdmfDeviceInfoVector,
                                        &uiNbrDeviceInfoInVector );
    code.setFromMMPCode(r);

    if ( code.IsOK() )
    {
        Trace( ONLOG_EXTENDED, ">> getServerDevices : %d devices reported by Server (Agent)", uiNbrDeviceInfoInVector );
    }
    else
    {
        Trace( ONLOG_EXTENDED, ">> getServerDevices : ***Error, command failed: %s ***", (LPCTSTR)((CString)code) );
    }

    CDevice device;
    pDevInfo = pTdmfDeviceInfoVector;

	bool bAddSource = true;  // TargetVolumeTag
	bool bAddTarget = false; // TargetVolumeTag

	// Search for Target Volume Tag
	bool bTargetVolumeTag = false;                                      // TargetVolumeTag
    for( unsigned int i=0; i<uiNbrDeviceInfoInVector; i++, pDevInfo++ ) // TargetVolumeTag
    {                                                                   // TargetVolumeTag
       if (strstr(pDevInfo->szDrivePath, "Target Volumes") != NULL)      // TargetVolumeTag
			bTargetVolumeTag = true;                                    // TargetVolumeTag
	}                                                                   // TargetVolumeTag
	if (bTargetVolumeTag == false)	// TargetVolumeTag
	{								// TargetVolumeTag
		bAddTarget = true;			// TargetVolumeTag
	}								// TargetVolumeTag

    pDevInfo = pTdmfDeviceInfoVector;
    for( unsigned int i=0; i<uiNbrDeviceInfoInVector; i++, pDevInfo++ )
    {
		CString strPath = pDevInfo->szDrivePath;

		if (strstr(strPath, "Target Volumes") != NULL)	// TargetVolumeTag
		{												// TargetVolumeTag
			bAddSource = false;							// TargetVolumeTag
			bAddTarget = true;							// TargetVolumeTag
		}												// TargetVolumeTag
		else											// TargetVolumeTag
		{
			if(strPath.GetLength() <= 3)
				strPath.Replace(":\\",":");
			device.m_strPath  = strPath;

			if (bInitInfoFull)
			{
				switch( pDevInfo->sFileSystem )
				{
				case MMP_MNGT_FS_FAT://FAT (Windows) File Systems 10 to 19
					device.m_strFileSystem  = "FAT";                break;
				case MMP_MNGT_FS_FAT_DYN: // FAT + DYNAMIC DRIVE
					device.m_strFileSystem  = "FAT DYNAMIC";        break;
				case MMP_MNGT_FS_FAT16:
					device.m_strFileSystem  = "FAT16";              break;
				case MMP_MNGT_FS_FAT16_DYN: // FAT16 + DYNAMIC DRIVE
					device.m_strFileSystem  = "FAT16 DYNAMIC";      break;
				case MMP_MNGT_FS_FAT32:
					device.m_strFileSystem  = "FAT32";              break;
				case MMP_MNGT_FS_FAT32_DYN: // FAT32 + DYNAMIC DRIVE
					device.m_strFileSystem  = "FAT32 DYNAMIC";      break;
				case MMP_MNGT_FS_NTFS://NTFS (Windows) File Systems 
					device.m_strFileSystem  = "NTFS";               break;
				case MMP_MNGT_FS_NTFS_DYN: // NTFS + DYNAMIC DRIVE
					device.m_strFileSystem  = "NTFS DYNAMIC";       break;
				case MMP_MNGT_FS_VXFS://HP File Systems : 30,...
					device.m_strFileSystem  = "VXFS";               break;
				case MMP_MNGT_FS_HFS:
					device.m_strFileSystem  = "HFS";                break;
				case MMP_MNGT_FS_UFS://Solaris File Systems : 40,...
					device.m_strFileSystem  = "UFS";                break;
				case MMP_MNGT_FS_JFS://AIX File Systems : 50,...
					device.m_strFileSystem  = "JFS";                break;
				case MMP_MNGT_FS_UNKNOWN://unknown or unformatted partition/volume
				default:
					device.m_strFileSystem  = "unknown FS";         break;
				}

				device.m_strDriveId     = pDevInfo->szDriveId;

				device.m_strStartOff    = pDevInfo->szStartOffset;
			}

			device.m_strLength      = pDevInfo->szLength ;

			device.m_bUseAsSource = bAddSource; // TargetVolumeTag
			device.m_bUseAsTarget = bAddTarget; // TargetVolumeTag

			listCDevice.push_back(device);
		}
	}

    delete [] pTdmfDeviceInfoVector;

    return code;
}

class GetDeviceThreadParams
{
public:
    GetDeviceThreadParams()
    {
        m_Mutex = CreateMutex(0,0,0);
        bUseObject = true;
    };
    ~GetDeviceThreadParams()
    {
        WaitForSingleObject(m_Mutex,60000);
        ReleaseMutex(m_Mutex);
        CloseHandle(m_Mutex);
    }

    //information for getDevice() call
    MMP_API             *pThis;         //object
    long				nHostId;        //getDevices() input param
    std::list<CDevice>  listDevices;    //getDevices() output param
    MMPAPI_Error        code;           //getDevices() return value.
	bool                bInitInfoFull;  //getDevices() input param

    //object management members
    bool                bUseObject;     //flag indicating how much 
    HANDLE              m_Mutex;
};

#define SOURCE  0
#define TARGET  1
MMPAPI_Error MMP_API::getDevices( const long nHostId,
								  bool bInitInfoFull,
                                 /*out*/std::list<CDevice> & listSourceDevices,
                                  const long nHostIdTarget,
								  bool bInitInfoFullTarget,
                                 /*out*/std::list<CDevice> & listTargetDevices )
{
    MMPAPI_Error code;//default = OK
    DWORD        tid;
    HANDLE       hThreads[2];
    GetDeviceThreadParams *pThrdParams[2];

    pThrdParams[0] = new GetDeviceThreadParams;
    pThrdParams[1] = new GetDeviceThreadParams;

    pThrdParams[SOURCE]->pThis   = this;  
    pThrdParams[SOURCE]->nHostId = nHostId;
	pThrdParams[SOURCE]->bInitInfoFull = bInitInfoFull;
    pThrdParams[TARGET]->pThis   = this;
    pThrdParams[TARGET]->nHostId = nHostIdTarget;
	pThrdParams[TARGET]->bInitInfoFull = bInitInfoFull;

    hThreads[SOURCE] = CreateThread(0,0,staticGetDeviceThread,pThrdParams[SOURCE],0,&tid);
    hThreads[TARGET] = CreateThread(0,0,staticGetDeviceThread,pThrdParams[TARGET],0,&tid);

    DWORD status = WaitForMultipleObjects( //wait for both thread to be completed.
						2,
						hThreads,
						TRUE,
						MMP_MNGT_TIMEOUT_GETDEVICES*1000 + 10*1000 // ardev 030110
						);

    if ( status >= WAIT_OBJECT_0 && status <= WAIT_OBJECT_0+1 )
    {   //both threads are done with their requests
        //check SOURCE request results
        if ( pThrdParams[SOURCE]->code.IsOK() )
        {
            listSourceDevices = pThrdParams[SOURCE]->listDevices;
        }
        else
        {   //keep error code 
            code = pThrdParams[SOURCE]->code;
        }
        //check TARGET request results
        if ( pThrdParams[TARGET]->code.IsOK() )
        {
            listTargetDevices = pThrdParams[TARGET]->listDevices;
        }
        else
        {   //keep error code 
            code = pThrdParams[TARGET]->code;
        }

        delete pThrdParams[SOURCE];
        delete pThrdParams[TARGET];
    }
    else
    {   //signal to any still running thread that pThreadParams must NOT be touched 

        //check on SOURCE thread
        if ( WAIT_OBJECT_0 == WaitForSingleObject(pThrdParams[SOURCE]->m_Mutex,1000) )
        {
            if ( WAIT_TIMEOUT == WaitForSingleObject(hThreads[SOURCE],0) )
            {   //SOURCE thread is still running.  Make sure it does not touch and releases pThrdParams . 
                pThrdParams[SOURCE]->bUseObject = false;
                ReleaseMutex(pThrdParams[SOURCE]->m_Mutex);
                //pThrdParams will be deleted by thread 
            }
            else
            {   //SOURCE thread has completed normally. its output data is available.
                if ( pThrdParams[SOURCE]->code.IsOK() )
                {
                    listSourceDevices = pThrdParams[SOURCE]->listDevices;
                }
                else
                {   //keep error code 
                    code = pThrdParams[SOURCE]->code;
                }
                delete pThrdParams[SOURCE];
            }
        }
        //check on TARGET thread
        if ( WAIT_OBJECT_0 == WaitForSingleObject(pThrdParams[TARGET]->m_Mutex,1000) )
        {
            if ( WAIT_TIMEOUT == WaitForSingleObject(hThreads[TARGET],0) )
            {   //TARGET thread is still running.  Make sure it does not touch and releases pThrdParams . 
                pThrdParams[TARGET]->bUseObject = false;
                ReleaseMutex(pThrdParams[TARGET]->m_Mutex);
                //pThrdParams will be deleted by thread 
            }
            else
            {   //TARGET thread has completed normally. its output data is available.
                if ( pThrdParams[TARGET]->code.IsOK() )
                {
                    listSourceDevices = pThrdParams[TARGET]->listDevices;
                }
                else
                {   //keep error code 
                    code = pThrdParams[TARGET]->code;
                }
                delete pThrdParams[TARGET];
            }
        }
    }

    CloseHandle(hThreads[SOURCE]);
    CloseHandle(hThreads[TARGET]);
    return code;
}

DWORD WINAPI MMP_API::staticGetDeviceThread(void* pContext)
{
    GetDeviceThreadParams *pParams = (GetDeviceThreadParams*)pContext;
    std::list<CDevice>    listDevices;

    MMPAPI_Error code = pParams->pThis->getDevices( pParams->nHostId, listDevices, pParams->bInitInfoFull );

    //make sure we can safely write to pParams
    if ( WAIT_OBJECT_0 == WaitForSingleObject(pParams->m_Mutex,60000) )
    {
        if (pParams->bUseObject)
        {   //fill with output parameters values
            pParams->code           = code;
            pParams->listDevices    = listDevices;
            ReleaseMutex(pParams->m_Mutex);
        }
        else
        {   //the object has been marked for release ... harakiri
            delete pParams;//mutex object will be released and destroyed by destructor 
        }
    }
    return 0;
}

/**
 * Remove a Server from TDMF database.
 */
MMPAPI_Error MMP_API::delServer(CServer *pServer)
{
    MMPAPI_Error code;
    std::list<CReplicationGroup>::iterator  grp, grpend, tmp;

    ///////////////////////////////////////////////////
    //1. delete all groups where this server is SOURCE
    ///////////////////////////////////////////////////
    grp    = pServer->m_listReplicationGroup.begin();
    grpend = pServer->m_listReplicationGroup.end();
    code.setCode(MMPAPI_Error::OK);
    while( grp != grpend && code.IsOK() )
    {
        code = delRepGroup( *grp, false );
        if ( code.IsOK() )
        {   //success, this group no longer exists on both Servers and Tdmf DB
            grp = pServer->m_listReplicationGroup.erase( grp );
        }
        else
        {
            CReplicationGroup & replgrp = *grp;
            Trace( ONLOG_EXTENDED, ">> delServer : *** failed to delete ReplGroup. Command failed: %s ***\n"
                        "   ReplGroup nbr=%ld of SOURCE server=%s, ik=%d. TARGET server=%s, ik=%d "
                ,(LPCTSTR)((CString)code)
                , replgrp.m_nGroupNumber
                , replgrp.m_pParent->m_strName.c_str()
                , replgrp.m_iDbSrcFk
                , replgrp.m_pServerTarget->m_strName.c_str()
                , replgrp.m_iDbTgtFk
                );
		}
		grp++;
    }

    ///////////////////////////////////////////////////
    //2. delete all groups where this server is TARGET
    //   scan all Servers of Domain
    ///////////////////////////////////////////////////
    std::list<CServer>::iterator  srvr    = pServer->m_pParent->m_listServer.begin();
    std::list<CServer>::iterator  srvrend = pServer->m_pParent->m_listServer.end();
    while( srvr != srvrend )
    {
        //scan each group of current server.
        grp    = (*srvr).m_listReplicationGroup.begin();
        grpend = (*srvr).m_listReplicationGroup.end();
        code.setCode(MMPAPI_Error::OK);
        while( grp != grpend && code.IsOK() )
        {   //if group TARGET Server is the Server to be deleted, zap this group.
            //rely on CServer::operator==()
			CReplicationGroup& RG = *grp;
            if ((RG.m_pServerTarget != NULL) && (*(RG.m_pServerTarget) == *pServer))
            {
                code = delRepGroup( *grp );
                if ( !code.IsOK() )
                {
                    CReplicationGroup & replgrp = *grp;
                    Trace( ONLOG_EXTENDED, ">> delServer : *** failed to delete ReplGroup. Command failed: %s ***\n"
                                "   ReplGroup nbr=%ld of SOURCE server=%s, ik=%d. TARGET server=%s, ik=%d "
                        ,(LPCTSTR)((CString)code)
                        , replgrp.m_nGroupNumber
                        , replgrp.m_pParent->m_strName.c_str()
                        , replgrp.m_iDbSrcFk
                        , replgrp.m_pServerTarget->m_strName.c_str()
                        , replgrp.m_iDbTgtFk
                        );
                }
            }
            grp++;
        }
		srvr++;
    }

    ///////////////////////////////////////////////////
    //3. delete all Script Server where this server is SOURCE
    ///////////////////////////////////////////////////
    std::list<CScriptServer>::iterator  ssrvr    = pServer->m_listScriptServerFile.begin();
    std::list<CScriptServer>::iterator  ssrvrend = pServer->m_listScriptServerFile.end();
    code.setCode(MMPAPI_Error::OK);
    while( ssrvr != ssrvrend && code.IsOK() )
    {
        CScriptServer* pScriptServer = &*ssrvr;
        code = delScriptServerFileFromDb(pScriptServer);
        if ( code.IsOK() )
        {   //success, this Script Server no longer exists on both Servers and Tdmf DB
            ssrvr = pServer->m_listScriptServerFile.erase( ssrvr );
        }
        else
        {
            Trace( ONLOG_EXTENDED, ">> delScriptServerFile : *** failed to delete ScriptServerFile. Command failed: %s ***\n"
                        "   ScriptServer ID =%ld of SOURCE server=%s"
                ,(LPCTSTR)((CString)code)
                , pScriptServer->m_iDbScriptSrvId
                , pScriptServer->m_pServer->m_strName.c_str()
                );
		}
		ssrvr++;
    }


    ///////////////////////////////////////////////////
    //4. Try to remove Server from DB.  
    //   Will fail if any dependant records (ReplGroup,ReplPair,Performance)
    //   remain in DB (one of the delRepGroup() calls failed).
    CString cszTmp;
    cszTmp.Format(  " %s = %d "
                    ,FsTdmfRecSrvInf::smszFldHostId
                    ,pServer->m_nHostID
                    );
    m_db->mpRecSrvInf->FtdLock();
    if ( !m_db->mpRecSrvInf->FtdDelete( cszTmp ) )
    {
        Trace( ONLOG_EXTENDED, ">> delServer : ***Error deleting DB Server record: %s ***", (LPCTSTR)cszTmp );
        m_db->mpRecSrvInf->FtdUnlock();
        return MMPAPI_Error( MMPAPI_Error::ERR_DELETING_DB_RECORD );
    }
    m_db->mpRecSrvInf->FtdUnlock();

    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error MMP_API::modifyCriticalServerCfgValues(CServer *pServer, bool bStopPrimaryGroups, bool bIPPortChanged)
{
    CString         cszTmp;
    MMPAPI_Error    code;
    //////int             iOldPort = pServer->m_nPort;    

    //////if ( iOldPort == iPort )
    //////    return MMPAPI_Error( MMPAPI_Error::OK );

    if ( bStopPrimaryGroups )
    {
        /////////////////////////////////////////////////////
        //Stop all Repl.Groups where this Server is involved
        /////////////////////////////////////////////////////
        //First, stop the Repl.Groups where the server is the Repl.Groups SOURCE Servers.
        /////////////////////////////////////////////////////
        code = tdmfKillPMD(*pServer);//kill all PMDs on this server
        if ( !code.IsOK() )
            return code;
        code = tdmfStop(*pServer);//stop all groups on this server
        if ( !code.IsOK() )
            return code;
	}
    if ( bIPPortChanged )
    {
        /////////////////////////////////////////////////////
        //Second, stop all Repl.Groups where pServer is the Repl.Groups TARGET Server.
        /////////////////////////////////////////////////////
		for(std::list<CReplicationGroup>::const_iterator it = pServer->m_listReplicationGroup.begin();
			it != pServer->m_listReplicationGroup.end(); it++)
		{
			//
			// Stop Repl.Group 
			//
			cszTmp.Format(" -g%03d ", it->m_nGroupNumber);
            code = tdmfCmd(pServer->m_nHostID, MMP_MNGT_TDMF_CMD_STOP, cszTmp);

            if ( code.IsOK() == false )
            {
				Trace( ONLOG_EXTENDED, ">> setIPPort : ***Warning, tdmfstop failed: %s ***", (LPCTSTR)((CString)code) );
            }
		}
    }//if ( bIPPortChanged )

    /////////////////////////////////////////////////////
    //2. Modify all Server cfg values by sending a Set Agent Configuration msg.
    //   pServer is already modified with new values.
    //   *!*!*  Critical cfg values modification will have the Agent TO REBOOT *!*!* .
    //   New IP Port value (if so) will also be saved to DB by Collector.
    /////////////////////////////////////////////////////    
    //////pServer->m_nPort = iPort;
    code = collectorTXAgentInfo(pServer);
    if ( ! code.IsOK() )
    {   //update failed on Agent. Back to previous value.
        //////pServer->m_nPort = iOldPort;
        return code;
    }

    if ( bIPPortChanged )
    {   /////////////////////////////////////////////////////
        //3. find all RepGroups where iSrcFk is the TARGET Server
        //   Advise the RepGroup SOURCE Server of the Port change by modifying the RepGroup config.
        //   Group must be modified.
        /////////////////////////////////////////////////////        

		int iSrcHostId,iTgtHostId,iSrcFk;

        class CSystemTmp : public CSystem
        {
        public:
            CSystemTmp(FsTdmfDb* db) { m_db = db; };
            ~CSystemTmp()            { m_db = 0;  };//set to 0 to avoid having the REAL ptr being destroyed !! 
        };
        CSystemTmp  systemTmp(m_db);
        CDomain     domainTmp;
        CServer     sourceTmp;

        cszTmp.Format( " %s = %d ", FsTdmfRecLgGrp::smszFldTgtFk, pServer->m_iDbSrvId );
        m_db->mpRecGrp->FtdLock();
        if ( m_db->mpRecGrp->FtdFirst(cszTmp) )
        {
            //3.1 For each RepGroup where iSrcFk is the TARGET Server ...
            do
            {   
                //3.2 Because Port number appears in the RepGroup .cfg file
                //   a new RepGroup config has to be sent to the SOURCE Server Agent.
                iSrcFk = atoi(m_db->mpRecGrp->FtdSel(FsTdmfRecLgGrp::smszFldSrcFk));
                CString cszWhere;
                cszWhere.Format( " %s = %d ", (LPCTSTR)m_db->mpRecSrvInf->smszFldSrvId, iSrcFk );
                m_db->mpRecSrvInf->FtdLock();
                if ( m_db->mpRecSrvInf->FtdFirst(cszWhere) )
                {
                    sourceTmp.Initialize(m_db->mpRecSrvInf, false);//sourceTmp.m_strIPAddress = (LPCTSTR)m_db->mpRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldSrvIp1);
                    m_db->mpRecSrvInf->FtdUnlock();

                    //get SOURCE Server HostId
                    iSrcHostId = sourceTmp.m_nHostID;//strtol( m_db->mpRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldHostId), 0, 10 );
                    iTgtHostId = 0;//indicates to SendCfgFileToAgent() to not send Repl.Group cfg file to TARGET server.

                    //create a CReplicationGroup object from current m_db->mpRecGrp (FsTdmfRecLgGrp*)
                    CReplicationGroup repgrp;
                    //replicate a valid System - Domain - Server - Repl.Group hierarchy 
                    //so that functions like Initialize() and SendCfgFileToAgent() work as intended.
                    repgrp.m_pServerTarget  = pServer;
                    repgrp.m_pParent        = &sourceTmp;
                    sourceTmp.m_pParent     = &domainTmp;
                    domainTmp.m_pParent     = &systemTmp;
                    repgrp.Initialize(m_db->mpRecGrp, true);//load RepGroup AND RepPairs from DB
                    //repgroup is now filled with info and its rep pair list

                    //3.3  Update this group config. on Source Server.
                    code = SendCfgFileToAgent( &repgrp, iSrcHostId, iTgtHostId );
                }
                else
                {   //error
                    m_db->mpRecSrvInf->FtdUnlock();

                    code.setCode( MMPAPI_Error::ERR_DATABASE_RELATION_ERROR );
                    code.setMessage( CString("RepGroup-Server relation failure: ") + cszWhere + CString(" . DB Err msg: ")  + m_db->FtdGetErrMsg() );
                }
            } 
            while( m_db->mpRecGrp->FtdNext() );
        }
        m_db->mpRecGrp->FtdUnlock();

    }//if ( bIPPortChanged )

    return code;
}

MMPAPI_Error MMP_API::setKey(CServer *pServer, const char* szKey) 
{
    MMPAPI_Error code;
    char         szAgentId[32];
    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;
    int ExpirationDate ;
    //upon success, Agent and TDMF DB are updated 
    int r = mmp_setTdmfAgentRegistrationKey(m_hMMP, 
                                            itoa(pServer->m_nHostID,szAgentId,16),
                                            true, //bIsHostId,
                                            szKey,
                                            &ExpirationDate );
    code.setFromMMPCode(r);
    if ( false == code.IsOK() )
        return code;
 
    //update object itself
    pServer->m_strKey = szKey;
    if ( ExpirationDate > 0 )
	    pServer->m_strKeyExpirationDate = (LPCTSTR)(CTime(ExpirationDate).Format("%Y-%m-%d %H:%M:%S"));
    else
        pServer->m_strKeyExpirationDate = (  ExpirationDate == -1 ? "NULL license" :
                                             ExpirationDate == -2 ? "EMPTY license" :
                                             ExpirationDate == -3 ? "license with BAD CHECKSUM"  :
                                             ExpirationDate == -4 ? "EXPIRED license"  :
                                             ExpirationDate == -5 ? "license refers to WRONG HOST"  :
                                             ExpirationDate == -6 ? "BAD SITE license"  : 
                                                                    "unexpected error with license"
                                           );
    return code;
}

MMPAPI_Error MMP_API::getSystemParameters(mmp_TdmfCollectorParams* params)
{
	// Read params directly from DB
	params->iCollectorTcpPort = 0;

    if ( m_db->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_CHECK_PERIOD ) )
    {
        CString value = m_db->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal);
        params->iDBCheckPeriodMinutes = atoi(value);//get number
        if ( params->iDBCheckPeriodMinutes > 0 )
        {
            value.MakeLower();
            if ( strstr(value,"hour") )
                params->iDBCheckPeriodMinutes *= 60;//hour to minutes
        }
    }
    else
    {   //add this NVP to DB
        params->iDBCheckPeriodMinutes = 60;
        m_db->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_CHECK_PERIOD , CString("60 minutes") );
        CString desc;
        desc.Format("Period at which TDMF tables restrictions are checked and enforced. Value can be specified with <minutes> or <hours> units. Default value = 60 minutes .");
        m_db->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, desc );
    }

    if ( m_db->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_PERF_TBL_MAX_NUMBER ) )
    {
        params->iDBPerformanceTableMaxNbr = atoi( m_db->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( params->iDBPerformanceTableMaxNbr <= 0 )
            params->iDBPerformanceTableMaxNbr = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        params->iDBPerformanceTableMaxNbr = 2000000;
        m_db->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_PERF_TBL_MAX_NUMBER , CString("2000000") );
        m_db->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the TDMF DB t_Performance table to this value. A value of 0 (zero) disables this restriction." );
    }

    if ( m_db->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_PERF_TBL_MAX_DAYS ) )
    {
        params->iDBPerformanceTableMaxDays = atoi( m_db->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( params->iDBPerformanceTableMaxDays <= 0 )
            params->iDBPerformanceTableMaxDays = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        params->iDBPerformanceTableMaxDays = 7;//default : 7 days
        m_db->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_PERF_TBL_MAX_DAYS , CString("7 day") );
        m_db->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the TDMF DB t_Performance table based on record time_stamp. An empty field disables this resctriction." );
    }

    if ( m_db->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_NUMBER ) )
    {
        params->iDBAlertStatusTableMaxNbr = atoi( m_db->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( params->iDBAlertStatusTableMaxNbr <= 0 )
            params->iDBAlertStatusTableMaxNbr = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        params->iDBAlertStatusTableMaxNbr = 1000000;
        m_db->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_NUMBER , CString("1000000") );
        m_db->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the TDMF DB t_Alert_Status table to this value. A value of 0 (zero) disables this restriction." );
    }

    if ( m_db->mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_DAYS ) )
    {
        params->iDBAlertStatusTableMaxDays = atoi( m_db->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal) );
        if ( params->iDBAlertStatusTableMaxDays <= 0 )
            params->iDBAlertStatusTableMaxDays = 0;//disables this restriction
    }
    else
    {   //add this NVP to DB
        params->iDBAlertStatusTableMaxDays = 7;//default : 7 days
        m_db->mpRecNvp->FtdNew( TDMF_NVP_NAME_DB_ALERTSTATUS_TBL_MAX_DAYS , CString("7 day") );
        m_db->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "Limit the number of records in the TDMF DB t_Alert_Status table based on record time_stamp. An empty field disables this resctriction." );
    }

	//bValidKey;
    //iCollectorBroadcastIPMask;  //(feature not implemented) mask used to perform an IP broadcast
    //iCollectorBroadcastTimeout; //(feature not implemented) min period before closing RX of broadcast responses
    //iTimeOut1;
    //iTimeOut2;
    //iTimeOut3;
    //iTimeOut4;
    //iTimeOut5;
    //iTimeOut6;
    //iTimeOut7;
    //iTimeOut8;
    //iTimeOut9;
    //iTimeOut10;

	return MMPAPI_Error::OK;
}


MMPAPI_Error MMP_API::setSystemParameters(const mmp_TdmfCollectorParams* params)
{
    MMPAPI_Error code;
    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;
    //upon success, Agent and TDMF DB are updated 
    int r = mmp_mngt_setSystemParameters(m_hMMP, params);
    code.setFromMMPCode(r);
    return code;
}


MMPAPI_Error    MMP_API::registerForNotifications(int iPeriod /*seconds*/, const HWND hWnd, UINT uiWinMsg)
{
	Trace( ONLOG_EXTENDED, "registerForNotifications");

    DWORD tid;

    m_hDestHwnd             = hWnd;
    m_uiDestWinMsg          = uiWinMsg;
    m_iNotifPeriod          = iPeriod;//seconds
    m_fnNotifCallback       = 0;

    m_hevNotifDataReady     = CreateEvent(0,0,0,0);
    m_hevNotifDataProcessed = CreateEvent(0,0,0,0);
    m_hevEndNotifThread     = CreateEvent(0,0,0,0);

    m_hNotifThread          = CreateThread(0,0,staticNotificationThread,this,0,&tid);

    m_SubscriptionsRegisterSet          = new NotifSubscriptionItemSet;

    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error    MMP_API::registerForNotifications(int iPeriod /*seconds*/, void (*fnNotifCallback)(void* /*pContext*/, TDMFNotificationMessage* /*pNotifMsg*/), void* pContext )
{
	Trace( ONLOG_EXTENDED, "registerForNotifications");

    DWORD tid;

    m_hDestHwnd             = 0;
    m_uiDestWinMsg          = 0;
    m_iNotifPeriod          = iPeriod;//seconds
    m_fnNotifCallback       = fnNotifCallback;
    m_pContext              = pContext;

    m_hevNotifDataReady     = CreateEvent(0,0,0,0);
    m_hevNotifDataProcessed = CreateEvent(0,0,0,0);
    m_hevEndNotifThread     = CreateEvent(0,0,0,0);

    m_hNotifThread          = CreateThread(0,0,staticNotificationThread,this,0,&tid);

    m_SubscriptionsRegisterSet          = new NotifSubscriptionItemSet;

    return MMPAPI_Error( MMPAPI_Error::OK );
}

void    MMP_API::unregisterForNotifications()
{
	Trace( ONLOG_EXTENDED, "unregisterForNotifications");

    if ( m_hNotifThread )
    {   
        SetEvent(m_hevEndNotifThread);
        WaitForSingleObject(m_hNotifThread,5000);
        CloseHandle(m_hNotifThread);
        m_hNotifThread = 0;
    }
    if ( m_hevEndNotifThread )
        CloseHandle(m_hevEndNotifThread);
    m_hevEndNotifThread = 0;

    delete m_SubscriptionsRegisterSet;
    m_SubscriptionsRegisterSet = 0;

    if( m_hevNotifDataReady )
        CloseHandle(m_hevNotifDataReady);
    m_hevNotifDataReady = 0;
    if( m_hevNotifDataProcessed )
        CloseHandle(m_hevNotifDataProcessed);
    m_hevNotifDataProcessed = 0;
}

/**
 * Add/Remove notification on one Server/Group or a list of Server/Group.
 */
MMPAPI_Error    MMP_API::requestNotification(const CReplicationGroup & repGroup, 
                                    enum eSubscriptionAction eAction, 
                                    enum eSubscriptionDataType eType 
                                    )
{	
	Trace( ONLOG_EXTENDED, "requestNotification %s type %s for 0x%X %d",
		   (eAction == ACT_ADD) ? "ADD" : "DELETE",
	       (eType == TYPE_STATE) ? "STATE" : "FULL_STATS",
		   (repGroup.m_nType & CReplicationGroup::GT_SOURCE) ? repGroup.m_pParent->m_nHostID : repGroup.m_pServerTarget->m_nHostID,
		   repGroup.m_nGroupNumber );

    TDMFNotificationSubscriptionMessage subsMsg;
    subsMsg.requestInit();
	if (repGroup.m_nType & CReplicationGroup::GT_SOURCE)
	{
		subsMsg.requestServer(repGroup.m_pParent->m_nHostID);
	}
	else
	{
		subsMsg.requestServer(repGroup.m_pServerTarget->m_nHostID);
	}

    subsMsg.requestGroup(repGroup.m_nGroupNumber,eAction,eType);
    subsMsg.requestComplete();

    //memorize this subscription
    m_SubscriptionsRegisterSet->Insert_Modify( 
        NotifSubscriptionItem(repGroup.m_pParent->m_nHostID,repGroup.m_nGroupNumber,eAction,eType)
                                      );
    requestToNotifThread(subsMsg.getSubscriptionRequestData(), subsMsg.getSubscriptionRequestDataLen());

    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error    MMP_API::requestNotification(std::list<CReplicationGroup> & listGroup, 
                                    enum eSubscriptionAction eAction, 
                                    enum eSubscriptionDataType eType 
                                    )
{
    std::list<CReplicationGroup>::const_iterator it  = listGroup.begin();
    std::list<CReplicationGroup>::const_iterator end = listGroup.end();

    if ( it != end )
    {   
        TDMFNotificationSubscriptionMessage subsMsg;
        subsMsg.requestInit();
        while ( it != end )
        {
	        //prefix Group request with their Source Server ID
			if ((*it).m_nType & CReplicationGroup::GT_SOURCE)
			{
				subsMsg.requestServer((*it).m_pParent->m_nHostID);
			}
			else
			{
				if ((*it).m_pServerTarget != NULL)
				{
					subsMsg.requestServer((*it).m_pServerTarget->m_nHostID);
				}
			}

            subsMsg.requestGroup((*it).m_nGroupNumber,eAction,eType);

			Trace( ONLOG_EXTENDED, "requestNotification %s type %s for 0x%X %d",
				   (eAction == ACT_ADD) ? "ADD" : "DELETE",
				   (eType == TYPE_STATE) ? "STATE" : "FULL_STATS",
				   ((*it).m_nType & CReplicationGroup::GT_SOURCE) ? (*it).m_pParent->m_nHostID : ((*it).m_pServerTarget ? (*it).m_pServerTarget->m_nHostID : -1),
				   (*it).m_nGroupNumber );

            //memorize this subscription
            m_SubscriptionsRegisterSet->Insert_Modify( 
                NotifSubscriptionItem((*it).m_pParent->m_nHostID,(*it).m_nGroupNumber,eAction, eType)
                                              );   

            it++;
        }
        subsMsg.requestComplete();

        requestToNotifThread(subsMsg.getSubscriptionRequestData(), subsMsg.getSubscriptionRequestDataLen());

        return MMPAPI_Error( MMPAPI_Error::OK );
    }
    
    return MMPAPI_Error( MMPAPI_Error::OK ); // 020725, lets now allow srv without grp
    //return MMPAPI_Error( MMPAPI_Error::ERR_EMPTY_OBJECT_LIST_PROVIDED );
}

MMPAPI_Error    MMP_API::requestNotification(   const CServer & server, 
                                                enum eSubscriptionAction eAction
                                                )
{
	Trace( ONLOG_EXTENDED, "requestNotification %s type %s for 0x%X",
		   (eAction == ACT_ADD) ? "ADD" : "DELETE",
	       "STATE",
		   server.m_nHostID);

    TDMFNotificationSubscriptionMessage subsMsg;
    subsMsg.requestInit();
    subsMsg.requestServer(server.m_nHostID,eAction,TYPE_STATE);
    subsMsg.requestComplete();

    //memorize this subscription
    m_SubscriptionsRegisterSet->Insert_Modify( 
        NotifSubscriptionItem(server.m_nHostID,INVALID_GRP_NBR /*Server -> specify no group number*/, eAction, TYPE_STATE )
                                      );
    requestToNotifThread(subsMsg.getSubscriptionRequestData(), subsMsg.getSubscriptionRequestDataLen());

    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error    MMP_API::requestNotification(std::list<CServer> & listServer, 
                                             enum eSubscriptionAction eAction
                                             )
{
	Trace( ONLOG_EXTENDED, "requestNotification");

    std::list<CServer>::const_iterator it  = listServer.begin();
    std::list<CServer>::const_iterator end = listServer.end();
    _ASSERT(it != end);//list of CServer should not be empty...
    if ( it != end )
    {
        TDMFNotificationSubscriptionMessage subsMsg;
        subsMsg.requestInit();
        while ( it != end )
        {
            subsMsg.requestServer((*it).m_nHostID,eAction,TYPE_STATE);

			Trace( ONLOG_EXTENDED, "requestNotification %s type %s for 0x%X",
				   (eAction == ACT_ADD) ? "ADD" : "DELETE",
				   "STATE",
				   (*it).m_nHostID);

            //memorize this subscription
            m_SubscriptionsRegisterSet->Insert_Modify( 
                NotifSubscriptionItem((*it).m_nHostID,INVALID_GRP_NBR/*Server -> specify no group number*/,eAction,TYPE_STATE)   
                                              );  
            it++;
        }
        subsMsg.requestComplete();

        requestToNotifThread(subsMsg.getSubscriptionRequestData(), subsMsg.getSubscriptionRequestDataLen());

        return MMPAPI_Error( MMPAPI_Error::OK );
    }
    
    return MMPAPI_Error( MMPAPI_Error::ERR_EMPTY_OBJECT_LIST_PROVIDED );
}

void MMP_API::requestToNotifThread(const char * pData, int iDataLen )
{
    if ( pData != 0 && iDataLen > 0 )
    {
        //get crit section : garanties exclusive access among request...() methods
        EnterCriticalSection(&m_csNotifRequest);

        //fill Notif. buffer
        //it is ok to refer to parameter because caller is blocked until thread has processed buffer
        m_pNotifBuffer      = (char*)pData;
        m_iNotifBufferLen   = iDataLen;

        //signal thread that buf is ready to be sent
        SetEvent(m_hevNotifDataReady);
        //wait for thread to be done with data buffer
        WaitForSingleObject(m_hevNotifDataProcessed,30000);

        LeaveCriticalSection(&m_csNotifRequest);
    }
}

/**
 * Thread launched from within staticNotificationThread().
 * It is used to simulate a subscription request, as if it comes from outside this object.
 */ 
DWORD WINAPI MMP_API::staticRequestToNotifThread(void *pThis)
{
    ((MMP_API*)pThis)->requestNotification();
    return 0;
}

/**
 * Used internally to subscribe to Collector after socket connection with Collector 
 * has been cut and re-connected.
 */
void        MMP_API::requestNotification()
{
    NotifSubscriptionItemSet::iterator item = m_SubscriptionsRegisterSet->begin();
    NotifSubscriptionItemSet::iterator end  = m_SubscriptionsRegisterSet->end();

    printf("\n\n requestNotification \n\n");

    if( item != end )
    {
        TDMFNotificationSubscriptionMessage subsMsg;
        int prevServer = INVALID_HOST_ID;

        subsMsg.requestInit();
        while( item != end )
        {
            if ( (*item).getGroupNbr() != INVALID_GRP_NBR )
            {   //GROUP state subscription request.
                if ( (*item).getHostId() != prevServer )
                {   //specify new server
                    subsMsg.requestServer( (*item).getHostId() );
                    prevServer = (*item).getHostId();
                }
                subsMsg.requestGroup((*item).getGroupNbr(),(*item).getAction(),(*item).getType());
            }
            else
            {   //SERVER state subscription request.
                subsMsg.requestServer((*item).getHostId(),(*item).getAction(),(*item).getType());
                prevServer = (*item).getHostId();
            }

            item++;
        }
        subsMsg.requestComplete();

        requestToNotifThread(subsMsg.getSubscriptionRequestData(), subsMsg.getSubscriptionRequestDataLen());
    }
}

DWORD WINAPI MMP_API::staticNotificationThread(void *pThis)
{
    ((MMP_API*)pThis)->NotificationThread();
    return 0;
}

/**
 * This thread manages the Notification msg exchanges with the Collector.
 * It sends notificaiotn request
 * 
 */
void MMP_API::NotificationThread()
{
    bool bConnectedToCollector = false;
    bool bThreadAlive = true;
    bool bRXTXErrorWithCollector = false;
    int  r,nHandles;
    HANDLE handles[3];
    HANDLE hReadDataFromSocket = CreateEvent(0,0,0,0);

    //open socket on collector
    //sock_t * socket = sock_create();
    m_socket = sock_create();

    MMPAPI_Error code = createMMPHandle();_ASSERT( code.IsOK() );

    while( bThreadAlive )
    {
        if ( !bConnectedToCollector )
        {   //attempt to connect to Collector 
			Trace(ONLOG_DEBUG, "attempt to connect to Collector");
            TDMFNotificationMessage *pstatusMsg;

            bConnectedToCollector = (0 == mmp_registerNotificationMessages( m_hMMP, m_socket, m_iNotifPeriod ) );
            if ( bConnectedToCollector )
            {   //assign one Win32 Event with socket 
                m_socket->hEvent = hReadDataFromSocket;
                r = sock_set_nonb(m_socket);_ASSERT(r == 0);

                ////////////////////////////////////////////////////////////////
                //connected to Collector. advise data consumer.
				Trace(ONLOG_DEBUG, "connected to Collector");
                pstatusMsg = new TDMFNotificationMessage(NOTIF_MSG_COLLECTOR_CONNECTED);

                ////////////////////////////////////////////////////////////////
                //prepare subscription requests history to be sent to Collector
                //send from a separate thread to have same conditions than other
                //
                DWORD tid;
                HANDLE h = CreateThread(0,0,staticRequestToNotifThread,this,0,&tid);
                CloseHandle(h);
            }
            else
            {   //error connecting to Collector. 
                //advise data consumer that Collector can not be reached 
				Trace(ONLOG_DEBUG, "error connecting to Collector.");
                pstatusMsg = new TDMFNotificationMessage(NOTIF_MSG_COLLECTOR_UNABLE_TO_CONNECT);
            }
            ////////////////////////////////////////////////////////////////
            //send connection attempt status to data consumer (object owner)
            //data consumer is responsible to free pstatusMsg .
            if ( m_hDestHwnd != 0 && m_uiDestWinMsg != 0 ) 
            {
                BOOL b = PostMessage(m_hDestHwnd, m_uiDestWinMsg, (LPARAM)0, (LPARAM)pstatusMsg );_ASSERT(b);
            } 
            else if ( m_fnNotifCallback != 0 )
                m_fnNotifCallback(m_pContext,pstatusMsg);
            else
                delete pstatusMsg;
        }

        if ( bConnectedToCollector )
        {   
            nHandles   = 3;
            handles[0] = m_hevEndNotifThread;//end thread
            handles[1] = hReadDataFromSocket;//data RX from Collector
            handles[2] = m_hevNotifDataReady;//requestNotification() called by user
        }
        else
        {
            nHandles   = 2;
            handles[0] = m_hevEndNotifThread;//end thread
            handles[1] = m_hevNotifDataReady;//requestNotification() called by user
        }


        DWORD status = WaitForMultipleObjects( nHandles, handles, FALSE, 10000 );//10 seconds timeout
        if (status == WAIT_OBJECT_0)//end thread
        {
            bThreadAlive = false;
        }
        else if (bConnectedToCollector && status == WAIT_OBJECT_0 + 1)//data RX from Collector
        {   //notification data RX from Collector
            unsigned long ulBytesToRead = tcp_get_numbytes(m_socket->sock);
            if ( ulBytesToRead == (unsigned long)-1 )
            {   //tcp_get_numbytes returned 0xffffffff, socket error, disconnect socket and retry later ...
                _ASSERT(0);
				Trace(ONLOG_DEBUG, "tcp_get_numbytes returned 0xffffffff, socket error");
                bRXTXErrorWithCollector = true;
            }
            else if ( ulBytesToRead > 0 )
            {
				Trace(ONLOG_DEBUG, "Message received.");
                TDMFNotificationMessage *prxMsg = new TDMFNotificationMessage;
                bool b = prxMsg->Recv( m_socket, 30000 /*millisecs*/ );_ASSERT(b);
                if ( b )
                {   //callee is responsible to free prxMsg .
                    if ( m_hDestHwnd != 0 && m_uiDestWinMsg != 0 )
                    {
                        BOOL b = PostMessage(m_hDestHwnd, m_uiDestWinMsg, (LPARAM)0, (LPARAM)prxMsg );_ASSERT(b);
                    }
                    else if ( m_fnNotifCallback != 0 )
                    {
                        m_fnNotifCallback(m_pContext,prxMsg);
                    }
                    else
                        delete prxMsg;
                }
                else
                {   //error rx from socket
                    delete prxMsg;
                    bRXTXErrorWithCollector = true;
                }
            }
        }
        else if ( (!bConnectedToCollector && status == WAIT_OBJECT_0 + 1) ||
                  (bConnectedToCollector  && status == WAIT_OBJECT_0 + 2) )//requestNotification() called by user
        {
			Trace(ONLOG_DEBUG, "send a NOTIF_MSG_SUBSCRIPTION message to Collector");
            //send a NOTIF_MSG_SUBSCRIPTION message to Collector
            TDMFNotificationMessage notifMsg(NOTIF_MSG_SUBSCRIPTION, m_iNotifBufferLen, m_pNotifBuffer );
            bool b = notifMsg.Send( m_socket );
            //thread is done with data buffer 
            SetEvent(m_hevNotifDataProcessed);
            if ( !b )
            {   //error tx on socket
                bRXTXErrorWithCollector = true;
            }
        }

        //Manage socket error with Collector
        if ( bRXTXErrorWithCollector )
        {
			Trace(ONLOG_DEBUG, "Collector can not be reached");
            bRXTXErrorWithCollector = false;
            //close monitoring datat socket with Collector
            sock_set_b(m_socket);
            sock_disconnect(m_socket);
            bConnectedToCollector = false;
            //advise data consumer that Collector can not be reached 
            //callee is responsible to free prxMsg .
            TDMFNotificationMessage *perrMsg = new TDMFNotificationMessage(NOTIF_MSG_COLLECTOR_COMMUNICATION_PROBLEM);
            if ( m_hDestHwnd != 0 && m_uiDestWinMsg != 0 )
            {
                BOOL b = PostMessage(m_hDestHwnd, m_uiDestWinMsg, (LPARAM)0, (LPARAM)perrMsg );_ASSERT(b);
            }
            else if ( m_fnNotifCallback != 0 )
            {
                m_fnNotifCallback(m_pContext,perrMsg);
            }
            else
                delete perrMsg;
        }
    }

    if ( bConnectedToCollector )
    {
        sock_set_b(m_socket);
        sock_disconnect(m_socket);
    }

    CloseHandle(hReadDataFromSocket);
    sock_delete(&m_socket);
}

/**
 * From provided CServer, fill a mmp_mngt_TdmfAgentConfigMsg_t and send it to Collector 
 */
MMPAPI_Error MMP_API::collectorTXAgentInfo(CServer *pServer)
{
    MMPAPI_Error code;
    char szAgentId[16];
    mmp_TdmfServerInfo agentCfg;

    agentCfg.iPort          = pServer->m_nPort;
    //must specify agentCfg.iBABSize in MegaBytes
    agentCfg.iBABSizeReq    = pServer->m_nBABSizeReq;
    //must specify agentCfg.iTCPWindowSize in KiloBytes
    agentCfg.iTCPWindowSize = pServer->m_nTCPWndSize;
    strcpy( agentCfg.szTDMFDomain, pServer->m_pParent->m_strName.c_str() );

    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;
    int r = mmp_mngt_setTdmfAgentConfig( m_hMMP, 
                                         itoa(pServer->m_nHostID,szAgentId,16),
                                         true, //bIsHostId,
                                         &agentCfg
                                        );
    code.setFromMMPCode(r);
    return code;
}

MMPAPI_Error MMP_API::collectorTXRepGroup( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId, bool bSymmetric )
{
    MMPAPI_Error code;
    int iRepGroupNum = bSymmetric ? repgrp->m_nSymmetricGroupNumber : repgrp->m_nGroupNumber;
    
    //3. From the RepGroup definition, create the .cfg file content. 
    int  r;
    char buf[16];
    char *pCfgData = 0;
    unsigned int  uiDataSize;
    //make sure we are ready to contact the Collector
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;
    //3.1 From the RepGroup definition (DB), create the .cfg file content. 
    //
    tdmf_create_lgcfgfile_buffer( repgrp, true /*bIsPrimary*/, &pCfgData, &uiDataSize, bSymmetric );
    if ( uiDataSize == 0 )
    {
        code.setCode( MMPAPI_Error::ERR_INTERNAL_ERROR );
        code.setMessage( " MMP_API::collectorTXRepGroup : Error while converting CReplicationGroup to .cfg file" );
        return code;
    }

    //
    //3.2.1 directory for PStore file MUST exist on SOURCE server.  make sure it does.
	char szPStorePath[_MAX_PATH];
	// Windows pstore
	if (strstr(repgrp->m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
	{
		char szDisk[_MAX_PATH],szDir[_MAX_PATH];
		_ASSERT( (repgrp->m_strPStoreFile.at(0) >= 'a' && repgrp->m_strPStoreFile.at(0) <= 'z') ||
			     (repgrp->m_strPStoreFile.at(0) >= 'A' && repgrp->m_strPStoreFile.at(0) <= 'Z') );
		_ASSERT( repgrp->m_strPStoreFile.at(1) == ':' );
		_splitpath(repgrp->m_strPStoreFile.c_str(),szDisk,szDir,NULL,NULL);
		sprintf(szPStorePath,"md \"%s%s\" ",szDisk,szDir); 

	    code = tdmfCmd(iSrcHostId, MMP_MNGT_TDMF_CMD_OS_CMD_EXE, szPStorePath);
		if ( code.IsOK() == false )
		{
			CString msg;
			msg.Format("***Error while communicating with Source Server 0x%08x. (%s)",iSrcHostId,szPStorePath);
			Trace( ONLOG_DEBUG, ">> addRepGroup : %s",(LPCTSTR)msg);
			code.setMessage(msg);
			free(pCfgData);
			return code;
		}
	}

    //
    //3.2.2 Send new config. to SOURCE 
    r = mmp_setTdmfAgentLGConfig( m_hMMP, 
                                  _itoa( iSrcHostId, buf, 16 ),
                                  true, //bIsHostId,
                                  iRepGroupNum,
                                  true, //bSourceSrvr,
                                  pCfgData,
                                  uiDataSize
                                );_ASSERT(r==0);
    if ( r != 0 )
    {
        Trace( ONLOG_DEBUG, ">> addRepGroup : ***Error, while setting a the RepGroup configuration on Server 0x%08x ***",iSrcHostId);
        free(pCfgData);
        //todo :
        //restore previous Src LGConfig ??
        return MMPAPI_Error( MMPAPI_Error::ERR_SET_SOURCE_REP_GROUP );
    }

    if ( iTgtHostId != INVALID_HOST_ID )
    {
        //
        //3.3.1 Journal directory MUST exist on TARGET server.  make sure it does.
        char szJournalPath[_MAX_PATH];
		// Windows journal
		if (strstr(repgrp->m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
		{
			_ASSERT( (repgrp->m_strJournalDirectory.at(0) >= 'a' && repgrp->m_strJournalDirectory.at(0) <= 'z') ||
				     (repgrp->m_strJournalDirectory.at(0) >= 'A' && repgrp->m_strJournalDirectory.at(0) <= 'Z') );
			_ASSERT( repgrp->m_strJournalDirectory.at(1) == ':' );
		}
		if (strstr(repgrp->m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
		{
			sprintf(szJournalPath,"md \"%s\" ",repgrp->m_strJournalDirectory.c_str()); 

	        code = tdmfCmd(iTgtHostId, MMP_MNGT_TDMF_CMD_OS_CMD_EXE, szJournalPath);
		    if ( code.IsOK() == false )
			{
				CString msg;
				msg.Format("***Error while communicating with Target Server 0x%08x. (%s)",iTgtHostId,szJournalPath);
				Trace( ONLOG_DEBUG, ">> addRepGroup : %s",(LPCTSTR)msg);
				code.setMessage(msg);
				free(pCfgData);
				return code;
			}
		}

        //3.3.2 From the RepGroup definition, create the .cfg file content. 
        //pCfgData = 0;
        //
        localReplacePYYYbySYYY(pCfgData,iRepGroupNum);
        //
        //3.3.3 Send new config. to TARGET
        r = mmp_setTdmfAgentLGConfig( m_hMMP, 
                                      _itoa( iTgtHostId, buf, 16 ),
                                      true, //bIsHostId,
                                      iRepGroupNum,
                                      false, //bSourceSrvr,
                                      pCfgData,
                                      uiDataSize
                                    );_ASSERT(r==0);
        if ( r != 0 )
        {
            Trace( ONLOG_DEBUG, ">> addRepGroup : ***Error, while setting a the RepGroup configuration on Server 0x%08x ***",iTgtHostId);
            free(pCfgData);
            //todo :
            //restore previous Src LGConfig ??
            return MMPAPI_Error( MMPAPI_Error::ERR_SET_TARGET_REP_GROUP );
        }
    }

    free(pCfgData);

    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error MMP_API::SendCfgFileToAgent( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId )
{
    MMPAPI_Error code;

	collectorTXRepGroup(repgrp, iSrcHostId, iTgtHostId, false);

	// Symmetric configuration
	if (repgrp->m_bSymmetric)
	{
		collectorTXRepGroup(repgrp, iTgtHostId, iSrcHostId, true);

		// Send .off file
		CScriptServer ScriptServer;
		ScriptServer.m_pServer = repgrp->m_pParent;
		CString cstrTmp;
		cstrTmp.Format("s%03d.off", repgrp->m_nSymmetricGroupNumber);
		ScriptServer.m_strFileName = cstrTmp;
		ScriptServer.m_strExtension = "";
		ScriptServer.m_strContent = " ";
		code = SendScriptServerFileToAgent(&ScriptServer);
	}

	return code;
}

MMPAPI_Error MMP_API::SendEmptyCfgFileToAgent( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId )
{
    MMPAPI_Error code;

	// Remove existing cfg files
	int  r;
	char buf[16];
	//
	// SOURCE server        
	//
	r = mmp_setTdmfAgentLGConfig( m_hMMP, 
					 			  _itoa( iSrcHostId, buf, 16 ),
								  true, //bIsHostId,
								  (short)repgrp->m_nGroupNumber,
								  true, //bPrimary,
								  NULL, //pCfgData,
								  0     //uiDataSize
								  );_ASSERT(r==0);
	if ( r != 0 )
	{
		Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, %d while deleting RepGroup %d from SOURCE Agent 0x%08x ***", r, repgrp->m_nGroupNumber, iSrcHostId);
		//todo :
		//restore previous Src LGConfig ??
		code = MMPAPI_Error( MMPAPI_Error::ERR_DELETING_SOURCE_REP_GROUP );
	}
	
	//
	// TARGET server        
	//
	if (iTgtHostId != INVALID_HOST_ID) 
	{
		r = mmp_setTdmfAgentLGConfig( m_hMMP, 
									  _itoa( iTgtHostId, buf, 16 ),
									  true, //bIsHostId,
									  (short)repgrp->m_nGroupNumber,
									  false,//bPrimary,
									  NULL, //pCfgData,
									  0     //uiDataSize
									  );_ASSERT(r==0);
		if ( r != 0 )
		{
			Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, %d while deleting RepGroup %d from TARGET Agent 0x%08x ***", r, repgrp->m_nGroupNumber, iTgtHostId);
			//todo :
			//restore previous Tgt LGConfig ??
			//restore previous Src LGConfig ??
			code = MMPAPI_Error( MMPAPI_Error::ERR_DELETING_TARGET_REP_GROUP );
		}
	} // if ( iTgtHostId != INVALID_HOST_ID )

	if (repgrp->m_bSymmetric)
	{
		RemoveSymmetricFiles( repgrp, iTgtHostId, iSrcHostId );
	}

	return code;
}

MMPAPI_Error MMP_API::RemoveSymmetricFiles( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId )
{
    MMPAPI_Error code;

	int  r;
	char buf[16];

	// Remove symmetric group's .cfg files
	r = mmp_setTdmfAgentLGConfig(m_hMMP, 
								 _itoa( iSrcHostId, buf, 16 ),
								 true, //bIsHostId,
								 (short)repgrp->m_nSymmetricGroupNumber,
								 true, //bPrimary,
								 NULL, //pCfgData,
								 0     //uiDataSize
								 );_ASSERT(r==0);
	if ( r != 0 )
	{
		Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, %d while deleting Symmetric RepGroup %d from SOURCE Agent 0x%08x ***", r, repgrp->m_nSymmetricGroupNumber, iSrcHostId);
		code = MMPAPI_Error( MMPAPI_Error::ERR_DELETING_SOURCE_REP_GROUP );
	}
					
	//
	// TARGET server        
	//
	if (iTgtHostId != INVALID_HOST_ID)
	{
		r = mmp_setTdmfAgentLGConfig( m_hMMP, 
									 _itoa( iTgtHostId, buf, 16 ),
									 true, //bIsHostId,
									 (short)repgrp->m_nSymmetricGroupNumber,
									 false,//bPrimary,
									 NULL, //pCfgData,
									 0     //uiDataSize
									 );_ASSERT(r==0);
		if ( r != 0 )
		{
			Trace( ONLOG_EXTENDED, ">> delRepGroup : ***Error, %d while deleting RepGroup %d from TARGET Agent 0x%08x ***", r, repgrp->m_nSymmetricGroupNumber, iTgtHostId );
			code = MMPAPI_Error( MMPAPI_Error::ERR_DELETING_TARGET_REP_GROUP );
		}
	} // if ( iTgtHostId != INVALID_HOST_ID )

	// Remove symmetric group's .off file
	CScriptServer ScriptServer;
	ScriptServer.m_pServer = repgrp->m_pParent;
	CString cstrTmp;
	cstrTmp.Format("s%03d.off", repgrp->m_nSymmetricGroupNumber);
	ScriptServer.m_strFileName = cstrTmp;
	ScriptServer.m_strExtension = "";
	ScriptServer.m_strContent = "";
	code = SendScriptServerFileToAgent(&ScriptServer);

	return code;
}

MMPAPI_Error MMP_API::SendScriptServerFileToAgent( CScriptServer* pScriptServer )
{
    MMPAPI_Error code;
    char szAgentId[16];

    if(pScriptServer == NULL)
    {
        code.setCode( MMPAPI_Error::ERR_UNKNOWN_SOURCE_SERVER );
        return code;
    }
  
    code = createMMPHandle();
    if ( code.IsOK() == false )
        return code;

    
    int r = mmp_setTdmfServerFile(m_hMMP, 
                              itoa(pScriptServer->m_pServer->m_nHostID,szAgentId,16),
                              true,
                               pScriptServer->m_strFileName.c_str(),
                               pScriptServer->m_strContent.c_str(),
                               pScriptServer->m_strContent.size(),
                               MMP_MNGT_FILETYPE_TDMF_BAT );
    code.setFromMMPCode(r);
    if ( false == code.IsOK() )
    {
        CString strMsg;
     
		if(pScriptServer->m_strContent.size() > 0)
		{
			strMsg.Format("The filename '%s' has not been sent to the server '%s' .\n\nPlease, verify that the collector is working properly.", 
			pScriptServer->m_strFileName.c_str(),
			pScriptServer->m_pServer->m_strName.c_str());
		}
		else
		{
			strMsg.Format("The filename '%s' is not removed from the server '%s'.\n\nPlease, verify that the collector is working properly.", 
			pScriptServer->m_strFileName.c_str(),
			pScriptServer->m_pServer->m_strName.c_str());
		}
      
        AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
 
        code = MMPAPI_Error::ERR_UNKNOWN_SENDING_SCRIPT_SERVER_FILE_TO_AGENT;
        return code;
    }
    
    return MMPAPI_Error( MMPAPI_Error::OK );
 
    
}

static void localReplacePYYYbySYYY(char * pCfgData, int iRepGroupNum)
{
    char szPnum[8];
    char *pWk;

    sprintf(szPnum,"p%03d",iRepGroupNum);

    pWk = pCfgData;
    while( pWk = strstr(pWk,szPnum) )
    {   //in-place modification of 'pYYY' for 'sYYY'
        *pWk = 's';
        pWk++;
    }

    //just in case 'p' was 'P' instead ...
    szPnum[0] = 'P';
    pWk = pCfgData;
    while( pWk = strstr(pWk,szPnum) )
    {   //in-place modification of 'PYYY' for 'sYYY'
        *pWk = 's';
        pWk++;
    }
}

MMPAPI_Error MMP_API::SetTunables( CReplicationGroup& repGroup )
{
	std::ostringstream oss;
	oss << "-g" << repGroup.m_nGroupNumber;

	// Chunk Delay
	oss << " CHUNKDELAY=" << repGroup.m_nChunkDelay;
	// Chunk Size
	oss << " CHUNKSIZE=" << repGroup.m_nChunkSizeKB;

	if (strstr(repGroup.m_pParent->m_strOSType.c_str(),"Windows") != 0 ||
		strstr(repGroup.m_pParent->m_strOSType.c_str(),"windows") != 0 ||
		strstr(repGroup.m_pParent->m_strOSType.c_str(),"WINDOWS") != 0)
	{
		// Compression
		oss << " COMPRESSION=" << (repGroup.m_bEnableCompression ? 1 : 0);
		// Refresh Timeout
		oss << " REFRESHTIMEOUT=" << (repGroup.m_bRefreshNeverTimeout ? -1 : repGroup.m_nRefreshTimeoutInterval);
		// Journal-less
		oss << " JOURNAL=" << (repGroup.m_bJournalLess ? "OFF" : "ON");
	}
	else  // Unix tunable
	{
		// Net Max Kbps
		oss << " NETMAXKBPS=";
		if (repGroup.m_bNetThreshold)
		{
		  oss << repGroup.m_nNetMaxKbps;
		}
		else
		{
			oss << -1;
		}
		// Stat Interval
		oss << " STATINTERVAL=" << repGroup.m_nStatInterval;
		// Max Stat File Size
		oss << " MAXSTATFILESIZE=" << repGroup.m_nMaxFileStatSize;
		// Compression
		oss << " COMPRESSION=" << (repGroup.m_bEnableCompression ? "ON" : "OFF");
		// Trace Throttle
		//oss << " TRACETHROTTLE=N";
	}
	// Sync Mode
	oss << " SYNCMODE=" << (repGroup.m_bSync ? "ON" : "OFF");
	if (repGroup.m_bSync)
	{
		// Sync Mode Depth
		oss << " SYNCMODEDEPTH=" <<  repGroup.m_nSyncDepth;
		// Sync Mode Timeout
		oss << " SYNCMODETIMEOUT=" << repGroup.m_nSyncTimeout;
	}
	
	return tdmfCmd(repGroup.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_SET, oss.str().c_str());
}

MMPAPI_Error MMP_API::GetTunables( CReplicationGroup& repGroup, std::string& strOutput )
{
    MMPAPI_Error code;

	CString cstrCmdParams;
	cstrCmdParams.Format("-g%03d", repGroup.m_nGroupNumber);
	code = tdmfCmd(repGroup.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_SET, cstrCmdParams);

	if (code.IsOK())
	{
		strOutput = m_strTdmfCmdOutput;
	}

	return code;
}

BOOL MMP_API::ParseTunables(std::string& strTunable, TdmfTunables* pTunables)
{
	////////////////////////////////////
	// Parse tunable buffer
	std::string::size_type pos;

	// Chunk Delay
	if ((pos = strTunable.find("CHUNKDELAY")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("CHUNKDELAY") + 1);
		pTunables->nChunkDelay = atoi(strValue.c_str());
	}
	// Chunk Size
	if ((pos = strTunable.find("CHUNKSIZE")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("CHUNKSIZE") + 1);
		pTunables->nChunkSize = atoi(strValue.c_str());
	}
	// Compression
	if ((pos = strTunable.find("COMPRESSION")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("COMPRESSION") + 1);
		
		// TODO On/Off or N/Y
		pTunables->bCompression = (atoi(strValue.c_str()) == 1) ? true : false;
	}
	// Sync Mode
	if ((pos = strTunable.find("SYNCMODE")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("SYNCMODE") + 1);		
		pTunables->bSync = (strValue.c_str() == "ON") ? true : false;
	}
	// Sync Mode Depth
	if ((pos = strTunable.find("SYNCMODEDEPTH")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("SYNCMODEDEPTH") + 1);
		pTunables->nSyncDepth = atoi(strValue.c_str());
	}
	// Sync Mode Timeout
	if ((pos = strTunable.find("SYNCMODETIMEOUT")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("SYNCMODETIMEOUT") + 1);
		pTunables->nSyncTimeout = atoi(strValue.c_str());
	}
	// Refresh Timeout
	if ((pos = strTunable.find("REFRESHTIMEOUT")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("REFRESHTIMEOUT") + 1);
		pTunables->nRefreshTimeout = atoi(strValue.c_str());
	}
	// Net Max Kbps
	if ((pos = strTunable.find("NETMAXKBPS")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("NETMAXKBPS") + 1);
		pTunables->nNetMaxKbps = atoi(strValue.c_str());
	}
	// Stat Interval
	if ((pos = strTunable.find("STATINTERVAL")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("STATINTERVAL") + 1);
		pTunables->nStatInterval = atoi(strValue.c_str());
	}
	// Max Stat File Size
	if ((pos = strTunable.find("MAXSTATFILESIZE")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("MAXSTATFILESIZE") + 1);
		pTunables->nMaxStatFileSize = atoi(strValue.c_str());
	}
	// Journal-less operation
	if ((pos = strTunable.find("JOURNAL")) != std::string::npos)
	{
		std::string strValue = strTunable.substr(pos + strlen("JOURNAL") + 1);
		while ((strValue.size() > 0) && (strValue.at(0) == ' '))
		{
			strValue.erase(0, 1);
		}
		pTunables->bJournalLess = ((strValue.compare(0, 2, "ON") == 0) || (strValue.compare(0, 1, "1") == 0)) ? false : true;
	}

	return TRUE;
}

BOOL MMP_API::IsTunableModified( CReplicationGroup& repgrp)
{
	BOOL bModified = TRUE;

#if 0
	std::string strTunable;
    MMPAPI_Error code = GetTunables(repgrp, strTunable);

	if (code.IsOK())
	{

		if (strstr(repgrp.m_pParent->m_strOSType.c_str(),"Windows") != 0 ||
			strstr(repgrp.m_pParent->m_strOSType.c_str(),"windows") != 0 ||
			strstr(repgrp.m_pParent->m_strOSType.c_str(),"WINDOWS") != 0)
		{
		}
		else  // Unix tunable
		{
		}
	}
#endif

    return bModified;
}

MMPAPI_Error MMP_API::SaveTunables( CReplicationGroup& repGroup, bool bUseTransaction )
{
    MMPAPI_Error code;

    CServer *psrcAgent = repGroup.m_pParent;

#ifdef _DEBUG
    _ASSERT( !IsBadWritePtr(psrcAgent,sizeof(CServer)) );
#endif

	if ((bUseTransaction == false) || m_db->mpRecGrp->BeginTrans())
	{
		try
		{
			m_db->mpRecSrvInf->FtdPos(psrcAgent->m_nHostID);
			m_db->mpRecGrp->FtdLock();

			CString cszTmp;
			//build a SQL WHERE clause
			cszTmp.Format(  " %s = %d "
							" AND %s = %d "
							, FsTdmfRecLgGrp::smszFldLgGrpId
							, repGroup.m_nGroupNumber
							, FsTdmfRecLgGrp::smszFldSrcFk
							, psrcAgent->m_iDbSrvId);

			if ( m_db->mpRecGrp->FtdFirst(cszTmp) )
			{   //record found, so RepGroup do exist.
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldChunkDelay,      (int)repGroup.m_nChunkDelay ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldChunkSize,       (int)repGroup.m_nChunkSizeKB ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldEnableCompression, (int)repGroup.m_bEnableCompression ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSyncMode,        (int)(repGroup.m_bSync != 0 ? 1 : 0) ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSyncModeDepth,   (int)repGroup.m_nSyncDepth ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSyncModeTimeOut, (int)repGroup.m_nSyncTimeout ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldRefreshNeverTimeOut,  (int)(repGroup.m_bRefreshNeverTimeout != 0 ? 1 : 0) ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldRefreshTimeOut,  (int)repGroup.m_nRefreshTimeoutInterval ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldMaxFileStatSize,  (int)repGroup.m_nMaxFileStatSize ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNetUsageThreshold,  (int)repGroup.m_bNetThreshold))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNetUsageValue,  (int)repGroup.m_nNetMaxKbps ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldUpdateInterval,  (int)repGroup.m_nStatInterval ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldJournalLess,  (int)repGroup.m_bJournalLess ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}

				 if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPrimaryDHCPNameUsed,  (int)repGroup.m_bPrimaryDHCPAdressUsed ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}

				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPrimaryEditedIPUsed,  (int)repGroup.m_bPrimaryEditedIPUsed ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				unsigned long ip = 0;
				if (repGroup.m_strPrimaryEditedIP.length() > 0)
				{
					ipstring_to_ip((char*)repGroup.m_strPrimaryEditedIP.c_str(),&ip);
				}

				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPrimaryEditedIP,  (int)ip))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}

				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtDHCPNameUsed,  (int)repGroup.m_bTargetDHCPAdressUsed ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}

				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtEditedIPUsed,  (int)repGroup.m_bTargetEditedIPUsed))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}
				if (repGroup.m_pServerTarget != NULL)
				{
					ip = 0;
					if (repGroup.m_strTargetEditedIP.length() > 0)
					{
						ipstring_to_ip((char*)repGroup.m_strTargetEditedIP.c_str(),&ip);
					}
					if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtEditedIP,  (int)ip))
					{
						throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
					}
				}

				// Commit changes
				if (bUseTransaction && (m_db->mpRecGrp->CommitTrans() == FALSE))
				{
					code = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
				}

				// Copy info to target group
				CReplicationGroup* repGroupTarget = repGroup.GetTargetGroup();
				if (repGroupTarget != NULL)
				{
					repGroupTarget->Copy(&repGroup);
				}
			}
		}
		catch(MMPAPI_Error eErr)
		{
			code = eErr;

			if (bUseTransaction)
			{
			// Rollback (save all or nothing)
			m_db->mpRecGrp->Rollback();
		}
		}

		//must unlock in reverse order
		m_db->mpRecGrp->FtdUnlock();
	}
	else
	{
		code = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

	return code;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct __TdmfGuiMessage  // Max of 64 bytes per msg
{
	DWORD nID;
	DWORD nType;
	BYTE  rgByte[56]; // 4 + 4 + 56 = Max of 64 bytes per msg
} TdmfGuiMsg;

MMPAPI_Error MMP_API::SendTdmfGuiMessage(DWORD nType, BYTE* pByte, UINT nLength)
{
	// Pack a message
	TdmfGuiMsg Msg;
	Msg.nID          = GetCurrentProcessId();
	Msg.nType        = nType;
	memcpy(Msg.rgByte, pByte, nLength);

	// Send it to all gui (via collector)
	mmp_mngt_sendGuiMsg(m_hMMP, (BYTE*)&Msg, sizeof(Msg));

    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error MMP_API::RetrieveTdmfGuiMessage(mmp_TdmfGuiMsg* pMsg,
											 DWORD*          pnID,
											 DWORD*          piType,
											 BYTE**          ppByte)
{
	TdmfGuiMsg* pGuiMsg = (TdmfGuiMsg*)pMsg->szMsg;

	*pnID   = pGuiMsg->nID;
	*piType = pGuiMsg->nType;
	*ppByte = pGuiMsg->rgByte;
	
    return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error MMP_API::SendTdmfObjectChangeMessage(int iModule, int iOp, int iDbDomain, int iDbSrvId, int iGroupNumber)
{
	TdmfGuiObjectChangeMessage Msg;

	Msg.iModule      = iModule;
	Msg.iOp          = iOp;
	Msg.iDbDomain    = iDbDomain;
	Msg.iDbSrvId     = iDbSrvId;
	Msg.iGroupNumber = iGroupNumber;

	SendTdmfGuiMessage(TDMFGUI_OBJECT_CHANGE, (BYTE*)&Msg, sizeof(Msg));

	return MMPAPI_Error( MMPAPI_Error::OK );
}

MMPAPI_Error MMP_API::SendTdmfTextMessage(char* pszMsg)
{
	SendTdmfGuiMessage(TDMFGUI_TEXT_MESSAGE, (BYTE*)pszMsg, strlen(pszMsg)+1);

	return MMPAPI_Error( MMPAPI_Error::OK );
}

std::string MMP_API::GetCmdName(enum tdmf_commands eCmd)
{
	std::string strCmdName;

	switch (eCmd)
	{
	case MMP_MNGT_TDMF_CMD_START:
		strCmdName = "START";
		break;
	case MMP_MNGT_TDMF_CMD_STOP:
		strCmdName = "STOP";
		break;
	case MMP_MNGT_TDMF_CMD_INIT:
		strCmdName = "INIT";
		break;
	case MMP_MNGT_TDMF_CMD_OVERRIDE:
		strCmdName = "OVERRIDE";
		break;
	case MMP_MNGT_TDMF_CMD_INFO:
		strCmdName = "INFO";
		break;
	case MMP_MNGT_TDMF_CMD_HOSTINFO:
		strCmdName = "HOSTINFO";
		break;
	case MMP_MNGT_TDMF_CMD_LICINFO:
		strCmdName = "LICINFO";
		break;
	case MMP_MNGT_TDMF_CMD_RECO:
		strCmdName = "RECO";
		break;
	case MMP_MNGT_TDMF_CMD_SET:
		strCmdName = "SET";
		break;
	case MMP_MNGT_TDMF_CMD_LAUNCH_PMD:
		strCmdName = "LAUNCH_PMD";
		break;
	case MMP_MNGT_TDMF_CMD_LAUNCH_REFRESH:
		strCmdName = "LAUNCH_REFRESH";
		break;
	case MMP_MNGT_TDMF_CMD_LAUNCH_BACKFRESH:
		strCmdName = "LAUNCH_BACKFRESH";
		break;
	case MMP_MNGT_TDMF_CMD_KILL_PMD:
		strCmdName = "KILL_PMD";
		break;
	case MMP_MNGT_TDMF_CMD_KILL_RMD:
		strCmdName = "KILL_RMD";
		break;
	case MMP_MNGT_TDMF_CMD_KILL_REFRESH:
		strCmdName = "KILL_REFRESH";
		break;
	case MMP_MNGT_TDMF_CMD_KILL_BACKFRESH:
		strCmdName = "KILL_BACKFRESH";
		break;
	case MMP_MNGT_TDMF_CMD_CHECKPOINT:
		strCmdName = "CHECKPOINT";
		break;
	case MMP_MNGT_TDMF_CMD_OS_CMD_EXE:
		strCmdName = "OS_CMD_EXE";
		break;
	case MMP_MNGT_TDMF_CMD_TESTTDMF:
		strCmdName = "TESTTDMF";
		break;
	case MMP_MNGT_TDMF_CMD_TRACE:
		strCmdName = "TRACE";
		break;
	case MMP_MNGT_TDMF_CMD_HANDLE:
		strCmdName = "ANALYZER"; // HANDLE
		break;
	case MMP_MNGT_TDMF_CMD_PANALYZE:
		strCmdName = "PANALYZE";
		break;
	default:
		{
			char szBuf[32];
			strCmdName = itoa(eCmd, szBuf, 10);
		}
		break;
	}
	
	return strCmdName;
}

MMPAPI_Error MMP_API::delSymmetricGroup(CReplicationGroup & repGroup)
{
    MMPAPI_Error code;

    CServer *psrcAgent = repGroup.m_pParent;
    CServer *ptgtAgent = repGroup.m_pServerTarget;

	// Stop Group
	CString cstrOptions;
	cstrOptions.Format("-g%03d", repGroup.m_nSymmetricGroupNumber);
	code = tdmfCmd(ptgtAgent->m_nHostID, MMP_MNGT_TDMF_CMD_KILL_PMD, cstrOptions);
	if (code.IsOK())
	{
		code = tdmfCmd(ptgtAgent->m_nHostID, MMP_MNGT_TDMF_CMD_STOP, cstrOptions);
	}

	// Save to DB
	if (m_db->mpRecGrp->BeginTrans())
	{
		try
		{
			m_db->mpRecSrvInf->FtdPos(psrcAgent->m_nHostID);
			m_db->mpRecGrp->FtdLock();

			CString cszTmp;
			//build a SQL WHERE clause
			cszTmp.Format(  " %s = %d "
							" AND %s = %d "
							, FsTdmfRecLgGrp::smszFldLgGrpId
							, repGroup.m_nGroupNumber
							, FsTdmfRecLgGrp::smszFldSrcFk
							, psrcAgent->m_iDbSrvId);

			if ( m_db->mpRecGrp->FtdFirst(cszTmp) )
			{   //record found, so RepGroup does exist.
				if (!m_db->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldSymmetric, (int)0 ))
				{
					throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
				}

				// Commit changes
				if (m_db->mpRecGrp->CommitTrans() == FALSE)
				{
					code = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
				}

				// Copy info to target group
				CReplicationGroup* repGroupTarget = repGroup.GetTargetGroup();
				if (repGroupTarget != NULL)
				{
					repGroupTarget->Copy(&repGroup);
				}
			}
		}
		catch(MMPAPI_Error eErr)
		{
			code = eErr;

			// Rollback (save all or nothing)
			m_db->mpRecGrp->Rollback();
		}

		//must unlock in reverse order
		m_db->mpRecGrp->FtdUnlock();
	}
	else
	{
		code = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

	// Remove cfg files and .off files (source/target) 
	code = RemoveSymmetricFiles( &repGroup, ptgtAgent ? ptgtAgent->m_nHostID : INVALID_HOST_ID, psrcAgent->m_nHostID );

	// Restore default values
	repGroup.m_bSymmetric = FALSE;
	repGroup.m_nSymmetricGroupNumber = 0;
	repGroup.m_bSymmetricNormallyStarted = FALSE;
	repGroup.m_nFailoverInitialState = 0;
	repGroup.m_nSymmetricConnectionState = FTD_M_UNDEF;
	repGroup.m_nSymmetricMode = FTD_M_UNDEF;
	repGroup.m_strSymmetricJournalDirectory = "";
	repGroup.m_strSymmetricPStoreFile = "";

	if (repGroup.GetTargetGroup())
	{
		repGroup.GetTargetGroup()->m_bSymmetric = FALSE;
		repGroup.GetTargetGroup()->m_nSymmetricGroupNumber = 0;
		repGroup.GetTargetGroup()->m_bSymmetricNormallyStarted = FALSE;
		repGroup.GetTargetGroup()->m_nFailoverInitialState = 0;
		repGroup.GetTargetGroup()->m_nSymmetricConnectionState = FTD_M_UNDEF;
		repGroup.GetTargetGroup()->m_nSymmetricMode = FTD_M_UNDEF;
		repGroup.GetTargetGroup()->m_strSymmetricJournalDirectory = "";
		repGroup.GetTargetGroup()->m_strSymmetricPStoreFile = "";
	}

	// Advise other GUI
	SendTdmfObjectChangeMessage(TDMF_GROUP, TDMF_MODIFY, psrcAgent->m_iDbDomainFk, psrcAgent->m_iDbSrvId, repGroup.m_nGroupNumber);

	return code;
}
