#if !defined(AFX_RGREPLICATIONPAIRSADMPAGE_H__4FDE3BBE_E14E_480A_9E4D_DFDF01D91055__INCLUDED_)
#define AFX_RGREPLICATIONPAIRSADMPAGE_H__4FDE3BBE_E14E_480A_9E4D_DFDF01D91055__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGReplicationPairsAdmPage.h : header file
//

#include <afxtempl.h>


/////////////////////////////////////////////////////////////////////////////
// CRGReplicationPairsAdmPage dialog

class CRGReplicationPairsAdmPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRGReplicationPairsAdmPage)

	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;

// Construction
public:
	CRGReplicationPairsAdmPage(TDMFOBJECTSLib::IReplicationGroup *pRG = NULL, bool bReadOnly = false);
	~CRGReplicationPairsAdmPage();

// Dialog Data
	//{{AFX_DATA(CRGReplicationPairsAdmPage)
	enum { IDD = IDD_RG_REPLICATION_PAIRS_ADM };
	CButton	m_DeleteButton;
	CButton	m_AddButton;
	CListCtrl	m_RPListCtrl;
	//}}AFX_DATA
	bool m_bPageModified;
	bool m_bReadOnly;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGReplicationPairsAdmPage)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGReplicationPairsAdmPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddButton();
	afx_msg void OnDeleteButton();
	afx_msg void OnDestroy();
	afx_msg void OnClickReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRdblclkReplicationPairList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGREPLICATIONPAIRSADMPAGE_H__4FDE3BBE_E14E_480A_9E4D_DFDF01D91055__INCLUDED_)
