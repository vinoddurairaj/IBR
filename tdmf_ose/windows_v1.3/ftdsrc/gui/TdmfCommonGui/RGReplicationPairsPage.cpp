// RGReplicationPairsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "MainFrm.h"
#include "RGReplicationPairsPage.h"
#include "ToolsView.h"
#include "ViewNotification.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CRGReplicationPairsPage property page

IMPLEMENT_DYNCREATE(CRGReplicationPairsPage, CPropertyPage)

CRGReplicationPairsPage::CRGReplicationPairsPage() : CPropertyPage(CRGReplicationPairsPage::IDD), m_bInUpdate(FALSE)
{
	//{{AFX_DATA_INIT(CRGReplicationPairsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRGReplicationPairsPage::~CRGReplicationPairsPage()
{
}

void CRGReplicationPairsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGReplicationPairsPage)
	DDX_Control(pDX, IDC_LIST_RP, m_ListCtrlReplicationPair);
	//}}AFX_DATA_MAP
}

void CRGReplicationPairsPage::AddColumn(int nIndex, char* szName, UINT nWidth)
{
	
	m_ListCtrlReplicationPair.InsertColumn(nIndex, _T(szName));
	m_ListCtrlReplicationPair.SetColumnWidth(nIndex, nWidth);

	CListViewColumnDef ColumnDef;
	ColumnDef.m_nIndex   = nIndex;
	ColumnDef.m_bVisible = TRUE;
	ColumnDef.m_strName  = szName;
	ColumnDef.m_nWidth   = nWidth;
	m_vecColumnDef.push_back(ColumnDef);
}


BEGIN_MESSAGE_MAP(CRGReplicationPairsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGReplicationPairsPage)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_RP, OnDeleteitemListRp)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_RP, OnColumnclick)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RP, OnItemchangedListRp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGReplicationPairsPage message handlers

