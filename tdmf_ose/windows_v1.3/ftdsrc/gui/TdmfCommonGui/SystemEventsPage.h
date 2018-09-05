#if !defined(AFX_SYSTEMEVENTSPAGE_H__58A7EBE1_2875_48AB_BBA1_6E3229D48384__INCLUDED_)
#define AFX_SYSTEMEVENTSPAGE_H__58A7EBE1_2875_48AB_BBA1_6E3229D48384__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemEventsPage.h : header file
//

#include "PropertyPageBase.h"
#include "ColumnSelectionDlg.h"


class CEventProperties;


/////////////////////////////////////////////////////////////////////////////
// CSystemEventsPage dialog

class CSystemEventsPage : public CPropertyPage, public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSystemEventsPage)

protected:
	BOOL                                 m_bEventListInitialized;
	long								 m_nEvent;   // Nb events in Db
    CImageList*                          m_pImageList;
	int                                  m_nTimerId;
	long                                 m_nRefreshFreq;
	bool                                 m_bInit;
	CEventProperties*                    m_pEventProperties;

// Construction
public:
	CSystemEventsPage();
	~CSystemEventsPage();

	std::vector<CListViewColumnDef> m_vecColumnDef;

	TDMFOBJECTSLib::IEventPtr GetEventAt    ( long pnIndex );
	long                      GetEventCount ();
	void                      EventInit     ();

	void PopupListViewContextMenu          ( CPoint pt, int nColumn );
	void AddColumn                         ( int nIndex, char* szName, UINT nWidth );
	virtual void SaveColumnDef             ();
	virtual void LoadColumnDef             ();

// Dialog Data
	//{{AFX_DATA(CSystemEventsPage)
	enum { IDD = IDD_SYSTEM_EVENTS };
	CListCtrl	m_ListCtrlEvent;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSystemEventsPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
	afx_msg LRESULT OnEventSelectionChange(WPARAM, LPARAM);


	BOOL CaptureTabKey() {return TRUE;}

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSystemEventsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnGetdispinfoList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOdfinditemList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMEVENTSPAGE_H__58A7EBE1_2875_48AB_BBA1_6E3229D48384__INCLUDED_)
