//
// MonitorToolDlg.cpp : implementation file
//
// Date			Who			What
//
// 7-19-2001	DTurrin		Code for splash screen added
//

#include "stdafx.h"
#include "Splash.h"
#include "MonitorTool.h"
#include "MonitorToolDlg.h"

#include "ftd_perf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
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
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMonitorToolDlg dialog

CMonitorToolDlg::CMonitorToolDlg(CWnd* pParent /*=NULL*/)
	: ETSLayoutDialog(CMonitorToolDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMonitorToolDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
//	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

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

void CMonitorToolDlg::DoDataExchange(CDataExchange* pDX)
{
	ETSLayoutDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMonitorToolDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMonitorToolDlg, ETSLayoutDialog)
	//{{AFX_MSG_MAP(CMonitorToolDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
	ON_LBN_DBLCLK(IDC_LIST_ERRORS, OnDblclkListErrors)
	ON_BN_CLICKED(IDC_BUTTON_CONFIG_TOOL, OnButtonConfigTool)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_NOW, OnButtonUpdateNow)
	ON_WM_RBUTTONDOWN()
	ON_LBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
	ON_LBN_SELCHANGE(IDC_LIST_UPDATE_INTERVAL, OnSelchangeListUpdateInterval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMonitorToolDlg message handlers

BOOL CMonitorToolDlg::OnInitDialog()
{
	CSplashWnd::EnableSplashScreen(TRUE); //Splash screen added on 7-19-2001
	CSplashWnd::ShowSplashScreen(this);   //by DTurrin@softek.fujitsu.com

	ETSLayoutDialog::OnInitDialog();

	CPane groupPane0 = paneCtrl( IDC_MESSAGES_STATIC, HORIZONTAL, GREEDY, nDefaultBorder, 10, 10 )
		<< item( IDC_LIST_ERRORS, GREEDY, 0, 0, 0, 18 );

	CPane groupPane1 = paneCtrl( IDC_DEVICES_STATIC, HORIZONTAL, GREEDY, nDefaultBorder, 10, 10 )
		<< item( IDC_LIST_DEVICES, GREEDY, 0, 0, 0, 52 );
	
	CPane groupPane2 = pane( HORIZONTAL, ABSOLUTE_VERT )
		<< item( IDC_UPDATE_INTERVAL_STATIC, GREEDY )
		<< item( IDC_LIST_UPDATE_INTERVAL, GREEDY )
		<< item( IDC_SECONDS_STATIC, GREEDY )
		<< item( IDC_BUTTON_UPDATE_NOW, GREEDY )
		<< item( IDC_BUTTON_CONFIG_TOOL, GREEDY )
		<< item( IDOK, GREEDY );
	
	CreateRoot(VERTICAL);

	m_RootPane->addPane(groupPane0, RELATIVE_VERT, 33);
	m_RootPane->addPane(groupPane1, GREEDY, 0);
	m_RootPane->addPane(groupPane2, RELATIVE_VERT, 0);

	UpdateLayout();

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
	m_perfp = NULL;
	m_iIndex = 0;
	m_dwNumEvents = 0;

	m_iConnectionPriority = GREY;
	m_iPercentPriority = GREY;
	m_iReadPriority = GREY;
	m_iWritePriority = GREY;
	m_iActualPriority = GREY;
	m_iEffectivePriority = GREY;
	m_iEntriesPriority = GREY;
	m_iBabPriority = GREY;

	//	m_szNewestEvent = NULL;
	memset(m_szNewestEvent, 0, sizeof(m_szNewestEvent));

	m_GetPerf = FALSE;
	memset(m_PrevDeviceInstanceData, 0, sizeof(m_PrevDeviceInstanceData));
	setDlgMemberVariables();
	readEventLog();
	initLogicalGroupList();
	initUpdateInterval();
	initDeviceListCtrl();
	if (createPerf() == -1)
		return FALSE;
	m_iTimeDiff = 10;
	time( &m_iLastUpdateTime );
	time( &m_iCurrentTime );
	m_iCurrentTime++;
	m_iUpdateInterval = 10000;

	getRegUpdateInterval();

#ifdef SFTK
	CWnd *cWndAppStarted = FindWindow(NULL, "Softek TDMF Monitor Tool");
#else
	CWnd *cWndAppStarted = FindWindow(NULL, PRODUCTNAME " Monitor Tool");
#endif
	if(cWndAppStarted)
	{
		cWndAppStarted->SetForegroundWindow( );
        if (cWndAppStarted->IsIconic())
			cWndAppStarted->ShowWindow(SW_RESTORE);

		OnCancel();
	}
	else
	{
#ifdef SFTK
		SetWindowText( "Softek TDMF Monitor Tool" );
#else
		SetWindowText( PRODUCTNAME " Monitor Tool" );
#endif

		startThreadToDoTimedQueries();
	}

	getRegColumnWidth();
	setColumnWidth();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMonitorToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		ETSLayoutDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMonitorToolDlg::OnPaint() 
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
		ETSLayoutDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMonitorToolDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMonitorToolDlg::OnSelchangeComboGroups() 
{
	int iGroupIndex = 0;

	iGroupIndex = m_pGroupsListBox->GetCurSel( );
	m_pGroupsListBox->GetText(iGroupIndex, m_szGroup);
}


void CMonitorToolDlg::setDlgMemberVariables() 
{
	m_pErrorsListBox = (CListBox *)GetDlgItem(IDC_LIST_ERRORS);

	m_pGroupsListBox = (CListBox *)GetDlgItem(IDC_COMBO_GROUPS);

	m_pDevicesListCtrl = (CListCtrl *)GetDlgItem(IDC_LIST_DEVICES);

	m_pUpdateIntervalCtrl = (CComboBox *)GetDlgItem(IDC_LIST_UPDATE_INTERVAL);
}

void CMonitorToolDlg::fillErrorMessageListBox(LPTSTR StringData)
{
	m_pErrorsListBox->AddString(StringData);
}

void CMonitorToolDlg::initUpdateInterval()
{
	char	szInterval[3];

	for(int i = 1; i <= 500; i++)
	{
		memset(szInterval, 0, sizeof(szInterval));
		itoa(i, szInterval, 10);
		m_pUpdateIntervalCtrl->AddString(szInterval);
	}
	m_pUpdateIntervalCtrl->SetCurSel(9);
}

void CMonitorToolDlg::readEventLog()
{
	EVENTLOGRECORD *pevlr;
	LPTSTR	StringData = NULL, SourceName;
    DWORD	dwRc, dwRead, dwNeeded, dwNext, dwNumEvents;
	BYTE	bBuffer[256];
	BOOL	bRc, NewestEvent = TRUE;

	// Open the Application event log. 
	m_hLog = OpenEventLog(NULL, "Application");
	
	GetNumberOfEventLogRecords(m_hLog, &dwNumEvents);

	// if it's been cleared - clear list and return
	if (m_dwNumEvents > 0 && dwNumEvents == 0) {
		m_pErrorsListBox->SetRedraw(FALSE);
		m_pErrorsListBox->ResetContent();
		goto errret;		
	}

	DWORD	i;

	// insert new events at front
	for (i = dwNumEvents; i > 0; i--) {

		pevlr = (EVENTLOGRECORD*)bBuffer;

		bRc = ReadEventLog(m_hLog,			// event log handle 
			EVENTLOG_SEQUENTIAL_READ |	// reads forward 
			EVENTLOG_BACKWARDS_READ,  
			i,							// record number  
			pevlr,						// pointer to buffer 
			0,							// size of read 
			&dwRead,					// number of bytes read 
			&dwNext);					// bytes in next record 

		dwNeeded = dwNext;

		pevlr = (EVENTLOGRECORD*)malloc(dwNeeded);

		if (!(bRc = ReadEventLog(m_hLog,// event log handle 
			EVENTLOG_SEQUENTIAL_READ |	// reads forward 
			EVENTLOG_BACKWARDS_READ,  
			i,							// record number  
			pevlr,						// pointer to buffer 
			dwNeeded,					// size of read 
			&dwRead,					// number of bytes read 
			&dwNext)))					// bytes in next record 
		{
			free(pevlr);
			goto errret;
		}

		SourceName = (LPSTR)((LPBYTE)pevlr + sizeof(EVENTLOGRECORD));

		if (!strcmp(SourceName, PRODUCTNAME)) {

			StringData = (LPSTR)((LPBYTE)pevlr + pevlr->StringOffset);
			
			if (NewestEvent) {

				NewestEvent = FALSE;

				// if there are new events or the size of the event
				// log has changed then re-read the event log

				if (m_dwNumEvents != dwNumEvents 
					|| strcmp(StringData, m_szNewestEvent))
				{
					memset(m_szNewestEvent, 0, sizeof(m_szNewestEvent));
					strcpy(m_szNewestEvent, StringData);

					//m_szNewestEvent = _strdup(StringData);
					m_pErrorsListBox->SetRedraw(FALSE);
					m_pErrorsListBox->ResetContent();
				} else {
					free(pevlr);
					goto errret;
				}
			}

			// add it
			fillErrorMessageListBox(StringData);			

		//	free(pevlr);
		}

		free(pevlr);		
    } 

errret:

	m_dwNumEvents = dwNumEvents;
	m_pErrorsListBox->SetRedraw(TRUE);
	dwRc = GetLastError();
	CloseEventLog(m_hLog);

}

void CMonitorToolDlg::OnDblclkListErrors() 
{
	CString	strError;
	m_pErrorsListBox->GetText(m_pErrorsListBox->GetCurSel(), strError); 

#ifdef SFTK
	::MessageBox(m_hWnd, strError, "Softek TDMF Monitor Tool", MB_OK);
#else
	::MessageBox(m_hWnd, strError, PRODUCTNAME " Monitor Tool", MB_OK);
#endif
	
}

int CMonitorToolDlg::createPerf()
{
 
	if ( (m_perfp = ftd_perf_create()) == NULL) {
		return -1;
	}
 
	// this opens the shared memory and mutex for you
	 if ( ftd_perf_init(m_perfp, TRUE) ) {
		return -1;
	}
	
	return 0;
}
 
void CMonitorToolDlg::destroyPerf()
{
    if (m_perfp)
        ftd_perf_delete(m_perfp);
}


int CMonitorToolDlg::getDataPerf()
{
	BOOL			bFreeMutex;
	int				rc;

	m_iIndex = 0;

	memset(m_DeviceInstanceData, 0, sizeof(m_DeviceInstanceData));
 	SetEvent(m_perfp->hGetEvent);
 	if ((rc = WaitForSingleObject(m_perfp->hSetEvent, 1000)) != WAIT_OBJECT_0) {
 		if (rc == WAIT_TIMEOUT) {
 			return -2;
		}
		return -1;
	}
 
	// ok now get the data
    if (m_perfp->hMutex != NULL) {
        if (WaitForSingleObject (m_perfp->hMutex, SHARED_MEMORY_MUTEX_TIMEOUT) == WAIT_FAILED) {
            // unable to obtain a lock
            bFreeMutex = FALSE;
        } else {
            bFreeMutex = TRUE;
        }
    } else {
        bFreeMutex = FALSE;
    }
 
    // point to the first instance structure in the shared buffer
    ftd_perf_instance_t *pThisDeviceInstanceData = m_perfp->pData;
 
    // process each of the instances in the shared memory buffer
    while (*((int*)pThisDeviceInstanceData) != -1){

        // set pointer to first counter data field
		// set m_DeviceInstanceData for each device
		m_DeviceInstanceData[m_iIndex].role = pThisDeviceInstanceData->role;
		m_DeviceInstanceData[m_iIndex].connection = pThisDeviceInstanceData->connection;
		m_DeviceInstanceData[m_iIndex].drvmode = pThisDeviceInstanceData->drvmode;
		
		m_DeviceInstanceData[m_iIndex].devid = pThisDeviceInstanceData->devid;
		m_DeviceInstanceData[m_iIndex].lgnum = pThisDeviceInstanceData->lgnum;
		
		m_DeviceInstanceData[m_iIndex].actual = pThisDeviceInstanceData->actual;
		m_DeviceInstanceData[m_iIndex].effective = pThisDeviceInstanceData->effective;
		m_DeviceInstanceData[m_iIndex].rsyncoff = pThisDeviceInstanceData->rsyncoff;
		m_DeviceInstanceData[m_iIndex].rsyncdelta = pThisDeviceInstanceData->rsyncdelta;
		m_DeviceInstanceData[m_iIndex].entries = pThisDeviceInstanceData->entries;
		m_DeviceInstanceData[m_iIndex].sectors = pThisDeviceInstanceData->sectors;
		m_DeviceInstanceData[m_iIndex].pctdone = pThisDeviceInstanceData->pctdone;
		m_DeviceInstanceData[m_iIndex].pctbab = pThisDeviceInstanceData->pctbab;
		m_DeviceInstanceData[m_iIndex].bytesread = pThisDeviceInstanceData->bytesread;
		m_DeviceInstanceData[m_iIndex].byteswritten = pThisDeviceInstanceData->byteswritten;
		memcpy(m_DeviceInstanceData[m_iIndex].wcszInstanceName, pThisDeviceInstanceData->wcszInstanceName, MAX_SIZEOF_INSTANCE_NAME * 2);
		
		m_iIndex++;

        // setup for the next instance
        pThisDeviceInstanceData++;
    }

    // done with the shared memory so free the mutex if one was 
    // acquired
    if (bFreeMutex) {
        ReleaseMutex (m_perfp->hMutex);
    }

	return 0;
} // perf get data
 

int CMonitorToolDlg::m_testServiceConnect()
{
	char	lhostname[MAXHOST];

	tcp_startup();

	sock_t *sockp = sock_create();
	gethostname(lhostname, sizeof(lhostname));

	if (sock_init(sockp, lhostname, lhostname, 0, 0, SOCK_STREAM, AF_INET, 1, 0) < 0) {
		goto errret;
	}
		
	sockp->port = FTD_SERVER_PORT;
	sockp->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	int	sockerr;
	if (sock_connect_nonb(sockp, sockp->port, 1, 0, &sockerr) < 0) {
		goto errret;
	}

	sock_disconnect(sockp);
	sock_delete(&sockp);
	tcp_cleanup();

	return 0;

errret:

	sock_delete(&sockp);
	tcp_cleanup();
	
	return -1;
}

void CMonitorToolDlg::doPerformanceData()
{
	static	int	runstatus = TRUE;	// assume service running
	int i, j, found;

	if (m_GetPerf) {
		// leave if we are already here
		return;
	}

	m_GetPerf = TRUE;

	int	rc;

	if ((rc = getDataPerf()) < 0) {
		if (rc == -2) {
			if (runstatus == TRUE) {
				// if service down then clear display - once
				if (m_testServiceConnect() < 0) {
					m_pDevicesListCtrl->DeleteAllItems();
					runstatus = FALSE;
				}
			}
		}
		m_GetPerf = FALSE;
		return;
	}

	if (runstatus == FALSE) {
		// must re-insert when service comes back
		for (i = 0; m_DeviceInstanceData[i].role; i++) {
			m_DeviceInstanceData[i].insert = 1;
		}
		runstatus = TRUE;
		goto display;
	}

	runstatus = TRUE;

	// find inserted items
	for (i = 0; m_DeviceInstanceData[i].role; i++) {
		m_DeviceInstanceData[i].insert = 0;
		found = FALSE;
		// locate entry in previous list
		for (j = 0; m_PrevDeviceInstanceData[j].role; j++) {
			if (m_DeviceInstanceData[i].lgnum == m_PrevDeviceInstanceData[j].lgnum) {
				if (m_DeviceInstanceData[i].devid == m_PrevDeviceInstanceData[j].devid) {
					// found it
					found = TRUE;
				}
			}
		}
		if (!found) {
#if 0
			char buf[256];
			sprintf(buf,"inserting %d:%d\n",
				m_DeviceInstanceData[i].lgnum,
				m_DeviceInstanceData[i].devid);
			MessageBox(buf, "insert",MB_OK);
#endif			
			m_DeviceInstanceData[i].insert = 1;
		}
	}

	// find deleted items
	for (i = 0; m_PrevDeviceInstanceData[i].role; i++) {
		m_PrevDeviceInstanceData[i].insert = 0;
		found = FALSE;
		// locate entry in current list
		for (j = 0; m_DeviceInstanceData[j].role; j++) {
			if (m_DeviceInstanceData[j].lgnum == m_PrevDeviceInstanceData[i].lgnum) {
				if (m_DeviceInstanceData[j].devid == m_PrevDeviceInstanceData[i].devid) {
					// found it
					found = TRUE;
				}
			}
		}
		if (!found) {
#if 0
			char buf[256];
			sprintf(buf,"deleting %d:%d\n",
				m_PrevDeviceInstanceData[i].lgnum,
				m_PrevDeviceInstanceData[i].devid);
			MessageBox(buf, "delete",MB_OK);
#endif
			m_PrevDeviceInstanceData[i].insert = -1;
		}
	}

display:

	fillDeviceListCtrl();

	m_GetPerf = FALSE;
}

UINT timedQuery(LPVOID pParam)
{
	CMonitorToolDlg *pMonitorToolDlg = (CMonitorToolDlg *) pParam;	

	DWORD	dwState = WAIT_TIMEOUT;

	while(dwState == WAIT_TIMEOUT)
	{
		// check for read of event log
		pMonitorToolDlg->readEventLog();

		// do perf monitor
		pMonitorToolDlg->doPerformanceData();

		dwState = WaitForSingleObject(pMonitorToolDlg->m_QueryEvent, pMonitorToolDlg->m_iUpdateInterval);
	}

	return 0;
}

void CMonitorToolDlg::startThreadToDoTimedQueries()
{
	m_QueryEvent = CreateEvent(NULL, TRUE, FALSE, "QueryEvent");
	m_hQuerieThread = AfxBeginThread( timedQuery, this );
}

void CMonitorToolDlg::OnOK() 
{
	SetEvent(m_QueryEvent);

	destroyPerf();

	setRegUpdateInterval();

	getColumnWidth();
	setRegColumnWidth();

	ETSLayoutDialog::OnOK();
}


void CMonitorToolDlg::getConfigPath(char *strPath)
{
	HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = _MAX_PATH;

    // Read the current state from the registry
    // Try opening the registry key:
    // HKEY_CURRENT_USER\Software\FullTime Software\<AppName>
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_KEY,
                        NULL,
                        &dwType,
                        (BYTE*)strPath,
                        &dwSize) != ERROR_SUCCESS) {
			
		}
		strcat(strPath, "\\");
	}
	
	RegCloseKey(happ);
}

