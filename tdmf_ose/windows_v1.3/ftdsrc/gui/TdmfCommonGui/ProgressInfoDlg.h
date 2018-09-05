#if !defined(AFX_PROGRESSINFODLG_H__BFEC711D_20D3_43FE_B7CA_6487D4D986EF__INCLUDED_)
#define AFX_PROGRESSINFODLG_H__BFEC711D_20D3_43FE_B7CA_6487D4D986EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProgressInfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgressInfoDlg dialog

class CProgressInfoDlg : public CDialog
{
// Construction
public:
	CProgressInfoDlg(CString strMsg, UINT nAviID, CWnd* pParent = NULL);
	~CProgressInfoDlg();
// Dialog Data
	//{{AFX_DATA(CProgressInfoDlg)
	enum { IDD = IDD_DIALOG_WAIT };
	CAnimateCtrl	m_AnimateCtrl;
	CString	m_strMsg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgressInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:

	// Generated message map functions
	//{{AFX_MSG(CProgressInfoDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGRESSINFODLG_H__BFEC711D_20D3_43FE_B7CA_6487D4D986EF__INCLUDED_)
