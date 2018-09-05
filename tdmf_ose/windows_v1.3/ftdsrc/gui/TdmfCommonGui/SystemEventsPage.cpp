// SystemEventsPage.cpp : implementation file
//


// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "MainFrm.h"
#include "SystemEventsPage.h"
#include "ToolsView.h"
#include "ViewNotification.h"
#include "EventProperties.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemEventsPage property page

IMPLEMENT_DYNCREATE(CSystemEventsPage, CPropertyPage)


// Mike Pollett
#include "../../tdmf.inc"

CSystemEventsPage::CSystemEventsPage()
	: CPropertyPage(CSystemEventsPage::IDD), m_nTimerId(0), m_nRefreshFreq(10000), m_bInit(false), m_pEventProperties(NULL)
{
	//{{AFX_DATA_INIT(CSystemEventsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Read refresh timer from registry
	DWORD dwType;
	char  szValue[MAX_PATH];
	ULONG nSize = MAX_PATH;

	#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
	if (SHGetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "TDMFRefreshEventTimer", &dwType, szValue, &nSize) == ERROR_SUCCESS)
	{
		m_nRefreshFreq = (*(int*)szValue) * 1000;
	}
}

CSystemEventsPage::~CSystemEventsPage()
{
}

void CSystemEventsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemEventsPage)
	DDX_Control(pDX, IDC_LIST1, m_ListCtrlEvent);
	//}}AFX_DATA_MAP
}

void CSystemEventsPage::AddColumn(int nIndex, char* szName, UINT nWidth)
{
	
	m_ListCtrlEvent.InsertColumn(nIndex, _T(szName));
	m_ListCtrlEvent.SetColumnWidth(nIndex, nWidth);

	CListViewColumnDef ColumnDef;
	ColumnDef.m_nIndex   = nIndex;
	ColumnDef.m_bVisible = TRUE;
	ColumnDef.m_strName  = szName;
	ColumnDef.m_nWidth   = nWidth;
	m_vecColumnDef.push_back(ColumnDef);
}


BEGIN_MESSAGE_MAP(CSystemEventsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSystemEventsPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST1, OnGetdispinfoList1)
	ON_NOTIFY(LVN_ODFINDITEM, IDC_LIST1, OnOdfinditemList1)
	ON_WM_TIMER()
	ON_MESSAGE(WM_EVENT_SELECTION_CHANGE, OnEventSelectionChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSystemEventsPage message handlers

BOOL CSystemEventsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_ListCtrlEvent.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	// Resize ListCtrl
	CTabCtrl* pTab = ((CPropertySheet*)GetParent())->GetTabControl();

	RECT Rect;
	pTab->GetClientRect(&Rect);

	m_ListCtrlEvent.MoveWindow(&Rect);

	// Insert columns.
	m_vecColumnDef.clear();
	int nColIndex = 0;

	AddColumn(nColIndex, "Type", 60);
	nColIndex++;

	AddColumn(nColIndex, "Date", 71);
	nColIndex++;

	AddColumn(nColIndex, "Time", 77);
	nColIndex++;

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	if ((pDoc->GetSelectedServer() == NULL) && (pDoc->GetSelectedReplicationGroup() == NULL))
	{
		AddColumn(nColIndex, "Source", 64);
		nColIndex++;
	}

	if (pDoc->GetSelectedReplicationGroup() == NULL)
	{
		AddColumn(nColIndex, "Group Number", 81);
		nColIndex++;
	}

	AddColumn(nColIndex, "Pair Number", 73);
	nColIndex++;

	AddColumn(nColIndex, "Description", LVSCW_AUTOSIZE_USEHEADER);
	nColIndex++;

	LoadColumnDef();

	m_bDisableSetActive     = false;

    m_pImageList = new CImageList();
  

    m_pImageList->Create(IDB_BITMAP_TREE_ICONS, 16, 16, RGB(0, 128, 128) );

 
    m_ListCtrlEvent.SetImageList(m_pImageList, LVSIL_SMALL);


  	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSystemEventsPage::EventInit ()
{
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	if (pDoc->GetSelectedReplicationGroup() != NULL)
	{
		pDoc->GetSelectedReplicationGroup()->GetEventFirst();
	}
	else if (pDoc->GetSelectedServer() != NULL)
	{	
		pDoc->GetSelectedServer()->GetEventFirst();
	}
	else
	{
		pDoc->m_pSystem->GetEventFirst();
	}

} // CSystemEventsPage::EventInit ()


TDMFOBJECTSLib::IEventPtr CSystemEventsPage::GetEventAt ( long pnIndex )
{
	CWaitCursor* lpWaitCursor = NULL;
	TDMFOBJECTSLib::IEventPtr lIeventPtr;
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	if (pDoc->GetSelectedReplicationGroup() != NULL)
	{
		if ( !pDoc->GetSelectedReplicationGroup()->IsEventAt( pnIndex ) )
		{
			lpWaitCursor = new CWaitCursor;
		}

		lIeventPtr = pDoc->GetSelectedReplicationGroup()->GetEventAt( pnIndex );
	}
	else if (pDoc->GetSelectedServer() != NULL)
	{	
		if ( !pDoc->GetSelectedServer()->IsEventAt( pnIndex ) )
		{
			lpWaitCursor = new CWaitCursor;
		}

		lIeventPtr = pDoc->GetSelectedServer()->GetEventAt( pnIndex );
	}
	else
	{
		if ( !pDoc->m_pSystem->IsEventAt( pnIndex ) )
		{
			lpWaitCursor = new CWaitCursor;
		}

		lIeventPtr = pDoc->m_pSystem->GetEventAt( pnIndex );
	}

	if ( lpWaitCursor )
	{
		delete lpWaitCursor;
	}

	return lIeventPtr;

} // CSystemEventsPage::GetEventAt ()


long CSystemEventsPage::GetEventCount ()
{
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	if (pDoc->GetSelectedReplicationGroup() != NULL)
	{
		return pDoc->GetSelectedReplicationGroup()->GetEventCount();
	}
	else if (pDoc->GetSelectedServer() != NULL)
	{
		return pDoc->GetSelectedServer()->GetEventCount();
	}

	return pDoc->m_pSystem->GetEventCount();

} // CSystemEventsPage::GetEventCount ()

void CSystemEventsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);

	if (IsWindow(m_ListCtrlEvent.m_hWnd))
	{
		m_ListCtrlEvent.SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOMOVE);

		int nLastColumnIndex = m_ListCtrlEvent.GetHeaderCtrl()->GetItemCount() - 1;
	}
}

