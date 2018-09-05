#if !defined(AFX_DTCTABCONFIG_H__E0E9B001_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
#define AFX_DTCTABCONFIG_H__E0E9B001_557D_11D3_BAF7_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DTCTabConfig.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDTCTabConfig dialog

class CDTCTabConfig : public CDialog
{
// Construction
public:
	CDTCTabConfig(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDTCTabConfig)
	enum { IDD = IDD_DIALOG_CONFIG2 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDTCTabConfig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:

	// Generated message map functions
	//{{AFX_MSG(CDTCTabConfig)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTCTABCONFIG_H__E0E9B001_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
