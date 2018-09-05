// ServerView.cpp : implementation file
//

#include "stdafx.h"
#include "TdmfCommonGui.h"
#include "TdmfCommonGuiDoc.h"
#include "MainFrm.h"
#include "ServerView.h"
#include "ServerContextMenu.h"
#include "ViewNotification.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerView

IMPLEMENT_DYNCREATE(CServerView, CListView)

CServerView::CServerView() : m_bInUpdate(FALSE)
{
   m_ImageList.Create(IDB_BITMAP_TREE_ICONS, 16, 16, RGB(0, 128, 128));
}

CServerView::~CServerView()
{
}

int CServerView::SetNewItem(int nIndex, TDMFOBJECTSLib::IServer* pServer, bool bInsert)
{
	CListCtrl& ListCtrl  = GetListCtrl();

	try
	{
        int nImage;
        char* pszState = "";
		if (pServer->Connected == FALSE)
		{
			pszState = "Disconnected";
            nImage = 14;
		}
		else
		{
			switch (pServer->GetState())
			{
			case TDMFOBJECTSLib::ElementError:
				pszState = "Error";
                nImage = 6;
				break;
			case TDMFOBJECTSLib::ElementWarning:
				pszState = "Warning";
                nImage = 7;
				break;
			case TDMFOBJECTSLib::ElementOk:
				pszState = "Normal";
                nImage = 8;
				break;
			default:
				pszState = "Not Started";
                nImage = 9;
				break;
			}
		}
       
       	if (bInsert)
		{
			// Insert new item
	      	nIndex = ListCtrl.InsertItem(nIndex,pszState,nImage);     // Name
        }
		else
		{
            ListCtrl.SetItem(nIndex, 0, LVIF_TEXT| LVIF_IMAGE, pszState, nImage, 0, 0, 0 );
	   }
   

        ListCtrl.SetItemText(nIndex,1,pServer->Name); //Name

		// Set others columns text
		std::ostringstream oss;
		
		ListCtrl.SetItemText(nIndex, 2, pServer->IPAddress[0]);         // IP Address
		oss << pServer->Port;
		ListCtrl.SetItemText(nIndex, 3, oss.str().c_str());          // Port
		
 		
		oss.str(std::string());
		oss << pServer->BABSize << " MB";
		ListCtrl.SetItemText(nIndex, 4, oss.str().c_str());          // BAB Size
		ListCtrl.SetItemText(nIndex, 5, pServer->OSType);            // OS Type
		ListCtrl.SetItemText(nIndex, 6, pServer->OSVersion);         // OS Version
		ListCtrl.SetItemText(nIndex, 7, pServer->AgentVersion);      // Agent Version
		ListCtrl.SetItemText(nIndex, 8, pServer->KeyExpirationDate); // Key Expiration Date
		
		if (bInsert)
		{
			// Save a ptr on the server
			ListCtrl.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IServer*)pServer));
			pServer->AddRef();
		}

        
	}
	CATCH_ALL_LOG_ERROR(1063);

	return nIndex;
}


BEGIN_MESSAGE_MAP(CServerView, CListView)
	//{{AFX_MSG_MAP(CServerView)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerView drawing

void CServerView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CServerView diagnostics

#ifdef _DEBUG
void CServerView::AssertValid() const
{
	CListView::AssertValid();
}

void CServerView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CServerView message handlers

void CServerView::AddColumn(int nIndex, char* szName, UINT nWidth, BOOL bVisible)
{
	CListCtrl& ListCtrl = GetListCtrl();

	ListCtrl.InsertColumn(nIndex, _T(szName));
	ListCtrl.SetColumnWidth(nIndex, bVisible ? nWidth : 0);

	CListViewColumnDef ColumnDef;
	ColumnDef.m_nIndex   = nIndex;
	ColumnDef.m_bVisible = bVisible;
	ColumnDef.m_strName  = szName;
	ColumnDef.m_nWidth   = nWidth;
	m_vecColumnDef.push_back(ColumnDef);
}

void CServerView::OnInitialUpdate()
{
	
	// this code only works for a report-mode list view
	ASSERT(GetStyle() & LVS_REPORT);
	
	// Gain a reference to the list control itself
	CListCtrl& ListCtrl = GetListCtrl();
	ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
    // Set image list
	ListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL );	
	
    // Insert columns.
	AddColumn(0, "State",         73, FALSE);
    AddColumn(1, "Server Name",   80);
	AddColumn(2, "IP Address",    80);
	AddColumn(3, "Port",          38);
	AddColumn(4, "BAB Size",      53);
	AddColumn(5, "OS Type",       75);
	AddColumn(6, "OS Version",    65);
	AddColumn(7, "Replicator Version", 74);
	AddColumn(8, "Key Expiration Date", LVSCW_AUTOSIZE_USEHEADER);

    LoadColumnDef();

  	CListView::OnInitialUpdate();
}

