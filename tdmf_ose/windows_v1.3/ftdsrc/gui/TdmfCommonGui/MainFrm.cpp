// MainFrm.cpp : implementation of the CMainFrame class
//


// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "TdmfCommonGui.h"
#include "TdmfCommonGuiDoc.h"
#include "MainFrm.h"
#include "SystemView.h"
#include "ToolsView.h"
#include "ServerView.h"
#include "ReplicationGroupView.h"
#include "ViewNotification.h"
#include "Splash.h"
#include "SelectServerNameDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAGIC_NUMBER (0xFADEFACE)


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_COMMAND(ID_DEBUG_MONITOR, OnDebugMonitor)
	ON_COMMAND(ID_MESSENGER,     OnMessenger)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_PANE_0, OnUpdateStatusBarPane0)
	//ON_UPDATE_COMMAND_UI(ID_PANE_1, OnUpdateStatusBarPane1)
	//ON_UPDATE_COMMAND_UI(ID_PANE_2, OnUpdateStatusBarPane2)

	ON_MESSAGE(WM_REPLICATION_GROUP_STATE_CHANGE, OnReplicationGroupStateChangeMsg)
	ON_MESSAGE(WM_SERVER_STATE_CHANGE, OnServerChangeMsg)
	ON_MESSAGE(WM_SERVER_CONNECTION_CHANGE, OnServerConnectionChangeMsg)
	ON_MESSAGE(WM_REPLICATION_GROUP_PERF_CHANGE, OnReplicationGroupPerfChangeMsg)
	ON_MESSAGE(WM_SERVER_PERF_CHANGE, OnServerPerfChangeMsg)
	ON_MESSAGE(WM_COLLECTOR_COMMUNICATION_STATUS, OnCollectorCommunicationStatusMsg)

	ON_MESSAGE(WM_DOMAIN_ADD,    OnDomainAddMsg)
	ON_MESSAGE(WM_DOMAIN_REMOVE, OnDomainRemoveMsg)
	ON_MESSAGE(WM_DOMAIN_MODIFY, OnDomainModifyMsg)

	ON_MESSAGE(WM_SERVER_ADD,    OnServerAddMsg)
	ON_MESSAGE(WM_SERVER_REMOVE, OnServerRemoveMsg)
	ON_MESSAGE(WM_SERVER_MODIFY, OnServerModifyMsg)
	ON_MESSAGE(WM_SERVER_BAB_NOT_OPTIMAL, OnServerBabNotOptimalMsg)

	ON_MESSAGE(WM_REPLICATION_GROUP_ADD,    OnReplicationGroupAddMsg)
	ON_MESSAGE(WM_REPLICATION_GROUP_REMOVE, OnReplicationGroupRemoveMsg)
	ON_MESSAGE(WM_REPLICATION_GROUP_MODIFY, OnReplicationGroupModifyMsg)
    ON_MESSAGE(WM_RECEIVED_STATISTICS_DATA, OnReceivedDataCollectorStatsMsg)
    ON_MESSAGE(WM_IPADRESS_UNKNOWN, OnIPAdressUnknown)

	ON_MESSAGE(WM_DEBUG_TRACE, OnDebugTraceMsg)

	ON_MESSAGE(WM_TEXT_MESSAGE, OnTextMessageMsg)
END_MESSAGE_MAP()

