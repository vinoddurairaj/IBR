// NewScriptServerFileName.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ScriptEditorDlg.h"
#include "NewScriptServerFileName.h"
#include "ImportScriptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TYPE_BAT _T("batch File")
#define EXTENSION_BAT _T("bat")
#define MAX_CHARACTERS_IN_FILE   7500
/////////////////////////////////////////////////////////////////////////////
// CScriptEditorDlg dialog


CScriptEditorDlg::CScriptEditorDlg(TDMFOBJECTSLib::IServer* pServer /*=NULL*/,BOOL bReadOnly/*=False*/, CWnd* pParent /*=NULL*/ )
	: m_pServer(pServer),m_bReadOnly(bReadOnly), CDialog(CScriptEditorDlg::IDD, pParent)
{
  
 	//{{AFX_DATA_INIT(CScriptEditorDlg)
	m_ScriptEdit = _T("");
	m_FileSelect = _T("");
	m_strTotChar = _T("0");
	//}}AFX_DATA_INIT
    m_pCrtScriptServerFile = NULL;
}
 

void CScriptEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScriptEditorDlg)
	DDX_Control(pDX, IDC_BTN_IMPORT_SCRIPTS, m_BTN_Import_Ctrl);
	DDX_Control(pDX, IDC_EDIT_TOT_CHAR, m_Tot_Char_Ctrl);
	DDX_Control(pDX, IDC_BTN_SAVE, m_BTN_Save_Ctrl);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_BTN_Delete_Ctrl);
	DDX_Control(pDX, IDC_BUTTON_NEW, m_BTN_New_Ctrl);
	DDX_Control(pDX, IDC_BUTTON_SEND, m_BTN_Send_CTRL);
	DDX_Control(pDX, IDC_CBO_FILE_SELECT, m_CBO_FileSelect_Ctrl);
	DDX_Control(pDX, IDC_RICHEDITCTRL, m_ScriptEdit_Ctrl);
	DDX_Text(pDX, IDC_RICHEDITCTRL, m_ScriptEdit);
	DDX_CBString(pDX, IDC_CBO_FILE_SELECT, m_FileSelect);
	DDX_Text(pDX, IDC_EDIT_TOT_CHAR, m_strTotChar);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScriptEditorDlg, CDialog)
	//{{AFX_MSG_MAP(CScriptEditorDlg)
	ON_CBN_SELCHANGE(IDC_CBO_FILE_SELECT, OnSelchangeCboFileSelect)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_SEND, OnButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_EN_CHANGE(IDC_RICHEDITCTRL, OnChangeRicheditctrl)
	ON_BN_CLICKED(IDC_BTN_SAVE, OnBtnSave)
	ON_BN_CLICKED(IDC_BTN_IMPORT_SCRIPTS, OnBtnImportScripts)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScriptEditorDlg message handlers

