// ReplicationGroup.cpp : Implementation of CReplicationGroup
#include "stdafx.h"
#include "ReplicationPair.h"
#include "ReplicationGroup.h"
#include "Server.h"
#include <set>
#include <iomanip>

#include "FsTdmfDb.h"
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
// CReplicationGroup

CReplicationPair& CReplicationGroup::AddNewReplicationPair()
{
	// Create a new Replication Pair
	CReplicationPair RP;
	RP.m_pParent = this;

	// Init its Pair number, find a unique one
	std::set<long> setPairNumbers;
	std::list<CReplicationPair>::iterator itRP;
	for (itRP = m_listReplicationPair.begin(); itRP != m_listReplicationPair.end(); itRP++)
	{
		setPairNumbers.insert(itRP->m_nPairNumber);
	}
	long nPairNumberUnique = 0;
	while(setPairNumbers.find(nPairNumberUnique) != setPairNumbers.end())
	{
		nPairNumberUnique++;
	}
	RP.m_nPairNumber = nPairNumberUnique;

	m_listReplicationPair.push_back(RP);

	return m_listReplicationPair.back();
}

void CReplicationGroup::setTarget(const CServer *pServerTarget)
{
    m_pServerTarget = (CServer *)pServerTarget;

    if ( m_pServerTarget && m_strJournalDirectory.empty() )
	{
        m_strJournalDirectory = m_pServerTarget->m_strJournalDirectory;
	}
}

bool CReplicationGroup::SetNewTargetServer(char* pszDomainName, long nHostID)
{
	bool bFound = false;

	// Find server
	CSystem* pSystem = m_pParent->m_pParent->m_pParent;

	// Search domain, then server
	std::list<CDomain>::iterator itDomain = pSystem->m_listDomain.begin();
	for(;itDomain != pSystem->m_listDomain.end(); itDomain++)
	{
		if (itDomain->m_strName == pszDomainName)
		{
			std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
			for (; itServer != itDomain->m_listServer.end(); itServer++)
			{
				if (itServer->m_nHostID == nHostID)
				{
					bFound = true;

					// Set target server
					setTarget(&*itServer);

					break;
				}
			}

			break;
		}
	}

	return bFound;
}

FsTdmfDb* CReplicationGroup::GetDB() 
{ 
    return m_pParent->GetDB(); 
}

MMP_API* CReplicationGroup::GetMMP() 
{ 
    return m_pParent->GetMMP(); 
}

