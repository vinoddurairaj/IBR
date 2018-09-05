// DTCConfigPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "DTCConfigPropSheet.h"
#include "AddModMirrors.h"

#include "System.h"
#include "Config.h"
extern "C" {
#include "iputil.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;
CDTCConfigPropSheet *lpPropsheet;
/////////////////////////////////////////////////////////////////////////////
// CDTCConfigPropSheet

IMPLEMENT_DYNAMIC(CDTCConfigPropSheet, CPropertySheet)

CDTCConfigPropSheet::CDTCConfigPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CDTCConfigPropSheet::CDTCConfigPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	
}

CDTCConfigPropSheet::~CDTCConfigPropSheet()
{
}


BEGIN_MESSAGE_MAP(CDTCConfigPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CDTCConfigPropSheet)
	ON_WM_CREATE()
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTCConfigPropSheet message handlers

BOOL CDTCConfigPropSheet::setSystemDlgValues()
{
	CPropertyPage *system;
	int	iCheckState;

    CWaitCursor wait;//display the hourglass cursor because validation can take some time ...

    /* [02-10-10] ac V
	int iIP1,iIP2,iIP3,iIP4;
    */

	// Get All values from the System Dlg
	system = GetPage(0);

    /* [02-10-10] ac V
	iIP1 = system->GetDlgItemInt( IDC_EDIT_PRIMARY_IP1, NULL, FALSE );
	iIP2 = system->GetDlgItemInt( IDC_EDIT_PRIMARY_IP2, NULL, FALSE );
	iIP3 = system->GetDlgItemInt( IDC_EDIT_PRIMARY_IP3, NULL, FALSE );
	iIP4 = system->GetDlgItemInt( IDC_EDIT_PRIMARY_IP4, NULL, FALSE );

	memset(m_structSystemValues.m_strPrimaryHostName, 0, sizeof(m_structSystemValues.m_strPrimaryHostName));
	sprintf(m_structSystemValues.m_strPrimaryHostName, "%03i.%03i.%03i.%03i", iIP1, iIP2, iIP3, iIP4);
    */
    system->GetDlgItemText( IDC_EDIT_PRI_HOST_OR_IP, m_structSystemValues.m_strPrimaryHostName, sizeof(m_structSystemValues.m_strPrimaryHostName) );

    /* [02-10-10] ac V
	iIP1 = system->GetDlgItemInt( IDC_EDIT_SECONDARY_IP1, NULL, FALSE );
	iIP2 = system->GetDlgItemInt( IDC_EDIT_SECONDARY_IP2, NULL, FALSE );
	iIP3 = system->GetDlgItemInt( IDC_EDIT_SECONDARY_IP3, NULL, FALSE );
	iIP4 = system->GetDlgItemInt( IDC_EDIT_SECONDARY_IP4, NULL, FALSE );

	memset(m_structSystemValues.m_strSecondHostName, 0, sizeof(m_structSystemValues.m_strSecondHostName));
	sprintf(m_structSystemValues.m_strSecondHostName, "%03i.%03i.%03i.%03i", iIP1, iIP2, iIP3, iIP4);
    */
    system->GetDlgItemText( IDC_EDIT_SEC_HOST_OR_IP, m_structSystemValues.m_strSecondHostName, sizeof(m_structSystemValues.m_strSecondHostName) );

	system->GetDlgItemText(IDC_EDIT_JOURNAL_DIR, m_structSystemValues.m_strJournalDir,
							sizeof(m_structSystemValues.m_strJournalDir));
	m_structSystemValues.m_iPortNum = 
		system->GetDlgItemInt ( IDC_EDIT_SECONDARY_PORT, NULL, TRUE );
	memset(m_structSystemValues.m_strNote, 0, sizeof(m_structSystemValues.m_strNote));
	system->GetDlgItemText(IDC_EDIT_SYSTEM_NOTE, m_structSystemValues.m_strNote,
							sizeof(m_structSystemValues.m_strNote));

	iCheckState = system->IsDlgButtonChecked(IDC_CHECK_CHAINING);
	if(iCheckState)
		m_structSystemValues.m_bChaining = TRUE;
	else
		m_structSystemValues.m_bChaining = FALSE;

	// [020712] AlRo V
	//system->DlgDirSelectComboBox( m_structSystemValues.m_strPStoreDev, IDC_LIST_PSTORE );
	// Becomes:
	CString lszPStore;
	system->GetDlgItemText ( IDC_EDIT_PSTORE, lszPStore );
	sprintf ( m_structSystemValues.m_strPStoreDev, "%.*s", _MAX_PATH-1, lszPStore );
	// [020712] AlRo ^

    // validation of some values
    sock_startup();
    unsigned long ip;
    char hostname[256];
    if ( name_is_ipstring(m_structSystemValues.m_strPrimaryHostName) )
    {
        if ( ipstring_to_ip(m_structSystemValues.m_strPrimaryHostName,&ip) < 0 )
        {
            CString errmsg = "Primary System \'";
            errmsg += m_structSystemValues.m_strPrimaryHostName;
            errmsg += "\' is not a valid IP address format.";
            AfxMessageBox(errmsg);
            sock_cleanup();
            return FALSE;
        }
        if ( ip_to_name(ip,hostname) < 0 )
        {
            CString errmsg = "Primary System \'";
            errmsg += m_structSystemValues.m_strPrimaryHostName;
            errmsg += "\' does not match any host on the network.";
            AfxMessageBox(errmsg);
            sock_cleanup();
            return FALSE;
        }
    }
    else
    {   //it is an host name.  Check if the network knows it...
        if ( name_to_ip(m_structSystemValues.m_strPrimaryHostName, &ip) < 0 )
        {
            CString errmsg = "Primary System \'";
            errmsg += m_structSystemValues.m_strPrimaryHostName;
            errmsg += "\' is not a recognized host name.";
            AfxMessageBox(errmsg);
            sock_cleanup();
            return FALSE;
        }
    }

    if ( name_is_ipstring(m_structSystemValues.m_strSecondHostName) )
    {
        if ( ipstring_to_ip(m_structSystemValues.m_strSecondHostName,&ip) < 0 )
        {
            CString errmsg = "Secondary System \'";
            errmsg += m_structSystemValues.m_strSecondHostName;
            errmsg += "\' is not a valid IP address format.";
            AfxMessageBox(errmsg);
            sock_cleanup();
            return FALSE;
        }
        if ( ip_to_name(ip,hostname) < 0 )
        {
            CString errmsg = "Secondary System \'";
            errmsg += m_structSystemValues.m_strSecondHostName;
            errmsg += "\' does not match any host on the network.";
            AfxMessageBox(errmsg);
            sock_cleanup();
            return FALSE;
        }
    }
    else
    {   //it is an host name.  Check if the network knows it...
        if ( name_to_ip(m_structSystemValues.m_strSecondHostName, &ip) < 0 )
        {
            CString errmsg = "Secondary System \'";
            errmsg += m_structSystemValues.m_strSecondHostName;
            errmsg += "\' is not a recognized host name.";
            AfxMessageBox(errmsg);
            sock_cleanup();
            return FALSE;
        }
    }

	//automatic conversion to loopback mode  rddev 030127
	if (	!strcmp(m_structSystemValues.m_strPrimaryHostName,"127.0.0.1")                                   
		 || !strcmp(m_structSystemValues.m_strSecondHostName,"127.0.0.1"))
	{
		strcpy(m_structSystemValues.m_strPrimaryHostName , "127.0.0.1\0");
		strcpy(m_structSystemValues.m_strSecondHostName ,  "127.0.0.1\0");
	}

    sock_cleanup();
    //validation of other values
    char szDrive[8],szDir[MAX_PATH],szFile[MAX_PATH];
    _splitpath(m_structSystemValues.m_strPStoreDev,szDrive,szDir,szFile,NULL);
    if ( szFile[0] == 0 )
    {
        CString errmsg = "PStore value \'";
        errmsg += m_structSystemValues.m_strPStoreDev;
        errmsg += "\' is not a complete file name.";
        AfxMessageBox(errmsg);
        return FALSE;
    }
    _splitpath(m_structSystemValues.m_strJournalDir,szDrive,szDir,szFile,NULL);
    if ( szDir[0] == 0 )
    {
        CString errmsg = "Journal value \'";
        errmsg += m_structSystemValues.m_strJournalDir;
        errmsg += "\' is not valid a directory name.";
        AfxMessageBox(errmsg);
        return FALSE;
    }




	// Send data to config obj
	lpConfig->setSysDlgValues();
    return TRUE;

}

