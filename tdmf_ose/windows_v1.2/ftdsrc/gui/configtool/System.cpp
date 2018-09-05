// System.cpp : implementation file
//
// Date			Who			What
//
// 7-19-2001	DTurrin		Code for splash screen added
//

#include "stdafx.h"
#include "Splash.h"
#include "DTCConfigTool.h"
#include "System.h"
#include "Config.h"
#include "DTCConfigToolDlg.h"
#include "Hostname.h"
#include "winsock2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;
/////////////////////////////////////////////////////////////////////////////
// CSystem property page

IMPLEMENT_DYNCREATE(CSystem, CPropertyPage)

CSystem::CSystem() : CPropertyPage(CSystem::IDD)
{
	//{{AFX_DATA_INIT(CSystem)
	m_szSysNote = _T("");
	//}}AFX_DATA_INIT
}

CSystem::~CSystem()
{
}

void CSystem::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystem)
	DDX_Text(pDX, IDC_EDIT_SYSTEM_NOTE, m_szSysNote);
	DDV_MaxChars(pDX, m_szSysNote, 240);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystem, CPropertyPage)
	//{{AFX_MSG_MAP(CSystem)
	ON_WM_CREATE()
	ON_WM_CANCELMODE()
	ON_WM_CAPTURECHANGED()
	//ON_BN_CLICKED(IDC_PMD_HOSTNAME1, OnHostnamePrim)
	//ON_BN_CLICKED(IDC_PMD_HOSTNAME2, OnHostnameSec)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystem message handlers


int CSystem::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void CSystem::OnCancelMode() 
{
	CPropertyPage::OnCancelMode();	
}

