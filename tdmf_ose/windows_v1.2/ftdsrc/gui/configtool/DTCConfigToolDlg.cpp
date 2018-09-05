// DTCConfigToolDlg.cpp : implementation file
//
// Date			Who			What
//
// 7-19-2001	DTurrin		Code for splash screen added
//

#include "stdafx.h"
#include "Splash.h"
#include "DTCConfigTool.h"
#include "DTCConfigToolDlg.h"
#include "AddGroup.h"
#include "DeleteGroup.h"
#include "Config.h"
#include "DTCConfigPropSheet.h"
#include "System.h"
#include "DTCDevices.h"
#include "Throttles.h"
#include "TunableParams.h"

#include "sockerrnum.h"
#include "ftdio.h"

#if defined(_OCTLIC)
#include "LicAPI.h"
#else
#include "license.h"
#include "ftd_lic.h"
#endif

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// DTurrin - Oct 16th, 2001
// Make sure that the Windows 2000 functions are used if they exist.
#if defined(_WINDOWS)
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

// ardev 020926 v
#define dsDefaultDir        "\\Program files\\Softek_Tdmf"    
#define dsDefaultPath       "C:" dsDefaultDir
#define dsDefaultPathJrn    dsDefaultPath "\\Journal"
#define dsDefaultPathPStore dsDefaultPath "\\PStore"
#define dsDefaultPStore     dsDefaultPathPStore "\\PStore"
// ardev 020926 ^
// rddev 020930 v
#define diBabSizeGranularity   32
// rddev 020930 ^

const int ID_HIDE_TIMER = 1;

CConfig Config;
extern CConfig	*lpConfig;
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CStatic	m_staticExpDate;
	CButton	m_buttonCheckLicenseKey;
	CEdit	m_editLicenseKey;
	CString	m_strLicenseKey;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
		BOOL	m_bLicenseKeyUpdate;

		void	readLicenseKey(char *szLicenseKey);
		void	writeLicenseKey(char *szLicenseKey);
		int		initLicenseKey(char	*szDate);
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckAboutLicenseKey();
	afx_msg void OnButtonAboutOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_strLicenseKey = _T("");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_STATIC_EXP_DATE, m_staticExpDate);
	DDX_Control(pDX, IDC_CHECK_ABOUT_LICENSE_KEY, m_buttonCheckLicenseKey);
	DDX_Control(pDX, IDC_EDI_LICENSE_KEY, m_editLicenseKey);
	DDX_Text(pDX, IDC_EDI_LICENSE_KEY, m_strLicenseKey);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_CHECK_ABOUT_LICENSE_KEY, OnCheckAboutLicenseKey)
	ON_BN_CLICKED(IDC_BUTTON_ABOUT_OK, OnButtonAboutOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTCConfigToolDlg dialog

CDTCConfigToolDlg::CDTCConfigToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDTCConfigToolDlg::IDD, pParent)
{
	m_strPath[0] = '\0';
	m_bAppStarted = FALSE;
#ifdef SFTK
	m_cWndAppStarted = FindWindow(NULL, "Softek TDMF Configuration Tool");
#else
	m_cWndAppStarted = FindWindow(NULL, PRODUCTNAME " Configuration Tool");
#endif
	if(m_cWndAppStarted)
		m_bAppStarted = TRUE;

	//{{AFX_DATA_INIT(CDTCConfigToolDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32

	// Use our icon
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_SFTK_LOGO);

#ifdef STK
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_STK_LOGO);
#endif

#ifdef MTI
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_MTI_LOGO);
#endif

#ifdef LG
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_LGTO_LOGO);
#endif
}

CDTCConfigToolDlg::~CDTCConfigToolDlg()
{
	
}

void CDTCConfigToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDTCConfigToolDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDTCConfigToolDlg, CDialog)
	//{{AFX_MSG_MAP(CDTCConfigToolDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ADD_GROUP, OnButtonAddGroup)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY_GROUP, OnButtonModifyGroup)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_GROUP, OnButtonDeleteGroup)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_LBN_SELCHANGE(IDC_LIST_GROUPS, OnSelchangeListGroups)
	ON_EN_CHANGE(IDC_EDIT_BAB_SIZE, OnChangeEditBabSize)
	ON_EN_UPDATE(IDC_EDIT_BAB_SIZE, OnUpdateEditBabSize)
	ON_CBN_EDITCHANGE(IDC_LIST_GROUPS, OnEditchangeListGroups)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTCConfigToolDlg message handlers

void CDTCConfigToolDlg::initBABSize()
{
	DWORD			iBabSize;
	int				nAccel = 1;
	int             maxBabSize = 0;
	UDACCEL			Accel;

	if (lpConfig->getRegBabSize(&iBabSize) == -1) {
		m_pSpinBABSize->EnableWindow(FALSE);
		m_pEditBABSize->EnableWindow(FALSE);
	}

	maxBabSize = getMaxBabSizeMb();
	Accel.nSec = 1;
	Accel.nInc = diBabSizeGranularity;
	m_pSpinBABSize->SetBuddy(m_pEditBABSize);
	m_pSpinBABSize->SetRange( 32, maxBabSize );
	m_pSpinBABSize->SetAccel( nAccel, &Accel );

	m_pSpinBABSize->SetPos(iBabSize);

	m_iBABSizeOrig = getBABSize();
}