BOOL CRGReplicationPairsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_ListCtrlReplicationPair.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	// Resize ListCtrl
	CTabCtrl* pTab = ((CPropertySheet*)GetParent())->GetTabControl();

	RECT Rect;
	pTab->GetClientRect(&Rect);

	m_ListCtrlReplicationPair.MoveWindow(&Rect);

	m_vecColumnDef.clear();

	// Insert columns.
	AddColumn(0, "Source",      60);
	AddColumn(1, "Source Size", 60);
	AddColumn(2, "Target",      60);
	AddColumn(3, "Target Size", 60);

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	m_bWindows = (strstr(pDoc->GetSelectedServer()->OSType, "Windows") != 0 ||
				  strstr(pDoc->GetSelectedServer()->OSType, "windows") != 0 ||
				  strstr(pDoc->GetSelectedServer()->OSType, "WINDOWS") != 0);

	m_bLinux = (strstr(pDoc->GetSelectedServer()->OSType, "Linux") != 0 ||
				strstr(pDoc->GetSelectedServer()->OSType, "linux") != 0 ||
				strstr(pDoc->GetSelectedServer()->OSType, "LINUX") != 0);



	if (m_bWindows)
	{
		AddColumn(4, "File System", 70);
		AddColumn(5, "Description", 319);
	}
	else
	{
		AddColumn(4,"DTC Device", 70);
		AddColumn(5, "Description", 319);
	}
	
	LoadColumnDef();

	RefreshValues();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRGReplicationPairsPage::RefreshValues()
{
	try
	{
		if (m_ListCtrlReplicationPair.m_hWnd != NULL)
		{
			m_ListCtrlReplicationPair.DeleteAllItems();
			
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
			
			if (pDoc->GetSelectedReplicationGroup() != NULL)
			{
				TDMFOBJECTSLib::IReplicationPairPtr pRP;
				
				long nNb = pDoc->GetSelectedReplicationGroup()->ReplicationPairCount;
				for (long i = 0; i < nNb; i++)
				{
					pRP = pDoc->GetSelectedReplicationGroup()->GetReplicationPair(i);
					SetNewItem(i, pRP);
				}
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1009);
}

int CRGReplicationPairsPage::SetNewItem(int nIndex, TDMFOBJECTSLib::IReplicationPair* pRP, bool bInsert)
{
	try
	{
		if (bInsert)
		{
			// Insert new item
			nIndex = m_ListCtrlReplicationPair.InsertItem(nIndex, pRP->SrcName); // Source
		}
		else
		{
			m_ListCtrlReplicationPair.SetItemText(nIndex, 0, pRP->SrcName);      // Source
		}
		
		// Set others columns text
		CString cstrNumber;

		cstrNumber.Format("%.1f MB", (atof(pRP->SrcSize)/1024)/1024);
		m_ListCtrlReplicationPair.SetItemText(nIndex, 1, cstrNumber);         // Source Size

		m_ListCtrlReplicationPair.SetItemText(nIndex, 2, pRP->TgtName);       // Target

		double f = (atof(pRP->TgtSize)/1024)/1024;
		if(f > 0)
			cstrNumber.Format("%.1f MB", (atof(pRP->TgtSize)/1024)/1024);
		else
			cstrNumber = _T("Unknown");

		m_ListCtrlReplicationPair.SetItemText(nIndex, 3, cstrNumber);         // Target Size

		if (m_bWindows)
		{
			m_ListCtrlReplicationPair.SetItemText(nIndex, 4, pRP->FileSystem);    // File System

			m_ListCtrlReplicationPair.SetItemText(nIndex, 5, pRP->Description);   // Description
		}
		else
		{
			CString strValue;

			if (m_bLinux)
			{
				strValue.Format("/dev/dtc/lg%d/dsk/dtc%d",pRP->GetParent()->GroupNumber,  pRP->PairNumber);
			}
			else
			{
				strValue.Format("/dev/dtc/lg%d/rdsk/dtc%d",pRP->GetParent()->GroupNumber,  pRP->PairNumber);
			}
				
			m_ListCtrlReplicationPair.SetItemText(nIndex, 4, strValue);    //Device
			m_ListCtrlReplicationPair.SetItemText(nIndex, 5,pRP->Description);// Description
		}

		if (bInsert)
		{
			// Save a ptr on the server
			m_ListCtrlReplicationPair.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IReplicationPair*)pRP));
			pRP->AddRef();
		}
	}
	CATCH_ALL_LOG_ERROR(1010);

	return nIndex;
}


void CRGReplicationPairsPage::OnDeleteitemListRp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	try
	{
		IUnknown* pUnk = (IUnknown*)m_ListCtrlReplicationPair.GetItemData(pNMListView->iItem);
		pUnk->Release();
	}
	CATCH_ALL_LOG_ERROR(1011);

	*pResult = 0;
}

void CRGReplicationPairsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	
	if (IsWindow(m_ListCtrlReplicationPair.m_hWnd))
	{
		m_ListCtrlReplicationPair.SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOMOVE);
	}	
}

BOOL CRGReplicationPairsPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();

	// this->PropertySheet->ToolsView
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars(false);
	
	return CPropertyPage::OnSetActive();
}

BOOL CRGReplicationPairsPage::OnKillActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();
	
	return CPropertyPage::OnKillActive();
}

BOOL CRGReplicationPairsPage::SelectPair(TDMFOBJECTSLib::IReplicationPair* pRP)
{
	for (int nIndex = 0; nIndex < m_ListCtrlReplicationPair.GetItemCount(); nIndex++)
	{
		TDMFOBJECTSLib::IReplicationPair* pRPItem = (TDMFOBJECTSLib::IReplicationPair*)m_ListCtrlReplicationPair.GetItemData(nIndex);

		if (pRPItem->IsEqual(pRP))
		{
			m_bInUpdate = TRUE;
			m_ListCtrlReplicationPair.SetItemState(nIndex, LVIS_SELECTED, LVIS_SELECTED);
			m_bInUpdate = FALSE;
			break;
		}
	}

	return TRUE;
}

