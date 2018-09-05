#include "stdafx.h"
#include "SHyperlink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const long HOVER_TIMER_ID = 1;
const COLORREF DEFAULT_FORE_COLOR = RGB(0,0,210);

const int TRI_SIZE_HEIGHT = 5;
const int TRI_SIZE_WIDTH = 8;

// Menu items
const int FIRST_ITEM_ID = 7500;
const int MAX_NB_ITEM = 100;

const int MINIMUM_SIZE = 10;

const int ID_BUTTON = 13500;


//=====================================================================================================
// 
//=====================================================================================================
SHyperlink::SHyperlink()
{
	m_isUsingBackColor = false;
	m_BackColor = -1;

	m_ForeColor = DEFAULT_FORE_COLOR;
	m_CurrentForeColor = m_ForeColor;

	// Hover
	m_HoverTimerSet = false;
	m_HoverForeColor = DEFAULT_FORE_COLOR;

	m_CurSel = -1;

	m_sMenuTitle = "";

	m_OperationMode = HL_MODE;
}



//=====================================================================================================
// 
//=====================================================================================================
SHyperlink::~SHyperlink()
{
}



//=====================================================================================================
// 
//=====================================================================================================
BEGIN_MESSAGE_MAP(SHyperlink, CStatic)
	//{{AFX_MSG_MAP(SHyperlink)
	ON_WM_PAINT()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_CREATE()
	ON_WM_NCHITTEST()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
  ON_COMMAND_RANGE(FIRST_ITEM_ID, FIRST_ITEM_ID + MAX_NB_ITEM, OnFileMenuItems)
	ON_BN_CLICKED(ID_BUTTON, OnButtonClicked)
END_MESSAGE_MAP()



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::OnPaint() 
{
	CStatic::OnPaint();

	// BUTTON MODE
	if (m_OperationMode == BUTTON_MODE)
	{
		if (m_Button.m_hWnd == NULL) {
			m_Button.Create(_T(""), WS_TABSTOP  | WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CRect(0,0,0,0), this, ID_BUTTON);
			m_Button.SetFont(this->GetFont());

			m_Button.LoadBitmaps(m_nIDBitmapResource, m_nIDBitmapResourceSel, m_nIDBitmapResourceFocus, m_nIDBitmapResourceDisabled);
			m_Button.SizeToContent();

			CRect buttonRect;
			m_Button.GetWindowRect(buttonRect);
		  //MoveWindow(buttonRect);
			SetWindowPos(NULL, buttonRect.left, buttonRect.top, buttonRect.Width(), buttonRect.Height(), SWP_NOZORDER | SWP_NOMOVE );
		}
		if (GetFocus() == this ) {
			m_Button.SetFocus();
		}
		m_Button.RedrawWindow();
	}

	// HYPERLINK MODE
	if (m_OperationMode == HL_MODE)
	{
		CRect rcBounds;
		GetWindowRect(&rcBounds);
		ScreenToClient(&rcBounds);

		CDC* pDC = GetDC();
		CFont* oldFont = pDC->SelectObject(this->GetFont());

		CString sText;
		GetWindowText(sText);

		CBrush brush;
		if (m_isUsingBackColor) {
			brush.CreateSolidBrush(m_BackColor);	
		} else {
			brush.CreateSysColorBrush(COLOR_BTNFACE);	
		}

		pDC->FillRect(&rcBounds, &brush);

		int width = pDC->GetTextExtent(sText).cx ;
		if (width	< MINIMUM_SIZE && width !=0) {
			width = MINIMUM_SIZE;
		}
		rcBounds.right = rcBounds.left + width;

		SetWindowPos(NULL, rcBounds.left, rcBounds.top, rcBounds.Width(), pDC->GetTextExtent("|WpQq").cy + TRI_SIZE_HEIGHT, SWP_NOZORDER | SWP_NOMOVE );

		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(m_CurrentForeColor);

		if (IsWindowEnabled()) {
			pDC->TextOut(0, 0, sText);
		} else {
			// Disabled
			pDC->SetTextColor(GetSysColor (COLOR_BTNHIGHLIGHT));
			pDC->TextOut(0, 0, sText);

			pDC->SetTextColor(GetSysColor (COLOR_GRAYTEXT));
			pDC->TextOut(-1, -1 , sText);
		}

		if (width != 0) {
			if (IsWindowEnabled()) {
				DrawUnderline(pDC, rcBounds);
			} else {
				DrawUnderline(pDC, rcBounds, false);
			}
		}
		pDC->SelectObject(oldFont);
	}	
	// Do not call CStatic::OnPaint() for painting messages
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::OnButtonClicked() 
{
	OnClicked();
}

/////////////////////////////////////////////////////////////////////////////
// TODO: Must be added in a common library
/////////////////////////////////////////////////////////////////////////////
void SHyperlink::DrawUnderline(CDC* pDC, CRect rcBounds, bool isEnable)
{
	// Draw the line and the triangle
	int PenWidth = 1;
	if (GetFocus() == this ) {
		PenWidth = 2;
	}

	COLORREF ForeColor;

	if (isEnable) {
		ForeColor = m_CurrentForeColor;
	} else {
		ForeColor = GetSysColor (COLOR_BTNHIGHLIGHT);
	}


	CPen pen(PS_SOLID , PenWidth,  ForeColor);
	CPen* oldPen = pDC->SelectObject(&pen);
	pDC->MoveTo(rcBounds.left, rcBounds.bottom - TRI_SIZE_HEIGHT);

	POINT point[3]= {{rcBounds.right - 1, rcBounds.bottom - TRI_SIZE_HEIGHT}, 
										{rcBounds.right - 1 - TRI_SIZE_WIDTH / 2, rcBounds.bottom - 1}, 
										{rcBounds.right - 1 - TRI_SIZE_WIDTH, rcBounds.bottom - TRI_SIZE_HEIGHT }};
	
	pDC->LineTo(rcBounds.right - 1, rcBounds.bottom - TRI_SIZE_HEIGHT);
	
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
		pDC->MoveTo(rcBounds.left, rcBounds.bottom - TRI_SIZE_HEIGHT );

		POINT point[3]= {{rcBounds.right -1 , rcBounds.bottom - TRI_SIZE_HEIGHT }, 
											{rcBounds.right -1  - TRI_SIZE_WIDTH / 2, rcBounds.bottom - 1 }, 
											{rcBounds.right -1  - TRI_SIZE_WIDTH, rcBounds.bottom - TRI_SIZE_HEIGHT }};
		
		pDC->LineTo(rcBounds.right - 1, rcBounds.bottom - TRI_SIZE_HEIGHT );
		
		CBrush brushFore(GetSysColor (COLOR_GRAYTEXT));
		CBrush* oldBrush = pDC->SelectObject(&brushFore);
		pDC->Polygon(point, 3);
		pDC->SelectObject(oldBrush);
		pDC->SelectObject(oldPen);
	}

}

