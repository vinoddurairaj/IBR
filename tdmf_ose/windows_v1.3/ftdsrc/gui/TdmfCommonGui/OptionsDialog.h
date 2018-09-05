#if !defined(AFX_OPTIONSDIALOG_H__B6860B3D_7BE3_4D48_84B2_2033ECF0D11C__INCLUDED_)
#define AFX_OPTIONSDIALOG_H__B6860B3D_7BE3_4D48_84B2_2033ECF0D11C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COptionsDialog dialog

class COptionsDatabasePage : public CPropertyPage
{
	DECLARE_DYNCREATE(COptionsDatabasePage)

// Construction
public:
	COptionsDatabasePage(TDMFOBJECTSLib::ISystem *pSystem = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COptionsDatabasePage)
	enum { IDD = IDD_OPTIONS_DATABASE };
	CSpinButtonCtrl	m_CleanupDelaySpin;
	CEdit	m_CleanupDelayEdit;
	CSpinButtonCtrl	m_PerfNbRecordsSpin;
	CEdit	m_PerfNbRecordsEdit;
	CSpinButtonCtrl	m_AlertNbRecordsSpin;
	CEdit	m_AlertNbRecordsEdit;
	CEdit	m_PerfDaysEdit;
	CEdit	m_AlertDaysEdit;
	CSpinButtonCtrl	m_PerfDaysSpin;
	CSpinButtonCtrl	m_AlertDaysSpin;
	CStatic	m_PerfSizeStatic;
	CStatic	m_DBSizeStatic;
	CButton	m_ClearPerfButton;
	CButton	m_ClearAlertButton;
	CStatic	m_AlertSizeStatic;
	long	m_iAlertDays;
	long	m_iPerfDays;
	long	m_iAlertNbRecords;
	long	m_iPerfNbRecords;
	long	m_iCleanupDelay;
	//}}AFX_DATA

	TDMFOBJECTSLib::ISystemPtr m_pSystem;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsDatabasePage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COptionsDatabasePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnClearAlertButton();
	afx_msg void OnClearPerfButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSDIALOG_H__B6860B3D_7BE3_4D48_84B2_2033ECF0D11C__INCLUDED_)
