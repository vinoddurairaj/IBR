#if !defined(AFX_RGSYMMETRICPAGE_H__6D830C27_7E9A_4CD5_ACD0_6F2837C179F5__INCLUDED_)
#define AFX_RGSYMMETRICPAGE_H__6D830C27_7E9A_4CD5_ACD0_6F2837C179F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGSymmetricPage.h : header file
//

#include "LocationEdit.h"


/////////////////////////////////////////////////////////////////////////////
// CRGSymmetricPage dialog

class CRGSymmetricPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRGSymmetricPage)

// Construction
public:
	CRGSymmetricPage(TDMFOBJECTSLib::IReplicationGroup *pRG = NULL, bool bNewSymmetric = false);
	~CRGSymmetricPage();

	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;
	bool m_bNewSymmetric;
	BOOL m_bPageModified;

// Dialog Data
	//{{AFX_DATA(CRGSymmetricPage)
	enum { IDD = IDD_RG_SYMMETRIC };
	CEdit	m_EditSymmGroupNumber;
	CButton	m_CheckboxSymmGroupStarted;
	CEdit	m_EditSymmetricJournal;
	CLocationEdit m_EditSymmetricPStore;
	BOOL	m_bSymmetricNormallyStarted;
	UINT	m_nSymmetricGroupNumber;
	int		m_nFailoverInitialState;
	CString	m_cstrMode;
	CString	m_cstrConnectionState;
	CString	m_cstrSymmetricJournal;
	CString	m_cstrSymmetricPStore;
	CString	m_cstrPStoreTitle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGSymmetricPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGSymmetricPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckSymmetricGroupNormallyStarted();
	afx_msg void OnRadioPassthru();
	afx_msg void OnRadioTracking();
	afx_msg void OnUpdateEditSymmetricGroupNumber();
	afx_msg void OnUpdateEditJournal();
	afx_msg void OnUpdateEditPstore();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGSYMMETRICPAGE_H__6D830C27_7E9A_4CD5_ACD0_6F2837C179F5__INCLUDED_)
