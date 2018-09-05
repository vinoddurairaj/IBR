#if !defined(AFX_RGSELECTSERVERDIALOG_H__DC19D172_8AD4_4C6F_8F5C_835E8F5FF9F3__INCLUDED_)
#define AFX_RGSELECTSERVERDIALOG_H__DC19D172_8AD4_4C6F_8F5C_835E8F5FF9F3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGSelectServerDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRGSelectServerDialog dialog

class CRGSelectServerDialog : public CDialog
{
	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;
	TDMFOBJECTSLib::IServerPtr m_pServer;
	TDMFOBJECTSLib::IDomainPtr m_pDomain;


// Construction
public:
	BOOL FindServerInListBox(int& nindex);
	BOOL IsDHCPAdressSelected();
	CRGSelectServerDialog(TDMFOBJECTSLib::IReplicationGroup *pRG = NULL, CWnd* pParent = NULL);   // standard constructor

	TDMFOBJECTSLib::IServerPtr m_pServerSelected;

// Dialog Data
	//{{AFX_DATA(CRGSelectServerDialog)
	enum { IDD = IDD_RG_SELECT_SERVER_DIALOG };
	CListBox	m_ServerListBox;
	CString	m_ServerSelected;
	BOOL	m_Chk_DHCP;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRGSelectServerDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:
    BOOL m_bDHCPAdressSelected;
	// Generated message map functions
	//{{AFX_MSG(CRGSelectServerDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeServerList();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGSELECTSERVERDIALOG_H__DC19D172_8AD4_4C6F_8F5C_835E8F5FF9F3__INCLUDED_)
