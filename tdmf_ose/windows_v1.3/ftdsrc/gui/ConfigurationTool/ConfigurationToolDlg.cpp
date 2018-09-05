// ConfigurationToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "ConfigurationToolDlg.h"
#include "Splash.h"
#include "DeleteGroup.h"
#include "Command.h"
#include "GroupPropertySheet.h"
#include "AddGroup.h"
#include <io.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
protected:
	CConfigurationToolDlg* m_pParent;

public:
	CAboutDlg(CConfigurationToolDlg* pParent);

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CEdit	m_EditLicenseKey;
	CStatic	m_Icon;
	CString	m_cstrLicenseKey;
	CString	m_cstrExpDate;
	CString	m_cstrAboutVersion;
	CString	m_cstrCopyright;
	BOOL	m_bEditLicenseKey;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAboutOk();
	afx_msg void OnCheckAboutLicenseKey();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg(CConfigurationToolDlg* pParent) : m_pParent(pParent), CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_cstrLicenseKey = _T("");
	m_cstrExpDate = _T("");
	m_cstrAboutVersion = _T("");
	m_cstrCopyright = _T("");
	m_bEditLicenseKey = FALSE;
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_EDI_LICENSE_KEY, m_EditLicenseKey);
	DDX_Control(pDX, IDC_STATIC_SFTK, m_Icon);
	DDX_Text(pDX, IDC_EDI_LICENSE_KEY, m_cstrLicenseKey);
	DDX_Text(pDX, IDC_STATIC_EXP_DATE, m_cstrExpDate);
	DDX_Text(pDX, IDC_STATIC_ABOUT_VERSION, m_cstrAboutVersion);
	DDX_Text(pDX, IDC_STATIC_COPYRIGHT, m_cstrCopyright);
	DDX_Check(pDX, IDC_CHECK_ABOUT_LICENSE_KEY, m_bEditLicenseKey);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_BUTTON_ABOUT_OK, OnButtonAboutOk)
	ON_BN_CLICKED(IDC_CHECK_ABOUT_LICENSE_KEY, OnCheckAboutLicenseKey)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pParent->m_ServerConfig.GetLicenseKey(m_cstrLicenseKey, m_cstrExpDate);

	CString strVersion	= m_pParent->m_ResourceManager.GetFullProductName();
	strVersion			+= _T(" Configuration Tool Version ");
	strVersion			+= m_pParent->m_ResourceManager.GetProductVersion();
    strVersion			+= _T("  Build ");
    strVersion			+= m_pParent->m_ResourceManager.GetProductBuild();
	m_cstrAboutVersion = strVersion;

	m_Icon.SetIcon(m_pParent->m_ResourceManager.GetApplicationIcon());
	
	CString cstrWndTitle;
	cstrWndTitle.Format("About %s Configuration Tool", m_pParent->m_ResourceManager.GetFullProductName());
	SetWindowText(cstrWndTitle);

	m_cstrCopyright = "Copyright (c) 2003, 2004 Softek Storage Solutions Corporation";

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnCheckAboutLicenseKey() 
{
	UpdateData();

	if (m_bEditLicenseKey)
	{
		m_EditLicenseKey.SetReadOnly(FALSE);
	}
	else
	{
		m_EditLicenseKey.SetReadOnly(TRUE);
	}
}

void CAboutDlg::OnButtonAboutOk() 
{
	UpdateData();
	m_pParent->m_ServerConfig.WriteLicenseKey(m_cstrLicenseKey);

	EndDialog(0);
}


/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolDlg dialog

