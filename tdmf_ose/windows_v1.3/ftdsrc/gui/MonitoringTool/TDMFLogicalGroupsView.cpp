// TDMFLogicalGroupsView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "TDMFLogicalGroupsView.h"
#include "Doc.h"
#include "MainFrm.h"
#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// These defines must be in the same order as the columns in SetColumnDefaults.
#define ColName							0 
#define ColLocalRead						1
#define ColLocalWrite					2
#define ColPercentBAB					3
#define ColBABEntries					4
#define ColPercentNetworkUsage		5
#define ColCompressionRate				6
#define ColJournalMegaByte				7
#define ColState							8
#define ColMode							9
#define ColAction							10

const _TCHAR tcUndefined[] = _T("UNDEFINED");
const _TCHAR tcNotImplemented[] = _T("Not Implemented");

/////////////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroupsView

IMPLEMENT_DYNCREATE(CTDMFLogicalGroupsView, SVBase)

CTDMFLogicalGroupsView::CTDMFLogicalGroupsView()
{
	m_pActiveSheet = &m_Sheet;
	m_bFirstUpdate = true;
}

CTDMFLogicalGroupsView::~CTDMFLogicalGroupsView()
{
	DeleteAllLogicalGroups();
}


BEGIN_MESSAGE_MAP(CTDMFLogicalGroupsView, SVBase)
	//{{AFX_MSG_MAP(CTDMFLogicalGroupsView)
		ON_WM_CREATE()
	ON_WM_ACTIVATE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, SVBase::OnFilePrintPreview)
END_MESSAGE_MAP()


void CTDMFLogicalGroupsView::InitialyzeTheView()
{
  	if (m_bFirstUpdate)		// new sheet, use defaults
	{
		SetColumnDefaults();

		if(m_pActiveSheet->caColumns.GetSize() > 0)
		AddColumns(m_pActiveSheet);

		m_bFirstUpdate = false;
	}

}

