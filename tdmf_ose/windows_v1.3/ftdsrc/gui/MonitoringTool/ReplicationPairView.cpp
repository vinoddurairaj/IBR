// CTDMFReplicationPairView.cpp: implementation of the CTDMFReplicationPairView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TDMFGUI.h"
#include "Doc.h"
#include "ReplicationPairView.h"
#include "MainFrm.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
IMPLEMENT_DYNCREATE(CTDMFReplicationPairView, SVBase)


// These defines must be in the same order as the columns in SetColumnDefaults.


#define ColRepPairID			0
#define ColNotes				1
#define ColSourceDisk		2
#define ColSourceDisk1		3
#define ColSourceDisk2		4
#define ColSourceDisk3		5
#define ColTargetDisk		6
#define ColTargetDisk1		7
#define ColTargetDisk2		8
#define ColTargetDisk3		9



const _TCHAR tcUndefined[] = _T("UNDEFINED");
const _TCHAR tcNotImplemented[] = _T("Not Implemented");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CTDMFReplicationPairView construction/destruction

CTDMFReplicationPairView::CTDMFReplicationPairView()
{
	m_pActiveSheet = &m_Sheet;
	m_bFirstUpdate = true;
}

CTDMFReplicationPairView::~CTDMFReplicationPairView()
{
	DeleteAllReplicationPairs();
}


BEGIN_MESSAGE_MAP(CTDMFReplicationPairView, SVBase)
	//{{AFX_MSG_MAP(CTDMFReplicationPairView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, SVBase::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, SVBase::OnFilePrintPreview)
END_MESSAGE_MAP()


void CTDMFReplicationPairView::InitialyzeTheView()
{
  	if (m_bFirstUpdate)		// new sheet, use defaults
	{
		SetColumnDefaults();

		if(m_pActiveSheet->caColumns.GetSize() > 0)
		AddColumns(m_pActiveSheet);

		m_bFirstUpdate = false;
	}

}

void CTDMFReplicationPairView::OnInitialUpdate()
{
	SVBase::OnInitialUpdate();

   InitialyzeTheView();
}

