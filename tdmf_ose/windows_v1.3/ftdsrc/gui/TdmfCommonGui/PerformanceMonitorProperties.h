#if !defined(AFX_PERFORMANCEMONITORPROPERTIES_H__77336A59_9188_4B37_8625_9C24A7115C91__INCLUDED_)
#define AFX_PERFORMANCEMONITORPROPERTIES_H__77336A59_9188_4B37_8625_9C24A7115C91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PerformanceMonitorProperties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPerformanceMonitorProperties dialog

class CPerformanceMonitorProperties : public CDialog
{
// Construction
public:
	CPerformanceMonitorProperties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPerformanceMonitorProperties)
	enum { IDD = IDD_PERFMON_PROPERTIES };
	CEdit	m_TXT_ValueEndCtrl;
	CEdit	m_TXT_ValueStartCtrl;
	CEdit	m_TXT_End_Date;
	CEdit	m_TXT_Start_Date;
	COleDateTime	m_DateTimeEnd;
	COleDateTime	m_DateTimeStart;
	int		m_nValueEnd;
	int		m_nValueStart;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerformanceMonitorProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPerformanceMonitorProperties)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFORMANCEMONITORPROPERTIES_H__77336A59_9188_4B37_8625_9C24A7115C91__INCLUDED_)
