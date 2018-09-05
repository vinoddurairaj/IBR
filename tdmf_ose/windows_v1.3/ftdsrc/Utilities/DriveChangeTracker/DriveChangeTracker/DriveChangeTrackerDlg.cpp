// DriveChangeTrackerDlg.cpp : implementation file
//



#include "stdafx.h"
#include "DriveChangeTracker.h"
#include "DriveChangeTrackerDlg.h"
#include ".\drivechangetrackerdlg.h"
#include "util.h"
#include "Dbt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define INITGUID

#include "initguid.h"
#include "guiddef.h"
#include "ioevent.h"
#include "objbase.h"


// CDriveChangeTrackerDlg dialog



CDriveChangeTrackerDlg::CDriveChangeTrackerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDriveChangeTrackerDlg::IDD, pParent)
    , m_csSelectedDrive(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDriveChangeTrackerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_DRIVE_SELECT, m_ListBoxDriveNames);
    DDX_Control(pDX, IDC_START_TRACKING, m_StartButtonCtrl);
    DDX_Control(pDX, IDC_STOP_TRACKING, m_StopButtonCtrl);
    DDX_Control(pDX, IDC_LIST6, m_TrackDataCtrl);
    DDX_CBString(pDX, IDC_COMBO_DRIVE_SELECT, m_csSelectedDrive);
}

BEGIN_MESSAGE_MAP(CDriveChangeTrackerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_MESSAGE(WM_DEVICECHANGE, OnDeviceChange)
    ON_BN_CLICKED(IDC_START_TRACKING, OnBnClickedStartTracking)
    ON_BN_CLICKED(IDC_STOP_TRACKING, OnBnClickedStopTracking)
END_MESSAGE_MAP()


// CDriveChangeTrackerDlg message handlers

BOOL CDriveChangeTrackerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
    //
    // Fill out list of drives here...
    // We assume max 1024 drives
    //
#define MAX_VOLUMES 1024
    
    CString         csArray[MAX_VOLUMES]; 

    m_bTracking     = FALSE;
    m_uiNumVolumes  = 0;
    m_notifyHandle  = NULL;
    m_uiNumVolumes  = GetAllVolumes(csArray,MAX_VOLUMES);

    if (m_uiNumVolumes)
    {
        for (unsigned int i = 0; i< m_uiNumVolumes;i++)
        {
            m_ListBoxDriveNames.AddString(csArray[i]);
        }   
    }
    else
    {
        //
        // We could not find any drives on this computer!!
        //
        // bye bye now!
        //
        return FALSE;
    }

    m_ListBoxDriveNames.SetCurSel(0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDriveChangeTrackerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDriveChangeTrackerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDriveChangeTrackerDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    OnOK();
}


//REFGUID  GUID_IO_MEDIA_ARRIVAL = "d07433c0-a98e-11d2-917a-00a0c9068ff3";

LRESULT CDriveChangeTrackerDlg::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{

    // DEVICE PNP CHANGES HERE!!
    if (m_bTracking)
    {
        if (DBT_CUSTOMEVENT == wParam)
        {
            PDEV_BROADCAST_HDR pBrHeader = (PDEV_BROADCAST_HDR)lParam;

            if (DBT_DEVTYP_HANDLE == pBrHeader->dbch_devicetype)
            {
                PDEV_BROADCAST_HANDLE pBrHandle = (PDEV_BROADCAST_HANDLE)lParam;

                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_MEDIA_ARRIVAL ))
                    InsertStringWithTime("Event: GUID_IO_MEDIA_ARRIVAL");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_MEDIA_ARRIVAL");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_MEDIA_REMOVAL ))
                    InsertStringWithTime("Event: GUID_IO_MEDIA_REMOVAL");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_MEDIA_REMOVAL");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_CHANGE ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_CHANGE");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_CHANGE");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_DISMOUNT ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_DISMOUNT");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_DISMOUNT");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_DISMOUNT_FAILED ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_DISMOUNT_FAILED");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_DISMOUNT_FAILED");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_LOCK ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_LOCK");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_LOCK");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_LOCK_FAILED ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_LOCK_FAILED");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_LOCK_FAILED");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_MOUNT ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_MOUNT");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_MOUNT");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_NAME_CHANGE ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_NAME_CHANGE");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_NAME_CHANGE");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE");
                if ( IsEqualGUID (pBrHandle->dbch_eventguid,GUID_IO_VOLUME_UNLOCK ))
                    InsertStringWithTime("Event: GUID_IO_VOLUME_UNLOCK");
                    //m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),"Event: GUID_IO_VOLUME_UNLOCK");
            }
        }
    }

    return 0;
}