void CRGReplicationPairsPage::OnItemchangedListRp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	try
	{
		if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVIS_SELECTED))
		{
			if (m_bInUpdate == FALSE)
			{
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_RP;
				VN.m_pUnk = (IUnknown*)pNMListView->lParam;
				VN.m_eParam = CViewNotification::ITEM;  // Selected Item changed

				pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
				
				// Restore focus
				SetFocus();
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1012);

	*pResult = 0;
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

void CRGReplicationPairsPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &m_ListCtrlReplicationPair;
	CompareData.m_nColumn = pNMListView->iSubItem;
	CompareData.m_bAscending = !CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	m_ListCtrlReplicationPair.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
	
	*pResult = 0;
}

BOOL CRGReplicationPairsPage::PreTranslateMessage(MSG* pMsg) 
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
	
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CRGReplicationPairsPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	HD_NOTIFY	*pHDN = (HD_NOTIFY*)lParam;

	LPNMHDR pNH = (LPNMHDR) lParam;
	// wParam is zero for Header ctrl
	if( wParam == 0 && pNH->code == NM_RCLICK )
	{
		// Right button was clicked on header
		CPoint pt(GetMessagePos());
		CPoint ptScreen = pt;

		CHeaderCtrl* pHeader = (CHeaderCtrl*)m_ListCtrlReplicationPair.GetDlgItem(0);
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
		CHeaderCtrl* pHeader = (CHeaderCtrl*)m_ListCtrlReplicationPair.GetDlgItem(0);
		CRect rcCol;
		Header_GetItemRect(pHeader->m_hWnd, pHDN->iItem, &rcCol);

		if (rcCol.Width() == 0)
		{
			m_vecColumnDef[pHDN->iItem].m_bVisible = FALSE;
		}
	}
	
	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}

void CRGReplicationPairsPage::PopupListViewContextMenu(CPoint pt, int nColumn)
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
	switch (nCmd)
	{
	case ID_SERVERVIEWCONTEXTMENU_SORTASCENDING:
		{
			static CCompareData CompareData;
			CompareData.m_pListCtrl = &m_ListCtrlReplicationPair;
			CompareData.m_nColumn = nColumn;
			CompareData.m_bAscending = FALSE;
			
			#ifndef LVM_SORTITEMSEX
				#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
			#endif
			m_ListCtrlReplicationPair.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_SORTDESCENDING:
		{
			static CCompareData CompareData;
			CompareData.m_pListCtrl = &m_ListCtrlReplicationPair;
			CompareData.m_nColumn = nColumn;
			CompareData.m_bAscending = TRUE;
			
			#ifndef LVM_SORTITEMSEX
				#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
			#endif
			m_ListCtrlReplicationPair.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
		}
		break;

	case ID_SERVERVIEWCONTEXTMENU_HIDE:
		{
			m_ListCtrlReplicationPair.SetColumnWidth(nColumn, 0);
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
					if ((m_ListCtrlReplicationPair.GetColumnWidth(it->m_nIndex) == 0) && it->m_bVisible)
					{
						m_ListCtrlReplicationPair.SetColumnWidth(it->m_nIndex, it->m_nWidth);
					}
					// Column is shown and we want to hide it 
					if ((m_ListCtrlReplicationPair.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == FALSE))
					{
						m_ListCtrlReplicationPair.SetColumnWidth(it->m_nIndex, 0);
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
				if ((m_ListCtrlReplicationPair.GetColumnWidth(it->m_nIndex) > 0) && (it->m_bVisible == TRUE))
				{
					m_ListCtrlReplicationPair.SetColumnWidth(it->m_nIndex, LVSCW_AUTOSIZE_USEHEADER );
				}
			}
			
		}
		break;
		
	}

	Menu.DestroyMenu();
}

void CRGReplicationPairsPage::OnDestroy() 
{
	SaveColumnDef();

	CPropertyPage::OnDestroy();	
}