void CDTCConfigPropSheet::setDTCDeviceValues()
{
	CPropertyPage *dtcDev;
	// Get All values from the DTCDevices Dlg

	dtcDev = GetPage(1);
	
	dtcDev->GetDlgItemText(IDC_EDIT_DTC_DEVICE, m_structDTCDevValues.m_strDTCDev,
							sizeof(m_structDTCDevValues.m_strDTCDev));
	dtcDev->GetDlgItemText(IDC_EDIT_DTC_REMARKS, m_structDTCDevValues.m_strRemarks,
							sizeof(m_structDTCDevValues.m_strRemarks));
	
	// These are now in the pop up dlg CAddModMirrors OnOK
//	dtcDev->DlgDirSelect( m_structDTCDevValues.m_strDataDev, IDC_LIST_DATA_DEVICE );
//	dtcDev->DlgDirSelect( m_structDTCDevValues.m_strMirrorDev, IDC_LIST_MIRROR_DEVICE );

	// Send data to config obj
	lpConfig->setDTCDeviceValues();
}

void CDTCConfigPropSheet::setThrottlesValues()
{
	CPropertyPage *throttles;
	int iCheckState;
	// Get All values from the Throttles Dlg

	throttles = GetPage(2);

	throttles->GetDlgItemText(IDC_EDIT_THROTTLE_EDITOR, m_structThrottleValues.m_strThrottle,
							sizeof(m_structThrottleValues.m_strThrottle));

	iCheckState = throttles->IsDlgButtonChecked(IDC_CHECK_CHAINING);
	if(iCheckState)
		m_structThrottleValues.m_bThrottleTrace = TRUE;
	else
		m_structThrottleValues.m_bThrottleTrace = FALSE;

	// Send data to config obj
	lpConfig->setThrottleValues();
}

