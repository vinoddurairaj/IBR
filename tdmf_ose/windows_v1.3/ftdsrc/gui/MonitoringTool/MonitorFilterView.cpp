// MonitorFilterView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "MonitorFilterView.h"
#include "ChooseItemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMonitorFilterView

IMPLEMENT_DYNCREATE(CMonitorFilterView, CFormView)

CMonitorFilterView::CMonitorFilterView()
	: CFormView(CMonitorFilterView::IDD)
{
	//{{AFX_DATA_INIT(CMonitorFilterView)
	//}}AFX_DATA_INIT
}

CMonitorFilterView::~CMonitorFilterView()
{
}

void CMonitorFilterView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMonitorFilterView)
	DDX_Control(pDX, IDC_EDIT_SERVER, m_Edit_ServerCtrl);
	DDX_Control(pDX, IDC_EDIT_PAIR, m_Edit_PairCtrl);
	DDX_Control(pDX, IDC_EDIT_LGROUP, m_Edit_LGroupCtrl);
	DDX_Control(pDX, IDC_APPLY, m_Btn_ApplyCtrl);
	DDX_Control(pDX, IDC_BTN_PICK_SERVER, m_Btn_PickServerCtrl);
	DDX_Control(pDX, IDC_BTN_PICK_LGROUP, m_Btn_PickLgroupCtrl);
	DDX_Control(pDX, IDC_BTN_PICK_PAIR, m_BTN_PickPairCtrl);
	DDX_Control(pDX, IDC_CBO_SORTBY, m_Cbo_SortbyCtrl);
	DDX_Control(pDX, IDC_CBO_SERVER, m_Cbo_ServerCtrl);
	DDX_Control(pDX, IDC_CBO_PICK_SERVER, m_Cbo_PickServerCtrl);
	DDX_Control(pDX, IDC_CBO_PICK_PAIR, m_Cbo_PickPairCtrl);
	DDX_Control(pDX, IDC_CBO_PICK_LGROUP, m_Cbo_PickLGroupCtrl);
	DDX_Control(pDX, IDC_CBO_PAIR, m_Cbo_PairCtrl);
	DDX_Control(pDX, IDC_CBO_LGROUP, m_Cbo_LGroupCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMonitorFilterView, CFormView)
	//{{AFX_MSG_MAP(CMonitorFilterView)
	ON_CBN_SELCHANGE(IDC_CBO_SERVER, OnSelchangeCboServer)
	ON_BN_CLICKED(IDC_BTN_PICK_SERVER, OnBtnPickServer)
	ON_CBN_SELCHANGE(IDC_CBO_LGROUP, OnSelchangeCboLgroup)
	ON_CBN_SELCHANGE(IDC_CBO_PAIR, OnSelchangeCboPair)
	ON_BN_CLICKED(IDC_BTN_PICK_LGROUP, OnBtnPickLgroup)
	ON_BN_CLICKED(IDC_BTN_PICK_PAIR, OnBtnPickPair)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMonitorFilterView diagnostics

#ifdef _DEBUG
void CMonitorFilterView::AssertValid() const
{
	CFormView::AssertValid();
}

void CMonitorFilterView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMonitorFilterView message handlers

void CMonitorFilterView::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	
	m_Cbo_SortbyCtrl.SetCurSel(0);
	m_Cbo_ServerCtrl.SetCurSel(0);
	m_Cbo_PairCtrl.SetCurSel(0);
	m_Cbo_LGroupCtrl.SetCurSel(0);
	m_Cbo_PickServerCtrl.ShowWindow(SW_HIDE);
	m_Cbo_PickPairCtrl.ShowWindow(SW_HIDE);
	m_Cbo_PickLGroupCtrl.ShowWindow(SW_HIDE);
	m_Edit_ServerCtrl.ShowWindow(SW_HIDE);
	m_Edit_PairCtrl.ShowWindow(SW_HIDE);
	m_Edit_LGroupCtrl.ShowWindow(SW_HIDE);
	m_Btn_ApplyCtrl.ShowWindow(SW_SHOW);
	m_Btn_PickServerCtrl.ShowWindow(SW_HIDE);
	m_Btn_PickLgroupCtrl.ShowWindow(SW_HIDE);
	m_BTN_PickPairCtrl.ShowWindow(SW_HIDE);
}

void CMonitorFilterView::OnSelchangeCboServer() 
{
	CString strValue;
   m_Cbo_ServerCtrl.GetWindowText(strValue);

	if(strValue.CompareNoCase(_T("Some server:")) == 0)
	{
		m_Cbo_PickServerCtrl.ShowWindow(SW_HIDE);
		m_Btn_PickServerCtrl.ShowWindow(SW_SHOW);
		m_Edit_ServerCtrl.ShowWindow(SW_SHOW);
	}
	else if( strValue.CompareNoCase(_T("The server:")) == 0)
	{
		m_Cbo_PickServerCtrl.ShowWindow(SW_SHOW);
		LoadTheServerPickCombo() ;
		m_Btn_PickServerCtrl.ShowWindow(SW_HIDE);
		m_Edit_ServerCtrl.ShowWindow(SW_HIDE);
	}
	else
	{
		m_Cbo_PickServerCtrl.ShowWindow(SW_HIDE);
      m_Btn_PickServerCtrl.ShowWindow(SW_HIDE);
		m_Edit_ServerCtrl.ShowWindow(SW_HIDE);
	}

	
}

