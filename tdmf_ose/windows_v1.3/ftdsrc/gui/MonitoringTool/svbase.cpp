// SVBase.cpp : implementation file
//


/*
	Right Click column header goes straight to Column Configurator
	Changed how selected rows are determined
	Selected Circle now takes pen of text color
	Export completed
	Fixed but in UpdateObject (lookaside buffer should be destroyed before repaint) and limit repaint to number of objects in table
	MESSAGE_DETAIL was being sent twice on mouse clicks
	PostMessage done instead of SendMessage
	SVFileSystem :: Change default justification for server from center to left
	Separated cell draw routines from OnDrawItem (so Print can call it) No visible difference
	SVGroup Added - Required adding to project
	SVLog Added - Required adding to project
	SVDExportFile Added
	IDD_V_EXPORTFILEOPEN	added dialog resource
	Improved SVDColumnSelector
	IDS_V_EXPORTFILENAME		added string resource
	IDS_V_EXPORTFILETYPECOMMA	added string resource
	IDS_V_EXPORTFILETYPETAB		added string resource
	SVFile.cpp - since bText is now supported for Booleans, the default was changed to set bText = false
	SVDefineRule SVDefineRuleC SVDefineRuleD SVDefineRuleN should be removed from project
	///////////////////////////////////////////////////////////////////
	// above sent 1/25/02
	///////////////////////////////////////////////////////////////////
	Added SVHeaderCtrl to disable doubleclick on column header (must add to project)
	If column width is being dragged to less than 20 pixels, hide the column
	Don't draw extender bar on bars (scatter points only)
	Added switch to fix graph part instead of text, requires SVColumn and SVSheet updates
	Added bTotals field in SVColumns. Not implemented though. Will allow the column to be totalled on the total bar
	Gray menu items if they are not applicable
	Added nPixelFixedWidth to force a size for fixed fields, otherwise m_nNumMarginUnits, etc. used
	remove resource.hm
// After Feb 6th
	Stopped calling SendDetailMessage twice
	Added Select All function (menu)
	allow variable dragging
	allow graphic portion fixed
	Registry Key processor (add to project SVRegKey)
	String entry IDS_REGISTRYNAME
	String Entry IDS_V_FILEALREADYEXISTS
	// following 4 are added as IDS_USERREG to identify all possible entries in the CURRENT_USER area
	String Entry IDS_USERREG_EXPORTFILENAME	
	String entry IDS_USERREG_EXPORTHEADINGS
	String entry IDS_USERREG_EXPORTDELIMITER
	String entry IDS_USERREG_EXPORTRANGE
	Menu IDR_V_LISTVIEW_CONTEXT changed (added Export...and Select All)
// after 2/15/02
	Fix bug in like/unlike column rule
	Resource dialog static change
			dialog		IDD_V_EXPORTFILEOPEN
			control		IDC_CHECK_HEADINGS
			new value	Include Headings as First Row
	added sort loop counter while in debug mode (perhaps a progress indicator if trying to sort a large number of rows)
	Column Selector not initializing to right column if scrolled (fix in SVHeaderCtrl)
	Redid right click on header. intent is to go straight to column selector. Previosu version did not work if view was scrolled
	support for ignore case on rules
*/
#include "stdafx.h"
#include "SVGlobal.h"
#include "MonitorRes.h"    

#include "SVBase.h"
#include "SVRegKey.h"
#include "SVDColumnSelector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define COLOREXTENDER RGB(128, 128, 128)	// color of the line that the scatter plots center
#define COLORFRAME RGB(128, 128, 128)		// color of the frame around the bar graph (if bar in frame chosen)


#define MAXSTRINGWIDTH 1000
#define DRAGMINIMUMWIDTH 20
int bSortDirection;
#define HEADERID 0		// see article Q281155

// set if m_pHdrImageList image list (see OnOnitialUpdate)
#define IMAGERULEONLY 4
#define IMAGEUPARROW 0	
#define IMAGEDOWNARROW 1
#define IMAGERULEOFFSETS 2  // imageuparrow + imageoffset = imageuparrowrule
#define IMAGEOFF -1

#define CELLHGRAPHMARGIN 6	// horizontal margin in a cell
#define CELLVGRAPHMARGIN 2	// vertical   margin in a cell


extern CFont*	gpAppFont;
/////////////////////////////////////////////////////////////////////////////
// SVBase

IMPLEMENT_DYNCREATE(SVBase, CPageView)

SVBase::SVBase()
 
{
	//{{AFX_DATA_INIT(SVBase)
	//}}AFX_DATA_INIT

	ASSERT (_WIN32_IE >= 0x0300);		// required for LVCOLUMN.iOrder

	m_nLastSort = -1;
	m_pnColHdrTable = 0;

	
	m_pHdrImageList = 0;
	m_pImageList = 0;

	m_ptSelectedCell.x = -1;
	m_ptSelectedCell.y = -1;
	m_bInit = false;
	m_bDisabled = true;
	m_pActiveSheet = 0;
	m_nLastMessageSentObject = -1;	// last object number that a message detail was sent to frame

}

SVBase::~SVBase()
{
	delete []m_pnColHdrTable;
	delete m_pHdrImageList;
	delete m_pImageList;
}

void SVBase::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SVBase)
	DDX_Control(pDX, IDC_BUTTON_TOTALLINE, m_btTotal);
	DDX_Control(pDX, IDC_LISTCTRL, m_lvMain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SVBase, CPageView)
	//{{AFX_MSG_MAP(SVBase)
	ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
	ON_WM_DRAWITEM()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LISTCTRL, OnColumnclickListctrl)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTCTRL, OnListctrlItemChanged)
	ON_COMMAND(ID_COLUMN_SELECTOR, OnColumnSelector)
	ON_COMMAND(ID_SELECT_SIMILAR, OnSelectSame)
	ON_COMMAND(ID_DELETE_SELECTED_ROWS, OnHideSelected)
	ON_COMMAND(ID_DELETE_NONSELECTED_ROWS, OnHideNotSelected)
	ON_COMMAND(ID_RECOVER_DELETED, OnRecoverDeleted)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_SELECT_ALL, OnSelectAll)
	ON_NOTIFY(NM_CLICK, IDC_LISTCTRL, OnClickListctrl)
	ON_NOTIFY(HDN_ENDTRACK, HEADERID, OnColumnHeaderEndTrack)
	ON_NOTIFY(HDN_ENDDRAG, HEADERID, OnColumnHeaderEndDrag)
	ON_NOTIFY(HDN_ITEMCHANGING, HEADERID, OnColumnHeaderChanging)
	ON_UPDATE_COMMAND_UI(ID_DELETE_SELECTED_ROWS, OnHideSelectUI)
	ON_UPDATE_COMMAND_UI(ID_DELETE_NONSELECTED_ROWS, OnHideNotSelectUI)
	ON_UPDATE_COMMAND_UI(ID_RECOVER_DELETED, OnRevealUI)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP


 
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SVBase diagnostics

#ifdef _DEBUG
void SVBase::AssertValid() const
{
	CPageView::AssertValid();
}

void SVBase::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// SVBase message handlers


void SVBase::OnDraw(CDC* pDC)
{

}

void SVBase::OnInitialUpdate() 
{
	if (m_bInit)
		return;


//	CSize csize = GetTotalSize( );
	CSize csize;
//	csize.cy = 1;
//	SetScrollSizes(MM_TEXT, csize);

	m_lvMain.Create ( WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_OWNERDRAWFIXED, CRect ( 0, 0, 1, 1), this, IDC_LISTCTRL );
	m_btTotal.Create ( CString(""), WS_VISIBLE | BS_OWNERDRAW | WS_CHILD, CRect ( 0, 0, 1, 1 ), this, IDC_BUTTON_TOTALLINE );
	m_btTotal.SetFont ( gpAppFont );

	DWORD dwStyle = GetListCtrl().GetExtendedStyle()|LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP;
	//ModifiedSelectDWORD dwStyle = GetListCtrl().GetExtendedStyle()|LVS_EX_HEADERDRAGDROP ;
	GetListCtrl().SetExtendedStyle(dwStyle);	
	

	m_pHdrImageList = new CImageList();
	m_pHdrImageList->Create(32, 16, ILC_MASK, 5, 0);
	m_pHdrImageList->Add(AfxGetApp()->LoadIcon(IDI_V_HDR_UPARROWNORULE));
	m_pHdrImageList->Add(AfxGetApp()->LoadIcon(IDI_V_HDR_DOWNARROWNORULE));
	m_pHdrImageList->Add(AfxGetApp()->LoadIcon(IDI_V_HDR_UPARROWRULE));
	m_pHdrImageList->Add(AfxGetApp()->LoadIcon(IDI_V_HDR_DOWNARROWRULE));
	m_pHdrImageList->Add(AfxGetApp()->LoadIcon(IDI_V_HDR_RULEONLY));
	GetListCtrl().GetHeaderCtrl()->SetImageList(m_pHdrImageList);

	m_pImageList = new CImageList();
	m_pImageList->Create(32, 16, ILC_MASK, 2, 0);
	m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_V_TRUE));
	m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_V_FALSE));

	CHeaderCtrl* pHdr = GetListCtrl().GetHeaderCtrl();

	m_nHdrColCount = pHdr->GetItemCount();
	if(m_nHdrColCount > 0)
		m_pnColHdrTable = new int[m_nHdrColCount];


	CClientDC dc(this);
	COleDateTime cdt = COleDateTime::GetCurrentTime();
	CString cs = cdt.Format();
	csize = dc.GetTextExtent(cs);
	m_nDateTimeMargin = csize.cx + csize.cx/10;
	csize = dc.GetTextExtent(_T("88,888,888,888.88"));
	m_nNumMarginLarge = csize.cx + csize.cx/4;
	csize = dc.GetTextExtent(_T("88.88"));
	m_nNumMarginSmall = csize.cx + csize.cx/4;
	csize = dc.GetTextExtent(_T("88 WWW"));
	m_nNumMarginUnits = csize.cx + csize.cx/4;
	m_bCurrentlyTrackingHeader = false;
	EnableToolTips(true);			

	m_bFixedOverride = false;
	m_bFixedOverrideChange = false;

// start of tooltip stuff
/*
	LVM_SETINFOTIP 

	EnableToolTips(true);			
	HWND hwnd;
	GetDlgItem(IDC_LISTCTRL, &hwnd);

	CWnd* cwnd = GetDlgItem(IDC_LISTCTRL);

	m_cTooltip.Create(cwnd , TTS_ALWAYSTIP);
	m_cTooltip.Activate(TRUE);

	::SendMessage(hwnd, LVM_SETTOOLTIPS, -1, 0);

*/
	m_HeaderCtrl.SubclassDlgItem(HEADERID, (CWnd*)&GetListCtrl());



	m_bInit = true;
	m_bDisabled = false;

	CPageView::OnInitialUpdate();
}

BOOL SVBase::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style &= ~WS_VSCROLL;
	return CPageView::PreCreateWindow(cs);
}

