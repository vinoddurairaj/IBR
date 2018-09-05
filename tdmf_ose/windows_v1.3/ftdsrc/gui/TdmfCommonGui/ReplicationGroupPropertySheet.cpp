// ReplicationGroupPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ReplicationGroupPropertySheet.h"
#include "ViewNotification.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupPropertySheet

IMPLEMENT_DYNAMIC(CReplicationGroupPropertySheet, CPropertySheet)

CReplicationGroupPropertySheet::CReplicationGroupPropertySheet(	CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage, TDMFOBJECTSLib::IReplicationGroup* pRG, bool bNewItem, bool bNewSymmetric)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
	 m_RGGeneralPage(pRG, bNewItem, (pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)),
	 m_RGTunablePage(pRG, ((strstr(pRG->Parent->OSType, "Windows") != 0) || 
						   (strstr(pRG->Parent->OSType, "windows") != 0) ||
						   (strstr(pRG->Parent->OSType, "WINDOWS") != 0))),
	 m_RGReplicationPairsAdmPage(pRG, (m_pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)),
	 m_RGThrottlePage(pRG, (pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)),
	 m_RGSymmetricPage(pRG, bNewSymmetric),
	 m_pDoc(pDoc), m_pRG(pRG), m_bNewItem(bNewItem)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;

	AddPage(&m_RGGeneralPage);
	AddPage(&m_RGTunablePage);
	AddPage(&m_RGReplicationPairsAdmPage);
	if (m_pRG->Symmetric)
	{
		AddPage(&m_RGSymmetricPage);
	}
		
	bool bIsWindows = ( strstr(pRG->Parent->OSType,"Windows") != 0 ||
							strstr(pRG->Parent->OSType,"windows") != 0 ||
							strstr(pRG->Parent->OSType,"WINDOWS") != 0 );

	if(!bIsWindows)
		AddPage(&m_RGThrottlePage);
}

CReplicationGroupPropertySheet::CReplicationGroupPropertySheet(	CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage, TDMFOBJECTSLib::IReplicationGroup* pRG, bool bNewItem, bool bNewSymmetric)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
	 m_RGGeneralPage(pRG, bNewItem, (pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)),
	 m_RGTunablePage(pRG, ((strstr(pRG->Parent->OSType, "Windows") != 0) || 
						   (strstr(pRG->Parent->OSType, "windows") != 0) ||
						   (strstr(pRG->Parent->OSType, "WINDOWS") != 0))),
	 m_RGReplicationPairsAdmPage(pRG, (pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)),
	 m_RGThrottlePage(pRG, (pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)),
	 m_RGSymmetricPage(pRG, bNewSymmetric),
	 m_pDoc(pDoc), m_pRG(pRG), m_bNewItem(bNewItem)
{
	m_psh.dwFlags = m_psh.dwFlags | PSH_NOAPPLYNOW;

	AddPage(&m_RGGeneralPage);
	AddPage(&m_RGTunablePage);
	AddPage(&m_RGReplicationPairsAdmPage);
	if (m_pRG->Symmetric)
	{
		AddPage(&m_RGSymmetricPage);
	}

	bool bIsWindows = ( strstr(pRG->Parent->OSType,"Windows") != 0 ||
							strstr(pRG->Parent->OSType,"windows") != 0 ||
							strstr(pRG->Parent->OSType,"WINDOWS") != 0 );

	if(!bIsWindows)
		AddPage(&m_RGThrottlePage);
}

CReplicationGroupPropertySheet::~CReplicationGroupPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CReplicationGroupPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CReplicationGroupPropertySheet)
	ON_COMMAND(ID_APPLY_NOW, OnApplyNow)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupPropertySheet message handlers


BOOL CReplicationGroupPropertySheet::IsInChainingMode()
{
  return m_RGGeneralPage.m_bChaining;
}

