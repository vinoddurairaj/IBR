// Server.cpp : Implementation of CServer
#include "stdafx.h"
#include "System.h"
#include "Server.h"
#include "DeviceList.h"
#include <set>

#include "FsTdmfDb.h"
#include "FsTdmfRecSrvScript.h"

#include "mmp_API.h"

extern "C"
{
#include "iputil.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServer

CReplicationGroup& CServer::AddNewReplicationGroup()
{
	// Create a new Replication Group
	CReplicationGroup RG;
	RG.m_pParent = this;
	if (!(strstr(m_strOSType.c_str(),"Windows") != 0 ||
		  strstr(m_strOSType.c_str(),"windows") != 0 ||
		  strstr(m_strOSType.c_str(),"WINDOWS") != 0))
	{
		RG.m_nChunkSizeKB = 2048;
	}
	m_listReplicationGroup.push_back(RG);

	return m_listReplicationGroup.back();
}

CScriptServer& CServer::AddNewScriptServerFile()
{
  	// Create a new Script Server Object
	CScriptServer SS;
	SS.m_pServer = this;
    SS.m_iDbSrvId = this->m_iDbSrvId;
    SS.m_bNew     = true;
	m_listScriptServerFile.push_back(SS);

	return m_listScriptServerFile.back();
}

FsTdmfDb* CServer::GetDB() 
{ 
    return m_pParent->GetDB(); 
}

MMP_API* CServer::GetMMP() 
{ 
    return m_pParent->GetMMP(); 
}

//Initialize object from provided TDMF database record
bool CServer::Initialize(FsTdmfRecSrvInf *pRec, bool bRecurse)
{
    //init data members from DB record

    m_iDbSrvId              = atoi( pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvId ) );
	m_iDbDomainFk           = atoi( pRec->FtdSel( FsTdmfRecSrvInf::smszFldDomFk ) );

	m_strName               = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldName );
	m_eState                = (enum ElementStates)atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldState ) );
	m_nHostID               = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldHostId ) );

	// Read all IP addresses
    {
		unsigned long   ip;
		char            buf[32];
		ip              = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp1 ) );
		ip_to_ipstring(ip, buf );
		m_vecstrIPAddress.push_back(buf);
    }
	{
		unsigned long   ip;
		char            buf[32];
		ip              = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp2 ) );
		if (ip > 0)
		{
			ip_to_ipstring(ip, buf );
			m_vecstrIPAddress.push_back(buf);
		}
    }
    {
		unsigned long   ip;
		char            buf[32];
		ip              = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp3 ) );
		if (ip > 0)
		{
			ip_to_ipstring(ip, buf );
			m_vecstrIPAddress.push_back(buf);
		}
    }
    {
		unsigned long   ip;
		char            buf[32];
		ip              = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp4 ) );
		if (ip > 0)
		{
			ip_to_ipstring(ip, buf );
			m_vecstrIPAddress.push_back(buf);
		}
    }

	m_strJournalDirectory   = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldDefaultJournalVol );
	m_strPStoreFile         = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldDefaultPStoreFile );
	m_nPort                 = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldIpPort ) );
	m_nReplicationPort      = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldReplicationPort ) );
    m_nBABSizeReq           = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldBabSizeReq ) );
    m_nBABSizeAct           = 0;//atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldBabSizeAct ) );
	m_nTCPWndSize           = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldTcpWinSize ) );
	m_strOSType             = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldOsType );
	m_strOSVersion          = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldOsVersion );
	m_strAgentVersion       = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldTdmfVersion );
	m_strKey                = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldKey );
	m_nNbrCPU               = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldNumberOfCpu ) );
	m_nRAMSizeKB            = atol( pRec->FtdSel( FsTdmfRecSrvInf::smszFldRamSize ) );
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp2;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp3;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp4;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldKeyOld;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldRtrIp1;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldRtrIp2;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldRtrIp3;
	//= (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldRtrIp4;
	m_strDescription        = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldNotes );
    m_strKeyExpirationDate  = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvInf::smszFldKeyExpire );
	m_strKeyExpirationDate.resize(19); // Chop last ".000"

    InitializeScriptServers( pRec );

    if ( bRecurse )
    {   
        return InitializeGroups(bRecurse,pRec);
    }

    return true;
}


bool CServer::InitializeGroups(bool bRecurseForPairs, FsTdmfRecSrvInf *pRecSrv)
{
    FsTdmfRecLgGrp  *pRecGrp = GetDB()->mpRecGrp;

    pRecSrv->FtdLock();
    //if server ptr not provided, get one from db
    if ( pRecSrv == 0 )
    {
        pRecSrv = GetDB()->mpRecSrvInf;
        _ASSERT(m_nHostID != 0);//Initialize(FsTdmfRecSrvInf *pRec, bool bRecurse) must be called prior to this method !
        if ( !pRecSrv->FtdPos( m_nHostID ) )
        {
            _ASSERT(0);//?? corrupted m_nHostID ? corrupted DB ?
            pRecSrv->FtdUnlock();
            return false;
        }
    }

    m_listReplicationGroup.clear();

    //
    //fill the list of CReplicationGroup owned by this Server
    //
    //FsTdmfRecSrvInf::smszFldSrvId already available in m_iDbSrvId
    //int SrcFk = atoi( pRec->FtdSel( FsTdmfRecSrvInf::smszFldSrvId ) );
    int SrcFk = m_iDbSrvId;

    CString cszWhere;
    cszWhere.Format( " %s = %d ", FsTdmfRecLgGrp::smszFldSrcFk, SrcFk );
    BOOL bLoop = pRecGrp->FtdFirst( cszWhere );
    while ( bLoop )
    {
        CReplicationGroup & newGroup = AddNewReplicationGroup();//group added to list.
        newGroup.Initialize(pRecGrp,bRecurseForPairs);//init from t_LogicalGroup record
        bLoop = pRecGrp->FtdNext();
    }

	// We have lately introduce the concept of having the target group
	// listed under the server.
	// Fill the list of CReplicationGroup where this server is a target
	{
		CString cszWhere;
		cszWhere.Format( " %s = %d ",
						 FsTdmfRecLgGrp::smszFldTgtFk, SrcFk );

		BOOL bLoop = pRecGrp->FtdFirst( cszWhere );
		while ( bLoop )
		{
			// Src server of the target group become the target server of the group
			// (Change target to remote)
			int iDbSrcFk = atoi( pRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk ) );

			CReplicationGroup & newGroup = AddNewReplicationGroup();//group added to list.
			newGroup.Initialize(pRecGrp, bRecurseForPairs);//init from t_LogicalGroup record

			newGroup.m_iDbTgtFk = iDbSrcFk;
			newGroup.m_nType = CReplicationGroup::GT_TARGET;

			bLoop = pRecGrp->FtdNext();
		}
	}

    pRecSrv->FtdUnlock();

	// Sort Group by number
	m_listReplicationGroup.sort();

    return true;
}