void CMonitorFilterView::OnSelchangeCboLgroup() 
{
	CString strValue;
   m_Cbo_LGroupCtrl.GetWindowText(strValue);

	if(strValue.CompareNoCase(_T("some logical group:")) == 0)
	{
		m_Cbo_PickLGroupCtrl.ShowWindow(SW_HIDE);
		m_Btn_PickLgroupCtrl.ShowWindow(SW_SHOW);
		m_Edit_LGroupCtrl.ShowWindow(SW_SHOW);
	}
	else if( strValue.CompareNoCase(_T("the group:")) == 0)
	{
		m_Cbo_PickLGroupCtrl.ShowWindow(SW_SHOW);
		LoadTheLogicalGroupPickCombo();
		m_Btn_PickLgroupCtrl.ShowWindow(SW_HIDE);
		m_Edit_LGroupCtrl.ShowWindow(SW_HIDE);
	}
	else if( strValue.CompareNoCase(_T("none")) == 0)
	{
		m_Cbo_PickLGroupCtrl.ShowWindow(SW_HIDE);
		m_Btn_PickLgroupCtrl.ShowWindow(SW_HIDE);
		m_Edit_LGroupCtrl.ShowWindow(SW_HIDE);
	}
	else
	{
		m_Cbo_PickLGroupCtrl.ShowWindow(SW_HIDE);
      m_Btn_PickLgroupCtrl.ShowWindow(SW_HIDE);
		m_Edit_LGroupCtrl.ShowWindow(SW_HIDE);
	}
	
}

void CMonitorFilterView::OnSelchangeCboPair() 
{
	CString strValue;
   m_Cbo_PairCtrl.GetWindowText(strValue);

	if(strValue.CompareNoCase(_T("some replication pairs:")) == 0)
	{
		m_Cbo_PickPairCtrl.ShowWindow(SW_HIDE);
		m_BTN_PickPairCtrl.ShowWindow(SW_SHOW);
		m_Edit_PairCtrl.ShowWindow(SW_SHOW);
	}
	else if( strValue.CompareNoCase(_T("the pair:")) == 0)
	{
		m_Cbo_PickPairCtrl.ShowWindow(SW_SHOW);
		LoadTheReplicationPairPickCombo();
		m_BTN_PickPairCtrl.ShowWindow(SW_HIDE);
		m_Edit_PairCtrl.ShowWindow(SW_HIDE);
	}
	else if( strValue.CompareNoCase(_T("none")) == 0)
	{
		m_Cbo_PickPairCtrl.ShowWindow(SW_HIDE);
		m_BTN_PickPairCtrl.ShowWindow(SW_HIDE);
		m_Edit_PairCtrl.ShowWindow(SW_HIDE);
	}
	else
	{
		m_Cbo_PickPairCtrl.ShowWindow(SW_HIDE);
      m_BTN_PickPairCtrl.ShowWindow(SW_HIDE);
		m_Edit_PairCtrl.ShowWindow(SW_HIDE);
	}
	
}

void CMonitorFilterView::OnBtnPickServer() 
{
	CStringArray arServer;
	CString strValue;
	CString strResult;

	for (int i =1; i<=15;i++)
	{
		strValue.Format(_T("Server %d"),i);
	
		arServer.Add(strValue);

	}

	CChooseItemDlg  dlg(&arServer,TypeServer,&strResult);
	if(dlg.DoModal())
	{
		m_Edit_ServerCtrl.SetWindowText( strResult );
	}

}

void CMonitorFilterView::OnBtnPickLgroup() 
{
	CStringArray arGroup;
	CString strValue;
	CString strResult;

	for (int i =1; i<=15;i++)
	{
		strValue.Format(_T("Logical Group %d"),i);
	
		arGroup.Add(strValue);

	}

	CChooseItemDlg  dlg(&arGroup,TypeLGroup,&strResult);
	if(dlg.DoModal())
	{
		m_Edit_LGroupCtrl.SetWindowText( strResult );
	}
	
}

void CMonitorFilterView::OnBtnPickPair() 
{
	CStringArray arRPair;
	CString strValue;
	CString strResult;

	for (int i =1; i<=15;i++)
	{
		strValue.Format(_T("Replication pair %d"),i);
	
		arRPair.Add(strValue);

	}


	CChooseItemDlg  dlg(&arRPair,TypeRPair,&strResult);
	if(dlg.DoModal())
	{
		m_Edit_PairCtrl.SetWindowText( strResult );
	}


}
void CMonitorFilterView::LoadTheServerPickCombo() 
{

	CString strValue;

	for (int i =1; i<=15;i++)
	{
		strValue.Format(_T("Server %d"),i);
	
		m_Cbo_PickServerCtrl.AddString(strValue);

	}
	m_Cbo_PickServerCtrl.SetCurSel(0);
}

void CMonitorFilterView::LoadTheLogicalGroupPickCombo() 
{

	CString strValue;

	for (int i =1; i<=15;i++)
	{
		strValue.Format(_T("Logical Group %d"),i);
	
		m_Cbo_PickLGroupCtrl.AddString(strValue);

	}
	m_Cbo_PickLGroupCtrl.SetCurSel(0);
}

void CMonitorFilterView::LoadTheReplicationPairPickCombo() 
{

	CString strValue;

	for (int i =1; i<=15;i++)
	{
		strValue.Format(_T("Replication pair %d"),i);
	
		m_Cbo_PickPairCtrl.AddString(strValue);

	}
	m_Cbo_PickPairCtrl.SetCurSel(0);
}