BOOL CScriptEditorDlg::OnInitDialog() 
{
	USES_CONVERSION;

	CDialog::OnInitDialog();

    CString strTitle;
    GetWindowText(strTitle);
    strTitle+= _T(" for server:  ");
    strTitle+= OLE2A(m_pServer->Name);
    SetWindowText(strTitle);

    m_ScriptEdit_Ctrl.SetEventMask(m_ScriptEdit_Ctrl.GetEventMask() | ENM_CHANGE);


    m_ScriptEdit_Ctrl.SetOptions(   ECOOP_OR,ECO_WANTRETURN |
                                     ECO_AUTOVSCROLL   |
                                    ECO_AUTOHSCROLL   );

	m_BTN_New_Ctrl.EnableWindow(!m_bReadOnly);
    m_BTN_Import_Ctrl.EnableWindow(!m_bReadOnly);

    LoadScriptComboBox();

  	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CScriptEditorDlg::LoadScriptComboBox()
{

  m_CBO_FileSelect_Ctrl.ResetContent( );
  m_ScriptEdit = _T("");
  m_pCrtScriptServerFile = NULL;

  if(m_pServer != NULL)
  {
      
    long nScriptServerFile = m_pServer->GetScriptServerFileCount();

    if(nScriptServerFile > 0)
    {
        for (long nIndex = 0; nIndex < nScriptServerFile; nIndex++)
	    {
            TDMFOBJECTSLib::IScriptServerFilePtr pSS = m_pServer->GetScriptServerFile(nIndex);

            int nItem = m_CBO_FileSelect_Ctrl.AddString(pSS->FileName);
            m_CBO_FileSelect_Ctrl.SetItemData(nItem,(DWORD)(TDMFOBJECTSLib::IScriptServerFile*)pSS);
            pSS->AddRef();
	    }

       m_ScriptEdit_Ctrl.SetReadOnly(m_bReadOnly); 
       m_ScriptEdit_Ctrl.EnableWindow(true);
       m_CBO_FileSelect_Ctrl.EnableWindow(true);
       m_CBO_FileSelect_Ctrl.SetCurSel(0);
       OnSelchangeCboFileSelect() ;
      
    }
    else
    {
        m_CBO_FileSelect_Ctrl.EnableWindow(false);
        m_ScriptEdit_Ctrl.EnableWindow(false);
        m_BTN_Delete_Ctrl.EnableWindow(false);
        m_BTN_Send_CTRL.EnableWindow(false);
        m_BTN_Save_Ctrl.EnableWindow(false);
   
    }

  }

  m_ScriptEdit_Ctrl.SetModify(false);

  return true;
}

void CScriptEditorDlg::OnSelchangeCboFileSelect() 
{

    VerifyIfTheFileContenthasBeenModify();

    m_pCrtScriptServerFile = (TDMFOBJECTSLib::IScriptServerFile*)m_CBO_FileSelect_Ctrl.GetItemData(m_CBO_FileSelect_Ctrl.GetCurSel());
   
    if(m_pCrtScriptServerFile != NULL)
    {
        m_ScriptEdit = (LPCTSTR)m_pCrtScriptServerFile->Content;
        m_ScriptEdit.TrimRight();
        m_strTotChar.Format("%d",m_ScriptEdit.GetLength() );
        m_FileSelect = (LPCTSTR)m_pCrtScriptServerFile->FileName;
        m_BTN_Delete_Ctrl.EnableWindow(!m_bReadOnly);
        m_BTN_Send_CTRL.EnableWindow(!m_pCrtScriptServerFile->IsNew() && !m_bReadOnly && !m_ScriptEdit_Ctrl.GetModify());
        m_BTN_Save_Ctrl.EnableWindow(!m_bReadOnly);
        m_ScriptEdit_Ctrl.SetReadOnly(m_bReadOnly);
        m_CBO_FileSelect_Ctrl.EnableWindow(true);
        m_ScriptEdit_Ctrl.EnableWindow(true);
    }
    else
    {
        m_BTN_Delete_Ctrl.EnableWindow(false);
        m_BTN_Send_CTRL.EnableWindow(false);
        m_BTN_Save_Ctrl.EnableWindow(false);
        m_CBO_FileSelect_Ctrl.EnableWindow(false);
        m_ScriptEdit = _T("");
        m_ScriptEdit_Ctrl.EnableWindow(false);
     }

     UpdateData(false);
     m_ScriptEdit_Ctrl.SetModify(false);

}

void CScriptEditorDlg::OnDestroy() 
{

   VerifyIfTheFileContenthasBeenModify();


	CDialog::OnDestroy();
	

	// Cleanup
	int nCount = m_CBO_FileSelect_Ctrl.GetCount( ) ;

	for(int i = 0; i < nCount; i++)
	{
		TDMFOBJECTSLib::IScriptServerFilePtr pSS = (TDMFOBJECTSLib::IScriptServerFile*)m_CBO_FileSelect_Ctrl.GetItemData(i);
	    if(pSS != NULL)
        {
            pSS->Release();
            pSS == NULL;
        }
	}	
}

void CScriptEditorDlg::OnButtonSend() 
{
    UpdateData(true);

    if(!m_ScriptEdit_Ctrl.GetModify())
    {
       if(m_pCrtScriptServerFile != NULL)
       {
           CWaitCursor wait;

           if(m_pCrtScriptServerFile->SendScriptServerFileToAgent() == 0)
            {
               
                CString strMsg;
                strMsg.Format("The filename '%s' has been sent to server '%s' successfully!", 
                                (LPCTSTR)m_pCrtScriptServerFile->FileName,
                                (LPCTSTR)m_pServer->Name);
                AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);

           }
       }
    }
    else
    {
       CString strMsg;
       strMsg.Format("You must save the file first before to send it.");
       AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);

    }
}

