#if !defined(AFX_NEWSCRIPTSERVERFILENAME_H__89F3D4EA_0E35_4BF8_9810_52899E54FB5E__INCLUDED_)
#define AFX_NEWSCRIPTSERVERFILENAME_H__89F3D4EA_0E35_4BF8_9810_52899E54FB5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewScriptServerFileName.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewScriptServerFileNameDlg dialog

class CNewScriptServerFileNameDlg : public CDialog
{
// Construction
public:
	CNewScriptServerFileNameDlg(BOOL bWindows, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewScriptServerFileNameDlg)
	enum { IDD = IDD_NEW_SCRIPTSERVER };
	CEdit	m_FileNameCtrl;
	CButton	m_BTN_OK_CTRL;
	CString	m_strFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewScriptServerFileNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	BOOL m_bWindows;

	// Generated message map functions
	//{{AFX_MSG(CNewScriptServerFileNameDlg)
	afx_msg void OnChangeEditFilename();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWSCRIPTSERVERFILENAME_H__89F3D4EA_0E35_4BF8_9810_52899E54FB5E__INCLUDED_)
