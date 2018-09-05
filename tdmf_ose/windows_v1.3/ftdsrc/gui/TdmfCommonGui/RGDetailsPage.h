#if !defined(AFX_RGDETAILSPAGE_H__41C1ED2E_89EC_4B46_BB8D_2857749235F6__INCLUDED_)
#define AFX_RGDETAILSPAGE_H__41C1ED2E_89EC_4B46_BB8D_2857749235F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGDetailsPage.h : header file
//

#include "PropertyPageBase.h"


/////////////////////////////////////////////////////////////////////////////
// CRGDetailsPage dialog

class CRGDetailsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRGDetailsPage)

// Construction
public:
	CRGDetailsPage();
	~CRGDetailsPage();

	void RefreshValues();
	void MoveCtrl(CWnd& WndCtrl, int nX, int nWidth = -1);

// Dialog Data
	//{{AFX_DATA(CRGDetailsPage)
	enum { IDD = IDD_RG_DETAILS };
	CEdit	m_EditJournal;
	CStatic	m_TitleJournalLess;
	CStatic	m_TitleTimeout;
	CStatic	m_TitleSyncMode;
	CStatic	m_TitleSyncDepth;
	CStatic	m_TitleCompression;
	CStatic	m_TitleChunkSize;
	CStatic	m_TitleChunkDelay;
	CStatic	m_TitlePStoreJournal;
	CEdit	m_EditTimeout;
	CEdit	m_EditRemoteServer;
	CEdit	m_EditSyncMode;
	CEdit	m_EditSyncDepth;
	CEdit	m_EditPStoreJournal;
	CEdit	m_EditPort;
	CEdit	m_EditGroupNumber;
	CEdit	m_EditDescription;
	CEdit	m_EditCompression;
	CEdit	m_EditChunkSize;
	CEdit	m_EditChunkDelay;
	CEdit	m_EditChaining;
	CRichEditCtrl	m_RichEditSettings;
	CRichEditCtrl	m_RichEditName;
	long	m_nChunkDelay;
	long	m_nChunkSize;
	CString	m_strDescription;
	long	m_nGroupNumber;
	long	m_nPort;
	long	m_nSynchDepth;
	CString	m_strSynchMode;
	CString	m_strTargetServer;
	long	m_nTimeout;
	CString	m_strTitle;
	CString	m_strSettingsTitle;
	CString	m_strCompression;
	CString	m_strChaining;
	CString	m_strPStoreJournal;
	CString	m_strPStoreJournalTitle;
	CString	m_cstrJournal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGDetailsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGDetailsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGDETAILSPAGE_H__41C1ED2E_89EC_4B46_BB8D_2857749235F6__INCLUDED_)