void CScriptEditorDlg::OnBtnSave() 
{
	UpdateData(true);

    if(m_pCrtScriptServerFile != NULL)
    {
        m_ScriptEdit.FreeExtra();
        m_pCrtScriptServerFile->Content = m_ScriptEdit.AllocSysString();
        m_pCrtScriptServerFile->Type = TYPE_BAT;
        m_pCrtScriptServerFile->Extension = EXTENSION_BAT;
    
    
        if(m_ScriptEdit.IsEmpty())
        {
            CString strMsg;
		    strMsg.Format("The filename '%s' will be deleted because it is empty. Do you want to continue?", (LPCTSTR)m_pCrtScriptServerFile->FileName);
            int nResult = AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION);
       
           if(nResult == IDYES )
           {
               CWaitCursor wait;

               OnButtonDelete() ;

           }
           else
           {
              return;
           }
        }
        else
        {
           m_ScriptEdit.TrimRight();
           if(m_ScriptEdit.GetLength() > MAX_CHARACTERS_IN_FILE)
            {
                if(m_pCrtScriptServerFile != NULL)
                {
                  CString strMsg;
		          strMsg.Format("Sorry, the content of the file '%s' will be truncated to '%d' characters.", (LPCTSTR)m_pCrtScriptServerFile->FileName,MAX_CHARACTERS_IN_FILE);
                  AfxMessageBox(strMsg, MB_OK | MB_ICONWARNING);
                  
                    CString strTruncated = m_ScriptEdit.Left(MAX_CHARACTERS_IN_FILE);
                    m_ScriptEdit = strTruncated;
                    UpdateData(false);
                    m_pCrtScriptServerFile->Content = m_ScriptEdit.AllocSysString();

                   if(m_pCrtScriptServerFile->SaveToDB() == 0)
                    {
                    //   CString strMsg;
                     //   strMsg.Format("The filename '%s' has been saved to database successfully!", (LPCTSTR)m_pCrtScriptServerFile->FileName);
                     //   AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
                        m_ScriptEdit_Ctrl.SetModify(false);
                        OnButtonSend();
                    }
                    else
                    {
                        CString strMsg;
                        strMsg.Format("The filename '%s' has not been saved to database!. Verify that the database is working properly", (LPCTSTR)m_pCrtScriptServerFile->FileName);
                        AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
                    }
                }
            }
           else
           {
            if(m_pCrtScriptServerFile->SaveToDB() == 0)
            {
              //  CString strMsg;
              //  strMsg.Format("The filename '%s' has been saved to database successfully!", (LPCTSTR)m_pCrtScriptServerFile->FileName);
             //   AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
                m_ScriptEdit_Ctrl.SetModify(false);
                OnButtonSend();
            }
            else
            {
                CString strMsg;
                strMsg.Format("The filename '%s' has not been saved to database!. Verify that the database is working properly", (LPCTSTR)m_pCrtScriptServerFile->FileName);
                AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
            }
           }
        }
    }
  

    m_ScriptEdit_Ctrl.SetModify(false);

    if(m_pCrtScriptServerFile != NULL)
        m_BTN_Send_CTRL.EnableWindow(!m_pCrtScriptServerFile->IsNew() && !m_bReadOnly && !m_ScriptEdit_Ctrl.GetModify());
	
}

