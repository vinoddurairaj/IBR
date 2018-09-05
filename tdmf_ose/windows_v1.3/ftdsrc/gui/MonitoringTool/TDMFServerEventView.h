#if !defined(AFX_TDMFSERVEREVENTVIEW_H__DCF749C1_CDB3_4DEB_80D7_3D6CD89C4517__INCLUDED_)
#define AFX_TDMFSERVEREVENTVIEW_H__DCF749C1_CDB3_4DEB_80D7_3D6CD89C4517__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TDMFServerEventView.h : header file
//

#include "Doc.h"
#include "SVBase.h"

class CTDMFEventInfo;

/////////////////////////////////////////////////////////////////////////////
// CTDMFServerEventView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CTDMFServerEventView : public SVBase
{
protected:
	DECLARE_DYNCREATE(CTDMFServerEventView)

// Attributes
public:
   CTDMFServerEventView();
	void	Update();

// Operations
public:
	bool				UpdateView();	// new collection
	virtual bool	StartView(int nSortColumn = -1, bool bSortDirection = ASCENDING);
	void				SetColumnDefaults();
	bool				AddRows(int nNumObjects);
	void				LoadThelistofEvents();
	void				DeleteAllEvents();
   void				InitialyzeTheView();

protected:
	CString			GetValueString(UINT nIndex, int nColumn);
	COleDateTime	GetValueDateTime(UINT nIndex, int nColumn);
	int				GetValueInt(UINT nObjectNumber,int nColumnNumber);
	bool				GetValueBool(UINT nIndex, int nColumn);
	double			GetValueDouble(UINT nIndex, int nColumn);

	bool				m_bFirstUpdate;
	SVSheet			m_Sheet;
	CMap<int, int, CTDMFEventInfo*, CTDMFEventInfo*>	m_mapEvents;
   CMenu				m_Menu;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDMFServerEventView)
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTDMFServerEventView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CTDMFServerEventView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFSERVEREVENTVIEW_H__DCF749C1_CDB3_4DEB_80D7_3D6CD89C4517__INCLUDED_)
