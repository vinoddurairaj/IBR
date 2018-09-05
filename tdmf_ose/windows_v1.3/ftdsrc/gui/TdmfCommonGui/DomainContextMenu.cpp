// DomainContextMenu.cpp: implementation of the CDomainContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ViewNotification.h"
#include "DomainContextMenu.h"
#include "DomainPropertySheet.h"
#include "SeverSelectionDialog.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDomainContextMenu::CDomainContextMenu(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IDomain* pDomain)
	: CContextMenuBase(pDoc), m_pDomain(pDomain)
{
}

CDomainContextMenu::~CDomainContextMenu()
{
}

void CDomainContextMenu::AddEntries()
{
	m_Menu.AppendMenu(MF_STRING | (m_pDoc->GetReadOnlyFlag()) ? MF_GRAYED : MF_ENABLED, ID_SERVERS, "&Insert Servers...");
	m_Menu.AppendMenu(MF_SEPARATOR);
	m_Menu.AppendMenu(MF_STRING | (m_pDoc->GetReadOnlyFlag()) ? MF_GRAYED : MF_ENABLED, ID_DELETE, "&Delete");
	m_Menu.AppendMenu(MF_SEPARATOR);
	m_Menu.AppendMenu(MF_STRING | (m_pDoc->GetReadOnlyFlag()) ? MF_GRAYED : MF_ENABLED, ID_PROPERTIES, "&Properties");

	if (!m_pDoc->GetConnectedFlag())
	{
		m_Menu.EnableMenuItem(ID_SERVERS,    MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		m_Menu.EnableMenuItem(ID_DELETE,     MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		m_Menu.EnableMenuItem(ID_PROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
}

void CDomainContextMenu::OnCommand(UINT nCommand)
{
	switch (nCommand)
	{
	case ID_SERVERS: // Server Selection
		{
			CSeverSelectionDialog SeverSelectionDialog(m_pDomain, m_pDoc);
			SeverSelectionDialog.DoModal();
		}
		break;

	case ID_DELETE:  // Delete
		try
		{
			CString strMsg;
			strMsg.Format("Are you sure you want to delete the domain '%S'?\n",
						  (BSTR)m_pDomain->Name);

			if (m_pDomain->ServerCount > 0)
			{
				strMsg += "\nAll the contained servers will be moved to the 'unassigned' domain.";
			}
			
			if (MessageBox(GetActiveWindow(), strMsg, "Confirm Domain Delete",
				MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
			{
				CWaitCursor wait;
				
				// First check if the deteted domain contains server.
				// If so, check if the 'unassigned' domain exist
				if (m_pDomain->ServerCount > 0)
				{
					long nDomainCount = m_pDoc->m_pSystem->DomainCount;
					TDMFOBJECTSLib::IDomainPtr pDomain;
					for (long nIndex = 0; nIndex < nDomainCount; nIndex++)
					{
						pDomain = m_pDoc->m_pSystem->GetDomain(nIndex);
						if (pDomain->Name == _bstr_t("unassigned domain"))
						{
							break;
						}
					}
					if (nIndex == nDomainCount)
					{
						// Domain doesn't exist so create it
						pDomain = m_pDoc->m_pSystem->CreateNewDomain();
						pDomain->Name = "unassigned domain";
						pDomain->Description = "Container for all the servers that "
							"are not assigned to a domain.";
						// Notify views
						CViewNotification VN;
						VN.m_nMessageId = CViewNotification::DOMAIN_ADD;
						VN.m_pUnk = (IUnknown*)pDomain;
						m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);					
					}
					// Move contained servers to the 'unassigned' domain
					for (nIndex = m_pDomain->ServerCount - 1; nIndex >= 0; nIndex--)
					{
						TDMFOBJECTSLib::IServerPtr pServer = m_pDomain->GetServer(nIndex);
						
						// Move server to its new destination domain
						if (pServer->MoveTo(pDomain) != 0)
						{
							MessageBox(GetActiveWindow(), "Cannot save new server's domain in database.", "Error", MB_OK | MB_ICONERROR);
						}
						
						// Re-Notify views that the server has been inserted in the new domain
						CViewNotification VN;
						VN.m_nMessageId = CViewNotification::SERVER_ADD;
						VN.m_pUnk = pServer;
						m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
						
						// Also re-add all its logical groups
						for (long nIndexGroup = 0; nIndexGroup < pServer->ReplicationGroupCount; nIndexGroup++)
						{
							TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(nIndexGroup); 
							
							VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_ADD;
							VN.m_pUnk = (IUnknown*) pRG;
							m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
						}
					}
				}

				// Remove domain
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::DOMAIN_REMOVE;
				VN.m_pUnk = (IUnknown*)m_pDomain;
				m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
				
				TDMFOBJECTSLib::ISystemPtr pSystem = m_pDomain->Parent;
				if (pSystem->RemoveDomain(m_pDomain) != 0)
				{
					MessageBox(GetActiveWindow(), "Cannot delete domain from database.", "Error", MB_OK | MB_ICONERROR);
				}
			}
		}
		CATCH_ALL_LOG_ERROR(1000);
		break;

	case ID_PROPERTIES:  // Properties
		try
		{
			_bstr_t bstrNameOld = m_pDomain->Name;
			_bstr_t bstrTitle = bstrNameOld + " Properties";

			CDomainPropertySheet DomainPropertySheet(m_pDoc, bstrTitle, NULL, 0, m_pDomain);
			if (DomainPropertySheet.DoModal() == IDOK)
			{
				// The actions are handled in the property sheet class
			}
		}
		CATCH_ALL_LOG_ERROR(1001);
		break;

	default:
		ASSERT(0);
	}
}
