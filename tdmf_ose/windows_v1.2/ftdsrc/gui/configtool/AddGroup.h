#if !defined(AFX_ADDGROUP_H__944D8F43_5969_11D3_BAF8_00C04F54F512__INCLUDED_)
#define AFX_ADDGROUP_H__944D8F43_5969_11D3_BAF8_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddGroup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddGroup dialog

class CAddGroup : public CDialog
{
// Construction
public:
	CAddGroup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddGroup)
	enum { IDD = IDD_DIALOG_ADD_GROUP };
	CString	m_szGroupNote;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddGroup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	int		m_iGroup;
	char	m_strGroupNote[_MAX_PATH];
	BOOL	m_bAddGroupCancel;

	CEdit	*m_pEditGroup;
	CEdit	*m_pEditGroupNote;

	int		getGroup();
	void	getGroupNote(char strGroupNote[_MAX_PATH]);
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddGroup)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDGROUP_H__944D8F43_5969_11D3_BAF8_00C04F54F512__INCLUDED_)
