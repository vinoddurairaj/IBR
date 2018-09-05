// RGReplicationPairsAdmPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGReplicationPairsAdmPage.h"
#include "RGSelectDevicesDialog.h"
#include "ReplicationGroupPropertySheet.h"
#include "RGGeneralPage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGReplicationPairsAdmPage property page

IMPLEMENT_DYNCREATE(CRGReplicationPairsAdmPage, CPropertyPage)

CRGReplicationPairsAdmPage::CRGReplicationPairsAdmPage(TDMFOBJECTSLib::IReplicationGroup *pRG, bool bReadOnly)
	 : CPropertyPage(CRGReplicationPairsAdmPage::IDD), m_pRG(pRG), m_bReadOnly(bReadOnly)
{
	//{{AFX_DATA_INIT(CRGReplicationPairsAdmPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bPageModified = false;
}

CRGReplicationPairsAdmPage::~CRGReplicationPairsAdmPage()
{
}

void CRGReplicationPairsAdmPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGReplicationPairsAdmPage)
	DDX_Control(pDX, IDC_DELETE_BUTTON, m_DeleteButton);
	DDX_Control(pDX, IDC_ADD_BUTTON, m_AddButton);
	DDX_Control(pDX, IDC_REPLICATION_PAIR_LIST, m_RPListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGReplicationPairsAdmPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGReplicationPairsAdmPage)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_DELETE_BUTTON, OnDeleteButton)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_CLICK, IDC_REPLICATION_PAIR_LIST, OnClickReplicationPairList)
	ON_NOTIFY(NM_DBLCLK, IDC_REPLICATION_PAIR_LIST, OnDblclkReplicationPairList)
	ON_NOTIFY(NM_RCLICK, IDC_REPLICATION_PAIR_LIST, OnRclickReplicationPairList)
	ON_NOTIFY(NM_RDBLCLK, IDC_REPLICATION_PAIR_LIST, OnRdblclkReplicationPairList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGReplicationPairsAdmPage message handlers

BOOL CRGReplicationPairsAdmPage::OnInitDialog() 
{
	USES_CONVERSION;

	CPropertyPage::OnInitDialog();
	
	m_bPageModified = false;
	
	m_RPListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_RPListCtrl.InsertColumn(0, "Source", LVCFMT_LEFT, 50);
	m_RPListCtrl.InsertColumn(1, "Target", LVCFMT_LEFT, 50);
	m_RPListCtrl.InsertColumn(2, "Description", LVCFMT_LEFT, 150);

	for(int i=0; i < m_pRG->ReplicationPairCount; i++)
	{
		TDMFOBJECTSLib::IReplicationPairPtr pRP;
		pRP = m_pRG->GetReplicationPair(i);

		if (pRP->ObjectState != TDMFOBJECTSLib::RPO_DELETED)
		{
			CString str = "";
			str += OLE2A(pRP->SrcName);
			int nIndex = m_RPListCtrl.InsertItem(0, (LPCTSTR)str);

			str = "";
			str += OLE2A(pRP->TgtName);
			m_RPListCtrl.SetItemText(0, 1, (LPCTSTR)str);

			str = "";
			str += OLE2A(pRP->Description);
			m_RPListCtrl.SetItemText(0, 2, (LPCTSTR)str);
			
			// Save a ref on the RP
			if (nIndex != LB_ERR)
			{
				m_RPListCtrl.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IReplicationPair*)pRP));
				pRP->AddRef();
			}
		}
	}
	
	m_RPListCtrl.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

	if ((m_RPListCtrl.GetItemCount() == 0) || m_bReadOnly)
	{
		m_DeleteButton.EnableWindow(FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRGReplicationPairsAdmPage::OnApply() 
{
	m_bPageModified = false;
	SetModified(FALSE);
	SendMessage (DM_SETDEFID, IDOK);
	
	return CPropertyPage::OnApply();
}

void CRGReplicationPairsAdmPage::OnAddButton() 
{
	CReplicationGroupPropertySheet* rgps = (CReplicationGroupPropertySheet*)GetParent();

	CRGSelectDevicesDialog rgsdd(m_pRG, rgps->m_pDoc, rgps);
	
	if(rgsdd.DoModal() == IDOK)
	{
		TDMFOBJECTSLib::IReplicationPairPtr pRP = m_pRG->CreateNewReplicationPair();
		pRP->CopyDevice(TRUE, rgsdd.m_pDeviceSource);
		pRP->CopyDevice(FALSE, rgsdd.m_pDeviceTarget);
		pRP->Description = (LPCTSTR)rgsdd.m_strDescription;

		CString str = "";
		str += rgsdd.m_sSource;
		int nIndex = m_RPListCtrl.InsertItem(0, (LPCTSTR)str);

		str = "";
		str += rgsdd.m_sTarget;
		m_RPListCtrl.SetItemText(0, 1, (LPCTSTR)str);

		str = "";
		str += rgsdd.m_strDescription;
		m_RPListCtrl.SetItemText(0, 2, (LPCTSTR)str);
		
		// Save a ref on the RP
		if (nIndex != LB_ERR)
		{
			m_RPListCtrl.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IReplicationPair*)pRP));
			pRP->AddRef();
		}

		m_DeleteButton.EnableWindow();
		m_RPListCtrl.SetItemState(m_RPListCtrl.GetItemCount()-1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_RPListCtrl.SetFocus();

		SetModified();
		m_bPageModified = true;
	}
}

