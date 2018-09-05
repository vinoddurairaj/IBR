// SVDColumnSelector.cpp : implementation file
//

#include "stdafx.h"
#include "SVGlobal.h"
#include "resource.h"     
#include "SVBase.h"
#include "SVDColumnSelector.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COL_NAME	0
#define COL_TYPE	1
#define COL_COLOR	2

#define UNKNOWN		-1

#define VALUE_ON	0
#define COLUMN_ON	1

/////////////////////////////////////////////////////////////////////////////
// SVDColumnSelector dialog


SVDColumnSelector::SVDColumnSelector(CArray<SVColumn, SVColumn>* pct,
								   int nStartColumn,
								   CWnd* pParent /*=NULL*/)
	: CDialog(SVDColumnSelector::IDD, pParent)
{
	//{{AFX_DATA_INIT(SVDColumnSelector)
	m_csProductName = _T("");
	m_bShowNum = FALSE;
	m_bShowGraph = FALSE;
	m_bHighlightCondition = FALSE;
	m_dNumValue = 0.0;
	m_csCharValue = _T("");
	m_bHighlightCondition2 = FALSE;
	m_bShowGraph2 = FALSE;
	m_nDateValue = 1;
	m_nDateStyle = -1;
	m_bIgnoreCase = FALSE;
	m_bShowDateText = FALSE;
	m_nRadioColval = -1;
	m_csColumnName = _T("");
	m_bRadioJCenter = 0;
	m_bRadioJRight = 0;
	m_bRadioJLeft = 0;
	//}}AFX_DATA_INIT
	m_pColumn = pct;
	m_nStartColumn = nStartColumn;

	m_bDirty = false;
	m_pImage = 0;

	m_clrSelectedLine = RGB(228, 228, 255);   
	m_clrNotSelectedLine = GetSysColor(COLOR_WINDOW);

	m_nGraphResourceID = -1;

	m_hIcon3Values = m_hIcon3Bars = m_hIcon1Bar = 0; 


}

SVDColumnSelector::~SVDColumnSelector()
{
	if (m_hIcon3Values)
		DestroyIcon(m_hIcon3Values);
	if (m_hIcon3Bars)
		DestroyIcon(m_hIcon3Bars);
	if (m_hIcon1Bar)
		DestroyIcon(m_hIcon1Bar);

}

void SVDColumnSelector::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SVDColumnSelector)
	DDX_Control(pDX, IDC_CURRENTDATE, m_shCurrentDate);
	DDX_Control(pDX, IDC_WITHINDATE, m_shWithinDate);
	DDX_Control(pDX, IDC_GRAPH2, m_shGraph2);
	DDX_Control(pDX, IDC_RULESUFFIX, m_shRuleSuffix);
	DDX_Control(pDX, IDC_CONDITION_CHAR, m_shConditionC);
	DDX_Control(pDX, IDC_CONDITION_NUM, m_shConditionN);
	DDX_Control(pDX, IDC_GRAPH, m_shGraph);
	DDX_Control(pDX, IDC_UNITS, m_shUnits);
	DDX_Control(pDX, IDC_BUTTON_RULE_COLOR, m_btnColor);
	DDX_Text(pDX, IDC_STATIC_PRODUCTNAME, m_csProductName);
	DDX_Check(pDX, IDC_CHECK_SHOWNUM, m_bShowNum);
	DDX_Check(pDX, IDC_CHECK_SHOWGRAPH, m_bShowGraph);
	DDX_Check(pDX, IDC_RADIO_JLEFT, m_bRadioJLeft);
	DDX_Check(pDX, IDC_RADIO_JCENTER, m_bRadioJCenter);
	DDX_Check(pDX, IDC_RADIO_JRIGHT, m_bRadioJRight);
	DDX_Check(pDX, IDC_CHECK_FILTER, m_bHighlightCondition);
	DDX_Text(pDX, IDC_NUMVALUE, m_dNumValue);
	DDV_MinMaxDouble(pDX, m_dNumValue, 0., 999999999999999.);
	DDX_Text(pDX, IDC_CHARVALUE, m_csCharValue);
	DDV_MaxChars(pDX, m_csCharValue, 255);
	DDX_Check(pDX, IDC_CHECK_FILTER2, m_bHighlightCondition2);
	DDX_Check(pDX, IDC_CHECK_SHOWGRAPH2, m_bShowGraph2);
	DDX_Text(pDX, IDC_DATEVALUE, m_nDateValue);
	DDX_Radio(pDX, IDC_RADIO_WITHIN, m_nDateStyle);
	DDX_Check(pDX, IDC_CHECK_IGNORE_CASE, m_bIgnoreCase);
	DDX_Check(pDX, IDC_CHECK_SHOWDATETEXT, m_bShowDateText);
	DDX_Radio(pDX, IDC_RADIO_COLVAL_VAL, m_nRadioColval);
	DDX_Text(pDX, IDC_STATIC_COLUMN, m_csColumnName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SVDColumnSelector, CDialog)
	//{{AFX_MSG_MAP(SVDColumnSelector)
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM( )
	ON_BN_CLICKED(IDC_BUTTON_RULE_COLOR, OnButtonRuleColor)
	ON_NOTIFY(NM_CLICK, IDC_LISTVIEW_COLUMN, OnClickListviewColumn)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTVIEW_COLUMN, OnItemchangedListviewColumn)
	ON_BN_CLICKED(IDC_CHECK_SHOWNUM, OnCheckShownum)
	ON_BN_CLICKED(IDC_CHECK_SHOWGRAPH, OnCheckShowgraph)
	ON_BN_CLICKED(IDC_RADIO_JCENTER, OnRadioJcenter)
	ON_BN_CLICKED(IDC_RADIO_JLEFT, OnRadioJleft)
	ON_BN_CLICKED(IDC_RADIO_JRIGHT, OnRadioJright)
	ON_BN_CLICKED(IDC_UNITS, OnUnits)
	ON_BN_CLICKED(IDC_GRAPH, OnGraph)
	ON_BN_CLICKED(IDC_CONDITION_NUM, OnConditionNum)
	ON_BN_CLICKED(IDC_CONDITION_CHAR, OnConditionChar)
	ON_BN_CLICKED(IDC_CHECK_FILTER, OnCheckFilter)
	ON_BN_CLICKED(IDC_CHECK_FILTER2, OnCheckFilter2)
	ON_BN_CLICKED(IDC_RULESUFFIX, OnRulesuffix)
	ON_EN_KILLFOCUS(IDC_NUMVALUE, OnKillfocusNumvalue)
	ON_EN_KILLFOCUS(IDC_CHARVALUE, OnKillfocusCharvalue)
	ON_EN_CHANGE(IDC_CHARVALUE, OnChangeCharvalue)
	ON_EN_CHANGE(IDC_NUMVALUE, OnChangeNumvalue)
	ON_BN_CLICKED(IDC_CHECK_SHOWGRAPH2, OnCheckShowgraph2)
	ON_BN_CLICKED(IDC_RADIO_WITHIN, OnRadioDate)
	ON_BN_CLICKED(IDC_WITHINDATE, OnWithindate)
	ON_BN_CLICKED(IDC_CURRENTDATE, OnCurrentdate)
	ON_EN_KILLFOCUS(IDC_DATEVALUE, OnKillfocusDatevalue)
	ON_EN_CHANGE(IDC_DATEVALUE, OnChangeDatevalue)
	ON_BN_CLICKED(IDC_RADIO_BOOLEAN_FALSE_CIRCLE, OnRadioBooleanFalseCircle)
	ON_BN_CLICKED(IDC_RADIO_BOOLEAN_FALSE_BLANK, OnRadioBooleanFalseBlank)
	ON_BN_CLICKED(IDC_RADIO_SUBCOLUMNS_3VALUES, OnRadioSubcolumns3values)
	ON_BN_CLICKED(IDC_RADIO_SUBCOLUMNS_3BARS, OnRadioSubcolumns3bars)
	ON_BN_CLICKED(IDC_RADIO_SUBCOLUMNS_1BAR, OnRadioSubcolumns1bar)
	ON_BN_CLICKED(IDC_CHECK_IGNORE_CASE, OnCheckIgnoreCase)
	ON_BN_CLICKED(IDC_CHECK_SHOWDATETEXT, OnCheckShowdatetext)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_RADIO_COLVAL_VAL, OnRadioColvalVal)
	ON_BN_CLICKED(IDC_RADIO_COLVAL_COL, OnRadioColvalCol)
	ON_BN_CLICKED(ID_HELP_CHAR_STD, OnHelpCharStd)
	ON_BN_CLICKED(ID_HELP_NUM_STD, OnHelpNumStd)
	ON_BN_CLICKED(ID_HELP_NUM_NOGRAPH, OnHelpNumNograph)
	ON_BN_CLICKED(ID_HELP_NUM_SUBCOLUMNS, OnHelpNumSubcolumns)
	ON_BN_CLICKED(ID_HELP_NUM_VALUECOLUMN, OnHelpNumValuecolumn)
	ON_BN_CLICKED(ID_HELP_DATE_STD, OnHelpDateStd)
	ON_BN_CLICKED(IDC_GRAPH2, OnGraph)
	ON_BN_CLICKED(IDC_RADIO_CURRENT, OnRadioDate)
	ON_BN_CLICKED(ID_HELP_BOOL_STD, OnHelpBoolStd)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_S_HL_CHANGED, OnClicked)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SVDColumnSelector message handlers

