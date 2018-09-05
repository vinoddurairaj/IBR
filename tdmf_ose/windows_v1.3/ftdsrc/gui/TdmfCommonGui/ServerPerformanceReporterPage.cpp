// ServerPerformanceReporterPage.cpp : implementation file
//

#include "stdafx.h"

#include "tdmfcommongui.h"
#include "ServerPerformanceReporterPage.h"
#include "ToolsView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceReporterPage property page

IMPLEMENT_DYNCREATE(CServerPerformanceReporterPage, CPropertyPage)

CServerPerformanceReporterPage::CServerPerformanceReporterPage() : 
	CPropertyPage(CServerPerformanceReporterPage::IDD)
{
	//{{AFX_DATA_INIT(CServerPerformanceReporterPage)
	//}}AFX_DATA_INIT
}

CServerPerformanceReporterPage::~CServerPerformanceReporterPage()
{
}

void CServerPerformanceReporterPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPerformanceReporterPage)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHART1, m_ChartFX); // Link variable to control 
	if (!pDX->m_bSaveAndValidate) // Link Chart FX pointer to control window 
	{
		m_pChartFX = m_ChartFX.GetControlUnknown(); 
	}
}


BEGIN_MESSAGE_MAP(CServerPerformanceReporterPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerPerformanceReporterPage)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceReporterPage message handlers

void CServerPerformanceReporterPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);	

	// Move (resize) control's wnd
	if (IsWindow(m_ChartFX.m_hWnd))
	{
		m_ChartFX.MoveWindow(0, 0, cx, cy);
	}
}

BOOL CServerPerformanceReporterPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Add Counter
	ICommandItemPtr pCmdItemAdd = m_pChartFX->GetCommands()->AddCommand(1);
	pCmdItemAdd->Text = "Counters Selection";
	pCmdItemAdd->Picture = 11;
		
	ICommandItemPtr pCmdItemClear = m_pChartFX->GetCommands()->AddCommand(2);
	pCmdItemClear->Text = "Clear Counters";
	pCmdItemClear->Picture = 1;
		
	m_pChartFX->GetToolBarObj()->AddItems(2,0);
	m_pChartFX->GetToolBarObj()->GetItem(0)->CommandID = 1;
	m_pChartFX->GetToolBarObj()->GetItem(1)->CommandID = 2;


	m_pChartFX->PutDataType(0, CDT_XVALUE);
	m_pChartFX->PutDataType(1, CDT_VALUE);


	//m_pChartFX->RealTimeStyle = (enum CfxRealTimeStyle)(CRT_LOOPPOS | CRT_NOWAITARROW);
	//m_pChartFX->MaxValues = 60;
	//m_pChartFX->TypeMask = (enum CfxType)(m_pChartFX->TypeMask | CT_EVENSPACING);

	//m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase = 10;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerPerformanceReporterPage::AddCounter(CReporterCounterInfo& CounterInfo)
{
	m_listCounterInfo.push_back(CounterInfo);

	GenerateReport();
}

BOOL CServerPerformanceReporterPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();

	return CPropertyPage::OnSetActive();
}

BOOL CServerPerformanceReporterPage::OnKillActive() 
{
	return CPropertyPage::OnKillActive();
}

class CRefreshParam
{
public:
	CRefreshParam() : m_pStreamChart(NULL), m_pStreamServer(NULL), m_pListCounterInfo(NULL) {}

public:
	IStream* m_pStreamChart;
	IStream* m_pStreamServer;
	IStream* m_pStreamXMLData;
	std::list<CReporterCounterInfo>* m_pListCounterInfo;
};