void CMonitorToolDlg::OnButtonConfigTool() 
{
	char	strFtdStartPath[_MAX_PATH];
	char	strCmdLineArgs[_MAX_PATH];
	char	strConfig[] = QNM "ConfigTool";
	char	strConfigExe[] = QNM "ConfigTool.exe";
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));

	getConfigPath(strFtdStartPath);
	strcat(strFtdStartPath, strConfigExe);
	strcat(strCmdLineArgs, strConfig);

	CreateProcess(strFtdStartPath, NULL, NULL, NULL, TRUE,
				  CREATE_NEW_CONSOLE, NULL, NULL, &si, &ProcessInfo);	
}


void CMonitorToolDlg::addGroupToGroupList(int	iGroupID)
{
	char		strGroupID[100];

	memset(strGroupID, 0, sizeof(strGroupID));
	sprintf(strGroupID, "%03d", iGroupID); 
	m_pGroupsListBox->AddString( strGroupID );
	m_pGroupsListBox->SelectString( -1, strGroupID );
}

void CMonitorToolDlg::initLogicalGroupList()
{
	char	strFileName[_MAX_PATH];
	FILE	*file;
	int		arrayGroups[1000];
	char	strPath[_MAX_PATH];
	int		iGroupID = 0;
	int		iGroupIndex = 0;

	getConfigPath(strPath);

	memset(arrayGroups, 0, sizeof(arrayGroups));
	for(int i = 0; i <= 999; i++)
	{
		memset(strFileName, 0, sizeof(strFileName));
		sprintf(strFileName, "%sp%03d.cfg", strPath, i);
		file = fopen(strFileName, "r");
		if(file)
		{
			arrayGroups[i] = i;
			iGroupID = arrayGroups[i];
			addGroupToGroupList(iGroupID);
			fclose(file);
		}
	}

	iGroupIndex = m_pGroupsListBox->GetCurSel();
	m_pGroupsListBox->GetText(iGroupIndex, m_szGroup);
}

