#if !defined(AFX_DTCCONFIGPROPSHEET_H__EDA354A8_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
#define AFX_DTCCONFIGPROPSHEET_H__EDA354A8_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DTCConfigPropSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDTCConfigPropSheet

class CDTCConfigPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CDTCConfigPropSheet)

// Construction
public:
	CDTCConfigPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CDTCConfigPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDTCConfigPropSheet)
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDTCConfigPropSheet();

	#define BUTTONOK		1
	#define BUTTONCANCEL	2
	#define BUTTONHELP		3

	int		m_iButtonState;

	struct	structSystemValues {
		char		m_strPrimaryHostName[_MAX_PATH];
		char		m_strPStoreDev[_MAX_PATH];
		char		m_strSecondHostName[_MAX_PATH];
		char		m_strJournalDir[_MAX_PATH];
		int			m_iPortNum;
		BOOL		m_bChaining;
		char		m_strNote[_MAX_PATH];
	} m_structSystemValues;

	struct	structDTCDevValues {
		char		m_strDTCDev[_MAX_PATH];
		char		m_strRemarks[_MAX_PATH];
		char		m_strDataDev[_MAX_PATH];
		char		m_strMirrorDev[_MAX_PATH];
		int			m_iNumDTCDevices;
	} m_structDTCDevValues;
	
	struct	structThrottleValues {
		char		m_strThrottle[4 * _MAX_PATH];
		BOOL		m_bThrottleTrace;
	} m_structThrottleValues;

	struct	structTunableParamValues {
		BOOL		m_bSyncMode;
		BOOL		m_bCompression;
		BOOL		m_bStatGen;
		BOOL		m_bNetThresh;
		int			m_iDepth;
		int			m_iTimeout;
		int			m_iUpdateInterval;
		int			m_iMaxStatFileSize;
		int			m_iMaxTranRate;
		int			m_iDelayWrites;
		long int	m_liRefreshInterval;
	} m_structTunableParamsValues;

	BOOL    setSystemDlgValues();
	void	setDTCDeviceValues();
	void	setThrottlesValues();
	void	setTunableValues();
	long int	formatUpdateInterval(char *);

	// Generated message map functions
protected:
	//{{AFX_MSG(CDTCConfigPropSheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnDestroy();
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTCCONFIGPROPSHEET_H__EDA354A8_5EEA_11D3_BAF9_00C04F54F512__INCLUDED_)