void CDTCConfigToolDlg::initTCPWinSize()
{
	DWORD	iTCPWinSize;
	char	strTCPWinSize[_MAX_PATH];

	memset(strTCPWinSize, 0, sizeof(strTCPWinSize));

	// Get Default from the Reg
	lpConfig->getRegTCPWinSize(&iTCPWinSize);
	iTCPWinSize /= 1024; // convert to kb
	itoa(iTCPWinSize, strTCPWinSize, 10);

	m_pEditTCPWinSize->SetSel( 0, -1, FALSE );
	m_pEditTCPWinSize->ReplaceSel(strTCPWinSize, FALSE);
}

void CDTCConfigToolDlg::initPortNum()
{
	DWORD	iPort;
	char	strPort[_MAX_PATH];

	memset(strPort, 0, sizeof(strPort));

	// Get Default from the Reg
	lpConfig->getRegPort(&iPort);
	itoa(iPort, strPort, 10);

	m_pEditPort->SetSel( 0, -1, FALSE );
	m_pEditPort->ReplaceSel(strPort, FALSE);
}

void CDTCConfigToolDlg::initLogicalGroupList()
{
	char	strFileName[_MAX_PATH];
	FILE	*file;
	int		arrayGroups[1000];

	lpConfig->getConfigPath(m_strPath);

	memset(arrayGroups, 0, sizeof(arrayGroups));
	for(int i = 0; i <= 999; i++)
	{
		memset(strFileName, 0, sizeof(strFileName));
		sprintf(strFileName, "%sp%03d.cfg", m_strPath, i);
		file = fopen(strFileName, "r");
		if(file)
		{
			arrayGroups[i] = i;
			m_iGroupID = arrayGroups[i];
			addGroupToGroupList();
			fclose(file);
		}
	}
}

void CDTCConfigToolDlg::initDlgCtrls()
{
	m_pSpinBABSize = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_BAB_SIZE);
	m_pEditBABSize = (CEdit *)GetDlgItem(IDC_EDIT_BAB_SIZE);
	m_pEditPort = (CEdit *)GetDlgItem(IDC_EDIT_PORT_NUM);
	m_pEditNote = (CEdit *)GetDlgItem(IDC_EDIT_GROUP_NOTE);
	m_pListGroup = (CComboBox *)GetDlgItem(IDC_LIST_GROUPS);
	m_pEditTCPWinSize = (CEdit *)GetDlgItem(IDC_EDIT_TCP_WIN_SIZE);
}

int	CDTCConfigToolDlg::initLicenseKey(char	*szDate)
{
	int				iRc = 0;

#if defined(_OCTLIC)
	unsigned long	ulWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	ulVersion;
	struct tm		*gmt;

	iRc = validateLicense(&ulWeeks, &feature_flags_ret, &ulVersion);

	if(ulVersion != m_iOEM)
		iRc = -3;

	if(iRc > 0)
	{
		// Time left in license key
		gmt = gmtime( (time_t *)&iRc );
		sprintf(szDate, "%s", asctime( gmt ));
	}
	else if(iRc == 0)
	{
		sprintf(szDate, "%s", "Permanent License");
	}
	else if(iRc == -1)
	{
		sprintf(szDate, "%s", "Expired Demo License");
	}
	else if(iRc == -2)
	{
		sprintf(szDate, "%s", "Invalid Access Code");
	}
	else if(iRc == -3)
	{
		sprintf(szDate, "%s", "Invalid License Key");
	}

#else // no octlic

	char	szLic[_MAX_PATH];

	memset(szLic, 0, sizeof(szLic));

	getLicenseKeyFromReg(szLic);
	iRc = ftd_lic_verify(szLic);

	if(iRc == 1)
	{
		sprintf(szDate, "%s", "Valid License");
	}
	else if(iRc == -2)
	{
		sprintf(szDate, "%s", "License Key Is Empty");
	}
	else if(iRc == -3)
	{
		sprintf(szDate, "%s", "Bad Checksum");
	}
	else if(iRc == -4)
	{
		sprintf(szDate, "%s", "Expired License");
	}
	else if(iRc == -5)
	{
		sprintf(szDate, "%s", "Wrong Host");
	}
	else if(iRc == -7)
	{
		sprintf(szDate, "%s", "Wrong Machine Type");
	}
	else if(iRc == -8)
	{
		sprintf(szDate, "%s", "Bad Feature Mask");
	}


#endif // octlic

	return iRc;
}

