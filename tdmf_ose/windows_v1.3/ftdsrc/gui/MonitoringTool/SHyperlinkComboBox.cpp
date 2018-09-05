// HyperCombo.cpp : implementation file
//

#include "stdafx.h"
#include "SHyperlinkComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int TEXT_VERT_POS = 0;
const int DEFAULT_MINIMUM_WIDTH = 22;
const int BORDER_SIZE = 2;

//=====================================================================================================
// SHyperlinkComboBox
//=====================================================================================================
SHyperlinkComboBox::SHyperlinkComboBox()
{
	m_ForeColor = DEFAULT_FORE_COLOR;
	m_CurrentForeColor = m_ForeColor;

	// Hover
	m_HoverTimerSet = false;
	m_HoverForeColor = DEFAULT_FORE_COLOR;

	m_isColorPicker = false;
	m_ColorPickerMode = ONE_COLOR_MODE;
	
	m_isUsingBackColor = false;

	m_MinimumWidth = DEFAULT_MINIMUM_WIDTH;
}


//=====================================================================================================
//
//=====================================================================================================
SHyperlinkComboBox::~SHyperlinkComboBox()
{
}


//=====================================================================================================
// SHyperlinkComboBox message handler
//=====================================================================================================
BEGIN_MESSAGE_MAP(SHyperlinkComboBox, CComboBox)
	//{{AFX_MSG_MAP(SHyperlinkComboBox)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT_EX(CBN_SELCHANGE, OnSelchange)
	ON_WM_NCHITTEST()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::OnPaint() 
{
	//CPaintDC dc(this); // device context for painting

	CWnd* edit=(CWnd*) GetWindow(GW_CHILD);
	if ((edit) && (edit->m_hWnd))
	{
		edit->ShowWindow(SW_HIDE);
		SetWindowText(_T("CBS_DROPDOWNLIST style must be set"));
	}

	CWnd::OnPaint();

	CDC* pdc = GetDC();
	
	if (pdc != NULL)
	{
		CFont* oldFont = pdc->SelectObject(GetFont());

		CString sText;
		this->GetWindowText(sText);

		CRect rcBounds;
		this->GetWindowRect(&rcBounds);
		this->ScreenToClient(&rcBounds);


		CBrush brush;
		if (m_isUsingBackColor) {
			brush.CreateSolidBrush(m_BackColor);	
		} else {
			brush.CreateSysColorBrush(COLOR_BTNFACE);	
		}

		pdc->FillRect(&rcBounds, &brush);

		if (m_isColorPicker) {
			CRect rcColor(rcBounds);
			rcColor.bottom -= (TRI_SIZE + 2 + BORDER_SIZE);

			CDC memdc;
			CBitmap memBitmap, *memBitmapOld;
			memdc.CreateCompatibleDC(pdc);
			memBitmap.CreateCompatibleBitmap(pdc, rcColor.Width(), rcColor.Height());
			memBitmapOld = memdc.SelectObject(&memBitmap);
		

			CRect memRect;
			memRect.SetRect(0, 0, rcColor.Width(), rcColor.Height());
			if (m_HoverForeColor != m_ForeColor && m_CurrentForeColor == m_HoverForeColor) {
				DrawColorBox(&memdc, GetCurSel(), memRect, true); 
			} else {
				DrawColorBox(&memdc, GetCurSel(), memRect); 
			}
			
			if (IsWindowEnabled()) {
				pdc->BitBlt(rcColor.left, rcColor.top, rcColor.Width(), rcColor.Height(), &memdc, 0, 0, SRCCOPY);
			} else {
				for (int j = 0; j < rcColor.Height(); j++) {
					for (int i = 0; i < rcColor.Width(); i+=2) {
						if ((j%2 == 1) && (i == 0)) i++;
						memdc.SetPixelV(i, j, RGB(192, 192, 192));
					}
				}
				pdc->BitBlt(rcColor.left, rcColor.top, rcColor.Width(), rcColor.Height(), &memdc, 0, 0, SRCCOPY);
			}

		} else {
			if (GetCount() == 0) {
				sText = "";
			} 

			int TextPosX = 0;
			int TextWidth = rcBounds.left + pdc->GetTextExtent(sText).cx + 1;

			rcBounds.right = TextWidth;

			if (rcBounds.right < m_MinimumWidth) {
				rcBounds.right = m_MinimumWidth;
				TextPosX = m_MinimumWidth / 2 - TextWidth / 2;
			}

			SetWindowPos(NULL, rcBounds.left, rcBounds.top, rcBounds.Width(), rcBounds.bottom - rcBounds.top, SWP_NOZORDER | SWP_NOMOVE );
			
			pdc->SetBkMode(TRANSPARENT);
			pdc->SetTextColor(m_CurrentForeColor);
			if (IsWindowEnabled()) {
				pdc->TextOut(TextPosX, TEXT_VERT_POS, sText);
			} else {
				//CBrush grayBrush;
				//grayBrush.CreateSolidBrush (GetSysColor (COLOR_GRAYTEXT));
				pdc->SetTextColor(GetSysColor (COLOR_BTNHIGHLIGHT));

				pdc->TextOut(TextPosX, TEXT_VERT_POS, sText);
				pdc->SetTextColor(GetSysColor (COLOR_GRAYTEXT));
				pdc->TextOut(TextPosX-1, TEXT_VERT_POS-1 , sText);
				//pdc->SetMapMode(MM_TEXT);
				//pdc->GrayString(&grayBrush, NULL, (LPARAM)((LPCTSTR)sText), 0, TextPosX, TEXT_VERT_POS, 0, 0);
			}
		}

		if (IsWindowEnabled()) {
			DrawUnderline(pdc, rcBounds);
		} else {
			DrawUnderline(pdc, rcBounds, false);
		}

		if (!m_isColorPicker) {
			// Increase the Listbox width
			CSize stringSize; 
			long WiderX = 0;	
			for (int i = 0; i < GetCount(); i++)
			{
				CString sItem;
				GetLBText(i, sItem);

				stringSize = pdc->GetTextExtent(sItem);
				if (stringSize.cx > WiderX) {
					WiderX = stringSize.cx;
				}
			}
			SetDroppedWidth(WiderX + 25); // TODO: calculate the 25 value (marge + scroll if present)
		}

		pdc->SelectObject(oldFont);

	}
	else {
	}
}

