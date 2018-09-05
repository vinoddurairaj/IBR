// ReplicationGroupView.cpp : implementation file
//

#include "stdafx.h"
#include "TdmfCommonGui.h"
#include "TdmfCommonGuiDoc.h"
#include "MainFrm.h"
#include "ReplicationGroupView.h"
#include "ViewNotification.h"
#include "ReplicationGroupContextMenu.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


class CItemData
{
public:
	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;
	//CProgressCtrl m_ProgressCtrl;
};


CString FormatBps(_bstr_t& bstrBps)
{
	CString cstrFormatedValue;

	__int64 nBps = _atoi64(bstrBps);

	double fKbps = ((double)nBps) / 1024;

	if (fKbps > 1024)
	{
		double fMbps = ((double)nBps) / (1024*1024);
		cstrFormatedValue.Format("%.1f MBps", fMbps);
	}
	else
	{
		cstrFormatedValue.Format("%.1f KBps", fKbps);
	}

	return cstrFormatedValue;
}


/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupView

IMPLEMENT_DYNCREATE(CReplicationGroupView, CListView)

CReplicationGroupView::CReplicationGroupView() : m_bInUpdate(FALSE)
{
    m_ImageList.Create(IDB_BITMAP_TREE_ICONS, 16, 16, RGB(0, 128, 128));
}

CReplicationGroupView::~CReplicationGroupView()
{
}

