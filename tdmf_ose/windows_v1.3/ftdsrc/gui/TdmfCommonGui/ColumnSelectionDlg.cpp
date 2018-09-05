// ColumnSelectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ColumnSelectionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColumnSelectionDlg dialog


CColumnSelectionDlg::CColumnSelectionDlg(std::vector<CListViewColumnDef>& vecColumnDef)
	: m_vecColumnDef(vecColumnDef), CDialog(CColumnSelectionDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CColumnSelectionDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CColumnSelectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CColumnSelectionDlg)
	DDX_Control(pDX, IDC_LIST1, m_ListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CColumnSelectionDlg, CDialog)
	//{{AFX_MSG_MAP(CColumnSelectionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColumnSelectionDlg message handlers

BOOL CColumnSelectionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Gain a reference to the list control itself
	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	// Insert columns.
	m_ListCtrl.InsertColumn(0, _T("Column"));
	m_ListCtrl.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);


	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
	{
		int nIndex = m_ListCtrl.InsertItem(99999/*MAX_INT*/, it->m_strName.c_str());
		if (it->m_bVisible)
		{
			m_ListCtrl.SetCheck(nIndex);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CColumnSelectionDlg::OnOK() 
{
	for(int nIndex = 0; nIndex < m_ListCtrl.GetItemCount(); nIndex++)
	{
		m_vecColumnDef[nIndex].m_bVisible = m_ListCtrl.GetCheck(nIndex);
	}

	CDialog::OnOK();
}
