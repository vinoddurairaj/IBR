// TunableParams.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "TunableParams.h"

#include "Config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;
/////////////////////////////////////////////////////////////////////////////
// CTunableParams property page

IMPLEMENT_DYNCREATE(CTunableParams, CPropertyPage)

CTunableParams::CTunableParams() : CPropertyPage(CTunableParams::IDD)
{
	//{{AFX_DATA_INIT(CTunableParams)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CTunableParams::~CTunableParams()
{
}

void CTunableParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTunableParams)
	DDX_Control(pDX, IDC_EDIT_REFRESH_INTERVAL, m_editTimeoutInt);
	DDX_Control(pDX, IDC_CHECK_NEVER_TIMEOUT, m_checkNeverTimeout);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTunableParams, CPropertyPage)
	//{{AFX_MSG_MAP(CTunableParams)
	ON_BN_CLICKED(IDC_CHECK_SYNC_MODE, OnCheckSyncMode)
	ON_BN_CLICKED(IDC_CHECK_STAT_GEN, OnCheckStatGen)
	ON_BN_CLICKED(IDC_CHECK_USAGE_THRESHOLD, OnCheckUsageThreshold)
	ON_WM_CANCELMODE()
	ON_BN_CLICKED(IDC_CHECK_NEVER_TIMEOUT, OnCheckNeverTimeout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTunableParams message handlers

void CTunableParams::OnCheckSyncMode() 
{
	CButton		*pCheckSyncMode;
	CEdit		*pEditDepth;
	CEdit		*pEditTimeOut;
	UINT		iCheckState;
	char		strDepth[10];

	pCheckSyncMode = (CButton *)GetDlgItem( IDC_CHECK_SYNC_MODE );
	iCheckState = pCheckSyncMode->GetState();

	pEditDepth = (CEdit *)GetDlgItem( IDC_EDIT_DEPTH );
	pEditTimeOut = (CEdit *)GetDlgItem( IDC_EDIT_TIMEOUT );
	
	if(iCheckState & 0x0003)
	{
		pEditDepth->SetReadOnly( FALSE );
		pEditTimeOut->SetReadOnly( FALSE );
	}
	else
	{
		pEditDepth->SetReadOnly( TRUE );
		pEditTimeOut->SetReadOnly( TRUE );
	}

	// Set depth field
	memset(strDepth, 0, sizeof(strDepth));
	itoa(lpConfig->m_structTunableParamsValues.m_iDepth , strDepth, 10);
	pEditDepth->SetSel(0, -1, FALSE);
	pEditDepth->ReplaceSel(strDepth, FALSE);

	// Set Timeout field
	memset(strDepth, 0, sizeof(strDepth));
	itoa(lpConfig->m_structTunableParamsValues.m_iTimeout , strDepth, 10);
	pEditTimeOut->SetSel(0, -1, FALSE);
	pEditTimeOut->ReplaceSel(strDepth, FALSE);

}

void CTunableParams::OnCheckStatGen() 
{
	CButton		*pCheckStatGen;
	CEdit		*pEditUpdateInt;
	CEdit		*pEditStatSize;
	UINT		iCheckState;

	pCheckStatGen = (CButton *)GetDlgItem( IDC_CHECK_STAT_GEN );
	iCheckState = pCheckStatGen->GetState();

	pEditUpdateInt = (CEdit *)GetDlgItem( IDC_EDIT_UPDATE_INTERVAL );
	pEditStatSize = (CEdit *)GetDlgItem( IDC_EDIT_MAX_STAT_SIZE );
	
	if(iCheckState & 0x0003)
	{
		pEditUpdateInt->SetReadOnly( FALSE );
		pEditStatSize->SetReadOnly( FALSE );
	}
	else
	{
		pEditUpdateInt->SetReadOnly( TRUE );
		pEditStatSize->SetReadOnly( TRUE );
	}
	
}

void CTunableParams::OnCheckUsageThreshold() 
{
	CButton		*pCheckNetThresh;
	CEdit		*pEditNetThresh;
	UINT		iCheckState;

	pCheckNetThresh = (CButton *)GetDlgItem( IDC_CHECK_USAGE_THRESHOLD );
	iCheckState = pCheckNetThresh->GetState();

	pEditNetThresh = (CEdit *)GetDlgItem( IDC_EDIT_MAX_TRAN_RATE );
	
	if(iCheckState & 0x0003)
	{
		pEditNetThresh->SetReadOnly( FALSE );
	}
	else
	{
		pEditNetThresh->SetReadOnly( TRUE );
	}
	
}

BOOL CTunableParams::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	if(lpConfig->m_structTunableParamsValues.m_bSyncMode)
	{
		CEdit	*pEditDepth;
		CEdit	*pEditTimeout;
		CButton	*pCheckSync;
		char	strDepth[10];

		// set check button state
		pCheckSync = (CButton *)GetDlgItem( IDC_CHECK_SYNC_MODE );
		pCheckSync->SetCheck(1);
		OnCheckSyncMode();

		// Set depth field
		memset(strDepth, 0, sizeof(strDepth));
		itoa(lpConfig->m_structTunableParamsValues.m_iDepth , strDepth, 10);
		pEditDepth = (CEdit *)GetDlgItem(IDC_EDIT_DEPTH);
		pEditDepth->SetSel(0, -1, FALSE);
		pEditDepth->ReplaceSel(strDepth, FALSE);

		// Set Timeout field
		memset(strDepth, 0, sizeof(strDepth));
		itoa(lpConfig->m_structTunableParamsValues.m_iTimeout , strDepth, 10);
		pEditTimeout = (CEdit *)GetDlgItem(IDC_EDIT_TIMEOUT);
		pEditTimeout->SetSel(0, -1, FALSE);
		pEditTimeout->ReplaceSel(strDepth, FALSE);
	}

	if(lpConfig->m_structTunableParamsValues.m_bCompression)
	{
		CButton	*pCompression;

		pCompression = (CButton *)GetDlgItem( IDC_CHECK_COMPRESSION );
		pCompression->SetCheck(1);
	}

	if(lpConfig->m_structTunableParamsValues.m_bNetThresh)
	{
		CEdit	*pEditTranRate;
		CButton	*pCheckNetThresh;
		char	strOutput[10];

		// set check button state
		pCheckNetThresh = (CButton *)GetDlgItem( IDC_CHECK_USAGE_THRESHOLD );
		pCheckNetThresh->SetCheck(1);
		OnCheckUsageThreshold();

		// Set depth field
		memset(strOutput, 0, sizeof(strOutput));
		itoa(lpConfig->m_structTunableParamsValues.m_iMaxTranRate , strOutput, 10);
		pEditTranRate = (CEdit *)GetDlgItem(IDC_EDIT_MAX_TRAN_RATE);
		pEditTranRate->SetSel(0, -1, FALSE);
		pEditTranRate->ReplaceSel(strOutput, FALSE);
	}

	if(lpConfig->m_structTunableParamsValues.m_bStatGen)
	{
		CEdit	*pEditUpdateInt;
		CEdit	*pEditMaxStatSize;
		CButton	*pCheckStat;
		char	strOutput[10];

		// set check button state
		pCheckStat = (CButton *)GetDlgItem( IDC_CHECK_STAT_GEN );
		pCheckStat->SetCheck(1);
		OnCheckStatGen();

		// Set depth field
		memset(strOutput, 0, sizeof(strOutput));
		itoa(lpConfig->m_structTunableParamsValues.m_iDepth , strOutput, 10);
		pEditUpdateInt = (CEdit *)GetDlgItem(IDC_EDIT_UPDATE_INTERVAL);
		pEditUpdateInt->SetSel(0, -1, FALSE);
		pEditUpdateInt->ReplaceSel(strOutput, FALSE);

		// Set Timeout field
		memset(strOutput, 0, sizeof(strOutput));
		itoa(lpConfig->m_structTunableParamsValues.m_iMaxStatFileSize , strOutput, 10);
		pEditMaxStatSize = (CEdit *)GetDlgItem(IDC_EDIT_MAX_STAT_SIZE);
		pEditMaxStatSize->SetSel(0, -1, FALSE);
		pEditMaxStatSize->ReplaceSel(strOutput, FALSE);
	}

	CEdit	*pEditDelayWrites;
	char	strOutput[12];

	memset(strOutput, 0, sizeof(strOutput));
	itoa(lpConfig->m_structTunableParamsValues.m_iDelayWrites , strOutput, 10);

	pEditDelayWrites = (CEdit *)GetDlgItem(IDC_EDIT_WRITE_DELAY);
	pEditDelayWrites->SetSel(0, -1, FALSE);
	pEditDelayWrites->ReplaceSel(strOutput, FALSE);


	// Refresh Interval
	memset(strOutput, 0, sizeof(strOutput));
	if(lpConfig->m_structTunableParamsValues.m_liRefreshInterval != -1)
	{
		m_editTimeoutInt.SetReadOnly(FALSE);

		// DTurrin - Sept 4th, 2001
		// Added a check to make sure that the Refresh Interval does not exceed
		// a value of 8639999 (ie. 99/23/59/59) before formatting it.
		if(lpConfig->m_structTunableParamsValues.m_liRefreshInterval <= 8639999)
		{
			formatRefreshInterval(lpConfig->m_structTunableParamsValues.m_liRefreshInterval, strOutput);
		}
		else
		{
			// If the Refresh Interval is greater than 8639999, set it to the
			// maximum value...
			formatRefreshInterval(8639999, strOutput);
		}

		m_editTimeoutInt.SetLimitText( 11 );
		m_editTimeoutInt.SetSel(0, -1, FALSE);
		m_editTimeoutInt.ReplaceSel(strOutput, FALSE);
	}
	else
	{
		m_checkNeverTimeout.SetCheck(1);
		m_editTimeoutInt.SetReadOnly(TRUE);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTunableParams::formatRefreshInterval(long int liRefreshInt, char *strFormattedData)
{
	int iDays = 0;
	int iHours = 0;
	int iMinutes = 0;
	int iSeconds = 0;
	int	iRemainder = 0;

	iDays = liRefreshInt / 86400;
	iRemainder = liRefreshInt % 86400;

	iHours = iRemainder / 3600;
	iRemainder = iRemainder % 3600;

	iMinutes = iRemainder / 60;
	iRemainder = iRemainder % 60;

	iSeconds = iRemainder;

	sprintf(strFormattedData, "%02d/%02d/%02d/%02d", iDays, iHours, iMinutes, iSeconds);
}


void CTunableParams::OnCancelMode() 
{
	CPropertyPage::OnCancelMode();
	
}

void CTunableParams::OnCheckNeverTimeout() 
{
	if(m_checkNeverTimeout.GetCheck( ))
	{
		lpConfig->m_structTunableParamsValues.m_liRefreshInterval = -1;
		m_editTimeoutInt.SetReadOnly(TRUE);
	}
	else
	{
		char	szOutput[11];

		memset(szOutput, 0, sizeof(szOutput));
		lpConfig->m_structTunableParamsValues.m_liRefreshInterval = 1;
		m_editTimeoutInt.SetReadOnly(FALSE);
		m_editTimeoutInt.SetLimitText( 11 );
		m_editTimeoutInt.GetLine(0, szOutput, sizeof(szOutput));
		m_editTimeoutInt.SetSel(0, -1, FALSE);
		if(strlen(szOutput) == 11)
			m_editTimeoutInt.ReplaceSel(szOutput, FALSE);
		else
			m_editTimeoutInt.ReplaceSel("00/00/00/01", FALSE);
	}
}
