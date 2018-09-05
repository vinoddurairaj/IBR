// TunablesPage.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "TunablesPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTunablesPage property page

IMPLEMENT_DYNCREATE(CTunablesPage, CPropertyPage)

CTunablesPage::CTunablesPage(CGroupConfig* pGroupConfig) : CPropertyPage(CTunablesPage::IDD), m_pGroupConfig(pGroupConfig)
{
	//{{AFX_DATA_INIT(CTunablesPage)
	m_bCompression = FALSE;
	m_bNeverTimeout = FALSE;
	m_bSyncMode = FALSE;
	m_cstrSyncDepth = _T("");
	m_cstrSyncTimeout = _T("");
	m_cstrRefreshInterval = _T("");
	m_bJournalLess = FALSE;
	//}}AFX_DATA_INIT

	m_bJournalLessBefore = FALSE;
}

void CTunablesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTunablesPage)
	DDX_Control(pDX, IDC_EDIT_TIMEOUT, m_EditSyncTimeout);
	DDX_Control(pDX, IDC_EDIT_DEPTH, m_EditSyncModeDepth);
	DDX_Control(pDX, IDC_EDIT_REFRESH_INTERVAL, m_EditTimeout);
	DDX_Check(pDX, IDC_CHECK_COMPRESSION, m_bCompression);
	DDX_Check(pDX, IDC_CHECK_NEVER_TIMEOUT, m_bNeverTimeout);
	DDX_Check(pDX, IDC_CHECK_SYNC_MODE, m_bSyncMode);
	DDX_Text(pDX, IDC_EDIT_DEPTH, m_cstrSyncDepth);
	DDV_MaxChars(pDX, m_cstrSyncDepth, 10);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_cstrSyncTimeout);
	DDV_MaxChars(pDX, m_cstrSyncTimeout, 10);
	DDX_Text(pDX, IDC_EDIT_REFRESH_INTERVAL, m_cstrRefreshInterval);
	DDV_MaxChars(pDX, m_cstrRefreshInterval, 11);
	DDX_Check(pDX, IDC_CHECK_JOURNAL_LESS, m_bJournalLess);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTunablesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTunablesPage)
	ON_BN_CLICKED(IDC_CHECK_NEVER_TIMEOUT, OnCheckNeverTimeout)
	ON_BN_CLICKED(IDC_CHECK_SYNC_MODE, OnCheckSyncMode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTunablesPage message handlers

BOOL CTunablesPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_bCompression        = m_pGroupConfig->GetCompression();
	m_bSyncMode           = m_pGroupConfig->GetSyncMode();
	m_cstrSyncDepth       = m_pGroupConfig->GetSyncDepth();
	m_cstrSyncTimeout     = m_pGroupConfig->GetSyncTimeout();
	m_bNeverTimeout       = m_pGroupConfig->GetRefreshNeverTimeout();
	m_bJournalLess        = m_pGroupConfig->GetJournalLess();
	m_bJournalLessBefore  = m_bJournalLess;

	// Format Refresh Interval
	int nRefreshInt = atoi(m_pGroupConfig->GetRefreshInterval());
	int iDays = 0;
	int iHours = 0;
	int iMinutes = 0;
	int iSeconds = 0;
	int	iRemainder = 0;

	iDays = nRefreshInt / 86400;
	iRemainder = nRefreshInt % 86400;

	iHours = iRemainder / 3600;
	iRemainder = iRemainder % 3600;

	iMinutes = iRemainder / 60;
	iRemainder = iRemainder % 60;

	iSeconds = iRemainder;

	m_cstrRefreshInterval.Format("%02d/%02d/%02d/%02d", iDays, iHours, iMinutes, iSeconds);
	
	UpdateData(FALSE);
	
	// Update fields state
	OnCheckNeverTimeout();
	OnCheckSyncMode();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTunablesPage::Validate()
{
	return TRUE;
}

BOOL CTunablesPage::SaveValues()
{
	if (m_hWnd)
	{
		UpdateData();
		
		m_pGroupConfig->SetCompression(m_bCompression ? true : false);
		m_pGroupConfig->SetSyncMode(m_bSyncMode ? true : false);
		m_pGroupConfig->SetSyncDepth(m_cstrSyncDepth);
		m_pGroupConfig->SetSyncTimeout(m_cstrSyncTimeout);
		m_pGroupConfig->SetRefreshNeverTimeout(m_bNeverTimeout ? true : false);
		if (m_bNeverTimeout)
		{
			m_pGroupConfig->SetRefreshInterval("0");
		}
		else
		{
			CString cstrTmp = m_cstrRefreshInterval;

			// De-format Refresh Interval
			int iSeconds = 0;
			int nStart = cstrTmp.ReverseFind('/') + 1;
			int nEnd   = cstrTmp.GetLength();
			if (nStart == -1)
			{
				iSeconds += atoi(cstrTmp);
			}
			else
			{
				CString cstrToken = cstrTmp.Mid(nStart, nEnd - nStart);
				iSeconds = atoi(cstrToken);
				cstrTmp = cstrTmp.Mid(0, nStart-1);

				nStart = cstrTmp.ReverseFind('/') + 1;
				nEnd   = cstrTmp.GetLength();
				if (nStart == -1)
				{
					iSeconds += atoi(cstrToken) * 60; // minutes
				}
				else
				{
					cstrToken = cstrTmp.Mid(nStart, nEnd - nStart);
					iSeconds += atoi(cstrToken) * 60;
					cstrTmp = cstrTmp.Mid(0, nStart-1);

					nStart = cstrTmp.ReverseFind('/') + 1;
					nEnd   = cstrTmp.GetLength();
					if (nStart == -1)
					{
						iSeconds += atoi(cstrToken) * 3600; // hours
					}
					else
					{
						cstrToken = cstrTmp.Mid(nStart, nEnd - nStart);
						iSeconds += atoi(cstrToken) * 3600;
						cstrTmp = cstrTmp.Mid(0, nStart-1);
						
						iSeconds += atoi(cstrTmp) * 86400; // days
					}
				}
			}

			// Save value in second
			CString cstrValue;
			cstrValue.Format("%d", iSeconds);
			m_pGroupConfig->SetRefreshInterval(cstrValue);
		}

		m_pGroupConfig->SetJournalLess(m_bJournalLess ? true : false);
	}
	
	return TRUE;
}

BOOL CTunablesPage::IsPMDRestartNeeded()
{
	return (m_bJournalLessBefore != m_bJournalLess);
}

void CTunablesPage::OnCheckNeverTimeout() 
{
	UpdateData();

	if (m_bNeverTimeout)
	{
		m_EditTimeout.SetReadOnly();
	}
	else
	{
		m_EditTimeout.SetReadOnly(FALSE);
	}
}

void CTunablesPage::OnCheckSyncMode() 
{
	UpdateData();

	if (m_bSyncMode)
	{
		m_EditSyncModeDepth.SetReadOnly(FALSE);
		m_EditSyncTimeout.SetReadOnly(FALSE);
	}
	else
	{
		m_EditSyncModeDepth.SetReadOnly();
		m_EditSyncTimeout.SetReadOnly();
	}
}
