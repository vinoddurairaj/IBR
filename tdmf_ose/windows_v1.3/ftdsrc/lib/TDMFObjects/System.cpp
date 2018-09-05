// System.cpp : Implementation of CSystem

// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "System.h"
#include "Domain.h"
#include "mmp_api.h"

#include "TDMFObjectsDef.h"
#include "libmngtmsg.h"
#include "FsTdmfRecNvpNames.h"
#include "license.h"

extern "C"
{
#include "ftd_cfgsys.h"
#include "tcp.h"
#include "iputil.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSystem

CSystem::CSystem() : m_db(0), m_hWnd(NULL), m_mmp(0), m_bCheckAccess(true),
		m_nTraceLevel(ONLOG_NOLOGGING), m_eTdmfRole(FsTdmfDbUserInfo::TdmfRoleUndef),
		m_nNbLockCmds(0)
#ifdef _TESTER_WND
		, m_SystemTestWnd(this)
#endif
{
#ifdef _TESTER_WND
	static CString strClassName = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW);
	m_SystemTestWnd.CreateEx(0, strClassName, "TdmfSystemTestWnd",
							 WS_POPUP, CRect(5, 5, 30, 30), NULL, 0);
#endif

 	char   szBuf[MAX_PATH];

	// Read trace info from registry
	if ( cfg_get_software_key_value("DtcObjectsTraceLevel", szBuf, CFG_IS_NOT_STRINGVAL) == CFG_OK )
	{
		m_nTraceLevel = atoi(szBuf);
	}

	m_strName = "System";
    InitializeTheCollectorStatsMember();
}

CSystem::~CSystem()
{
    if ( m_db )
        delete m_db;
    m_db = 0;
    if ( m_mmp )
        delete m_mmp ;
    m_mmp = 0;

	// call one WSACleanup call for every successful WSAStartup
	tcp_cleanup();
}

bool CSystem::GetFirstKeyLogMsg(std::string& strDate, std::string& strHostname, long* pnHostId, std::string& strRegKey, std::string& strExpDate)
{
	if (m_db->mpRecKeyLog->FtdFirst())
	{
		strDate     = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldTs);
		strDate.resize(19); // Chop last ".000"
		strHostname = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldHostname);
		*pnHostId   = atoi(m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldHostId));
		strRegKey   = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldKey);
		strExpDate  = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldExpDate);

		return true;
	}

	return false;
}

bool CSystem::GetNextKeyLogMsg(std::string& strDate, std::string& strHostname, long* pnHostId, std::string& strRegKey, std::string& strExpDate)
{
	if (m_db->mpRecKeyLog->FtdNext())
	{
		strDate     = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldTs);
		strDate.resize(19); // Chop last ".000"
		strHostname = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldHostname);
		*pnHostId   = atoi(m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldHostId));
		strRegKey   = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldKey);
		strExpDate  = m_db->mpRecKeyLog->FtdSel(FsTdmfRecKeyLog::smszFldExpDate);

		return true;
	}

	return false;
}

BOOL CSystem::LogUsersActions()
{
	BOOL bLog = FALSE;

	if (m_db != NULL)
	{
		m_db->mpRecNvp->FtdLock();
		if (m_db->mpRecNvp->FtdPos(TDMF_NVP_NAME_LOG_USERS_ACTIVITIES))
		{
			bLog = atoi(m_db->mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal));
		}
		m_db->mpRecNvp->FtdUnlock();
	}

	return bLog;
}

void CSystem::LogUsersActions(BOOL bLog)
{
	if (m_db != NULL)
	{
		if (!m_db->mpRecNvp->FtdUpdNvp(TDMF_NVP_NAME_LOG_USERS_ACTIVITIES, bLog))
		{
			if (!m_db->mpRecNvp->FtdNew(TDMF_NVP_NAME_LOG_USERS_ACTIVITIES, CString(bLog ? "1" : "0")))
			{
	            CString lszMsg;
		        lszMsg.Format("dbserver_db_update_NVP_PerfCfg(): Cant access <%s>\n<%s>",
							   TDMF_NVP_NAME_LOG_USERS_ACTIVITIES,
							   m_db->FtdGetErrMsg());
				m_db->FtdErrReset();
			} 
		}
    }
}

bool CSystem::GetFirstLogMsg(std::string& strDate, std::string& strSource, std::string& strUser, std::string& strMsg)
{
	if (m_db->mpRecSysLog->FtdFirst())
	{
		strDate   = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldTs);
		strDate.resize(19); // Chop last ".000"
		strSource = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldHostname);
		strUser   = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldUser);
		strMsg    = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldMsg);

		return true;
	}

	return false;
}

bool CSystem::GetNextLogMsg(std::string& strDate, std::string& strSource, std::string& strUser, std::string& strMsg)
{
	if (m_db->mpRecSysLog->FtdNext())
	{
		strDate   = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldTs);
		strDate.resize(19); // Chop last ".000"
		strSource = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldHostname);
		strUser   = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldUser);
		strMsg    = m_db->mpRecSysLog->FtdSel(FsTdmfRecSysLog::smszFldMsg);

		return true;
	}

	return false;
}

bool CSystem::DeleteAllLogMsg()
{
	if (m_db->mpRecSysLog->FtdDelete())
	{
		return true;
	}

	return false;
}

void CSystem::LogUserAction(LPCSTR lpcstrMsg)
{
	USES_CONVERSION;

	if (LogUsersActions())
	{
		///////////////////////////////////////////////////
		// Log info
		
		// Get hostname
		char szHostname[32];
		gethostname(szHostname, 32);
		
		// Get user's name
		CString cstrUserName = OLE2T(m_bstrUser);
		if (cstrUserName.GetLength() == 0)
		{
			char  szName[64];
			DWORD nLength = 64;
			if (GetUserName(szName, &nLength))
			{
				cstrUserName = szName;
			}
		}

		m_db->mpRecSysLog->FtdLock();
		// GMT Date Time, User Name, Message
		m_db->mpRecSysLog->FtdNew(CTime::GetCurrentTime().FormatGmt("%Y-%m-%d %H:%M:%S"), szHostname, cstrUserName, lpcstrMsg);
		m_db->mpRecSysLog->FtdUnlock();
	}
}

void CSystem::Trace(unsigned short thislevel, const char* message, ...)
{
	va_list marker;
	va_start(marker, message);

	TraceOut(thislevel, message, marker);

	va_end(marker);
}

void CSystem::TraceOut(unsigned short thislevel, const char* message, va_list marker)
{
	// Post a trace msg to the gui
	if (thislevel <= m_nTraceLevel)
	{
		CComBSTR bstrMsg;

		COleDateTime DateTime(COleDateTime::GetCurrentTime());
		bstrMsg = DateTime.Format("%Y-%m-%d %H:%M:%S");

		std::ostringstream oss;
		oss << thislevel;
		bstrMsg += CComBSTR("|");
		bstrMsg += CComBSTR(oss.str().c_str());

		char szBuf[MAX_PATH];
		_vsnprintf(szBuf, MAX_PATH, message, marker);
		bstrMsg += CComBSTR("|"); 
		bstrMsg += CComBSTR(szBuf);

		PostMessage(m_hWnd, WM_DEBUG_TRACE, (WPARAM)(bstrMsg.Copy()), (LPARAM)0);
	}
}

CDomain& CSystem::AddNewDomain()
{
	// Create a new domain
	CDomain Domain;
	Domain.m_pParent = this;
	m_listDomain.push_back(Domain);

	return m_listDomain.back();
}

bool CSystem::AlreadyExistDomain(char* pszDomainName, long lKey)
{

    bool bFound = false;
    std::list<CDomain>::iterator itDomain = m_listDomain.begin();
    int nResult;
	while ((itDomain != m_listDomain.end()) && (bFound == false))
	{
        nResult = _stricmp( itDomain->m_strName.c_str(), pszDomainName );

      	if (nResult == 0)
		{
            if(lKey !=  itDomain->GetKey())
            {
			    bFound = true;
                break;
            }

		}

		// Next domain
		itDomain++;
	}

    return bFound;
}

bool CSystem::AddDomain(int iDbKa)
{
	CString cszWhere;
	cszWhere.Format("%s = %d", FsTdmfRecDom::smszFldKa, iDbKa);

	if (m_db->mpRecDom->FtdFirst(cszWhere))
	{
		// Create the domain
		CDomain& Domain = AddNewDomain();
		Domain.Initialize(m_db->mpRecDom, false);

		// Send a notification to the client (GUI)
		PostMessage(m_hWnd, WM_DOMAIN_ADD, iDbKa, 0);

		return true;
	}

	return false;
}

bool CSystem::RemoveDomain(int iDbKa)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			// Send a notification to the client (GUI)
			SendMessage(m_hWnd, WM_DOMAIN_REMOVE, iDbKa, 0);

			// delete it after we remove it from the gui
			m_listDomain.erase(it);

			return true;
		}
		it++;
	}

	return false;
}

bool CSystem::ModifyDomain(int iDbKa)
{
	CString cszWhere;
	cszWhere.Format("%s = %d", FsTdmfRecDom::smszFldKa, iDbKa);

	if (m_db->mpRecDom->FtdFirst(cszWhere))
	{
		std::list<CDomain>::iterator it = m_listDomain.begin();
		while(it != m_listDomain.end())
		{
			if (it->m_iDbK == iDbKa)
			{
				it->Initialize(m_db->mpRecDom, false);

				// Send a notification to the client (GUI)
				PostMessage(m_hWnd, WM_DOMAIN_MODIFY, iDbKa, 0);

				return true;
			}

			it++;
		}
	}

	return false;
}

bool CSystem::AddServerToDomain(int iDbKa, int iSrvId)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			CString cszWhere;
			cszWhere.Format("%s = %d", FsTdmfRecSrvInf::smszFldSrvId, iSrvId);
			if (m_db->mpRecSrvInf->FtdFirst(cszWhere))
			{
				CServer& Server = it->AddNewServer();

				Server.Initialize(m_db->mpRecSrvInf, false);

				// Send a notification to the client (GUI)
				PostMessage(m_hWnd, WM_SERVER_ADD, iDbKa, iSrvId);

				return true;
			}
		}
		it++;
	}

	return false;
}