void CMonitorToolDlg::loadImageList()
{
	HICON		hIcon;
	
	// populate image list
	m_ImageList.Create(16, 16, ILC_COLOR, 1, 8);
	
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_GREEN);
	m_ImageList.Add(hIcon);
	
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_RED);
	m_ImageList.Add(hIcon);

	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_YELLOW);
	m_ImageList.Add(hIcon);

	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_NO_ACTIVITY);
	m_ImageList.Add(hIcon);

	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_DRIVE_STACK_OK);
	m_ImageList.Add(hIcon);

	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_DEVICE_OK);
	m_ImageList.Add(hIcon);

	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_DEVICE_ERROR);
	m_ImageList.Add(hIcon);

	hIcon = AfxGetApp()->LoadIcon(IDI_ICON_GROUP_ERROR);
	m_ImageList.Add(hIcon);
	
	m_pDevicesListCtrl->SetImageList( &m_ImageList, LVSIL_SMALL );
}

void CMonitorToolDlg::initDeviceListCtrl()
{
	loadImageList();

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 80;
	lvColumn.cchTextMax = _MAX_PATH;
	lvColumn.iImage = 3;

	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = "Group / Device";
	m_pDevicesListCtrl->InsertColumn( 1, &lvColumn);
	lvColumn.mask = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = "Connection";
	m_pDevicesListCtrl->InsertColumn( 2, &lvColumn);
	lvColumn.pszText = "Mode / %Done";
	m_pDevicesListCtrl->InsertColumn( 3, &lvColumn);
	lvColumn.pszText = "Read Kbps";
	m_pDevicesListCtrl->InsertColumn( 4, &lvColumn);
	lvColumn.pszText = "Write Kbps";
	m_pDevicesListCtrl->InsertColumn( 5, &lvColumn);
	lvColumn.pszText = "Actual Net";
	m_pDevicesListCtrl->InsertColumn( 6, &lvColumn);
	lvColumn.pszText = "Effective Net";
	m_pDevicesListCtrl->InsertColumn( 7, &lvColumn);
	lvColumn.pszText = "Entries";
	m_pDevicesListCtrl->InsertColumn( 8, &lvColumn);
	lvColumn.pszText = "% In BAB";
	m_pDevicesListCtrl->InsertColumn( 9, &lvColumn);

//m_pDevicesListCtrl->Arrange(LVA_SNAPTOGRID);
   
}

