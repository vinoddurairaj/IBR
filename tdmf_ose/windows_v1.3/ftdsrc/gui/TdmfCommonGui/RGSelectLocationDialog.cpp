// RGSelectLocationDialog.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGSelectLocationDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGSelectLocationDialog dialog


CRGSelectLocationDialog::CRGSelectLocationDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CRGSelectLocationDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRGSelectLocationDialog)
	m_cstrLocation = _T("");
	//}}AFX_DATA_INIT
}


void CRGSelectLocationDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGSelectLocationDialog)
	DDX_Control(pDX, IDC_LOCATION_LIST, m_ListLocation);
	DDX_LBString(pDX, IDC_LOCATION_LIST, m_cstrLocation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGSelectLocationDialog, CDialog)
	//{{AFX_MSG_MAP(CRGSelectLocationDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGSelectLocationDialog message handlers

BOOL CRGSelectLocationDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int dx = 0;
	CDC* pDC = m_ListLocation.GetDC();

	for (unsigned int i = 0; i < m_vecLocation.size(); i++)
	{
		m_ListLocation.AddString(m_vecLocation[i]);

		// Find the longest string in the list box.
		CSize sz = pDC->GetTextExtent(m_vecLocation[i]);
		if (sz.cx > dx)
		{
			dx = sz.cx;
		}
	}

	m_ListLocation.ReleaseDC(pDC);
			
	// Set the horizontal extent so every character of all strings can be scrolled to.
	m_ListLocation.SetHorizontalExtent(dx);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
