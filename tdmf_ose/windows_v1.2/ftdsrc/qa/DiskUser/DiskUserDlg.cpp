// DiskUserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DiskUser.h"
#include "DiskUserDlg.h"
#include "DlgProxy.h"
#include "ThreadUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MyTimerId 55

/////////////////////////////////////////////////////////////////////////////
// CDiskUserDlg dialog

IMPLEMENT_DYNAMIC(CDiskUserDlg, CDialog);

CDiskUserDlg::CDiskUserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDiskUserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDiskUserDlg)
	m_uiDiskSize = 0;
	m_uiNumThreads = 0;
	m_CurrentDrive = _T("");
	m_CurrentPriority = _T("");
	m_uiNumThreadsExecuting = 0;
	m_Kb_DiskSizePerThread = 0;
	m_TestStatus = _T("");
	m_uiSleepRange = 0;
	m_csReadWriteRatio = _T("");
	m_csSleepRange = _T("");
	m_iSliderPosition = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = NULL;
}

CDiskUserDlg::~CDiskUserDlg()
{
	// If there is an automation proxy for this dialog, set
	//  its back pointer to this dialog to NULL, so it knows
	//  the dialog has been deleted.
	if (m_pAutoProxy != NULL)
		m_pAutoProxy->m_pDialog = NULL;
}

void CDiskUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiskUserDlg)
	DDX_Control(pDX, IDC_EDIT_SleepRange, m_CtrlSleepRange);
	DDX_Control(pDX, IDC_SLIDER_ReadWriteRatio, m_SliderCtrl);
	DDX_Control(pDX, IDC_EDIT_NumThreads, m_CtrlNumThreads);
	DDX_Control(pDX, IDC_EDIT_DiskSize, m_CtrlDiskSize);
	DDX_Control(pDX, IDC_LISTPRIORITIES, m_ListBoxPriorities);
	DDX_Control(pDX, IDC_LISTDRIVENAMES, m_ListBoxDriveNames);
	DDX_Control(pDX, IDSTOP, m_StopButton);
	DDX_Control(pDX, IDSTART, m_StartButton);
	DDX_Text(pDX, IDC_EDIT_DiskSize, m_uiDiskSize);
	DDV_MinMaxUInt(pDX, m_uiDiskSize, 0, 100);
	DDX_Text(pDX, IDC_EDIT_NumThreads, m_uiNumThreads);
	DDV_MinMaxUInt(pDX, m_uiNumThreads, 0, MAX_THREADS);
	DDX_CBString(pDX, IDC_LISTDRIVENAMES, m_CurrentDrive);
	DDX_CBString(pDX, IDC_LISTPRIORITIES, m_CurrentPriority);
	DDX_Text(pDX, IDC_EDIT_NumThreadsExecuting, m_uiNumThreadsExecuting);
	DDX_Text(pDX, IDC_EDIT_Kb_DiskSizePerThread, m_Kb_DiskSizePerThread);
	DDX_Text(pDX, IDC_STATIC_TestStatus, m_TestStatus);
	DDX_Text(pDX, IDC_EDIT_SleepRange, m_uiSleepRange);
	DDV_MinMaxUInt(pDX, m_uiSleepRange, 1, 1000);
	DDX_Text(pDX, IDC_ReadWriteRatio, m_csReadWriteRatio);
	DDX_Text(pDX, IDC_SleepRange, m_csSleepRange);
	DDX_Slider(pDX, IDC_SLIDER_ReadWriteRatio, m_iSliderPosition);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDiskUserDlg, CDialog)
	//{{AFX_MSG_MAP(CDiskUserDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDSTART, OnStart)
	ON_BN_CLICKED(IDSTOP, OnStop)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_NumThreads, OnChangeEDITNumThreads)
	ON_EN_CHANGE(IDC_EDIT_DiskSize, OnChangeEDITDiskSize)
	ON_EN_CHANGE(IDC_EDIT_SleepRange, OnChangeEDITSleepRange)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiskUserDlg message handlers
DWORD           GetDriveLetters (CString *  Array, DWORD size );