BOOL CDTCConfigToolDlg::OnInitDialog()
{
	CSplashWnd::EnableSplashScreen(TRUE); //Splash screen added on 7-20-2001
	CSplashWnd::ShowSplashScreen(this);   //by DTurrin@softek.fujitsu.com

	CDialog::OnInitDialog();

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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
#if defined(_OCTLIC)

#ifdef STK 
	m_iOEM = LICENSE_VERSION_STK_50;
#elif MTI
	m_iOEM = LICENSE_VERSION_MDS_50;
#else
	m_iOEM = LICENSE_VERSION_REPLICATION_43;
#endif

#endif // octlic

	initDlgCtrls();
	initBABSize();
	initTCPWinSize();
	initPortNum();
	initLogicalGroupList();
	bDeletedGroup = TRUE;
	OnSelchangeListGroups();

	m_bPropSheetInit = FALSE;

	// Get license key info
	memset(m_szDate, 0, sizeof(m_szDate));
	int iRc = initLicenseKey(m_szDate);
	if(iRc < 0)
	{
		strcat(m_szDate, ": Goto the about box to update your key.");
		::MessageBox(NULL, m_szDate, PRODUCTNAME, MB_OK);
	}

#ifdef SFTK
	SetWindowText( "Softek TDMF Configuration Tool" );
#else
	SetWindowText( PRODUCTNAME " Configuration Tool" );
#endif

	if(m_bAppStarted)
		OnCancel();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDTCConfigToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
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

void CDTCConfigToolDlg::OnPaint() 
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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDTCConfigToolDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDTCConfigToolDlg::initPropSheet()
{
	CSystem			system;
	CDTCDevices		devices;
	CThrottles		throttles;
	CTunableParams	tunableParams;
	HANDLE			handle;
	CString			strDtcPath = "\\\\.\\" QNM "\\lg";
	char			strDTCGroup[_MAX_PATH];

	m_bPropSheetInit = TRUE;

	CDTCConfigPropSheet sheetConfig(IDS_STRING_DIALOG_CAPTION, NULL, 0);	
	sheetConfig.AddPage(&system);
	sheetConfig.AddPage(&devices);
//	sheetConfig.AddPage(&throttles);
	sheetConfig.AddPage(&tunableParams);

	sheetConfig.DoModal();

	// Get the button state of the config tool (Tab Dlg)
	// BUTTONOK, BUTTONCANCEL, BUTTONHELP
	m_iButtonState = sheetConfig.m_iButtonState;
	if(m_iButtonState == BUTTONOK)
	{
		if ( !lpConfig->IsValChanged() ) // ardeb 020913
		{
			return;
		}
		else if ( !lpConfig->Is3ValValid() ) // ardeb 020913
		{
			AfxMessageBox ( "DTC Device Values corrupted\n" );
			return;
		}


		m_iBABSize = getBABSize();
		m_iTCPWinSize = getTCPWinSize();
		m_iPortNum = getPortNum();
		
		writeInfoToConfig();

		lpConfig->writeConfigFile();
		lpConfig->setTunablesInPStore();//ac

		memset(strDTCGroup, 0, sizeof(strDTCGroup));
		itoa(m_iGroupID, strDTCGroup, 10);
		strDtcPath = strDtcPath + strDTCGroup;

		if ( !CtDlgCheckDir() )
		{
			return;
		}

		SetLastError(0);

		handle = CreateFile(strDtcPath, GENERIC_READ, FILE_SHARE_READ, 
					NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if(handle == INVALID_HANDLE_VALUE)
		{
			DWORD dwRc = GetLastError();

			if(dwRc != (FTD_DRIVER_ERROR_CODE | EBUSY))
			{
				if(IDYES == ::MessageBox(NULL, "Would you like to start the logical group ?", 
									PRODUCTNAME, MB_YESNO|MB_ICONQUESTION))
				{
					SetCursor(LoadCursor(NULL, IDC_WAIT));

					lpConfig->setStartGroup();
					//ac lpConfig->setTunablesInPStore();
					
					SetCursor(LoadCursor(NULL, IDC_ARROW));
				}
				else
				{	//ac
					//::MessageBox(NULL, "Tunable Parameters will not be saved if the group is not started!", 
					//				PRODUCTNAME, MB_OK|MB_ICONEXCLAMATION);
				}
			}
		}
		else
		{
			CloseHandle(handle);

			SetCursor(LoadCursor(NULL, IDC_WAIT));
			//ac lpConfig->setTunablesInPStore();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
	}	
}

int CDTCConfigToolDlg::getBABSize()
{
	char	strBABSize[10];
	CString	strBabTemp;
	int		iBab = 0;
    CString strBabMsg;

	memset(strBABSize, 0, sizeof(strBABSize));
	m_pEditBABSize->GetLine( 1, strBABSize, sizeof(strBABSize));

	strBabTemp = strBABSize;
	strBabTemp.Remove( ',' );

	iBab = atoi((LPCTSTR) strBabTemp);
	iBab = (iBab / diBabSizeGranularity) * diBabSizeGranularity;

	if (!babSizeOk(iBab))
		{
		strBabMsg.Format("The BAB size is outside limits:\nMIN: 32 MB required \nMAX: %d MB.\n", getMaxBabSizeMb());
		AfxMessageBox(strBabMsg);

		if(iBab < 32)
			iBab = 32;

		if(iBab > getMaxBabSizeMb())
			iBab = getMaxBabSizeMb();
		}

//	return(atoi((LPCTSTR) strBabTemp));
	return(iBab);
}

int CDTCConfigToolDlg::getTCPWinSize()
{
	char	strTCPWinSize[10];

	memset(strTCPWinSize, 0, sizeof(strTCPWinSize));
	m_pEditTCPWinSize->GetLine( 1, strTCPWinSize, sizeof(strTCPWinSize));

	return(atoi(strTCPWinSize) * 1024); // to bytes for registry
}

int CDTCConfigToolDlg::getPortNum()
{
	char	strPortNum[10];
	
	memset(strPortNum, 0, sizeof(strPortNum));
	m_pEditPort->GetLine( 1, strPortNum, sizeof(strPortNum));

	return(atoi(strPortNum));
}

void CDTCConfigToolDlg::getGroupNote(char strGroupNote[_MAX_PATH])
{
	memset(strGroupNote, 0, _MAX_PATH);
	int iLength = m_pEditNote->LineLength(-1);
	m_pEditNote->GetLine(1, strGroupNote, iLength);
}

void CDTCConfigToolDlg::getCfgFileInUse(char strFile[_MAX_PATH])
{
	memset(strFile, 0, sizeof(strFile));
	sprintf(strFile, "%sp%03d.cfg", m_strPath, m_iGroupID);
}

void CDTCConfigToolDlg::writeInfoToConfig()
{
	m_iBABSize = getBABSize();
	m_iTCPWinSize = getTCPWinSize();
	m_iPortNum = getPortNum();
	getGroupNote(m_strGroupNote);
	getCfgFileInUse(m_strCfgFileInUse);

	lpConfig->setDevCfg(m_iBABSize, m_iTCPWinSize, m_iPortNum,
				m_iGroupID, m_strGroupNote,
				m_strPath, m_strCfgFileInUse);
}


void CDTCConfigToolDlg::shutDownSystem()
{
	HANDLE hToken;              // handle to process token 
	TOKEN_PRIVILEGES tkp;       // pointer to token structure 
 
	BOOL fResult;               // system shutdown flag 
 
	// Get the current process token handle so we can get shutdown 
	// privilege. 
 
	OpenProcessToken(GetCurrentProcess(), 
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
 
	// Get the LUID for shutdown privilege. 
 
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
 
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
	// Get shutdown privilege for this process. 
 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES) NULL, 0); 
 
	// Display the shutdown dialog box and start the time-out countdown. 
	fResult = InitiateSystemShutdown( 
		NULL,                                  // shut down local computer 
		"Click on the main window and press \
		 the Escape key to cancel shutdown.",  // message to user 
		20,                                    // time-out period 
		FALSE,                                 // ask user to close apps 
		TRUE);                                 // reboot after shutdown 
 
	// Disable shutdown privilege. 
 
	tkp.Privileges[0].Attributes = 0; 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
			(PTOKEN_PRIVILEGES) NULL, 0); 
 
}

void CDTCConfigToolDlg::OnOK() 
{
	m_iBABSize = getBABSize();
	m_iTCPWinSize = getTCPWinSize();
	m_iPortNum = getPortNum();

	// DTurrin - Sept 10th, 2001
	// If m_iTCPWinSize is lesser or equal to 0, send a warning.
	if (m_iTCPWinSize <= 0)
	{
		AfxMessageBox("The TCP Window Size must be greater than 0");
		return;
	}
	// If m_iPortNum is lesser or equal to 0, send a warning.
	if (m_iPortNum <= 0)
	{
		AfxMessageBox("The Primary Port Number must be greater than 0");
		return;
	}

	if(bDeletedGroup)
	{   
		writeInfoToConfig(); 	
		if(m_bPropSheetInit)
		{
			lpConfig->writeConfigFile(); // Save parameters in REGISTRY
		}
	}
	bDeletedGroup = TRUE;

	// Verify BAB Size value before saving it in REGISTRY
	if(m_iBABSizeOrig != m_iBABSize)
	{
		if(IDOK == ::MessageBox(NULL, "Restart The Computer Now?", 
								PRODUCTNAME, MB_OKCANCEL))
		{
			shutDownSystem();
		}
    }

	CDialog::OnOK();
}

void CDTCConfigToolDlg::displayNote()
{
	m_pEditNote->SetSel(0, -1, FALSE);
	m_pEditNote->ReplaceSel(m_strGroupNote, FALSE);
}

void CDTCConfigToolDlg::addGroupToGroupList()
{
	char		strGroupID[100];

	memset(strGroupID, 0, sizeof(strGroupID));
	sprintf(strGroupID, "%03d", m_iGroupID); 
	m_pListGroup->AddString( strGroupID );
	m_pListGroup->SelectString( -1, strGroupID );
}

void CDTCConfigToolDlg::deleteGroupFromList()
{
	m_pListGroup->DeleteString( m_pListGroup->GetCurSel( ));
	
	// delete note
	m_pEditNote->SetSel(0, -1, FALSE);
	m_pEditNote->ReplaceSel(" ", FALSE);
}


// ardev 020926
// Make sure the default dir is created if used.
BOOL CDTCConfigToolDlg::CtDlgCheckDir ()
{
	char lcaPth [ _MAX_PATH  ];
	char lcaDrv [ _MAX_DRIVE ];
	char lcaDir [ _MAX_DIR   ];
	char lcaFnm [ _MAX_FNAME ];
	char lcaExt [ _MAX_EXT   ];
	_splitpath( 
		lpConfig->m_structSystemValues.m_strPStoreDev, 
		lcaDrv, lcaDir, NULL, NULL 
	);

	_snprintf ( lcaPth, _MAX_PATH, "%s%s", lcaDrv, lcaDir );

	if ( _access ( lcaPth, 0 ) == -1 )
	{
		CString lszMsg;
		lszMsg.Format ( 
			"Would you like to create the directory: %s ?", lcaPth 
		);
		if ( IDYES != ::MessageBox ( 
							NULL, lszMsg, PRODUCTNAME, 
							MB_YESNO | MB_ICONQUESTION
						)
		   )
		{
			return FALSE;
		}
		else if ( !CtCreateDir( lcaPth ) )
		{
			return FALSE;
		}
	}

	for ( 
		int liC = strlen ( lpConfig->m_structSystemValues.m_strJournalDir );
		liC > 3;
		liC--
		)
	{
		if (  lpConfig->m_structSystemValues.m_strJournalDir[liC] == '\\' )
		{
			lpConfig->m_structSystemValues.m_strJournalDir[liC] = '\0';
		}
		else
		{
			break;
		}
	}

/* ardev 021212 v
// The following option was remove because it would create confusion
// when using with a non-loopback setting.

	_splitpath( 
		lpConfig->m_structSystemValues.m_strJournalDir, 
		lcaDrv, lcaDir, lcaFnm, lcaExt 
	);

	_snprintf ( lcaPth, _MAX_PATH, "%s%s%s%s", lcaDrv, lcaDir, lcaFnm, lcaExt );

	if ( _access ( lcaPth, 0 ) == -1 )
	{
		CString lszMsg;
		lszMsg.Format ( 
			"Would you like to create the directory: %s ?", lcaPth 
		);
		if ( IDYES != ::MessageBox ( 
							NULL, lszMsg, PRODUCTNAME, 
							MB_YESNO | MB_ICONQUESTION
						)
		   )
		{
			return FALSE;
		}
		else if ( !CtCreateDir( lcaPth ) )
		{
			return FALSE;
		}
	}
*/
	return TRUE;

} // CDTCConfigToolDlg::CtDlgCheckDir ()


// ardev 020926
// Complete directory creation.
// Has to include the drive ex.: F:\\Alpha\\Bravo\\Charly\\Delta\\Echo
// pcpPth has to have _MAX_PATH space
BOOL CDTCConfigToolDlg::CtCreateDir ( char* pcpPth )
{
	int liLen = strlen ( pcpPth );
	pcpPth [ liLen++ ] = '\\';
	pcpPth [ liLen++ ] = '\0';

	for ( int liC = 3; liC < liLen; liC++ )
	{
		if ( pcpPth[liC] == '\\' )
		{
			pcpPth[liC] = '\0';
			if ( _access ( pcpPth, 0 ) == -1 )
			{
				if ( _mkdir ( pcpPth ) == -1 )
				{
					pcpPth[liC] = '\\';
					return FALSE;
				}
			}
			pcpPth[liC] = '\\';
		}
	}

	return TRUE;

} // CDTCConfigToolDlg::CtCreateDir ()


void CDTCConfigToolDlg::createNewCfgFile()
{
	char	strFileName[_MAX_PATH];
	FILE	*file;

	memset(strFileName, 0, sizeof(strFileName));
	sprintf(strFileName, "%sp%03d.cfg", m_strPath, m_iGroupID);
	file = fopen(strFileName, "w+");
	if(file)
		fclose(file);

	memset(m_strCfgFileInUse, 0, sizeof(m_strCfgFileInUse));
	strcpy(m_strCfgFileInUse, strFileName);
}

void CDTCConfigToolDlg::OnButtonAddGroup() 
{
	CAddGroup	AddGroup;
	char		strPort[100];

	AddGroup.DoModal();

	if(!AddGroup.m_bAddGroupCancel)
	{
		memset(lpConfig->m_szDeviceListInfo, 0, sizeof(lpConfig->m_szDeviceListInfo));
		memset(lpConfig->m_iMirrorIndexArray, 0, sizeof(lpConfig->m_iMirrorIndexArray));
		lpConfig->m_iMirrorDevIndex = 0;

		lpConfig->m_bAddFileFlag = FALSE;
	
		m_iGroupID = AddGroup.m_iGroup;
		memset(m_strGroupNote, 0, _MAX_PATH);
		strcpy(m_strGroupNote, AddGroup.m_strGroupNote);

		// set secondary port to primary port # by default
		memset(strPort, 0, sizeof(strPort));
		GetDlgItemText(IDC_EDIT_PORT_NUM, strPort, sizeof(strPort));
		lpConfig->m_structSystemValues.m_iPortNum = atoi(strPort);
		// ardeb 020918 v Give default value
		sprintf 
		( 
			lpConfig->m_structSystemValues.m_strPStoreDev, 
			"%.*s%-d.Dat", _MAX_PATH-8, dsDefaultPStore, m_iGroupID
		);
		sprintf 
		( 
			lpConfig->m_structSystemValues.m_strJournalDir, 
			"%.*s", _MAX_PATH-1, dsDefaultPathJrn
		);
		// ardeb 020918 ^ Give default value

		// Fill Group list box & note
		addGroupToGroupList();
		displayNote();

		createNewCfgFile();

		initPropSheet();

		OnSelchangeListGroups();
	}
}

void CDTCConfigToolDlg::OnButtonModifyGroup() 
{
	int iRc;
	int	iNumGroups = 0;

	memset(lpConfig->m_szDeviceListInfo, 0, sizeof(lpConfig->m_szDeviceListInfo));
	memset(lpConfig->m_iMirrorIndexArray, 0, sizeof(lpConfig->m_iMirrorIndexArray));
	lpConfig->m_iMirrorDevIndex = 0;

	// must set this up to check if they 
	// selected a group
	iNumGroups = m_pListGroup->GetCount( );
	if(iNumGroups)
	{
		lpConfig->m_bAddFileFlag = TRUE;

		memset(m_strCfgFileInUse, 0, sizeof(m_strCfgFileInUse));
		sprintf(m_strCfgFileInUse, "%sp%03d.cfg", m_strPath, m_iGroupID);

		writeInfoToConfig();

		iRc = lpConfig->readConfigFile();
		if(iRc)
		{
			lpConfig->getTunablesFromPStore();

			initPropSheet();

			OnSelchangeListGroups();
		}
	}
	else
	{
		::MessageBox(NULL, "No groups available to modify", PRODUCTNAME, MB_OK);
	}
}

void CDTCConfigToolDlg::OnButtonDeleteGroup() 
{
	CDeleteGroup DeleteGroup;
	char		strGroup[3];
	char		strFileName[_MAX_PATH];
	int			iNumGroups = 0;

	// must set this up to check if they 
	// selected a group
	iNumGroups = m_pListGroup->GetCount( );
	if(iNumGroups)
	{
		m_pListGroup->GetLBText(m_pListGroup->GetCurSel(), strGroup);

		DeleteGroup.DoModal();

		if(DeleteGroup.m_bDeleteGroup)
		{
			// Need to re-initiate Config info in case the group being
			// deleted is not the first one in the list.
			writeInfoToConfig();

			// killpmd and stop group
			lpConfig->stopGroup();

			// DTurrin - Sept 10th, 2001
			// If the group has previously been started, it will
			// have an associated pxxx.cur file which also needs
			// to be deleted.
			sprintf(strFileName, "%sp%s.cur", m_strPath, strGroup);
#if defined(_WINDOWS)
			if (_access(strFileName,0) == 0) DeleteFile(strFileName);
#else
			if (access(strFileName,F_OK) == 0) DeleteFile(strFileName);
#endif
			
			sprintf(strFileName, "%sp%s.cfg", m_strPath, strGroup); 
			DeleteFile(strFileName);
			
			deleteGroupFromList();

			bDeletedGroup = FALSE;

			m_pListGroup->SetCurSel(0);
			m_pListGroup->GetLBText( 0, strGroup );
			m_iGroupID = atoi(strGroup);
		}
	}
	else
	{
		::MessageBox(NULL, "No groups available to delete", PRODUCTNAME, MB_OK);
	}
}

void CDTCConfigToolDlg::OnCancel() 
{	
	char	strFileName[_MAX_PATH];
	CFile	file;
	CFileException e;
	DWORD	dwLength = 0;
	int		iRc = 0;


	if(!m_bAppStarted)
	{
		memset(strFileName, 0, sizeof(strFileName));
		sprintf(strFileName, "%sp%03d.cfg", m_strPath, m_iGroupID);
		iRc = file.Open(strFileName, CFile::modeWrite, &e);
		if(iRc > 0)
		{
			dwLength = file.GetLength();
			file.Close();
			if(dwLength == 0)
				CFile::Remove(strFileName);
		}
	}
	else
	{
		m_cWndAppStarted->SetForegroundWindow( );
	}

	CDialog::OnCancel();
}

void CDTCConfigToolDlg::OnDestroy() 
{
	CDialog::OnDestroy();	
}

void CDTCConfigToolDlg::OnClose() 
{
	CDialog::OnClose();
}

BOOL CDTCConfigToolDlg::DestroyWindow() 
{
	return CDialog::DestroyWindow();
}

void CDTCConfigToolDlg::OnSelchangeListGroups() 
{
	int		iSel;
	char	strGroup[3];
	char	strNote[_MAX_PATH];

	memset(strNote, 0, sizeof(strNote));
	memset(strGroup, 0, sizeof(strGroup));

	// change the file and group stuff
	iSel = m_pListGroup->GetCurSel( );
	if(iSel != LB_ERR)
	{
		m_pListGroup->GetLBText( iSel, strGroup );
		m_iGroupID = atoi(strGroup); 

		lpConfig->readNote(m_strPath, strGroup, strNote);
	}
	// Update note edit ctrl with strNote
	SetDlgItemText( IDC_EDIT_GROUP_NOTE, strNote );
	
}

void CDTCConfigToolDlg::OnChangeEditBabSize() 
{
	
	
	
}

void CDTCConfigToolDlg::OnUpdateEditBabSize() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}

