// LocationEdit.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "LocationEdit.h"
#include "ProgressInfoDlg.h"
#include "RGSelectLocationDialog.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocationEdit

CLocationEdit::CLocationEdit()
{
}

CLocationEdit::~CLocationEdit()
{
}


BEGIN_MESSAGE_MAP(CLocationEdit, CEdit)
	//{{AFX_MSG_MAP(CLocationEdit)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBrowseButton)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLocationEdit message handlers

void CLocationEdit::OnBrowseButton()
{
	TDMFOBJECTSLib::IDeviceListPtr pDeviceList = NULL;

	if (m_pDoc->m_mapDeviceList.find((char*)m_pServer->GetName()) != m_pDoc->m_mapDeviceList.end())
	{
		pDeviceList = m_pDoc->m_mapDeviceList[(char*)m_pServer->GetName()].m_T;
	}

	if (pDeviceList == NULL)
	{
		CWaitCursor Wait;
		CProgressInfoDlg ProgressInfoDlg("Querying servers for devices...", IDR_AVI_UPDATE_APP, this);
		
		pDeviceList = m_pServer->GetDeviceList();

		m_pDoc->m_mapDeviceList[(char*)m_pServer->GetName()] = pDeviceList;
	}

	CRGSelectLocationDialog RGSelectLocationDialog;

	TDMFOBJECTSLib::IDevicePtr pDevice;
	for(int i=0; i < pDeviceList->DeviceCount; i++)
	{
		pDevice = pDeviceList->GetDevice(i);
		CString cstrDevice = (BSTR)pDevice->GetName();

		BOOL bWindows = (strstr(m_pServer->OSType, "Windows") != 0 ||
			    		 strstr(m_pServer->OSType, "windows") != 0 ||
					     strstr(m_pServer->OSType, "WINDOWS") != 0);

		BOOL bLinux   = (strstr(m_pServer->OSType, "Linux") != 0 ||
			    		 strstr(m_pServer->OSType, "linux") != 0 ||
					     strstr(m_pServer->OSType, "LINUX") != 0);

		if ((!bWindows) && (!bLinux))
		{
			// WR15984
			// Remove first 'r' that is after a '/'
			int nPos = cstrDevice.Find("/r");
			if (nPos != -1)
			{
				cstrDevice.Delete(nPos + 1);
			}
		}

		RGSelectLocationDialog.AddString(cstrDevice);
	}

	if (RGSelectLocationDialog.DoModal() == IDOK)
	{
		CString cstrLocation;
		if (RGSelectLocationDialog.GetSelection(cstrLocation))
		{
			SetWindowText(cstrLocation);
			GetParent()->PostMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_UPDATE), (LPARAM)GetParent()->m_hWnd);
		}
	}

	// Reset focus
	SetFocus();
}

void CLocationEdit::OnKillFocus(CWnd* pNewWnd) 
{
	CEdit::OnKillFocus(pNewWnd);
	
	DestroyBrowseButton();
}

void CLocationEdit::OnSetFocus(CWnd* pOldWnd) 
{
	CEdit::OnSetFocus(pOldWnd);
	
	CreateBrowseButton();	
}