void CSystem::getLocalDriveSize(char *szDrive, char *szSize)
{
	ULARGE_INTEGER FreeBytesAvailableToCaller, TotalNumberOfBytes, TotalNumberOfFreeBytes;
	int iTotal = 0;

	memset(szSize, 0, sizeof(szSize));
	if (GetDiskFreeSpaceEx(szDrive, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
	{
		iTotal = (int)(TotalNumberOfBytes.QuadPart/(1024 * 1024));
		itoa(iTotal, szSize, 10);
	}
	else
		strcpy(szSize, "UnKnown");
}

void CSystem::checkDriveType()
{
	CComboBox	*pStore;
	int			iNumItems, i;
	char		strText[_MAX_PATH], strDrive[_MAX_PATH], strWinDir[_MAX_PATH];
	UINT		iDrive;
	CString		strWinDirLower = "";

	GetWindowsDirectory(strWinDir, _MAX_PATH);
	strWinDirLower = strWinDir;
	strWinDirLower.MakeUpper();


	pStore = (CComboBox *)GetDlgItem(IDC_LIST_PSTORE);
	iNumItems = pStore->GetCount( );
	for(i = 0; i < iNumItems; i++)
	{
		memset(strText, 0, sizeof(strText));
		memset(strDrive, 0, sizeof(strDrive));
		pStore->GetLBText(i, strText);
		strText[2] = strText[2] - 32;
		sprintf(strDrive, "%c:", strText[2]);
		iDrive = GetDriveType(strDrive);
		if(iDrive == DRIVE_REMOVABLE || iDrive == DRIVE_CDROM || iDrive == DRIVE_REMOTE ||
			iDrive == DRIVE_RAMDISK || strWinDirLower.GetAt(0) == strText[2])
		{
			pStore->DeleteString( i );
			iNumItems--;
			i--;
		}
		else
		{
			char strDriveAndTotal[_MAX_PATH];
			char strDriveLetter[10];
			char strTotal[20];
			int iTotal = 0;

			memset(strDriveLetter, 0, sizeof(strDriveLetter));
			sprintf(strDriveLetter, "%s\\", strDrive); 
			
			getLocalDriveSize(strDriveLetter, strTotal);

			if(strcmp(strTotal, "UnKnown") == 0)
			{
				memset(strDriveAndTotal, 0, sizeof(strDriveAndTotal));
				sprintf(strDriveAndTotal, "%s  %s MB", strText, strTotal);
				pStore->DeleteString( i );
				pStore->InsertString(i, strDriveAndTotal);
			}
			else
			{
				pStore->DeleteString( i );
				iNumItems--;
				i--;
			}
		}
	}
}

BOOL CSystem::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// [020712] AlRo v
	//char		strDir[100];
	//CComboBox	*PStoreList;
	//int		iNumItems, i;
	// [020712] AlRo ^
	CString		strPstoreLower = "";
	char		strLocalHost[_MAX_PATH];

    /* [02-10-10] ac V
	int iIP1,iIP2,iIP3,iIP4;

	m_EditPrimIP1.LimitText(3);
	m_EditPrimIP2.LimitText(3);
	m_EditPrimIP3.LimitText(3);
	m_EditPrimIP4.LimitText(3);

	m_EditSecIP1.LimitText(3);
	m_EditSecIP2.LimitText(3);
	m_EditSecIP3.LimitText(3);
	m_EditSecIP4.LimitText(3);
    ac ^ */
	
	// [020712] AlRo v
	//PStoreList = (CComboBox *)GetDlgItem(IDC_LIST_PSTORE);

	//memset(strDir, 0, sizeof(strDir));
	//strcpy(strDir,"[-*-]");
	//DlgDirListComboBox(strDir, IDC_LIST_PSTORE, 0, DDL_DRIVES);

	//checkDriveType();
	// [020712] AlRo ^

	if(lpConfig->m_bAddFileFlag) // In modify group mode
	{
		if(strlen(lpConfig->m_structSystemValues.m_strPrimaryHostName) == 0)
		{
			// get the local host name
			WORD wVersionRequested;
			WSADATA wsaData;
			wVersionRequested = MAKEWORD( 2, 2 );
			WSAStartup( wVersionRequested, &wsaData );
			gethostname(lpConfig->m_structSystemValues.m_strPrimaryHostName, sizeof(lpConfig->m_structSystemValues.m_strPrimaryHostName));
			WSACleanup( );
		}

        /* [02-10-10] ac V
		lpConfig->parseIPString(lpConfig->m_structSystemValues.m_strPrimaryHostName,
									&iIP1, &iIP2, &iIP3, &iIP4);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP1, iIP1, FALSE);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP2, iIP2, FALSE);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP3, iIP3, FALSE);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP4, iIP4, FALSE);

		lpConfig->parseIPString(lpConfig->m_structSystemValues.m_strSecondHostName,
									&iIP1, &iIP2, &iIP3, &iIP4);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP1, iIP1, FALSE);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP2, iIP2, FALSE);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP3, iIP3, FALSE);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP4, iIP4, FALSE);
        ac ^ */
		SetDlgItemText(IDC_EDIT_PRI_HOST_OR_IP, lpConfig->m_structSystemValues.m_strPrimaryHostName );
		SetDlgItemText(IDC_EDIT_SEC_HOST_OR_IP, lpConfig->m_structSystemValues.m_strSecondHostName );
		
		SetDlgItemText(IDC_EDIT_JOURNAL_DIR, lpConfig->m_structSystemValues.m_strJournalDir );
		SetDlgItemText(IDC_EDIT_PSTORE,      lpConfig->m_structSystemValues.m_strPStoreDev );
		SetDlgItemText(IDC_EDIT_SYSTEM_NOTE, lpConfig->m_structSystemValues.m_strNote );
		if(lpConfig->m_structSystemValues.m_bChaining)
		{
			CheckDlgButton(IDC_CHECK_CHAINING, 1);
		}
		else
		{
			CheckDlgButton(IDC_CHECK_CHAINING, 0);
		}
		
		SetDlgItemInt(IDC_EDIT_SECONDARY_PORT, lpConfig->m_structSystemValues.m_iPortNum, TRUE);

		// [020712] AlRo v
		//CString		strPstoreLower = "";
		//char		strText[_MAX_PATH];
		//BOOL		bFoundPStore = FALSE;

		//strPstoreLower = lpConfig->m_structSystemValues.m_strPStoreDev[0];
		//strPstoreLower.MakeUpper();

		
		//iNumItems = PStoreList->GetCount( );
		//if(iNumItems > 0)
		//{
		//	for(i = 0; i < iNumItems; i++)
		//	{
		//		memset(strText, 0, sizeof(strText));
		//		PStoreList->GetLBText(i, strText);
		//		if(strText[2] == strPstoreLower.GetAt(0))
		//		{
		//			//  One found so select it
		//			PStoreList->SetCurSel( i );
		//			bFoundPStore = TRUE;
		//		}
		//	}
		//	if(!bFoundPStore)
		//	{
		//		// Raw partitions but none selected, so set first as default
		//		PStoreList->SetCurSel( 0 );
		//	}
		//}
		//else
		//{
		//	// No raw partitions on machine
		//	PStoreList->InsertString(0, "No Raw Partitions Found");
		//}
		// [020712] AlRo ^
	}
	else // Add group
	{
		// [020712] AlRo v
		//iNumItems = PStoreList->GetCount( );

		//if(iNumItems > 0)
		//{
		//	// Raw partitions but none selected, so set first as default
		//	PStoreList->SetCurSel( 0 );
		//}
		//else
		//{
		//	// No raw partitions on machine
		//	PStoreList->InsertString(0, "No Raw Partitions Found");
		//}
		// [020712] AlRo ^

		SetDlgItemInt(IDC_EDIT_SECONDARY_PORT, lpConfig->m_structSystemValues.m_iPortNum, TRUE);
	
		// get the local host
		unsigned long ip;
		IN_ADDR in_addr;
		char	*ipStr;

		memset(strLocalHost, 0, sizeof(strLocalHost));
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 2 );
		WSAStartup( wVersionRequested, &wsaData );
		gethostname(strLocalHost, sizeof(strLocalHost));
		sock_name_to_ip(strLocalHost, &ip);
		in_addr.S_un.S_addr = ip;
		ipStr = inet_ntoa(in_addr);

        /* [02-10-10] ac V
		lpConfig->parseIPString(ipStr, &iIP1, &iIP2, &iIP3, &iIP4);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP1, iIP1, FALSE);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP2, iIP2, FALSE);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP3, iIP3, FALSE);
		SetDlgItemInt(IDC_EDIT_PRIMARY_IP4, iIP4, FALSE);
        ac ^ */
        strcpy( lpConfig->m_structSystemValues.m_strPrimaryHostName , ipStr );
		SetDlgItemText(IDC_EDIT_PRI_HOST_OR_IP, lpConfig->m_structSystemValues.m_strPrimaryHostName );

        /* [02-10-10] ac V
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP1, 127, FALSE);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP2,   0, FALSE);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP3,   0, FALSE);
		SetDlgItemInt(IDC_EDIT_SECONDARY_IP4,   1, FALSE);
        ac ^ */
        lpConfig->m_structSystemValues.m_strSecondHostName[0] = '\0';
		SetDlgItemText(IDC_EDIT_SEC_HOST_OR_IP, lpConfig->m_structSystemValues.m_strSecondHostName );

		SetDlgItemText(IDC_EDIT_JOURNAL_DIR, lpConfig->m_structSystemValues.m_strJournalDir );
		SetDlgItemText(IDC_EDIT_PSTORE,      lpConfig->m_structSystemValues.m_strPStoreDev );

		WSACleanup( );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/* ac 02-10-10  V
BOOL CSystem::PreTranslateMessage(MSG* pMsg) 
{
	//Code for splash screen added on 7-20-2001
	//by DTurrin@softek.fujitsu.com
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;
	
	return CPropertyPage::PreTranslateMessage(pMsg);
}
ac ^ */

// ==========================================
// OnHostnamePrim
//
// Opens the Host Name Dialog which finds the
// Host Name associated with the Primary IP
// Address entered in the System Dialog.
//
// ==========================================
/*
void CSystem::OnHostnamePrim() 
{
	char strSavedHost[255];
	int  iIP1,iIP2,iIP3,iIP4;
	int  nRet = -1;

	// Make a copy of the current Primary Host Name
	memset(strSavedHost, 0, sizeof(strSavedHost));
	sprintf(strSavedHost,lpConfig->m_structSystemValues.m_strPrimaryHostName);

	// Get the Dialog's Primary IP Address value
	iIP1 = GetDlgItemInt( IDC_EDIT_PRIMARY_IP1, NULL, FALSE );
	iIP2 = GetDlgItemInt( IDC_EDIT_PRIMARY_IP2, NULL, FALSE );
	iIP3 = GetDlgItemInt( IDC_EDIT_PRIMARY_IP3, NULL, FALSE );
	iIP4 = GetDlgItemInt( IDC_EDIT_PRIMARY_IP4, NULL, FALSE );
	memset(lpConfig->m_structSystemValues.m_strPrimaryHostName, 0, sizeof(lpConfig->m_structSystemValues.m_strPrimaryHostName));
	sprintf(lpConfig->m_structSystemValues.m_strPrimaryHostName, "%03i.%03i.%03i.%03i", iIP1, iIP2, iIP3, iIP4);

	// Open the "Host Name" Dialog
	CHostname hostWin;
	hostWin.host_Type = PRIM_HOST;
	nRet = hostWin.DoModal();
	switch (nRet)
	{
	case -1:
		AfxMessageBox("Dialog box could not be created!");
		return;
		break;
	case IDCANCEL:
	case IDABORT:
		return;
		break;
	default:
		break;
	}

	// If "Ok" was cliqued, replace the Dialog's current
	// Primary IP Address value with the one corresponding
	// to the host name typed in the "Host Name" Dialog
	if(strlen(lpConfig->m_structSystemValues.m_strPrimaryHostName) == 0)
	{
		// If no primary hostname has been chosen,
		// get the local host
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 2 );
		WSAStartup( wVersionRequested, &wsaData );
		gethostname(lpConfig->m_structSystemValues.m_strPrimaryHostName, sizeof(lpConfig->m_structSystemValues.m_strPrimaryHostName));
		WSACleanup( );
	}
	lpConfig->parseIPString(lpConfig->m_structSystemValues.m_strPrimaryHostName,
								&iIP1, &iIP2, &iIP3, &iIP4);
	SetDlgItemInt(IDC_EDIT_PRIMARY_IP1, iIP1, FALSE);
	SetDlgItemInt(IDC_EDIT_PRIMARY_IP2, iIP2, FALSE);
	SetDlgItemInt(IDC_EDIT_PRIMARY_IP3, iIP3, FALSE);
	SetDlgItemInt(IDC_EDIT_PRIMARY_IP4, iIP4, FALSE);

	// Restore the previously saved Primary Host Name value
	// in case the Cancel button is pressed.
	sprintf(lpConfig->m_structSystemValues.m_strPrimaryHostName,strSavedHost);
	
	return;
}

// ==========================================
// OnHostnameSec
//
// Opens the Host Name Dialog which finds the
// Host Name associated with the Secondary IP
// Address entered in the System Dialog.
//
// ==========================================
void CSystem::OnHostnameSec() 
{
	char strSavedHost[255];
	int iIP1,iIP2,iIP3,iIP4;
	int nRet = -1;
	
	// Make a copy of the current Secondary Host Name
	memset(strSavedHost, 0, sizeof(strSavedHost));
	sprintf(strSavedHost,lpConfig->m_structSystemValues.m_strSecondHostName);

	// Get the Dialog's Secondary IP Address value
	iIP1 = GetDlgItemInt( IDC_EDIT_SECONDARY_IP1, NULL, FALSE );
	iIP2 = GetDlgItemInt( IDC_EDIT_SECONDARY_IP2, NULL, FALSE );
	iIP3 = GetDlgItemInt( IDC_EDIT_SECONDARY_IP3, NULL, FALSE );
	iIP4 = GetDlgItemInt( IDC_EDIT_SECONDARY_IP4, NULL, FALSE );
	memset(lpConfig->m_structSystemValues.m_strSecondHostName, 0, sizeof(lpConfig->m_structSystemValues.m_strSecondHostName));
	sprintf(lpConfig->m_structSystemValues.m_strSecondHostName, "%03i.%03i.%03i.%03i", iIP1, iIP2, iIP3, iIP4);
	
	// Open the "Host Name" Dialog
	CHostname hostWin;
	hostWin.host_Type = SEC_HOST;
	nRet = hostWin.DoModal();
	switch (nRet)
	{
	case -1:
		AfxMessageBox("Dialog box could not be created!");
		return;
		break;
	case IDCANCEL:
	case IDABORT:
		return;
		break;
	default:
		break;
	}

	// If "Ok" was cliqued, replace the Dialog's current
	// Secondary IP Address value with the one corresponding
	// to the host name typed in the "Host Name" Dialog
	lpConfig->parseIPString(lpConfig->m_structSystemValues.m_strSecondHostName,
								&iIP1, &iIP2, &iIP3, &iIP4);
	SetDlgItemInt(IDC_EDIT_SECONDARY_IP1, iIP1, FALSE);
	SetDlgItemInt(IDC_EDIT_SECONDARY_IP2, iIP2, FALSE);
	SetDlgItemInt(IDC_EDIT_SECONDARY_IP3, iIP3, FALSE);
	SetDlgItemInt(IDC_EDIT_SECONDARY_IP4, iIP4, FALSE);

	// Restore the previously saved Secondary Host Name value
	// in case the Cancel button is pressed.
	sprintf(lpConfig->m_structSystemValues.m_strSecondHostName,strSavedHost);

	return;
}
*/