bool CServer::InitializeScriptServers( FsTdmfRecSrvInf *pRecSrv )
{
   FsTdmfRecSrvScript  *pRecScriptServer = pRecSrv->mpDb->mpRecScriptSrv;

    pRecScriptServer->FtdLock();
    //if server ptr not provided, get one from db
    if ( pRecSrv == 0 )
    {
        pRecSrv = GetDB()->mpRecSrvInf;
        _ASSERT(m_nHostID != 0);//Initialize(FsTdmfRecSrvInf *pRec, bool bRecurse) must be called prior to this method !
        if ( !pRecSrv->FtdPos( m_nHostID ) )
        {
            _ASSERT(0);//?? corrupted m_nHostID ? corrupted DB ?
            pRecSrv->FtdUnlock();
            return false;
        }
    }

    m_listScriptServerFile.clear();

    //
    //fill the list of CScriptServer owned by this Server
    //
    //FsTdmfRecSrvInf::smszFldSrvId already available in m_iDbSrvId
    int SrcFk = m_iDbSrvId;

    CString cszWhere;
    cszWhere.Format( " %s = %d ", FsTdmfRecSrvScript::smszFldSrvId, SrcFk );
    BOOL bLoop = pRecScriptServer->FtdFirst( cszWhere );
    while ( bLoop )
    {
        CScriptServer& newScriptServer = AddNewScriptServerFile();//ScriptServer  added to list.
        newScriptServer.Initialize(pRecScriptServer);
        bLoop = pRecScriptServer->FtdNext();
    }


    pRecScriptServer->FtdUnlock();

	// Sort Script Server by number
//	m_listScriptServerFile.sort();

    return true;
}

bool CServer::EnablePerformanceNotifications(BOOL bEnable)
{
	bool bRetVal = false;
	MMP_API* pMMP = GetMMP();

	if (bEnable == TRUE)
	{
		if (pMMP->requestNotification(m_listReplicationGroup, ACT_ADD, TYPE_FULL_STATS).IsOK())
		{
			bRetVal = true;
		}
	}
	else
	{
		if (pMMP->requestNotification(m_listReplicationGroup, ACT_DELETE, TYPE_FULL_STATS).IsOK())
		{
			bRetVal = true;
		}
	}

	return bRetVal;
}

long CServer::GetTargetReplicationGroupCount()
{
	FsTdmfRecLgGrp  *pRecGrp = GetDB()->mpRecGrp;

	std::ostringstream oss;
	oss << "Target_Fk = " << m_iDbSrvId;

	return pRecGrp->FtdCount(oss.str().c_str());
}

long CServer::GetScriptServerFileCount()
{

	long nCount = m_listScriptServerFile.size();

	return nCount;

}

long CServer::GetReplicationPairCount()
{
	long nPairCount = 0;

	std::list<CReplicationGroup>::iterator itRG;
	for (itRG = m_listReplicationGroup.begin(); itRG != m_listReplicationGroup.end(); itRG++)
	{
		nPairCount += itRG->m_listReplicationPair.size();
	}

	return nPairCount;
}


long CServer::GetTargetReplicationPairCount()
{
	FsTdmfRecLgGrp  *pRecGrp = GetDB()->mpRecGrp;

	return pRecGrp->FtdGetTargetReplicationPairCount(m_iDbSrvId);
}


// ardev 021128 v
CEvent* CServer::GetEventAt ( long nIndex )
{
	CEvent* pEvent = m_EventList.GetAt( nIndex );

	if ( pEvent == NULL )
	{
		CSystem* lpSystem = m_pParent->m_pParent;
		pEvent = m_EventList.ReadRangeFromDb( lpSystem, nIndex, m_iDbSrvId );
	}

	return pEvent;

} // CServer::GetEventAt ()


long CServer::GetEventCount ()
{
	CString cszWhere;
	cszWhere.Format ( " %s = %d ", FsTdmfRecAlert::smszFldSrcFk, m_iDbSrvId );

	return GetDB()->mpRecAlert->FtdCount( cszWhere );

} // CServer::GetEventCount ()


BOOL CServer::IsEventAt ( long nIndex )
{
	CEvent* pEvent = m_EventList.GetAt(nIndex);

	if ( pEvent == NULL )
	{
		return FALSE;
	}

	return TRUE;

} // CServer::GetEventAt ()
// ardev 021128 ^


CEvent* CServer::GetEventFirst()
{
	m_EventList.Reset();

	return NULL;
}

int CServer::MoveToDomain(long nKeyDomain, CServer** ppServer)
{
	MMPAPI_Error Err = MMPAPI_Error::OK;
	CServer* pServer;
	BOOL     bServerRemoved = FALSE;

	CDomain* pDomainSource = m_pParent;
	CSystem* pSystem = m_pParent->m_pParent;

	*ppServer = NULL;

	// Remove server from source domain
	std::list<CServer>::iterator itServer = pDomainSource->m_listServer.begin();
	while(itServer != pDomainSource->m_listServer.end())
	{
		if (itServer->GetKey() == GetKey())
		{
			// Keep the Server object... to be able to add it in another domain

			pServer = &*itServer;

			bServerRemoved = TRUE;
			break;
		}
		itServer++;
	}
		
	// Add it to destination domain
	if (bServerRemoved)
	{
		std::list<CDomain>::iterator it = pSystem->m_listDomain.begin();
		while(it != pSystem->m_listDomain.end())
		{
			if (it->GetKey() == nKeyDomain)
			{
				// Insert new server in new domain
				CServer& ServerNew = it->AddNewServer();
				// Copy old server in new server
				ServerNew = *pServer;
				// Set Parent ptr
				ServerNew.m_pParent = &*it;

				// Set Parent ptr for each replication group
				std::list<CReplicationGroup>::iterator itRG = ServerNew.m_listReplicationGroup.begin();
				while(itRG != ServerNew.m_listReplicationGroup.end())
				{
					itRG->m_pParent = &ServerNew;

					if (itRG->m_pServerTarget == pServer)
					{
						itRG->m_pServerTarget = &ServerNew;
					}

					itRG++;
				}

				// Save change to DB
				Err = ServerNew.GetMMP()->assignDomain(&ServerNew, &*it);

				*ppServer = &ServerNew;

				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("Server - Move '%s' to '%s' domain", ServerNew.m_strName.c_str(), it->m_strName.c_str());
				ServerNew.m_pParent->m_pParent->LogUserAction(cstrLogMsg);
			}
			it++;
		}
	}

	// Reset other RG's target server ptr
	if (bServerRemoved)
	{
		bool bModif = false;

		std::list<CDomain>::iterator itDomain = pSystem->m_listDomain.begin();
		while(itDomain != pSystem->m_listDomain.end())
		{
			std::list<CServer>::iterator it = itDomain->m_listServer.begin();
			while(it != itDomain->m_listServer.end())
			{
				std::list<CReplicationGroup>::iterator itRG = it->m_listReplicationGroup.begin();
				while(itRG != it->m_listReplicationGroup.end())
				{
					if (itRG->m_pServerTarget == pServer)
					{
						itRG->m_pServerTarget = *ppServer;

						bModif = true;
					}
					
					itRG++;
				}

				it++;
			}

			// Update domains state
			if (bModif)
			{
				itDomain->UpdateStatus();
			}

			itDomain++;
		}
	}

	// Remove from source domain
	if (bServerRemoved)
	{
		pDomainSource->m_listServer.erase(itServer);
	}

	return Err;
}

