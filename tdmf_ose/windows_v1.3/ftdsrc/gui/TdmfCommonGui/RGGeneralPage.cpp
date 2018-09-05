// RGGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGGeneralPage.h"
#include "RGSelectServerDialog.h"
#include "ReplicationGroupPropertySheet.h"
#include "ViewNotification.h"
#include "commctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGGeneralPage property page

IMPLEMENT_DYNCREATE(CRGGeneralPage, CPropertyPage)

CRGGeneralPage::CRGGeneralPage(TDMFOBJECTSLib::IReplicationGroup *pRG, bool bNewItem, bool bReadOnly)
	: CPropertyPage(CRGGeneralPage::IDD), m_pRG(pRG), m_bNewItem(bNewItem),
	  m_IsTargetServerModified(false), m_bReadOnly(bReadOnly)
{
	//{{AFX_DATA_INIT(CRGGeneralPage)
	m_strDescription = _T("");
	m_nGroupNb = 0;
	m_strJournal = _T("");
	m_strPStore = _T("");
	m_bChaining = FALSE;
	m_cstrPStoreTitle = _T("");
	m_cstrPrimaryName = _T("");
	m_cstrTargetName = _T("");
	m_cstrPrimaryLink = _T("");
	m_cstrTargetLink = _T("");
	//}}AFX_DATA_INIT

	if (m_pRG)
	{
        //Keep old value
        m_bPrimaryServerEditedUsedSaved   = m_pRG->IsPrimaryEditedIPUsed();
		m_bstrPrimaryServerEditedIPSaved  = m_pRG->PrimaryEditedIP;
        m_bPrimaryServerDHCPNameUsedSaved = m_pRG->IsPrimaryDHCPAdressUsed();

 		m_pTargetServerSaved              = m_pRG->TargetServer;    
        m_bTargetServerEditedUsedSaved    = m_pRG->IsTargetEditedIPUsed();
		m_bstrTargetServerEditedIPSaved   = m_pRG->TargetEditedIP;
        m_bTargetServerDHCPNameUsedSaved  = m_pRG->IsTargetDHCPAdressUsed();

		m_nGroupNbSaved                   = m_pRG->GroupNumber;
		m_bstrPStoreSaved                 = m_pRG->PStoreDirectory;
		m_bstrNameSaved                   = m_pRG->Name;
	}

  	m_bPageModified = false;
}

CRGGeneralPage::~CRGGeneralPage()
{
}

void CRGGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGGeneralPage)
	DDX_Control(pDX, IDC_COMBO_TARGET, m_ComboTarget);
	DDX_Control(pDX, IDC_COMBO_PRIMARY, m_ComboPrimary);
	DDX_Control(pDX, IDC_BUTTON_SERVER, m_ButtonServer);
	DDX_Control(pDX, IDC_EDIT_PSTORE, m_EditPStore);
	DDX_Control(pDX, IDC_EDIT_JOURNAL, m_EditJournal);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_EditDescription);
	DDX_Control(pDX, IDC_CHECK_CHAINING, m_CheckChaining);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 200);
	DDX_Text(pDX, IDC_EDIT_GROUP_NUMBER, m_nGroupNb);
	DDX_Text(pDX, IDC_EDIT_JOURNAL, m_strJournal);
	DDV_MaxChars(pDX, m_strJournal, 300);
	DDX_Text(pDX, IDC_EDIT_PSTORE, m_strPStore);
	DDV_MaxChars(pDX, m_strPStore, 300);
	DDX_Check(pDX, IDC_CHECK_CHAINING, m_bChaining);
	DDX_Text(pDX, IDC_PSTORE_TITLE, m_cstrPStoreTitle);
	DDX_Text(pDX, IDC_EDIT_PRIMARY_SERVER_NAME, m_cstrPrimaryName);
	DDX_Text(pDX, IDC_EDIT_TARGET_SERVER_NAME, m_cstrTargetName);
	DDX_CBString(pDX, IDC_COMBO_PRIMARY, m_cstrPrimaryLink);
	DDX_CBString(pDX, IDC_COMBO_TARGET, m_cstrTargetLink);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGGeneralPage)
	ON_BN_CLICKED(IDC_BUTTON_SERVER, OnSelectServerButton)
	ON_BN_CLICKED(IDC_CHECK_CHAINING, OnCheckChaining)
	ON_EN_UPDATE(IDC_EDIT_DESCRIPTION, OnUpdateEditDescription)
	ON_EN_UPDATE(IDC_EDIT_GROUP_NUMBER, OnUpdateEditGroupNumber)
	ON_EN_UPDATE(IDC_EDIT_JOURNAL, OnUpdateEditJournal)
	ON_EN_UPDATE(IDC_EDIT_PSTORE, OnUpdateEditPstore)
	ON_CBN_EDITCHANGE(IDC_COMBO_PRIMARY, OnEditchangeComboPrimary)
	ON_CBN_SELCHANGE(IDC_COMBO_PRIMARY, OnSelchangeComboPrimary)
	ON_CBN_EDITCHANGE(IDC_COMBO_TARGET, OnEditchangeComboTarget)
	ON_CBN_SELCHANGE(IDC_COMBO_TARGET, OnSelchangeComboTarget)
	ON_CBN_KILLFOCUS(IDC_COMBO_PRIMARY, OnKillfocusComboPrimary)
	ON_CBN_KILLFOCUS(IDC_COMBO_TARGET, OnKillfocusComboTarget)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGGeneralPage message handlers