CConfigurationToolDlg::CConfigurationToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigurationToolDlg::IDD, pParent), m_bShutdown(false)
{
	//{{AFX_DATA_INIT(CConfigurationToolDlg)
	m_cstrBABSize = _T("");
	m_cstrTcpWindowSize = _T("");
	m_cstrPort = _T("");
	m_cstrNote = _T("");
	//}}AFX_DATA_INIT

	// Init Rebranding dll
	char szFileName[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	GetModuleFileName(NULL, szFileName, _MAX_PATH);
	_splitpath(szFileName, drive, dir, fname, ext);
	_makepath(szFileName, drive, dir, "RBRes", "dll");
	m_ResourceManager.SetResourceDllName(szFileName);

	// Get Application's Icon
	m_hIcon = m_ResourceManager.GetApplicationIcon();
}

void CConfigurationToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigurationToolDlg)
	DDX_Control(pDX, IDC_LIST_GROUPS, m_ListGroups);
	DDX_Control(pDX, IDC_EDIT_BAB_SIZE, m_EditBABSize);
	DDX_Control(pDX, IDC_SPIN_BAB_SIZE, m_SpinBABSize);
	DDX_Text(pDX, IDC_EDIT_BAB_SIZE, m_cstrBABSize);
	DDX_Text(pDX, IDC_EDIT_TCP_WIN_SIZE, m_cstrTcpWindowSize);
	DDX_Text(pDX, IDC_EDIT_PORT_NUM, m_cstrPort);
	DDX_Text(pDX, IDC_EDIT_GROUP_NOTE, m_cstrNote);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConfigurationToolDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigurationToolDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_LIST_GROUPS, OnSelchangeListGroups)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_GROUP, OnButtonDeleteGroup)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY_GROUP, OnButtonModifyGroup)
	ON_BN_CLICKED(IDC_BUTTON_ADD_GROUP, OnButtonAddGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolDlg message handlers

BOOL CConfigurationToolDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CSplashWnd::EnableSplashScreen(TRUE);
	CSplashWnd::ShowSplashScreen(&m_ResourceManager, this);

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CString cstrWndTitle = m_ResourceManager.GetFullProductName();
	cstrWndTitle += " Configuration Tool";
	SetWindowText(cstrWndTitle);

	//////////////////////////////////////////////////////////////////////
	//LoadInitialValues

	// Init BAB size
	m_SpinBABSize.SetBuddy(&m_EditBABSize);
	m_SpinBABSize.SetRange(0, m_ServerConfig.GetMaxBABSize());
	UDACCEL	Accel = {1, 32};
	m_SpinBABSize.SetAccel(1, &Accel);

	int nBABSize = m_ServerConfig.GetBABSize();
	if (nBABSize == -1)
	{
		m_cstrBABSize = _T("");
		m_SpinBABSize.EnableWindow(FALSE);
		m_EditBABSize.EnableWindow(FALSE);
	}
	else
	{
		m_cstrBABSize.Format("%d", nBABSize);
	}

	// Init TCP window size
	m_cstrTcpWindowSize.Format("%d", m_ServerConfig.GetTcpWindowSize());

	// Init Port Number
	m_cstrPort.Format("%d", m_ServerConfig.GetPrimaryPortNumber());

	// Init Group list
	CGroupConfig* pGroupConfig = m_ServerConfig.GetFirstGroup();
	while (pGroupConfig != NULL)
	{
		CString cstrGroupID;
		cstrGroupID.Format("%03d", pGroupConfig->GetGroupNb());
		m_ListGroups.AddString(cstrGroupID);

		pGroupConfig = m_ServerConfig.GetNextGroup();
	}
	// Select the last item in the combo box.
	int nCount = m_ListGroups.GetCount();
	if (nCount > 0)
	{
		m_ListGroups.SetCurSel(nCount-1);

		CString cstrGroupNb;
		m_ListGroups.GetLBText(nCount-1, cstrGroupNb);
		pGroupConfig = m_ServerConfig.GetGroup(atoi(cstrGroupNb));
		m_cstrNote = pGroupConfig->GetNote();
	}

	// Update display
	UpdateData(FALSE);

	// Get license key info
	CString strDate;
	int iRc = m_ServerConfig.InitLicenseKey(strDate);
	if(iRc < 0)
	{
		strDate += ": Goto the about box to update your key.";
		::MessageBox(NULL, strDate, m_ResourceManager.GetFullProductName(), MB_OK);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CConfigurationToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout(this);
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CConfigurationToolDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CConfigurationToolDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CConfigurationToolDlg::OnSelchangeListGroups() 
{
	// Update Note field
	CString cstrGroupNb;
	m_ListGroups.GetLBText(m_ListGroups.GetCurSel(), cstrGroupNb);

	CGroupConfig* pGroupConfig = m_ServerConfig.GetGroup(atoi(cstrGroupNb));
	m_cstrNote = pGroupConfig->GetNote();
	UpdateData(FALSE);
}

void CConfigurationToolDlg::ShutDownSystem()
{
	HANDLE hToken;              // handle to process token 
	TOKEN_PRIVILEGES tkp;       // pointer to token structure 
 
	BOOL fResult;               // system shutdown flag 
 
	// Get the current process token handle so we can get shutdown privilege. 
 	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
 
	// Get the LUID for shutdown privilege. 
 	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
 
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
	// Get shutdown privilege for this process. 
 	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
 
	// TODO: Add code to handle cancel or change the message
	// Display the shutdown dialog box and start the time-out countdown. 
	fResult = InitiateSystemShutdown(NULL,                                  // shut down local computer 
									 "Click on the main window and press the Escape key to cancel shutdown.",  // message to user 
									 20,                                    // time-out period 
									 FALSE,                                 // ask user to close apps 
									 TRUE);                                 // reboot after shutdown 

	// Disable shutdown privilege. 
	tkp.Privileges[0].Attributes = 0; 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);
}

void CConfigurationToolDlg::CancelShutDownSystem() 
{
	// Cancel Shutdown
	HANDLE hToken;              // handle to process token 
	TOKEN_PRIVILEGES tkp;       // pointer to token structure 
	
	// Get the current process token handle  so we can get shutdown privilege. 
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
	{
		TRACE("OpenProcessToken failed."); 
	}
	
	// Get the LUID for shutdown privilege. 
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
	
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
	
	// Get shutdown privilege for this process. 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
	
	// Cannot test the return value of AdjustTokenPrivileges. 
	if (GetLastError() != ERROR_SUCCESS) 
	{
		TRACE("AdjustTokenPrivileges enable failed."); 
	}
	
	// Prevent the system from shutting down. 
	BOOL fResult = AbortSystemShutdown(NULL); 
	if (!fResult) 
	{ 
		TRACE("AbortSystemShutdown failed."); 
	} 
	
	// Disable shutdown privilege. 
	tkp.Privileges[0].Attributes = 0; 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
	
	if (GetLastError() != ERROR_SUCCESS)
	{
		TRACE("AdjustTokenPrivileges disable failed."); 
	}
	
	m_bShutdown = false;
}

void CConfigurationToolDlg::OnOK() 
{
	UpdateData();

	if (atoi(m_cstrTcpWindowSize) <= 0)
	{
		MessageBox("The TCP Window Size must be greater than 0", m_ResourceManager.GetFullProductName());
		return;
	}

	if (atoi(m_cstrPort) <= 0)
	{
		MessageBox("The Primary Port Number must be greater than 0", m_ResourceManager.GetFullProductName());
		return;
	}

	if ((atoi(m_cstrBABSize) < 0) || ((UINT)atoi(m_cstrBABSize) > m_ServerConfig.GetMaxBABSize()))
	{
		CString cstrBabMsg;
		cstrBabMsg.Format("The BAB size is outside limits:\nMIN: 0 MB required \nMAX: %d MB.\n",
						  m_ServerConfig.GetMaxBABSize());
		MessageBox(cstrBabMsg, m_ResourceManager.GetFullProductName());
		return;
	}

	// Save values
	m_ServerConfig.SetBABSize(atoi(m_cstrBABSize));
	m_ServerConfig.SetTcpWindowSize(atoi(m_cstrTcpWindowSize));
	m_ServerConfig.SetPrimaryPortNumber(atoi(m_cstrPort));
	m_ServerConfig.Save();

	// Verify BAB Size value before saving it in REGISTRY
	if (m_ServerConfig.NeedToReboot())
	{
		if (IDOK == ::MessageBox(NULL, "Restart The Computer Now?", 
								 m_ResourceManager.GetFullProductName(), MB_OKCANCEL))
		{
			ShutDownSystem();

			// Give the user a chance to cancel the reboot
			m_bShutdown = true;

			return;
		}
    }

	CDialog::OnOK();
}

void CConfigurationToolDlg::OnButtonDeleteGroup() 
{
	// must set this up to check if they selected a group
	if (m_ListGroups.GetCount())
	{
		int nCurSelIndex = m_ListGroups.GetCurSel();
		CString cstrGroup;
		m_ListGroups.GetLBText(nCurSelIndex, cstrGroup);

		CDeleteGroup DeleteGroup;
		if (DeleteGroup.DoModal() == IDOK)
		{
			// killpmd and stop group
			CCommand::StopGroup(atoi(cstrGroup));

			CString cstrFileName;
			// If the group has previously been started, it will have an
			// associated pxxx.cur file which also needs to be deleted.
			cstrFileName.Format("%sp%s.cur", m_ServerConfig.GetInstallPath(), cstrGroup);
			if (_access(cstrFileName, 0) == 0)
			{
				DeleteFile(cstrFileName);
			}
			
			// Delete .cfg file
			cstrFileName.Format("%sp%s.cfg", m_ServerConfig.GetInstallPath(), cstrGroup);
			DeleteFile(cstrFileName);
			
			// Delete from combobox
			m_ListGroups.DeleteString(nCurSelIndex);
			int nNextSel = (nCurSelIndex > 0) ? nCurSelIndex - 1 : 0;
			m_ListGroups.SetCurSel(nNextSel);

			// Update Note field
			if (m_ListGroups.GetCount() > 0)
			{
				CString cstrGroupNew;
				m_ListGroups.GetLBText(nNextSel, cstrGroupNew);
				CGroupConfig* pGroupConfig = m_ServerConfig.GetGroup(atoi(cstrGroupNew));
				m_cstrNote = pGroupConfig->GetNote();
			}
			else
			{
				m_cstrNote = "";
			}

			// Delete from cache
			m_ServerConfig.DeleteGroup(atoi(cstrGroup));

			// Update display
			UpdateData(FALSE);
		}
	}
	else
	{
		::MessageBox(NULL, "No groups available to delete", m_ResourceManager.GetFullProductName(), MB_OK);
	}
}

void CConfigurationToolDlg::OnButtonModifyGroup() 
{
	if (m_ListGroups.GetCount() > 0)
	{
		CString cstrGroup;
		m_ListGroups.GetLBText(m_ListGroups.GetCurSel(), cstrGroup);
		CGroupConfig* pGroupConfig = m_ServerConfig.GetGroup(atoi(cstrGroup));

		pGroupConfig->SaveInitialValues();

		pGroupConfig->ReadConfigFile();

		if (pGroupConfig->ReadTunables() == false)
		{
			CString cstrError;
			cstrError.Format("Failed to read Tunable Parameters for group %03d.\nUsing default values!", atoi(cstrGroup));
			::MessageBox(NULL, cstrError, m_ResourceManager.GetFullProductName(), MB_OK|MB_ICONWARNING);
		}

		CString cstrWndTitle = m_ResourceManager.GetFullProductName();
		cstrWndTitle += " Configuration Tool";
		CGroupPropertySheet GroupPropSheet(pGroupConfig, cstrWndTitle);
		if (GroupPropSheet.DoModal() != IDOK)
		{
			pGroupConfig->RestoreInitialValues();
		}

		// Update Note field
		m_cstrNote = pGroupConfig->GetNote();
		UpdateData(FALSE);
	}
	else
	{
		::MessageBox(NULL, "No groups available to modify", m_ResourceManager.GetFullProductName(), MB_OK);
	}
}

void CConfigurationToolDlg::OnButtonAddGroup() 
{
	CAddGroup AddGroup(m_ServerConfig.GetInstallPath(), m_ResourceManager.GetFullProductName());

	if (AddGroup.DoModal() == IDOK)
	{
		CGroupConfig* pGroupConfig = m_ServerConfig.AddGroup(AddGroup.GetGroup());
		pGroupConfig->SetNote(AddGroup.GetGroupNote());

		CString cstrWndTitle = m_ResourceManager.GetFullProductName();
		cstrWndTitle += " Configuration Tool";
		CGroupPropertySheet GroupPropSheet(pGroupConfig, cstrWndTitle);
		if (GroupPropSheet.DoModal() != IDOK)
		{
			m_ServerConfig.DeleteGroup(AddGroup.GetGroup());
		}
		else
		{
			CString cstrGroupID;
			cstrGroupID.Format("%03d", pGroupConfig->GetGroupNb());
			int nIndex = m_ListGroups.AddString(cstrGroupID);
			m_ListGroups.SetCurSel(nIndex);

			// Update Note field
			m_cstrNote = pGroupConfig->GetNote();
			UpdateData(FALSE);
		}
	}
}

BOOL CConfigurationToolDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (m_bShutdown && (pMsg->wParam == VK_ESCAPE))
		{
			CancelShutDownSystem();
			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}