void SVDColumnSelector::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (nIDCtl == IDC_LISTVIEW_COLUMN)
	{
		// this is not used if the imagelist is attached to the clistctrl, the height is taken from the imagelist
		lpMeasureItemStruct->itemHeight = 16;
	}
}

void SVDColumnSelector::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	switch (nIDCtl)
	{
		case IDC_BUTTON_PICTURE_TRUE:
			{
				CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
				CRect cr;
				GetDlgItem(IDC_BUTTON_PICTURE_TRUE)->GetClientRect(cr);
				int nItem = GetSelectedItem();
				if (nItem < 0)
					break;
				SVColumn* pCol = (SVColumn*)&(m_pColumn->GetAt(nItem));
				CBrush brush(pCol->clr), *pbOld;	
				pbOld = pDC->SelectObject(&brush);
				pDC->Ellipse(cr);
				pDC->SelectObject(pbOld);
			}
			break;

		case IDC_BUTTON_RULE_COLOR:
			DrawButtonRuleColor(lpDrawItemStruct);
			break;
		case IDC_LISTVIEW_COLUMN:
			DrawListviewColumn(lpDrawItemStruct);
			break;
	}
}

void SVDColumnSelector::DrawButtonRuleColor( LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn* pCol = (SVColumn*)&(m_pColumn->GetAt(nItem));
	CRect crButton = lpDrawItemStruct->rcItem;
	crButton.DeflateRect(2,2);
	pDC->FillRect(crButton, &CBrush(pCol->clrRule[COLORRULE0]));
	CBrush br(RGB(0, 0, 0));		// draw a black border around it
	pDC->FrameRect(crButton, &br);	// (same)
}

void SVDColumnSelector::DrawListviewColumn( LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CString csText;
	CRect crIcon;
	m_pLCColumns->GetItemRect(lpDrawItemStruct->itemID, crIcon, LVIR_ICON);
	CRect crCell;
	m_pLCColumns->GetItemRect(lpDrawItemStruct->itemID,crCell, LVIR_LABEL);
	CRect crAll;
	m_pLCColumns->GetItemRect(lpDrawItemStruct->itemID,crAll, LVIR_BOUNDS);

	
	static _TCHAR szBuff[MAX_PATH];
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = lpDrawItemStruct->itemID;
	lvi.iSubItem = 0;
	lvi.pszText = szBuff;
	lvi.cchTextMax = sizeof(szBuff);
	lvi.stateMask = 0xFFFF;		
	m_pLCColumns->GetItem(&lvi);	

	bool bSelected = false;
	if (lpDrawItemStruct->itemState & ODS_SELECTED)
		bSelected = true;

	SVColumn  svCol = (m_pColumn->GetAt(lpDrawItemStruct->itemData));


	if (bSelected)
		pDC->FillRect(crAll,&CBrush(m_clrSelectedLine)); 
	else 
		pDC->FillRect(crAll,&CBrush(m_clrNotSelectedLine));

// display checkbox
	crIcon.DeflateRect(6,6);
	CopyCheckBox(pDC, &crIcon, (svCol.cShowStatus == SHOW));

	crCell.DeflateRect(1,1);

	pDC->DrawText(svCol.csHeaderText, -1, crCell,
		DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | 
		DT_MODIFYSTRING );// |DT_END_ELLIPSIS );

	// fill in datatype column
	m_pLCColumns->GetSubItemRect(lpDrawItemStruct->itemID, COL_TYPE, LVIR_LABEL, crCell);
	if (bSelected)
		pDC->FillRect(crCell,&CBrush(m_clrSelectedLine));
	else 
		pDC->FillRect(crCell,&CBrush(m_clrNotSelectedLine));
	
	if (svCol.ct >= COLTYPEDATE)
		csText = RESCHAR(IDS_V_COLTYPE_DATETIME);
	else if (svCol.ct >= COLTYPENUM)
		csText = RESCHAR(IDS_V_COLTYPE_NUM);
	else if (svCol.ct >= COLTYPECHAR)
		csText = RESCHAR(IDS_V_COLTYPE_CHAR);
	else if (svCol.ct == ctBoolean)
		csText = RESCHAR(IDS_V_COLTYPE_BOOLEAN);
	pDC->DrawText(csText, -1, crCell,
		DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
	
	// draw the primary color cell
	if (svCol.sc == scNone){
		m_pLCColumns->GetSubItemRect(lpDrawItemStruct->itemID, COL_COLOR, LVIR_LABEL, crCell);
	if (bSelected)
		pDC->FillRect(crCell,&CBrush(m_clrSelectedLine));
	else 
		pDC->FillRect(crCell,&CBrush(m_clrNotSelectedLine));
	crCell.DeflateRect(2,2);
	crCell.left = crCell.left + (crCell.Width()/2) - (crCell.Height()/2);
	crCell.right = crCell.left + crCell.Height();
	pDC->FillRect(crCell,&CBrush(svCol.clr));			
	}
}

void SVDColumnSelector::OnButtonRuleColor() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = m_pColumn->GetAt(nItem);
	if (!svCol.IsRule())
		return;
	COLORREF clrNew = AskColor(svCol.clrRule[COLORRULE0]);
	if (clrNew != -1)
	{
		svCol.clrRule[COLORRULE0] = clrNew;
		m_pColumn->SetAt(nItem, svCol);
		RedrawControl(IDC_BUTTON_RULE_COLOR);
		m_bDirty = true;
	}
}


void SVDColumnSelector::RedrawControl(int nCtlID)
{
	RECT rect;
	CWnd* pw = GetDlgItem(nCtlID);
	pw->GetClientRect( &rect);
	pw->ClientToScreen( &rect);
	ScreenToClient( &rect);
	RedrawWindow();
}

void SVDColumnSelector::AddRow(int nCol)
{
	LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT; 
	lvi.iIndent = 0;
    lvi.iSubItem = 0; 
	lvi.iImage = 0; 
    lvi.pszText = LPSTR_TEXTCALLBACK; 
	lvi.iItem = m_pLCColumns->GetItemCount()+1;  
	lvi.lParam = (LPARAM) nCol;
	m_pLCColumns->InsertItem (&lvi);
}

void SVDColumnSelector::AddRows()
{
	for (int i=0; i<m_pColumn->GetSize(); i++)
		AddRow(i);
}

BOOL SVDColumnSelector::OnInitDialog() 
{
	m_csProductName = RESCHAR(IDS_V_PRODUCTNAME);
	m_pLCColumns = (CListCtrl*)GetDlgItem(IDC_LISTVIEW_COLUMN);

	CDialog::OnInitDialog();
	InitCheckBox();
	AddRows(); 

	m_pImage = new CImageList();
	m_pImage->Create (24, 24, TRUE, 2, 0);

	// Associate CImageList with CListCtrl.
    m_pLCColumns->SetImageList (m_pImage, LVSIL_SMALL);

	RECT rect;
	m_pLCColumns->GetClientRect(&rect);
	int nWidth = rect.right / 4;

	CString csHeader = RESCHAR(IDS_V_SHOWOPTION);
	csHeader += "       ";
	csHeader +=  RESCHAR(IDS_V_COLNAME);

	m_pLCColumns->InsertColumn(COL_NAME, csHeader, LVCFMT_CENTER, nWidth * 2);
	m_pLCColumns->InsertColumn(COL_TYPE, RESCHAR(IDS_V_COLTYPE), LVCFMT_LEFT, nWidth);
	m_pLCColumns->InsertColumn(COL_COLOR, RESCHAR(IDS_V_COLCOLOR), LVCFMT_CENTER, nWidth);

	if (m_nStartColumn >= 0)
	{
		m_pLCColumns->SetItemState(m_nStartColumn,LVIS_SELECTED, LVIS_SELECTED);
		m_pLCColumns->EnsureVisible(m_nStartColumn, FALSE);
	}
	SetControls(m_nStartColumn);		// this is -1 if not starting at a particular column
	CWnd* pw = GetDlgItem(IDC_LISTVIEW_COLUMN);
	pw->SetFocus();
	
	SetUnitTypes();
	SetConditionN();
	SetConditionC();
	SetRuleSuffix();
	SetGraph();
	SetDate();
	SetConditionFromColumn();

	// move boolean controls to position
	// get x and y anchor
	CRect crBase, crBase1;
	GetDlgItem(IDC_CHECK_SHOWGRAPH2)->GetWindowRect(crBase);
	GetDlgItem(IDC_RADIO_JLEFT)->GetWindowRect(crBase1);
	CRect crGroupBase;
	GetDlgItem(IDC_STATIC_BOOLEAN_GROUP)->GetWindowRect(crGroupBase);
	int nOffsetX = crBase1.left - crGroupBase.left;
	int nOffsetY = crBase1.top - crGroupBase.top;

	CRect cr;

	GetDlgItem(IDC_STATIC_BOOLEAN_GROUP)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_BOOLEAN_GROUP)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_BOOLEAN_TRUE)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_BOOLEAN_TRUE)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_BOOLEAN_FALSE)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_BOOLEAN_FALSE)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_BOOLEAN_FALSEBLANK)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_BOOLEAN_FALSEBLANK)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_BUTTON_PICTURE_TRUE)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_BUTTON_PICTURE_TRUE)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_BLANK)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_BLANK)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	HICON hIcon;

	hIcon = AfxGetApp()->LoadIcon(IDI_V_FALSE);
	if (hIcon)
	{
		((CButton*)GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE))->ModifyStyle(0, BS_ICON);
		((CButton*)GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE))->SetIcon(hIcon);
	}
