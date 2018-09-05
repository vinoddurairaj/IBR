    // SystemTestWnd.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfobjects.h"
#include "SystemTestWnd.h"
#include "System.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//

const UINT WM_TDMF_STATE_EVENT   = ::RegisterWindowMessage(_T("TdmfStateEvent"));
const UINT WM_TDMF_SERVER_EVENT  = ::RegisterWindowMessage(_T("TdmfServerEvent"));
const UINT WM_TDMF_PERF_EVENT    = ::RegisterWindowMessage(_T("TdmfPerfEvent"));
const UINT WM_TDMF_NEW_DOMAIN    = ::RegisterWindowMessage(_T("TdmfNewDomain"));
const UINT WM_TDMF_REMOVE_DOMAIN = ::RegisterWindowMessage(_T("TdmfRemoveDomain"));
const UINT WM_TDMF_MODIFY_DOMAIN = ::RegisterWindowMessage(_T("TdmfModifyDomain"));
const UINT WM_TDMF_NEW_SERVER    = ::RegisterWindowMessage(_T("TdmfNewServer"));
const UINT WM_TDMF_REMOVE_SERVER = ::RegisterWindowMessage(_T("TdmfRemoveServer"));
const UINT WM_TDMF_MODIFY_SERVER = ::RegisterWindowMessage(_T("TdmfModifyServer"));
const UINT WM_TDMF_NEW_GROUP     = ::RegisterWindowMessage(_T("TdmfNewGroup"));
const UINT WM_TDMF_REMOVE_GROUP  = ::RegisterWindowMessage(_T("TdmfRemoveGroup"));
const UINT WM_TDMF_MODIFY_GROUP  = ::RegisterWindowMessage(_T("TdmfModifyGroup"));


/////////////////////////////////////////////////////////////////////////////
// CSystemTestWnd

CSystemTestWnd::CSystemTestWnd(CSystem* pSystem) : m_pSystem(pSystem)
{
}

CSystemTestWnd::~CSystemTestWnd()
{
}


BEGIN_MESSAGE_MAP(CSystemTestWnd, CWnd)
	//{{AFX_MSG_MAP(CSystemTestWnd)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(WM_TDMF_STATE_EVENT,  OnTdmfStateEvent)
	ON_REGISTERED_MESSAGE(WM_TDMF_SERVER_EVENT, OnTdmfServerEvent)
	ON_REGISTERED_MESSAGE(WM_TDMF_PERF_EVENT,   OnTdmfPerfEvent)

	ON_REGISTERED_MESSAGE(WM_TDMF_NEW_DOMAIN,    OnTdmfAddNewDomain)
	ON_REGISTERED_MESSAGE(WM_TDMF_REMOVE_DOMAIN, OnTdmfRemoveDomain)
	ON_REGISTERED_MESSAGE(WM_TDMF_MODIFY_DOMAIN, OnTdmfModifyDomain)

	ON_REGISTERED_MESSAGE(WM_TDMF_NEW_SERVER,    OnTdmfAddNewServer)
	ON_REGISTERED_MESSAGE(WM_TDMF_REMOVE_SERVER, OnTdmfRemoveServer)
	ON_REGISTERED_MESSAGE(WM_TDMF_MODIFY_SERVER, OnTdmfModifyServer)

	ON_REGISTERED_MESSAGE(WM_TDMF_NEW_GROUP,    OnTdmfAddNewGroup)
	ON_REGISTERED_MESSAGE(WM_TDMF_REMOVE_GROUP, OnTdmfRemoveGroup)
	ON_REGISTERED_MESSAGE(WM_TDMF_MODIFY_GROUP, OnTdmfModifyGroup)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSystemTestWnd message handlers

LRESULT CSystemTestWnd::OnTdmfStateEvent(WPARAM wParam, LPARAM lParam)
{
	long nMode = HIWORD(lParam) >> 8;
	long nConnectionState = (HIWORD(lParam) & 0x0F) - 1;

	m_pSystem->UpdateModeAndStatus(0, wParam, LOWORD(lParam), nMode, nConnectionState, 0, 0, 0, 0, 0, 0, 0);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfServerEvent(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->UpdateServerConnectionState(wParam, lParam, 0, 0);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfPerfEvent(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->UpdatePerformanceData(0, LOWORD(wParam), HIWORD(wParam),
									 0, 0, 0, 0, LOWORD(lParam), HIWORD(lParam), 0, 0);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfAddNewDomain(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->AddDomain(wParam);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfRemoveDomain(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->RemoveDomain(wParam);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfModifyDomain(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->ModifyDomain(wParam);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfAddNewServer(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->AddServerToDomain(wParam, lParam);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfRemoveServer(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->RemoveServerFromDomain(wParam, lParam);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfModifyServer(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->ModifyServerInDomain(wParam, lParam);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfAddNewGroup(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->AddGroupToServerInDomain(wParam, lParam >> 16, lParam & 0xffff);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfRemoveGroup(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->RemoveGroupFromServerInDomain(wParam, lParam >> 16, lParam & 0xffff);

	return 0;
}

LRESULT CSystemTestWnd::OnTdmfModifyGroup(WPARAM wParam, LPARAM lParam)
{
	m_pSystem->ModifyGroupInServerInDomain(wParam, lParam >> 16, lParam & 0xffff);

	return 0;
}
