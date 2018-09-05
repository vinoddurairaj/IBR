// ToolsView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguidoc.h"
#include "ToolsView.h"
#include "ViewNotification.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolsView

IMPLEMENT_DYNCREATE(CToolsView, CScrollView)

CToolsView::CToolsView() : m_pSystemSheet(NULL), m_nPageSet(-1),
	m_nSystemActivePage(0), m_nServerActivePage(0), m_nRGActivePage(0),
	m_bScrollbarsEnabled(false), m_Size(680, 250)
{
}

CToolsView::~CToolsView()
{
	if (m_pSystemSheet != NULL)
	{
		delete m_pSystemSheet;
	}
}

void CToolsView::RemoveAllPages()
{
	// save page index
	int nActiveIndex = m_pSystemSheet->GetActiveIndex();
	switch (m_nPageSet)
	{
	case 0:
		m_nSystemActivePage = nActiveIndex;
		break;
	case 2:
		m_nServerActivePage = nActiveIndex;
		break;
	case 3:
		m_nRGActivePage = nActiveIndex;
		break;
	}

	// Delete pages
	int nLastPage = m_pSystemSheet->GetPageCount() - 1;
	int nActivePage = m_pSystemSheet->GetActiveIndex();
	for (;nLastPage >= 0; nLastPage--)
	{
		// don't remove active page now... To avoid unnecessary page acivation
		if (nLastPage != nActivePage)
		{
			m_pSystemSheet->RemovePage(nLastPage);
		}
	}
	// Now it's time to delete the active page
	m_pSystemSheet->RemovePage(0);
}

void CToolsView::EnableScrollBars(bool bEnable)
{
	RECT Rect;

	m_bScrollbarsEnabled = bEnable;

	if (bEnable)
	{
		SetScrollSizes(MM_TEXT, m_Size);
		GetClientRect(&Rect);
		OnSize(0, max(Rect.right, m_Size.cx), max(Rect.bottom, m_Size.cy));
	}
	else
	{
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		GetClientRect(&Rect);
		OnSize(0, Rect.right, Rect.bottom);
	}
}


BEGIN_MESSAGE_MAP(CToolsView, CScrollView)
	//{{AFX_MSG_MAP(CToolsView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CHAR()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CToolsView diagnostics

#ifdef _DEBUG
void CToolsView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CToolsView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CToolsView message handlers

int CToolsView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pSystemSheet = new CSystemSheet(this);

	if (!m_pSystemSheet->Create(this, WS_CHILD|WS_VISIBLE, 0))
	{
		delete m_pSystemSheet;
		m_pSystemSheet = NULL;
		return -1;
	}

	return 0;
}

BOOL CToolsView::PreCreateWindow(CREATESTRUCT& cs) 
{
	return CScrollView::PreCreateWindow(cs);
}

void CToolsView::OnDraw(CDC* pDC) 
{
	
}

void CToolsView::OnSize(UINT nType, int cx, int cy) 
{
	CScrollView::OnSize(nType, cx, cy);

	if (IsWindow(m_pSystemSheet->m_hWnd)) // May not have been created when first called!
	{
		if (m_bScrollbarsEnabled)
		{
			RECT Rect;
			GetClientRect(&Rect);

			int x = max(Rect.right, m_Size.cx);
			int y = max(Rect.bottom, m_Size.cy);
			CPoint point = GetScrollPosition();

			m_pSystemSheet->Resize(-point.x, -point.y, x, y);
		}
		else
		{
			m_pSystemSheet->Resize(0, 0, cx, cy);
		}
	}
}

