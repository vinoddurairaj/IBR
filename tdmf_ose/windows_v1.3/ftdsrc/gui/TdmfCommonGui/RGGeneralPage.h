#if !defined(AFX_RGGENERALPAGE_H__000B0A4F_D61B_474E_A2CF_1B37D7A68BA7__INCLUDED_)
#define AFX_RGGENERALPAGE_H__000B0A4F_D61B_474E_A2CF_1B37D7A68BA7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGGeneralPage.h : header file
//

#include "LocationEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CRGGeneralPage dialog

class CRGGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRGGeneralPage)

	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;	
	bool                                 m_bNewItem;
    CFont*	                             m_pOriginalFont;

public:

	TDMFOBJECTSLib::IServerPtr m_pTargetServerSaved;  // Original Target
	long                       m_nGroupNbSaved;
	_bstr_t                    m_bstrNameSaved;
	_bstr_t                    m_bstrPStoreSaved;
	BOOL                       m_bPrimaryServerEditedUsedSaved;
	_bstr_t                    m_bstrPrimaryServerEditedIPSaved;
    BOOL                       m_bPrimaryServerDHCPNameUsedSaved;
    BOOL                       m_bTargetServerEditedUsedSaved;
	_bstr_t                    m_bstrTargetServerEditedIPSaved;
    BOOL                       m_bTargetServerDHCPNameUsedSaved;

// Construction
public:

	CRGGeneralPage(TDMFOBJECTSLib::IReplicationGroup *pRG = NULL, bool bNewItem = false, bool bReadOnly = false);
	~CRGGeneralPage();

	void RemoveTargetGroup();
	void AddTargetGroup(long nGroupNumber = -1);

	bool LinkIsValid(LPCSTR,BOOL bForPrimaryServer);
	bool LinkIsAnIP(LPCSTR);

// Dialog Data
	//{{AFX_DATA(CRGGeneralPage)
	enum { IDD = IDD_RG_GENERAL };
	CComboBox	m_ComboTarget;
	CComboBox	m_ComboPrimary;
	CButton	        m_ButtonServer;
    CLocationEdit	m_EditPStore;
	CEdit	        m_EditJournal;  
    CEdit	        m_EditDescription;
    CButton	        m_CheckChaining;
    CString	        m_strDescription;
	UINT	        m_nGroupNb;
	CString	        m_strJournal;
	CString	        m_strPStore;
	BOOL	        m_bChaining;
	CString	        m_cstrPStoreTitle;
	CString	m_cstrPrimaryName;
	CString	m_cstrTargetName;
	CString	m_cstrPrimaryLink;
	CString	m_cstrTargetLink;
	//}}AFX_DATA

	BOOL m_bPageModified;
    BOOL m_IsTargetServerModified;
 	BOOL m_bReadOnly;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGGeneralPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectServerButton();
	afx_msg void OnCheckChaining();
	afx_msg void OnUpdateEditDescription();
	afx_msg void OnUpdateEditGroupNumber();
	afx_msg void OnUpdateEditJournal();
	afx_msg void OnUpdateEditPstore();
	afx_msg void OnEditchangeComboPrimary();
	afx_msg void OnSelchangeComboPrimary();
	afx_msg void OnEditchangeComboTarget();
	afx_msg void OnSelchangeComboTarget();
	afx_msg void OnKillfocusComboPrimary();
	afx_msg void OnKillfocusComboTarget();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGGENERALPAGE_H__000B0A4F_D61B_474E_A2CF_1B37D7A68BA7__INCLUDED_)
