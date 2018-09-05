// DevicesPage.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "DevicesPage.h"
#include "PairCreationDlg.h"
#include "Command.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDevicesPage property page

IMPLEMENT_DYNCREATE(CDevicesPage, CPropertyPage)

CDevicesPage::CDevicesPage(CGroupConfig* pGroupConfig, LPCSTR lpcstrProductName)
 : CPropertyPage(CDevicesPage::IDD), m_pGroupConfig(pGroupConfig), m_cstrProductName(lpcstrProductName)
{
	//{{AFX_DATA_INIT(CDevicesPage)
	m_cstrRemarks = _T("");
	//}}AFX_DATA_INIT
}

void CDevicesPage::SetListBoxHorizontalExtent()
{
	// Find the longest string in the list box.
	CString str;
	CSize   sz;
	int     dx=0;
	CDC*    pDC = m_ListDevices.GetDC();
	for (int i = 0; i < m_ListDevices.GetCount(); i++)
	{
		m_ListDevices.GetText(i, str);
		sz = pDC->GetTextExtent(str + "          "); // add spaces cause \t aren't properly sized (5 spaces per \t)
		
		if (sz.cx > dx)
		{
			dx = sz.cx;
		}
	}
	m_ListDevices.ReleaseDC(pDC);
	
	// Set the horizontal extent so every character of all strings can be scrolled to.
	m_ListDevices.SetHorizontalExtent(dx);
}

void CDevicesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDevicesPage)
	DDX_Control(pDX, IDC_EDIT_DTC_REMARKS, m_EditRemarks);
	DDX_Control(pDX, IDC_LIST_DTC_DEVICES, m_ListDevices);
	DDX_Text(pDX, IDC_EDIT_DTC_REMARKS, m_cstrRemarks);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDevicesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDevicesPage)
	ON_LBN_SELCHANGE(IDC_LIST_DTC_DEVICES, OnSelchangeListDtcDevices)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY_DTC, OnButtonModifyDtc)
	ON_EN_CHANGE(IDC_EDIT_DTC_REMARKS, OnChangeEditDtcRemarks)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDevicesPage message handlers

BOOL CDevicesPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Write all pairs
	int nCount = m_pGroupConfig->GetPairCount();
	for(int i = 0; i < nCount; i++)
	{
		CDevicePairConfig* pDevicePairConfig = m_pGroupConfig->GetPair(i);

		CString cstrDevice;
		cstrDevice.Format("%s\t=>\t%s",
			CCommand::FormatDriveName(pDevicePairConfig->m_cstrDataDev),
			CCommand::FormatDriveName(pDevicePairConfig->m_cstrMirrorDev));

		m_ListDevices.AddString(cstrDevice);
	}
	
	if (nCount > 0)
	{
		m_ListDevices.SetCurSel(0);
		SetListBoxHorizontalExtent();
		OnSelchangeListDtcDevices();
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDevicesPage::OnSelchangeListDtcDevices()
{
	int nSelSected = m_ListDevices.GetCurSel();

	if (nSelSected != LB_ERR)
	{
		m_EditRemarks.SetReadOnly(FALSE);
		m_cstrRemarks = m_pGroupConfig->GetPair(nSelSected)->m_cstrRemarks;
	}
	else
	{
		m_EditRemarks.SetReadOnly();
		m_cstrRemarks = "";
	}

	UpdateData(FALSE);
}

void CDevicesPage::OnButtonAdd() 
{
	CPairCreationDlg PairCreationDlg(m_cstrProductName, m_pGroupConfig);

	if (PairCreationDlg.DoModal() == IDOK)
	{
		int nNewIndex = m_pGroupConfig->AddPair("", "", PairCreationDlg.m_cstrDataDevice, PairCreationDlg.m_cstrMirrorDevice, 0);

		CString cstrDevice;
		cstrDevice.Format("%s\t=>\t%s",
			CCommand::FormatDriveName(m_pGroupConfig->GetPair(nNewIndex)->m_cstrDataDev),
			CCommand::FormatDriveName(m_pGroupConfig->GetPair(nNewIndex)->m_cstrMirrorDev));

		m_ListDevices.SetCurSel(m_ListDevices.AddString(cstrDevice));

		SetListBoxHorizontalExtent();
		OnSelchangeListDtcDevices();
	}
}

void CDevicesPage::OnButtonModifyDtc() 
{
	int nSelSected = m_ListDevices.GetCurSel();

	CDevicePairConfig* pDevicePairConfig = m_pGroupConfig->GetPair(nSelSected);
	CPairCreationDlg PairCreationDlg(m_cstrProductName, m_pGroupConfig, pDevicePairConfig->m_cstrDataDev, pDevicePairConfig->m_cstrMirrorDev);

	if (PairCreationDlg.DoModal() == IDOK)
	{
		pDevicePairConfig->m_cstrDataDev = PairCreationDlg.m_cstrDataDevice;
		pDevicePairConfig->m_cstrMirrorDev = PairCreationDlg.m_cstrMirrorDevice;

		CString cstrDevice;
		cstrDevice.Format("%s\t=>\t%s",
			CCommand::FormatDriveName(pDevicePairConfig->m_cstrDataDev),
			CCommand::FormatDriveName(pDevicePairConfig->m_cstrMirrorDev));

		m_ListDevices.DeleteString(nSelSected);
		m_ListDevices.InsertString(nSelSected, cstrDevice);
		m_ListDevices.SetCurSel(nSelSected);
		SetListBoxHorizontalExtent();
		OnSelchangeListDtcDevices();
	}
}

void CDevicesPage::OnButtonDelete() 
{
	int nCurSelIndex = m_ListDevices.GetCurSel();

	if (nCurSelIndex >= 0)
	{
		m_pGroupConfig->RemovePair(nCurSelIndex);
		m_ListDevices.DeleteString(nCurSelIndex);

		int nCount = m_ListDevices.GetCount();
		if (nCount > 0)
		{
			m_ListDevices.SetCurSel(nCount-1);
		}

		SetListBoxHorizontalExtent();
		OnSelchangeListDtcDevices();
	}
}

BOOL CDevicesPage::Validate()
{
	return TRUE;
}

BOOL CDevicesPage::SaveValues()
{
	if (m_hWnd)
	{
		UpdateData();

		// Pairs are saved as they are created and removed

		// Remarks is saved on demand
		int nCurSelIndex = m_ListDevices.GetCurSel();
		if (nCurSelIndex >= 0)
		{
			m_pGroupConfig->GetPair(nCurSelIndex)->m_cstrRemarks = m_cstrRemarks;
		}
	}

	return TRUE;
}

void CDevicesPage::OnChangeEditDtcRemarks() 
{
	SaveValues();
}