int CMonitorToolDlg::getIconColor(int iColumn)
{
	LVCOLUMN lvColumn;
	
	lvColumn.mask = LVCF_FMT | LVCF_IMAGE ;
	lvColumn.fmt = LVCFMT_LEFT | LVCFMT_COL_HAS_IMAGES | LVCFMT_IMAGE;
	lvColumn.cchTextMax = _MAX_PATH;

	m_pDevicesListCtrl->GetColumn(iColumn, &lvColumn);
	
	return lvColumn.iImage;
}

void CMonitorToolDlg::changeIconColor(int iColor, int iColumn)
{
	LVCOLUMN lvColumn;
	
	lvColumn.mask = LVCF_FMT | LVCF_IMAGE ;
	lvColumn.fmt = LVCFMT_LEFT | LVCFMT_COL_HAS_IMAGES | LVCFMT_IMAGE;
	lvColumn.cchTextMax = _MAX_PATH;
	lvColumn.iImage = iColor;

	m_pDevicesListCtrl->SetColumn(iColumn, &lvColumn);
}

void CMonitorToolDlg::changeIconColor(int iColor, int iColumn, int check)
{
	LVCOLUMN lvColumn;
	
	if (getIconColor(iColumn) != iColor) {
		lvColumn.mask = LVCF_FMT | LVCF_IMAGE ;
		lvColumn.fmt = LVCFMT_LEFT | LVCFMT_COL_HAS_IMAGES | LVCFMT_IMAGE;
		lvColumn.cchTextMax = _MAX_PATH;
		lvColumn.iImage = iColor;
		m_pDevicesListCtrl->SetColumn(iColumn, &lvColumn);
	}
}

