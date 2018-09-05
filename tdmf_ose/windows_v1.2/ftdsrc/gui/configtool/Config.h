#if !defined(AFX_CONFIG_H__D5047A13_5AFE_11D3_BAF8_00C04F54F512__INCLUDED_)
#define AFX_CONFIG_H__D5047A13_5AFE_11D3_BAF8_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Config.h : header file
//

#include "DTCConfigPropSheet.h"
#include <afxtempl.h>

#include	"sock.h"

/////////////////////////////////////////////////////////////////////////////
// CConfig window

class CConfig : public CWnd
{
// Construction
public:
	CConfig();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfig)
	//}}AFX_VIRTUAL

// Implementation
public:
	#define REG_PATH "Software\\" OEMNAME "\\" PRODUCTNAME "\\CurrentVersion"
	#define REG_KEY "InstallPath"
	#define REG_PARAM_PATH "SYSTEM\\CurrentControlSet\\Services\\" DRIVERNAME "\\Parameters"
	#define REG_KEY_BAB "num_chunks"
	#define REG_KEY_MAXMEM "maxmem"
	#define REG_KEY_TCP "tcp_window_size"
	#define REG_KEY_PORT "port"
	
	struct structDevCfg {
		int		iBABSize;
		int		iTCPWinSize;
		int		iPortNum;
		int		iGroupID;
		char	strGroupNote[_MAX_PATH];
		char	strPath[_MAX_PATH];
		char	strCfgFileInUse[_MAX_PATH];
	} m_structDevCfg;

	struct structDevCfg m_structDevCfgCpy; //  ardeb 020913

	struct	structSystemValues {
		char		m_strPrimaryHostName[_MAX_PATH];
		char		m_strPStoreDev[_MAX_PATH];
		char		m_strSecondHostName[_MAX_PATH];
		char		m_strJournalDir[_MAX_PATH];
		int			m_iPortNum;
		BOOL		m_bChaining;
		char		m_strNote[_MAX_PATH];
	} m_structSystemValues;

	struct structSystemValues m_structSystemValuesCpy; //  ardeb 020913

	struct	structDTCDevValues {
		char		m_strDTCDev    [_MAX_PATH];
		char		m_strRemarks   [_MAX_PATH];
		char		m_strDataDev   [_MAX_PATH];
		char		m_strMirrorDev [_MAX_PATH];
		char		m_strPri3Val   [_MAX_PATH]; // ardeb 020913
		char		m_strSec3Val   [_MAX_PATH]; // ardeb 020913
		int			m_iNumDTCDevices;
	} m_structDTCDevValues;

	// See m_listStructDTCDevValuesCpy ardeb 020913

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

	int		m_iMirrorDevIndex;
	char	m_szDeviceListInfo    [100] [_MAX_PATH];
	int		m_iMirrorIndexArray   [500];
	char	m_szCurrentDevices    [500];
	int		m_iModDeleteSel;
	int		m_iMirrorDev;

	CDTCConfigPropSheet *m_sheetConfig;
	BOOL	m_bAddFileFlag;
	CList	<struct structDTCDevValues, struct structDTCDevValues>m_listStructDTCDevValues;
	CList	<struct structDTCDevValues, struct structDTCDevValues>m_listStructDTCDevValuesCpy;

	virtual ~CConfig             ();
	void	getConfigPath        (char *strPath);
	void	setDevCfg            (
				int iBABSize, int	iTCPWinSize, int iPortNum,
				int	iGroupID, char strGroupNote[_MAX_PATH],
				char strPath[_MAX_PATH], char strCfgFileInUse[_MAX_PATH]
				);
	void	setPropSheetValues   (CDTCConfigPropSheet *sheetConfig);
	int		readConfigFile       ();
	void	writeConfigFile      ();
	void	BakInitVal           (); // ardeb 020913
	BOOL    IsValChanged         (); // ardeb 020913
	BOOL    Is3ValValid          (); // ardeb 020913
	void	setSysDlgValues      ();
	void	setDTCDeviceValues   ();
	void	setThrottleValues    ();
	void	setTunableParams     ();
	void	parseFile            (char *pdest, char strReadValue[_MAX_PATH]);
	void	readSysValues        (CFile *file, int iFileSize);
	void	readDTCDevices       (CFile *file, int iFileSize);
	void	readThrottleInfo     (CFile *file, int iFileSize);
	void	readNote             (char *strPath, char *strGroupID, char *strNote);
	int		getRegBabSize        (DWORD *iBabSize);
	void	getRegTCPWinSize     (DWORD *iTCPWinSize);
	void	getRegPort           (DWORD *iPort);
	void	setTunablesInPStore  ();
	void	getTunablesFromPStore();
	void	setStartGroup        ();
	void	waitForProcessEnd    (PROCESS_INFORMATION *ProcessInfo);
	void	stopGroup            ();
	unsigned long	gethostid    (char *szDir);
	void	parseIPString(char *szIP, int *iIP1, int *iIP2, int *iIP3, int *iIP4);


	sock_t *sockp;
	sock_t *listener;
	char	m_szRemoteDevices[500];
	void	setSocketConnection(char *szReceiveLine, int iRecBufferSize);
	void	disconnectSocket();
	void	updateCurrentmirror();

	// Generated message map functions
protected:
	//{{AFX_MSG(CConfig)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIG_H__D5047A13_5AFE_11D3_BAF8_00C04F54F512__INCLUDED_)
