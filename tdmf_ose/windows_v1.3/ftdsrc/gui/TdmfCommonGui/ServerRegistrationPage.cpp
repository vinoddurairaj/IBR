// ServerRegistrationPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ServerRegistrationPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerRegistrationPage property page

IMPLEMENT_DYNCREATE(CServerRegistrationPage, CPropertyPage)

CServerRegistrationPage::CServerRegistrationPage(TDMFOBJECTSLib::IServer *pServer)
	: CPropertyPage(CServerRegistrationPage::IDD), m_pServer(pServer)
{
	//{{AFX_DATA_INIT(CServerRegistrationPage)
	m_strRegKey = _T("");
	//}}AFX_DATA_INIT

	m_bPageModified = false;
}

CServerRegistrationPage::~CServerRegistrationPage()
{
}

void CServerRegistrationPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerRegistrationPage)
	DDX_Text(pDX, IDC_EDIT_KEY, m_strRegKey);
	DDV_MaxChars(pDX, m_strRegKey, 40);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerRegistrationPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerRegistrationPage)
	ON_EN_UPDATE(IDC_EDIT_KEY, OnUpdateEditKey)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerRegistrationPage message handlers

BOOL CServerRegistrationPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_bPageModified = false;

	if (m_pServer != NULL)
	{
		m_strRegKey = (BSTR)m_pServer->RegKey;
	}

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CServerRegistrationPage::OnApply() 
{
	if (m_pServer != NULL && m_bPageModified)
	{
		UpdateData();

		m_strRegKey.TrimLeft();
		m_strRegKey.TrimRight();
		m_pServer->RegKey = (LPCTSTR)m_strRegKey;

		m_bPageModified = false;
		SetModified(FALSE);
		SendMessage (DM_SETDEFID, IDOK);
	}
	
	return CPropertyPage::OnApply();
}

BOOL CServerRegistrationPage::OnKillActive() 
{
	UpdateData();

	// Validate
	//if (m_strName.IsEmpty())
	//{
	//	AfxMessageBox("You must enter a name.", MB_OK|MB_ICONINFORMATION);	
	//	m_EditName.SetFocus();
	//	m_EditName.SetSel(0, -1);
	//
	//	return FALSE;
	//}
	
	return CPropertyPage::OnKillActive();
}


void CServerRegistrationPage::OnUpdateEditKey() 
{
	SetModified();
	m_bPageModified = true;
}
