#if !defined(AFX_DTCDEVICES_H__EDA354A7_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
#define AFX_DTCDEVICES_H__EDA354A7_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DTCDevices.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDTCDevices dialog

class CDTCDevices : public CPropertyPage
{
	DECLARE_DYNCREATE(CDTCDevices)

// Construction
public:
	CDTCDevices();
	~CDTCDevices();

// Dialog Data
	//{{AFX_DATA(CDTCDevices)
	enum { IDD = IDD_DIALOG_DTC_DEV };
	CListBox	m_listDtcDevs;
	CString	m_szDtcRemarks;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDTCDevices)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	POSITION	m_pos;
	int			m_iSelSelected;

	void		clearDTCList();
	void		clearDTCRemark();
	void		clearNewDTC();

protected:
	// Generated message map functions
	//{{AFX_MSG(CDTCDevices)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonRefreshDeviceList();
	virtual BOOL OnInitDialog();
	afx_msg void OnCancelMode();
	afx_msg void OnButtonModifyDtc();
	afx_msg void OnSelchangeListDtcDevices();
	afx_msg void OnChangeEditDtcRemarks();
	afx_msg void OnUpdateEditDtcRemarks();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTCDEVICES_H__EDA354A7_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