void CReplicationGroupPropertySheet::OnApplyNow()
{
	CWaitCursor WaitCursor;
	BOOL bSave = FALSE;

	if ((m_pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF) &&
		(m_RGGeneralPage.m_bPageModified || m_RGReplicationPairsAdmPage.m_bPageModified))
	{
		MessageBox("Replication group is currently running.  "
				   "Changes will be apply the next time you restart it.",
				   "Warning", MB_ICONINFORMATION|MB_OK);
	}

	if (m_RGGeneralPage.m_bPageModified && m_RGGeneralPage.OnKillActive())
	{
		m_RGGeneralPage.OnApply();
		bSave = TRUE;
	}
				
	if (m_RGReplicationPairsAdmPage.m_bPageModified && m_RGReplicationPairsAdmPage.OnKillActive())
	{
		m_RGReplicationPairsAdmPage.OnApply();
		bSave = TRUE;
	}
	
	if (m_RGTunablePage.m_bPageModified && m_RGTunablePage.OnKillActive())
	{
		m_RGTunablePage.OnApply();
		
		if (bSave == FALSE)
		{
			// save tunable
			if (m_pRG->SaveTunables() != 0)
			{
				MessageBox("Cannot save changes to database.", "Error", MB_OK | MB_ICONERROR);
			}
			else
			{
				// Fire an Object Change Notification		
				CViewNotification VN;
				VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_CHANGE;
				VN.m_pUnk = (IUnknown*)m_pRG;
				VN.m_eParam = CViewNotification::PROPERTIES;
				m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
			}
		}
		
		if (m_pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF)
		{
			// Apply tunable
			if (m_pRG->SetTunables() != 0)
			{
				MessageBox("Cannot apply changes.", "Error", MB_OK | MB_ICONERROR);
			}
		}
	}
	
	if (m_RGSymmetricPage.m_bPageModified && m_RGSymmetricPage.OnKillActive())
	{
		m_RGSymmetricPage.OnApply();
		bSave = TRUE;
	}
	
	if (bSave)
	{
		// Save changes to DB
		CComBSTR bstrWarningMsg;
		long nErrCode = m_pRG->SaveToDB(m_RGGeneralPage.m_nGroupNbSaved,
			m_RGGeneralPage.m_pTargetServerSaved ? m_RGGeneralPage.m_pTargetServerSaved->HostID : 0, &bstrWarningMsg);
		
		if (nErrCode != 0)
		{
			switch (nErrCode)
			{
			case TDMFOBJECTSLib::TDMF_ERROR_DELETING_DB_RECORD:
			case TDMFOBJECTSLib::TDMF_ERROR_CREATING_DB_RECORD:
			case TDMFOBJECTSLib::TDMF_ERROR_UPDATING_DB_RECORD:
			case TDMFOBJECTSLib::TDMF_ERROR_DATABASE_TRANSACTION:
				MessageBox("Cannot save changes to database.", "Error", MB_OK | MB_ICONERROR);
				break;	
				
			case TDMFOBJECTSLib::TDMF_ERROR_INTERNAL_ERROR:
			case TDMFOBJECTSLib::TDMF_ERROR_SET_SOURCE_REP_GROUP:
			case TDMFOBJECTSLib::TDMF_ERROR_SET_TARGET_REP_GROUP:
			case TDMFOBJECTSLib::TDMF_ERROR_DELETING_TARGET_REP_GROUP:
			case TDMFOBJECTSLib::TDMF_ERROR_DELETING_SOURCE_REP_GROUP:
				MessageBox("Cannot send changes to servers.", "Error", MB_OK | MB_ICONERROR);
				break;
			}
		}

		if (bstrWarningMsg.Length() > 0)
		{
			MessageBoxW(::GetActiveWindow(), bstrWarningMsg, L"Warning", MB_OK | MB_ICONWARNING);
		}
		
		// If it's a new group send an ADD notification, otherwise send a CHANGE notification
		if (m_bNewItem)
		{
			m_bNewItem = FALSE;
		}
		
		// Fire an Object Change Notification
		CViewNotification::CHANGE_PARAMS eParam = CViewNotification::PROPERTIES;
		if(m_pRG->Name != m_RGGeneralPage.m_bstrNameSaved)
		{
			eParam = (CViewNotification::CHANGE_PARAMS)(eParam | CViewNotification::NAME);
		}
		
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_CHANGE;
		VN.m_pUnk = (IUnknown*)m_pRG;
		VN.m_eParam = eParam;
		VN.m_dwParam1 = (DWORD)(BSTR)m_RGGeneralPage.m_bstrNameSaved;
		m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
	}
}

void CReplicationGroupPropertySheet::OnOK()
{
	if (!(m_RGGeneralPage.m_bPageModified || m_RGTunablePage.m_bPageModified || m_RGReplicationPairsAdmPage.m_bPageModified || m_RGSymmetricPage.m_bPageModified))
	{
		EndDialog(IDOK);
	}

	if (((!m_RGGeneralPage.m_bPageModified) || m_RGGeneralPage.OnKillActive()) &&
		((!m_RGTunablePage.m_bPageModified) || m_RGTunablePage.OnKillActive()) &&
		((!m_RGReplicationPairsAdmPage.m_bPageModified) || m_RGReplicationPairsAdmPage.OnKillActive()) &&
		((!m_RGSymmetricPage.m_bPageModified) || m_RGSymmetricPage.OnKillActive()))
	{
		OnApplyNow();

		EndDialog(IDOK);
	}
}

void CReplicationGroupPropertySheet::OnCancel()
{
	m_RGGeneralPage.OnCancel();
	m_RGTunablePage.OnCancel();
	m_RGReplicationPairsAdmPage.OnCancel();
	m_RGSymmetricPage.OnCancel();

	if (m_bNewItem)
	{
		// Fire an Object New (add) Notification
		CViewNotification VN;
		VN.m_nMessageId = CViewNotification::REPLICATION_GROUP_REMOVE;
		VN.m_pUnk = (IUnknown*) m_pRG;
		m_pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);

		if (m_pRG->Parent->RemoveReplicationGroup(m_pRG) == S_FALSE)
		{
			CString strMsg;
			strMsg.Format("Unable to delete replication group '%S'?\n\n",
				(BSTR)m_pRG->Name);
			MessageBox(strMsg, "Replication Group Deletion Error", MB_OK|MB_ICONINFORMATION);
		}
	}

	EndDialog(IDCANCEL);
}


BOOL CReplicationGroupPropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	CWnd* pOKButton = GetDlgItem (IDOK);
	pOKButton->SetWindowText("Save");
	
	return bResult;
}
