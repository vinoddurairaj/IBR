// DiskUserDlg.h : header file
//

#if !defined(AFX_DISKUSERDLG_H__FD6287F8_AE52_454A_ACA7_CCE13449ACC0__INCLUDED_)
#define AFX_DISKUSERDLG_H__FD6287F8_AE52_454A_ACA7_CCE13449ACC0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDiskUserDlgAutoProxy;

/////////////////////////////////////////////////////////////////////////////
// CDiskUserDlg dialog

class CDiskUserDlg : public CDialog
{
	DECLARE_DYNAMIC(CDiskUserDlg);
	friend class CDiskUserDlgAutoProxy;

// Construction
public:
	CDiskUserDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CDiskUserDlg();

// Dialog Data
	//{{AFX_DATA(CDiskUserDlg)
	enum { IDD = IDD_DISKUSER_DIALOG };
	CEdit	m_CtrlSleepRange;
	CSliderCtrl	m_SliderCtrl;
	CEdit	m_CtrlNumThreads;
	CEdit	m_CtrlDiskSize;
	CComboBox	m_ListBoxPriorities;
	CComboBox	m_ListBoxDriveNames;
	CButton	m_StopButton;
	CButton	m_StartButton;
	UINT	m_uiDiskSize;
	UINT	m_uiNumThreads;
	CString	m_CurrentDrive;
	CString	m_CurrentPriority;
	UINT	m_uiNumThreadsExecuting;
	UINT	m_Kb_DiskSizePerThread;
	CString	m_TestStatus;
	UINT	m_uiSleepRange;
	CString	m_csReadWriteRatio;
	CString	m_csSleepRange;
	int		m_iSliderPosition;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiskUserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDiskUserDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

    bool    m_Valid;

	// Generated message map functions
	//{{AFX_MSG(CDiskUserDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnStart();
	afx_msg void OnStop();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnChangeEDITNumThreads();
	afx_msg void OnChangeEDITDiskSize();
	afx_msg void OnChangeEDITSleepRange();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISKUSERDLG_H__FD6287F8_AE52_454A_ACA7_CCE13449ACC0__INCLUDED_)