/////////////////////////////////////////////////////////////////////////////
// TODO: Must be added in a common library
/////////////////////////////////////////////////////////////////////////////
void SHyperlinkComboBox::DrawUnderline(CDC* pDC, CRect rcBounds, bool isEnable)
{
	// Draw the line and the triangle
	int PenWidth = 1;
//	if (GetFocus() == this ) {
//		PenWidth = 2;
//	}

	COLORREF ForeColor;

	if (isEnable) {
		ForeColor = m_CurrentForeColor;
	} else {
		ForeColor = GetSysColor (COLOR_BTNHIGHLIGHT);
	}


	CPen pen(PS_SOLID , PenWidth,  ForeColor);
	CPen* oldPen = pDC->SelectObject(&pen);
	pDC->MoveTo(rcBounds.left, rcBounds.bottom - TRI_SIZE - BORDER_SIZE);

	POINT point[3]= {{rcBounds.right - BORDER_SIZE, rcBounds.bottom - TRI_SIZE - BORDER_SIZE}, 
										{rcBounds.right - BORDER_SIZE - TRI_SIZE_WIDTH, rcBounds.bottom - 1 - BORDER_SIZE}, 
										{rcBounds.right - BORDER_SIZE - TRI_SIZE_WIDTH * 2, rcBounds.bottom - TRI_SIZE - BORDER_SIZE}};
	
	pDC->LineTo(rcBounds.right - 1, rcBounds.bottom - TRI_SIZE - BORDER_SIZE);
	
	CBrush brushFore(ForeColor);
	CBrush* oldBrush = pDC->SelectObject(&brushFore);
	pDC->Polygon(point, 3);

	//Restore Object
	pDC->SelectObject(oldBrush);
	pDC->SelectObject(oldPen);

	// if Disable
	if (!isEnable) {
		rcBounds.left --;
		rcBounds.top --;
		rcBounds.bottom--;
		rcBounds.right--;
		

		CPen pen(PS_SOLID , PenWidth,  GetSysColor (COLOR_GRAYTEXT));
		CPen* oldPen = pDC->SelectObject(&pen);
		pDC->MoveTo(rcBounds.left, rcBounds.bottom - TRI_SIZE - BORDER_SIZE);

		POINT point[3]= {{rcBounds.right - BORDER_SIZE, rcBounds.bottom - TRI_SIZE - BORDER_SIZE}, 
											{rcBounds.right - BORDER_SIZE - TRI_SIZE_WIDTH, rcBounds.bottom - 1 - BORDER_SIZE}, 
											{rcBounds.right - BORDER_SIZE - TRI_SIZE_WIDTH * 2, rcBounds.bottom - TRI_SIZE - BORDER_SIZE}};
		
		pDC->LineTo(rcBounds.right - 1, rcBounds.bottom - TRI_SIZE - BORDER_SIZE);
		
		CBrush brushFore(GetSysColor (COLOR_GRAYTEXT));
		CBrush* oldBrush = pDC->SelectObject(&brushFore);
		pDC->Polygon(point, 3);
		pDC->SelectObject(oldBrush);
		pDC->SelectObject(oldPen);
	}

}