int CMonitorToolDlg::getDeviceIconColor(int iColumn)
{
	LVITEM	pItem;

	memset(&pItem, 0, sizeof(pItem));

	pItem.iItem = iColumn;
	pItem.iSubItem = 0;
	pItem.mask = LVIF_IMAGE;

	m_pDevicesListCtrl->GetItem(&pItem);
	
	return pItem.iImage;
}

void CMonitorToolDlg::changeDeviceIconColor(int iColor, int iColumn)
{
	m_pDevicesListCtrl->SetItem(iColumn, 0, LVIF_IMAGE, NULL, iColor, 0, 0, 0L);
}

void CMonitorToolDlg::changeDeviceIconColor(int iColor, int iColumn, int check)
{
	if (getDeviceIconColor(iColumn) != iColor) {
		m_pDevicesListCtrl->SetItem(iColumn, 0, LVIF_IMAGE, NULL, iColor, 0, 0, 0L);
	}
}

void CMonitorToolDlg::fillDeviceListCtrl()
{
	int		iGroupIndex, iDevIndex = 0, iGroupNum;
	char	strData[_MAX_PATH];
	char	strDev[_MAX_PATH];
	char	strModePercent[_MAX_PATH];

	char	strConnection[3][15] = {"CONNECTED", "NOT CONNECTED", "ACCUMULATE"};
	char	strModes[5][10] = {"PASSTHRU", "NORMAL", "TRACKING", "REFRESH", "BACKFRESH"};
	int		iMode = 1;
	int		iNum = 0;
	CSize	size(0, 17);
	__int64	bytesread_delta, byteswritten_delta, actual_delta, effective_delta;

	m_iConnectionPriority = GREY;
	m_iPercentPriority = GREY;
	m_iReadPriority = GREY;
	m_iWritePriority = GREY;
	m_iActualPriority = GREY;
	m_iEffectivePriority = GREY;
	m_iEntriesPriority = GREY;
	m_iBabPriority = GREY;

	time( &m_iCurrentTime );

	m_iTimeDiff = m_iCurrentTime - m_iLastUpdateTime;
	if(m_iTimeDiff == 0)
		m_iTimeDiff++;

	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE;
	lvItem.iImage = 1;

	iNum = m_pDevicesListCtrl->GetTopIndex();
	size.cy = size.cy * iNum;

	iGroupIndex = m_pGroupsListBox->GetCurSel( );
	m_pGroupsListBox->GetText(iGroupIndex, m_szGroup);
	iGroupNum = atoi(m_szGroup);

	int		i, j;
	char	strConnect[15], strMode[_MAX_PATH];
	
	// delete items
	for (i = 0, j = 0; m_PrevDeviceInstanceData[i].role; i++, j++)
	{
		if (m_PrevDeviceInstanceData[i].insert == -1) {
			// delete item from the list control
			m_pDevicesListCtrl->DeleteItem(j);
			// re-init the item in the prev array
			memset(&m_PrevDeviceInstanceData[i], 0, sizeof(m_PrevDeviceInstanceData[i]));
			// backup one in list
			j--;
		}
	}
	
	for (i = 0; i < m_iIndex; i++)
	{
		int size = wcstombs(strDev, m_DeviceInstanceData[i].wcszInstanceName, _MAX_PATH);
		
		// only if item needs inserted in list control
		if (m_DeviceInstanceData[i].insert == 1) {
			if(m_DeviceInstanceData[i].role == 'p')
				m_pDevicesListCtrl->InsertItem(LVIF_IMAGE | LVIF_TEXT, i, strDev, 0, 0, 4, 0L);
			else
				m_pDevicesListCtrl->InsertItem(LVIF_IMAGE | LVIF_TEXT, i, strDev, 0, 0, 5, 0L);
		}

		switch( m_DeviceInstanceData[i].drvmode ) 
		{
			case FTD_MODE_PASSTHRU:
				iMode = 0;
				break;
			case FTD_MODE_NORMAL:
				iMode = 1;
				break;
			case FTD_MODE_TRACKING:
				iMode = 2;
				break;
			case FTD_MODE_REFRESH:
				iMode = 3;
				break;
			case FTD_MODE_BACKFRESH:
				iMode = 4;
				break;
		} // end switch

		memset(strConnect, 0, sizeof(strConnect));

		// connection
		if(m_DeviceInstanceData[i].connection == 1)
		{
			// connected (GREEN)
			m_pDevicesListCtrl->GetItemText(i, 1, strConnect, sizeof(strConnect));
			if (strcmp(strConnect, strConnection[0]))
			{
				m_pDevicesListCtrl->SetItemText(i, 1, strConnection[0]);
				
				if(m_iConnectionPriority <= GREEN)
				{
					m_iConnectionPriority = GREEN;
					changeIconColor(0, 1, 1);
				}
			}
		}
		else if(m_DeviceInstanceData[i].connection == -1)
		{
			m_pDevicesListCtrl->GetItemText(i, 1, strConnect, sizeof(strConnect));
			if (strcmp(strConnect, strConnection[2])) {
				// accumulate (RED)
				m_pDevicesListCtrl->SetItemText(i, 1, strConnection[2]);

				if(m_iConnectionPriority <= RED)
				{
					m_iConnectionPriority = RED;
					changeIconColor(1, 1, 1);
				}
			}
		}
		else
		{
			m_pDevicesListCtrl->GetItemText(i, 1, strConnect, sizeof(strConnect));
			if (strcmp(strConnect, strConnection[1])) {
				// not connected (YELLOW)
				m_pDevicesListCtrl->SetItemText(i, 1, strConnection[1]);

				if(m_iConnectionPriority <= YELLOW)
				{
					m_iConnectionPriority = YELLOW;
					changeIconColor(2, 1, 1);
				}
			}
		}
		// end connection

		// percent done and mode
		memset(strData, 0, sizeof(strData));
		memset(strModePercent, 0, sizeof(strModePercent));
		
		itoa(m_DeviceInstanceData[i].pctdone, strData, 10);
		
		m_pDevicesListCtrl->GetItemText(i, 2, strMode, sizeof(strMode));

		if (m_DeviceInstanceData[i].insert == 1 
			|| strcmp(strMode, strModes[iMode]))
		{
			if(iMode == 0 || iMode == 1 || iMode == 2) {
				sprintf(strModePercent, "%s", strModes[iMode]);	
			} else {
				sprintf(strModePercent, "%s / %s", strModes[iMode], strData);
			}
			m_pDevicesListCtrl->SetItemText(i, 2, strModePercent);
		}

		if(iMode == 1 || iMode == 0)
		{	
			// GREEN
			if(m_iPercentPriority <= GREEN)
			{
				m_iPercentPriority = GREEN;
				changeIconColor(0, 2, 1);
			}
			// DTurrin - Oct 15th, 2001
			// Added this check to set the device
			// icon color when the state changes
			// to NORMAL or PASSTHRU
			if(m_DeviceInstanceData[i].role == 'p')
				changeDeviceIconColor(4, i, 1);
			else
				changeDeviceIconColor(5, i, 1);
		}
		else if(iMode == 3 || iMode == 4)
		{
			// YELLOW
			if(m_iPercentPriority <= YELLOW)
			{
				m_iPercentPriority = YELLOW;
				changeIconColor(2, 2, 1);
			}
		}
		else if(iMode == 2)
		{
			// RED
			if(m_iPercentPriority <= RED)
			{
				m_iPercentPriority = RED;
				changeIconColor(1, 2, 1);
			}
			if(m_DeviceInstanceData[i].role == 'p')
				changeDeviceIconColor(7, i, 1);
			else
				changeDeviceIconColor(6, i, 1);
		}
		// End percent done and mode

		// if i/o stats are 0 then show 0 instead of calculating
		// because it causes a huge # blip on the monitor

		memset(strData, 0, sizeof(strData));
		bytesread_delta = m_DeviceInstanceData[i].bytesread - m_PrevDeviceInstanceData[i].bytesread;
		if (bytesread_delta <= 0) {
			_ui64toa(0, strData, 10);
		} else {
			_ui64toa(bytesread_delta/1024/m_iTimeDiff, strData, 10);
		}
		m_pDevicesListCtrl->SetItemText(i, 3, strData);
		if (bytesread_delta/1024/10 > 0)
		{
			// GREEN
			if(m_iReadPriority <= GREEN)
			{
				m_iReadPriority = GREEN;
				changeIconColor(0, 3, 1);
			}
		}
		else
		{
			// GREY
			if(m_iReadPriority <= GREY)
			{
				m_iReadPriority = GREY;
				changeIconColor(3, 3, 1);
			}
		}
	
		memset(strData, 0, sizeof(strData));
		byteswritten_delta = m_DeviceInstanceData[i].byteswritten - m_PrevDeviceInstanceData[i].byteswritten;
		if (byteswritten_delta <= 0) {
			_ui64toa(0, strData, 10);
		} else {
			_ui64toa(byteswritten_delta/1024/m_iTimeDiff, strData, 10);
		}
		m_pDevicesListCtrl->SetItemText(i, 4, strData);
		if (byteswritten_delta/1024/10 > 0)
		{
			// GREEN
			if(m_iWritePriority <= GREEN)
			{
				m_iWritePriority = GREEN;
				changeIconColor(0, 4, 1);
			}
		}
		else
		{
			// GREY
			if(m_iWritePriority <= GREY)
			{
				m_iWritePriority = GREY;
				changeIconColor(3, 4, 1);
			}
		}

		memset(strData, 0, sizeof(strData));
		actual_delta = m_DeviceInstanceData[i].actual - m_PrevDeviceInstanceData[i].actual;
		if (actual_delta <= 0) {
			_ui64toa(0, strData, 10);
		} else {
			_ui64toa(actual_delta/1024/m_iTimeDiff, strData, 10);
		}
		m_pDevicesListCtrl->SetItemText(i, 5, strData);
		if (actual_delta/1024/10 > 0)
		{
			// GREEN
			if(m_iActualPriority <= GREEN)
			{
				m_iActualPriority = GREEN;
				changeIconColor(0, 5, 1);
			}
		}
		else
		{
			// GREY
			if(m_iActualPriority <= GREY)
			{
				m_iActualPriority = GREY;
				changeIconColor(3, 5, 1);
			}
		}

		memset(strData, 0, sizeof(strData));
		effective_delta = m_DeviceInstanceData[i].effective - m_PrevDeviceInstanceData[i].effective;
		if (effective_delta <= 0) {
			_ui64toa(0, strData, 10);
		} else {
			_ui64toa(effective_delta/1024/m_iTimeDiff, strData, 10);
		}
		m_pDevicesListCtrl->SetItemText(i, 6, strData);
		if (effective_delta/1024/10 > 0)
		{
			// GREEN
			if(m_iEffectivePriority <= GREEN)
			{
				m_iEffectivePriority = GREEN;
				changeIconColor(0, 6, 1);
			}
		}
		else
		{
			// GREY
			if(m_iEffectivePriority <= GREY)
			{
				m_iEffectivePriority = GREY;
				changeIconColor(3, 6, 1);
			}
		}

		memset(strData, 0, sizeof(strData));
		itoa(m_DeviceInstanceData[i].entries, strData, 10);
		m_pDevicesListCtrl->SetItemText(i, 7, strData);
		if(m_DeviceInstanceData[i].entries > 1)
		{
			// GREEN
			if(m_iEntriesPriority <= GREEN)
			{
				m_iEntriesPriority = GREEN;
				changeIconColor(0, 7, 1);
			}
		}
		else
		{
			// GREY
			if(m_iEntriesPriority <= GREY)
			{
				m_iEntriesPriority = GREY;
				changeIconColor(3, 7, 1);
			}
		}

		memset(strData, 0, sizeof(strData));
		itoa(m_DeviceInstanceData[i].pctbab, strData, 10);
		m_pDevicesListCtrl->SetItemText(i, 8, strData);
		if(m_DeviceInstanceData[i].pctbab > 80)
		{
			// RED
			if(m_iBabPriority <= RED)
			{
				m_iBabPriority = RED;
				changeIconColor(1, 8, 1);
			}
			if(m_DeviceInstanceData[i].role == 'p')
				changeDeviceIconColor(7, i, 1);
			else
				changeDeviceIconColor(6, i, 1);
		}
		else if(m_DeviceInstanceData[i].pctbab > 50 && m_DeviceInstanceData[i].pctbab < 80)
		{
			// YELLOW
			if(m_iBabPriority <= YELLOW)
			{
				m_iBabPriority = YELLOW;
				changeIconColor(2, 8, 1);
			}
		}
		else if(m_DeviceInstanceData[i].pctbab < 50)
		{
			// GREEN
			if(m_iBabPriority <= GREEN)
			{
				m_iBabPriority = GREEN;
				changeIconColor(0, 8, 1);
			}
		}
	
		memset(&m_PrevDeviceInstanceData[i], 0, sizeof(ftd_perf_instance_t));
		m_PrevDeviceInstanceData[i] = m_DeviceInstanceData[i];
	}

	m_pDevicesListCtrl->Scroll(size);
	time( &m_iLastUpdateTime );
}



