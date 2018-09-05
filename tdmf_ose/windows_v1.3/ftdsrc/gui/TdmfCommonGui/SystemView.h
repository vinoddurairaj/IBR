#if !defined(AFX_SYSTEMVIEW_H__561FF69A_C806_4D54_828E_0A9DDCEC546E__INCLUDED_)
#define AFX_SYSTEMVIEW_H__561FF69A_C806_4D54_828E_0A9DDCEC546E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemView.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CSystemView view

class CSystemView : public CView
{
protected:
	CSystemView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSystemView)

	void SetItemImage(HTREEITEM hTreeItem);
	HTREEITEM FindDomain(TDMFOBJECTSLib::IDomainPtr pDomain);
	HTREEITEM FindDomain(CString strName);
	HTREEITEM FindDomain(long nKey);
	HTREEITEM FindServer(HTREEITEM hDomain, TDMFOBJECTSLib::IServerPtr pServer);
	HTREEITEM FindServer(HTREEITEM hDomain, long nHostID);
	HTREEITEM FindReplicationGroup(HTREEITEM hServer, TDMFOBJECTSLib::IReplicationGroupPtr pRG);
	HTREEITEM FindReplicationGroup(HTREEITEM hServer, CString strName);
	HTREEITEM FindReplicationGroup(HTREEITEM hServer, long nGroupNumber, HTREEITEM hPrev = NULL);
	void      ContextMenuAction(HTREEITEM hItem, CPoint* pPoint = NULL, BOOL bShow = TRUE, UINT nCmd = 0);
	void      SaveTreeStructure(HTREEITEM hItem, std::set<std::string> &setKey);
	void      RestoreTreeStructure(HTREEITEM hItem, std::set<std::string> &setKey, std::string& strSelection);

	long        m_nCurrentDomain;
	long        m_nCurrentServer;
	std::string m_strCurrentRG;
	
	HTREEITEM   m_hTreeItemDomain;
	HTREEITEM   m_hTreeItemServer;
	HTREEITEM   m_hTreeItemRG;

	HTREEITEM	m_hPreviousTreeItem;

	CImageList  m_ImageList;

	bool        m_bInUpdate;

	CTreeCtrl   m_TreeCtrl;
	CStatic     m_StaticLogo;

// Attributes
public:

// Operations
public:
	CTreeCtrl& GetTreeCtrl()
	{
		return m_TreeCtrl;
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
	BOOL PreTranslateMessage(MSG* pMsg);
	BOOL OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult);

// Implementation
protected:
	virtual ~CSystemView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CSystemView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMVIEW_H__561FF69A_C806_4D54_828E_0A9DDCEC546E__INCLUDED_)
