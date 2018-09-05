// ImportScriptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ImportScriptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_CHARACTERS	80

/////////////////////////////////////////////////////////////////////////////
// CImportScriptDlg dialog


CImportScriptDlg::CImportScriptDlg(TDMFOBJECTSLib::IServer* pServer /*=NULL*/, CWnd* pParent /*=NULL*/ )
	: m_pServer(pServer), CDialog(CImportScriptDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportScriptDlg)
	m_strFileName = _T("");
	//}}AFX_DATA_INIT
}


void CImportScriptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportScriptDlg)
	DDX_Control(pDX, IDC_IMPORT_FILE, m_BTN_Import);
    DDX_Control(pDX, IDC_RADIO_All, m_RADIO_ALL_FILES);
    DDX_Control(pDX, IDC_RADIO_SPECIFIC, m_RADIO_SPECIFIC_FILE);
	DDX_Control(pDX, IDC_EDIT_FILENAME, m_Edit_Filename_Ctrl);
	DDX_Control(pDX, IDC_CHECK_OVERWRITE_ALL, m_Chk_OverwriteAll_Ctrl);
	DDX_Text(pDX, IDC_EDIT_FILENAME, m_strFileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportScriptDlg, CDialog)
	//{{AFX_MSG_MAP(CImportScriptDlg)
	ON_BN_CLICKED(IDC_RADIO_All, OnRADIOAll)
	ON_BN_CLICKED(IDC_RADIO_SPECIFIC, OnRadioSpecific)
	ON_BN_CLICKED(IDC_IMPORT_FILE, OnImportFile)
	ON_EN_CHANGE(IDC_EDIT_FILENAME, OnChangeEditFilename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportScriptDlg message handlers

BOOL CImportScriptDlg::OnInitDialog() 
{
	USES_CONVERSION;

	CDialog::OnInitDialog();
	
    CString strTitle;
    GetWindowText(strTitle);
    strTitle+= _T(" for server:  ");
    strTitle+= OLE2A(m_pServer->Name);
    SetWindowText(strTitle);	

    m_RADIO_ALL_FILES.SetCheck(TRUE);

    OnRADIOAll();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CImportScriptDlg::OnChangeEditFilename() 
{
    UpdateData(true);

    m_BTN_Import.EnableWindow(!m_strFileName.IsEmpty());

    if(m_strFileName.GetLength() > MAX_CHARACTERS)
    {
       AfxMessageBox("You have exceed the maximum of characters allowed for the filename", MB_OK|MB_ICONSTOP); 
       m_strFileName = m_strFileName.Left(MAX_CHARACTERS);
       UpdateData(false);
       m_Edit_Filename_Ctrl.SetFocus();
       m_Edit_Filename_Ctrl.SetSel(0, -1);
       return;
    }
	
}

void CImportScriptDlg::OnRADIOAll() 
{

    BOOL bCheck = m_RADIO_ALL_FILES.GetCheck();
	
 	m_Chk_OverwriteAll_Ctrl.EnableWindow(bCheck);
 	m_Chk_OverwriteAll_Ctrl.SetCheck(FALSE);

	m_Edit_Filename_Ctrl.EnableWindow(!bCheck);
   
    m_BTN_Import.EnableWindow(TRUE);
	
}

void CImportScriptDlg::OnRadioSpecific() 
{
  
    BOOL bCheck = m_RADIO_SPECIFIC_FILE.GetCheck();
	
  	m_Chk_OverwriteAll_Ctrl.EnableWindow(!bCheck);
    m_Chk_OverwriteAll_Ctrl.SetCheck(bCheck);
	m_Edit_Filename_Ctrl.EnableWindow(bCheck);

    m_BTN_Import.EnableWindow(!m_strFileName.IsEmpty());
	
}

void CImportScriptDlg::OnImportFile() 
{
    CWaitCursor wait;

    UpdateData(TRUE);

    if(m_pServer == NULL)
    {
      return;
    }

    long nCountBefore = m_pServer->GetScriptServerFileCount();

	if(m_RADIO_SPECIFIC_FILE.GetCheck())
    {
        if(m_strFileName.IsEmpty() || m_strFileName.GetLength() < 4 )
        {
            CString strMsg;
            strMsg.Format("You must specify a valid filename.");
            AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
            m_Edit_Filename_Ctrl.SetFocus();
            return;
        }
        else
        {
          if(!ImportSpecificFile(m_strFileName))
          {
             m_Edit_Filename_Ctrl.SetFocus();
             m_Edit_Filename_Ctrl.SetSel(0, -1);
          }
        }
            
    }
    else
    {
		bool bWindows = (strstr(m_pServer->OSType, "Windows") != 0 ||
						strstr(m_pServer->OSType, "windows") != 0 ||
						strstr(m_pServer->OSType, "WINDOWS") != 0);
		CString strExtension;

		if (bWindows)
		{
			strExtension = _T("*.bat");
		}
		else
		{
			strExtension = _T("*.sh");
		}

        ImportAllFiles(strExtension);
    }
    long nCountAfter = m_pServer->GetScriptServerFileCount();
    if(nCountAfter > nCountBefore)
    {  
       long nTot = nCountAfter - nCountBefore;
       CString strMsg;
       strMsg.Format("%d new file(s) has been imported.",nTot);
       AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
    }
}

BOOL CImportScriptDlg::ImportAllFiles(CString strExtension)
{
   if(m_pServer != NULL)
   {
     
     if(m_pServer->ImportAllScriptServerFiles(m_Chk_OverwriteAll_Ctrl.GetCheck(),(LPCTSTR)strExtension) == 0)
     {
        return TRUE;
     }
         
   }

   return FALSE;

}

BOOL CImportScriptDlg::ImportSpecificFile(CString strFilename)
{
 

	if(m_pServer != NULL)
	{
		bool bWindows = (strstr(m_pServer->OSType, "Windows") != 0 ||
						strstr(m_pServer->OSType, "windows") != 0 ||
						strstr(m_pServer->OSType, "WINDOWS") != 0);
		if (!bWindows)
		{
			strFilename.MakeLower();
		}

		if(m_pServer->ImportOneScriptServerFile((LPCTSTR)strFilename) == 0)
			return TRUE;
   }

   return FALSE;
}


