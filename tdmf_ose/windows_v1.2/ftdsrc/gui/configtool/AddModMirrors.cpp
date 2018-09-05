// AddModMirrors.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "AddModMirrors.h"
#include "Config.h"

#include <afxsock.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;
extern CDTCConfigPropSheet *lpPropsheet;
/////////////////////////////////////////////////////////////////////////////
// CAddModMirrors dialog


CAddModMirrors::CAddModMirrors(CWnd* pParent /*=NULL*/)
	: CDialog(CAddModMirrors::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddModMirrors)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddModMirrors::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddModMirrors)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddModMirrors, CDialog)
	//{{AFX_MSG_MAP(CAddModMirrors)
	ON_LBN_SELCHANGE(IDC_LIST_DATA_DEV, OnSelchangeListDataDev)
	ON_LBN_SELCHANGE(IDC_LIST_MIRROR_DEV, OnSelchangeListMirrorDev)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddModMirrors message handlers

void CAddModMirrors::OnSelchangeListDataDev() 
{
	// TODO: Add your control notification handler code here
	
}

void CAddModMirrors::OnSelchangeListMirrorDev() 
{
	// TODO: Add your control notification handler code here
	
}

void CAddModMirrors::OnOK() 
{
	CComboBox *pMirrorDev;
	lpConfig->m_iMirrorDev = 0;

	pMirrorDev = (CComboBox *)GetDlgItem(IDC_LIST_MIRROR_DEV);
	lpConfig->m_iMirrorDev = pMirrorDev->GetCurSel();

	if(!m_bAddOrMod)
	{
		this->DlgDirSelectComboBox( lpPropsheet->m_structDTCDevValues.m_strDataDev, IDC_LIST_DATA_DEV );
		this->DlgDirSelectComboBox( lpPropsheet->m_structDTCDevValues.m_strMirrorDev, IDC_LIST_MIRROR_DEV );

		this->DlgDirSelectComboBox( lpConfig->m_structDTCDevValues.m_strMirrorDev, IDC_LIST_MIRROR_DEV );
	}
	else
	{
		memset(lpPropsheet->m_structDTCDevValues.m_strDTCDev, 0, sizeof(lpPropsheet->m_structDTCDevValues.m_strDTCDev));
		this->DlgDirSelectComboBox( lpPropsheet->m_structDTCDevValues.m_strDataDev, IDC_LIST_DATA_DEV );
		this->DlgDirSelectComboBox( lpPropsheet->m_structDTCDevValues.m_strMirrorDev, IDC_LIST_MIRROR_DEV );

		memset(lpConfig->m_structDTCDevValues.m_strDTCDev, 0, sizeof(lpConfig->m_structDTCDevValues.m_strDTCDev));
		this->DlgDirSelectComboBox( lpConfig->m_structDTCDevValues.m_strDataDev, IDC_LIST_DATA_DEV );
		this->DlgDirSelectComboBox( lpConfig->m_structDTCDevValues.m_strMirrorDev, IDC_LIST_MIRROR_DEV );
	}

	m_bOK = TRUE;
	
	CDialog::OnOK();
}

void CAddModMirrors::OnCancel() 
{
	m_bOK = FALSE;
	lpConfig->m_iModDeleteSel = -1;
	
	CDialog::OnCancel();
}

