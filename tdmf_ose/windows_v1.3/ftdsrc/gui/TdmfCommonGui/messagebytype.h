#if !defined(AFX_MESSAGEBYTYPE_H__33907401_9C63_4051_A51C_0EF16DE0F6B0__INCLUDED_)
#define AFX_MESSAGEBYTYPE_H__33907401_9C63_4051_A51C_0EF16DE0F6B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MessageByType.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMessageByTypeDlg dialog

class CMessageByTypeDlg : public CDialog
{

   TDMFOBJECTSLib::ISystemPtr m_pSystem;
// Construction
public:
	void UpdateTheMsgFromCollectorStats(TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats);
	CMessageByTypeDlg(TDMFOBJECTSLib::ISystem* pSystem,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMessageByTypeDlg)
	enum { IDD = IDD_MESSAGE_BY_TYPE }     ;  
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
	//{{AFX_VIRTUAL(CMessageByTypeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMessageByTypeDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MESSAGEBYTYPE_H__33907401_9C63_4051_A51C_0EF16DE0F6B0__INCLUDED_)
