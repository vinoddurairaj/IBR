#if !defined(AFX_TUNABLESPAGE_H__E7FB5E78_8AB3_4251_8962_617C56E7E3E0__INCLUDED_)
#define AFX_TUNABLESPAGE_H__E7FB5E78_8AB3_4251_8962_617C56E7E3E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TunablesPage.h : header file
//

#include "GroupConfig.h"


/////////////////////////////////////////////////////////////////////////////
// CTunablesPage dialog

class CTunablesPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CTunablesPage)

	CGroupConfig* m_pGroupConfig;

// Construction
public:
	CTunablesPage(CGroupConfig* pGroupConfig);
	CTunablesPage() {}
	~CTunablesPage() {}

	BOOL Validate();
	BOOL SaveValues();
	BOOL IsPMDRestartNeeded();

// Dialog Data
	//{{AFX_DATA(CTunablesPage)
	enum { IDD = IDD_DIALOG_TUNABLE_PARAMS };
	CEdit	m_EditSyncTimeout;
	CEdit	m_EditSyncModeDepth;
	CEdit	m_EditTimeout;
	BOOL	m_bCompression;
	BOOL	m_bNeverTimeout;
	BOOL	m_bSyncMode;
	CString	m_cstrSyncDepth;
	CString	m_cstrSyncTimeout;
	CString	m_cstrRefreshInterval;
	BOOL	m_bJournalLess;
	//}}AFX_DATA

	BOOL    m_bJournalLessBefore;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTunablesPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTunablesPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckNeverTimeout();
	afx_msg void OnCheckSyncMode();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TUNABLESPAGE_H__E7FB5E78_8AB3_4251_8962_617C56E7E3E0__INCLUDED_)