void CMonitorToolDlg::OnButtonUpdateNow() 
{
	readEventLog();

	doPerformanceData();
}

void CMonitorToolDlg::OnRButtonDown(UINT nFlags, CPoint point) 
{
	ETSLayoutDialog::OnRButtonDown(nFlags, point);
}

void CMonitorToolDlg::OnSelchangeListUpdateInterval() 
{
	int iInterval = m_pUpdateIntervalCtrl->GetCurSel();

	m_iUpdateInterval = (iInterval + 1) * 1000;
}

void CMonitorToolDlg::setRegUpdateInterval()
{
	HKEY	happ;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                     KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		RegSetValueEx(happ, REG_KEY_UPDATE_INTERVAL, 0, REG_DWORD, (const BYTE *)&m_iUpdateInterval,
						sizeof(DWORD));
		
		RegCloseKey(happ);
	}
}

void CMonitorToolDlg::getRegUpdateInterval()
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_KEY_UPDATE_INTERVAL,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {
	
			m_iUpdateInterval = dwReadValue;
			m_pUpdateIntervalCtrl->SetCurSel((m_iUpdateInterval / 1000) - 1);
		}
		else
		{
			// no key there so create it
			m_iUpdateInterval = 10000;
			setRegUpdateInterval();
		}
	}
	
	RegCloseKey(happ);
}