BOOL CServerView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// default is report view and full row selection
	cs.dwExStyle = 0;
	cs.style &= ~LVS_TYPEMASK;
	cs.style |= LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
	return CListView::PreCreateWindow(cs);
}

void CServerView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CListCtrl& ListCtrl  = GetListCtrl();
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();

	if (lHint == CViewNotification::VIEW_NOTIFICATION)
	{
		CViewNotification* pVN = (CViewNotification*)pHint;
		
		switch (pVN->m_nMessageId)
		{
		case CViewNotification::SELECTION_CHANGE_SYSTEM:
			try
			{
				// Cleanup
				ListCtrl.DeleteAllItems();
				pDoc->SelectServer(NULL);
			}
			CATCH_ALL_LOG_ERROR(1064);
			break;
			
		case CViewNotification::SELECTION_CHANGE_DOMAIN:
			try
			{
				// Cleanup
				ListCtrl.DeleteAllItems();
				pDoc->SelectServer(NULL);
				
				// Populate control
				long nNb = pDoc->GetSelectedDomain()->ServerCount;
				for (long i = 0; i < nNb; i++)
				{
					TDMFOBJECTSLib::IServerPtr pServer = pDoc->GetSelectedDomain()->GetServer(i);
					SetNewItem(i, pServer);
				}
                
               
			}
			CATCH_ALL_LOG_ERROR(1065);
			break;
				
		case CViewNotification::SELECTION_CHANGE_SERVER:
			try
			{
				TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
				
				int nNbItem = ListCtrl.GetItemCount();
				int nIndex = 0;
				TDMFOBJECTSLib::IServer* pServerItem = NULL;
				
				while (nIndex < nNbItem)
				{
					pServerItem = (TDMFOBJECTSLib::IServer*)GetListCtrl().GetItemData(nIndex);
					if (pServerItem && (pServerItem->IsEqual(pServer)))
					{
						m_bInUpdate = TRUE;
						ListCtrl.SetItemState(nIndex, LVIS_SELECTED, LVIS_SELECTED);
                        ListCtrl.EnsureVisible(nIndex,false);
                        m_bInUpdate = FALSE;
						break;
					}
					nIndex++;
				}
			}
			CATCH_ALL_LOG_ERROR(1066);
			break;

		case CViewNotification::SERVER_ADD:
			try
			{
				TDMFOBJECTSLib::IServerPtr pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
				if (pServer == NULL)
				{
					for (int nIndex = 0; nIndex < pDoc->m_pSystem->DomainCount; nIndex++)
					{
						TDMFOBJECTSLib::IDomainPtr pDomainCur = pDoc->m_pSystem->GetDomain(nIndex);
						if (pDomainCur->GetKey() == (long)pVN->m_dwParam1)
						{
							for (int nServerIndex = 0; nServerIndex < pDomainCur->ServerCount; nServerIndex++)
							{
								TDMFOBJECTSLib::IServerPtr pServerCur = pDomainCur->GetServer(nServerIndex);
								if (pServerCur->GetKey() == (long)pVN->m_dwParam2)
								{
									pServer = pServerCur;
									break;
								}
							}
							break;
						}
					}
				}

				if ((pDoc->GetSelectedDomain() != NULL) &&
					(pDoc->GetSelectedDomain()->IsEqual(pServer->Parent)))
				{
					// Append new item
					SetNewItem(ListCtrl.GetItemCount(), pServer);
  				}
			}
			CATCH_ALL_LOG_ERROR(1067);
			break;
			
		case CViewNotification::SERVER_REMOVE:
			try
			{
				TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
				
				if ((pDoc->GetSelectedDomain() != NULL) && 
					((pServer == NULL)|| pDoc->GetSelectedDomain()->IsEqual(pServer->Parent)))
				{
					int nNbItem = ListCtrl.GetItemCount();
					int nIndex = 0;
					BOOL bFound = FALSE;
					
					// Find server
					while ((nIndex < nNbItem) && (bFound == FALSE)) 
					{
						TDMFOBJECTSLib::IServer* pServerItem = (TDMFOBJECTSLib::IServer*)ListCtrl.GetItemData(nIndex);
						if ((pServer && pServerItem->IsEqual(pServer)) ||
							(pServerItem->GetKey() == (long)pVN->m_dwParam2))
						{
							// Remove server
							ListCtrl.DeleteItem(nIndex);
							bFound = TRUE;
						}
						nIndex++;
					}
				}
			}
			CATCH_ALL_LOG_ERROR(1068);
			break;
			
		case CViewNotification::SERVER_CHANGE:
			try
			{
				if (pVN->m_eParam != CViewNotification::PERFORMANCE)
				{
					TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
					int nNbItem = ListCtrl.GetItemCount();
					int nIndex = 0;
					BOOL bFound = FALSE;
					
					// If the message doesn't contains a server object,
					// find the server using its HostID.
					if (pServer == NULL)
					{
						long nDomainKey = pVN->m_dwParam1;
						long nServerKey = pVN->m_dwParam2;
						if ((pDoc->GetSelectedDomain() != NULL) &&
							(pDoc->GetSelectedDomain()->GetKey() == nDomainKey))
						{
							// Find server
							while ((nIndex < nNbItem) && (bFound == FALSE)) 
							{
								TDMFOBJECTSLib::IServer* pServerItem = (TDMFOBJECTSLib::IServer*)ListCtrl.GetItemData(nIndex);
								if (pServerItem->GetKey() == nServerKey)
								{
									// Refresh server
									SetNewItem(nIndex, pServerItem, false);
									bFound = TRUE;
								}
								nIndex++;
							}
                            
						}
					}
					// otherwise the message contains a server object
					else if ((pDoc->GetSelectedDomain() != NULL) &&
						(pDoc->GetSelectedDomain()->IsEqual(pServer->Parent)))
					{
						// Find server
						while ((nIndex < nNbItem) && (bFound == FALSE)) 
						{
							TDMFOBJECTSLib::IServer* pServerItem = (TDMFOBJECTSLib::IServer*)ListCtrl.GetItemData(nIndex);
							if (pServerItem->IsEqual(pServer))
							{
								// Refresh server
								SetNewItem(nIndex, pServerItem, false);
								bFound = TRUE;
							}
							nIndex++;
						}
                        
					}
                   
				}
			}
			CATCH_ALL_LOG_ERROR(1069);
			break;
												
		case CViewNotification::TAB_SYSTEM_NEXT:
		case CViewNotification::TAB_REPLICATION_GROUP_PREVIOUS:
			try
			{
				SetFocus();
			}
			CATCH_ALL_LOG_ERROR(1070);
			break;
		}
	}	
}

