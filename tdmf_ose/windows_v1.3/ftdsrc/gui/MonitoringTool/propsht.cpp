// AllControlsSheet.cpp : implementation file
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "propsht.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainPropertySheet

IMPLEMENT_DYNAMIC(CMainPropertySheet, CPropertySheet)

CMainPropertySheet::CMainPropertySheet()
{
  AddControlPages();
}

CMainPropertySheet::CMainPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	AddControlPages();

	// TODO :: Add the pages for the rest of the controls here.
}

CMainPropertySheet::CMainPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddControlPages();
}

CMainPropertySheet::~CMainPropertySheet()
{
}

void CMainPropertySheet::AddControlPages()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_psh.dwFlags |= PSP_USEHICON;
	m_psh.hIcon = m_hIcon;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;    // Lose the Apply Now button
	m_psh.dwFlags &= ~PSH_HASHELP;  // Lose the Help button


	AddPage(&m_Replications);
	AddPage(&m_ReplicationGroup);

}

BEGIN_MESSAGE_MAP(CMainPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CMainPropertySheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainPropertySheet message handlers

BOOL CMainPropertySheet::OnInitDialog()
{
	return CPropertySheet::OnInitDialog();
}