void CToolsView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (lHint == CViewNotification::VIEW_NOTIFICATION)
	{
		CViewNotification* pVN = (CViewNotification*)pHint;
		switch (pVN->m_nMessageId)
		{
		case CViewNotification::SELECTION_CHANGE_SYSTEM:
			{
				// in the case of a disconnect (m_dwParam1 == TRUE),
				// make sure the system's event page is not selected
				if (pVN->m_dwParam1)
				{
					m_pSystemSheet->SetActivePage(0);
				}

				RemoveAllPages();
					
				m_pSystemSheet->AddPage(&m_SystemDetailsPage);
				m_pSystemSheet->AddPage(&m_SystemEventsPage);
				
				m_nPageSet = 0;

				m_pSystemSheet->SetActivePage((!pVN->m_dwParam1) ? m_nSystemActivePage : 0);
			}
			break;
			
		case CViewNotification::SELECTION_CHANGE_DOMAIN:
			{
				RemoveAllPages();

				m_pSystemSheet->AddPage(&m_DomainDetailsPage);

				m_nPageSet = 1;
				m_pSystemSheet->SetActivePage(0);
			}
			break;

		case CViewNotification::SELECTION_CHANGE_SERVER:
			{
				if (pVN->m_eParam != CViewNotification::STATE)
				{
					RemoveAllPages();

					TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)pVN->m_pUnk;

					m_pSystemSheet->AddPage(&m_ServerDetailsPage);
					m_pSystemSheet->AddPage(&m_ServerEventsPage);
					m_pSystemSheet->AddPage(&m_ServerPerformanceMonitorPage);
					m_ServerPerformanceMonitorPage.m_bAllGroups = TRUE;
                    m_ServerPerformanceMonitorPage.m_pRG = NULL;
					//m_pSystemSheet->AddPage(&m_ServerPerformanceReporterPage);
					
					CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
                    if(pDoc->GetReadOnlyFlag() == FALSE)
						m_pSystemSheet->AddPage(&m_ServerCommandsPage);

					m_nPageSet = 2;
					m_pSystemSheet->SetActivePage(m_nServerActivePage);
				}
			}
			break;

		case CViewNotification::SELECTION_CHANGE_RG:
			{
				if (pVN->m_eParam != CViewNotification::STATE)
				{
					RemoveAllPages();
					
					TDMFOBJECTSLib::IReplicationGroup* pRG = (TDMFOBJECTSLib::IReplicationGroup*)pVN->m_pUnk;
					
					m_pSystemSheet->AddPage(&m_RGDetailsPage);
					m_pSystemSheet->AddPage(&m_RGReplicationPairsPage);
					m_pSystemSheet->AddPage(&m_RGEventsPage);
			
					m_pSystemSheet->AddPage(&m_ServerPerformanceMonitorPage);
					m_ServerPerformanceMonitorPage.m_bAllGroups = FALSE;
					m_ServerPerformanceMonitorPage.m_pRG = pRG;
					
					CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
                    if(pDoc->GetReadOnlyFlag() == FALSE)
						m_pSystemSheet->AddPage(&m_RGCommandsPage);
 
					m_nPageSet = 3;
					m_pSystemSheet->SetActivePage(m_nRGActivePage);
				}
			}
			break;

		case CViewNotification::SELECTION_CHANGE_RP:
			{
				if (pVN->m_eParam != CViewNotification::STATE)
				{
					ASSERT(m_nPageSet == 3);
					m_pSystemSheet->SetActivePage(1);

					TDMFOBJECTSLib::IReplicationPair* pRP = (TDMFOBJECTSLib::IReplicationPair*)pVN->m_pUnk;
					m_RGReplicationPairsPage.SelectPair(pRP);
				}
			}
			break;

		case CViewNotification::DOMAIN_CHANGE:
			if ((m_nPageSet == 1) && (m_pSystemSheet->GetActiveIndex() == 0))
			{
				m_DomainDetailsPage.RefreshValues();
			}
			break;

		case CViewNotification::SERVER_CHANGE:
			if ((m_nPageSet == 2) && (m_pSystemSheet->GetActiveIndex() == 0))
			{
				m_ServerDetailsPage.RefreshValues();
			}
			break;

		case CViewNotification::REPLICATION_GROUP_CHANGE:
			if (m_nPageSet == 3)
			{
				m_RGDetailsPage.RefreshValues();

				if (pVN->m_eParam == CViewNotification::PROPERTIES)
				{
					m_RGReplicationPairsPage.RefreshValues();
				}
			}
			break;

		case CViewNotification::TAB_REPLICATION_GROUP_NEXT:
		case CViewNotification::TAB_SYSTEM_PREVIOUS:
			{
				m_pSystemSheet->GetTabControl()->SetFocus();
			}
			break;

        case CViewNotification::RECEIVED_COLLECTOR_STATISTICS_DATA:
            {
                m_SystemDetailsPage.UpdateTheDataStatsFromTheCollector();
				m_ServerPerformanceMonitorPage.UpdateTheTimeStampFromTheCollector();
			}
            break;
         }
	}
}

void CToolsView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_TAB)
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();
		CViewNotification VN;

		if (GetKeyState(VK_SHIFT) & ~1)
		{
			VN.m_nMessageId = CViewNotification::TAB_TOOLS_PREVIOUS;
		}
		else
		{
			VN.m_nMessageId = CViewNotification::TAB_TOOLS_NEXT;
		}

		pDoc->UpdateAllViews(this, CViewNotification::VIEW_NOTIFICATION, &VN);
	}

	CScrollView::OnChar(nChar, nRepCnt, nFlags);
}

void CToolsView::OnInitialUpdate() 
{
	CScrollView::OnInitialUpdate();

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)GetDocument();

	m_SystemDetailsPage.SetSystem(pDoc->m_pSystem);	
	m_ServerPerformanceMonitorPage.SetSystem(pDoc->m_pSystem);	

	EnableScrollBars();
}

BOOL CToolsView::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	
}
