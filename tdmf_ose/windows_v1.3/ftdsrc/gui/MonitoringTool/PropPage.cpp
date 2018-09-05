// PropPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MonitorRes.h"
#include "PropPage.h"
#include "ViewHostTop.h"
#include "ViewHostBottom.h"
#include "ViewPairTop.h"
#include "ViewPairBottom.h"
#include "ViewOverviewTop.h"
#include "ViewOverviewBottom.h"
#include "TDMFMonitorView.h"
#include "MonitorFilterView.h"
#include "TDMFEventView.h"
#include "TDMFServersView.h"
#include "BlankView.h"
#include "DetailsServerPage.h"
#include "RightTopToolbarView.h"
#include "RightBottomToolbarView.h"
#include "TDMFLogicalGroupsView.h"
#include "TDMFServerEventView.h"
#include "ServerPerformanceView.h"
#include "PerformanceToolbarView.h"
#include "ServerCommandView.h"
#include "ReplicationPairView.h"
#include "RightBottomView.h"

/* TEMPORARY */ #include "TestView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(PropPg_Replications, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_Events, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_DetailsServer, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_ReplicationGroups, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_ServerEvents, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_Performance, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_ServerCommand, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_Monitoring, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_Administration, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_ServerVolumes, GenericPropPage)

IMPLEMENT_DYNCREATE(PropPg_Overview, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_1, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_5, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_Monitor, GenericPropPage)
IMPLEMENT_DYNCREATE(PropPg_Host, GenericPropPage)

/////////////////////////////////////////////////////////////////////////////
// PropPg_Replications property page

PropPg_Replications::PropPg_Replications()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightTopToolbarView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CTDMFServersView);
}

PropPg_Replications::~PropPg_Replications()
{
}

BOOL PropPg_Replications::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_Replications::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Replications)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Replications, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Replications)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_Replications::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	GenericPropPage::OnShowWindow(bShow, nStatus);
	

	
}

void PropPg_Replications::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
}

void PropPg_Replications::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
	if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

BOOL PropPg_Replications::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_Events property page

PropPg_Events::PropPg_Events()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightTopToolbarView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CTDMFEventView);
}

PropPg_Events::~PropPg_Events()
{
}

BOOL PropPg_Events::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_Events::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Events)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Events, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Events)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_Events::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
		if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

void PropPg_Events::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
		if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

BOOL PropPg_Events::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}
/////////////////////////////////////////////////////////////////////////////
// PropPg_DetailsServer property page

PropPg_DetailsServer::PropPg_DetailsServer()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightBottomToolbarView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CDetailsServerView);
}

PropPg_DetailsServer::~PropPg_DetailsServer()
{
}

BOOL PropPg_DetailsServer::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_DetailsServer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_DetailsServer)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_DetailsServer, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_DetailsServer)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL PropPg_DetailsServer::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_ReplicationGroups property page

PropPg_ReplicationGroups::PropPg_ReplicationGroups()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightTopToolbarView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CTDMFLogicalGroupsView);
}

PropPg_ReplicationGroups::~PropPg_ReplicationGroups()
{
}

BOOL PropPg_ReplicationGroups::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_ReplicationGroups::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_ReplicationGroups)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_ReplicationGroups, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_ReplicationGroups)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_ReplicationGroups::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
}

void PropPg_ReplicationGroups::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
	if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

BOOL PropPg_ReplicationGroups::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_ServerEvents property page

PropPg_ServerEvents::PropPg_ServerEvents()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightBottomToolbarView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CTDMFServerEventView);
}

PropPg_ServerEvents::~PropPg_ServerEvents()
{
}

BOOL PropPg_ServerEvents::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_ServerEvents::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_ServerEvents)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_ServerEvents, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_ServerEvents)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_ServerEvents::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
		if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

void PropPg_ServerEvents::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
		if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

BOOL PropPg_ServerEvents::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_Performance property page

PropPg_Performance::PropPg_Performance()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CPerformanceToolbarView);
   m_aPanel[1][0].pCRuntimeClass = RUNTIME_CLASS(CServerPerformanceView);

}