BOOL CSystemEventsPage::OnSetActive () 
{
	if (!m_bDisableSetActive)
	{
		// this->PropertySheet->ToolsView
		((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars(false);
		
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
		
		if (pDoc->GetConnectedFlag())
		{
			m_bInit = false;
			
			m_nEvent = GetEventCount();
			m_ListCtrlEvent.SetItemCountEx(m_nEvent, LVSICF_NOSCROLL);
			
			// Unselect items
			POSITION  pos = m_ListCtrlEvent.GetFirstSelectedItemPosition();
			while (pos != NULL)
			{
				int nSelItem = m_ListCtrlEvent.GetNextSelectedItem(pos);
				m_ListCtrlEvent.SetItemState(nSelItem, 0, LVIS_SELECTED);
			}
		}
		
		m_nTimerId = SetTimer(1, m_nRefreshFreq, NULL);
	}

	return CPropertyPage::OnSetActive();
}


BOOL CSystemEventsPage::OnKillActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();

	if (m_nTimerId != 0)
	{
		KillTimer(m_nTimerId);
	}
	return CPropertyPage::OnKillActive();
}


//////////////////////////////////////////////////////////////////////////////
// Sorting

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


void CSystemEventsPage::OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult) 
{

#ifdef ENABLE_SORT

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &m_ListCtrlEvent;
	CompareData.m_nColumn = pNMListView->iSubItem;
	CompareData.m_bAscending = !CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	m_ListCtrlEvent.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
	
#endif

	*pResult = 0;
}

