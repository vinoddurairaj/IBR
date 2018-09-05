#if !defined(AFX_THROTTLES_H__EDA354A4_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
#define AFX_THROTTLES_H__EDA354A4_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Throttles.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CThrottles dialog

class CThrottles : public CPropertyPage
{
	DECLARE_DYNCREATE(CThrottles)

// Construction
public:
	CThrottles();
	~CThrottles();

// Dialog Data
	//{{AFX_DATA(CThrottles)
	enum { IDD = IDD_DIALOG_THROTTLES };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CThrottles)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CThrottles)
	afx_msg void OnButtonThrottleBuilder();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THROTTLES_H__EDA354A4_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
