#if !defined(AFX_DOMAINGENERALPAGE_H__C28856F8_D0E8_4AAD_B946_39B3D818C474__INCLUDED_)
#define AFX_DOMAINGENERALPAGE_H__C28856F8_D0E8_4AAD_B946_39B3D818C474__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DomainGeneralPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDomainGeneralPage dialog

class CDomainGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDomainGeneralPage)

private:
	TDMFOBJECTSLib::IDomainPtr m_pDomain;

// Construction
public:
	CDomainGeneralPage(TDMFOBJECTSLib::IDomain* pDomain = NULL, bool bNewItem = false);
	~CDomainGeneralPage();

	void SetDomain(TDMFOBJECTSLib::IDomain* pDomain)
	{
		m_pDomain = pDomain;
	}


// Dialog Data
	//{{AFX_DATA(CDomainGeneralPage)
	enum { IDD = IDD_DOMAIN_GENERAL };
	CEdit	m_EditDescription;
	CEdit	m_EditName;
	CString	m_strDescription;
	CString	m_strName;
	//}}AFX_DATA

	bool m_bPageModified;
	bool m_bNewItem;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDomainGeneralPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDomainGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateEditDescription();
	afx_msg void OnUpdateEditName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOMAINGENERALPAGE_H__C28856F8_D0E8_4AAD_B946_39B3D818C474__INCLUDED_)
