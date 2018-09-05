#if !defined(AFX_DEBUGMONITOR_H__2EDA899F_83CA_40C3_A588_6F70188C110C__INCLUDED_)
#define AFX_DEBUGMONITOR_H__2EDA899F_83CA_40C3_A588_6F70188C110C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DebugMonitor.h : header file
//

#include "TdmfCommonGuiDoc.h"

/////////////////////////////////////////////////////////////////////////////
// CDebugMonitor dialog

class CDebugMonitor : public CDialog
{
 

   TDMFOBJECTSLib::ISystemPtr m_pSystem;
// Construction
public:
	void UpdateTheMsgFromCollectorStats(TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats);

protected:
	int m_nTraceLevel;
	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CDebugMonitor(CWnd* pParent = NULL);   // standard constructor
	void AddTrace(const CString rgcstrMsg[3]);
	void SetDocument(CTdmfCommonGuiDoc* pDoc) {m_pDoc = pDoc;}
    void ContractDialog();

// Dialog Data
	//{{AFX_DATA(CDebugMonitor)
	enum { IDD = IDD_DEBUG_MONITOR };
	CButton	m_ButtonSave;
	CListCtrl	m_ListCtrlMsg;
    CString m_AliveAgents                  ;
    CString m_AliveMsgPerHour              ;
    CString	m_AliveMsgPerMin               ;
    CString	m_AgentAliveSocket             ;
    CString	m_AgentInfo                    ;
    CString	m_AgentInfoRequest             ;
    CString	m_AgentState                   ;
    CString	m_AlertData                    ;
    CString	m_Default                      ;
    CString	m_GetAgentGenConfig            ;
    CString	m_GetAllDevice                 ;
    CString	m_GetDbParams                  ;
    CString	m_GetLgConfig                  ;
    CString	m_GroupMonitoring              ;
    CString	m_GroupState                   ;
    CString	m_GuiMsg                       ;
    CString	m_MonitoringDateRegistration   ;
    CString	m_PerfConfig                   ;
    CString	m_PerfMsg                      ;
    CString	m_Registration_Key             ;
    CString	m_SetAgentGenConfig            ;
    CString	m_SetAllDevices                ;
    CString	m_SetDbParams                  ;
    CString	m_SetLgConfig                  ;
    CString	m_StatusMsg                    ;
    CString	m_TdmfCommand                  ;
	CString	m_TdmfCommonGuiRegistration    ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugMonitor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDebugMonitor)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSave();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnRadio1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGMONITOR_H__2EDA899F_83CA_40C3_A588_6F70188C110C__INCLUDED_)