void SVBase::OnContextMenu(CWnd* pWnd, CPoint point) 
{

	m_nItemSelected = -1;
	m_nSubItemSelected = -1;

	CPoint cpClient = point;
	ScreenToClient(&cpClient);

	CString cs;

	bool bSelected = (GetCountSelected() > 0);

// find cell where clicked
	LVHITTESTINFO  lvhit;
	lvhit.pt = cpClient;
	int nItem = GetListCtrl().SubItemHitTest(&lvhit);
	

// if nItem < 0
//		not on a row, put up menu anyway
// if nItem >= 0
//		on a row, select item and put up menu


	if (nItem >= 0)
	{
		SetSelectedCell(&lvhit);
		CRect cr;
		if (GetListCtrl().GetSubItemRect(lvhit.iItem, lvhit.iSubItem, LVIR_LABEL, cr))
		{	// move y coordinate to just below the cell so the 'selected cell' is visible
			CPoint cp = cr.BottomRight();
			ClientToScreen(&cp);
			point.y = cp.y;
				
		}
	}



	CMenu menu;
	menu.LoadMenu(IDR_V_LISTVIEW_CONTEXT);
	CMenu* pMenu = menu.GetSubMenu(0);

//ID_DELETE_ROWS_NOTSELECTED
//ID_DELETE_ROWS_SELECTED
	if (pMenu)
	{
		if (!m_pActiveSheet->bUpdatable)
			pMenu->DeleteMenu(ID_COLUMN_SELECTOR, MF_BYCOMMAND);


//		if (!bSelected)		// if no selected cells disable some menu commands
//			pMenu->EnableMenuItem(ID_LVCM_CREATE_SELECTED, MF_BYCOMMAND | MF_GRAYED);

		if (GetHiddenRowCount() == 0)
			pMenu->EnableMenuItem(ID_RECOVER_DELETED, MF_BYCOMMAND | MF_GRAYED);
		if (GetCountSelected() == 0)
			pMenu->EnableMenuItem(ID_DELETE_SELECTED_ROWS, MF_BYCOMMAND | MF_GRAYED);
		if (GetCountSelected() == GetListCtrl().GetItemCount())
			pMenu->EnableMenuItem(ID_DELETE_NONSELECTED_ROWS, MF_BYCOMMAND | MF_GRAYED);
/*
		if (nItem >= 0)
		{
			CString csWork;
			CString csItemText = GetListCtrl().GetItemText(m_nItemSelected, m_nSubItemSelected);

			if (!csItemText.IsEmpty())
			{
				if (pMenu->GetMenuString(ID_DELETE_ROWS_MATCH, cs, MF_BYCOMMAND) > 0)
				{
						csWork.Format(cs, csItemText); 
						pMenu->ModifyMenu(ID_DELETE_ROWS_MATCH, MF_BYCOMMAND, ID_DELETE_ROWS_MATCH, csWork);
				}
				else
					pMenu->DeleteMenu(ID_DELETE_ROWS_MATCH, MF_BYCOMMAND);

				if (pMenu->GetMenuString(ID_DELETE_ROWS_NOMATCH, cs, MF_BYCOMMAND) > 0)
				{
						csWork.Format(cs, csItemText); 
						pMenu->ModifyMenu(ID_DELETE_ROWS_NOMATCH, MF_BYCOMMAND, ID_DELETE_ROWS_NOMATCH, csWork);
				}
				else
					pMenu->DeleteMenu(ID_DELETE_ROWS_NOMATCH, MF_BYCOMMAND);

			}
			else
			{
				pMenu->DeleteMenu(ID_DELETE_ROWS_MATCH, MF_BYCOMMAND);
				pMenu->DeleteMenu(ID_DELETE_ROWS_NOMATCH, MF_BYCOMMAND);
			}
		}
		else
		{
			pMenu->DeleteMenu(ID_DELETE_ROWS_MATCH, MF_BYCOMMAND);
			pMenu->DeleteMenu(ID_DELETE_ROWS_NOMATCH, MF_BYCOMMAND);
		}

*/
 		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, 
			point.x, point.y, this,0);
		menu.DestroyMenu();
	}


	
}

void SVBase::OnDestroy() 
{
	DeleteAllRows();
	if (m_pActiveSheet)
	{
	
		m_pActiveSheet->pView = 0;				

	}
	CPageView::OnDestroy();
}

void SVBase::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{

	if (m_bDisabled)
		return;

	if (nIDCtl == IDC_BUTTON_TOTALLINE)
	{
		CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		CRect cr;
		m_btTotal,GetClientRect(cr);

		CString cs;
		if (GetHiddenRowCount() >0)
			cs.Format(RESCHAR(IDS_V_TOTALLINEHIDDEN),
					GetTotalsType(),
					GetListCtrl().GetItemCount(),
					GetCountSelected(),
					GetHiddenRowCount());
		else
			cs.Format(RESCHAR(IDS_V_TOTALLINE),
					GetTotalsType(),
					GetListCtrl().GetItemCount(),
					GetCountSelected());

		pDC->DrawText(cs, -1, cr, DT_VCENTER );

		
	}
	else if (nIDCtl == IDC_LISTCTRL)
{
// OnDrawItem is called once per each row that requires repainting


	if (m_bOrderDirty)		// this is set when the user drags the column header
	{						// make sure I keep the sheet in synch
		GetListCtrl().GetColumnOrderArray(m_pActiveSheet->nColumnOrder);	// save new column layout in the sheet
		m_bOrderDirty = false;
	}
// left and right margin in cell

	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_ORDER | LVCF_SUBITEM;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	COLORREF clrOld = pDC->GetTextColor();

	UINT uiFlags = ILD_TRANSPARENT;
	int nItem = lpDrawItemStruct->itemID;
	BOOL bFocus = (GetFocus() == this);

	LV_ITEM lvi;
	lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.stateMask = 0xFFFF;		
	GetListCtrl().GetItem(&lvi);	

	UINT nObjectNumber = lvi.lParam;

	CRect crWindow;
	GetListCtrl().GetClientRect(crWindow);
	crWindow.bottom += GetSystemMetrics(SM_CYHSCROLL);
	crWindow.right += GetSystemMetrics(SM_CXVSCROLL);

	CRect crCell, crTextCell, crGraphCell;

	for (int nColumn = 0; GetListCtrl().GetColumn(nColumn, &lvc); nColumn++, lvi.iSubItem++)
	{
		ASSERT (nColumn == lvi.iSubItem);
		ASSERT(nColumn < m_pActiveSheet->caColumns.GetSize());
		if (nColumn >= m_pActiveSheet->caColumns.GetSize())		// if I don't have a column entry for this column, just bail
			return;

		BOOL b = GetListCtrl().GetSubItemRect(lvi.iItem, lvi.iSubItem, LVIR_LABEL, crCell);
		ASSERT (b);


		// if crCell is completely outside of the window, don't bother drawing
		if ( (!crWindow.PtInRect(crCell.TopLeft())) && (!crWindow.PtInRect(crCell.BottomRight())) )
			continue;

		// if cell is hidden, don't bother drawing
		if (crCell.Width() == 0)
			continue;

		SVColumn *pCol;		
		pCol = &m_pActiveSheet->caColumns[nColumn];
		if ( (pCol->bGraphic) && (pCol->bMinMaxDirty))		// if minimum and maximums have not been set
			SetMinMaxValues(nColumn);						// then set them

		if (pCol->cShowStatus != SHOW)
			continue;

		SetCellDimensions(pCol, crCell, crTextCell, crGraphCell);	// take our available cell and divide into text and graphic components
		bool bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED);


		DrawCell(pDC, crCell, crTextCell, crGraphCell, // rectangles where drawing will take place, set in SetCellDimensions
			pCol,			// pointer to SVColumn structure
			nObjectNumber,	// object number in the collection
			nColumn,		// logical column number (order that column was added, same order that SVColumn table is)
			nItem,			// listctrl item number (used to determine background color, same as Object number unless it was resorted)
			bSelected);		// whether it is selected or not (used for color selection)
	}	
}
}