bool CSystem::MoveServerToDomain(int iDbKa, int iSrvId, int iDbKaNew)
{
	CString szMsg;
	szMsg.Format("Trying to find domain %d and server %d.  Move it to domain %d", iDbKa, iSrvId, iDbKaNew);
	OutputDebugString(szMsg);

	std::list<CDomain>::iterator itDomain = m_listDomain.begin();
	while(itDomain != m_listDomain.end())
	{
		if (itDomain->m_iDbK == iDbKa)
		{
			// Find server
			std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
			while(itServer != itDomain->m_listServer.end())
			{
				if (itServer->m_iDbSrvId == iSrvId)
				{
					// Send a notification to the client (GUI)
					SendMessage(m_hWnd, WM_SERVER_REMOVE, iDbKa, iSrvId);

					// Find new domain
					std::list<CDomain>::iterator itDomainNew = m_listDomain.begin();
					while(itDomainNew != m_listDomain.end())
					{
						if (itDomainNew->m_iDbK == iDbKaNew)
						{
							CServer& Server = itDomainNew->AddNewServer();
							Server = *itServer;
							itServer->m_iDbDomainFk = itDomainNew->m_iDbK;

							// Send a notification to the client (GUI)
							PostMessage(m_hWnd, WM_SERVER_ADD, iDbKaNew, iSrvId);

							break;
						}
						itDomainNew++;
					}

					// Remove from source domain
					itDomain->m_listServer.erase(itServer);

					return true;
				}
				itServer++;
			}
		}
		itDomain++;
	}

	return false;
}

bool CSystem::RemoveServerFromDomain(int iDbKa, int iSrvId)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			// Find server
			std::list<CServer>::iterator itServer = it->m_listServer.begin();
			while(itServer != it->m_listServer.end())
			{
				if (itServer->m_iDbSrvId == iSrvId)
				{
					// Send a notification to the client (GUI)
					SendMessage(m_hWnd, WM_SERVER_REMOVE, iDbKa, iSrvId);

					// delete it after we remove it from the gui
					it->m_listServer.erase(itServer);

					return true;
				}

				itServer++;
			}
		}
		it++;
	}

	return false;
}

bool CSystem::ModifyServerInDomain(int iDbKa, int iSrvId)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			// Find server
			std::list<CServer>::iterator itServer = it->m_listServer.begin();
			while(itServer != it->m_listServer.end())
			{
				if (itServer->m_iDbSrvId == iSrvId)
				{
					CString cszWhere;
					cszWhere.Format("%s = %d", FsTdmfRecSrvInf::smszFldSrvId, iSrvId);
					if (m_db->mpRecSrvInf->FtdFirst(cszWhere))
					{
						itServer->Initialize(m_db->mpRecSrvInf, false);

						// Send a notification to the client (GUI)
						PostMessage(m_hWnd, WM_SERVER_MODIFY, iDbKa, iSrvId);

						return true;
					}
				}
				itServer++;
			}
		}
		it++;
	}

	return false;
}

bool CSystem::AddGroupToServerInDomain(int iDbKa, int iSrvId, int iGrpId)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			// Find server
			std::list<CServer>::iterator itServer = it->m_listServer.begin();
			while(itServer != it->m_listServer.end())
			{
				if (itServer->m_iDbSrvId == iSrvId)
				{
					CString cszWhere;
					cszWhere.Format("%s = %d AND %s = %d", FsTdmfRecLgGrp::smszFldSrcFk, iSrvId, FsTdmfRecLgGrp::smszFldLgGrpId, iGrpId);
					if (m_db->mpRecGrp->FtdFirst(cszWhere))
					{
						CReplicationGroup& RG = itServer->AddNewReplicationGroup();

						RG.Initialize(m_db->mpRecGrp, true);

						// Set target server
						CServer* pServerTarget = NULL;
						std::list<CServer>::iterator itServerTarget = it->m_listServer.begin();
						while(itServerTarget != it->m_listServer.end())
						{
							if (itServerTarget->m_iDbSrvId == RG.m_iDbTgtFk)
							{
								RG.m_pServerTarget = &*itServerTarget;
								break;
							}
							itServerTarget++;
						}
						
						// Send a notification to the client (GUI)
						long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 3 * sizeof(long));
						rglValue[0] = iSrvId;
						rglValue[1] = iGrpId;
						rglValue[2] = TRUE;
						PostMessage(m_hWnd, WM_REPLICATION_GROUP_ADD, iDbKa, (LPARAM)rglValue);
						
						// Add target group
						CReplicationGroup* pRGTarget = RG.CreateAssociatedTargetGroup();
						if (pRGTarget != NULL)
						{
							// Send a notification to the client (GUI)
							long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 3 * sizeof(long));
							rglValue[0] = pRGTarget->m_pParent->m_iDbSrvId;
							rglValue[1] = pRGTarget->m_nGroupNumber;
							rglValue[2] = FALSE;
							PostMessage(m_hWnd, WM_REPLICATION_GROUP_ADD, iDbKa, (LPARAM)rglValue);
						}

						return true;
					}
				}
				itServer++;
			}
		}
		it++;
	}

	return false;
}

bool CSystem::RemoveGroupFromServerInDomain(int iDbKa, int iSrvId, int iGrpId)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			// Find server
			std::list<CServer>::iterator itServer = it->m_listServer.begin();
			while(itServer != it->m_listServer.end())
			{
				if (itServer->m_iDbSrvId == iSrvId)
				{
					// Find Group
					std::list<CReplicationGroup>::iterator itRG = itServer->m_listReplicationGroup.begin();
					while(itRG != itServer->m_listReplicationGroup.end())
					{
						if (itRG->m_nGroupNumber == iGrpId)
						{
							// Send a notification to the client (GUI)
							long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 3 * sizeof(long));
							rglValue[0] = iSrvId;
							rglValue[1] = iGrpId;
							rglValue[2] = TRUE;
							SendMessage(m_hWnd, WM_REPLICATION_GROUP_REMOVE, iDbKa, (LPARAM)rglValue);

							// Also remove target group
							CReplicationGroup* pRGTarget = itRG->GetTargetGroup();
							if (pRGTarget != NULL)
							{
								CServer* pServerTarget = pRGTarget->m_pParent;
								// Send a notification to the client (GUI)
								rglValue = (long*)GlobalAlloc(GMEM_FIXED, 3 * sizeof(long));
								rglValue[0] = pServerTarget->m_iDbSrvId;
								rglValue[1] = pRGTarget->m_nGroupNumber;
								rglValue[2] = FALSE;
								SendMessage(m_hWnd, WM_REPLICATION_GROUP_REMOVE, iDbKa, (LPARAM)rglValue);

								// delete it after we remove it from the gui
								itServer->m_listReplicationGroup.erase(itRG);
							
								// delete target group
								std::list<CReplicationGroup>::iterator itRGTarget = pServerTarget->m_listReplicationGroup.begin();
								while(itRGTarget != pServerTarget->m_listReplicationGroup.end())
								{
									if (itRGTarget->m_nGroupNumber == pRGTarget->m_nGroupNumber)
									{
										pServerTarget->m_listReplicationGroup.erase(itRGTarget);
										break;
									}
									itRGTarget++;
								}
							}

							return true;
						}
						itRG++;
					}
				}
				itServer++;
			}
		}
		it++;
	}

	return false;
}

bool CSystem::ModifyGroupInServerInDomain(int iDbKa, int iSrvId, int iGrpId)
{
	std::list<CDomain>::iterator it = m_listDomain.begin();
	while(it != m_listDomain.end())
	{
		if (it->m_iDbK == iDbKa)
		{
			// Find server
			std::list<CServer>::iterator itServer = it->m_listServer.begin();
			while(itServer != it->m_listServer.end())
			{
				if (itServer->m_iDbSrvId == iSrvId)
				{
					// Find Group
					std::list<CReplicationGroup>::iterator itRG = itServer->m_listReplicationGroup.begin();
					while(itRG != itServer->m_listReplicationGroup.end())
					{
						if (itRG->m_nGroupNumber == iGrpId)
						{
							CString cszWhere;
							cszWhere.Format("%s = %d AND %s = %d", FsTdmfRecLgGrp::smszFldSrcFk, iSrvId, FsTdmfRecLgGrp::smszFldLgGrpId, iGrpId);
							if (m_db->mpRecGrp->FtdFirst(cszWhere))
							{
								itRG->m_listReplicationPair.clear();

								itRG->Initialize(m_db->mpRecGrp, true);

								// Send a notification to the client (GUI)
								long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 3 * sizeof(long));
								rglValue[0] = iSrvId;
								rglValue[1] = iGrpId;
								rglValue[2] = TRUE;
								PostMessage(m_hWnd, WM_REPLICATION_GROUP_MODIFY, iDbKa, (LPARAM)rglValue);

								// Target group
								CReplicationGroup* pRGTarget = itRG->GetTargetGroup();
								if (pRGTarget != NULL)
								{
									pRGTarget->Copy(&*itRG);

									// Send a notification to the client (GUI)
									long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 3 * sizeof(long));
									rglValue[0] = pRGTarget->m_pParent->m_iDbSrvId;
									rglValue[1] = pRGTarget->m_nGroupNumber;
									rglValue[2] = FALSE;
									PostMessage(m_hWnd, WM_REPLICATION_GROUP_MODIFY, iDbKa, (LPARAM)rglValue);
								}

								return true;
							}
						}
						itRG++;
					}
				}
				itServer++;
			}
		}
		it++;
	}

	return false;
}

