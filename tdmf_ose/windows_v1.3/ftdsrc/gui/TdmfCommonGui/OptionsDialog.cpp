// OptionsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "OptionsDialog.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//

struct DeleteTableWorkingThreadParams
{
	IStream* pStream;
	enum TDMFOBJECTSLib::tagTdmfDBTable eTable;
};

void DeleteTableWorkingThread(void* pVoid)
{
	CoInitialize(NULL);

	struct DeleteTableWorkingThreadParams* pParams = (struct DeleteTableWorkingThreadParams*) pVoid;
		
	// Retrieve the interface pointer within your thread routine
	TDMFOBJECTSLib::ISystemPtr pSystem;
	if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pParams->pStream, TDMFOBJECTSLib::IID_ISystem, (LPVOID*)&pSystem)))
	{
		pSystem->DeleteTableRecords(pParams->eTable);
	}
	
	CoUninitialize();
}


/////////////////////////////////////////////////////////////////////////////
// COptionsDatabasePage dialog

IMPLEMENT_DYNCREATE(COptionsDatabasePage, CPropertyPage)

COptionsDatabasePage::COptionsDatabasePage(TDMFOBJECTSLib::ISystem *pSystem)
	: CPropertyPage(COptionsDatabasePage::IDD), m_pSystem(pSystem)
{
	//{{AFX_DATA_INIT(COptionsDatabasePage)
	m_iAlertDays      = 0;
	m_iPerfDays       = 0;
	m_iAlertNbRecords = 0;
	m_iPerfNbRecords  = 0;
	m_iCleanupDelay   = 0;
	//}}AFX_DATA_INIT
}


void COptionsDatabasePage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDatabasePage)
	DDX_Control(pDX, IDC_CLEANUP_DELAY_SPIN, m_CleanupDelaySpin);
	DDX_Control(pDX, IDC_CLEANUP_DELAY_EDIT, m_CleanupDelayEdit);
	DDX_Control(pDX, IDC_PERF_NB_SPIN, m_PerfNbRecordsSpin);
	DDX_Control(pDX, IDC_PERF_NB_EDIT, m_PerfNbRecordsEdit);
	DDX_Control(pDX, IDC_ALERT_NB_SPIN, m_AlertNbRecordsSpin);
	DDX_Control(pDX, IDC_ALERT_NB_EDIT, m_AlertNbRecordsEdit);
	DDX_Control(pDX, IDC_PERF_DAYS_EDIT, m_PerfDaysEdit);
	DDX_Control(pDX, IDC_ALERT_DAYS_EDIT, m_AlertDaysEdit);
	DDX_Control(pDX, IDC_PERF_DAYS_SPIN, m_PerfDaysSpin);
	DDX_Control(pDX, IDC_ALERT_DAYS_SPIN, m_AlertDaysSpin);
	DDX_Control(pDX, IDC_PERF_SIZE_STATIC, m_PerfSizeStatic);
	DDX_Control(pDX, IDC_DB_SIZE_STATIC, m_DBSizeStatic);
	DDX_Control(pDX, IDC_CLEAR_PERF_BUTTON, m_ClearPerfButton);
	DDX_Control(pDX, IDC_CLEAR_ALERT_BUTTON, m_ClearAlertButton);
	DDX_Control(pDX, IDC_ALERT_SIZE_STATIC, m_AlertSizeStatic);
	DDX_Text(pDX, IDC_ALERT_DAYS_EDIT, m_iAlertDays);
	DDX_Text(pDX, IDC_PERF_DAYS_EDIT, m_iPerfDays);
	DDX_Text(pDX, IDC_ALERT_NB_EDIT, m_iAlertNbRecords);
	DDX_Text(pDX, IDC_PERF_NB_EDIT, m_iPerfNbRecords);
	DDX_Text(pDX, IDC_CLEANUP_DELAY_EDIT, m_iCleanupDelay);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDatabasePage, CPropertyPage)
	//{{AFX_MSG_MAP(COptionsDatabasePage)
	ON_BN_CLICKED(IDC_CLEAR_ALERT_BUTTON, OnClearAlertButton)
	ON_BN_CLICKED(IDC_CLEAR_PERF_BUTTON, OnClearPerfButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDatabasePage message handlers

BOOL COptionsDatabasePage::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CWaitCursor WaitCursor;

	m_AlertDaysSpin.SetRange(0, 99);
	m_AlertNbRecordsSpin.SetRange32(0, LONG_MAX);
	m_PerfDaysSpin.SetRange(0, 99);
	m_PerfNbRecordsSpin.SetRange32(0, LONG_MAX);
	m_CleanupDelaySpin.SetRange(0, 9999);
	m_AlertDaysEdit.SetLimitText(2);
	m_AlertNbRecordsEdit.SetLimitText(10);
	m_PerfDaysEdit.SetLimitText(2);
	m_PerfNbRecordsEdit.SetLimitText(10);
	m_CleanupDelayEdit.SetLimitText(4);

	long    nNbRows = 0;
	CString cstrSize;

	CString str = "Size :  ";
	str += (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_ALL_TABLES, &nNbRows);
	m_DBSizeStatic.SetWindowText((LPCTSTR)str);

	cstrSize = (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_ALERT_TABLE, &nNbRows);
	str.Format("Size :  %s    (%d rows)", cstrSize, nNbRows);
	m_AlertSizeStatic.SetWindowText((LPCTSTR)str);

	cstrSize = (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_PERFORMANCE_TABLE, &nNbRows);
	str.Format("Size :  %s    (%d rows)", cstrSize, nNbRows);		
	m_PerfSizeStatic.SetWindowText((LPCTSTR)str);

	m_iCleanupDelay   = m_pSystem->GetDeleteDelay();
	m_pSystem->GetDeleteRecords(TDMFOBJECTSLib::TDMF_DB_ALERT_TABLE, &m_iAlertDays, &m_iAlertNbRecords);
	m_pSystem->GetDeleteRecords(TDMFOBJECTSLib::TDMF_DB_PERFORMANCE_TABLE, &m_iPerfDays, &m_iPerfNbRecords);
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void COptionsDatabasePage::OnClearAlertButton() 
{
	int result = MessageBox("Are you sure you want to delete all the\nAlert table records from the database?",
		"Delete Records", MB_YESNO | MB_ICONQUESTION);

	if(result == IDYES)
	{
		struct DeleteTableWorkingThreadParams Params;
		if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(TDMFOBJECTSLib::IID_ISystem, (LPUNKNOWN)m_pSystem, &Params.pStream)))
		{
			Params.eTable  = TDMFOBJECTSLib::TDMF_DB_ALERT_TABLE;
			if (_beginthread(DeleteTableWorkingThread, 0, (void*)&Params) == -1)
			{
				// Error
				MessageBox("Unable to start \"Delete Table\" thread.", "Error", MB_OK | MB_ICONINFORMATION);
			}
		}
		else
		{
			AfxMessageBox("Unexpected Error: 999", MB_OK | MB_ICONINFORMATION);
		}
	
		//CString str = "Size :  ";
		//str += (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_ALERT_TABLE);
		
		//m_AlertSizeStatic.SetWindowText((LPCTSTR)str);

		//str = "Size :  ";
		//str += (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_ALL_TABLES);
		
		//m_DBSizeStatic.SetWindowText((LPCTSTR)str);

		//UpdateData(FALSE);

		MessageBox("Due to the database manager, the table records changes are not going to affect the table\nsizes immediately."
			"  The new table sizes will be available between 10 and 30 seconds.\n\nTo refresh the table sizes, click on "
			"'Clear Table' again after the elapsed time.",
			"Table Size Changes", MB_OK | MB_ICONINFORMATION);
	}
}

