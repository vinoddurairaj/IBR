// TDMFMonitorView.h : interface of the CTDMFMonitorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TDMFMONITORVIEW_H__AB3F7CA6_D059_44D1_B338_0EE51978D174__INCLUDED_)
#define AFX_TDMFMONITORVIEW_H__AB3F7CA6_D059_44D1_B338_0EE51978D174__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Doc.h"
#include "SVBase.h"

class CTDMFMonitorInfo;

class CTDMFMonitorView : public SVBase
{
protected: // create from serialization only
	CTDMFMonitorView();
	DECLARE_DYNCREATE(CTDMFMonitorView)

// Attributes
public:

// Operations
public:
	bool				UpdateView();	// new collection
	virtual bool	StartView(int nSortColumn = -1, bool bSortDirection = ASCENDING);
	void				SetColumnDefaults();
	bool				AddRows(int nNumObjects);
	void				LoadThelistofMonitors();
	void				DeleteAllMonitors();


protected:
	CString			GetValueString(UINT nIndex, int nColumn);
	COleDateTime	GetValueDateTime(UINT nIndex, int nColumn);
	int				GetValueInt(UINT nObjectNumber,int nColumnNumber);
	bool				GetValueBool(UINT nIndex, int nColumn);
	double			GetValueDouble(UINT nIndex, int nColumn);

	SVSheet			m_Sheet;
	CMap<int, int, CTDMFMonitorInfo*, CTDMFMonitorInfo*>	m_mapMonitors;
   CMenu				m_Menu;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDMFMonitorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTDMFMonitorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTDMFMonitorView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFMONITORVIEW_H__AB3F7CA6_D059_44D1_B338_0EE51978D174__INCLUDED_)
