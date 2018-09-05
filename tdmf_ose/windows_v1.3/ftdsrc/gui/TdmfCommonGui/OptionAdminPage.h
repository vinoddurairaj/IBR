#if !defined(AFX_OPTIONADMINPAGE_H__D3691BE3_303C_4BD1_8267_5F22F426AAB2__INCLUDED_)
#define AFX_OPTIONADMINPAGE_H__D3691BE3_303C_4BD1_8267_5F22F426AAB2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionAdminPage.h : header file
//

#include "tdmfcommonguiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// COptionAdminPage dialog

class COptionAdminPage : public CPropertyPage
{
	DECLARE_DYNCREATE(COptionAdminPage)

	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	COptionAdminPage(CTdmfCommonGuiDoc* pDoc = NULL);
	~COptionAdminPage();

// Dialog Data
	//{{AFX_DATA(COptionAdminPage)
	enum { IDD = IDD_OPTIONS_ADMIN };
	BOOL	m_bLog;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionAdminPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionAdminPage)
	afx_msg void OnButtonViewLog();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonViewKeylog();
	afx_msg void OnButtonExportKeylog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONADMINPAGE_H__D3691BE3_303C_4BD1_8267_5F22F426AAB2__INCLUDED_)