void CServerView::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();

	try
	{
		if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVIS_SELECTED))
		{
			pDoc->SelectServer((TDMFOBJECTSLib::IServer*)pNMListView->lParam);
			
			if (m_bInUpdate == FALSE)
			{
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_SERVER;
				VN.m_pUnk = (IUnknown*)pNMListView->lParam;
				VN.m_eParam = CViewNotification::ITEM;  // Selected Item changed

				pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				
				// Restore focus
				SetFocus();
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1071);

	*pResult = 0;
}

class CCompareData
{
public:
	CListCtrl* m_pListCtrl;
	int        m_nColumn;
	BOOL       m_bAscending;

	CCompareData() : m_bAscending(FALSE) {}
};

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CCompareData* pCompareData = (CCompareData*)lParamSort;
	CString strItem1 = pCompareData->m_pListCtrl->GetItemText(lParam1, pCompareData->m_nColumn);
	CString strItem2 = pCompareData->m_pListCtrl->GetItemText(lParam2, pCompareData->m_nColumn);

	return strcmp(strItem2, strItem1) * (pCompareData->m_bAscending ? 1 : -1);
}

void CServerView::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CListCtrl& ListCtrl = GetListCtrl();

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &ListCtrl;
	CompareData.m_nColumn = pNMListView->iSubItem;
	CompareData.m_bAscending = !CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	ListCtrl.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);

	*pResult = 0;
}

void CServerView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int nHitItem = GetListCtrl().HitTest(point);

	if (nHitItem == -1)
	{
		return;
	}

	if (nHitItem == GetListCtrl().GetSelectionMark())
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_SERVER;
		VN.m_pUnk = (IUnknown*)GetListCtrl().GetItemData(nHitItem);
		VN.m_eParam = CViewNotification::ITEM;
		pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
	}

	CListView::OnLButtonDown(nFlags, point);
}

void CServerView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if (GetListCtrl().HitTest(point) == -1)
	{
		return;
	}
	CListView::OnLButtonDblClk(nFlags, point);
}

void CServerView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	if (GetListCtrl().HitTest(point) == -1)
	{
		return;
	}	
	CListView::OnRButtonDblClk(nFlags, point);
}

