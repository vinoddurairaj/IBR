#if !defined(AFX_TUNABLEPARAMS_H__EDA354A5_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
#define AFX_TUNABLEPARAMS_H__EDA354A5_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TunableParams.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTunableParams dialog

class CTunableParams : public CPropertyPage
{
	DECLARE_DYNCREATE(CTunableParams)

// Construction
public:
	CTunableParams();
	~CTunableParams();

// Dialog Data
	//{{AFX_DATA(CTunableParams)
	enum { IDD = IDD_DIALOG_TUNABLE_PARAMS };
	CEdit	m_editTimeoutInt;
	CButton	m_checkNeverTimeout;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTunableParams)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	void	formatRefreshInterval(long int liRefreshInt, char *strFormattedData);

protected:
	// Generated message map functions
	//{{AFX_MSG(CTunableParams)
	afx_msg void OnCheckSyncMode();
	afx_msg void OnCheckStatGen();
	afx_msg void OnCheckUsageThreshold();
	virtual BOOL OnInitDialog();
	afx_msg void OnCancelMode();
	afx_msg void OnCheckNeverTimeout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TUNABLEPARAMS_H__EDA354A5_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