static UINT indicators[] =
{	
	ID_SEPARATOR,
	ID_PANE_0,
//	ID_PANE_1,
//	ID_PANE_2,
//	ID_PANE_3,
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame(): m_bInitialized(false)
{
	// Load view configuration
	LoadConfig();
	m_strPane0 = _T("N/A");
	m_strPane1 = _T("N/A");
	m_strPane2 = _T("N/A");

}

CMainFrame::~CMainFrame()
{
	// Save view configuration
	SaveConfig();

	// Cleanup
	for (std::map<std::string, std::ostrstream*>::iterator it = m_mapStream.begin();
		 it != m_mapStream.end(); it++)
	{
		it->second->freeze(0);
		delete it->second;
	}
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (g_bStatusbar)
	{
		if (!m_wndStatusBar.Create(this) ||
			!m_wndStatusBar.SetIndicators(indicators,
			sizeof(indicators)/sizeof(UINT)))
		{
			TRACE0("Failed to create status bar\n");
			return -1;      // fail to create
		}
	}

    m_bInitialized = true;

	CMenu* pMenu = GetMenu();
	if (pMenu != NULL && pMenu->GetMenuItemCount() > 2)
	{
		CMenu* pMenuSub = pMenu->GetSubMenu(2);
		if (pMenuSub != NULL)
		{
			CString cstrMenu = "About ";
			cstrMenu += theApp.m_ResourceManager.GetProductName();
			cstrMenu += "...";
			pMenuSub->ModifyMenu(ID_APP_ABOUT, MF_BYCOMMAND | MF_STRING, ID_APP_ABOUT, cstrMenu);
		}
	}

   	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// Use the specific class name established earlier.
	cs.lpszClass = _T("TdmfClass");      

	m_nWidth  = 880;
	m_nHeight = 660;
	m_nPosX   = 150;
	m_nPosY   = 100;

	m_nPaneLeftWidth = 200;
	m_nPaneRightTopHeight = 160;
	m_nPaneRightMiddleHeight = 160;

	if (m_mapStream.find("MainFrm") != m_mapStream.end())
	{
		std::ostrstream* poss = m_mapStream["MainFrm"];
		std::istrstream iss(m_mapStream["MainFrm"]->str(), m_mapStream["MainFrm"]->pcount());

		iss.read((char*)&m_nPosX,   sizeof(long));
		iss.read((char*)&m_nPosY,   sizeof(long));
		iss.read((char*)&m_nWidth,  sizeof(int));
		iss.read((char*)&m_nHeight, sizeof(int));
		iss.read((char*)&m_nPaneLeftWidth, sizeof(long));
		iss.read((char*)&m_nPaneRightTopHeight, sizeof(long));
		iss.read((char*)&m_nPaneRightMiddleHeight, sizeof(long));
	}

#ifndef TDMF_IN_A_DLL
	cs.cx = m_nWidth;
	cs.cy = m_nHeight;
	cs.x  = m_nPosX;
	cs.y  = m_nPosY;
#endif

	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
 
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CSplashWnd::ShowSplashScreen(this);

   	if (!CFrameWnd::OnCreateClient(lpcs, pContext))
		return FALSE;

	if(!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if(!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CSystemView), CSize(m_nPaneLeftWidth, 0), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}
	
	if(!m_wndSplitter2.CreateStatic( &m_wndSplitter, 3, 1, WS_CHILD | WS_VISIBLE, m_wndSplitter.IdFromRowCol( 0, 1 )))    // 1 row, 1 col
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}

	if(!m_wndSplitter2.CreateView(0, 0, RUNTIME_CLASS(CServerView), CSize(0, m_nPaneRightTopHeight), pContext) ||
	   !m_wndSplitter2.CreateView(1, 0, RUNTIME_CLASS(CReplicationGroupView), CSize(0, m_nPaneRightMiddleHeight), pContext) ||
	   !m_wndSplitter2.CreateView(2, 0, RUNTIME_CLASS(CToolsView), CSize(0, 0), pContext))
	{
		m_wndSplitter.DestroyWindow();
		m_wndSplitter2.DestroyWindow();
		return FALSE;
	}

	if (g_bRedBar)
	{
#ifndef TDMF_IN_A_DLL
		// DialogBar
		if (!m_wndDlgBar.Create(this, IDD_SOFTEK, CBRS_TOP, IDD_SOFTEK))
		{
			return FALSE;
		}
#endif
	}

	// Lock timer
	SetTimer(1, 60000, NULL);

	// Debug Monitor
	m_DebugMonitor.Create(IDD_DEBUG_MONITOR, GetDesktopWindow());

#ifdef TEST_MESSAGES
	// Messenger Window
	m_Messenger.Create(IDD_MESSENGER, GetDesktopWindow());
#endif

	return TRUE;
}

LRESULT CMainFrame::OnReplicationGroupStateChangeMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	long *rglValue = (long*)lParam;

	VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_CHANGE;
	VN.m_eParam = CViewNotification::STATE;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;      // Domain's IdKa
	VN.m_dwParam2 = rglValue[0]; // SrvId
	VN.m_dwParam3 = rglValue[1]; // Rep Group Number

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	// Cleanup
	GlobalFree((HGLOBAL)rglValue);

	return 0;
}