BOOL CDiskUserDlg::OnInitDialog()
{
    CString csArray[256];
    DWORD   dwNumDrives;
    char    dbgbuf[250];

	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

    m_uiSleepRange  = 10;
    m_uiNumThreads  = 5;
    m_uiDiskSize    = 10;
    m_Valid         = true;

    m_SliderCtrl.SetTicFreq(10);
    m_SliderCtrl.SetRange(0,100,TRUE);
    m_iSliderPosition = 50;
   
    sprintf(dbgbuf,"Read %ld%%", m_iSliderPosition );
    m_csReadWriteRatio = dbgbuf;
    sprintf(dbgbuf,"(%ld - %ld ms)", m_uiSleepRange*10, m_uiSleepRange*100 );
    m_csSleepRange = dbgbuf;

    m_TestStatus = "Idle";
    //
    // Read current state!
    //
    UpdateData(FALSE);

    //
    // Get list of drives
    //
    dwNumDrives = GetDriveLetters(csArray,256);

    //
    // Add them to the listbox
    //
    for (DWORD i = 0; i< dwNumDrives;i++)
    {
        m_ListBoxDriveNames.AddString(csArray[i]);
        if (i==0)
        {
            m_ListBoxDriveNames.SelectString(0,csArray[i]);
        }
    }

    m_ListBoxPriorities.AddString("Below Normal");
    m_ListBoxPriorities.AddString("Normal");
    m_ListBoxPriorities.AddString("Above Normal");

    m_StopButton.EnableWindow(FALSE);
    m_StartButton.EnableWindow(TRUE);

    m_ListBoxPriorities.SelectString(0,"Normal");

    m_CtrlDiskSize.SetLimitText(3);
    m_CtrlNumThreads.SetLimitText(4);

    UpdateData(TRUE);


    ::SetTimer(m_hWnd, MyTimerId , 1000, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDiskUserDlg::OnPaint() 
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
HCURSOR CDiskUserDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CDiskUserDlg::OnClose() 
{
	if (CanExit())
		CDialog::OnClose();
}

void CDiskUserDlg::OnOK() 
{
	if (CanExit())
		CDialog::OnOK();
}

void CDiskUserDlg::OnCancel() 
{
	if (CanExit())
		CDialog::OnCancel();
}

BOOL CDiskUserDlg::CanExit()
{
	// If the proxy object is still around, then the automation
	//  controller is still holding on to this application.  Leave
	//  the dialog around, but hide its UI.
	if (m_pAutoProxy != NULL)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}

void CDiskUserDlg::OnStart() 
{
    ULARGE_INTEGER  liDiskFreeSpace;
    unsigned int    ActualSize      = 0;
    //
    // get values
    //
    UpdateData(TRUE);

    //
    // Validate them
    //
    if (    (m_uiNumThreads > MAX_THREADS)
        &&  (m_uiDiskSize > 100)    )
    {
        return;
    }

#if _DEBUG
    OutputDebugString("Validate priority\n");
#endif

    //
    // Validate selected priority!
    //
    switch (m_ListBoxPriorities.GetCurSel())
    {
        case 0:
#if _DEBUG
            OutputDebugString("Below normal priority threads will be created\n");
#endif
            SetPriorityClass(GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);
            break;

        case 1:
#if _DEBUG
            OutputDebugString("Normal priority threads will be created\n");
#endif
            SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
            break;

        case 2:
            //
            // Warn user on above normal!
            // 
            if (IDCANCEL == ::MessageBox(   NULL,
                    "You will run the threads at high priority!\nThis is very intensive for the system\nUndesireable effects may occur\nPress Cancel to abort\n",
                    "WARNING",
                    MB_OKCANCEL ) )
            {
                return;
            }
#if _DEBUG
            OutputDebugString("Above normal priority threads will be created\n");
#endif
            SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);
            break;
    }

    

    //
    // Validate disk selection and get disk usage for each thread
    //
    TU_GetDiskFreeSpaceEx(m_CurrentDrive,&liDiskFreeSpace);

    if (liDiskFreeSpace.QuadPart == 0)
    {
        //
        // No space available on drive!
        //
        return;
    }

    //
    // Disable Start (and edits), enable Stop
    //
	m_StopButton.EnableWindow(TRUE);
    m_StartButton.EnableWindow(FALSE);
    m_CtrlNumThreads.EnableWindow(FALSE);
    m_ListBoxDriveNames.EnableWindow(FALSE);
    m_ListBoxPriorities.EnableWindow(FALSE);
    m_CtrlDiskSize.EnableWindow(FALSE);
    m_SliderCtrl.EnableWindow(FALSE);
    m_CtrlSleepRange.EnableWindow(FALSE);

    //
    // Actual size = free size * disk% / numthreads
    //
    ActualSize = (unsigned int)(liDiskFreeSpace.QuadPart * m_uiDiskSize / 100 / m_uiNumThreads);


    //
    // Clear threads
    //
    TU_InitThreadList(m_iSliderPosition,m_uiSleepRange);

    m_TestStatus = "Running";

#define _START_THREADS 
#ifdef _START_THREADS
#pragma message("Threads active!")
    //
    // Start threads!
    //
    if (m_uiNumThreads)
    {
        for (unsigned int i = 0; i < m_uiNumThreads; i++)
        {
            TU_StartDiskThread(i, ActualSize, m_CurrentDrive.GetAt(0));
            Sleep(100);
        }
    }
#else
#pragma message("Threads inactive!")
#endif


    m_uiNumThreadsExecuting = TU_GetNumThreadsExecuting();

    m_Kb_DiskSizePerThread = ActualSize / 1024;

    UpdateData(FALSE);
}

void CDiskUserDlg::OnStop() 
{
	// TODO: Add your control notification handler code here
    m_StopButton.EnableWindow(FALSE);
    m_StartButton.EnableWindow(TRUE);
    m_CtrlNumThreads.EnableWindow(TRUE);
    m_ListBoxDriveNames.EnableWindow(TRUE);
    m_ListBoxPriorities.EnableWindow(TRUE);
    m_CtrlDiskSize.EnableWindow(TRUE);
    m_SliderCtrl.EnableWindow(TRUE);
    m_CtrlSleepRange.EnableWindow(TRUE);

    m_TestStatus = "Stopped";

    m_uiNumThreadsExecuting = TU_GetNumThreadsExecuting();
    
    TU_StopDiskThreads();

    UpdateData(FALSE);
}

void CDiskUserDlg::OnTimer(UINT nIDEvent) 
{
    static unsigned int m_iNumThreads = 0;
	// TODO: Add your message handler code here and/or call default
	
    if (nIDEvent == MyTimerId)
    {
        m_uiNumThreadsExecuting = TU_GetNumThreadsExecuting();

        if (m_iNumThreads != m_uiNumThreadsExecuting)
        {
            m_iNumThreads = m_uiNumThreadsExecuting;
            UpdateData(FALSE);
        }
    }

#if _DEBUG
    {
        char        DbgMsg[255];
        int         iNmReadsOk  = TU_GetNumReadsOK();
        static int  ipNmReadsOk = 0;
        int         iNmReadsNOk = TU_GetNumReadsNOK();
        static int  ipNmReadsNOk= 0;
        int         iNmWriteOk  = TU_GetNumWritesOK();
        static int  ipNmWriteOk = 0;
        int         iNmWriteNOk = TU_GetNumWritesNOK();
        static int  ipNmWriteNOk= 0;

        if (    (ipNmReadsOk    !=  iNmReadsOk) 
            ||  (ipNmReadsNOk   !=  iNmReadsNOk)    )
        {
            sprintf(DbgMsg,"DiskUser: %8ld Reads  %8ld broken reads\n",TU_GetNumReadsOK(),TU_GetNumReadsNOK());
            OutputDebugString(DbgMsg);
        }

        if (    (ipNmWriteOk    !=  iNmWriteOk)
            ||  (ipNmWriteNOk   !=  iNmWriteNOk)    )
        {
            sprintf(DbgMsg,"DiskUser: %8ld Writes %8ld broken writes\n",TU_GetNumWritesOK(),TU_GetNumWritesNOK());
            OutputDebugString(DbgMsg);
        }

        ipNmReadsOk  = iNmReadsOk; 
        ipNmReadsNOk = iNmReadsNOk;
        ipNmWriteOk  = iNmWriteOk; 
        ipNmWriteNOk = iNmWriteNOk;

    }
#endif

	CDialog::OnTimer(nIDEvent);
}

void CDiskUserDlg::OnChangeEDITNumThreads() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
    if (m_uiNumThreads > MAX_THREADS)
    {
        m_uiNumThreads = MAX_THREADS;
        UpdateData(FALSE);
    }

}

void CDiskUserDlg::OnChangeEDITDiskSize() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
    if (m_uiDiskSize > 100)
    {
        m_uiDiskSize = 100;
        UpdateData(FALSE);
    }
}

void CDiskUserDlg::OnChangeEDITSleepRange() 
{
    char dbgbuf[250];
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
    if (    (m_uiSleepRange > 1000)
        ||  (m_uiSleepRange < 1)    )
    {
        m_uiSleepRange = (m_uiSleepRange > 1000 ? 1000 : 1);
    }
    
    sprintf(dbgbuf,"(%ld - %ld ms)", m_uiSleepRange*10, m_uiSleepRange*100 );
    m_csSleepRange = dbgbuf;
	
    UpdateData(FALSE);
}

void CDiskUserDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
    char    dbgbuf[256];
	// TODO: Add your message handler code here and/or call default
    //
    // TODO: Add your message handler code here and/or call default
	//
    UpdateData();

    // Set the value in the control
    
    sprintf(dbgbuf,"Read %ld%%", m_iSliderPosition );
    m_csReadWriteRatio = dbgbuf;
	
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

    UpdateData(FALSE);
}
