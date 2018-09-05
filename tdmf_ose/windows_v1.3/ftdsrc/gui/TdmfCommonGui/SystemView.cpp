// SystemView.cpp : implementation file
//


// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "TdmfCommonGuiDoc.h"
#include "SystemView.h"
#include "SystemContextMenu.h"
#include "DomainContextMenu.h"
#include "ServerContextMenu.h"
#include "ReplicationGroupContextMenu.h"
#include "ViewNotification.h"
#include "ServerWarningDlg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSystemView

IMPLEMENT_DYNCREATE(CSystemView, CView)

CSystemView::CSystemView() : m_bInUpdate(false), m_nCurrentDomain(-1), m_nCurrentServer(-1)
{
	m_ImageList.Create(IDB_BITMAP_TREE_ICONS, 16, 4, RGB(0, 128, 128));
}

CSystemView::~CSystemView()
{
}

void CSystemView::SetItemImage(HTREEITEM hTreeItem)
{
	CTreeCtrl& TreeCtrl = GetTreeCtrl();

	try
	{
		IUnknown* pUnk = (IUnknown*)TreeCtrl.GetItemData(hTreeItem);
		
		TDMFOBJECTSLib::IReplicationPairPtr  pRP;
		TDMFOBJECTSLib::IReplicationGroupPtr pRG;
		TDMFOBJECTSLib::IServerPtr pServer;
		TDMFOBJECTSLib::IDomainPtr pDomain;
		TDMFOBJECTSLib::ISystemPtr pSystem;

		if ((pRP = pUnk) != NULL)
		{
			// 15 - 18
			try
			{
				TreeCtrl.SetItemImage(hTreeItem, 18, 18);
			}
			CATCH_ALL_LOG_ERROR(1078);
		}
		else if ((pRG = pUnk) != NULL)
		{
			// 10 - 13
			try
			{
				if (pRG->Symmetric == FALSE)
				{
					switch (pRG->GetState())
					{
					case TDMFOBJECTSLib::ElementError:
						TreeCtrl.SetItemImage(hTreeItem, 10, 10);
						break;
					case TDMFOBJECTSLib::ElementWarning:
						TreeCtrl.SetItemImage(hTreeItem, 11, 11);
						break;
					case TDMFOBJECTSLib::ElementOk:
						TreeCtrl.SetItemImage(hTreeItem, 12, 12);
						break;
					default:
						TreeCtrl.SetItemImage(hTreeItem, 13, 13);
						break;
					}
				}
				else
				{
					switch (pRG->GetState())
					{
					case TDMFOBJECTSLib::ElementError:
						TreeCtrl.SetItemImage(hTreeItem, 22, 22);
						break;
					case TDMFOBJECTSLib::ElementWarning:
						TreeCtrl.SetItemImage(hTreeItem, 23, 23);
						break;
					case TDMFOBJECTSLib::ElementOk:
						TreeCtrl.SetItemImage(hTreeItem, 24, 24);
						break;
					default:
						TreeCtrl.SetItemImage(hTreeItem, 25, 25);
						break;
					}
				}
			}
			CATCH_ALL_LOG_ERROR(1079);
		}
		else if ((pServer = pUnk) != NULL)
		{
			// 6 - 9
			try
			{
				switch (pServer->GetState())
				{
				case TDMFOBJECTSLib::ElementError:
					TreeCtrl.SetItemImage(hTreeItem, 6, 6);
					break;
				case TDMFOBJECTSLib::ElementWarning:
					TreeCtrl.SetItemImage(hTreeItem, 7, 7);
					break;
				case TDMFOBJECTSLib::ElementOk:
					TreeCtrl.SetItemImage(hTreeItem, 8, 8);
					break;
				default:
					TreeCtrl.SetItemImage(hTreeItem, 9, 9);
					break;
				}
				
				// Report connection problem
				if (pServer->Connected == FALSE)
				{
					TreeCtrl.SetItemImage(hTreeItem, 14, 14);
				}
			}
			CATCH_ALL_LOG_ERROR(1080);
		}
		else if ((pDomain = pUnk) != NULL)
		{
			// 2 - 5
			try
			{
				switch (pDomain->GetState())
				{
				case TDMFOBJECTSLib::ElementError:
					TreeCtrl.SetItemImage(hTreeItem, 2, 2);
					break;
				case TDMFOBJECTSLib::ElementWarning:
					TreeCtrl.SetItemImage(hTreeItem, 3, 3);
					break;
				case TDMFOBJECTSLib::ElementOk:
					TreeCtrl.SetItemImage(hTreeItem, 4, 4);
					break;
				default:
					TreeCtrl.SetItemImage(hTreeItem, 5, 5);
					break;
				}
			}
			CATCH_ALL_LOG_ERROR(1081);
		}
		else if ((pSystem = pUnk) != NULL)
		{
			// 0, 1
			try
			{
				CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
				
				if (pDoc->GetConnectedFlag() == TRUE)
				{
					TreeCtrl.SetItemImage(hTreeItem, 0, 0);
				}
				else
				{
					TreeCtrl.SetItemImage(hTreeItem, 1, 1);
				}
			}
			CATCH_ALL_LOG_ERROR(1082);
		}
	}
	CATCH_ALL_LOG_ERROR(1083);
}

