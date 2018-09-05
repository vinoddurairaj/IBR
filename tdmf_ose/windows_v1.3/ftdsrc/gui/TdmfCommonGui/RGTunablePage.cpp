// RGTunablePage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGTunablePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGTunablePage property page

IMPLEMENT_DYNCREATE(CRGTunablePage, CPropertyPage)

CRGTunablePage::CRGTunablePage(TDMFOBJECTSLib::IReplicationGroup *pRG, BOOL bWindows)
	: CPropertyPage(bWindows ? IDD_RG_TUNABLE : IDD_RG_TUNABLE_UNIX), m_pRG(pRG), m_bWindows(bWindows), m_bJournalLessBefore(FALSE)
{
	//{{AFX_DATA_INIT(CRGTunablePage)
	m_bCompression = FALSE;
	m_bNeverTimeout = FALSE;
	m_bSync = FALSE;
	m_nDepth = 0;
	m_nTimeout = 0;
	m_strTimeoutInterval = _T("");
	m_nChunkDelay = 0;
	m_nChunkSize = 0;
	m_nStatUpdateInterval = 0;
	m_nStatMaxSize = 0;
	m_bNetThreshold = FALSE;
	m_nNetTheshold = 0;
	m_bJournalLess = FALSE;
	//}}AFX_DATA_INIT

	m_bPageModified = false;
}

CRGTunablePage::~CRGTunablePage()
{
}

void CRGTunablePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGTunablePage)
	DDX_Control(pDX, IDC_CHECK_SYNC, m_ButtonSync);
	DDX_Control(pDX, IDC_CHECK_COMPRESSION, m_ButtonCompression);
	DDX_Control(pDX, IDC_EDIT_TIMEOUT, m_EditTimeout);
	DDX_Control(pDX, IDC_EDIT_DEPTH, m_EditDepth);
	DDX_Check(pDX, IDC_CHECK_COMPRESSION, m_bCompression);
	if (m_bWindows)
	{
		DDX_Check(pDX, IDC_CHECK_NEVERTIMEOUT, m_bNeverTimeout);
		DDX_Control(pDX, IDC_CHECK_NEVERTIMEOUT, m_ButtonNeverTimeout);
		DDX_Control(pDX, IDC_EDIT_TIMEOUT_INTERVAL, m_EditTimeoutInterval);
		DDX_Text(pDX, IDC_EDIT_TIMEOUT_INTERVAL, m_strTimeoutInterval);
		DDX_Check(pDX, IDC_CHECK_JOURNAL_LESS, m_bJournalLess);
	}
	else
	{
		DDX_Text(pDX, IDC_EDIT_STAT_UPDATE_INTERVAL, m_nStatUpdateInterval);
		DDX_Text(pDX, IDC_EDIT_STAT_MAX_SIZE, m_nStatMaxSize);
		DDX_Check(pDX, IDC_CHECK_NET_THRESHOLD, m_bNetThreshold);
		DDX_Control(pDX, IDC_CHECK_NET_THRESHOLD, m_ButtonNetThreshold);
		DDX_Control(pDX, IDC_EDIT_NET_THRESHOLD, m_EditNetThreshold);
		DDX_Text(pDX, IDC_EDIT_NET_THRESHOLD, m_nNetTheshold);
	}
	DDX_Check(pDX, IDC_CHECK_SYNC, m_bSync);
	DDX_Text(pDX, IDC_EDIT_DEPTH, m_nDepth);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeout);
	DDX_Text(pDX, IDC_EDIT_CHUNKDELAY, m_nChunkDelay);
	DDX_Text(pDX, IDC_EDIT_CHUNKSIZE, m_nChunkSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGTunablePage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGTunablePage)
	ON_BN_CLICKED(IDC_CHECK_COMPRESSION, OnCheckCompression)
	ON_BN_CLICKED(IDC_CHECK_NEVERTIMEOUT, OnCheckNevertimeout)
	ON_BN_CLICKED(IDC_CHECK_SYNC, OnCheckSync)
	ON_BN_CLICKED(IDC_CHECK_NET_THRESHOLD, OnCheckNetThreshold)
	ON_BN_CLICKED(IDC_CHECK_JOURNAL_LESS, OnCheckJournalLess)
	ON_EN_UPDATE(IDC_EDIT_DEPTH, OnUpdateEditDepth)
	ON_EN_UPDATE(IDC_EDIT_TIMEOUT, OnUpdateEditTimeout)
	ON_EN_UPDATE(IDC_EDIT_CHUNKDELAY, OnUpdateEditChunkdelay)
	ON_EN_UPDATE(IDC_EDIT_CHUNKSIZE, OnUpdateEditChunksize)
	ON_EN_UPDATE(IDC_EDIT_TIMEOUT_INTERVAL, OnUpdateEditTimeoutInterval)
	ON_EN_UPDATE(IDC_EDIT_STAT_UPDATE_INTERVAL, OnUpdateEditStatUpdateInterval)
	ON_EN_UPDATE(IDC_EDIT_STAT_MAX_SIZE, OnUpdateEditStatMaxSize)
	ON_EN_UPDATE(IDC_EDIT_NET_THRESHOLD, OnUpdateEditNetThreshold)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGTunablePage message handlers

