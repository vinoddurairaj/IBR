#if !defined(AFX_EVENTPROPERTIES_H__10383CC3_425F_4CB4_BCE9_F77C4196F5D0__INCLUDED_)
#define AFX_EVENTPROPERTIES_H__10383CC3_425F_4CB4_BCE9_F77C4196F5D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventProperties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventProperties dialog

class CEventProperties : public CDialog
{
// Construction
public:
	CEventProperties(CTdmfCommonGuiDoc* pDoc, UINT nEventIndex, UINT m_nMaxIndex, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEventProperties)
	enum { IDD = IDD_DIALOG_EVENT_PROPERTIES };
	CButton	m_ButtonUp;
	CButton	m_ButtonDown;
	CButton	m_ButtonCopy;
	CString	m_cstrDescription;
	CString	m_cstrDate;
	CString	m_cstrGroupNumber;
	CString	m_cstrPairNumber;
	CString	m_cstrSource;
	CString	m_cstrTime;
	CString	m_cstrType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEventProperties)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonCopy();
	afx_msg void OnButtonDown();
	afx_msg void OnButtonUp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	CWnd*              m_pParent;
	CTdmfCommonGuiDoc* m_pDoc;
	UINT               m_nEventIndex;
	UINT               m_nMaxIndex;

	TDMFOBJECTSLib::IEventPtr GetEventAt(long pnIndex);
	bool                      RefreshDisplay();
	void                      UpdateButtonsStatus();

public:
	void                      SetMaxEvent(UINT nEventMax);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTPROPERTIES_H__10383CC3_425F_4CB4_BCE9_F77C4196F5D0__INCLUDED_)
