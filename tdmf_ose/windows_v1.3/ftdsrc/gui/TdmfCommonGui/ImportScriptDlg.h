#if !defined(AFX_IMPORTSCRIPTDLG_H__F6D19EAD_3837_42DA_8B51_25564C1F61D8__INCLUDED_)
#define AFX_IMPORTSCRIPTDLG_H__F6D19EAD_3837_42DA_8B51_25564C1F61D8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ImportScriptDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImportScriptDlg dialog

class CImportScriptDlg : public CDialog
{
// Construction
public:
	BOOL ImportSpecificFile(CString strFilename);
	BOOL ImportAllFiles(CString strExtension);
	CImportScriptDlg(TDMFOBJECTSLib::IServer* pServer = NULL,CWnd* pParent = NULL);   // standard constructor

    TDMFOBJECTSLib::IServerPtr m_pServer;
// Dialog Data
	//{{AFX_DATA(CImportScriptDlg)
	enum { IDD = IDD_IMPORT_SCRIPT_FILE };
	CButton	m_BTN_Import;
    CButton m_RADIO_ALL_FILES;
    CButton m_RADIO_SPECIFIC_FILE;
	CEdit	m_Edit_Filename_Ctrl;
	CButton	m_Chk_OverwriteAll_Ctrl;
	CString	m_strFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportScriptDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportScriptDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRADIOAll();
	afx_msg void OnRadioSpecific();
	afx_msg void OnImportFile();
	afx_msg void OnChangeEditFilename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMPORTSCRIPTDLG_H__F6D19EAD_3837_42DA_8B51_25564C1F61D8__INCLUDED_)
