#if !defined(AFX_SERVERDETAILSPAGE_H__4C4EDF94_3C9D_45B9_83DB_111D8FADD385__INCLUDED_)
#define AFX_SERVERDETAILSPAGE_H__4C4EDF94_3C9D_45B9_83DB_111D8FADD385__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerDetailsPage.h : header file
//

#include "PropertyPageBase.h"


/////////////////////////////////////////////////////////////////////////////
// CServerDetailsPage dialog

class CServerDetailsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerDetailsPage)

// Construction
public:
	CServerDetailsPage();
	~CServerDetailsPage();

	void RefreshValues();
	void MoveCtrl(CWnd& WndCtrl, int nX, int nWidth = -1);

// Dialog Data
	//{{AFX_DATA(CServerDetailsPage)
	enum { IDD = IDD_SERVER_DETAILS };
	CEdit	m_EditRAM;
	CEdit	m_EditCPU;
	CListCtrl	m_ListJournal;
	CEdit	m_EditRegistrationKey;
	CEdit	m_EditOs;
	CEdit	m_EditIPAddress;
	CEdit	m_EditHostId;
	CEdit	m_EditSocketSize;
	CEdit	m_EditPStoreSize;
	CEdit	m_EditBabUsed;
	CEdit	m_EditBabSize;
	CEdit	m_EditBabEntries;
	CStatic	m_TitleTCPSize;
	CStatic	m_TitlePStoreSize;
	CStatic	m_TitleJournalSize;
	CStatic	m_TitleBabUsed;
	CStatic	m_TitleBabSize;
	CStatic	m_TitleBabEntries;
	CRichEditCtrl	m_RichEditSettings;
	CRichEditCtrl	m_RichEditName;
	CString	m_strIPAddress;
	CString	m_strOS;
	long	m_nNbRGTarget;
	long	m_nNbRGSource;
	long	m_nNbRPSource;
	long	m_nNbRPTarget;
	long	m_nBABEntries;
	CString	m_strBABUsed;
	CString	m_strBABSize;
	CString	m_strName;
	CString	m_strSettingsLabel;
	CString	m_strSocketBufSize;
	CString	m_strHostId;
	CString	m_strPStoreSize;
	CString	m_strRegistrationKey;
	CString	m_strCPU;
	CString	m_strRAMSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerDetailsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerDetailsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERDETAILSPAGE_H__4C4EDF94_3C9D_45B9_83DB_111D8FADD385__INCLUDED_)
