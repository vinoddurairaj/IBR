// SystemGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "SystemGeneralPage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSystemGeneralPage property page

IMPLEMENT_DYNCREATE(CSystemGeneralPage, CPropertyPage)

CSystemGeneralPage::CSystemGeneralPage(CTdmfCommonGuiDoc* pDoc) : 
	CPropertyPage(CSystemGeneralPage::IDD), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(CSystemGeneralPage)
	m_cstrRegKey = _T("");
	//}}AFX_DATA_INIT
}

CSystemGeneralPage::~CSystemGeneralPage()
{
}

void CSystemGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemGeneralPage)
	DDX_Control(pDX, IDC_EDIT_REG_KEY, m_EditRegKey);
	DDX_Text(pDX, IDC_EDIT_REG_KEY, m_cstrRegKey);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystemGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSystemGeneralPage)
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
	ON_EN_UPDATE(IDC_EDIT_REG_KEY, OnUpdateEditRegKey)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemGeneralPage message handlers


BOOL CSystemGeneralPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_EditRegKey.SetLimitText(29);
	m_EditRegKey.SetMargins(5, 5);

	if (!m_pDoc->UserIsAdministrator())
	{
		m_EditRegKey.SetReadOnly();
	}

	CString cstrKey = (BSTR)m_pDoc->m_pSystem->CollectorRegistrationKey;

	if (cstrKey.GetLength() > 0)
	{
		cstrKey.TrimLeft();
		cstrKey.TrimRight();
		m_cstrRegKey = cstrKey;
		
		UpdateData(FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSystemGeneralPage::OnApply() 
{
	UpdateData();

	CString cstrKey = m_cstrRegKey;

	// Remove all spaces
	cstrKey.Remove(' ');
	cstrKey.Remove('\t');  // Tabs

	// Reformat key (6 x 4 char) and validate format
	m_cstrRegKey = "";
	int nLength = cstrKey.GetLength();
	int nIndex  = 0;
	while (nIndex < nLength)
	{
		if ((nIndex> 0) && (nIndex%4 == 0))
		{
			m_cstrRegKey += " ";
		}
		m_cstrRegKey +=  cstrKey.Mid(nIndex, 4);

		nIndex += 4;
	}
	UpdateData(FALSE);	

	// Check length 
    if(cstrKey.GetLength() != 24)
	{
		m_EditRegKey.SetFocus();
		AfxMessageBox("The key entered is incomplete.",MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}

	// Save key
	m_pDoc->m_pSystem->CollectorRegistrationKey = _bstr_t(m_cstrRegKey);

	m_pDoc->OnToolsRefresh();
	
	return CPropertyPage::OnApply();
}

void CSystemGeneralPage::OnUpdateEditRegKey() 
{
	SetModified();
}