BOOL CRGTunablePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	try
	{
		m_bPageModified = false;

		if (m_pRG != NULL)
		{
			m_nChunkDelay   = m_pRG->ChunkDelay;
			m_nChunkSize    = m_pRG->ChunkSize;
			
			m_bCompression  = m_pRG->EnableCompression;

			m_bSync =    m_pRG->Sync;
			if(m_bSync)
			{
				m_EditDepth.EnableWindow();
				m_EditTimeout.EnableWindow();
			}
			else
			{
				m_EditDepth.EnableWindow(FALSE);
				m_EditTimeout.EnableWindow(FALSE);
			}
			m_nDepth =   m_pRG->SyncDepth ;
			m_nTimeout = m_pRG->SyncTimeout;

			if (m_bWindows)
			{
				m_bNeverTimeout = m_pRG->RefreshNeverTimeout;
				if(m_bNeverTimeout)
				{
					m_EditTimeoutInterval.EnableWindow(FALSE);
				}
				else
				{
					m_EditTimeoutInterval.EnableWindow(TRUE);
				}
				m_EditTimeoutInterval.SetLimitText(11);

				// Fill Timeout Interval edit box
				int iDays = 0;
				int iHours = 0;
				int iMinutes = 0;
				int iSeconds = 0;
				int	iRemainder = 0;
				
				iDays = m_pRG->RefreshTimeoutInterval / 86400;
				iRemainder = m_pRG->RefreshTimeoutInterval % 86400;
				
				iHours = iRemainder / 3600;
				iRemainder = iRemainder % 3600;
				
				iMinutes = iRemainder / 60;
				iRemainder = iRemainder % 60;
				
				iSeconds = iRemainder;
				
				m_strTimeoutInterval.Format("%02d/%02d/%02d/%02d", iDays, iHours, iMinutes, iSeconds);

				m_bJournalLess = m_pRG->JournalLess;
				// Save original JournalLess flag
				m_bJournalLessBefore = m_bJournalLess;
			}
			else
			{
				m_nStatUpdateInterval = m_pRG->StatInterval;
				m_nStatMaxSize = m_pRG->MaxFileStatSize;
				m_bNetThreshold = m_pRG->NetThreshold;
				m_nNetTheshold = m_pRG->NetMaxKbps;
				if (m_bNetThreshold)
				{
					m_EditNetThreshold.EnableWindow();
				}
				else
				{
					m_EditNetThreshold.EnableWindow(FALSE);
				}
			}

			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1015);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRGTunablePage::OnKillActive() 
{
	// If button is uncheck, enter a default valid value
	if (m_ButtonSync.GetCheck() == 0)
	{
		CString cstrText;
		m_EditDepth.GetWindowText(cstrText);
		if (cstrText.GetLength() == 0)
		{
			m_EditDepth.SetWindowText("0");
		}
		m_EditTimeout.GetWindowText(cstrText);
		if (cstrText.GetLength() == 0)
		{
			m_EditTimeout.SetWindowText("0");
		}
	}
	if ((!m_bWindows) && m_ButtonNetThreshold.GetCheck() == 0)
	{
		CString cstrText;
		m_EditNetThreshold.GetWindowText(cstrText);
		if (cstrText.GetLength() == 0)
		{
			m_EditNetThreshold.SetWindowText("1");
		}
	}

	UpdateData();

	// Validate
	if (m_bWindows)
	{
		// ChunkDelay
		if ((m_nChunkDelay < 0) || (m_nChunkDelay > 214783647))
		{
			MessageBox("Invalid Chunk Delay.  Please enter a valid number (0 to 214783647).", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
		// ChunkSize
		if ((m_nChunkSize < 32) || (m_nChunkSize > 4000))
		{
			MessageBox("Invalid Chunk Size.  Please enter a valid number (32 to 4000).", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
		// Sync Mode
		if (m_ButtonSync.GetCheck() == 1)  // validate only if checked
		{
			// Sync Mode Depth
			if ((m_nDepth < 1) || (m_nDepth > 214783647))
			{
				MessageBox("Invalid Sync Mode Depth.  Please enter a valid number (1 to 214783647).", "Error", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
			// Sync Mode Timeout
			if ((m_nTimeout < 1) || (m_nTimeout > 214783647))
			{
				MessageBox("Sync Mode Timeout.  Please enter a valid number (1 to 214783647).", "Error", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
		}
		// RefreshTimeout
		if (m_ButtonNeverTimeout.GetCheck() == 0)  // validate only if not checked
		{
			if (m_strTimeoutInterval.GetLength() == 11)
			{
				int iDays = 0;
				int iHours = 0;
				int iMinutes = 0;
				int iSeconds = 0;
			
				if(m_strTimeoutInterval[2] != '/' || m_strTimeoutInterval[5] != '/' || m_strTimeoutInterval[8] != '/')
				{
					MessageBox("Invalid Timeout Interval format (dd/hh/mm/ss)", "Error", MB_OK | MB_ICONSTOP);
					return FALSE;
				}
				
				if(m_strTimeoutInterval[0] != '0' || m_strTimeoutInterval[1] != '0')
				{
					iDays = atoi((LPCTSTR)m_strTimeoutInterval.Mid(0, 2));
					if(iDays == 0)
					{
						MessageBox("Invalid day number.  Please enter a valid day number (00 to 99).", "Error", MB_OK | MB_ICONSTOP);
						return FALSE;
					}
				}
				
				if(m_strTimeoutInterval[3] != '0' || m_strTimeoutInterval[4] != '0')
				{
					iHours = atoi((LPCTSTR)m_strTimeoutInterval.Mid(3, 2));
					if(iHours == 0 || iHours > 23)
					{
						MessageBox("Invalid hour number.  Please enter a valid hour number (00 to 23).", "Error", MB_OK | MB_ICONSTOP);
						return FALSE;
					}
				}
				
				if(m_strTimeoutInterval[6] != '0' || m_strTimeoutInterval[7] != '0')
				{
					iMinutes = atoi((LPCTSTR)m_strTimeoutInterval.Mid(6, 2));
					if(iMinutes == 0 || iMinutes > 59)
					{
						MessageBox("Invalid minute number.  Please enter a valid minute number (00 to 59).", "Error", MB_OK | MB_ICONSTOP);
						return FALSE;
					}
				}
				
				if(m_strTimeoutInterval[9] != '0' || m_strTimeoutInterval[10] != '0')
				{
					iSeconds = atoi((LPCTSTR)m_strTimeoutInterval.Mid(9, 2));
					if(iSeconds == 0 || iSeconds > 59)
					{
						MessageBox("Invalid second number.  Please enter a valid second number (00 to 59).", "Error", MB_OK | MB_ICONSTOP);
						return FALSE;
					}
				}
				
				m_pRG->RefreshTimeoutInterval = (iDays * 86400) + (iHours * 3600) + (iMinutes * 60) + iSeconds;
			}
			else
			{
				MessageBox("Invalid Timeout Interval format (dd/hh/mm/ss)", "Error", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
		}
	}
	else
	{
		// ChunkDelay
		if ((m_nChunkDelay < 0) || (m_nChunkDelay > 999))
		{
			MessageBox("Invalid Chunk Delay.  Please enter a valid number (0 to 999).", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
		// ChunkSize
		if ((m_nChunkSize < 64) || (m_nChunkSize > 16384))
		{
			MessageBox("Invalid Chunk Size.  Please enter a valid number (64 to 16384).", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
		// Sync Mode
		if (m_ButtonSync.GetCheck() == 1)  // validate only if checked
		{
			// Sync Mode Depth
			if ((m_nDepth < 1) || (m_nDepth > 214783647))
			{
				MessageBox("Invalid Sync Mode Depth.  Please enter a valid number (1 to 214783647).", "Error", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
			// Sync Mode Timeout
			if ((m_nTimeout < 1) || (m_nTimeout > 86400))
			{
				MessageBox("Sync Mode Timeout.  Please enter a valid number (1 to 86400).", "Error", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
		}
		// Net Max KBps
		if (m_ButtonNetThreshold.GetCheck() == 1)  // validate only if checked
		{
			if ((m_nNetTheshold < 1) || (m_nNetTheshold > 214783647))
			{
				MessageBox("Invalid Net Max KBps.  Please enter a valid number (1 to 214783647).", "Error", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
		}

		// Stat Interval
		if ((m_nStatUpdateInterval < 1) || (m_nStatUpdateInterval > 86400))
		{
			MessageBox("Invalid Stat Interval.  Please enter a valid number (1 to 86400).", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
		// Max Stat File Size
		if ((m_nStatMaxSize < 1) || (m_nStatMaxSize > 32000))
		{
			MessageBox("Invalid Max Stat File Size.  Please enter a valid number (1 to 32000).", "Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
	}

	return CPropertyPage::OnKillActive();
}

BOOL CRGTunablePage::OnApply() 
{
	try
	{
		if (m_pRG != NULL && m_bPageModified)
		{
			UpdateData();

			m_pRG->ChunkDelay				= m_nChunkDelay;
			m_pRG->ChunkSize				= m_nChunkSize;
			m_pRG->EnableCompression		= m_bCompression;
			m_pRG->RefreshNeverTimeout		= m_bNeverTimeout;
			m_pRG->Sync						= m_bSync;
			m_pRG->SyncDepth				= m_nDepth;
			m_pRG->SyncTimeout				= m_nTimeout;

			m_pRG->NetThreshold				= m_bNetThreshold;
			m_pRG->NetMaxKbps				= m_nNetTheshold;
			m_pRG->StatInterval				= m_nStatUpdateInterval;
			m_pRG->MaxFileStatSize			= m_nStatMaxSize;

			m_pRG->JournalLess              = m_bJournalLess;
			m_pRG->ForcePMDRestart          = false;

			if ((m_pRG->Mode != TDMFOBJECTSLib::FTD_M_UNDEF) && (m_bJournalLessBefore != m_bJournalLess))
			{
				CString cstrMsg;

				if (m_bJournalLess)
				{
					cstrMsg = "To activate journal-less option, the group's PMD/RMD need to be restarted.\nDo you want to restart them now?";
				}
				else
				{
					cstrMsg = "To deactivate journal-less option, the group's PMD/RMD need to be restarted.\nDo you want to restart them now?";
				}

				if (MessageBox(cstrMsg, "Journal-less Warning", MB_YESNO) == IDYES)
				{
					m_pRG->ForcePMDRestart = true;
				}
			}

			m_bPageModified = false;
			SetModified(FALSE);
			SendMessage (DM_SETDEFID, IDOK);
		}
	}
	CATCH_ALL_LOG_ERROR(1016);

	return CPropertyPage::OnApply();
}

void CRGTunablePage::OnCheckCompression() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnCheckNevertimeout() 
{
	if(m_ButtonNeverTimeout.GetCheck() == IS_CHECKED)
	{
		m_EditTimeoutInterval.EnableWindow(FALSE);
	}
	else
	{
		m_EditTimeoutInterval.EnableWindow(TRUE);
	}

	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnCheckSync() 
{
	if(m_ButtonSync.GetCheck() == IS_NOT_CHECKED)
	{
		m_EditDepth.EnableWindow(FALSE);
		m_EditTimeout.EnableWindow(FALSE);
	}
	else
	{
		m_EditDepth.EnableWindow(TRUE);
		m_EditTimeout.EnableWindow(TRUE);
	}

	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnCheckNetThreshold() 
{
	if(m_ButtonNetThreshold.GetCheck() == IS_NOT_CHECKED)
	{
		m_EditNetThreshold.EnableWindow(FALSE);
	}
	else
	{
		m_EditNetThreshold.EnableWindow();
	}

	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnCheckJournalLess()
{
	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnUpdateEditDepth() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnUpdateEditTimeout() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnUpdateEditTimeoutInterval() 
{
	SetModified();
	m_bPageModified = true;
}

void CRGTunablePage::OnUpdateEditChunkdelay() 
{
	SetModified();
	m_bPageModified = true;	
}

void CRGTunablePage::OnUpdateEditChunksize() 
{
	SetModified();
	m_bPageModified = true;	
}

void CRGTunablePage::OnUpdateEditStatUpdateInterval()
{
	SetModified();
	m_bPageModified = true;	
}

void CRGTunablePage::OnUpdateEditStatMaxSize()
{
	SetModified();
	m_bPageModified = true;	
}

void CRGTunablePage::OnUpdateEditNetThreshold()
{
	SetModified();
	m_bPageModified = true;	
}