void CTDMFLogicalGroupsView::OnInitialUpdate()
{
	SVBase::OnInitialUpdate();


   InitialyzeTheView();
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroupsView printing

BOOL CTDMFLogicalGroupsView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTDMFLogicalGroupsView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTDMFLogicalGroupsView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroupsView diagnostics

#ifdef _DEBUG
void CTDMFLogicalGroupsView::AssertValid() const
{
	SVBase::AssertValid();
}

void CTDMFLogicalGroupsView::Dump(CDumpContext& dc) const
{
	SVBase::Dump(dc);
}
#endif //_DEBUG


bool CTDMFLogicalGroupsView::UpdateView()	// new collection
{

	LoadThelistofLogicalGroups();

	int nNumObjects = m_mapLogicalGroups.GetCount(); 

	DeleteAllRows();

	return AddRows(nNumObjects);
}


bool CTDMFLogicalGroupsView::AddRows(int nNumObjects)	
{
	if (nNumObjects <= 0)
		return false;

	for (int i=0; i< nNumObjects; i++)
		AddRow(i);

	return true;

}

void CTDMFLogicalGroupsView::SetColumnDefaults()
{
	m_pActiveSheet->caColumns.RemoveAll();	

	SVColumn svCol;

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("T-Name"); 
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.csHeaderText = _T("L Reads");
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.csHeaderText = _T("L Writes");
	m_pActiveSheet->caColumns.Add(svCol);

		
	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctInt;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.nValueFromColumn = ColBABEntries;
	svCol.csHeaderText = _T("BAB Entries");
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.csHeaderText = _T("% Net");
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctFloat;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.csHeaderText = _T("% Net");
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Comp Rate"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ut = utNone;
	svCol.ct = ctInt;
	svCol.gdt = gdtBar;
	svCol.bGraphic = true;
	svCol.bGraphable = true;
	svCol.csHeaderText = _T("Jrnl MB");
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("State");
	m_pActiveSheet->caColumns.Add(svCol);


	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Mode");
	m_pActiveSheet->caColumns.Add(svCol);
	
	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Action");
	m_pActiveSheet->caColumns.Add(svCol);

	m_pActiveSheet->bUseDefaults = false;

}



CString CTDMFLogicalGroupsView::GetValueString(UINT nIndex, int nColumn)
{
	CString strValue;
	CTDMFLogicalGroup* p;

	switch (nColumn)
	{	
		case ColName:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			strValue = p->m_strGroupName;
			break;
		case ColState:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			strValue = p->GetStringState(p->m_nState);
			break;
		case ColMode:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			strValue = p->GetStringMode(p->m_nMode);
			break;
		case ColAction:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			strValue = p->GetStringAction(p->m_nAction);
			break;
		default:
			strValue = tcNotImplemented;
			break;
	}

	return strValue;
} 

COleDateTime CTDMFLogicalGroupsView::GetValueDateTime(UINT nIndex, int nColumn)
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

int CTDMFLogicalGroupsView::GetValueInt(UINT nIndex,int nColumn)
{
	int nValue = 0;
	CTDMFLogicalGroup* p;

	switch (nColumn)
	{
		case ColLocalRead:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			nValue = p->m_nLocalReads;
		break;
		case ColLocalWrite:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			nValue = p->m_nLocalWrites;
		break;
		case ColBABEntries:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			nValue = p->m_nBABEntries;
		break;
		case ColJournalMegaByte:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			nValue = p->m_nJournalMegaByte;
		break;
		default:
			nValue = 0;
		break;
	}

	return nValue;
}

double CTDMFLogicalGroupsView::GetValueDouble(UINT nIndex,int nColumn)
{
	double dValue = 0;

	CTDMFLogicalGroup* p;


	switch (nColumn)
	{
	
		case ColPercentBAB:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			dValue = p->m_dPourcentBABUsed;
		break;
		case ColPercentNetworkUsage:
			m_mapLogicalGroups.Lookup(nIndex+1, p);
			dValue = p->m_dNetworkUtilisation;
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
	Function :		CTDMFLogicalGroupsView::GetValueBool
	Description :	If value is in a column boolean get the value from the object
	Return :			bool	-	
	Parameters :
		UINT nIndex	-	number of the object in the list
		int nColumn	-	index of the column
	Note :
\*---------------------------------------------------------------------------*/
bool CTDMFLogicalGroupsView::GetValueBool(UINT nIndex, int nColumn)
{

	UINT nValue = 0;
//	CTDMFVolume* pVolume;

//	switch (nColumn)
//	{
	//	case ColAvailable:
//			m_mapLogicalGroups.Lookup(nIndex+1, pVolume);
	//		nValue = pVolume->m_bAvailable;
//		break;
//	}

	if ((nValue > 0))
		return true;
	return false;

}

bool CTDMFLogicalGroupsView::StartView(int nSortColumn , bool bSortDirection)
{

	void* pObject = NULL;
	void* pMaps = 0;

	SVBase::StartView(pObject,&m_Sheet,pMaps,nSortColumn,bSortDirection);

	return true;
}

void	CTDMFLogicalGroupsView::LoadThelistofLogicalGroups()
{

	DeleteAllLogicalGroups();

	CString strValue;
	for (int i = 1; i<=15; i++)
	{
		strValue.Format(" %d ",i);
	 	CTDMFLogicalGroup* p = new CTDMFLogicalGroup();
		if (p)
		{
			p->m_nID = i;
			p->m_strGroupName = _T("LG") + strValue;
			p->m_nLocalReads = 0;
			p->m_nLocalWrites = 0;
			p->m_dPourcentBABUsed = 0;
			p->m_nBABEntries = i;
			p->m_dNetworkUtilisation = 0;
			p->m_strCompressionRatio = _T("1:1");
			p->m_nJournalMegaByte = i;
			m_mapLogicalGroups.SetAt(p->m_nID, p);  //SetAt should be in all Map<>, but check if it's already present

		}
	}

}

void CTDMFLogicalGroupsView::DeleteAllLogicalGroups()
{
	POSITION pos = m_mapLogicalGroups.GetStartPosition();
	while(pos != NULL)
	{
		 int nID;
		 CTDMFLogicalGroup* p;
		 
		 m_mapLogicalGroups.GetNextAssoc(pos, nID, p);
		 delete p;
	}

	m_mapLogicalGroups.RemoveAll();
}

void CTDMFLogicalGroupsView::Update()
{
	UpdateView();
}

void CTDMFLogicalGroupsView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
		UpdateView();
/*	switch (lHint)
	{
		case UVH_CHANGE_GRAPH_TYPE:
			{
			
			};
  		break;
	
	}
*/	
	
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroupsView message handlers
void CTDMFLogicalGroupsView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}



int CTDMFLogicalGroupsView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (SVBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
  	return 0;
}

void CTDMFLogicalGroupsView::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	SVBase::OnActivate(nState, pWndOther, bMinimized);
	
	//attach the view to the main document
	if(!m_pDocument)
	{
		Doc* p = GetDocument();
		p->AddView(this);
	}

	if(nState == WA_ACTIVE )
	{
     Update();
	}	
}