bool CReplicationGroup::Initialize(FsTdmfRecLgGrp *pRec, bool bRecurse)
{
    CString tmp;
    //init data members from DB record
	m_iDbSrcFk              = atoi( pRec->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk ) );
	m_iDbTgtFk              = atoi( pRec->FtdSel( FsTdmfRecLgGrp::smszFldTgtFk ) );
	if (m_iDbSrcFk == m_iDbTgtFk)
	{
		m_nType = GT_SOURCE | GT_LOOPBACK;
	}
	else
	{
		m_nType = GT_SOURCE;
	}

    m_nGroupNumber          = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId ) );
	m_strPStoreFile         = (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldPStore );
	m_strJournalDirectory   = (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldJournalVol );
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldChainning );
    m_bChaining             = atoi( (LPCTSTR)tmp ) != 0 ; //!tmp.IsEmpty() && ( tmp.Compare("1") || tmp.CompareNoCase("true") || tmp.CompareNoCase("yes") );
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldSymmetric );
	m_bSymmetric            = atoi( (LPCTSTR)tmp ) != 0 ;
	m_nSymmetricGroupNumber = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldSymmetricGroupNumber ) );
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldSymmetricNormallyStarted );
	m_bSymmetricNormallyStarted = atoi( (LPCTSTR)tmp ) != 0 ;
	m_nFailoverInitialState = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldFailoverInitialState ) );
	m_strSymmetricJournalDirectory = (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldSymmetricJournalDirectory);
	m_strSymmetricPStoreFile = (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldSymmetricPStoreFile);
	m_strDescription        = (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldNotes );
	m_strThrottles			= (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldThrottles);
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldSyncMode );
    m_bSync                 = atoi( (LPCTSTR)tmp ) != 0 ; //!tmp.IsEmpty() && ( tmp.Compare("1") || tmp.CompareNoCase("true") || tmp.CompareNoCase("yes") );
	m_nSyncDepth            = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldSyncModeDepth ) );
	m_nSyncTimeout          = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldSyncModeTimeOut ) );
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldRefreshNeverTimeOut );
	m_bRefreshNeverTimeout  = atoi( (LPCTSTR)tmp ) != 0 ; //!tmp.IsEmpty() && ( tmp.Compare("1") || tmp.CompareNoCase("true") || tmp.CompareNoCase("yes") );
	m_nRefreshTimeoutInterval=atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldRefreshTimeOut ) );
	m_nConnectionState      = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldConnection ) );
	m_nChunkDelay           = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldChunkDelay ) );
	m_nChunkSizeKB          = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldChunkSize ) );
	m_nMaxFileStatSize      = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldMaxFileStatSize ) );
    m_strStateTimeStamp     = (LPCTSTR)pRec->FtdSel( FsTdmfRecLgGrp::smszFldStateTimeStamp );
	m_strStateTimeStamp.resize(19); // Chop last ".000"
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldEnableCompression );
	m_bEnableCompression    = atoi( (LPCTSTR)tmp ) != 0 ; //!tmp.IsEmpty() && ( tmp.Compare("1") || tmp.CompareNoCase("true") || tmp.CompareNoCase("yes") );
	tmp                     = pRec->FtdSel( FsTdmfRecLgGrp::smszFldNetUsageThreshold ); 
	m_bNetThreshold         = atoi( (LPCTSTR)tmp ) != 0 ; //!tmp.IsEmpty() && ( tmp.Compare("1") || tmp.CompareNoCase("true") || tmp.CompareNoCase("yes") );
	m_nNetMaxKbps           = atol(pRec->FtdSel( FsTdmfRecLgGrp::smszFldNetUsageValue ) );
	m_nStatInterval         = atol(pRec->FtdSel( FsTdmfRecLgGrp::smszFldUpdateInterval ) );
	m_bJournalLess          = atol(pRec->FtdSel( FsTdmfRecLgGrp::smszFldJournalLess ) );
	
	m_bPrimaryDHCPAdressUsed		= atoi(pRec->FtdSel( FsTdmfRecLgGrp::smszFldPrimaryDHCPNameUsed ));
    m_bPrimaryEditedIPUsed      = atoi(pRec->FtdSel( FsTdmfRecLgGrp::smszFldPrimaryEditedIPUsed )  ); 	
	// Read all IP addresses
    {
		unsigned long   ip;
		char            buf[32];
		ip              = atol( pRec->FtdSel( FsTdmfRecLgGrp::smszFldPrimaryEditedIP ) );
		ip_to_ipstring(ip, buf );
		m_strPrimaryEditedIP = buf;
    } 
    
    m_bTargetDHCPAdressUsed     = atoi(pRec->FtdSel( FsTdmfRecLgGrp::smszFldTgtDHCPNameUsed ) );
    m_bTargetEditedIPUsed       = atoi(pRec->FtdSel( FsTdmfRecLgGrp::smszFldTgtEditedIPUsed ) );
	{
		unsigned long   ip;
		char            buf[32];
		ip              = atol( pRec->FtdSel(FsTdmfRecLgGrp::smszFldTgtEditedIP ) );
		if (ip > 0)
		{
			ip_to_ipstring(ip, buf );
			m_strTargetEditedIP = buf;
		}
    }
    
    //********** members without DB correspondance ... ************
    //m_nJournalSize        =;
	//m_nPStoreSize
	//m_nMode               =;

    //if no specific PStore for this Group, use Server default PStore.
    if ( m_pParent && m_strPStoreFile.empty() )
	{
		std::ostringstream oss;
		oss << m_pParent->m_strPStoreFile;
		if (strstr(m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
		{
			oss	<< ((*m_pParent->m_strPStoreFile.rbegin()) == '\\' ? "" : "\\")
				<< "PStore"
				<< std::setw(3) << std::setfill('0') << m_nGroupNumber
				<< ".dat";
		}
        m_strPStoreFile = oss.str();
	}

    //if no specific Journal dir. for this Group, use Target Server default Journal dir.
    if ( m_pServerTarget && m_strJournalDirectory.empty() )
	{
        m_strJournalDirectory = m_pServerTarget->m_strJournalDirectory;
	}

    if ( bRecurse )
    {   
        //fill the list of CReplicationGroup owned by this Server
        int SrcFk = atoi( pRec->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk ) );
        int GrpFk = atoi( pRec->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId ) );
        FsTdmfRecPair *pRecPair = GetDB()->mpRecPair;

        CString cszWhere;
        cszWhere.Format( " %s = %d AND %s = %d ", FsTdmfRecPair::smszFldGrpFk, GrpFk , FsTdmfRecPair::smszFldSrcFk, SrcFk );
        BOOL bLoop = pRecPair->FtdFirst( cszWhere );
        while ( bLoop )
        {
            CReplicationPair & newPair = AddNewReplicationPair();//pair added to list
            newPair.Initialize(pRecPair);//init from t_ReplicationPair record
            bLoop = pRecPair->FtdNext();
        }
    }

	// Read from DB so... set object state to saved
	m_eObjectState = RPO_SAVED;

    return true;
}