int CServer::SaveToDB()
{
	MMPAPI_Error Err = MMPAPI_Error::OK;

	MMP_API* mmp = GetMMP();

	FsTdmfRecSrvInf *pSvrTbl = GetDB()->mpRecSrvInf;

    pSvrTbl->FtdLock();

	// Find DB record
	if (pSvrTbl->BeginTrans())
	{
		try
		{
			if ( !pSvrTbl->FtdPos( m_nHostID ) )
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_FINDING_DB_RECORD);
			}

			if (m_bConnected)
			{
				// Get previous Port, BAB Size and Registration Key values
				long        nPortOld    = atol( pSvrTbl->FtdSel( FsTdmfRecSrvInf::smszFldIpPort ) );
				long        nBABSizeOld = atol( pSvrTbl->FtdSel( FsTdmfRecSrvInf::smszFldBabSizeReq ) );
				long        nTcpWinSizeOld = atol( pSvrTbl->FtdSel( FsTdmfRecSrvInf::smszFldTcpWinSize ) );
				std::string strKeyOld   = (LPCTSTR)pSvrTbl->FtdSel( FsTdmfRecSrvInf::smszFldKey );
				
				// Change key
				if (m_strKey != strKeyOld)
				{
					Err = mmp->setKey(this, m_strKey.c_str());
				}
				
				// If critical configuration values have changed (BAB Size, Port and TCP/IP wnd size)
				bool bIPPortChanged = (m_nPort != nPortOld);
				bool bBABSizeChanged = (m_nBABSizeReq != nBABSizeOld);
				if (bIPPortChanged || bBABSizeChanged || (m_nTCPWndSize != nTcpWinSizeOld))
				{
					// Advise collector
					Err = mmp->modifyCriticalServerCfgValues(this, bIPPortChanged || bBABSizeChanged, bIPPortChanged);
					if (!Err.IsOK())
					{
						throw Err;
					}
				}
			}

			// Save changes to db
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldState, m_eState ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldDefaultJournalVol, m_strJournalDirectory.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldDefaultPStoreFile, m_strPStoreFile.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort, m_nPort ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldReplicationPort, m_nReplicationPort ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldBabSizeReq, m_nBABSizeReq ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldTcpWinSize, m_nTCPWndSize ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldKey, m_strKey.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pSvrTbl->FtdUpd( FsTdmfRecSrvInf::smszFldNotes, m_strDescription.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			
			// Commit changes
			if (pSvrTbl->CommitTrans() == FALSE)
			{
				Err = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
			}
			else
			{
				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("Server - Edit Properties: '%s'", m_strName.c_str());
				m_pParent->m_pParent->LogUserAction(cstrLogMsg);

				// Advise other GUI
				GetMMP()->SendTdmfObjectChangeMessage(TDMF_SERVER, TDMF_MODIFY, m_iDbDomainFk, m_iDbSrvId, 0);
			}
		}
		catch(MMPAPI_Error eErr)
		{
			Err = eErr;

			// Rollback (save all or nothing)
			pSvrTbl->Rollback();
		}
	}
	else
	{
		Err = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

    pSvrTbl->FtdUnlock();

	return Err;
}

long CServer::LaunchCommand(enum tdmf_commands eCmd, char* pszOptions, char* pszLog, CString& cstrMsg)
{
    MMPAPI_Error Err = GetMMP()->tdmfCmd(m_nHostID, eCmd, pszOptions);
	cstrMsg = Err;

	if ((eCmd == MMP_MNGT_TDMF_CMD_START) && Err.IsOK())
	{
		std::list<CReplicationGroup>::iterator it;
		for (it = m_listReplicationGroup.begin(); it != m_listReplicationGroup.end(); it++)
		{
			if (GetMMP()->IsTunableModified(*it))
			{
				GetMMP()->SetTunables(*it);
			}
		}
	}

	// Log Command
	if ((pszLog != NULL) && (strlen(pszLog) > 0))
	{
		m_strCmdHistory += pszLog;
		m_strCmdHistory += ";";

		m_strLastCmdOutput = GetMMP()->getTdmfCmdOutput();
	}

	// Log
	CString cstrLogMsg;
	cstrLogMsg.Format("Server - Launch Command '%s %s' to '%s'", GetMMP()->GetCmdName(eCmd).c_str(), pszOptions, m_strName.c_str());
	m_pParent->m_pParent->LogUserAction(cstrLogMsg);

    return Err;
}

int CServer::GetDeviceList(CDeviceList** ppDeviceList,
						   long          nHostIDTarget,
						   CDeviceList** ppDeviceListTarget)

{
	MMPAPI_Error Err = MMPAPI_Error::OK;

	CDeviceList* pDeviceList = new CDeviceList;
	CDeviceList* pDeviceListTarget = NULL;

	if (ppDeviceListTarget != NULL)
	{
		pDeviceListTarget = new CDeviceList;
	}

	if ((pDeviceList != NULL) && ((ppDeviceListTarget == NULL) || (pDeviceListTarget != NULL)))
	{
		if (nHostIDTarget == 0)
		{
			bool bIsWindows = ( strstr(m_strOSType.c_str(),"Windows") != 0 ||
								strstr(m_strOSType.c_str(),"windows") != 0 ||
								strstr(m_strOSType.c_str(),"WINDOWS") != 0 );

			Err = GetMMP()->getDevices(m_nHostID, pDeviceList->m_listDevice, bIsWindows);
		}
		else
		{
			bool bIsWindows = ( strstr(m_strOSType.c_str(),"Windows") != 0 ||
								strstr(m_strOSType.c_str(),"windows") != 0 ||
								strstr(m_strOSType.c_str(),"WINDOWS") != 0 );

			Err = GetMMP()->getDevices(m_nHostID, bIsWindows, pDeviceList->m_listDevice,
									   nHostIDTarget, bIsWindows, pDeviceListTarget->m_listDevice);
		}
	}

	*ppDeviceList = pDeviceList;
	if (ppDeviceListTarget != NULL)
	{
		*ppDeviceListTarget = pDeviceListTarget;
	}

	return Err;
}