BOOL CSystemEventsPage::PreTranslateMessage(MSG* pMsg) 
{
	if ((pMsg->message == WM_KEYDOWN) &&
		(pMsg->wParam == VK_TAB))
	{
		if (!(GetKeyState(VK_SHIFT) & ~1))
		{
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
			CViewNotification  VN;
			
			VN.m_nMessageId = CViewNotification::TAB_TOOLS_NEXT;
			pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
			
			return TRUE;
		}
	}
	else if ((pMsg->message == WM_KEYDOWN) &&
			 (pMsg->wParam == 'C'))
	{
		if (GetKeyState(VK_CONTROL) < 0)
		{
			if (m_ListCtrlEvent.GetSelectedCount() > 0)
			{
				if (OpenClipboard())
				{
					EmptyClipboard();

					int nItem = -1;
					CString cstrMsg;
					int nNbColumn = m_ListCtrlEvent.GetHeaderCtrl()->GetItemCount();

					while ((nItem = m_ListCtrlEvent.GetNextItem(nItem, LVNI_SELECTED)) != -1)
					{
						for (int i = 0; i < nNbColumn; i++)
						{
							cstrMsg += m_ListCtrlEvent.GetItemText(nItem, i);
							cstrMsg += " ";
						}
						cstrMsg += "\r\n";
					}

					// Allocate a global memory object for the text.
					HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, cstrMsg.GetLength() + 1);

					if (hglbCopy != NULL)
					{ 
					    // Lock the handle and copy the text to the buffer.
					       LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
						strcpy(lptstrCopy, cstrMsg);
						GlobalUnlock(hglbCopy);
 
						// Place the handle on the clipboard.
						SetClipboardData(CF_TEXT, hglbCopy);
					}

					CloseClipboard();
				}
			}
		}
	}

	return CPropertyPage::PreTranslateMessage(pMsg);
}


void CSystemEventsPage::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	CEventProperties EventProperties(pDoc, m_nEvent - pNMListView->iItem, m_nEvent, this);
	m_pEventProperties = &EventProperties;
	EventProperties.DoModal();
	m_pEventProperties = NULL;

	*pResult = 0;
}

BOOL CSystemEventsPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	HD_NOTIFY	*pHDN = (HD_NOTIFY*)lParam;

	LPNMHDR pNH = (LPNMHDR) lParam;
	// wParam is zero for Header ctrl
	if( wParam == 0 && pNH->code == NM_RCLICK )
	{
		// Right button was clicked on header
		CPoint pt(GetMessagePos());
		CPoint ptScreen = pt;

		CHeaderCtrl* pHeader = (CHeaderCtrl*)m_ListCtrlEvent.GetDlgItem(0);
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
		CHeaderCtrl* pHeader = (CHeaderCtrl*)m_ListCtrlEvent.GetDlgItem(0);
		CRect rcCol;
		Header_GetItemRect(pHeader->m_hWnd, pHDN->iItem, &rcCol);

		if (rcCol.Width() == 0)
		{
			m_vecColumnDef[pHDN->iItem].m_bVisible = FALSE;
		}
	}

	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}

void CSystemEventsPage::PopupListViewContextMenu(CPoint pt, int nColumn)
{
	CMenu Menu;
	Menu.LoadMenu(IDR_SERVERVIEW_MENU);

	// Get the submenu(0)
	CMenu* pMenu = Menu.GetSubMenu(0);
	ASSERT(pMenu);

#ifndef ENABLE_SORT
	pMenu->EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_SORTASCENDING,  MF_BYCOMMAND | MF_GRAYED);
	pMenu->EnableMenuItem(ID_SERVERVIEWCONTEXTMENU_SORTDESCENDING, MF_BYCOMMAND | MF_GRAYED);
#endif

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
	switch (nCmd)
	{
	case ID_SERVERVIEWCONTEXTMENU_SORTASCENDING:
		{
			static CCompareData CompareData;
			CompareData.m_pListCtrl = &m_ListCtrlEvent;
			CompareData.m_nColumn = nColumn;
			CompareData.m_bAscending = FALSE;
			
			#ifndef LVM_SORTITEMSEX
				#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
			#endif
			m_ListCtrlEvent.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_SORTDESCENDING:
		{
			static CCompareData CompareData;
			CompareData.m_pListCtrl = &m_ListCtrlEvent;
			CompareData.m_nColumn = nColumn;
			CompareData.m_bAscending = TRUE;
			
			#ifndef LVM_SORTITEMSEX
				#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
			#endif
			m_ListCtrlEvent.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_HIDE:
		{
			m_ListCtrlEvent.SetColumnWidth(nColumn, 0);
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
					if ((m_ListCtrlEvent.GetColumnWidth(it->m_nIndex) == 0) && it->m_bVisible)
					{
						m_ListCtrlEvent.SetColumnWidth(it->m_nIndex, it->m_nWidth);
					}
					// Column is shown and we want to hide it 
					if ((m_ListCtrlEvent.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == FALSE))
					{
						m_ListCtrlEvent.SetColumnWidth(it->m_nIndex, 0);
					}
				}
			}
		}
		break;
		case ID_SERVERVIEWCONTEXTMENU_AUTOSIZECOLUMNS:
		{
		
			for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
			it != m_vecColumnDef.end() ;it++)
			{
				// Column is shown and we want adjust the width
				if ((m_ListCtrlEvent.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == TRUE))
				{
					m_ListCtrlEvent.SetColumnWidth(it->m_nIndex, LVSCW_AUTOSIZE_USEHEADER );
				}
			}
			
		}
		break;
	}

	Menu.DestroyMenu();
}

