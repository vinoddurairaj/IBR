// AddGroup.cpp : implementation file
//
#include "stdafx.h"
#include "DTCConfigTool.h"
#include "Config.h"
#include "AddGroup.h"

#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddGroup dialog


CAddGroup::CAddGroup(CWnd* pParent /*=NULL*/)
	: CDialog(CAddGroup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddGroup)
	m_szGroupNote = _T("");
	//}}AFX_DATA_INIT
}


void CAddGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddGroup)
	DDX_Text(pDX, IDC_EDIT_GROUP_NOTE, m_szGroupNote);
	DDV_MaxChars(pDX, m_szGroupNote, 240);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddGroup, CDialog)
	//{{AFX_MSG_MAP(CAddGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddGroup message handlers

int CAddGroup::getGroup()
{
	char	strGroup[10];
	
	m_pEditGroup = (CEdit *)GetDlgItem(IDC_EDIT_GROUP);
	memset(strGroup, 0, sizeof(strGroup));
	m_pEditGroup->GetLine( 1, strGroup, sizeof(strGroup));

	// DTurrin - Sept 10th, 2001
	// If IDC_EDIT_GROUP was empty, return 1000.
	if (strlen(strGroup) == 0) return(1000);

	return(atoi(strGroup));
}

void CAddGroup::getGroupNote(char strGroupNote[_MAX_PATH])
{
	m_pEditGroupNote = (CEdit *)GetDlgItem(IDC_EDIT_GROUP_NOTE);
	memset(strGroupNote, 0, _MAX_PATH);
	// DTurrin - Sept 26th, 2001
	// To make sure that the remarks string ends with a NULL
	// character, insert NULL character in buffer's last position.
	if (m_pEditGroupNote->LineLength() == 0)
	{
		strcpy(strGroupNote,"");
	}
	else
	{
		m_pEditGroupNote->GetLine( 0, strGroupNote, _MAX_PATH);
		strGroupNote[_MAX_PATH - 1] = NULL;
	}
}

void CAddGroup::OnOK() 
{
	extern CConfig	*lpConfig;

	char strPath[_MAX_PATH], strCfgFileInUse[_MAX_PATH];
	char strAskOW[255];

	m_iGroup = getGroup();

	// DTurrin - Sept 10th, 2001
	// If IDC_EDIT_GROUP was empty, send a warning.
	if (m_iGroup >= 1000)
	{
		AfxMessageBox("Please enter a Group number between 0 and 999");
		return; // Tell the user that the dialog cannot be closed without
		        // entering the necessary values and do not close the dialog.
	}

	// Dturrin - Oct 16th, 2001
	// If the group already exists, ask for confirmation before
	// Overwriting it. Also, if the group is started, it cannot
	// be overwritten.
	lpConfig->getConfigPath(strPath);
	memset(strCfgFileInUse, 0, sizeof(strCfgFileInUse));
	sprintf(strCfgFileInUse, "%sp%03d.cfg", strPath, m_iGroup);
#if defined(_WINDOWS)
	if (_access(strCfgFileInUse,0) == 0) {
#else
	if (access(strCfgFileInUse,F_OK) == 0) {
#endif
		sprintf(strAskOW,"Group %i already exists. Overwrite it?", m_iGroup);
		if(IDOK != ::MessageBox(NULL, strAskOW,	PRODUCTNAME, MB_OKCANCEL))
		{
			return;
		}
		else
		{
			sprintf(strCfgFileInUse, "%sp%03d.cur", strPath, m_iGroup);
#if defined(_WINDOWS)
			if (_access(strCfgFileInUse,0) == 0)
			{
#else
			if (access(strCfgFileInUse,F_OK) == 0)
			{
#endif
				sprintf(strAskOW,"Found file p%03d.cur. Group %i must be stopped before overwriting it.", m_iGroup, m_iGroup);
				AfxMessageBox(strAskOW);
				return;
			}
		}
	}

	getGroupNote(m_strGroupNote);

	CDialog::OnOK();
}

BOOL CAddGroup::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_pEditGroup = (CEdit *)GetDlgItem(IDC_EDIT_GROUP);	
	m_pEditGroup->SetLimitText( 3 );
	m_bAddGroupCancel = FALSE;
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddGroup::OnCancel() 
{
	m_bAddGroupCancel = TRUE;
	
	CDialog::OnCancel();
}
