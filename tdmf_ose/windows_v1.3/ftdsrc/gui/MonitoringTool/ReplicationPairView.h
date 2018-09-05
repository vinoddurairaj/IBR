// CTDMFReplicationPairView.h: interface for the CTDMFReplicationPairView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTDMFReplicationPairView_H__4A24F3E7_BB86_47C7_AFBC_0A665438EA3B__INCLUDED_)
#define AFX_CTDMFReplicationPairView_H__4A24F3E7_BB86_47C7_AFBC_0A665438EA3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SVBase.h"


class CTDMFReplicationPair;

class CTDMFReplicationPairView : public SVBase  
{
protected: // create from serialization only
	CTDMFReplicationPairView();
	DECLARE_DYNCREATE(CTDMFReplicationPairView)

// Attributes
public:
   void Update();

// Operations
public:
	bool				UpdateView();	// new collection
	virtual bool	StartView(int nSortColumn = -1, bool bSortDirection = ASCENDING);
	void				SetColumnDefaults();
	bool				AddRows(int nNumObjects);
	void				LoadThelistofReplicationPairs();
	void				DeleteAllReplicationPairs();
   void				InitialyzeTheView();

protected:
	CString			GetValueString(UINT nIndex, int nColumn);
	COleDateTime	GetValueDateTime(UINT nIndex, int nColumn);
	int				GetValueInt(UINT nObjectNumber,int nColumnNumber);
	bool				GetValueBool(UINT nIndex, int nColumn);
	double			GetValueDouble(UINT nIndex, int nColumn);
	bool				m_bFirstUpdate;
	SVSheet			m_Sheet;
	CMap<int, int, CTDMFReplicationPair*, CTDMFReplicationPair*>	m_mapReplicationPairs;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDMFReplicationPairView)
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTDMFReplicationPairView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTDMFReplicationPairView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()

};

#endif // !defined(AFX_SVLog_H__4A24F3E7_BB86_47C7_AFBC_0A665438EA3B__INCLUDED_)
