#if !defined(AFX_OPTIONPROPERTYSHEET_H__B6C189EC_473F_4482_8ED6_F3D479D80770__INCLUDED_)
#define AFX_OPTIONPROPERTYSHEET_H__B6C189EC_473F_4482_8ED6_F3D479D80770__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionPropertySheet.h : header file
//

#include "TdmfCommonGuiDoc.h"
#include "OptionGeneralPage.h"
#include "OptionsRegKeyPage.h"


/////////////////////////////////////////////////////////////////////////////
// COptionPropertySheet

class COptionPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(COptionPropertySheet)

protected:
	COptionGeneralPage   m_OptionGeneralPage;
	COptionsRegKeyPage   m_OptionRegKeyPage;

// Construction
public:
	COptionPropertySheet(CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	COptionPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionPropertySheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COptionPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(COptionPropertySheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONPROPERTYSHEET_H__B6C189EC_473F_4482_8ED6_F3D479D80770__INCLUDED_)
