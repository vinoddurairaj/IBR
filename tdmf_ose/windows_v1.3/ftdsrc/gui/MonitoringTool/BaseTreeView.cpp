///////////////////////////////////////////////////////////////////////////////
// File        : BaseTreeView.cpp
// Description : Implements shared base tree view
//               
// Author      : Mario Parent (Thursday, June 15, 2000)
//

#include "stdafx.h"
#include "BaseTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseTreeView

IMPLEMENT_DYNCREATE(CBaseTreeView, CTreeView)


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::CBaseTreeView
// Description : Constructor used to also initialize members
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
CBaseTreeView::CBaseTreeView() : m_treeCtrl(GetTreeCtrl())
{
	m_pImageList = NULL;
	m_htiCrntSelected = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::~CBaseTreeView
// Description : Destructor is responsible to destroy the image list
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
CBaseTreeView::~CBaseTreeView()
{
	DeleteImageList();
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::DeleteImageList
// Description : Deletes the image list and nulls it out
//
// Returns     : void 
//
// Author      : Mario Parent (Thursday, June 29, 2000)
//
void CBaseTreeView::DeleteImageList()
{
	// distroy the Image List
	if (m_pImageList)
		delete m_pImageList, m_pImageList = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::PreCreateWindow
// Description : Used to define added styles to this tree view. If you override
//               in your derived class be sure to call this base class implementation
//
// Returns     : BOOL 
// Argument    : CREATESTRUCT& cs
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
BOOL CBaseTreeView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= (TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS 
						| TVS_SHOWSELALWAYS | TVS_TRACKSELECT);
	
	return CTreeView::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::SetupImageList
// Description : Call this from your derived class to setup the image list
//               this tree view is going to use for all of its nodes.
//               Magenta is used as the default transparent color.
//
// Returns     : BOOL - TRUE if successful
// Argument    : UINT nBmpID - the resource bmp id with all the tree node images
// Argument    : UINT nImageWidth - common width of each image
// Argument    : COLORREF crMask - RGB value to define the mask (transparent color)
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
//Modify by		: Mario Parent(23Aug2000) Add DeleteImageList();
//Modified by  : Mario Parent (Tuesday, October 17, 2000)
//             :  added the third argument for the mask.
//
BOOL CBaseTreeView::SetupImageList(UINT nBmpID, UINT nImageWidth, COLORREF crMask)
{
   DeleteImageList();

	m_pImageList = new CImageList();
	if (!m_pImageList)
		return FALSE;

	if (!m_pImageList->Create(nBmpID, nImageWidth, 4, crMask))
		return FALSE;

	m_treeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::GetItemImageID
// Description : Use this to retreive the image id for a given tree node.
//               Tipically used as a tree item identification method.
//
// Returns     : int - image offset into the nBmpID in SetupImageList, -1 if invalid
// Argument    : HTREEITEM hti - the tree item we are querying for
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
int CBaseTreeView::GetItemImageID(HTREEITEM hti)
{
	int nImage = -1, nSelectedImage = -1;
	
	if (hti != NULL)
		m_treeCtrl.GetItemImage(hti, nImage, nSelectedImage);

	return nImage;
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::InsertItem
// Description : Used to insert tree items into this tree view.
//
// Returns     : HTREEITEM - the tree item inserted
// Argument    : HTREEITEM htiParent - its parent, NULL if root
// Argument    : int nImage - the image offset into the nBmpID in SetupImageList
// Argument    : int nSelImage - the selected image offset into the nBmpID in SetupImageList
// Argument    : CString strName - item name to display
// Argument    : CTDMFObject *pDataNode - item data it represents to store for retreival
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
HTREEITEM CBaseTreeView::InsertItem(HTREEITEM htiParent, int nImage, int nSelImage, CString strName, CTDMFObject *pDataNode)
{
	TV_INSERTSTRUCT	tviItem;

	// load up the structure
	tviItem.hParent = htiParent;
	tviItem.hInsertAfter = TVI_LAST;
	tviItem.item.iImage = nImage;
	tviItem.item.iSelectedImage = nSelImage;
	tviItem.item.pszText = strName.GetBuffer(strName.GetLength());
	tviItem.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;

	// now insert it
	HTREEITEM htiNew = m_treeCtrl.InsertItem(&tviItem);
	if (htiNew)
		m_treeCtrl.SetItemData(htiNew, (DWORD) pDataNode);

	return htiNew;		
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::InsertItemUnique
// Description : Call this to insert a unique tree item into the tree view.
//               Duplicates will be rejected based on the CTDMFObject *pDataNode attribute
//
// Returns     : HTREEITEM - tree item if successful, NULL if not
// Argument    : HTREEITEM htiParent - the parent of the tree item to insert, NULL if root
// Argument    : int nImage - the image offset into the nBmpID in SetupImageList
// Argument    : int nSelImage - the selected image offset into the nBmpID in SetupImageList
// Argument    : CString strName - item name to display
// Argument    : CTDMFObject *pDataNode - item data it represents to store for retreival
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
HTREEITEM CBaseTreeView::InsertItemUnique(HTREEITEM htiParent, int nImage, int nSelImage, CString strName, CTDMFObject *pDataNode)
{
	BOOL bFound = FALSE;	

	// First see if it's already in the tree
	HTREEITEM htiSib = m_treeCtrl.GetChildItem(htiParent);
	while (htiSib && !bFound)
	{
		CTDMFObject* pNode = (CTDMFObject*) m_treeCtrl.GetItemData(htiSib);
		if (pDataNode != NULL && pDataNode == pNode)
			bFound = TRUE;
		else
			htiSib = m_treeCtrl.GetNextSiblingItem(htiSib);
	}

	// if already in the tree view do not insert it
	if (bFound)
		return NULL;

	return InsertItem(htiParent, nImage, nSelImage, strName, pDataNode);	
}



/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::ExpandBranch
// Description : - Expands a branch completely
// Returns     : void 
// Argument    : HTREEITEM hti item to expand
//
// Author      : Mario Parent (Tuesday, August 01, 2000)
//
void CBaseTreeView::ExpandBranch( HTREEITEM hti )
{
      if( m_treeCtrl.ItemHasChildren( hti ) && hti != NULL)
		{
                m_treeCtrl.Expand( hti, TVE_EXPAND );
                hti = m_treeCtrl.GetChildItem( hti );
               
			 do
			 {
             ExpandBranch( hti );
          }
			 while( (hti = m_treeCtrl.GetNextSiblingItem( hti )) != NULL );
       }
       m_treeCtrl.EnsureVisible( m_treeCtrl.GetSelectedItem() );
}

/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::SortBranch
// Description : - Sort a branch completely 
// Returns     : void 
// Argument    : HTREEITEM hti item to sort
//
// Author      : Mario Parent (Tuesday, August 01, 2000)
//
void CBaseTreeView::SortBranch( HTREEITEM hti )
{
      if( m_treeCtrl.ItemHasChildren( hti ) && hti != NULL)
		{
          m_treeCtrl.SortChildren( hti );
          
			 hti = m_treeCtrl.GetChildItem( hti );
               
			 do
			 {
             SortBranch( hti );
          }
			 while( (hti = m_treeCtrl.GetNextSiblingItem( hti )) != NULL );
       }
       
}

/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::ReInitialize
// Description : Re-initializes this view, currently not in use
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
void CBaseTreeView::ReInitialize()
{
}


BEGIN_MESSAGE_MAP(CBaseTreeView, CTreeView)
	//{{AFX_MSG_MAP(CBaseTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBaseTreeView diagnostics

#ifdef _DEBUG
void CBaseTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CBaseTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBaseTreeView message handlers


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::OnSelChanged
// Description : Tracks changes in tree item selection. In your derived class
//               Trap this event and call this base implementation
//
// Returns     : void 
// Argument    : NMHDR* pNMHDR - standard windows stuff
// Argument    : LRESULT* pResult
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
void CBaseTreeView::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	m_htiCrntSelected = pNMTreeView->itemNew.hItem;
	
	*pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::OnInitialUpdate
// Description : Initializes this tree view
//               If your derived class handles OnInitialUpdate() call the base
//               implementation as well.
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
void CBaseTreeView::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();
	
	ReInitialize();	
}


/////////////////////////////////////////////////////////////////////////////
// Function    : CBaseTreeView::OnItemExpanded
// Description : Tracks the parent expanding to show children. Call this from
//               your derived class.
//
// Returns     : void 
// Argument    : NMHDR* pNMHDR - standard windows stuff
// Argument    : LRESULT* pResult
//
// Author      : Mario Parent (Thursday, June 15, 2000)
//
void CBaseTreeView::OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int nImage, nSelectedImage;

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM htiExpanded = pNMTreeView->itemNew.hItem;

	if (m_treeCtrl.GetItemImage(htiExpanded, nImage, nSelectedImage))
		switch (pNMTreeView->action)
		{
			case TVE_EXPAND:
				m_treeCtrl.SetItemImage(htiExpanded, nSelectedImage, nSelectedImage);
				break;

			case TVE_COLLAPSE:
				m_treeCtrl.SetItemImage(htiExpanded, nImage, nImage);
				break;
		}
	
	*pResult = 0;
}
