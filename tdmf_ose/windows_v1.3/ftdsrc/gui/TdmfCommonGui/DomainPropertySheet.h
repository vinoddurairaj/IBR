#if !defined(AFX_DOMAINPROPERTYSHEET_H__2ECE62C1_DFCD_4223_80F8_DD67B69E3B47__INCLUDED_)
#define AFX_DOMAINPROPERTYSHEET_H__2ECE62C1_DFCD_4223_80F8_DD67B69E3B47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DomainPropertySheet.h : header file
//

#include "DomainGeneralPage.h"
#include "TdmfCommonGuiDoc.h"
#include "ViewNotification.h"


/////////////////////////////////////////////////////////////////////////////
// CDomainPropertySheet

class CDomainPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CDomainPropertySheet)

private:
	CDomainGeneralPage m_DomainGeneralPage;
	TDMFOBJECTSLib::IDomainPtr m_pDomain;
	CTdmfCommonGuiDoc* m_pDoc;
	bool               m_bNewItem;

// Construction
public:
	CDomainPropertySheet(CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0, TDMFOBJECTSLib::IDomain* pDomain = NULL, bool bNewItem = false);
	CDomainPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0, TDMFOBJECTSLib::IDomain* pDomain = NULL, bool bNewItem = false);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainPropertySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDomainPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDomainPropertySheet)
	afx_msg void OnApplyNow();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOMAINPROPERTYSHEET_H__2ECE62C1_DFCD_4223_80F8_DD67B69E3B47__INCLUDED_)