BOOL CRGGeneralPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
    
    try
	{   
        m_pOriginalFont = GetFont();

       	m_bPageModified = false;
	
		if (m_pRG != NULL)
		{
			m_strDescription = (BSTR)m_pRG->Description;
			m_nGroupNb   = m_pRG->GroupNumber;

			m_cstrPrimaryName = (BSTR)m_pRG->Parent->Name;

			// Fill Primary ComboBox
			for (int i = 0; i < m_pRG->Parent->IPAddressCount; i++)
			{
				m_ComboPrimary.AddString(m_pRG->Parent->GetIPAddress(i));
			}
			m_ComboPrimary.AddString(m_cstrPrimaryName);

			if (m_pRG->IsPrimaryEditedIPUsed())
			{
				m_cstrPrimaryLink = (BSTR)m_pRG->PrimaryEditedIP;
			}
			else if (m_pRG->IsPrimaryDHCPAdressUsed())
			{
				m_cstrPrimaryLink = m_cstrPrimaryName;
			}
			else
			{
				m_cstrPrimaryLink = (BSTR)m_pRG->Parent->GetIPAddress(0);
			}

			if( int nPos = m_ComboPrimary.FindStringExact(0,m_cstrPrimaryLink) != CB_ERR )
			{
				m_ComboPrimary.SetCurSel(nPos);
			}

			m_strPStore  = (BSTR)m_pRG->PStoreDirectory;

			if (m_pRG->TargetServer != NULL)
			{
				m_cstrTargetName = (BSTR)m_pRG->TargetServer->Name;
	
				// Fill Taret ComboBox
				for (int i = 0; i < m_pRG->TargetServer->IPAddressCount; i++)
				{
					m_ComboTarget.AddString(m_pRG->TargetServer->GetIPAddress(i));
				}
				m_ComboTarget.AddString(m_cstrTargetName);
				// Loopback consideration
				if (m_pRG->Parent->IsEqual(m_pRG->TargetServer))
				{
					m_ComboPrimary.InsertString(0, "127.0.0.1");
					m_ComboTarget.InsertString(0, "127.0.0.1");
				}

				if (m_pRG->IsTargetEditedIPUsed())
				{
					m_cstrTargetLink = (BSTR)m_pRG->TargetEditedIP;
					m_ComboTarget.InsertString(0, m_cstrTargetLink);
				}
				else if (m_pRG->IsTargetDHCPAdressUsed())
				{
					m_cstrTargetLink = m_cstrTargetName;
				}
				else
				{
					m_cstrTargetLink = (BSTR)m_pRG->TargetServer->GetIPAddress(0);
				}
				
				if( int nPos = m_ComboTarget.FindStringExact(0,m_cstrTargetLink) != CB_ERR )
				{
					m_ComboTarget.SetCurSel(nPos);
				}

			}
			m_strJournal = (BSTR)m_pRG->JournalDirectory;
			m_bChaining  = m_pRG->Chaining;

			if (strstr(m_pRG->GetParent()->OSType, "Windows") != 0 ||
		    strstr(m_pRG->GetParent()->OSType, "windows") != 0 ||
			strstr(m_pRG->GetParent()->OSType, "WINDOWS") != 0)
			{
				m_cstrPStoreTitle = "Pstore File:";
			}
			else
			{
				m_cstrPStoreTitle = "Location:";

				CReplicationGroupPropertySheet* rgps = (CReplicationGroupPropertySheet*)GetParent();
				m_EditPStore.Initialize(rgps->m_pDoc, m_pRG->GetParent());
			}

            

			UpdateData(false);
		}

		if (m_bReadOnly)
 		{
 			m_EditDescription.SetReadOnly();
 			m_EditJournal.SetReadOnly();
 			m_EditPStore.SetReadOnly();
 			m_ButtonServer.EnableWindow(FALSE);
 			m_CheckChaining.EnableWindow(FALSE);
			m_ComboPrimary.EnableWindow(FALSE);
			m_ComboTarget.EnableWindow(FALSE);
 		}
	}
	CATCH_ALL_LOG_ERROR(1006);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRGGeneralPage::OnKillActive() 
{
	UpdateData();

	if (m_pRG->TargetServer == NULL)
	{
		MessageBox("Please select a valid target server.", "Error", MB_OK | MB_ICONSTOP);
		m_ButtonServer.SetFocus();
		return FALSE;
	}

	if (wcsstr((BSTR)m_pRG->Parent->OSType, L"Windows") != NULL)
	{
		// Check PStore file
		if ((!m_strPStore.IsEmpty() && ( PathIsRelative(LPCTSTR(m_strPStore)) || m_strPStore.Find('*')!=-1 || 
			 m_strPStore.Find('?')!=-1 || m_strPStore.Find('\"')!=-1 || m_strPStore.Find('<')!=-1 || 
			 m_strPStore.Find('>')!=-1 || m_strPStore.Find('|')!=-1 )) ||
			(m_strPStore.IsEmpty()))
		{
			MessageBox("Please enter a valid Persistent Store File.", "Error", MB_OK | MB_ICONSTOP);
			m_EditPStore.SetFocus();
			m_EditPStore.SetSel(0, -1);
			return FALSE;
		}
	
		// Check Journal directory
		if ((!m_strJournal.IsEmpty() && ( PathIsRelative(LPCTSTR(m_strJournal)) || m_strJournal.Find('*')!=-1 || 
			 m_strJournal.Find('?')!=-1 || m_strJournal.Find('\"')!=-1 || m_strJournal.Find('<')!=-1 || 
			 m_strJournal.Find('>')!=-1 || m_strJournal.Find('|')!=-1 )) ||
			 (m_strJournal.IsEmpty()))
		{
			MessageBox("Please enter a valid Journal directory.", "Error", MB_OK | MB_ICONSTOP);
			m_EditJournal.SetFocus();
			m_EditJournal.SetSel(0, -1);
			return FALSE;
		}
	}
	
	if ((m_strJournal.GetLength() > 0) && (m_strJournal[m_strJournal.GetLength()-1] == '\\'))
	{
		m_strJournal.Delete(m_strJournal.GetLength()-1);
		UpdateData(FALSE);
	}

	if (!LinkIsValid(m_cstrPrimaryLink, true))
	{
		MessageBox("Please enter a valid Primary Replication Link.", "Error", MB_OK | MB_ICONSTOP);
		m_ComboPrimary.SetFocus();
		return FALSE;
	}

	if (!LinkIsValid(m_cstrTargetLink,false))
	{
		MessageBox("Please enter a valid Target Replication Link.", "Error", MB_OK | MB_ICONSTOP);
		m_ComboTarget.SetFocus();
		return FALSE;
	}

	return CPropertyPage::OnKillActive();
}

