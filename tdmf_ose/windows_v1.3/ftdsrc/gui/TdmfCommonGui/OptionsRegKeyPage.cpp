// OptionsRegKeyPage.cpp : implementation file
//


// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "OptionsRegKeyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsRegKeyPage property page

IMPLEMENT_DYNCREATE(COptionsRegKeyPage, CPropertyPage)

COptionsRegKeyPage::COptionsRegKeyPage(CTdmfCommonGuiDoc* pDoc) :
	 CPropertyPage(COptionsRegKeyPage::IDD), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(COptionsRegKeyPage)
	m_cstrRegKey1 = _T("");
	m_cstrRegKey2 = _T("");
	m_cstrRegKey3 = _T("");
	m_cstrRegKey4 = _T("");
	m_cstrRegKey5 = _T("");
	m_cstrRegKey6 = _T("");
	//}}AFX_DATA_INIT
}

COptionsRegKeyPage::~COptionsRegKeyPage()
{
}

void COptionsRegKeyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsRegKeyPage)
	DDX_Control(pDX, IDC_EDIT_REG_KEY6, m_EditRegKey6);
	DDX_Control(pDX, IDC_EDIT_REG_KEY5, m_EditRegKey5);
	DDX_Control(pDX, IDC_EDIT_REG_KEY4, m_EditRegKey4);
	DDX_Control(pDX, IDC_EDIT_REG_KEY3, m_EditRegKey3);
	DDX_Control(pDX, IDC_EDIT_REG_KEY2, m_EditRegKey2);
	DDX_Control(pDX, IDC_EDIT_REG_KEY1, m_EditRegKey1);
	DDX_Text(pDX, IDC_EDIT_REG_KEY1, m_cstrRegKey1);
	DDX_Text(pDX, IDC_EDIT_REG_KEY2, m_cstrRegKey2);
	DDX_Text(pDX, IDC_EDIT_REG_KEY3, m_cstrRegKey3);
	DDX_Text(pDX, IDC_EDIT_REG_KEY4, m_cstrRegKey4);
	DDX_Text(pDX, IDC_EDIT_REG_KEY5, m_cstrRegKey5);
	DDX_Text(pDX, IDC_EDIT_REG_KEY6, m_cstrRegKey6);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsRegKeyPage, CPropertyPage)
	//{{AFX_MSG_MAP(COptionsRegKeyPage)
	ON_EN_UPDATE(IDC_EDIT_REG_KEY1, OnUpdateEditRegKey1)
	ON_EN_UPDATE(IDC_EDIT_REG_KEY2, OnUpdateEditRegKey2)
	ON_EN_UPDATE(IDC_EDIT_REG_KEY3, OnUpdateEditRegKey3)
	ON_EN_UPDATE(IDC_EDIT_REG_KEY4, OnUpdateEditRegKey4)
	ON_EN_UPDATE(IDC_EDIT_REG_KEY5, OnUpdateEditRegKey5)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsRegKeyPage message handlers



// Mike Pollett
#include "../../tdmf.inc"

#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"

BOOL COptionsRegKeyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	DWORD dwType;
	char  szValue[64];
	ULONG nSize = 64;

	m_EditRegKey1.SetLimitText(4);
	m_EditRegKey2.SetLimitText(4);
	m_EditRegKey3.SetLimitText(4);
	m_EditRegKey4.SetLimitText(4);
	m_EditRegKey5.SetLimitText(4);
	m_EditRegKey6.SetLimitText(4);
				
	m_EditRegKey1.SetMargins( 5, 5 );
	m_EditRegKey2.SetMargins( 5, 5 );
	m_EditRegKey3.SetMargins( 5, 5 );
	m_EditRegKey4.SetMargins( 5, 5 );
	m_EditRegKey5.SetMargins( 5, 5 );
	m_EditRegKey6.SetMargins( 5, 5 );

	if (SHGetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "DtcRegKey", &dwType, &szValue, &nSize) == ERROR_SUCCESS)
	{
		if (strlen(szValue) > 0)
		{
			CString cstrKey = szValue;
			cstrKey.TrimLeft();
			
			int nIndex = cstrKey.Find(' ');
			if (nIndex != -1)
			{
				m_cstrRegKey1 = cstrKey.Left(nIndex);
				cstrKey.Delete(0, nIndex+1);
			}
			nIndex = cstrKey.Find(' ');
			if (nIndex != -1)
			{
				m_cstrRegKey2 = cstrKey.Left(nIndex);
				cstrKey.Delete(0, nIndex+1);
			}
			nIndex = cstrKey.Find(' ');
			if (nIndex != -1)
			{
				m_cstrRegKey3 = cstrKey.Left(nIndex);
				cstrKey.Delete(0, nIndex+1);
			}
			nIndex = cstrKey.Find(' ');
			if (nIndex != -1)
			{
				m_cstrRegKey4 = cstrKey.Left(nIndex);
				cstrKey.Delete(0, nIndex+1);
			}
			nIndex = cstrKey.Find(' ');
			if (nIndex != -1)
			{
				m_cstrRegKey5 = cstrKey.Left(nIndex);
				cstrKey.Delete(0, nIndex+1);
			}
			if (cstrKey.GetLength() > 0)
			{
				m_cstrRegKey6 = cstrKey;
			}

			UpdateData(FALSE);
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL COptionsRegKeyPage::OnApply() 
{	
	CString cstrKey;
	cstrKey.Format("%s %s %s %s %s %s", m_cstrRegKey1, m_cstrRegKey2, m_cstrRegKey3,
		m_cstrRegKey4, m_cstrRegKey5, m_cstrRegKey6);

	SHSetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "DtcRegKey", REG_SZ, cstrKey, cstrKey.GetLength());

	return CPropertyPage::OnApply();
}

void COptionsRegKeyPage::OnUpdateEditRegKey1() 
{
	SetModified();

    if(m_EditRegKey1.GetWindowTextLength() == 4)
    {
        m_EditRegKey2.SetFocus();
    }
   
}

void COptionsRegKeyPage::OnUpdateEditRegKey2() 
{
	SetModified();
    
    if(m_EditRegKey2.GetWindowTextLength() == 4)
    {
        m_EditRegKey3.SetFocus();
    }
}

void COptionsRegKeyPage::OnUpdateEditRegKey3() 
{
	SetModified();

    if(m_EditRegKey3.GetWindowTextLength() == 4)
    {
        m_EditRegKey4.SetFocus();
    }
}

void COptionsRegKeyPage::OnUpdateEditRegKey4() 
{
	SetModified();
    
    if(m_EditRegKey4.GetWindowTextLength() == 4)
    {
        m_EditRegKey5.SetFocus();
    }
}

void COptionsRegKeyPage::OnUpdateEditRegKey5() 
{
	SetModified();

    if(m_EditRegKey5.GetWindowTextLength() == 4)
    {
        m_EditRegKey6.SetFocus();
    }
}