void SVBase::DrawCell(CDC* pDC, CRect crCell, CRect crTextCell, CRect crGraphCell, SVColumn* pCol, UINT nObjectNumber, int nColumn, int nItem, bool bSelected)
{
	bool bExtender = true;    // use extenders
	int nRule;
	CString csValue;
	switch (pCol->ct)
	{
	//////////////////////////////////////////////////////////////////////////
	// Numbers
	//////////////////////////////////////////////////////////////////////////
		case ctInt:
		case ctInt64:
		case ctFloat:
		{

			double d;
			if (pCol->ct == ctInt)
				d = (double)GetValueInt(nObjectNumber, nColumn);
			else if (pCol->ct == ctInt64)
				d = (double)GetValueInt64(nObjectNumber, nColumn);
			else
				d = GetValueDouble(nObjectNumber, nColumn);
			nRule = TestColumnRule(d, nObjectNumber, nColumn);
			SetColors(pDC, crCell, nRule, nColumn, bSelected, pCol, nItem);
	///////////////////////////////////////////////////////////////////////////
	// Numeric Graphical Components
	//////////////////////////////////////////////////////////////////////////
			if (pCol->bGraphic)
			{
				///////////////////////////////////////////////////////////////////////////
				// Numeric Subcolumns
				//////////////////////////////////////////////////////////////////////////
				if (pCol->sc != scNone)
				{
					COLORREF clrTable[MAXSUBCOLUMNS] = {RGB(0, 255,0), RGB(255, 255,0), RGB(255, 0, 0), 
											RGB(0, 0,255), RGB(255, 0, 255), RGB(0, 255,255)};
					if (pCol->sc == scSingleBar)
					{
						crGraphCell.DeflateRect(0, CELLVGRAPHMARGIN);
						int nNumSubColumns = pCol->GetNumberSubColumns();
						int nTable[MAXSUBCOLUMNS];
						int nPixelTable[MAXSUBCOLUMNS];
						if (!GetGroupCountsInt(nObjectNumber, nColumn, nNumSubColumns, nTable))
							return;		// unable to get values, levae column blank and return immediately

						GetMultiPixelTable(nNumSubColumns, nTable, nPixelTable, crGraphCell.Width());
						crGraphCell.DeflateRect(0,2);
 						for (int i=0; i<nNumSubColumns; i++)
						{
							crGraphCell.right = crGraphCell.left + nPixelTable[i];
							pDC->FillRect(crGraphCell, &CBrush(clrTable[i]));
							crGraphCell.left = crGraphCell.right;
						}

					}
					else if (pCol->sc == scMultiBars)
					{
						int nTable[MAXSUBCOLUMNS];
						int nPixelTable[MAXSUBCOLUMNS];
						int nNumSubColumns = pCol->GetNumberSubColumns();
						if (!GetGroupCountsInt(nObjectNumber, nColumn, nNumSubColumns, nTable))
							return;		// unable to get values, levae column blank and return immediately
						int nWidthEachTable = ((crGraphCell.Width() - ((nNumSubColumns - 1) * CELLHGRAPHMARGIN)) / nNumSubColumns);
						GetMultiPixelTable(nNumSubColumns, nTable, nPixelTable, nWidthEachTable);
						crGraphCell.DeflateRect(0, CELLVGRAPHMARGIN);
						// each crGraphCellFull would contain all the subcell values
						crGraphCell.DeflateRect(0,2);
						CRect crGraphCellFull = crGraphCell;
 						for (int i=0; i<nNumSubColumns; i++)
						{

							crGraphCellFull.right = crGraphCellFull.left + nWidthEachTable;
							crGraphCell = crGraphCellFull;
							crGraphCell.right = crGraphCell.left + nPixelTable[i];
							pDC->FillRect(crGraphCellFull, &CBrush(::GetSysColor(COLOR_WINDOW))); // fill with background in case it is the selected row
							pDC->FillRect(crGraphCell, &CBrush(clrTable[i]));
							pDC->FrameRect(crGraphCellFull, &CBrush(COLORFRAME));
							crGraphCellFull.left = crGraphCellFull.right + CELLHGRAPHMARGIN;
						}
	
					}

					else if (pCol->sc == scMultiValues)
					{
						int nTable[MAXSUBCOLUMNS];
						int nNumSubColumns = pCol->GetNumberSubColumns();
						if (!GetGroupCountsInt(nObjectNumber, nColumn, nNumSubColumns, nTable))
							return;	
					
						int nSubCellWidth = crGraphCell.Width() / nNumSubColumns;

						CRect crIcon;
			
						crGraphCell.right = crGraphCell.left;

 						for (int i=0; i<nNumSubColumns; i++)
						{
							crGraphCell.right += nSubCellWidth;
							if (nTable[i] > 0)
							{
								crIcon = crGraphCell;
								crIcon.right = crIcon.left + crIcon.Height();
								crIcon.DeflateRect(2,2);
								DrawCircle(pDC, crIcon,m_clrTextColor, clrTable[i]);
								crGraphCell.left = crIcon.right + 2;
								csValue.Format(_T("%d"), nTable[i]);
								pDC->DrawText(csValue, -1, crGraphCell,
									DT_LEFT | 
									DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER); 
							}
							crGraphCell.left = crGraphCell.right + 1;
						}
	
					}
					


				}	// end of subcolumns
				else
				{
				// no subcolumns
				
				//////////////////////////////////////////////////////////////////////////
				// Numeric Bar Graph (in frame)
				//////////////////////////////////////////////////////////////////////////
					if (pCol->gdt == gdtBarInFrame)
					{
						int x = GetXPixel(pCol, nObjectNumber, nColumn, crGraphCell.Width());
						crGraphCell.DeflateRect(0,2);
						CRect crGraphCellFull = crGraphCell;
						if (x < 2)			// minimum pixel width is 2
							x = 2;
						crGraphCell.right = crGraphCell.left + (int)x;
						pDC->FillRect(crGraphCellFull, &CBrush(::GetSysColor(COLOR_WINDOW))); // fill with background in case it is the selected row
						pDC->FillRect(crGraphCell, &CBrush(m_clrGraphicColor));
						pDC->FrameRect(crGraphCellFull, &CBrush(COLORFRAME));
						
						/* not implememted
						if (pCol->nSpike > 0)
						{
							//int x = (int)ConvertXPixel(pCol->nSpike, pCol->dMinValue, pCol->dMaxValue, crGraphCellFull.Width());
							int x = (int)ConvertXPixel(pCol->nSpike, 0, 100, crGraphCellFull.Width());
							x = 426;
							CPen pen(PS_SOLID, 0, RGB(0,0,255)), *pOldPen;
							pOldPen = pDC->SelectObject(&pen);
							pDC->MoveTo(x, crGraphCellFull.top);
							pDC->LineTo(x, crGraphCellFull.bottom);
							pDC->SelectObject(pOldPen);
						}
						*/
					}
					//////////////////////////////////////////////////////////////////////////
					// Numeric Bar Graph (not in a frame)
					//////////////////////////////////////////////////////////////////////////

					else if (pCol->gdt == gdtBar)
					{
						if (false && bExtender) // draw gray extender line (temporary disable)
						{
							CPen *pOldPen, pen(PS_SOLID, 2, COLOREXTENDER);
							pOldPen = pDC->SelectObject(&pen);
							int nMidY = ((crGraphCell.bottom-crGraphCell.top)/2)+crGraphCell.top ;
							pDC->MoveTo(crGraphCell.left, nMidY);
							pDC->LineTo(crGraphCell.right-1, nMidY);
							pDC->SelectObject(pOldPen);
						}
						int x = GetXPixel(pCol, nObjectNumber, nColumn, crGraphCell.Width());
						if (x < 1)
						x = 1;
						crGraphCell.right = crGraphCell.left + (int)x;
						crGraphCell.DeflateRect(0,2);
						pDC->FillRect(crGraphCell, &CBrush(m_clrGraphicColor));

					}

					//////////////////////////////////////////////////////////////////////////
					// Numeric Scatter Points
					//////////////////////////////////////////////////////////////////////////
					else if (pCol->gdt == gdtScatterPlot)
					{
						if (bExtender) // draw gray extender line
						{
							CPen *pOldPen, pen(PS_SOLID, 2, COLOREXTENDER);
							pOldPen = pDC->SelectObject(&pen);
							int nMidY = ((crGraphCell.bottom-crGraphCell.top)/2)+crGraphCell.top ;
							pDC->MoveTo(crGraphCell.left, nMidY);
							pDC->LineTo(crGraphCell.right-1, nMidY);
							pDC->SelectObject(pOldPen);
						}
						crGraphCell.DeflateRect(0,2);
						int nBarWidth = crGraphCell.Height()/2;
						int x = GetXPixel(pCol, nObjectNumber, nColumn, crGraphCell.Width());
						crGraphCell.right = crGraphCell.left + x + 1;
						crGraphCell.left = crGraphCell.right - nBarWidth;
						if (crGraphCell.left < crCell.left)
						{
							crGraphCell.left = crCell.left;
							crGraphCell.right = crGraphCell.left + nBarWidth;
						}
						if (crGraphCell.right > crCell.right)
						{
							crGraphCell.right = crCell.right;
							crGraphCell.left = crCell.right - nBarWidth;
							if (crGraphCell.left < crCell.left)
								crGraphCell.left = crCell.left;
						}
						pDC->FillRect(crGraphCell, &CBrush(m_clrGraphicColor));

					}
					else if (pCol->gdt != gdtNone)
					{
						ASSERT(false);				// nothing else valid for numbers
						return;	
					}
				}
			}
			if (pCol->bText)
			{
				// add a little margin
				crCell.left += CELLHGRAPHMARGIN;
				crCell.right -= CELLHGRAPHMARGIN;
				csValue = GetFormattedNumber(pCol, nObjectNumber, nColumn);	
				pDC->DrawText(csValue, -1, crTextCell,
					pCol->nJustify | 
					DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | 
					DT_MODIFYSTRING);// |DT_END_ELLIPSIS );
			}

		}
		break;

//////////////////////////////////////////////////////////////////////////
// Date
//////////////////////////////////////////////////////////////////////////
		case ctDateTime:
		{
			COleDateTime cdt = GetValueDateTime(nObjectNumber, nColumn);
#define JAN011970 25569.0
		//testdate	
//			if (nColumn == 0)
//				cdt = GetRandomDateTime();					

			nRule = TestColumnRule(cdt, nObjectNumber, nColumn);	
			SetColors(pDC, crCell, nRule, nColumn, bSelected, pCol, nItem);
			if ( (cdt.GetStatus() != COleDateTime::valid) || (cdt.m_dt == JAN011970) )
				return;

			if (pCol->bText) 			// do we need to write text?
			{
				CString cs = cdt.Format(); //a
				pDC->DrawText(cs,
					-1,
					crTextCell,
					pCol->nJustify | 
					DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);

			}
//////////////////////////////////////////////////////////////////////////
// Date Circle If True
//////////////////////////////////////////////////////////////////////////
			if (pCol->bGraphic)
			{
				if (pCol->gdt == gdtCircleWhenTrue)
				{
					if (nRule != 0)			// rule true?
					{
						crGraphCell.right = crGraphCell.left + crGraphCell.Height();
						DrawCircle(pDC, crGraphCell, m_clrTextColor, pCol->clrRule[COLORRULE0]);
					}
				}

//////////////////////////////////////////////////////////////////////////
// Date Circle If False
//////////////////////////////////////////////////////////////////////////
				else if (pCol->gdt == gdtCircleWhenFalse)
				{
					if (nRule == 0)			// rule false?
					{
						crGraphCell.right = crGraphCell.left + crGraphCell.Height();
						DrawCircle(pDC, crGraphCell, m_clrTextColor, pCol->clrRule[COLORRULE0]);
					}
				}

//////////////////////////////////////////////////////////////////////////
// Date Scatter Points
//////////////////////////////////////////////////////////////////////////
				else if (pCol->gdt == gdtScatterPlot)
				{	
					if (bExtender) // draw gray extender line
					{
						CPen *pOldPen, pen(PS_SOLID, 2, COLOREXTENDER);
						pOldPen = pDC->SelectObject(&pen);
						int nMidY = ((crGraphCell.bottom-crGraphCell.top)/2)+crGraphCell.top ;
						pDC->MoveTo(crGraphCell.left, nMidY);
						pDC->LineTo(crGraphCell.right-1, nMidY);
						pDC->SelectObject(pOldPen);
					}
					crGraphCell.DeflateRect(0,2);
					int nBarWidth = crGraphCell.Height();
					int x = GetXPixel(pCol, cdt, crGraphCell.Width());
					crGraphCell.left = crGraphCell.left + x;
					crGraphCell.right = crGraphCell.left + nBarWidth;

					if (crGraphCell.left < crCell.left)
					{
						crGraphCell.left = crCell.left;
						crGraphCell.right = crGraphCell.left + nBarWidth;
					}
					if (crGraphCell.right > crCell.right)
					{
						crGraphCell.right = crCell.right;
						crGraphCell.left = crCell.right - nBarWidth;
						if (crGraphCell.left < crCell.left)
							crGraphCell.left = crCell.left;
					}

					pDC->FillRect(crGraphCell, &CBrush(m_clrGraphicColor));

				}
			}
		}
		break;
///////////////////////////////////////////////////////////////////////////////
// boolean
//////////////////////////////////////////////////////////////////////////////
		case ctBoolean:
		{
			ASSERT (pCol->rt == rtNone);	// no rules supported with Boolean's
			SetColors(pDC, crCell, 0, nColumn, bSelected, pCol, nItem);
/*
			if (pCol->nJustify == DT_LEFT)
			{
				crCell.right = crCell.left + crCell.Height();
			}
			else if (pCol->nJustify == DT_RIGHT)
			{
				crCell.left = crCell.right - crCell.Height();
			}
			else //(DT_CENTER)
			{
				crCell.left = crCell.left + (crCell.Width()/2) - (crCell.Height()/2);
				crCell.right = crCell.left + crCell.Height();
			}
			*/
			crGraphCell.DeflateRect(CELLVGRAPHMARGIN, CELLVGRAPHMARGIN);
//			crGraphCell.left = crGraphCell.right - crGraphCell.Height();
			crGraphCell.left = crGraphCell.left + (crGraphCell.Width()/2) - (crGraphCell.Height()/2);
			crGraphCell.right = crGraphCell.left + crGraphCell.Height();


			//int nImage = 1; //pCol->nImageTrue;;
			bool b = GetValueBool(nObjectNumber, nColumn);
			if (b) 
				DrawCircle(pDC, crGraphCell, m_clrTextColor, m_clrGraphicColor);
			else
			{
				if (pCol->gdt == gdtEmptyCircle)
					DrawCircle(pDC, crGraphCell, m_clrTextColor, m_clrBackgroundColor);
			}

			if (pCol->bText)
			{
				// add a little margin
				if (b)
					csValue = RESCHAR(IDS_V_TRUE);
				else
					csValue = RESCHAR(IDS_V_FALSE);
				crTextCell.left += CELLHGRAPHMARGIN;
				crTextCell.right -= CELLHGRAPHMARGIN;
				pDC->DrawText(csValue, -1, crTextCell,
				pCol->nJustify | 
				DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);  

			}	

//			 m_pImageList->DrawIndirect(pDC, nImage, crCell.TopLeft(), 
//				CSize(crCell.Width(), crCell.Height()), CPoint(0, 0));

		}
		break;
//////////////////////////////////////////////////////////////////////////
// ctChar
//////////////////////////////////////////////////////////////////////////
		case ctChar:

		{
			CString  csValue = GetValueString(nObjectNumber, nColumn);

			nRule = TestColumnRule(csValue, pCol);
			SetColors(pDC, crCell, nRule, nColumn, bSelected, pCol, nItem);
			if (pCol->ct == ctUnknown) 
				csValue = RESCHAR(IDS_V_UNKNOWN);
			
			// add a little margin
			crCell.left += CELLHGRAPHMARGIN;
			crCell.right -= CELLHGRAPHMARGIN;
			pDC->DrawText(csValue, -1, crCell,
				pCol->nJustify | 
				DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);  
				//DT_MODIFYSTRING |DT_END_ELLIPSIS );
		}
		break;
		default:
			ASSERT (false);		// need to account for new ct
	}	// end of column switch

}		// end of drawitem(IDC_LISTCTRL)


 
void SVBase::OnSize(UINT nType, int cx, int cy) 
{
	if (m_lvMain)
	{

		PositionHeader();
		CRect cr;
		cr.top = 0;
		cr.left = 0;
		cr.right = cx;
		cr.bottom = cy - (1 * GetSystemMetrics(SM_CYHSCROLL));
		m_crTotalLine = cr;
		m_crTotalLine.top = cr.bottom+1;
		m_crTotalLine.bottom = cy;
		m_btTotal.MoveWindow(m_crTotalLine, true);
		m_lvMain.MoveWindow(cr, true);
	}
	
	CPageView::OnSize(nType, cx, cy);
	

	
}



