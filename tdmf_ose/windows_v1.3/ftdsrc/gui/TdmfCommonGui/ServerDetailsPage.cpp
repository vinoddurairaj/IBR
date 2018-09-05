// ServerDetailsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "ServerDetailsPage.h"
#include "ToolsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//

CString FormatDiskSize(BSTR bstrSize)
{
	CString cstrSize;

	char* rgszUnit[] = { "Bytes", "KB", "MB", "GB"};

	__int64 liSize = _wtoi64(bstrSize);
	double  dSize  = (double)liSize;
	int     nUnit  = 0;
	int     nDiv   = 1024;

	while ((dSize > 1024) && (nUnit <= ((sizeof(rgszUnit)/sizeof(char*))-1)))
	{
		dSize = (double)liSize / nDiv;

		nUnit++;
		nDiv *= 1024;  
	}
	
	cstrSize.Format("%.1f %s", dSize, rgszUnit[nUnit]);

	int nIndex;
	if ((nIndex = cstrSize.Find(".0")) > 0)
	{
		cstrSize.Delete(nIndex, 2);
	}

	return cstrSize;
}

/////////////////////////////////////////////////////////////////////////////
// CServerDetailsPage property page

IMPLEMENT_DYNCREATE(CServerDetailsPage, CPropertyPage)

CServerDetailsPage::CServerDetailsPage() : CPropertyPage(CServerDetailsPage::IDD)
{
	//{{AFX_DATA_INIT(CServerDetailsPage)
	m_strIPAddress = _T("");
	m_strOS = _T("");
	m_nNbRGTarget = 0;
	m_nNbRGSource = 0;
	m_nNbRPSource = 0;
	m_nNbRPTarget = 0;
	m_nBABEntries = 0;
	m_strBABUsed = _T("");
	m_strBABSize = _T("");
	m_strName = _T("");
	m_strSettingsLabel = _T("Settings and Statistics");
	m_strSocketBufSize = _T("");
	m_strHostId = _T("");
	m_strPStoreSize = _T("");
	m_strRegistrationKey = _T("");
	m_strCPU = _T("");
	m_strRAMSize = _T("");
	//}}AFX_DATA_INIT
}

CServerDetailsPage::~CServerDetailsPage()
{
}

void CServerDetailsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerDetailsPage)
	DDX_Control(pDX, IDC_EDIT_RAM, m_EditRAM);
	DDX_Control(pDX, IDC_EDIT_CPU, m_EditCPU);
	DDX_Control(pDX, IDC_LIST_JOURNAL, m_ListJournal);
	DDX_Control(pDX, IDC_EDIT_REGISTRATION_KEY, m_EditRegistrationKey);
	DDX_Control(pDX, IDC_EDIT_OS, m_EditOs);
	DDX_Control(pDX, IDC_EDIT_IPADDRESS, m_EditIPAddress);
	DDX_Control(pDX, IDC_EDIT_HOST_ID, m_EditHostId);
	DDX_Control(pDX, IDC_EDIT_SOCKET, m_EditSocketSize);
	DDX_Control(pDX, IDC_EDIT_PSTORE_SIZE, m_EditPStoreSize);
	DDX_Control(pDX, IDC_EDIT_BAB_USED, m_EditBabUsed);
	DDX_Control(pDX, IDC_EDIT_BAB_SIZE, m_EditBabSize);
	DDX_Control(pDX, IDC_EDIT_BAB_ENTRIES, m_EditBabEntries);
	DDX_Control(pDX, IDC_TITLE_TCP_SIZE, m_TitleTCPSize);
	DDX_Control(pDX, IDC_TITLE_PSTORE_SIZE, m_TitlePStoreSize);
	DDX_Control(pDX, IDC_TITLE_JOURNAL_SIZE, m_TitleJournalSize);
	DDX_Control(pDX, IDC_TITLE_BAB_USED, m_TitleBabUsed);
	DDX_Control(pDX, IDC_TITLE_BAB_SIZE, m_TitleBabSize);
	DDX_Control(pDX, IDC_TITLE_BAB_ENTRIES, m_TitleBabEntries);
	DDX_Control(pDX, IDC_RICHEDIT_SETTINGS, m_RichEditSettings);
	DDX_Control(pDX, IDC_RICHEDIT_NAME, m_RichEditName);
	DDX_Text(pDX, IDC_EDIT_IPADDRESS, m_strIPAddress);
	DDX_Text(pDX, IDC_EDIT_OS, m_strOS);
	DDX_Text(pDX, IDC_EDIT_RG_TARGET, m_nNbRGTarget);
	DDX_Text(pDX, IDC_EDIT_RG_SOURCE, m_nNbRGSource);
	DDX_Text(pDX, IDC_EDIT_RP_SOURCE, m_nNbRPSource);
	DDX_Text(pDX, IDC_EDIT_RP_TARGET, m_nNbRPTarget);
	DDX_Text(pDX, IDC_EDIT_BAB_ENTRIES, m_nBABEntries);
	DDX_Text(pDX, IDC_EDIT_BAB_USED, m_strBABUsed);
	DDX_Text(pDX, IDC_EDIT_BAB_SIZE, m_strBABSize);
	DDX_Text(pDX, IDC_RICHEDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_RICHEDIT_SETTINGS, m_strSettingsLabel);
	DDX_Text(pDX, IDC_EDIT_SOCKET, m_strSocketBufSize);
	DDX_Text(pDX, IDC_EDIT_HOST_ID, m_strHostId);
	DDX_Text(pDX, IDC_EDIT_PSTORE_SIZE, m_strPStoreSize);
	DDX_Text(pDX, IDC_EDIT_REGISTRATION_KEY, m_strRegistrationKey);
	DDX_Text(pDX, IDC_EDIT_CPU, m_strCPU);
	DDX_Text(pDX, IDC_EDIT_RAM, m_strRAMSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerDetailsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerDetailsPage)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerDetailsPage message handlers

