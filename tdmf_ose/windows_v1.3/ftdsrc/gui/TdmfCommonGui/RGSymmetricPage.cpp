// RGSymmetricPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGSymmetricPage.h"
#include "ReplicationGroupPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGSymmetricPage property page

IMPLEMENT_DYNCREATE(CRGSymmetricPage, CPropertyPage)

CRGSymmetricPage::CRGSymmetricPage(TDMFOBJECTSLib::IReplicationGroup *pRG, bool bNewSymmetric) :
	CPropertyPage(CRGSymmetricPage::IDD), m_bPageModified(bNewSymmetric), m_pRG(pRG), m_bNewSymmetric(bNewSymmetric)
{
	//{{AFX_DATA_INIT(CRGSymmetricPage)
	m_bSymmetricNormallyStarted = FALSE;
	m_nSymmetricGroupNumber = 0;
	m_nFailoverInitialState = 1;
	m_cstrMode = _T("");
	m_cstrConnectionState = _T("");
	m_cstrSymmetricJournal = _T("");
	m_cstrSymmetricPStore = _T("");
	m_cstrPStoreTitle = _T("");
	//}}AFX_DATA_INIT
}

CRGSymmetricPage::~CRGSymmetricPage()
{
}

void CRGSymmetricPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGSymmetricPage)
	DDX_Control(pDX, IDC_EDIT_SYMMETRIC_GROUP_NUMBER, m_EditSymmGroupNumber);
	DDX_Control(pDX, IDC_CHECK_SYMMETRIC_GROUP_NORMALLY_STARTED, m_CheckboxSymmGroupStarted);
	DDX_Control(pDX, IDC_EDIT_JOURNAL, m_EditSymmetricJournal);
	DDX_Control(pDX, IDC_EDIT_PSTORE, m_EditSymmetricPStore);
	DDX_Check(pDX, IDC_CHECK_SYMMETRIC_GROUP_NORMALLY_STARTED, m_bSymmetricNormallyStarted);
	DDX_Text(pDX, IDC_EDIT_SYMMETRIC_GROUP_NUMBER, m_nSymmetricGroupNumber);
	DDX_Radio(pDX, IDC_RADIO_PASSTHRU, m_nFailoverInitialState);
	DDX_Text(pDX, IDC_MODE, m_cstrMode);
	DDX_Text(pDX, IDC_STATUS, m_cstrConnectionState);
	DDX_Text(pDX, IDC_EDIT_JOURNAL, m_cstrSymmetricJournal);
	DDX_Text(pDX, IDC_EDIT_PSTORE, m_cstrSymmetricPStore);
	DDX_Text(pDX, IDC_PSTORE_TITLE, m_cstrPStoreTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGSymmetricPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGSymmetricPage)
	ON_BN_CLICKED(IDC_CHECK_SYMMETRIC_GROUP_NORMALLY_STARTED, OnCheckSymmetricGroupNormallyStarted)
	ON_BN_CLICKED(IDC_RADIO_PASSTHRU, OnRadioPassthru)
	ON_BN_CLICKED(IDC_RADIO_TRACKING, OnRadioTracking)
	ON_EN_UPDATE(IDC_EDIT_SYMMETRIC_GROUP_NUMBER, OnUpdateEditSymmetricGroupNumber)
	ON_EN_UPDATE(IDC_EDIT_JOURNAL, OnUpdateEditJournal)
	ON_EN_UPDATE(IDC_EDIT_PSTORE, OnUpdateEditPstore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGSymmetricPage message handlers

BOOL CRGSymmetricPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	try
	{
		if (m_pRG != NULL)
		{
			bool bIsWindows = ( strstr(m_pRG->Parent->OSType,"Windows") != 0 ||
								strstr(m_pRG->Parent->OSType,"windows") != 0 ||
								strstr(m_pRG->Parent->OSType,"WINDOWS") != 0 );

			m_bSymmetricNormallyStarted = m_pRG->SymmetricNormallyStarted;

			if (m_bNewSymmetric)
			{
				m_nSymmetricGroupNumber = (m_pRG->GroupNumber < 500) ? 500 + m_pRG->GroupNumber : m_pRG->GroupNumber - 500;
			}
			else
			{
				m_nSymmetricGroupNumber = m_pRG->SymmetricGroupNumber;
			}

			switch (m_pRG->GetFailoverInitialState())
			{
			case TDMFOBJECTSLib::FTD_MODE_PASSTHRU:
				m_nFailoverInitialState = 0;
				break;
			case TDMFOBJECTSLib::FTD_MODE_TRACKING:
				m_nFailoverInitialState = 1;
				break;
			}

			bool bStarted = true;
			switch (m_pRG->GetSymmetricMode())
			{
			case TDMFOBJECTSLib::FTD_MODE_PASSTHRU:
				m_cstrMode = "Passthru";
				break;
			case TDMFOBJECTSLib::FTD_MODE_NORMAL:
				m_cstrMode = "Normal";
				break;
			case TDMFOBJECTSLib::FTD_MODE_TRACKING:
				m_cstrMode = "Tracking";
				break;
			case TDMFOBJECTSLib::FTD_MODE_REFRESH:
				m_cstrMode = "Refresh";
				break;
			case TDMFOBJECTSLib::FTD_MODE_BACKFRESH:
				m_cstrMode = "Backfresh";
				break;
			case TDMFOBJECTSLib::FTD_MODE_CHECKPOINT:
				m_cstrMode = "Checkpoint";
				break;
			case TDMFOBJECTSLib::FTD_M_UNDEF:
				m_cstrMode = "Not Started";
				bStarted = false;
				break;
			}
			if (bStarted)
			{
				switch (m_pRG->GetSymmetricConnectionStatus())
				{
				case TDMFOBJECTSLib::FTD_PMD_ONLY:
					m_cstrConnectionState = "PMD Only";
					break;
				case TDMFOBJECTSLib::FTD_CONNECTED:
					m_cstrConnectionState = "Connected";
					break;
				case TDMFOBJECTSLib::FTD_ACCUMULATE:
					m_cstrConnectionState = "Accumulate";
					break;
				}
			}

			if (m_bNewSymmetric)
			{
				if (bIsWindows)
				{
					CString cstrTmp;
					cstrTmp.Format("%S\\PStore%03d.dat", (BSTR)m_pRG->TargetServer->PStoreDirectory, m_nSymmetricGroupNumber);
					m_cstrSymmetricPStore  =  cstrTmp;
				}
				m_cstrSymmetricJournal = (BSTR)m_pRG->Parent->JournalDirectory;
			}
			else
			{
				m_cstrSymmetricPStore  =  (BSTR)m_pRG->SymmetricPStore;
				m_cstrSymmetricJournal = (BSTR)m_pRG->SymmetricJournal;
			}

			if (bIsWindows == false)
			{
				m_cstrPStoreTitle = "Pstore Location:";

				CReplicationGroupPropertySheet* rgps = (CReplicationGroupPropertySheet*)GetParent();
				m_EditSymmetricPStore.Initialize(rgps->m_pDoc, m_pRG->GetTargetServer());
			}
			else
			{
				m_cstrPStoreTitle = "Pstore File:";
			}

			// Disable fields if symm group is running
			if (m_pRG->GetSymmetricMode() == TDMFOBJECTSLib::FTD_M_UNDEF)
			{
				m_CheckboxSymmGroupStarted.EnableWindow();
				m_EditSymmGroupNumber.EnableWindow();
				m_EditSymmetricPStore.EnableWindow();
				m_EditSymmetricJournal.EnableWindow();
			}
			else
			{
				m_CheckboxSymmGroupStarted.EnableWindow(FALSE);
				m_EditSymmGroupNumber.EnableWindow(FALSE);
				m_EditSymmetricPStore.EnableWindow(FALSE);
				m_EditSymmetricJournal.EnableWindow(FALSE);
			}

			// Disable for Unix
			if (!bIsWindows)
			{
				m_CheckboxSymmGroupStarted.EnableWindow(FALSE);
			}
			else
			{
				// Disable when connection state is not accumulate (start will fail anyway)
				if (m_pRG->ConnectionStatus != TDMFOBJECTSLib::FTD_ACCUMULATE)
				{
					m_CheckboxSymmGroupStarted.EnableWindow(FALSE);
				}
			}

			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1300);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRGSymmetricPage::OnKillActive() 
{
	UpdateData();

	bool bIsWindows = ( strstr(m_pRG->Parent->OSType,"Windows") != 0 ||
						strstr(m_pRG->Parent->OSType,"windows") != 0 ||
						strstr(m_pRG->Parent->OSType,"WINDOWS") != 0 );

	// Validate Group number
	if ((m_nSymmetricGroupNumber < 0) || (m_nSymmetricGroupNumber > 999))
	{
		MessageBox("Invalid Group Number.  Please enter a valid number.", "Error", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// Group Number must also be unique on both source and target
	long nCount = m_pRG->Parent->GetReplicationGroupCount();
	for (int i = 0; i < nCount; i++)
	{
		if ((m_pRG->Parent->GetReplicationGroup(i)->GroupNumber == (long)m_nSymmetricGroupNumber) ||
			(m_pRG->Parent->GetReplicationGroup(i)->Symmetric && (m_pRG->Parent->GetReplicationGroup(i)->SymmetricGroupNumber == (long)m_nSymmetricGroupNumber) && (!m_pRG->Parent->GetReplicationGroup(i)->IsEqual(m_pRG)) && (!m_pRG->Parent->GetReplicationGroup(i)->IsEqual(m_pRG->GetTargetGroup()))))
		{
			MessageBox("Group Number is already used on source server.  Please enter a unique number.", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
	}
	nCount = m_pRG->TargetServer->GetReplicationGroupCount();
	for (i = 0; i < nCount; i++)
	{
		if ((m_pRG->TargetServer->GetReplicationGroup(i)->GroupNumber == (long)m_nSymmetricGroupNumber) ||
			(m_pRG->TargetServer->GetReplicationGroup(i)->Symmetric && (m_pRG->TargetServer->GetReplicationGroup(i)->SymmetricGroupNumber == (long)m_nSymmetricGroupNumber) && (!m_pRG->TargetServer->GetReplicationGroup(i)->IsEqual(m_pRG)) && (!m_pRG->TargetServer->GetReplicationGroup(i)->IsEqual(m_pRG->GetTargetGroup()))))
		{
			MessageBox("Group Number is already used on target server.  Please enter a unique number.", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
	}

	if (bIsWindows)
	{
		// Check PStore file
		if ((!m_cstrSymmetricPStore.IsEmpty() && ( PathIsRelative(LPCTSTR(m_cstrSymmetricPStore)) || m_cstrSymmetricPStore.Find('*')!=-1 || 
			 m_cstrSymmetricPStore.Find('?')!=-1 || m_cstrSymmetricPStore.Find('\"')!=-1 || m_cstrSymmetricPStore.Find('<')!=-1 || 
			 m_cstrSymmetricPStore.Find('>')!=-1 || m_cstrSymmetricPStore.Find('|')!=-1 )) ||
			 (m_cstrSymmetricPStore.IsEmpty()))
		{
			MessageBox("Please enter a valid Persistent Store File.", "Error", MB_OK | MB_ICONSTOP);
			m_EditSymmetricPStore.SetFocus();
			m_EditSymmetricPStore.SetSel(0, -1);
			return FALSE;
		}
	
		// Check Journal directory
		if ((!m_cstrSymmetricJournal.IsEmpty() && ( PathIsRelative(LPCTSTR(m_cstrSymmetricJournal)) || m_cstrSymmetricJournal.Find('*')!=-1 || 
			 m_cstrSymmetricJournal.Find('?')!=-1 || m_cstrSymmetricJournal.Find('\"')!=-1 || m_cstrSymmetricJournal.Find('<')!=-1 || 
			 m_cstrSymmetricJournal.Find('>')!=-1 || m_cstrSymmetricJournal.Find('|')!=-1 )) ||
			 (m_cstrSymmetricJournal.IsEmpty()))
		{
			MessageBox("Please enter a valid Journal directory.", "Error", MB_OK | MB_ICONSTOP);
			m_EditSymmetricJournal.SetFocus();
			m_EditSymmetricJournal.SetSel(0, -1);
			return FALSE;
		}
		if ((m_cstrSymmetricJournal.GetLength() > 0) && (m_cstrSymmetricJournal[m_cstrSymmetricJournal.GetLength()-1] == '\\'))
		{
			m_cstrSymmetricJournal.Delete(m_cstrSymmetricJournal.GetLength()-1);
			UpdateData(FALSE);
		}
	}

	return CPropertyPage::OnKillActive();
}

BOOL CRGSymmetricPage::OnApply() 
{
	try
	{
		if (m_pRG != NULL && m_bPageModified)
		{
			UpdateData();
			
			m_pRG->SymmetricNormallyStarted = m_bSymmetricNormallyStarted;
			m_pRG->SymmetricGroupNumber = m_nSymmetricGroupNumber;
			switch (m_nFailoverInitialState)
			{
			case 0:
				m_pRG->FailoverInitialState = TDMFOBJECTSLib::FTD_MODE_PASSTHRU;
				break;

			case 1:
				m_pRG->FailoverInitialState = TDMFOBJECTSLib::FTD_MODE_TRACKING;
				break;
			}

			m_pRG->SymmetricJournal = (LPCTSTR)m_cstrSymmetricJournal;
			m_pRG->SymmetricPStore  = (LPCTSTR)m_cstrSymmetricPStore;

			m_bPageModified = false;
			SetModified(FALSE);
			SendMessage (DM_SETDEFID, IDOK);
		}
	}
	CATCH_ALL_LOG_ERROR(1016);

	return CPropertyPage::OnApply();
}

void CRGSymmetricPage::OnCheckSymmetricGroupNormallyStarted() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGSymmetricPage::OnRadioPassthru() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGSymmetricPage::OnRadioTracking() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGSymmetricPage::OnUpdateEditSymmetricGroupNumber() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGSymmetricPage::OnUpdateEditJournal() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGSymmetricPage::OnUpdateEditPstore() 
{
	SetModified();
	m_bPageModified = true;
}
