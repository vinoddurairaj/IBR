// ServerGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ServerGeneralPage.h"
#include "ServerPropertySheet.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define  VALIDATE_BAB_SIZE_MINIMUM_VALUE 0

/////////////////////////////////////////////////////////////////////////////
// CServerGeneralPage property page

IMPLEMENT_DYNCREATE(CServerGeneralPage, CPropertyPage)

CServerGeneralPage::CServerGeneralPage(TDMFOBJECTSLib::IServer *pServer)
	: CPropertyPage(CServerGeneralPage::IDD), m_pServer(pServer)
{
	//{{AFX_DATA_INIT(CServerGeneralPage)
	m_nBABSize = 0;
	m_strDescription = _T("");
	m_nPort = 0;
	m_strPStore = _T("");
	m_nTCPWindowSize = 0;
	m_strJournal = _T("");
	m_cstrPStoreTitle = _T("");
	m_cstrRegKey = _T("");
	m_nReplicationPort = 0;
	//}}AFX_DATA_INIT

	m_bPageModified = false;
	m_nMaxBabSize = 192;
}

CServerGeneralPage::~CServerGeneralPage()
{
}

void CServerGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerGeneralPage)
	DDX_Control(pDX, IDC_LABEL_REPLICATION_PORT, m_LabelReplicationPort);
	DDX_Control(pDX, IDC_EDIT_REPLICATION_PORT, m_EditReplicationPort);
	DDX_Control(pDX, IDC_EDIT_TCP_WINDOW, m_EditTCPWindowSize);
	DDX_Control(pDX, IDC_EDIT_PORT, m_EditPort);
	DDX_Control(pDX, IDC_EDIT_REG_KEY, m_EditRegKey);
	DDX_Control(pDX, IDC_EDIT_JOURNAL, m_JournalEdit);
	DDX_Control(pDX, IDC_EDIT_PSTORE, m_PStoreEdit);
	DDX_Control(pDX, IDC_EDIT_BAB_SIZE, m_BABSizeEdit);
	DDX_Control(pDX, IDC_SPIN_BAB_SIZE, m_SpinButton);
	DDX_Text(pDX, IDC_EDIT_BAB_SIZE, m_nBABSize);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Text(pDX, IDC_EDIT_PSTORE, m_strPStore);
	DDV_MaxChars(pDX, m_strPStore, 300);
	DDX_Text(pDX, IDC_EDIT_TCP_WINDOW, m_nTCPWindowSize);
	DDX_Text(pDX, IDC_EDIT_JOURNAL, m_strJournal);
	DDV_MaxChars(pDX, m_strJournal, 300);
	DDX_Text(pDX, IDC_PSTORE_TITLE, m_cstrPStoreTitle);
	DDX_Text(pDX, IDC_EDIT_REG_KEY, m_cstrRegKey);
	DDX_Text(pDX, IDC_EDIT_REPLICATION_PORT, m_nReplicationPort);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerGeneralPage)
	ON_EN_UPDATE(IDC_EDIT_BAB_SIZE, OnUpdateEditBabSize)
	ON_EN_UPDATE(IDC_EDIT_PSTORE, OnUpdateEditPstore)
	ON_EN_UPDATE(IDC_EDIT_TCP_WINDOW, OnUpdateEditTcpWindow)
	ON_EN_UPDATE(IDC_EDIT_PORT, OnUpdateEditPort)
	ON_EN_UPDATE(IDC_EDIT_DESCRIPTION, OnUpdateEditDescription)
	ON_EN_UPDATE(IDC_EDIT_JOURNAL, OnUpdateEditJournal)
	ON_EN_UPDATE(IDC_EDIT_REG_KEY, OnUpdateEditRegKey)
	ON_EN_UPDATE(IDC_EDIT_REPLICATION_PORT, OnUpdateEditReplicationPort)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerGeneralPage message handlers