//=====================================================================================================
//
//=====================================================================================================
BOOL SHyperlinkComboBox::OnSelchange() 
{
	// TODO: find why both are necessary. PostMessage is not necessare if old combo is not paint.
	Refresh();
	PostMessage(WM_PAINT, 0, 0);	
	return FALSE;
}



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::OnSetFocus(CWnd* pOldWnd) 
{
	CComboBox::OnSetFocus(pOldWnd);
	Refresh();
}



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::OnKillFocus(CWnd* pNewWnd) 
{
	CComboBox::OnKillFocus(pNewWnd);
	Refresh();
}


//=====================================================================================================
//
//=====================================================================================================
UINT SHyperlinkComboBox::OnNcHitTest(CPoint point) 
{

	if (m_HoverForeColor != m_ForeColor)
	{
		if (!m_HoverTimerSet) 
		{
			m_CurrentForeColor = m_HoverForeColor;
			SendMessage(WM_PAINT, 0, 0);	
			SetTimer(HOVER_TIMER_ID, 10, NULL);
			m_HoverTimerSet = true;
		}
	}

	return CComboBox::OnNcHitTest(point);
}



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::AddColor(long colorA, long colorB, long colorC) 
{	
	m_ColorPickerMode = THREE_COLOR_MODE;
	m_isColorPicker = true;
	CString text;
	text.Format(_T("%lu,%lu,%lu"), colorA, colorB, colorC);

	int ret = AddString(text);
}



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::AddColor(long color) 
{	
	m_ColorPickerMode = ONE_COLOR_MODE;
	m_isColorPicker = true;
	CString text;
	text.Format(_T("%lu"), color);

	int ret = AddString(text);
}



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == HOVER_TIMER_ID)
	{
		POINT pt;
		GetCursorPos(&pt);
		CRect rcItem;
		GetWindowRect(&rcItem);

		// if the mouse cursor within the control?
		if(!this->GetDroppedState() && !rcItem.PtInRect(pt)) 
		{
			KillTimer(HOVER_TIMER_ID);

			m_HoverTimerSet = false;

			//if (!m_fGotFocus) {
				m_CurrentForeColor = m_ForeColor;
				SendMessage(WM_PAINT, 0, 0);	
			//}
			return;
		}
	}

	CComboBox::OnTimer(nIDEvent);
}


//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC dc;
	
	if (!dc.Attach(lpDrawItemStruct->hDC))
		return;

	if (lpDrawItemStruct->itemAction & ODA_FOCUS)
	{
		DrawColorBox(&dc, lpDrawItemStruct->itemID, lpDrawItemStruct->rcItem, true);
	}
	else 
	{
		DrawColorBox(&dc, lpDrawItemStruct->itemID, lpDrawItemStruct->rcItem);
	}
	
	dc.Detach();	
}



//=====================================================================================================
//
//=====================================================================================================
void SHyperlinkComboBox::DrawColorBox(CDC* pdc, long itemID, CRect rect, bool isFocus) 
{
	if (m_isColorPicker) {
		CRect rcColor(rect);

		if (m_ColorPickerMode == THREE_COLOR_MODE) {
			//First part
			int part = rcColor.Width() / 3;
			rcColor.right = part;
			pdc->FillSolidRect(&rcColor, GetColorA(itemID));

			//Second part
			rcColor.left = rcColor.right;
			rcColor.right = rcColor.left + part;
			pdc->FillSolidRect(&rcColor, GetColorB(itemID));
		
			//Third part
			rcColor.left = rcColor.right;
			rcColor.right = rcColor.left + part;
			pdc->FillSolidRect(&rcColor, GetColorC(itemID));

		} else {
			int part = rcColor.Width();
			pdc->FillSolidRect(&rcColor, GetColor(itemID));
		}

		if (isFocus) {
			pdc->Draw3dRect(rect, RGB(0,0,0), RGB(255,255,255));
		} else {
			pdc->Draw3dRect(rect, RGB(255,255,255), RGB(0,0,0));
		}		
	} 
}