void SVBase::PositionHeader()
{
	return;
//fix Q137520
	CListCtrl* pLC = &m_lvMain;
	CHeaderCtrl* pHC = pLC->GetHeaderCtrl();
	
//   HWND  hwndHeader = GetWindow(hwndListView, GW_CHILD);
	DWORD dwStyle = ::GetWindowLong(pLC->m_hWnd, GWL_STYLE);

   /*
      To ensure that the first item will be visible, create the control
      without the LVS_NOSCROLL style and then add it here.
   */ 
   dwStyle |= LVS_NOSCROLL;
   ::SetWindowLong(pLC->m_hWnd, GWL_STYLE, dwStyle);

   /*
      Only do this if the ListView is in report view and you were able to
      get the header hWnd.
   */ 
   if(((dwStyle & LVS_TYPEMASK) == LVS_REPORT) && pHC)
      {
      RECT        rc;
      HD_LAYOUT   hdLayout;
      WINDOWPOS   wpos;

      pLC->GetClientRect(&rc);
      hdLayout.prc = &rc;
      hdLayout.pwpos = &wpos;

      Header_Layout(pHC->m_hWnd, &hdLayout);

      ::SetWindowPos(  pHC->m_hWnd,
                     wpos.hwndInsertAfter,
                     wpos.x,
                     wpos.y,
                     wpos.cx,
                     wpos.cy,
                     wpos.flags | SWP_SHOWWINDOW);

      //ListView_EnsureVisible(hwndListView, 0, FALSE);
	  pLC->EnsureVisible(0, FALSE);
   } 
} 
bool SVBase::AddColumns(SVSheet* p)
{

	int n = p->caColumns.GetSize();

	for (int i=0; i<p->caColumns.GetSize(); i++)
		//AddColumn(&(p->caColumns.GetAt(i)));
		AddColumn(&(p->caColumns[i]));

	if (p->csName.IsEmpty())
	{
		CRuntimeClass* pc = GetRuntimeClass();
		p->csName = pc->m_lpszClassName;
		return true;
	}
// resuing a sheet
	CRuntimeClass* pc = GetRuntimeClass();
	if (p->csName != pc->m_lpszClassName)
	{
		CString cs;
		CString cs1 = pc->m_lpszClassName;
		cs.Format(_T("Sheet Created with %s, now being used with %s"),
			p->csName,
			cs1);
		AfxMessageBox(cs);
		ASSERT (false); // look above
		return false;
	}

	return true;
}

int SVBase::AddColumn(SVColumn* pCol)
{

	if ( (pCol->gdt != gdtNone) || (pCol->sc != scNone) )
	{
		pCol->bGraphic = true;
		pCol->bGraphable = true;		// temp, should be set on a column by column basis
	}

	// check column constraints
	if (pCol->sc != scNone)
	{	
		//CString csClass = GetRuntimeClass()->m_lpszClassName;
		// multi subcolumns are limited to a range of 2 through MAXSUBCOLUMNS)
		ASSERT ( (pCol->nSubColumns >= 2) && (pCol->nSubColumns <= MAXSUBCOLUMNS) );
		// multi subcolumns must be of type ctInt, if expanding this, don't forget ::CompareFunc
		ASSERT (pCol->ct == ctInt);			// subcolumns only supports int's
		ASSERT (pCol->bText == false);		// can't also have text
	}
	else
	{
		ASSERT (pCol->nSubColumns == 1);
	}

	int nNumColumns = GetListCtrl().GetHeaderCtrl()->GetItemCount();
	ASSERT (nNumColumns < MAXNUMCOLUMNS);

	pCol->bMinMaxDirty = true;


	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvc.fmt = LVCFMT_BITMAP_ON_RIGHT;
	lvc.cx = pCol->nPixelWidth;
	lvc.pszText = (LPTSTR)(LPCTSTR)(pCol->csHeaderText);
	lvc.cchTextMax = pCol->csHeaderText.GetLength() * sizeof( _TCHAR);
	int nCol = GetListCtrl().InsertColumn(nNumColumns, &lvc);
//	int nCol = m_pActiveSheet->caColumns.Add(*pCol);
	HDITEM    curItem;
	curItem.mask = 0;

	CHeaderCtrl* pHdr = GetListCtrl().GetHeaderCtrl();
	//trace pHdr != 0	
	pHdr->GetItem(nCol, &curItem);
	curItem.pszText = (LPTSTR)(LPCTSTR)(pCol->csHeaderText);
	curItem.cchTextMax = pCol->csHeaderText.GetLength() * sizeof( _TCHAR);

	curItem.mask= HDI_FORMAT | HDI_TEXT;
	curItem.fmt= HDF_BITMAP_ON_RIGHT | HDF_STRING | pCol->nHeaderJustify;
	pHdr->SetItem(nCol, &curItem);
	SetHdrImage(nCol);

	return nCol;
}

void SVBase::SetHdrImage(int nColumn)
{
	if (nColumn == m_nLastSort)			// is this the current sort column?
	{
		int nImageNum = ::bSortDirection ? IMAGEUPARROW : IMAGEDOWNARROW;
		if (m_pActiveSheet->caColumns[nColumn].IsRule())		// if a rule defined
			nImageNum += IMAGERULEOFFSETS;			// offset to the 'rules' icons
		SetHdrImage(nColumn, nImageNum);
		
	}
	else
	{
		if (m_pActiveSheet->caColumns[nColumn].IsRule())		
			SetHdrImage(nColumn, IMAGERULEONLY);
		else
			SetHdrImage(nColumn, IMAGEOFF);
	}

}


void SVBase::SetHdrImage(int nColumn, int nImageNumber)
{
#define MAXCOLUMNWIDTH 256
	HDITEM    curItem;
	CHeaderCtrl* pHdr = GetListCtrl().GetHeaderCtrl();	
	
	curItem.mask = HDI_IMAGE | HDI_FORMAT | HDI_TEXT;
	_TCHAR szColumnText[MAXCOLUMNWIDTH];
	curItem.pszText = szColumnText;
	curItem.cchTextMax  = MAXCOLUMNWIDTH;

	SVColumn* pCol = &m_pActiveSheet->caColumns[nColumn];
	CString cs = pCol->csHeaderText; 

	pHdr->GetItem(nColumn, &curItem);

	if (nImageNumber == IMAGEOFF)
	{
		curItem.iImage = 0;
		curItem.fmt &= ~HDF_IMAGE;			// remove image
	}
	else
	{
		curItem.iImage = nImageNumber;
		curItem.fmt |=  HDF_IMAGE ;
	}
	
	pHdr->SetItem(nColumn, &curItem);
}



int SVBase::AddRow(UINT nObjectNumber)
{
	LV_ITEM lvi;
    lvi.mask = /*LVIF_TEXT |*/ LVIF_PARAM | LVIF_INDENT; 
	lvi.iIndent = 0;
    lvi.iSubItem = 0; 
	lvi.iImage = 0; 
    lvi.pszText = 0;//LPSTR_TEXTCALLBACK; 
	lvi.iItem = GetListCtrl().GetItemCount()+1;  
	lvi.lParam = (LPARAM) nObjectNumber;
	int nItem = GetListCtrl().InsertItem (&lvi);
	if (nItem < 0)
	{
		CString cs;
		cs.Format(_T("Max is %d"), GetListCtrl().GetItemCount());
		AfxMessageBox(cs);
	}
	return nItem;
}

int SVBase::AddRowsN(IEnumeration* p)
{
//	for (int i=0; i<p-get_Count(); i++)
//		AddRowN(i):
	return 0;
}

int SVBase::TestColumnRule(double dValue, UINT nObjectNumber, int nColumn)
{
	SVColumn* pCol = &m_pActiveSheet->caColumns[nColumn];
	if (!pCol->IsRule())
		return 0; 

	double dRetCode;
	if (pCol->rt & (rtHighColumn | rtEqualColumn | rtLowColumn)) // value is in another column?
	{ // only supports int's for 
		dRetCode = dValue - GetValueInt(nObjectNumber, pCol->nValueFromColumn);
	}
	else
		dRetCode = dValue - pCol->dRuleValue;

	if (dRetCode < 0)
	{
		if ( pCol->rt & ( rtLow | rtLowColumn) )
			return 1;
		else
			return 0;
	}
	if (dRetCode > 0)
	{
		if ( pCol->rt & ( rtHigh | rtHighColumn) )
			return 1;
		else
			return 0;
	}
	if ( pCol->rt & ( rtEquals | rtEqualColumn) ) 
		return 1;

	return 0;



}

int SVBase::TestColumnRule(COleDateTime cdtvalue, UINT nObjectNumber, int nColumn)
{
	SVColumn* pCol = &m_pActiveSheet->caColumns[nColumn];

	if (!pCol->IsRule())
		return 0;

	if (cdtvalue.GetStatus() != COleDateTime::valid)
		return 0;

	if ( (pCol->rt & rtCurrentMonth) || (pCol->rt & rtCurrentDay) )
	{
		COleDateTime cdtCurrent = COleDateTime::GetCurrentTime();
		int nDay = (pCol->rt & rtCurrentMonth) ? 1 : cdtCurrent.GetDay();
		COleDateTime cdtTest(	cdtCurrent.GetYear(),
								cdtCurrent.GetMonth(),
								nDay, 0, 0, 0);	// day, hour, minute, second
		
		if (cdtvalue >= cdtTest) 
			return 1;
		else
			return 0;
	}


	else if (pCol->rt & rtWithinHours) // if within pCol->dRuleValue hours, return 1, else 0
	{
		COleDateTimeSpan dts(0, (int)pCol->dRuleValue, 0, 0);
		COleDateTime cdt = COleDateTime::GetCurrentTime() - dts;
		if (cdt <= cdtvalue)
			return 1;
		return 0;
	}
	else if (pCol->rt & rtWithinDays) // if within pCol->dRuleValue hours, return 1, else 0
	{
		COleDateTimeSpan dts((int)pCol->dRuleValue, 0, 0, 0);
		COleDateTime cdt = COleDateTime::GetCurrentTime() - dts;
		if (cdt <= cdtvalue)
			return 1;
		return 0;
	}
	else if (pCol->rt & rtWithinWeeks) // if within pCol->dRuleValue hours, return 1, else 0
	{
		COleDateTimeSpan dts((int)pCol->dRuleValue*7, 0, 0, 0);
		COleDateTime cdt = COleDateTime::GetCurrentTime() - dts;
		if (cdt <= cdtvalue)
			return 1;
		return 0;
	}


	else if (pCol->rt == rtLow)
		return (cdtvalue < pCol->cdtRuleValue) ? +1 : 0;
	else if (pCol->rt == rtHigh)
		return (cdtvalue > pCol->cdtRuleValue) ? +1 : 0;

	return (cdtvalue == pCol->cdtRuleValue) ? +1 : 0;

}

int SVBase::TestColumnRule(CString csValue, SVColumn* pCol)
{
	if (!pCol->IsRule())
		return 0;

	CString csColRule;
	csColRule = pCol->csRuleValue;
	if (pCol->bIgnoreCase)
	{
		csValue.MakeUpper();
		csColRule.MakeUpper();
	}

	if ( (pCol->rt & rtLike) || (pCol->rt & rtNotLike) )
	{
		bool bMatch = CompareMask(csColRule, (LPCTSTR)csValue);
		return (pCol->rt & rtLike) ? bMatch : !bMatch;
	}
	bool bMatch = (csColRule.Compare(csValue) == 0);
	return (pCol->rt & rtEquals) ? bMatch : !bMatch;
}



bool SVBase::CompareMask(const TCHAR* mp, const TCHAR* fp)
{
#define ONECHAR '%'
#define ANYCHAR '*'
#define NUMCHAR '#'
#define MATCH true
#define NOMATCH false

TCHAR cOneChar = _T(ONECHAR);
TCHAR cAnyChar = _T(ANYCHAR);
TCHAR cNumChar = _T(NUMCHAR);
TCHAR cHighNum = _T('9');
TCHAR cLowNum  = _T('0');

#define LENGTH 1 //sizeof( _TCHAR)

	mp-=LENGTH;                        // just cause I start out incrementing
	fp-=LENGTH;                        // (same)


	while (*(mp+=LENGTH) && *(fp+=LENGTH))
	{
		if( *mp == cOneChar)
		//if( (_tcscmp(mp, &szOneChar)) == 0)		// a match
			continue;
		if(*mp == *fp)				// a match
			continue;

		if (*fp <= cHighNum) 
			if (*fp >= cLowNum)
				if (*mp == cNumChar)
			continue;
// at this point, we have a nomatch		
//		if( (_tcscmp(mp, &szAnyChar)) != 0)		// if not the wildcard caharcter
		if( *mp != cAnyChar)
			return NOMATCH;

		while (*mp == cAnyChar)		// ignore adjacent AnyChar masks
			mp+=LENGTH;


		if (!*(mp))					// if the AnyChar is the last char of mask, it is a match
			return MATCH;
		if (*mp == cOneChar)	        // if the OneChar followed the AnyChar, it is a char match
			continue;
		if ( (*fp <= cHighNum) && (*fp >= cLowNum) && (*mp == cNumChar) )
			continue;

		while (*mp != *fp)			// resynch the mask to the field
		{
			if (!*(fp+=sizeof( _TCHAR)))           // if I run out of field before resynching, a nomatch
				return NOMATCH;
		}
		// set an indicator here indicating we resynched
		// because it may not be the right synch point
		// exmaple
		// mp = *t
		// fp = TAT
		// we just matched on the first T
		// we need to indicate that if a future mismatch occurs,
		// we need to resynch
	}
	
	// either mask or field ran out
	if (!*mp)		// mask ran out
	{
		fp+=LENGTH;
		if (!*fp)						// if field also out ..
			return MATCH;				// .. then a match
		else								// otherwise ..
			return NOMATCH;			// ..a no match
	}
// mask still has stuff in it and the field ran out
// if the stuff left in the mask is the AnyChar, it is a match
// else it is a nomatch
	while (*mp == cAnyChar)     // ignore adjacent AnyChar masks
	{
		mp+=LENGTH;
		if (!*(mp))                // if the AnyChar is the last char of mask, it is a match
			return MATCH;
		else
			return NOMATCH;					// else field ran out and there is still mask
	}
	return NOMATCH;
}