int CSystem::Initialize(HWND hWnd)
{
	m_hWnd = hWnd;
    char tmp[80];
	CString cszDBServer;
	char szCollectorIP[32];

	enum ErrorCode
	{
		ERR_SUCCESS = 0,
		ERR_DB_OPEN = 1,
		ERR_KEY_COLLECTOR_IP   = 2,
		ERR_KEY_COLLECTOR_PORT = 3,
		ERR_MMP_CREATION = 4,
		ERR_MMP_PROTOCOL = 5,
		ERR_DB_ACCESS    = 6,
		ERR_DISCONNECTED = 7,
		ERR_INVALID_KEY  = 8,
	};

	// Init tcp (for name/ip resolution)
	tcp_startup();

	if (m_strName.length() > 0)
	{
		if (name_is_ipstring((char*)m_strName.c_str()))
		{
			strncpy(szCollectorIP, m_strName.c_str(), 32);
		}
		else
		{	
			unsigned long ip;
			if (name_to_ip((char*)m_strName.c_str(), &ip) == 0)
			{
				ip_to_ipstring(ip, szCollectorIP);
			}
			else
			{
				strncpy(szCollectorIP, "", 32);
			}
		}

		// Get database's info from registry
		cszDBServer = m_strName.c_str();
		cszDBServer += "\\DTC";
	}
	else
	{
		szCollectorIP[0] = '\0';
	}

	m_strDatabaseServer = cszDBServer;
	m_strCollectorIP = szCollectorIP;

	int  iDbPort = 0;
	if ( cfg_get_software_key_value("DtcDbPort", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
	{
		iDbPort = atoi(tmp);
	}

	// Get collector's info from registry    
	if ( cfg_get_software_key_value("DtcDontCheckAccess", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
	{
		m_bCheckAccess = false;
	}

	int  iCollectorPort;
	if ( cfg_get_software_key_value("DtcCollectorPort", tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
        return ERR_KEY_COLLECTOR_PORT;
	m_strCollectorPort = tmp;
	iCollectorPort = atoi(tmp);

	//////////////////////////////
	// Check registration key
	//////////////////////////////
	tmp[0] = '\0';
	if ( cfg_get_software_key_sz_value("DtcCheckKey", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
	{
		if ( cfg_get_software_key_value("DtcRegKey", tmp, 80) == CFG_OK )
		{
			if (strlen(tmp) == 0)
			{
				return ERR_INVALID_KEY;//error
			}
			else
			{
				char* rgszKeys[2];
				rgszKeys[0] = tmp;
				rgszKeys[1] = NULL;

				if (check_key(rgszKeys, np_crypt_key_pmd, strlen(tmp)) != LICKEY_OK)
				{
					return ERR_INVALID_KEY;//error
				}
			}
		}
	}

	//////////////////////////////
	//create permanent link with TDMF DB.
	//////////////////////////////
	if (m_strName.length() > 0)
	{
		m_db = new FsTdmfDb( (BSTR)m_bstrUser, (BSTR)m_bstrPwd, cszDBServer, "DtcDb", iDbPort );
		if ( !m_db->FtdIsOpen() )
		{
			delete m_db;
			m_db = 0;
			return ERR_DB_OPEN;//error
		}
	}
	else
	{
		m_strName = "Not Connected";

		return ERR_DISCONNECTED;
	}

    //////////////////////////////
    //Init communication with TDMF Collector 
    //////////////////////////////

	// Get collector's version from DB
	if (m_db->mpRecNvp->FtdPos(TDMF_NVP_NAME_COLLECTOR_VER))
	{
		m_strCollectorVersion = (LPCTSTR)m_db->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldVal );
	}
	else
	{
		return ERR_DB_ACCESS ;
	}

	// Get collector's Host Id from DB
	if (m_db->mpRecNvp->FtdPos(TDMF_NVP_NAME_COLLECTOR_HOST_ID))
	{
		std::string strHostId = (LPCTSTR)m_db->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldVal );
		std::transform(strHostId.begin(), strHostId.end(), strHostId.begin(), toupper);

		m_strCollectorHostId = "0x";
		m_strCollectorHostId += strHostId;
	}
	else
	{
		return ERR_DB_ACCESS ;
	}

	int nMMPProtocolVersion = 0;
	if (m_db->mpRecNvp->FtdPos(TDMF_NVP_NAME_COLLECTOR_MMP_VER))
	{
		nMMPProtocolVersion = atoi(m_db->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldVal ));
	}

	int nGuiCollectorProtocolVersion = 0;
	if (m_db->mpRecNvp->FtdPos(TDMF_NVP_NAME_GUI_COLLECTOR_PROTOCOL_VER))
	{
		nGuiCollectorProtocolVersion = atoi(m_db->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldVal ));
	}

	if ((MMP_PROTOCOL_VERSION == nMMPProtocolVersion) && 
		(GUI_COLLECTOR_PROTOCOL_VERSION == nGuiCollectorProtocolVersion))
	{
		//create object through which Collector will be accessed
		m_mmp = new MMP_API;
		if ( m_mmp == 0 )
			return ERR_MMP_CREATION;

		m_mmp->initTrace(this);
		m_mmp->initCollectorIPAddr(szCollectorIP);
		m_mmp->initCollectorIPPort(iCollectorPort);
	}
	else
	{
		return ERR_MMP_PROTOCOL;
	}

	return ERR_SUCCESS;
}

void CSystem::Uninitialize()
{
	CString cstrLogMsg;
	cstrLogMsg.Format("Disconnected");
	LogUserAction(cstrLogMsg);
}

long CSystem::Open(char* pszDSN)
{
	CString cstrLogMsg;
	cstrLogMsg.Format("Connected");
	LogUserAction(cstrLogMsg);

	m_mmp->initDB(m_db);

	// Load info from DB  (or at least a part of it)

	// Load all domain
	BOOL bLoop = m_db->mpRecDom->FtdFirst();

	std::map<int, CDomain*> mapDomain;
	std::map<int, CServer*> mapServer;
	std::map<__int64, CReplicationGroup*> mapGroup;
	std::map<__int64, CReplicationGroup*> mapGroupTarget;

    while ( bLoop )
    {   
	    CDomain& newDomain = AddNewDomain();   //domain added to list
        if (newDomain.Initialize(m_db->mpRecDom, false))  //init domain from t_Domain record
		{
			mapDomain[newDomain.m_iDbK] = &newDomain;
		}
        bLoop = m_db->mpRecDom->FtdNext();
    }

	// load all servers
	bLoop = m_db->mpRecSrvInf->FtdFirst("1 = 1"); // We must have a where clause, otherwise it builds one for us
    while ( bLoop )
    {   
	    CServer newServer;
        if (newServer.Initialize(m_db->mpRecSrvInf, false))
		{
			CDomain* pDomain = mapDomain[newServer.m_iDbDomainFk];
			if (pDomain != NULL)
			{
				CServer& Server = pDomain->AddNewServer();
				Server = newServer;

				mapServer[newServer.m_iDbSrvId] = &Server;
			}
		}

        bLoop = m_db->mpRecSrvInf->FtdNext();
    }

	// Load all groups
	bLoop = m_db->mpRecGrp->FtdFirst("1 = 1"); // We must have a where clause, otherwise it builds one for us
    while ( bLoop )
    {   
	    CReplicationGroup newGroup;
        if (newGroup.Initialize(m_db->mpRecGrp, false))
		{
			CServer* pServer = mapServer[newGroup.m_iDbSrcFk];
			if (pServer != NULL)
			{
				CReplicationGroup& Group = pServer->AddNewReplicationGroup();
				Group = newGroup;
				
				__int64 nGroupKey = (((__int64)newGroup.m_iDbSrcFk) << 32) + newGroup.m_nGroupNumber;
				mapGroup[nGroupKey] = &Group;
				
				// Set target server
				CServer* pServerTarget = mapServer[newGroup.m_iDbTgtFk];
				if (pServerTarget != NULL)
				{
					Group.setTarget(pServerTarget);
				
					// Also create a group on target server
					CReplicationGroup& GroupTarget = pServerTarget->AddNewReplicationGroup();
					GroupTarget = newGroup;
					GroupTarget.m_iDbSrcFk = newGroup.m_iDbTgtFk;
					GroupTarget.m_iDbTgtFk = newGroup.m_iDbSrcFk;
					GroupTarget.m_nType = CReplicationGroup::GT_TARGET;
					GroupTarget.setTarget(pServer);
					mapGroupTarget[nGroupKey] = &GroupTarget;
				}
			}
		}

        bLoop = m_db->mpRecGrp->FtdNext();
    }

	// Load all pairs
	bLoop = m_db->mpRecPair->FtdFirst("1 = 1"); // We must have a where clause, otherwise it builds one for us
    while ( bLoop )
    {   
	    CReplicationPair newPair;
        if (newPair.Initialize(m_db->mpRecPair))
		{
			__int64 nGroupKey = (((__int64)newPair.m_iSrcFk) << 32) + newPair.m_iGrpFk;

			CReplicationGroup* pGroup = mapGroup[nGroupKey];
			if (pGroup != NULL)
			{
				CReplicationPair& Pair = pGroup->AddNewReplicationPair();
				Pair = newPair;
				
				// Also add pairs to target group
				CReplicationGroup* pGroupTarget = mapGroupTarget[nGroupKey];
				CReplicationPair& PairTarget = pGroupTarget->AddNewReplicationPair();
				PairTarget = newPair;
			}
		}

        bLoop = m_db->mpRecPair->FtdNext();
    }

	InitializeModeAndStatus();

	LoadCommandMenuEntries();

    m_mmp->registerForNotifications( 2 /*2 sec, default period*/, OnNotificationMsg, this ); 

	return 0;
}

long CSystem::RequestOwnership(BOOL bRequest)
{
	bool bOwnershipGranted = true;
	long nRetCode;
	MMPAPI_Error Err = MMPAPI_Error::OK;

	if (m_bCheckAccess && (m_mmp != NULL))
	{
		if (bRequest)
		{
			Err = m_mmp->requestTDMFOwnership(m_hWnd, &bOwnershipGranted);
		}
		else
		{
			Err = m_mmp->releaseTDMFOwnership(m_hWnd, &bOwnershipGranted);
		}
	}

	if (Err != MMPAPI_Error::OK)
	{
		nRetCode = -1;
	}
	else
	{
		nRetCode = bOwnershipGranted;
	}

	return nRetCode;
}

void CSystem::InitializeModeAndStatus()
{
	std::list<CDomain>::iterator itDomain;
	for (itDomain = m_listDomain.begin(); itDomain != m_listDomain.end(); itDomain++)
	{
		// Compute Domain state
		CElementState StateDomain(STATE_UNDEF);

		std::list<CServer>::iterator itServer;
		for (itServer = itDomain->m_listServer.begin(); itServer != itDomain->m_listServer.end(); itServer++)
		{
			// Compute Server state
			CElementState StateServer(STATE_UNDEF);

			std::list<CReplicationGroup>::iterator itRG;
			for (itRG = itServer->m_listReplicationGroup.begin(); itRG != itServer->m_listReplicationGroup.end(); itRG++)
			{
				CElementState StateRGNew(itRG->m_nMode, itRG->m_nConnectionState);
				itRG->m_eState = StateRGNew.State();

				StateServer += itRG->m_eState;
			}

			// Update Server state
			itServer->m_eState = StateServer.State();

			StateDomain += itServer->m_bConnected ? itServer->m_eState : STATE_ERROR;
		}

		// Update domain state
		itDomain->m_eState = StateDomain.State();
	}
}

void CSystem::ResetModeAndStatus(long nHostID)
{
	std::list<CDomain>::iterator itDomain;
	for (itDomain = m_listDomain.begin(); itDomain != m_listDomain.end(); itDomain++)
	{
		// Compute Domain state
		CElementState StateDomain(STATE_UNDEF);

		std::list<CServer>::iterator itServer;
		for (itServer = itDomain->m_listServer.begin(); itServer != itDomain->m_listServer.end(); itServer++)
		{
			if (itServer->m_nHostID == nHostID)
			{
				// Compute Server state
				CElementState StateServer(STATE_UNDEF);

				std::list<CReplicationGroup>::iterator itRG;
				for (itRG = itServer->m_listReplicationGroup.begin(); itRG != itServer->m_listReplicationGroup.end(); itRG++)
				{
					itRG->m_nMode = FTD_M_UNDEF;
					itRG->m_nConnectionState = STATE_UNDEF;
					CElementState StateRGNew(itRG->m_nMode, itRG->m_nConnectionState);
					itRG->m_eState = StateRGNew.State();

					// Send a Notification
					long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 2 * sizeof(long));
					rglValue[0] = itServer->GetKey();
					rglValue[1] = itRG->m_nGroupNumber;
					PostMessage(m_hWnd, WM_REPLICATION_GROUP_STATE_CHANGE, itDomain->GetKey(), (LPARAM)rglValue);

					StateServer += itRG->m_eState;
				}

				// Update Server state
				itServer->m_eState = StateServer.State();

				// Send a Notification
				PostMessage(m_hWnd, WM_SERVER_STATE_CHANGE, itDomain->GetKey(), itServer->GetKey());
			}

			StateDomain += itServer->m_bConnected ? itServer->m_eState : STATE_ERROR;
		}

		if (itDomain->m_eState != StateDomain.State())
		{
			// Update domain state
			itDomain->m_eState = StateDomain.State();
		
			// Send a notification to the client (GUI)
			PostMessage(m_hWnd, WM_DOMAIN_MODIFY, itDomain->GetKey(), 0);
		}
	}
}

bool CSystem::UpdateModeAndStatus(time_t  StateStimeStamp,
								  long    nHostID,
								  long    nRepGroupNumber,
								  long    nMode,
								  long    nConnectionState,
								  long    nPctDone,
								  __int64 liPStoreSz,
								  __int64 liPStoreDiskTotalSize,
								  __int64 liPStoreDiskFreeSize,
								  __int64 liJournalSz,
								  __int64 liJournalDiskTotalSize,
								  __int64 liJournalDiskFreeSize)

{
	// Find the Host ID of the target server
	long nHostIDTarget = -1;
	{
		bool bTargetFound = false;
		std::list<CDomain>::iterator itDomain = m_listDomain.begin();
		while ((itDomain != m_listDomain.end()) && (bTargetFound == false))
		{
			std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
			while ((itServer != itDomain->m_listServer.end()) && (bTargetFound == false))
			{
				if (itServer->m_nHostID == nHostID)
				{
					std::list<CReplicationGroup>::iterator itRG;
					for (itRG = itServer->m_listReplicationGroup.begin(); itRG != itServer->m_listReplicationGroup.end(); itRG++)
					{
						if ((itRG->m_nGroupNumber == nRepGroupNumber) ||
							(itRG->m_bSymmetric && (itRG->m_nSymmetricGroupNumber == nRepGroupNumber)))
						{
							if(itRG->m_pServerTarget != NULL)
							{
								nHostIDTarget = itRG->m_pServerTarget->m_nHostID;
								bTargetFound = true;
								break;
							}
						}
					}
				}

				// Next Server
				itServer++;
			}

			// Next Domain
			itDomain++;
		}
	}

	// Find server, then replication group
	bool bFoundSource = false;
	bool bFoundTarget = false;

	std::list<CDomain>::iterator itDomain = m_listDomain.begin();
	while ((itDomain != m_listDomain.end()) && (bFoundSource == false) && (bFoundTarget == false))
	{
		bool bServerUpdated = false;

		// Pre-compute Domain state so we won't have to rescan the list
		CElementState StateDomain(STATE_UNDEF);

		std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
		while (itServer != itDomain->m_listServer.end())
		{
			bool bGroupUpdated = false;

			// Pre-compute Server state so we won't have to rescan the list
			CElementState StateServer(STATE_UNDEF);

			if ((itServer->m_nHostID == nHostID) || (itServer->m_nHostID == nHostIDTarget))
			{
				__int64 liPStoreTotalSz = 0;
				__int64 liJournalTotalSz = 0;
				bool    bNotifServer = false;

				std::map<std::string, __int64> mapPStore;
				std::map<std::string, CJournalDrive> mapJournalDrive;

				std::list<CReplicationGroup>::iterator itRG;

				for (itRG = itServer->m_listReplicationGroup.begin(); itRG != itServer->m_listReplicationGroup.end(); itRG++)
				{
					if (itRG->m_nGroupNumber == nRepGroupNumber)
					{
						if (itRG->m_nType & CReplicationGroup::GT_SOURCE)
						{
							bFoundSource = true;
						}
						else
						{
							bFoundTarget = true;
						}

						bool bGroupNotif = false;
						int nModeNew = FTD_M_UNDEF;

						// Change it mode and/or connection state, then compute its state
						BOOL bStarted = (nMode & FTD_MODE_STARTED);

						if (bStarted)
						{
							BOOL bCheckpoint = (nMode & FTD_MODE_CHECKPOINT);

							nModeNew = nMode & ~(FTD_MODE_CHECKPOINT | FTD_MODE_STARTED);
							if (bCheckpoint && (nModeNew == FTD_MODE_NORMAL))
							{
								nModeNew = FTD_MODE_CHECKPOINT;
							}
						}

						if (nModeNew != itRG->m_nMode)
						{
							itRG->m_nMode = nModeNew;
							bGroupNotif = true;
						}

						std::string strStateTimeStamp = (LPCTSTR)CTime(StateStimeStamp).Format("%Y-%m-%d %H:%M:%S");
						if (strStateTimeStamp != itRG->m_strStateTimeStamp)
						{
							itRG->m_strStateTimeStamp = strStateTimeStamp;
							bGroupNotif = true;
						}

						if (nConnectionState != itRG->m_nConnectionState)
						{
							itRG->m_nConnectionState = nConnectionState;
							bGroupNotif = true;
						}

						if (nPctDone != itRG->m_nPctDone)
						{
							itRG->m_nPctDone = nPctDone;
							bGroupNotif = true;
						}

						if (liJournalSz != itRG->m_liJournalSize)
						{
							itRG->m_liJournalSize = liJournalSz;
							bGroupNotif = true;
							bNotifServer = true;
						}

						if (liPStoreSz != itRG->m_liPStoreSize)
						{
							itRG->m_liPStoreSize = liPStoreSz;
							bGroupNotif = true;
							bNotifServer = true;
						}

						if (liJournalDiskTotalSize != itRG->m_liDiskTotalSize)
						{
							itRG->m_liDiskTotalSize = liJournalDiskTotalSize;
							bGroupNotif = true;
							bNotifServer = true;
						}

						if (liJournalDiskFreeSize != itRG->m_liDiskFreeSize)
						{
							itRG->m_liDiskFreeSize = liJournalDiskFreeSize;
							bGroupNotif = true;
							bNotifServer = true;
						}

						// Check if the state has changed
						CElementState StateRGNew(itRG->m_nMode, itRG->m_nConnectionState);
						if (StateRGNew.State() != itRG->m_eState)
						{
							itRG->m_eState = StateRGNew.State();
							bGroupUpdated = true;
							bGroupNotif = true;
						}

						// Send a notification to the client (GUI)
						if (bGroupNotif)
						{
							long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 2 * sizeof(long));
							rglValue[0] = itServer->GetKey();
							rglValue[1] = nRepGroupNumber;
							PostMessage(m_hWnd, WM_REPLICATION_GROUP_STATE_CHANGE, itDomain->GetKey(), (LPARAM)rglValue);
						}
					}
					else if (itRG->m_bSymmetric && (itRG->m_nSymmetricGroupNumber == nRepGroupNumber))
					{
						if (itRG->m_nType & CReplicationGroup::GT_SOURCE)
						{
							bFoundSource = true;
							bFoundTarget = true;

							//bool bGroupNotif = false;
							int nModeNew = FTD_M_UNDEF;

							// Change it mode and/or connection state, then compute its state
							BOOL bStarted = (nMode & FTD_MODE_STARTED);
							
							if (bStarted)
							{
								BOOL bCheckpoint = (nMode & FTD_MODE_CHECKPOINT);
								
								nModeNew = nMode & ~(FTD_MODE_CHECKPOINT | FTD_MODE_STARTED);
								if (bCheckpoint && (nModeNew == FTD_MODE_NORMAL))
								{
									nModeNew = FTD_MODE_CHECKPOINT;
								}
							}
							
							if (nModeNew != itRG->m_nSymmetricMode)
							{
								itRG->m_nSymmetricMode = nModeNew;
								//bGroupNotif = true;
							}
							
							if (nConnectionState != itRG->m_nSymmetricConnectionState)
							{
								itRG->m_nSymmetricConnectionState = nConnectionState;
								//bGroupNotif = true;
							}

							// Not necessary for now, symmetric mode/state not shown in real-time
							// Send a notification to the client (GUI)
							//if (bGroupNotif)
							//{
							//}
						}
					}
					
					// Update Server's sizes
					if (itRG->m_nType & CReplicationGroup::GT_SOURCE)
					{
						if (mapPStore.find(itRG->m_strPStoreFile) == mapPStore.end())
						{
							liPStoreTotalSz += itRG->m_liPStoreSize;
							mapPStore[itRG->m_strPStoreFile] = itRG->m_liPStoreSize; 
						}
					}

					if (itRG->m_nType & CReplicationGroup::GT_TARGET)
					{
						std::string strDrive = itRG->m_strJournalDirectory.substr(0, itRG->m_strJournalDirectory.find('\\'));
						if (strstr(itRG->m_pParent->m_strOSType.c_str(), "Windows") != NULL) 
						{
							std::transform(strDrive.begin(), strDrive.end(), strDrive.begin(), toupper);
						}

						mapJournalDrive[strDrive].m_liJournalTotalSize += itRG->m_liJournalSize;
						if (itRG->m_liDiskTotalSize > 0)
						{
							mapJournalDrive[strDrive].m_liDiskTotalSize = itRG->m_liDiskTotalSize;
						}
						if (itRG->m_liDiskFreeSize > 0)
						{
							mapJournalDrive[strDrive].m_liDiskFreeSize = itRG->m_liDiskFreeSize;
						}
					}

					StateServer += itRG->m_eState;
				}

				// Server's PStore and Journal size
				if (liPStoreTotalSz != itServer->m_liPStoreSize)
				{
					itServer->m_liPStoreSize = liPStoreTotalSz;
					bNotifServer = true;
				}
				if (bNotifServer)
				{
					itServer->m_mapJournalDrive.clear();
					itServer->m_mapJournalDrive = mapJournalDrive;

					// Send a Notification
					PostMessage(m_hWnd, WM_SERVER_STATE_CHANGE, itDomain->GetKey(), itServer->GetKey());
				}
			}

			// Propagate state change: Check if the server state has changed
			if (bGroupUpdated)
			{
				if (itServer->m_eState != StateServer.State())
				{
					itServer->m_eState = StateServer.State();
					bServerUpdated = true;

					// Send a notification to the client (GUI)
					PostMessage(m_hWnd, WM_SERVER_STATE_CHANGE, itDomain->GetKey(), itServer->GetKey());
				}
			}

			StateDomain += itServer->m_bConnected ? itServer->m_eState : STATE_ERROR;

			// Next Server
			itServer++;
		}

		// Propagate state change: Check if the domain state has changed
		if (bServerUpdated)
		{
			if (itDomain->m_eState != StateDomain.State())
			{
				itDomain->m_eState = StateDomain.State();

				// Send a notification to the client (GUI)
				PostMessage(m_hWnd, WM_DOMAIN_MODIFY, itDomain->GetKey(), 0);
			}
		}

		// Next domain
		itDomain++;
	}

	return true;
}

bool CSystem::UpdateServerConnectionState(long nHostID, BOOL bConnected, int nBABSizeAllocated, int nBABSizeRequested)
{
	bool bUpdated = false;

	// Find server, then replication group
	bool bFound = false;

	std::list<CDomain>::iterator itDomain = m_listDomain.begin();
	while ((itDomain != m_listDomain.end()) && (bFound == false))
	{
		std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
		while ((itServer != itDomain->m_listServer.end()) && (bFound == false))
		{
			if (itServer->m_nHostID == nHostID)
			{
				if (itServer->m_bConnected != bConnected)
				{
					itServer->m_bConnected = bConnected;
					// Send a notification to the client (GUI)
					PostMessage(m_hWnd, WM_SERVER_CONNECTION_CHANGE, itDomain->m_iDbK, itServer->GetKey());

					if (!itServer->m_bConnected)
					{
						// Reset Allocated BAB size value; Collector will resend it
						itServer->m_nBABSizeAct = 0;
					}
					else
					{
						if (itServer->m_bConnected)
						{
							itServer->m_nBABSizeAct = nBABSizeAllocated;
							itServer->m_nBABSizeReq = nBABSizeRequested;
							// Send a notification to the client (GUI)
							PostMessage(m_hWnd, WM_SERVER_MODIFY, itDomain->m_iDbK, itServer->GetKey());

							if (itServer->m_nBABSizeAct != itServer->m_nBABSizeReq)
							{
								// Send a notification to the client (GUI)
								PostMessage(m_hWnd, WM_SERVER_BAB_NOT_OPTIMAL, itDomain->m_iDbK, itServer->GetKey());
							}
						}
					}
				}

				bFound = true;

				itDomain->UpdateStatus();
			}

			// Next Server
			itServer++;
		}

		// Next domain
		itDomain++;
	}

	if (bFound == false)
	{
		// Read it from DB
        FsTdmfRecSrvInf *pRecSrvInf = m_db->mpRecSrvInf;

        CString cszWhere;
        cszWhere.Format( " %s = %d ", FsTdmfRecSrvInf::smszFldHostId, nHostID);
        if (pRecSrvInf->FtdFirst( cszWhere ))
		{
			// Check if domain already exists
			int iDomainFk = atoi(pRecSrvInf->FtdSel(FsTdmfRecSrvInf::smszFldDomFk));

			CDomain* pDomain = NULL;
			bool bFound = false;
			std::list<CDomain>::iterator itDomain = m_listDomain.begin();
			while ((itDomain != m_listDomain.end()) && (bFound == false))
			{
				if (itDomain->m_iDbK == iDomainFk)
				{
					bFound = true;
					pDomain = &*itDomain;
				}

				itDomain++;
			}
			if (!bFound)
			{
				CString cszWhere;
				cszWhere.Format("%s = %d", FsTdmfRecDom::smszFldKa, iDomainFk);
				if (m_db->mpRecDom->FtdFirst(cszWhere))
				{
					// Create the domain
					CDomain& Domain = AddNewDomain();
					Domain.Initialize(m_db->mpRecDom, false);
					pDomain = &Domain;

					// Send a notification to the client (GUI)
					PostMessage(m_hWnd, WM_DOMAIN_ADD, pDomain->GetKey(), 0);
				}
			}

			// Add server to domain
			if (pDomain != NULL)
			{
				CServer& Server = pDomain->AddNewServer();

				CString cszWhere;
				cszWhere.Format("%s = %d", FsTdmfRecSrvInf::smszFldHostId, nHostID);
				if (m_db->mpRecSrvInf->FtdFirst(cszWhere))
				{
					Server.Initialize(pRecSrvInf, false);

					// Send a notification to the client (GUI)
					PostMessage(m_hWnd, WM_SERVER_ADD, pDomain->GetKey(), Server.GetKey());

					Server.m_bConnected = bConnected;
					// Send a notification to the client (GUI)
					PostMessage(m_hWnd, WM_SERVER_CONNECTION_CHANGE, pDomain->GetKey(), Server.GetKey());
				}
			}
		}
	}

	return bUpdated;
}

bool CSystem::UpdatePerformanceData(time_t  StateStimeStamp,
									long    nHostID,
									long    nGroupNumber,
									__int64 nReadKbps,
									__int64 nWriteKbps,
									__int64 nActualNet,
									__int64 nEffectiveNet,
									int     nEntries,
									int     nPctBAB,
									int     nPctDone,
									int     nSectors)
{
	bool bUpdated = false;

	// Find server, then replication group
	bool bFound = false;

	std::list<CDomain>::iterator itDomain = m_listDomain.begin();
	while ((itDomain != m_listDomain.end()) && (bFound == false))
	{
		std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
		while ((itServer != itDomain->m_listServer.end()) && (bFound == false))
		{
			if (itServer->m_nHostID == nHostID)
			{
				long nServerPctBAB  = 100;
				long nServerEntries = 0;

				std::list<CReplicationGroup>::iterator itRG;
				for (itRG = itServer->m_listReplicationGroup.begin(); itRG != itServer->m_listReplicationGroup.end(); itRG++)
				{
					if (itRG->m_nGroupNumber == nGroupNumber)
					{
						bFound = true;
						std::string strTimeStamp = (LPCTSTR)CTime(StateStimeStamp).Format("%Y-%m-%d %H:%M:%S");

						// Check if the perf data have changed
						if ((itRG->m_nReadKbps != nReadKbps) ||
							(itRG->m_nWriteKbps != nWriteKbps) ||
							(itRG->m_nActualNet != nActualNet) ||
							(itRG->m_nEffectiveNet != nEffectiveNet) ||
							(itRG->m_nBABEntries != nEntries) || 
							(itRG->m_nPctBAB != nPctBAB) || 
							(itRG->m_nPctDone != nPctDone) ||
							(itRG->m_strStateTimeStamp != strTimeStamp))
						{
							itRG->m_nReadKbps     = nReadKbps;
							itRG->m_nWriteKbps    = nWriteKbps;
							itRG->m_nActualNet    = nActualNet;
							itRG->m_nEffectiveNet = nEffectiveNet;
							itRG->m_nPctBAB       = nPctBAB;
							itRG->m_nBABEntries   = nEntries;
							itRG->m_nPctDone      = nPctDone;

							itRG->m_strStateTimeStamp = (LPCTSTR)CTime(StateStimeStamp).Format("%Y-%m-%d %H:%M:%S");
							bUpdated = true;

							// Send a notification to the client (GUI)
							long *rglValue = (long*)GlobalAlloc(GMEM_FIXED, 2 * sizeof(long));

							rglValue[0] = itServer->GetKey();

							rglValue[1] = nGroupNumber;
							PostMessage(m_hWnd, WM_REPLICATION_GROUP_PERF_CHANGE, itDomain->GetKey(), (LPARAM)rglValue);

							// We have lately introduce the concept of having the target group
							// listed under the server.  We have to update the perf of these
							// groups.
							// If it's a source group, update also the perf of the group
							// listed under the target server.
							if ((itRG->m_nType & CReplicationGroup::GT_SOURCE)
								&& !(itRG->m_nType & CReplicationGroup::GT_LOOPBACK))  // No need to send it twice if in loopback
							{
								UpdatePerformanceData(StateStimeStamp, itRG->m_pServerTarget->m_nHostID, 
													  nGroupNumber, nReadKbps, nWriteKbps, nActualNet, nEffectiveNet,
													  nEntries, nPctBAB, nPctDone, nSectors);
							}
						}
					}

					if (itRG->m_nPctBAB < nServerPctBAB)
					{
						nServerPctBAB = itRG->m_nPctBAB;
					}

					if (itRG->m_nType & CReplicationGroup::GT_SOURCE)
					{
						nServerEntries += itRG->m_nBABEntries;
					}
				}

				// Save server's performance data
				if (bUpdated)
				{
					itServer->m_nPctBAB  = nServerPctBAB;
					itServer->m_nEntries = nServerEntries;
					
					// Send a notification to the client (GUI)
					PostMessage(m_hWnd, WM_SERVER_PERF_CHANGE, itDomain->GetKey(), nHostID);
				}
			}

			// Next Server
			itServer++;
		}

		// Next domain
		itDomain++;
	}

	return bUpdated;
}
									
bool CSystem::UpdateObjects(BYTE* pbMsg)
{
	TdmfGuiObjectChangeMessage ObjectChangeMsg;
	memcpy(&ObjectChangeMsg, pbMsg, sizeof(ObjectChangeMsg));
	
	// Update OM
	switch (ObjectChangeMsg.iModule)
	{
	case TDMF_DOMAIN:
		{
			switch (ObjectChangeMsg.iOp)
			{
			case TDMF_ADD_NEW:
				AddDomain(ObjectChangeMsg.iDbDomain);
				break;
			case TDMF_MODIFY:
				ModifyDomain(ObjectChangeMsg.iDbDomain);
				break;
			case TDMF_REMOVE:
				RemoveDomain(ObjectChangeMsg.iDbDomain);
				break;
			}
		}
		break;
		
	case TDMF_SERVER:
		{
			switch (ObjectChangeMsg.iOp)
			{
			case TDMF_MOVE:
				MoveServerToDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId, ObjectChangeMsg.iGroupNumber);
				break;
			case TDMF_ADD_NEW:
				AddServerToDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId);
				break;
			case TDMF_MODIFY:
				ModifyServerInDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId);
				break;
			case TDMF_REMOVE:
				RemoveServerFromDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId);
				break;
			}
		}
		break;
		
	case TDMF_GROUP:
		{
			switch (ObjectChangeMsg.iOp)
			{
			case TDMF_ADD_NEW:
				AddGroupToServerInDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId, ObjectChangeMsg.iGroupNumber);
				break;
			case TDMF_MODIFY:
				ModifyGroupInServerInDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId, ObjectChangeMsg.iGroupNumber);
				break;
			case TDMF_REMOVE:
				RemoveGroupFromServerInDomain(ObjectChangeMsg.iDbDomain, ObjectChangeMsg.iDbSrvId, ObjectChangeMsg.iGroupNumber);
				break;
			}
		}
		break;
	}

	return true;
}