void CScriptEditorDlg::OnButtonNew() 
{

    VerifyIfTheFileContenthasBeenModify();

    if (m_pServer != NULL)
    {
              
		bool bWindows = (strstr(m_pServer->OSType, "Windows") != 0 ||
			            strstr(m_pServer->OSType, "windows") != 0 ||
			            strstr(m_pServer->OSType, "WINDOWS") != 0);

		CNewScriptServerFileNameDlg NewScriptServerDlg(bWindows);

		if(NewScriptServerDlg.DoModal() == IDOK) 
		{
			if(!IsFilenameAlreadyExistInTheComboBox(NewScriptServerDlg.m_strFileName))
			{
				if (!bWindows)
				{
					NewScriptServerDlg.m_strFileName.MakeLower();
				}		
				
				TDMFOBJECTSLib::IScriptServerFilePtr pSS = m_pServer->CreateScriptServerFile();
				if(pSS)
				{
					_bstr_t bstrValue = NewScriptServerDlg.m_strFileName;
					pSS->FileName = bstrValue;
					
					COleDateTime cdtCurrent = COleDateTime::GetCurrentTime();  
					CString strDate = cdtCurrent.Format("%Y-%m-%d %H:%M:%S");
					pSS->CreationDate = strDate.AllocSysString();
					
					pSS->Type             = TYPE_BAT;
					pSS->Extension        = EXTENSION_BAT; 
					pSS->ParentServerID   = m_pServer->Key;
					
					pSS->Content          = (_T("<new file>"));
					
					int nItem = m_CBO_FileSelect_Ctrl.InsertString(-1, (LPCTSTR)pSS->FileName);
					m_CBO_FileSelect_Ctrl.SetItemData(nItem,(DWORD)(TDMFOBJECTSLib::IScriptServerFile*)pSS);
					pSS->AddRef();
					
					SelectTheScriptServerFile(pSS->ScriptServerID);
					m_ScriptEdit_Ctrl.SetModify(true);
					m_BTN_Send_CTRL.EnableWindow(!pSS->IsNew() && !m_bReadOnly && !m_ScriptEdit_Ctrl.GetModify());
				}
            }
            else
			{
				CString strMsg;
				strMsg.Format("The filename '%s' already exist please enter another one.", (LPCTSTR)NewScriptServerDlg.m_strFileName);
				AfxMessageBox(strMsg, MB_OK | MB_ICONWARNING);
	        }
		}
	}
}



void CScriptEditorDlg::OnButtonDelete() 
{
    if(m_pCrtScriptServerFile != NULL)
    {
        int nIndex = m_CBO_FileSelect_Ctrl.FindStringExact(-1,m_pCrtScriptServerFile->FileName);

        if(nIndex == CB_ERR )
            return;
       
        if(m_pServer != NULL)
        {
            BOOL bIsNew = m_pCrtScriptServerFile->IsNew();
            CWaitCursor wait;

            CString strFilename = (LPCTSTR) m_pCrtScriptServerFile->FileName;
	      
            if(!bIsNew)
            {
                //Send empty ScriptServer to server to delete it
                m_pCrtScriptServerFile->Content = _T("");
                if(m_pCrtScriptServerFile->SendScriptServerFileToAgent() == 0)
                {

                    CString strMsg;
                    strMsg.Format("The filename '%s' has been removed from server '%s' successfully!", 
                                    (LPCTSTR)m_pCrtScriptServerFile->FileName,
                                    (LPCTSTR)m_pServer->Name);
                    AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
                 }
			}
            if( m_pServer->RemoveScriptServerFile(m_pCrtScriptServerFile->ScriptServerID))
            {
	          //  if(!bIsNew)
              //  {
               //     CString strMsg;
	          //     strMsg.Format("The filename '%s' has been removed from the database successfully!",strFilename);
	          //      AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
              //  } 
                
                //release the object
	            m_pCrtScriptServerFile->Release();
	            m_pCrtScriptServerFile = NULL;

       
	            //Remove from ComboBox
	            m_CBO_FileSelect_Ctrl.SetItemData(nIndex,NULL);  
	            m_CBO_FileSelect_Ctrl.DeleteString( nIndex );

	            if(m_CBO_FileSelect_Ctrl.GetCount( ) > 0)
	            {
                    int nIndex = m_CBO_FileSelect_Ctrl.GetCurSel();
                    if( nIndex == CB_ERR)
                    {
                        m_CBO_FileSelect_Ctrl.SetCurSel(0);
                    }
                    else
                    {
                        m_CBO_FileSelect_Ctrl.SetCurSel(nIndex);
                    }
		            
		            OnSelchangeCboFileSelect() ;
	            }
	            else
	            {
		            m_BTN_Delete_Ctrl.EnableWindow(false);
		            m_BTN_Send_CTRL.EnableWindow(false);
		            m_BTN_Save_Ctrl.EnableWindow(false);
		            m_CBO_FileSelect_Ctrl.EnableWindow(false);
		            m_ScriptEdit = _T("");
		            m_ScriptEdit_Ctrl.EnableWindow(false);
		            UpdateData(false);
	            }
            }
        }
    }
}

