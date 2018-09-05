// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__C71B742D_9EDF_4EBD_88F4_48F893AF85F7__INCLUDED_)
#define AFX_MAINFRM_H__C71B742D_9EDF_4EBD_88F4_48F893AF85F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SoftekdialogBar.h"
#include "DebugMonitor.h"
#include "Messenger.h"


class CMainFrame : public CFrameWnd 
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

	void LoadConfig();
	void SaveConfig();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	int  m_nWidth;
	int  m_nHeight;
	long m_nPosX;
	long m_nPosY;

	CSplitterWnd m_wndSplitter;
	CSplitterWnd m_wndSplitter2;
	long m_nPaneLeftWidth;
	long m_nPaneRightTopHeight;
	long m_nPaneRightMiddleHeight;

	bool m_bInitialized;

	CSoftekDialogBar m_wndDlgBar;

	CStatusBar       m_wndStatusBar;
    CString			 m_strPane0;
	CString			 m_strPane1;
	CString			 m_strPane2;

	CDebugMonitor    m_DebugMonitor;

#ifdef TEST_MESSAGES
	CMessenger       m_Messenger;
#endif

public:
	std::map<std::string, std::ostrstream*> m_mapStream;

	TDMFOBJECTSLib::IReplicationGroupPtr FindReplicationGroup(int nDomainKey, int nSrvId, int nGroupNumber, BOOL bIsSource);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnDebugMonitor();
	afx_msg void OnMessenger();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateStatusBarPane0(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStatusBarPane1(CCmdUI *pCmdUI);
    afx_msg void OnUpdateStatusBarPane2(CCmdUI *pCmdUI);
	//}}AFX_MSG
	afx_msg LRESULT OnReplicationGroupStateChangeMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnServerChangeMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnServerConnectionChangeMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnReplicationGroupPerfChangeMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerPerfChangeMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollectorCommunicationStatusMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDomainAddMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDomainRemoveMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDomainModifyMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerAddMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerRemoveMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerModifyMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerBabNotOptimalMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDebugTraceMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReplicationGroupAddMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReplicationGroupRemoveMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReplicationGroupModifyMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTextMessageMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnReceivedDataCollectorStatsMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIPAdressUnknown(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__C71B742D_9EDF_4EBD_88F4_48F893AF85F7__INCLUDED_)