int CServer::RemoveFromDB()
{
	if (GetMMP()->delServer(this).IsOK())
	{
		// Log
		CString cstrLogMsg;
		cstrLogMsg.Format("Server - Remove '%s'", m_strName.c_str());
		m_pParent->m_pParent->LogUserAction(cstrLogMsg);

		// Advise other GUI
		GetMMP()->SendTdmfObjectChangeMessage(TDMF_SERVER, TDMF_REMOVE, m_iDbDomainFk, m_iDbSrvId, 0);
	}

	return MMPAPI_Error::OK;
}

void CServer::ParseXMLQuery(CString cstrXML, std::list<CStatsInfo>& listStats)
{
	// Skip header
	CString cstrDataXML = cstrXML.Mid(cstrXML.Find("</COLUMNS>") + strlen("</COLUMNS>"));
		
	// For each row "<ROW Group="0" Stat="2"/>
	int nRow;
	while ((nRow = cstrDataXML.Find("<ROW")) != -1)
	{
		cstrDataXML = cstrDataXML.Mid(nRow + strlen("<ROW"));
		cstrDataXML.TrimLeft();

		ASSERT(strncmp(cstrDataXML, "Group", strlen("Group")) == 0);
		cstrDataXML = cstrDataXML.Mid(strlen("Group"));
		cstrDataXML.TrimLeft();

		ASSERT(strncmp(cstrDataXML, "=", strlen("=")) == 0);
		cstrDataXML = cstrDataXML.Mid(strlen("="));
		cstrDataXML.TrimLeft();

		ASSERT(strncmp(cstrDataXML, "\"", strlen("\"")) == 0);
		cstrDataXML = cstrDataXML.Mid(strlen("\""));
		cstrDataXML.TrimLeft();

		int nGroupId = atoi(cstrDataXML);
		cstrDataXML = cstrDataXML.Mid(cstrDataXML.Find("\"") + strlen("\""));
		cstrDataXML.TrimLeft();

		ASSERT(strncmp(cstrDataXML, "Stat", strlen("Stat")) == 0);
		cstrDataXML = cstrDataXML.Mid(strlen("Stat"));
		cstrDataXML.TrimLeft();

		ASSERT(strncmp(cstrDataXML, "=", strlen("=")) == 0);
		cstrDataXML = cstrDataXML.Mid(strlen("="));
		cstrDataXML.TrimLeft();

		ASSERT(strncmp(cstrDataXML, "\"", strlen("\"")) == 0);
		cstrDataXML = cstrDataXML.Mid(strlen("\""));
		cstrDataXML.TrimLeft();

		int nStatId = atoi(cstrDataXML);

		////////////////////////////////
		// Add Stat to list

		// Check if group is already added
		for (std::list<CStatsInfo>::iterator it = listStats.begin();
			 (it != listStats.end()) && (it->m_nGroupId != nGroupId); it++);
		if (it == listStats.end())
		{
			CStatsInfo StatsInfo;
			StatsInfo.m_nGroupId = nGroupId;
			listStats.push_back(StatsInfo);
			it = listStats.end();
			it--;
		}
		// Check if the stat already exists for that group
		for (std::list<CStat>::iterator itStat = it->m_listStat.begin();
			 (itStat != it->m_listStat.end()) && (itStat->m_nStatId != nStatId); itStat++);
		if (itStat == it->m_listStat.end())
		{
			CStat Stat;
			Stat.m_nStatId = nStatId;
			it->m_listStat.push_back(Stat);
		}
	}
}

void CServer::FillStats(std::list<CStatsInfo>& listStats, DATE dateBegin, DATE dateEnd)
{
	FsTdmfRecPerf *pPerfTbl = GetDB()->mpRecPerf;
	pPerfTbl->FtdLock();

	for (std::list<CStatsInfo>::iterator it = listStats.begin(); it != listStats.end(); it++)
	{
		// SELECT *
		// FROM   t_Performance
		// WHERE  (Source_Fk = m_iDbSrvId) AND (LgGroupId_Fk = GroupId) AND (PairId_Fk = PairId) 
		// AND (Time_Stamp > Start) AND (Time_Stamp < Stop)

		COleDateTime DateTimeBegin(dateBegin);
		COleDateTime DateTimeEnd(dateEnd);
		CString cstrDateBegin = DateTimeBegin.Format();
		CString cstrDateEnd   = DateTimeEnd.Format();

		// Find DB record
		CString cszWhere;
		cszWhere.Format(" %s = %d AND %s = %d AND %s = %d AND %s >= '%s' AND %s <= '%s'",
						 FsTdmfRecPerf::smszFldSrcFk, m_iDbSrvId,
						 FsTdmfRecPerf::smszFldGrpFk, it->m_nGroupId,
						 FsTdmfRecPerf::smszFldDevId, -1 /*PairId*/,
						 FsTdmfRecPerf::smszFldTs,    cstrDateBegin,
						 FsTdmfRecPerf::smszFldTs,    cstrDateEnd);

		CString cszOrder;
		cszOrder.Format(" %s ASC ", FsTdmfRecPerf::smszFldTs);

		BOOL bLoop = pPerfTbl->FtdFirst(cszWhere, cszOrder);
		while ( bLoop )
		{   
			for (std::list<CStat>::iterator itStat = it->m_listStat.begin(); itStat != it->m_listStat.end(); itStat++)
			{
				COleDateTime DateTime;
				CString strDate = pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldTs);
				strDate.Replace("-", "/");
				strDate = strDate.Left(19);
				
				if (DateTime.ParseDateTime(strDate) != FALSE)
				{
					COleDateTimeSpan TimeSpan = DateTime /* - */;
					
					switch (itStat->m_nStatId)
					{
					case eBABEntries:
						itStat->m_vecValues.push_back(CPoint((int)TimeSpan.GetTotalMinutes(), atoi(pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldEntries))));
						break;
					case ePctBABFull:
						itStat->m_vecValues.push_back(CPoint((int)TimeSpan.GetTotalMinutes(), atoi(pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldPctBab))));
						break;
					case ePctDone:
						itStat->m_vecValues.push_back(CPoint((int)TimeSpan.GetTotalMinutes(), atoi(pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldPctDone))));
						break;
					case eReadBytes:
						itStat->m_vecValues.push_back(CPoint((int)TimeSpan.GetTotalMinutes(), atoi(pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldBytesRead))));
						break;
					case eWriteBytes:
						itStat->m_vecValues.push_back(CPoint((int)TimeSpan.GetTotalMinutes(), atoi(pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldBytesWritten))));
						break;
					case eBABSectors:
						itStat->m_vecValues.push_back(CPoint((int)TimeSpan.GetTotalMinutes(), atoi(pPerfTbl->FtdSel(FsTdmfRecPerf::smszFldSectors))));
						break;
					}
				}
			}
			
			bLoop = pPerfTbl->FtdNext();
		}
	}

	pPerfTbl->FtdUnlock();
}

