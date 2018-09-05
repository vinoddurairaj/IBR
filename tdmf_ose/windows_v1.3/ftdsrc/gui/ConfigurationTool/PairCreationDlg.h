#if !defined(AFX_PAIRCREATIONDLG_H__F148DD36_ADA3_4DF9_9DED_60123EDE50A0__INCLUDED_)
#define AFX_PAIRCREATIONDLG_H__F148DD36_ADA3_4DF9_9DED_60123EDE50A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PairCreationDlg.h : header file
//

#include "GroupConfig.h"


/////////////////////////////////////////////////////////////////////////////
// CPairCreationDlg dialog

class CPairCreationDlg : public CDialog
{
protected:
	LPCSTR m_lpcstrDataDev;
	LPCSTR m_lpcstrMirrorDev;

	CString m_cstrProductName;
	CGroupConfig* m_pGroupConfig;

	void   GetDrivesInfo(LPCSTR lpcstrHost, UINT nPort, UINT nGroupId, CComboBox* pComboBox, bool bSource = true);

// Construction
public:
	CPairCreationDlg(LPCSTR lpcstrProductName, CGroupConfig* pGroupConfig, LPCSTR lpcstrDataDev = NULL, LPCSTR lpcstrMirrorDev = NULL, CWnd* pParent = NULL);   // standard constructor

	CString m_cstrDataDevice;
	CString m_cstrMirrorDevice;

// Dialog Data
	//{{AFX_DATA(CPairCreationDlg)
	enum { IDD = IDD_DIALOG_ADD_MOD_MIRROR };
	CComboBox	m_ListMirrorDev;
	CComboBox	m_ListDataDev;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPairCreationDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPairCreationDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeListDataDev();
	afx_msg void OnSelchangeListMirrorDev();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAIRCREATIONDLG_H__F148DD36_ADA3_4DF9_9DED_60123EDE50A0__INCLUDED_)
