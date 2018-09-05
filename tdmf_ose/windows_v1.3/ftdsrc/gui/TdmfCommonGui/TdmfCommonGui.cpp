// TdmfCommonGui.cpp : Defines the class behaviors for the application.
//


// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "TdmfCommonGui.h"
#include "MainFrm.h"
#include "TdmfCommonGuiDoc.h"
#include "TdmfDocTemplate.h"
#include "Splash.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// 

static BOOL bClassRegistered = FALSE;

bool g_bStaticLogo = true;
bool g_bStatusbar = true;
bool g_bRedBar = false;


/////////////////////////////////////////////////////////////////////////////
// 

class CTdmfCommandLineInfo : public CCommandLineInfo
{
	virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
	{
		if (stricmp(lpszParam, "NoSplash") == 0)
		{
			m_bShowSplash = FALSE;
		}
	}
};


/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiApp

BEGIN_MESSAGE_MAP(CTdmfCommonGuiApp, CWinApp)
	//{{AFX_MSG_MAP(CTdmfCommonGuiApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
	ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
	ON_COMMAND(ID_HELP_SEARCH, OnHelpSearch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiApp construction

// Mike Pollett
#include "../../tdmf.inc"
CTdmfCommonGuiApp::CTdmfCommonGuiApp() : m_cstrHelpFile("TDMF1_3.chm")
{
	DWORD dwType;
	char  szValue[MAX_PATH];
	ULONG nSize = MAX_PATH;

	// Read Help file path from registry
	nSize = MAX_PATH;
	#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
	if (SHGetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "DtcHelpFile", &dwType, szValue, &nSize) == ERROR_SUCCESS)
	{
		m_cstrHelpFile = szValue;
	}
}

CTdmfCommonGuiApp::~CTdmfCommonGuiApp()
{
#ifdef  TDMF_IN_A_DLL
	if (CDocManager::pStaticList != NULL)
	{
		delete CDocManager::pStaticList;
		CDocManager::pStaticList = NULL;
	}
	if (CDocManager::pStaticDocManager != NULL)
	{
		delete CDocManager::pStaticDocManager;
		CDocManager::pStaticDocManager = NULL;
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTdmfCommonGuiApp object

CTdmfCommonGuiApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiApp initialization

BOOL CTdmfCommonGuiApp::InitInstance()
{
	// Set resource dll name
	char szFileName[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	GetModuleFileName(NULL, szFileName, _MAX_PATH);
	_splitpath(szFileName, drive, dir, fname, ext);
	_makepath(szFileName, drive, dir, "RBRes", "dll");
	m_ResourceManager.SetResourceDllName(szFileName);

	// Set application's name
	CString cstrAppName = m_ResourceManager.GetProductName();
	cstrAppName += " Common Console";
	free((void*)m_pszAppName);
	m_pszAppName = _tcsdup(cstrAppName);

	// Register your unique class name that you wish to use
	WNDCLASS wndcls;
	
	memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL
	// defaults
	
	wndcls.style = CS_DBLCLKS;
	wndcls.lpfnWndProc = ::DefWindowProc;
	wndcls.hInstance = AfxGetInstanceHandle();
	wndcls.hIcon = m_ResourceManager.GetApplicationIcon();
	// icon
	wndcls.hCursor = LoadCursor( IDC_ARROW );
	wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wndcls.lpszMenuName = NULL;
	
	// Specify your own class name for using FindWindow later
	wndcls.lpszClassName = _T("TdmfClass");
	
	// Register the new class and exit if it fails
	if(!AfxRegisterClass(&wndcls))
	{
		TRACE("Class Registration Failed\n");
		return FALSE;
	}

	bClassRegistered = TRUE;

	CoInitialize(NULL);

	AfxInitRichEdit();

	AfxEnableControlContainer();

#if (_MSC_VER < 1300)
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifndef TDMF_IN_A_DLL
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

#ifdef TDMF_IN_A_DLL

	CTdmfDocTemplate* pDocTemplate;
	pDocTemplate = new CTdmfDocTemplate(IDR_MAINFRAME,
										RUNTIME_CLASS(CTdmfCommonGuiDoc),
										RUNTIME_CLASS(CMainFrame),  // main SDI frame window
										NULL);
	AddDocTemplate(pDocTemplate);

	return CWinApp::InitInstance();

#else

	CSingleDocTemplate* pDocTemplate;

	pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME,
										  RUNTIME_CLASS(CTdmfCommonGuiDoc),
										  RUNTIME_CLASS(CMainFrame),  // main SDI frame window
										  NULL);

	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CTdmfCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	CString cstrTitle = m_ResourceManager.GetFullProductName();
	cstrTitle += " - Common Console";
	m_pMainWnd->SetWindowText(cstrTitle);

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();


   	return TRUE;

#endif
}


/////////////////////////////////////////////////////////////////////////////
//  Function that draws the Softek logo with a transparent background
//  in the About box.
/////////////////////////////////////////////////////////////////////////////
void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart,
                           short yStart, COLORREF cTransparentColor)
{
	BITMAP     bm;
	COLORREF   cColor;
	HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
	HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
	HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
	POINT      ptSize;

	hdcTemp = CreateCompatibleDC(hdc);
	SelectObject(hdcTemp, hBitmap);   // Select the bitmap

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	ptSize.x = bm.bmWidth;            // Get width of bitmap
	ptSize.y = bm.bmHeight;           // Get height of bitmap
	DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device

									 // to logical points

	// Create some DCs to hold temporary data.
	hdcBack   = CreateCompatibleDC(hdc);
	hdcObject = CreateCompatibleDC(hdc);
	hdcMem    = CreateCompatibleDC(hdc);
	hdcSave   = CreateCompatibleDC(hdc);

	// Create a bitmap for each DC. DCs are required for a number of
	// GDI functions.

	// Monochrome DC
	bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

	// Monochrome DC
	bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

	bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
	bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

	// Each DC must select a bitmap object to store pixel data.
	bmBackOld   = (HBITMAP)SelectObject(hdcBack, bmAndBack);
	bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
	bmMemOld    = (HBITMAP)SelectObject(hdcMem, bmAndMem);
	bmSaveOld   = (HBITMAP)SelectObject(hdcSave, bmSave);

	// Set proper mapping mode.
	SetMapMode(hdcTemp, GetMapMode(hdc));

	// Save the bitmap sent here, because it will be overwritten.
	BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

	// Set the background color of the source DC to the color.
	// contained in the parts of the bitmap that should be transparent
	cColor = SetBkColor(hdcTemp, cTransparentColor);

	// Create the object mask for the bitmap by performing a BitBlt
	// from the source bitmap to a monochrome bitmap.
	BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0,
		  SRCCOPY);

	// Set the background color of the source DC back to the original
	// color.
	SetBkColor(hdcTemp, cColor);

	// Create the inverse of the object mask.
	BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0,
		  NOTSRCCOPY);

	// Copy the background of the main DC to the destination.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,
		  SRCCOPY);

	// Mask out the places where the bitmap will be placed.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

	// Mask out the transparent colored pixels on the bitmap.
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

	// XOR the bitmap with the background on the destination DC.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

	// Copy the destination to the screen.
	BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0,
		  SRCCOPY);

	// Place the original bitmap back into the bitmap sent here.
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

	// Delete the memory bitmaps.
	DeleteObject(SelectObject(hdcBack, bmBackOld));
	DeleteObject(SelectObject(hdcObject, bmObjectOld));
	DeleteObject(SelectObject(hdcMem, bmMemOld));
	DeleteObject(SelectObject(hdcSave, bmSaveOld));

	// Delete the memory DCs.
	DeleteDC(hdcMem);
	DeleteDC(hdcBack);
	DeleteDC(hdcObject);
	DeleteDC(hdcSave);
	DeleteDC(hdcTemp);
}