void CServer::CreateXMLResult(std::list<CStatsInfo>& listStats, CString& cstrDataXML)
{
	cstrDataXML = "<?xml version=\"1.0\"?>"
				  "<DATA>"
				  "<COLUMNS>";
	for (std::list<CStatsInfo>::iterator it = listStats.begin(); it != listStats.end(); it++)
	{
		for (std::list<CStat>::iterator itStat = it->m_listStat.begin(); itStat != it->m_listStat.end(); itStat++)
		{
			CString cstrColumn;
			cstrColumn.Format("<COLUMN NAME=\"GSX%d%d\" TYPE=\"Integer\"/>\n", it->m_nGroupId, itStat->m_nStatId);
			cstrDataXML += cstrColumn;
			cstrColumn.Format("<COLUMN NAME=\"GSY%d%d\" TYPE=\"Integer\"/>\n", it->m_nGroupId, itStat->m_nStatId);
			cstrDataXML += cstrColumn;

		}
	}
	cstrDataXML += "</COLUMNS>";

	// Data
	UINT nIndex = 0;
	bool bData;

	do
	{
		bData = false;

		cstrDataXML += "<ROW ";
		for (std::list<CStatsInfo>::iterator it = listStats.begin(); it != listStats.end(); it++)
		{
			for (std::list<CStat>::iterator itStat = it->m_listStat.begin();
				 itStat != it->m_listStat.end(); itStat++)
			{
				if (nIndex < itStat->m_vecValues.size())
				{
					CString cstrValue;

					// X value
					cstrValue.Format("GSX%d%d = \"%d\" ", it->m_nGroupId, itStat->m_nStatId, itStat->m_vecValues[nIndex].x);
					cstrDataXML += cstrValue;

					// Y value
					cstrValue.Format("GSY%d%d = \"%d\" ", it->m_nGroupId, itStat->m_nStatId, itStat->m_vecValues[nIndex].y);
					cstrDataXML += cstrValue;

					bData = true;
				}
			}
		}
		cstrDataXML += "/>\n";

		nIndex++;

	} while (bData);

	// Footer
	cstrDataXML += "</DATA>";
}

CString CServer::GetPerformanceValues(CString Stats, DATE dateBegin, DATE dateEnd)
{
	std::list<CStatsInfo> listStats;

	// Parse Stats buffer
	ParseXMLQuery(Stats, listStats);

	// Get data from DB
	FillStats(listStats, dateBegin, dateEnd);

	// Create the resulting XML buffer
	CString cstrRetVal;
	CreateXMLResult(listStats, cstrRetVal);

	return cstrRetVal;
}

BOOL CServer::FindTheScriptFileInTheList(CString strFilename)
{
    BOOL bResult = FALSE;

    if(m_listScriptServerFile.size() <= 0)
       return FALSE;

    std::list<CScriptServer>::iterator it;
    for (it = this->m_listScriptServerFile.begin(); it != this->m_listScriptServerFile.end(); it++)
    {
 		if (_stricmp(it->m_strFileName.c_str(), strFilename ) == 0)
		{
           	bResult = TRUE;		
			break;
		}
    }
    return bResult;
}

BOOL CServer::FindTheScriptFilesMissingOnServer(CStringArray& arrStrFilename, std::list<CScriptServer>& listImportScriptServer)
{
    BOOL bResult = FALSE;

    if(m_listScriptServerFile.size() <= 0)
       return FALSE;

    std::list<CScriptServer>::iterator it;
    std::list<CScriptServer>::iterator itImportList;
    for (it = this->m_listScriptServerFile.begin(); it != this->m_listScriptServerFile.end(); it++)
    {
        BOOL bFound = FALSE;
        for (itImportList = listImportScriptServer.begin(); itImportList != listImportScriptServer.end(); itImportList++)
        {
          	if (_stricmp(it->m_strFileName.c_str(),itImportList->m_strFileName.c_str() ) == 0)
		    {
           	    bFound = TRUE;		
			    break;
		    }
        }
        if(!bFound)
        {
          arrStrFilename.Add(CString(it->m_strFileName.c_str()));
          bResult = TRUE;
        }
	
    }
    return bResult;
}

long CServer::ImportAllScriptServerFiles(BOOL bOverwriteExistingFile, char* strFileExtension)
{
    MMPAPI_Error code = MMPAPI_Error::OK;

    CSystem* pSystem = m_pParent->m_pParent;
    
    long lFileType;
    if(!IsValidScriptServerFileName(strFileExtension,lFileType))
    {
       CString strMsg;
       strMsg.Format("'%s': Invalid entry for filename.", strFileExtension);
       MessageBox( pSystem->m_hWnd,(LPCTSTR) strMsg, _T("Import Script File"),  MB_OK | MB_ICONERROR );
	   return MMPAPI_Error::ERR_UNKNOWN_SCRIPT_SERVER_FILE;
    }

    //retreives all file from Server.  .
    std::list<CScriptServer> listScriptServer;
    code = GetMMP()->ImportAllScriptServerFiles( *this,strFileExtension, lFileType, listScriptServer);
    if ( !code.IsOK() )
    {
       CString strMsg;
       strMsg.Format("Import did not work for files '%s' \n\nReason : '%s'.", strFileExtension ,(CString)code);
       MessageBox( pSystem->m_hWnd,(LPCTSTR) strMsg, _T("Import Script File"),  MB_OK | MB_ICONERROR );	   
	   return code;
    }

    CStringArray arrStrFilename;
    if(FindTheScriptFilesMissingOnServer(arrStrFilename,listScriptServer))
    {
       CString strArrayFilename;
       for (int i=0;i < arrStrFilename.GetSize();i++)
       {
            strArrayFilename += arrStrFilename.ElementAt(i);
            strArrayFilename += _T("; ");
       }
       CString strMsg;
       strMsg.Format("Theses files are missing on the server '%s' : (",m_strName.c_str());
       strMsg +=  strArrayFilename;
       strMsg += _T(")");
       MessageBox( pSystem->m_hWnd,(LPCTSTR) strMsg, _T("Import Script File"),  MB_OK | MB_ICONEXCLAMATION  );
    }

	std::list<CScriptServer>::iterator itImport;
	for (itImport = listScriptServer.begin();
		 itImport != listScriptServer.end(); itImport++)
	{

       	BOOL bImport = TRUE;

        long lType;
        if(!IsValidScriptServerFileName(itImport->m_strFileName.c_str(),lType))
        {
            bImport = FALSE;
		    continue;
        }

		if(itImport->m_strContent.size() <= 0)
		{
	       bImport = FALSE;
           continue;
		}
		

        std::list<CScriptServer>::iterator it;
        for(it = m_listScriptServerFile.begin(); it != m_listScriptServerFile.end(); it++)
        {
          if (_stricmp(it->m_strFileName.c_str(), itImport->m_strFileName.c_str() ) == 0)
           {
			    if(bOverwriteExistingFile)
                {
                    it->m_strContent = itImport->m_strContent;
                    it->SaveToDB();                    
                }
                else
                {
                    CString strMsg;
                  	strMsg.Format("The filename '%s' will be overwritten. Do you want to continue?", it->m_strFileName.c_str());
                    if ( MessageBox( pSystem->m_hWnd,(LPCTSTR) strMsg, _T("Import Script File"),  MB_YESNO | MB_ICONQUESTION ) == IDYES)
			        {
                      it->m_strContent = itImport->m_strContent;
                      it->SaveToDB();   
                    }
                }
                bImport = FALSE;
				break;
           }
        }
        if (bImport)
		{
			// Create a new Script Server Object
	        CScriptServer SS;
	        SS.m_pServer = this;
            SS.m_iDbSrvId = this->m_iDbSrvId;
            SS.m_strFileName = itImport->m_strFileName;
            SS.m_strContent = itImport->m_strContent;
            SS.m_bNew     = true;

            m_listScriptServerFile.push_back(SS);
            SS.SaveToDB();
		}
    }
  return code;
}

