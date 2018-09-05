#if !defined(AFX_DELETEGROUP_H__944D8F44_5969_11D3_BAF8_00C04F54F512__INCLUDED_)
#define AFX_DELETEGROUP_H__944D8F44_5969_11D3_BAF8_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DeleteGroup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeleteGroup dialog

class CDeleteGroup : public CDialog
{
// Construction
public:
	CDeleteGroup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeleteGroup)
	enum { IDD = IDD_DIALOG_DELETE_GROUP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteGroup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	bool m_bDeleteGroup;

protected:

	// Generated message map functions
	//{{AFX_MSG(CDeleteGroup)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DELETEGROUP_H__944D8F44_5969_11D3_BAF8_00C04F54F512__INCLUDED_)