long int CDTCConfigPropSheet::formatUpdateInterval(char *strUpdateInterval)
{
	char	strDays[3];
	char	strHours[3];
	char	strMinutes[3];
	char	strSeconds[3];

	int		iDays = 0, iHours = 0, iMinutes = 0, iSeconds = 0;
	long int	liTotalTime;

	sprintf(strDays, "%c%c", strUpdateInterval[0], strUpdateInterval[1]);
	sprintf(strHours, "%c%c", strUpdateInterval[3], strUpdateInterval[4]);
	sprintf(strMinutes, "%c%c", strUpdateInterval[6], strUpdateInterval[7]);
	sprintf(strSeconds, "%c%c", strUpdateInterval[9], strUpdateInterval[10]);

	iDays = atoi(strDays);
	iHours = atoi(strHours);
	iMinutes = atoi(strMinutes);
	iSeconds = atoi(strSeconds);

	// Dturrin - Sept 9th, 2001
	// Added these checks to make sure that the maximum interval (ie. 99/23/59/59)
	// does not get exceeded.
	if(iSeconds > 59)
	{
		iSeconds = iSeconds - 60;
		iMinutes = iMinutes + 1;
	}
	if(iMinutes > 59)
	{
		iMinutes = iMinutes - 60;
		iHours   = iHours + 1;
	}
	while(iHours > 23)
	{
		iHours = iHours - 24;
		if (iDays < 99) iDays = iDays + 1;
	}

	liTotalTime = (iDays * 86400) + (iHours * 3600) + (iMinutes * 60) + iSeconds;

	return liTotalTime;
}

void CDTCConfigPropSheet::setTunableValues()
{
	CPropertyPage *tunableVals;
	int	iCheckState;
	// Get All values from the Tunable params Dlg

	// change this back to 3 if throttles are added in
	tunableVals = GetPage(2);

	m_structTunableParamsValues.m_iDepth = tunableVals->GetDlgItemInt(IDC_EDIT_DEPTH, NULL,
															TRUE);

	m_structTunableParamsValues.m_iTimeout = tunableVals->GetDlgItemInt(IDC_EDIT_TIMEOUT, NULL,
															TRUE);

	m_structTunableParamsValues.m_iUpdateInterval = tunableVals->GetDlgItemInt(IDC_EDIT_UPDATE_INTERVAL, NULL,
															TRUE);

	m_structTunableParamsValues.m_iMaxStatFileSize = tunableVals->GetDlgItemInt(IDC_EDIT_MAX_STAT_SIZE, NULL,
															TRUE);

	m_structTunableParamsValues.m_iMaxTranRate = tunableVals->GetDlgItemInt(IDC_EDIT_MAX_TRAN_RATE, NULL,
															TRUE);

	m_structTunableParamsValues.m_iDelayWrites = tunableVals->GetDlgItemInt(IDC_EDIT_WRITE_DELAY, NULL,
															TRUE);

	char	strRefreshInt[15];
	CEdit	*pRefreshInterval;
	memset(strRefreshInt, 0, sizeof(strRefreshInt));
	if(lpConfig->m_structTunableParamsValues.m_liRefreshInterval != -1)
	{
		pRefreshInterval = (CEdit *)tunableVals->GetDlgItemText(IDC_EDIT_REFRESH_INTERVAL, strRefreshInt, sizeof(strRefreshInt));
		m_structTunableParamsValues.m_liRefreshInterval = formatUpdateInterval(strRefreshInt);
	}
	else
	{	
		m_structTunableParamsValues.m_liRefreshInterval = -1;
	}	
	
	iCheckState = tunableVals->IsDlgButtonChecked(IDC_CHECK_SYNC_MODE);
	if(iCheckState)
	{
		m_structTunableParamsValues.m_bSyncMode = TRUE;
		
		// DTurrin - Sept. 24th, 2001
		// A Warning will pop up if IDC_EDIT_DEPTH
		// or IDC_EDITTIMEOUT are out of range.
		if ((m_structTunableParamsValues.m_iDepth == 0)      ||
			(m_structTunableParamsValues.m_iDepth > 99999999)  )
		{
			AfxMessageBox("Warning: Invalid Sync Mode Depth. Default value will be used.");
		}
		if ((m_structTunableParamsValues.m_iTimeout == 0)      ||
			(m_structTunableParamsValues.m_iTimeout > 99999999)  )
		{
			AfxMessageBox("Warning: Invalue Sync Mode Timeout. Default value will be used.");
		}
	}
	else
		m_structTunableParamsValues.m_bSyncMode = FALSE;

	iCheckState = tunableVals->IsDlgButtonChecked(IDC_CHECK_COMPRESSION);
	if(iCheckState)
		m_structTunableParamsValues.m_bCompression = TRUE;
	else
		m_structTunableParamsValues.m_bCompression = FALSE;

	iCheckState = tunableVals->IsDlgButtonChecked(IDC_CHECK_STAT_GEN);
	if(iCheckState)
		m_structTunableParamsValues.m_bStatGen = TRUE;
	else
		m_structTunableParamsValues.m_bStatGen = FALSE;

	iCheckState = tunableVals->IsDlgButtonChecked(IDC_CHECK_USAGE_THRESHOLD);
	if(iCheckState)
		m_structTunableParamsValues.m_bNetThresh = TRUE;
	else
		m_structTunableParamsValues.m_bNetThresh = FALSE;

	// Send data to config obj
	lpConfig->setTunableParams();
	
	// DTurrin - Sept 6th, 2001
	// When the OK button is pressed, any changes will now be
	// saved in the pstore.
	//ac  lpConfig->setTunablesInPStore();
}

