#if !defined(AFX_LOGINDLG_H__05410CF8_99C7_420C_AE33_0AC3B26CB9D9__INCLUDED_)
#define AFX_LOGINDLG_H__05410CF8_99C7_420C_AE33_0AC3B26CB9D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoginDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog

class CLoginDlg : public CDialog
{
// Construction
public:
	CLoginDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoginDlg)
	enum { IDD = IDD_LOGIN };
	CStatic	m_CtrlAppIcon;
	CEdit	m_EditUserId;
	CEdit	m_EditPassword;
	CComboBox	m_ComboBoxCollector;
	CString	m_cstrPassword;
	CString	m_cstrUserID;
	BOOL	m_bSaveInfo;
	CString	m_cstrCollectorName;
	int		m_nAuthentication;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoginDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckSave();
	virtual void OnOK();
	afx_msg void OnAuthentication();
	afx_msg void OnAuthenticationMsde();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINDLG_H__05410CF8_99C7_420C_AE33_0AC3B26CB9D9__INCLUDED_)