void CRGReplicationPairsPage::SaveColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (m_bWindows)
	{
		if (pFrame->m_mapStream.find("CRGReplicationPairsPage") != pFrame->m_mapStream.end())
		{
			pFrame->m_mapStream["CRGReplicationPairsPage"]->freeze(0);
			delete pFrame->m_mapStream["CRGReplicationPairsPage"];
		}
		
		std::ostrstream* poss = new std::ostrstream;
		pFrame->m_mapStream["CRGReplicationPairsPage"] = poss;
		
		// Save Column info
		for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
		{
			int nWidth = m_ListCtrlReplicationPair.GetColumnWidth(it->m_nIndex);
			
			pFrame->m_mapStream["CRGReplicationPairsPage"]->write((char*)&(nWidth), sizeof(nWidth));
		}
		
		// Save column order info
		int* pnIndex = new int[m_vecColumnDef.size()];
		m_ListCtrlReplicationPair.GetColumnOrderArray(pnIndex);
		pFrame->m_mapStream["CRGReplicationPairsPage"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
		delete[] pnIndex;
	}
	else // dirty quick patch
	{
		if (pFrame->m_mapStream.find("CRGReplicationPairsPageUnix") != pFrame->m_mapStream.end())
		{
			pFrame->m_mapStream["CRGReplicationPairsPageUnix"]->freeze(0);
			delete pFrame->m_mapStream["CRGReplicationPairsPageUnix"];
		}
		
		std::ostrstream* poss = new std::ostrstream;
		pFrame->m_mapStream["CRGReplicationPairsPageUnix"] = poss;
		
		// Save Column info
		for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
		{
			int nWidth = m_ListCtrlReplicationPair.GetColumnWidth(it->m_nIndex);
			
			pFrame->m_mapStream["CRGReplicationPairsPageUnix"]->write((char*)&(nWidth), sizeof(nWidth));
		}
		
		// Save column order info
		int* pnIndex = new int[m_vecColumnDef.size()];
		m_ListCtrlReplicationPair.GetColumnOrderArray(pnIndex);
		pFrame->m_mapStream["CRGReplicationPairsPageUnix"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
		delete[] pnIndex;
	}
}

void CRGReplicationPairsPage::LoadColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	
	if (m_bWindows)
	{		
		if (pFrame->m_mapStream.find("CRGReplicationPairsPage") != pFrame->m_mapStream.end())
		{
			std::istrstream iss(pFrame->m_mapStream["CRGReplicationPairsPage"]->str(), pFrame->m_mapStream["CRGReplicationPairsPage"]->pcount());
			
			for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
			it != m_vecColumnDef.end() ;it++)
			{
				int nWidth;
				iss.read((char*)&nWidth, sizeof(int));
				
				it->m_bVisible = (nWidth != 0);
				
				m_ListCtrlReplicationPair.SetColumnWidth(it->m_nIndex, nWidth);
			}
			
			int* pnIndex = new int[m_vecColumnDef.size()];
			iss.read((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
			m_ListCtrlReplicationPair.SetColumnOrderArray(m_vecColumnDef.size(), pnIndex);
			delete[] pnIndex;
		}
	}
	else
	{
		if (pFrame->m_mapStream.find("CRGReplicationPairsPageUnix") != pFrame->m_mapStream.end())
		{
			std::istrstream iss(pFrame->m_mapStream["CRGReplicationPairsPageUnix"]->str(), pFrame->m_mapStream["CRGReplicationPairsPageUnix"]->pcount());
			
			for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
			it != m_vecColumnDef.end() ;it++)
			{
				int nWidth;
				iss.read((char*)&nWidth, sizeof(int));
				
				it->m_bVisible = (nWidth != 0);
				
				m_ListCtrlReplicationPair.SetColumnWidth(it->m_nIndex, nWidth);
			}
			
			int* pnIndex = new int[m_vecColumnDef.size()];
			iss.read((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
			m_ListCtrlReplicationPair.SetColumnOrderArray(m_vecColumnDef.size(), pnIndex);
			delete[] pnIndex;
		}
	}
}