BOOL CDTCConfigPropSheet::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if(wParam == 1)
	{
		// OK button
		lpConfig->setPropSheetValues(this);
		
		m_iButtonState = BUTTONOK;
		SetActivePage(0);
		if ( !setSystemDlgValues() )
            return TRUE;//some System parameters are invalid
		SetActivePage(1);
		setDTCDeviceValues();
//		SetActivePage(2);
//		setThrottlesValues();
		SetActivePage(2);
		setTunableValues();
	}
	else if(wParam == 2)
	{
		// Cancel button
		m_iButtonState = BUTTONCANCEL;
        return FALSE;
	}
	else if(wParam == 9)
	{
		m_iButtonState = BUTTONHELP;
		// Help button
		// DTurrin - Sept 26th, 2001
		// Temporarily disabled until the configtool.HLP file
		// is available.
		AfxMessageBox("Please refer to the TDMFBlock.pdf document for help on tdmfconfigtool.exe");
		return TRUE;
	}
	
	return CPropertySheet::OnCommand(wParam, lParam);
}

int CDTCConfigPropSheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Clear all structs
	memset(m_structSystemValues.m_strPrimaryHostName, 0, sizeof(m_structSystemValues.m_strPrimaryHostName));
	memset(m_structSystemValues.m_strPStoreDev, 0, sizeof(m_structSystemValues.m_strPStoreDev));
	memset(m_structSystemValues.m_strSecondHostName, 0, sizeof(m_structSystemValues.m_strSecondHostName));
	memset(m_structSystemValues.m_strJournalDir, 0, sizeof(m_structSystemValues.m_strJournalDir));
	
	memset(m_structDTCDevValues.m_strDTCDev, 0, sizeof(m_structDTCDevValues.m_strDTCDev));
	memset(m_structDTCDevValues.m_strRemarks, 0, sizeof(m_structDTCDevValues.m_strRemarks));
	memset(m_structDTCDevValues.m_strDataDev, 0, sizeof(m_structDTCDevValues.m_strDataDev));
	memset(m_structDTCDevValues.m_strMirrorDev, 0, sizeof(m_structDTCDevValues.m_strMirrorDev));

	memset(m_structThrottleValues.m_strThrottle, 0, sizeof(m_structThrottleValues.m_strThrottle));

	return 0;
}

void CDTCConfigPropSheet::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CPropertySheet::OnActivate(nState, pWndOther, bMinimized);

#ifdef SFTK
	SetWindowText( "Softek TDMF Configuration Tool" );
#else
	SetWindowText( PRODUCTNAME " Configuration Tool" );
#endif
		
	lpConfig->setPropSheetValues(this);

	lpPropsheet = this;

}

void CDTCConfigPropSheet::OnDestroy() 
{
	CPropertySheet::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CDTCConfigPropSheet::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CPropertySheet::OnActivateApp(bActive, hTask);
	
	// TODO: Add your message handler code here
}