COLORREF SVBase::GetDefaultBackgroundColor(int nDisplayRowNumber)
{
	if ((nDisplayRowNumber/3) % 2)
		return RGB(240,240,240);
	else
		return ::GetSysColor(COLOR_WINDOW);
}
void SVBase::SetMinMaxValues(int nColumn)
{
	SVColumn*  pCol = &m_pActiveSheet->caColumns[nColumn];
//	svColumn.dMinValue = 0;
//	svColumn.dMaxValue = 99;
//	m_pActiveSheet->caColumns.SetAt(nColumn, svColumn);
//	return;


	if (pCol->bMinMaxSet)		// this means the report already set it, better not mess with it
		return;						// next release, I will have to do totals though

	switch (pCol->ct)
	{
		case ctInt:
			pCol->nMinValue = GetLowestValueInt(nColumn);
			pCol->nMaxValue = GetHighestValueInt(nColumn);
			ASSERT (pCol->nMinValue <= pCol->nMaxValue);
			//pCol->nTotal = 
			break;
		case ctInt64:
			pCol->n64MinValue = GetLowestValueInt64(nColumn);
			pCol->n64MaxValue = GetHighestValueInt64(nColumn);
			ASSERT (pCol->n64MinValue <= pCol->n64MaxValue);
			//pCol->n64Total = 
			break;
		case ctFloat:
			pCol->dMinValue = GetLowestValueDouble(nColumn);
			pCol->dMaxValue = GetHighestValueDouble(nColumn);
			ASSERT (pCol->dMinValue <= pCol->dMaxValue);
			//pCol->dTotal = 
			break;
		case ctDateTime:
			pCol->cdtMin = GetLowestValueDateTime(nColumn);
			pCol->cdtMax = GetHighestValueDateTime(nColumn);
//			ASSERT (pCol->cdtMin <= pCol->cdtMax);
			break;
	}
	pCol->bMinMaxDirty = false;	

}


double SVBase::GetDoubleValue(CString csValue)
{
	return _tcstod(csValue, 0);
}

void SVBase::SetCellDimensions(SVColumn* pCol, CRect crCella, CRect &crTextCell, CRect &crGraphCell)
{	

	// crCella is the entire cell available
	// crTextCell is set to the size of the text piece
	// crGraphCell is set to the size of the Graphic piece
	// divide the cell into the text piece and the graphic piece

//	if the control key is down, 
//		set text part
//
//
	CRect crCell = crCella;
	crCell.DeflateRect(CELLHGRAPHMARGIN,0);	
	if (pCol->sc != scNone)		// all the multiple subcolumns do the drawing in the crGraphCell part
	{
		crTextCell.SetRectEmpty();
		crGraphCell = crCell;
		return;
	}
	if (pCol->bText)
	{ // Text
		if (pCol->gdt != gdtNone)
		{
			//	Text / Graphic
			// m_bFixedOverride = true means control key is pressed
			int nMargin = 0;
			
			if (m_bFixedOverride)		// control key pressed?
			{
				nMargin = crCell.Width()/2;
			}
	
			else if (pCol->nPixelFixedWidth > 0)
				nMargin = pCol->nPixelFixedWidth;
			else
			{
				if (pCol->IsNumber() || pCol->ct == ctBoolean)
				{
					if (pCol->ut != utNone)
						nMargin = m_nNumMarginUnits;
					else if ( (pCol->ct == ctInt) && (pCol->nMaxValue < 1000) )
						nMargin = m_nNumMarginSmall;
					else
						nMargin = m_nNumMarginLarge;
				}
				else if (pCol->IsDate())
					nMargin = m_nDateTimeMargin;
			}
				
			pCol->nPixelFixedWidth = nMargin;


			if (!pCol->bFixedText)
			{ // the graphic part of the cell is fixed
				crGraphCell = crCell;
				crGraphCell.left =  crGraphCell.right - nMargin;
				if (crGraphCell.left < crCell.left)
					crGraphCell.left = crCell.left;
			
				crTextCell = crCell;
				crTextCell.right = crGraphCell.left - CELLHGRAPHMARGIN;
				if (crTextCell.left < crCell.left)
					crTextCell.left = crCell.left;
			}

			else	
			{ // the text part of the cell is fixed				
				crTextCell = crCell;
				crTextCell.right =  crTextCell.left + nMargin;
				if (crTextCell.right > crCell.right)
					crTextCell.right = crCell.right;
			
				crGraphCell = crCell;
				crGraphCell.left = crTextCell.right + CELLHGRAPHMARGIN;
				if (crGraphCell.right > crCell.right)
					crGraphCell.right = crCell.right;
			}
			if ( (crGraphCell.left < crTextCell.right) || (crGraphCell.left >= crGraphCell.right) )
				crGraphCell.SetRectEmpty();
			else
				crGraphCell.DeflateRect(0, CELLVGRAPHMARGIN);

			if (pCol->bFixedText)
				pCol->nPixelFixedWidth = nMargin;
			else
				pCol->nPixelFixedWidth = crCell.Width() - nMargin;
			}
		else
		{	//  Text  / No Graphic
			crTextCell = crCell;
			crGraphCell.SetRectEmpty();
		}
	}
	else
	{ // No Text
		if (pCol->gdt != gdtNone)
		{
			//  No Text /  Graphic
			crTextCell.SetRectEmpty();
			crGraphCell = crCell;
			crGraphCell.DeflateRect(0, CELLVGRAPHMARGIN);
		}
		else
		{	
			//  No Text / No Graphic
			crTextCell.SetRectEmpty();
			crGraphCell.SetRectEmpty();
		}
	}
}

void SVBase::DrawCircle(CDC* pDC, CRect crect, COLORREF clrPen, COLORREF clrBackground)
{
	CBrush brush(clrBackground), *pbOld;
	CPen pen(PS_SOLID, 1, clrPen), *ppOld;
	pbOld = pDC->SelectObject(&brush);
	ppOld = pDC->SelectObject(&pen);
	pDC->Ellipse(crect);
	pDC->SelectObject(pbOld);
	pDC->SelectObject(ppOld);

}	
void SVBase::OnListctrlItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
// called if user clicks or keyboards to a new item
// generally called to deselect the old item and another time to select the new
// the focus bit cab be combined wither with the new or the old
// if the current item is 0 and the user clicks item 1, a typical sequence is
//		Call 1
//			p->iItem		= 0
//			p->uOldState	= LVIS_SELECTED
//			p->uNewState	= 0	
//		Call 2
//			p->iItem		= 0
//			p->uOldState	= LVIS_FOCUSED
//			p->uNewState	= 0	
//		Call 3
//			p->iItem		= 1
//			p->uOldState	= 0
//			p->uNewState	= LVIS_SELECTED | LVIS_FOCUSED	
//
// sometimes they can be combined, same sceanrio might come as 
//		Call 1
//			p->iItem		= 0
//			p->uOldState	= LVIS_SELECTED
//			p->uNewState	= 0	
//		Call 2
//			p->iItem		= 1
//			p->uOldState	= LVIS_FOCUSED
//			p->uNewState	= LVIS_SELECTED | LVIS_FOCUSED	
//
// point is, it is best not to depend on a certain sequence
// all we are interested in is whether it was selected or deselected
// we can determine this by looking at the LVIS_SELECTED bit in the uOldState or uNewState
// The bit settings are
//	#define LVIS_FOCUSED            0x0001
//	#define LVIS_SELECTED           0x0002

	LPNMLISTVIEW p = (LPNMLISTVIEW) pNMHDR;
		//temp
		//TRACE(_T("Item %d Old %d New %d\n"), p->iItem, p->uOldState, p->uNewState);
	if (p->uChanged & LVIF_STATE)
	{
		if (p->uNewState & LVIS_SELECTED)
		{
			if (m_nItemSelected != p->iItem)
				SetSelectedCell(p->iItem, p->iSubItem);
		}
		else if (p->uOldState & LVIS_SELECTED) // old state selected? (means item is being de-selected because we have already determined that the new state is not LVIS_SELECTED)
		{
				// let it deselect, but send a message to the detailer indicating that another row should be detailed

				// if the item deselected is the last one we sent a message to, clear it
				if (p->iItem == m_nLastMessageSentObject)
				m_nLastMessageSentObject = -1;
				int nItem = GetListCtrl().GetNextItem(-1, LVNI_SELECTED);
				if (nItem >= 0)
					SendDetailMessage(0);
//13811		  	else
//13811  			SendDetailMessage((UINT) -1, 0);	// this indicates that no objects are selected
			
		}
	}
	*pResult = 0;

}

void SVBase::OnColumnclickListctrl(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CWaitCursor cw;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	int nPreviousSort = m_nLastSort;		// save last sort position	


	if (m_nLastSort == pNMListView->iSubItem)
		::bSortDirection = !::bSortDirection; // same column, flip direction
	else
	{
		m_nLastSort = pNMListView->iSubItem;
		::bSortDirection = ASCENDING;
	}
	m_stSortParm.nColumnNumber = pNMListView->iSubItem;
	m_stSortParm.p = this;
#ifdef _DEBUG
	m_nCtr = 0;
#endif
	GetListCtrl().SortItems(CompareFunc, (LPARAM)&m_stSortParm);	
	
	// position listview to ensure it displays the first selected row
	EnsureSelectedLineVisible();

	if (nPreviousSort >= 0)			// any column image to undo?
		SetHdrImage(nPreviousSort);	// yes, undo it
	SetHdrImage(m_nLastSort);		// set column image for new sort column

	*pResult = 0;

}
#ifdef _DEBUG
int SVBase::m_nCtr = 0;
#endif
int SVBase::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lpParm)
{
// multi column
//		save preceding columns
//		if this compare matches EQUAL, use preceding columns
//		save all previous columns until a column sorted with no duplicates (make that the first column) (iow, there is no need to keep column numbers prior to that, they will not be used)
//
// subcolumns
//
#ifdef _DEBUG
		m_nCtr++;
#endif
		stSortParm* pSortParm = (stSortParm*)lpParm;
		double dResult;
		int nResult;

		// get the datatype of this column
		SVBase* p = (SVBase*)pSortParm->p;
		int ColNum = pSortParm->nColumnNumber;
		_ColType ct =  p->m_pActiveSheet->caColumns[ColNum].ct;

		switch (ct)
		{
		case ctChar:
			{
			CString cs1 = p->GetValueString(lParam1, ColNum);
			CString cs2 = p->GetValueString(lParam2, ColNum);
			if (p->m_pActiveSheet->caColumns[ColNum].bIgnoreCase)
				dResult = cs1.CompareNoCase(cs2);
			else
				dResult = cs1.Compare(cs2);
			
			break;
			}
		case ctInt:
			{
				int nNumSubColumns = p->m_pActiveSheet->caColumns[ColNum].GetNumberSubColumns();
				if ( (nNumSubColumns == 1) || (p->m_pActiveSheet->caColumns[ColNum].sc == scNone) )
				{
					int i1 = p->GetValueInt(lParam1, ColNum);
					int i2 = p->GetValueInt(lParam2, ColNum);
					dResult = i1 - i2;
				}
				else if ( (nNumSubColumns == 2) || (nNumSubColumns == 3) )
				{
					int nTable1[MAXSUBCOLUMNS];
					int nTable2[MAXSUBCOLUMNS];

					if (!p->GetGroupCountsInt(lParam1, ColNum, nNumSubColumns, nTable1))
					{
						dResult = 0;
						break;
					}
					if (!p->GetGroupCountsInt(lParam2, ColNum, nNumSubColumns, nTable2))
					{
						dResult = 0;
						break;
					}

					
					dResult = nTable1[0] - nTable2[0];
					if (dResult != 0)
						break;
					dResult = nTable1[1] - nTable2[1];
					if (nNumSubColumns == 3)
					{
						if (dResult != 0)
							break;
						dResult = nTable1[2] - nTable2[2];
					}

				}
				break;
			}

		case ctInt64:
			{
				__int64 i1 = p->GetValueInt64(lParam1, ColNum);
				__int64 i2 = p->GetValueInt64(lParam2, ColNum);
				dResult = double(i1 - i2);
				break;
			}
		case ctBoolean:
			{
				int n1 = (int)p->GetValueBool(lParam1, ColNum);
				int n2 = (int)p->GetValueBool(lParam2, ColNum);
	
				dResult = n1-n2;
				break;
			}

		case ctFloat:
			{
				// make it a number
				double d1 = p->GetValueDouble(lParam1, ColNum);
				double d2 = p->GetValueDouble(lParam2, ColNum);
	
				dResult = d1-d2;
				break;
			}
		case ctDateTime:
			{ 
				COleDateTime cdt1 = p->GetValueDateTime(lParam1, ColNum);
				COleDateTime cdt2 = p->GetValueDateTime(lParam2, ColNum);
				if (cdt1.GetStatus() != COleDateTime::valid)
				{
					if (cdt2.GetStatus() != COleDateTime::valid)
						dResult = 0;	// neither date valid, make them compare equal
					else
						dResult = 1;	// make invalid date sort lower
					break;
				}
				if (cdt2.GetStatus() != COleDateTime::valid)
				{
					dResult =   -1;		// make invalid date sort lower
					break;
				}

				if (cdt1 < cdt2)
					dResult = -1;
				else if(cdt1 > cdt2)
					dResult = 1;
				else
					dResult = 0;
				break;
			}
		case ctUnknown:
  		default:
			dResult = 0;
		}
		if (dResult < 0)
			nResult = -1;
		else if (dResult > 0)
			nResult = 1;
		else 
			nResult = 0;
		return ::bSortDirection ? nResult : -nResult; 
	
}