void CTDMFReplicationPairView::Update()
{
	UpdateView();
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFReplicationPairView printing

BOOL CTDMFReplicationPairView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTDMFReplicationPairView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTDMFReplicationPairView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTDMFReplicationPairView diagnostics

#ifdef _DEBUG
void CTDMFReplicationPairView::AssertValid() const
{
	SVBase::AssertValid();
}

void CTDMFReplicationPairView::Dump(CDumpContext& dc) const
{
	SVBase::Dump(dc);
}
#endif //_DEBUG

bool CTDMFReplicationPairView::UpdateView()	// new collection
{

	LoadThelistofReplicationPairs();

	int nNumObjects = m_mapReplicationPairs.GetCount(); 

	DeleteAllRows();

	return AddRows(nNumObjects);
}


bool CTDMFReplicationPairView::AddRows(int nNumObjects)	
{
	if (nNumObjects <= 0)
		return false;

	for (int i=0; i< nNumObjects; i++)
		AddRow(i);

	return true;

}


void CTDMFReplicationPairView::SetColumnDefaults()
{


	m_pActiveSheet->caColumns.RemoveAll();	

	SVColumn svCol;

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctInt;
	svCol.ut = utNone;
	svCol.csHeaderText = _T("Replication Pair ID"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Notes"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Source disk"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Source disk1"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Source disk2"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Source disk3"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Target disk"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Target disk1"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Target disk2"); 
	m_pActiveSheet->caColumns.Add(svCol);

	svCol.Clear();
	svCol.nPixelWidth = 120;
	svCol.ct = ctChar;
	svCol.csHeaderText = _T("Target disk3"); 
	m_pActiveSheet->caColumns.Add(svCol);

	m_pActiveSheet->bUseDefaults = false;

}



CString CTDMFReplicationPairView::GetValueString(UINT nIndex, int nColumn)
{
	CString strValue = tcNotImplemented;
	CTDMFReplicationPair* p;
   
	m_mapReplicationPairs.Lookup(nIndex+1, p);
	if(p == NULL)
	{
     return strValue;
	}

	switch (nColumn)
	{
		case ColNotes:
		strValue = p->m_strNotes;
			break;
		case ColSourceDisk:
			strValue = p->m_strSourceDisk;
			break;
		case ColSourceDisk1:
			strValue = p->m_strSourceDisk1;
			break;
		case ColSourceDisk2:
			strValue = p->m_strSourceDisk2;
			break;
		case ColSourceDisk3:
			strValue = p->m_strSourceDisk3;
			break;
		case ColTargetDisk:
			strValue = p->m_strTargetDisk;
			break;
		case ColTargetDisk1:
			strValue = p->m_strTargetDisk1;
			break;
		case ColTargetDisk2:
			strValue = p->m_strTargetDisk2;
			break;
		case ColTargetDisk3:
			strValue = p->m_strTargetDisk3;
			break;
		default:
			strValue = tcNotImplemented;
			break;
	}

	return strValue;
} 

COleDateTime CTDMFReplicationPairView::GetValueDateTime(UINT nIndex, int nColumn)
{
/*	switch (nColumn)
	{
		case ColCrnt_DateTime:
			return COleDateTime::GetCurrentTime();
			break;
	}
*/	COleDateTime cdt;
	return cdt;

}

int CTDMFReplicationPairView::GetValueInt(UINT nIndex,int nColumn)
{
	int nValue = 0;
	CTDMFReplicationPair* p;

	m_mapReplicationPairs.Lookup(nIndex+1, p);
	if(p == NULL)
	{
     return nValue;
	}

	switch (nColumn)
	{
		case ColRepPairID:
			nValue = p->m_nDtcID;
		break;
		default:
			nValue = 0;
		break;
	}

	return nValue;
}

bool CTDMFReplicationPairView::GetValueBool(UINT nIndex, int nColumn)
{

	UINT nValue = 0;

/*	switch (nColumn)
	{
		case ColSyncMode:
			nValue = -1;
	}
*/
	if ((nValue > 0))
		return true;
	return false;

}

double CTDMFReplicationPairView::GetValueDouble(UINT nIndex,int nColumn)
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

bool CTDMFReplicationPairView::StartView(int nSortColumn , bool bSortDirection)
{

	void* pObject = NULL;
	void* pMaps = 0;

	SVBase::StartView(pObject,&m_Sheet,pMaps,nSortColumn,bSortDirection);


	return true;
}

int CTDMFReplicationPairView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (SVBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	

	return 0;
}

void CTDMFReplicationPairView::DeleteAllReplicationPairs()
{
	POSITION pos = m_mapReplicationPairs.GetStartPosition();
	while(pos != NULL)
	{
		 int nID;
		 CTDMFReplicationPair* p;
		 
		 m_mapReplicationPairs.GetNextAssoc(pos, nID, p);
		 delete p;
	}

	m_mapReplicationPairs.RemoveAll();
}

void	CTDMFReplicationPairView::LoadThelistofReplicationPairs()
{

	DeleteAllReplicationPairs();

	CString strValue;
	for (int i = 1; i<=15; i++)
	{
		strValue.Format(" %d ",i);
	 	CTDMFReplicationPair* p = new CTDMFReplicationPair();
		if (p)
		{

			p->m_nDtcID				= i;
			p->m_strNotes			= _T("Notes");
			p->m_strSourceDisk	= _T("123456789");	
			p->m_strSourceDisk1	= _T("123456789");	
			p->m_strSourceDisk2	= _T("123456789");	
			p->m_strSourceDisk3	= _T("123456789");	

			p->m_strTargetDisk	= _T("123456789");	
			p->m_strTargetDisk1	= _T("123456789");	
			p->m_strTargetDisk2	= _T("123456789");	
			p->m_strTargetDisk3	= _T("123456789");	

			m_mapReplicationPairs.SetAt(p->m_nDtcID, p);  //SetAt should be in all Map<>, but check if it's already present

		}
	}

}

void CTDMFReplicationPairView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
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