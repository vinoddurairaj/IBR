// Collector_StatsDlg.h : header file
//

#if !defined(AFX_COLLECTOR_STATSDLG_H__408CBF4A_BB9A_47BD_85F1_7FBAE7CAD985__INCLUDED_)
#define AFX_COLLECTOR_STATSDLG_H__408CBF4A_BB9A_47BD_85F1_7FBAE7CAD985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CCollector_StatsDlg dialog

class CCollector_StatsDlg : public CDialog
{
// Construction
public:
	CCollector_StatsDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCollector_StatsDlg)
	enum { IDD = IDD_COLLECTOR_STATS_DIALOG };
	CString	m_csNumOfDatabaseMessagesPending;
	CString	m_csNumOfMessageThreadsPending;
	CString	m_csCollectorTime;
	CString	m_csNumMessagesPerHour;
	CString	m_csNumMessagesPerMinute;
	CString	m_csNumThreadsPerHour;
	CString	m_csNumThreadsPerMinute;
	CString	m_csMsg0;
	CString	m_csMsg1; 
	CString	m_csMsg2; 
	CString	m_csMsg3; 
	CString	m_csMsg4; 
	CString	m_csMsg5; 
	CString	m_csMsg6; 
	CString	m_csMsg7; 
	CString	m_csMsg8; 
	CString	m_csMsg9; 
	CString	m_csMsg10;
	CString	m_csMsg11;
	CString	m_csMsg12;
	CString	m_csMsg13;
	CString	m_csMsg14;
	CString	m_csMsg15;
	CString	m_csMsg16;
	CString	m_csMsg17;
	CString	m_csMsg18;
	CString	m_csMsg19;
	CString	m_csMsg20;
	CString	m_csMsg21;
	CString	m_csMsg22;
	CString	m_csMsg23;
	CString	m_csAliveAgents;
	CString	m_csAliveMessagesPerHour;
	CString	m_csAliveMessagesPerMinute;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCollector_StatsDlg)
	public:
    virtual LRESULT OnCollectorStatisticsMessage(WPARAM wParam, LPARAM lParam);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON   m_hIcon;
    bool    m_bMapCreated;
    char*   m_pMemoryMappedFile;
    
	// Generated message map functions
	//{{AFX_MSG(CCollector_StatsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

static bool DestroyMapFile(HANDLE H_MMFile, char * p_MMFile);
static char * ReturnRW_MapedViewOfFile(HANDLE h_MMFile);
static bool CreateMapFile (LPCSTR cszFileName, DWORD size);
static void FlushMapFile(char * p_MMFile, DWORD size);
static int ip_to_ipstring(unsigned long ip, char *ipstring);


#endif // !defined(AFX_COLLECTOR_STATSDLG_H__408CBF4A_BB9A_47BD_85F1_7FBAE7CAD985__INCLUDED_)
