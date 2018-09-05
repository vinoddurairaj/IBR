#if !defined(AFX_DOMAINDETAILSPAGE_H__75FC5C0B_C71B_4512_A9B2_F563A6328320__INCLUDED_)
#define AFX_DOMAINDETAILSPAGE_H__75FC5C0B_C71B_4512_A9B2_F563A6328320__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DomainDetailsPage.h : header file
//

#include "PropertyPageBase.h"


/////////////////////////////////////////////////////////////////////////////
// CDomainDetailsPage dialog

class CDomainDetailsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CDomainDetailsPage)

// Construction
public:
	CDomainDetailsPage();
	~CDomainDetailsPage();

	void RefreshValues();

// Dialog Data
	//{{AFX_DATA(CDomainDetailsPage)
	enum { IDD = IDD_DOMAIN_DETAILS };
	CEdit	m_EditDescription;
	CRichEditCtrl	m_RichEditTitle;
	CString	m_strDescription;
	CString	m_strTitle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDomainDetailsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDomainDetailsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOMAINDETAILSPAGE_H__75FC5C0B_C71B_4512_A9B2_F563A6328320__INCLUDED_)
