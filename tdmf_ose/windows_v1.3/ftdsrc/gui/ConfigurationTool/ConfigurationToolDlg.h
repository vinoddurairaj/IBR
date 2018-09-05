// ConfigurationToolDlg.h : header file
//

#if !defined(AFX_CONFIGURATIONTOOLDLG_H__8EEFEAC5_E707_4D70_BB8D_9683D32458D1__INCLUDED_)
#define AFX_CONFIGURATIONTOOLDLG_H__8EEFEAC5_E707_4D70_BB8D_9683D32458D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "ServerConfig.h"
#include "ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolDlg dialog

class CConfigurationToolDlg : public CDialog
{
// Construction
public:
	CConfigurationToolDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigurationToolDlg)
	enum { IDD = IDD_CONFIGURATIONTOOL_DIALOG };
	CComboBox	m_ListGroups;
	CEdit	m_EditBABSize;
	CSpinButtonCtrl	m_SpinBABSize;
	CString	m_cstrBABSize;
	CString m_cstrTcpWindowSize;
	CString m_cstrPort;
	CString	m_cstrNote;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigurationToolDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON  m_hIcon;
	bool   m_bShutdown;

	void   ShutDownSystem();
	void   CancelShutDownSystem();

public:
	CServerConfig    m_ServerConfig;
	CResourceManager m_ResourceManager;

	// Generated message map functions
	//{{AFX_MSG(CConfigurationToolDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeListGroups();
	afx_msg void OnButtonDeleteGroup();
	virtual void OnOK();
	afx_msg void OnButtonModifyGroup();
	afx_msg void OnButtonAddGroup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGURATIONTOOLDLG_H__8EEFEAC5_E707_4D70_BB8D_9683D32458D1__INCLUDED_)
