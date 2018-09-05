#if !defined(AFX_REPLICATIONGROUPVIEW_H__D0FB87A4_E1AE_4D32_A31E_50FAB0AEB9EA__INCLUDED_)
#define AFX_REPLICATIONGROUPVIEW_H__D0FB87A4_E1AE_4D32_A31E_50FAB0AEB9EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReplicationGroupView.h : header file
//

#include "ColumnSelectionDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupView view

class CReplicationGroupView : public CListView
{
protected:
	CReplicationGroupView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CReplicationGroupView)
 
    CImageList  m_ImageList;

	int SetNewItem(int nIndex, TDMFOBJECTSLib::IReplicationGroup* pRG, bool bInsert = true);
    void AutoResizeAllVisibleColumns();
	BOOL m_bInUpdate;

	std::vector<CListViewColumnDef> m_vecColumnDef;
	void PopupListViewContextMenu(CPoint pt, int nColumn);
	void AddColumn(int nIndex, char* szName, UINT nWidth, BOOL bVisible = TRUE);
	void SaveColumnDef();
	void LoadColumnDef();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReplicationGroupView)
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
	virtual ~CReplicationGroupView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CReplicationGroupView)
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPLICATIONGROUPVIEW_H__D0FB87A4_E1AE_4D32_A31E_50FAB0AEB9EA__INCLUDED_)
