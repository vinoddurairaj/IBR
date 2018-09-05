// RGDetailsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "RGDetailsPage.h"
#include "ToolsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGDetailsPage property page

IMPLEMENT_DYNCREATE(CRGDetailsPage, CPropertyPage)

CRGDetailsPage::CRGDetailsPage() : CPropertyPage(CRGDetailsPage::IDD)
{
	//{{AFX_DATA_INIT(CRGDetailsPage)
	m_nChunkDelay = 0;
	m_nChunkSize = 0;
	m_strDescription = _T("");
	m_nGroupNumber = 0;
	m_nPort = 0;
	m_nSynchDepth = 0;
	m_strSynchMode = _T("");
	m_strTargetServer = _T("");
	m_nTimeout = 0;
	m_strTitle = _T("Replication Group");
	m_strSettingsTitle = _T("Parameters");
	m_strCompression = _T("");
	m_strChaining = _T("");
	m_strPStoreJournal = _T("");
	m_strPStoreJournalTitle = _T("");
	m_cstrJournal = _T("");
	//}}AFX_DATA_INIT
}

CRGDetailsPage::~CRGDetailsPage()
{
}

void CRGDetailsPage::RefreshValues()
{
	try
	{
		if (m_hWnd != NULL)
		{
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
		
			if (pDoc->GetSelectedReplicationGroup() != NULL)
			{
				std::ostringstream oss;
				
				m_nGroupNumber = pDoc->GetSelectedReplicationGroup()->GroupNumber;
				
				m_strDescription = (BSTR)pDoc->GetSelectedReplicationGroup()->Description;
				
				if ((BSTR)pDoc->GetSelectedReplicationGroup()->IsSource)
				{
					if (strstr(pDoc->GetSelectedReplicationGroup()->GetParent()->OSType, "Windows") != 0 ||
						strstr(pDoc->GetSelectedReplicationGroup()->GetParent()->OSType, "windows") != 0 ||
						strstr(pDoc->GetSelectedReplicationGroup()->GetParent()->OSType, "WINDOWS") != 0)
					{
						m_strPStoreJournalTitle = "PStore File:";
						m_EditJournal.ShowWindow(SW_SHOW);
						m_TitleJournalLess.ShowWindow(SW_SHOW);
					}
					else
					{
						m_strPStoreJournalTitle = "PStore Location:";
						m_EditJournal.ShowWindow(SW_HIDE);
						m_TitleJournalLess.ShowWindow(SW_HIDE);
					}
					m_strPStoreJournal = (BSTR)pDoc->GetSelectedReplicationGroup()->PStoreDirectory;
				}
				else
				{
					m_strPStoreJournalTitle = "Journal Directory:";
					m_strPStoreJournal = (BSTR)pDoc->GetSelectedReplicationGroup()->JournalDirectory;
				}
				
				m_strTargetServer = (BSTR)pDoc->GetSelectedReplicationGroup()->TargetName;
				
				m_nPort = pDoc->GetSelectedReplicationGroup()->Parent->Port;
				
				m_strChaining = pDoc->GetSelectedReplicationGroup()->Chaining ? "Yes" : "No";
				
				m_strSynchMode = pDoc->GetSelectedReplicationGroup()->Sync ? "Synch" : "Asynch";
				
				m_nSynchDepth = pDoc->GetSelectedReplicationGroup()->SyncDepth;
				
				m_nTimeout = pDoc->GetSelectedReplicationGroup()->SyncTimeout;
				
				m_strCompression = pDoc->GetSelectedReplicationGroup()->EnableCompression ? "On" : "Off";
				
				m_nChunkDelay = pDoc->GetSelectedReplicationGroup()->ChunkDelay;
				
				m_nChunkSize = pDoc->GetSelectedReplicationGroup()->ChunkSize;

				m_cstrJournal = pDoc->GetSelectedReplicationGroup()->JournalLess ? "No" : "Yes";

				UpdateData(FALSE);
			}
		}
	}
	CATCH_ALL_LOG_ERROR(1005);
}

void CRGDetailsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGDetailsPage)
	DDX_Control(pDX, IDC_EDIT_JOURNAL_LESS, m_EditJournal);
	DDX_Control(pDX, IDC_TITLE_JOURNAL_LESS, m_TitleJournalLess);
	DDX_Control(pDX, IDC_TITLE_TIMEOUT, m_TitleTimeout);
	DDX_Control(pDX, IDC_TITLE_SYNC_MODE, m_TitleSyncMode);
	DDX_Control(pDX, IDC_TITLE_SYNC_DEPTH, m_TitleSyncDepth);
	DDX_Control(pDX, IDC_TITLE_COMPRESSION, m_TitleCompression);
	DDX_Control(pDX, IDC_TITLE_CHUNK_SIZE, m_TitleChunkSize);
	DDX_Control(pDX, IDC_TITLE_CHUNK_DELAY, m_TitleChunkDelay);
	DDX_Control(pDX, IDC_PStoreJournal_Title, m_TitlePStoreJournal);
	DDX_Control(pDX, IDC_EDIT_TIMEOUT, m_EditTimeout);
	DDX_Control(pDX, IDC_EDIT_TARGET_SERVER, m_EditRemoteServer);
	DDX_Control(pDX, IDC_EDIT_SYNCH_MODE, m_EditSyncMode);
	DDX_Control(pDX, IDC_EDIT_SYNCH_DEPTH, m_EditSyncDepth);
	DDX_Control(pDX, IDC_EDIT_PSTOREJOURNAL, m_EditPStoreJournal);
	DDX_Control(pDX, IDC_EDIT_PORT, m_EditPort);
	DDX_Control(pDX, IDC_EDIT_GROUP_NUMBER, m_EditGroupNumber);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_EditDescription);
	DDX_Control(pDX, IDC_EDIT_COMPRESSION, m_EditCompression);
	DDX_Control(pDX, IDC_EDIT_CHUNK_SIZE, m_EditChunkSize);
	DDX_Control(pDX, IDC_EDIT_CHUNK_DELAY, m_EditChunkDelay);
	DDX_Control(pDX, IDC_EDIT_CHAINING, m_EditChaining);
	DDX_Control(pDX, IDC_RICHEDIT_SETTINGS, m_RichEditSettings);
	DDX_Control(pDX, IDC_RICHEDIT_NAME, m_RichEditName);
	DDX_Text(pDX, IDC_EDIT_CHUNK_DELAY, m_nChunkDelay);
	DDX_Text(pDX, IDC_EDIT_CHUNK_SIZE, m_nChunkSize);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDX_Text(pDX, IDC_EDIT_GROUP_NUMBER, m_nGroupNumber);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Text(pDX, IDC_EDIT_SYNCH_DEPTH, m_nSynchDepth);
	DDX_Text(pDX, IDC_EDIT_SYNCH_MODE, m_strSynchMode);
	DDX_Text(pDX, IDC_EDIT_TARGET_SERVER, m_strTargetServer);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeout);
	DDX_Text(pDX, IDC_RICHEDIT_NAME, m_strTitle);
	DDX_Text(pDX, IDC_RICHEDIT_SETTINGS, m_strSettingsTitle);
	DDX_Text(pDX, IDC_EDIT_COMPRESSION, m_strCompression);
	DDX_Text(pDX, IDC_EDIT_CHAINING, m_strChaining);
	DDX_Text(pDX, IDC_EDIT_PSTOREJOURNAL, m_strPStoreJournal);
	DDX_Text(pDX, IDC_PStoreJournal_Title, m_strPStoreJournalTitle);
	DDX_Text(pDX, IDC_EDIT_JOURNAL_LESS, m_cstrJournal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGDetailsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGDetailsPage)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGDetailsPage message handlers

BOOL CRGDetailsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Set Name format
	m_RichEditName.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));
	m_RichEditSettings.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));

	CHARFORMAT cf;
	cf.dwMask = CFM_BOLD;
	cf.dwEffects = CFE_BOLD;
	m_RichEditSettings.SetDefaultCharFormat(cf);

	cf.dwMask = CFM_BOLD | CFM_SIZE;
	m_RichEditName.GetDefaultCharFormat(cf);
	cf.dwEffects = CFE_BOLD;
	cf.yHeight = (LONG)(cf.yHeight * 1.5);
	m_RichEditName.SetDefaultCharFormat(cf);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRGDetailsPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();

	// Init values
	RefreshValues();

	return CPropertyPage::OnSetActive();
}

void CRGDetailsPage::MoveCtrl(CWnd& WndCtrl, int nX, int nRight)
{
	CRect Rect;

	if (WndCtrl.m_hWnd != NULL)
	{
		WndCtrl.GetWindowRect(&Rect);
		ScreenToClient(&Rect);
		int nWidth = Rect.Width();

		if (nX > 0)
		{
			Rect.left = nX;
		}

		if (nRight < 0)
		{
			Rect.right = Rect.left + nWidth;
		}
		else
		{
			Rect.right = nRight;
		}

		WndCtrl.MoveWindow(&Rect);
	}
}

void CRGDetailsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);

	int nMiddle = cx/2;
	int nOffset = 12;
	int nOffsetEdit = 150;

	// Left Columns (just resize editbox)
	MoveCtrl(m_EditGroupNumber,   -1, nMiddle - nOffset);
	MoveCtrl(m_EditDescription,   -1, nMiddle - nOffset);
	MoveCtrl(m_EditRemoteServer,  -1, nMiddle - nOffset);
	MoveCtrl(m_EditPort,          -1, nMiddle - nOffset);
	MoveCtrl(m_EditChaining,      -1, nMiddle - nOffset);
	MoveCtrl(m_EditPStoreJournal, -1, nMiddle - nOffset);

	// Right columns (Move label and editbox and reize editbox)
	int nRightEdit  = cx - nOffset; 

	MoveCtrl(m_RichEditSettings, nMiddle);
	
	MoveCtrl(m_TitleSyncMode,        nMiddle + nOffset);
	MoveCtrl(m_EditSyncMode,         nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleSyncDepth,       nMiddle + nOffset);
	MoveCtrl(m_EditSyncDepth,        nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleTimeout,         nMiddle + nOffset);
	MoveCtrl(m_EditTimeout,          nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleCompression,     nMiddle + nOffset);
	MoveCtrl(m_EditCompression,      nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleChunkDelay,      nMiddle + nOffset);
	MoveCtrl(m_EditChunkDelay,       nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleChunkSize,       nMiddle + nOffset);
	MoveCtrl(m_EditChunkSize,        nMiddle + nOffsetEdit, nRightEdit);
	MoveCtrl(m_TitleJournalLess,     nMiddle + nOffset);
	MoveCtrl(m_EditJournal,          nMiddle + nOffsetEdit, nRightEdit);
}