void CDTCConfigToolDlg::OnEditchangeListGroups() 
{
	// TODO: Add your control notification handler code here
	
}

// This function is used to read the old UNIX style license key
// We are now using the Oct License keys.
void CAboutDlg::readLicenseKey(char *szLicenseKey)
{
	CFile	file;
	BOOL	bFileOpen;
	int		iFileSize = 0;
	char	szFileName[_MAX_PATH];
	char	szPath[_MAX_PATH];

	memset(szFileName, 0, sizeof(szFileName));
	lpConfig->getConfigPath(szPath);
	sprintf(szFileName,"%s%s.lic", szPath, QNM);

	bFileOpen = file.Open(szFileName, CFile::modeRead, NULL);

	if(bFileOpen)
	{
		iFileSize = file.GetLength();
		file.Read(szLicenseKey, iFileSize);
		file.Close();
	}
}

// This function is used to write the old UNIX style license key
// We are now using the Oct License keys.
void CAboutDlg::writeLicenseKey(char *szLicenseKey)
{
#if defined(_OCTLIC)
	CFile	file;
	BOOL	bFileOpen;
	char	szFileName[_MAX_PATH];
	char	szPath[_MAX_PATH];

	memset(szFileName, 0, sizeof(szFileName));
	lpConfig->getConfigPath(szPath);
	sprintf(szFileName,"%s%s.lic", szPath, QNM);

	bFileOpen = file.Open(szFileName, CFile::modeCreate|CFile::modeReadWrite, NULL);

	if(bFileOpen)
	{
		file.Write(szLicenseKey, sizeof(szLicenseKey));
		file.Close();
	}
#else // no octlic
	writeLicenseKeyToReg(szLicenseKey);
#endif // octlic
}

