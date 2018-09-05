#if !defined(AFX_SCRIPTEDITORPAGE_H__BCCBA250_6C87_440C_B47B_2641463BD6CD__INCLUDED_)
#define AFX_SCRIPTEDITORPAGE_H__BCCBA250_6C87_440C_B47B_2641463BD6CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScriptEditorPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScriptEditorPage dialog

class CScriptEditorPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CScriptEditorPage)

// Construction
public:
	CScriptEditorPage();
	~CScriptEditorPage();
	BOOL AllTheContentISCorrect(CString &str);
	void VerifyIfTheFileContenthasBeenModify();
	bool IsFilenameAlreadyExistInTheComboBox(CString strFilename);
	bool SelectTheScriptServerFile(long ScriptServerID);
	BOOL LoadScriptComboBox();
 
// Dialog Data
	//{{AFX_DATA(CScriptEditorPage)
	enum { IDD = IDD_SCRIPT_EDITOR_PAGE };
	CButton	m_BTN_Import_Ctrl;
	CEdit	m_Tot_Char_Ctrl;
	CButton	m_BTN_Save_Ctrl;
	CButton	m_BTN_Delete_Ctrl;
	CButton	m_BTN_New_Ctrl;
	CButton	m_BTN_Send_CTRL;
	CComboBox	m_CBO_FileSelect_Ctrl;
	CRichEditCtrl	m_ScriptEdit_Ctrl;
	CString	m_ScriptEdit;
	CString	m_FileSelect;
	CString	m_strTotChar;
    //}}AFX_DATA
  
   CRichEditCtrl m_RichEditCtrl;
   BOOL m_bReadOnly;
   TDMFOBJECTSLib::IServerPtr m_pServer;
   TDMFOBJECTSLib::IScriptServerFilePtr m_pCrtScriptServerFile;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScriptEditorPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScriptEditorPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCboFileSelect();
	afx_msg void OnDestroy();
	afx_msg void OnButtonSend();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonDelete();
	afx_msg void OnChangeRicheditctrl();
	afx_msg void OnBtnSave();
	afx_msg void OnBtnImportScripts();	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRIPTEDITORPAGE_H__BCCBA250_6C87_440C_B47B_2641463BD6CD__INCLUDED_)