bool CSystem::FowardTextMessage(UINT nID, BYTE* pbMsg)
{
	SendMessage(m_hWnd, WM_TEXT_MESSAGE, nID, (LPARAM)pbMsg);
		
	return true;
}

/*static*/
void CSystem::OnNotificationMsg(void* pThis, TDMFNotificationMessage* pNotificationMessage)
{
	CSystem* pSystem = (CSystem*)pThis;

	// We can receive more than one message at once
    const char *data    = pNotificationMessage->getData();
    const char *dataend = pNotificationMessage->getData() + pNotificationMessage->getLength();
 

	// Messages with no data
	if (data == dataend)
	{
		enum NotificationMsgTypes eType = pNotificationMessage->getType();

		if (eType == NOTIF_MSG_COLLECTOR_CONNECTED)
		{
			pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_COLLECTOR_CONNECTED message received");
			PostMessage(pSystem->m_hWnd, WM_COLLECTOR_COMMUNICATION_STATUS, 0, 0);
		}
		else if (eType == NOTIF_MSG_COLLECTOR_UNABLE_TO_CONNECT)
		{
			pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_COLLECTOR_UNABLE_TO_CONNECT message received");
			PostMessage(pSystem->m_hWnd, WM_COLLECTOR_COMMUNICATION_STATUS, 1, 0);
		}
		else if (eType == NOTIF_MSG_COLLECTOR_COMMUNICATION_PROBLEM)
		{
			pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_COLLECTOR_COMMUNICATION_PROBLEM message received");
			PostMessage(pSystem->m_hWnd, WM_COLLECTOR_COMMUNICATION_STATUS, 2, 0);
		}
	}

	// Other messages
    while (data < dataend)
    {
		enum NotificationMsgTypes eType = pNotificationMessage->getType();

        if (eType == NOTIF_MSG_RG_RAW_PERF_DATA)
        {
            if (data + sizeof(mnep_NotifMsgDataRepGrpRawPerf) <= dataend)
			{
				mnep_NotifMsgDataRepGrpRawPerf* pMsgData = (mnep_NotifMsgDataRepGrpRawPerf*)data;

				pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_RG_RAW_PERF_DATA for (0x%x, %d) at %d: "
							   "Read = %I64d, Written = %I64d, Actual = %I64d, Effective = %I64d, "
							   "Entries = %d, % BAB = %d",
							   pMsgData->agentUID.iHostID,
							   pMsgData->perf.lgnum,
							   pMsgData->agentUID.tStimeTamp,
							   pMsgData->perf.bytesread,
							   pMsgData->perf.byteswritten,
							   pMsgData->perf.actual,
							   pMsgData->perf.effective,
							   pMsgData->perf.entries,
							   pMsgData->perf.pctbab);

				// Update Objects
				pSystem->UpdatePerformanceData(pMsgData->agentUID.tStimeTamp,
											   pMsgData->agentUID.iHostID,
											   pMsgData->perf.lgnum,
											   pMsgData->perf.bytesread,
											   pMsgData->perf.byteswritten,
											   pMsgData->perf.actual,
											   pMsgData->perf.effective,
											   pMsgData->perf.entries,
											   pMsgData->perf.pctbab,
											   pMsgData->perf.pctdone,
											   pMsgData->perf.sectors);
			}
            data += sizeof(mnep_NotifMsgDataRepGrpRawPerf);
        }
        else if (eType == NOTIF_MSG_RG_STATUS_AND_MODE)
        {
            if (data + sizeof(mnep_NotifMsgDataRepGrpStatusMode) <= dataend)
			{
				mnep_NotifMsgDataRepGrpStatusMode* pMsgData = (mnep_NotifMsgDataRepGrpStatusMode*)data;

				pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_RG_STATUS_AND_MODE for (0x%x, %d) at %d:"
							   "Mode = %d, State = %d, % Done = %d, "
							   "PStore (Size = %I64d Disk Total Size = %I64d Disk Free Size = %I64d), "
							   "Journal (Size = %I64d Disk Total Size = %I64d Disk Free Size = %I64d)",
							   pMsgData->agentUID.iHostID,
							   pMsgData->sRepGroupNumber,
							   pMsgData->agentUID.tStimeTamp,
							   pMsgData->sState, // Mode
							   pMsgData->sConnection, // Status
							   pMsgData->sPctDone,
							   pMsgData->liPStoreSz,
							   pMsgData->liPStoreDiskTotalSz,
							   pMsgData->liPStoreDiskFreeSz,
							   pMsgData->liJournalSz,
							   pMsgData->liJournalDiskTotalSz,
							   pMsgData->liJournalDiskFreeSz);

				// Update Objects
				pSystem->UpdateModeAndStatus(pMsgData->agentUID.tStimeTamp,
											 pMsgData->agentUID.iHostID,
										     pMsgData->sRepGroupNumber,
											 pMsgData->sState, // Mode
											 pMsgData->sConnection, // Status
											 pMsgData->sPctDone,
											 pMsgData->liPStoreSz,
											 pMsgData->liPStoreDiskTotalSz,
											 pMsgData->liPStoreDiskFreeSz,
											 pMsgData->liJournalSz,
											 pMsgData->liJournalDiskTotalSz,
											 pMsgData->liJournalDiskFreeSz);
			}
            data += sizeof(mnep_NotifMsgDataRepGrpStatusMode);
        }
		else if ((eType == NOTIF_MSG_SERVER_ACTIVITY) || (eType == NOTIF_MSG_SERVER_NO_ACTIVITY))
		{
            if (data + sizeof(mnep_NotifMsgDataServerInfo) <= dataend)
			{
				mnep_NotifMsgDataServerInfo* pMsgData = (mnep_NotifMsgDataServerInfo*)data;

				pSystem->Trace(ONLOG_EXTENDED, (eType == NOTIF_MSG_SERVER_ACTIVITY) ? 
							   "NOTIF_MSG_SERVER_ACTIVITY message received for 0x%x" :
							   "NOTIF_MSG_SERVER_NO_ACTIVITY message receivedfor 0x%x",
							   pMsgData->agentUID.iHostID);

				// Update Objects
				pSystem->UpdateServerConnectionState(pMsgData->agentUID.iHostID, (eType == NOTIF_MSG_SERVER_ACTIVITY), pMsgData->nBABAllocated, pMsgData->nBABRequested);

				if (eType == NOTIF_MSG_SERVER_NO_ACTIVITY)
				{
					//pSystem->ResetModeAndStatus(pMsgData->iHostID);
				}
			}
            data += sizeof(mnep_NotifMsgDataServerInfo);
		}
		else if (eType == NOTIF_MSG_GUI_MSG)
		{
			if (data + sizeof(mmp_TdmfGuiMsg) <= dataend)
			{
				mmp_TdmfGuiMsg* pMsg = (mmp_TdmfGuiMsg*)data;

				DWORD nID;
				DWORD nType;
				BYTE* pbMsg = NULL;
				pSystem->GetMMP()->RetrieveTdmfGuiMessage(pMsg, &nID, &nType, &pbMsg);

				pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_GUI_MSG message received from 0x%x", nID);

				if ((nID != GetCurrentProcessId()) && (nType == TDMFGUI_OBJECT_CHANGE))
				{
					pSystem->Trace(ONLOG_EXTENDED, "TDMFGUI_OBJECT_CHANGE message received");

					pSystem->UpdateObjects(pbMsg);
				}
				else if (nType == TDMFGUI_TEXT_MESSAGE)
				{
					pSystem->FowardTextMessage(nID, pbMsg);
				}
			}
			data += sizeof(mmp_TdmfGuiMsg);
		}
        else if (eType == NOTIF_MSG_COLLECTOR_STAT)
		{
           if (data + sizeof(mmp_TdmfCollectorState) <= dataend)
			{
		
			    mmp_TdmfCollectorState* pCollectorStatsMsg = (mmp_TdmfCollectorState*)data;

                if(pCollectorStatsMsg != NULL)
                {
                    pSystem->SetTdmfCollectorState(pCollectorStatsMsg);

                    pSystem->Trace(ONLOG_EXTENDED, "NOTIF_MSG_COLLECTOR_STAT message received");
		             
                    PostMessage(pSystem->m_hWnd, WM_RECEIVED_STATISTICS_DATA, 0, 0);
               
                }
           	}
			data += sizeof(mmp_TdmfCollectorState);
        
		}
        else 
        {
			pSystem->Trace(ONLOG_EXTENDED, "Undefined message received");
            _ASSERT(0);
			data = dataend;
        }
    }

	// Delete the notification message
	if (pNotificationMessage != NULL)
	{
		delete pNotificationMessage;
	}
}

