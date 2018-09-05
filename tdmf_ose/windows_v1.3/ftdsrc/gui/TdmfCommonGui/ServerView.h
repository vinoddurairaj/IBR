#if !defined(AFX_SERVERVIEW_H__D0FB87A4_E1AE_4D32_A31E_50FAB0AEB9EA__INCLUDED_)
#define AFX_SERVERVIEW_H__D0FB87A4_E1AE_4D32_A31E_50FAB0AEB9EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerView.h : header file
//

#include "ColumnSelectionDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CServerView view

class CServerView : public CListView
{
protected:
	CServerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerView)

	//TDMFOBJECTSLib::IDomainPtr m_pDomainParent;

	BOOL		m_bInUpdate;
    CImageList  m_ImageList;
	int SetNewItem(int nIndex, TDMFOBJECTSLib::IServer* pServer, bool bInsert = true);

	std::vector<CListViewColumnDef> m_vecColumnDef;
	void PopupListViewContextMenu(CPoint pt, int nColumn);
	void AddColumn(int nIndex, char* szName, UINT nWidth, BOOL bVisible = TRUE);
	void SaveColumnDef();
	void LoadColumnDef();

// Attributes
public:

// Operations
public:
	void AutoResizeAllVisibleColumns();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CServerView)
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERVIEW_H__D0FB87A4_E1AE_4D32_A31E_50FAB0AEB9EA__INCLUDED_)
