#if !defined(AFX_SYSTEMPROPERTYSHEET_H__5DCEF9DA_7F1B_460A_A7BB_4588F74599CD__INCLUDED_)
#define AFX_SYSTEMPROPERTYSHEET_H__5DCEF9DA_7F1B_460A_A7BB_4588F74599CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemPropertySheet.h : header file
//

#include "SystemUsersPage.h"
#include "OptionsDialog.h"
#include "OptionAdminPage.h"


/////////////////////////////////////////////////////////////////////////////
// CSystemPropertySheet

class CSystemPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CSystemPropertySheet)

	CSystemUsersPage     m_SystemUsersPage;
	COptionsDatabasePage m_OptionsDatabasePage;
	COptionAdminPage     m_OptionAdminPage;

// Construction
public:
	CSystemPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemPropertySheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSystemPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSystemPropertySheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMPROPERTYSHEET_H__5DCEF9DA_7F1B_460A_A7BB_4588F74599CD__INCLUDED_)
