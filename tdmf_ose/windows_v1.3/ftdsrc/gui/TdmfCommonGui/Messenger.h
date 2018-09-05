#if !defined(AFX_MESSENGER_H__5B4D0C39_8460_494F_AC98_ADD40E8809A4__INCLUDED_)
#define AFX_MESSENGER_H__5B4D0C39_8460_494F_AC98_ADD40E8809A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Messenger.h : header file
//

#include "TdmfCommonGuiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// CMessenger dialog

class CMessenger : public CDialog
{
protected:
	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CMessenger(CWnd* pParent = NULL);   // standard constructor

	void SetDocument(CTdmfCommonGuiDoc* pDoc) {m_pDoc = pDoc;}
	void AddMessage(UINT nID, char* pszMsg);

// Dialog Data
	//{{AFX_DATA(CMessenger)
	enum { IDD = IDD_MESSENGER };
	CString	m_cstrMessage;
	CString	m_cstrCommunications;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessenger)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMessenger)
	afx_msg void OnButtonSend();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MESSENGER_H__5B4D0C39_8460_494F_AC98_ADD40E8809A4__INCLUDED_)