int	CAboutDlg::initLicenseKey(char	*szDate)
{
	int				iRc = 0;
	

#if defined(_OCTLIC)

	unsigned long	ulWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	ulVersion;
	struct tm		*gmt;

	iRc = validateLicense(&ulWeeks, &feature_flags_ret, &ulVersion);

	if(iRc > 0)
	{
		// Time left in license key
		gmt = gmtime( (time_t *)&iRc );
		sprintf(szDate, "%s", asctime( gmt ));
	}
	else if(iRc == 0)
	{
		sprintf(szDate, "%s", "Permanent License");
	}
	else if(iRc == -1)
	{
		sprintf(szDate, "%s", "Expired Demo License");
	}
	else if(iRc == -2)
	{
		sprintf(szDate, "%s", "Invalid Access Code");
	}
	else if(iRc == -3)
	{
		sprintf(szDate, "%s", "Invalid License Key");
	}

#else // no octlic

	char	szLic[_MAX_PATH];

	memset(szLic, 0, sizeof(szLic));

	getLicenseKeyFromReg(szLic);
	iRc = ftd_lic_verify(szLic);

	if(iRc == 1)
	{
		sprintf(szDate, "%s", "Valid License");
	}
	else if(iRc == -2)
	{
		sprintf(szDate, "%s", "License Key Is Empty");
	}
	else if(iRc == -3)
	{
		sprintf(szDate, "%s", "Bad Checksum");
	}
	else if(iRc == -4)
	{
		sprintf(szDate, "%s", "Expired License");
	}
	else if(iRc == -5)
	{
		sprintf(szDate, "%s", "Wrong Host");
	}
	else if(iRc == -7)
	{
		sprintf(szDate, "%s", "Wrong Machine Type");
	}
	else if(iRc == -8)
	{
		sprintf(szDate, "%s", "Bad Feature Mask");
	}


#endif // octlic

	return iRc;
}

