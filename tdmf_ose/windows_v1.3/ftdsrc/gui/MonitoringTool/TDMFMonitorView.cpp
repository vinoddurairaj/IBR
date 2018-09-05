// TDMFMonitorView.cpp : implementation of the CTDMFMonitorView class
//

#include "stdafx.h"
#include "TDMFGUI.h"
#include "Doc.h"
#include "TDMFMonitorView.h"
#include "MainFrm.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// These defines must be in the same order as the columns in SetColumnDefaults.
#define ColServer							0 
#define ColConnection					1
#define ColLogicalGroup					2
#define ColReplicationPair				3
#define ColMode							4
#define ColLocalRead						5
#define ColLocalWrite					6
#define ColNetActual						7
#define ColNetEffect						8
#define ColEntries						9
#define ColPBABUsed						10

const _TCHAR tcUndefined[] = _T("UNDEFINED");
const _TCHAR tcNotImplemented[] = _T("Not Implemented");


/////////////////////////////////////////////////////////////////////////////
// CTDMFMonitorView

IMPLEMENT_DYNCREATE(CTDMFMonitorView, SVBase)

BEGIN_MESSAGE_MAP(CTDMFMonitorView, SVBase)
	//{{AFX_MSG_MAP(CTDMFMonitorView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, SVBase::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDMFMonitorView construction/destruction

CTDMFMonitorView::CTDMFMonitorView()
{
	m_pActiveSheet = &m_Sheet;

}

CTDMFMonitorView::~CTDMFMonitorView()
{
	DeleteAllMonitors();
}

BOOL CTDMFMonitorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return SVBase::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFMonitorView drawing

void CTDMFMonitorView::OnDraw(CDC* pDC)
{


	// TODO: add draw code for native data here
}