void COptionsDatabasePage::OnClearPerfButton() 
{
	int result = MessageBox("     Are you sure you want to delete all the\nPerformance table records from the database?",
		"Delete Records", MB_YESNO | MB_ICONQUESTION);

	if(result == IDYES)
	{
		struct DeleteTableWorkingThreadParams Params;
		if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(TDMFOBJECTSLib::IID_ISystem, (LPUNKNOWN)m_pSystem, &Params.pStream)))
		{
			Params.eTable  = TDMFOBJECTSLib::TDMF_DB_PERFORMANCE_TABLE;
			if (_beginthread(DeleteTableWorkingThread, 0, (void*)&Params) == -1)
			{
				// Error
				MessageBox("Unable to start \"Delete Table\" thread.", "Error", MB_OK | MB_ICONINFORMATION);
			}
		}
		else
		{
			AfxMessageBox("Unexpected Error: 998", MB_OK | MB_ICONINFORMATION);
		}
	
		//CString str = "Size :  ";
		//str += (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_PERFORMANCE_TABLE);
		
		//m_PerfSizeStatic.SetWindowText((LPCTSTR)str);

		//str = "Size :  ";
		//str += (LPCTSTR)m_pSystem->GetTableSize(TDMFOBJECTSLib::TDMF_DB_ALL_TABLES);
		
		//m_DBSizeStatic.SetWindowText((LPCTSTR)str);

		//UpdateData(FALSE);

		MessageBox("Due to the database manager, the table records changes are not going to affect the table\nsizes immediately."
			"  The new table sizes will be available between 10 and 30 seconds.\n\nTo refresh the table sizes, click on "
			"'Clear Table' again after the elapsed time.",
			"Table Size Changes", MB_OK | MB_ICONINFORMATION);
	}
}

BOOL COptionsDatabasePage::OnApply() 
{
	UpdateData();

	m_pSystem->SetDeleteRecords(TDMFOBJECTSLib::TDMF_DB_ALERT_TABLE, m_iAlertDays, m_iAlertNbRecords);
	m_pSystem->SetDeleteRecords(TDMFOBJECTSLib::TDMF_DB_PERFORMANCE_TABLE, m_iPerfDays, m_iPerfNbRecords);
	m_pSystem->SetDeleteDelay(m_iCleanupDelay);

	return CPropertyPage::OnApply();
}

