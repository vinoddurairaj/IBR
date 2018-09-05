#if !defined(AFX_SYSTEMDETAILSPAGE_H__E3BD18B5_878B_4C25_91D9_EF283DDE8BF2__INCLUDED_)
#define AFX_SYSTEMDETAILSPAGE_H__E3BD18B5_878B_4C25_91D9_EF283DDE8BF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemDetailsPage.h : header file
//

#include "PropertyPageBase.h"
#include "MessageByType.h"

/////////////////////////////////////////////////////////////////////////////
// CSystemDetailsPage dialog

class CSystemDetailsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSystemDetailsPage)

	TDMFOBJECTSLib::ISystemPtr m_pSystem;
  

// Construction
public:

	CSystemDetailsPage(TDMFOBJECTSLib::ISystem* pSystem = NULL);
	~CSystemDetailsPage();

	BOOL UpdateTheDataStatsFromTheCollector();
	void SetSystem(TDMFOBJECTSLib::ISystem* pSystem)
	{
		m_pSystem = pSystem;
	}

// Dialog Data
	//{{AFX_DATA(CSystemDetailsPage)
	enum { IDD = IDD_SYSTEM_DETAILS };
	CRichEditCtrl	m_RichEditCollector;
	CRichEditCtrl	m_RichEditDatabase;
	CRichEditCtrl	m_StatsTitleCtrl;
	CRichEditCtrl	m_RichEditName;
	CString	m_strName;
	double	m_dDBMsgPerHour;
	double	m_dDBMsgPerMin;
	double	m_dThrdPerHour;
	double	m_dThrdPerMin;
	double	m_dThrdPending;
	double	m_dDBMsgPending;
	CString	m_CollectorTimestamp;
	CString	m_Str_CollectorStatsTitle;
	CString	m_cstrDatabase;
	CString	m_cstrHostId;
	CString	m_cstrIP;
	CString	m_cstrPort;
	CString	m_cstrVersion;
	CString	m_cstrDatabaseTitle;
	CString	m_cstrTitleCollector;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSystemDetailsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSystemDetailsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMDETAILSPAGE_H__E3BD18B5_878B_4C25_91D9_EF283DDE8BF2__INCLUDED_)