void CMonitorToolDlg::getRegColumnWidth()
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_ONE,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth1 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth1 = m_pDevicesListCtrl->GetColumnWidth(1);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_TWO,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth2 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth2 = m_pDevicesListCtrl->GetColumnWidth(2);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_THREE,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth3 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth3 = m_pDevicesListCtrl->GetColumnWidth(3);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_FOUR,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth4 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth4 = m_pDevicesListCtrl->GetColumnWidth(4);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_FIVE,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth5 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth5 = m_pDevicesListCtrl->GetColumnWidth(5);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_SIX,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth6 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth6 = m_pDevicesListCtrl->GetColumnWidth(6);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_SEVEN,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth7 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth7 = m_pDevicesListCtrl->GetColumnWidth(7);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_EIGHT,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth8 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth8 = m_pDevicesListCtrl->GetColumnWidth(8);
		}

		if ( RegQueryValueEx(happ,
                        REG_KEY_COLUMN_ZERO,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS) {

			m_iColWidth0 = dwReadValue;
		}
		else
		{
			// no key there so create it
			m_iColWidth0 = m_pDevicesListCtrl->GetColumnWidth(0);
		}
	}
	
	RegCloseKey(happ);
}

void CMonitorToolDlg::setRegColumnWidth()
{
		HKEY	happ;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                     KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		RegSetValueEx(happ, REG_KEY_COLUMN_ZERO, 0, REG_DWORD, (const BYTE *)&m_iColWidth0, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_ONE, 0, REG_DWORD, (const BYTE *)&m_iColWidth1, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_TWO, 0, REG_DWORD, (const BYTE *)&m_iColWidth2, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_THREE, 0, REG_DWORD, (const BYTE *)&m_iColWidth3, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_FOUR, 0, REG_DWORD, (const BYTE *)&m_iColWidth4, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_FIVE, 0, REG_DWORD, (const BYTE *)&m_iColWidth5, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_SIX, 0, REG_DWORD, (const BYTE *)&m_iColWidth6, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_SEVEN, 0, REG_DWORD, (const BYTE *)&m_iColWidth7, sizeof(DWORD));
		RegSetValueEx(happ, REG_KEY_COLUMN_EIGHT, 0, REG_DWORD, (const BYTE *)&m_iColWidth8, sizeof(DWORD));
		
		RegCloseKey(happ);
	}	
}