void CServerView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	UINT uFlags;
	int nIndex = GetListCtrl().HitTest(point, &uFlags);
	if (nIndex != -1)
	{
		CListView::OnRButtonDown(nFlags, point);

		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
		TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)GetListCtrl().GetItemData(nIndex);

		CServerContextMenu ServerContextMenu(pDoc, pServer);
		ServerContextMenu.Show();
	}
}

void CServerView::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	try
	{
		IUnknown* pUnk = (IUnknown*)GetListCtrl().GetItemData(pNMListView->iItem);
		pUnk->Release();
	}
	CATCH_ALL_LOG_ERROR(1072);
	
	*pResult = 0;
}

void CServerView::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDown = (LV_KEYDOWN*)pNMHDR;
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();

	switch(pLVKeyDown->wVKey)
	{
	case  VK_TAB:
		{
			CViewNotification VN;
			if (GetKeyState(VK_SHIFT) & ~1)
			{
				VN.m_nMessageId = CViewNotification::TAB_SERVER_PREVIOUS;
			}
			else
			{
				VN.m_nMessageId = CViewNotification::TAB_SERVER_NEXT;
			}
			pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
		}
		break;

	case VK_DELETE:
		{
			POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
			if (pos != NULL)
			{
				int nIndex = GetListCtrl().GetNextSelectedItem(pos);

				TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)GetListCtrl().GetItemData(nIndex);

				if (pDoc->GetConnectedFlag() && (!pServer->Connected) && 
					(pDoc->GetReadOnlyFlag() == FALSE))
				{
					CServerContextMenu ServerContextMenu(pDoc, pServer);
					ServerContextMenu.SendCommand(ID_DELETE);
				}

				// Don't lose focus
				SetFocus();
			}
		}
		break;

	case VK_APPS:
		{
			POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
			if (pos != NULL)
			{
				int nIndex = GetListCtrl().GetNextSelectedItem(pos);

				RECT Rect;
				if (GetListCtrl().GetItemRect(nIndex, &Rect, LVIR_ICON))
				{
					CPoint Point((Rect.right - Rect.left)/2 + Rect.left,
								 (Rect.bottom - Rect.top)/2 + Rect.top);
					ClientToScreen(&Point);

					TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)GetListCtrl().GetItemData(nIndex);

					CServerContextMenu ServerContextMenu(pDoc, pServer);
					ServerContextMenu.Show(&Point);

					// Don't lose focus
					SetFocus();
				}
			}
		}
		break;
	}

	*pResult = 0;
}


BOOL CServerView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	HD_NOTIFY	*pHDN = (HD_NOTIFY*)lParam;

	LPNMHDR pNH = (LPNMHDR) lParam;
	// wParam is zero for Header ctrl
	if( wParam == 0 && pNH->code == NM_RCLICK )
	{
		// Right button was clicked on header
		CPoint pt(GetMessagePos());
		CPoint ptScreen = pt;

		CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
		pHeader->ScreenToClient(&pt);
		
		// Determine the column index
		int index = -1;
		CRect rcCol;
		for (int i=0; Header_GetItemRect(pHeader->m_hWnd, i, &rcCol); i++)
		{
			if (rcCol.PtInRect(pt))
			{
				index = i;
				break;
			}
		}

		// Add your right click action code
		PopupListViewContextMenu(ptScreen, index);
	}
	else if ((wParam == 0) &&
			 ((pNH->code == HDN_BEGINTRACKW) || (pNH->code == HDN_BEGINTRACKA) ||
			  (pNH->code == HDN_DIVIDERDBLCLICKA) || (pNH->code == HDN_DIVIDERDBLCLICKW)))
	{
		if (m_vecColumnDef[pHDN->iItem].m_bVisible == FALSE)
		{
			*pResult = TRUE;                // disable tracking
			return TRUE;                    // Processed message
		}
	}
	else if ((wParam == 0) && ((pNH->code == HDN_ENDTRACKW) || (pNH->code == HDN_ENDTRACKA)))
	{
		CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
		CRect rcCol;
		Header_GetItemRect(pHeader->m_hWnd, pHDN->iItem, &rcCol);

		if (rcCol.Width() == 0)
		{
			m_vecColumnDef[pHDN->iItem].m_bVisible = FALSE;
		}
	}

	return CListView::OnNotify(wParam, lParam, pResult);
}

