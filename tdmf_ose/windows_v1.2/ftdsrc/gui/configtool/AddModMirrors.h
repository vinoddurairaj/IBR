#if !defined(AFX_ADDMODMIRRORS_H__A2ACA783_7684_11D3_BAFB_00C04F54F512__INCLUDED_)
#define AFX_ADDMODMIRRORS_H__A2ACA783_7684_11D3_BAFB_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddModMirrors.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddModMirrors dialog


class CAddModMirrors : public CDialog
{
// Construction
public:
	CAddModMirrors(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddModMirrors)
	enum { IDD = IDD_DIALOG_ADD_MOD_MIRROR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddModMirrors)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL	m_bAddOrMod;
	BOOL	m_bOK;
	char	m_szRemoteDevices[500];

	void	checkDriveType();
	void	getLocalDriveSize(char *szDrive, char *szSize);
	void	deleteDevice();
	void	modDevice();
	void	addDevice();
	void	getMirrorDevInfo();

protected:

	// Generated message map functions
	//{{AFX_MSG(CAddModMirrors)
	afx_msg void OnSelchangeListDataDev();
	afx_msg void OnSelchangeListMirrorDev();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDMODMIRRORS_H__A2ACA783_7684_11D3_BAFB_00C04F54F512__INCLUDED_)