void CSystemEventsPage::OnDestroy()
{
    if(m_pImageList)
        delete m_pImageList;

	CPropertyPage::OnDestroy();

	SaveColumnDef();
}

void CSystemEventsPage::SaveColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CSystemEventsPage") != pFrame->m_mapStream.end())
	{
		pFrame->m_mapStream["CSystemEventsPage"]->freeze(0);
		delete pFrame->m_mapStream["CSystemEventsPage"];
	}

	std::ostrstream* poss = new std::ostrstream;
	pFrame->m_mapStream["CSystemEventsPage"] = poss;

	// Save Column info
	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
	{
		int nWidth = m_ListCtrlEvent.GetColumnWidth(it->m_nIndex);
		
		pFrame->m_mapStream["CSystemEventsPage"]->write((char*)&(nWidth), sizeof(nWidth));
	}

	// Save column order info
	int* pnIndex = new int[m_vecColumnDef.size()];
	m_ListCtrlEvent.GetColumnOrderArray(pnIndex);
	pFrame->m_mapStream["CSystemEventsPage"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
	delete[] pnIndex;
}

void CSystemEventsPage::LoadColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CSystemEventsPage") != pFrame->m_mapStream.end())
	{
		std::istrstream iss(pFrame->m_mapStream["CSystemEventsPage"]->str(), pFrame->m_mapStream["CSystemEventsPage"]->pcount());

		for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
			it != m_vecColumnDef.end() ;it++)
		{
			int nWidth;
			iss.read((char*)&nWidth, sizeof(int));

			it->m_bVisible = (nWidth != 0);

			m_ListCtrlEvent.SetColumnWidth(it->m_nIndex, nWidth);
		}

		int* pnIndex = new int[m_vecColumnDef.size()];
		iss.read((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
		m_ListCtrlEvent.SetColumnOrderArray(m_vecColumnDef.size(), pnIndex);
		delete[] pnIndex;
	}
}

