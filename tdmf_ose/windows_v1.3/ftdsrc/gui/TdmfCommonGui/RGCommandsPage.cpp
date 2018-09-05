// RGCommandsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "RGCommandsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CRGCommandsPage property page

IMPLEMENT_DYNCREATE(CRGCommandsPage, CServerCommandsPage)

CRGCommandsPage::CRGCommandsPage()
{
	//{{AFX_DATA_INIT(CRGCommandsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bServer = false;
}

CRGCommandsPage::~CRGCommandsPage()
{
}

void CRGCommandsPage::FillCommandVector()
{
	m_vecStCmd.clear();

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	if (pDoc->GetSelectedReplicationGroup()->IsSource)
	{
		PushBackCommand("checkpoint",    "[-g+#{group}|-a]+[-p|-s]+[-on|-off]",TDMFOBJECTSLib::CMD_CHECKPOINT);
		PushBackCommand("info",          "[-g+#{group}|-a|-v]",       TDMFOBJECTSLib::CMD_INFO);
		PushBackCommand("killbackfresh", "[-g+#{group}|-a|-h]",       TDMFOBJECTSLib::CMD_KILL_BACKFRESH);
		PushBackCommand("killpmd",       "[-g+#{group}|-a|-h]",       TDMFOBJECTSLib::CMD_KILL_PMD);
		PushBackCommand("killrefresh",   "[-g+#{group}|-a|-h]",       TDMFOBJECTSLib::CMD_KILL_REFRESH);
		PushBackCommand("killrmd",       "[-g+#{group}|-a]",          TDMFOBJECTSLib::CMD_KILL_RMD);
		PushBackCommand("launchbackfresh", "[-g+#{group}|-a]",        TDMFOBJECTSLib::CMD_LAUNCH_BACKFRESH);
		PushBackCommand("launchpmd",     "[-g+#{group}|-a]",          TDMFOBJECTSLib::CMD_LAUNCH_PMD);
		PushBackCommand("launchrefresh", "[-g+#{group}&[-f|-c]|-a]&[-f|-c]",TDMFOBJECTSLib::CMD_LAUNCH_REFRESH);
		PushBackCommand("override",      "[-g+#{group}|-a]+[clear BAB]|[state+passthru|normal|tracking|refresh|backfresh]",TDMFOBJECTSLib::CMD_OVERRIDE);
		PushBackCommand("panalyze",      "[-g+#{group}|-v]",          TDMFOBJECTSLib::CMD_PANALYZE);
		PushBackCommand("reco",          "[-g+#{group}|-a]+[-d]",     TDMFOBJECTSLib::CMD_RECO);
		PushBackCommand("set",           "[-g+#{group}]+[{parameter_name}={value}]",TDMFOBJECTSLib::CMD_SET);
		PushBackCommand("start",         "[-g+#{group}|-a][-b][-f]",  TDMFOBJECTSLib::CMD_START);
		PushBackCommand("stop",          "[-g+#{group}|-a|-s]",       TDMFOBJECTSLib::CMD_STOP);
	}
	else
	{
		PushBackCommand("checkpoint",    "[-g+#{group}|-a]+[-p|-s][-on|-off]",TDMFOBJECTSLib::CMD_CHECKPOINT);
	}
}

CString CRGCommandsPage::GetDefaultArg(int nItemIndex)
{
	CString strArg;

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	if (pDoc->GetSelectedReplicationGroup()->IsSource)
	{
		if (m_vecStCmd[nItemIndex].strCmd == "override")
		{
			strArg.Format("-g %d", pDoc->GetSelectedReplicationGroup()->GroupNumber);
		}
		else
		{
			strArg.Format("-g%d", pDoc->GetSelectedReplicationGroup()->GroupNumber);
		}
	}
	else
	{
		strArg.Format("-g%d -s", pDoc->GetSelectedReplicationGroup()->GroupNumber);
	}

	return strArg;
}

void CRGCommandsPage::DoDataExchange(CDataExchange* pDX)
{
	CServerCommandsPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGCommandsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGCommandsPage, CServerCommandsPage)
	//{{AFX_MSG_MAP(CRGCommandsPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGCommandsPage message handlers
