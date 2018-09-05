// ReportConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ReportConfigDlg.h"
#include "ServerPerformanceReporterPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReportConfigDlg dialog


CReportConfigDlg::CReportConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReportConfigDlg::IDD, pParent), m_pServerPerformanceReporterPage(NULL)
{
	//{{AFX_DATA_INIT(CReportConfigDlg)
	//}}AFX_DATA_INIT
}


void CReportConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReportConfigDlg)
	DDX_Control(pDX, ID_BUTTON_ADD, m_ButtonAdd);
	DDX_Control(pDX, IDC_LIST_STATS, m_ListStats);
	DDX_Control(pDX, IDC_LIST_GROUPS, m_ListGroup);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReportConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CReportConfigDlg)
	ON_LBN_SELCHANGE(IDC_LIST_GROUPS, OnSelchangeListGroups)
	ON_LBN_SELCHANGE(IDC_LIST_STATS, OnSelchangeListStats)
	ON_BN_CLICKED(ID_BUTTON_ADD, OnButtonAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportConfigDlg message handlers

BOOL CReportConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ButtonAdd.EnableWindow(FALSE);

	if (m_pServer != NULL)
	{
		for (int nIndex = 0; nIndex < m_pServer->ReplicationGroupCount; nIndex++)
		{
			TDMFOBJECTSLib::IReplicationGroupPtr pGroup = m_pServer->GetReplicationGroup(nIndex);
			if (pGroup->IsSource)
			{
				int nItem = m_ListGroup.AddString(pGroup->Name);
				m_ListGroup.SetItemData(nItem, pGroup->GroupNumber);
			}
		}
	}

	int nItemStat; 
	nItemStat = m_ListStats.AddString("% BAB Full");
	m_ListStats.SetItemData(nItemStat, CReporterCounterInfo::ePctBABFull);
	nItemStat = m_ListStats.AddString("% Done");
	m_ListStats.SetItemData(nItemStat, CReporterCounterInfo::ePctDone);
	nItemStat = m_ListStats.AddString("Entries in BAB");
	m_ListStats.SetItemData(nItemStat, CReporterCounterInfo::eBABEntries);
	nItemStat = m_ListStats.AddString("Read Bytes");
	m_ListStats.SetItemData(nItemStat, CReporterCounterInfo::eReadBytes);
	nItemStat = m_ListStats.AddString("Write Bytes");
	m_ListStats.SetItemData(nItemStat, CReporterCounterInfo::eWriteBytes);
    nItemStat = m_ListStats.AddString("Actual Net");
	m_ListStats.SetItemData(nItemStat, CReporterCounterInfo::eActualNet);
    nItemStat = m_ListStats.AddString("Effective Net");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CReportConfigDlg::OnSelchangeListGroups() 
{
	if (m_ListStats.GetCurSel() != LB_ERR)
	{
		m_ButtonAdd.EnableWindow();
	}
}

void CReportConfigDlg::OnSelchangeListStats() 
{
	if (m_ListGroup.GetCurSel() != LB_ERR)
	{
		m_ButtonAdd.EnableWindow();
	}	
}

void CReportConfigDlg::OnButtonAdd() 
{
	if (m_pServerPerformanceReporterPage != NULL)
	{
		CReporterCounterInfo CounterInfo;

    	CounterInfo.m_nLgGroupId = m_ListGroup.GetItemData(m_ListGroup.GetCurSel());
		CounterInfo.m_ePerfData  = (enum CReporterCounterInfo::Stats)m_ListStats.GetItemData(m_ListStats.GetCurSel());

    	m_pServerPerformanceReporterPage->AddCounter(CounterInfo);
	}
}
