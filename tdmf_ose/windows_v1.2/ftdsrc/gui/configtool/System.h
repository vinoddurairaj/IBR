#if !defined(AFX_SYSTEM_H__EDA354A6_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
#define AFX_SYSTEM_H__EDA354A6_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// System.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSystem dialog

class CSystem : public CPropertyPage
{
	DECLARE_DYNCREATE(CSystem)

// Construction
public:
	CSystem();
	~CSystem();

// Dialog Data
	//{{AFX_DATA(CSystem)
	enum { IDD = IDD_DIALOG_SYSTEM };
	CString	m_szSysNote;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSystem)
	//public:
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	void	checkDriveType();
	void	getLocalDriveSize(char *szDrive, char *szSize);

protected:
	// Generated message map functions
	//{{AFX_MSG(CSystem)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnCancelMode();
	virtual BOOL OnInitDialog();
    //ac 02-10-10
	//afx_msg void OnHostnamePrim();
	//afx_msg void OnHostnameSec();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEM_H__EDA354A6_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
