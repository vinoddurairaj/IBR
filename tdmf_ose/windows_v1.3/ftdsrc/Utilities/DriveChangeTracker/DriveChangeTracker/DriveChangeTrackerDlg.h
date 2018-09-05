// DriveChangeTrackerDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CDriveChangeTrackerDlg dialog
class CDriveChangeTrackerDlg : public CDialog
{
// Construction
public:
	CDriveChangeTrackerDlg(CWnd* pParent = NULL);	// standard constructor
    
    LRESULT CDriveChangeTrackerDlg::OnDeviceChange(WPARAM wParam, LPARAM lParam);

// Dialog Data
	enum { IDD = IDD_DRIVECHANGETRACKER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
private:
    BOOL CDriveChangeTrackerDlg::OpenTrackedDevice(void);
    void CDriveChangeTrackerDlg::CloseTrackedDevice(void);
    void CDriveChangeTrackerDlg::InsertStringWithTime(LPSTR String);



// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    // Disk selection combo box
    CComboBox m_ListBoxDriveNames;
    afx_msg void OnBnClickedStartTracking();
private:
    unsigned int    m_uiNumVolumes;
    BOOL            m_bTracking;
    HANDLE          m_notifyHandle;
    HANDLE          m_volumeHandle;
public:
    CButton m_StartButtonCtrl;
    CButton m_StopButtonCtrl;
    afx_msg void OnBnClickedStopTracking();
    CListBox m_TrackDataCtrl;
    CString m_csSelectedDrive;
};