void CDriveChangeTrackerDlg::OnBnClickedStartTracking()
{
    CString TempString;

    //
    // Check if there are volumes listed!
    //
    if (!m_uiNumVolumes)
    {
        //
        // No volumes detected! byebye!
        //
        return;        
    }
    UpdateData(TRUE);

    if (OpenTrackedDevice())
    {
        TempString = _T("Tracking Drive:") + m_csSelectedDrive;
    }
    else
    {
        char * cpBuf = TempString.GetBuffer(MAX_PATH);
        sprintf(cpBuf,"Unable to track Drive:%s Error:[0x%08x]",(LPCTSTR)m_csSelectedDrive,GetLastError());
        TempString.ReleaseBuffer();
    }

    //
    // Display result
    //
    m_TrackDataCtrl.AddString(TempString);

    m_ListBoxDriveNames.EnableWindow(FALSE);
    m_StartButtonCtrl.EnableWindow(FALSE);
    m_StopButtonCtrl.EnableWindow(TRUE);
    m_TrackDataCtrl.EnableWindow(TRUE);
    m_bTracking = TRUE;
    UpdateData(FALSE);
}

void CDriveChangeTrackerDlg::OnBnClickedStopTracking()
{
    CloseTrackedDevice();
    m_ListBoxDriveNames.EnableWindow(TRUE);
    m_StartButtonCtrl.EnableWindow(TRUE);
    m_StopButtonCtrl.EnableWindow(FALSE);
    m_TrackDataCtrl.EnableWindow(FALSE);
    m_bTracking = FALSE;
    UpdateData(FALSE);
}

BOOL CDriveChangeTrackerDlg::OpenTrackedDevice(void)
{
    DEV_BROADCAST_HANDLE    hHdr;
    char                    szVolumeName[8];       

    //
    // Only valid drives are supported for now!
    // don't currently support mountpoints...
    //
    if (        _T("") == m_csSelectedDrive
        ||  (   m_csSelectedDrive.GetLength() > 3 )   )
    {
        return FALSE;
    }

    sprintf(szVolumeName, TEXT("\\\\.\\%c:"), m_csSelectedDrive.GetAt(0));

    m_volumeHandle =    CreateFile( 
                            szVolumeName,
                            0, // desired access = 0 => query only
                            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

    if (INVALID_HANDLE_VALUE == m_volumeHandle)
    {
        return FALSE;
    }

    memset(&hHdr, 0, sizeof(DEV_BROADCAST_HANDLE));
    hHdr.dbch_size          = sizeof(DEV_BROADCAST_HANDLE);
    hHdr.dbch_handle        = m_volumeHandle;
    hHdr.dbch_devicetype    = DBT_DEVTYP_HANDLE;
    m_notifyHandle          = RegisterDeviceNotification(m_hWnd, &hHdr,DEVICE_NOTIFY_WINDOW_HANDLE);

    if (        m_notifyHandle
        &&  (INVALID_HANDLE_VALUE != m_notifyHandle))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void CDriveChangeTrackerDlg::CloseTrackedDevice(void)
{
    if (        m_notifyHandle
        &&  (INVALID_HANDLE_VALUE != m_notifyHandle)    )
    {
        UnregisterDeviceNotification(m_notifyHandle);
        m_notifyHandle = INVALID_HANDLE_VALUE;
    }
}

void CDriveChangeTrackerDlg::InsertStringWithTime(LPSTR String)
{
    CTime   cTempTime(time(0));
    CString time_string = COleDateTime(cTempTime.GetTime()).Format(_T("%H:%M:%S"));

    time_string = time_string + _T(" ") + String;
    
    m_TrackDataCtrl.InsertString(m_TrackDataCtrl.GetCount(),time_string );
}