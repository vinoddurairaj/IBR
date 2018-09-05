#if !defined(AFX_DEVICESPAGE_H__A08AF572_668E_4794_9AA8_5A0FC515EFA3__INCLUDED_)
#define AFX_DEVICESPAGE_H__A08AF572_668E_4794_9AA8_5A0FC515EFA3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DevicesPage.h : header file
//

#include "GroupConfig.h"


/////////////////////////////////////////////////////////////////////////////
// CDevicesPage dialog

class CDevicesPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDevicesPage)

protected:
	CGroupConfig* m_pGroupConfig;
	CString m_cstrProductName;

// Construction
public:
	CDevicesPage(CGroupConfig* pGroupConfig, LPCSTR lpcstrProductName);
	CDevicesPage() {}
	~CDevicesPage() {}

	void SetListBoxHorizontalExtent();
	BOOL Validate();
	BOOL SaveValues();

// Dialog Data
	//{{AFX_DATA(CDevicesPage)
	enum { IDD = IDD_DIALOG_DTC_DEV };
	CEdit	m_EditRemarks;
	CListBox	m_ListDevices;
	CString	m_cstrRemarks;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDevicesPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDevicesPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListDtcDevices();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonModifyDtc();
	afx_msg void OnChangeEditDtcRemarks();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEVICESPAGE_H__A08AF572_668E_4794_9AA8_5A0FC515EFA3__INCLUDED_)