long CReplicationGroup::LaunchCommand(enum tdmf_commands eCmd, char* pszOptions, char* pszLog, BOOL bSymmetric, CString& cstrMsg)
{
	MMPAPI_Error Err;

	if (bSymmetric)
	{
		Err = GetMMP()->tdmfCmd(m_pServerTarget->m_nHostID, eCmd, pszOptions);
		cstrMsg = Err;
	}
	else
	{
		Err = GetMMP()->tdmfCmd(m_pParent->m_nHostID, eCmd, pszOptions);
		cstrMsg = Err;
		
		if ((eCmd == MMP_MNGT_TDMF_CMD_START) && Err.IsOK())
		{
			if (GetMMP()->IsTunableModified(*this))
			{
				GetMMP()->SetTunables(*this);
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
	cstrLogMsg.Format("Replication Group - Launch Command '%s %s' to '%s - group# %d'", GetMMP()->GetCmdName(eCmd).c_str(), pszOptions, m_pParent->m_strName.c_str(), m_nGroupNumber);
	m_pParent->m_pParent->m_pParent->LogUserAction(cstrLogMsg);

    return Err;
}

int CReplicationGroup::SaveToDB(long nOldGroupNumber, long nOldTgtHostId, std::string* pstrWarning, bool bUseTransaction)
{
	m_eObjectState = RPO_SAVED;

	if (m_nType & GT_SOURCE)
	{
		return GetMMP()->addRepGroup(*this, nOldGroupNumber, nOldTgtHostId, pstrWarning, bUseTransaction);
	}

	return MMPAPI_Error::OK;
}

int CReplicationGroup::RemoveFromDB()
{
	if ((m_eObjectState == RPO_SAVED) && (m_nType & GT_SOURCE))
	{
		return GetMMP()->delRepGroup(*this);
	}

	return MMPAPI_Error::OK;
}

/**
 * This method performs verification on members values.
 * Its status indicates if the Replication Group is configured properly
 * can be sent to its TDMF Server/Agent.
 */
int     CReplicationGroup::IsValid()
{
    int iInvalidMembers = 0;

	// A target server must be entered
    if (m_pServerTarget == 0)
    {
        iInvalidMembers |= REPGRP_NO_TARGET_SERVER_SPECIFIED;
    }

	// Windows pstore and journal validation
	if (strstr(m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
	{
		// PStore file and Journal directory must be there and must begin with a drive letter
		// length must be greater or equal than 3 => "X:"
		if ( m_strPStoreFile.length() < 2 || !isalpha(m_strPStoreFile.at(0)) )
		{
			iInvalidMembers |= REPGRP_INVALID_PSTORE_FILENAME ;
		}
		{
			char szDrive[MAX_PATH],szDir[MAX_PATH],szFile[MAX_PATH];
			_splitpath(m_strPStoreFile.c_str(),szDrive,szDir,szFile,NULL);
			if ( szFile[0] == 0 )
			{
				iInvalidMembers |= REPGRP_INVALID_PSTORE_FILENAME ;
			}
		}
		if ( m_strJournalDirectory.length() < 2 || !isalpha(m_strJournalDirectory.at(0)) )
		{
			iInvalidMembers |= REPGRP_INVALID_JOURNAL_DIR ;
		}
	}
	else // Unix pstore and journal validation
	{
		if ( m_strPStoreFile.length() == 0 )
		{
			iInvalidMembers |= REPGRP_INVALID_PSTORE_FILENAME ;
		}
		if ( m_strJournalDirectory.length() == 0 )
		{
			iInvalidMembers |= REPGRP_INVALID_JOURNAL_DIR ;
		}
	}

	// At least one pair must be defined
    if ( m_listReplicationPair.size() == 0 )
    {
        iInvalidMembers |= REPGRP_ERROR_NO_REPPAIR;
    }

	return iInvalidMembers;
}

long CReplicationGroup::GetUniqueGroupNumber()
{
    //
	// Group Number must be unique in both source and target
    //
	std::set<long> setGroupNumbers;
	std::list<CReplicationGroup>::iterator itRG;
	for (itRG = m_pParent->m_listReplicationGroup.begin(); itRG != m_pParent->m_listReplicationGroup.end(); itRG++)
	{
		// skip ourself and loopback target group
		if ((&*itRG != this) && (!((itRG->m_nType & GT_TARGET) && (itRG->m_pServerTarget == itRG->m_pParent))))
		{
			setGroupNumbers.insert(itRG->m_nGroupNumber);
			setGroupNumbers.insert(itRG->m_nSymmetricGroupNumber);
		}
	}
	if (m_pServerTarget && (m_pServerTarget != m_pParent))
	{
		for (itRG = m_pServerTarget->m_listReplicationGroup.begin(); itRG != m_pServerTarget->m_listReplicationGroup.end(); itRG++)
		{
			// skip target group
			if (!((itRG->m_nType & GT_TARGET) && (itRG->m_pServerTarget == m_pParent)))
			{
				setGroupNumbers.insert(itRG->m_nGroupNumber);
				setGroupNumbers.insert(itRG->m_nSymmetricGroupNumber);
			}
		}
	}

	long nGroupNumberUnique = 0;

	// First check if old group number is still ok
	if (setGroupNumbers.find(m_nGroupNumber) == setGroupNumbers.end())
	{
		nGroupNumberUnique = m_nGroupNumber;
	}
	else
	{
		while(setGroupNumbers.find(nGroupNumberUnique) != setGroupNumbers.end())
		{
			nGroupNumberUnique++;
		}
	}

	//By default, Group inherits some Server values
	std::ostringstream oss;
	if (m_pParent->m_strPStoreFile.length() > 0)
	{
		oss << m_pParent->m_strPStoreFile;

		if (strstr(m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
		{
			oss << ((*m_pParent->m_strPStoreFile.rbegin()) == '\\' ? "" : "\\");
		}
	}
	if (strstr(m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
	{
		oss << "PStore"
			<< std::setw(3) << std::setfill('0') << nGroupNumberUnique
			<< ".dat";
	}
	m_strPStoreFile = oss.str();

	return nGroupNumberUnique;
}


// ardev 021129 v
CEvent* CReplicationGroup::GetEventAt ( long nIndex )
{
	CEvent* pEvent = m_EventList.GetAt( nIndex );

	if ( pEvent == NULL )
	{
		CSystem* lpSystem = m_pParent->m_pParent->m_pParent;
		long nSrvId = m_pParent->m_iDbSrvId;

		if (m_nType & GT_TARGET)
		{
			nSrvId = m_pServerTarget->m_iDbSrvId;
		}

		pEvent = m_EventList.ReadRangeFromDb(lpSystem, nIndex, nSrvId, m_nGroupNumber);
	}

	return pEvent;

} // CReplicationGroup::GetEventAt ()


long CReplicationGroup::GetEventCount ()
{
	CString cszWhere;

	if (m_nType & CReplicationGroup::GT_SOURCE)
	{
		cszWhere.Format(" %s = %d AND %s = %d ",
						FsTdmfRecAlert::smszFldSrcFk, m_pParent->m_iDbSrvId,
						FsTdmfRecAlert::smszFldGrpFk, m_nGroupNumber,
						m_nGroupNumber);
	}
	else
	{
		cszWhere.Format(" %s = %d AND %s = %d ",
						FsTdmfRecAlert::smszFldSrcFk, m_pServerTarget->m_iDbSrvId,
						FsTdmfRecAlert::smszFldGrpFk, m_nGroupNumber,
						m_nGroupNumber);
	}

	return GetDB()->mpRecAlert->FtdCount( cszWhere );

} // CReplicationGroup::GetEventCount ()


BOOL CReplicationGroup::IsEventAt ( long nIndex )
{
	CEvent* pEvent = m_EventList.GetAt(nIndex);

	if ( pEvent == NULL )
	{
		return FALSE;
	}

	return TRUE;

} // CReplicationGroup::GetEventAt ()
// ardev 021129 ^


CEvent* CReplicationGroup::GetEventFirst()
{
	m_EventList.Reset();

	return NULL;
}

CReplicationGroup* CReplicationGroup::GetTargetGroup()
{	
	// Search for same group on target server
	if (m_pServerTarget != NULL)
	{
		std::list<CReplicationGroup>::iterator it;
		for (it = m_pServerTarget->m_listReplicationGroup.begin(); it != m_pServerTarget->m_listReplicationGroup.end(); it++)
		{
			if ((it->m_nGroupNumber == m_nGroupNumber) && (it->m_nType & GT_TARGET))
			{
				return &*it;
			}
		}
	}

	return NULL;
}

CReplicationGroup* CReplicationGroup::CreateAssociatedTargetGroup()
{
	if (m_pServerTarget != NULL)
	{
		CReplicationGroup& RG = m_pServerTarget->AddNewReplicationGroup();

		RG = *this;

		RG.m_iDbTgtFk = m_iDbSrcFk;
		RG.m_pServerTarget = m_pParent;
		RG.m_iDbSrcFk = m_iDbTgtFk;
		RG.m_pParent  = m_pServerTarget;
		
		RG.m_eState = STATE_UNDEF;
		RG.m_nMode = FTD_M_UNDEF;
		RG.m_nConnectionState = FTD_UNDEF;

		RG.m_nType = GT_TARGET;

		return &RG;
	}

	return NULL;
}

bool CReplicationGroup::Copy(CReplicationGroup* pRepGroupSource)
{
	m_strPStoreFile           = pRepGroupSource->m_strPStoreFile;
	m_strJournalDirectory     = pRepGroupSource->m_strJournalDirectory;
	m_bSymmetric              = pRepGroupSource->m_bSymmetric;
	m_nSymmetricGroupNumber   = pRepGroupSource->m_nSymmetricGroupNumber;
	m_bSymmetricNormallyStarted = pRepGroupSource->m_bSymmetricNormallyStarted;
	m_nFailoverInitialState   = pRepGroupSource->m_nFailoverInitialState;
	m_strSymmetricJournalDirectory = pRepGroupSource->m_strSymmetricJournalDirectory;
	m_strSymmetricPStoreFile  = pRepGroupSource->m_strSymmetricPStoreFile;
    m_bChaining               = pRepGroupSource->m_bChaining;
	m_strDescription          = pRepGroupSource->m_strDescription;
	m_nConnectionState        = pRepGroupSource->m_nConnectionState;
    m_strStateTimeStamp       = pRepGroupSource->m_strStateTimeStamp;
	m_listReplicationPair     = pRepGroupSource->m_listReplicationPair;
	m_strThrottles			  = pRepGroupSource->m_strThrottles;
	m_nChunkDelay             = pRepGroupSource->m_nChunkDelay;
	m_nChunkSizeKB            = pRepGroupSource->m_nChunkSizeKB;	
	m_bEnableCompression      = pRepGroupSource->m_bEnableCompression;
    m_bSync                   = pRepGroupSource->m_bSync;
	m_nSyncDepth              = pRepGroupSource->m_nSyncDepth;
	m_nSyncTimeout            = pRepGroupSource->m_nSyncTimeout;
	m_bRefreshNeverTimeout    = pRepGroupSource->m_bRefreshNeverTimeout;
	m_nRefreshTimeoutInterval = pRepGroupSource->m_nRefreshTimeoutInterval;
	m_nMaxFileStatSize        = pRepGroupSource->m_nMaxFileStatSize;
	m_bNetThreshold           = pRepGroupSource->m_bNetThreshold;
	m_nNetMaxKbps             = pRepGroupSource->m_nNetMaxKbps;
	m_nStatInterval           = pRepGroupSource->m_nStatInterval;
	m_bJournalLess            = pRepGroupSource->m_bJournalLess;
   
	m_bPrimaryDHCPAdressUsed		= pRepGroupSource->m_bPrimaryDHCPAdressUsed;
	m_bPrimaryEditedIPUsed			= pRepGroupSource->m_bPrimaryEditedIPUsed;
	m_strPrimaryEditedIP			= pRepGroupSource->m_strPrimaryEditedIP;

	m_bTargetDHCPAdressUsed			= pRepGroupSource->m_bTargetDHCPAdressUsed;
	m_bTargetEditedIPUsed			= pRepGroupSource->m_bTargetEditedIPUsed;
	m_strTargetEditedIP				= pRepGroupSource->m_strTargetEditedIP;

	return true;
}

int CReplicationGroup::SaveTunables()
{
	if (m_nType & GT_SOURCE)
	{
		return GetMMP()->SaveTunables(*this);
	}

	return MMPAPI_Error::OK;
}

int CReplicationGroup::SetTunables()
{
	if (m_nType & GT_SOURCE)
	{
		MMPAPI_Error code;

		code = GetMMP()->SetTunables(*this);

		if (m_bForcePMDRestart)
		{
			// Restart pmd/rmd
			std::ostringstream ossOptions;
			ossOptions << "-g" << m_nGroupNumber;
			if (GetMMP()->tdmfCmd(m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_KILL_PMD, ossOptions.str().c_str()).IsOK())
			{
				GetMMP()->tdmfCmd(m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_LAUNCH_PMD, ossOptions.str().c_str());
			}

			m_bForcePMDRestart = FALSE;
		}

		return code;
	}

	return MMPAPI_Error::OK;
}

void CReplicationGroup::LockCmds()
{
	m_bLockCmds = true;
	m_pParent->m_pParent->m_pParent->m_nNbLockCmds++;
}

void CReplicationGroup::UnlockCmds()
{
	m_bLockCmds = false;
	m_pParent->m_pParent->m_pParent->m_nNbLockCmds--;
}

bool CReplicationGroup::RemoveSymmetricGroup()
{
	if (m_nType & GT_SOURCE)
	{
		return GetMMP()->delSymmetricGroup(*this).IsOK();
	}

	return true;
}

long CReplicationGroup::Failover(long& nWarning)
{
	MMPAPI_Error Err;
	
	nWarning = 0;

	BOOL bPrimaryOnline = m_pParent->m_bConnected;
	// If primary is online
	if (bPrimaryOnline)
	{
		// KILLPMD of the main group
		std::ostringstream ossOptions;
		ossOptions << "-g" << m_nGroupNumber;
		Err = GetMMP()->tdmfCmd(m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_KILL_PMD, ossOptions.str().c_str());
		// STOP the main group
		Err = GetMMP()->tdmfCmd(m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_STOP, ossOptions.str().c_str());
	}

	// Remove the s5##.off flag and put a s###.off
	if (Err.IsOK())
	{
		CScriptServer ScriptServer;
		CString cstrTmp;

		if (bPrimaryOnline)
		{
			// Remove .off file
			ScriptServer.m_pServer = m_pParent;
			cstrTmp.Format("s%03d.off", m_nSymmetricGroupNumber);
			ScriptServer.m_strFileName = cstrTmp;
			ScriptServer.m_strExtension = "";
			ScriptServer.m_strContent = "";
			Err = GetMMP()->SendScriptServerFileToAgent(&ScriptServer);
		}
		else
		{
			nWarning = 1;
		}

		// Send .off file
		ScriptServer.m_pServer = m_pServerTarget;
		cstrTmp.Format("s%03d.off", m_nGroupNumber);
		ScriptServer.m_strFileName = cstrTmp;
		ScriptServer.m_strExtension = "";
		ScriptServer.m_strContent = " ";
		Err = GetMMP()->SendScriptServerFileToAgent(&ScriptServer);
	}

	// The symmetric group will now be the main group and the "0##" - the symmetric one

	//Copy group
	CReplicationGroup RGCopy = *this;
	CServer* pServer = m_pParent;
	CServer* pServerTarget = m_pServerTarget;

	if (Err.IsOK())
	{
		// Remove old normal group
		long nGrpNumber = m_nGroupNumber;
		std::list<CReplicationGroup>::iterator it = m_pParent->m_listReplicationGroup.begin();
		while(it != pServer->m_listReplicationGroup.end())
		{
			if ((it->m_nGroupNumber == nGrpNumber) && (it->m_nType & GT_SOURCE))
			{
				// Remove it from DB
				if (it->m_eObjectState == RPO_SAVED)
				{
					Err = GetMMP()->delRepGroup(*it, false);
				}
				if (Err.IsOK())
				{
					pServer->m_listReplicationGroup.erase(it);
				}

				break; // there's only one group source
			}
			it++;
		}

		// Remove old target group too
		it = pServerTarget->m_listReplicationGroup.begin();
		while(it != pServerTarget->m_listReplicationGroup.end())
		{
			if ((it->m_nGroupNumber == nGrpNumber) && (it->m_nType & GT_TARGET))
			{
				if (Err.IsOK())
				{
					pServerTarget->m_listReplicationGroup.erase(it);
				}

				break; // only one target group
			}
			it++;
		}
	}

	// Create new group (based on copy with switched properties)
	CReplicationGroup& RG = pServerTarget->AddNewReplicationGroup();
	if (Err.IsOK())
	{
		RG = RGCopy;

		long nGroupNumberOld = RG.m_nGroupNumber;
		RG.m_nGroupNumber = RG.m_nSymmetricGroupNumber;
		RG.m_nSymmetricGroupNumber = nGroupNumberOld;
			
		RG.m_pServerTarget = RGCopy.m_pParent;
		int iDbFkOld = RG.m_iDbSrcFk;
		RG.m_iDbSrcFk = RG.m_iDbTgtFk;
		RG.m_iDbTgtFk = iDbFkOld;
		
		long nModeOld = RG.m_nMode;
		RG.m_nMode = RG.m_nSymmetricMode;
		RG.m_nSymmetricMode = nModeOld;
		
		std::string strFileOld = RG.m_strPStoreFile;
		RG.m_strPStoreFile = RG.m_strSymmetricPStoreFile;
		RG.m_strSymmetricPStoreFile = strFileOld;
		
		std::string strDirectoryOld = RG.m_strJournalDirectory;
		RG.m_strJournalDirectory = RG.m_strSymmetricJournalDirectory;
		RG.m_strSymmetricJournalDirectory = strDirectoryOld;

		// Switch devices
		std::list<CReplicationPair>::iterator itRP;
		for (itRP = RG.m_listReplicationPair.begin(); itRP != RG.m_listReplicationPair.end(); itRP++)
		{
			CDevice DeviceSourceOld = itRP->m_DeviceSource;
			itRP->m_DeviceSource = itRP->m_DeviceTarget;
			itRP->m_DeviceTarget = DeviceSourceOld;
			itRP->m_eObjectState = CReplicationPair::RPO_NEW;
		}
		
		Err = RG.GetMMP()->addRepGroup(RG, RG.m_nGroupNumber, RG.m_pServerTarget->m_nHostID, NULL, true, false);
		CReplicationGroup* pRGTarget = RG.CreateAssociatedTargetGroup();
		pRGTarget->m_nMode = RG.m_nMode;
		pRGTarget->m_nConnectionState = RG.m_nConnectionState;
	}
	
	// START the failover (symmetric) group if not started
	if (Err.IsOK() && (RG.m_nMode == FTD_M_UNDEF))
	{
		std::ostringstream ossOptions;
		ossOptions << "-g" << RG.m_nGroupNumber;
		Err = RG.GetMMP()->tdmfCmd(RG.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_START, ossOptions.str().c_str());
	}

	// We can now assume that failover group is started.

	// Set tunables
	if (Err.IsOK())
	{
		Err = RG.GetMMP()->SetTunables(RG);
	}
	
	// Put failover group in Tracking or Passthru mode, depending on user's selection
	if (Err.IsOK())
	{
		std::ostringstream ossOptions;
		ossOptions << "-g " << RG.m_nGroupNumber;
		switch (RG.m_nFailoverInitialState)
		{
		case FTD_MODE_PASSTHRU:
			ossOptions  << " state passthru";
			break;
		case FTD_MODE_TRACKING:
			ossOptions  << " state tracking";
			break;
		}
		Err = RG.GetMMP()->tdmfCmd(RG.m_pParent->m_nHostID, MMP_MNGT_TDMF_CMD_OVERRIDE, ossOptions.str().c_str());
	}

	// Put the original group in PASSTHRU mode.
   	if (bPrimaryOnline && Err.IsOK() && RG.m_bSymmetricNormallyStarted)
	{
		std::ostringstream ossOptions;
		ossOptions << "-g" << RG.m_nSymmetricGroupNumber;
		Err = RGCopy.GetMMP()->tdmfCmd(RG.m_pServerTarget->m_nHostID, MMP_MNGT_TDMF_CMD_START, ossOptions.str().c_str());
		if (Err.IsOK())
		{
			ossOptions.str("");
			ossOptions << "-g " << RG.m_nSymmetricGroupNumber << " state passthru";
			Err = RGCopy.GetMMP()->tdmfCmd(RG.m_pServerTarget->m_nHostID, MMP_MNGT_TDMF_CMD_OVERRIDE, ossOptions.str().c_str());
		}
	}

	return Err;
}
