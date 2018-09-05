#if !defined(AFX_SYSTEMTESTWND_H__93651B75_A11A_4F10_A0D3_32F9C47E6C66__INCLUDED_)
#define AFX_SYSTEMTESTWND_H__93651B75_A11A_4F10_A0D3_32F9C47E6C66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemTestWnd.h : header file
//


class CSystem;

/////////////////////////////////////////////////////////////////////////////
// CSystemTestWnd window

class CSystemTestWnd : public CWnd
{
protected:
	CSystem* m_pSystem;

// Construction
public:
	CSystemTestWnd(CSystem* pSystem);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemTestWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSystemTestWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSystemTestWnd)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	afx_msg LRESULT OnTdmfStateEvent(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfServerEvent(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfPerfEvent(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfAddNewDomain(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfRemoveDomain(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfModifyDomain(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfAddNewServer(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfRemoveServer(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfModifyServer(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfAddNewGroup(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfRemoveGroup(WPARAM, LPARAM);
	afx_msg LRESULT OnTdmfModifyGroup(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMTESTWND_H__93651B75_A11A_4F10_A0D3_32F9C47E6C66__INCLUDED_)