int CReplicationGroupView::SetNewItem(int nIndex, TDMFOBJECTSLib::IReplicationGroup* pRG, bool bInsert)
{
	CListCtrl& ListCtrl  = GetListCtrl();

	try
	{
		CItemData* pItemData = NULL;
        int nImage;
        char* pszState = "";
		if (pRG->Parent->Connected == FALSE)
		{
			pszState = "Disconnected";
            nImage = 14;
		}
		else
		{
			switch (pRG->GetState())
			{
			case TDMFOBJECTSLib::ElementError:
				pszState = "Error";
                nImage =  10;
				break;
			case TDMFOBJECTSLib::ElementWarning:
				pszState = "Warning";
                nImage =  11;
				break;
			case TDMFOBJECTSLib::ElementOk:
                nImage =  12;
				pszState = "Normal";
				break;
			default:
				pszState = "Not Started";
                nImage =  13;
				break;
			}
		}

        if (bInsert)
		{
			// Insert new item
	        nIndex = ListCtrl.InsertItem(nIndex, pszState,nImage);

			// Set Item Data
			pItemData = new CItemData;
			pItemData->m_pRG = pRG;

			// Save Item data
			ListCtrl.SetItemData(nIndex, (DWORD)pItemData);
		}
		else
		{
		   pItemData = (CItemData*)ListCtrl.GetItemData(nIndex);

           ListCtrl.SetItem(nIndex, 0, LVIF_TEXT| LVIF_IMAGE, pszState, nImage, 0, 0, 0 );
           
		}

      

		ListCtrl.SetItemText(nIndex, 1, pRG->Name);        // Name
      
		
		// Set others columns text
		if (pRG->TargetName == pRG->Parent->Name)
		{
			ListCtrl.SetItemText(nIndex, 2, "Loopback");
		}
		else
		{
			ListCtrl.SetItemText(nIndex, 2, pRG->TargetName);      // Target Server's Name
		}

	

		std::ostringstream oss;
		bool bStarted = true;
        int value = pRG->GetMode();
		switch (pRG->GetMode())
		{
		case TDMFOBJECTSLib::FTD_MODE_PASSTHRU:
			oss << "Passthru";
			break;
		case TDMFOBJECTSLib::FTD_MODE_NORMAL:
			oss << "Normal";
			break;
		case TDMFOBJECTSLib::FTD_MODE_TRACKING:
			oss<<  "Tracking";
			break;
		case TDMFOBJECTSLib::FTD_MODE_REFRESH:
			oss << "Refresh (" << pRG->PctDone << "%)";
			break;
		case TDMFOBJECTSLib::FTD_MODE_BACKFRESH:
			oss << "Backfresh (" << pRG->PctDone << "%)";
			break;
		case TDMFOBJECTSLib::FTD_MODE_CHECKPOINT:
			oss << "Checkpoint";
			break;
		case TDMFOBJECTSLib::FTD_M_UNDEF:
			oss << "Not Started";
			bStarted = false;
			break;
		}
		ListCtrl.SetItemText(nIndex, 3, oss.str().c_str());              // Mode
		char* pszStatus = "";
		if (bStarted)
		{
			switch (pRG->GetConnectionStatus())
			{
			case TDMFOBJECTSLib::FTD_PMD_ONLY:
				pszStatus = "PMD Only";
				break;
			case TDMFOBJECTSLib::FTD_CONNECTED:
				pszStatus = "Connected";
				break;
			case TDMFOBJECTSLib::FTD_ACCUMULATE:
				pszStatus = "Accumulate";
				break;
			}
		}
		ListCtrl.SetItemText(nIndex, 4, pszStatus);             // Status
		
		oss.str(std::string());
		oss << (pRG->Sync ? "On" : "Off");
		ListCtrl.SetItemText(nIndex, 5, oss.str().c_str());    // Sync/Async
		
		oss.str(std::string());
		oss << (pRG->Chaining ? "On" : "Off");
		ListCtrl.SetItemText(nIndex, 6, oss.str().c_str());    // Chaining
	

		ListCtrl.SetItemText(nIndex, 7, pRG->StateTimeStamp);  // Time Stamp
		
		oss.str(std::string());
		oss << pRG->GroupNumber;
		ListCtrl.SetItemText(nIndex, 8, oss.str().c_str());    // Group Number

		ListCtrl.SetItemText(nIndex, 9, FormatBps(pRG->ReadKbps));    // Read Kbps
		// Late change: Read is no more in Kbps but in Bps

		ListCtrl.SetItemText(nIndex, 10, FormatBps(pRG->WriteKbps));   // Write Kbps
		// Late change: Write is no more in Kbps but in Bps

		ListCtrl.SetItemText(nIndex, 11, FormatBps(pRG->ActualNet));   // Actual Net

		ListCtrl.SetItemText(nIndex, 12, FormatBps(pRG->EffectiveNet));   // Effective Net

		oss.str(std::string());
		oss << pRG->BABEntries;
		ListCtrl.SetItemText(nIndex, 13, oss.str().c_str());   // Entries

		oss.str(std::string());
		oss << pRG->PctBAB;
		ListCtrl.SetItemText(nIndex, 14, oss.str().c_str());   // % In BAB

		CString cstrPctJournal;

		__int64 liTotalSpace = _atoi64(pRG->JournalSize) + _atoi64(pRG->DiskFreeSize);
		int nPct = 0;
		if (liTotalSpace > 0)
		{
			__int64 liJournalSize = _atoi64(pRG->JournalSize);
			nPct = (int)(((double)liJournalSize / liTotalSpace) * 100);
		}
		cstrPctJournal.Format("%d", nPct);
		ListCtrl.SetItemText(nIndex, 15, cstrPctJournal);      // % Journal Disk Space
	}
	CATCH_ALL_LOG_ERROR(1035);

	return nIndex;
}


BEGIN_MESSAGE_MAP(CReplicationGroupView, CListView)
	//{{AFX_MSG_MAP(CReplicationGroupView)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupView drawing

void CReplicationGroupView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupView diagnostics

#ifdef _DEBUG
void CReplicationGroupView::AssertValid() const
{
	CListView::AssertValid();
}

void CReplicationGroupView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

void CReplicationGroupView::AddColumn(int nIndex, char* szName, UINT nWidth, BOOL bVisible)
{
	CListCtrl& ListCtrl = GetListCtrl();

	ListCtrl.InsertColumn(nIndex, _T(szName));
	ListCtrl.SetColumnWidth(nIndex,  bVisible ? nWidth : 0);

	CListViewColumnDef ColumnDef;
	ColumnDef.m_nIndex   = nIndex;
	ColumnDef.m_bVisible = bVisible;
	ColumnDef.m_strName  = szName;
	ColumnDef.m_nWidth   = nWidth;
	m_vecColumnDef.push_back(ColumnDef);
}

