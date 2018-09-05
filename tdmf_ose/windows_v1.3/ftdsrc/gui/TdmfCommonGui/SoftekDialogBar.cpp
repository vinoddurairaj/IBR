// SoftekDialogBar.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "SoftekDialogBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSoftekDialogBar

CSoftekDialogBar::CSoftekDialogBar()
{
}

CSoftekDialogBar::~CSoftekDialogBar()
{
}

BEGIN_MESSAGE_MAP(CSoftekDialogBar, CDialogBar)
	//{{AFX_MSG_MAP(CSoftekDialogBar)
	ON_WM_SIZE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSoftekDialogBar message handlers


void CSoftekDialogBar::OnSize(UINT nType, int cx, int cy) 
{
	CDialogBar::OnSize(nType, cx, cy);

	CStatic* pWndLogo = (CStatic*)GetDlgItem(IDC_SOFTEK_LOGO);
	if (pWndLogo != NULL)
	{
		BITMAP bitmap;
		CBitmap::FromHandle(pWndLogo->GetBitmap())->GetBitmap(&bitmap);
		pWndLogo->MoveWindow(cx - bitmap.bmWidth, 0, bitmap.bmWidth, bitmap.bmHeight);
	}
}

void CSoftekDialogBar::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	CRect Rect;
	GetClientRect(&Rect);
	// Adjust Rect
	Rect.left   -= 2;
	//Rect.right  -= 2;
	//Rect.top    -= 1;
	Rect.bottom -= 1;

	CBrush Brush(RGB(255,0,0));

	dc.FillRect(&Rect, &Brush);

	// Do not call CDialogBar::OnPaint() for painting messages
}
