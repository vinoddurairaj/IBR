// ChooseItemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ChooseItemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseItemDlg dialog


CChooseItemDlg::CChooseItemDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChooseItemDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChooseItemDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pArray		= NULL;
	m_nType     = 0;
	m_pstrResult = NULL;
}

CChooseItemDlg::CChooseItemDlg(CStringArray* p, int nType, CString* pstrResult,CWnd* pParent /*=NULL*/)
	: CDialog(CChooseItemDlg::IDD, pParent),m_pArray(p),m_nType(nType)
{
	m_pstrResult = pstrResult;
  
}


void CChooseItemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseItemDlg)
	DDX_Control(pDX, IDC_LIST_ITEM, m_List_Item);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChooseItemDlg, CDialog)
	//{{AFX_MSG_MAP(CChooseItemDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseItemDlg message handlers

BOOL CChooseItemDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	BOOL bResult = false;

	switch (m_nType)
	{	
		case TypeServer:
			SetWindowText(_T("Select the servers"));
			LoadTheListBox() ? bResult = true : bResult = false;
			break;
		case TypeLGroup:
			SetWindowText(_T("Select the logical groups"));
			LoadTheListBox() ? bResult = true : bResult = false;
			break;
		case TypeRPair:
			SetWindowText(_T("Select the replication pairs"));
			LoadTheListBox() ? bResult = true : bResult = false;
			break;
		default:
			break;
	}
		
	return bResult;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CChooseItemDlg::LoadTheListBox()
{

	BOOL bResult = false;

	m_List_Item.ResetContent();
	
  int nCount = m_pArray->GetSize();

  if (nCount > 0)
  {
		for (int i = 0; i < nCount; i++) 
		{
			m_List_Item.AddString(m_pArray->GetAt(i));
		
		} 

		bResult = true;
  }
  else
  {
		bResult = false;
  }

  return bResult;
}

void CChooseItemDlg::OnOK() 
{
	int nCount = m_List_Item.GetSelCount();
	CString strValue = _T("");
	if(nCount > 0)
	{
		m_aryIndex.SetSize(nCount);
		m_List_Item.GetSelItems(nCount,m_aryIndex.GetData()); 
	
		for (int i = 0; i < nCount; i++) 
		{
			m_List_Item.AddString(m_pArray->GetAt(i));
		   m_List_Item.GetText(i,strValue);
			strValue += _T(",");
			*m_pstrResult += strValue;
      
		} 
	
	}

	CDialog::OnOK();
}