PropPg_Performance::~PropPg_Performance()
{
}

BOOL PropPg_Performance::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);

	return bRet;
}

void PropPg_Performance::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Performance)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Performance, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Performance)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_Performance::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}
	
}

void PropPg_Performance::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
if(m_PageSplitter.GetRowCount() > 1)
	{
		m_PageSplitter.SetRowInfo(0,30,0);
		m_PageSplitter.SetRowInfo(1,300,0);
		m_PageSplitter.RecalcLayout( );
	}

}

BOOL PropPg_Performance::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_ServerCommand property page

PropPg_ServerCommand::PropPg_ServerCommand()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightBottomToolbarView);
   m_aPanel[1][0].pCRuntimeClass = RUNTIME_CLASS(CServerCommandView);

}

PropPg_ServerCommand::~PropPg_ServerCommand()
{
}

BOOL PropPg_ServerCommand::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);

	return bRet;
}

void PropPg_ServerCommand::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_ServerCommand)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_ServerCommand, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_ServerCommand)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_ServerCommand::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,30,0);
			m_PageSplitter.SetRowInfo(1,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}

	
}

void PropPg_ServerCommand::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,30,0);
			m_PageSplitter.SetRowInfo(1,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}


}

BOOL PropPg_ServerCommand::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_Monitoring property page

PropPg_Monitoring::PropPg_Monitoring()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CTDMFServersView);
   m_aPanel[1][0].pCRuntimeClass = RUNTIME_CLASS(CTDMFLogicalGroupsView);
	m_aPanel[2][0].pCRuntimeClass = RUNTIME_CLASS(CRightBottomView);

}

PropPg_Monitoring::~PropPg_Monitoring()
{
}

BOOL PropPg_Monitoring::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);

	return bRet;
}

void PropPg_Monitoring::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Monitoring)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Monitoring, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Monitoring)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_Monitoring::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,100,0);
			m_PageSplitter.SetRowInfo(1,200,0);
			m_PageSplitter.SetRowInfo(2,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}

	
}

void PropPg_Monitoring::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,100,0);
			m_PageSplitter.SetRowInfo(1,200,0);
			m_PageSplitter.SetRowInfo(2,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}


}

BOOL PropPg_Monitoring::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_Administration property page

PropPg_Administration::PropPg_Administration()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CTDMFServerEventView);
   m_aPanel[1][0].pCRuntimeClass = RUNTIME_CLASS(CBlankView);
}

PropPg_Administration::~PropPg_Administration()
{
}

BOOL PropPg_Administration::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);

	return bRet;
}

void PropPg_Administration::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Administration)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Administration, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Administration)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_Administration::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,100,0);
			m_PageSplitter.SetRowInfo(1,200,0);
			m_PageSplitter.RecalcLayout( );
		}
	}
	
}

void PropPg_Administration::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,100,0);
			m_PageSplitter.SetRowInfo(1,200,0);
			m_PageSplitter.RecalcLayout( );
		}
	}

}

BOOL PropPg_Administration::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_ServerVolumes property page

PropPg_ServerVolumes::PropPg_ServerVolumes()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CRightTopToolbarView);
   m_aPanel[1][0].pCRuntimeClass = RUNTIME_CLASS(CTDMFReplicationPairView);

}

PropPg_ServerVolumes::~PropPg_ServerVolumes()
{
}

BOOL PropPg_ServerVolumes::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);

	return bRet;
}

void PropPg_ServerVolumes::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_ServerVolumes)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_ServerVolumes, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_ServerVolumes)
	ON_WM_SIZE()
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PropPg_ServerVolumes::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,30,0);
			m_PageSplitter.SetRowInfo(1,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}

	
}

BOOL PropPg_ServerVolumes::OnSetActive() 
{
  GenericPropPage::OnSetActive();

  Update();

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_Overview property page

PropPg_Overview::PropPg_Overview()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(ViewOverviewTop);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(ViewOverviewBottom);

}

PropPg_Overview::~PropPg_Overview()
{
}

