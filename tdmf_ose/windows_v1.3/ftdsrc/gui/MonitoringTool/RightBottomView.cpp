// RightView.cpp : implementation of the CRightBottomView class
//

#include "stdafx.h"
#include "TDMFGUI.h"

#include "Doc.h"
#include "RightBottomView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRightBottomView

IMPLEMENT_DYNCREATE(CRightBottomView, CPageView)

BEGIN_MESSAGE_MAP(CRightBottomView, CPageView)
   //{{AFX_MSG_MAP(CRightBottomView)
   ON_WM_CREATE()
   ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRightBottomView construction/destruction

CRightBottomView::CRightBottomView() : m_pPropFrame(NULL), m_pPropSheet(NULL)
{
}

CRightBottomView::~CRightBottomView()
{

}

/////////////////////////////////////////////////////////////////////////////
// CRightBottomView message handlers
void CRightBottomView::OnProperties()
{
   // TODO: The property sheet attached to your project
   // via this function is not hooked up to any message
   // handler.  In order to actually use the property sheet,
   // you will need to associate this function with a control
   // in your project such as a menu item or tool bar button.
   //
   // If mini frame does not already exist, create a new one.
   // Otherwise, unhide it

   if(m_pPropFrame == NULL)
   {
      m_pPropFrame = new PropSheetFrame;
      CRect rect(0, 0, 0, 0);
      CString strTitle;
      VERIFY(strTitle.LoadString(IDS_PROPSHT_CAPTION));

      if(!m_pPropFrame->Create(NULL, strTitle,
         WS_POPUP | WS_CAPTION | WS_SYSMENU, rect, this))
      {
         delete m_pPropFrame;
         m_pPropFrame = NULL;
         return;
      }
      m_pPropFrame->CenterWindow();
   }

   // Before unhiding the modeless property sheet, update its
   // settings appropriately.  For example, if you are reflecting
   // the state of the currently selected item, pick up that
   // information from the active view and change the property
   // sheet settings now.

   if(m_pPropFrame != NULL && !m_pPropFrame->IsWindowVisible())
      m_pPropFrame->ShowWindow(SW_SHOW);
}

int CRightBottomView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   if(CPageView::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_pPropSheet = new PropSheet(); 
   
   // Add all of the property pages here, BEFORE calling Create() on the
   // property sheet. Note that the order that they appear in here will be
   // the order they appear in on screen.  By default, the first page of
   // the set is the active one. One way to make a different property page
   // the active one is to call SetActivePage().
	m_Volumes.Construct(m_pPropSheet);
	m_ServerEvents.Construct(m_pPropSheet);
	m_Performance.Construct(m_pPropSheet);
	m_ServerCommand.Construct(m_pPropSheet);
	m_pPropSheet->Create(this, WS_CHILD | WS_VISIBLE,0); 


   return 0;
}

void CRightBottomView::OnDestroy() 
{
	CPageView::OnDestroy();
   // m_pPropSheet->DestroyWindow();	
}

void CRightBottomView::OnInitialUpdate() 
{
	CPageView::OnInitialUpdate();
	
}

void CRightBottomView::OnSize(UINT nType, int cx, int cy) 
{
   CPageView::OnSize(nType, cx, cy);
   
   ASSERT(m_pPropSheet);
   m_pPropSheet->Resize(cx, cy);
}

BOOL CRightBottomView::OnEraseBkgnd(CDC* pDC) 
{
	// return CPageView::OnEraseBkgnd(pDC);
   return TRUE;
}

void CRightBottomView::OnDraw(CDC* pDC) 
{
   // TODO: Add your specialized code here and/or call the base class
   Doc* pDoc = GetDocument();
   ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CRightBottomView diagnostics

#ifdef _DEBUG
void CRightBottomView::AssertValid() const
{
   CPageView::AssertValid();
}

void CRightBottomView::Dump(CDumpContext& dc) const
{
   CPageView::Dump(dc);
}

#endif //_DEBUG


void CRightBottomView::OnUpdate(CPageView* pSender, LPARAM lHint, CObject* pHint) 
{
	m_pPropSheet->GetActivePage()->Update();


	switch (lHint)
	{
	
		case UVH_CHANGE_MODE:
			{
				CString strValue = ((CMessage*)pHint)->m_strMessage;
				if(strValue.CompareNoCase(_T("Monitoring")) == 0)
				{
					//m_pPropSheet->RemovePage(&m_DetailsServer);
				
				}
				else
				{
				//	m_pPropSheet->AddPage(&m_DetailsServer);
				}
				
			};
  		break;

		
	
	}


}
