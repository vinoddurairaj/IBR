#if !defined(AFX_SYSTEMS_H__E0E9B003_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
#define AFX_SYSTEMS_H__E0E9B003_557D_11D3_BAF7_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Systems.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSystems dialog

class CSystems : public CDialog
{
// Construction
public:
	CSystems(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSystems)
	enum { IDD = IDD_DIALOG_SYSTEM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystems)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSystems)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMS_H__E0E9B003_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
