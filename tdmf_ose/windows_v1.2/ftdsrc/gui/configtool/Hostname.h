#if !defined(AFX_HOSTNAME_H__8E5EC000_A9FB_4218_8275_2F80891311D3__INCLUDED_)
#define AFX_HOSTNAME_H__8E5EC000_A9FB_4218_8275_2F80891311D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// hostname.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHostname dialog

class CHostname : public CDialog
{
// Construction
public:
	CHostname(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHostname)
	enum { IDD = IDD_HOSTNAME };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHostname)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:

	#define PRIM_HOST  1
	#define SEC_HOST   2

	int host_Type;

protected:

	// Generated message map functions
	//{{AFX_MSG(CHostname)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOSTNAME_H__8E5EC000_A9FB_4218_8275_2F80891311D3__INCLUDED_)
