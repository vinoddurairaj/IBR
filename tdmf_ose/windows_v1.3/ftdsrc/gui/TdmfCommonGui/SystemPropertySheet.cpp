// SystemPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "SystemPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemPropertySheet

IMPLEMENT_DYNAMIC(CSystemPropertySheet, CPropertySheet)

CSystemPropertySheet::CSystemPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage), m_SystemUsersPage(pDoc),
	m_OptionsDatabasePage(pDoc->m_pSystem), m_OptionAdminPage(pDoc)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;

	AddPage(&m_SystemUsersPage);

	if (!pDoc->GetReadOnlyFlag())
	{
		AddPage(&m_OptionsDatabasePage);
	}
	if (pDoc->UserIsAdministrator())
	{
		AddPage(&m_OptionAdminPage);
	}
}

CSystemPropertySheet::~CSystemPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CSystemPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CSystemPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemPropertySheet message handlers