CEvent* CSystem::GetEventAt ( long nIndex )
{
	CEvent* pEvent = m_EventList.GetAt(nIndex);

	if ( pEvent == NULL )
	{
		pEvent = m_EventList.ReadRangeFromDb( this, nIndex );
	}

	return pEvent;

} // CSystem::GetEventAt ()


long CSystem::GetEventCount ()
{
	return m_db->mpRecAlert->FtdCount();

} // CSystem::GetEventCount ()


BOOL CSystem::IsEventAt ( long nIndex )
{
	CEvent* pEvent = m_EventList.GetAt(nIndex);

	if ( pEvent == NULL )
	{
		return FALSE;
	}

	return TRUE;

} // CSystem::GetEventAt ()


// This Should Be GetEventInit ()
CEvent* CSystem::GetEventFirst ()
{
	m_EventList.Reset();

	return NULL;

} // CSystem::GetEventFirst ()

std::string CSystem::GetServerName ( long plSrvId )
{
	if ( plSrvId < 0 )
	{
		return "";
	}
	else if ( m_mapServerName.find ( plSrvId ) != m_mapServerName.end () )
	{
		return m_mapServerName[plSrvId];
	}
	else
	{
		// Check in internal structure
		for (std::list<CDomain>::iterator itDomain = m_listDomain.begin();
			 itDomain != m_listDomain.end(); itDomain++)
		{
			for (std::list<CServer>::iterator itServer = itDomain->m_listServer.begin();
				 itServer != itDomain->m_listServer.end(); itServer++)
			{
				if (itServer->m_iDbSrvId == plSrvId)
				{
					m_mapServerName[plSrvId] = itServer->m_strName;
					return itServer->m_strName;
				}
			}
		}

		return "";
	}
} // CSystem::GetServerName ()

