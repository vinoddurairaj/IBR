// MonitorToolDlg.h : header file
//

#if !defined(AFX_MONITORTOOLDLG_H__ABAE1597_9EBD_11D3_AB67_000000000000__INCLUDED_)
#define AFX_MONITORTOOLDLG_H__ABAE1597_9EBD_11D3_AB67_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ftd_perf.h"
#include "sock.h"
#include "ftd.h"
#include "ETSLayout.h"

#include "..\..\lib\libResMgr\ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////
// CMonitorToolDlg dialog

class CMonitorToolDlg : public ETSLayoutDialog
{
// Construction
public:
	CMonitorToolDlg(CWnd* pParent = NULL);	// standard constructor
	
	DECLARE_LAYOUT();

// Dialog Data
	//{{AFX_DATA(CMonitorToolDlg)
	enum { IDD = IDD_MONITORTOOL_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMonitorToolDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

public:
	#define REG_PATH "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
	#define REG_KEY "InstallPath"
	#define REG_KEY_UPDATE_INTERVAL	"MonitorUpdateInterval"
	#define REG_KEY_COLUMN_ZERO "MonColumnZero"
	#define REG_KEY_COLUMN_ONE "MonColumnOne"
	#define REG_KEY_COLUMN_TWO "MonColumnTwo"
	#define REG_KEY_COLUMN_THREE "MonColumnThree"
	#define REG_KEY_COLUMN_FOUR "MonColumnFour"
	#define REG_KEY_COLUMN_FIVE "MonColumnFive"
	#define REG_KEY_COLUMN_SIX "MonColumnSix"
	#define REG_KEY_COLUMN_SEVEN "MonColumnSeven"
	#define REG_KEY_COLUMN_EIGHT "MonColumnEight"

//	#define FTD_MODE_PASSTHRU  16
//	#define FTD_MODE_NORMAL    1
//	#define FTD_MODE_TRACKING  2
//	#define FTD_MODE_REFRESH   3
//	#define FTD_MODE_BACKFRESH 32

	/* the first stab at logical group states */

	#define FTD_M_JNLUPDATE     0x01
	#define FTD_M_BITUPDATE     0x02
	#define FTD_MODE_PASSTHRU   0x10
	#define FTD_MODE_NORMAL     FTD_M_JNLUPDATE
	#define FTD_MODE_TRACKING   FTD_M_BITUPDATE
	#define FTD_MODE_REFRESH    (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)
	#define FTD_MODE_BACKFRESH  0x20
	#define FTD_MODE_FULLREFRESH    (0x40 | FTD_M_BITUPDATE)    //used only in Unix
	#define FTD_MODE_STARTED    0x100
	#define FTD_MODE_CHECKPOINT 0x200


	#define GREY	0
	#define GREEN	1
	#define YELLOW	2
	#define RED		3

	CResourceManager m_ResourceManager;
	TCHAR* productName;
	TCHAR* fullProductName;
	TCHAR* appName;

	HANDLE				m_hLog;
	CListBox			*m_pErrorsListBox;
	CListBox			*m_pGroupsListBox;
	CListCtrl			*m_pDevicesListCtrl;
	CComboBox			*m_pUpdateIntervalCtrl;
	CWinThread			*m_hQuerieThread;
	HANDLE				m_QueryEvent;
	char				m_szGroup[3];
	ftd_perf_instance_t	m_DeviceInstanceData[100];
	ftd_perf_instance_t	m_PrevDeviceInstanceData[100];
	int					m_iIndex;
	CImageList			m_ImageList;
	time_t				m_iCurrentTime, m_iLastUpdateTime, m_iTimeDiff;
	int					m_iUpdateInterval;
	DWORD				m_dwNumEvents;
//	char				*m_szNewestEvent;
	char				m_szNewestEvent[2 * 1024];
	int					m_GetPerf;
	ftd_perf_t			*m_perfp;

	int					m_iConnectionPriority;
	int					m_iPercentPriority;
	int					m_iReadPriority;
	int					m_iWritePriority;
	int					m_iActualPriority;
	int					m_iEffectivePriority;
	int					m_iEntriesPriority;
	int					m_iBabPriority;

	int					m_iColWidth0;
	int					m_iColWidth1;
	int					m_iColWidth2;
	int					m_iColWidth3;
	int					m_iColWidth4;
	int					m_iColWidth5;
	int					m_iColWidth6;
	int					m_iColWidth7;
	int					m_iColWidth8;

	void	readEventLog();
	void	fillErrorMessageListBox(LPTSTR	StringData);
	void	setDlgMemberVariables();
	void	startThreadToDoTimedQueries();
	void	getConfigPath(char *strPath);
	void	initLogicalGroupList();
	void	addGroupToGroupList(int	iGroupID);
	void	initDeviceListCtrl();
	void	doPerformanceData();
	int		createPerf();
	void	destroyPerf();
	int		getDataPerf();
	void	fillDeviceListCtrl();
	void	loadImageList();
	int		getIconColor(int iColumn);
	void	changeIconColor(int iColor, int iColumn);
	void	changeIconColor(int iColor, int iColumn, int check);
	int		getDeviceIconColor(int iColumn);
	void	changeDeviceIconColor(int iColor, int iColumn);
	void	changeDeviceIconColor(int iColor, int iColumn, int check);
	void	initUpdateInterval();
	void	setRegUpdateInterval();
	void	getRegUpdateInterval();
	void	getRegColumnWidth();
	void	setRegColumnWidth();
	void	setColumnWidth();
	void	getColumnWidth();
	
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMonitorToolDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeComboGroups();
	afx_msg void OnDblclkListErrors();
	virtual void OnOK();
	afx_msg void OnButtonConfigTool();
	afx_msg void OnButtonUpdateNow();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelchangeListUpdateInterval();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int		m_testServiceConnect();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MONITORTOOLDLG_H__ABAE1597_9EBD_11D3_AB67_000000000000__INCLUDED_)