HTREEITEM CSystemView::FindDomain(TDMFOBJECTSLib::IDomainPtr pDomain)
{
	HTREEITEM hDomain = NULL;

	try
	{
		CTreeCtrl& TreeCtrl = GetTreeCtrl();
		HTREEITEM hSystem = TreeCtrl.GetRootItem();
		HTREEITEM hTreeItem = TreeCtrl.GetChildItem(hSystem);
		while (hTreeItem != NULL)
		{
			TDMFOBJECTSLib::IDomain* pDomainNode = (TDMFOBJECTSLib::IDomain*)TreeCtrl.GetItemData(hTreeItem);
			if (pDomainNode->IsEqual(pDomain))
			{
				hDomain = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1084);

	return hDomain;
}

HTREEITEM CSystemView::FindDomain(long nKey)
{
	HTREEITEM hDomain = NULL;

	try
	{
		CTreeCtrl& TreeCtrl = GetTreeCtrl();
		HTREEITEM hSystem = TreeCtrl.GetRootItem();
		HTREEITEM hTreeItem = TreeCtrl.GetChildItem(hSystem);
		while (hTreeItem != NULL)
		{
			TDMFOBJECTSLib::IDomain* pDomainNode = (TDMFOBJECTSLib::IDomain*)TreeCtrl.GetItemData(hTreeItem);
			if (pDomainNode->GetKey() == nKey)
			{
				hDomain = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1085);

	return hDomain;
}

HTREEITEM CSystemView::FindServer(HTREEITEM hDomain, TDMFOBJECTSLib::IServerPtr pServer)
{
	HTREEITEM hServer = NULL;

	try
	{
		CTreeCtrl& TreeCtrl = GetTreeCtrl();
		HTREEITEM hTreeItem = TreeCtrl.GetChildItem(hDomain);
		while (hTreeItem != NULL)
		{
			TDMFOBJECTSLib::IServer* pServerNode = (TDMFOBJECTSLib::IServer*)TreeCtrl.GetItemData(hTreeItem);
			if (pServerNode->IsEqual(pServer))
			{
				hServer = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1086);

	return hServer;
}

HTREEITEM CSystemView::FindServer(HTREEITEM hDomain, long nKey)
{
	HTREEITEM hServer = NULL;

	try
	{
		CTreeCtrl& TreeCtrl = GetTreeCtrl();
		HTREEITEM hTreeItem = TreeCtrl.GetChildItem(hDomain);
		while (hTreeItem != NULL)
		{
			TDMFOBJECTSLib::IServer* pServerNode = (TDMFOBJECTSLib::IServer*)TreeCtrl.GetItemData(hTreeItem);
			if (pServerNode->GetKey() == nKey)
			{
				hServer = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1087);

	return hServer;
}

HTREEITEM CSystemView::FindReplicationGroup(HTREEITEM hServer, TDMFOBJECTSLib::IReplicationGroupPtr pRG)
{
	HTREEITEM hRG = NULL;

	try
	{
		CTreeCtrl& TreeCtrl = GetTreeCtrl();
		HTREEITEM hTreeItem = TreeCtrl.GetChildItem(hServer);
		while (hTreeItem != NULL)
		{
			TDMFOBJECTSLib::IReplicationGroup* pRGNode = (TDMFOBJECTSLib::IReplicationGroup*)TreeCtrl.GetItemData(hTreeItem);
			if (pRGNode->IsEqual(pRG))
			{
				hRG = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1088);

	return hRG;
}

HTREEITEM CSystemView::FindReplicationGroup(HTREEITEM hServer, CString strName)
{
	HTREEITEM hRG = NULL;

	try
	{
		CTreeCtrl& TreeCtrl = GetTreeCtrl();
		HTREEITEM hTreeItem = TreeCtrl.GetChildItem(hServer);
		while (hTreeItem != NULL)
		{
			if (TreeCtrl.GetItemText(hTreeItem) == strName)
			{
				hRG = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1089);

	return hRG;
}

HTREEITEM CSystemView::FindReplicationGroup(HTREEITEM hServer, long nGroupNumber, HTREEITEM hPrev)
{
	HTREEITEM hRG = NULL;

	try
	{
		HTREEITEM hTreeItem;
		CTreeCtrl& TreeCtrl = GetTreeCtrl();

		if (hPrev == NULL)
		{
			hTreeItem = TreeCtrl.GetChildItem(hServer);
		}
		else
		{
			hTreeItem = TreeCtrl.GetNextItem(hPrev, TVGN_NEXT);
		}

		while (hTreeItem != NULL)
		{
			TDMFOBJECTSLib::IReplicationGroup* pRGNode = (TDMFOBJECTSLib::IReplicationGroup*)TreeCtrl.GetItemData(hTreeItem);
			if (pRGNode->GroupNumber == nGroupNumber)
			{
				hRG = hTreeItem;
				break;
			}
			
			hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
		}
	}
	CATCH_ALL_LOG_ERROR(1090);

	return hRG;
}


BEGIN_MESSAGE_MAP(CSystemView, CView)
	//{{AFX_MSG_MAP(CSystemView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TVN_SELCHANGED, 0x1005, OnSelchanged)
	ON_NOTIFY(TVN_DELETEITEM, 0x1005, OnDeleteitem)
	ON_NOTIFY(NM_RCLICK, 0x1005, OnRclick)
	ON_NOTIFY(TVN_KEYDOWN, 0x1005, OnKeydown)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

BOOL CSystemView::OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
   // need to handle both ANSI and UNICODE 
	// versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	
	UINT nID = pNMHDR->idFrom;

	CTreeCtrl& TreeCtrl = GetTreeCtrl();
	CString cstrTipText;
	
	//Get item pointed by mouse
	const MSG* pMessage = GetCurrentMessage();
	ASSERT(pMessage);

	CPoint pt = pMessage->pt;
	ScreenToClient( &pt );
	UINT nFlags;
	HTREEITEM hItem = TreeCtrl.HitTest(pt, &nFlags);

	if(hItem != NULL)
	{
		if(nFlags & TVHT_ONITEM)
		{
			m_hPreviousTreeItem = hItem;
				
			HTREEITEM hFindItem = NULL;
				
			IUnknown* pUnk = (IUnknown*)TreeCtrl.GetItemData(hItem);
				
			TDMFOBJECTSLib::IReplicationPairPtr pRP;
			TDMFOBJECTSLib::IReplicationGroupPtr pRG;
			TDMFOBJECTSLib::IServerPtr pServer;
			TDMFOBJECTSLib::IDomainPtr pDomain;
			TDMFOBJECTSLib::ISystemPtr pSystem;

			try
			{
				if ((pRP = pUnk) != NULL)
				{
				}
				else if ((pRG = pUnk) != NULL)
				{
					std::string str;
					str = "Description : ";
					str += pRG->Description;
					str += "\n";
					str += "Status : ";
					
					switch (pRG->GetConnectionStatus())
					{
					case TDMFOBJECTSLib::FTD_PMD_ONLY:
						str += "PMD Only";
						break;
					case TDMFOBJECTSLib::FTD_CONNECTED:
						str += "Connected";
						break;
					case TDMFOBJECTSLib::FTD_ACCUMULATE:
						str += "Accumulate";
						break;
					}
					
					str += "\nTarget Server Name : ";
					if(pRG->TargetName.length() > 0)
					{
						str += pRG->TargetName;
					}
					
					cstrTipText = str.c_str();
				}
				else if ((pServer = pUnk) != NULL)
				{
					std::string str = "Status : ";
					
					if (pServer->Connected == FALSE)
					{
						str += "Disconnected";
					}
					else
					{
						switch (pServer->GetState())
						{
						case TDMFOBJECTSLib::ElementError:
							str += "Error";
							break;
						case TDMFOBJECTSLib::ElementWarning:
							str += "Warning";
							break;
						case TDMFOBJECTSLib::ElementOk:
							str += "Normal";
							break;
						default:
							str += "Not Started";
							break;
						}
					}
					
					str += "\nIP Address : ";
					if(pServer->IPAddress[0].length() > 0)
					{
						str += pServer->IPAddress[0];
					}
					
					cstrTipText = str.c_str();
				}
				else if ((pDomain = pUnk) != NULL)
				{
					std::string str = "";
					
					if(pDomain->Description.length() > 0)
					{
						str += pDomain->Description;
					}
					
					cstrTipText = str.c_str();
					cstrTipText.TrimLeft();
					cstrTipText.TrimRight();
					if(!cstrTipText.IsEmpty())
					{
						TreeCtrl.GetToolTips()->UpdateTipText(cstrTipText,this);
					}
					else
					{
						cstrTipText = "No description";
					}
				}
				else if ((pSystem = pUnk) != NULL)
				{
					CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
					
					if (pDoc->GetConnectedFlag() == TRUE)
					{
						cstrTipText = "Connected...";
					}
					else
					{
						cstrTipText = "Not connected...";
					}
				}
			}
			CATCH_ALL_LOG_ERROR(1091);
		}
	}
	else 
	{
		cstrTipText = "";
	}

	try
	{
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			lstrcpyn(pTTTA->szText, cstrTipText, 80);
		}
		else
		{
			_mbstowcsz(pTTTW->szText, cstrTipText, 80);
		}
	}
	CATCH_ALL_LOG_ERROR(1092);

   *pResult = 0;

	return TRUE;    // message was handled
}

/////////////////////////////////////////////////////////////////////////////
// CSystemView drawing

void CSystemView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CSystemView diagnostics

#ifdef _DEBUG
void CSystemView::AssertValid() const
{
	CView::AssertValid();
}

void CSystemView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSystemView message handlers

BOOL CSystemView::PreCreateWindow(CREATESTRUCT& cs) 
{
	return CView::PreCreateWindow(cs);
}

int CSystemView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_TreeCtrl.Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES |
					  TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS,
					  CRect(lpCreateStruct->x, lpCreateStruct->y, lpCreateStruct->cx, lpCreateStruct->cy),
					  this, 0x1005);

	if (g_bStaticLogo)
	{
		// Create a child bitmap static control.
		m_StaticLogo.Create(_T("Softek"), WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE, CRect(0,0,102,20), this);
		// Set the bitmap
		m_StaticLogo.SetBitmap(theApp.m_ResourceManager.GetTreeViewLogo());
	}

	return 0;
}

void CSystemView::OnSize(UINT nType, int cx, int cy) 
{
	CTreeCtrl& TreeCtrl = GetTreeCtrl();

	if (g_bStaticLogo)
	{
		TreeCtrl.MoveWindow(1, 1, cx-1, cy-30);
		m_StaticLogo.MoveWindow(0, cy-24, cx, 20);
	}
	else
	{
		TreeCtrl.MoveWindow(1, 1, cx-1, cy-1);
	}

	CView::OnSize(nType, cx, cy);
}

void CSystemView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CTreeCtrl& TreeCtrl = GetTreeCtrl();
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();

	if (pSender != NULL)
	{
		m_bInUpdate = true;
	}

	try
	{
		if (lHint == CViewNotification::VIEW_NOTIFICATION)
		{
			CViewNotification* pVN = (CViewNotification*)pHint;
			switch (pVN->m_nMessageId)
			{
			case CViewNotification::SELECTION_CHANGE_SYSTEM:
				{
					try
					{
						TreeCtrl.SelectItem(TreeCtrl.GetRootItem());
					}
					CATCH_ALL_LOG_ERROR(1093);
				}
				break;

			case CViewNotification::SELECTION_CHANGE_SERVER:
				{
					try
					{
						TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
					
						HTREEITEM hTreeItem = TreeCtrl.GetChildItem(m_hTreeItemDomain);
						while (hTreeItem != NULL)
						{
							TDMFOBJECTSLib::IServer* pServerNode = (TDMFOBJECTSLib::IServer*)TreeCtrl.GetItemData(hTreeItem);
							if (pServer->IsEqual(pServerNode))
							{
								TreeCtrl.SelectItem(hTreeItem);
								m_hTreeItemServer = hTreeItem;
							}
							hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
						}
					}
					CATCH_ALL_LOG_ERROR(1094);
				}
				break;
				
			case CViewNotification::SELECTION_CHANGE_RG:
				{
					try
					{
						TDMFOBJECTSLib::IReplicationGroup* pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
					
						HTREEITEM hTreeItem = TreeCtrl.GetChildItem(m_hTreeItemServer);
						while (hTreeItem != NULL)
						{
							TDMFOBJECTSLib::IReplicationGroup* pRGNode = (TDMFOBJECTSLib::IReplicationGroup*)TreeCtrl.GetItemData(hTreeItem);
							if (pRG->IsEqual(pRGNode))
							{
								TreeCtrl.SelectItem(hTreeItem);
							}
							hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
						}
					}
					CATCH_ALL_LOG_ERROR(1095);
				}
				break;

			case CViewNotification::SELECTION_CHANGE_RP:
				{
					try
					{
						TDMFOBJECTSLib::IReplicationPair* pRP = (TDMFOBJECTSLib::IReplicationPair*)pVN->m_pUnk;
						
						HTREEITEM hTreeItem = TreeCtrl.GetChildItem(m_hTreeItemRG);
						while (hTreeItem != NULL)
						{
							TDMFOBJECTSLib::IReplicationPair* pRPNode = (TDMFOBJECTSLib::IReplicationPair*)TreeCtrl.GetItemData(hTreeItem);
							if (pRP->IsEqual(pRPNode))
							{
								TreeCtrl.SelectItem(hTreeItem);
							}
							hTreeItem = TreeCtrl.GetNextItem(hTreeItem, TVGN_NEXT);
						}
					}
					CATCH_ALL_LOG_ERROR(1096);
				}
				break;
				
			case CViewNotification::REFRESH_SYSTEM:
				{
					try
					{
						pDoc->SelectDomain(NULL);
						pDoc->SelectServer(NULL);
						pDoc->SelectReplicationGroup(NULL);
						
						// Save Tree structure and selection
						std::set<std::string> setKey;
						std::string strKeySelection = (LPCSTR)TreeCtrl.GetItemText(TreeCtrl.GetSelectedItem());
						SaveTreeStructure(TreeCtrl.GetRootItem(), setKey);

						// Select root node before beginning the real refresh process (and after tree struct has been saved)
						TreeCtrl.SelectItem(TreeCtrl.GetRootItem());

						long lErr = pDoc->m_pSystem->Refresh();
						if (lErr & 0x10000)
						{
							AfxMessageBox("Collector's registration key is invalid.", MB_OK | MB_ICONINFORMATION);
						}
						lErr = lErr & 0x1111;  // Remove invalid key error
						
						pDoc->SetConnectedFlag(lErr == 0);
						
						m_nCurrentDomain   = -1;
						m_hTreeItemDomain  = NULL;
						m_nCurrentServer   = -1;
						m_hTreeItemServer  = NULL;
						m_strCurrentRG     = "";
						m_hTreeItemRG      = NULL;
						
						TreeCtrl.DeleteAllItems();
						OnInitialUpdate();
						
						if (pDoc->GetConnectedFlag())
						{
							// Restore Tree structure and selection
							RestoreTreeStructure(TreeCtrl.GetRootItem(), setKey, strKeySelection);
						}
						
						if (TreeCtrl.GetSelectedItem() == NULL)
						{
							TreeCtrl.SelectItem(TreeCtrl.GetRootItem());
						}
					}
					CATCH_ALL_LOG_ERROR(1097);
				}
				break;

			case CViewNotification::SYSTEM_CHANGE:
				{
					try
					{
						HTREEITEM hSystem = TreeCtrl.GetRootItem();
						SetItemImage(hSystem);
						CToolTipCtrl* pToolTip = TreeCtrl.GetToolTips();
						if (pDoc->GetConnectedFlag() == TRUE)
						{
							pToolTip->SetTipBkColor(RGB(128,255,128)); // GREEN
						}
						else
						{
							pToolTip->SetTipBkColor(RGB(255,0,0)); // RED
						}
						
						if (TreeCtrl.GetSelectedItem() == NULL)
						{
							// Select root
							TreeCtrl.SelectItem(TreeCtrl.GetRootItem());
						}
					}
					CATCH_ALL_LOG_ERROR(1098);
				}
				break;
				
			case CViewNotification::DOMAIN_ADD:
				{
					try
					{
						HTREEITEM hSystem = TreeCtrl.GetRootItem();
						TDMFOBJECTSLib::IDomainPtr pDomain = (TDMFOBJECTSLib::IDomain*)pVN->m_pUnk;
						if (pDomain == NULL)
						{
							for (int nIndex = 0; nIndex < pDoc->m_pSystem->DomainCount; nIndex++)
							{
								TDMFOBJECTSLib::IDomainPtr pDomainCur = pDoc->m_pSystem->GetDomain(nIndex);
								if (pDomainCur->GetKey() == (long)pVN->m_dwParam1)
								{
									pDomain = pDomainCur;
								}
							}
						}
						
						if (pDomain != NULL)
						{
							HTREEITEM hDomain = TreeCtrl.InsertItem(pDomain->Name, hSystem);
							TreeCtrl.SetItemData(hDomain, (DWORD)((IUnknown*)pDomain));
							pDomain->AddRef();
							SetItemImage(hDomain);
						}
					}
					CATCH_ALL_LOG_ERROR(1099);
				}
				break;
				
			case CViewNotification::DOMAIN_REMOVE:
				{
					try
					{
						TDMFOBJECTSLib::IDomainPtr pDomain = (TDMFOBJECTSLib::IDomain*)pVN->m_pUnk;
						HTREEITEM hDomain = NULL;
						if (pDomain != NULL)
						{
							hDomain = FindDomain(pDomain);
						}
						else
						{
							hDomain = FindDomain(pVN->m_dwParam1);
						}
						
						if (hDomain != NULL)
						{
							TreeCtrl.DeleteItem(hDomain);
						}
					}
					CATCH_ALL_LOG_ERROR(1100);
				}
				break;
				
			case CViewNotification::DOMAIN_CHANGE:
				{
					try
					{
						TDMFOBJECTSLib::IDomainPtr pDomain = (TDMFOBJECTSLib::IDomain*)pVN->m_pUnk;
						HTREEITEM hDomain = FindDomain(pVN->m_dwParam1);
						if (hDomain != NULL)
						{
							if (pDomain == NULL)
							{
								pDomain = (TDMFOBJECTSLib::IDomain*)TreeCtrl.GetItemData(hDomain);
							}

							TreeCtrl.SetItemText(hDomain, pDomain->Name);
							SetItemImage(hDomain);
						}
					}
					CATCH_ALL_LOG_ERROR(1101);
				}
				break;
				
			case CViewNotification::SERVER_ADD:
				{
					try
					{
						TDMFOBJECTSLib::IDomainPtr pDomain;
						TDMFOBJECTSLib::IServerPtr pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
						
						if (pServer == NULL)
						{
							for (int nIndex = 0; nIndex < pDoc->m_pSystem->DomainCount; nIndex++)
							{
								TDMFOBJECTSLib::IDomainPtr pDomainCur = pDoc->m_pSystem->GetDomain(nIndex);
								if (pDomainCur->GetKey() == (long)pVN->m_dwParam1)
								{
									pDomain = pDomainCur;
									
									for (int nServerIndex = 0; nServerIndex < pDomain->ServerCount; nServerIndex++)
									{
										TDMFOBJECTSLib::IServerPtr pServerCur = pDomain->GetServer(nServerIndex);
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
						else
						{
							pDomain = pServer->Parent;
						}
						
						// Find domain
						HTREEITEM hDomain = FindDomain(pDomain);
						if (hDomain != NULL)
						{
							HTREEITEM hServer = TreeCtrl.InsertItem(pServer->Name, hDomain);
							TreeCtrl.SetItemData(hServer, (DWORD)((IUnknown*)pServer));
							pServer->AddRef();
							SetItemImage(hServer);
						}
					}
					CATCH_ALL_LOG_ERROR(1102);
				}
				break;
				
			case CViewNotification::SERVER_REMOVE:
				{
					try
					{
						TDMFOBJECTSLib::IServerPtr pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;
						HTREEITEM hDomain = NULL;
						
						if (pServer != NULL)
						{
							TDMFOBJECTSLib::IDomainPtr pDomain = pServer->Parent;
							
							// Find domain
							hDomain = FindDomain(pDomain);
						}
						else
						{
							hDomain = FindDomain(pVN->m_dwParam1);
						}
						
						if (hDomain != NULL)
						{
							HTREEITEM hServer = NULL;
							
							// Find server (in domain)
							if (pServer != NULL)
							{
								hServer = FindServer(hDomain, pServer);
							}
							else
							{
								hServer = FindServer(hDomain, pVN->m_dwParam2);
							}
							
							if (hServer != NULL)
							{
								TreeCtrl.DeleteItem(hServer);
							}
						}
					}
					CATCH_ALL_LOG_ERROR(1103);
				}
				break;
				
			case CViewNotification::SERVER_CHANGE:
				{
					try
					{
						if ((pVN->m_eParam & CViewNotification::STATE) ||
							(pVN->m_eParam & CViewNotification::CONNECTION))
						{
							// Find domain
							HTREEITEM hDomain = FindDomain(pVN->m_dwParam1);
							if (hDomain != NULL)
							{
								// Find server with key
								long nKey = (long)pVN->m_dwParam2;
								HTREEITEM hServer = FindServer(hDomain, nKey);
								// Update state
								if (hServer != NULL)
								{
									SetItemImage(hServer);
								}
							}
						}
					}
					CATCH_ALL_LOG_ERROR(1104);
				}
				break;
				
			case CViewNotification::REPLICATION_GROUP_ADD:
				{
					try
					{
						TDMFOBJECTSLib::IReplicationGroupPtr pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
						TDMFOBJECTSLib::IServerPtr pServer = pRG->Parent;
						TDMFOBJECTSLib::IDomainPtr pDomain = pServer->Parent;
						
						// Find server (in domain)
						HTREEITEM hDomain = FindDomain(pDomain);
						if (hDomain != NULL)
						{
							HTREEITEM hServer = FindServer(hDomain, pServer);
							if (hServer != NULL)
							{
								HTREEITEM hRG = TreeCtrl.InsertItem(pRG->Name, hServer);
								TreeCtrl.SetItemData(hRG, (DWORD)((IUnknown*)pRG));
								pRG->AddRef();
								SetItemImage(hRG);
								
								// Add all its pairs
								long nNbRP = pRG->ReplicationPairCount;
								for (long nIndexRP = 0; nIndexRP < nNbRP; nIndexRP++)
								{
									TDMFOBJECTSLib::IReplicationPairPtr pRP = pRG->GetReplicationPair(nIndexRP);
									HTREEITEM hRP = TreeCtrl.InsertItem(pRP->Name, hRG);
									TreeCtrl.SetItemData(hRP, (DWORD)(IUnknown*)(pRP));
									pRP->AddRef();
									SetItemImage(hRP);
								}
							}
						}
					}
					CATCH_ALL_LOG_ERROR(1105);
				}
				break;

			case CViewNotification::REPLICATION_GROUP_REMOVE:
				{
					try
					{
						TDMFOBJECTSLib::IReplicationGroupPtr pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
						TDMFOBJECTSLib::IServerPtr pServer = pRG->Parent;
						TDMFOBJECTSLib::IDomainPtr pDomain = pServer->Parent;
						
						// Find server (in domain)
						HTREEITEM hDomain = FindDomain(pDomain);
						if (hDomain != NULL)
						{
							HTREEITEM hServer = FindServer(hDomain, pServer);
							if (hServer != NULL)
							{
								// Find replication group
								HTREEITEM hRG = FindReplicationGroup(hServer, pRG);
								if (hRG != NULL)
								{
									TreeCtrl.DeleteItem(hRG);
								}
							}
						}
					}
					CATCH_ALL_LOG_ERROR(1106);
				}
				break;
				
			case CViewNotification::REPLICATION_GROUP_CHANGE:
				{
					try
					{
						if (pVN->m_eParam & CViewNotification::NAME)
						{
							TDMFOBJECTSLib::IReplicationGroupPtr pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
							TDMFOBJECTSLib::IServerPtr pServer = pRG->Parent;
							TDMFOBJECTSLib::IDomainPtr pDomain = pServer->Parent;
							
							_bstr_t bstrNameOld = (BSTR)pVN->m_dwParam1;
							
							// Find domain
							HTREEITEM hDomain = FindDomain(pDomain);
							if (hDomain != NULL)
							{
								// Find server
								HTREEITEM hServer = FindServer(hDomain, pServer);
								if (hServer != NULL)
								{
									// Find replication group
									HTREEITEM hRG = FindReplicationGroup(hServer, CString((BSTR)bstrNameOld));
									if (hRG != NULL)
									{
										TreeCtrl.SetItemText(hRG, pRG->Name);
									}
								}
							}
						}
						if (pVN->m_eParam & CViewNotification::STATE)
						{
							// Find domain with name
							int nDomainKey = pVN->m_dwParam1;
							HTREEITEM hDomain = FindDomain(nDomainKey);
							if (hDomain != NULL)
							{
								// Find server
								long nKey = (long)pVN->m_dwParam2;
								HTREEITEM hServer = FindServer(hDomain, nKey);
								if (hServer != NULL)
								{
									// Find replication group
									long nGroupNumber = (long)pVN->m_dwParam3;
									HTREEITEM hRG = FindReplicationGroup(hServer, nGroupNumber);
									if (hRG != NULL)
									{
										SetItemImage(hRG);
									}
									
									// The group can be there twice (if in loopback)
									// Find replication group
									hRG = FindReplicationGroup(hServer, nGroupNumber, hRG);
									if (hRG != NULL)
									{
										SetItemImage(hRG);
									}
								}
							}
						}
						if (pVN->m_eParam & CViewNotification::PROPERTIES)
						{
							TDMFOBJECTSLib::IReplicationGroupPtr pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
							TDMFOBJECTSLib::IServerPtr pServer = pRG->Parent;
							TDMFOBJECTSLib::IDomainPtr pDomain = pServer->Parent;
							
							// Find domain
							HTREEITEM hDomain = FindDomain(pDomain);
							if (hDomain != NULL)
							{
								// Find server
								HTREEITEM hServer = FindServer(hDomain, pServer);
								if (hServer != NULL)
								{
									// Find source replication group
									HTREEITEM hRG = FindReplicationGroup(hServer, pRG);
									if (hRG != NULL)
									{
										SetItemImage(hRG);

										// Delete old pairs
										if (TreeCtrl.ItemHasChildren(hRG))
										{
											HTREEITEM hNextItem;
											HTREEITEM hChildItem = TreeCtrl.GetChildItem(hRG);
											
											while (hChildItem != NULL)
											{
												hNextItem = TreeCtrl.GetNextItem(hChildItem, TVGN_NEXT);
												TreeCtrl.DeleteItem(hChildItem);
												hChildItem = hNextItem;
											}
										}
										
										// Add new pairs
										long nNbRP = pRG->ReplicationPairCount;
										for (long nIndexRP = 0; nIndexRP < nNbRP; nIndexRP++)
										{
											TDMFOBJECTSLib::IReplicationPairPtr pRP = pRG->GetReplicationPair(nIndexRP);
											HTREEITEM hRP = TreeCtrl.InsertItem(pRP->Name, hRG);
											TreeCtrl.SetItemData(hRP, (DWORD)(IUnknown*)(pRP));
											pRP->AddRef();
											SetItemImage(hRP);
										}
									}
									
									// Find target replication group
									TDMFOBJECTSLib::IReplicationGroupPtr pRGTarget = pRG->GetTargetGroup();
									if (pRGTarget != NULL)
									{
										HTREEITEM hServer = FindServer(hDomain, pRGTarget->Parent);
										HTREEITEM hRG = FindReplicationGroup(hServer, pRGTarget);
										if (hRG != NULL)
										{
											SetItemImage(hRG);

											// Delete old pairs
											if (TreeCtrl.ItemHasChildren(hRG))
											{
												HTREEITEM hNextItem;
												HTREEITEM hChildItem = TreeCtrl.GetChildItem(hRG);
												
												while (hChildItem != NULL)
												{
													hNextItem = TreeCtrl.GetNextItem(hChildItem, TVGN_NEXT);
													TreeCtrl.DeleteItem(hChildItem);
													hChildItem = hNextItem;
												}
											}
											
											// Add new pairs
											long nNbRP = pRGTarget->ReplicationPairCount;
											for (long nIndexRP = 0; nIndexRP < nNbRP; nIndexRP++)
											{
												TDMFOBJECTSLib::IReplicationPairPtr pRP = pRGTarget->GetReplicationPair(nIndexRP);
												HTREEITEM hRP = TreeCtrl.InsertItem(pRP->Name, hRG);
												TreeCtrl.SetItemData(hRP, (DWORD)(IUnknown*)(pRP));
												pRP->AddRef();
												SetItemImage(hRP);
											}
										}
									}
									// Ensure the little plus sign is added
									TreeCtrl.Invalidate();
								}
							}
							
						}
					}
					CATCH_ALL_LOG_ERROR(1107);
				}
				break;
				
			case CViewNotification::SERVER_BAB_NOT_OPTIMAL:
				if (pDoc->m_bShowServersWarnings)
				{
					// Find domain
					HTREEITEM hDomain = FindDomain(pVN->m_dwParam1);
					if (hDomain != NULL)
					{
						// Find server with key
						long nKey = (long)pVN->m_dwParam2;
						HTREEITEM hServer = FindServer(hDomain, nKey);
						// Update state
						if (hServer != NULL)
						{
							TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)TreeCtrl.GetItemData(hServer);
							CServerWarningDlg ServerWarningDlg(pDoc, pServer);
							ServerWarningDlg.DoModal();
						}
					}
				}
				break;

			case CViewNotification::TAB_TOOLS_NEXT:
			case CViewNotification::TAB_SERVER_PREVIOUS:
				{
					SetFocus();
				}
				break;
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1108);

	m_bInUpdate = false;
}

void CSystemView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW*       pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CTdmfCommonGuiDoc* pDoc        = (CTdmfCommonGuiDoc*)GetDocument();

	try
	{
		IUnknown* pUnk = (IUnknown*)pNMTreeView->itemNew.lParam;
			
		// System is selected
		TDMFOBJECTSLib::ISystemPtr pSystem;
		if ((pSystem = pUnk) != NULL)
		{
			try
			{
				m_nCurrentDomain   = -1;
				m_hTreeItemDomain  = NULL;
				m_nCurrentServer   = -1;
				m_hTreeItemServer  = NULL;
				m_strCurrentRG     = "";
				m_hTreeItemRG      = NULL;
				
				pDoc->SelectDomain(NULL);
				
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_SYSTEM;
				VN.m_dwParam1 = 1;
				pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
			}
			CATCH_ALL_LOG_ERROR(1109);
		}
		
		// A new domain is selected
		TDMFOBJECTSLib::IDomainPtr pDomain;
		if ((pDomain = pUnk) != NULL)
		{
			try
			{
				m_nCurrentDomain   = pDomain->Key;
				m_hTreeItemDomain  = pNMTreeView->itemNew.hItem;
				m_nCurrentServer   = -1;
				m_hTreeItemServer  = NULL;
				m_strCurrentRG     = "";
				m_hTreeItemRG      = NULL;
				
				pDoc->SelectDomain(pDomain);
				
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_DOMAIN;
				VN.m_pUnk = pDomain;
				pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
			}
			CATCH_ALL_LOG_ERROR(1110);
		}
		
		// A new server is selected
		TDMFOBJECTSLib::IServerPtr pServer;
		if ((pServer = pUnk) != NULL)
		{
			try
			{
				pDomain = pServer->GetParent();
				// if we have selected a server from a different domain
				if (pDomain->Key != m_nCurrentDomain)
				{
					m_nCurrentDomain = pDomain->Key;
					m_hTreeItemDomain  = GetTreeCtrl().GetParentItem(pNMTreeView->itemNew.hItem);
					
					pDoc->SelectDomain(pDomain);
					
					// Send a 'Domain Selection Update'
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_DOMAIN;
					VN.m_pUnk = pDomain;
					pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
				
				m_nCurrentServer  = pServer->Key;
				m_hTreeItemServer  = pNMTreeView->itemNew.hItem;
				m_strCurrentRG     = "";
				m_hTreeItemRG      = NULL;
				
				if (m_bInUpdate == false)
				{
				// Send a 'Server Selection Update'
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_SERVER;
				VN.m_pUnk = pServer;
				pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
			}
			}
			CATCH_ALL_LOG_ERROR(1111);
		}
		
		// A new RG is selected
		TDMFOBJECTSLib::IReplicationGroupPtr pRG;
		
		if ((pRG = pUnk) != NULL)
		{
			try
			{
				pServer = pRG->GetParent();
				pDomain = pServer->GetParent();
				
				// if we have selected a Replication Group from a different domain
				if (pDomain->Key != m_nCurrentDomain)
				{
					m_nCurrentDomain  = pDomain->Key;
					m_hTreeItemDomain  = GetTreeCtrl().GetParentItem(GetTreeCtrl().GetParentItem(pNMTreeView->itemNew.hItem));
					m_nCurrentServer  = -1;
					m_hTreeItemServer  = NULL;
					
					pDoc->SelectDomain(pDomain);
					
					// Send a 'Domain Selection Update'
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_DOMAIN;
					VN.m_pUnk = pDomain;
					pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
				
				// if we have selected a Replication Group from a different Server
				if (pServer->Key != m_nCurrentServer)
				{
					m_nCurrentServer  = pServer->Key;
					m_hTreeItemServer  = GetTreeCtrl().GetParentItem(pNMTreeView->itemNew.hItem);
					
					if (m_bInUpdate == false)
					{
					// Send a 'Server Selection Update'
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_SERVER;
					VN.m_pUnk = pServer;
					pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
				}
				
				m_strCurrentRG = pRG->Key;
				m_hTreeItemRG  = pNMTreeView->itemNew.hItem;
				
				if (m_bInUpdate == false)
				{
				// Send a 'Replication Group Selection Update'
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_RG;
				VN.m_pUnk = pRG;
				pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
			}
			}
			CATCH_ALL_LOG_ERROR(1112);
		}
		
		// A new RP is selected
		TDMFOBJECTSLib::IReplicationPairPtr pRP;
		
		if ((pRP = pUnk) != NULL)
		{
			try
			{
				pRG     = pRP->GetParent();
				pServer = pRG->GetParent();
				pDomain = pServer->GetParent();
				
				// if we have selected a Replication Pair from a different domain
				if (pDomain->Key != m_nCurrentDomain)
				{
					m_nCurrentDomain  = pDomain->Key;
					m_hTreeItemDomain  = GetTreeCtrl().GetParentItem(GetTreeCtrl().GetParentItem(GetTreeCtrl().GetParentItem(pNMTreeView->itemNew.hItem)));
					m_nCurrentServer  = -1;
					m_hTreeItemServer  = NULL;
					
					pDoc->SelectDomain(pDomain);
					
					// Send a 'Domain Selection Update'
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_DOMAIN;
					VN.m_pUnk = pDomain;
					pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
				
				// if we have selected a Replication Pair from a different Server
				if (pServer->Key != m_nCurrentServer)
				{
					m_nCurrentServer  = pServer->Key;
					m_hTreeItemServer  = GetTreeCtrl().GetParentItem(GetTreeCtrl().GetParentItem(pNMTreeView->itemNew.hItem));
					m_strCurrentRG = "";
					m_hTreeItemRG  = NULL;
					
					// Send a 'Server Selection Update'
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_SERVER;
					VN.m_pUnk = pServer;
					pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
				
				// if we have selected a Replication Pair from a different Replication Group
				if (pRG->Key != _bstr_t(m_strCurrentRG.c_str()))
				{
					m_strCurrentRG = pRG->Key;
					m_hTreeItemRG = GetTreeCtrl().GetParentItem(pNMTreeView->itemNew.hItem);
					
					// Send a 'Rep Group Selection Update'
					CViewNotification VN;
					VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_RG;
					VN.m_pUnk = pRG;
					pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
				}
				
				// Send a 'Replication Pair Selection Update'
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::SELECTION_CHANGE_RP;
				VN.m_pUnk = pRP;
				pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
			}
			CATCH_ALL_LOG_ERROR(1113);
		}

		// Restore focus
		SetFocus();
	}
	CATCH_ALL_LOG_ERROR(1114);

	*pResult = 0;
}

void CSystemView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CTreeCtrl&         TreeCtrl = GetTreeCtrl();
	CTdmfCommonGuiDoc* pDoc     = (CTdmfCommonGuiDoc*)GetDocument();
	// Enable tool tips
	CToolTipCtrl* pToolTip = TreeCtrl.GetToolTips();
	pToolTip->AddTool(&TreeCtrl, LPSTR_TEXTCALLBACK);

	CRect rc(2, 2, 2, 2);
	pToolTip->SetMargin(&rc);
	pToolTip->SetMaxTipWidth(300);
	pToolTip->SetDelayTime(TTDT_INITIAL, 1000);
	pToolTip->SetDelayTime(TTDT_RESHOW,  1000);

	if (pDoc->GetConnectedFlag() == TRUE)
	{
		pToolTip->SetTipBkColor(RGB(128,255,128)); // GREEN
	}
	else
	{
		pToolTip->SetTipBkColor(RGB(255,0,0)); // RED
	}

	// Set image list
	TreeCtrl.SetImageList(&m_ImageList, TVSIL_NORMAL);

	// System
	try
	{
		HTREEITEM hSystem = TreeCtrl.InsertItem(pDoc->m_pSystem->Name);
		TreeCtrl.SetItemData(hSystem, (DWORD)((IUnknown*)pDoc->m_pSystem));
		pDoc->m_pSystem->AddRef();
		SetItemImage(hSystem);

		// Domains
		long nNbDomain = pDoc->m_pSystem->DomainCount;
		for (long nIndexDomain = 0; nIndexDomain < nNbDomain; nIndexDomain++)
		{
			TDMFOBJECTSLib::IDomainPtr pDomain = pDoc->m_pSystem->GetDomain(nIndexDomain);
			HTREEITEM hDomain = TreeCtrl.InsertItem(pDomain->Name, hSystem);
			TreeCtrl.SetItemData(hDomain, (DWORD)((IUnknown*)pDomain));
			pDomain->AddRef();
			SetItemImage(hDomain);
			
			// Servers
			long nNbServer = pDomain->ServerCount;
			for (long nIndexServer = 0; nIndexServer < nNbServer; nIndexServer++)
			{
				TDMFOBJECTSLib::IServerPtr pServer = pDomain->GetServer(nIndexServer);

				HTREEITEM hServer = TreeCtrl.InsertItem(pServer->Name, hDomain);
				TreeCtrl.SetItemData(hServer, (DWORD)((IUnknown*)pServer));
				pServer->AddRef();
				SetItemImage(hServer);
				
				// Replication Groups
				long nNbRG = pServer->ReplicationGroupCount;
				for (long nIndexRG = 0; nIndexRG < nNbRG; nIndexRG++)
				{
					TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(nIndexRG);
					HTREEITEM hRG = TreeCtrl.InsertItem(pRG->Name, hServer);
					TreeCtrl.SetItemData(hRG, (DWORD)(IUnknown*)(pRG));
					pRG->AddRef();
					SetItemImage(hRG);

					// Replication Pairs
					long nNbRP = pRG->ReplicationPairCount;
					for (long nIndexRP = 0; nIndexRP < nNbRP; nIndexRP++)
					{
						TDMFOBJECTSLib::IReplicationPairPtr pRP = pRG->GetReplicationPair(nIndexRP);
						HTREEITEM hRP = TreeCtrl.InsertItem(pRP->Name, hRG);
						TreeCtrl.SetItemData(hRP, (DWORD)(IUnknown*)(pRP));
						pRP->AddRef();
						SetItemImage(hRP);
					}
				}
			}
		}

		// Expand first level
		TreeCtrl.Expand(hSystem, TVE_EXPAND);
	}
	CATCH_ALL_LOG_ERROR(1115);
}

void CSystemView::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	try
	{
		IUnknown* pUnk = (IUnknown*)GetTreeCtrl().GetItemData(pNMTreeView->itemOld.hItem);
		pUnk->Release();
	}
	CATCH_ALL_LOG_ERROR(1116);

	*pResult = 0;
}

void CSystemView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	/* Get the cursor position for this message */
	DWORD dwPos = GetMessagePos();
	/* Convert the co-ords into a CPoint structure */
	CPoint pt(GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos));
	/* convert to screen co-ords for the hittesting to work */
	ScreenToClient(&pt);

	HTREEITEM hItem = GetTreeCtrl().HitTest(pt);
	ContextMenuAction(hItem);

	*pResult = 0;
}

void CSystemView::ContextMenuAction(HTREEITEM hItem, CPoint* pPoint, BOOL bShow, UINT nCmd)
{
	try
	{
		if (hItem != NULL)
		{
			IUnknown* pUnk = (IUnknown*)GetTreeCtrl().GetItemData(hItem);
			
			TDMFOBJECTSLib::ISystemPtr pSystem;
			if ((pSystem = pUnk) != NULL)
			{
				CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
				CSystemContextMenu SystemContextMenu(pDoc);
				if (bShow)
				{
					SystemContextMenu.Show(pPoint);
				}
				else
				{
					SystemContextMenu.SendCommand(nCmd);
				}
			}
			
			TDMFOBJECTSLib::IDomainPtr pDomain;
			if ((pDomain = pUnk) != NULL)
			{
				CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
				CDomainContextMenu DomainContextMenu(pDoc, pDomain);
				if (bShow)
				{
					DomainContextMenu.Show(pPoint);
				}
				else
				{
					if ((nCmd != ID_DELETE) || (pDoc->GetReadOnlyFlag() == FALSE))
					{
						DomainContextMenu.SendCommand(nCmd);
					}
				}
			}
			
			TDMFOBJECTSLib::IServerPtr pServer;
			if ((pServer = pUnk) != NULL)
			{
				CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
				CServerContextMenu ServerContextMenu(pDoc, pServer);
				if (bShow)
				{
					ServerContextMenu.Show(pPoint);
				}
				else
				{
					if ((nCmd != ID_DELETE) ||
						(pDoc->GetConnectedFlag() && (!pServer->Connected) && (pDoc->GetReadOnlyFlag() == FALSE)))
					{
						ServerContextMenu.SendCommand(nCmd);
					}
				}
			}
			
			TDMFOBJECTSLib::IReplicationGroupPtr pRG;
			if ((pRG = pUnk) != NULL)
			{
				CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
				CReplicationGroupContextMenu RGContextMenu(pDoc, pRG);
				if (bShow)
				{
					RGContextMenu.Show(pPoint);
				}
				else
				{
					if ((nCmd != ID_DELETE) || 
						(pDoc->GetConnectedFlag() && pRG->IsSource &&
						 pRG->Parent->Connected && (pRG->Mode == TDMFOBJECTSLib::FTD_M_UNDEF) &&
						 (pDoc->GetReadOnlyFlag() == FALSE)))
					{
						RGContextMenu.SendCommand(nCmd);
					}
				}
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1117);
}

BOOL CSystemView::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_MOUSEMOVE && pMsg->hwnd == GetTreeCtrl().m_hWnd)
	{
		CPoint Point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		UINT Flag;		
		HTREEITEM hItem = GetTreeCtrl().HitTest(Point, &Flag);
		
		if(hItem != NULL)
		{
			if(Flag & TVHT_ONITEM)
			{
				if(m_hPreviousTreeItem != hItem)
				{
					m_hPreviousTreeItem = hItem;
					GetTreeCtrl().GetToolTips()->Pop();
				}
			}
			else 
			{
				m_hPreviousTreeItem = NULL;
				GetTreeCtrl().GetToolTips()->Pop();
			}
		}
		else 
		{
			m_hPreviousTreeItem = NULL;			
			GetTreeCtrl().GetToolTips()->Pop();
		}
	}

	return CView::PreTranslateMessage(pMsg);
}

void CSystemView::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;

	switch (pTVKeyDown->wVKey)
	{
	case VK_TAB:
		{
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
			
			CViewNotification VN;
			if (GetKeyState(VK_SHIFT) & ~1)
			{
				VN.m_nMessageId = CViewNotification::TAB_SYSTEM_PREVIOUS;
			}
			else
			{
				VN.m_nMessageId = CViewNotification::TAB_SYSTEM_NEXT;
			}
			pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
		}
		break;

	case VK_DELETE:
		{
			HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
			if (hItem != NULL)
			{
				ContextMenuAction(hItem, NULL, FALSE, ID_DELETE);
			}
		}
		break;

	case VK_APPS:
		{
			HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
			if (hItem != NULL)
			{
				RECT Rect;
				if (GetTreeCtrl().GetItemRect(hItem, &Rect, TRUE))
				{
					CPoint Point((Rect.right - Rect.left)/2 + Rect.left,
								 (Rect.bottom - Rect.top)/2 + Rect.top);
					ClientToScreen(&Point);
					ContextMenuAction(hItem, &Point);
				}
			}
		}
		break;
	}
	
	*pResult = 0;
}

void CSystemView::SaveTreeStructure(HTREEITEM hItem, std::set<std::string> &setKey)
{
	HTREEITEM hTreeItem = GetTreeCtrl().GetChildItem(hItem);

	while (hTreeItem != NULL)
	{
		if ((GetTreeCtrl().GetItemState(hTreeItem, TVIS_EXPANDED) & TVIS_EXPANDED) != 0)
		{	
			setKey.insert((LPCSTR)(GetTreeCtrl().GetItemText(hTreeItem)));
			SaveTreeStructure(hTreeItem, setKey);
		}

		hTreeItem = GetTreeCtrl().GetNextItem(hTreeItem, TVGN_NEXT);
	}
}

void CSystemView::RestoreTreeStructure(HTREEITEM hItem, std::set<std::string> &setKey, std::string& strSelection)
{
	HTREEITEM hTreeItem = GetTreeCtrl().GetChildItem(hItem);

	while (hTreeItem != NULL)
	{
		std::string strKey = (LPCSTR)(GetTreeCtrl().GetItemText(hTreeItem));
		if (strKey == strSelection)
		{
			GetTreeCtrl().SelectItem(hTreeItem);
		}

		if (setKey.find(strKey) != setKey.end())
		{	
			GetTreeCtrl().Expand(hTreeItem, TVE_EXPAND);
			RestoreTreeStructure(hTreeItem, setKey, strSelection);
		}

		hTreeItem = GetTreeCtrl().GetNextItem(hTreeItem, TVGN_NEXT);
	}
}
