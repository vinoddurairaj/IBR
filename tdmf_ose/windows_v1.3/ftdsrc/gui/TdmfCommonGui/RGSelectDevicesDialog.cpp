// RGSelectDevicesDialog.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGSelectDevicesDialog.h"
#include "ProgressInfoDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


class CCompareData
{
public:
	CListCtrl* m_pListCtrl;
	int        m_nColumn;
	BOOL       m_bAscending;

	CCompareData() : m_bAscending(FALSE) {}
};

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CCompareData* pCompareData = (CCompareData*)lParamSort;

	if(pCompareData->m_nColumn != 2)
	{
		CString strItem1 = pCompareData->m_pListCtrl->GetItemText(lParam1, pCompareData->m_nColumn);
		CString strItem2 = pCompareData->m_pListCtrl->GetItemText(lParam2, pCompareData->m_nColumn);

		return strcmp(strItem2, strItem1) * (pCompareData->m_bAscending ? 1 : -1);
	}
	else
	{
		TDMFOBJECTSLib::IDevicePtr pDevice1 = (TDMFOBJECTSLib::IDevice*)pCompareData->m_pListCtrl->GetItemData(lParam1/*, pCompareData->m_nColumn*/);
		TDMFOBJECTSLib::IDevicePtr pDevice2 = (TDMFOBJECTSLib::IDevice*)pCompareData->m_pListCtrl->GetItemData(lParam2/*, pCompareData->m_nColumn*/);

		_int64 num1 = _wtoi64(pDevice1->Size);
		_int64 num2 = _wtoi64(pDevice2->Size);
		if(num2 < num1)
			return -1 * (pCompareData->m_bAscending ? 1 : -1);
		else if(num2 > num1)
			return 1 * (pCompareData->m_bAscending ? 1 : -1);
		else
			return 0;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CRGSelectDevicesDialog dialog

CRGSelectDevicesDialog::CRGSelectDevicesDialog(TDMFOBJECTSLib::IReplicationGroup *pRG,
											   CTdmfCommonGuiDoc* pDoc,
											    CReplicationGroupPropertySheet* pRGPS,
											   CWnd* pParent /*=NULL*/,
											   CString* pRPStr /*=NULL*/)
	: CDialog(CRGSelectDevicesDialog::IDD, pParent), m_pRG(pRG), m_pDoc(pDoc), m_pRGPropertySheet(pRGPS)
{
	//{{AFX_DATA_INIT(CRGSelectDevicesDialog)
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
	m_pRPStr = pRPStr;

}

BOOL CRGSelectDevicesDialog::IsLocalTarget(TDMFOBJECTSLib::IDevicePtr pDevice)
{
	TDMFOBJECTSLib::IServerPtr pServer = m_pRG->GetParent();

	long nCountGroup = pServer->ReplicationGroupCount;
	for (long nIndexGroup = 0; nIndexGroup < nCountGroup; nIndexGroup++)
	{
		TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(nIndexGroup);

		if ((pRG->IsSource == FALSE) ||
			(pRG->IsSource && pRG->Parent->IsEqual(pRG->TargetServer)))
		{
			long nCountPair = pRG->ReplicationPairCount;

			for (long nIndexPair = 0; nIndexPair < nCountPair; nIndexPair++)
			{
				TDMFOBJECTSLib::IReplicationPairPtr pRP = pRG->GetReplicationPair(nIndexPair);

                CString strTargetName = (BSTR) pRP->TgtName;
                strTargetName.Replace(":\\",":");
                CString strDeviceName = (BSTR) pDevice->Name;
                strDeviceName.Replace(":\\",":");
                 
                if( strDeviceName.CompareNoCase( strTargetName ) == 0 )
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

BOOL CRGSelectDevicesDialog::IsSourceOrTargetOnTargetServer(TDMFOBJECTSLib::IDevicePtr pDevice)
{
	TDMFOBJECTSLib::IServerPtr pServer = m_pRG->TargetServer;

	CString strDeviceName = (BSTR)pDevice->Name;
	strDeviceName.Replace(":\\",":");

	long nCountGroup = pServer->ReplicationGroupCount;
	for (long nIndexGroup = 0; nIndexGroup < nCountGroup; nIndexGroup++)
	{
		TDMFOBJECTSLib::IReplicationGroupPtr pRG = pServer->GetReplicationGroup(nIndexGroup);

		long nCountPair = pRG->ReplicationPairCount;

		for (long nIndexPair = 0; nIndexPair < nCountPair; nIndexPair++)
		{
			TDMFOBJECTSLib::IReplicationPairPtr pRP = pRG->GetReplicationPair(nIndexPair);

            CString strSourceName = (BSTR) pRP->SrcName;
            strSourceName.Replace(":\\",":");
            CString strTargetName = (BSTR) pRP->TgtName;
            strTargetName.Replace(":\\",":");
             
 			if ((pRG->IsSource && pRG->Parent->IsEqual(pRG->TargetServer) && ( strDeviceName.CompareNoCase( strTargetName ) == 0 )) ||
				((!m_pRGPropertySheet->IsInChainingMode()) && (pRG->IsSource) && ( strDeviceName.CompareNoCase( strSourceName ) == 0 )) ||
				((!pRG->IsSource) && ( strDeviceName.CompareNoCase( strTargetName ) == 0 )))
			{
				return TRUE;
			}
		}
	}

	// Also check in current group (target group hasn't be created yet)
	long nCountPair = m_pRG->ReplicationPairCount;
	for (long nIndexPair = 0; nIndexPair < nCountPair; nIndexPair++)
	{
		TDMFOBJECTSLib::IReplicationPairPtr pRP = m_pRG->GetReplicationPair(nIndexPair);

		CString strTargetName = (BSTR)pRP->TgtName;
        strTargetName.Replace(":\\",":");

		if (strDeviceName.CompareNoCase(strTargetName) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CRGSelectDevicesDialog::IsAlreadySourceInGroup(TDMFOBJECTSLib::IDevicePtr pDevice)
{
	if (m_pRG->IsSource == TRUE)
	{
		long nCountPair = m_pRG->ReplicationPairCount;

		for (long nIndexPair = 0; nIndexPair < nCountPair; nIndexPair++)
		{
			TDMFOBJECTSLib::IReplicationPairPtr pRP = m_pRG->GetReplicationPair(nIndexPair);
		  
            CString strSourceName = (BSTR) pRP->SrcName;
            strSourceName.Replace(":\\",":");
            CString strDeviceName = (BSTR) pDevice->Name;
            strDeviceName.Replace(":\\",":");

			if (strDeviceName.CompareNoCase( strSourceName ) == 0 )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

void CRGSelectDevicesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGSelectDevicesDialog)
	DDX_Control(pDX, ID_REFRESH_DEVICES, m_RefreshButton);
	DDX_Control(pDX, IDC_SECONDARY_STATIC, m_SecondaryStatic);
	DDX_Control(pDX, IDC_PRIMARY_STATIC, m_PrimaryStatic);
	DDX_Control(pDX, IDC_DESCRIPTION_STATIC, m_DescriptionStatic);
	DDX_Control(pDX, IDC_BORDER, m_BorderButton);
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_DESCRIPTION_EDIT, m_DescriptionEdit);
	DDX_Control(pDX, IDC_SECONDARY_LIST, m_SecondaryListBox);
	DDX_Control(pDX, IDC_PRIMARY_LIST, m_PrimaryListBox);
	DDX_Text(pDX, IDC_DESCRIPTION_EDIT, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGSelectDevicesDialog, CDialog)
	//{{AFX_MSG_MAP(CRGSelectDevicesDialog)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_PRIMARY_LIST, OnColumnclickPrimaryList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_SECONDARY_LIST, OnColumnclickSecondaryList)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(ID_REFRESH_DEVICES, OnRefreshDevices)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGSelectDevicesDialog message handlers

BOOL CRGSelectDevicesDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
    CRect rect;

	ModifyStyle(0, WS_SYSMENU);

	CString primStr = "";
	primStr += "Primary Server '";
	primStr += _strupr(m_pRG->Parent->Name);
	primStr += "' :";
	m_PrimaryStatic.SetWindowText(primStr);

	m_PrimaryListBox.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_SecondaryListBox.SetExtendedStyle(LVS_EX_FULLROWSELECT);

    TDMFOBJECTSLib::IServerPtr pServer = m_pRG->GetParent();
	if (strstr(pServer->OSType, "Windows") != 0 ||
		strstr(pServer->OSType, "windows") != 0 ||
		strstr(pServer->OSType, "WINDOWS") != 0)
	{
		m_PrimaryListBox.InsertColumn(0, "Drive", LVCFMT_LEFT, 40);
		m_PrimaryListBox.InsertColumn(1, "File System", LVCFMT_LEFT, 75);
		m_PrimaryListBox.InsertColumn(2, "Size", LVCFMT_LEFT, 160);
		m_SecondaryListBox.InsertColumn(0, "Drive", LVCFMT_LEFT, 40);
		m_SecondaryListBox.InsertColumn(1, "File System", LVCFMT_LEFT, 75);
		m_SecondaryListBox.InsertColumn(2, "Size", LVCFMT_LEFT, 160);
		
   
        if(OnReadWindowSizeFromRegistry(rect))
        {
          MoveWindow(0, 0, rect.Width(), rect.Height());
        }
        else
        {
           MoveWindow(0, 0, 637, 445);
        }
       
	}
	else
	{
		m_PrimaryListBox.InsertColumn(0, "Location", LVCFMT_LEFT, 210);
		m_PrimaryListBox.InsertColumn(1, "Size", LVCFMT_LEFT, 165);
		m_SecondaryListBox.InsertColumn(0, "Location", LVCFMT_LEFT, 210);
		m_SecondaryListBox.InsertColumn(1, "Size", LVCFMT_LEFT, 165);
	
        if(OnReadWindowSizeFromRegistry(rect))
        {
          MoveWindow(0, 0, rect.Width(), rect.Height());
        }
        else
        {
           MoveWindow(0, 0, 837, 445);
        }
     	
	}
	
	CenterWindow();
	RefreshValues();

    m_PrimaryListBox.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER );
    m_SecondaryListBox.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CRGSelectDevicesDialog::RefreshValues()
{
	USES_CONVERSION;

	TDMFOBJECTSLib::IDevicePtr pDevice;
	TDMFOBJECTSLib::IServerPtr pServer;
	TDMFOBJECTSLib::IDeviceListPtr pDeviceListSource;
	TDMFOBJECTSLib::IDeviceListPtr pDeviceListTarget;

	pServer = m_pRG->GetParent();

	if (m_pDoc->m_mapDeviceList.find((char*)pServer->GetName()) != m_pDoc->m_mapDeviceList.end())
	{
		pDeviceListSource = m_pDoc->m_mapDeviceList[(char*)pServer->GetName()].m_T;
	}
	if (m_pDoc->m_mapDeviceList.find((char*)m_pRG->TargetServer->GetName()) != m_pDoc->m_mapDeviceList.end())
	{
		pDeviceListTarget = m_pDoc->m_mapDeviceList[(char*)m_pRG->TargetServer->GetName()].m_T;
	}

	if ((pDeviceListSource == NULL) || (pDeviceListTarget == NULL))
	{
		CWaitCursor Wait;
		CProgressInfoDlg ProgressInfoDlg("Querying servers for devices...", IDR_AVI_UPDATE_APP, this);
		
		pServer->GetDeviceLists(m_pRG->TargetServer, &pDeviceListSource, &pDeviceListTarget);

		m_pDoc->m_mapDeviceList[(char*)pServer->GetName()] = pDeviceListSource;
		m_pDoc->m_mapDeviceList[(char*)m_pRG->TargetServer->GetName()] = pDeviceListTarget;
	}


	for(int i=0; i < pDeviceListSource->DeviceCount; i++)
	{
		pDevice = pDeviceListSource->GetDevice(i);
		int nColumnIndex = 1;

		if ((!IsLocalTarget(pDevice)) && 
			(!IsAlreadySourceInGroup(pDevice))
			&& (pDevice->CanBeSource) // TargetVolumeTag
			)
		{
			CString str = "";
			str += OLE2A(pDevice->Name);
			int nIndex = m_PrimaryListBox.InsertItem(0, (LPCTSTR)str);
			
			if (strstr(pServer->OSType, "Windows") != 0 ||
				strstr(pServer->OSType, "windows") != 0 ||
				strstr(pServer->OSType, "WINDOWS") != 0)
			{
				str = "";
				str += OLE2A(pDevice->FileSystem);
				m_PrimaryListBox.SetItemText(0, nColumnIndex, (LPCTSTR)str);
				nColumnIndex++;
			}
			
			str = "";
			str += OLE2A(pDevice->Size);
			if(str.GetLength() < 4)
			{
				char numStr[256];
				NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
				GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
				str = numStr;
				str += " bytes";
			}
			else if(str.GetLength() < 7)
			{
				CString str2 = "";
				str2.Format("%.1f", atof((LPCTSTR)str)/1024);
				str2 += " KB (";
				char numStr[256];
				NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
				GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
				str = str2;
				str += numStr;
				str += " bytes)";
			}
			else if(str.GetLength() < 10)
			{
				CString str2 = "";
				str2.Format("%.1f", atof((LPCTSTR)str)/1048576);
				str2 += " MB (";
				char numStr[256];
				NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
				GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
				str = str2;
				str += numStr;
				str += " bytes)";
			}
			else
			{
				CString str2 = "";
				str2.Format("%.1f", atof((LPCTSTR)str)/1073741824);
				str2 += " GB (";
				char numStr[256];
				NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
				GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
				str = str2;
				str += numStr;
				str += " bytes)";
			}
			
			m_PrimaryListBox.SetItemText(0, nColumnIndex, (LPCTSTR)str);
			
			if(nIndex != -1)
			{
				m_PrimaryListBox.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IDevice*)pDevice));
				pDevice->AddRef();
			}
		}
	}
	
	if (m_pRG->TargetServer != NULL)
	{
		CString secStr = "";
		secStr += "Secondary Server '";
		secStr += _strupr(m_pRG->TargetServer->Name);
		secStr += "' :";
		m_SecondaryStatic.SetWindowText(secStr);

		for(int j=0; j < pDeviceListTarget->DeviceCount; j++)
		{
			pDevice = pDeviceListTarget->GetDevice(j);
			int nColumnIndex = 1;

			if ((IsSourceOrTargetOnTargetServer(pDevice) == FALSE)
				&& (pDevice->CanBeTarget)) // TargetVolumeTag
			{
				CString str = "";
				str += OLE2A(pDevice->Name);
				int nIndex = m_SecondaryListBox.InsertItem(0, (LPCTSTR)str);
				
				if (strstr(pServer->OSType, "Windows") != 0 ||
					strstr(pServer->OSType, "windows") != 0 ||
					strstr(pServer->OSType, "WINDOWS") != 0)
				{
					str = "";
					str += OLE2A(pDevice->FileSystem);
					m_SecondaryListBox.SetItemText(0, nColumnIndex, (LPCTSTR)str);
					nColumnIndex++;
				}
				
				str = "";
				str += OLE2A(pDevice->Size);
				if(str.GetLength() < 4)
				{
					char numStr[256];
					NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
					GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
					str = numStr;
					str += " bytes";
				}
				else if(str.GetLength() < 7)
				{
					CString str2 = "";
					str2.Format("%.1f", atof((LPCTSTR)str)/1024);
					str2 += " KB (";
					char numStr[256];
					NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
					GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
					str = str2;
					str += numStr;
					str += " bytes)";
				}
				else if(str.GetLength() < 10)
				{
					CString str2 = "";
					str2.Format("%.1f", atof((LPCTSTR)str)/1048576);
					str2 += " MB (";
					char numStr[256];
					NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
					GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
					str = str2;
					str += numStr;
					str += " bytes)";
				}
				else
				{
					CString str2 = "";
					str2.Format("%.1f", atof((LPCTSTR)str)/1073741824);
					str2 += " GB (";
					char numStr[256];
					NUMBERFMT NumberFmt = { 0, 0, 3, ".", ",", 1 };
					GetNumberFormat(NULL, NULL, (LPCTSTR)str, &NumberFmt, numStr, 256);
					str = str2;
					str += numStr;
					str += " bytes)";
				}
				
				m_SecondaryListBox.SetItemText(0, nColumnIndex, (LPCTSTR)str);
				
				if(nIndex != -1)
				{
					m_SecondaryListBox.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IDevice*)pDevice));
					pDevice->AddRef();
				}
			}
		}
	}

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &m_PrimaryListBox;
	CompareData.m_nColumn = 0;
	CompareData.m_bAscending = CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	m_PrimaryListBox.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
	
	CompareData.m_pListCtrl = &m_SecondaryListBox;
	CompareData.m_nColumn = 0;
	CompareData.m_bAscending = CompareData.m_bAscending;

	m_SecondaryListBox.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
	
	m_SecondaryListBox.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_PrimaryListBox.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);	

	m_OKButton.EnableWindow((m_PrimaryListBox.GetItemCount() >0 ) &&
							(m_SecondaryListBox.GetItemCount() >0 ));
}

void CRGSelectDevicesDialog::OnOK() 
{
	UpdateData();

	int iSourceIndex = m_PrimaryListBox.GetSelectionMark();
	int iTargetIndex = m_SecondaryListBox.GetSelectionMark();

	if ((iSourceIndex != LB_ERR) && (iTargetIndex != LB_ERR))
	{
		TDMFOBJECTSLib::IServerPtr pServer = m_pRG->GetParent();

		bool bWindows = (strstr(pServer->OSType, "Windows") != 0 ||
						 strstr(pServer->OSType, "windows") != 0 ||
						 strstr(pServer->OSType, "WINDOWS") != 0);
		
		m_pDeviceSource = (TDMFOBJECTSLib::IDevice*)m_PrimaryListBox.GetItemData(iSourceIndex);
		m_pDeviceTarget = (TDMFOBJECTSLib::IDevice*)m_SecondaryListBox.GetItemData(iTargetIndex);
			
		if (bWindows)
		{
			m_sSource = m_PrimaryListBox.GetItemText(iSourceIndex, 2);
			m_sTarget = m_SecondaryListBox.GetItemText(iTargetIndex, 2);
		}
		else
		{
			m_sSource = m_PrimaryListBox.GetItemText(iSourceIndex, 1);
			m_sTarget = m_SecondaryListBox.GetItemText(iTargetIndex, 1);
		}
		m_sSource.Delete(0, m_sSource.Find('(')+1);
		m_sSource.TrimRight(" bytes)");
		m_sSource.Remove(',');
		m_sTarget.Delete(0, m_sTarget.Find('(')+1);
		m_sTarget.TrimRight(" bytes)");
		m_sTarget.Remove(',');

		if (bWindows)
		{
			if (_atoi64(m_sSource) > _atoi64(m_sTarget))
			{
					MessageBox("You must select a target device that have a size greater or equal to the source device.", "Error");
					return;
			}
			else if (_atoi64(m_sTarget) > _atoi64(m_sSource))
			{
				if (MessageBox("Target device is larger than source device.\nDo you want to continue?", "Warning", MB_YESNO|MB_ICONQUESTION) == IDNO)
				{
					return;
				}
			}
		}
		else
		{
			if(_atoi64(m_sSource) != _atoi64(m_sTarget))
			{
				MessageBox("Devices size do not match, data could be lost.", "Warning", MB_OK|MB_ICONWARNING);
			}
		}

		if (bWindows)
		{
			m_sSource = m_PrimaryListBox.GetItemText(iSourceIndex, 1);
			m_sTarget = m_SecondaryListBox.GetItemText(iTargetIndex, 1);
		
			// Dont check BASIC/DYNAMIC attribute
			m_sSource.Replace(" DYNAMIC", "");
			m_sTarget.Replace(" DYNAMIC", "");

			if(m_sSource != m_sTarget)
			{
				MessageBox("You must select devices that have the same file system and disk type.", "Error");
				return;
			}
		}
		
		m_sSource = m_PrimaryListBox.GetItemText(iSourceIndex, 0);
		m_sTarget = m_SecondaryListBox.GetItemText(iTargetIndex, 0);

		if (m_pRG->GetParent()->IsEqual(m_pRG->TargetServer))
		{
			if((m_sSource == m_sTarget) ||
			   (bWindows && (m_pDeviceSource->DriveId == m_pDeviceTarget->DriveId) &&
                (m_pDeviceSource->StartOff == m_pDeviceTarget->StartOff)))
			{
				MessageBox("The devices chosen are the same. Please choose a different target device.", "Error");
				return;
			}
		}
	}

	CDialog::OnOK();
}

void CRGSelectDevicesDialog::OnDestroy() 
{
    OnWriteWindowSizeToRegistry() ;

	CDialog::OnDestroy();

	// Cleanup
	int nCount = m_PrimaryListBox.GetItemCount();

	for(int i = 0; i < nCount; i++)
	{
		TDMFOBJECTSLib::IDevice* pDevice = (TDMFOBJECTSLib::IDevice*)m_PrimaryListBox.GetItemData(i);
		pDevice->Release();
	}

	nCount = m_SecondaryListBox.GetItemCount();

	for(int j = 0; j < nCount; j++)
	{
		TDMFOBJECTSLib::IDevice* pDevice = (TDMFOBJECTSLib::IDevice*)m_SecondaryListBox.GetItemData(j);
		pDevice->Release();
	}


}

void CRGSelectDevicesDialog::OnColumnclickPrimaryList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &m_PrimaryListBox;
	CompareData.m_nColumn = pNMListView->iSubItem;
	CompareData.m_bAscending = !CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	m_PrimaryListBox.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
	
	*pResult = 0;
}