void CServerView::PopupListViewContextMenu(CPoint pt, int nColumn)
{
	CMenu Menu;
	Menu.LoadMenu(IDR_SERVERVIEW_MENU);

	// Get the submenu(0)
	CMenu* pMenu = Menu.GetSubMenu(0);
	ASSERT(pMenu);

	if (nColumn < 0)
	{
		pMenu->EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_SORTASCENDING,  MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_SORTDESCENDING, MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_HIDE,           MF_BYCOMMAND | MF_GRAYED);
	}

	// Display Context Menu
	UINT nCmd = ::TrackPopupMenuEx(pMenu->m_hMenu,
								   TPM_LEFTALIGN|TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,
								   pt.x, pt.y, m_hWnd, NULL);
		
	// Take action
	CListCtrl& ListCtrl = GetListCtrl();
	switch (nCmd)
	{
	case ID_SERVERVIEWCONTEXTMENU_SORTASCENDING:
		{
			static CCompareData CompareData;
			CompareData.m_pListCtrl = &ListCtrl;
			CompareData.m_nColumn = nColumn;
			CompareData.m_bAscending = FALSE;
			
			#ifndef LVM_SORTITEMSEX
				#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
			#endif
			ListCtrl.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_SORTDESCENDING:
		{
			static CCompareData CompareData;
			CompareData.m_pListCtrl = &ListCtrl;
			CompareData.m_nColumn = nColumn;
			CompareData.m_bAscending = TRUE;
			
			#ifndef LVM_SORTITEMSEX
				#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
			#endif
			ListCtrl.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_HIDE:
		{
			ListCtrl.SetColumnWidth(nColumn, 0);
			m_vecColumnDef[nColumn].m_bVisible = FALSE;
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_COLUMNS:
		{
			CColumnSelectionDlg ColSelDlg(m_vecColumnDef);

			if (ColSelDlg.DoModal() == IDOK)
			{
				for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
					it != m_vecColumnDef.end() ;it++)
				{
					// Column is hiden and we want to show it
					if ((ListCtrl.GetColumnWidth(it->m_nIndex) == 0) && it->m_bVisible)
					{
						ListCtrl.SetColumnWidth(it->m_nIndex, it->m_nWidth);
					}
					// Column is shown and we want to hide it 
					if ((ListCtrl.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == FALSE))
					{
						ListCtrl.SetColumnWidth(it->m_nIndex, 0);
					}
				}
			}
		}
		break;
		case ID_SERVERVIEWCONTEXTMENU_AUTOSIZECOLUMNS:
		{
		
		    AutoResizeAllVisibleColumns();
			
		}
		break;
       
	}

	Menu.DestroyMenu();
}

void CServerView::OnDestroy() 
{
	CListView::OnDestroy();
	
	SaveColumnDef();
}

void CServerView::SaveColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CServerView") != pFrame->m_mapStream.end())
	{
		pFrame->m_mapStream["CServerView"]->freeze(0);
		delete pFrame->m_mapStream["CServerView"];
	}

	std::ostrstream* poss = new std::ostrstream;
	pFrame->m_mapStream["CServerView"] = poss;

	// Save Column info
	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
	{
		int nWidth = GetListCtrl().GetColumnWidth(it->m_nIndex);
		
		pFrame->m_mapStream["CServerView"]->write((char*)&(nWidth), sizeof(nWidth));
	}

	// Save column order info
	int* pnIndex = new int[m_vecColumnDef.size()];
	GetListCtrl().GetColumnOrderArray(pnIndex);
	pFrame->m_mapStream["CServerView"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
	delete[] pnIndex;
}

void CServerView::LoadColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CServerView") != pFrame->m_mapStream.end())
	{
		std::ostrstream* poss = pFrame->m_mapStream["CServerView"];

		std::istrstream iss(pFrame->m_mapStream["CServerView"]->str(), pFrame->m_mapStream["CServerView"]->pcount());

		for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
			it != m_vecColumnDef.end() ;it++)
		{
			int nWidth;
			iss.read((char*)&nWidth, sizeof(int));

			it->m_bVisible = (nWidth != 0);

			GetListCtrl().SetColumnWidth(it->m_nIndex, nWidth);
		}

		int* pnIndex = new int[m_vecColumnDef.size()];
		iss.read((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
		GetListCtrl().SetColumnOrderArray(m_vecColumnDef.size(), pnIndex);
		delete[] pnIndex;
	}
}



void CServerView::AutoResizeAllVisibleColumns()
{
    CListCtrl& ListCtrl  = GetListCtrl();

  	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
	it != m_vecColumnDef.end() ;it++)
	{
		// Column is shown and we want adjust the width
		if ((ListCtrl.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == TRUE))
		{
			ListCtrl.SetColumnWidth(it->m_nIndex, LVSCW_AUTOSIZE_USEHEADER );
		}
	}
}


