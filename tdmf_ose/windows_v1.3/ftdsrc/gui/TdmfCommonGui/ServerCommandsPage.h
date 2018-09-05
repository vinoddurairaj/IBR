#if !defined(AFX_SERVERCOMMANDSPAGE_H__F165CBB5_1E56_4BD9_8C66_9D1E6FEC92FA__INCLUDED_)
#define AFX_SERVERCOMMANDSPAGE_H__F165CBB5_1E56_4BD9_8C66_9D1E6FEC92FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerCommandsPage.h : header file
//

#include "PropertyPageBase.h"


/////////////////////////////////////////////////////////////////////////////
// CServerCommandsPage dialog

class CServerCommandsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerCommandsPage)

protected:
	struct StCmd
	{
		std::string strCmd;
		std::string strArg;
		TDMFOBJECTSLib::tagTdmfCommand eCmd;
	};

	std::vector<struct StCmd> m_vecStCmd;
	bool                      m_bServer;

	void PushBackCommand(char* pszCmd, char* pszArg, TDMFOBJECTSLib::tagTdmfCommand eCmd);
	virtual void FillCommandVector();
	virtual CString GetDefaultArg(int nItemIndex);


// Construction
public:
	CServerCommandsPage();
	~CServerCommandsPage();

// Dialog Data
	//{{AFX_DATA(CServerCommandsPage)
	enum { IDD = IDD_SERVER_COMMANDS };
	CEdit	m_CtrlLast;
	CComboBox	m_CboCmdHst;
	CComboBox	m_CboCmdLst;
	CString	m_szCmdDesc;
	CString	m_szCmd;
	CString	m_szResult;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerCommandsPage)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	BOOL CaptureTabKey() {return TRUE;}

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerCommandsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboCmdLst();
	afx_msg void OnButGo();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERCOMMANDSPAGE_H__F165CBB5_1E56_4BD9_8C66_9D1E6FEC92FA__INCLUDED_)
