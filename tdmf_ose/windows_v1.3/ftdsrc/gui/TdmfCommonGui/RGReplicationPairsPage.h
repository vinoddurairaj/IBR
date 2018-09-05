#if !defined(AFX_RGREPLICATIONPAIRSPAGE_H__6609AC72_5104_4AF6_BC44_7B51E0A84028__INCLUDED_)
#define AFX_RGREPLICATIONPAIRSPAGE_H__6609AC72_5104_4AF6_BC44_7B51E0A84028__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGReplicationPairsPage.h : header file
//

#include "PropertyPageBase.h"
#include "ColumnSelectionDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CRGReplicationPairsPage dialog

class CRGReplicationPairsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRGReplicationPairsPage)

protected:
	BOOL m_bInUpdate;
	BOOL m_bWindows;
	BOOL m_bLinux; 

// Construction
public:
	CRGReplicationPairsPage();
	~CRGReplicationPairsPage();

	void RefreshValues();
	int  SetNewItem(int nIndex, TDMFOBJECTSLib::IReplicationPair* pRP, bool bInsert = true);
	BOOL SelectPair(TDMFOBJECTSLib::IReplicationPair* pRP);

	std::vector<CListViewColumnDef> m_vecColumnDef;
	void PopupListViewContextMenu(CPoint pt, int nColumn);
	void AddColumn(int nIndex, char* szName, UINT nWidth);
	void SaveColumnDef();
	void LoadColumnDef();

// Dialog Data
	//{{AFX_DATA(CRGReplicationPairsPage)
	enum { IDD = IDD_RG_REPLICATION_PAIRS };
	CListCtrl	m_ListCtrlReplicationPair;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGReplicationPairsPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

	BOOL CaptureTabKey() {return TRUE;}

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGReplicationPairsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDeleteitemListRp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnItemchangedListRp(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGREPLICATIONPAIRSPAGE_H__6609AC72_5104_4AF6_BC44_7B51E0A84028__INCLUDED_)
