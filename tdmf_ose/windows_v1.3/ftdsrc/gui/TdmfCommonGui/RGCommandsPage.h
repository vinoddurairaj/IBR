#if !defined(AFX_RGCOMMANDSPAGE_H__11216AD5_5DA9_4DF8_97EE_9493AF081189__INCLUDED_)
#define AFX_RGCOMMANDSPAGE_H__11216AD5_5DA9_4DF8_97EE_9493AF081189__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGCommandsPage.h : header file
//

#include "ServerCommandsPage.h"


/////////////////////////////////////////////////////////////////////////////
// CRGCommandsPage dialog

class CRGCommandsPage : public CServerCommandsPage
{
	DECLARE_DYNCREATE(CRGCommandsPage)

// Construction
public:
	CRGCommandsPage();
	~CRGCommandsPage();

	void FillCommandVector();
	CString GetDefaultArg(int nItemIndex);

// Dialog Data
	//{{AFX_DATA(CRGCommandsPage)
	enum { IDD = IDD_SERVER_COMMANDS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGCommandsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGCommandsPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGCOMMANDSPAGE_H__11216AD5_5DA9_4DF8_97EE_9493AF081189__INCLUDED_)
