#if !defined(AFX_SERVERGENERALPAGE_H__8F2316F7_A81A_407C_A4D6_955813BA72AD__INCLUDED_)
#define AFX_SERVERGENERALPAGE_H__8F2316F7_A81A_407C_A4D6_955813BA72AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerGeneralPage.h : header file
//

#include "LocationEdit.h"


/////////////////////////////////////////////////////////////////////////////
// CServerGeneralPage dialog

class CServerGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CServerGeneralPage)

private:
	TDMFOBJECTSLib::IServerPtr m_pServer;

// Construction
public:
	CServerGeneralPage(TDMFOBJECTSLib::IServer *pServer = NULL);
	~CServerGeneralPage();

// Dialog Data
	//{{AFX_DATA(CServerGeneralPage)
	enum { IDD = IDD_SERVER_GENERAL };
	CStatic	m_LabelReplicationPort;
	CEdit	m_EditReplicationPort;
	CEdit	m_EditTCPWindowSize;
	CEdit	m_EditPort;
	CEdit	m_EditRegKey;
	CEdit	m_JournalEdit;
	CLocationEdit	m_PStoreEdit;
	CEdit	m_BABSizeEdit;
	CSpinButtonCtrl	m_SpinButton;
	long	m_nBABSize;
	CString	m_strDescription;
	long	m_nPort;
	CString	m_strPStore;
	long	m_nTCPWindowSize;
	CString	m_strJournal;
	CString	m_cstrPStoreTitle;
	CString	m_cstrRegKey;
	long	m_nReplicationPort;
	//}}AFX_DATA
	bool m_bPageModified;
	int  m_nMaxBabSize;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerGeneralPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateEditBabSize();
	afx_msg void OnUpdateEditPstore();
	afx_msg void OnUpdateEditTcpWindow();
	afx_msg void OnUpdateEditPort();
	afx_msg void OnUpdateEditDescription();
	afx_msg void OnUpdateEditJournal();
	afx_msg void OnUpdateEditRegKey();
	afx_msg void OnUpdateEditReplicationPort();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERGENERALPAGE_H__8F2316F7_A81A_407C_A4D6_955813BA72AD__INCLUDED_)
