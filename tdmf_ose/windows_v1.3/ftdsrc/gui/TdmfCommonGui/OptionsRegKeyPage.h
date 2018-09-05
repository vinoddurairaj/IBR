#if !defined(AFX_OPTIONSREGKEYPAGE_H__62464717_4050_470E_9BCE_1C17600B1A87__INCLUDED_)
#define AFX_OPTIONSREGKEYPAGE_H__62464717_4050_470E_9BCE_1C17600B1A87__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsRegKeyPage.h : header file
//

#include "tdmfcommonguiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// COptionsRegKeyPage dialog

class COptionsRegKeyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(COptionsRegKeyPage)

	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	COptionsRegKeyPage(CTdmfCommonGuiDoc* pDoc = NULL);
	~COptionsRegKeyPage();

// Dialog Data
	//{{AFX_DATA(COptionsRegKeyPage)
	enum { IDD = IDD_OPTIONS_REG_KEY };
	CEdit	m_EditRegKey6;
	CEdit	m_EditRegKey5;
	CEdit	m_EditRegKey4;
	CEdit	m_EditRegKey3;
	CEdit	m_EditRegKey2;
	CEdit	m_EditRegKey1;
	CString	m_cstrRegKey1;
	CString	m_cstrRegKey2;
	CString	m_cstrRegKey3;
	CString	m_cstrRegKey4;
	CString	m_cstrRegKey5;
	CString	m_cstrRegKey6;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionsRegKeyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionsRegKeyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateEditRegKey1();
	afx_msg void OnUpdateEditRegKey2();
	afx_msg void OnUpdateEditRegKey3();
	afx_msg void OnUpdateEditRegKey4();
	afx_msg void OnUpdateEditRegKey5();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSREGKEYPAGE_H__62464717_4050_470E_9BCE_1C17600B1A87__INCLUDED_)