void CRGSelectDevicesDialog::OnColumnclickSecondaryList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &m_SecondaryListBox;
	CompareData.m_nColumn = pNMListView->iSubItem;
	CompareData.m_bAscending = !CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	m_SecondaryListBox.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);
	
	*pResult = 0;
}

void CRGSelectDevicesDialog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	if(::IsWindow(m_PrimaryListBox.GetSafeHwnd()))
	{
		m_PrimaryListBox.MoveWindow(27, 58, cx/2-32, cy-180);
		m_PrimaryStatic.MoveWindow(27, 38, cx/2-32, 13);
		m_SecondaryListBox.MoveWindow(cx/2+5, 58, cx/2-32, cy-180);
		m_SecondaryStatic.MoveWindow(cx/2+5, 38, cx/2-32, 13);
		m_DescriptionEdit.MoveWindow(27, cy-85, cx-55, 23);
		m_DescriptionStatic.MoveWindow(27, cy-105, 57, 13);
		m_BorderButton.MoveWindow(11, 11, cx-23, cy-60);

		m_OKButton.MoveWindow(cx - 172, cy-35, 75, 23);
		m_CancelButton.MoveWindow(cx - 87, cy-35, 75, 23);
		m_RefreshButton.MoveWindow(11, cy-35, 101, 23);
	}

	RedrawWindow();
}

