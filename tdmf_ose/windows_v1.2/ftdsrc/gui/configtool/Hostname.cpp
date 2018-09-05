// hostname.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "Config.h"
#include "Hostname.h"
#include "tcp.h"

#include "winsock2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;

/////////////////////////////////////////////////////////////////////////////
// CHostname dialog


CHostname::CHostname(CWnd* pParent /*=NULL*/)
	: CDialog(CHostname::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHostname)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CHostname::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHostname)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHostname, CDialog)
	//{{AFX_MSG_MAP(CHostname)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHostname message handlers

BOOL CHostname::OnInitDialog() 
{
	CDialog::OnInitDialog();

	struct hostent *hostInfo;
	u_long         ipAddress;
	char           strIPAddress[255], strHostName[255];
	char           strPortion[5], errorMessage[355];
	char           seps[]   = ".";
	char           *token;

#if defined(_WINDOWS)
    (void)tcp_startup();
#endif

	// Get saved IP Address format (a.b.c.d) host name information
	memset(strHostName, 0, sizeof(strHostName));
	if (host_Type == PRIM_HOST)
	{
		strcpy(strHostName,lpConfig->m_structSystemValues.m_strPrimaryHostName);
	}
	else
	{
		strcpy(strHostName,lpConfig->m_structSystemValues.m_strSecondHostName);
	}

	if(strlen(strHostName) != 0)
	{
		// Remove trailing 0's from IP Address Potion
		token = strtok( strHostName, seps );
		sprintf(strPortion,"%d.",atoi(token));
		strcpy(strIPAddress,strPortion);

		token = strtok( NULL, seps );
		sprintf(strPortion,"%d.",atoi(token));
		strcat(strIPAddress,strPortion);

		token = strtok( NULL, seps );
		sprintf(strPortion,"%d.",atoi(token));
		strcat(strIPAddress,strPortion);

		token = strtok( NULL, seps );
		sprintf(strPortion,"%d",atoi(token));
		strcat(strIPAddress,strPortion);

		// Convert to standard IP address format
		ipAddress = inet_addr(strIPAddress);
		// Get HOSTENT structure information
		hostInfo = gethostbyaddr((char *)&ipAddress,sizeof(ipAddress),AF_INET);

		if (hostInfo != NULL)
		{
			sprintf(strHostName,hostInfo->h_name);
			// Put host name in Edit Dialog Box
			SetDlgItemText(IDC_HOSTNAME,strHostName);
		}
		else
		{
			sprintf(errorMessage,"No Host Name found for IP Adress %s",strIPAddress);
			AfxMessageBox(errorMessage);
		}
	}

	
#if defined(_WINDOWS)
	(void)tcp_cleanup();
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CHostname::OnOK() 
{
	struct hostent  *hostInfo;
	struct in_addr  in;
	char            **p;
	char            strHostName[_MAX_PATH], errorMessage[_MAX_PATH + 100];
	int             errnum;

#if defined(_WINDOWS)
    (void)tcp_startup();
#endif

	// Retrieve Edit Dialog Box value
	GetDlgItemText(IDC_HOSTNAME, strHostName, sizeof(strHostName));

	if (strlen(strHostName) != 0)
	{
		// Insert a NULL character at the end of the host name
		// in case the Edit Dialog Box contained too many characters
		strHostName[_MAX_PATH - 1] = NULL;

		hostInfo = gethostbyname(strHostName);
		if (hostInfo == NULL)
		{
#if defined(_WINDOWS)
			errnum = WSAGetLastError();
#else
			errnum = errno;
#endif
			sprintf(errorMessage,"Host information for\n-> %s <-\nnot found: (errno = %d)",
				    strHostName, errnum);

			AfxMessageBox(errorMessage);

			strcpy(strHostName,"");
		}
		else
		{
			// Retreive the IP Address and copy it to
			// the strHostName buffer
			p = hostInfo->h_addr_list;
			(void) memcpy(&in.s_addr, *p, sizeof (in.s_addr));
			strcpy(strHostName,inet_ntoa(in));
		}
	}

	// Set the corresponding host name to it's new value
	if (host_Type == PRIM_HOST)
	{
		strcpy(lpConfig->m_structSystemValues.m_strPrimaryHostName,strHostName);
	}
	else
	{
		strcpy(lpConfig->m_structSystemValues.m_strSecondHostName,strHostName);
	}

#if defined(_WINDOWS)
	(void)tcp_cleanup();
#endif

	CDialog::OnOK();
}
