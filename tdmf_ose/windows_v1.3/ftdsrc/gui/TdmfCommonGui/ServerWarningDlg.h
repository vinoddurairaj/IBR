#if !defined(AFX_SERVERWARNINGDLG_H__07DF972E_2D9C_48F0_B871_1942A2598BD9__INCLUDED_)
#define AFX_SERVERWARNINGDLG_H__07DF972E_2D9C_48F0_B871_1942A2598BD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerWarningDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerWarningDlg dialog

class CServerWarningDlg : public CDialog
{
private:
	TDMFOBJECTSLib::IServerPtr m_pServer;
	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CServerWarningDlg(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IServer* pServer, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServerWarningDlg)
	enum { IDD = IDD_DIALOG_SERVER_WARNING };
	BOOL	m_bShowWarnings;
	long	m_nRequested;
	long	m_nAllocated;
	CString	m_cstrMsg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerWarningDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServerWarningDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERWARNINGDLG_H__07DF972E_2D9C_48F0_B871_1942A2598BD9__INCLUDED_)
