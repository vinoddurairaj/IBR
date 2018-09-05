#if !defined(AFX_SERVERPROPERTYSHEET_H__C885F684_5EA4_44F8_80EB_F43F8BBA9F4F__INCLUDED_)
#define AFX_SERVERPROPERTYSHEET_H__C885F684_5EA4_44F8_80EB_F43F8BBA9F4F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerPropertySheet.h : header file
//

#include "ServerGeneralPage.h"
#include "TdmfCommonGuiDoc.h"
#include "ViewNotification.h"
#include "ScriptEditorPage.h"

/////////////////////////////////////////////////////////////////////////////
// CServerPropertySheet

class CServerPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CServerPropertySheet)

private:
	CServerGeneralPage			m_ServerGeneralPage;
  //  CScriptEditorPage           m_ScriptEditorPage;
	TDMFOBJECTSLib::IServerPtr	m_pServer;

// Construction
public:
	CServerPropertySheet(CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0, TDMFOBJECTSLib::IServer* pServer = NULL);
	CServerPropertySheet(CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0, TDMFOBJECTSLib::IServer* pServer = NULL);

// Attributes
public:
	bool                        m_bContinue;
	bool                        m_bReboot;
	CTdmfCommonGuiDoc*			m_pDoc;
	bool                        m_bTcpWindowSizeChanged;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerPropertySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CServerPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CServerPropertySheet)
	afx_msg void OnApplyNow();
	afx_msg void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPROPERTYSHEET_H__C885F684_5EA4_44F8_80EB_F43F8BBA9F4F__INCLUDED_)