std::string CSystem::GetTableSize(short Table, long* pCount)
{
	std::ostringstream oss;
	int size = 0;
	double fSize = 0.0f;
	oss.str("");

	switch(Table)
	{
	case _TDMF_DB_ALL_TABLES:
		oss << (LPCTSTR)GetDB()->mpRecDom->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecSrvInf->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecGrp->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecPair->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecPerf->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecHum->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecAlert->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecCmd->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());

		oss.str("");
		oss << (LPCTSTR)GetDB()->mpRecNvp->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
	
		break;
	case _TDMF_DB_DOMAIN_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecDom->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecDom->FtdCount();
		break;
	case _TDMF_DB_SERVER_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecSrvInf->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecSrvInf->FtdCount();
		break;
	case _TDMF_DB_GROUP_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecGrp->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecGrp->FtdCount();
		break;
	case _TDMF_DB_PAIR_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecPair->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecPair->FtdCount();
		break;
	case _TDMF_DB_PERFORMANCE_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecPerf->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecPerf->FtdCount();
		break;
	case _TDMF_DB_PEOPLE_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecHum->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecHum->FtdCount();
		break;
	case _TDMF_DB_ALERT_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecAlert->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecAlert->FtdCount();
		break;
	case _TDMF_DB_COMMAND_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecCmd->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecCmd->FtdCount();
		break;
	case _TDMF_DB_PARAMETERS_TABLE:
		oss << (LPCTSTR)GetDB()->mpRecNvp->FtdGetTableSize();
		oss.str().erase(oss.str().length()-3, oss.str().length()-1);
		size += atoi(oss.str().c_str());
		*pCount = GetDB()->mpRecNvp->FtdCount();
		break;
	default:
		oss << "-3 KB";
		*pCount = 0;;
	}


	if(size >= 0)
	{
		oss.str("");
		fSize += size;
 		CString strValue;    

		if(fSize >= 1000)
		{
			if(fSize >= 1000000)
			{
				fSize = fSize / 1000000;
                strValue.Format(_T("%.2f"), fSize);

 				oss << (LPCTSTR)strValue << " GB";
			}
			else
			{
				fSize = fSize / 1000;
                strValue.Format(_T("%.1f"), fSize);
 				oss << (LPCTSTR)strValue << " MB";
			}
		}
		else
		{
            strValue.Format(_T("%.0f"), fSize);
 			oss << (LPCTSTR)strValue << " KB";
		}
	}

	return oss.str();
}

