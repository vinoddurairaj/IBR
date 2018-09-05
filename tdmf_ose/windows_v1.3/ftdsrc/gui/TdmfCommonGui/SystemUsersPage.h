#if !defined(AFX_SYSTEMUSERSPAGE_H__009CDE0A_C720_45C0_90E5_74D825ABB77D__INCLUDED_)
#define AFX_SYSTEMUSERSPAGE_H__009CDE0A_C720_45C0_90E5_74D825ABB77D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemUsersPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSystemUsersPage dialog

class CSystemUsersPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSystemUsersPage)

	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CSystemUsersPage(CTdmfCommonGuiDoc* pDoc = NULL);
	~CSystemUsersPage();

// Dialog Data
	//{{AFX_DATA(CSystemUsersPage)
	enum { IDD = IDD_SYSTEM_USERS };
	CListCtrl	m_ListUsers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSystemUsersPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSystemUsersPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMUSERSPAGE_H__009CDE0A_C720_45C0_90E5_74D825ABB77D__INCLUDED_)
