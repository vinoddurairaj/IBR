// SVHeaderCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "TDMFGUI.h"
#include "SVHeaderCtrl.h"
#include "SVBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SVHeaderCtrl

SVHeaderCtrl::SVHeaderCtrl()
{
}

SVHeaderCtrl::~SVHeaderCtrl()
{
}


BEGIN_MESSAGE_MAP(SVHeaderCtrl, CHeaderCtrl)
	//{{AFX_MSG_MAP(SVHeaderCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SVHeaderCtrl message handlers

void SVHeaderCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// just disable this so double click a column header does nothing	
//	CHeaderCtrl::OnLButtonDblClk(nFlags, point);
}

void SVHeaderCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CListCtrl* plc = (CListCtrl*)GetParent();
	SVBase* pv = (SVBase*)plc->GetParent();
  
	LVHITTESTINFO  lvhit;
	CPoint cpClient = point;
	ScreenToClient(&cpClient);
	// now take into account the window scroll shift 
	cpClient.x -= plc->GetScrollPos(SB_HORZ );
	lvhit.pt = cpClient;
	int nItem = plc->SubItemHitTest(&lvhit);
	pv->LaunchColumnSelector(lvhit.iSubItem);	// go straight to column configurator if header right clicked
}
void SVHeaderCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	// get pixel at point, if column move dont do CHeaderCtrl::OnLButtonDown
	
	CHeaderCtrl::OnLButtonDown(nFlags, point);
}

void SVHeaderCtrl::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	CHeaderCtrl::OnWindowPosChanging(lpwndpos);
	
	RedrawWindow();
	
}
