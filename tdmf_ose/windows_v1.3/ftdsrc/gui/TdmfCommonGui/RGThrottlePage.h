#if !defined(AFX_RGTHROTTLEPAGE_H__0345B74C_3E1B_4C37_BFB7_5CE75FD91C83__INCLUDED_)
#define AFX_RGTHROTTLEPAGE_H__0345B74C_3E1B_4C37_BFB7_5CE75FD91C83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGThrottlePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRGThrottlePage dialog

class CRGThrottlePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRGThrottlePage)

	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;	

// Construction
public:
	CRGThrottlePage(TDMFOBJECTSLib::IReplicationGroup *pRG = NULL, bool bReadOnly = false);
	~CRGThrottlePage();

// Dialog Data
	//{{AFX_DATA(CRGThrottlePage)
	enum { IDD = IDD_RG_THROTTLE };
	CEdit		m_Edit_Throttle_Ctrl;
	CString		m_strThrottles;
	//}}AFX_DATA
    BOOL m_bReadOnly;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGThrottlePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGThrottlePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGTHROTTLEPAGE_H__0345B74C_3E1B_4C37_BFB7_5CE75FD91C83__INCLUDED_)
