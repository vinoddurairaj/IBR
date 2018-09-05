#if !defined(AFX_SYSTEMGENERALPAGE_H__9147FA40_3916_4ADC_A116_7D64FC0AA02C__INCLUDED_)
#define AFX_SYSTEMGENERALPAGE_H__9147FA40_3916_4ADC_A116_7D64FC0AA02C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemGeneralPage.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CSystemGeneralPage dialog

class CSystemGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSystemGeneralPage)

	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CSystemGeneralPage(CTdmfCommonGuiDoc* pDoc = NULL);
	~CSystemGeneralPage();

// Dialog Data
	//{{AFX_DATA(CSystemGeneralPage)
	enum { IDD = IDD_SYSTEM_GENERAL };
	CEdit	m_EditRegKey;
	CString	m_cstrRegKey;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSystemGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSystemGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateEditRegKey();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMGENERALPAGE_H__9147FA40_3916_4ADC_A116_7D64FC0AA02C__INCLUDED_)
