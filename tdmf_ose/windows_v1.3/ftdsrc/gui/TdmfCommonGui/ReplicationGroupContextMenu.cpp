// ReplicationGroupContextMenu.cpp: implementation of the CReplicationGroupContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "TdmfCommonGuiDoc.h"
#include "ReplicationGroupContextMenu.h"
#include "ViewNotification.h"
#include "ReplicationGroupPropertySheet.h"
#include "ScriptEditorDlg.h"
#include "ImportScriptDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// 

class CheckModeTimer
{
protected:
	static UINT_PTR           m_uIDEvent;
	static CTdmfCommonGuiDoc* m_pDoc;
	static long               m_nDomainKey;
	static long               m_nServerKey;
	static long               m_nGroupNumber;
	static long               m_nNbCheck;

public:
	static void Init(CTdmfCommonGuiDoc* pDoc, long nDomainKey, long nServerKey, long nGroupNumber)
	{
		m_uIDEvent     = 0;
		m_pDoc         = pDoc;
		m_nDomainKey   = nDomainKey;
		m_nServerKey   = nServerKey;
		m_nGroupNumber = nGroupNumber;
		m_nNbCheck     = 0;
	}

	static VOID CALLBACK CheckModeTimerFunc(HWND hwnd,         // handle to window
											UINT uMsg,         // WM_TIMER message
											UINT_PTR idEvent,  // timer identifier
											DWORD dwTime)      // current system time
	{
		KillTimer(NULL, m_uIDEvent);
		
		// search for the new group
		TDMFOBJECTSLib::IReplicationGroupPtr pRGFound; 
		long nNbDomain = m_pDoc->m_pSystem->DomainCount;
		for (long nIndexDomain = 0; nIndexDomain < nNbDomain; nIndexDomain++)
		{
			TDMFOBJECTSLib::IDomainPtr pDomain = m_pDoc->m_pSystem->GetDomain(nIndexDomain);
			
			if (pDomain->Key == m_nDomainKey)
			{
				// Servers
				long nNbServer = pDomain->ServerCount;
				for (long nIndexServer = 0; nIndexServer < nNbServer; nIndexServer++)
				{
					TDMFOBJECTSLib::IServerPtr pServer = pDomain->GetServer(nIndexServer);
					
					if (pServer->Key == m_nServerKey)
					{
						// Replication Groups
						long nNbRG = pServer->ReplicationGroupCount;
						for (long nIndexRG = 0; nIndexRG < nNbRG; nIndexRG++)
						{
							TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(nIndexRG);
							
							if (pRG->IsSource && (pRG->GroupNumber == m_nGroupNumber))
							{
								pRGFound = pRG;
								break;
							}	
						}
						break;
					}
				}
				break;
			}
		}
		
		// Check groups mode; if unix symm group is still running...warning
		if (pRGFound != NULL)
		{
			m_nNbCheck++;

			if (pRGFound->SymmetricMode != TDMFOBJECTSLib::FTD_M_UNDEF)
			{
				CString cstrMsg;
				cstrMsg.Format("WARNING: After Failover, group %d and group %d are both running (group %d was not stopped properly).\n"
					"In order to prevent data integrity problems. you should stop one of the groups immediately.",
					pRGFound->GroupNumber, pRGFound->SymmetricGroupNumber, pRGFound->SymmetricGroupNumber);
				MessageBox(AfxGetMainWnd()->GetSafeHwnd(), cstrMsg, "Replication Group Failover Warning", MB_OK|MB_ICONWARNING);
			}
			else
			{
				// Schedile next check (1, 3, 5 seconds)
				if (m_nNbCheck < 3)
				{
					Start((2*m_nNbCheck+1)*1000);
				}
			}
		}
	}

	static void Start(UINT nDelay = 1000)
	{
		m_uIDEvent = SetTimer(NULL, 0, nDelay, CheckModeTimerFunc);
	}
};

UINT_PTR           CheckModeTimer::m_uIDEvent = 0;
CTdmfCommonGuiDoc* CheckModeTimer::m_pDoc = 0;
long               CheckModeTimer::m_nDomainKey = 0;
long               CheckModeTimer::m_nServerKey = 0;
long               CheckModeTimer::m_nGroupNumber = 0;
long               CheckModeTimer::m_nNbCheck = 0;