/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupView message handlers

void CReplicationGroupView::OnInitialUpdate()
{
	// this code only works for a report-mode list view
	ASSERT(GetStyle() & LVS_REPORT);
	
	// Gain a reference to the list control itself
	CListCtrl& ListCtrl = GetListCtrl();
	ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
	
    // Set image list
	ListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL );	

	// Insert columns.
	AddColumn(0,  "State",            73, FALSE);
    AddColumn(1,  "Group Id",         81);
	AddColumn(2,  "Remote Server",    109);
	AddColumn(3,  "Mode",             67);
	AddColumn(4,  "Status",           80);
	AddColumn(5,  "Sync Mode",        68);
	AddColumn(6,  "Chaining",         59);
	AddColumn(7,  "Time Stamp",       96);
	AddColumn(8,  "Group Number",     LVSCW_AUTOSIZE_USEHEADER, FALSE);
	AddColumn(9,  "Read",             LVSCW_AUTOSIZE_USEHEADER);
	AddColumn(10, "Write",            LVSCW_AUTOSIZE_USEHEADER);
	AddColumn(11, "Actual Net",       LVSCW_AUTOSIZE_USEHEADER);
	AddColumn(12, "Effective Net",    LVSCW_AUTOSIZE_USEHEADER);
	AddColumn(13, "Entries",          LVSCW_AUTOSIZE_USEHEADER);
	AddColumn(14, "% In BAB",         LVSCW_AUTOSIZE_USEHEADER);
	AddColumn(15, "Journal % Size",   LVSCW_AUTOSIZE_USEHEADER);

	LoadColumnDef();

	CListView::OnInitialUpdate();
}

BOOL CReplicationGroupView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// default is report view and full row selection
	cs.style &= ~LVS_TYPEMASK;
	cs.style |= LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
	return CListView::PreCreateWindow(cs);
}

