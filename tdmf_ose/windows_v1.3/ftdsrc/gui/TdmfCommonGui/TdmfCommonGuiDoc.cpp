// TdmfCommonGuiDoc.cpp : implementation of the CTdmfCommonGuiDoc class
//

#include "stdafx.h"
#include "TdmfCommonGui.h"
#include "TdmfCommonGuiDoc.h"
#include "ViewNotification.h"
#include "LoginDlg.h"
#include "OptionPropertySheet.h"
#include "../../tdmf.inc"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiDoc

IMPLEMENT_DYNCREATE(CTdmfCommonGuiDoc, CDocument)

BEGIN_MESSAGE_MAP(CTdmfCommonGuiDoc, CDocument)
	//{{AFX_MSG_MAP(CTdmfCommonGuiDoc)
	ON_COMMAND(ID_TOOLS_REFRESH, OnToolsRefresh)
	ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_OPTIONS, OnUpdateToolsOptions)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_REFRESH, OnUpdateToolsRefresh)
	ON_UPDATE_COMMAND_UI(ID_FILE_CONNECT, OnUpdateFileConnect)
	ON_COMMAND(ID_FILE_CONNECT, OnFileConnect)
	ON_COMMAND(ID_FILE_DISCONNECT, OnFileDisconnect)
	ON_UPDATE_COMMAND_UI(ID_FILE_DISCONNECT, OnUpdateFileDisconnect)
	ON_COMMAND(ID_TOOLS_SHOWSERVERSWARNING, OnToolsShowserverswarning)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SHOWSERVERSWARNING, OnUpdateToolsShowserverswarning)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiDoc construction/destruction

