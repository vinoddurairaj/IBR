#if !defined(AFX_SERVERREGISTRATIONPAGE_H__33228B71_A848_4D1D_89BD_B52FF3EB1346__INCLUDED_)
#define AFX_SERVERREGISTRATIONPAGE_H__33228B71_A848_4D1D_89BD_B52FF3EB1346__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerRegistrationPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerRegistrationPage dialog

class CServerRegistrationPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CServerRegistrationPage)

private:
	TDMFOBJECTSLib::IServerPtr m_pServer;

// Construction
public:
	CServerRegistrationPage(TDMFOBJECTSLib::IServer *pServer = NULL);
	~CServerRegistrationPage();

// Dialog Data
	//{{AFX_DATA(CServerRegistrationPage)
	enum { IDD = IDD_SERVER_REGISTRATION };
	CString	m_strRegKey;
	//}}AFX_DATA
	bool m_bPageModified;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerRegistrationPage)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerRegistrationPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateEditKey();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERREGISTRATIONPAGE_H__33228B71_A848_4D1D_89BD_B52FF3EB1346__INCLUDED_)