long CServer::ImportOneScriptServerFile(char* strFilename)
{
    MMPAPI_Error code = MMPAPI_Error::OK;
    CSystem* pSystem = m_pParent->m_pParent;
    long lFileType;

    if(!IsValidScriptServerFileName(strFilename,lFileType))
    {
        CString strMsg;
        strMsg.Format("'%s': Invalid entry for filename.", strFilename);
        MessageBox( pSystem->m_hWnd,(LPCTSTR) strMsg, _T("Import Script File"),  MB_OK | MB_ICONERROR );	   
	 
        return MMPAPI_Error::ERR_UNKNOWN_SCRIPT_SERVER_FILE;
    }

	CString str(strFilename);
	if(str.Left(2) == _T("*."))
	{
		ImportAllScriptServerFiles(false,strFilename);
		return MMPAPI_Error::OK;
	}
	 
    //retreives all file from Server.  .
    std::list<CScriptServer> listScriptServer;
    CScriptServer ScriptServer;
    BOOL bImport = TRUE;

    code = GetMMP()->ImportOneScriptServerFile(*this, strFilename, lFileType,ScriptServer);
    if ( !code.IsOK() )
    {
       CString strMsg;
       strMsg.Format("Import did not work for file '%s' \n\nReason : '%s'.", strFilename ,(CString)code);
       MessageBox( pSystem->m_hWnd,(LPCTSTR) strMsg, _T("Import Script File"),  MB_OK | MB_ICONERROR );	   
	 	   
	   return code;
    }

    std::list<CScriptServer>::iterator it;
    for(it = m_listScriptServerFile.begin(); it != m_listScriptServerFile.end(); it++)
    {
       if ( _stricmp(it->m_strFileName.c_str(), strFilename ) == 0)
       {
            it->m_strContent = ScriptServer.m_strContent;
            it->SaveToDB();                    
            bImport = FALSE;
			break;
       }
    }
    if (bImport)
	{
		// Create a new Script Server Object
	    ScriptServer.m_pServer = this;
        ScriptServer.m_iDbSrvId = this->m_iDbSrvId;
        ScriptServer.m_bNew     = true;
        m_listScriptServerFile.push_back(ScriptServer);
        ScriptServer.SaveToDB();
  	}

  return code;
}

/*
 * Updates one replication group 
 * .cfg file uploaded from Server and 
 */
long CServer::ImportReplGroup(std::string& strErrorMsg)
{
	enum ImportError {
		IMPORT_SUCCESS,
		IMPORT_GRP_ALREADY_DEFINED,
		IMPORT_MISSING_TARGET_SVR,
		IMPORT_ERROR,
		IMPORT_WARNING,
		IMPORT_GRP_SYMMETRIC,
	};

	strErrorMsg = "";
	enum ImportError ImportRetCode = IMPORT_SUCCESS;

	//retreives .cfg from Server.  Fills repgrps as much as possible.
	std::list<CReplicationGroup> listRepGrp;
	std::list<std::string> liststrPrimary;
	std::list<std::string> liststrTarget;
    MMPAPI_Error code = GetMMP()->getRepGroupCfg(*this, listRepGrp, liststrPrimary, liststrTarget);
    if ( !code.IsOK() )
    {
		strErrorMsg = (CString)code;
        return IMPORT_ERROR;
    }

	std::map<UINT, CDeviceList*> mappDeviceList;
	CDeviceList* pDeviceListSource = NULL;
	GetDeviceList(&pDeviceListSource);
	mappDeviceList[m_nHostID] = pDeviceListSource;

	// Start a database transaction
	GetDB()->mpRecGrp->BeginTrans();

	std::list<CReplicationGroup>::iterator it;
	std::list<std::string>::iterator itPrimary;
	std::list<std::string>::iterator itTarget;
	for (it = listRepGrp.begin(), itPrimary = liststrPrimary.begin(),itTarget = liststrTarget.begin();
		 it != listRepGrp.end(); it++, itPrimary++, itTarget++)
	{
		enum ImportError ImportGroup = IMPORT_SUCCESS;

		// Import only if there is no group with that group number
		for (std::list<CReplicationGroup>::iterator itRGServer = m_listReplicationGroup.begin();
			itRGServer != m_listReplicationGroup.end(); itRGServer++)
		{
			if (itRGServer->m_nGroupNumber == it->m_nGroupNumber)
			{
				// don't import this group
				ImportGroup = IMPORT_GRP_ALREADY_DEFINED;
				break;
			}
			else if (itRGServer->m_nSymmetricGroupNumber == it->m_nGroupNumber)
			{
				// don't import this group
				ImportGroup = IMPORT_GRP_SYMMETRIC;
				break;
			}
		}

		if (ImportGroup == IMPORT_SUCCESS)
		{
			if (!ImportOneReplGroup(*it, *itPrimary,*itTarget, mappDeviceList))
			{
				ImportGroup = IMPORT_MISSING_TARGET_SVR;
			}
		}

		// Report results
		if (strErrorMsg.length() == 0)
		{
			strErrorMsg = "The following replication groups have been found on server:\n\n";
		}

		std::ostringstream oss;
		oss << "Replication Group #" << it->m_nGroupNumber << " : ";
		if (ImportGroup != IMPORT_SUCCESS)
		{
			switch (ImportGroup)
			{
			case IMPORT_GRP_ALREADY_DEFINED:
				oss << "Not Imported - Already defined on server.";
				break;
			case IMPORT_GRP_SYMMETRIC:
				oss << "Not Imported - Already used in a Symmetric configuration.";
				break;
			case IMPORT_MISSING_TARGET_SVR:
				oss << "Not Imported - Target server: " << *itTarget << " not found.";
				break;
			}

			ImportRetCode = IMPORT_WARNING;
		}
		else
		{
			oss << "Successfully Imported.";
		}
		oss << "\n";
		strErrorMsg += oss.str();

		ImportRetCode = IMPORT_WARNING;
	}

	// Commit database transaction
	GetDB()->mpRecGrp->CommitTrans();

	// Cleanup
	for(std::map<UINT, CDeviceList*>::iterator i = mappDeviceList.begin(); i != mappDeviceList.end(); i++)
	{
		delete i->second;
	}

	return ImportRetCode;
}