LRESULT CMainFrame::OnServerChangeMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_CHANGE;
	VN.m_eParam = CViewNotification::STATE;
	VN.m_dwParam1 = wParam; // Domain's Ik
	VN.m_dwParam2 = lParam; // Key

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnServerConnectionChangeMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_CHANGE;
	VN.m_eParam = CViewNotification::CONNECTION;
	VN.m_dwParam1 = wParam; // Domain's Ik
	VN.m_dwParam2 = lParam; // Key

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnReplicationGroupPerfChangeMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	long *rglValue = (long*)lParam;
	
	VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_CHANGE;
	VN.m_eParam = CViewNotification::PERFORMANCE;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;      // Domain's IdKa
	VN.m_dwParam2 = rglValue[0]; // SrvId
	VN.m_dwParam3 = rglValue[1]; // Rep Group Number

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	// Cleanup
	GlobalFree((HGLOBAL)rglValue);

	return 0;
}

LRESULT CMainFrame::OnServerPerfChangeMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_CHANGE;
	VN.m_eParam = CViewNotification::PERFORMANCE;
	VN.m_dwParam1 = wParam; // Domain's Ik
	VN.m_dwParam2 = lParam; // Key

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnCollectorCommunicationStatusMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;

	switch (wParam)
	{
	case 0:
		((CTdmfCommonGuiDoc*)GetActiveDocument())->SetConnectedFlag(TRUE);
		VN.m_nMessageId = CViewNotification::SYSTEM_CHANGE;
		GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
		break;

	case 1:
		((CTdmfCommonGuiDoc*)GetActiveDocument())->SetConnectedFlag(FALSE);
		VN.m_nMessageId = CViewNotification::SYSTEM_CHANGE;
		GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
		break;

	case 2:
		MessageBox("A communication error occurred.", "Error", MB_OK | MB_ICONINFORMATION);
		break;
	}

	return 0;
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();
	if (pDoc && (pDoc->GetConnectedFlag() == TRUE))
	{
		pDoc->m_pSystem->raw_RequestOwnership(TRUE);
	}

	CFrameWnd::OnTimer(nIDEvent);
}

LRESULT CMainFrame::OnDomainAddMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::DOMAIN_ADD;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnDomainRemoveMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::DOMAIN_REMOVE;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnDomainModifyMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::DOMAIN_CHANGE;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnServerAddMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_ADD;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;
	VN.m_dwParam2 = lParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnServerRemoveMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_REMOVE;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;
	VN.m_dwParam2 = lParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnServerModifyMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_CHANGE;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;
	VN.m_dwParam2 = lParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

LRESULT CMainFrame::OnServerBabNotOptimalMsg(WPARAM wParam, LPARAM lParam)
{
	CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::SERVER_BAB_NOT_OPTIMAL;
	VN.m_pUnk = NULL;
	VN.m_dwParam1 = wParam;
	VN.m_dwParam2 = lParam;

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

	return 0;
}