/*
//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::DrawUnderline(CDC* pDC, CRect rcBounds)
{
	// Draw the line and the ???
	int PenWidth = 1;

	CPen pen(PS_SOLID , PenWidth,  m_CurrentForeColor);
	CPen* oldPen = pDC->SelectObject(&pen);
	pDC->MoveTo(rcBounds.left, rcBounds.bottom - TRI_SIZE_HEIGHT);

	POINT point[3]= {{rcBounds.right-1 , rcBounds.bottom - TRI_SIZE_HEIGHT}, 
										{rcBounds.right -1 - TRI_SIZE_WIDTH, rcBounds.bottom - 1}, 
										{rcBounds.right -1 - 2*TRI_SIZE_WIDTH , rcBounds.bottom - TRI_SIZE_HEIGHT}};
	
	pDC->LineTo(rcBounds.right, rcBounds.bottom-TRI_SIZE_HEIGHT);
	if (GetFocus() == this ) {
		pDC->MoveTo(rcBounds.left, rcBounds.bottom - TRI_SIZE_HEIGHT + 1);
		pDC->LineTo(rcBounds.right, rcBounds.bottom-TRI_SIZE_HEIGHT + 1);
	}
	
	CBrush brushFore(m_CurrentForeColor);
	CBrush* oldBrush = pDC->SelectObject(&brushFore);
	pDC->Polygon(point, 3);
	pDC->SelectObject(oldPen);

	//pDC->FillRect(CRect(rcBounds.right , rcBounds.bottom - TRI_SIZE_HEIGHT, rcBounds.right - TRI_SIZE_HEIGHT , rcBounds.bottom), &brushFore), 
	//Restore Object
	pDC->SelectObject(oldBrush);
}*/


