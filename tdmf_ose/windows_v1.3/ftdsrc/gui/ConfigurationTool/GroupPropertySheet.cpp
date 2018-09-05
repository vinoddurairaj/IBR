// GroupPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "GroupPropertySheet.h"
#include "Command.h"
#include  <io.h>
#include <direct.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupPropertySheet

IMPLEMENT_DYNAMIC(CGroupPropertySheet, CPropertySheet)

CGroupPropertySheet::CGroupPropertySheet(CGroupConfig* pGroupConfig, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(pszCaption, pParentWnd, iSelectPage), m_pGroupConfig(pGroupConfig),
	  m_SystemPage(pGroupConfig), m_DevicesPage(pGroupConfig, pszCaption), m_TunablesPage(pGroupConfig)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;

	AddPage(&m_SystemPage);
	AddPage(&m_DevicesPage);
	AddPage(&m_TunablesPage);
}

CGroupPropertySheet::~CGroupPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CGroupPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CGroupPropertySheet)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupPropertySheet message handlers

void CGroupPropertySheet::OnOK()
{
	CString cstrTitle;
	GetWindowText(cstrTitle);

	SetActivePage(0);
	if (m_SystemPage.Validate() && m_DevicesPage.Validate() && m_TunablesPage.Validate())
	{
		if (m_SystemPage.SaveValues() && m_DevicesPage.SaveValues() && m_TunablesPage.SaveValues())
		{
			// Save group info to cfg file
			m_pGroupConfig->SaveToFile();

			// Create PStore dir
			char lcaDrv[_MAX_DRIVE];
			char lcaDir[_MAX_DIR];
			_splitpath(m_pGroupConfig->GetPStore(), lcaDrv, lcaDir, NULL, NULL);
			CString cstrDir;
			cstrDir.Format("%s%s", lcaDrv, lcaDir);
			if (_access(cstrDir, 0) == -1)
			{
				CString lszMsg;
				lszMsg.Format("Would you like to create the directory: %s ?", cstrDir);
				if (IDYES == ::MessageBox (NULL, lszMsg, cstrTitle, MB_YESNO | MB_ICONQUESTION))
				{
					_mkdir(cstrDir);
				}
			}

			bool bAlreadyStarted = m_pGroupConfig->IsStarted();

			// Start group ?
			if (m_pGroupConfig->IsValid() && (bAlreadyStarted == false))
			{
				if (IDYES == ::MessageBox(NULL, "Would you like to start the logical group ?\n\n(Remember to copy the .cfg file to the secondary server)\n",
											cstrTitle, MB_YESNO|MB_ICONQUESTION))
				{
					CWaitCursor WaitCursor;
					
					CCommand::StartGroup(m_pGroupConfig->GetGroupNb());
				}
			}

			// Save Tunables
			m_pGroupConfig->SaveTunables();

			if (bAlreadyStarted && m_TunablesPage.IsPMDRestartNeeded())
			{
				CString cstrMsg;

				if (m_TunablesPage.m_bJournalLess)
				{
					cstrMsg = "To activate journal-less option, the group's PMD/RMD need to be restarted.\nDo you want to restart them now?";
				}
				else
				{
					cstrMsg = "To deactivate journal-less option, the group's PMD/RMD need to be restarted.\nDo you want to restart them now?";
				}
				
				if (MessageBox(cstrMsg, "Journal-less Warning", MB_YESNO) == IDYES)
				{
					CWaitCursor WaitCursor;
					
					CCommand::KillPMD(m_pGroupConfig->GetGroupNb());
					CCommand::LaunchPMD(m_pGroupConfig->GetGroupNb());
				}
			}

			EndDialog(IDOK);
		}
	}
}

void CGroupPropertySheet::OnCancel()
{
	EndDialog(IDCANCEL);
}

BOOL CGroupPropertySheet::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	ASSERT(pResult != NULL);
	NMHDR* pNMHDR = (NMHDR*)lParam;

	if (pNMHDR->code == TCN_SELCHANGE)
	{
		m_SystemPage.SaveValues();
		m_DevicesPage.SaveValues();
		m_TunablesPage.SaveValues();
	}

	return CPropertySheet::OnNotify(wParam, lParam, pResult);
}