// subcolumn icons

	m_hIcon3Values = (HICON)LoadImage(AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDI_V_SC3VALUES),
			IMAGE_ICON,
			32,8, 
			0);
	if (m_hIcon3Values)
	{
		((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES))->ModifyStyle(0, BS_ICON);
		((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES))->SetIcon(m_hIcon3Values);
	}

	m_hIcon3Bars = (HICON)LoadImage(AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDI_V_SC3BARS),
			IMAGE_ICON,
			32,8, 
			0);
	if (m_hIcon3Bars)
	{
		((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS))->ModifyStyle(0, BS_ICON);
		((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS))->SetIcon(m_hIcon3Bars);
	}

	m_hIcon1Bar = (HICON)LoadImage(AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDI_V_SC1BAR),
			IMAGE_ICON,
			32,8, 
			0);
	if (m_hIcon1Bar)
	{
		((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR))->ModifyStyle(0, BS_ICON);
		((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR))->SetIcon(m_hIcon1Bar);
	}

	// now move subcolumns to position
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_GROUP)->GetWindowRect(crGroupBase);
	nOffsetX = crBase.left - crGroupBase.left;
	nOffsetY = crBase.top - crGroupBase.top;

	GetDlgItem(IDC_STATIC_SUBCOLUMNS_GROUP)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_GROUP)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_SUBCOLUMNS)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_SUBCOLUMNS_3VALUES)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_3VALUES)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_SUBCOLUMNS_3BARS)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_3BARS)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	GetDlgItem(IDC_STATIC_SUBCOLUMNS_1BAR)->GetWindowRect(cr);ScreenToClient(cr);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_1BAR)->SetWindowPos(0, cr.left+nOffsetX, cr.top+nOffsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL SVDColumnSelector::DestroyWindow() 
{
	delete m_pImage;
	TermCheckBox();
	return CDialog::DestroyWindow();
}

void SVDColumnSelector::InitCheckBox()
{
	m_pbmCheckOn = new CBitmap;
	m_pbmCheckOn->LoadBitmap(IDB_V_CHECKOFF);
	m_pbmCheckOn->GetBitmap(&m_bmpInfoCheckOn);	// Get the size of the bitmap

	m_pbmCheckOff = new CBitmap;
	m_pbmCheckOff->LoadBitmap(IDB_V_CHECKON);
	m_pbmCheckOff->GetBitmap(&m_bmpInfoCheckOff);	// Get the size of the bitmap
}

void SVDColumnSelector::TermCheckBox()
{
	delete m_pbmCheckOn;
	delete m_pbmCheckOff;
}

bool SVDColumnSelector::CopyCheckBox(CDC* pToDC, CRect* pcrTo, int nImage)
{
// nImage =	0 = CheckOff
//			1 = CheckOn

    CDC dcMemory;
    dcMemory.CreateCompatibleDC(pToDC);

    // Select the bitmap into the in-memory DC
    CBitmap* pOldBitmap = dcMemory.SelectObject((nImage != 0) ? m_pbmCheckOff : m_pbmCheckOn);

    // Copy the bits from the in-memory DC into the on-screen DC 

	return (pToDC->StretchBlt(pcrTo->left, pcrTo->top, pcrTo->Width(), pcrTo->Height(), 
						&dcMemory,0, 0, m_bmpInfoCheckOn.bmWidth, m_bmpInfoCheckOn.bmHeight,
						SRCCOPY) == TRUE);
}

void SVDColumnSelector::OnClickListviewColumn(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	LVHITTESTINFO  lvhit;
	lvhit.pt = pNMListView->ptAction;

	int nItem = m_pLCColumns->SubItemHitTest(&lvhit);

	CRect crIcon;
		
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		m_pLCColumns->GetItemRect(nItem, crIcon, LVIR_ICON);
		if (crIcon.PtInRect(pNMListView->ptAction))
		{
			SVColumn  svCol = (m_pColumn->GetAt(nItem));
			if (svCol.cShowStatus == SHOW)
				svCol.cShowStatus = HIDE;
			else
				svCol.cShowStatus = SHOW;

			svCol.bColumnWidthDirty = true;
			m_pColumn->SetAt(nItem,svCol);
			RedrawControl(IDC_LISTVIEW_COLUMN);
			m_bDirty = true;
		}
		else if ((svCol.sc == scNone) && (lvhit.iSubItem == COL_COLOR))
		{
			COLORREF clrNew = AskColor(svCol.clr);
			if (clrNew != -1)
			{
				svCol.clr = clrNew;
				m_pColumn->SetAt(nItem,svCol);
				RedrawControl(IDC_LISTVIEW_COLUMN);
				m_bDirty = true;
			}
		}
	}
	*pResult = 0;
}

void SVDColumnSelector::SetControls(int nItem)
{
	CString cs, cs1;
	CWnd* pw;
	m_nSelectedItem = nItem;

	SetCheckBoxes();

	pw = GetDlgItem(IDC_STATIC_HEADER);
	pw->SetWindowText(RESCHAR(IDS_V_NOSELECTEDCOLUMN));
	GetDlgItem(IDC_STATIC_HEADER)->ShowWindow(SW_HIDE);	
	GetDlgItem(IDC_STATIC_NOCOLUMN_NOTE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_FILTER)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_FILTER2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_FILTER)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_FILTER2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_SHOWNUM)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_SHOWGRAPH)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_SHOWGRAPH2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_UNITS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_UNITS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_GRAPH)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_GRAPH2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_GRAPH)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_GRAPH2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_JLEFT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_JCENTER)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_JRIGHT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CONDITION_NUM)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CONDITION_CHAR)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHARVALUE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_IGNORE_CASE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RULESUFFIX)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_SETCOLOR)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_DATEVALUE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_WITHIN)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_CURRENT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_WITHINDATE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CURRENTDATE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_SHOWDATETEXT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_COLVAL_VAL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_COLVAL_COL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_COLUMN)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_FILLER)->ShowWindow(SW_HIDE);

// boolean 
	GetDlgItem(IDC_STATIC_BOOLEAN_GROUP)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_BOOLEAN_TRUE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_BOOLEAN_FALSE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_BOOLEAN_FALSEBLANK)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_BLANK)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_PICTURE_TRUE)->ShowWindow(SW_HIDE);

//subcolumns
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_GROUP)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_3VALUES)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_3BARS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_SUBCOLUMNS_1BAR)->ShowWindow(SW_HIDE);

//help buttons
	GetDlgItem(ID_HELP_CHAR_STD)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_HELP_NUM_STD)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_HELP_NUM_NOGRAPH)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_HELP_NUM_SUBCOLUMNS)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_HELP_NUM_VALUECOLUMN)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_HELP_DATE_STD)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_HELP_BOOL_STD)->ShowWindow(SW_HIDE);

	if (nItem < 0)
	{
		GetDlgItem(IDC_STATIC_NOCOLUMN_NOTE)->ShowWindow(SW_SHOW);
		return;
	}
	SVColumn* pCol = (SVColumn*)&(m_pColumn->GetAt(nItem));

	if (pCol->IsDate())
	{
		AddJustify(pCol);

		pw = GetDlgItem(IDC_STATIC_FILTER2);
		cs = RESCHAR(IDS_V_IFTEMPLATE);
		cs1.Format(cs, pCol->csHeaderText);
		pw->SetWindowText(cs1);
		pw->ShowWindow(SW_SHOW);

		GetDlgItem(IDC_CHECK_FILTER2)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_GRAPH2)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_CHECK_SHOWGRAPH2)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_GRAPH2)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_CHECK_SHOWDATETEXT)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_DATEVALUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_WITHIN)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_CURRENT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_WITHINDATE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_CURRENTDATE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SETCOLOR)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_SHOW);
		RedrawControl(IDC_BUTTON_RULE_COLOR);
		GetDlgItem(ID_HELP)->ShowWindow(SW_HIDE);
		GetDlgItem(ID_HELP_DATE_STD)->ShowWindow(SW_SHOW);
	}
	else if(pCol->sc != scNone)
	{
		GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SUBCOLUMNS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SUBCOLUMNS_3VALUES)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SUBCOLUMNS_3BARS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SUBCOLUMNS_1BAR)->ShowWindow(SW_SHOW);
		cs.Format(RESCHAR(IDS_V_COLUMNNAME), pCol->csHeaderText);
		pw = GetDlgItem(IDC_STATIC_HEADER);
		pw->SetWindowText(cs);
		pw->ModifyStyle(SS_LEFT | SS_RIGHT | SS_CENTER, SS_CENTER);
		pw->ShowWindow(SW_SHOW);
		GetDlgItem(ID_HELP)->ShowWindow(SW_HIDE);
		GetDlgItem(ID_HELP_NUM_SUBCOLUMNS)->ShowWindow(SW_SHOW);
	}
	else if (pCol->IsNumber())
	{

		AddJustify(pCol);

		pw = GetDlgItem(IDC_STATIC_FILTER);
		cs = RESCHAR(IDS_V_IFTEMPLATE);
		cs1.Format(cs, pCol->csHeaderText);
		pw->SetWindowText(cs1);
		pw->ShowWindow(SW_SHOW);

		GetDlgItem(IDC_CHECK_FILTER)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_CHECK_SHOWNUM)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_UNITS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_UNITS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_GRAPH)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_CHECK_SHOWGRAPH)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_GRAPH)->ShowWindow(pCol->bGraphable ? SW_SHOW : SW_HIDE);
		GetDlgItem(IDC_CONDITION_NUM)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RULESUFFIX)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SETCOLOR)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_SHOW);
		RedrawControl(IDC_BUTTON_RULE_COLOR);
		GetDlgItem(ID_HELP)->ShowWindow(SW_HIDE);
		if (pCol->bGraphable)
			GetDlgItem(ID_HELP_NUM_STD)->ShowWindow(SW_SHOW);
		else
			GetDlgItem(ID_HELP_NUM_NOGRAPH)->ShowWindow(SW_SHOW);
	}
	else if (pCol->IsCharacter())
	{
		AddJustify(pCol);

		pw = GetDlgItem(IDC_STATIC_FILTER);
		cs = RESCHAR(IDS_V_IFTEMPLATE);
		cs1.Format(cs, pCol->csHeaderText);
		pw->SetWindowText(cs1);
		pw->ShowWindow(SW_SHOW);

		GetDlgItem(IDC_CHECK_FILTER)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_CONDITION_CHAR)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_CHARVALUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_CHECK_IGNORE_CASE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_SETCOLOR)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_SHOW);
		RedrawControl(IDC_BUTTON_RULE_COLOR);
		GetDlgItem(ID_HELP)->ShowWindow(SW_HIDE);
		GetDlgItem(ID_HELP_CHAR_STD)->ShowWindow(SW_SHOW);
	}
	else if (pCol->ct == ctBoolean)
	{
		GetDlgItem(IDC_STATIC_BOOLEAN_TRUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_BOOLEAN_FALSE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_BOOLEAN_FALSEBLANK)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_BLANK)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_PICTURE_TRUE)->ShowWindow(SW_SHOW);
		cs.Format(RESCHAR(IDS_V_COLUMNNAME), pCol->csHeaderText);
		pw = GetDlgItem(IDC_STATIC_HEADER);
		pw->SetWindowText(cs);
		pw->ModifyStyle(SS_LEFT | SS_RIGHT | SS_CENTER, SS_CENTER);
		pw->ShowWindow(SW_SHOW);
		GetDlgItem(ID_HELP)->ShowWindow(SW_HIDE);
		GetDlgItem(ID_HELP_BOOL_STD)->ShowWindow(SW_SHOW);
	}
	else		// data type invalid / unaccounted for
	{
		CString cs;
		cs = RESCHAR(IDS_V_UNKNOWNDATATYPE);
		AfxMessageBox(cs,MB_ICONSTOP);
	}	
}

