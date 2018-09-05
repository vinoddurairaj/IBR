#if !defined(AFX_CLOGVIEWERDLG_H__26FC84BE_5AF1_4AC0_B1FB_2EF1957F048B__INCLUDED_)
#define AFX_CLOGVIEWERDLG_H__26FC84BE_5AF1_4AC0_B1FB_2EF1957F048B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogViewerDlg.h : header file
//

#include "tdmfcommonguiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// CLogViewerDlg dialog

class CLogViewerDlg : public CDialog
{
	CTdmfCommonGuiDoc* m_pDoc;
	BOOL               m_bActionsLog;

	void RefreshData();

// Construction
public:
	CLogViewerDlg(CTdmfCommonGuiDoc* pDoc, BOOL bActionsLog = TRUE, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLogViewerDlg)
	enum { IDD = IDD_LOG_VIEWER };
	CListCtrl	m_ListMsg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogViewerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLogViewerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnColumnclickListMsg(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLOGVIEWERDLG_H__26FC84BE_5AF1_4AC0_B1FB_2EF1957F048B__INCLUDED_)