void CMonitorToolDlg::setColumnWidth()
{
	m_pDevicesListCtrl->SetColumnWidth(0, m_iColWidth0);
	m_pDevicesListCtrl->SetColumnWidth(1, m_iColWidth1);
	m_pDevicesListCtrl->SetColumnWidth(2, m_iColWidth2);
	m_pDevicesListCtrl->SetColumnWidth(3, m_iColWidth3);
	m_pDevicesListCtrl->SetColumnWidth(4, m_iColWidth4);
	m_pDevicesListCtrl->SetColumnWidth(5, m_iColWidth5);
	m_pDevicesListCtrl->SetColumnWidth(6, m_iColWidth6);
	m_pDevicesListCtrl->SetColumnWidth(7, m_iColWidth7);
	m_pDevicesListCtrl->SetColumnWidth(8, m_iColWidth8);
}

void CMonitorToolDlg::getColumnWidth()
{
	m_iColWidth0 = m_pDevicesListCtrl->GetColumnWidth(0);
	m_iColWidth1 = m_pDevicesListCtrl->GetColumnWidth(1);
	m_iColWidth2 = m_pDevicesListCtrl->GetColumnWidth(2);
	m_iColWidth3 = m_pDevicesListCtrl->GetColumnWidth(3);
	m_iColWidth4 = m_pDevicesListCtrl->GetColumnWidth(4);
	m_iColWidth5 = m_pDevicesListCtrl->GetColumnWidth(5);
	m_iColWidth6 = m_pDevicesListCtrl->GetColumnWidth(6);
	m_iColWidth7 = m_pDevicesListCtrl->GetColumnWidth(7);
	m_iColWidth8 = m_pDevicesListCtrl->GetColumnWidth(8);
}


BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

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
	SetWindowText( "About " PRODUCTNAME " Monitor Tool" );
	SetDlgItemText(IDC_STATIC_VERSION, PRODUCTNAME" "VERSION" "BLDSEQNUM);
	SetDlgItemText(IDC_STATIC_COPYRIGHT, "Copyright (c) 2001 " OEMNAME);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg) 
{
	//Code for splash screen added on 7-19-2001
	//by DTurrin@softek.fujitsu.com
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;
	
	return CDialog::PreTranslateMessage(pMsg);
}