int CRGSelectDevicesDialog::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	ModifyStyle(DS_MODALFRAME, WS_POPUP | WS_THICKFRAME);

	return 0;
}

void CRGSelectDevicesDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	lpMMI->ptMinTrackSize.x = 437;
	lpMMI->ptMinTrackSize.y = 345;
		
	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CRGSelectDevicesDialog::OnRefreshDevices() 
{
	// Cleanup
	int nCount = m_PrimaryListBox.GetItemCount();

	for(int i = 0; i < nCount; i++)
	{
		TDMFOBJECTSLib::IDevice* pDevice = (TDMFOBJECTSLib::IDevice*)m_PrimaryListBox.GetItemData(i);
		pDevice->Release();
	}

	nCount = m_SecondaryListBox.GetItemCount();

	for(int j = 0; j < nCount; j++)
	{
		TDMFOBJECTSLib::IDevice* pDevice = (TDMFOBJECTSLib::IDevice*)m_SecondaryListBox.GetItemData(j);
		pDevice->Release();
	}

	m_PrimaryListBox.DeleteAllItems();
	m_SecondaryListBox.DeleteAllItems();

	m_pDoc->m_mapDeviceList.erase((char*)m_pRG->Parent->GetName());
	m_pDoc->m_mapDeviceList.erase((char*)m_pRG->TargetServer->GetName());

	RefreshValues();
}


