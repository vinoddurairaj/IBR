// AllControlsSheet.h : header file
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


#include "PropPage.h"
/////////////////////////////////////////////////////////////////////////////
// CMainPropertySheet

class CMainPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CMainPropertySheet)

// Construction
public:
   CMainPropertySheet();
	CMainPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMainPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

protected:
	void AddControlPages(void);

// Attributes
public:

	PropPg_Replications	m_Replications;
	PropPg_ReplicationGroups  m_ReplicationGroup;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainPropertySheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainPropertySheet();
	virtual BOOL OnInitDialog();

	// Generated message map functions
protected:

	HICON m_hIcon;

	//{{AFX_MSG(CMainPropertySheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
