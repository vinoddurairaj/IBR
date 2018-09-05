#if !defined(AFX_RGTUNABLEPAGE_H__B21D0012_6F05_4E2F_A4DA_03A363F91CF3__INCLUDED_)
#define AFX_RGTUNABLEPAGE_H__B21D0012_6F05_4E2F_A4DA_03A363F91CF3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGTunablePage.h : header file
//

#define	IS_NOT_CHECKED	0
#define	IS_CHECKED		1
/////////////////////////////////////////////////////////////////////////////
// CRGTunablePage dialog

class CRGTunablePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRGTunablePage)

	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;

// Construction
public:
	CRGTunablePage(TDMFOBJECTSLib::IReplicationGroup *pRG = NULL, BOOL bWindows = TRUE);
	~CRGTunablePage();

// Dialog Data
	//{{AFX_DATA(CRGTunablePage)
	enum { IDD = IDD_RG_TUNABLE };
	CButton	m_ButtonSync;
	CButton	m_ButtonNeverTimeout;
	CButton	m_ButtonCompression;
	CEdit	m_EditTimeoutInterval;
	CEdit	m_EditTimeout;
	CEdit	m_EditDepth;
	BOOL	m_bCompression;
	BOOL	m_bNeverTimeout;
	BOOL	m_bSync;
	long	m_nDepth;
	long	m_nTimeout;
	CString	m_strTimeoutInterval;
	UINT	m_nChunkDelay;
	UINT	m_nChunkSize;
	UINT	m_nStatUpdateInterval;
	UINT	m_nStatMaxSize;
	CButton m_ButtonNetThreshold;
	BOOL	m_bNetThreshold;
	CEdit   m_EditNetThreshold;
	UINT	m_nNetTheshold;
	BOOL    m_bJournalLess;

	//}}AFX_DATA
	bool m_bPageModified;
	BOOL m_bWindows;
	BOOL m_bJournalLessBefore;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGTunablePage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGTunablePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckCompression();
	afx_msg void OnCheckNevertimeout();
	afx_msg void OnCheckSync();
	afx_msg void OnCheckNetThreshold();
	afx_msg void OnCheckJournalLess();
	afx_msg void OnUpdateEditDepth();
	afx_msg void OnUpdateEditTimeout();
	afx_msg void OnUpdateEditTimeoutInterval();
	afx_msg void OnUpdateEditChunkdelay();
	afx_msg void OnUpdateEditChunksize();
	afx_msg void OnUpdateEditStatUpdateInterval();
	afx_msg void OnUpdateEditStatMaxSize();
	afx_msg void OnUpdateEditNetThreshold();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGTUNABLEPAGE_H__B21D0012_6F05_4E2F_A4DA_03A363F91CF3__INCLUDED_)