BOOL CServer::ImportOneReplGroup(CReplicationGroup& RepGrp, std::string& strPrimaryHostName, std::string& strTargetHostName, std::map<UINT, CDeviceList*> mappDeviceList)
{

//Verify the status of the primary server
  	if(stricmp(strPrimaryHostName.c_str(), "127.0.0.1") == 0)
	{
		RepGrp.m_bPrimaryEditedIPUsed = true;
		RepGrp.m_strPrimaryEditedIP = _T("127.0.0.1");
	}
	else if ((stricmp(strPrimaryHostName.c_str(), m_strName.c_str()) == 0) )
		{
			RepGrp.m_bPrimaryDHCPAdressUsed = true;
		}
		else
		{
		 // Check if the ip adress is in the vector
			for (UINT i = 0; i < m_vecstrIPAddress.size(); i++)
			{
			  if ((stricmp(strPrimaryHostName.c_str(), m_vecstrIPAddress[i].c_str()) == 0) )
			  {
				if(i > 0)
				{
				   RepGrp.m_bPrimaryEditedIPUsed = true;
				   RepGrp.m_strPrimaryEditedIP  = m_vecstrIPAddress[i].c_str();
				}
				break;
			  }
			}
		}
 


//check for a loopback repl. grp
//non case sensitive name comparaison
    bool   bLoopback = false;

	if(stricmp(strTargetHostName.c_str(), "127.0.0.1") == 0)
	{
		RepGrp.m_bTargetEditedIPUsed = true;
		RepGrp.m_strTargetEditedIP = _T("127.0.0.1");
		bLoopback = true;
	}
	else if ((stricmp(strTargetHostName.c_str(), m_strName.c_str()) == 0) )
		{
			RepGrp.m_bTargetDHCPAdressUsed = true;
			bLoopback = true;
		}
		else
		{
		 // Check if the ip adress is in the vector
			for (UINT i = 0; i < m_vecstrIPAddress.size(); i++)
			{
			  if ((stricmp(strTargetHostName.c_str(), m_vecstrIPAddress[i].c_str()) == 0) )
			  {
				if(i > 0)
				{
				   RepGrp.m_bTargetEditedIPUsed = true;
				   RepGrp.m_strTargetEditedIP = m_vecstrIPAddress[i].c_str();
				}

				bLoopback = true;
				break;
			  }
			}
		}
  
    RepGrp.m_pParent       = this;
    RepGrp.m_pServerTarget = 0;

    if ( bLoopback )
    {
        RepGrp.m_pServerTarget			= this;
		RepGrp.m_bPrimaryEditedIPUsed	= RepGrp.m_bTargetEditedIPUsed;
		RepGrp.m_bPrimaryDHCPAdressUsed	= RepGrp.m_bTargetDHCPAdressUsed;
     	RepGrp.m_strPrimaryEditedIP		= RepGrp.m_strTargetEditedIP;

    }
    else //check for a non loopback repl. grp
    {  
		//not in loopback mode. check if other Server is known by TDMF System
        std::list<CServer>::iterator it  = m_pParent->m_listServer.begin();
        std::list<CServer>::iterator end = m_pParent->m_listServer.end();
        CServer *pOtherSrvr = NULL;
        bool bFound = false;
        //try to find other server in the CSystem objects (same Domain...)
        while (it != end && !bFound)
        {
            pOtherSrvr = &(*it);

			// Check if the ip adress is in the vector
			for (UINT i = 0; i < pOtherSrvr->m_vecstrIPAddress.size(); i++)
			{
			  if ((stricmp(strTargetHostName.c_str(), pOtherSrvr->m_vecstrIPAddress[i].c_str()) == 0) )
			  {
				if(i > 0)
				{
					RepGrp.m_bTargetEditedIPUsed = true;
					RepGrp.m_strTargetEditedIP	= pOtherSrvr->m_vecstrIPAddress[i].c_str();
               	}

				bFound = true;
				break;
			  }
			}
			if (!bFound)
			{
				if ((stricmp(strTargetHostName.c_str(), pOtherSrvr->m_strName.c_str()) == 0))
				{
					RepGrp.m_bTargetDHCPAdressUsed = true;
					bFound = true;
				}
			}
            it++;
        }
        if (bFound)
        {
            RepGrp.m_pServerTarget = pOtherSrvr;
        }
    }//else bLoopback

	// Ask user for a target server
	if (RepGrp.m_pServerTarget == NULL)
	{
		// Send a notification to the client (GUI)
		void *pTargetHostName = &strTargetHostName;
		int nServerId = SendMessage(m_pParent->m_pParent->m_hWnd, WM_IPADRESS_UNKNOWN, m_pParent->GetKey(), (LPARAM)pTargetHostName);
		if (nServerId > 0)
		{
			std::list<CServer>::iterator it  = m_pParent->m_listServer.begin();
			for (; it != m_pParent->m_listServer.end(); it++)
			{
				if (it->GetKey() == nServerId)
				{
					RepGrp.m_pServerTarget       = &*it;
					RepGrp.m_bTargetEditedIPUsed = true;
					RepGrp.m_strTargetEditedIP	 = strTargetHostName;

					break;
				}
			}
		}
	}

	//if other host is unknown to TDMF Ssytem, do not add group to list ... signal this to user	
	if (RepGrp.m_pServerTarget != NULL)
	{
		//first check of already exists in list
		std::list<CReplicationGroup>::iterator it  = m_listReplicationGroup.begin();
		std::list<CReplicationGroup>::iterator end = m_listReplicationGroup.end();
		bool bFound = false;
		while (it != end && !bFound)
		{
			bFound = (it->m_nGroupNumber == RepGrp.m_nGroupNumber);
			it++;
		}
		if (bFound)
		{   //grp already exists, so just update with the one received.
			//todo : CReplicationGroup::operator=() ???
			//overwrite grp definition.
			//*pGrp = RepGrp;
			*it = RepGrp;
		}
		else
		{   //imported a new group.  
			//create it in list and then overwrite it.
			CReplicationGroup& Grp = AddNewReplicationGroup();
			//overwrite grp definition.
			Grp = RepGrp;

			// Get additional device info from agent
			{
				CDeviceList* pDeviceListSource = mappDeviceList[m_nHostID];
				CDeviceList* pDeviceListTarget;
				
				if (Grp.m_pServerTarget == this)
				{
					pDeviceListTarget = pDeviceListSource;
				}
				else
				{
					if (mappDeviceList.find(Grp.m_pServerTarget->m_nHostID) != mappDeviceList.end())
					{
						pDeviceListTarget = mappDeviceList[Grp.m_pServerTarget->m_nHostID];
					}
					else
					{
						// TODO Don't re-get device of source server
					GetDeviceList(&pDeviceListSource, Grp.m_pServerTarget->m_nHostID, &pDeviceListTarget);
						mappDeviceList[Grp.m_pServerTarget->m_nHostID] = pDeviceListTarget;
					}
				}

				std::list<CReplicationPair>::iterator it;
				for (it = Grp.m_listReplicationPair.begin(); it != Grp.m_listReplicationPair.end(); it++)
				{
					std::list<CDevice>::iterator itDevice;
					for (itDevice = pDeviceListSource->m_listDevice.begin(); itDevice != pDeviceListSource->m_listDevice.end(); itDevice++)
					{
						if (it->m_DeviceSource.m_strPath == itDevice->m_strPath)
						{
							if (!(strstr(m_strOSType.c_str(),"Windows") != 0 ||
								strstr(m_strOSType.c_str(),"windows") != 0 ||
								strstr(m_strOSType.c_str(),"WINDOWS") != 0))
							{
								it->m_DeviceSource.m_strLength = itDevice->m_strLength;
							}
							else
							{
								it->m_DeviceSource.m_strFileSystem = itDevice->m_strFileSystem;
								it->m_strFileSystem = itDevice->m_strFileSystem;
							}
							break;
						}
					}
					
					for (itDevice = pDeviceListTarget->m_listDevice.begin(); itDevice != pDeviceListTarget->m_listDevice.end(); itDevice++)
					{
						if (it->m_DeviceTarget.m_strPath == itDevice->m_strPath)
						{
							if (!(strstr(m_strOSType.c_str(),"Windows") != 0 ||
								  strstr(m_strOSType.c_str(),"windows") != 0 ||
								  strstr(m_strOSType.c_str(),"WINDOWS") != 0))
							{
								it->m_DeviceTarget.m_strLength = itDevice->m_strLength;
							}
							else
							{
								it->m_DeviceTarget.m_strFileSystem = itDevice->m_strFileSystem;
							}
							break;
						}
					}
				}
			}

			//save new Repl.Group info to DB
			Grp.SaveToDB(Grp.m_nGroupNumber, Grp.m_pParent->m_nHostID, NULL, false);

			// Import tunables
			std::string strTunables;
			if (GetMMP()->GetTunables(Grp, strTunables).IsOK())
			{
				if ((strTunables.find("Bad header") == std::string::npos) &&
 					(strTunables.find("The system cannot find the file") == std::string::npos) &&
					(strTunables.find("Failed to read attributes") == std::string::npos))
				{
					MMP_API::TdmfTunables Tunables;

					// Set defaut values
					Tunables.nChunkDelay      = 0;
					Tunables.nChunkSize       = 256;
					Tunables.bCompression     = 0;
					Tunables.bSync            = 0;
					Tunables.nSyncDepth       = 1;
					Tunables.nSyncTimeout     = 30;
					Tunables.nRefreshTimeout  = -1;
					Tunables.nNetMaxKbps      = 0;
					Tunables.nStatInterval    = 10;
					Tunables.nMaxStatFileSize = 64;
					Tunables.bJournalLess	  = FALSE;
						
					if (GetMMP()->ParseTunables(strTunables, &Tunables))
					{
						Grp.m_nChunkDelay = Tunables.nChunkDelay;
						Grp.m_nChunkSizeKB = Tunables.nChunkSize;
						Grp.m_bEnableCompression = Tunables.bCompression;
						Grp.m_bSync = Tunables.bSync;
						Grp.m_nSyncDepth = Tunables.nSyncDepth;
						Grp.m_nSyncTimeout = Tunables.nSyncTimeout;
						Grp.m_nRefreshTimeoutInterval = Tunables.nRefreshTimeout;
						if (Tunables.nRefreshTimeout == -1)
						{
							Grp.m_bRefreshNeverTimeout = true;
							Grp.m_nRefreshTimeoutInterval = 0;
						}
						Grp.m_nNetMaxKbps = Tunables.nNetMaxKbps;
						Grp.m_nStatInterval = Tunables.nStatInterval;
						Grp.m_nMaxFileStatSize = Tunables.nMaxStatFileSize;
						Grp.m_bJournalLess	= Tunables.bJournalLess;

						GetMMP()->SaveTunables(Grp, false);
					}
				}
			}
		}
	}
	else
	{   //not added because other host is unknown to TDMF System (DB)...
		return FALSE;
	}
    
    return TRUE;
}

