// ServerPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ServerPropertySheet.h"
#include "ProgressInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerPropertySheet

IMPLEMENT_DYNAMIC(CServerPropertySheet, CPropertySheet)

CServerPropertySheet::CServerPropertySheet(CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage, TDMFOBJECTSLib::IServer* pServer)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage), m_ServerGeneralPage(pServer), m_pDoc(pDoc), m_pServer(pServer),
	m_bReboot(false), m_bTcpWindowSizeChanged(false)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;
	AddPage(&m_ServerGeneralPage);

 /*   m_ScriptEditorPage.m_pServer = m_pServer;
    m_ScriptEditorPage.m_bReadOnly = !m_pServer->Connected;
    AddPage(&m_ScriptEditorPage);
    */
}

CServerPropertySheet::CServerPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage, TDMFOBJECTSLib::IServer* pServer)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage), m_ServerGeneralPage(pServer), m_pDoc(pDoc), m_pServer(pServer),
	m_bReboot(false), m_bTcpWindowSizeChanged(false)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;
	AddPage(&m_ServerGeneralPage);

  /*  m_ScriptEditorPage.m_pServer = m_pServer;
    m_ScriptEditorPage.m_bReadOnly = !m_pServer->Connected;
    AddPage(&m_ScriptEditorPage);
    */
}

CServerPropertySheet::~CServerPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CServerPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CServerPropertySheet)
	ON_COMMAND(ID_APPLY_NOW, OnApplyNow)
	ON_COMMAND(IDOK, OnOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

class CRebootParam
{
public:
	CRebootParam() : m_pStream(NULL) {}
	virtual ~CRebootParam() {}

public:
	IStream*   m_pStream;
	bool       m_bReboot;
	CDocument* m_pDoc;
};

static UINT LaunchRebootThread(LPVOID pParam)
{
	CoInitialize(NULL);

	CRebootParam* pRebootParam = (CRebootParam*)pParam;

	// Retrieve the interface pointer within your thread routine
	TDMFOBJECTSLib::IServerPtr pServer;
	if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pRebootParam->m_pStream, TDMFOBJECTSLib::IID_IServer, (LPVOID*)&pServer)))
	{
		long nErr = pServer->SaveToDB();
				
		if (nErr != 0)
		{
			// ERR_BAD_OR_MISSING_REGISTRATION_KEY
			if (nErr == TDMFOBJECTSLib::TDMF_ERROR_BAD_OR_MISSING_REGISTRATION_KEY)
			{
				MessageBox(AfxGetMainWnd()->GetSafeHwnd(), "Warning: You have entered an invalid registation key.", "Error", MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				MessageBox(AfxGetMainWnd()->GetSafeHwnd(), "Cannot save changes to database.", "Error", MB_OK | MB_ICONERROR);
			}
		}
	
		::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_SERVER_MODIFY, pServer->Parent->GetKey(), pServer->GetKey());
		
		pServer->UnlockCmds();
	}
	
	// CLeanup
	delete pRebootParam;

	CoUninitialize();

	return 0;
}

// CServerPropertySheet message handlers
void CServerPropertySheet::OnApplyNow()
{
	m_bContinue = true;

	if (m_bReboot)
	{
		if (strstr(m_pServer->OSType, "Windows") != 0 ||
			strstr(m_pServer->OSType, "windows") != 0 ||
			strstr(m_pServer->OSType, "WINDOWS") != 0)
		{
			if (MessageBox("Saving changes will stop the groups and reboot the server.  Are you sure you want to save your changes now?",
				"Confirm Server Reboot", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) != IDYES)
			{
				m_bContinue = false;
			}
		}
		else
		{
			if (MessageBox("Saving changes will stop the groups and do a kernel rebuild.  Are you sure you want to save your changes now?",
				"Confirm Kernel Rebuild", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) != IDYES)
			{
				m_bContinue = false;
			}
		}
	}
	else if (m_bTcpWindowSizeChanged)
 	{
 		MessageBox("Changes done to the TCP Window size will not take effect until groups are restarted.",
 				   "Warning", MB_OK | MB_ICONWARNING | MB_APPLMODAL);
	}

	if (m_bContinue)
	{
		CWaitCursor WaitCursor;
		BOOL bSave = FALSE;

		if (m_ServerGeneralPage.m_bPageModified && m_ServerGeneralPage.OnKillActive())
		{
			m_ServerGeneralPage.OnApply();
			bSave = TRUE;
		}

		if (bSave)
		{
			CRebootParam* pParam = new CRebootParam;
			if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(TDMFOBJECTSLib::IID_IServer, (LPUNKNOWN)m_pServer, &pParam->m_pStream)))
			{
				pParam->m_bReboot = m_bReboot;
				pParam->m_pDoc    = m_pDoc;
				m_pServer->LockCmds();
				AfxBeginThread(LaunchRebootThread, pParam, 0, 0, 0, NULL);
			}
			else
			{
				AfxMessageBox("Unexpected Error: 994", MB_OK | MB_ICONINFORMATION);
				delete pParam;
			}
		}
	}
}

void CServerPropertySheet::OnOK()
{
	if (!m_ServerGeneralPage.m_bPageModified)
	{
		EndDialog(IDOK);
	}
	else
	{
		if ((!m_ServerGeneralPage.m_bPageModified) || m_ServerGeneralPage.OnKillActive())
		{
			OnApplyNow();

			if (m_bContinue)
			{
				EndDialog(IDOK);
			}
		}
	}
}

BOOL CServerPropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	CWnd* pOKButton = GetDlgItem (IDOK);
	pOKButton->SetWindowText("Save");

	m_bReboot = false;

	return bResult;
}
