#if !defined(AFX_TDMFLOGICALGROUPSVIEW_H__AFDC250B_36A5_419E_8421_1996183C4BD0__INCLUDED_)
#define AFX_TDMFLOGICALGROUPSVIEW_H__AFDC250B_36A5_419E_8421_1996183C4BD0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TDMFLogicalGroupsView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroupsView view

#include "Doc.h"
#include "SVBase.h"
#include "PageView.h"

class CTDMFLogicalGroup;

class CTDMFLogicalGroupsView : public SVBase
{
protected:
	CTDMFLogicalGroupsView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTDMFLogicalGroupsView)

// Attributes
public:
   void Update();
// Operations
public:
  	bool				UpdateView();	// new collection
	virtual bool	StartView(int nSortColumn = -1, bool bSortDirection = ASCENDING);
	void				SetColumnDefaults();
	bool				AddRows(int nNumObjects);
	void				LoadThelistofLogicalGroups();
	void				DeleteAllLogicalGroups();
   void				InitialyzeTheView();

protected:
	CString			GetValueString(UINT nIndex, int nColumn);
	COleDateTime	GetValueDateTime(UINT nIndex, int nColumn);
	int				GetValueInt(UINT nObjectNumber,int nColumnNumber);
	bool				GetValueBool(UINT nIndex, int nColumn);
	double			GetValueDouble(UINT nIndex, int nColumn);
	bool				m_bFirstUpdate;
	SVSheet			m_Sheet;
	CMap<int, int, CTDMFLogicalGroup*, CTDMFLogicalGroup*>	m_mapLogicalGroups;
	
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDMFLogicalGroupsView)
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTDMFLogicalGroupsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTDMFLogicalGroupsView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFLOGICALGROUPSVIEW_H__AFDC250B_36A5_419E_8421_1996183C4BD0__INCLUDED_)
