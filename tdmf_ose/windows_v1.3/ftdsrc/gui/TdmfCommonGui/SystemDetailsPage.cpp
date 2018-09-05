// SystemDetailsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "SystemDetailsPage.h"
#include "ToolsView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemDetailsPage property page

IMPLEMENT_DYNCREATE(CSystemDetailsPage, CPropertyPage)

CSystemDetailsPage::CSystemDetailsPage(TDMFOBJECTSLib::ISystem* pSystem)
	: CPropertyPage(CSystemDetailsPage::IDD), m_pSystem(pSystem)
{
	//{{AFX_DATA_INIT(CSystemDetailsPage)
	m_strName               = _T("N/A");
	m_dDBMsgPerHour         = 0;
	m_dDBMsgPerMin          = 0;
	m_dThrdPerHour          = 0;
	m_dThrdPerMin           = 0;
	m_dThrdPending          = 0;
	m_dDBMsgPending         = 0;
	m_CollectorTimestamp    = _T("N/A");
	m_Str_CollectorStatsTitle = _T("");
	m_cstrDatabase = _T("");
	m_cstrHostId = _T("");
	m_cstrIP = _T("");
	m_cstrPort = _T("");
	m_cstrVersion = _T("");
	m_cstrDatabaseTitle = _T("");
	m_cstrTitleCollector = _T("");
	//}}AFX_DATA_INIT
 
}

CSystemDetailsPage::~CSystemDetailsPage()
{
  
}

void CSystemDetailsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemDetailsPage)
	DDX_Control(pDX, IDC_RICHEDIT_TITLE_COLLECTOR, m_RichEditCollector);
	DDX_Control(pDX, IDC_RICHEDIT_TITLE_DATABASE, m_RichEditDatabase);
	DDX_Control(pDX, IDC_RICHEDIT_STATS_TITLE, m_StatsTitleCtrl);
	DDX_Control(pDX, IDC_RICHEDIT_NAME, m_RichEditName);
	DDX_Text(pDX, IDC_RICHEDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_MSG_PER_HOUR, m_dDBMsgPerHour);
	DDX_Text(pDX, IDC_MSG_PER_MIN, m_dDBMsgPerMin);
	DDX_Text(pDX, IDC_MSG_THRD_HOUR, m_dThrdPerHour);
	DDX_Text(pDX, IDC_MSG_THRD_MINUTE, m_dThrdPerMin);
	DDX_Text(pDX, IDC_MSG_THRD_PEND, m_dThrdPending);
	DDX_Text(pDX, IDC_DB_MSG_PEND, m_dDBMsgPending);
	DDX_Text(pDX, IDC_COLLECTOR_TIMESTAMP, m_CollectorTimestamp);
	DDX_Text(pDX, IDC_RICHEDIT_STATS_TITLE, m_Str_CollectorStatsTitle);
	DDX_Text(pDX, IDC_EDIT_DATABASE, m_cstrDatabase);
	DDX_Text(pDX, IDC_EDIT_HOSTID, m_cstrHostId);
	DDX_Text(pDX, IDC_EDIT_IP, m_cstrIP);
	DDX_Text(pDX, IDC_EDIT_PORT, m_cstrPort);
	DDX_Text(pDX, IDC_EDIT_VERSION, m_cstrVersion);
	DDX_Text(pDX, IDC_RICHEDIT_TITLE_DATABASE, m_cstrDatabaseTitle);
	DDX_Text(pDX, IDC_RICHEDIT_TITLE_COLLECTOR, m_cstrTitleCollector);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystemDetailsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSystemDetailsPage)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemDetailsPage message handlers

BOOL CSystemDetailsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	try
	{
		// Set Name format
		m_RichEditName.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));
		CHARFORMAT cf;
		cf.dwMask = CFM_BOLD | CFM_SIZE;
		m_RichEditName.GetDefaultCharFormat(cf);
		cf.dwEffects = CFE_BOLD;
		cf.yHeight = (LONG)(cf.yHeight * 1.5);
		m_RichEditName.SetDefaultCharFormat(cf);
	
        // Set Collector Stats Name format
		m_StatsTitleCtrl.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));
		cf.dwMask = CFM_BOLD | CFM_SIZE;
		m_StatsTitleCtrl.GetDefaultCharFormat(cf);
		cf.dwEffects = CFE_BOLD;
		cf.yHeight = (LONG)(cf.yHeight * 1.1);
		m_StatsTitleCtrl.SetDefaultCharFormat(cf);
        
        m_Str_CollectorStatsTitle = _T("Collector Statistics");

        // Set Collector Info format
		m_RichEditCollector.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));
		cf.dwMask = CFM_SIZE;
		m_RichEditCollector.GetDefaultCharFormat(cf);
		cf.yHeight = (LONG)(cf.yHeight * 1.1);
		m_RichEditCollector.SetDefaultCharFormat(cf);
        
		m_cstrTitleCollector = _T("Collector");

		// Set Database Title format
		m_RichEditDatabase.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));
		cf.dwMask = CFM_SIZE;
		m_RichEditDatabase.GetDefaultCharFormat(cf);
		cf.yHeight *= (LONG)(cf.yHeight * 1.1);
		m_RichEditDatabase.SetDefaultCharFormat(cf);
        
		m_cstrDatabaseTitle = _T("Database");

 		if (m_pSystem != NULL)
		{
			m_strName = (BSTR)m_pSystem->Name;

			CComBSTR bstrDatabase;
			CComBSTR bstrVersion;
			CComBSTR bstrIP;
			CComBSTR bstrPort;
			CComBSTR bstrHostId;
			m_pSystem->GetDescription(&bstrDatabase, &bstrVersion, &bstrIP, &bstrPort, &bstrHostId);
			
			m_cstrDatabase = bstrDatabase;
			m_cstrDatabase.Replace("\\TDMF", "");
			m_cstrVersion  = bstrVersion;
			m_cstrIP       = bstrIP;
			m_cstrPort     = bstrPort;
			m_cstrHostId   = bstrHostId;
			
			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1077);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSystemDetailsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	
	if (IsWindow(this->m_hWnd))
	{
		SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOMOVE);
	}
	
	this->RedrawWindow();
}

BOOL CSystemDetailsPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars(true);

	return CPropertyPage::OnSetActive();
}

BOOL CSystemDetailsPage::UpdateTheDataStatsFromTheCollector()
{
    bool bResult = false;
    //database message 

    TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats ;
    
    pICollectorStats = m_pSystem->GetCollectorStats();


    if(pICollectorStats != NULL)
    {

        m_dDBMsgPerHour  = pICollectorStats->GetDBMsgPerHour();
        m_dDBMsgPerMin   = pICollectorStats->GetDBMsgPerMin();
        m_dDBMsgPending  = pICollectorStats->GetDBMsgPending();

        //collector message
        m_dThrdPerHour    = pICollectorStats->GetThrdPerHour();
        m_dThrdPerMin     = pICollectorStats->GetThrdPerMin();
        m_dThrdPending    = pICollectorStats->GetThrdPending();

  
        CTime TimeStamp(pICollectorStats->GetTimeCollector());
        CString strTimeStamp;
        if(TimeStamp != 0)
        {
            strTimeStamp = TimeStamp.Format( "%A, %B %d, %Y %H:%M:%S" );
            m_CollectorTimestamp = strTimeStamp;
        }
        else
        {
            m_CollectorTimestamp = _T("N/A");
        }


        // Update window
        //
        if(m_hWnd != NULL)
            UpdateData(false);
    

        bResult = true;

    }

    return bResult;
}

int CSystemDetailsPage::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}


