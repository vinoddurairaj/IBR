///////////////////////////////////////////////////////////////////////////////
// File        : BaseTreeView.h
// Description : Defines shared base tree view class. Derive your tree views from
//               this class to benefit from implemented functinality
//               
// Author      : Mario Parent (Thursday, June 15, 2000)
//

#if !defined(AFX_BASETREEVIEW_H__262DFD08_193D_11D4_8550_00D009081798__INCLUDED_)
#define AFX_BASETREEVIEW_H__262DFD08_193D_11D4_8550_00D009081798__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BaseTreeView.h : header file
//
class CTDMFObject;
/////////////////////////////////////////////////////////////////////////////
// CBaseTreeView view

class CBaseTreeView : public CTreeView
{
protected:
	CBaseTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBaseTreeView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBaseTreeView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBaseTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CImageList*		m_pImageList;			// image list for the tree control
	CTreeCtrl&		m_treeCtrl;				// handle to the tree control of this view
	HTREEITEM		m_htiCrntSelected;	// currently selected tree item

	virtual void ReInitialize();
	virtual HTREEITEM InsertItem(HTREEITEM htiParent, int nImage, int nSelImage, CString strName, CTDMFObject *pTDMFObject);
   void ExpandBranch( HTREEITEM hti );
	void SortBranch( HTREEITEM hti );
	int GetItemImageID(HTREEITEM hti);
	BOOL SetupImageList(UINT nBmpID, UINT nImageWidth = 16, COLORREF crMask = RGB(255, 0, 255));

	// Generated message map functions
protected:
	void DeleteImageList();
	HTREEITEM InsertItemUnique(HTREEITEM htiParent, int nImage, int nSelImage, CString strName, CTDMFObject *pTDMFObject);
	//{{AFX_MSG(CBaseTreeView)
	afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BASETREEVIEW_H__262DFD08_193D_11D4_8550_00D009081798__INCLUDED_)
