// SystemPage.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "SystemPage.h"
extern "C"
{
#include "iputil.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemPage property page

IMPLEMENT_DYNCREATE(CSystemPage, CPropertyPage)

CSystemPage::CSystemPage(CGroupConfig* pGroupConfig) : CPropertyPage(CSystemPage::IDD), m_pGroupConfig(pGroupConfig)
{
	//{{AFX_DATA_INIT(CSystemPage)
	m_cstrPrimaryHost = _T("");
	m_cstrPStore = _T("");
	m_cstrNote = _T("");
	m_cstrSecondaryHost = _T("");
	m_cstrSecondaryPort = _T("");
	m_cstrJournal = _T("");
	m_bChaining = FALSE;
	//}}AFX_DATA_INIT
}

void CSystemPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemPage)
	DDX_Text(pDX, IDC_EDIT_PRI_HOST_OR_IP, m_cstrPrimaryHost);
	DDX_Text(pDX, IDC_EDIT_PSTORE, m_cstrPStore);
	DDX_Text(pDX, IDC_EDIT_SYSTEM_NOTE, m_cstrNote);
	DDX_Text(pDX, IDC_EDIT_SEC_HOST_OR_IP, m_cstrSecondaryHost);
	DDX_Text(pDX, IDC_EDIT_SECONDARY_PORT, m_cstrSecondaryPort);
	DDX_Text(pDX, IDC_EDIT_JOURNAL_DIR, m_cstrJournal);
	DDX_Check(pDX, IDC_CHECK_CHAINING, m_bChaining);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystemPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSystemPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemPage message handlers

BOOL CSystemPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_cstrPrimaryHost   = m_pGroupConfig->GetPrimaryHost();
	m_cstrPStore        = m_pGroupConfig->GetPStore();
	m_cstrNote          = m_pGroupConfig->GetNote();
	m_cstrSecondaryHost = m_pGroupConfig->GetSecondaryHost();
	m_cstrSecondaryPort = m_pGroupConfig->GetSecondaryPort();
	m_cstrJournal       = m_pGroupConfig->GetJournal();
	m_bChaining         = m_pGroupConfig->GetChaining();
	
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSystemPage::Validate()
{
    // validation of some values
    unsigned long ip;
    char hostname[256];

	char szHostnamePrimary[256];
	char szHostnameSecondary[256];
	strcpy(szHostnamePrimary, m_cstrPrimaryHost);
	strcpy(szHostnameSecondary, m_cstrSecondaryHost);

	CString cstrTitle;
	GetParent()->GetWindowText(cstrTitle);

    if (name_is_ipstring(szHostnamePrimary))
    {
        if (ipstring_to_ip(szHostnamePrimary, &ip) < 0)
        {
            CString errmsg = "Primary System \'";
            errmsg += m_cstrPrimaryHost;
            errmsg += "\' is not a valid IP address format.";
			MessageBox(errmsg, cstrTitle);
            return FALSE;
        }
        if (ip_to_name(ip, hostname) < 0)
        {
            CString errmsg = "Primary System \'";
            errmsg += m_cstrPrimaryHost;
            errmsg += "\' does not match any host on the network.";
            MessageBox(errmsg, cstrTitle);
            return FALSE;
        }
    }
    else
    {   //it is an host name.  Check if the network knows it...
        if (name_to_ip(szHostnamePrimary, &ip) < 0)
        {
            CString errmsg = "Primary System \'";
            errmsg += m_cstrPrimaryHost;
            errmsg += "\' is not a recognized host name.";
            MessageBox(errmsg, cstrTitle);
            return FALSE;
        }
    }

    if (name_is_ipstring(szHostnameSecondary))
    {
        if (ipstring_to_ip(szHostnameSecondary, &ip) < 0)
        {
            CString errmsg = "Secondary System \'";
            errmsg += m_cstrSecondaryHost;
            errmsg += "\' is not a valid IP address format.";
            MessageBox(errmsg, cstrTitle);
            return FALSE;
        }
        if (ip_to_name(ip, hostname) < 0)
        {
            CString errmsg = "Secondary System \'";
            errmsg += m_cstrSecondaryHost;
            errmsg += "\' does not match any host on the network.";
            MessageBox(errmsg, cstrTitle);
            return FALSE;
        }
    }
    else
    {   //it is an host name.  Check if the network knows it...
        if (name_to_ip(szHostnameSecondary, &ip) < 0)
        {
            CString errmsg = "Secondary System \'";
            errmsg += m_cstrSecondaryHost;
            errmsg += "\' is not a recognized host name.";
            MessageBox(errmsg, cstrTitle);
            return FALSE;
        }
    }

	// TODO: conversion should not be done here
	//automatic conversion to loopback mode
	if (!strcmp(m_cstrPrimaryHost,"127.0.0.1") || !strcmp(m_cstrSecondaryHost, "127.0.0.1"))
	{
		m_cstrPrimaryHost   = "127.0.0.1\0";
		m_cstrSecondaryHost = "127.0.0.1\0";
	}

    //validation of other values
    char szDrive[8],szDir[MAX_PATH],szFile[MAX_PATH];
    _splitpath(m_cstrPStore, szDrive, szDir, szFile, NULL);
    if (szFile[0] == 0)
    {
        CString errmsg = "PStore value \'";
        errmsg += m_cstrPStore;
        errmsg += "\' is not a complete file name.";
        MessageBox(errmsg, cstrTitle);
        return FALSE;
    }
    _splitpath(m_cstrJournal, szDrive, szDir, szFile, NULL);
    if (szDir[0] == 0)
    {
        CString errmsg = "Journal value \'";
        errmsg += m_cstrJournal;
        errmsg += "\' is not valid a directory name.";
        MessageBox(errmsg, cstrTitle);
        return FALSE;
    }

	return TRUE;
}

BOOL CSystemPage::SaveValues()
{
	UpdateData();

	m_pGroupConfig->SetPrimaryHost(m_cstrPrimaryHost);
	m_pGroupConfig->SetPStore(m_cstrPStore);
	m_pGroupConfig->SetNote(m_cstrNote);
	m_pGroupConfig->SetSecondaryHost(m_cstrSecondaryHost);
	m_pGroupConfig->SetSecondaryPort(m_cstrSecondaryPort);
	m_pGroupConfig->SetJournal(m_cstrJournal);
	m_pGroupConfig->SetChaining(m_bChaining ? true : false);

	return TRUE;
}