BOOL CServerDetailsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Set Name format
	m_RichEditName.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));
	m_RichEditSettings.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));

	CHARFORMAT cf;
	cf.dwMask = CFM_BOLD;
	cf.dwEffects = CFE_BOLD;
	m_RichEditSettings.SetDefaultCharFormat(cf);

	cf.dwMask = CFM_BOLD | CFM_SIZE;
	m_RichEditName.GetDefaultCharFormat(cf);
	cf.dwEffects = CFE_BOLD;
	cf.yHeight = (LONG)(cf.yHeight * 1.5);
	m_RichEditName.SetDefaultCharFormat(cf);

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	m_ListJournal.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_ListJournal.SetBkColor(GetSysColor(COLOR_BTNFACE));
	m_ListJournal.SetTextBkColor(GetSysColor(COLOR_BTNFACE));

	if (pDoc->GetSelectedServer() != NULL)
	{
		if (strstr(pDoc->GetSelectedServer()->OSType, "Windows") != 0 ||
			strstr(pDoc->GetSelectedServer()->OSType, "windows") != 0 ||
			strstr(pDoc->GetSelectedServer()->OSType, "WINDOWS") != 0)
		{
			m_TitlePStoreSize.ShowWindow(SW_SHOW);
			m_EditPStoreSize.ShowWindow(SW_SHOW);
			
			m_ListJournal.InsertColumn(0, "Size");
			m_ListJournal.SetColumnWidth(0, 80);
			m_ListJournal.InsertColumn(1, "Drive");
			m_ListJournal.SetColumnWidth(1, 40);
			m_ListJournal.InsertColumn(2, "Disk Size");
			m_ListJournal.SetColumnWidth(2, 80);
			m_ListJournal.InsertColumn(3, "Disk Free Size");
			m_ListJournal.SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER);
		}
		else
		{
			m_TitlePStoreSize.ShowWindow(SW_HIDE);
			m_EditPStoreSize.ShowWindow(SW_HIDE);
			
			m_ListJournal.InsertColumn(0, "Size");
			m_ListJournal.SetColumnWidth(0, 40);
			m_ListJournal.InsertColumn(1, "Location");
			m_ListJournal.SetColumnWidth(1, 80);
			m_ListJournal.InsertColumn(2, "Volume Size");
			m_ListJournal.SetColumnWidth(2, 70);
			m_ListJournal.InsertColumn(3, "Volume Free Size");
			m_ListJournal.SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CServerDetailsPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();

	// Init values
	RefreshValues();

	return CPropertyPage::OnSetActive();
}


