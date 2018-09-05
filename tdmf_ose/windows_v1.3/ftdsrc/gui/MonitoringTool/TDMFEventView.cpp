// CTDMFEventView.cpp : implementation of the CTDMFEventView class
//

#include "stdafx.h"
#include "TDMFGUI.h"
#include "Doc.h"
#include "TDMFEventView.h"
#include "MainFrm.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// These defines must be in the same order as the columns in SetColumnDefaults.
#define ColEventTime						0 
#define ColMessageType					1
#define ColSeverity						2
#define ColMessage						3

const _TCHAR tcUndefined[] = _T("UNDEFINED");
const _TCHAR tcNotImplemented[] = _T("Not Implemented");


/////////////////////////////////////////////////////////////////////////////
// CTDMFEventView

IMPLEMENT_DYNCREATE(CTDMFEventView, SVBase)

BEGIN_MESSAGE_MAP(CTDMFEventView, SVBase)
	//{{AFX_MSG_MAP(CTDMFEventView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, SVBase::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDMFEventView construction/destruction

CTDMFEventView::CTDMFEventView()
{
	m_pActiveSheet = &m_Sheet;
  	m_bFirstUpdate = true;
}

CTDMFEventView::~CTDMFEventView()
{
	DeleteAllEvents();
}

void CTDMFEventView::Update()
{
	UpdateView();
}

void CTDMFEventView::InitialyzeTheView()
{
  	if (m_bFirstUpdate)		// new sheet, use defaults
	{
		SetColumnDefaults();

		if(m_pActiveSheet->caColumns.GetSize() > 0)
		AddColumns(m_pActiveSheet);

		m_bFirstUpdate = false;
	}

}

void CTDMFEventView::OnInitialUpdate()
{
	SVBase::OnInitialUpdate();

	InitialyzeTheView();
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFEventView printing

BOOL CTDMFEventView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTDMFEventView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTDMFEventView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFEventView diagnostics

#ifdef _DEBUG
void CTDMFEventView::AssertValid() const
{
	SVBase::AssertValid();
}

void CTDMFEventView::Dump(CDumpContext& dc) const
{
	SVBase::Dump(dc);
}

#endif //_DEBUG

bool CTDMFEventView::UpdateView()	// new collection
{

	LoadThelistofEvents();

	int nNumObjects = m_mapEvents.GetCount(); 

	DeleteAllRows();

	return AddRows(nNumObjects);
}


bool CTDMFEventView::AddRows(int nNumObjects)	
{
	if (nNumObjects <= 0)
		return false;

	for (int i=0; i< nNumObjects; i++)
		AddRow(i);

	return true;

}

void CTDMFEventView::SetColumnDefaults()
{
	m_pActiveSheet->caColumns.RemoveAll();	

	SVColumn svCol;

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctDateTime;
	svCol.csHeaderText = _T("Event Date Time"); 
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Message Type"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctInt;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.clrRule[COLORRULE0] = RGB(255,0,0);
	svCol.rt = rtHigh;
	svCol.dRuleValue = 5;
	svCol.nMinValue = 1;
	svCol.nMaxValue = 5;
	svCol.bRule = true;
	svCol.csHeaderText = _T("Severity");
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 700;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Message"); 
	m_pActiveSheet->caColumns.Add(svCol);


	m_pActiveSheet->bUseDefaults = false;

}



CString CTDMFEventView::GetValueString(UINT nIndex, int nColumn)
{
	CString strValue;
	CTDMFEventInfo* p;

	switch (nColumn)
	{	
		case ColMessageType:
			m_mapEvents.Lookup(nIndex+1, p);
			strValue = p->m_strMessageType;
			break;
		case ColMessage:
			m_mapEvents.Lookup(nIndex+1, p);
			strValue = p->m_strMessage;
			break;
		default:
			strValue = tcNotImplemented;
			break;
	}

	return strValue;
} 

COleDateTime CTDMFEventView::GetValueDateTime(UINT nIndex, int nColumn)
{

	COleDateTime dtDate;
	CTDMFEventInfo* p;

	switch (nColumn)
	{
		case ColEventTime:
			m_mapEvents.Lookup(nIndex+1, p);
			dtDate = p->m_dtEventTime;

		break;
	}
	

	return dtDate;

}

int CTDMFEventView::GetValueInt(UINT nIndex,int nColumn)
{
	int nValue = 0;
	CTDMFEventInfo* p;

	switch (nColumn)
	{
		case ColSeverity:
			m_mapEvents.Lookup(nIndex+1, p);
			nValue = p->m_nSeverity;
		break;
		default:
			nValue = 0;
		break;
	}

	return nValue;
}

double CTDMFEventView::GetValueDouble(UINT nIndex,int nColumn)
{
	double dValue = 0;
/*		CTDMFEventInfo* p;



	switch (nColumn)
	{
		default:
			dValue = 0;
		break;
	}
*/
	return dValue;
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-17			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFEventView::GetValueBool
	Description :	If value is in a column boolean get the value from the object
	Return :			bool	-	
	Parameters :
		UINT nIndex	-	number of the object in the list
		int nColumn	-	index of the column
	Note :
\*---------------------------------------------------------------------------*/
bool CTDMFEventView::GetValueBool(UINT nIndex, int nColumn)
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

bool CTDMFEventView::StartView(int nSortColumn , bool bSortDirection)
{

	void* pObject = NULL;
	void* pMaps = 0;

	SVBase::StartView(pObject,&m_Sheet,pMaps,nSortColumn,bSortDirection);

	return true;
}

void	CTDMFEventView::LoadThelistofEvents()
{

	DeleteAllEvents();

	CString strValue;
	for (int i = 1; i<=15; i++)
	{
		strValue.Format(" %d ",i);
	 	CTDMFEventInfo* p = new CTDMFEventInfo();
		if (p)
		{
			p->m_nID = i;
			p->m_strMessage		= _T("Message ") + strValue;
			p->m_dtEventTime.GetCurrentTime();
			p->m_strMessageType	= _T("Warning");
			p->m_nSeverity			= 5;
			m_mapEvents.SetAt(p->m_nID, p);  //SetAt should be in all Map<>, but check if it's already present

		}
	}

}

void CTDMFEventView::DeleteAllEvents()
{
	POSITION pos = m_mapEvents.GetStartPosition();
	while(pos != NULL)
	{
		 int nID;
		 CTDMFEventInfo* p;
		 
		 m_mapEvents.GetNextAssoc(pos, nID, p);
		 delete p;
	}

	m_mapEvents.RemoveAll();
}


int CTDMFEventView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (SVBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//attach the view to the main document
	Doc* p = GetDocument();
	p->AddView(this);
	
	return 0;
}
