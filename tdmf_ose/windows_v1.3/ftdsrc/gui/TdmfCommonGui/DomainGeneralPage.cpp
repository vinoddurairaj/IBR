// DomainGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "DomainGeneralPage.h"
#include "DomainPropertySheet.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDomainGeneralPage property page

IMPLEMENT_DYNCREATE(CDomainGeneralPage, CPropertyPage)

CDomainGeneralPage::CDomainGeneralPage(TDMFOBJECTSLib::IDomain* pDomain, bool bNewItem)
	: CPropertyPage(CDomainGeneralPage::IDD), m_pDomain(pDomain), m_bNewItem(bNewItem)
{
	//{{AFX_DATA_INIT(CDomainGeneralPage)
	m_strDescription = _T("");
	m_strName = _T("");
	//}}AFX_DATA_INIT
	m_bPageModified = false;
 
}

CDomainGeneralPage::~CDomainGeneralPage()
{
}

void CDomainGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDomainGeneralPage)
	DDX_Control(pDX, IDC_EDIT_NAME, m_EditName);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 80);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 30);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDomainGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDomainGeneralPage)
	ON_EN_UPDATE(IDC_EDIT_DESCRIPTION, OnUpdateEditDescription)
	ON_EN_UPDATE(IDC_EDIT_NAME, OnUpdateEditName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDomainGeneralPage message handlers

BOOL CDomainGeneralPage::OnKillActive() 
{
	UpdateData();

    m_strName.TrimLeft();
    m_strName.TrimRight();
 
	// Validate
	if (m_strName.IsEmpty())
	{
		AfxMessageBox("You must enter a name.", MB_OK|MB_ICONINFORMATION);	
		m_EditName.SetFocus();
		m_EditName.SetSel(0, -1);
      
		return FALSE;
	}
    else
    {
        TDMFOBJECTSLib::ISystemPtr  pSystem = m_pDomain->Parent;

        if(pSystem)
        {
            long lKey =  m_pDomain->GetKey();
            if(pSystem->AlreadyExistDomain((LPCSTR)m_strName , lKey ))
            {

                CString strMessage;

                strMessage.Format("The name ' %s ' already exist. Please use another one!",m_strName);

                AfxMessageBox( strMessage , MB_OK|MB_ICONEXCLAMATION);  

                m_EditName.SetFocus();
		        m_EditName.SetSel(0, -1);

		        return FALSE;
 
             }
        }
    }

	return CPropertyPage::OnKillActive();
}

BOOL CDomainGeneralPage::OnApply() 
{
	try
	{
		if (m_pDomain != NULL && m_bPageModified)
		{
			UpdateData();
						
			m_pDomain->Name = (LPCTSTR)m_strName;
			m_strDescription.TrimLeft();
			m_strDescription.TrimRight();
			m_pDomain->Description = (LPCTSTR)m_strDescription;
			
			m_bPageModified = false;
			SetModified(FALSE);
			SendMessage (DM_SETDEFID, IDOK);
		}
	}
	CATCH_ALL_LOG_ERROR(1003);

	return CPropertyPage::OnApply();
}

BOOL CDomainGeneralPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	try
	{
		if (m_pDomain != NULL)
		{
			m_strName = (BSTR)m_pDomain->Name;
			m_strDescription = (BSTR)m_pDomain->Description;
			m_strDescription.TrimLeft();
			m_strDescription.TrimRight();
			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1004);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDomainGeneralPage::OnUpdateEditDescription() 
{
	SetModified();
	m_bPageModified = true;
}

void CDomainGeneralPage::OnUpdateEditName() 
{
	SetModified();
	m_bPageModified = true;
}

BOOL CDomainGeneralPage::OnSetActive() 
{
	if (m_bNewItem)
	{
		m_bPageModified = true;
		SetModified();
	}
	else
	{
		m_bPageModified = false;
	}
	
	return CPropertyPage::OnSetActive();
}