void SVDColumnSelector::OnItemchangedListviewColumn(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// user selected a new item on the column list (either with a mouse click or a arrow move
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (pNMListView->uNewState & ODS_SELECTED)
	{
		SetControls(pNMListView->iItem);
		SetConditionFromColumn();
		SetUnitFromColumn();
		SetGraphFromColumn();
	}
	*pResult = 0;
}

COLORREF SVDColumnSelector::AskColor(COLORREF clr)
{
	CColorDialog cdg(clr, CC_SOLIDCOLOR);
	int nButton = cdg.DoModal();
	if (nButton == IDOK)
		if (clr != cdg.GetColor())
			return cdg.GetColor();
	return -1; 
}

void SVDColumnSelector::OnCheckShownum() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn svCol = m_pColumn->GetAt(nItem);
	if (!svCol.IsNumber())
		return;
	UpdateData(TRUE);		// get values from dialog
	svCol.bText = (m_bShowNum == TRUE);
	m_pColumn->SetAt(nItem, svCol);
	GetDlgItem(IDC_STATIC_UNITS)->EnableWindow(m_bShowNum);
	GetDlgItem(IDC_UNITS)->EnableWindow(m_bShowNum);
	m_shUnits.Refresh();
}

void SVDColumnSelector::OnCheckShowgraph() 
{
	int nItem = GetSelectedItem();	// get the column row
	if (nItem < 0)
		return;
	UpdateData(TRUE);		// get the value
	SVColumn svCol = m_pColumn->GetAt(nItem);
	if (svCol.IsNumber())
	{
		m_shGraph.SetCurSel(ColumnGraphToItem(svCol.gdt));	//update selection
		m_nGraphResourceID = m_shGraph.GetCurSel();
		svCol.bGraphic = m_bShowGraph ? true : false;
		GetDlgItem(IDC_STATIC_GRAPH)->EnableWindow(m_bShowGraph);
		GetDlgItem(IDC_GRAPH)->EnableWindow(m_bShowGraph);
		m_shGraph.Refresh();
		svCol.gdt = SelectedGraphToColumn(m_nGraphResourceID, true); 
	}
	if (svCol.IsDate())
	{
		m_shGraph2.SetCurSel(ColumnGraphToItem(svCol.gdt));	//update selection
		m_nGraphResourceID = m_shGraph2.GetCurSel();
		svCol.bGraphic = m_bShowGraph2 ? true : false;
		GetDlgItem(IDC_STATIC_GRAPH2)->EnableWindow(m_bShowGraph2);
		GetDlgItem(IDC_GRAPH2)->EnableWindow(m_bShowGraph2);
		m_shGraph2.Refresh();
		svCol.gdt = SelectedGraphToColumn(m_nGraphResourceID, false);
	}
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::SetCheckBoxes()
{
// this sets checkboxes and radio buttons from column values

	m_bRadioJLeft = m_bRadioJCenter = m_bRadioJRight = FALSE;		
	m_bShowNum = m_bShowGraph = m_bShowGraph2 = FALSE;
	m_bHighlightCondition = m_bHighlightCondition2 = FALSE;
	m_nDateStyle = 1;	// set date radio to current
	int nItem;
	SVColumn* pCol;
	nItem = GetSelectedItem();

	if (nItem >= 0)
	{
		pCol = (SVColumn*)&(m_pColumn->GetAt(nItem));

		// IDC_CHECK_SHOWNUM
		if (pCol->IsNumber())
			m_bShowNum = pCol->bText;

		// IDC_CHECK_SHOWGRAPH/2
		if (pCol->IsGraph())
			m_bShowGraph = m_bShowGraph2 = TRUE;

		// DC_CHECK_FILTER/2
		if (pCol->IsRule())
			m_bHighlightCondition = m_bHighlightCondition2 = TRUE;

		// IDC_RADIO_Jxxxxx
		if (pCol->nJustify == DT_LEFT)
			m_bRadioJLeft = TRUE;
		else if (pCol->nJustify == DT_CENTER)
			m_bRadioJCenter = TRUE;
		else if (pCol->nJustify == DT_RIGHT)
			m_bRadioJRight = TRUE;

		// IDC_CHECK_IGNORE_CASE
		m_bIgnoreCase = pCol->bIgnoreCase ? TRUE : FALSE;

		// IDC_RADIO_COLVAL_VAL, IDC_RADIO_COLVAL_COL
		m_nRadioColval = (
			(pCol->rt == rtEqualColumn) ||
			(pCol->rt == rtLowColumn) ||
			(pCol->rt == rtHighColumn)
			) ? COLUMN_ON : VALUE_ON;
	}
	UpdateData(FALSE);
}

void SVDColumnSelector::OnRadioJcenter() 
{
	SetJustifyButton(DT_CENTER, SS_CENTER); 				
}

void SVDColumnSelector::OnRadioJleft() 
{
	SetJustifyButton(DT_LEFT, SS_LEFT); 
}

void SVDColumnSelector::OnRadioJright() 
{
	SetJustifyButton(DT_RIGHT, SS_RIGHT); 
}

void SVDColumnSelector::SetJustifyButton(int nJustify, int nTextFormat)
{
	int nItem = GetSelectedItem();
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		svCol.nJustify = nJustify;
		m_pColumn->SetAt(nItem, svCol);

		// Set the text to the corresponding position
		CWnd* pw = GetDlgItem(IDC_STATIC_HEADER);
		pw->ModifyStyle(SS_LEFT | SS_RIGHT | SS_CENTER, nTextFormat, 0);
		pw->RedrawWindow();

		m_bDirty = true;
	}
}

// following #defines must be in synch with SetUnitTypes order
#define UTNONE	0
#define UTKILO	1
#define UTMEGA	2
#define UTGIGA	3
#define UTTERA	4
#define UTMIXED	5
void SVDColumnSelector::SetUnitTypes() 
{
	m_shUnits.AddString(RESCHAR(IDS_V_UTNONE));
	m_shUnits.AddString(RESCHAR(IDS_V_UTKILO));
	m_shUnits.AddString(RESCHAR(IDS_V_UTMEGA));
	m_shUnits.AddString(RESCHAR(IDS_V_UTGIGA));
	m_shUnits.AddString(RESCHAR(IDS_V_UTTERA));
	m_shUnits.AddString(RESCHAR(IDS_V_UTMIXED));

	SetUnitFromColumn();
}

int SVDColumnSelector::ColumnUnitToItem(_UnitType ut)
{
	switch (ut)
	{
		case utNone:
			return UTNONE;
		case utKilo:
			return UTKILO;
		case utMega:
			return UTMEGA;
		case utGiga:
			return UTGIGA;
		case utTera:
			return UTTERA;
		case utMixed:
			return UTMIXED;
	}
	return UTMIXED;
}