BOOL PropPg_Overview::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_Overview::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Overview)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Overview, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Overview)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





/////////////////////////////////////////////////////////////////////////////
// PropPg_5 property page

PropPg_5::PropPg_5()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[0][2].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[0][3].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[0][4].pCRuntimeClass = RUNTIME_CLASS(TestView);

   m_aPanel[1][0].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[1][1].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[1][2].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[1][3].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[1][4].pCRuntimeClass = RUNTIME_CLASS(TestView);

   m_aPanel[2][0].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[2][1].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[2][2].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[2][3].pCRuntimeClass = RUNTIME_CLASS(TestView);
   m_aPanel[2][4].pCRuntimeClass = RUNTIME_CLASS(TestView);
}

PropPg_5::~PropPg_5()
{
}

BOOL PropPg_5::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_5::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_5)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_5, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_5)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// BOOL PropPg_5::OnInitDialog() 
// {
// 	GenericPropPage::OnInitDialog();
// 	
//    CRect rc;
//    GetClientRect(&rc);
//    m_pSplitterFrame = new SplitterFrame();
// 	BOOL b = m_pSplitterFrame->Create(NULL, _T(""), WS_VISIBLE | WS_CHILD, rc, this);
//    ASSERT(b);
// 	
// 	// return TRUE unless you set the focus to a control
// 	// EXCEPTION: OCX Property Pages should return FALSE
// 	return TRUE;
// }

/////////////////////////////////////////////////////////////////////////////
// PropPg_Monitor property page

PropPg_Monitor::PropPg_Monitor()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CMonitorFilterView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CTDMFMonitorView);
	m_aPanel[0][2].pCRuntimeClass = RUNTIME_CLASS(CTDMFEventView);
}

PropPg_Monitor::~PropPg_Monitor()
{
}

BOOL PropPg_Monitor::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_Monitor::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Monitor)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Monitor, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Monitor)
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int PropPg_Monitor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (GenericPropPage::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void PropPg_Monitor::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	GenericPropPage::OnShowWindow(bShow, nStatus);

	if(bShow)
	{
		m_PageSplitter.SetRowInfo(0,175,20);
		m_PageSplitter.SetRowInfo(1,300,20);
		m_PageSplitter.SetRowInfo(2,150,20);
		m_PageSplitter.RecalcLayout( );


	}
}

/////////////////////////////////////////////////////////////////////////////
// PropPg_Host property page

PropPg_Host::PropPg_Host()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(ViewHostTop);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(ViewHostBottom);
}

PropPg_Host::~PropPg_Host()
{
}

BOOL PropPg_Host::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_Host::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_Host)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_Host, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_Host)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// PropPg_1 property page

PropPg_1::PropPg_1()
{
   m_aPanel[0][0].pCRuntimeClass = RUNTIME_CLASS(CTDMFServersView);
   m_aPanel[0][1].pCRuntimeClass = RUNTIME_CLASS(CBlankView);
}

PropPg_1::~PropPg_1()
{
}

BOOL PropPg_1::Construct(PropSheet* pParentSheet)
{
   BOOL bRet = GenericPropPage::Construct(pParentSheet, (SPanelData*)m_aPanel,
                                          eROWS, eCOLS, eIDS);
   return bRet;
}

void PropPg_1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPg_1)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PropPg_1, GenericPropPage)
	//{{AFX_MSG_MAP(PropPg_1)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





void PropPg_DetailsServer::OnSize(UINT nType, int cx, int cy) 
{
	GenericPropPage::OnSize(nType, cx, cy);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,300,0);
			m_PageSplitter.SetRowInfo(1,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}
}

void PropPg_DetailsServer::OnSizing(UINT fwSide, LPRECT pRect) 
{
	GenericPropPage::OnSizing(fwSide, pRect);
	
	if(m_PageSplitter)
	{
		if(m_PageSplitter.GetRowCount() > 1)
		{
			m_PageSplitter.SetRowInfo(0,300,0);
			m_PageSplitter.SetRowInfo(1,300,0);
			m_PageSplitter.RecalcLayout( );
		}
	}
	
}