void CAddModMirrors::getLocalDriveSize(char *szDrive, char *szSize)
{
	ULARGE_INTEGER FreeBytesAvailableToCaller, TotalNumberOfBytes, TotalNumberOfFreeBytes;
	int iTotal = 0;

	memset(szSize, 0, sizeof(szSize));
	if (GetDiskFreeSpaceEx(szDrive, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
	{
		iTotal = (int)(TotalNumberOfBytes.QuadPart / (1024 * 1024));
		itoa(iTotal, szSize, 10);
	}
	else
		strcpy(szSize, "UnKnown");
}


void CAddModMirrors::checkDriveType()
{
	CComboBox	*MirrorDev;
	CComboBox	*DataDev;
	int			iNumItems, i, n, index = 0;
	char		strText[_MAX_PATH], strDrive[_MAX_PATH], strWinDir[_MAX_PATH];
	char		szReceiveInfo[3 * 1024];
	UINT		iDrive;
	CString		strLowerWinDir = "";
	int			iDeviceCount = 0;
	int			iDeviceIndexByChar = 0;
	int			iDeviceIndex = 0;

	GetWindowsDirectory(strWinDir, _MAX_PATH);
	strLowerWinDir = strWinDir;
	strLowerWinDir.MakeUpper();
	
	memset(m_szRemoteDevices, 0, sizeof(m_szRemoteDevices));


	//Socket
	memset(szReceiveInfo, 0, sizeof(szReceiveInfo));
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	lpConfig->setSocketConnection(szReceiveInfo, sizeof(szReceiveInfo));
	lpConfig->disconnectSocket();
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	//End Socket

	if(strlen(szReceiveInfo) > 0)
	{
		// Fill in remote devices
		memset(lpConfig->m_szDeviceListInfo, 0, sizeof(lpConfig->m_szDeviceListInfo));

		MirrorDev = (CComboBox *)GetDlgItem(IDC_LIST_MIRROR_DEV);
		memset(strDrive, 0, sizeof(strDrive));
		while(szReceiveInfo[index] != 0)
		{
			n = 0;
			while(szReceiveInfo[index] != '{')
			{
				strDrive[n] = szReceiveInfo[index];
				index++;
				n++;
			}
			
			index = index + 2;
			iDeviceIndexByChar = 0;
			while(szReceiveInfo[index] != '\\')
			{
				lpConfig->m_szDeviceListInfo[iDeviceIndex] [iDeviceIndexByChar] = szReceiveInfo[index];
				
				iDeviceIndexByChar++;
				index++;
			}
			iDeviceIndex++;

			m_szRemoteDevices[iDeviceIndex - 1] = strDrive[2];
			
			index++;
			MirrorDev->AddString(strDrive);
			memset(strDrive, 0, sizeof(strDrive));

		}
		// Fill in remote devices
	}
	else
	{
		// fill a-z we did not get a connection
		char	chLetter = 65;
		MirrorDev = (CComboBox *)GetDlgItem(IDC_LIST_MIRROR_DEV);
		for(int x = 0; x < 26; x++)
		{
			memset(strDrive, 0, sizeof(strDrive));
			sprintf(strDrive, "[-%c-]  Unknown Size", chLetter);
			MirrorDev->AddString(strDrive);
			chLetter++;
		}
		// End no connection
	}
	
	// Primary devices
	DataDev = (CComboBox *)GetDlgItem(IDC_LIST_DATA_DEV);
	iNumItems = DataDev->GetCount( );
	for(i = 0; i < iNumItems; i++)
	{
		memset(strText, 0, sizeof(strText));
		memset(strDrive, 0, sizeof(strDrive));
		DataDev->GetLBText(i, strText);
		sprintf(strDrive, "%c:", strText[2]);
		strText[2] = strText[2] - 32;
		iDrive = GetDriveType(strDrive);
		if(iDrive == DRIVE_REMOVABLE || iDrive == DRIVE_CDROM || iDrive == DRIVE_REMOTE ||
				iDrive == DRIVE_RAMDISK || strText[2] == strLowerWinDir.GetAt(0))
		{
			DataDev->DeleteString( i );
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

			memset(strDriveAndTotal, 0, sizeof(strDriveAndTotal));
			sprintf(strDriveAndTotal, "%s  %s MB", strText, strTotal);
			DataDev->DeleteString( i );
			DataDev->InsertString(i, strDriveAndTotal);
			
		}
	}
	// End Primary devices
}

void CAddModMirrors::getMirrorDevInfo()
{
	int			n, index = 0;
	char		strDrive[_MAX_PATH];
	char		szReceiveInfo[3 * 1024];
	CString		strLowerWinDir = "";
	int			iDeviceCount = 0;
	int			iDeviceIndexByChar = 0;
	int			iDeviceIndex = 0;

	memset(m_szRemoteDevices, 0, sizeof(m_szRemoteDevices));
	memset(lpConfig->m_iMirrorIndexArray, 0, sizeof(lpConfig->m_iMirrorIndexArray));
	lpConfig->m_iMirrorDevIndex = 0;

	//Socket
	memset(szReceiveInfo, 0, sizeof(szReceiveInfo));
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	lpConfig->setSocketConnection(szReceiveInfo, sizeof(szReceiveInfo));
	lpConfig->disconnectSocket();
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	//End Socket

	if(strlen(szReceiveInfo) > 0)
	{
		// Fill in remote devices
		memset(lpConfig->m_szDeviceListInfo, 0, sizeof(lpConfig->m_szDeviceListInfo));

		memset(strDrive, 0, sizeof(strDrive));
		while(szReceiveInfo[index] != 0)
		{
			n = 0;
			while(szReceiveInfo[index] != '{')
			{
				strDrive[n] = szReceiveInfo[index];
				index++;
				n++;
			}
			
			index = index + 2;
			iDeviceIndexByChar = 0;
			while(szReceiveInfo[index] != '\\')
			{
				lpConfig->m_szDeviceListInfo[iDeviceIndex] [iDeviceIndexByChar] = szReceiveInfo[index];
				iDeviceIndexByChar++;
				index++;
			}
			iDeviceIndex++;

			m_szRemoteDevices[iDeviceIndex - 1] = strDrive[2];
			
			index++;
			memset(strDrive, 0, sizeof(strDrive));
		}
	}

}

void CAddModMirrors::addDevice()
{
	getMirrorDevInfo();

	for(int i = 0; (unsigned int)i < strlen(lpConfig->m_szCurrentDevices); i++)
	{
		for(int j = 0; (unsigned int)j < strlen(m_szRemoteDevices); j++)
		{
			if(lpConfig->m_szCurrentDevices[i] == m_szRemoteDevices[j])
			{
				lpConfig->m_iMirrorIndexArray[lpConfig->m_iMirrorDevIndex] = j;
				lpConfig->m_iMirrorDevIndex++;
			}
		}
	}

	lpConfig->m_iMirrorIndexArray[lpConfig->m_iMirrorDevIndex] = lpConfig->m_iMirrorDev;
}

void CAddModMirrors::modDevice()
{
	getMirrorDevInfo();

	if(lpConfig->m_iModDeleteSel >= 0)
	{
		lpConfig->m_szCurrentDevices[lpConfig->m_iModDeleteSel] = '+';
		if(lpConfig->m_iModDeleteSel == 0)
			lpConfig->m_iMirrorDevIndex++;
	}

	for(int i = 0; (unsigned int)i < strlen(lpConfig->m_szCurrentDevices); i++)
	{
		for(int j = 0; (unsigned int)j < strlen(m_szRemoteDevices); j++)
		{
			if(lpConfig->m_szCurrentDevices[i] == m_szRemoteDevices[j])
			{
				lpConfig->m_iMirrorIndexArray[lpConfig->m_iMirrorDevIndex] = j;
				lpConfig->m_iMirrorDevIndex++;
			}
		}
	}

	lpConfig->m_iMirrorIndexArray[lpConfig->m_iModDeleteSel] = lpConfig->m_iMirrorDev;
}

void CAddModMirrors::deleteDevice()
{
	getMirrorDevInfo();

	if(lpConfig->m_iModDeleteSel >= 0)
	{
		lpConfig->m_szCurrentDevices[lpConfig->m_iModDeleteSel] = '+';
	}

	for(int i = 0; (unsigned int)i < strlen(lpConfig->m_szCurrentDevices); i++)
	{
		for(int j = 0; (unsigned int)j < strlen(m_szRemoteDevices); j++)
		{
			if(lpConfig->m_szCurrentDevices[i] == m_szRemoteDevices[j])
			{
				lpConfig->m_iMirrorIndexArray[lpConfig->m_iMirrorDevIndex] = j;
				lpConfig->m_iMirrorDevIndex++;
			}
		}
	}
}

BOOL CAddModMirrors::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CComboBox  *MirrorDev;
	CComboBox *DataDev;

	char	strDir[100];
	memset(strDir, 0, sizeof(strDir));
	strcpy(strDir,"[-*-]");
	DlgDirListComboBox(strDir, IDC_LIST_DATA_DEV, 0, DDL_DRIVES);

	lpConfig->sockp = NULL;

	checkDriveType();

	DataDev = (CComboBox *)GetDlgItem(IDC_LIST_DATA_DEV);
	MirrorDev = (CComboBox *)GetDlgItem(IDC_LIST_MIRROR_DEV);

	if(!m_bAddOrMod)
	{
		if(lpConfig->m_bAddFileFlag)
		{
			int iNumItems, i;
			char strText[_MAX_PATH];

			DataDev->EnableWindow(FALSE);
			iNumItems = DataDev->GetCount( );
			for(i = 0; i <= iNumItems; i++)
			{
				memset(strText, 0, sizeof(strText));
				DataDev->GetLBText(i, strText);
				if(strText[2] == lpConfig->m_structDTCDevValues.m_strDataDev[0])
				{
					DataDev->SetCurSel( i );
				}
			}

			iNumItems = MirrorDev->GetCount( );
			for(i = 0; i <= iNumItems; i++)
			{
				memset(strText, 0, sizeof(strText));
				MirrorDev->GetLBText(i, strText);
				if(strText[2] == lpConfig->m_structDTCDevValues.m_strMirrorDev[0])
				{
					MirrorDev->SetCurSel( i );
				}
			}
	
		}
	}
	else
	{
		DataDev->SetCurSel( 0 );
		MirrorDev->SetCurSel( 0 );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