_UnitType SVDColumnSelector::SelectedUnitToColumn(long nID)
{
	switch (nID)
	{
		case UTNONE:
			return utNone;
		case UTKILO:
			return utKilo;
		case UTMEGA:
			return utMega;
		case UTGIGA:
			return utGiga;
		case UTTERA:
			return utTera;
		case UTMIXED:
			return utMixed;
	}
	return utMixed;
}

void SVDColumnSelector::OnClicked(WPARAM wParam, LPARAM lParam) 
{
	switch (m_nClickedID)
	{
		case IDC_UNITS:
			OnUnitTypeSelected();
			break;

		case IDC_GRAPH:
			OnGraphStyleSelected();
			break;

		case IDC_CONDITION_NUM:
			OnConditionNumSelected();
			break;

		case IDC_RULESUFFIX:
			OnRuleSuffixSelected();
			break;

		case IDC_CONDITION_CHAR:
			OnConditionCharSelected();
			break;

		case IDC_WITHINDATE:
			OnWithinDateSelected();
			break;

		case IDC_CURRENTDATE:
			OnCurrentDateSelected();
			break;
	}
}
void SVDColumnSelector::OnUnitTypeSelected() 
{
	int nItem = GetSelectedItem();	// get the column
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		svCol.ut = SelectedUnitToColumn(m_shUnits.GetCurSel());
		m_pColumn->SetAt(nItem, svCol);
	}
}
void SVDColumnSelector::SetUnitFromColumn() 
{
	int nSel = UTMIXED;				// default
	int nItem = GetSelectedItem();	// get the column, if one selected
	if (nItem >= 0)					// if column selected
	{
		SVColumn svCol = (m_pColumn->GetAt(nItem));
		if (svCol.IsNumber())
		{
			nSel = ColumnUnitToItem(svCol.ut);
			m_bShowNum = svCol.bText;
			GetDlgItem(IDC_STATIC_UNITS)->EnableWindow((m_bShowNum) ? TRUE : FALSE);
			GetDlgItem(IDC_UNITS)->EnableWindow((m_bShowNum) ? TRUE : FALSE);
		}
	}
	m_shUnits.SetCurSel(nSel);		// set selection
}

void SVDColumnSelector::OnUnits() 
{
	m_nClickedID = IDC_UNITS;
}
void SVDColumnSelector::OnGraph() 
{
	m_nClickedID = IDC_GRAPH;
}

// following #defines must be in order added in SetGraphFromColumn
#define SCATTER			0	// number and date
#define CIRCLETRUE		1	// date only
#define CIRCLEFALSE		2	// date only
#define BAR				1	// number only
#define BARINFRAME		2	// number only
void SVDColumnSelector::SetGraph()
{
	m_shGraph.AddString(RESCHAR(IDS_V_GRAPH_SCATTER));
	m_shGraph.AddString(RESCHAR(IDS_V_GRAPH_BAR));
	m_shGraph.AddString(RESCHAR(IDS_V_GRAPH_BARINFRAME));
	m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_SCATTER));
	m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_CIRCLETRUE)); //date
	m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_CIRCLEFALSE));	//date 
	
	SetGraphFromColumn();
}

void SVDColumnSelector::SetGraphFromColumn()
{
	int nItem = GetSelectedItem();	// get the column, if one selected
	if (nItem >= 0)
	{
		SVColumn svCol = (m_pColumn->GetAt(nItem));
		m_bShowGraph2 = m_bShowGraph = (svCol.IsGraph())? TRUE : FALSE;
		int nSel = ColumnGraphToItem(svCol.gdt);
		if (svCol.IsNumber())
		{
			GetDlgItem(IDC_STATIC_GRAPH)->EnableWindow((m_bShowGraph) ? TRUE : FALSE);
			GetDlgItem(IDC_GRAPH)->EnableWindow((m_bShowGraph) ? TRUE : FALSE);
			m_shGraph.SetCurSel(nSel);		// set selection
		}
		if (svCol.IsDate())
		{
			GetDlgItem(IDC_STATIC_GRAPH2)->EnableWindow((m_bShowGraph2) ? TRUE : FALSE);
			GetDlgItem(IDC_GRAPH2)->EnableWindow((m_bShowGraph2) ? TRUE : FALSE);
			m_bShowDateText = svCol.bText ? TRUE : FALSE;
			m_shGraph2.ResetContent();
			if (svCol.bRule)
			{
				m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_SCATTER));
				m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_CIRCLETRUE));
				m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_CIRCLEFALSE));
				svCol.gdt = SelectedGraphToColumn(nSel, false);
			}
			else
			{	// rule not tested so graphic must only allow scatterplot
				m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_SCATTER));
				nSel = SCATTER;						// force selection reset
				svCol.gdt = SelectedGraphToColumn(SCATTER, false);
			}
			m_pColumn->SetAt(nItem, svCol);			// make sure gdt is reset
			m_shGraph2.SetCurSel(nSel);				// set selection
		}

		UpdateData(FALSE);
	}
}

_GraphicalDisplayType SVDColumnSelector::SelectedGraphToColumn(long nID, bool bNumber)
{
	if (bNumber)
	{
		switch (nID)
		{
		case SCATTER:
			return gdtScatterPlot;
		case BAR:
			return gdtBar;
		case BARINFRAME:
			return gdtBarInFrame;
		}	
	}
	else
	{
		switch (nID)
		{
		case SCATTER:
			return gdtScatterPlot;
		case CIRCLETRUE:
			return gdtCircleWhenTrue;
		case CIRCLEFALSE:
			return gdtCircleWhenFalse;
		}
	}
	return gdtScatterPlot;
}

int SVDColumnSelector::ColumnGraphToItem(_GraphicalDisplayType gdt)
{
	switch (gdt)
	{
		case gdtScatterPlot:
			return SCATTER;
		case gdtBar:
			return BAR;
		case gdtBarInFrame:
			return BARINFRAME;
		case gdtCircleWhenTrue:
			return CIRCLETRUE;
		case gdtCircleWhenFalse:
			return CIRCLEFALSE;
	}
	return SCATTER;
}

void SVDColumnSelector::OnGraphStyleSelected() 
{
	int nItem = GetSelectedItem();	// get the column
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		if (svCol.IsNumber())
			svCol.gdt = SelectedGraphToColumn(m_shGraph.GetCurSel(), true);
		if (svCol.IsDate())
			svCol.gdt = SelectedGraphToColumn(m_shGraph2.GetCurSel(), false);
		m_pColumn->SetAt(nItem, svCol);
	}
}
// keep #defines in same order as set below
#define EQUAL_N		0
#define LOW_N		1
#define HIGH_N		2
void SVDColumnSelector::SetConditionN()
{
	m_shConditionN.AddString(RESCHAR(IDS_V_EQUALS));
	m_shConditionN.AddString(RESCHAR(IDS_V_LESSTHAN));
	m_shConditionN.AddString(RESCHAR(IDS_V_GREATERTHAN));
	m_shConditionN.SetWindowText(RESCHAR(IDS_V_NORULEDEFINED));
	m_shConditionN.Refresh();
}

// keep #defines in same order as set below
#define NONE_N	0
#define KB_N	1
#define MB_N	2
#define GB_N	3
#define TB_N	4
void SVDColumnSelector::SetRuleSuffix()
{
	m_shRuleSuffix.AddString(_T("..."));
	m_shRuleSuffix.AddString(_T("KB"));
	m_shRuleSuffix.AddString(_T("MB"));
	m_shRuleSuffix.AddString(_T("GB"));
	m_shRuleSuffix.AddString(_T("TB"));

	SetRuleSuffixFromColumn();
}

// keep #defines in same order as set below
#define EQUAL_C		0
#define NOTEQUAL_C	1
#define LIKE_C		2
#define NOTLIKE_C	3
void SVDColumnSelector::SetConditionC()
{
	m_shConditionC.AddString(RESCHAR(IDS_V_EQUALS));
	m_shConditionC.AddString(RESCHAR(IDS_V_NOTEQUALS));
	m_shConditionC.AddString(RESCHAR(IDS_V_LIKE));
	m_shConditionC.AddString(RESCHAR(IDS_V_NOTLIKE));
	m_shConditionC.SetWindowText(RESCHAR(IDS_V_NORULEDEFINED));
	m_shConditionC.Refresh();
}

void SVDColumnSelector::OnConditionNum() 
{
	m_nClickedID = IDC_CONDITION_NUM;
}

void SVDColumnSelector::OnConditionChar() 
{
	m_nClickedID = IDC_CONDITION_CHAR;
}
void SVDColumnSelector::OnConditionNumSelected()
{
	// get current rule, if any
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	UpdateData(TRUE);
	SVColumn svCol = m_pColumn->GetAt(nItem);
	svCol.rt = SelectedRuleNumToColumn(m_shConditionN.GetCurSel());
	svCol.rs = SelectedRuleSuffixToColumn(m_shRuleSuffix.GetCurSel());
	svCol.dRuleValue = m_dNumValue * RuleSuffixToMultiple(svCol.rs);
	m_pColumn->SetAt(nItem, svCol);
	if (svCol.rt != rtNone)
	{
		GetDlgItem(IDC_NUMVALUE)->EnableWindow(TRUE);
		GetDlgItem(IDC_RULESUFFIX)->EnableWindow(TRUE);
	}
	GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_SHOW);
	RedrawControl(IDC_BUTTON_RULE_COLOR);
	GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(TRUE);
}

