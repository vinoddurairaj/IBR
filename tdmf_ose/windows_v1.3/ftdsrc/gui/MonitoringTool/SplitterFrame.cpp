// SplitterFrame.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "SplitterFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SplitterFrame

BEGIN_MESSAGE_MAP(SplitterFrame, CFrameWnd)
	//{{AFX_MSG_MAP(SplitterFrame)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SplitterFrame message handlers

BOOL SplitterFrame::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}