BOOL CServerGeneralPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	try
	{
		m_bPageModified = false;
		
		int	nAccel = 1;
		UDACCEL	Accel;
		
		Accel.nSec = 1;
		Accel.nInc = 32;
		m_SpinButton.SetAccel( nAccel, &Accel );

		if (strstr(m_pServer->OSType, "Windows") != 0 ||
			strstr(m_pServer->OSType, "windows") != 0 ||
			strstr(m_pServer->OSType, "WINDOWS") != 0)
		{
			#define BAB_GRANULARITY 32
			m_nMaxBabSize = (((m_pServer->RAMSize/1024 * 6) / 10) / BAB_GRANULARITY) * BAB_GRANULARITY; 

			if (wcsstr((BSTR)m_pServer->OSVersion, L"NT") != NULL)
			{
				m_nMaxBabSize = __min(192, m_nMaxBabSize);
			}
			else
			{
				m_nMaxBabSize = __min(224, m_nMaxBabSize);
			}
		}
		else
		{
			m_nMaxBabSize = m_pServer->RAMSize/1024 - (32 * m_pServer->NbrCPU);
			m_nMaxBabSize = __min(1547, m_nMaxBabSize);

			m_LabelReplicationPort.ShowWindow(SW_SHOW);
			m_EditReplicationPort.ShowWindow(SW_SHOW);
		}

		m_SpinButton.SetRange32(VALIDATE_BAB_SIZE_MINIMUM_VALUE, m_nMaxBabSize);

		
		if (m_pServer != NULL)
		{ 
			m_nBABSize = m_pServer->BABSize;
			m_strDescription = (BSTR)m_pServer->Description;
			m_nPort = m_pServer->Port;
			m_nReplicationPort = m_pServer->ReplicationPort;
			m_strPStore = (BSTR)m_pServer->PStoreDirectory;
			m_strJournal = (BSTR)m_pServer->JournalDirectory;
			m_nTCPWindowSize = m_pServer->TCPWndSize;

			m_EditRegKey.SetLimitText(29);
            m_EditRegKey.SetMargins(5, 5);

			m_cstrRegKey = (BSTR)m_pServer->RegKey;

			if (strstr(m_pServer->OSType, "Windows") != 0 ||
				strstr(m_pServer->OSType, "windows") != 0 ||
				strstr(m_pServer->OSType, "WINDOWS") != 0)
			{
				m_cstrPStoreTitle = "PStore File:";
			}
			else
			{
				m_cstrPStoreTitle = "PStore Location:";

				CServerPropertySheet* rgps = (CServerPropertySheet*)GetParent();
				m_PStoreEdit.Initialize(rgps->m_pDoc, m_pServer);
			}
			
		}
		
		UpdateData(FALSE);
	}		
	CATCH_ALL_LOG_ERROR(1061);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CServerGeneralPage::OnKillActive() 
{
	UpdateData();

	// Check key format
	CString cstrKey = m_cstrRegKey;
	// Remove all spaces
	cstrKey.Remove(' ');
	cstrKey.Remove('\t');  // Tabs
	// Reformat key (6 x 4 char) and validate format
	m_cstrRegKey = "";
	int nLength = cstrKey.GetLength();
	int nIndex  = 0;
	while (nIndex < nLength)
	{
		if ((nIndex> 0) && (nIndex%4 == 0))
		{
			m_cstrRegKey += " ";
		}
		m_cstrRegKey +=  cstrKey.Mid(nIndex, 4);
		nIndex += 4;
	}
	UpdateData(FALSE);
	// Check length 
    if(cstrKey.GetLength() != 24)
	{
		m_EditRegKey.SetFocus();
		AfxMessageBox("The key entered is incomplete.",MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}

	// Validate
#ifdef VALIDATE_BAB_SIZE_MINIMUM_VALUE
	if(m_nBABSize < VALIDATE_BAB_SIZE_MINIMUM_VALUE)
	{
		CString strMsg;
		strMsg.Format("The lowest BAB size is %d .\n\nPlease enter a valid BAB Size.",
				  VALIDATE_BAB_SIZE_MINIMUM_VALUE);

		AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);

		m_BABSizeEdit.SetFocus();
				
		char babSize[20];
		itoa(VALIDATE_BAB_SIZE_MINIMUM_VALUE, babSize, 10);
		m_BABSizeEdit.SetWindowText(babSize);
				
		m_BABSizeEdit.SetSel(0, -1);

		return FALSE;
	}
#endif

	if(m_nBABSize > m_nMaxBabSize)
	{
		std::ostringstream message;
		message << "The highest BAB size for this server is " << m_nMaxBabSize
		        << ".\n\nPlease enter a valid BAB Size.";
		MessageBox(message.str().c_str(), "Error", MB_OK | MB_ICONSTOP);
		
		m_BABSizeEdit.SetFocus();
		char szBABLimit[20];
		m_BABSizeEdit.SetWindowText(itoa(m_nMaxBabSize, szBABLimit, 10));
		
		m_BABSizeEdit.SetSel(0, -1);

		return FALSE;
	}

	if ((m_nPort < 0) || (m_nPort > 65535))
	{
		MessageBox("The port number must be in the 0-65535 range.\n\nPlease enter a valid port number.", "Error", MB_OK | MB_ICONSTOP);
		
		m_EditPort.SetFocus();		
		m_EditPort.SetSel(0, -1);

		return FALSE;
	}

	if ((m_nReplicationPort < 0) || (m_nReplicationPort > 65535))
	{
		MessageBox("The port number must be in the 0-65535 range.\n\nPlease enter a valid port number.", "Error", MB_OK | MB_ICONSTOP);
		
		m_EditReplicationPort.SetFocus();		
		m_EditReplicationPort.SetSel(0, -1);

		return FALSE;
	}


	if ((m_nTCPWindowSize < 1) || (m_nTCPWindowSize > 779263))
	{
		MessageBox("The TCP Window Size must be in the 1-779263 range.\n\nPlease enter a valid TCP Window Size.", "Error", MB_OK | MB_ICONSTOP);
		
		m_EditTCPWindowSize.SetFocus();		
		m_EditTCPWindowSize.SetSel(0, -1);

		return FALSE;
	}

	if (strstr(m_pServer->OSType, "Windows") != 0 ||
		strstr(m_pServer->OSType, "windows") != 0 ||
		strstr(m_pServer->OSType, "WINDOWS") != 0)
	{
		if(!m_strPStore.IsEmpty() && ( PathIsRelative(LPCTSTR(m_strPStore)) || m_strPStore.Find('*')!=-1 || 
			m_strPStore.Find('?')!=-1 || m_strPStore.Find('\"')!=-1 || m_strPStore.Find('<')!=-1 || 
			m_strPStore.Find('>')!=-1 || m_strPStore.Find('|')!=-1 ))
		{
			MessageBox("Please enter a valid Persistent Store Directory.", "Error", MB_OK | MB_ICONSTOP);
			m_PStoreEdit.SetFocus();
			m_PStoreEdit.SetSel(0, -1);

			return FALSE;
		}
	
		if(!m_strJournal.IsEmpty() && ( PathIsRelative(LPCTSTR(m_strJournal)) || m_strJournal.Find('*')!=-1 || 
			m_strJournal.Find('?')!=-1 || m_strJournal.Find('\"')!=-1 || m_strJournal.Find('<')!=-1 || 
			m_strJournal.Find('>')!=-1 || m_strJournal.Find('|')!=-1 ))
		{
			MessageBox("Please enter a valid Journal directory.", "Error", MB_OK | MB_ICONSTOP);
			m_JournalEdit.SetFocus();
			m_JournalEdit.SetSel(0, -1);

			return FALSE;
		}
	}
	
	return CPropertyPage::OnKillActive();
}

