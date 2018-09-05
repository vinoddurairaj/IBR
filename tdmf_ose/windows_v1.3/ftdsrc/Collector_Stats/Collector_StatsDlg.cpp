// Collector_StatsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Collector_Stats.h"
#include "Collector_StatsDlg.h"
#include "statistics.h"
#include "libmngt.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static HANDLE   g_hMMFile = 0;

/////////////////////////////////////////////////////////////////////////////
// CCollector_StatsDlg dialog

CCollector_StatsDlg::CCollector_StatsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCollector_StatsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCollector_StatsDlg)
	m_csNumOfDatabaseMessagesPending = _T("");
	m_csNumOfMessageThreadsPending = _T("");
	m_csCollectorTime = _T("");
	m_csNumMessagesPerHour = _T("");
	m_csNumMessagesPerMinute = _T("");
	m_csNumThreadsPerHour = _T("");
	m_csNumThreadsPerMinute = _T("");
	m_csMsg0 = _T("");
	m_csAliveAgents = _T("");
	m_csAliveMessagesPerHour = _T("");
	m_csAliveMessagesPerMinute = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCollector_StatsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCollector_StatsDlg)
	DDX_Text(pDX, IDC_DB_MSG_PEND, m_csNumOfDatabaseMessagesPending);
	DDX_Text(pDX, IDC_MSG_THRD_PEND, m_csNumOfMessageThreadsPending);
	DDX_Text(pDX, IDC_COL_TIME_STAMP, m_csCollectorTime);
	DDX_Text(pDX, IDC_MSG_PER_HOUR, m_csNumMessagesPerHour);
	DDX_Text(pDX, IDC_MSG_PER_MIN, m_csNumMessagesPerMinute);
	DDX_Text(pDX, IDC_MSG_THRD_HOUR, m_csNumThreadsPerHour);
	DDX_Text(pDX, IDC_MSG_THRD_MINUTE, m_csNumThreadsPerMinute);
	DDX_Text(pDX, IDC_MESG_0 , m_csMsg0 );
	DDX_Text(pDX, IDC_MESG_1 , m_csMsg1 );
	DDX_Text(pDX, IDC_MESG_2 , m_csMsg2 );
	DDX_Text(pDX, IDC_MESG_3 , m_csMsg3 );
	DDX_Text(pDX, IDC_MESG_4 , m_csMsg4 );
	DDX_Text(pDX, IDC_MESG_5 , m_csMsg5 );
	DDX_Text(pDX, IDC_MESG_6 , m_csMsg6 );
	DDX_Text(pDX, IDC_MESG_7 , m_csMsg7 );
	DDX_Text(pDX, IDC_MESG_8 , m_csMsg8 );
	DDX_Text(pDX, IDC_MESG_9 , m_csMsg9 );
	DDX_Text(pDX, IDC_MESG_10, m_csMsg10);
	DDX_Text(pDX, IDC_MESG_11, m_csMsg11);
	DDX_Text(pDX, IDC_MESG_12, m_csMsg12);
	DDX_Text(pDX, IDC_MESG_13, m_csMsg13);
	DDX_Text(pDX, IDC_MESG_14, m_csMsg14);
	DDX_Text(pDX, IDC_MESG_15, m_csMsg15);
	DDX_Text(pDX, IDC_MESG_16, m_csMsg16);
	DDX_Text(pDX, IDC_MESG_17, m_csMsg17);
	DDX_Text(pDX, IDC_MESG_18, m_csMsg18);
	DDX_Text(pDX, IDC_MESG_19, m_csMsg19);
	DDX_Text(pDX, IDC_MESG_20, m_csMsg20);
	DDX_Text(pDX, IDC_MESG_21, m_csMsg21);
	DDX_Text(pDX, IDC_MESG_22, m_csMsg22);
	DDX_Text(pDX, IDC_MESG_23, m_csMsg23);
	DDX_Text(pDX, IDC_ALIVE_AGENTS, m_csAliveAgents);
	DDX_Text(pDX, IDC_ALIVE_MSG_PER_HOUR, m_csAliveMessagesPerHour);
	DDX_Text(pDX, IDC_ALIVE_MSG_PER_MIN, m_csAliveMessagesPerMinute);
	//}}AFX_DATA_MAP
}

const UINT    wm_CollMsg = RegisterWindowMessage( COLLECTOR_MESSAGE_STRING );

