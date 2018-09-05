#if !defined(AFX_TIMERANGEDLG_H__DCC13EC4_A1DA_4B86_88CE_02765135D0E0__INCLUDED_)
#define AFX_TIMERANGEDLG_H__DCC13EC4_A1DA_4B86_88CE_02765135D0E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TimeRangeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTimeRangeDlg dialog

class CTimeRangeDlg : public CDialog
{
// Construction
public:
	CTimeRangeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTimeRangeDlg)
	enum { IDD = IDD_TIMERANGE };
	CSpinButtonCtrl	m_Spin_CTRL;
	UINT	m_value;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeRangeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTimeRangeDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMERANGEDLG_H__DCC13EC4_A1DA_4B86_88CE_02765135D0E0__INCLUDED_)
