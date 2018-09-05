// SystemContextMenu.cpp: implementation of the CSystemContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "SystemContextMenu.h"
#include "DomainPropertySheet.h"
#include "ViewNotification.h"
#include "LoginDlg.h"
#include "SystemPropertySheet.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemContextMenu::CSystemContextMenu(CTdmfCommonGuiDoc* pDoc) : 
	CContextMenuBase(pDoc)
{
}

CSystemContextMenu::~CSystemContextMenu()
{
}

void CSystemContextMenu::AddEntries()
{
	if (!m_pDoc->GetConnectedFlag())
	{
		m_Menu.AppendMenu(MF_STRING, ID_CONNECT, "Connect...");
	}
	else
	{
		m_Menu.AppendMenu(MF_STRING, ID_DISCONNECT, "Disconnect");
	}

	m_Menu.AppendMenu(MF_STRING, ID_REFRESH, "Refresh");
	m_Menu.AppendMenu(MF_SEPARATOR);

	m_Menu.AppendMenu(MF_STRING | (!m_pDoc->GetConnectedFlag() || m_pDoc->GetReadOnlyFlag()) ? MF_GRAYED : MF_ENABLED, ID_DOMAIN, "Add Domain...");
	m_Menu.AppendMenu(MF_SEPARATOR);

	m_Menu.AppendMenu(MF_STRING | (!m_pDoc->GetConnectedFlag() || m_pDoc->GetReadOnlyFlag()) ? MF_GRAYED : MF_ENABLED, ID_PROPERTIES, "&Properties...");

	if (!m_pDoc->GetConnectedFlag() || m_pDoc->GetReadOnlyFlag())
	{
		m_Menu.EnableMenuItem(ID_REFRESH, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		m_Menu.EnableMenuItem(ID_PROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
}

void CSystemContextMenu::OnCommand(UINT nCommand)
{
	switch (nCommand)
	{
	case ID_REFRESH:  // Refresh
		{
			m_pDoc->OnToolsRefresh();
		}
		break;

	case ID_DOMAIN:  // Add Domain
		{
			TDMFOBJECTSLib::IDomainPtr pDomain = m_pDoc->m_pSystem->CreateNewDomain();

			CDomainPropertySheet     DomainPropertySheet(m_pDoc, "New Domain Properties", NULL, 0, pDomain, TRUE);
			DomainPropertySheet.DoModal();
		}
		break;

	case ID_CONNECT:
		{
			m_pDoc->OnFileConnect();
		}
		break;

	case ID_DISCONNECT:
		{
			m_pDoc->OnFileDisconnect();
		}
		break;

	case ID_PROPERTIES:
		try
		{
			_bstr_t bstrTitle   = "'" + m_pDoc->m_pSystem->Name + " Collector' Properties";
			CSystemPropertySheet ServerPropertySheet(m_pDoc, bstrTitle, NULL, 0);
			if (ServerPropertySheet.DoModal() == IDOK)
			{
				// The actions are handled in the property sheet class
			}
		}
		CATCH_ALL_LOG_ERROR(1076);
		break;

	default:
		ASSERT(0);
	}
}