void CServerDetailsPage::RefreshValues()
{
	try
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
		
		if (pDoc->GetSelectedServer() != NULL)
		{
			std::ostringstream oss;

			m_strName = (BSTR)pDoc->GetSelectedServer()->Name;

			oss << (char*)pDoc->GetSelectedServer()->IPAddress[0] << ":" << pDoc->GetSelectedServer()->Port;
			m_strIPAddress = oss.str().c_str();

			oss.str(std::string());
			oss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << pDoc->GetSelectedServer()->HostID;
			oss << std::dec;
			m_strHostId = oss.str().c_str();

			oss.str(std::string());
			oss << (char*)pDoc->GetSelectedServer()->RegKey;
			m_strRegistrationKey = oss.str().c_str();

			oss.str(std::string());
			oss << (char*)pDoc->GetSelectedServer()->OSType << " " << (char*)pDoc->GetSelectedServer()->OSVersion;
			m_strOS = oss.str().c_str();

            oss.str(std::string());
			oss << pDoc->GetSelectedServer()->NbrCPU ;
            m_strCPU = oss.str().c_str();

            oss.str(std::string());
			oss << pDoc->GetSelectedServer()->RAMSize << " KB"; ;
            m_strRAMSize = oss.str().c_str();
 
			m_nNbRGTarget = pDoc->GetSelectedServer()->TargetReplicationGroupCount;
			m_nNbRGSource = pDoc->GetSelectedServer()->ReplicationGroupCount - m_nNbRGTarget;

			m_nNbRPTarget = pDoc->GetSelectedServer()->TargetReplicationPairCount;
			m_nNbRPSource = pDoc->GetSelectedServer()->ReplicationPairCount - m_nNbRPTarget;

			oss.str(std::string());
			oss << pDoc->GetSelectedServer()->BABSizeAllocated << "/";
			oss << pDoc->GetSelectedServer()->BABSize << " MB ";
			m_strBABSize = oss.str().c_str();

			oss.str(std::string());
			oss << pDoc->GetSelectedServer()->PctBAB << " %";
			m_strBABUsed = oss.str().c_str();

			m_nBABEntries = pDoc->GetSelectedServer()->BABEntries;
		
			oss.str(std::string());
			oss << pDoc->GetSelectedServer()->TCPWndSize << " KB";
			m_strSocketBufSize = oss.str().c_str(); 
			
			m_strPStoreSize = (BSTR)pDoc->GetSelectedServer()->PStoreSize;

			m_ListJournal.DeleteAllItems();
			long nDriveCount = pDoc->GetSelectedServer()->JournalDriveCount;
			for (long nDriveIndex = 0; nDriveIndex < nDriveCount; nDriveIndex++)
			{
				m_ListJournal.InsertItem(nDriveIndex, FormatDiskSize(pDoc->GetSelectedServer()->JournalSize[nDriveIndex]));
				m_ListJournal.SetItemText(nDriveIndex, 1, pDoc->GetSelectedServer()->JournalDrive[nDriveIndex]);
				m_ListJournal.SetItemText(nDriveIndex, 2, FormatDiskSize(pDoc->GetSelectedServer()->JournalDiskSize[nDriveIndex]));
				m_ListJournal.SetItemText(nDriveIndex, 3, FormatDiskSize(pDoc->GetSelectedServer()->JournalDiskFreeSize[nDriveIndex]));
			}

			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1060);
}

void CServerDetailsPage::MoveCtrl(CWnd& WndCtrl, int nX, int nRight)
{
	CRect Rect;

	if (WndCtrl.m_hWnd != NULL)
	{
		WndCtrl.GetWindowRect(&Rect);
		ScreenToClient(&Rect);
		int nWidth = Rect.Width();

		if (nX > 0)
		{
			Rect.left = nX;
		}

		if (nRight < 0)
		{
			Rect.right = Rect.left + nWidth;
		}
		else
		{
			Rect.right = nRight;
		}

		WndCtrl.MoveWindow(&Rect);
	}
}

void CServerDetailsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	
	int nMiddle = cx/2;
	int nOffset = 12;
	int nOffsetEdit = 150;

	// Left Columns (just resize editbox)
	MoveCtrl(m_EditIPAddress, -1, nMiddle);
	MoveCtrl(m_EditHostId,    -1, nMiddle);
	MoveCtrl(m_EditRegistrationKey, -1, nMiddle);
	MoveCtrl(m_EditOs,        -1, nMiddle);
    MoveCtrl(m_EditCPU,        -1, nMiddle);
    MoveCtrl(m_EditRAM,        -1, nMiddle);

	//...

	// Right columns (Move label and editbox and reize editbox)
	int nRightEdit  = cx - nOffset; 

	MoveCtrl(m_RichEditSettings, nMiddle);
	MoveCtrl(m_TitleBabEntries,  nMiddle + nOffset);
	MoveCtrl(m_EditBabEntries,   nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleBabSize,     nMiddle + nOffset);
	MoveCtrl(m_EditBabSize,      nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleBabUsed,     nMiddle + nOffset);
	MoveCtrl(m_EditBabUsed,      nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleJournalSize, nMiddle + nOffset);
	MoveCtrl(m_TitlePStoreSize,  nMiddle + nOffset);
	MoveCtrl(m_EditPStoreSize,   nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleTCPSize,     nMiddle + nOffset);
	MoveCtrl(m_EditSocketSize,   nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_ListJournal,      nMiddle + (2*nOffset));
}
