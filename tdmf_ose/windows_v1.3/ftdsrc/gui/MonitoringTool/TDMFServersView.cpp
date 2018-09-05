// TDMFServersView.cpp : implementation of the CTDMFServersView class
//

#include "stdafx.h"
#include "TDMFGUI.h"
#include "Doc.h"
#include "TDMFServersView.h"
#include "MainFrm.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// These defines must be in the same order as the columns in SetColumnDefaults.
#define ColServer							0 
#define ColIPAddress						1
#define ColOS								2
#define ColOSLevel						3
#define ColDefinedLGs					4
#define ColActiveLgs						5
#define ColVolume							6
#define ColStatus							7


const _TCHAR tcUndefined[] = _T("UNDEFINED");
const _TCHAR tcNotImplemented[] = _T("Not Implemented");


/////////////////////////////////////////////////////////////////////////////
// CTDMFServersView

IMPLEMENT_DYNCREATE(CTDMFServersView, SVBase)

/////////////////////////////////////////////////////////////////////////////
// CTDMFServersView construction/destruction

CTDMFServersView::CTDMFServersView()
{
	m_pActiveSheet = &m_Sheet;
	m_bFirstUpdate = true;
}

CTDMFServersView::~CTDMFServersView()
{
	DeleteAllServers();
}


BEGIN_MESSAGE_MAP(CTDMFServersView, SVBase)
	//{{AFX_MSG_MAP(CTDMFServersView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, SVBase::OnFilePrintPreview)
END_MESSAGE_MAP()


void CTDMFServersView::InitialyzeTheView()
{
  	if (m_bFirstUpdate)		// new sheet, use defaults
	{
		SetColumnDefaults();

		if(m_pActiveSheet->caColumns.GetSize() > 0)
		AddColumns(m_pActiveSheet);

		m_bFirstUpdate = false;
	}

}

void CTDMFServersView::OnInitialUpdate()
{
	SVBase::OnInitialUpdate();


   InitialyzeTheView();
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFServersView printing

BOOL CTDMFServersView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTDMFServersView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTDMFServersView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFServersView diagnostics

#ifdef _DEBUG
void CTDMFServersView::AssertValid() const
{
	SVBase::AssertValid();
}

void CTDMFServersView::Dump(CDumpContext& dc) const
{
	SVBase::Dump(dc);
}
#endif //_DEBUG

bool CTDMFServersView::UpdateView()	// new collection
{

	LoadThelistofServers();

	int nNumObjects = m_mapServers.GetCount(); 

	DeleteAllRows();

	return AddRows(nNumObjects);
}


bool CTDMFServersView::AddRows(int nNumObjects)	
{
	if (nNumObjects <= 0)
		return false;

	for (int i=0; i< nNumObjects; i++)
		AddRow(i);

	return true;

}

void CTDMFServersView::SetColumnDefaults()
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
	svCol.csHeaderText = _T("IP Address"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("OS"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("OS Level"); 
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
	svCol.nValueFromColumn = ColDefinedLGs;
	svCol.csHeaderText = _T("Defined LGs");
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
	svCol.nValueFromColumn = ColActiveLgs;
	svCol.csHeaderText = _T("Active LGs");
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
	svCol.nValueFromColumn = ColVolume;
	svCol.csHeaderText = _T("Volumes");
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
	svCol.nValueFromColumn = ColStatus;
	svCol.csHeaderText = _T("Status");
	m_pActiveSheet->caColumns.Add(svCol);

	m_pActiveSheet->bUseDefaults = false;

}



CString CTDMFServersView::GetValueString(UINT nIndex, int nColumn)
{
	CString strValue = tcNotImplemented;
	CTDMFServer* p;

	m_mapServers.Lookup(nIndex+1, p);
	if(p == NULL)
	{
     return strValue;
	}

	switch (nColumn)
	{	
		case ColServer:
			strValue = p->m_strServerName;
			break;
		case ColIPAddress:
			strValue = p->m_strServerIPAdress1;
			break;
		case ColOS:
			strValue = p->m_strOS_Type;
			break;
		case ColOSLevel:
			strValue = p->m_strOS_Version;
			break;
		default:
			strValue = tcNotImplemented;
			break;
	}

	return strValue;
} 

COleDateTime CTDMFServersView::GetValueDateTime(UINT nIndex, int nColumn)
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

int CTDMFServersView::GetValueInt(UINT nIndex,int nColumn)
{
	int nValue = 0;
	CTDMFServer* p;

	m_mapServers.Lookup(nIndex+1, p);
	if(p == NULL)
	{
     return nValue;
	}

	switch (nColumn)
	{
		case ColDefinedLGs:
			nValue = p->m_mapLogicalGroups.GetCount();
		break;
		case ColActiveLgs:
			nValue = p->GetNumberOfActiveLogicalGroup();
		break;
		case ColVolume:
			nValue = p->m_arVolumes.GetSize();
		break;
		case ColStatus:
			nValue = p->m_nState;
		break;
		default:
			nValue = 0;
		break;
	}

	return nValue;
}

double CTDMFServersView::GetValueDouble(UINT nIndex,int nColumn)
{
	double dValue = 0;
/*	CTDMFServer* p;


	switch (nColumn)
	{
	
		case ColPBABUsed:
			m_mapServers.Lookup(nIndex+1, p);
			dValue = p->m_dPercentBABUsed;
		break;
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
	Function :		CTDMFServersView::GetValueBool
	Description :	If value is in a column boolean get the value from the object
	Return :			bool	-	
	Parameters :
		UINT nIndex	-	number of the object in the list
		int nColumn	-	index of the column
	Note :
\*---------------------------------------------------------------------------*/
bool CTDMFServersView::GetValueBool(UINT nIndex, int nColumn)
{

	UINT nValue = 0;
//	CTDMFVolume* pVolume;

//	switch (nColumn)
//	{
	//	case ColAvailable:
//			m_mapServers.Lookup(nIndex+1, pVolume);
	//		nValue = pVolume->m_bAvailable;
//		break;
//	}

	if ((nValue > 0))
		return true;
	return false;

}

bool CTDMFServersView::StartView(int nSortColumn , bool bSortDirection)
{

	void* pObject = NULL;
	void* pMaps = 0;

	SVBase::StartView(pObject,&m_Sheet,pMaps,nSortColumn,bSortDirection);

	return true;
}

void	CTDMFServersView::LoadThelistofServers()
{

	DeleteAllServers();

	CString strValue;
	for (int i = 1; i<=15; i++)
	{
		strValue.Format(" %d ",i);
	 	CTDMFServer* p = new CTDMFServer();
		if (p)
		{
			p->m_nID = i;
			p->m_strServerName = _T("Server") + strValue;
			p->m_strServerIPAdress1 = _T("000.000.000.000");
			p->m_strOS_Type = _T("NT");
			p->m_strOS_Version = _T("sp1");
			p->LoadAllLogicalGroups();
			p->LoadAllVolumes();
			m_mapServers.SetAt(p->m_nID, p);  //SetAt should be in all Map<>, but check if it's already present

		}
	}

}

void CTDMFServersView::DeleteAllServers()
{
	POSITION pos = m_mapServers.GetStartPosition();
	while(pos != NULL)
	{
		 int nID;
		 CTDMFServer* p;
		 
		 m_mapServers.GetNextAssoc(pos, nID, p);
		 delete p;
	}

	m_mapServers.RemoveAll();
}

void CTDMFServersView::Update()
{
	UpdateView();
}

void CTDMFServersView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
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
// CTDMFServersView message handlers
void CTDMFServersView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}



int CTDMFServersView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (SVBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	

  	return 0;
}
