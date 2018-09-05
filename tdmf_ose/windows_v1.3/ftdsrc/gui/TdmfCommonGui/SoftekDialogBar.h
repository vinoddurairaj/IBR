#if !defined(AFX_SOFTEKDIALOGBAR_H__6C5C9DF4_B59B_485E_924A_C22F3989F23D__INCLUDED_)
#define AFX_SOFTEKDIALOGBAR_H__6C5C9DF4_B59B_485E_924A_C22F3989F23D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SoftekDialogBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSoftekDialogBar window

class CSoftekDialogBar : public CDialogBar
{
// Construction
public:
	CSoftekDialogBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSoftekDialogBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSoftekDialogBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSoftekDialogBar)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SOFTEKDIALOGBAR_H__6C5C9DF4_B59B_485E_924A_C22F3989F23D__INCLUDED_)
