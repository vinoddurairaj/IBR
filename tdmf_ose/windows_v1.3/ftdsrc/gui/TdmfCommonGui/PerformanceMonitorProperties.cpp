// PerformanceMonitorProperties.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "PerformanceMonitorProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerformanceMonitorProperties dialog


CPerformanceMonitorProperties::CPerformanceMonitorProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CPerformanceMonitorProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPerformanceMonitorProperties)
	m_DateTimeEnd = COleDateTime::GetCurrentTime();
	m_DateTimeStart = COleDateTime::GetCurrentTime();
	m_nValueEnd = 0;
	m_nValueStart = 0;
	//}}AFX_DATA_INIT
}


void CPerformanceMonitorProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPerformanceMonitorProperties)
	DDX_Control(pDX, IDC_EDIT_VALUE_END, m_TXT_ValueEndCtrl);
	DDX_Control(pDX, IDC_EDIT_VALUE_START, m_TXT_ValueStartCtrl);
	DDX_Control(pDX, IDC_EDIT_TIME_END, m_TXT_End_Date);
	DDX_Control(pDX, IDC_EDIT_TIME_START, m_TXT_Start_Date);
	DDX_Text(pDX, IDC_EDIT_TIME_END, m_DateTimeEnd);
	DDX_Text(pDX, IDC_EDIT_TIME_START, m_DateTimeStart);
	DDX_Text(pDX, IDC_EDIT_VALUE_END, m_nValueEnd);
	DDV_MinMaxInt(pDX, m_nValueEnd, 1, 1000000);
	DDX_Text(pDX, IDC_EDIT_VALUE_START, m_nValueStart);
	DDV_MinMaxInt(pDX, m_nValueStart, 0, 1000000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPerformanceMonitorProperties, CDialog)
	//{{AFX_MSG_MAP(CPerformanceMonitorProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPerformanceMonitorProperties message handlers

void CPerformanceMonitorProperties::OnOK() 
{
    
    UpdateData();

    if(m_DateTimeStart.GetStatus() == COleDateTime::invalid)
    {
      m_TXT_Start_Date.SetFocus();
      return;
    }

    if(m_DateTimeEnd.GetStatus() == COleDateTime::invalid)
    {
      m_TXT_End_Date.SetFocus();
      return;
    }

    COleDateTimeSpan dRes =  m_DateTimeEnd - m_DateTimeStart;
    if	(dRes.m_span <= 0)
    {

        AfxMessageBox("The 'Time End' field must be greater then 'Time Start' field", MB_OK|MB_ICONSTOP);
        
        m_TXT_End_Date.SetFocus();
        return;
    }

    int nValue = m_nValueEnd - m_nValueStart;
    if	(nValue <= 0)
    {

        AfxMessageBox("The 'Value End' field must be greater then 'Value Start' field", MB_OK|MB_ICONSTOP);
        
        m_TXT_ValueEndCtrl.SetFocus();
        return;
    }


	CDialog::OnOK() ;
}

BOOL CPerformanceMonitorProperties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_TXT_ValueEndCtrl.SetLimitText(7);
	m_TXT_ValueStartCtrl.SetLimitText(7);
	m_TXT_End_Date.SetLimitText(21);
	m_TXT_Start_Date.SetLimitText(21);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