BEGIN_MESSAGE_MAP(CCollector_StatsDlg, CDialog)
	//{{AFX_MSG_MAP(CCollector_StatsDlg)
	ON_WM_PAINT()
    ON_REGISTERED_MESSAGE(wm_CollMsg, OnCollectorStatisticsMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCollector_StatsDlg message handlers

BOOL CCollector_StatsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	SetWindowText(COLLECTOR_STATS_MESSAGE_WINDOW_NAME);

    m_bMapCreated = FALSE;

    if (CreateMapFile(COLLECTOR_MESSAGE_STRING,sizeof(mmp_TdmfCollectorState)))
    {
        m_pMemoryMappedFile = ReturnRW_MapedViewOfFile(g_hMMFile);
        if (m_pMemoryMappedFile)
            m_bMapCreated = TRUE;
    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCollector_StatsDlg::OnPaint() 
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

LRESULT CCollector_StatsDlg::OnCollectorStatisticsMessage(WPARAM wParam, LPARAM lParam)
{
    mmp_TdmfCollectorState *pStatisticsBuffer;
    char                    TempMessage[256];
 
    //
    // Validate message contents before accessing the 
    // buffer that was passed to us
    //
    if (COLLECTOR_MAGIC_NUMBER==wParam)
    {
        //
        // Get memory mapped file contents (if exists)
        //
        if (!m_pMemoryMappedFile)
        {
            return 0;
        }

        pStatisticsBuffer = (mmp_TdmfCollectorState*)m_pMemoryMappedFile;

        //
        // Copy values read from collector to our local values
        //
        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbThrdRng);
        m_csNumOfMessageThreadsPending = TempMessage;
        
        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbPDBMsg);
        m_csNumOfDatabaseMessagesPending = TempMessage;
        
       
        CTime CurrentCollectorTime(pStatisticsBuffer->CollectorTime);
        m_csCollectorTime = CurrentCollectorTime.Format("%H:%M:%S,%A, %B %d, %Y");
 
        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbPDBMsgPerHr);
        m_csNumMessagesPerHour = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbPDBMsgPerMn);
        m_csNumMessagesPerMinute = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbThrdRngHr);
        m_csNumThreadsPerHour = TempMessage;
        
        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbThrdRngPerMn);
        m_csNumThreadsPerMinute = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_SET_LG_CONFIG                   );
        m_csMsg0 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GET_LG_CONFIG                   );
        m_csMsg1 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO_REQUEST              );
        m_csMsg2 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO                      );
        m_csMsg3 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_REGISTRATION_KEY                );
        m_csMsg4 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_TDMF_CMD                        );
        m_csMsg5 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG            );
        m_csMsg6 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG            );
        m_csMsg7 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_SET_ALL_DEVICES                 );
        m_csMsg8 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GET_ALL_DEVICES                 );
        m_csMsg9 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_ALERT_DATA                      );
        m_csMsg10 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_STATUS_MSG                      );
        m_csMsg11 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_PERF_MSG                        );
        m_csMsg12 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_PERF_CFG_MSG                    );
        m_csMsg13 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION    );
        m_csMsg14 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_ALIVE_SOCKET              );
        m_csMsg15 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GROUP_STATE                     );
        m_csMsg16 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GROUP_MONITORING                );
        m_csMsg17 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION      );
        m_csMsg18 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_SET_DB_PARAMS                   );
        m_csMsg19 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GET_DB_PARAMS                   );
        m_csMsg20 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_STATE                     );
        m_csMsg21 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_MMP_MNGT_GUI_MSG                         );
        m_csMsg22 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->TdmfMessagesStates.Nb_default                                  );
        m_csMsg23 = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbAgentsAlive                                                  );
        m_csAliveAgents = TempMessage; 

        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbAliveMsgPerMn                                                );
        m_csAliveMessagesPerMinute = TempMessage;

        sprintf(TempMessage,"%ld",pStatisticsBuffer->NbAliveMsgPerHr                                                );
        m_csAliveMessagesPerHour = TempMessage;

        //
        // Update window
        //
        UpdateData(false);

    }
	return 0;
}


static bool CreateMapFile (LPCSTR cszFileName, DWORD size)
{
    if (g_hMMFile)
    {
        //
        // There is already a handle! Get out of here
        //
        return false;
    }

    //
    // Try to open file mapping
    //
    g_hMMFile = OpenFileMapping(FILE_MAP_WRITE, 
                                FALSE, 
                                cszFileName);

    //
    // Allocate at least 4K
    //
    if (size < 4096)
    {
        size = 4096;
    }

    //
    // If file mapping did not work, create it
    //
    if (!g_hMMFile)
    {
        g_hMMFile = CreateFileMapping ( (HANDLE)0xFFFFFFFF,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        size,
                                        cszFileName);
    }

    if (g_hMMFile)
    {
        return true;
    }

    return false;
}

static char * ReturnRW_MapedViewOfFile(HANDLE h_MMFile)
{
    char * p_MMFile;

    p_MMFile = (char *)MapViewOfFile (h_MMFile, 
                                      FILE_MAP_WRITE, 
                                      0, 
                                      0, 
                                      0);
    
    

    return p_MMFile;
}

static bool DestroyMapFile(HANDLE H_MMFile, char * p_MMFile)
{
    if (!H_MMFile)
    {
        return FALSE;
    }

    if (p_MMFile)
    {
        UnmapViewOfFile (p_MMFile);
    }

    CloseHandle(H_MMFile);

    return true;
}

static void FlushMapFile(char * p_MMFile, DWORD size)
{
    if (g_hMMFile)
    {
        FlushViewOfFile(p_MMFile,size);
    }
}