// Mike Pollett
#include "../../tdmf.inc"

BOOL CRGSelectDevicesDialog::OnReadWindowSizeFromRegistry(CRect& rect) 
{

    DWORD  width,height ;
    CRegKey RegKey;
    BOOL bResult = false;

   #define MENU_KEY "SOFTWARE\\" OEMNAME "\\Dtc\\CurrentVersion\\Views"

    // Open key
    if(RegKey.Create(HKEY_LOCAL_MACHINE,MENU_KEY) == ERROR_SUCCESS)
    {
        bool bSuccess = FALSE;

#if (_MSC_VER < 1300)
        if(RegKey.QueryValue(width, "DeviceWindowWidth") == ERROR_SUCCESS)
#else
		if(RegKey.QueryDWORDValue("DeviceWindowWidth", width) == ERROR_SUCCESS)
#endif
        {
#if (_MSC_VER < 1300)
            if(RegKey.QueryValue(height, "DeviceWindowHeight") == ERROR_SUCCESS)
#else
			if(RegKey.QueryDWORDValue("DeviceWindowHeight", height) == ERROR_SUCCESS)
#endif
            {
                bResult = true;
            }
        }
    }         

    if(bResult)
    {
        rect.SetRect(0, 0, width, height);
    }
   
    // Close key
    RegKey.Close();

    return bResult;
}

BOOL CRGSelectDevicesDialog::OnWriteWindowSizeToRegistry() 
{
    CRegKey RegKey;
    BOOL bResult = false;

   #define MENU_KEY "SOFTWARE\\" OEMNAME "\\Dtc\\CurrentVersion\\Views"


    // Open key
    if( RegKey.Create( HKEY_LOCAL_MACHINE, MENU_KEY) == ERROR_SUCCESS)
    {
        CRect rect;
        GetWindowRect(&rect);

         // Set value
#if (_MSC_VER < 1300)
         RegKey.SetValue(rect.Width(), "DeviceWindowWidth");
         RegKey.SetValue(rect.Height(), "DeviceWindowHeight");
#else
		RegKey.SetDWORDValue("DeviceWindowWidth", rect.Width());
		RegKey.SetDWORDValue("DeviceWindowHeight", rect.Height());
#endif

		// Close key
		RegKey.Close();

        bResult = true;
    }
  
    return bResult;
}
