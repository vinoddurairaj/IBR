// EmptyPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "EmptyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEmptyPage property page

IMPLEMENT_DYNCREATE(CEmptyPage, CPropertyPage)

CEmptyPage::CEmptyPage() : CPropertyPage(CEmptyPage::IDD)
{
	//{{AFX_DATA_INIT(CEmptyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CEmptyPage::~CEmptyPage()
{
}

void CEmptyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEmptyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEmptyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CEmptyPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmptyPage message handlers
