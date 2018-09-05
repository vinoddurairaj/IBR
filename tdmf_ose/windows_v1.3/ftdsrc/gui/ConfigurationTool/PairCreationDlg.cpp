// PairCreationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "configurationtool.h"
#include "PairCreationDlg.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPairCreationDlg dialog


CPairCreationDlg::CPairCreationDlg(LPCSTR lpcstrProductName, CGroupConfig* pGroupConfig, LPCSTR lpcstrDataDev, LPCSTR lpcstrMirrorDev, CWnd* pParent /*=NULL*/)
	: CDialog(CPairCreationDlg::IDD, pParent), m_lpcstrDataDev(lpcstrDataDev), m_lpcstrMirrorDev(lpcstrMirrorDev),
	  m_cstrProductName(lpcstrProductName), m_pGroupConfig(pGroupConfig)
{
	//{{AFX_DATA_INIT(CPairCreationDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPairCreationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPairCreationDlg)
	DDX_Control(pDX, IDC_LIST_MIRROR_DEV, m_ListMirrorDev);
	DDX_Control(pDX, IDC_LIST_DATA_DEV, m_ListDataDev);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPairCreationDlg, CDialog)
	//{{AFX_MSG_MAP(CPairCreationDlg)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_LIST_DATA_DEV, OnSelchangeListDataDev)
	ON_CBN_SELCHANGE(IDC_LIST_MIRROR_DEV, OnSelchangeListMirrorDev)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPairCreationDlg message handlers

BOOL CPairCreationDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Fill the two ComboBox
	if (m_pGroupConfig->GetPrimaryHost().IsEmpty())
	{
		::MessageBox(NULL, "No Primary Machine Specified", m_cstrProductName, MB_OK);
	}
	else
	{
		GetDrivesInfo(m_pGroupConfig->GetPrimaryHost(), atoi(m_pGroupConfig->GetPrimaryPort()), m_pGroupConfig->GetGroupNb(), &m_ListDataDev);
	}
	if (m_pGroupConfig->GetSecondaryHost().IsEmpty())
	{
		::MessageBox(NULL, "No Secondary Machine Specified", m_cstrProductName, MB_OK);
	}
	else
	{
		GetDrivesInfo(m_pGroupConfig->GetSecondaryHost(), atoi(m_pGroupConfig->GetSecondaryPort()), m_pGroupConfig->GetGroupNb(), &m_ListMirrorDev, false);
	}

	// Select devices in list
	if (m_lpcstrDataDev == NULL)
	{
		m_ListDataDev.SetCurSel(0);
	}
	else
	{
		m_ListDataDev.EnableWindow(FALSE);

		int nCount = m_ListDataDev.GetCount();
		for (int i = 0; i < nCount; i++)
		{
			CString cstrText;
			m_ListDataDev.GetLBText(i, cstrText);
			cstrText.Remove('[');
			cstrText.Remove('-');
			cstrText.Remove(']');
			cstrText = CCommand::FormatDriveName(cstrText);

			CString cstrDataDev = CCommand::FormatDriveName(m_lpcstrDataDev);

			if (cstrText == cstrDataDev)
			{
				m_ListDataDev.SetCurSel(i);
			}
		}

	}
	OnSelchangeListDataDev();

	if (m_lpcstrMirrorDev == NULL)
	{
		m_ListMirrorDev.SetCurSel(0);
	}
	else
	{
		int nCount = m_ListMirrorDev.GetCount();
		for (int i = 0; i < nCount; i++)
		{
			CString cstrText;
			m_ListMirrorDev.GetLBText(i, cstrText);
			cstrText.Remove('[');
			cstrText.Remove('-');
			cstrText.Remove(']');
			cstrText = CCommand::FormatDriveName(cstrText);

			CString cstrMirrorDev = CCommand::FormatDriveName(m_lpcstrMirrorDev);

			if (cstrText == cstrMirrorDev)
			{
				m_ListMirrorDev.SetCurSel(i);
			}
		}
	}
	OnSelchangeListMirrorDev();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPairCreationDlg::GetDrivesInfo(LPCSTR lpcstrHost, UINT nPort, UINT nGroupId, CComboBox* pComboBox, bool bSource)
{
	#define IsPtrEqualTargetVolumeTag(_Value) (strstr(szReceiveInfo, "Target Volumes") == _Value) // TargetVolumeTag
	#define TargetVolumeTagSize (strlen("Target Volumes") + 1)									  // TargetVolumeTag

	// Connect to server to get device info
	char szReceiveInfo[3 * 1024];
	memset(szReceiveInfo, 0, sizeof(szReceiveInfo));
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	CString cstrTmpLocal = m_pGroupConfig->GetPrimaryHost();
	CString cstrTmpRemote = lpcstrHost;
	CCommand::GetRemoteDevicesRawInfo(m_cstrProductName, cstrTmpLocal.GetBuffer(256), cstrTmpRemote.GetBuffer(256), nPort, nGroupId, szReceiveInfo, sizeof(szReceiveInfo));
	cstrTmpLocal.ReleaseBuffer();
	cstrTmpRemote.ReleaseBuffer();
	
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	// Parse server's response
	if (strlen(szReceiveInfo) > 0)
	{
		int index = 0;
		bool bAddSource = true;  // TargetVolumeTag
		bool bAddTarget = false; // TargetVolumeTag

		if (IsPtrEqualTargetVolumeTag(NULL)) // TargetVolumeTag
		{									 // TargetVolumeTag
			bAddTarget = true;				 // TargetVolumeTag
		}									 // TargetVolumeTag

		while (szReceiveInfo[index] != 0)
		{
			if (IsPtrEqualTargetVolumeTag(&(szReceiveInfo[index])))  // TargetVolumeTag
			{														 // TargetVolumeTag
				bAddSource = false;									 // TargetVolumeTag
				bAddTarget = true;									 // TargetVolumeTag
				index += TargetVolumeTagSize;						 // TargetVolumeTag
			}														 // TargetVolumeTag
			else													 // TargetVolumeTag
			{
				int n = 0;
				char strDrive[_MAX_PATH];
				memset(strDrive, 0, sizeof(strDrive));

				while(szReceiveInfo[index] != '{')
				{
					strDrive[n] = szReceiveInfo[index];
					index++;
					n++;
				}
			
				index = index + 2; // TODO : why a +2?

				int iDeviceIndexByChar = 0;
				char strDriveInfo[_MAX_PATH];
				memset(strDriveInfo, 0, sizeof(strDriveInfo));

				while(szReceiveInfo[index] != '\\')
				{
					strDriveInfo[iDeviceIndexByChar] = szReceiveInfo[index];
				
					iDeviceIndexByChar++;
					index++;
				}
				index++;

				if ((bSource && bAddSource) || (!bSource && bAddTarget)) // TargetVolumeTag
				{
					int nNewEntryIndex = pComboBox->AddString(strDrive);
					pComboBox->SetItemDataPtr(nNewEntryIndex, new CString(strDriveInfo));
				}
			}
		}
	}
	else
	{
		// fill a-z: we did not get a connection
		for (int i = 0; i < 26; i++)
		{
			CString cstrDrive;
			cstrDrive.Format("[-%c-]  Unknown Size", 65 + i);
			pComboBox->AddString(cstrDrive);
		}
	}
}

void CPairCreationDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	for (int i = 0; i < m_ListDataDev.GetCount(); i++)
	{
		void* p = m_ListDataDev.GetItemDataPtr(i);
		if (p != NULL)
		{
			delete (CString*)p;
		}
	}

	for (i = 0; i < m_ListMirrorDev.GetCount(); i++)
	{
		void* p = m_ListMirrorDev.GetItemDataPtr(i);
		if (p != NULL)
		{
			delete (CString*)p;
		}
	}
}

void CPairCreationDlg::OnSelchangeListDataDev() 
{
	int nCurSel = m_ListDataDev.GetCurSel();

	if (nCurSel >= 0)
	{
		m_ListDataDev.GetLBText(nCurSel, m_cstrDataDevice);
		m_cstrDataDevice.Remove('[');
		m_cstrDataDevice.Remove('-');
		m_cstrDataDevice.Remove(']');
		m_cstrDataDevice = CCommand::FormatDriveName(m_cstrDataDevice);

		if (m_ListDataDev.GetItemDataPtr(nCurSel) != NULL)
		{
			m_cstrDataDevice += " ";
			m_cstrDataDevice += *(CString*)m_ListDataDev.GetItemDataPtr(nCurSel);
		}
	}
	else
	{
		m_cstrDataDevice = "";
	}
}

void CPairCreationDlg::OnSelchangeListMirrorDev() 
{
	int nCurSel = m_ListMirrorDev.GetCurSel();

	if (nCurSel  >= 0)
	{
		m_ListMirrorDev.GetLBText(nCurSel, m_cstrMirrorDevice);
		m_cstrMirrorDevice.Remove('[');
		m_cstrMirrorDevice.Remove('-');
		m_cstrMirrorDevice.Remove(']');
		m_cstrMirrorDevice = CCommand::FormatDriveName(m_cstrMirrorDevice);

		if (m_ListMirrorDev.GetItemDataPtr(nCurSel) != NULL)
		{
			m_cstrMirrorDevice += " ";
			m_cstrMirrorDevice += *(CString*)m_ListMirrorDev.GetItemDataPtr(nCurSel);
		}
	}
	else
	{
		m_cstrMirrorDevice = "";
	}
}