bool CSystem::DeleteTableRecords(short Table)
{
	BOOL result;
	GetDB()->BeginTrans();

	switch(Table)
	{
	case _TDMF_DB_ALL_TABLES:
		result = GetDB()->mpRecDom->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecSrvInf->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecGrp->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecPair->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecPerf->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecHum->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecAlert->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecCmd->FtdDeleteTableRecords();
		if(!result)
			break;

		result = GetDB()->mpRecNvp->FtdDeleteTableRecords();
		if(!result)
			break;

		break;
	case _TDMF_DB_DOMAIN_TABLE:
		result = GetDB()->mpRecDom->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_SERVER_TABLE:
		result = GetDB()->mpRecSrvInf->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_GROUP_TABLE:
		result = GetDB()->mpRecGrp->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_PAIR_TABLE:
		result = GetDB()->mpRecPair->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_PERFORMANCE_TABLE:
		result = GetDB()->mpRecPerf->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_PEOPLE_TABLE:
		result = GetDB()->mpRecHum->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_ALERT_TABLE:
		result = GetDB()->mpRecAlert->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_COMMAND_TABLE:
		result = GetDB()->mpRecCmd->FtdDeleteTableRecords();
		break;
	case _TDMF_DB_PARAMETERS_TABLE:
		result = GetDB()->mpRecNvp->FtdDeleteTableRecords();
		break;
	default:
		result = FALSE;
	}

	if(result)
	{
		GetDB()->CommitTrans();
		return true;
	}
	else
	{
		GetDB()->Rollback();
		return false;
	}
}

void CSystem::SetTdmfCollectorState( mmp_TdmfCollectorState* pTdmfCollectorState) 
{ 
    if(pTdmfCollectorState)
    {
        m_CollectorStatsMsg.CollectorTime =  pTdmfCollectorState->CollectorTime;
        m_CollectorStatsMsg.NbAgentsAlive = pTdmfCollectorState->NbAgentsAlive;
        m_CollectorStatsMsg.NbAliveMsgPerHr = pTdmfCollectorState->NbAliveMsgPerHr;
        m_CollectorStatsMsg.NbAliveMsgPerMn = pTdmfCollectorState->NbAliveMsgPerMn;
        m_CollectorStatsMsg.NbPDBMsg      = pTdmfCollectorState->NbPDBMsg;
        m_CollectorStatsMsg.NbPDBMsgPerHr = pTdmfCollectorState->NbPDBMsgPerHr;
        m_CollectorStatsMsg.NbPDBMsgPerMn = pTdmfCollectorState->NbPDBMsgPerMn;
        m_CollectorStatsMsg.NbThrdRng = pTdmfCollectorState->NbThrdRng;
        m_CollectorStatsMsg.NbThrdRngHr = pTdmfCollectorState->NbThrdRngHr;
        m_CollectorStatsMsg.NbThrdRngPerMn = pTdmfCollectorState->NbThrdRngPerMn;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_default = pTdmfCollectorState->TdmfMessagesStates.Nb_default;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_ALIVE_SOCKET = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_ALIVE_SOCKET;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO_REQUEST = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO_REQUEST;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_STATE = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_STATE;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_ALERT_DATA = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_ALERT_DATA;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_ALL_DEVICES = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_ALL_DEVICES;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_DB_PARAMS = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_DB_PARAMS;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_LG_CONFIG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_LG_CONFIG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GROUP_MONITORING = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GROUP_MONITORING;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GROUP_STATE = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GROUP_STATE;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GUI_MSG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GUI_MSG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_PERF_CFG_MSG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_PERF_CFG_MSG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_PERF_MSG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_PERF_MSG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_REGISTRATION_KEY = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_REGISTRATION_KEY;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_ALL_DEVICES = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_ALL_DEVICES;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_DB_PARAMS = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_DB_PARAMS;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_LG_CONFIG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_LG_CONFIG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_STATUS_MSG = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_STATUS_MSG;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_TDMF_CMD = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_TDMF_CMD;
        m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION = pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION;

    }
    return;
}

bool CSystem::SetDeleteRecords(short Table, long days, long NbRecords)
{
	mmp_TdmfCollectorParams TCP;
	GetMMP()->getSystemParameters(&TCP);

	switch(Table)
	{
	case _TDMF_DB_ALERT_TABLE:
		TCP.iDBAlertStatusTableMaxDays = (int)days;
		TCP.iDBAlertStatusTableMaxNbr  = (int)NbRecords;
		break;
	case _TDMF_DB_PERFORMANCE_TABLE:
		TCP.iDBPerformanceTableMaxDays = (int)days;
		TCP.iDBPerformanceTableMaxNbr  = (int)NbRecords;
		break;
	default:
		return false;
	}

	GetMMP()->setSystemParameters(&TCP);

	return true;
}

void CSystem::GetDeleteRecords(short Table, long* pDays, long* pNbRecords)
{
	mmp_TdmfCollectorParams TCP;
	GetMMP()->getSystemParameters(&TCP);
	int days = 0;

	switch(Table)
	{
	case _TDMF_DB_ALERT_TABLE:
		*pDays = TCP.iDBAlertStatusTableMaxDays;
		*pNbRecords = TCP.iDBAlertStatusTableMaxNbr;
		break;
	case _TDMF_DB_PERFORMANCE_TABLE:
		*pDays = TCP.iDBPerformanceTableMaxDays;
		*pNbRecords = TCP.iDBPerformanceTableMaxNbr;
		break;
	}
}

long CSystem::GetDeleteDelay()
{
	mmp_TdmfCollectorParams TCP;
	GetMMP()->getSystemParameters(&TCP);

	return TCP.iDBCheckPeriodMinutes;
}

bool CSystem::SetDeleteDelay(long nNewValue)
{
	mmp_TdmfCollectorParams TCP;
	GetMMP()->getSystemParameters(&TCP);

	TCP.iDBCheckPeriodMinutes = nNewValue;

	GetMMP()->setSystemParameters(&TCP);

	return true;
}

// Mike Pollett
#include "../../tdmf.inc"