void CSystemEventsPage::OnGetdispinfoList1 ( NMHDR* pNMHDR, LRESULT* pResult ) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM*     pItem     = &(pDispInfo)->item;
	int          iItem     = pItem->iItem;

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	if (!m_bInit)
	{
		EventInit();
		m_bInit = true;
	}

	TDMFOBJECTSLib::IEventPtr pEvent;
	pEvent = GetEventAt(m_nEvent - pItem->iItem);
   
 	std::ostringstream oss;

	if ( ( pEvent != NULL ) && ( pItem->mask & LVIF_IMAGE ))
	{

        
        BSTR szType;
        pEvent->get_Type(&szType);
        CString strType = (BSTR) szType;   

        if (strType == _T("FATAL"))
        {
          pItem->iImage = 19;
        }
        else if(strType == _T("WARNING"))
            {
               pItem->iImage = 20;
            }
            else 
            {
               pItem->iImage = 21;
            }
            
    }

 	if ( ( pEvent != NULL ) && ( pItem->mask & LVIF_TEXT ))
	{
		// then display the appropriate column
		switch ( pItem->iSubItem )
		{
		case 0:
			lstrcpy ( pItem->pszText, pEvent->Type );
            
 			break;

		case 1:
			lstrcpy ( pItem->pszText, pEvent->Date );
			break;

		case 2:
			lstrcpy ( pItem->pszText, pEvent->Time );
			break;

		case 3:
			if ((pDoc->GetSelectedServer() == NULL) && (pDoc->GetSelectedReplicationGroup() == NULL))
			{
				lstrcpy ( pItem->pszText, pEvent->Source );
			}
			else if (pDoc->GetSelectedReplicationGroup() == NULL)
			{
				if (pEvent->GroupID >= 0)
				{
					oss << pEvent->GroupID;
				}

				lstrcpy ( pItem->pszText, oss.str().c_str() );
			}
			else
			{
				if (pEvent->PairID >= 0)
				{
					oss << pEvent->PairID;
				}

				lstrcpy ( pItem->pszText, oss.str().c_str() );
			}
			break;

		case 4:
			if ((pDoc->GetSelectedServer() == NULL) && (pDoc->GetSelectedReplicationGroup() == NULL))
			{
				if (pEvent->GroupID >= 0)
				{
					oss << pEvent->GroupID;
				}

				lstrcpy ( pItem->pszText, oss.str().c_str() );
			}
			else if (pDoc->GetSelectedReplicationGroup() == NULL)
			{
				if (pEvent->PairID >= 0)
				{
					oss << pEvent->PairID;
				}

				lstrcpy ( pItem->pszText, oss.str().c_str() );
			}
			else
			{
				lstrcpy ( pItem->pszText, pEvent->Description );
			}
			break;

		case 5:
			if ((pDoc->GetSelectedServer() == NULL) && (pDoc->GetSelectedReplicationGroup() == NULL))
			{
				if (pEvent->PairID >= 0)
				{
					oss << pEvent->PairID;
				}

				lstrcpy ( pItem->pszText, oss.str().c_str() );
			}
			else if (pDoc->GetSelectedReplicationGroup() == NULL)
			{
				lstrcpy ( pItem->pszText, pEvent->Description );
			}
			break;

		case 6:
			if ((pDoc->GetSelectedServer() == NULL) && (pDoc->GetSelectedReplicationGroup() == NULL))
			{
				lstrcpy ( pItem->pszText, pEvent->Description );
			}
			break;

		default:
			ASSERT(0);
			break;
		}
	}

	*pResult = 0;

	// Reset timer
	if (m_nTimerId != 0)
	{
		KillTimer(m_nTimerId);
	}
	m_nTimerId = SetTimer(1, m_nRefreshFreq, NULL);

} // CSystemEventsPage::OnGetdispinfoList1 ()

void CSystemEventsPage::OnOdfinditemList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)pNMHDR;
	
	int nItem = -1;

	if (m_ListCtrlEvent.GetSelectedCount() > 0)
	{
		nItem = m_ListCtrlEvent.GetNextItem(-1, LVNI_SELECTED);
	}

	*pResult = nItem;
}

void CSystemEventsPage::OnTimer(UINT nIDEvent) 
{
	int nSelectedPos = -1;
	POSITION  pos = m_ListCtrlEvent.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		nSelectedPos = m_ListCtrlEvent.GetNextSelectedItem(pos);
	}

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	if (pDoc->GetConnectedFlag())
	{
		long lnEvent = GetEventCount();
		if (lnEvent > m_nEvent)
		{
			// cache info
			GetEventAt(lnEvent);

			ShowWindow(SW_HIDE);
			
			for (; lnEvent > m_nEvent; m_nEvent++)
			{
				m_ListCtrlEvent.InsertItem(0, "XXX");
				
				if (nSelectedPos > -1)
				{
					nSelectedPos++;
					
					m_ListCtrlEvent.EnsureVisible(nSelectedPos, FALSE);
				}
			}
			
			ShowWindow(SW_SHOWNORMAL);

			// Notify EventProperties dialog
			if (m_pEventProperties != NULL)
			{
				m_pEventProperties->SetMaxEvent(m_nEvent);
			}
		}
	}

	CPropertyPage::OnTimer(nIDEvent);
}

LRESULT CSystemEventsPage::OnEventSelectionChange(WPARAM wParam, LPARAM lParam)
{
	if (m_ListCtrlEvent.GetSelectedCount() > 0)
	{
		int nItemPrev = m_ListCtrlEvent.GetNextItem(-1, LVNI_SELECTED);
		int nItemNew  = nItemPrev + wParam;

		if (nItemNew > -1)
		{
			m_ListCtrlEvent.SetItemState(nItemPrev, 0, LVIS_SELECTED|LVIS_FOCUSED);
			m_ListCtrlEvent.SetItemState(nItemNew, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
			m_ListCtrlEvent.EnsureVisible(nItemNew, FALSE);
		}
	}

	return TRUE;
}
