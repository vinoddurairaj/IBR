// NewScriptServerFileName.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "NewScriptServerFileName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_CHARACTERS	80

/////////////////////////////////////////////////////////////////////////////
// CNewScriptServerFileNameDlg dialog


CNewScriptServerFileNameDlg::CNewScriptServerFileNameDlg(BOOL bWindows, CWnd* pParent /*=NULL*/)
	: CDialog(CNewScriptServerFileNameDlg::IDD, pParent), m_bWindows(bWindows)
{
	//{{AFX_DATA_INIT(CNewScriptServerFileNameDlg)
	m_strFileName = _T("");
	//}}AFX_DATA_INIT
}


void CNewScriptServerFileNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewScriptServerFileNameDlg)
	DDX_Control(pDX, IDC_EDIT_FILENAME, m_FileNameCtrl);
	DDX_Control(pDX, IDOK, m_BTN_OK_CTRL);
	DDX_Text(pDX, IDC_EDIT_FILENAME, m_strFileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewScriptServerFileNameDlg, CDialog)
	//{{AFX_MSG_MAP(CNewScriptServerFileNameDlg)
	ON_EN_CHANGE(IDC_EDIT_FILENAME, OnChangeEditFilename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewScriptServerFileNameDlg message handlers

void CNewScriptServerFileNameDlg::OnChangeEditFilename() 
{
    UpdateData(true);

	m_strFileName.MakeLower();

    m_BTN_OK_CTRL.EnableWindow(!m_strFileName.IsEmpty());

    if(m_strFileName.GetLength() > MAX_CHARACTERS)
    {
       AfxMessageBox("You have exceed the maximum of characters allowed for the filename", MB_OK|MB_ICONSTOP); 
       m_strFileName = m_strFileName.Left(MAX_CHARACTERS);
       UpdateData(false);
       m_FileNameCtrl.SetFocus();
       m_FileNameCtrl.SetSel(0, -1);
       return;
    }

}

BOOL CNewScriptServerFileNameDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    m_FileNameCtrl.LimitText(MAX_CHARACTERS);
	
    m_BTN_OK_CTRL.EnableWindow(!m_strFileName.IsEmpty());	

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewScriptServerFileNameDlg::OnOK() 
{
    UpdateData(true);

    m_strFileName.TrimRight();

    if(m_strFileName.GetLength() > MAX_CHARACTERS)
    {
       AfxMessageBox("You have exceed the maximum of characters allowed for the filename", MB_OK|MB_ICONSTOP); 
       m_FileNameCtrl.SetFocus();
       m_FileNameCtrl.SetSel(0, -1);
       return;
    }

    CString strExtension = m_strFileName.Right(4);

	if(m_bWindows && strExtension.CompareNoCase(_T(".bat")) != 0)
    {
       AfxMessageBox("You must enter a valid filename with extension '.bat'", MB_OK|MB_ICONSTOP); 
       m_FileNameCtrl.SetFocus();
       m_FileNameCtrl.SetSel(0, -1);
       
       return;
    }

	strExtension = m_strFileName.Right(3);

	if(!m_bWindows && strExtension.CompareNoCase(_T(".sh")) != 0)
    {
       AfxMessageBox("You must enter a valid filename with extension '.sh'", MB_OK|MB_ICONSTOP); 
       m_FileNameCtrl.SetFocus();
       m_FileNameCtrl.SetSel(0, -1);
       
       return;
    }

	CDialog::OnOK();
}
