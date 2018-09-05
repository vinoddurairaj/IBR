#if !defined(AFX_RGSELECTDEVICESDIALOG_H__57DDC57A_C3D9_4CFD_B041_39FAA06A0A96__INCLUDED_)
#define AFX_RGSELECTDEVICESDIALOG_H__57DDC57A_C3D9_4CFD_B041_39FAA06A0A96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tdmfcommonguiDoc.h"
#include "ReplicationGroupPropertySheet.h"

// RGSelectDevicesDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRGSelectDevicesDialog dialog

class CRGSelectDevicesDialog : public CDialog
{
	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;
	CTdmfCommonGuiDoc*                   m_pDoc;

	void RefreshValues();
	BOOL IsLocalTarget(TDMFOBJECTSLib::IDevicePtr pDevice);
	BOOL IsSourceOrTargetOnTargetServer(TDMFOBJECTSLib::IDevicePtr pDevice);
	BOOL IsAlreadySourceInGroup(TDMFOBJECTSLib::IDevicePtr pDevice);
	BOOL IsSymmetricPair();
    BOOL OnReadWindowSizeFromRegistry(CRect& rect); 
    BOOL OnWriteWindowSizeToRegistry(); 

// Construction
public:
	CRGSelectDevicesDialog(TDMFOBJECTSLib::IReplicationGroup *pRG,
						   CTdmfCommonGuiDoc* pDoc, CReplicationGroupPropertySheet* rgps,
						   CWnd* pParent = NULL, CString* pRPStr = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRGSelectDevicesDialog)
	enum { IDD = IDD_RG_SELECT_DEVICES };
	CButton	m_RefreshButton;
	CStatic	m_SecondaryStatic;
	CStatic	m_PrimaryStatic;
	CStatic	m_DescriptionStatic;
	CButton	m_BorderButton;
	CButton	m_OKButton;
	CButton	m_CancelButton;
	CEdit	m_DescriptionEdit;
	CListCtrl	m_SecondaryListBox;
	CListCtrl	m_PrimaryListBox;
	CString	m_strDescription;
	//}}AFX_DATA

	CReplicationGroupPropertySheet* m_pRGPropertySheet;
	CString* m_pRPStr;
	CString m_sSource;
	CString m_sTarget;

	TDMFOBJECTSLib::IDevicePtr m_pDeviceSource;
	TDMFOBJECTSLib::IDevicePtr m_pDeviceTarget;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRGSelectDevicesDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRGSelectDevicesDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnColumnclickPrimaryList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickSecondaryList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnRefreshDevices();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGSELECTDEVICESDIALOG_H__57DDC57A_C3D9_4CFD_B041_39FAA06A0A96__INCLUDED_)
