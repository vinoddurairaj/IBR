// ServerContextMenu.cpp: implementation of the CServerContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "TdmfCommonGuiDoc.h"
#include "ServerContextMenu.h"
#include "ViewNotification.h"
#include "ServerPropertySheet.h"
#include "ReplicationGroupPropertySheet.h"
#include "ScriptEditorDlg.h"
#include "ImportScriptDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerContextMenu::CServerContextMenu(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IServer* pServer)
	: CContextMenuBase(pDoc), m_pServer(pServer)
{
}

CServerContextMenu::~CServerContextMenu()
{
}

void CServerContextMenu::AddEntries()
{
	try
	{
		m_Menu.AppendMenu(MF_STRING | (m_pServer->Connected && !m_pDoc->GetReadOnlyFlag() && !m_pServer->IsLockCmds) ? MF_ENABLED : MF_GRAYED, ID_REPLICATION_GROUP, "&Add Replication Group...");
		m_Menu.AppendMenu(MF_STRING | (m_pServer->Connected && !m_pDoc->GetReadOnlyFlag() && !m_pServer->IsLockCmds) ? MF_ENABLED : MF_GRAYED, ID_IMPORT, "&Import Replication Group");

		AddCommandEntries("Server", m_pServer->Connected && m_pDoc->GetConnectedFlag() && !m_pDoc->GetReadOnlyFlag() && !m_pServer->IsLockCmds);
		m_Menu.AppendMenu(MF_SEPARATOR);

		m_Menu.AppendMenu(MF_STRING | (!m_pServer->Connected && !m_pDoc->GetReadOnlyFlag() && !m_pServer->IsLockCmds) ? MF_ENABLED : MF_GRAYED, ID_DELETE, "&Delete");
		m_Menu.AppendMenu(MF_SEPARATOR);

		m_Menu.AppendMenu(MF_STRING | (m_pServer->Connected && !m_pDoc->GetReadOnlyFlag() && !m_pServer->IsLockCmds) ? MF_ENABLED : MF_GRAYED, ID_PROPERTIES, "&Properties...");

		if (!m_pDoc->GetConnectedFlag() || m_pDoc->GetReadOnlyFlag())
		{
			m_Menu.EnableMenuItem(ID_REPLICATION_GROUP, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			m_Menu.EnableMenuItem(ID_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			m_Menu.EnableMenuItem(ID_PROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

        m_Menu.AppendMenu(MF_SEPARATOR);

        CMenu ScriptMenu;
        ScriptMenu.CreatePopupMenu();
        ScriptMenu.AppendMenu(MF_STRING, ID_SERVERVIEWCONTEXTMENU_SCRIPT, "&Edit...");
        ScriptMenu.AppendMenu(MF_STRING, ID_SERVERVIEWCONTEXTMENU_IMPORT_SCRIPT, "&Import...");

        m_Menu.AppendMenu(MF_POPUP, (UINT) ScriptMenu.m_hMenu, "&Script");
         
      
        if (!m_pDoc->GetConnectedFlag() || 
            !m_pServer->Connected ||
			m_pServer->IsLockCmds)
		{
			m_Menu.EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_SCRIPT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
 	    }

        if (!m_pDoc->GetConnectedFlag() || 
            !m_pServer->Connected ||
            m_pDoc->GetReadOnlyFlag() ||
			m_pServer->IsLockCmds)
  		{
        	m_Menu.EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_IMPORT_SCRIPT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        }

    }
	CATCH_ALL_LOG_ERROR(1041);
}

class CServerLaunchCmdParam
{
public:
	CServerLaunchCmdParam(IStream*                            pStream,
						  enum ContextMenuCommands            eId,
						  enum TDMFOBJECTSLib::tagTdmfCommand eCommand,
						  const char*                         cstrArg)
		: m_pStream(pStream), m_eId(eId), m_eCommand(eCommand), m_cstrArg(cstrArg)
	{
		m_cstrArg = cstrArg;
	}

	virtual ~CServerLaunchCmdParam() {}

public:
	IStream*                            m_pStream;
	enum ContextMenuCommands            m_eId;
	enum TDMFOBJECTSLib::tagTdmfCommand m_eCommand;
	CString                             m_cstrArg;
};

static UINT LaunchCommandThread(LPVOID pParam)
{
	CoInitialize(NULL);

	CServerLaunchCmdParam* pLaunchCmdParam = (CServerLaunchCmdParam*)pParam;

	// Retrieve the interface pointer within your thread routine
	TDMFOBJECTSLib::IServerPtr pServer;
	if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pLaunchCmdParam->m_pStream, TDMFOBJECTSLib::IID_IServer, (LPVOID*)&pServer)))
	{
		long nRetCode = 0;
		CComBSTR bstrMessage;
		std::ostringstream ossOptions;
		bool bWindows = (strstr(pServer->OSType, "Windows") != 0 ||
				         strstr(pServer->OSType, "windows") != 0 ||
				         strstr(pServer->OSType, "WINDOWS") != 0);

		if ((bWindows) && (pLaunchCmdParam->m_eId == ID_CMD_START))
		{
			ossOptions << "-a -f " << (LPCSTR)pLaunchCmdParam->m_cstrArg;
		}
		else
		{
			ossOptions << "-a " << (LPCSTR)pLaunchCmdParam->m_cstrArg;
		}
		
		// Issue a Kill PMD before any Stop command
		if (pLaunchCmdParam->m_eId == ID_CMD_STOP_IMMEDIATE)
		{
			nRetCode = pServer->LaunchCommand(TDMFOBJECTSLib::CMD_KILL_PMD, ossOptions.str().c_str(), "", &bstrMessage);
		}
		if (nRetCode == 0)
		{
			nRetCode = pServer->LaunchCommand(pLaunchCmdParam->m_eCommand, ossOptions.str().c_str(), "", &bstrMessage);
		}
		
		// Display an error msg
		if (nRetCode != 0)
		{
			_bstr_t bstrTmp(bstrMessage, true);
			CString cstrTitle;
			CString strMenuString =	(BSTR)pServer->Parent->Parent->GetCommandMenuString("Server", pLaunchCmdParam->m_eId);
			cstrTitle.Format("'%s %s' on '%S'", strMenuString, ossOptions.str().c_str(), (BSTR)pServer->Name);
			
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

		pServer->UnlockCmds();
	}
	
	// CLeanup
	delete pLaunchCmdParam;

	CoUninitialize();

	return 0;
}

long CServerContextMenu::LaunchCommand(enum ContextMenuCommands eId, enum TDMFOBJECTSLib::tagTdmfCommand eCommand, const char* pszArg)
{
	bool bLaunchCommand = false;

	if (m_pServer->RegKey.length() == 0)
	{
		CString strMsg;
		strMsg.Format("The registration key is not set for the server '%S'.  Are you sure you want to continue?",
					  (BSTR)m_pServer->Name);
		if (MessageBox(GetActiveWindow(), strMsg, "Empty Registration Key",
			MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL) == IDNO)
		{
			return 1;
		}
	}

	CString strMenuString =	(BSTR)m_pDoc->m_pSystem->GetCommandMenuString("Server", eId);

	CString strMsg;
	strMsg.Format("Are you sure you want to send a '%s' command to the server '%S'?",
				  strMenuString, (BSTR)m_pServer->Name);
	if (MessageBox(GetActiveWindow(), strMsg, "Confirm Server Command",
		MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
	{
		bLaunchCommand = true;
	}

	if (bLaunchCommand)
	{
		CServerLaunchCmdParam* pParam = new CServerLaunchCmdParam(NULL, eId, eCommand, pszArg);
		if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(TDMFOBJECTSLib::IID_IServer, (LPUNKNOWN)m_pServer, &pParam->m_pStream)))
		{
			m_pServer->LockCmds();
			AfxBeginThread(LaunchCommandThread, pParam, 0, 0, 0, NULL);
		}
		else
		{
			AfxMessageBox("Unexpected Error: 996", MB_OK | MB_ICONINFORMATION);
			delete pParam;
		}
	}

	return 0;
}

void CServerContextMenu::OnCommand(UINT nCommand)
{
	CWaitCursor WaitCursor;

	switch (nCommand)
	{
	case ID_REPLICATION_GROUP:
		try
		{
			TDMFOBJECTSLib::IReplicationGroupPtr pRG = m_pServer->CreateNewReplicationGroup();
			
			// Fire an Object New (add) Notification
			CViewNotification VN;
			VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_ADD;
			VN.m_pUnk = (IUnknown*) pRG;
			m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

			CReplicationGroupPropertySheet RGPropertySheet(m_pDoc, "New Replication Group Properties", NULL, 0, pRG, TRUE);
			RGPropertySheet.DoModal();
		}
		CATCH_ALL_LOG_ERROR(1042);
		break;
		
	case ID_CMD_LAUNCH_BACKFRESH:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_BACKFRESH, TDMFOBJECTSLib::CMD_LAUNCH_BACKFRESH);
		}
		CATCH_ALL_LOG_ERROR(1043);
		break;
		
	case ID_CMD_KILL_BACKFRESH:
		try
		{
			LaunchCommand(ID_CMD_KILL_BACKFRESH, TDMFOBJECTSLib::CMD_KILL_BACKFRESH);
		}
		CATCH_ALL_LOG_ERROR(1044);
		break;
		
	case ID_CMD_LAUNCH_PMD:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_PMD, TDMFOBJECTSLib::CMD_LAUNCH_PMD);
		}
		CATCH_ALL_LOG_ERROR(1045);
		break;
		
	case ID_CMD_KILL_PMD:
		try
		{
			LaunchCommand(ID_CMD_KILL_PMD, TDMFOBJECTSLib::CMD_KILL_PMD);
		}
		CATCH_ALL_LOG_ERROR(1046);
		break;
		
	case ID_CMD_LAUNCH_REFRESH_SMART:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_REFRESH_SMART, TDMFOBJECTSLib::CMD_LAUNCH_REFRESH);
		}
		CATCH_ALL_LOG_ERROR(1047);
		break;


	case ID_CMD_LAUNCH_REFRESH_FULL:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_REFRESH_FULL, TDMFOBJECTSLib::CMD_LAUNCH_REFRESH, "-f");
		}
		CATCH_ALL_LOG_ERROR(1048);
		break;

	case ID_CMD_LAUNCH_REFRESH_CHECKSUM:
		try
		{
			LaunchCommand(ID_CMD_LAUNCH_REFRESH_CHECKSUM, TDMFOBJECTSLib::CMD_LAUNCH_REFRESH, "-c");
		}
		CATCH_ALL_LOG_ERROR(1049);
		break;
		
	case ID_CMD_KILL_REFRESH:
		try
		{
			LaunchCommand(ID_CMD_KILL_REFRESH, TDMFOBJECTSLib::CMD_KILL_REFRESH);
		}
		CATCH_ALL_LOG_ERROR(1050);
		break;
		
	case ID_CMD_START:
		try
		{
			LaunchCommand(ID_CMD_START, TDMFOBJECTSLib::CMD_START);
		}
		CATCH_ALL_LOG_ERROR(1051);
		break;
		
	case ID_CMD_STOP:
		try
		{
			LaunchCommand(ID_CMD_STOP, TDMFOBJECTSLib::CMD_STOP);
		}
		CATCH_ALL_LOG_ERROR(1052);		
		break;

	case ID_CMD_STOP_IMMEDIATE:
		try
		{
			LaunchCommand(ID_CMD_STOP_IMMEDIATE, TDMFOBJECTSLib::CMD_STOP);
		}
		CATCH_ALL_LOG_ERROR(1053);		
		break;

	case ID_CMD_CHECKPOINT_ON:
		try
		{
			LaunchCommand(ID_CMD_CHECKPOINT_ON, TDMFOBJECTSLib::CMD_CHECKPOINT, "on");
		}
		CATCH_ALL_LOG_ERROR(1054);
		break;

	case ID_CMD_CHECKPOINT_OFF:
		try
		{
			LaunchCommand(ID_CMD_CHECKPOINT_OFF, TDMFOBJECTSLib::CMD_CHECKPOINT, "off");
		}
		CATCH_ALL_LOG_ERROR(1055);
		break;

	case ID_CMD_KILL_RMD:
		try
		{
			LaunchCommand(ID_CMD_KILL_RMD, TDMFOBJECTSLib::CMD_KILL_RMD);
		}
		CATCH_ALL_LOG_ERROR(1056);
		break;

		
	case ID_DELETE:
		try
		{
			CString strMsg;
			strMsg.Format("Are you sure you want to delete the server '%S'?\n\n"
						  "This will only remove it from the database.  If you want "
						  "to completely remove it from your system, you must "
						  "uninstalled it from its physical location.\n\n"
						  "Also note that all the underlying replication groups will be lost.",
						  (BSTR)m_pServer->Name);
			
			if (MessageBox(GetActiveWindow(), strMsg, "Confirm Server Delete",
				MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
			{
				CWaitCursor wait;
				
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SERVER_REMOVE;
				VN.m_pUnk = (IUnknown*) m_pServer;
				m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
				
				TDMFOBJECTSLib::IDomainPtr pDomain = m_pServer->Parent;
				if (pDomain->RemoveServer(m_pServer) != 0)
				{
					MessageBox(GetActiveWindow(), "Cannot delete server from database.", "Error", MB_OK | MB_ICONERROR);
				}
			}
		}
		CATCH_ALL_LOG_ERROR(1057);
		break;

	case ID_IMPORT:
		try
		{
			CString strMsg;
			strMsg.Format("Are you sure you want to import all the replication groups defined (*.cfg) on server '%S'?\n\n"
						  "Replication groups that already exist or that don't have a valid target server will not be imported.",
						  (BSTR)m_pServer->Name);
			if (MessageBox(GetActiveWindow(), strMsg, "Confirm Replication Groups Importation",
				MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
			{
				CComBSTR bstrTmp;
				int nRetCode = m_pServer->Import(&bstrTmp);

				// Force a refresh
				m_pDoc->OnToolsRefresh();

				if (nRetCode != 0)
				{
					_bstr_t bstrMessage(bstrTmp, true);
					if (bstrMessage.length() > 0)
					{
						AfxMessageBox(bstrMessage, MB_OK | MB_ICONINFORMATION);
					}
					else
					{
						std::ostringstream oss;
						oss << "The command has returned the following error code: " << nRetCode;
						AfxMessageBox(oss.str().c_str(), MB_OK | MB_ICONINFORMATION);
					}
				}
			}
		}
		CATCH_ALL_LOG_ERROR(1058);
		break;
		
	case ID_PROPERTIES:
		try
		{
			_bstr_t bstrTitle   = m_pServer->Name + " Properties";
			CServerPropertySheet ServerPropertySheet(m_pDoc, bstrTitle, NULL, 0, m_pServer);
			if (ServerPropertySheet.DoModal() == IDOK)
			{
				// The actions are handled in the property sheet class
			}
		}
		CATCH_ALL_LOG_ERROR(1059);
		break;
	case ID_SERVERVIEWCONTEXTMENU_SCRIPT:
	{
	
		CScriptEditorDlg dlg(m_pServer,m_pDoc->GetReadOnlyFlag());
        dlg.DoModal();
		
	}
	break;

    case ID_SERVERVIEWCONTEXTMENU_IMPORT_SCRIPT:
	{
	
        CImportScriptDlg dlg(m_pServer);
        dlg.DoModal();
		
	}
	break;
	
    default:
		ASSERT(0);
	}		
}