void SVDColumnSelector::OnConditionCharSelected()
{
	// get current rule, if any
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	UpdateData(TRUE);
	SVColumn svCol = m_pColumn->GetAt(nItem);
	svCol.csRuleValue = m_csCharValue;
	svCol.rt = SelectedRuleCharToColumn(m_shConditionC.GetCurSel());
	m_pColumn->SetAt(nItem, svCol);
	if (svCol.rt != rtNone)
		GetDlgItem(IDC_CHARVALUE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_SHOW);
	RedrawControl(IDC_BUTTON_RULE_COLOR);
	GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(TRUE);
}

_RuleType SVDColumnSelector::SelectedRuleCharToColumn(long nID)
{
	switch (nID)
	{
		case EQUAL_C:
			return rtEquals;
		case NOTEQUAL_C:
			return rtNotEquals;
		case LIKE_C:
			return rtLike;
		case NOTLIKE_C:
			return rtNotLike;
	}
	return rtNone;
}

int SVDColumnSelector::ColumnRuleCharToItem(_RuleType rt)
{
	switch (rt)
	{
		case rtEquals:
			return EQUAL_C;
		case rtNotEquals:
			return NOTEQUAL_C;
		case rtLike:
			return LIKE_C;
		case rtNotLike:
			return NOTLIKE_C;
	}
	return UNKNOWN;
}

_RuleType SVDColumnSelector::SelectedRuleNumToColumn(long nID)
{
	if (m_nRadioColval == COLUMN_ON)
		switch (nID)
		{
			case EQUAL_N:
				return rtEqualColumn;
			case LOW_N:
				return rtLowColumn;
			case HIGH_N:
				return rtHighColumn;
		}
	else
		switch (nID)
		{
			case EQUAL_N:
				return rtEquals;
			case LOW_N:
				return rtLow;
			case HIGH_N:
				return rtHigh;
		}
	return rtNone;
}

int SVDColumnSelector::ColumnRuleNumToItem(_RuleType rt)
{
	switch (rt)
	{
		case rtEquals:
			return EQUAL_N;
		case rtLow:
			return LOW_N;
		case rtHigh:
			return HIGH_N;
		case rtEqualColumn:
			return EQUAL_N;
		case rtLowColumn:
			return LOW_N;
		case rtHighColumn:
			return HIGH_N;
	}
	return UNKNOWN;
}

void SVDColumnSelector::SetConditionFromColumn() 
{
	int nSel;
	int nItem = GetSelectedItem();	// get the column, if one selected
	m_nPreviousItem = nItem;		// save current column
	if (nItem >= 0)					// if column selected
	{
		SVColumn svCol = (m_pColumn->GetAt(nItem));
		m_bHighlightCondition = svCol.bRule ? TRUE : FALSE;
		UpdateData(FALSE);
		if (svCol.IsNumber())		// only applies to number columns
		{
			// special if numeric subcolumns
			if (svCol.sc != scNone)
			{
				if (svCol.sc == scMultiValues)
				{
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES))->SetCheck(1);
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS))->SetCheck(0);
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR))->SetCheck(-0);
				}
				else if (svCol.sc == scMultiBars)
				{
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES))->SetCheck(0);
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS))->SetCheck(1);
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR))->SetCheck(0);
				}
				else if (svCol.sc == scSingleBar)
				{
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3VALUES))->SetCheck(0);
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_3BARS))->SetCheck(0);
					((CButton*)GetDlgItem(IDC_RADIO_SUBCOLUMNS_1BAR))->SetCheck(1);
				}
			}
			else	// all other number types
			{
				GetDlgItem(IDC_STATIC_FILTER)->EnableWindow(m_bHighlightCondition);
				GetDlgItem(IDC_CONDITION_NUM)->EnableWindow(m_bHighlightCondition);
				GetDlgItem(IDC_RADIO_COLVAL_VAL)->EnableWindow(m_bHighlightCondition);
				GetDlgItem(IDC_RADIO_COLVAL_COL)->EnableWindow(m_bHighlightCondition);
				GetDlgItem(IDC_STATIC_COLUMN)->EnableWindow(m_bHighlightCondition);
				GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(m_bHighlightCondition ? SW_SHOW :SW_HIDE);
				RedrawControl(IDC_BUTTON_RULE_COLOR);
				GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(m_bHighlightCondition);
				m_dNumValue = 0;
				if (svCol.rt == rtNone)	// disable value edit if no rule
				{
					GetDlgItem(IDC_NUMVALUE)->EnableWindow(FALSE);
					GetDlgItem(IDC_RULESUFFIX)->EnableWindow(FALSE);
				}
				else
				{
					GetDlgItem(IDC_NUMVALUE)->EnableWindow(m_bHighlightCondition);
					GetDlgItem(IDC_RULESUFFIX)->EnableWindow(m_bHighlightCondition);
				}

				if ((svCol.dRuleValue > 0) && (svCol.rt != rtNone))
					m_dNumValue = svCol.dRuleValue / RuleSuffixToMultiple(svCol.rs);

				if (svCol.nValueFromColumn >= 0)
				{
					GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_HIDE);
					GetDlgItem(IDC_CHARVALUE)->ShowWindow(SW_HIDE);
					GetDlgItem(IDC_RULESUFFIX)->ShowWindow(SW_HIDE);
					GetDlgItem(IDC_RADIO_COLVAL_VAL)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_RADIO_COLVAL_COL)->ShowWindow(SW_SHOW);
					GetDlgItem(ID_HELP_NUM_STD)->ShowWindow(SW_HIDE);
					GetDlgItem(ID_HELP_NUM_VALUECOLUMN)->ShowWindow(SW_SHOW);

					if ((svCol.rt == rtEqualColumn) ||
						(svCol.rt == rtLowColumn )||
						(svCol.rt == rtHighColumn))
					{
						GetDlgItem(IDC_STATIC_COLUMN)->ShowWindow(SW_SHOW);
						SVColumn*  pCol = &m_pColumn->GetAt(svCol.nValueFromColumn);
						m_csColumnName = pCol->csHeaderText;
						m_nRadioColval = COLUMN_ON;
					}
					else
					{
						GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_SHOW);
						GetDlgItem(IDC_STATIC_FILLER)->ShowWindow(SW_SHOW);
						m_nRadioColval = VALUE_ON;
					}
				}

				nSel = ColumnRuleNumToItem(svCol.rt);
				if (nSel != UNKNOWN)
					m_shConditionN.SetCurSel(nSel);		// set selection
				else 
				{
					//GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_HIDE);
					GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(FALSE);
					m_shConditionN.SetWindowText(RESCHAR(IDS_V_NORULEDEFINED));
				}
				nSel = ColumnRuleSuffixToItem(svCol.rs);
				m_shRuleSuffix.SetCurSel(nSel);		// set selection
				m_shRuleSuffix.Refresh();
				m_shConditionN.Refresh();
			}
		}
		else if (svCol.IsCharacter())
		{
			GetDlgItem(IDC_STATIC_FILTER)->EnableWindow(m_bHighlightCondition);
			GetDlgItem(IDC_CONDITION_CHAR)->EnableWindow(m_bHighlightCondition);
			GetDlgItem(IDC_CHARVALUE)->EnableWindow(m_bHighlightCondition);
			GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(m_bHighlightCondition ? SW_SHOW :SW_HIDE);
			RedrawControl(IDC_BUTTON_RULE_COLOR);
			GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(m_bHighlightCondition);
			m_csCharValue = svCol.csRuleValue;

			nSel = ColumnRuleCharToItem(svCol.rt);
			if (nSel != UNKNOWN)
				m_shConditionC.SetCurSel(nSel);		// set selection
			else
			{
				GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_HIDE);
				GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(FALSE);
				GetDlgItem(IDC_CHARVALUE)->EnableWindow(FALSE);
				m_shConditionC.SetWindowText(RESCHAR(IDS_V_NORULEDEFINED));
			}
			m_shConditionC.Refresh();
		}
		else if (svCol.IsDate())
			SetDateRuleFromColumn();
		else if (svCol.ct == ctBoolean)
		{
			if (svCol.gdt == gdtEmptyCircle)
			{
				((CButton*)GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE))->SetCheck(1);
				((CButton*)GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_BLANK))->SetCheck(0);
			}	
			else
			{
				((CButton*)GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_CIRCLE))->SetCheck(0);
				((CButton*)GetDlgItem(IDC_RADIO_BOOLEAN_FALSE_BLANK))->SetCheck(1);
			}
		}
		UpdateData(FALSE);
	}
}