void SVBase::DeleteAllRows()
{
	// if the list 
	//  empty the listview control
	GetListCtrl().DeleteAllItems();

}
////////////////////////////////////////////////////////////////
//   Public functions
////////////////////////////////////////////////////////////////
POSITION SVBase::GetFirstSelectedItem()
{
	return GetListCtrl().GetFirstSelectedItemPosition();
}

int SVBase::GetNextSelectedItem(POSITION& pos) 
{
	if (pos)
	{
		int nItem = GetListCtrl().GetNextSelectedItem(pos);
		int nObject = GetListCtrl().GetItemData(nItem);
		return nObject;
	}
	return -1;
}

int SVBase::SelectAll(bool bVisibleOnly)
{
	ASSERT (bVisibleOnly);	// for now, only do visible ones
	int nCount = GetListCtrl().GetItemCount();
	for (int i=0; i<nCount; i++)
	{
		SelectItem(i);
	}
	return nCount;
}

int SVBase::SelectMatchedItems()
{
	// where is the last selected cell?	
	if ( (m_nItemSelected < 0) || (m_nSubItemSelected < 0) )
		return 0;
	
	int nCtr = 0;
	// just do a string compare for now

	CString csWork;
		
	CString csCompare = GetValueAsText(GetListCtrl().GetItemData(m_nItemSelected), m_nSubItemSelected);;



	for (int i = 0; i<GetListCtrl().GetItemCount(); i++)
	{
		csWork = GetValueAsText(GetListCtrl().GetItemData(i), m_nSubItemSelected);
		if (csCompare.Compare(csWork) == 0)
		{
				SelectItem(i);
		}
	}

	return nCtr;
}

int SVBase::GetHiddenRowCount()
{

	return m_caDeletedObjects.GetSize();
}


int SVBase::RevealAll(bool bReSort)
{
	int nCount = GetHiddenRowCount();
	for (int i=0; i<nCount; i++)
		AddRow(m_caDeletedObjects[i]);
	m_caDeletedObjects.RemoveAll();

	if (bReSort && m_nLastSort >= 0)
	{
		m_stSortParm.nColumnNumber = m_nLastSort;
		m_stSortParm.p = this;
		GetListCtrl().SortItems(CompareFunc, (LPARAM)&m_stSortParm);	
	}

	m_btTotal.RedrawWindow();

	return nCount;	
}

int SVBase::HideSelected() 
{

	int nCtr = 0;

	CArray <int, int> caItems;

	int nItem = GetListCtrl().GetNextItem(-1, LVNI_SELECTED);
	while(nItem >= 0)
	{
		caItems.Add(nItem);
		nCtr++;
		nItem = GetListCtrl().GetNextItem(nItem, LVNI_SELECTED);
	}

	for (int i=caItems.GetSize()-1; i>=0; i--)	// delete backwards so counts remain valid through loop
		DeleteRow(caItems[i]);

	m_nItemSelected = m_nSubItemSelected = -1;	// these are no longer valid

	m_btTotal.RedrawWindow();					// ensure new counts are displayed

	return nCtr;
}


int SVBase::HideNotSelected() 
{
	int nCtr = 0;

	for (int i=GetListCtrl().GetItemCount()-1; i >= 0; i--)
	{
		if ( (GetListCtrl().GetItemState(i, LVIS_SELECTED) != LVIS_SELECTED) )
		{
			DeleteRow(i);
			nCtr++;
		}
	}

	m_btTotal.RedrawWindow();

	return nCtr;
}


////////////////////////////////////////////////////////////////
void SVBase::OnSelectSame() 
{
	SelectMatchedItems();
}
void SVBase::OnHideSelected() 
{
	HideSelected();
}

void SVBase::OnHideNotSelected() 
{
	HideNotSelected();
}
void SVBase::OnRecoverDeleted()
{
	RevealAll(true);
}
void SVBase::OnColumnSelector()
{
	LaunchColumnSelector(m_nSubItemSelected);
}

void SVBase::LaunchColumnSelector(int nCol) 
{
	
	// if I was on a column, pass it so it will initialize there
	//make a copy of the array in case the user cancel's the dialog
	CArray<SVColumn, SVColumn> caColumns;
	for (int i=0; i<m_pActiveSheet->caColumns.GetSize(); i++)
		caColumns.Add(m_pActiveSheet->caColumns[i]);
	SVDColumnSelector sdcs(&caColumns, nCol, this);
	int nButton = sdcs.DoModal();
	if (nButton == IDOK)
	{

		m_pActiveSheet->caColumns.RemoveAll();
		for (int i=0; i<caColumns.GetSize(); i++)
		{
			m_pActiveSheet->caColumns.Add(caColumns[i]);
			SetHdrImage(i);
		}

//		Have to go through the table twice, 
//		if i try and do the column widths in the previous loop,
//		a repaint may occur before I have finished

		for (i=0; i<m_pActiveSheet->caColumns.GetSize(); i++)
		{
			if (m_pActiveSheet->caColumns[i].bColumnWidthDirty)
			{
				if (m_pActiveSheet->caColumns[i].cShowStatus != SHOW)
					SetColumnWidth(i, 0);
				else
				{
					if (m_pActiveSheet->caColumns[i].nPixelWidth <  DRAGMINIMUMWIDTH)
						m_pActiveSheet->caColumns[i].nPixelWidth = 3 * DRAGMINIMUMWIDTH;
					SetColumnWidth(i, m_pActiveSheet->caColumns[i].nPixelWidth);
				}

				m_pActiveSheet->caColumns[i].bColumnWidthDirty = false;
			}
		}
	}

	GetListCtrl().RedrawWindow();		// needed in ccmain
	RedrawWindow();						// needed in smmain
}

void SVBase::SetColumnWidth(int nColumn, int nWidth)
{

	LVCOLUMN lvc;
	lvc.cx = nWidth;
	lvc.mask = LVCF_WIDTH;
	GetListCtrl().SetColumn(nColumn, &lvc);
}

void SVBase::OnSelectAll() 
{
	SelectAll();
}
void SVBase::OnHelp() 
{

	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 
//#endif
//	AfxGetApp()->WinHelp(HID_ topic ID, HELP_CONTEXT);
	
}
bool SVBase::UpdateView(void *p)	// new collection
{
	ASSERT(false);		// support not in derived view
	return false;
}
bool SVBase::StartView(void *p, SVSheet *pSheet, void* pMaps, int nSortColumn, bool bSortDirection)
{

	ASSERT (pSheet->pView == 0);	// ensure I am not reusing some view
	pSheet->pView = this;			


	if (!pSheet)
		return false;

	m_pActiveSheet = pSheet;

//	m_pActiveSheet->caColumns

//qq	m_pActiveSheet->caColumns.RemoveAll();


	StartSubView(p, pMaps);
#ifdef _DEBUG 
		{
			CRuntimeClass* pc = GetRuntimeClass();
			if (pSheet->csName != pc->m_lpszClassName)
			{
				CString cs;
				cs.Format(_T("Sheet Created with %s, now being used with %s"),
					pSheet->csName,
					pc->m_lpszClassName);
				AfxMessageBox(cs);
				ASSERT (false); // look above
			}
		}
#endif



// set column order
	int nNumColumns = m_pActiveSheet->caColumns.GetSize();
	if (m_pActiveSheet->nColumnOrder[0] != -1)	// -1 means never initialzied
		GetListCtrl().SetColumnOrderArray(nNumColumns, m_pActiveSheet->nColumnOrder);
	m_bOrderDirty = false;	// no need to build the array
// set column widths
	for (int i=0; i<nNumColumns; i++)
	{
		if (m_pActiveSheet->caColumns[i].cShowStatus != SHOW)
			SetColumnWidth(i, 0);
		else
			SetColumnWidth(i, m_pActiveSheet->caColumns[i].nPixelWidth);
	}
	
	if (nSortColumn >= 0)
	{
		m_nLastSort = nSortColumn;
		::bSortDirection = bSortDirection;
		m_stSortParm.nColumnNumber = nSortColumn;
		m_stSortParm.p = this;
		GetListCtrl().SortItems(CompareFunc, (LPARAM)&m_stSortParm);	
		SetHdrImage(m_nLastSort);		// set column image for new sort column

	}

	GetListCtrl().RedrawWindow();
	m_nItemSelected = -1;
	m_nSubItemSelected = -1;

	SelectItem(0);

	return true;
}
bool SVBase::StartSubView(void *p, void *pMaps)
{
	ASSERT(false);
	return true;
}


LRESULT SVBase::SendDetailMessage(int nColumn)
{
	// send a message to the topmost item that is selected
	int nItem = GetListCtrl().GetNextItem(-1, LVNI_SELECTED);
	if (nItem >= 0)
	{
		int nObjectNumber = GetListCtrl().GetItemData(nItem);
		if (nObjectNumber >= 0)
			return SendDetailMessage((UINT) nObjectNumber, nColumn);
	}
	return true;
}


LRESULT SVBase::SendDetailMessage(UINT nObjectNumber, int nColumn)
{
	// pass the object number and the last column to the frame (only if the previous send object number is different)
	if ((UINT)m_nLastMessageSentObject != nObjectNumber)
	{
		GetParent()->PostMessage(MESSAGE_DETAIL, nObjectNumber, nColumn);
		m_nLastMessageSentObject = nObjectNumber;
	}
	return true;
}



int SVBase::DeleteRowsThatMatch(int nColumn, _TCHAR* lpszmyString, bool bDelete, int nStart)
{

	_TCHAR szString[MAXSTRINGWIDTH];
	int nCounter = 0;

	for (int i=GetListCtrl().GetItemCount()-1; i >= nStart; i--)
	{
		GetListCtrl().GetItemText(i, nColumn, szString, MAXSTRINGWIDTH-1);
		if ( (CompareMask(lpszmyString, szString)) ^ (!bDelete) )
		{
			DeleteRow(i);
			nCounter++;
		}
	}

	return nCounter;
}

