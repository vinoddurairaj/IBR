// OptionPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "OptionPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionPropertySheet

IMPLEMENT_DYNAMIC(COptionPropertySheet, CPropertySheet)

COptionPropertySheet::COptionPropertySheet(CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage), m_OptionRegKeyPage(pDoc),m_OptionGeneralPage(pDoc)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;

/*	if (pDoc->GetConnectedFlag())
	{
     	AddPage(&m_OptionGeneralPage);
	}
*/
	AddPage(&m_OptionRegKeyPage);
}

COptionPropertySheet::COptionPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage), m_OptionRegKeyPage(pDoc),m_OptionGeneralPage(pDoc)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;

/*	if (pDoc->GetConnectedFlag())
	{
		AddPage(&m_OptionGeneralPage);
	}
*/
	AddPage(&m_OptionRegKeyPage);
}

COptionPropertySheet::~COptionPropertySheet()
{
}


BEGIN_MESSAGE_MAP(COptionPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(COptionPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionPropertySheet message handlers