CheckModeTimer g_CheckModeTimer;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CReplicationGroupContextMenu::CReplicationGroupContextMenu(CTdmfCommonGuiDoc* pDoc,
														   TDMFOBJECTSLib::IReplicationGroup* pRG)
	: CContextMenuBase(pDoc), m_pRG(pRG)
{
}

CReplicationGroupContextMenu::~CReplicationGroupContextMenu()
{
}

void CReplicationGroupContextMenu::AddEntries()
{
	try
	{
		int nPlatefrom = (strstr(m_pRG->Parent->OSType, "Windows") != 0 ||
						  strstr(m_pRG->Parent->OSType, "windows") != 0 ||
						  strstr(m_pRG->Parent->OSType, "WINDOWS") != 0) ? 0 : 1;

		m_Menu.AppendMenu(MF_STRING, ID_REPLICATION_PAIRS , "&Replication Pairs...");

		// Symmetric
        CMenu menuSymmetric;
        menuSymmetric.CreatePopupMenu();
		
		// Allow new symmetric creation only if OS version are the same
		CString cstrSourceOSVersion = (BSTR)m_pRG->Parent->OSVersion;
		cstrSourceOSVersion.MakeLower();
		cstrSourceOSVersion = cstrSourceOSVersion.Left(cstrSourceOSVersion.Find(" service pack "));
		CString cstrTargetOSVersion = (BSTR)m_pRG->TargetServer->OSVersion;
		cstrTargetOSVersion.MakeLower();
		cstrTargetOSVersion = cstrTargetOSVersion.Left(cstrTargetOSVersion.Find(" service pack "));
		bool bOSVersionDiffer = (cstrSourceOSVersion != cstrTargetOSVersion);

		menuSymmetric.AppendMenu(MF_STRING | (m_pRG->Symmetric || !m_pRG->Parent->Connected || bOSVersionDiffer) ? MF_GRAYED : MF_ENABLED, ID_REPLICATION_SYMMETRIC_ADD, "Add Symmetric Configuration...");
		menuSymmetric.AppendMenu(MF_STRING | (m_pRG->Symmetric && m_pRG->Parent->Connected && ((m_pRG->Mode == TDMFOBJECTSLib::FTD_M_UNDEF) || (m_pRG->ConnectionStatus == TDMFOBJECTSLib::FTD_ACCUMULATE) || (m_pRG->SymmetricMode == TDMFOBJECTSLib::FTD_M_UNDEF))) ? MF_ENABLED : MF_GRAYED, ID_REPLICATION_SYMMETRIC_REMOVE, "Remove Symmetric Configuration...");

		// Depending on symmetric group's mode, enable/disable stat/stop
		bool bSymmetricAlreadyStarted = m_pRG->SymmetricMode != TDMFOBJECTSLib::FTD_M_UNDEF;
		bool bCanStartSymmetricGroup = ((nPlatefrom == 0) && m_pRG->Parent->Connected && m_pRG->Symmetric && !bSymmetricAlreadyStarted && ((m_pRG->Mode == TDMFOBJECTSLib::FTD_MODE_PASSTHRU) || (m_pRG->Mode == TDMFOBJECTSLib::FTD_MODE_TRACKING) || (m_pRG->Mode == TDMFOBJECTSLib::FTD_M_UNDEF)));
        menuSymmetric.AppendMenu(MF_STRING | bCanStartSymmetricGroup ? MF_ENABLED : MF_GRAYED, ID_REPLICATION_SYMMETRIC_START, "Start Symmetric Group");
		menuSymmetric.AppendMenu(MF_STRING | (m_pRG->Parent->Connected && m_pRG->Symmetric && bSymmetricAlreadyStarted && ((m_pRG->Mode == TDMFOBJECTSLib::FTD_M_UNDEF) || (m_pRG->ConnectionStatus == TDMFOBJECTSLib::FTD_ACCUMULATE))) ? MF_ENABLED : MF_GRAYED, ID_REPLICATION_SYMMETRIC_STOP, "Stop Symmetric Group");
		
		// TODO : enable/disable Failover command
		bool bFailoverEnable = (m_pRG->Symmetric && ((m_pRG->Mode == TDMFOBJECTSLib::FTD_MODE_NORMAL) || (m_pRG->Mode == TDMFOBJECTSLib::FTD_MODE_PASSTHRU) || (m_pRG->Mode == TDMFOBJECTSLib::FTD_MODE_TRACKING) || (m_pRG->Mode == TDMFOBJECTSLib::FTD_M_UNDEF)));
		menuSymmetric.AppendMenu(MF_STRING | bFailoverEnable ? MF_ENABLED : MF_GRAYED, ID_REPLICATION_SYMMETRIC_FAILOVER, "Failover");

		UINT uFlag = MF_STRING | MF_ENABLED | MF_POPUP;
		if ((!m_pDoc->GetConnectedFlag()) || (m_pRG->IsLockCmds) ||
			(m_pRG->Parent->IsLockCmds) || m_pDoc->GetReadOnlyFlag() || (m_pRG->IsSource == FALSE))
		{
			uFlag = MF_STRING | MF_GRAYED;
		}
        m_Menu.AppendMenu(uFlag, (UINT) menuSymmetric.m_hMenu, "Symmetric");

		// Commands
		AddCommandEntries(m_pRG->IsSource ? "Group" : "Target Group", m_pRG->Parent->Connected && m_pDoc->GetConnectedFlag() && (!m_pDoc->GetReadOnlyFlag()) && (!m_pRG->IsLockCmds) && (!m_pRG->Parent->IsLockCmds), m_pRG->Mode, m_pRG->ConnectionStatus, nPlatefrom);
		m_Menu.AppendMenu(MF_SEPARATOR);

		m_Menu.AppendMenu(MF_STRING, ID_DELETE , "&Delete");
		m_Menu.AppendMenu(MF_SEPARATOR);
		m_Menu.AppendMenu(MF_STRING, ID_PROPERTIES, "&Properties...");

		if ((!m_pDoc->GetConnectedFlag()) || (!m_pRG->Parent->Connected) || (m_pRG->IsLockCmds) ||
			(m_pRG->Parent->IsLockCmds) || m_pDoc->GetReadOnlyFlag() || (m_pRG->IsSource == FALSE))
		{
			m_Menu.EnableMenuItem(ID_REPLICATION_PAIRS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			m_Menu.EnableMenuItem(ID_PROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}
		if ((!m_pDoc->GetConnectedFlag()) || (!m_pRG->Parent->Connected) ||
			(m_pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF) || m_pDoc->GetReadOnlyFlag() ||
			(m_pRG->IsSource == FALSE) || m_pRG->IsLockCmds || m_pRG->Parent->IsLockCmds)
		{
			m_Menu.EnableMenuItem(ID_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

        m_Menu.AppendMenu(MF_SEPARATOR);
   
        CMenu ScriptMenu;
        ScriptMenu.CreatePopupMenu();
        ScriptMenu.AppendMenu(MF_STRING, ID_SERVERVIEWCONTEXTMENU_SCRIPT, "&Edit...");
        ScriptMenu.AppendMenu(MF_STRING, ID_SERVERVIEWCONTEXTMENU_IMPORT_SCRIPT, "&Import...");
      
        m_Menu.AppendMenu(MF_POPUP, (UINT) ScriptMenu.m_hMenu, "&Script");
  
       
        if ((m_pRG->IsSource == FALSE) || 
            !m_pDoc->GetConnectedFlag() || 
            !m_pRG->Parent->Connected ||
			m_pRG->IsLockCmds ||
			m_pRG->Parent->IsLockCmds)
		{
			m_Menu.EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_SCRIPT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
  		}

        if ((m_pRG->IsSource == FALSE) || 
            !m_pDoc->GetConnectedFlag() || 
            !m_pRG->Parent->Connected ||
             m_pDoc->GetReadOnlyFlag() ||
  			 m_pRG->IsLockCmds ||
			 m_pRG->Parent->IsLockCmds)

   		{
        	m_Menu.EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_IMPORT_SCRIPT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        }
       
	}
	CATCH_ALL_LOG_ERROR(1017);
}

class CRGLaunchCmdParam
{
public:
	CRGLaunchCmdParam(IStream*                            pStream,
	  				  enum ContextMenuCommands            eId,
					  enum TDMFOBJECTSLib::tagTdmfCommand eCommand,
					  const char*                         lpcstrArg,
					  bool                                bSymmetric)
		: m_pStream(pStream), m_eId(eId), m_eCommand(eCommand), m_cstrArg(lpcstrArg), m_bSymmetric(bSymmetric)
	{
	}

	virtual ~CRGLaunchCmdParam() {}

public:
	IStream*                            m_pStream;
	enum ContextMenuCommands            m_eId;
	enum TDMFOBJECTSLib::tagTdmfCommand m_eCommand;
	CString                             m_cstrArg;
	bool                                m_bSymmetric;
};

static UINT LaunchCommandThread(LPVOID pParam)
{
	CoInitialize(NULL);

	CRGLaunchCmdParam* pLaunchCmdParam = (CRGLaunchCmdParam*)pParam;

	// Retrieve the interface pointer within your thread routine
	TDMFOBJECTSLib::IReplicationGroupPtr pRG;
	if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pLaunchCmdParam->m_pStream, TDMFOBJECTSLib::IID_IReplicationGroup, (LPVOID*)&pRG)))
	{
		long nRetCode = 0;
		CComBSTR bstrMessage;

		std::ostringstream ossOptions;
        bool bWindows = (strstr(pRG->Parent->OSType, "Windows") != 0 ||
				         strstr(pRG->Parent->OSType, "windows") != 0 ||
				         strstr(pRG->Parent->OSType, "WINDOWS") != 0);

		long nGroupNumber = (pLaunchCmdParam->m_bSymmetric == false) ? pRG->GroupNumber : pRG->SymmetricGroupNumber;

		if ((bWindows) && (pLaunchCmdParam->m_eId == ID_CMD_START))
		{
			ossOptions << "-g" << nGroupNumber << " -f " << (LPCSTR)pLaunchCmdParam->m_cstrArg;
		}
		else
		{
			ossOptions << "-g" << nGroupNumber << " " << (LPCSTR)pLaunchCmdParam->m_cstrArg;
		}

		// Issue a Kill PMD before any Stop command
		if (pLaunchCmdParam->m_eId == ID_CMD_STOP_IMMEDIATE)
		{
			nRetCode = pRG->LaunchCommand(TDMFOBJECTSLib::CMD_KILL_PMD, ossOptions.str().c_str(), "", pLaunchCmdParam->m_bSymmetric, &bstrMessage);
		}
		if (nRetCode == 0)
		{
			nRetCode = pRG->LaunchCommand(pLaunchCmdParam->m_eCommand, ossOptions.str().c_str(), "", pLaunchCmdParam->m_bSymmetric, &bstrMessage);
		}

		// If it's a symmetric START, put group in passthru
		if ((nRetCode == 0) && (pLaunchCmdParam->m_eId == ID_CMD_START) && pLaunchCmdParam->m_bSymmetric)
		{
			ossOptions.str("");
			ossOptions << "-g" << nGroupNumber << " state passthru";
			nRetCode = pRG->LaunchCommand(TDMFOBJECTSLib::CMD_OVERRIDE, ossOptions.str().c_str(), "", pLaunchCmdParam->m_bSymmetric, &bstrMessage);
		}

		// Display an error msg
		if (nRetCode != 0)
		{
			_bstr_t bstrTmp(bstrMessage, true);
			CString cstrTitle;
			CString strMenuString =	(BSTR)pRG->Parent->Parent->Parent->GetCommandMenuString("Group", pLaunchCmdParam->m_eId);
			cstrTitle.Format("'%s %s' on '%S'", strMenuString, ossOptions.str().c_str(), (BSTR)pRG->Parent->Name);
			
			if (bstrTmp.length() > 0)
			{
				MessageBox(AfxGetMainWnd()->GetSafeHwnd(), bstrTmp, cstrTitle, MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				std::ostringstream oss;
				oss << "The command has returned the following error code: " << nRetCode;
				MessageBox(AfxGetMainWnd()->GetSafeHwnd(), oss.str().c_str(), cstrTitle, MB_OK | MB_ICONINFORMATION);
			}
		}

		pRG->UnlockCmds();
	}
	
	// Cleanup
	delete pLaunchCmdParam;

	CoUninitialize();

	return 0;
}

long CReplicationGroupContextMenu::LaunchCommand(enum ContextMenuCommands            eId,
												 enum TDMFOBJECTSLib::tagTdmfCommand eCommand,
												 const char*                         pszArg,
												 bool                                bSymmetric)
{
	long nRetCode = 0;
	bool bLaunchCommand = false;
	CComBSTR bstrMessage;

	if ((m_pRG->Parent->RegKey.length() == 0) ||
		(bSymmetric && (m_pRG->TargetServer->RegKey.length() == 0)))
	{
		CString strMsg;
		strMsg.Format("The registration key is not set for the server '%S'.  Are you sure you want to continue?",
					  (BSTR)m_pRG->Parent->Name);
		if (MessageBox(GetActiveWindow(), strMsg, "Empty Registration Key",
			MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL) == IDNO)
		{
			return 0;
		}
	}

	CString strMenuString =	(BSTR)m_pDoc->m_pSystem->GetCommandMenuString("Group", eId);

	CString strMsg;
	if (bSymmetric == false)
	{
		strMsg.Format("Are you sure you want to send a '%s' command to the replication group '%S'?",
					   strMenuString, (BSTR)m_pRG->Name);
	}
	else
	{
		strMsg.Format("Are you sure you want to send a '%s' command to the symmetric replication group of '%S'?",
					   strMenuString, (BSTR)m_pRG->Name);
	}

	if (MessageBox(GetActiveWindow(), strMsg, "Confirm Replication Group Command",
		MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
	{
		bLaunchCommand = true;
	}

	if (bLaunchCommand)
	{
		CRGLaunchCmdParam* pParam = new CRGLaunchCmdParam(NULL, eId, eCommand, pszArg, bSymmetric);
		if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(TDMFOBJECTSLib::IID_IReplicationGroup, (LPUNKNOWN)m_pRG, &pParam->m_pStream)))
		{
			m_pRG->LockCmds();
			AfxBeginThread(LaunchCommandThread, pParam, 0, 0, 0, NULL);
		}
		else
		{
			AfxMessageBox("Unexpected Error: 997", MB_OK | MB_ICONINFORMATION);
			delete pParam;
		}
	}

	return 0;
}

void CReplicationGroupContextMenu::OnCommand(UINT nCommand)
{
	CWaitCursor WaitCursor;

	switch (nCommand)
	{
	case ID_REPLICATION_PAIRS:
		try
		{
			_bstr_t bstrTitle = m_pRG->Name + " Properties";
			CReplicationGroupPropertySheet RGPropertySheet(m_pDoc, bstrTitle, NULL, 0, m_pRG);
			RGPropertySheet.SetActivePage(2);
			if (RGPropertySheet.DoModal() == IDOK)
			{
				// Actions handle in the property sheet.
			}				
		}
		CATCH_ALL_LOG_ERROR(1018);
		break;
		
	case ID_REPLICATION_SYMMETRIC_ADD:
		try
		{
			m_pRG->Symmetric = true;

			_bstr_t bstrTitle = m_pRG->Name + " Properties";
			CReplicationGroupPropertySheet RGPropertySheet(m_pDoc, bstrTitle, NULL, 0, m_pRG, false, true);
			RGPropertySheet.SetActivePage(3);
			if (RGPropertySheet.DoModal() == IDOK)
			{
				// Actions handle in the property sheet.
			}
			else
			{
				// User cancels the symmetric creation, restore old value
				m_pRG->Symmetric = false;
			}
		}
		CATCH_ALL_LOG_ERROR(1200);
		break;

	case ID_REPLICATION_SYMMETRIC_REMOVE:
		try
		{
			if (MessageBox(GetActiveWindow(), "Are you sure you want to delete Symmetric group?", "Symmetric Group Deletion", MB_ICONQUESTION|MB_YESNO) == IDYES)
			{
				if (SUCCEEDED(m_pRG->RemoveSymmetricGroup()))
				{
					// Update display
					// Fire an Object Change Notification
					CViewNotification::CHANGE_PARAMS eParam = CViewNotification::PROPERTIES;
					
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_CHANGE;
					VN.m_pUnk = (IUnknown*)m_pRG;
					VN.m_eParam = eParam;
					m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
			}
		}
		CATCH_ALL_LOG_ERROR(1201);
		break;

	case ID_REPLICATION_SYMMETRIC_START:
		try
		{
			LaunchCommand(ID_CMD_START, TDMFOBJECTSLib::CMD_START, "", true);
		}
		CATCH_ALL_LOG_ERROR(1202);
		break;

	case ID_REPLICATION_SYMMETRIC_STOP:
		try
		{
			LaunchCommand(ID_CMD_STOP, TDMFOBJECTSLib::CMD_STOP, "", true);
		}
		CATCH_ALL_LOG_ERROR(1203);
		break;

	case ID_REPLICATION_SYMMETRIC_FAILOVER:
		try
		{
			if (MessageBox(GetActiveWindow(), "Are you sure you wish to failover?", "Failover Confirmation", MB_ICONQUESTION|MB_YESNO) == IDYES)
			{
				long nWarning = 0;
				// Cache info before invalidating pRG
				long nSymmetricGroupNumber = m_pRG->SymmetricGroupNumber;
				_bstr_t bstrPrimary = m_pRG->Parent->Name;
				g_CheckModeTimer.Init(m_pDoc, m_pRG->Parent->Parent->Key, m_pRG->TargetServer->Key, m_pRG->SymmetricGroupNumber); // grp number after the failover
				
		        bool bWindows = (strstr(m_pRG->Parent->OSType, "Windows") != 0 ||
						         strstr(m_pRG->Parent->OSType, "windows") != 0 ||
						         strstr(m_pRG->Parent->OSType, "WINDOWS") != 0);

				if (m_pRG->Failover(&nWarning) != 0)
				{
					CString strMsg;
					strMsg.Format("Unable to failover group '%S'\n\n", (BSTR)m_pRG->Name);
					MessageBox(GetActiveWindow(), strMsg, "Replication Group Failover Error", MB_OK|MB_ICONINFORMATION);
				}
				
				// Force a refresh
				m_pDoc->OnToolsRefresh();
				// Don't use m_pRG anymore
				m_pRG = NULL;

				if (nWarning != 0)
				{
					CString strMsg;
					strMsg.Format("Failover was unable to remove s%03d.off file on primary server (%S).\nLaunch 'dtcreco -g%03d -d' command when the server is available to enable data replication to the mirror device for group %d.",
								  nSymmetricGroupNumber, (BSTR)bstrPrimary, nSymmetricGroupNumber, nSymmetricGroupNumber);
					MessageBox(GetActiveWindow(), strMsg, "Replication Group Failover Warning", MB_OK|MB_ICONINFORMATION);
				}

				if (bWindows == false)
				{
					// Schedule a mode check
					g_CheckModeTimer.Start();
				}
			}
		}
		CATCH_ALL_LOG_ERROR(1204);
		break;

	case ID_CMD_LAUNCH_BACKFRESH:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_BACKFRESH, TDMFOBJECTSLib::CMD_LAUNCH_BACKFRESH);
		}
		CATCH_ALL_LOG_ERROR(1019);
		break;
		
	case ID_CMD_KILL_BACKFRESH:
		try
		{
			LaunchCommand(ID_CMD_KILL_BACKFRESH, TDMFOBJECTSLib::CMD_KILL_BACKFRESH);
		}
		CATCH_ALL_LOG_ERROR(1020);
		break;
		
	case ID_CMD_LAUNCH_PMD:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_PMD, TDMFOBJECTSLib::CMD_LAUNCH_PMD);
		}
		CATCH_ALL_LOG_ERROR(1021);
		break;
		
	case ID_CMD_KILL_PMD:
		try
		{
			LaunchCommand(ID_CMD_KILL_PMD, TDMFOBJECTSLib::CMD_KILL_PMD);
		}
		CATCH_ALL_LOG_ERROR(1022);
		break;
		
	case ID_CMD_LAUNCH_REFRESH_SMART:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_REFRESH_SMART, TDMFOBJECTSLib::CMD_LAUNCH_REFRESH);
		}
		CATCH_ALL_LOG_ERROR(1023);
		break;

	case ID_CMD_LAUNCH_REFRESH_FULL:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_REFRESH_FULL, TDMFOBJECTSLib::CMD_LAUNCH_REFRESH, "-f");
		}
		CATCH_ALL_LOG_ERROR(1024);
		break;

	case ID_CMD_LAUNCH_REFRESH_CHECKSUM:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_REFRESH_CHECKSUM, TDMFOBJECTSLib::CMD_LAUNCH_REFRESH, "-c");
		}
		CATCH_ALL_LOG_ERROR(1025);
		break;
		
	case ID_CMD_KILL_REFRESH:
		try
		{
			LaunchCommand(ID_CMD_KILL_REFRESH, TDMFOBJECTSLib::CMD_KILL_REFRESH);
		}
		CATCH_ALL_LOG_ERROR(1026);
		break;
		
	case ID_CMD_START:
		try
		{
			LaunchCommand(ID_CMD_START, TDMFOBJECTSLib::CMD_START);
		}
		CATCH_ALL_LOG_ERROR(1027);
		break;
			
	case ID_CMD_STOP:
		try
		{
			LaunchCommand(ID_CMD_STOP, TDMFOBJECTSLib::CMD_STOP);
		}
		CATCH_ALL_LOG_ERROR(1028);
		break;

	case ID_CMD_STOP_IMMEDIATE:
		try
		{
			LaunchCommand(ID_CMD_STOP_IMMEDIATE, TDMFOBJECTSLib::CMD_STOP);
		}
		CATCH_ALL_LOG_ERROR(1029);		
		break;

	case ID_CMD_CHECKPOINT_ON:
		try
		{
			CString  cstrArg;
			if (m_pRG->IsSource == FALSE)
			{
				cstrArg += "-s ";
			}
			cstrArg += "-on";
			LaunchCommand(ID_CMD_CHECKPOINT_ON, TDMFOBJECTSLib::CMD_CHECKPOINT, cstrArg);
		}
		CATCH_ALL_LOG_ERROR(1030);
		break;

	case ID_CMD_CHECKPOINT_OFF:
		try
		{
			CString  cstrArg;
			if (m_pRG->IsSource == FALSE)
			{
				cstrArg += "-s ";
			}
			cstrArg += "-off";

			LaunchCommand(ID_CMD_CHECKPOINT_OFF, TDMFOBJECTSLib::CMD_CHECKPOINT, cstrArg);
		}
		CATCH_ALL_LOG_ERROR(1031);
		break;

	case ID_CMD_KILL_RMD:
		try
		{
			LaunchCommand(ID_CMD_KILL_RMD, TDMFOBJECTSLib::CMD_KILL_RMD);
		}
		CATCH_ALL_LOG_ERROR(1032);
		break;

	case ID_DELETE:
		try
		{
			CString strMsg;
			strMsg.Format("Are you sure you want to delete the replication group '%S'?\n\n",
						  (BSTR)m_pRG->Name);
			
			if (MessageBox(GetActiveWindow(), strMsg, "Confirm Replication Group Delete",
				MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
			{
				CWaitCursor wait;
				TDMFOBJECTSLib::IReplicationGroupPtr pRGTarget = m_pRG->GetTargetGroup();
				
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_REMOVE;
				VN.m_pUnk = (IUnknown*) m_pRG;
				m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

				TDMFOBJECTSLib::IServerPtr pServer = m_pRG->Parent;
				if (pServer->RemoveReplicationGroup(m_pRG) != 0)
				{
					CString strMsg;
					strMsg.Format("Unable to delete replication group '%S'?\n\n",
								  (BSTR)m_pRG->Name);
					MessageBox(GetActiveWindow(), strMsg, "Replication Group Deletion Error",
							   MB_OK|MB_ICONINFORMATION);
				}

				if (pRGTarget)
				{
					VN.m_pUnk = (IUnknown*) pRGTarget;
					m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

					TDMFOBJECTSLib::IServerPtr pServerTarget = pRGTarget->Parent;				
					if (pServerTarget->RemoveReplicationGroup(pRGTarget) != 0)
					{
						CString strMsg;
						strMsg.Format("Unable to delete replication group '%S'?\n\n",
									  (BSTR)m_pRG->Name);
						MessageBox(GetActiveWindow(), strMsg, "Replication Group Deletion Error",
								   MB_OK|MB_ICONINFORMATION);
					}
				}
			}
		}
		CATCH_ALL_LOG_ERROR(1033);
		break;
		
	case ID_PROPERTIES:
		try
		{
			_bstr_t bstrTitle = m_pRG->Name + " Properties";
			CReplicationGroupPropertySheet RGPropertySheet(m_pDoc, bstrTitle, NULL, 0, m_pRG);
			if (RGPropertySheet.DoModal() == IDOK)
			{
				// Actions handle in the property sheet.
			}
		}
		CATCH_ALL_LOG_ERROR(1034);
		break;
		
    case ID_SERVERVIEWCONTEXTMENU_SCRIPT:
	{
	
		CScriptEditorDlg dlg(m_pRG->Parent,m_pDoc->GetReadOnlyFlag());
        dlg.DoModal();
		
	}
	break;

    case ID_SERVERVIEWCONTEXTMENU_IMPORT_SCRIPT:
	{
	
		CImportScriptDlg dlg(m_pRG->Parent);
        dlg.DoModal();
		
	}
	break;

	default:
		ASSERT(0);
	}
}