// Find Group
TDMFOBJECTSLib::IReplicationGroupPtr CMainFrame::FindReplicationGroup(int nDomainKey, int nSrvId, int nGrpNumber, BOOL bIsSource)
{
	TDMFOBJECTSLib::IReplicationGroupPtr pRGNew; 

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();
	long nDomainCount = pDoc->m_pSystem->DomainCount;
	for (long nIndexDomain = 0; nIndexDomain < nDomainCount; nIndexDomain++)
	{
		TDMFOBJECTSLib::IDomainPtr pDomain = pDoc->m_pSystem->GetDomain(nIndexDomain);
		if (pDomain->GetKey() == nDomainKey)
		{
			long nServerCount = pDomain->ServerCount;
			for (long nIndexServer = 0; nIndexServer < nServerCount; nIndexServer++)
			{
				TDMFOBJECTSLib::IServerPtr pServer = pDomain->GetServer(nIndexServer);
				if (pServer->GetKey() == nSrvId)
				{
					long nRGCount = pServer->ReplicationGroupCount;
					for (long nIndexRG = 0; nIndexRG < nRGCount; nIndexRG++)
					{
						TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(nIndexRG);
						if ((pRG->GroupNumber == nGrpNumber) && (pRG->IsSource == bIsSource))
						{
							pRGNew = pRG;
							break;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return pRGNew;
}


LRESULT CMainFrame::OnReplicationGroupAddMsg(WPARAM wParam, LPARAM lParam)
{
	TDMFOBJECTSLib::IReplicationGroupPtr pRGNew; 
	long *rglValue = (long*)lParam;

	int nDomainKey = wParam;
	int iSrvId     = rglValue[0];
	int nGrpNumber = rglValue[1];
	BOOL bIsSource = rglValue[2];

	pRGNew = FindReplicationGroup(nDomainKey, iSrvId, nGrpNumber, bIsSource);

	if (pRGNew != NULL)
	{
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_ADD;
		VN.m_pUnk = pRGNew;

		GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
	}

	// Cleanup
	GlobalFree((HGLOBAL)rglValue);

	return 0;
}

LRESULT CMainFrame::OnReplicationGroupRemoveMsg(WPARAM wParam, LPARAM lParam)
{
	long *rglValue = (long*)lParam;

	int nDomainKey = wParam;
	int iSrvId     = rglValue[0];
	int nGrpNumber = rglValue[1];
	BOOL bIsSource = rglValue[2];

	TDMFOBJECTSLib::IReplicationGroupPtr pRGRemoved = FindReplicationGroup(nDomainKey, iSrvId, nGrpNumber, bIsSource);

	if (pRGRemoved != NULL)
	{
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_REMOVE;
		VN.m_pUnk = pRGRemoved;

		GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
	}

	// Cleanup
	GlobalFree((HGLOBAL)rglValue);

	return 0;
}


LRESULT CMainFrame::OnIPAdressUnknown(WPARAM wParam, LPARAM lParam)
{
	int nServerId = 0;

	std::string* pstrTargetHostName = (std::string*)lParam;

    CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();
    if (pDoc)
	{
		long nDomainCount = pDoc->m_pSystem->DomainCount;
		for (long nIndexDomain = 0; nIndexDomain < nDomainCount; nIndexDomain++)
		{
			TDMFOBJECTSLib::IDomainPtr pDomain = pDoc->m_pSystem->GetDomain(nIndexDomain);
			if (pDomain->GetKey() == (long)wParam)
			{
 				CSelectServerNameDlg SelectServerNameDlg(pDomain, pDoc, pstrTargetHostName);
				if (SelectServerNameDlg.DoModal() == IDOK)
				{
					nServerId = SelectServerNameDlg.m_nServerId;
				}
				break;
			}
		}
	}

   	return nServerId;
}


LRESULT CMainFrame::OnReceivedDataCollectorStatsMsg(WPARAM wParam, LPARAM lParam)
{

    CViewNotification VN;
	
	VN.m_nMessageId = CViewNotification::RECEIVED_COLLECTOR_STATISTICS_DATA;
	VN.m_dwParam1 = wParam; // Domain's Ik
	VN.m_dwParam2 = lParam; // Key

	GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
  

    //for debugWindow
    CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();
    if(pDoc)
    {
        TDMFOBJECTSLib::ISystem* pSystem = pDoc->m_pSystem;
        TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats ;
    
        pICollectorStats = pSystem->GetCollectorStats();


        if(pICollectorStats != NULL)
        {

			m_DebugMonitor.UpdateTheMsgFromCollectorStats(pICollectorStats);
		   
			CTime TimeStamp(pICollectorStats->GetTimeCollector());
		 
			if(TimeStamp != 0)
			{
			    m_strPane0 = TimeStamp.Format( "%H:%M" );
			}
			else
			{
				m_strPane0 = _T("N/A");
			}
      
            //m_strPane1.Format(_T("%d"), pDoc->m_pSystem->GetUserCount());

            m_strPane2.Format(_T("%d"), pICollectorStats->GetDBMsgPending());
        }
    }
    
	return 0;

}

LRESULT CMainFrame::OnReplicationGroupModifyMsg(WPARAM wParam, LPARAM lParam)
{
	long *rglValue = (long*)lParam;

	int nDomainKey = wParam;
	int iSrvId     = rglValue[0];
	int nGrpNumber = rglValue[1];
	BOOL bIsSource = rglValue[2];

	TDMFOBJECTSLib::IReplicationGroupPtr pRGModified = FindReplicationGroup(nDomainKey, iSrvId, nGrpNumber, bIsSource);

	if (pRGModified != NULL)
	{
		CViewNotification VN;	
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_CHANGE;
		VN.m_eParam = CViewNotification::PROPERTIES;
		VN.m_pUnk = pRGModified;

		GetActiveDocument()->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
	}

	// Cleanup
	GlobalFree((HGLOBAL)rglValue);

	return 0;
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC) 
{
	return CFrameWnd::OnEraseBkgnd(pDC);
}

void CMainFrame::OnDestroy() 
{
	CFrameWnd::OnDestroy();
	
	// Save configuration
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();

	if (m_mapStream.find("MainFrm") != m_mapStream.end())
	{
		m_mapStream["MainFrm"]->freeze(0);
		delete m_mapStream["MainFrm"];
	}

	std::ostrstream* poss = new std::ostrstream;
	m_mapStream["MainFrm"] = poss;

#ifdef TDMF_IN_A_DLL
	m_mapStream["MainFrm"]->write((char*)&(m_nPosX),   sizeof(long));
	m_mapStream["MainFrm"]->write((char*)&(m_nPosY),   sizeof(long));
	m_mapStream["MainFrm"]->write((char*)&(m_nWidth),  sizeof(int));
	m_mapStream["MainFrm"]->write((char*)&(m_nHeight), sizeof(int));
#else
	CRect Rect;
	GetWindowRect(&Rect);
	int nWidth = Rect.Width();
	int nHeight = Rect.Height();
	m_mapStream["MainFrm"]->write((char*)&(Rect.left), sizeof(long));
	m_mapStream["MainFrm"]->write((char*)&(Rect.top),  sizeof(long));
	m_mapStream["MainFrm"]->write((char*)&(nWidth),    sizeof(int));
	m_mapStream["MainFrm"]->write((char*)&(nHeight),   sizeof(int));
#endif

	CWnd* pWndPaneLeft        = m_wndSplitter.GetPane(0, 0);
	CWnd* pWndPaneRightTop    = m_wndSplitter2.GetPane(0, 0);
	CWnd* pWndPaneRightMiddle = m_wndSplitter2.GetPane(1, 0);

	CRect RectPaneLeft;
	CRect RectPaneRightTop;
	CRect RectPaneRightMiddle;

	pWndPaneLeft->GetWindowRect(&RectPaneLeft);
	RectPaneLeft.OffsetRect(-RectPaneLeft.left, -RectPaneLeft.top);

	pWndPaneRightTop->GetWindowRect(&RectPaneRightTop);
	RectPaneRightTop.OffsetRect(-RectPaneRightTop.left, -RectPaneRightTop.top);

	pWndPaneRightMiddle->GetWindowRect(&RectPaneRightMiddle);
	RectPaneRightMiddle.OffsetRect(-RectPaneRightMiddle.left, -RectPaneRightMiddle.top);

	m_mapStream["MainFrm"]->write((char*)&(RectPaneLeft.right), sizeof(long));
	m_mapStream["MainFrm"]->write((char*)&(RectPaneRightTop.bottom), sizeof(long));
	m_mapStream["MainFrm"]->write((char*)&(RectPaneRightMiddle.bottom), sizeof(long));
}

// Mike Pollett
#include "../../tdmf.inc"
// Load/Save Config
void CMainFrame::LoadConfig()
{
	#define MENU_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion\\Views\\"
	
	HKEY hKey;
	long nErr;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, MENU_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		const UINT nBufSizeMax = 1024;
		TCHAR achId[nBufSizeMax];
		DWORD dwBufLen = nBufSizeMax;
		if ((nErr = RegQueryValueEx(hKey, TEXT("Config"), NULL, NULL, (LPBYTE)achId, &dwBufLen)) == ERROR_SUCCESS)
		{
			std::istrstream iss(achId, dwBufLen);

			int nMagicNumber;
			iss.read((char*)&nMagicNumber, sizeof(nMagicNumber));

			if (nMagicNumber == MAGIC_NUMBER)
			{
				bool bEnd = false;

				while (bEnd == false)
				{
					int nStrSize;
					int nSize;

					iss.read((char*)&nStrSize, sizeof(nStrSize));
					if ((nStrSize < nBufSizeMax) && (iss.eof() == 0))
					{
						char* pszName = new char[nStrSize + 1];
						iss.read(pszName, nStrSize);
						pszName[nStrSize] = '\0';
				
						iss.read((char*)&nSize, sizeof(nSize));
						if (nSize < nBufSizeMax)
						{
							char* pBuf = new char[nSize];
							iss.read(pBuf, nSize);

							std::ostrstream* poss = new std::ostrstream;
							poss->write(pBuf, nSize);
							m_mapStream[pszName] = poss;

							delete[] pBuf;
						}
						else // There's an error in the stored stream
						{
							bEnd = true;
						}
						delete[] pszName;
					}
					else
					{
						bEnd = true;
					}
				}
			}
		}
		else if (nErr != ERROR_FILE_NOT_FOUND)
		{
			CString str;
			str.Format("Unable to load view configuration from registry.\nError = %d", nErr);
			MessageBox(str, "Registry Reading Error", MB_OK | MB_ICONINFORMATION);
		}

		RegCloseKey(hKey);
	}
}

void CMainFrame::SaveConfig()
{
	std::ostrstream oss;

	int nMagicNumber = MAGIC_NUMBER;
	oss.write((char*)&(nMagicNumber), sizeof(nMagicNumber));

	for (std::map<std::string, std::ostrstream*>::iterator it = m_mapStream.begin();
		 it != m_mapStream.end(); it++)
	{
		int nSize = it->first.size();
		oss.write((char*)&(nSize), sizeof(nSize));
		oss << it->first;

		nSize = it->second->pcount();
		oss.write((char*)&(nSize), sizeof(nSize));
		oss.write(it->second->str(), nSize);
	}

	//
	#define MENU_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion\\Views\\"
	
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
					   MENU_KEY,
					   NULL,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey,
					   NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey,
					  "Config",
					  0,
					  REG_BINARY,
					  (LPBYTE) oss.str(),
					  oss.pcount());

		RegFlushKey(hKey);

		RegCloseKey(hKey);
	}

	oss.freeze(0);
}


// Debug Monitor

void CMainFrame::OnDebugMonitor() 
{

	DWORD dwType;
	DWORD  Value;
	ULONG nSize = sizeof(Value);

	#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
	if (SHGetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "DebugMonitor", &dwType, &Value, &nSize) == ERROR_SUCCESS)
	{
        CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();
        m_DebugMonitor.SetDocument(pDoc);
        m_DebugMonitor.ShowWindow(SW_SHOWNORMAL);
	}
   else
    {
       m_DebugMonitor.ShowWindow(SW_HIDE);
    }
}

LRESULT CMainFrame::OnDebugTraceMsg(WPARAM wParam, LPARAM lParam)
{
	USES_CONVERSION;

	CComBSTR bstrMsg;
	bstrMsg.Attach((BSTR)wParam);

	CString cstrBuf = OLE2A(bstrMsg);
	
	CString rgcstrMsg[3];
	
	int nSeparator1 = cstrBuf.Find('|');
	rgcstrMsg[0] = cstrBuf.Mid(0, nSeparator1);
	nSeparator1++;

	int nSeparator2 = cstrBuf.Find('|', nSeparator1);
	rgcstrMsg[1] = cstrBuf.Mid(nSeparator1, nSeparator2 - nSeparator1);
	nSeparator2++;

	rgcstrMsg[2] = cstrBuf.Right(cstrBuf.GetLength() - nSeparator2);

	m_DebugMonitor.AddTrace(rgcstrMsg);

	return 0;
}


// Messenger

void CMainFrame::OnMessenger() 
{
#ifdef TEST_MESSAGES
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetActiveDocument();
	m_Messenger.SetDocument(pDoc);

	m_Messenger.ShowWindow(SW_SHOWNORMAL);
#endif
}

LRESULT CMainFrame::OnTextMessageMsg(WPARAM wParam, LPARAM lParam)
{
#ifdef TEST_MESSAGES
	USES_CONVERSION;

	UINT  nId    = wParam;
	char* pszMsg = (char*)lParam;

	m_Messenger.AddMessage(nId, pszMsg);
#endif

	return 0;
}

void CMainFrame::OnClose() 
{
#ifndef TDMF_IN_A_DLL
	ShowWindow(SW_SHOWNOACTIVATE);
#endif

	CFrameWnd::OnClose();
}


void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}

void CMainFrame::OnUpdateStatusBarPane0(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(); 
    CString str;
    str.Format( " Collector Clock: %s", m_strPane0 );
	pCmdUI->SetText( str );
}

void CMainFrame::OnUpdateStatusBarPane1(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(); 
    CString str;
    str.Format( "Nbr of users: %s", m_strPane1 ); 
    pCmdUI->SetText( str ); 
}

void CMainFrame::OnUpdateStatusBarPane2(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(); 
    CString str;
    str.Format( "Database Pending: %s", m_strPane2 ); 
    pCmdUI->SetText( str ); 
}




