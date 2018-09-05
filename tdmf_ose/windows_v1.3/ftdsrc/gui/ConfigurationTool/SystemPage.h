#if !defined(AFX_SYSTEMPAGE_H__CFC87D0F_D4E9_4218_AF4F_4328B3D577A8__INCLUDED_)
#define AFX_SYSTEMPAGE_H__CFC87D0F_D4E9_4218_AF4F_4328B3D577A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemPage.h : header file
//

#include "GroupConfig.h"


/////////////////////////////////////////////////////////////////////////////
// CSystemPage dialog

class CSystemPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSystemPage)

protected:
	CGroupConfig* m_pGroupConfig;

// Construction
public:
	CSystemPage(CGroupConfig* pGroupConfig);
	CSystemPage() {}
	~CSystemPage() {}

	BOOL Validate();
	BOOL SaveValues();

// Dialog Data
	//{{AFX_DATA(CSystemPage)
	enum { IDD = IDD_DIALOG_SYSTEM };
	CString	m_cstrPrimaryHost;
	CString	m_cstrPStore;
	CString	m_cstrNote;
	CString	m_cstrSecondaryHost;
	CString	m_cstrSecondaryPort;
	CString	m_cstrJournal;
	BOOL	m_bChaining;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSystemPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSystemPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMPAGE_H__CFC87D0F_D4E9_4218_AF4F_4328B3D577A8__INCLUDED_)