BOOL CAboutDlg::OnInitDialog() 
{
	char	szLicenseKey[_MAX_PATH];
	char	szDate[100];

	CDialog::OnInitDialog();

	m_bLicenseKeyUpdate = FALSE;

	// init the license key
	memset(szLicenseKey, 0, sizeof(szLicenseKey));
	memset(szDate, 0, sizeof(szDate));

#if !defined(_OCTLIC)
	// UNIX style lic key
//	readLicenseKey(szLicenseKey);
//	m_strLicenseKey.Format("%s", szLicenseKey);
	getLicenseKeyFromReg(szLicenseKey);
#else
	// oct Lic key
	getLicenseKeyFromReg(szLicenseKey);
#endif
	m_editLicenseKey.SetSel(0, -1, FALSE);
	m_editLicenseKey.ReplaceSel(szLicenseKey, FALSE);

	initLicenseKey(szDate);
	SetDlgItemText( IDC_STATIC_EXP_DATE, szDate );

#ifndef SFTK
	// If this is not the Softek TDMF version
	// of the product, use the appropriate
	// Window text. (DTurrin - July 24th, 2001)
	SetDlgItemText( IDC_STATIC_ABOUT_VERSION, PRODUCTNAME" "VERSION" "BLDSEQNUM );
#endif

	CStatic	*pIcon;

	pIcon = (CStatic *)GetDlgItem(IDC_STATIC_SFTK);

#ifdef STK
	pIcon = (CStatic *)GetDlgItem(IDC_STATIC_STK);
#endif

#ifdef MTI
	pIcon = (CStatic *)GetDlgItem(IDC_STATIC_MTI);
#endif

#ifdef LG
	pIcon = (CStatic *)GetDlgItem(IDC_STATIC_LGTO);
#endif

	pIcon->ShowWindow(SW_SHOWNORMAL);
	
#ifndef SFTK
	// If this is not the Softek TDMF version
	// of the product, use the appropriate
	// Window text. (DTurrin - July 24th, 2001)

	SetWindowText( "About " PRODUCTNAME " Configuration Tool" );

	SetDlgItemText(IDC_STATIC_COPYRIGHT, "Copyright (C) 2001 " OEMNAME);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnCheckAboutLicenseKey() 
{
	UINT		iCheckState;

	iCheckState = m_buttonCheckLicenseKey.GetState();
	if(iCheckState & 0x0003)
	{
		m_bLicenseKeyUpdate = TRUE;
		m_editLicenseKey.SetReadOnly(FALSE);
	}
	else
	{
		m_bLicenseKeyUpdate = FALSE;
		m_editLicenseKey.SetReadOnly(TRUE);
	}
}


void CAboutDlg::OnButtonAboutOk() 
{
	int		iRc = 0;
	char	szLicenseKey[_MAX_PATH];

	memset(szLicenseKey, 0, sizeof(szLicenseKey));

	if(m_bLicenseKeyUpdate)
	{
		m_editLicenseKey.GetLine(0, szLicenseKey, sizeof(szLicenseKey));
		m_strLicenseKey.Format("%s", szLicenseKey);

#if !defined(_OCTLIC)
		// UNIX STYLE lic key
		writeLicenseKey(szLicenseKey);
#else
		// Oct lic key
		writeLicenseKeyToReg(szLicenseKey);
#endif
	}

	// Set the license key where it has to be set (either file or reg)

	EndDialog(iRc);	
}


// DTurrin - Oct 16th, 2001
//
// This procedure determines if the BAB size entered by
// the user is too big. If the entered BAB size is greater
// than 60% of the Paged Pool memory, than the procedure
// returns FALSE. Esle it returns TRUE.
//
// NOTE : This function can cause problems if the amount of
//        physical memory on a Windows NT system version
//        is equal to or greater than 4 GB.
BOOL CDTCConfigToolDlg::babSizeOk(int babSizeMb)
{
#if defined(_WINDOWS)

   int maxBabSizeMb;

   maxBabSizeMb = getMaxBabSizeMb();

   if ((babSizeMb >= 32) && (babSizeMb <= maxBabSizeMb)) return TRUE;

   return FALSE;

#endif // #if defined(_WINDOWS)

   return TRUE;
}

int CDTCConfigToolDlg::getMaxBabSizeMb()
{
#if defined(_WINDOWS)

   int maxPhysMemMb = 0;
   int babSizeMb = 0; 
#if !defined NTFOUR // WIN2K PagedPoolSize
   int maxPagedPoolSize = 224;
#else // NT 4.0 MAX PagedPoolSize
   int maxPagedPoolSize = 160;
#endif

#if !defined MEMORYSTATUSEX

   // Windows version is before Windows 2000
   MEMORYSTATUS stat;
   GlobalMemoryStatus (&stat);
   maxPhysMemMb = (int)(((stat.dwTotalPhys / (1024 * 1024)) * 6) / 10);

#else // #if !defined MEMORYSTATUSEX

   // If the OS version is Windows 2000 or greater,
   // GlobalMemoryStatusEx must be used instead of
   // GlobalMemoryStatus in case the memory size
   // exceeds 4 GBs. 
   MEMORYSTATUSEX statex;
   statex.dwLength = sizeof(statex);
   GlobalMemoryStatusEx (&statex);
   maxPhysMemMb = (int)(((statex.ullTotalPhys / (1024 * 1024)) * 6) / 10);

#endif // #if !defined MEMORYSTATUSEX

   babSizeMb = (maxPhysMemMb / diBabSizeGranularity) * diBabSizeGranularity; //Evaluate in 32MB Granularity

   if (babSizeMb > maxPagedPoolSize) 
	   babSizeMb = maxPagedPoolSize;

#endif // #if defined(_WINDOWS)
   return babSizeMb;
}