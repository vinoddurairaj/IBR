// Systems.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "Systems.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystems dialog


CSystems::CSystems(CWnd* pParent /*=NULL*/)
	: CDialog(CSystems::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSystems)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSystems::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystems)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystems, CDialog)
	//{{AFX_MSG_MAP(CSystems)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystems message handlers