//=====================================================================================================
// This function adds a title entry to a popup menu
//=====================================================================================================
void SHyperlink::AddMenuTitle(CMenu* popup, LPCTSTR title)
{
	// insert a separator item at the top
	popup->InsertMenu(0, MF_BYPOSITION | MF_SEPARATOR, 0, title);

	// insert title item
	// note: item is not selectable (disabled) but not grayed
	popup->InsertMenu(0, MF_BYPOSITION | MF_STRING | MF_DISABLED, 0, title);

	SetMenuDefaultItem(popup->m_hMenu, 0, TRUE);
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::PreSubclassWindow() 
{
	CRect rcBounds;
	GetWindowRect(&rcBounds);
	ScreenToClient(&rcBounds);
	
	CStatic::PreSubclassWindow();

	PostMessage(WM_PAINT, 0, 0);	
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::OnKillFocus(CWnd* pNewWnd) 
{
	CStatic::OnKillFocus(pNewWnd);
	
	Refresh();
}



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::OnSetFocus(CWnd* pOldWnd) 
{
	CStatic::OnSetFocus(pOldWnd);
	
	Refresh();
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::Refresh() {
	SendMessage(WM_PAINT, 0, 0);	
}


//=====================================================================================================
// 
//=====================================================================================================
BOOL SHyperlink::OnClicked() 
{
	SetFocus();	

	CRect rect;
	GetWindowRect(&rect);
		
	CMenu menu;
	menu.CreatePopupMenu();

	for (int i = 0; i < m_arrayString.GetSize(); i++) {
		if (m_arrayString[i] == _T("")) {
			menu.AppendMenu(MF_SEPARATOR, 0, _T(""));
		} else {
			menu.AppendMenu(MF_STRING, FIRST_ITEM_ID + i, m_arrayString[i]);
		}
	}
	
	if (m_sMenuTitle != _T("")) {
		AddMenuTitle(&menu, m_sMenuTitle);
	}

	menu.TrackPopupMenu(TPM_LEFTALIGN, rect.left, rect.bottom,  this);

	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SHyperlink::OnFileMenuItems(UINT nID)
{
	//CString tmp;
	//tmp.Format("Item selected is: %d", nID - 7500);
	//MessageBox(tmp);
	m_CurSel = nID - FIRST_ITEM_ID;

	SetCurSel(m_CurSel - GetNBSeparator(m_CurSel));

	//SetWindowText(m_arrayString[m_CurSel]);
	//Refresh();
	GetParent()->PostMessage(WM_S_HL_CHANGED, (WPARAM)this, m_CurSel);

}


//=====================================================================================================
// 
//=====================================================================================================
int SHyperlink::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CRect rcBounds;
	GetWindowRect(&rcBounds);
	ScreenToClient(&rcBounds);
	
	PostMessage(WM_PAINT, 0, 0);	
	
	return 0;
}


//=====================================================================================================
// 
//=====================================================================================================
COLORREF SHyperlink::GetForeColor() 
{
	return m_ForeColor;	
}



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetForeColor(COLORREF color) 
{
	m_ForeColor = color;	
	m_CurrentForeColor = m_ForeColor;
}


//=====================================================================================================
// 
//=====================================================================================================
COLORREF SHyperlink::GetBackColor() 
{
	return m_BackColor;	
}



//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetBackColor(COLORREF color) 
{
	m_BackColor = color;	
	Refresh();
}


//=====================================================================================================
// 
//=====================================================================================================
UINT SHyperlink::OnNcHitTest(CPoint point) 
{
	if (m_HoverForeColor != m_ForeColor)
	{
		if (!m_HoverTimerSet) {
			m_CurrentForeColor = m_HoverForeColor;
			SendMessage(WM_PAINT, 0, 0);	
			SetTimer(HOVER_TIMER_ID, 10, NULL);
			m_HoverTimerSet = true;
		}
	}
	
	return CStatic::OnNcHitTest(point);
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == HOVER_TIMER_ID)
	{
		POINT pt;
		GetCursorPos(&pt);
		CRect rcItem;
		GetWindowRect(&rcItem);

		// if the mouse cursor within the control?
		if(/*!this->GetDroppedState() &&*/ !rcItem.PtInRect(pt)) {
			KillTimer(HOVER_TIMER_ID);

			m_HoverTimerSet = false;

			//if (!m_fGotFocus) {
				m_CurrentForeColor = m_ForeColor;
				SendMessage(WM_PAINT, 0, 0);	
			//}
			return;
		}
	}
	
	CStatic::OnTimer(nIDEvent);
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_SPACE) {
		OnClicked(); 
	}

	CStatic::OnKeyDown(nChar, nRepCnt, nFlags);
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetUsingBackColor(bool value) 
{
	m_isUsingBackColor = value;
}

//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::ResetContent() 
{
	m_arrayString.RemoveAll();
	SetCurSel(-1);
	SetWindowText(_T(""));
	Refresh();
}


//=====================================================================================================
// 
//=====================================================================================================
bool SHyperlink::isUsingBackColor() 
{
	return m_isUsingBackColor;
}


//=====================================================================================================
// 
//=====================================================================================================
COLORREF SHyperlink::GetHoverForeColor() 
{
	return m_HoverForeColor;	
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetHoverForeColor(COLORREF color) 
{
	m_HoverForeColor = color;	
}


//=====================================================================================================
// 
//=====================================================================================================
long SHyperlink::GetUserData() {
	return m_UserData;
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetUserData(long UserData){
	m_UserData = UserData;
}


//=====================================================================================================
// 
//=====================================================================================================
long SHyperlink::GetNBSeparator(int pos) {
	int ret = 0;
	if ((pos < 0 ) && (pos >= m_arrayString.GetSize())) {
		pos = -1;
	}

	if (pos == -1) {
		pos = m_arrayString.GetSize();
	}

	for (int i = 0; i < pos; i++) {
		if (m_arrayString[i] == "") {
			ret++;
		}
	}
	return ret;
}


//=====================================================================================================
// 
//=====================================================================================================
long SHyperlink::GetCurSel() {
	return m_CurSel;
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetCurSel(long CurSel){
	if ((CurSel >= 0 ) && (CurSel < GetCount())) {
		m_CurSel = CurSel;

		CString sText = m_arrayString[ConvertPos(CurSel)];
		sText.TrimLeft();
		SetWindowText(sText);
	} else {
		m_CurSel = -1;
		SetWindowText(_T(""));
	}
	Refresh();
}


//=====================================================================================================
// 
//=====================================================================================================
long SHyperlink::ConvertPos(long pos) {
	int stringCount = -1;
	int arrayPos;
	// Skip separators 
	for (arrayPos = 0; arrayPos < m_arrayString.GetSize(); arrayPos++) {
		if (m_arrayString[arrayPos] != "") {
			stringCount++;
		}
		if (stringCount == pos)
			break;
	}
	return arrayPos;
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::GetLBText(int nIndex, CString& rString) {
	if ((nIndex >= 0 ) && (nIndex < GetCount())) {

		rString = m_arrayString[ConvertPos(nIndex)];
	} else {
		rString = "";
	}
}


//=====================================================================================================
// 
//=====================================================================================================
CString SHyperlink::GetMenuTitle() {
	return m_sMenuTitle;
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetMenuTitle(CString sMenuTitle){
	m_sMenuTitle = sMenuTitle;
}


//=====================================================================================================
// 
//=====================================================================================================
int SHyperlink::GetCount() {
	return m_arrayString.GetSize() - GetNBSeparator(-1);
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::AddString(LPCTSTR value){
	m_arrayString.Add(value);
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetWindowText(LPCTSTR value) {
	switch (m_OperationMode ) {
		case HL_MODE:
			CStatic::SetWindowText(value);
			break;

		case BUTTON_MODE:
			//m_Button.SetWindowText(value);
			//CStatic::SetWindowText(value);
			break;

	}
}


//=====================================================================================================
// Deprecated, LoadBitmaps must be used instead
//=====================================================================================================
void SHyperlink::SetBitmaps(UINT nIDBitmapResource, UINT nIDBitmapResourceSel, UINT nIDBitmapResourceFocus, UINT nIDBitmapResourceDisabled) {
	LoadBitmaps(nIDBitmapResource, nIDBitmapResourceSel, nIDBitmapResourceFocus, nIDBitmapResourceDisabled);
}


//=====================================================================================================
// 
//=====================================================================================================
BOOL SHyperlink::LoadBitmaps(UINT nIDBitmapResource, UINT nIDBitmapResourceSel, UINT nIDBitmapResourceFocus, UINT nIDBitmapResourceDisabled) {
	switch (m_OperationMode ) {
		case HL_MODE:
			break;

		case BUTTON_MODE:
			m_nIDBitmapResource = nIDBitmapResource;
			m_nIDBitmapResourceSel = nIDBitmapResourceSel;
			m_nIDBitmapResourceFocus = nIDBitmapResourceFocus;
			m_nIDBitmapResourceDisabled = nIDBitmapResourceDisabled;
			break;

	}

	return TRUE;
}


//=====================================================================================================
// 
//=====================================================================================================
void SHyperlink::SetOperationMode(int value ) {
	m_OperationMode = value;
}


/*
//=====================================================================================================
// 
//=====================================================================================================
BOOL SHyperlink::ShowWindow( int nCmdShow ) {
	switch (m_OperationMode ) {
		case HL_MODE:
			break;

		case BUTTON_MODE:
			if (::IsWindow(m_Button.m_hWnd))
			m_Button.ShowWindow(nCmdShow);;
			break;

	}
	int ret = CStatic::ShowWindow(nCmdShow);
	return ret;

}

*/