//=====================================================================================================
// ONE_COLOR_MODE
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetColor(int itemID) {
	if (itemID < 0) return -1;

	CString color;
	if (GetLBTextLen(itemID) > 0) {
		GetLBText(itemID, color);
	}
	return _ttol(color);
}



//=====================================================================================================
// THREE_COLOR_MODE
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetColorA(int itemID) {
	if (itemID < 0) return -1;

	CString color;
	if (GetLBTextLen(itemID) > 0) {
		GetLBText(itemID, color);
	}

	return _ttol(color.Left(color.FindOneOf(_T(","))));
}



//=====================================================================================================
// THREE_COLOR_MODE
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetColorB(int itemID) {
	if (itemID < 0) return -1;

	CString color;
	GetLBText(itemID, color);

	return _ttol(color.Mid(color.FindOneOf(_T(",")) + 1, color.ReverseFind(',') - color.FindOneOf(_T(",")) - 1));
}



//=====================================================================================================
// THREE_COLOR_MODE
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetColorC(int itemID) {
	if (itemID < 0) return -1;

	CString color;
	GetLBText(itemID, color);

	return _ttol(color.Right(color.GetLength() - color.ReverseFind(',') - 1));
}



//=====================================================================================================
// 
//=====================================================================================================
int SHyperlinkComboBox::SetCurSel(int id) {
	
	if (m_hWnd) { 
		int ret = CComboBox::SetCurSel(id);
		SendMessage(WM_PAINT, 0, 0);	

		return ret;
	}
	return CB_ERR;
}



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlinkComboBox::Refresh() {
	SendMessage(WM_PAINT, 0, 0);	
}



//=====================================================================================================
// 
//=====================================================================================================
int SHyperlinkComboBox::DeleteString(UINT nIndex) {	
	if ((UINT)GetCurSel() == nIndex) {
		if ( (nIndex == 0) && (GetCount() >= 2)) {
			SetCurSel(1);								
		} else {
			SetCurSel(0);				
		}
	}

	int ret = CComboBox::DeleteString(nIndex);

	SendMessage(WM_PAINT, 0, 0);	

	return ret;
}



//=====================================================================================================
// 
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetForeColor() 
{
	return m_ForeColor;	
}



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlinkComboBox::SetForeColor(COLORREF color) 
{
	m_ForeColor = color;	
	m_CurrentForeColor = m_ForeColor;
}



//=====================================================================================================
// 
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetBackColor() 
{
	return m_BackColor;	
}



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlinkComboBox::SetBackColor(COLORREF color) 
{
	m_BackColor = color;	
	Refresh();
}



//=====================================================================================================
// 
//=====================================================================================================
BOOL SHyperlinkComboBox::DestroyWindow() 
{
		
	return CComboBox::DestroyWindow();
}


//=====================================================================================================
// 
//=====================================================================================================
long SHyperlinkComboBox::GetMinimumWidth() 
{
	return m_MinimumWidth;	
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlinkComboBox::SetMinimumWidth(long value) 
{
	m_MinimumWidth = value;	
}


//=====================================================================================================
// 
//=====================================================================================================
COLORREF SHyperlinkComboBox::GetHoverForeColor() 
{
	return m_HoverForeColor;	
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlinkComboBox::SetHoverForeColor(COLORREF color) 
{
	m_HoverForeColor = color;	
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlinkComboBox::SetUsingBackColor(bool value) 
{
	m_isUsingBackColor = value;
}


//=====================================================================================================
// 
//=====================================================================================================
bool SHyperlinkComboBox::isUsingBackColor() 
{
	return m_isUsingBackColor;
}

void SHyperlinkComboBox::OnLButtonDown(UINT nFlags, CPoint point) 
{
	GetParent()->SendMessage ( WM_PARENTNOTIFY, WM_LBUTTONDOWN, 0 );	
	CComboBox::OnLButtonDown(nFlags, point);
}
