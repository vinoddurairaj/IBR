// DTCDevices.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "DTCDevices.h"
#include "Config.h"
#include "AddModMirrors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;
/////////////////////////////////////////////////////////////////////////////
// CDTCDevices property page

IMPLEMENT_DYNCREATE(CDTCDevices, CPropertyPage)

CDTCDevices::CDTCDevices() : CPropertyPage(CDTCDevices::IDD)
{
	//{{AFX_DATA_INIT(CDTCDevices)
	m_szDtcRemarks = _T("");
	//}}AFX_DATA_INIT

	m_iSelSelected = 0;
}

CDTCDevices::~CDTCDevices()
{
}

void CDTCDevices::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDTCDevices)
	DDX_Control(pDX, IDC_LIST_DTC_DEVICES, m_listDtcDevs);
	DDX_Text(pDX, IDC_EDIT_DTC_REMARKS, m_szDtcRemarks);
	DDV_MaxChars(pDX, m_szDtcRemarks, 80);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDTCDevices, CPropertyPage)
	//{{AFX_MSG_MAP(CDTCDevices)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH_DEVICE_LIST, OnButtonRefreshDeviceList)
	ON_WM_CANCELMODE()
	ON_BN_CLICKED(IDC_BUTTON_MODIFY_DTC, OnButtonModifyDtc)
	ON_LBN_SELCHANGE(IDC_LIST_DTC_DEVICES, OnSelchangeListDtcDevices)
	ON_EN_CHANGE(IDC_EDIT_DTC_REMARKS, OnChangeEditDtcRemarks)
	ON_EN_UPDATE(IDC_EDIT_DTC_REMARKS, OnUpdateEditDtcRemarks)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTCDevices message handlers

void CDTCDevices::OnButtonAdd() 
{
	CAddModMirrors	AddModMirrors;

	m_iSelSelected = m_listDtcDevs.GetCurSel( );
	lpConfig->m_iModDeleteSel = m_iSelSelected;

	m_iSelSelected = m_listDtcDevs.GetCount();

	clearDTCList();
	
	AddModMirrors.m_bAddOrMod =  TRUE;
	AddModMirrors.DoModal();

	if(AddModMirrors.m_bOK)
	{
		AddModMirrors.addDevice();

		lpConfig->m_listStructDTCDevValues.AddTail(lpConfig->m_structDTCDevValues);

//		OnInitDialog();	

		m_pos = lpConfig->m_listStructDTCDevValues.GetTailPosition();

		clearDTCRemark();
	}

	lpConfig->m_bAddFileFlag = TRUE;
	OnInitDialog();	
}

void CDTCDevices::OnButtonDelete() 
{
	CAddModMirrors addmodmirrors;

	m_iSelSelected = m_listDtcDevs.GetCurSel( );
	lpConfig->m_iModDeleteSel = m_iSelSelected;

	addmodmirrors.deleteDevice();

	if(m_iSelSelected >=0)
	{
		m_iSelSelected--;

		lpConfig->m_listStructDTCDevValues.RemoveAt( m_pos );
		
		clearDTCList();

		clearDTCRemark();
		
		OnInitDialog();
	}
}

void CDTCDevices::OnButtonRefreshDeviceList() 
{
	::MessageBox(NULL, "Refresh List", "DTC Devs", MB_OK);
	
}


BOOL CDTCDevices::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	lpConfig->m_iModDeleteSel = -1;

	memset(lpConfig->m_szCurrentDevices, 0, sizeof(lpConfig->m_szCurrentDevices));

	// Read in all profiles from the cfg and fill list
	char strDevice[_MAX_PATH];
	CListBox *pListDevices;

	pListDevices = (CListBox *)GetDlgItem(IDC_LIST_DTC_DEVICES);
	
	// Write all
	if(lpConfig->m_bAddFileFlag) // In modify group mode
	{
		int iDTCListCount = lpConfig->m_listStructDTCDevValues.GetCount();
		POSITION head = lpConfig->m_listStructDTCDevValues.GetHeadPosition();

		for(int i = 1; i <= iDTCListCount; i++)
		{
			clearNewDTC();
			lpConfig->m_structDTCDevValues = lpConfig->m_listStructDTCDevValues.GetNext(head);
			
			memset(strDevice, 0, sizeof(strDevice));
			sprintf(strDevice, "%s\t=>\t%s",
					lpConfig->m_structDTCDevValues.m_strDataDev, 
					lpConfig->m_structDTCDevValues.m_strMirrorDev);

			lpConfig->m_szCurrentDevices[i - 1] = lpConfig->m_structDTCDevValues.m_strMirrorDev[0];

			pListDevices->AddString( strDevice );
		}
		
		if(m_iSelSelected >= 0)
		{
			pListDevices->SetCurSel(m_iSelSelected);
			OnSelchangeListDtcDevices();
		}
		lpConfig->updateCurrentmirror();
	}
	else // Add group
	{
		lpConfig->m_listStructDTCDevValues.RemoveAll();
		lpConfig->m_listStructDTCDevValuesCpy.RemoveAll();
		clearNewDTC();
		clearDTCList();
	}	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDTCDevices::OnCancelMode() 
{
	CPropertyPage::OnCancelMode();	
}