void SVDColumnSelector::OnCheckFilter() 
{
	// note:  when highlight box is checked from 'off' to 'on',
	//	followed by a click to another listview column,
	//	OnKillfocus is called for current column
	//	BUT GetSelectedItem value has already switched to new column
	int nItem = GetSelectedItem();
	m_nPreviousItem = nItem;	//save current column
	if (nItem < 0)
		return;
	SVColumn svCol = m_pColumn->GetAt(nItem);
	UpdateData(TRUE);		// get values from dialog
	svCol.bRule = (m_bHighlightCondition == TRUE);
	GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(m_bHighlightCondition ? SW_SHOW :SW_HIDE);
	RedrawControl(IDC_BUTTON_RULE_COLOR);
	GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(m_bHighlightCondition);
	GetDlgItem(IDC_STATIC_FILTER)->EnableWindow(m_bHighlightCondition);
	GetDlgItem(IDC_CONDITION_NUM)->EnableWindow(m_bHighlightCondition);
	GetDlgItem(IDC_CONDITION_CHAR)->EnableWindow(m_bHighlightCondition);
	GetDlgItem(IDC_RADIO_COLVAL_VAL)->EnableWindow(m_bHighlightCondition);
	GetDlgItem(IDC_RADIO_COLVAL_COL)->EnableWindow(m_bHighlightCondition);
	GetDlgItem(IDC_STATIC_COLUMN)->EnableWindow(m_bHighlightCondition);
	if (svCol.rt == rtNone)	// disable value edit box if no rule set
	{
		GetDlgItem(IDC_NUMVALUE)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHARVALUE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RULESUFFIX)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_NUMVALUE)->EnableWindow(m_bHighlightCondition);
		GetDlgItem(IDC_CHARVALUE)->EnableWindow(m_bHighlightCondition);
		GetDlgItem(IDC_RULESUFFIX)->EnableWindow(m_bHighlightCondition);
	}

	if (svCol.nValueFromColumn >= 0)	// value from column allowed
	{
		GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CHARVALUE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RULESUFFIX)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_FILLER)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RADIO_COLVAL_VAL)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_COLVAL_COL)->ShowWindow(SW_SHOW);

		if (m_nRadioColval == VALUE_ON)
		{
			GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_STATIC_FILLER)->ShowWindow(SW_SHOW);
		}
		else
		{
			GetDlgItem(IDC_STATIC_COLUMN)->ShowWindow(SW_SHOW);
			SVColumn*  pCol = &m_pColumn->GetAt(svCol.nValueFromColumn);
			m_csColumnName = pCol->csHeaderText;
		}
	}
	else if ((svCol.IsCharacter() && ColumnRuleCharToItem(svCol.rt) == UNKNOWN) ||
		(svCol.IsNumber() && ColumnRuleNumToItem(svCol.rt) == UNKNOWN))
	{
		GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(FALSE);
	}
	m_pColumn->SetAt(nItem, svCol);
	UpdateData(FALSE);
	m_shConditionN.Refresh();
	m_shConditionC.Refresh();
	m_shRuleSuffix.Refresh();
}
void SVDColumnSelector::OnCheckFilter2() // date
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	UpdateData(TRUE);		// get values from dialog
	SVColumn svCol = m_pColumn->GetAt(nItem);
	svCol.bRule = (m_bHighlightCondition2 == TRUE);
	GetDlgItem(IDC_RADIO_WITHIN)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_RADIO_CURRENT)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_CURRENTDATE)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_WITHINDATE)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_DATEVALUE)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_STATIC_FILTER2)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(m_bHighlightCondition2);
	GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow((m_bHighlightCondition2) ? SW_SHOW :SW_HIDE);
	m_shWithinDate.Refresh();
	m_shCurrentDate.Refresh();
	m_shGraph2.ResetContent();
	if (m_bHighlightCondition2)
	{
		m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_SCATTER));
		m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_CIRCLETRUE));
		m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_CIRCLEFALSE)); 
		int nSel = ColumnGraphToItem(svCol.gdt);
		m_shGraph2.SetCurSel(nSel);
		svCol.gdt = SelectedGraphToColumn(nSel, false);
	}
	else
	{
		// highlight checkbox is off so graphic must only allow scatterplot
		m_shGraph2.AddString(RESCHAR(IDS_V_GRAPH_SCATTER));
		m_shGraph2.SetCurSel(SCATTER);		// set selection
		svCol.gdt = SelectedGraphToColumn(SCATTER, false);
	}
	m_pColumn->SetAt(nItem, svCol);
	RedrawControl(IDC_BUTTON_RULE_COLOR);
	if (m_bHighlightCondition2)
		OnRadioDate();	// enable/disable radio fields
}

void SVDColumnSelector::OnRulesuffix() 
{
	m_nClickedID = IDC_RULESUFFIX;
}

void SVDColumnSelector::OnRuleSuffixSelected()
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	UpdateData(TRUE);
	SVColumn svCol = m_pColumn->GetAt(nItem);
	svCol.rs = SelectedRuleSuffixToColumn(m_shRuleSuffix.GetCurSel());
	svCol.dRuleValue = m_dNumValue * RuleSuffixToMultiple(svCol.rs);
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::SetRuleSuffixFromColumn()
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	int nSel;
	SVColumn svCol = m_pColumn->GetAt(nItem);
	nSel = ColumnRuleSuffixToItem(svCol.rs);
	if (nSel <= NONE_N)
		m_shRuleSuffix.SetWindowText(_T("...") );		// set none
	else
	m_shRuleSuffix.SetCurSel(nSel);
	UpdateData(FALSE);
}

int SVDColumnSelector::ColumnRuleSuffixToItem(_RuleSuffix rs)
{
	switch (rs)
	{
		case rsNone:
			return NONE_N;
		case rsKB:
			return KB_N;
		case rsMB:
			return MB_N;
		case rsGB:
			return GB_N;
		case rsTB:
			return TB_N;
	}
	return NONE_N;
}

_RuleSuffix SVDColumnSelector::SelectedRuleSuffixToColumn(long nID)
{
	switch (nID)
	{
		case NONE_N:
			return rsNone;
		case KB_N:
			return rsKB;
		case MB_N:
			return rsMB;
		case GB_N:
			return rsGB;
		case TB_N:
			return rsTB;
	}
	return rsNone;
}

double SVDColumnSelector::RuleSuffixToMultiple(_RuleSuffix rs)
{
	switch (rs)
	{
		case rsNone:
			return 1;
		case rsKB:
			return 1024;
		case rsMB:
			return 1048576;
		case rsGB:
			return 1073741824;
		case rsTB:
			return 1099511627776;
	}
	return 1;
}

void SVDColumnSelector::OnKillfocusNumvalue() 
{
	// if column list is clicked, current column changes before OnKillfocus
	int nItem = GetPreviousItem();	// get the column
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		svCol.rs = SelectedRuleSuffixToColumn(m_shRuleSuffix.GetCurSel());
		svCol.dRuleValue = m_dNumValue * RuleSuffixToMultiple(svCol.rs);
		m_pColumn->SetAt(nItem, svCol);
	}
}


void SVDColumnSelector::OnKillfocusCharvalue() 
{
	int nItem = GetPreviousItem();	// get the column
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		svCol.csRuleValue = m_csCharValue;
		m_pColumn->SetAt(nItem, svCol);
	}
}

void SVDColumnSelector::OnChangeCharvalue() 
{
	UpdateData(TRUE);
}

void SVDColumnSelector::OnChangeNumvalue() 
{
	UpdateData(TRUE);
}


void SVDColumnSelector::OnCheckShowgraph2() 
{
	OnCheckShowgraph();
}

void SVDColumnSelector::OnRadioDate() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn svCol = m_pColumn->GetAt(nItem);
	UpdateData(TRUE);

	if (m_nDateStyle == RADIO_WITHIN_ON)
	{
		svCol.rt = SelectedRuleDateWithToColumn(m_shWithinDate.GetCurSel());
		svCol.nDateRadio = RADIO_WITHIN_ON;

		GetDlgItem(IDC_WITHINDATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_DATEVALUE)->EnableWindow(TRUE);
		GetDlgItem(IDC_CURRENTDATE)->EnableWindow(FALSE);
	}
	else if (m_nDateStyle == RADIO_CURRENT_ON)
	{
		svCol.rt = SelectedRuleDateCurToColumn(m_shCurrentDate.GetCurSel());
		svCol.nDateRadio = RADIO_CURRENT_ON;

		GetDlgItem(IDC_WITHINDATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_DATEVALUE)->EnableWindow(FALSE);
		GetDlgItem(IDC_CURRENTDATE)->EnableWindow(TRUE);
	}
	else // should not happen
	{
		GetDlgItem(IDC_WITHINDATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_DATEVALUE)->EnableWindow(FALSE);
		GetDlgItem(IDC_CURRENTDATE)->EnableWindow(FALSE);
	}
	m_shCurrentDate.Refresh();
	m_shWithinDate.Refresh();
	m_pColumn->SetAt(nItem, svCol);
}

#define HOURS		0	//WithinDate
#define DAYS		1	//WithinDate
#define WEEKS		2	//WithinDate

#define DAY			0	//CurrentDate
#define MONTH		1	//CurrentDate
void SVDColumnSelector::SetDate() 
{
	m_shWithinDate.AddString(RESCHAR(IDS_V_HOURS));
	m_shWithinDate.AddString(RESCHAR(IDS_V_DAYS));
	m_shWithinDate.AddString(RESCHAR(IDS_V_WEEKS));
	m_shWithinDate.SetCurSel(HOURS);

	m_shCurrentDate.AddString(RESCHAR(IDS_V_DAY));
	m_shCurrentDate.AddString(RESCHAR(IDS_V_MONTH));
	m_shCurrentDate.SetCurSel(DAY);
}

int SVDColumnSelector::ColumnRuleDateWithToItem(_RuleType rt)
{
	switch (rt)
	{
		case rtWithinHours:
			return HOURS;
		case rtWithinDays:
			return DAYS;
		case rtWithinWeeks:
			return WEEKS;
	}
	return DAYS;
}