void CReplicationGroupView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CListCtrl& ListCtrl = GetListCtrl();
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();

	try
	{
		if (lHint == CViewNotification::VIEW_NOTIFICATION)
		{
			m_bInUpdate = TRUE;
			
			CViewNotification* pVN = (CViewNotification*)pHint;
			switch (pVN->m_nMessageId)
			{
			case CViewNotification::SELECTION_CHANGE_SYSTEM:
			case CViewNotification::SELECTION_CHANGE_DOMAIN:
				// Cleanup
				ListCtrl.DeleteAllItems();
				pDoc->SelectReplicationGroup(NULL);
				break;
				
			case CViewNotification::SELECTION_CHANGE_SERVER:
				{
					// Cleanup
					ListCtrl.DeleteAllItems();
					pDoc->SelectReplicationGroup(NULL);
					
					// Populate control
					TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
					long nNb = pServer->ReplicationGroupCount;
					for (long i = 0; i < nNb; i++)
					{
						TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(i); 
						SetNewItem(i, pRG);
					}
                   
				}
				break;
				
			case CViewNotification::SELECTION_CHANGE_RG:
				{
					TDMFOBJECTSLib::IReplicationGroup* pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
					
					int  nNbItem = ListCtrl.GetItemCount();
					int  nIndex  = 0;
					BOOL bFound  = FALSE;
					
					// Find replication group
					while ((nIndex < nNbItem) && (bFound == FALSE))
					{
						TDMFOBJECTSLib::IReplicationGroup* pRGNode = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;
						
						if (pRGNode->IsEqual(pRG))
						{
							ListCtrl.SetItemState(nIndex, LVIS_SELECTED, LVIS_SELECTED);
							ListCtrl.EnsureVisible(nIndex,false);
							bFound = TRUE;
						}
						nIndex++;
					}
				}		
				break;
				
			case CViewNotification::REPLICATION_GROUP_ADD:
				{
					TDMFOBJECTSLib::IReplicationGroup* pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
					
					if ((pDoc->GetSelectedServer() != NULL) && (pDoc->GetSelectedServer()->IsEqual(pRG->Parent)))
					{
						// Append new item
						SetNewItem(ListCtrl.GetItemCount(), pRG);
					}
                    
				}
				break;
				
			case CViewNotification::REPLICATION_GROUP_REMOVE:
				{
					TDMFOBJECTSLib::IReplicationGroup* pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
					
					if ((pDoc->GetSelectedServer() != NULL) && (pDoc->GetSelectedServer()->IsEqual(pRG->Parent)))
					{
						int  nNbItem = ListCtrl.GetItemCount();
						int  nIndex  = 0;
						BOOL bFound  = FALSE;
						// Find replication group
						while ((nIndex < nNbItem) && (bFound == FALSE))
						{
							TDMFOBJECTSLib::IReplicationGroup* pRGNode = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;
							
							if (pRGNode->IsEqual(pRG))
							{
								// Remove server
								ListCtrl.DeleteItem(nIndex);
								
								bFound = TRUE;
							}
							nIndex++;
						}
					}
				}
				break;
				
			case CViewNotification::REPLICATION_GROUP_CHANGE:
				{
					TDMFOBJECTSLib::IReplicationGroup* pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
					int  nNbItem = ListCtrl.GetItemCount();
					int  nIndex  = 0;
					BOOL bFound  = FALSE;
					
					// If the message doesn't contains a Replication Group object,
					// find the RG using its Group Number.
					if (pRG == NULL)
					{
						int nDomainKey   = pVN->m_dwParam1;
						int nSrvId       = pVN->m_dwParam2;
						int nGroupNumber = pVN->m_dwParam3;
						if ((pDoc->GetSelectedServer() != NULL) && (pDoc->GetSelectedServer()->GetKey() == nSrvId))
						{
							// Find replication group
							while (nIndex < nNbItem)
							{
								TDMFOBJECTSLib::IReplicationGroup* pRGNode = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;
								
								if (pRGNode->GroupNumber == nGroupNumber)
								{
									// Update item
									SetNewItem(nIndex, pRGNode, false);
								}
								nIndex++;
							}
                            
						}
					}
					// otherwise the message contains a server object
					else if ((pDoc->GetSelectedServer() != NULL) && (pDoc->GetSelectedServer()->IsEqual(pRG->Parent)))
					{
						// Find replication group
						while ((nIndex < nNbItem) && (bFound == FALSE))
						{
							TDMFOBJECTSLib::IReplicationGroup* pRGNode = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;
							
							if (pRGNode->IsEqual(pRG))
							{
								// Update item
								SetNewItem(nIndex, pRG, false);
								
								bFound = TRUE;
							}
							nIndex++;
						}
					}
				}
				break;
				
			case CViewNotification::TAB_SERVER_NEXT:
			case CViewNotification::TAB_TOOLS_PREVIOUS:
				{
					SetFocus();
				}
				break;
				
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1036);

	m_bInUpdate = FALSE;
	
}

void CReplicationGroupView::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	if (pNMListView->uNewState & LVIS_SELECTED)
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
		pDoc->SelectReplicationGroup(((CItemData*)pNMListView->lParam)->m_pRG);

		if(m_bInUpdate == FALSE)
		{
			CViewNotification VN;
			VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_RG;
			VN.m_pUnk = (IUnknown*)(((CItemData*)pNMListView->lParam)->m_pRG);
			VN.m_eParam = CViewNotification::ITEM;  // Selected Item changed

			pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);

			// Restore focus
			SetFocus();
		}
	}
		
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

void CReplicationGroupView::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CReplicationGroupView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if (GetListCtrl().HitTest(point) == -1)
	{
		return;
	}
	CListView::OnLButtonDblClk(nFlags, point);
}

void CReplicationGroupView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (GetListCtrl().HitTest(point) == -1)
	{
		return;
	}
	CListView::OnLButtonDown(nFlags, point);
}

void CReplicationGroupView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	if (GetListCtrl().HitTest(point) == -1)
	{
		return;
	}
	CListView::OnRButtonDblClk(nFlags, point);
}