/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CStatic	m_CtrlTeamLogo;
	CStatic	m_CtrlIcon;
	CStatic	m_TitleStatic;
	CEdit	m_TrademarkEdit;
	CStatic	m_TdmfOse;
	CFont	m_Font;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_TEAM_LOGO, m_CtrlTeamLogo);
	DDX_Control(pDX, IDC_TDMF_ICON, m_CtrlIcon);
	DDX_Control(pDX, IDC_TDMF_STATIC, m_TitleStatic);
	DDX_Control(pDX, IDC_TRADEMARK, m_TrademarkEdit);
	DDX_Control(pDX, IDC_TDMF_OSE, m_TdmfOse);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CTdmfCommonGuiApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiApp message handlers


int CTdmfCommonGuiApp::ExitInstance() 
{
	CoUninitialize();

	if(bClassRegistered)
	{
		::UnregisterClass(_T("TdmfClass"),AfxGetInstanceHandle());
	}

	return CWinApp::ExitInstance();
}


BOOL CTdmfCommonGuiApp::PreTranslateMessage(MSG* pMsg)
{
#ifndef TDMF_IN_A_DLL
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;
#endif

	return CWinApp::PreTranslateMessage(pMsg);
}

void CAboutDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	DrawTransparentBitmap(dc, m_TdmfOse.GetBitmap(), 55, 20, RGB(255,255,255));	
	
	// Do not call CDialog::OnPaint() for painting messages
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));		// zero out structure
	lf.lfWeight = FW_BOLD;
	lf.lfHeight = 15;

	m_Font.CreateFontIndirect(&lf);			// create the font

	m_TitleStatic.SetFont(&m_Font);

	CString strVersion	= theApp.m_ResourceManager.GetFullProductName();
	strVersion			+= _T(" Common Console\r\nVersion ");
	strVersion			+= theApp.m_ResourceManager.GetProductVersion();
    strVersion			+= _T("  Build ");
    strVersion			+= theApp.m_ResourceManager.GetProductBuild();
  	m_TitleStatic.SetWindowText(strVersion);

	m_TrademarkEdit.SetWindowText(theApp.m_ResourceManager.GetCopyrightNotice());
	
	m_CtrlIcon.SetIcon(theApp.m_ResourceManager.GetApplicationIcon());

	CString cstrWndTitle = "About ";
	cstrWndTitle += theApp.m_ResourceManager.GetFullProductName();
	SetWindowText(cstrWndTitle);

	m_TdmfOse.SetBitmap(theApp.m_ResourceManager.GetAboutBoxLogo());

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTdmfCommonGuiApp::OnHelpContents() 
{
	HWND hWndHelp = NULL;

    hWndHelp = ::HtmlHelp(HWND_DESKTOP, m_cstrHelpFile, HH_DISPLAY_TOC, 0);

	if (hWndHelp == NULL)
	{	
		DWORD dwErr = GetLastError();
		if (dwErr == 0)
		{
			dwErr = 2;
		}

		LPVOID lpMsgBuf;
		
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL,
					  dwErr,
					  0, // Default language
					  (LPTSTR) &lpMsgBuf,
					  0,
					  NULL);

		::MessageBox(m_pMainWnd->m_hWnd, (LPCTSTR)lpMsgBuf, "Help Error", MB_OK | MB_ICONINFORMATION);

		// Free the buffer.
		LocalFree(lpMsgBuf);
	}
}

