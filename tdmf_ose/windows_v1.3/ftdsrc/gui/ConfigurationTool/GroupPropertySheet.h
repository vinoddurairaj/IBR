#if !defined(AFX_GROUPPROPERTYSHEET_H__4DF61BC9_ADCA_4016_918C_B38979FF8A8F__INCLUDED_)
#define AFX_GROUPPROPERTYSHEET_H__4DF61BC9_ADCA_4016_918C_B38979FF8A8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupPropertySheet.h : header file
//

#include "GroupConfig.h"
#include "SystemPage.h"
#include "DevicesPage.h"
#include "TunablesPage.h"


/////////////////////////////////////////////////////////////////////////////
// CGroupPropertySheet

class CGroupPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CGroupPropertySheet)

// Construction
public:
	CGroupPropertySheet(CGroupConfig* pGroupConfig, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
protected:
	CGroupConfig* m_pGroupConfig;

	CSystemPage   m_SystemPage;
	CDevicesPage  m_DevicesPage;
	CTunablesPage m_TunablesPage;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupPropertySheet)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGroupPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGroupPropertySheet)
	afx_msg void OnOK();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPPROPERTYSHEET_H__4DF61BC9_ADCA_4016_918C_B38979FF8A8F__INCLUDED_)
