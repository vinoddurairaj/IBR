// SystemUsersPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "SystemUsersPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemUsersPage property page

IMPLEMENT_DYNCREATE(CSystemUsersPage, CPropertyPage)

CSystemUsersPage::CSystemUsersPage(CTdmfCommonGuiDoc* pDoc) : 
	CPropertyPage(CSystemUsersPage::IDD), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(CSystemUsersPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSystemUsersPage::~CSystemUsersPage()
{
}

void CSystemUsersPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemUsersPage)
	DDX_Control(pDX, IDC_LIST_USERS, m_ListUsers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystemUsersPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSystemUsersPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemUsersPage message handlers

BOOL CSystemUsersPage::OnInitDialog() 
{
	// Impro...

	CPropertyPage::OnInitDialog();

	m_ListUsers.InsertColumn(0, "Location");
	m_ListUsers.SetColumnWidth(0, 100);

	m_ListUsers.InsertColumn(1, "User");
	m_ListUsers.SetColumnWidth(1, 100);

	m_ListUsers.InsertColumn(2, "Program");
	m_ListUsers.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER );

	// Refresh Data
	{
		USES_CONVERSION;

		CComBSTR bstrLocation;
		CComBSTR bstrUser;
		CComBSTR bstrType;
		CComBSTR bstrApp;

		m_pDoc->m_pSystem->GetFirstUser(&bstrLocation, &bstrUser, &bstrType, &bstrApp);

		while (bstrLocation.Length() > 0)
		{
			if (!(bstrUser == "DtcCollector"))
			{
				m_ListUsers.InsertItem(0, OLE2A(bstrLocation));
				m_ListUsers.SetItemText(0, 1, OLE2A(bstrUser));
				if (bstrApp == "Softek Replicator")  // Dummy replacement for re-branding. The database gets its information from the main exec resource: ProductName
				{
					bstrApp = theApp.m_ResourceManager.GetFullProductName();
				}
				m_ListUsers.SetItemText(0, 2, OLE2A(bstrApp));
			}

			m_pDoc->m_pSystem->GetNextUser(&bstrLocation, &bstrUser, &bstrType, &bstrApp);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