int SVBase::DeleteRowsThatMatch(int nColumn, int nLow, int nHigh, bool bDelete, int nStart)
{

	int nCounter = 0;
	int nWork;
	for (int i=GetListCtrl().GetItemCount()-1; i >= nStart; i--)
	{
		nWork = GetValueInt(i, nColumn);
		if ( ((nLow <= nWork) && (nWork <= nHigh))  ^ (!bDelete) )
		{
			DeleteRow(i);
			nCounter++;
		}
	}

	return nCounter;
}
int SVBase::DeleteRowsThatMatch(int nColumn, double dLow, double dHigh, bool bDelete, int nStart)
{

	int nCounter = 0;
	double dWork;
	for (int i=GetListCtrl().GetItemCount()-1; i >= nStart; i--)
	{
		dWork = GetValueDouble(i, nColumn);
		if ( ((dLow <= dWork) && (dWork <= dHigh))  ^ (!bDelete) )
		{
			DeleteRow(i);
			nCounter++;
		}
	}

	return nCounter;
}

int SVBase::DeleteRowsThatMatch(int nColumn, COleDateTime cdtLow, COleDateTime cdtHigh, bool bDelete, int nStart)
{

	int nCounter = 0;
	COleDateTime cdtWork;
	for (int i=GetListCtrl().GetItemCount()-1; i >= nStart; i--)
	{
		cdtWork = GetValueDateTime(i, nColumn);
		if ( ((cdtLow <= cdtWork) && (cdtWork <= cdtHigh))  ^ (!bDelete) )
		{
			DeleteRow(i);
			nCounter++;
		}
	}

	return nCounter;
}



//void SVBase::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult) 
//{/
//	LV_DISPINFO* pLVEVi = (LV_DISPINFO*)pNMHDR;
//	int col;
//	 if (pLVEVi->item.mask & LVIF_TEXT)	// if caller wants text
//	 {
//		col = pLVEVi->item.iSubItem; 
//		if (col < 0)
//			return;
////		_tcscpy (pLVEVi->item.pszText, (LPCTSTR) GetValueAsText(pLVEVi->item.lParam, col));
//	//_tcscpy (pLVEVi->item.pszText
//	 }
//	*pResult = 0;
//}

	//COleDateTime

CString SVBase::GetValueAsText(SVColumn* pCol, UINT nObjectNumber, int nColumn)
{
	if (pCol->IsDate())
	{
		COleDateTime cdt = GetValueDateTime(nObjectNumber, nColumn);
		if  (cdt.GetStatus() != COleDateTime::valid)
			return "";
		else
			return cdt.Format();
	}
	if (pCol->IsNumber())
		return 	GetFormattedNumber(pCol, nObjectNumber, nColumn);
	if (pCol->IsCharacter())
		return GetValueString(nObjectNumber, nColumn); 
	if (pCol->ct == ctBoolean)
		return (GetValueBool(nObjectNumber, nColumn) ? RESCHAR(IDS_V_TRUE) : RESCHAR(IDS_V_FALSE));
	
	return _T("Not Implemented (j11)");
}
CString SVBase::GetValueAsText(UINT nObjectNumber, int nColumn)
{
	// routine has been disabled, replaced with one above 
	ASSERT (false);
	return "";
}



bool SVBase::GetValueBool(UINT nObjectNumber, int nColumn)
{
	if ((nObjectNumber % 2))
		return true;
	return false;

	ASSERT (false);
	return false;
}

CString SVBase::GetValueString(UINT nObjectNumber, int nColumnNumber)
{
	static CString cs;
	cs = GetValueAsText(nObjectNumber, nColumnNumber);
	return cs;
}

int SVBase::GetValueInt(UINT nObjectNumber,int nColumnNumber)
{
	return nObjectNumber;
}

__int64 SVBase::GetValueInt64(UINT nObjectNumber,int nColumnNumber)
{
	return nObjectNumber;
}

double SVBase::GetValueDouble(UINT nObjectNumber, int nColumnNumber)
{
	return nObjectNumber;
}

COleDateTime SVBase::GetValueDateTime(UINT nObjectNumber, int nColumnNumber)
{
	COleDateTime cdt;
	cdt = COleDateTime::GetCurrentTime();
	return cdt;
}
int SVBase::GetCountSelected()
{
	return GetListCtrl().GetSelectedCount(); 
}

int SVBase::GetCountNotSelected()
{
	return GetListCtrl().GetItemCount() - GetListCtrl().GetSelectedCount(); 
}


void SVBase::OnClickListctrl(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// if just one item selected, do it

	int nCount = GetCountSelected();


//	if (nCount == 1)
	{

		DWORD dwPos = ::GetMessagePos();
		CPoint point((int)LOWORD(dwPos), (int) HIWORD(dwPos));
		ScreenToClient(&point);

		// find cell where clicked
		LVHITTESTINFO  lvhit;
		lvhit.pt = point;
		int nItem = GetListCtrl().SubItemHitTest(&lvhit);


//		if (nItem >= 0)
//			SetSelectedCell(&lvhit);
	}
	m_btTotal.RedrawWindow();		// redraw total line
}

bool SVBase::SetSelectedCell(LVHITTESTINFO* plv)
{
	GetListCtrl().RedrawItems(m_nItemSelected, m_nItemSelected);
	return SetSelectedCell(plv->iItem, plv->iSubItem);
}

bool SVBase::SetSelectedCell(int nItem, int nSubItem)
{

	m_nItemSelected = nItem;
	m_nSubItemSelected = nSubItem;
	GetListCtrl().RedrawItems(m_nItemSelected, m_nItemSelected);	
	LRESULT rc = SendDetailMessage(m_nSubItemSelected);
	return (rc != 0);
}


void SVBase::SetMenuItem(CMenu* pMenu, int nAfterPosition, int nColumn)
{

	SVColumn* pCol = &m_pActiveSheet->caColumns[nColumn];

	if (pCol->IsNumber())
	{
		CString csWork;
		CString csTemplate;
		CString csItemText = GetListCtrl().GetItemText(m_nItemSelected, m_nSubItemSelected);
		if (!csItemText.IsEmpty())
		{
			csTemplate = RESCHAR(IDS_V_MENU_NUMBER_LESS);
			csWork.Format(csTemplate, csItemText); 
			pMenu->InsertMenu(nAfterPosition, MF_BYPOSITION, 1001, csWork);
			pMenu->InsertMenu(nAfterPosition+1, MF_BYPOSITION, 1002, csWork);
		}

	}

}



bool SVBase::SelectItem(int nItem)
{
	if ( (nItem >= 0) && (GetListCtrl().GetItemCount() > nItem) )
		return (GetListCtrl().SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, 0x000f) == TRUE);
	return false;
}

void SVBase::DeleteRow(int nItem)
{
	m_caDeletedObjects.Add(GetListCtrl().GetItemData(nItem));
	GetListCtrl().DeleteItem(nItem);
}

bool SVBase::RedrawObject(UINT nObjectNumber)
{
	ASSERT(false);	// sorry, can you please use bool SVBase::UpdateObject(UINT nObjectNumber)
	return UpdateObject(nObjectNumber);
}
bool SVBase::UpdateObject(UINT nObjectNumber)
{

	DestroySavedValues(nObjectNumber);

// if the row that contains the object number is visible, do a repaint on that row	


	int nCurrent = GetListCtrl().GetTopIndex();
	int nCountPerPage = GetListCtrl().GetCountPerPage();
	int nItemCount = GetListCtrl().GetItemCount();
	int nLast;
	if (nCountPerPage > nItemCount)
		nLast = nCurrent + nItemCount;
	else
		nLast = nCurrent + nCountPerPage;

	bool bFound = false;
	for (;nCurrent < nLast; nCurrent++)
	{
		if (GetListCtrl().GetItemData(nCurrent) == (DWORD)nObjectNumber)
		{
			GetListCtrl().RedrawItems(nCurrent, nCurrent);
			bFound = true;
		}
	}
	return bFound;
}
void SVBase::DestroySavedValues(UINT nObjectNumber)
{
}
void SVBase::SetColumnWidthsInSheet()
{
	int nNumColumns = GetListCtrl().GetHeaderCtrl()->GetItemCount();
	for (int i=0; i<nNumColumns; i++)
		m_pActiveSheet->caColumns[i].nPixelWidth = GetListCtrl().GetColumnWidth(i);
}
void SVBase::OnColumnHeaderEndDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMHEADER phdn = (LPNMHEADER) pNMHDR;
	int nItem = phdn->iItem;	// item that was moved
	HDITEM* pItem = phdn->pitem;
	if (pItem->mask & HDI_ORDER)
	{
		// the column order is not yet done, I find it easier to use the GetColumnOrderArray
		// than to rearrange all the columns myself. Therefore, I will just set a switch
		// indicating it needs to be donw, and I will do it in paint
//		GetListCtrl().GetColumnOrderArray(m_pActiveSheet->nColumnOrder);	// save new column layout in the sheet
		m_bOrderDirty = true;		
	}
	else
		ASSERT(false);		// how did I get here then
//	*pResult = 1;	// to disallow the drag, set to 1
	*pResult = 0;	
}



void SVBase::OnColumnHeaderChanging(NMHDR* pNMHDR, LRESULT* pResult)
{

	LPNMHEADER phdn = (LPNMHEADER) pNMHDR;

//	static bool bOldKey = false;		// start out with control key down
	int nKey =  GetKeyState(VK_CONTROL);

	// m_bFixedOverride = true means to override the users setting on what is fixed
	if (nKey < 0)	// control key hit
	{
		m_bFixedOverride = true;
//		SVColumn* pCol = &m_pActiveSheet->caColumns[phdn->iItem];//			pCol->bFixedText ^= true;
//		if (pCol->bFixedText)
//		{
//			//pCol->nPixelWidth;	
//			pCol->nPixelFixedWidth = pCol->nPixelWidth - pCol->nPixelFixedWidth;
//			pCol->bFixedText = false; //change to graphic prioriry
//		}
	}
	else
		m_bFixedOverride = false;



	// has control key switched settings?
//	if ( bOldKey != m_bFixedOverride)		// is this different than prior
//	{
//		bOldKey = m_bFixedOverride;
//		m_bFixedOverrideChange = true;
//	}
	

	
	if ( (phdn->pitem->mask & HDI_WIDTH) )
	{
			if (phdn->pitem->cxy < DRAGMINIMUMWIDTH)
				phdn->pitem->cxy = 0;
	}
	*pResult = 0;
	m_bCurrentlyTrackingHeader = false;

}


void SVBase::OnColumnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult) 
{

	m_bFixedOverride = false;	// tunr this off in case it was on
	HD_NOTIFY * phdn = (HD_NOTIFY *) pNMHDR;

	if (phdn->pitem->mask & HDI_WIDTH) 
	{

//		int nKey =  GetKeyState(VK_CONTROL);

		// m_bFixedOverride = true means to override the users setting on what is fixed
//		if (nKey < 0)
//		{
//			SVColumn* pCol = &m_pActiveSheet->caColumns[phdn->iItem];//			pCol->bFixedText ^= true;
//			if (pCol->bFixedText)
//			{
//				//pCol->nPixelWidth;	
//				pCol->nPixelFixedWidth = pCol->nPixelWidth - pCol->nPixelFixedWidth;
//				pCol->bFixedText = false; //change to graphic prioriry
				
//			}
		// switch prority
//		if (m_pActiveSheet->caColumns[phdn->iItem].cShowStatus == HIDE;

//	}
//		m_bFixedOverride = true;
//	else
//		m_bFixedOverride = false;
	


		if (phdn->pitem->cxy > 0) 		// use dragged a column open?
		{
			if (m_pActiveSheet->caColumns[phdn->iItem].cShowStatus != SHOW)	// was it hidden?
			{
				m_pActiveSheet->caColumns[phdn->iItem].cShowStatus = SHOW;
				GetListCtrl().RedrawWindow();
			}
		}

		if (phdn->pitem->cxy < DRAGMINIMUMWIDTH)		// use dragged a column closed?
		{
			if (m_pActiveSheet->caColumns[phdn->iItem].cShowStatus == SHOW)	// was it shown?
			{
				m_pActiveSheet->caColumns[phdn->iItem].cShowStatus = HIDE;
				GetListCtrl().RedrawWindow();
			}
		}
		m_pActiveSheet->caColumns[phdn->iItem].nPixelWidth = phdn->pitem->cxy;
		
	}
	
	*pResult = 0;
}


BOOL SVBase::OnNeedText( UINT id, NMHDR * pTTTStruct, LRESULT * pResult )
{
	if (!pTTTStruct)
		return TRUE;
// first part of pTTTStruct is the header, use this to find out if it is from our list control
	if ((HWND)pTTTStruct->idFrom == GetDlgItem(IDC_LISTCTRL)->m_hWnd)
	{
		// past the header is the stuff that pertains to the tooltip "need text"
		TOOLTIPTEXT* pt = (TOOLTIPTEXT*) pTTTStruct;
		_tcscpy(pt->szText,_T("scarf city"));
	}
	//temp
//		TOOLTIPTEXTA* pttta = (TOOLTIPTEXTA*)pTTTStruct;
//end of temp
	return TRUE;
}