BOOL CRGGeneralPage::OnApply() 
{
	try
	{
		if (m_pRG != NULL && m_bPageModified)
		{
			UpdateData();

			m_pRG->Description = (LPCTSTR)m_strDescription;
			m_pRG->JournalDirectory = (LPCTSTR)m_strJournal;
			m_pRG->PStoreDirectory = (LPCTSTR)m_strPStore;
			m_pRG->Chaining = m_bChaining;

			// Save group's primary and target link
			if (_bstr_t(m_cstrPrimaryLink) == m_pRG->Parent->GetIPAddress(0))
			{
				m_pRG->SetPrimaryDHCPAddressUsed(false);
				m_pRG->SetPrimaryEditedAddressUsed(false);
				m_pRG->PrimaryEditedIP = m_pRG->Parent->GetIPAddress(0);
			}
			else
			{
				if (LinkIsAnIP(m_cstrPrimaryLink))
				{
					m_pRG->SetPrimaryDHCPAddressUsed(false);
					m_pRG->SetPrimaryEditedAddressUsed(true);
					m_pRG->PrimaryEditedIP = _bstr_t(m_cstrPrimaryLink);
				}
				else  // Primary link is a server name
				{
					m_pRG->SetPrimaryDHCPAddressUsed(true);
					m_pRG->SetPrimaryEditedAddressUsed(false);
					m_pRG->PrimaryEditedIP = "";
				}
			}

			if (m_pRG->TargetServer != NULL)
			{
				if (_bstr_t(m_cstrTargetLink) == m_pRG->TargetServer->GetIPAddress(0))
				{
					m_pRG->SetTargetDHCPAddressUsed(false);
					m_pRG->SetTargetEditedAddressUsed(false);
					m_pRG->TargetEditedIP = m_pRG->TargetServer->GetIPAddress(0);
				}
				else
				{
					if (LinkIsAnIP(m_cstrTargetLink))
					{
						m_pRG->SetTargetDHCPAddressUsed(false);
						m_pRG->SetTargetEditedAddressUsed(true);
						m_pRG->TargetEditedIP = (bstr_t)m_cstrTargetLink;
					}
					else  // Primary link is a server name
					{
						m_pRG->SetTargetDHCPAddressUsed(true);
						m_pRG->SetTargetEditedAddressUsed(false);
						m_pRG->TargetEditedIP = "";
					}
				}
			}

			m_bPageModified = false;
			SetModified(FALSE);
			SendMessage (DM_SETDEFID, IDOK);
		}
	}
	CATCH_ALL_LOG_ERROR(1007);

	return CPropertyPage::OnApply();
}

