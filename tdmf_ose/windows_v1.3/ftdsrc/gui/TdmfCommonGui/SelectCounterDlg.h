#if !defined(AFX_SELECTCOUNTERDLG_H__C16F8910_5793_4FF1_82F4_8890F72EFECF__INCLUDED_)
#define AFX_SELECTCOUNTERDLG_H__C16F8910_5793_4FF1_82F4_8890F72EFECF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectCounterDlg.h : header file
//

#include <afxtempl.h>		// carray

class CServerPerformanceMonitorPage;
class CCounterInfo;

/////////////////////////////////////////////////////////////////////////////
// CSelectCounterDlg dialog

class CSelectCounterDlg : public CDialog
{
// Construction
public:
	CSelectCounterDlg(CServerPerformanceMonitorPage* pServerPerformanceMonitorPage,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectCounterDlg)
	enum { IDD = IDD_SELECT_PERF_MONITOR };
	CSpinButtonCtrl	m_Spin_Time;
	CButton	m_ButtonAdd;
	CListBox	m_ListStats;
	CListBox	m_ListGroup;
	UINT	m_Edit_Time;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectCounterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectCounterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CServerPerformanceMonitorPage*			m_pServerPerformanceMonitorPage;
	TDMFOBJECTSLib::IServerPtr				m_pServer;
    TDMFOBJECTSLib::IReplicationGroupPtr	m_pRG;
    CArray<int,int>							m_aryListBoxSelGroups;
    CArray<int,int>							m_aryListBoxSelStats;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTCOUNTERDLG_H__C16F8910_5793_4FF1_82F4_8890F72EFECF__INCLUDED_)