CString SVBase::GetTotalsType()
{
	return RESCHAR(IDS_V_DEFAULTTYPE);
}
double SVBase::ConvertXPixel(double dValue, double dMinValue, double dMaxValue, double dWidthPixel)
{
	double x = ((dValue - dMinValue)	/ (dMaxValue-dMinValue)) * dWidthPixel;
	return x;
}

int SVBase::GetXPixel(SVColumn *pCol, COleDateTime cdt, int nTotalWidth)
{
	double d, x;
	COleDateTimeSpan dCurrent = cdt - pCol->cdtMin;
	COleDateTimeSpan dRange = pCol->cdtMax - pCol->cdtMin;
//temp
//	CString cs1 = pCol->cdtMin.Format();
//	CString cs2 = pCol->cdtMax.Format();
//	CString cs3 = cdt.Format();
	d = dCurrent / dRange;
	x = d * nTotalWidth;
	if (x < 0)
		x = 0;
	if (x > nTotalWidth)
		x = nTotalWidth;
	return (int)x;

}
int SVBase::GetXPixel(SVColumn *pCol, UINT nObjectNumber, int nColumn, int nTotalWidth)
{
	switch (pCol->ct)
	{
		case ctInt:
		{
			double dRange = pCol->nMaxValue - pCol->nMinValue;
			double d = (double)GetValueInt(nObjectNumber, nColumn);
			double x = ((d - (double)pCol->nMinValue) / dRange) * (double)nTotalWidth;
			return (int)x;
			break;
		}
		case ctInt64:
		{
			double d = (double)GetValueInt64(nObjectNumber, nColumn);
			double x = (double)((d - pCol->n64MinValue) / (pCol->n64MaxValue- pCol->n64MinValue)) * nTotalWidth;
			return (int)x;
			break;
		}
		case ctFloat:
		{
			double d = GetValueDouble(nObjectNumber, nColumn);
			double x = ((d - pCol->dMinValue) / (pCol->dMaxValue- pCol->dMinValue)) * nTotalWidth;
			return (int)x;
			break;
		}
		case ctDateTime:
		{
			ASSERT(false);
			double d, x;
			COleDateTime cdt = GetValueDateTime(nObjectNumber, nColumn);
			COleDateTimeSpan dCurrent = cdt - pCol->cdtMin;
			COleDateTimeSpan dRange = pCol->cdtMax - pCol->cdtMin;
//debug
			CString cs1 = pCol->cdtMin.Format();
			CString cs2 = pCol->cdtMax.Format();
			CString cs3 = cdt.Format();
			d = dCurrent / dRange;
			x = d * nTotalWidth;
			return (int)x;
			break;
		}
		
	}
	return 0;
}


CString SVBase::GetFormattedNumber(SVColumn *pCol, UINT nObjectNumber, int nColumn)
{

	CString csValue;
	switch (pCol->ct)
	{
		case ctInt:
			{
				int n = GetValueInt(nObjectNumber, nColumn);
				csValue = m_SVFormatNumber.FormatNumber(n, pCol->ut);
				break;
			}
		case ctInt64:
			{
				__int64 n = GetValueInt64(nObjectNumber, nColumn);
				if (pCol->ut == utNone) 
					csValue = m_SVFormatNumber.FormatNumber((double)n, pCol->ut, true);
				else
					csValue = m_SVFormatNumber.FormatNumber((double)n, pCol->ut);

//				csValue = m_SVFormatNumber.FormatNumber((double)n, pCol->ut);//true);
				break;
			}
		case ctFloat:
			{
				double d = GetValueDouble(nObjectNumber, nColumn);
				if (pCol->ut == utNone) 
					csValue = m_SVFormatNumber.FormatNumber(d, pCol->ut, true);
				else
					csValue = m_SVFormatNumber.FormatNumber(d, pCol->ut);
				break;
			}
	}
	return csValue;
}

bool SVBase::GetMultiPixelTable(int nCount, int *nValueTable, int *nPixelTable, int nPixelTotal)
{
	int nValueTotal = 0;
	for (int i=0; i<nCount; i++)
		nValueTotal += nValueTable[i];
	double dRatio = (double)nValueTotal / (double) nPixelTotal;
	for (i=0; i<nCount; i++)
			nPixelTable[i] = (int)((double)nValueTable[i] / dRatio);
	return true;
}

void SVBase::SetColors(CDC* pDC,  CRect crCell, int nRule, int nColumn, bool bSelected, SVColumn *pCol, int nDisplayRowNumber)
{
	bool bColorText = false;		// here until support put in SVColumn

// if selected
//		BackgroundColor = Windows Selected Background Color
//		Text Color		= Windows Selected Text Color
//		Graphic Color	= Color per SVColumn Specification
// if not selected
//		BackgroundColor = Per View Specifier (or Windows Default Window Color)
//		Text Color
//			if (bColorText and Text Only)
//						= Color per SVColumn Specification 
//			else
//						= Windows Non-Selected Text Color
//		Graphic Color	= Color per SVColumn Specification





// set graphic color
	m_clrGraphicColor = ::GetSysColor(COLOR_WINDOWTEXT);	// default graphic color

	if (pCol->clr != -1)	// user selected a color
	{ 
		if ( (pCol->rt == rtNone) || (nRule == 0) )	// no rule or rule = false
			m_clrGraphicColor = pCol->clr;
		else
		{
			// a binary state rule is in effect
			// the rule is true
			if (pCol->clrRule[COLORRULE0] != -1) 
				m_clrGraphicColor = pCol->clrRule[COLORRULE0];
			else	// rule in effect but no color picked, use default
				m_clrGraphicColor = ::GetSysColor(COLOR_WINDOWTEXT);
		
		}
	}	

// set text color and background color
	if (bSelected)
	{
		m_clrTextColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_clrBackgroundColor = ::GetSysColor(COLOR_HIGHLIGHT);
	}
	else
	{	// not selected, if a graphic component, color the text the standard color
		// else color it the rule color
		// bColorText is there in case we later have an option to color it regardless (currently set to false)
		if (bColorText || (!pCol->bGraphic) )
			m_clrTextColor = m_clrGraphicColor;
		else
			m_clrTextColor = ::GetSysColor(COLOR_WINDOWTEXT);

		m_clrBackgroundColor = GetDefaultBackgroundColor(nDisplayRowNumber);
	}


//	if ( (nItem == m_nItemSelected) && (nColumn == m_nSubItemSelected) )
//	{
//			pDC->FrameRect(crCell, &CBrush(RGB(255,0,0)));
//
//	}




// if sort column, shade it
	if (nColumn == m_nLastSort)
	{
		m_clrBackgroundColor = RGB(
			GetRValue(m_clrBackgroundColor) - 10,
			GetGValue(m_clrBackgroundColor) - 10,
			GetBValue(m_clrBackgroundColor) - 10);
	}

	pDC->FillRect(crCell, &CBrush(m_clrBackgroundColor));
	pDC->SetTextColor(m_clrTextColor);
}

COleDateTime SVBase::GetRandomDateTime()
{
// just for testing

	static bool bFirst = true;
	if (bFirst)
	{
		srand( (unsigned)time( NULL ) );
		bFirst = false;
	}
	
	COleDateTime cdt = COleDateTime::GetCurrentTime();
	int nDays = rand() % TESTDATESPAN;
	COleDateTimeSpan dts(nDays, 0, 0, 0);

	return cdt-dts;


}

CString SVBase::GetColumnName(int nColumn)
{
#define TEXTLEN 100
	CHeaderCtrl* pHdr = GetListCtrl().GetHeaderCtrl();
	if (nColumn == -1)
		nColumn = GetMainColumnNumber();
	if ( (nColumn < 0) || (nColumn >= pHdr->GetItemCount()) )
		return "";
	TCHAR lpBuffer[TEXTLEN];
	HDITEM hdItem;
	hdItem.mask = HDI_TEXT;
	hdItem.pszText = lpBuffer;
	hdItem.cchTextMax = TEXTLEN;
	pHdr->GetItem(nColumn, &hdItem);
	return lpBuffer;
}

int SVBase::GetMainColumnNumber()
{ // derived classes override this if main column number is not column 0
	return 0;
}


CString SVBase::GetPathOnly(CString csFile)
{
	return csFile.Left(csFile.ReverseFind('\\')+1);
}
CString SVBase::GetDefaultPath()
{
	CWinApp* pApp = AfxGetApp();  
	TCHAR szFilename[_MAX_FNAME+1]; _tcscpy(szFilename, pApp->m_pszExeName);  
	TCHAR szFileExt[5] = _T(".exe");
	TCHAR szOutput[_MAX_PATH+1];
	TCHAR* pszOutputExt;
	CString csPath;

	DWORD dwLen = SearchPath(
				0,					// search path
				szFilename,			// file name
				szFileExt,			// file extension
				_MAX_PATH,			// size of buffer
				szOutput,			// found file name buffer
				&pszOutputExt);		// file component
	if (dwLen > 0)
	{
		csPath = szOutput;
		csPath = csPath.Left(pszOutputExt-szOutput);
	}
	return csPath;	
}
CString SVBase::FormatSysMessage(long lMsg)
{
#define MESSAGESIZE 256
	CString csReturn;
	TCHAR pBuff[MESSAGESIZE+1];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
	NULL,
	lMsg,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language,
	pBuff,
	MESSAGESIZE, // DWORD nSize, 
	0); //va_list *Arguments  
	return pBuff;
}

bool SVBase::EnsureSelectedLineVisible()
{

	int nItem = GetListCtrl().GetNextItem(-1, LVNI_SELECTED);
	if (nItem >= 0)
	{
		GetListCtrl().EnsureVisible(nItem, FALSE);
		return true;
	}
	return false;
}

void SVBase::DisableView()
{
	m_bDisabled = true;
}

int SVBase::zOnToolHitTest(CPoint  point, TOOLINFO * pti) const
{

//	int nRow, nCol;
	RECT rectCell;
//	nRrow = CellRectFromPoint(point, &rectCell, &nCol );

//	if ( nRow == -1 ) 
//		return -1;

	pti->hwnd = GetDlgItem(IDC_LISTCTRL)->m_hWnd;
//	pti->uId = (UINT)((nRow<<10)+(nCol&0x3ff)+1);
	pti->uId = 34; 

	pti->lpszText = LPSTR_TEXTCALLBACK;

	rectCell.left = 0;
	rectCell.right = 1000;
	rectCell.top = 0;
	rectCell.bottom = 500;
	pti->rect = rectCell;
	return pti->uId;
	
	return 0;
}
bool SVBase::OnToolTipText(UINT id, NMHDR * pnmhdr, LRESULT * presult )
{
	TOOLTIPTEXTA* pttta = (TOOLTIPTEXTA*)pnmhdr;

/*
	// need to handle both ansi and unicode versions of the message
	tooltiptexta* pttta = (tooltiptexta*)pnmhdr;
	tooltiptextw* ptttw = (tooltiptextw*)pnmhdr;
	cstring strtiptext;
	uint nid = pnmhdr->idfrom;

	if( nid == 0 )	  	// notification in nt from automatically
		return false;   	// created tooltip

	int row = ((nid-1) >> 10) & 0x3fffff ;
	int col = (nid-1) & 0x3ff;
	strtiptext = getitemtext( row, col );

#ifndef _unicode
	if (pnmhdr->code == ttn_needtexta)
		lstrcpyn(pttta->sztext, strtiptext, 80);
	else
		_mbstowcsz(ptttw->sztext, strtiptext, 80);
#else
	if (pnmhdr->code == ttn_needtexta)
		_wcstombsz(pttta->sztext, strtiptext, 80);
	else
		lstrcpyn(ptttw->sztext, strtiptext, 80);
#endif
	*presult = 0;
*/
	return true;    // message was handled
}

void SVBase::OnHideSelectUI(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetCountSelected() > 0);	
}
void SVBase::OnHideNotSelectUI(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetCountSelected() != GetListCtrl().GetItemCount());
	
}
void SVBase::OnRevealUI(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetHiddenRowCount() > 0);	
}

