// AddGroup.cpp : implementation file
//
#include "stdafx.h"
#include "AddGroup.h"

#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddGroup dialog


CAddGroup::CAddGroup(LPCSTR lpcstrInstallPath, LPCSTR lpcstrProductName, CWnd* pParent /*=NULL*/)
	: CDialog(CAddGroup::IDD, pParent), m_cstrInstallPath(lpcstrInstallPath), m_cstrProductName(lpcstrProductName)
{
	//{{AFX_DATA_INIT(CAddGroup)
	m_cstrGroupNote = _T("");
	m_cstrGroupNumber = _T("");
	//}}AFX_DATA_INIT
}


void CAddGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddGroup)
	DDX_Control(pDX, IDC_EDIT_GROUP, m_EditGroupNumber);
	DDX_Text(pDX, IDC_EDIT_GROUP_NOTE, m_cstrGroupNote);
	DDX_Text(pDX, IDC_EDIT_GROUP, m_cstrGroupNumber);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddGroup, CDialog)
	//{{AFX_MSG_MAP(CAddGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddGroup message handlers

int CAddGroup::GetGroup()
{
	return m_cstrGroupNumber.IsEmpty() ? 1000 : atoi(m_cstrGroupNumber);
}

CString CAddGroup::GetGroupNote()
{
	return m_cstrGroupNote;
}

void CAddGroup::OnOK() 
{
	UpdateData();

	// If IDC_EDIT_GROUP was empty, send a warning.
	if (GetGroup() >= 1000)
	{
		MessageBox("Please enter a Group number between 0 and 999", m_cstrProductName);
		return; // Tell the user that the dialog cannot be closed without
		        // entering the necessary values and do not close the dialog.
	}

	// If the group already exists, ask for confirmation before
	// Overwriting it. Also, if the group is started, it cannot
	// be overwritten.

	CString cstrCfgFileInUse;
	cstrCfgFileInUse.Format("%sp%03d.cfg", m_cstrInstallPath, GetGroup());
	if (_access(cstrCfgFileInUse, 0) == 0)
	{
		CString cstrMsg;
		cstrMsg.Format("Group %i already exists. Overwrite it?", GetGroup());
		if(IDOK != ::MessageBox(NULL, cstrMsg, m_cstrProductName, MB_OKCANCEL))
		{
			return;
		}
		else
		{
			cstrCfgFileInUse.Format("%sp%03d.cur", m_cstrInstallPath, GetGroup());
			if (_access(cstrCfgFileInUse,0) == 0)
			{
				cstrMsg.Format("Found file p%03d.cur. Group %i must be stopped before overwriting it.", GetGroup(), GetGroup());
				MessageBox(cstrMsg, m_cstrProductName);
				return;
			}
		}
	}

	CDialog::OnOK();
}

BOOL CAddGroup::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_EditGroupNumber.SetLimitText(3);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
