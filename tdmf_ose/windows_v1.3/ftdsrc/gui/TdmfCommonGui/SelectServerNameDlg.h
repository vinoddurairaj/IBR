#if !defined(AFX_SELECTSERVERNAMEDLG_H__DF463273_3A40_459F_B940_65956120C8A1__INCLUDED_)
#define AFX_SELECTSERVERNAMEDLG_H__DF463273_3A40_459F_B940_65956120C8A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectServerNameDlg.h : header file
//
#include "TdmfCommonGuiDoc.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectServerNameDlg dialog

class CSelectServerNameDlg : public CDialog
{
	TDMFOBJECTSLib::IDomainPtr m_pDomain;
    std::string* m_pstrTargetHostName;
	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CSelectServerNameDlg(TDMFOBJECTSLib::IDomain* pDomain, CTdmfCommonGuiDoc* pDoc, std::string* pstrTargetHostName ,CWnd* pParent = NULL);   // standard constructor
	
	int m_nServerId;

// Dialog Data
	//{{AFX_DATA(CSelectServerNameDlg)
	enum { IDD = IDD_SELECT_SERVERNAME };
	CListBox	m_List_Server;
	CString	m_StrMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectServerNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectServerNameDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTSERVERNAMEDLG_H__DF463273_3A40_459F_B940_65956120C8A1__INCLUDED_)