CTdmfCommonGuiDoc::CTdmfCommonGuiDoc() : m_bConnected(FALSE), m_bReadOnly(TRUE), m_bShowServersWarnings(TRUE)
{
	try
	{
		HRESULT hr = m_pSystem.CreateInstance(TDMFOBJECTSLib::CLSID_System);

		DWORD dwType;
		DWORD dwValue;
		ULONG nSize = sizeof(DWORD);
		#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"

		if (SHGetValue(HKEY_CURRENT_USER, FTD_SOFTWARE_KEY, "DtcServersWarnings", &dwType, &dwValue, &nSize) == ERROR_SUCCESS)
		{
			if (dwValue == 0)
			{
				m_bShowServersWarnings = FALSE;
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1118);
}

CTdmfCommonGuiDoc::~CTdmfCommonGuiDoc()
{
}

BOOL CTdmfCommonGuiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	try
	{	
		POSITION pos = GetFirstViewPosition();
		CView* pView = GetNextView(pos);

		long nErr  = 1;
		bool bCancel = false;

		long nNbTrial = 0;

		CLoginDlg LoginDlg;

		do
		{
			if (LoginDlg.m_bSaveInfo 
#ifndef TDMF_IN_A_DLL
				|| (LoginDlg.DoModal() == IDOK)
#endif
				)
			{
				m_pSystem->UserID   = (LPCSTR)LoginDlg.m_cstrUserID;
				m_pSystem->Password = (LPCSTR)LoginDlg.m_cstrPassword;
				m_pSystem->Name     = (LPCSTR)LoginDlg.m_cstrCollectorName;
					
				nErr = m_pSystem->Init((long)((pView->GetParentFrame())->m_hWnd));
					
				LoginDlg.m_bSaveInfo = FALSE;
					
				nNbTrial++;
			}
			else
			{
				m_pSystem->Name     = "";
				m_pSystem->UserID   = "";
				m_pSystem->Password = "";
					
				nErr = m_pSystem->Init((long)((pView->GetParentFrame())->m_hWnd));
				bCancel = true;
			}
		} while ((nErr == 1) && (nNbTrial < 3) && (!bCancel));

		if (nErr == 0)
		{
			HRESULT hr = m_pSystem->raw_RequestOwnership(TRUE);

			if (FAILED(hr))
			{
				AfxMessageBox("Unable to connect to collector.", MB_OK | MB_ICONWARNING);
				m_pSystem->Uninitialize();
			}
			else
			{
				if (hr == S_FALSE)
				{
					AfxMessageBox("Maximum collector connections exceeded.", MB_OK | MB_ICONWARNING);
				}
				else
				{
					SetConnectedFlag(TRUE);
					if (m_pSystem->Open("") != 0)
					{
						AfxMessageBox("Collector's registration key is invalid.", MB_OK | MB_ICONINFORMATION);
					}
				}
			}
		}
		else if (bCancel == false)
		{
			std::string strMsg;

			switch(nErr)
			{
			case 1:
				strMsg = "Database connection failure.";
				break;
			case 2:
			case 3:
				strMsg = "Registry reading failure.";
				break;
			case 4:
				strMsg = "Object creation failure.";
				break;
			case 5:
				strMsg = "Communication protocole mismatch.";
				break;
			case 6:
				strMsg = "Database access error.";
				break;
			case 8:
				strMsg = "Invalid registration key.";
				break;
			default:
				strMsg = "System initialization failure.";
			}
			AfxMessageBox(strMsg.c_str(), MB_OK | MB_ICONINFORMATION);
		}
	}
	catch(...)
	{
		AfxMessageBox("System initialization failure.", MB_OK | MB_ICONINFORMATION);
		SetConnectedFlag(FALSE);
	}

	return TRUE;
}

void CTdmfCommonGuiDoc::OnCloseDocument() 
{
	try
	{
		if (m_pSystem != NULL)
		{
			m_pSystem->raw_RequestOwnership(FALSE);
			m_pSystem->Uninitialize();
		}
	}
	CATCH_ALL_LOG_ERROR(1119);

	CDocument::OnCloseDocument();
}


/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiDoc serialization

void CTdmfCommonGuiDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiDoc diagnostics

#ifdef _DEBUG
void CTdmfCommonGuiDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTdmfCommonGuiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiDoc commands

void CTdmfCommonGuiDoc::OnToolsRefresh() 
{
	try
	{
		CWaitCursor Wait;

		// Notify views
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REFRESH_SYSTEM;
		UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
	}
	CATCH_ALL_LOG_ERROR(1120);
}

void CTdmfCommonGuiDoc::OnUpdateToolsRefresh(CCmdUI* pCmdUI) 
{
	if (!GetConnectedFlag() || m_pSystem->IsLockCmds)
	{
		pCmdUI->Enable(FALSE);
	}
}

void CTdmfCommonGuiDoc::OnToolsOptions() 
{
	CWaitCursor* pWaitCursor =  new CWaitCursor();   // display wait cursor

	COptionPropertySheet OptionPS(this, "Options");

	delete pWaitCursor;

	OptionPS.DoModal();
}

void CTdmfCommonGuiDoc::OnUpdateToolsOptions(CCmdUI* pCmdUI) 
{
	// Only if reg key check is enabled for the gui
	//{
		pCmdUI->Enable(FALSE);
	//}
}

void CTdmfCommonGuiDoc::OnUpdateFileConnect(CCmdUI* pCmdUI) 
{
	if (GetConnectedFlag() || m_pSystem->IsLockCmds)
	{
		pCmdUI->Enable(FALSE);
	}	
}

void CTdmfCommonGuiDoc::OnUpdateFileDisconnect(CCmdUI* pCmdUI) 
{
	if (!GetConnectedFlag() || m_pSystem->IsLockCmds)
	{
		pCmdUI->Enable(FALSE);
	}
}

void CTdmfCommonGuiDoc::OnFileConnect() 
{
	CLoginDlg LoginDlg;
	BOOL      bCancelled = FALSE;
	
	if (LoginDlg.DoModal() == IDOK)
	{
		m_pSystem->UserID   = (LPCSTR)LoginDlg.m_cstrUserID;
		m_pSystem->Password = (LPCSTR)LoginDlg.m_cstrPassword;
		m_pSystem->Name     = (LPCSTR)LoginDlg.m_cstrCollectorName;
	}
	else
	{
		m_pSystem->UserID   = "";
		m_pSystem->Password = "";
		m_pSystem->Name     = "";
		
		bCancelled = TRUE;
	}

	POSITION pos = GetFirstViewPosition();
	CView* pView = GetNextView(pos);

	int nErr = m_pSystem->Init((long)((pView->GetParentFrame())->m_hWnd));

	if (nErr == 0)
	{
		HRESULT hr = m_pSystem->raw_RequestOwnership(TRUE);
		
		if (FAILED(hr))
		{
			AfxMessageBox("Unable to connect to collector.", MB_OK | MB_ICONWARNING);
			m_pSystem->Uninitialize();
		}
		else
		{
			if (hr == S_FALSE)
			{
				AfxMessageBox("Maximum collector connections exceeded.", MB_OK | MB_ICONWARNING);
			}
			else
			{
				OnToolsRefresh();

				if ((!GetConnectedFlag()) && (!bCancelled))
				{
					CString cstrMsg = "Unable to connect to collector or database server.";
					MessageBox(GetActiveWindow(), cstrMsg, "Connection Error", MB_ICONINFORMATION | MB_APPLMODAL);
				}
			}
		}
	}
	else if (bCancelled == FALSE)
	{
		std::string strMsg;
		
		switch(nErr)
		{
		case 1:
			strMsg = "Database connection failure.";
			break;
		case 2:
		case 3:
			strMsg = "Registry reading failure.";
			break;
		case 4:
			strMsg = "Object creation failure.";
			break;
		case 5:
			strMsg = "Communication protocole mismatch.";
			break;
		case 6:
			strMsg = "Database access error.";
			break;
		case 8:
			strMsg = "Invalid registration key.";
			break;
		default:
			strMsg = "System initialization failure.";
		}
		AfxMessageBox(strMsg.c_str(), MB_OK | MB_ICONINFORMATION);
	}
}

void CTdmfCommonGuiDoc::OnFileDisconnect() 
{
	m_pSystem->Name = "";
	m_pSystem->UserID   = "";
	m_pSystem->Password = "";

	OnToolsRefresh();	
}

BOOL CTdmfCommonGuiDoc::UserIsAdministrator()
{
	if (m_pSystem->UserRole == _bstr_t("Administrator"))
	{
		return TRUE;
	}

	return FALSE;
}

void CTdmfCommonGuiDoc::OnUpdateToolsShowserverswarning(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bShowServersWarnings ? 1 : 0);
}

void CTdmfCommonGuiDoc::OnToolsShowserverswarning() 
{
	m_bShowServersWarnings = m_bShowServersWarnings ? FALSE : TRUE;

	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
					   FTD_SOFTWARE_KEY,
					   NULL,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey,
					   NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, "DtcServersWarnings", 0, REG_DWORD, (LPBYTE)&m_bShowServersWarnings, sizeof(m_bShowServersWarnings));
		RegFlushKey(hKey);
		RegCloseKey(hKey);
	}
}
