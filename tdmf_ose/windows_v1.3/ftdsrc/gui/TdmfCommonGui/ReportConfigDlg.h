#if !defined(AFX_REPORTCONFIGDLG_H__C16F8910_5793_4FF1_82F4_8890F72EFECF__INCLUDED_)
#define AFX_REPORTCONFIGDLG_H__C16F8910_5793_4FF1_82F4_8890F72EFECF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReportConfigDlg.h : header file
//


class CServerPerformanceReporterPage;


/////////////////////////////////////////////////////////////////////////////
// CReportConfigDlg dialog

class CReportConfigDlg : public CDialog
{
// Construction
public:
	CReportConfigDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReportConfigDlg)
	enum { IDD = IDD_DIALOG_ADD_COUNTERS };
	CButton	m_ButtonAdd;
	CListBox	m_ListStats;
	CListBox	m_ListGroup;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReportConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListGroups();
	afx_msg void OnSelchangeListStats();
	afx_msg void OnButtonAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	TDMFOBJECTSLib::IServerPtr m_pServer;
	CServerPerformanceReporterPage* m_pServerPerformanceReporterPage;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPORTCONFIGDLG_H__C16F8910_5793_4FF1_82F4_8890F72EFECF__INCLUDED_)