static UINT RefreshThread(LPVOID pParam)
{
	USES_CONVERSION;

	CoInitialize(NULL);

	CRefreshParam* pRefreshParam = (CRefreshParam*)pParam;

	// Retrieve the interface pointer within your thread routine
	IChartFXPtr pChartFX;
	TDMFOBJECTSLib::IServerPtr pServer;
	IXMLDataPtr pXMLData;
	if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pRefreshParam->m_pStreamChart, IID_IChartFX, (LPVOID*)&pChartFX)) && 
		SUCCEEDED(CoGetInterfaceAndReleaseStream(pRefreshParam->m_pStreamServer, TDMFOBJECTSLib::IID_IServer, (LPVOID*)&pServer)) &&
		SUCCEEDED(CoGetInterfaceAndReleaseStream(pRefreshParam->m_pStreamXMLData, IID_IXMLData, (LPVOID*)&pXMLData)))
	{
		CString cstrDataXML = "<?xml version=\"1.0\"?>\n"
							  "<DATA>\n"
							  "<COLUMNS>\n"
					          "<COLUMN NAME=\"Group\" TYPE=\"Integer\"/>\n"
							  "<COLUMN NAME=\"Stat\"  TYPE=\"Integer\"/>\n"
							  "</COLUMNS>\n";

		int i = 0;
		for (std::list<CReporterCounterInfo>::iterator it = pRefreshParam->m_pListCounterInfo->begin();
			 it != pRefreshParam->m_pListCounterInfo->end(); it++)
		{
			CString cstrRow;
			cstrRow.Format("<ROW Group=\"%d\" Stat=\"%d\"/>\n", it->m_nLgGroupId, it->m_ePerfData);
			cstrDataXML += cstrRow;

			pChartFX->PutDataType(i, CDT_XVALUE);
			pChartFX->PutDataType(i+1, CDT_VALUE);
			i += 2;
		}
		cstrDataXML += "</DATA>\n";

		// Get perf data from server
		COleDateTime DateEnd = COleDateTime::GetCurrentTime();
		COleDateTime DateBegin = DateEnd - COleDateTimeSpan(1, 0, 0, 0);
		cstrDataXML = OLE2A(pServer->GetPerformanceValues((LPCSTR)cstrDataXML, DateBegin, DateEnd));

        AfxMessageBox(cstrDataXML);

		// Display data
		pXMLData->LoadFromString((LPCSTR)cstrDataXML);
		pChartFX->GetExternalData((IUnknown*)pXMLData);
	}

	delete pRefreshParam;

	CoUninitialize();

	return 0;
}

void CServerPerformanceReporterPage::GenerateReport() 
{
	CRefreshParam* pParam = new CRefreshParam();
	pParam->m_pListCounterInfo = &m_listCounterInfo;

	IXMLDataPtr pXMLData;
	if (SUCCEEDED(pXMLData.CreateInstance(CLSID_ClientXMLData)))
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
		if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(IID_IChartFX, (LPUNKNOWN)m_pChartFX, &pParam->m_pStreamChart)) &&
			SUCCEEDED(CoMarshalInterThreadInterfaceInStream(TDMFOBJECTSLib::IID_IServer, (LPUNKNOWN)pDoc->GetSelectedServer(), &pParam->m_pStreamServer)) &&
			SUCCEEDED(CoMarshalInterThreadInterfaceInStream(IID_IXMLData, (LPUNKNOWN)pXMLData, &pParam->m_pStreamXMLData)))
		{
			AfxBeginThread(RefreshThread, pParam, 0, 0, 0, NULL);
		}
		else
		{
			AfxMessageBox("Unexpected Error: 995", MB_OK | MB_ICONINFORMATION);
			delete pParam;
		}
	}
}


BEGIN_EVENTSINK_MAP(CServerPerformanceReporterPage, CPropertyPage)
    //{{AFX_EVENTSINK_MAP(CServerPerformanceReporterPage)
	ON_EVENT(CServerPerformanceReporterPage, IDC_CHART1, 27 /* UserCommand */, OnUserCommandChart1, VTS_I4 VTS_I4 VTS_PI2)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CServerPerformanceReporterPage::OnUserCommandChart1(long wParam, long lParam, short FAR* nRes) 
{
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	switch (wParam)
	{
	case 1:
		{
			CReportConfigDlg ReportConfigDlg;
	 		ReportConfigDlg.m_pServer = pDoc->GetSelectedServer();
 			ReportConfigDlg.m_pServerPerformanceReporterPage = this;
			ReportConfigDlg.DoModal();
		}
		break;

	case 2:
		break;
	}
}