_RuleType SVDColumnSelector::SelectedRuleDateWithToColumn(long nID)
{
	switch (nID)
	{
		case HOURS:
			return rtWithinHours;
		case DAYS:
			return rtWithinDays;
		case WEEKS:
			return rtWithinWeeks;
	}
	return rtNone;
}

int SVDColumnSelector::ColumnRuleDateCurToItem(_RuleType rt)
{
	switch (rt)
	{
		case rtCurrentDay:
			return DAY;
		case rtCurrentMonth:
			return MONTH;
	}
	return DAY;
}

_RuleType SVDColumnSelector::SelectedRuleDateCurToColumn(long nID)
{
	switch (nID)
	{
		case DAY:
			return rtCurrentDay;
		case MONTH:
			return rtCurrentMonth;
	}
	return rtNone;
}

void SVDColumnSelector::SetDateRuleFromColumn()
{
	int nItem = GetSelectedItem();	// get the column, if one selected
	if (nItem >= 0)
	{
		SVColumn svCol = (m_pColumn->GetAt(nItem));
		if (svCol.nDateRadio != RADIO_CURRENT_ON && svCol.nDateRadio != RADIO_WITHIN_ON)
			svCol.nDateRadio = m_nDateStyle;	// init column
		m_nDateStyle = svCol.nDateRadio;
		GetDlgItem(IDC_RADIO_WITHIN)->EnableWindow(m_bHighlightCondition2);
		GetDlgItem(IDC_RADIO_CURRENT)->EnableWindow(m_bHighlightCondition2);
		GetDlgItem(IDC_CURRENTDATE)->EnableWindow(m_bHighlightCondition2);
		GetDlgItem(IDC_WITHINDATE)->EnableWindow(m_bHighlightCondition2);
		GetDlgItem(IDC_DATEVALUE)->EnableWindow(m_bHighlightCondition2);
		GetDlgItem(IDC_STATIC_FILTER2)->EnableWindow(m_bHighlightCondition2);
		GetDlgItem(IDC_BUTTON_RULE_COLOR)->ShowWindow((m_bHighlightCondition2) ? SW_SHOW :SW_HIDE);
		RedrawControl(IDC_BUTTON_RULE_COLOR);
		GetDlgItem(IDC_STATIC_SETCOLOR)->EnableWindow(m_bHighlightCondition2);
		m_nDateValue = (UINT)svCol.dRuleValue;
		UpdateData(FALSE);

		if (svCol.rt != rtNone)	// if rule, set radio field selections
		{
			int nSel;
			if (m_nDateStyle == RADIO_CURRENT_ON)
			{
				nSel = ColumnRuleDateCurToItem(svCol.rt);
				m_shCurrentDate.SetCurSel(nSel);		// set selection
				m_shCurrentDate.Refresh();
			}
			else if (m_nDateStyle == RADIO_WITHIN_ON)
			{
				nSel = ColumnRuleDateWithToItem(svCol.rt);
				m_shWithinDate.SetCurSel(nSel);			// set selection
				m_shWithinDate.Refresh();
			}
		if (m_bHighlightCondition2)
			OnRadioDate();	// enable/disable radio button fields
		}
	}
}

void SVDColumnSelector::OnWithindate() 
{
	m_nClickedID = IDC_WITHINDATE;
}

void SVDColumnSelector::OnWithinDateSelected() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn svCol = m_pColumn->GetAt(nItem);
	svCol.rt = SelectedRuleDateWithToColumn(m_shWithinDate.GetCurSel());
	svCol.nDateRadio = RADIO_WITHIN_ON;
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::OnCurrentdate() 
{
	m_nClickedID = IDC_CURRENTDATE;
}

void SVDColumnSelector::OnCurrentDateSelected() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn svCol = m_pColumn->GetAt(nItem);
	svCol.rt = SelectedRuleDateCurToColumn(m_shCurrentDate.GetCurSel());
	svCol.nDateRadio = RADIO_CURRENT_ON;
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::OnKillfocusDatevalue() 
{
	// if column list is clicked, current column changes before OnKillfocus
	int nItem = GetPreviousItem();	// get the proper column
	if (nItem >= 0)
	{
		SVColumn  svCol = (m_pColumn->GetAt(nItem));
		svCol.dRuleValue = m_nDateValue;
		m_pColumn->SetAt(nItem, svCol);
	}
}

void SVDColumnSelector::OnChangeDatevalue() 
{
		UpdateData(TRUE);
}

void SVDColumnSelector::OnRadioBooleanFalseCircle() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.gdt = gdtEmptyCircle;
	m_pColumn->SetAt(nItem, svCol);
	m_bDirty = true;	
}

void SVDColumnSelector::OnRadioBooleanFalseBlank() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.gdt = gdtCircleWhenTrue;
	m_pColumn->SetAt(nItem, svCol);
	m_bDirty = true;
}

void SVDColumnSelector::OnRadioSubcolumns3values() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.sc = scMultiValues;
	svCol.nSubColumns = 3;
	m_pColumn->SetAt(nItem, svCol);
	m_bDirty = true;
	
}

void SVDColumnSelector::OnRadioSubcolumns3bars() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.sc = scMultiBars;
	svCol.nSubColumns = 3;
	m_pColumn->SetAt(nItem, svCol);
	m_bDirty = true;
	
}

void SVDColumnSelector::OnRadioSubcolumns1bar() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.sc = scSingleBar;
	svCol.nSubColumns = 3;
	m_pColumn->SetAt(nItem, svCol);
	m_bDirty = true;
	
}

void SVDColumnSelector::AddJustify(SVColumn* pCol)
{
	GetDlgItem(IDC_STATIC_HEADER)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_RADIO_JLEFT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_RADIO_JCENTER)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_RADIO_JRIGHT)->ShowWindow(SW_SHOW);
	CWnd* pw = GetDlgItem(IDC_STATIC_HEADER);
	CString cs;
	cs.Format(RESCHAR(IDS_V_COLUMNNAME), pCol->csHeaderText);
	if (pCol->nJustify == DT_CENTER)
		pw->ModifyStyle(SS_LEFT | SS_RIGHT | SS_CENTER, SS_CENTER, 0);
	else if (pCol->nJustify == DT_RIGHT)
		pw->ModifyStyle(SS_LEFT | SS_RIGHT | SS_CENTER, SS_RIGHT, 0);
	else
		pw->ModifyStyle(SS_LEFT | SS_RIGHT | SS_CENTER, SS_LEFT, 0);
	pw->SetWindowText(cs);
}

void SVDColumnSelector::OnCheckIgnoreCase() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	UpdateData(TRUE);
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.bIgnoreCase = m_bIgnoreCase ? true : false;
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::OnCheckShowdatetext() 
{
	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	UpdateData(TRUE);
	SVColumn  svCol = (m_pColumn->GetAt(nItem));
	svCol.bText = m_bShowDateText ? true : false;
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::OnHelp() 
{
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 
//	AfxGetApp()->WinHelp(HID_HELP_GENERIC topic ID, HELP_CONTEXT);
}

void SVDColumnSelector::OnRadioColvalVal() 
{
	UpdateData(TRUE);	// m_nRadioColval VALUE_ON
	GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_FILLER)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_COLUMN)->ShowWindow(SW_HIDE);

	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = m_pColumn->GetAt(nItem);
	svCol.rt = SelectedRuleNumToColumn(m_shConditionN.GetCurSel());
	m_pColumn->SetAt(nItem, svCol);
}

void SVDColumnSelector::OnRadioColvalCol() 
{
	UpdateData(TRUE);	// m_nRadioColval COLUMN_ON
	GetDlgItem(IDC_NUMVALUE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_FILLER)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_COLUMN)->ShowWindow(SW_SHOW);

	int nItem = GetSelectedItem();
	if (nItem < 0)
		return;
	SVColumn  svCol = m_pColumn->GetAt(nItem);
	svCol.rt = SelectedRuleNumToColumn(m_shConditionN.GetCurSel());
	m_pColumn->SetAt(nItem, svCol);
	svCol = m_pColumn->GetAt(svCol.nValueFromColumn);
	m_csColumnName = svCol.csHeaderText;
	UpdateData(FALSE);
}

void SVDColumnSelector::OnHelpCharStd() 
{
//	AfxGetApp()->WinHelp(HID_HELP_CHAR_STD, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}

void SVDColumnSelector::OnHelpNumStd() 
{
//	AfxGetApp()->WinHelp(HID_HELP_NUM_STD, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}

void SVDColumnSelector::OnHelpNumNograph() 
{
//	AfxGetApp()->WinHelp(HID_HELP_NUM_NOGRAPH, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}

void SVDColumnSelector::OnHelpNumSubcolumns() 
{
//	AfxGetApp()->WinHelp(HID_HELP_NUM_SUBCOLUMNS, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}

void SVDColumnSelector::OnHelpNumValuecolumn() 
{
//	AfxGetApp()->WinHelp(HID_HELP_NUM_VALUECOLUMN, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}

void SVDColumnSelector::OnHelpDateStd() 
{
//	AfxGetApp()->WinHelp(HID_HELP_DATE_STD, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}

void SVDColumnSelector::OnHelpBoolStd() 
{
//	AfxGetApp()->WinHelp(HID_HELP_BOOL_STD, HELP_CONTEXT);
	AfxGetApp()->WinHelp(NULL, HELP_FINDER); 	
}
