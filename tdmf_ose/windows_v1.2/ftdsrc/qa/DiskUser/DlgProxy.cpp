// DlgProxy.cpp : implementation file
//

#include "stdafx.h"
#include "DiskUser.h"
#include "DlgProxy.h"
#include "DiskUserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiskUserDlgAutoProxy

IMPLEMENT_DYNCREATE(CDiskUserDlgAutoProxy, CCmdTarget)

CDiskUserDlgAutoProxy::CDiskUserDlgAutoProxy()
{
	EnableAutomation();
	
	// To keep the application running as long as an automation 
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT (AfxGetApp()->m_pMainWnd != NULL);
	ASSERT_VALID (AfxGetApp()->m_pMainWnd);
	ASSERT_KINDOF(CDiskUserDlg, AfxGetApp()->m_pMainWnd);
	m_pDialog = (CDiskUserDlg*) AfxGetApp()->m_pMainWnd;
	m_pDialog->m_pAutoProxy = this;
}

CDiskUserDlgAutoProxy::~CDiskUserDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	if (m_pDialog != NULL)
		m_pDialog->m_pAutoProxy = NULL;
	AfxOleUnlockApp();
}

void CDiskUserDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CDiskUserDlgAutoProxy, CCmdTarget)
	//{{AFX_MSG_MAP(CDiskUserDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CDiskUserDlgAutoProxy, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CDiskUserDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDiskUser to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {505EF922-49C9-4DD9-A4CF-1D5AA9418CA3}
static const IID IID_IDiskUser =
{ 0x505ef922, 0x49c9, 0x4dd9, { 0xa4, 0xcf, 0x1d, 0x5a, 0xa9, 0x41, 0x8c, 0xa3 } };

BEGIN_INTERFACE_MAP(CDiskUserDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CDiskUserDlgAutoProxy, IID_IDiskUser, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in StdAfx.h of this project
// {A60210E5-28D9-4020-8099-FBCE8CC4CEEC}
IMPLEMENT_OLECREATE2(CDiskUserDlgAutoProxy, "DiskUser.Application", 0xa60210e5, 0x28d9, 0x4020, 0x80, 0x99, 0xfb, 0xce, 0x8c, 0xc4, 0xce, 0xec)

/////////////////////////////////////////////////////////////////////////////
// CDiskUserDlgAutoProxy message handlers
