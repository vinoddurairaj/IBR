#if !defined(AFX_LEFTTREEVIEW_H__705D3F21_1F4C_43B1_8DFE_B8E93DBF6030__INCLUDED_)
#define AFX_LEFTTREEVIEW_H__705D3F21_1F4C_43B1_8DFE_B8E93DBF6030__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LeftTreeView.h : header file
//

#include "BaseTreeView.h"

class CMainFrame;
/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView view

class CLeftTreeView : public CBaseTreeView
{
protected:
	CLeftTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CLeftTreeView)

// Attributes
public:
		CMainFrame* m_pMainFrm;
// Operations
public:
	virtual void	ReInitialize();
   void				LoadAllTheDomainsInTheTreeView();
	void				LoadAllTheServerForDomain(CTDMFObject* pDomain);
	void				LoadAllTheLogicalGroupForServer(CTDMFObject* pServer);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLeftTreeView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLeftTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CLeftTreeView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEFTTREEVIEW_H__705D3F21_1F4C_43B1_8DFE_B8E93DBF6030__INCLUDED_)