void CRGGeneralPage::OnSelectServerButton()
{
	bool bChangeTarget = false;
	// Save control info in vars
	UpdateData();

	CRGSelectServerDialog ssd(m_pRG);

	int result = ssd.DoModal();

	if(result == IDOK)
    {
        if(m_pRG->TargetServer == NULL) 
        {
            bChangeTarget = true;
        }
        else if (!m_pRG->TargetServer->IsEqual(ssd.m_pServerSelected)) // if another server has been selected
		{
			// Display a warning if it's a target change
			if (m_pRG->TargetServer != NULL)
			{
				CString strMsg;
				strMsg = "Changing the target server will reset some of the replication group settings."
						 "  It will reset the journal directory and remove all the existing replication pairs."
						 "  Are you sure you want to continue?";
			    if (MessageBox(strMsg, "Confirm Target Server Change", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
				{
					bChangeTarget = true;
				}
			}
		}
    }

	if (bChangeTarget)
	{
		m_IsTargetServerModified = true;

		// We have lately introduce the concept of having the target group
		// listed under the server.
		// Remove the target group on the old target server
		if (m_pRG->TargetServer != NULL)
		{
			RemoveTargetGroup();
		}

		// Save new target server
		m_pRG->TargetServer = ssd.m_pServerSelected;
		m_cstrTargetName = (BSTR)ssd.m_pServerSelected->Name;

		// Fill Target ComboBox
		for (int j = m_ComboTarget.GetCount() - 1; j >= 0; j--)
		{
			m_ComboTarget.DeleteString(j);
		}
		for (int i = 0; i < m_pRG->TargetServer->IPAddressCount; i++)
		{
			m_ComboTarget.AddString(m_pRG->TargetServer->GetIPAddress(i));
		}
		m_ComboTarget.AddString(m_cstrTargetName);
		// Loopback consideration
		if (m_pRG->Parent->IsEqual(m_pRG->TargetServer))
		{
			m_ComboPrimary.InsertString(0, "127.0.0.1");
			m_ComboTarget.InsertString(0, "127.0.0.1");
		}
		else
		{
			int nIndex = m_ComboPrimary.FindString(-1, "127.0.0.1");
			if (nIndex != CB_ERR)
			{
				m_ComboPrimary.DeleteString(nIndex);
				m_cstrPrimaryLink = (BSTR)m_pRG->Parent->GetIPAddress(0);
			}
		}

        //By default, use public IP
		m_cstrTargetLink = (BSTR)ssd.m_pServerSelected->GetIPAddress(0);
        m_pRG->SetTargetDHCPAddressUsed(false);
        m_pRG->SetTargetEditedAddressUsed(false);

		// Find a unique group number (unique on both servers)
		m_nGroupNb = m_pRG->GetUniqueGroupNumber();
		m_pRG->GroupNumber = m_nGroupNb;

		// We have lately introduce the concept of having the target group
		// listed under the server.
		// Create a target group on the target server
		AddTargetGroup(m_nGroupNb);

		// Remove all replication pairs
		for (long nIndex = m_pRG->ReplicationPairCount - 1; nIndex >= 0; nIndex--)
		{
			TDMFOBJECTSLib::IReplicationPairPtr pRP = m_pRG->GetReplicationPair(nIndex);
			// if it's an old object
			if (pRP->ObjectState == TDMFOBJECTSLib::RPO_SAVED)
			{
				// Mark the object as deleted
				pRP->ObjectState = TDMFOBJECTSLib::RPO_DELETED;
			}
			else
			{
				m_pRG->RemoveReplicationPair(pRP);
			}
		}

        // Set journal directory (insert target server default value)
		m_strJournal = (BSTR)ssd.m_pServerSelected->JournalDirectory;

		UpdateData(FALSE);
		SetModified();
		m_bPageModified = true;
	}
}

void CRGGeneralPage::OnCheckChaining() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnUpdateEditDescription() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnUpdateEditGroupNumber() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnUpdateEditJournal() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnUpdateEditPstore() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnEditchangeComboPrimary() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnSelchangeComboPrimary() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnEditchangeComboTarget() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnSelchangeComboTarget() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGGeneralPage::OnKillfocusComboPrimary() 
{
   if(m_pRG->TargetServer != NULL) 
   {
		if (m_pRG->Parent->IsEqual(m_pRG->TargetServer))
		{
			UpdateData();
			m_cstrTargetLink = m_cstrPrimaryLink;		
			UpdateData(FALSE);
		}
   }
}

void CRGGeneralPage::OnKillfocusComboTarget() 
{
   if(m_pRG->TargetServer != NULL) 
   {
		if (m_pRG->Parent->IsEqual(m_pRG->TargetServer))
		{
			UpdateData();
			m_cstrPrimaryLink = m_cstrTargetLink;
			UpdateData(FALSE);
		}	
	 }
}

BOOL CRGGeneralPage::OnSetActive() 
{
	if (m_bNewItem)
	{
		m_bPageModified = true;
		SetModified();
	}
	else
	{
		m_bPageModified = false;
	}

	return CPropertyPage::OnSetActive();
}

void CRGGeneralPage::OnCancel() 
{
	try
	{
		if (m_pRG != NULL && m_bPageModified)
		{
			// We have lately introduce the concept of having the target group
			// listed under the server.
			// Remove the target group on the modified target server
			RemoveTargetGroup();

            m_pRG->SetPrimaryDHCPAddressUsed(m_bPrimaryServerDHCPNameUsedSaved);
            m_pRG->SetPrimaryEditedAddressUsed(m_bPrimaryServerEditedUsedSaved);
			m_pRG->PrimaryEditedIP = m_bstrPrimaryServerEditedIPSaved;

            m_pRG->TargetServer = m_pTargetServerSaved;  
 		    m_pRG->SetTargetDHCPAddressUsed(m_bTargetServerDHCPNameUsedSaved);
 		    m_pRG->SetTargetEditedAddressUsed(m_bTargetServerEditedUsedSaved);
			m_pRG->TargetEditedIP = m_bstrTargetServerEditedIPSaved ;

            m_pRG->GroupNumber = 	m_nGroupNbSaved;
		    m_pRG->PStoreDirectory =    m_bstrPStoreSaved;

			// We have lately introduce the concept of having the target group
			// listed under the server.
			// Create a target group on the target server
			AddTargetGroup();
		}
	}
	CATCH_ALL_LOG_ERROR(1008);
	
	CPropertyPage::OnCancel();
}

void CRGGeneralPage::RemoveTargetGroup()
{
	TDMFOBJECTSLib::IReplicationGroupPtr pRGRemoved = m_pRG->GetTargetGroup();

	if (pRGRemoved)
	{
		// Fire an Object Delete (remove) Notification
		CReplicationGroupPropertySheet* rgps = (CReplicationGroupPropertySheet*)GetParent();
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_REMOVE;
		VN.m_pUnk = (IUnknown*) pRGRemoved;
		rgps->m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
		
		TDMFOBJECTSLib::IServerPtr pServer = pRGRemoved->Parent;
		if (pServer->RemoveReplicationGroup(pRGRemoved) != 0)
		{
			CString strMsg;
			strMsg.Format("Unable to delete replication group '%S'?\n\n", (BSTR)pRGRemoved->Name);
			MessageBox(strMsg, "Replication Group Deletion Error", MB_OK|MB_ICONINFORMATION);
		}
	}
}

void CRGGeneralPage::AddTargetGroup(long nGroupNumber)
{
	TDMFOBJECTSLib::IReplicationGroupPtr pRGTarget = m_pRG->CreateAssociatedTargetGroup();

	if (pRGTarget)
	{
		if (nGroupNumber > 0)
		{
			pRGTarget->GroupNumber = nGroupNumber;
		}

		// Fire an Object New (add) Notification
		CReplicationGroupPropertySheet* rgps = (CReplicationGroupPropertySheet*)GetParent();
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_ADD;
		VN.m_pUnk = (IUnknown*) pRGTarget;
		rgps->m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
	}
}

bool CRGGeneralPage::LinkIsValid(LPCSTR lpcstrLink, BOOL bForPrimaryServer)
{
	bool bValid = false;

	if (LinkIsAnIP(lpcstrLink))
	{
		CString cstrLink = lpcstrLink;

   		int a1 = atoi(cstrLink);
		int nIndex = cstrLink.Find('.', 0);
		if (nIndex > -1)
		{
			cstrLink = cstrLink.Mid(nIndex + 1);
			int a2 = atoi(cstrLink);
			nIndex = cstrLink.Find('.', 0);
			if (nIndex > -1)
			{
				cstrLink = cstrLink.Mid(nIndex + 1);
				int a3 = atoi(cstrLink);
				nIndex = cstrLink.Find('.', 0);
				if (nIndex > -1)
				{
					cstrLink = cstrLink.Mid(nIndex + 1);

					int a4 = atoi(cstrLink);
					if ((a1 <= 255) && (a2 <= 255) && (a3 <= 255) && (a4 <= 255))
					{
						bValid = true;
					}
				}
			}
		}
	}
	else
	{
        	if(bForPrimaryServer)
		{
		  if (stricmp(lpcstrLink, m_pRG->Parent->Name) == 0)
		  {
			bValid = true;
		  }
		}
		else
		{
		  if(m_pRG->TargetServer != NULL)
		  {
		     if (stricmp(lpcstrLink, m_pRG->TargetServer->Name) == 0)
		     {
		 	bValid = true;
		     }
		  }
		}
	}

	return bValid;
}

bool CRGGeneralPage::LinkIsAnIP(LPCSTR lpcstrLink)
{
    CString cstrLink = lpcstrLink;
    
	int nCount = cstrLink.Remove('.');

	if(nCount != 3)
		return false;

	if (atoi(lpcstrLink) > 0)
	{
		return true;
	}

	return false;
}