void CDTCDevices::OnButtonModifyDtc() 
{
	CAddModMirrors	AddModMirrors;
	
	m_iSelSelected = m_listDtcDevs.GetCurSel( );
	lpConfig->m_iModDeleteSel = m_iSelSelected;
	
	if(m_iSelSelected >= 0)
	{
		clearDTCList();

		AddModMirrors.m_bAddOrMod =  FALSE;
		AddModMirrors.DoModal();
		
		if(AddModMirrors.m_bOK)
		{
			lpConfig->m_listStructDTCDevValues.SetAt( m_pos, lpConfig->m_structDTCDevValues);

			AddModMirrors.modDevice();
		//	OnInitDialog();
		}
		lpConfig->m_bAddFileFlag = TRUE;
		OnInitDialog();
	}
}

void CDTCDevices::OnSelchangeListDtcDevices() 
{
	int			iSelSected;
	CEdit		*editRemark;

	iSelSected = m_listDtcDevs.GetCurSel( );
	if (iSelSected == LB_ERR)
		return;

	m_pos = lpConfig->m_listStructDTCDevValues.FindIndex( iSelSected );
	clearNewDTC();
	lpConfig->m_structDTCDevValues = lpConfig->m_listStructDTCDevValues.GetAt( m_pos );

	editRemark = (CEdit *)GetDlgItem(IDC_EDIT_DTC_REMARKS);
	editRemark->SetReadOnly(FALSE);
	editRemark->SetSel(0, -1, FALSE);
	editRemark->ReplaceSel(lpConfig->m_structDTCDevValues.m_strRemarks ,FALSE);
}

void CDTCDevices::OnChangeEditDtcRemarks() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	CEdit		*editRemark;
	
	editRemark = (CEdit *)GetDlgItem(IDC_EDIT_DTC_REMARKS);

	clearNewDTC();

	lpConfig->m_structDTCDevValues = lpConfig->m_listStructDTCDevValues.GetAt( m_pos );
	
	// DTurrin - Sept 26th, 2001
	// To make sure that the remarks string ends with a NULL
	// character, insert NULL character in buffer's last position.
	if (editRemark->LineLength() == 0)
	{
		strcpy(lpConfig->m_structDTCDevValues.m_strRemarks,"");
	}
	else
	{
		editRemark->GetLine(0, lpConfig->m_structDTCDevValues.m_strRemarks, _MAX_PATH);
		lpConfig->m_structDTCDevValues.m_strRemarks[_MAX_PATH - 1] = NULL;
	}

	lpConfig->m_listStructDTCDevValues.SetAt( m_pos, lpConfig->m_structDTCDevValues);
}

void CDTCDevices::OnUpdateEditDtcRemarks() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.

}

void CDTCDevices::clearDTCList()
{
	CListBox		*pListDevices;
	int				iNumDev = 0;

	pListDevices = (CListBox *)GetDlgItem(IDC_LIST_DTC_DEVICES);
	iNumDev = pListDevices->GetCount();
	for(int i = iNumDev; i >= 0; i--)
		pListDevices->DeleteString(i);
}

void CDTCDevices::clearDTCRemark()
{
	CEdit	*editRemark;
	DWORD	dwSelLen = -1;		

	editRemark = (CEdit *)GetDlgItem(IDC_EDIT_DTC_REMARKS);
	
	dwSelLen = editRemark->GetSel();
	if(dwSelLen)
	{
		editRemark->SetSel(0, -1, FALSE);
		editRemark->ReplaceSel("" ,FALSE);
	}
	memset(lpConfig->m_structDTCDevValues.m_strRemarks, 0, sizeof(lpConfig->m_structDTCDevValues.m_strRemarks));
	lpConfig->m_listStructDTCDevValues.SetAt( m_pos, lpConfig->m_structDTCDevValues);
}

void CDTCDevices::clearNewDTC()
{
	lpConfig->m_structDTCDevValues.m_iNumDTCDevices = 0;
	memset(lpConfig->m_structDTCDevValues.m_strDataDev, 0, sizeof(lpConfig->m_structDTCDevValues.m_strDataDev));
	memset(lpConfig->m_structDTCDevValues.m_strDTCDev, 0, sizeof(lpConfig->m_structDTCDevValues.m_strDTCDev));
	memset(lpConfig->m_structDTCDevValues.m_strMirrorDev, 0, sizeof(lpConfig->m_structDTCDevValues.m_strMirrorDev));
	memset(lpConfig->m_structDTCDevValues.m_strRemarks, 0, sizeof(lpConfig->m_structDTCDevValues.m_strRemarks));
}