// Commands Menus Definitions
void CSystem::LoadCommandMenuEntries()
{
	#define MENU_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion\\Menus\\"
	
	HKEY hKeyRoot;
		
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					MENU_KEY,
					0,
					KEY_READ,
					&hKeyRoot) == ERROR_SUCCESS)
	{
		HKEY  hKeyElement;
		TCHAR achKeyElement[MAX_PATH];
		DWORD cValueName = MAX_PATH;
		int   nElementIndex = 0;
		FILETIME ftLastWriteTime;

		while (RegEnumKeyEx(hKeyRoot, nElementIndex, achKeyElement, &cValueName, NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS)
		{
			if(RegOpenKeyEx(hKeyRoot, achKeyElement, 0, KEY_READ, &hKeyElement) == ERROR_SUCCESS)
			{
				std::vector<CCommandMenuEntry> vecCommands;
				m_mapCommandMenuEntries.insert(STRING2VEC::value_type(achKeyElement, vecCommands));

				TCHAR achKeyCmd[MAX_PATH];
				HKEY  hKeyCmd;
				int   nCmdIndex = 0;
				cValueName = MAX_PATH;

				while (RegEnumKeyEx(hKeyElement, nCmdIndex, achKeyCmd, &cValueName, NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS)
				{
					CCommandMenuEntry CommandMenuEntry;
					CommandMenuEntry.m_strEntry = achKeyCmd;

					if(RegOpenKeyEx(hKeyElement, achKeyCmd, 0, KEY_READ, &hKeyCmd) == ERROR_SUCCESS)
					{
						// Get Command Id
						TCHAR achId[32];
						DWORD dwBufLen = 32;
						if (RegQueryValueEx(hKeyCmd, TEXT("Command Id"), NULL, NULL, (LPBYTE)achId, &dwBufLen) == ERROR_SUCCESS)
						{
							CommandMenuEntry.m_nCmd = *((DWORD*)achId); 
						}

						// Read conditions
						TCHAR achKeyCondition[MAX_PATH];
						DWORD cValueCondition = MAX_PATH;
						int nConditionIndex = 0;
						CConditionElement* pConditionRoot = NULL;
						while (RegEnumKeyEx(hKeyCmd, nConditionIndex, achKeyCondition, &cValueCondition, NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS)
						{
							HKEY hKeyCondition;
							if(RegOpenKeyEx(hKeyCmd, achKeyCondition, 0, KEY_READ, &hKeyCondition) == ERROR_SUCCESS)
							{
								CConditionElementEvaluation* pEvalMode = NULL;
								CConditionElementEvaluation* pEvalConnStatus = NULL;
								CConditionElementEvaluation* pEvalPlatform = NULL;
								CConditionElement *pCondition = NULL;

								TCHAR achId[32];
								DWORD dwBufLen = 32;
								if (RegQueryValueEx(hKeyCondition, TEXT("Mode"), NULL, NULL, (LPBYTE)achId, &dwBufLen) == ERROR_SUCCESS)
								{
									pEvalMode = new CConditionElementEvaluation(CConditionElementEvaluation::VariableMode, *((DWORD*)achId));
								}
								if (RegQueryValueEx(hKeyCondition, TEXT("ConnectionState"), NULL, NULL, (LPBYTE)achId, &dwBufLen) == ERROR_SUCCESS)
								{
									pEvalConnStatus = new CConditionElementEvaluation(CConditionElementEvaluation::VariableConnectionStatus, *((DWORD*)achId));
								}
								if (RegQueryValueEx(hKeyCondition, TEXT("Platform"), NULL, NULL, (LPBYTE)achId, &dwBufLen) == ERROR_SUCCESS)
								{
									pEvalPlatform = new CConditionElementEvaluation(CConditionElementEvaluation::VariablePlatform, *((DWORD*)achId));
								}

								// keep a ptr on the condition read
								if ((pEvalConnStatus != NULL) && (pEvalMode != NULL) && (pEvalPlatform != NULL))
								{
									CConditionElement* pCond1 = new CConditionElementAnd(pEvalConnStatus, pEvalMode);
									pCondition = new CConditionElementAnd(pCond1, pEvalPlatform);
								}
								else if ((pEvalConnStatus != NULL) && (pEvalMode != NULL))
								{
									pCondition = new CConditionElementAnd(pEvalConnStatus, pEvalMode);

								}
								else if ((pEvalConnStatus != NULL) && (pEvalPlatform != NULL))
								{
									pCondition = new CConditionElementAnd(pEvalConnStatus, pEvalPlatform);

								}
								else if ((pEvalPlatform != NULL) && (pEvalMode != NULL))
								{
									pCondition = new CConditionElementAnd(pEvalPlatform, pEvalMode);

								}
								else if (pEvalMode != NULL)
								{
									pCondition = pEvalMode;
								}
								else if (pEvalConnStatus != NULL)
								{
									pCondition = pEvalConnStatus;
								}
								else if (pEvalPlatform != NULL)
								{
									pCondition = pEvalPlatform;
								}

								// isert it in the global condition
								if (pCondition != NULL)
								{
									if (pConditionRoot == NULL)
									{
										pConditionRoot = pCondition;
									}
									else
									{
										pConditionRoot = new CConditionElementOr(pConditionRoot, pCondition);
									}
								}

								RegCloseKey(hKeyCondition);
							}

							// next condition
							nConditionIndex++;
							cValueCondition = MAX_PATH;
						}
						CommandMenuEntry.m_pConditionElement = pConditionRoot;
					
						RegCloseKey(hKeyCmd);
					}

					// Save entry
					m_mapCommandMenuEntries[std::string(achKeyElement)].push_back(CommandMenuEntry);

					// next key
					nCmdIndex++;
					cValueName = MAX_PATH;
				}

				RegCloseKey(hKeyElement);
			}

			// Next Element
			nElementIndex++;
			cValueName = MAX_PATH;
		}

		RegCloseKey(hKeyRoot);
	}	
}

int CSystem::GetNbCommandMenuEntries(char* szKey)
{
	return m_mapCommandMenuEntries[szKey].size();
}

std::string CSystem::GetCommandMenuEntryName(char* szKey, int nIndex)
{
	if ((UINT)nIndex < m_mapCommandMenuEntries[szKey].size())
	{
		return m_mapCommandMenuEntries[szKey].at(nIndex).m_strEntry;
	}

	return "";
}

int CSystem::GetCommandMenuEntryId(char* szKey, int nIndex)
{
	if ((UINT)nIndex < m_mapCommandMenuEntries[szKey].size())
	{
		return m_mapCommandMenuEntries[szKey].at(nIndex).m_nCmd;
	}

	return -1;
}

BOOL CSystem::IsCommandMenuEntryEnabled(char* szKey, int nIndex, int nMode, int nConnectionStatus, int nPlatform)
{
	if ((UINT)nIndex < m_mapCommandMenuEntries[szKey].size())
	{
		if (m_mapCommandMenuEntries[szKey].at(nIndex).m_pConditionElement != NULL)
		{
			return m_mapCommandMenuEntries[szKey].at(nIndex).m_pConditionElement->Evaluate(nMode, nConnectionStatus, nPlatform);
		}
		else
		{
			return TRUE;
		}
	}

	return FALSE;
}

std::string CSystem::GetCommandMenuString(char* szKey, int Id)
{
	std::vector<CCommandMenuEntry>& vecTmp = m_mapCommandMenuEntries[szKey];
	std::vector<CCommandMenuEntry>::iterator it;

	for (it = vecTmp.begin(); it != vecTmp.end(); it++)
	{
		if (it->m_nCmd == Id)
		{
			return it->m_strEntry;
		}
	}

	return "";
}

static CString GetType(FsTdmfDbUserInfo::TdmfLoginTypes eType)
{
	CString cstrType;

	switch (eType)
	{
	case FsTdmfDbUserInfo::TdmfLoginWindowsGroup:
		cstrType = "Windows Group";
		break;

	case FsTdmfDbUserInfo::TdmfLoginWindowsUser:
		cstrType = "Windows User";
		break;

	case FsTdmfDbUserInfo::TdmfLoginStandard:
		cstrType= "Standard";
		break;

	case FsTdmfDbUserInfo::TdmfLoginUndef:
		cstrType= "";
		break;
	}

	return cstrType;
}

static CString GetRole(FsTdmfDbUserInfo::TdmfRoles eRole)
{
	CString cstrRole;

	switch (eRole)
	{
	case FsTdmfDbUserInfo::TdmfRoleAdministrator:
		cstrRole = "Administrator";
		break;

	case FsTdmfDbUserInfo::TdmfRoleSupervisor:
		cstrRole = "Supervisor";
		break;

	case FsTdmfDbUserInfo::TdmfRoleUser:
	case FsTdmfDbUserInfo::TdmfRoleUndef:
		cstrRole = "User";
		break;
	}

	return cstrRole;
}

std::string CSystem::GetUserRole()
{
	if (m_eTdmfRole == FsTdmfDbUserInfo::TdmfRoleUndef)
	{
		m_eTdmfRole = m_db->FtdGetUserRole((BSTR)m_bstrUser);
	}
	
	std::string strRole = GetRole(m_eTdmfRole);

	return strRole;
}

bool CSystem::GetFirstUser(std::string& strLocation, std::string& strUserName, std::string& strType, std::string& strApp)
{
	m_listUser.clear();

	m_db->FtdGetUsersInfo(m_listUser);

	m_itUser = m_listUser.begin();
	
	if (m_itUser == m_listUser.end())
	{
		return false;
	}
	else
	{
		FsTdmfDbUserInfo& DbUserInfo = *m_itUser;
		strLocation = DbUserInfo.m_cstrLocation;
		strUserName = m_itUser->m_cstrName;
		strType     = GetType(DbUserInfo.m_eType);
		strApp      = DbUserInfo.m_cstrApp;
	}

	return true;
}

bool CSystem::GetNextUser(std::string& strLocation, std::string& strUserName, std::string& strType, std::string& strApp)
{
	m_itUser++;

	if (m_itUser == m_listUser.end())
	{
		return false;
	}
	else
	{
		FsTdmfDbUserInfo& DbUserInfo = *m_itUser;
		strLocation = DbUserInfo.m_cstrLocation;
		strUserName = DbUserInfo.m_cstrName;
		strType     = GetType(DbUserInfo.m_eType);
		strApp      = DbUserInfo.m_cstrApp;
	}

	return true;
}

void CSystem::InitializeTheCollectorStatsMember()
{
    m_CollectorStatsMsg.CollectorTime =  0;
    m_CollectorStatsMsg.NbAgentsAlive = 0;
    m_CollectorStatsMsg.NbAliveMsgPerHr = 0;
    m_CollectorStatsMsg.NbAliveMsgPerMn = 0;
    m_CollectorStatsMsg.NbPDBMsg      = 0;
    m_CollectorStatsMsg.NbPDBMsgPerHr = 0;
    m_CollectorStatsMsg.NbPDBMsgPerMn = 0;
    m_CollectorStatsMsg.NbThrdRng = 0;
    m_CollectorStatsMsg.NbThrdRngHr = 0;
    m_CollectorStatsMsg.NbThrdRngPerMn = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_default = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_ALIVE_SOCKET = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO_REQUEST = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_AGENT_STATE = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_ALERT_DATA = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_ALL_DEVICES = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_DB_PARAMS = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GET_LG_CONFIG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GROUP_MONITORING = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GROUP_STATE = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_GUI_MSG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_PERF_CFG_MSG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_PERF_MSG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_REGISTRATION_KEY = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_ALL_DEVICES = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_DB_PARAMS = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_SET_LG_CONFIG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_STATUS_MSG = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_TDMF_CMD = 0;
    m_CollectorStatsMsg.TdmfMessagesStates.Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION = 0;

}

long CSystem::GetUserCount()
{
  	m_listUser.clear();

	m_db->FtdGetUsersInfo(m_listUser);

    return m_listUser.size();
}

long CSystem::GetTimeOutFromDB()
{
    MMPAPI_Error Err = MMPAPI_Error::OK;
	long nRetCode = Err;

    m_TimeOutArray.RemoveAll();

    mmp_TdmfCollectorParams TCP;
    Err = GetMMP()->getSystemParameters(&TCP);

   	if (Err != MMPAPI_Error::OK)
	{
	  nRetCode = -1;
	}
	else
	{
		m_TimeOutArray.SetAtGrow(0, TCP.iTimeOut1);
        m_TimeOutArray.SetAtGrow(1, TCP.iTimeOut2);
        m_TimeOutArray.SetAtGrow(2, TCP.iTimeOut3);
        m_TimeOutArray.SetAtGrow(3, TCP.iTimeOut4);
        m_TimeOutArray.SetAtGrow(4, TCP.iTimeOut5);
        m_TimeOutArray.SetAtGrow(5, TCP.iTimeOut6);
        m_TimeOutArray.SetAtGrow(6, TCP.iTimeOut7);
        m_TimeOutArray.SetAtGrow(7, TCP.iTimeOut8);
        m_TimeOutArray.SetAtGrow(8, TCP.iTimeOut9);
        m_TimeOutArray.SetAtGrow(9, TCP.iTimeOut10);
	}

	return nRetCode;
}

long CSystem::SetTimeOutToDB()
{
    MMPAPI_Error Err = MMPAPI_Error::OK;
    long nRetCode = MMPAPI_Error::OK;

    mmp_TdmfCollectorParams TCP;
    if(m_TimeOutArray.GetSize() > 0)
    {
        TCP.iTimeOut1 =   m_TimeOutArray.GetAt(0);
        TCP.iTimeOut2 =   m_TimeOutArray.GetAt(1); 
        TCP.iTimeOut3 =   m_TimeOutArray.GetAt(2);
        TCP.iTimeOut4 =   m_TimeOutArray.GetAt(3);
        TCP.iTimeOut5 =   m_TimeOutArray.GetAt(4);
        TCP.iTimeOut6 =   m_TimeOutArray.GetAt(5);
        TCP.iTimeOut7 =   m_TimeOutArray.GetAt(6);
        TCP.iTimeOut8 =   m_TimeOutArray.GetAt(7);
        TCP.iTimeOut9 =   m_TimeOutArray.GetAt(8);
        TCP.iTimeOut10 =  m_TimeOutArray.GetAt(9);
    
        Err = GetMMP()->setSystemParameters(&TCP);
    }
      
    if (Err != MMPAPI_Error::OK)
	{
	  nRetCode = -1;
	}

    return nRetCode;
}

int CSystem::GetTimeOut(int nIndex)
{
  if(nIndex <= m_TimeOutArray.GetUpperBound( ))
    return m_TimeOutArray[nIndex];
  else
    return -1;
}

void CSystem::SetTimeOut(int nIndex, int nValue)
{
  if(nValue <= m_TimeOutArray.GetUpperBound( )) 
    m_TimeOutArray[nIndex] = nValue;

}
   
