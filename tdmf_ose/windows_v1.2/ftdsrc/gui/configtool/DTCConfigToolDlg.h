// DTCConfigToolDlg.h : header file
//

#if !defined(AFX_DTCCONFIGTOOLDLG_H__E0E9AFF9_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
#define AFX_DTCCONFIGTOOLDLG_H__E0E9AFF9_557D_11D3_BAF7_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined(_OCTLIC)
#include "LicAPI.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CDTCConfigToolDlg dialog

class CDTCConfigToolDlg : public CDialog
{

// Construction
public:
	CDTCConfigToolDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDTCConfigToolDlg)
	enum { IDD = IDD_DTCCONFIGTOOL_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDTCConfigToolDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	int		m_iBABSize;
	int		m_iBABSizeOrig;
	int		m_iTCPWinSize;
	int		m_iPortNum;
	int		m_iGroupID;
	char	m_strGroupNote[_MAX_PATH];
	char	m_strPath[_MAX_PATH];
	char	m_strCfgFileInUse[_MAX_PATH];
	int		m_iButtonState;
	BOOL	bDeletedGroup;
	BOOL	m_bPropSheetInit;
	BOOL	m_bAppStarted;
	CWnd	*m_cWndAppStarted;
	char	m_szDate[_MAX_PATH];
	int		m_iOEM;

	CComboBox		*m_pListGroup;
	CEdit			*m_pEditBABSize;
	CEdit			*m_pEditTCPWinSize;
	CEdit			*m_pEditPort;
	CEdit			*m_pEditNote;
	CSpinButtonCtrl	*m_pSpinBABSize;

	~CDTCConfigToolDlg        ();
	void initDlgCtrls         ();
	void initPropSheet        ();
	void initBABSize          ();
	void initTCPWinSize       ();
	void initPortNum          ();
	void initLogicalGroupList ();
	int  getBABSize           ();
	int  getTCPWinSize        ();
	int  getPortNum           ();
	void writeInfoToConfig    ();
	void displayNote          ();
	void addGroupToGroupList  ();
	void deleteGroupFromList  ();
	void createNewCfgFile     ();
	void getGroupNote         (char strGroupNote[_MAX_PATH]);
	void getCfgFileInUse      (char strFile[_MAX_PATH]);
	void shutDownSystem       ();
	int	 initLicenseKey       (char *szDate);
	BOOL CtDlgCheckDir        (); // ardev 020926
	BOOL CtCreateDir          ( char* pcpPath );
	
protected:
	HICON m_hIcon;


	// Generated message map functions
	//{{AFX_MSG(CDTCConfigToolDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnButtonAddGroup();
	afx_msg void OnButtonModifyGroup();
	afx_msg void OnButtonDeleteGroup();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSelchangeListGroups();
	afx_msg void OnChangeEditBabSize();
	afx_msg void OnUpdateEditBabSize();
	afx_msg void OnEditchangeListGroups();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:

    BOOL babSizeOk(int iBab);
	int getMaxBabSizeMb();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTCCONFIGTOOLDLG_H__E0E9AFF9_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
