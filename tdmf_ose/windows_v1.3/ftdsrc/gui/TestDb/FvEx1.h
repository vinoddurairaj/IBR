#if !defined(AFX_FVEX1_H__15E18AA3_B53A_489C_B504_736BB76D3143__INCLUDED_)
#define AFX_FVEX1_H__15E18AA3_B53A_489C_B504_736BB76D3143__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FvEx1.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// FvEx1 form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class FvEx1 : public CFormView
{
protected:
	FvEx1();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(FvEx1)

// Form Data
public:
	//{{AFX_DATA(FvEx1)
	enum { IDD = IDDFV_Ex1 };
	CString	m_szSql;
	CString	m_szAdd;
	CString	m_szSrvName;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

	int     IpSzToInt ( CString  pszIp  );
	CString IpIntToSz ( unsigned puIp   );
	CString IpIntToSz ( CString  pszIp  );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(FvEx1)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~FvEx1();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(FvEx1)
	afx_msg void OnFills();
	afx_msg void OnShowTbl();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FVEX1_H__15E18AA3_B53A_489C_B504_736BB76D3143__INCLUDED_)