bool CScriptEditorDlg::SelectTheScriptServerFile(long ScriptServerID)
{

    TDMFOBJECTSLib::IScriptServerFilePtr pSS;
  	for (int i = 0; i < m_CBO_FileSelect_Ctrl.GetCount(); i++)
	{
		pSS = (TDMFOBJECTSLib::IScriptServerFile*)m_CBO_FileSelect_Ctrl.GetItemData(i);
	
        if (pSS->ScriptServerID == ScriptServerID)
		{
			m_CBO_FileSelect_Ctrl.SetCurSel(i);
            OnSelchangeCboFileSelect() ;
            return true;
		}
	}
    return false;

    
}

void CScriptEditorDlg::OnChangeRicheditctrl() 
{
    UpdateData(true);
    m_ScriptEdit.TrimRight();
    m_strTotChar.Format("%d",m_ScriptEdit.GetLength() );
    m_Tot_Char_Ctrl.SetWindowText(m_strTotChar);

    if(m_ScriptEdit.GetLength() > MAX_CHARACTERS_IN_FILE)
    {
        
        if(m_pCrtScriptServerFile != NULL)
        {
          CString strMsg;
		  strMsg.Format("Sorry, the content of the file '%s' can not exceed '%d' characters.\nPlease reduce the content before to save.", (LPCTSTR)m_pCrtScriptServerFile->FileName,MAX_CHARACTERS_IN_FILE);
          AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
        }
        m_BTN_Save_Ctrl.EnableWindow(false);
        m_BTN_Send_CTRL.EnableWindow(false);
    }
    else
    {
        m_BTN_Save_Ctrl.EnableWindow(!m_bReadOnly);
        m_BTN_Send_CTRL.EnableWindow(!m_bReadOnly && !m_ScriptEdit_Ctrl.GetModify());
    }
 
    
}

bool CScriptEditorDlg::IsFilenameAlreadyExistInTheComboBox(CString strFilename)
{
   	// Cleanup
	int nCount = m_CBO_FileSelect_Ctrl.GetCount( ) ;

	for(int i = 0; i < nCount; i++)
	{
		TDMFOBJECTSLib::IScriptServerFilePtr pSS = (TDMFOBJECTSLib::IScriptServerFile*)m_CBO_FileSelect_Ctrl.GetItemData(i);
	    if(pSS != NULL)
        {
            if(pSS->FileName == _bstr_t(strFilename))
            {
                return true;
            }

        }
           
	}	
    return false;
}

void CScriptEditorDlg::VerifyIfTheFileContenthasBeenModify()
{

    if(m_ScriptEdit_Ctrl.GetModify())
    {
        if(m_pCrtScriptServerFile != NULL)
        {
           CString strMsg;
           strMsg.Format("Do you want to save your changes made to the file '%s' ?", (LPCTSTR)m_pCrtScriptServerFile->FileName);
           int nResult = AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION);

           if(nResult == IDYES )
           {
                OnBtnSave();
           }
           else
           {
              if(m_pCrtScriptServerFile->IsNew())
              {
                OnButtonDelete();
              }
              m_ScriptEdit_Ctrl.SetModify(false);
           }
        }
    }		
}



BOOL CScriptEditorDlg::AllTheContentISCorrect(CString &str)
{
    CString strResult;
    if(!str.IsEmpty())
    {
       strResult = str.SpanExcluding(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,;:/'"));
       int value = str.FindOneOf(_T("<"));
    }
    return true;
}

void CScriptEditorDlg::OnBtnImportScripts() 
{
    int nScriptID = -1 ;
    if(m_pCrtScriptServerFile != NULL)
    {
      nScriptID = m_pCrtScriptServerFile->ScriptServerID;
    }

	CImportScriptDlg dlg(m_pServer);
    dlg.DoModal();
    
    LoadScriptComboBox();
    
    if(nScriptID != -1)
    {
      SelectTheScriptServerFile(nScriptID);
    }
}
