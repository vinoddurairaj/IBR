// SeverSelectionDialog.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "SeverSelectionDialog.h"
#include "ViewNotification.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSeverSelectionDialog dialog

CSeverSelectionDialog::CSeverSelectionDialog(TDMFOBJECTSLib::IDomain* pDomain, CTdmfCommonGuiDoc* pDoc, CWnd* pParent /*=NULL*/)
	: CDialog(CSeverSelectionDialog::IDD, pParent), m_pDomain(pDomain), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(CSeverSelectionDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CSeverSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSeverSelectionDialog)
	DDX_Control(pDX, IDC_LIST_SERVER, m_ListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSeverSelectionDialog, CDialog)
	//{{AFX_MSG_MAP(CSeverSelectionDialog)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_SERVER, OnDeleteitemListServer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSeverSelectionDialog message handlers

BOOL CSeverSelectionDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	try
	{
		// Gain a reference to the list control itself
		m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
		
		// Insert columns.
		m_ListCtrl.InsertColumn(0, _T("Domain              "));
		m_ListCtrl.InsertColumn(1, _T("Name                "));
		m_ListCtrl.InsertColumn(2, _T("Description         "));
		
		// Set reasonable widths for our columns
		m_ListCtrl.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
		m_ListCtrl.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
		m_ListCtrl.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
		
		// Populate list
		TDMFOBJECTSLib::ISystemPtr pSystem = m_pDomain->Parent;
		int nIndex = 0;
		long nNbDomain = pSystem->DomainCount;
		for (long i = 0; i < nNbDomain; i++)
		{
			TDMFOBJECTSLib::IDomainPtr pDomain = pSystem->GetDomain(i);
			
			if (pDomain->IsEqual(m_pDomain) == FALSE)
			{
				long nNbServer = pDomain->ServerCount;
				for (long i = 0; i < nNbServer; i++)
				{
					TDMFOBJECTSLib::IServerPtr pServer = pDomain->GetServer(i);
					
					nIndex = m_ListCtrl.InsertItem(nIndex+1, pDomain->Name);
					m_ListCtrl.SetItemText(nIndex, 1, pServer->Name);
					m_ListCtrl.SetItemText(nIndex, 2, pServer->Description);
					m_ListCtrl.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IServer*)pServer));
					pServer->AddRef();
				}
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1073);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSeverSelectionDialog::GetServerDependencies(TDMFOBJECTSLib::IServerPtr pServer,
												  std::map<long, CAdapt<TDMFOBJECTSLib::IServerPtr> >& mapServerDependent)
{
	for (int i = 0; i < pServer->ReplicationGroupCount; i++)
	{
		TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(i);
		if ((pRG->TargetServer->Key != pServer->Key) &&
			(mapServerDependent.find(pRG->TargetServer->Key) == mapServerDependent.end()))
		{
			mapServerDependent[pRG->TargetServer->GetKey()] = pRG->TargetServer;
			GetServerDependencies(pRG->TargetServer, mapServerDependent);
		}
	}

}

void CSeverSelectionDialog::OnOK() 
{
	CWaitCursor WaitCursor;

	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	try
	{
		if (pos != NULL)
		{
			CViewNotification VN;

			while (pos)
			{
				int nItem = m_ListCtrl.GetNextSelectedItem(pos);
				TDMFOBJECTSLib::IServerPtr pServer = (TDMFOBJECTSLib::IServer*)m_ListCtrl.GetItemData(nItem);

				std::map<long, CAdapt<TDMFOBJECTSLib::IServerPtr> > mapServer;
				mapServer[pServer->GetKey()] = pServer;
				// Get server's dependencies
				GetServerDependencies(pServer, mapServer);

				bool bContinue = true;
				if (mapServer.size() > 1)
				{
					CString cstrMsg;
					cstrMsg.Format("Moving '%S' will also move the following servers:\n\n", (BSTR)pServer->Name);

					std::map<long, CAdapt<TDMFOBJECTSLib::IServerPtr> >::iterator it;
					for (it = mapServer.begin(); it != mapServer.end(); it++)
					{
						if (it->second.m_T->Name != pServer->Name)
						{
							CString cstrServer;
							cstrServer.Format("  %S\n", (BSTR)(it->second.m_T->Name));
							cstrMsg += cstrServer;
						}
					}
					cstrMsg += "\nAre you sure you want to continue?";
					
					if (MessageBox(cstrMsg, "Move Confirmation", MB_YESNO | MB_ICONINFORMATION) != IDYES)
					{
						bContinue = false;
					}
				}

				if (bContinue)
				{
					std::map<long, CAdapt<TDMFOBJECTSLib::IServerPtr> >::iterator it;
					for (it = mapServer.begin(); it != mapServer.end(); it++)
					{
						// Notify views that we'll remove the server from a domain
						VN.m_nMessageId = CViewNotification::SERVER_REMOVE;
						VN.m_pUnk = it->second.m_T;
						m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
						
						// Move server to its new destination domain
						if (it->second.m_T->MoveTo(m_pDomain) != 0)
						{
							MessageBox("Cannot save new server's domain in database.", "Error", MB_OK | MB_ICONERROR);
						}
					}
					
					// Notify views
					for (it = mapServer.begin(); it != mapServer.end(); it++)
					{
						// Re-Notify views that the server(s) has been inserted in a new domain
						VN.m_nMessageId = CViewNotification::SERVER_ADD;
						VN.m_pUnk = it->second.m_T;
						m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
						
						// Also re-add all its logical groups
						for (long nIndex = 0; nIndex < it->second.m_T->ReplicationGroupCount; nIndex++)
						{
							TDMFOBJECTSLib::IReplicationGroupPtr pRG = it->second.m_T->GetReplicationGroup(nIndex); 
							
							VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_ADD;
							VN.m_pUnk = (IUnknown*) pRG;
							m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
						}
					}
				}
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1074);

	CDialog::OnOK();
}

void CSeverSelectionDialog::OnDeleteitemListServer(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	try
	{
		IUnknown* pUnk = (IUnknown*)m_ListCtrl.GetItemData(pNMListView->iItem);
		pUnk->Release();
	}
	CATCH_ALL_LOG_ERROR(1075);

	*pResult = 0;
}