void CTDMFMonitorView::OnInitialUpdate()
{
	SVBase::OnInitialUpdate();

	LoadThelistofMonitors();

	if (m_pActiveSheet->bUseDefaults)		// new sheet, use defaults
		SetColumnDefaults();

	AddColumns(m_pActiveSheet);

	UpdateView();
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFMonitorView printing

BOOL CTDMFMonitorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTDMFMonitorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTDMFMonitorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFMonitorView diagnostics

#ifdef _DEBUG
void CTDMFMonitorView::AssertValid() const
{
	SVBase::AssertValid();
}

void CTDMFMonitorView::Dump(CDumpContext& dc) const
{
	SVBase::Dump(dc);
}
#endif //_DEBUG

bool CTDMFMonitorView::UpdateView()	// new collection
{
	int nNumObjects = m_mapMonitors.GetCount(); 

	DeleteAllRows();

	return AddRows(nNumObjects);
}


bool CTDMFMonitorView::AddRows(int nNumObjects)	
{
	if (nNumObjects <= 0)
		return false;

	for (int i=0; i< nNumObjects; i++)
		AddRow(i);

	return true;

}

void CTDMFMonitorView::SetColumnDefaults()
{
	m_pActiveSheet->caColumns.RemoveAll();	

	SVColumn svCol;

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Server"); 
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Connection"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Logical Group"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Replication Pair"); 
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Mode/%Done"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtEquals;
	svCol.dRuleValue = 14;
	svCol.bRule = true;
	svCol.nValueFromColumn = ColLocalRead;
	svCol.csHeaderText = _T("Local Read");
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtEquals;
	svCol.dRuleValue = 14;
	svCol.bRule = true;
	svCol.nValueFromColumn = ColLocalWrite;
	svCol.csHeaderText = _T("Local Write");
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtEquals;
	svCol.dRuleValue = 14;
	svCol.bRule = true;
	svCol.nValueFromColumn = ColNetActual;
	svCol.csHeaderText = _T("Net Actual");
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtEquals;
	svCol.dRuleValue = 14;
	svCol.bRule = true;
	svCol.nValueFromColumn = ColNetEffect;
	svCol.csHeaderText = _T("Net Effect");
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctInt;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtEquals;
	svCol.dRuleValue = 14;
	svCol.bRule = true;
	svCol.nValueFromColumn = ColEntries;
	svCol.csHeaderText = _T("Entries");
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtEquals;
	svCol.dRuleValue = 14;
	svCol.bRule = true;
	svCol.nValueFromColumn = ColPBABUsed;
	svCol.csHeaderText = _T("%BAB Used");
	m_pActiveSheet->caColumns.Add(svCol);

	m_pActiveSheet->bUseDefaults = false;

}



CString CTDMFMonitorView::GetValueString(UINT nIndex, int nColumn)
{
	CString strValue;
	CTDMFMonitorInfo* p;

	switch (nColumn)
	{	
		case ColServer:
			m_mapMonitors.Lookup(nIndex+1, p);
			strValue = p->m_strServer;
			break;
		case ColConnection:
			m_mapMonitors.Lookup(nIndex+1, p);
			strValue = p->m_strConnection;
			break;
		case ColLogicalGroup:
			m_mapMonitors.Lookup(nIndex+1, p);
			strValue = p->m_strLogicalGroup;
			break;
		case ColReplicationPair:
			m_mapMonitors.Lookup(nIndex+1, p);
			strValue = p->m_strReplicationPair;
			break;
		case ColMode:
			m_mapMonitors.Lookup(nIndex+1, p);
			strValue = p->m_strMode;
			break;
		default:
			strValue = tcNotImplemented;
			break;
	}

	return strValue;
} 

COleDateTime CTDMFMonitorView::GetValueDateTime(UINT nIndex, int nColumn)
{
/*	switch (nColumn)
	{
		case ColCrnt_DateTime:
		//	return COleDateTime::GetCurrentTime();
		break;
	}
*/	
	COleDateTime cdt;
	return cdt;

}

int CTDMFMonitorView::GetValueInt(UINT nIndex,int nColumn)
{
	int nValue = 0;
	CTDMFMonitorInfo* p;

	switch (nColumn)
	{
		case ColEntries:
			m_mapMonitors.Lookup(nIndex+1, p);
			nValue = p->m_nEntries;
		break;
		default:
			nValue = 0;
		break;
	}

	return nValue;
}

double CTDMFMonitorView::GetValueDouble(UINT nIndex,int nColumn)
{
	double dValue = 0;
	CTDMFMonitorInfo* p;


	switch (nColumn)
	{
		case ColLocalRead:
			m_mapMonitors.Lookup(nIndex+1, p);
			dValue = p->m_dLocalRead;
		break;
		case ColLocalWrite:
			m_mapMonitors.Lookup(nIndex+1, p);
			dValue = p->m_dLocalWrite;
		break;
		case ColNetActual:
			m_mapMonitors.Lookup(nIndex+1, p);
			dValue = p->m_dNetActual;
		break;
		case ColNetEffect:
			m_mapMonitors.Lookup(nIndex+1, p);
			dValue = p->m_dNetEffect;
		break;
		case ColPBABUsed:
			m_mapMonitors.Lookup(nIndex+1, p);
			dValue = p->m_dPercentBABUsed;
		break;
		default:
			dValue = 0;
		break;
	}

	return dValue;
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-17			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFMonitorView::GetValueBool
	Description :	If value is in a column boolean get the value from the object
	Return :			bool	-	
	Parameters :
		UINT nIndex	-	number of the object in the list
		int nColumn	-	index of the column
	Note :
\*---------------------------------------------------------------------------*/
bool CTDMFMonitorView::GetValueBool(UINT nIndex, int nColumn)
{

	UINT nValue = 0;
//	CTDMFVolume* pVolume;

//	switch (nColumn)
//	{
	//	case ColAvailable:
//			m_mapMonitors.Lookup(nIndex+1, pVolume);
	//		nValue = pVolume->m_bAvailable;
//		break;
//	}

	if ((nValue > 0))
		return true;
	return false;

}

bool CTDMFMonitorView::StartView(int nSortColumn , bool bSortDirection)
{

	void* pObject = NULL;
	void* pMaps = 0;

	SVBase::StartView(pObject,&m_Sheet,pMaps,nSortColumn,bSortDirection);

	return true;
}

void	CTDMFMonitorView::LoadThelistofMonitors()
{

	CString strValue;
	for (int i = 1; i<=15; i++)
	{
		strValue.Format(" %d ",i);
	 	CTDMFMonitorInfo* p = new CTDMFMonitorInfo();
		if (p)
		{
			p->m_nID = i;
			p->m_strServer = _T("Server");
			p->m_strConnection = _T("Connected");
			p->m_strLogicalGroup = _T("Logical Group") + strValue;
			m_mapMonitors.SetAt(p->m_nID, p);  //SetAt should be in all Map<>, but check if it's already present

		}
	}

}

void CTDMFMonitorView::DeleteAllMonitors()
{
	POSITION pos = m_mapMonitors.GetStartPosition();
	while(pos != NULL)
	{
		 int nID;
		 CTDMFMonitorInfo* p;
		 
		 m_mapMonitors.GetNextAssoc(pos, nID, p);
		 delete p;
	}

	m_mapMonitors.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFMonitorView message handlers
void CTDMFMonitorView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}

int CTDMFMonitorView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (SVBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//attach the view to the main document
	Doc* p = GetDocument();
	p->AddView(this);
	
	return 0;
}