BOOL CServerGeneralPage::OnApply() 
{
	try
	{
		if (m_pServer != NULL && m_bPageModified)
		{
			UpdateData();

			// Every field is valid, so transfer the data to the server pointer
			m_pServer->RegKey = _bstr_t(m_cstrRegKey);
			m_pServer->BABSize = m_nBABSize;
			m_pServer->Description = (LPCTSTR)m_strDescription;
			m_pServer->Port = m_nPort;
			m_pServer->ReplicationPort = m_nReplicationPort;
			m_pServer->PStoreDirectory = (LPCTSTR)m_strPStore;
			m_pServer->JournalDirectory = (LPCTSTR)m_strJournal;
			m_pServer->TCPWndSize = m_nTCPWindowSize;

			
			m_bPageModified = false;
			//SetModified(FALSE);
			SendMessage (DM_SETDEFID, IDOK);
		}
	}
	CATCH_ALL_LOG_ERROR(1062);

	return CPropertyPage::OnApply();
}

void CServerGeneralPage::OnUpdateEditBabSize() 
{
	//SetModified();
	((CServerPropertySheet*)GetParent())->m_bReboot = true;
	m_bPageModified = true;
}

void CServerGeneralPage::OnUpdateEditPstore() 
{
	//SetModified();
	m_bPageModified = true;
}

void CServerGeneralPage::OnUpdateEditTcpWindow() 
{
	//SetModified();
	// Windows Agents don't need to be rebooted when TCP window changes.
	if (!(strstr(m_pServer->OSType, "Windows") != 0 ||
		  strstr(m_pServer->OSType, "windows") != 0 ||
		  strstr(m_pServer->OSType, "WINDOWS") != 0))
	{
		((CServerPropertySheet*)GetParent())->m_bReboot = true;
	}
	((CServerPropertySheet*)GetParent())->m_bTcpWindowSizeChanged = true;
	m_bPageModified = true;
}

void CServerGeneralPage::OnUpdateEditPort() 
{
	//SetModified();
	((CServerPropertySheet*)GetParent())->m_bReboot = true;
	m_bPageModified = true;
}

void CServerGeneralPage::OnUpdateEditDescription() 
{
	//SetModified();
	m_bPageModified = true;
}

void CServerGeneralPage::OnUpdateEditJournal() 
{
	//SetModified();
	m_bPageModified = true;
}

void CServerGeneralPage::OnUpdateEditRegKey() 
{
	//SetModified();
    m_bPageModified = true;	
}

void CServerGeneralPage::OnUpdateEditReplicationPort() 
{
	//SetModified();
	((CServerPropertySheet*)GetParent())->m_bReboot = true;
	m_bPageModified = true;	
}


