#if !defined(AFX_EMPTYPAGE_H__96CC5EE4_08F5_4DCC_A66E_74728E4E15D6__INCLUDED_)
#define AFX_EMPTYPAGE_H__96CC5EE4_08F5_4DCC_A66E_74728E4E15D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EmptyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEmptyPage dialog

class CEmptyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CEmptyPage)

// Construction
public:
	CEmptyPage();
	~CEmptyPage();

// Dialog Data
	//{{AFX_DATA(CEmptyPage)
	enum { IDD = IDD_EMPTY_PAGE};
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CEmptyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CEmptyPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMPTYPAGE_H__96CC5EE4_08F5_4DCC_A66E_74728E4E15D6__INCLUDED_)