void CTdmfCommonGuiApp::OnHelpIndex() 
{
	HWND hWndHelp = NULL;

    hWndHelp = ::HtmlHelp(HWND_DESKTOP, m_cstrHelpFile, HH_DISPLAY_INDEX, 0);

	if (hWndHelp == NULL)
	{	
		DWORD dwErr = GetLastError();
		if (dwErr == 0)
		{
			dwErr = 2;
		}

		LPVOID lpMsgBuf;
		
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL,
					  dwErr,
					  0, // Default language
					  (LPTSTR) &lpMsgBuf,
					  0,
					  NULL);

		::MessageBox(m_pMainWnd->m_hWnd, (LPCTSTR)lpMsgBuf, "Help Error", MB_OK | MB_ICONINFORMATION);

		// Free the buffer.
		LocalFree(lpMsgBuf);
	}
}

void CTdmfCommonGuiApp::OnHelpSearch() 
{
	HWND hWndHelp = NULL;
	HH_FTS_QUERY q;

	q.cbStruct         = sizeof(HH_FTS_QUERY);
	q.fUniCodeStrings  = FALSE;
	q.pszSearchQuery   = "";
	q.iProximity       = HH_FTS_DEFAULT_PROXIMITY;
	q.fStemmedSearch   = FALSE;
	q.fTitleOnly       = FALSE;
	q.fExecute         = FALSE;
	q.pszWindow        = NULL;

    hWndHelp = ::HtmlHelp(HWND_DESKTOP, m_cstrHelpFile, HH_DISPLAY_SEARCH, (DWORD)&q);

	if (hWndHelp == NULL)
	{	
		DWORD dwErr = GetLastError();
		if (dwErr == 0)
		{
			dwErr = 2;
		}

		LPVOID lpMsgBuf;
		
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL,
					  dwErr,
					  0, // Default language
					  (LPTSTR) &lpMsgBuf,
					  0,
					  NULL);

		::MessageBox(m_pMainWnd->m_hWnd, (LPCTSTR)lpMsgBuf, "Help Error", MB_OK | MB_ICONINFORMATION);

		// Free the buffer.
		LocalFree(lpMsgBuf);
	}
}


void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	static DWORD dwInternal = 0;

	CRect Rect;
	m_CtrlIcon.GetWindowRect(&Rect);
	ClientToScreen(&point);
	
	if ((nFlags & MK_CONTROL) && Rect.PtInRect(point) && (LOWORD(m_TrademarkEdit.GetSel()) == m_TrademarkEdit.GetWindowTextLength()))
	{
		if (!(dwInternal++ ^ ~0xFFFFFFFD))
		{
			CString cstrRes;
			cstrRes.Format("#%d", IDR_METAFILE2);
			HRSRC hRsrc = FindResource(AfxGetResourceHandle(), cstrRes, "METAFILE");
			if (hRsrc != NULL)
			{
				HGLOBAL hResource = LoadResource(AfxGetResourceHandle(), hRsrc);

				BYTE* lpData = (BYTE*)LockResource(hResource);
				if (lpData != NULL)
				{
					HENHMETAFILE hMetaFile = SetEnhMetaFileBits(27900, lpData);

					m_CtrlTeamLogo.SetEnhMetaFile(hMetaFile);

					m_CtrlTeamLogo.ShowWindow(SW_SHOW);

					 /* Release the memory */
					UnlockResource(hResource);
				}
				FreeResource(hResource);
			}
		}
	}
	else
	{
		dwInternal ^= dwInternal;
		m_CtrlTeamLogo.ShowWindow(SW_HIDE);
	}

	CDialog::OnLButtonDown(nFlags, point);
}