bool CServer::RemoveScriptServerFile(long nDbScriptSrvId)
{
  	std::list<CScriptServer>::iterator it = m_listScriptServerFile.begin();
	while(it != m_listScriptServerFile.end())
	{
		if (it->m_iDbScriptSrvId == nDbScriptSrvId)
		{
            if(!it->m_bNew)
            {
                 // Remove it from DB
			    if(it->RemoveFromDB() == MMPAPI_Error::OK)
                {
                    // delete it 
			        m_listScriptServerFile.erase(it);
                    return true;
                }
            }
            else
            {
                // delete it 
			    m_listScriptServerFile.erase(it);
                return true;
            }
           
		}
		it++;
	}

	return false;
}

BOOL CServer::IsValidScriptServerFileName(const char* strScriptServerFilename, long& lFileType)
{
  lFileType = -1;
  CString strFilename(strScriptServerFilename,strlen(strScriptServerFilename));
  
  if(strFilename.IsEmpty())
      return FALSE;

  CString strExtension = strFilename.Right(3);

  if((strExtension.CompareNoCase(_T("bat")) == 0) ||
	 (strExtension.CompareNoCase(_T(".sh")) == 0))
  {
    lFileType = MMP_MNGT_FILETYPE_TDMF_BAT;
    return TRUE;
  }
 
  return FALSE;
}

void CServer::LockCmds()
{
	m_bLockCmds = true;
	m_pParent->m_pParent->m_nNbLockCmds++;
}

void CServer::UnlockCmds()
{
	m_bLockCmds = false;
	m_pParent->m_pParent->m_nNbLockCmds--;
}