void CRGReplicationPairsAdmPage::OnDeleteButton() 
{
	int iSelectedItem = m_RPListCtrl.GetSelectionMark();

	if(iSelectedItem != LB_ERR)
	{
		int result = MessageBox("Are you sure you want to delete this replication pair?", "Delete Replication Pair", MB_YESNO | MB_ICONQUESTION);

		if(result == IDYES)
		{
			TDMFOBJECTSLib::IReplicationPair* pRP = (TDMFOBJECTSLib::IReplicationPair*)m_RPListCtrl.GetItemData(iSelectedItem);
			// if it's an old object
			if (pRP->ObjectState == TDMFOBJECTSLib::RPO_SAVED)
			{
				// Mark the object as deleted
				pRP->ObjectState = TDMFOBJECTSLib::RPO_DELETED;
			}
			else
			{
				m_pRG->RemoveReplicationPair(pRP);
			}
			pRP->Release();

			m_RPListCtrl.DeleteItem(iSelectedItem);
			m_RPListCtrl.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

			if(m_RPListCtrl.GetItemCount() == 0)
			{
				m_DeleteButton.EnableWindow(FALSE);
			}

			SetModified();
			m_bPageModified = true;
		}
	}
}

BOOL CRGReplicationPairsAdmPage::OnSetActive() 
{
	CReplicationGroupPropertySheet* rgps = (CReplicationGroupPropertySheet*)GetParent();
	CRGGeneralPage* rggp = (CRGGeneralPage*)rgps->GetPage(0);

	if ((m_pRG->TargetServer == NULL) ||
		(m_pRG->TargetServer->IPAddress[0] == _bstr_t("0.0.0.0")) ||
		 m_bReadOnly)
	{
		m_AddButton.EnableWindow(FALSE);
	}
	else
	{
		m_AddButton.EnableWindow();
	}

	if(rggp->m_IsTargetServerModified)
	{
		m_RPListCtrl.DeleteAllItems();

		for(int i=0; i < m_pRG->ReplicationPairCount; i++)
		{
			TDMFOBJECTSLib::IReplicationPairPtr pRP = m_pRG->GetReplicationPair(i);
			ASSERT(pRP->ObjectState == TDMFOBJECTSLib::RPO_DELETED);
		}

		m_DeleteButton.EnableWindow(FALSE);

		rggp->m_IsTargetServerModified = false;
	}

	return CPropertyPage::OnSetActive();
}

void CRGReplicationPairsAdmPage::OnCancel() 
{
	// Restore initial state
	// Remove new item
	// Set deleted object back to saved
	
	TDMFOBJECTSLib::IReplicationPairPtr pRP;

	for(int i = m_pRG->ReplicationPairCount; i > 0; i--)
	{
		pRP = m_pRG->GetReplicationPair(i-1);  // Zero based index

		if (pRP->ObjectState == TDMFOBJECTSLib::RPO_NEW)
		{
			m_pRG->RemoveReplicationPair(pRP);
		}
		else if (pRP->ObjectState == TDMFOBJECTSLib::RPO_DELETED)
		{
			pRP->ObjectState = TDMFOBJECTSLib::RPO_SAVED;
		}
	}

	CPropertyPage::OnCancel();
}

void CRGReplicationPairsAdmPage::OnDestroy() 
{
	CPropertyPage::OnDestroy();

	// Cleanup
	int nCount = m_RPListCtrl.GetItemCount();
	if(nCount != LB_ERR)
	{
		for(int nIndex = 0; nIndex < nCount; nIndex++)
		{
			TDMFOBJECTSLib::IReplicationPair* pRP = (TDMFOBJECTSLib::IReplicationPair*)m_RPListCtrl.GetItemData(nIndex);
			pRP->Release();
		}
	}
}

void CRGReplicationPairsAdmPage::OnClickReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	int nHitItem = m_RPListCtrl.HitTest(pNMListView->ptAction);

	if ((nHitItem == -1) || m_bReadOnly)
	{
		m_DeleteButton.EnableWindow(FALSE);
	}
	else
	{
		m_DeleteButton.EnableWindow();
	}

	*pResult = 0;
}

void CRGReplicationPairsAdmPage::OnDblclkReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	int nHitItem = m_RPListCtrl.HitTest(pNMListView->ptAction);

	if ((nHitItem == -1) || m_bReadOnly)
	{
		m_DeleteButton.EnableWindow(FALSE);
	}
	else
	{
		m_DeleteButton.EnableWindow();
	}
	
	*pResult = 0;
}

void CRGReplicationPairsAdmPage::OnRclickReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	int nHitItem = m_RPListCtrl.HitTest(pNMListView->ptAction);

	if ((nHitItem == -1) || m_bReadOnly)
	{
		m_DeleteButton.EnableWindow(FALSE);
	}
	else
	{
		m_DeleteButton.EnableWindow();
	}
	
	*pResult = 0;
}

void CRGReplicationPairsAdmPage::OnRdblclkReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	int nHitItem = m_RPListCtrl.HitTest(pNMListView->ptAction);

	if ((nHitItem == -1) || m_bReadOnly)
	{
		m_DeleteButton.EnableWindow(FALSE);
	}
	else
	{
		m_DeleteButton.EnableWindow();
	}
	
	*pResult = 0;
}
