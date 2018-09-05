// LeftTreeView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "LeftTreeView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView

IMPLEMENT_DYNCREATE(CLeftTreeView, CBaseTreeView)

CLeftTreeView::CLeftTreeView()
{
}

CLeftTreeView::~CLeftTreeView()
{
}


BEGIN_MESSAGE_MAP(CLeftTreeView, CBaseTreeView)
	//{{AFX_MSG_MAP(CLeftTreeView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView drawing

void CLeftTreeView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView diagnostics

#ifdef _DEBUG
void CLeftTreeView::AssertValid() const
{
	CBaseTreeView::AssertValid();
}

void CLeftTreeView::Dump(CDumpContext& dc) const
{
	CBaseTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Function    : CLeftTreeView::ReInitialize
// Description : Reload the tree view 
//
// Returns     : void 
//
// Author      : Mario Parent (3 août, 2000)
//
void CLeftTreeView::ReInitialize()
{
	CBaseTreeView::ReInitialize();

	CWaitCursor cWait;

 
	if (SetupImageList(IDB_TREE_IMAGE,22))
	{
	   SetRedraw(FALSE);
		// Establish main frame pointer
		m_pMainFrm = (CMainFrame*) AfxGetMainWnd();

		m_treeCtrl.DeleteAllItems();

		HTREEITEM hRoot = m_treeCtrl.GetRootItem() ;

		m_treeCtrl.ModifyStyle(TVS_TRACKSELECT,NULL);
	
		ExpandBranch(hRoot);

		m_treeCtrl.EnsureVisible(hRoot);

		m_htiCrntSelected = m_treeCtrl.GetRootItem();
		m_treeCtrl.Select(m_htiCrntSelected, TVGN_CARET);	
	
		SetRedraw(TRUE);
		Invalidate();

	}
	cWait.Restore();

}

void CLeftTreeView::LoadAllTheDomainsInTheTreeView()
{

}

void CLeftTreeView::LoadAllTheServerForDomain(CTDMFObject* pDomain)
{

}

void CLeftTreeView::LoadAllTheLogicalGroupForServer(CTDMFObject* pServer)
{

}

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView message handlers
