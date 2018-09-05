// DomainPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "DomainPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDomainPropertySheet

IMPLEMENT_DYNAMIC(CDomainPropertySheet, CPropertySheet)

CDomainPropertySheet::CDomainPropertySheet(CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage, TDMFOBJECTSLib::IDomain* pDomain, bool bNewItem)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage), m_DomainGeneralPage(pDomain, bNewItem), m_pDomain(pDomain), m_pDoc(pDoc), m_bNewItem(bNewItem)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;
	AddPage(&m_DomainGeneralPage);
}

CDomainPropertySheet::CDomainPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage, TDMFOBJECTSLib::IDomain* pDomain, bool bNewItem)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage), m_DomainGeneralPage(pDomain, bNewItem), m_pDomain(pDomain), m_pDoc(pDoc), m_bNewItem(bNewItem)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;
	AddPage(&m_DomainGeneralPage);
}

CDomainPropertySheet::~CDomainPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CDomainPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CDomainPropertySheet)
	ON_COMMAND(ID_APPLY_NOW, OnApplyNow)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDomainPropertySheet message handlers
void CDomainPropertySheet::OnApplyNow()
{
	CWaitCursor WaitCursor;

	if (m_DomainGeneralPage.OnKillActive())
	{
		// Save name before UpdateData in OnApply
		_bstr_t bstrNameOld = m_pDomain->Name;
		
		m_DomainGeneralPage.OnApply();
		
		// Save changes to DB
        int nResult = m_pDomain->SaveToDB();
		if ( nResult != 0)
		{
			MessageBox("Cannot save changes to database.", "Error", MB_OK | MB_ICONERROR);
		}

		// If it's a new domain send an ADD notification, otherwise send a CHANGE notification
		if (m_bNewItem)
		{
			m_bNewItem = FALSE;
			
			// Fire an Object New (add) Notification
			CViewNotification VN;
			VN.m_nMessageId = CViewNotification::DOMAIN_ADD;
			VN.m_pUnk = (IUnknown*)m_pDomain;
			m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
		}
		else
		{
			// Fire an Object Change Notification	
			CViewNotification VN;
			VN.m_nMessageId = CViewNotification::DOMAIN_CHANGE;
			VN.m_pUnk = (IUnknown*)m_pDomain;
			VN.m_dwParam1 = m_pDomain->GetKey();
			m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
		}
	}
}

void CDomainPropertySheet::OnOK()
{
	if (!m_DomainGeneralPage.m_bPageModified)
	{
		EndDialog(IDOK);
	}
	else
	{
		if (m_DomainGeneralPage.OnKillActive())
		{
			OnApplyNow();

			EndDialog(IDOK);
		}
	}
}

void CDomainPropertySheet::OnCancel()
{
	if (m_bNewItem)
	{
		m_pDoc->m_pSystem->RemoveDomain(m_pDomain);
	}

	EndDialog(IDCANCEL);
}

BOOL CDomainPropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	CWnd* pOKButton = GetDlgItem (IDOK);
	pOKButton->SetWindowText("Save");
	
	return bResult;
}