void CReplicationGroupView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int nIndex = GetListCtrl().HitTest(point);
	if (nIndex == -1)
	{
		return;
	}

	CListView::OnRButtonDown(nFlags, point);

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
	TDMFOBJECTSLib::IReplicationGroup* pRG = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;

	CReplicationGroupContextMenu RGContextMenu(pDoc, pRG);
	RGContextMenu.Show();
}

void CReplicationGroupView::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	try
	{
		delete (CItemData*)GetListCtrl().GetItemData(pNMListView->iItem);
	}
	CATCH_ALL_LOG_ERROR(1037);

	*pResult = 0;
}

void CReplicationGroupView::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
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
				VN.m_nMessageId = CViewNotification::TAB_REPLICATION_GROUP_PREVIOUS;
			}
			else
			{
				VN.m_nMessageId = CViewNotification::TAB_REPLICATION_GROUP_NEXT;
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

				TDMFOBJECTSLib::IReplicationGroup* pRG = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;

				if (pDoc->GetConnectedFlag() && pRG->IsSource &&
					pRG->Parent->Connected && (pRG->Mode == TDMFOBJECTSLib::FTD_M_UNDEF) &&
					(pDoc->GetReadOnlyFlag() == FALSE))
				{
					CReplicationGroupContextMenu RGContextMenu(pDoc, pRG);
					RGContextMenu.SendCommand(ID_DELETE);
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

					TDMFOBJECTSLib::IReplicationGroup* pRG = ((CItemData*)GetListCtrl().GetItemData(nIndex))->m_pRG;
					
					if (pRG->IsSource)
					{
						CReplicationGroupContextMenu RGContextMenu(pDoc, pRG);
						RGContextMenu.Show(&Point);
					}

					// Don't lose focus
					SetFocus();
				}
			}
		}
		break;
	}

	*pResult = 0;
}

BOOL CReplicationGroupView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
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

void CReplicationGroupView::PopupListViewContextMenu(CPoint pt, int nColumn)
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
                        ListCtrl.SetColumnWidth(it->m_nIndex,LVSCW_AUTOSIZE_USEHEADER);
						//ListCtrl.SetColumnWidth(it->m_nIndex, it->m_nWidth);
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

void CReplicationGroupView::OnDestroy() 
{
	CListView::OnDestroy();
	
	SaveColumnDef();
}

void CReplicationGroupView::SaveColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CReplicationGroupView") != pFrame->m_mapStream.end())
	{
		pFrame->m_mapStream["CReplicationGroupView"]->freeze(0);
		delete pFrame->m_mapStream["CReplicationGroupView"];
	}

	std::ostrstream* poss = new std::ostrstream;
	pFrame->m_mapStream["CReplicationGroupView"] = poss;

	// Save Column info
	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
	{
		int nWidth = GetListCtrl().GetColumnWidth(it->m_nIndex);
		
		pFrame->m_mapStream["CReplicationGroupView"]->write((char*)&(nWidth), sizeof(nWidth));
	}

	// Save column order info
	int* pnIndex = new int[m_vecColumnDef.size()];
	GetListCtrl().GetColumnOrderArray(pnIndex);
	pFrame->m_mapStream["CReplicationGroupView"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
	delete[] pnIndex;
}

void CReplicationGroupView::LoadColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CReplicationGroupView") != pFrame->m_mapStream.end())
	{
		std::istrstream iss(pFrame->m_mapStream["CReplicationGroupView"]->str(), pFrame->m_mapStream["CReplicationGroupView"]->pcount());

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

void CReplicationGroupView::AutoResizeAllVisibleColumns()
{

    CListCtrl& ListCtrl  = GetListCtrl();

    ListCtrl.SetRedraw(false);

  	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
	it != m_vecColumnDef.end() ;it++)
	{
		// Column is shown and we want adjust the width
		if ((ListCtrl.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == TRUE))
		{
			ListCtrl.SetColumnWidth(it->m_nIndex, LVSCW_AUTOSIZE_USEHEADER );
		}
	}
    ListCtrl.SetRedraw(true);
}
