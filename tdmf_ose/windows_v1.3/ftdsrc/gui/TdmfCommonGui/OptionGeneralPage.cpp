// OptionGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "OptionGeneralPage.h"
#include "libmngtdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionGeneralPage property page

IMPLEMENT_DYNCREATE(COptionGeneralPage, CPropertyPage)

COptionGeneralPage::COptionGeneralPage(CTdmfCommonGuiDoc* pDoc) : 
    CPropertyPage(COptionGeneralPage::IDD), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(COptionGeneralPage)
    m_nT1   = 1;
    m_nT2   = 1;
    m_nT3   = 1;	
    m_nT4   = 1;     
    m_nT5   = 1;     
    m_nT6   = 1;     
    m_nT7   = 1;     
    m_nT8   = 1;
    m_nT9   = 1;
    m_nT10  = 1;
	//}}AFX_DATA_INIT
}

COptionGeneralPage::~COptionGeneralPage()
{
}

void COptionGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionGeneralPage)
	DDX_Control(pDX, IDC_SPIN_T2, m_SpinT2);
	DDX_Control(pDX, IDC_SPIN_T1, m_SpinT1);
	DDX_Control(pDX, IDC_SPIN_T3, m_SpinT3);
	DDX_Control(pDX, IDC_SPIN_T4, m_SpinT4);
	DDX_Control(pDX, IDC_SPIN_T5, m_SpinT5);
	DDX_Control(pDX, IDC_SPIN_T6, m_SpinT6);
	DDX_Control(pDX, IDC_SPIN_T7, m_SpinT7);
	DDX_Control(pDX, IDC_SPIN_T8, m_SpinT8);
	DDX_Control(pDX, IDC_SPIN_T9, m_SpinT9);
	DDX_Control(pDX, IDC_SPIN_T10, m_SpinT10);

    DDX_Control(pDX, IDC_EDIT_T9, m_EditT9);
    DDX_Control(pDX, IDC_EDIT_T8, m_EditT8);
    DDX_Control(pDX, IDC_EDIT_T7, m_EditT7);
    DDX_Control(pDX, IDC_EDIT_T6, m_EditT6);
    DDX_Control(pDX, IDC_EDIT_T5, m_EditT5);
    DDX_Control(pDX, IDC_EDIT_T4, m_EditT4);
    DDX_Control(pDX, IDC_EDIT_T3, m_EditT3);
	DDX_Control(pDX, IDC_EDIT_T2, m_EditT2);
	DDX_Control(pDX, IDC_EDIT_T1, m_EditT1);
	DDX_Control(pDX, IDC_EDIT_T10, m_EditT10);	
    DDX_Text(pDX, IDC_EDIT_T1, m_nT1);
	DDV_MinMaxUInt(pDX, m_nT1, 1, 99);
	DDX_Text(pDX, IDC_EDIT_T2, m_nT2);
	DDV_MinMaxUInt(pDX, m_nT2, 1, 99);
	DDX_Text(pDX, IDC_EDIT_T10, m_nT10);
	DDV_MinMaxUInt(pDX, m_nT10, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T9, m_nT9);
	DDV_MinMaxUInt(pDX, m_nT9, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T8, m_nT8);
	DDV_MinMaxUInt(pDX, m_nT8, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T7, m_nT7);
	DDV_MinMaxUInt(pDX, m_nT7, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T6, m_nT6);
	DDV_MinMaxUInt(pDX, m_nT6, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T5, m_nT5);
	DDV_MinMaxUInt(pDX, m_nT5, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T4, m_nT4);
	DDV_MinMaxUInt(pDX, m_nT4, 0, 99);
    DDX_Text(pDX, IDC_EDIT_T3, m_nT3);
	DDV_MinMaxUInt(pDX, m_nT3, 0, 99);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(COptionGeneralPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionGeneralPage message handlers

BOOL COptionGeneralPage::OnApply() 
{

	UpdateData();

    if(m_pDoc != NULL)
        if(m_pDoc->m_pSystem != NULL)
        {
            m_pDoc->m_pSystem->SetTimeOut(0, m_nT1);
            m_pDoc->m_pSystem->SetTimeOut(1, m_nT2);
            m_pDoc->m_pSystem->SetTimeOut(2, m_nT3);
            m_pDoc->m_pSystem->SetTimeOut(3, m_nT4);
            m_pDoc->m_pSystem->SetTimeOut(4, m_nT5);
            m_pDoc->m_pSystem->SetTimeOut(5, m_nT6);
            m_pDoc->m_pSystem->SetTimeOut(6, m_nT7);
            m_pDoc->m_pSystem->SetTimeOut(7, m_nT8);
            m_pDoc->m_pSystem->SetTimeOut(8, m_nT9);
            m_pDoc->m_pSystem->SetTimeOut(9, m_nT10);
       
            m_pDoc->m_pSystem->SetTimeOutToDB();
        }

	return CPropertyPage::OnApply();
}

BOOL COptionGeneralPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
  //Call function to load variables from collector

	m_SpinT1.SetRange(1, 99);
	m_SpinT2.SetRange(1, 99);
    m_SpinT3.SetRange(1, 99);
    m_SpinT4.SetRange(1, 99);
    m_SpinT5.SetRange(1, 99);
    m_SpinT6.SetRange(1, 99);
    m_SpinT7.SetRange(1, 99);
    m_SpinT8.SetRange(1, 99);
    m_SpinT9.SetRange(1, 99);
    m_SpinT10.SetRange(1, 99);
	m_EditT1.SetLimitText(2);
	m_EditT10.SetLimitText(2);
	m_EditT9.SetLimitText(2);
    m_EditT8.SetLimitText(2);
    m_EditT7.SetLimitText(2);
    m_EditT6.SetLimitText(2);
    m_EditT5.SetLimitText(2);
    m_EditT4.SetLimitText(2);
 	m_EditT3.SetLimitText(2);   
    m_EditT1.SetMargins( 5, 5 );
    m_EditT2.SetMargins( 5, 5 );
	m_EditT10.SetMargins( 5, 5 );
	m_EditT9.SetMargins( 5, 5 );
    m_EditT8.SetMargins( 5, 5 );
    m_EditT7.SetMargins( 5, 5 );
    m_EditT6.SetMargins( 5, 5 );
    m_EditT5.SetMargins( 5, 5 );
    m_EditT4.SetMargins( 5, 5 );
 	m_EditT3.SetMargins( 5, 5 );   


    if(m_pDoc != NULL)
        if(m_pDoc->m_pSystem != NULL)
        {
       	    if(m_pDoc->m_pSystem->GetTimeOutFromDB())
            {
                m_nT1 = m_pDoc->m_pSystem->GetTimeOut(0);
                m_nT2 = m_pDoc->m_pSystem->GetTimeOut(1);
                m_nT3 = m_pDoc->m_pSystem->GetTimeOut(2);
                m_nT4 = m_pDoc->m_pSystem->GetTimeOut(3);
                m_nT5 = m_pDoc->m_pSystem->GetTimeOut(4);
                m_nT6 = m_pDoc->m_pSystem->GetTimeOut(5);
                m_nT7 = m_pDoc->m_pSystem->GetTimeOut(6);
                m_nT8 = m_pDoc->m_pSystem->GetTimeOut(7);
                m_nT9 = m_pDoc->m_pSystem->GetTimeOut(8);
                m_nT10 = m_pDoc->m_pSystem->GetTimeOut(9);
            }
        }
   
    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
