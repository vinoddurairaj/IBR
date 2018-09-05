// RGEventsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "MainFrm.h"
#include "RGEventsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGEventsPage property page

IMPLEMENT_DYNCREATE(CRGEventsPage, CSystemEventsPage)

CRGEventsPage::CRGEventsPage()
{
	//{{AFX_DATA_INIT(CRGEventsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRGEventsPage::~CRGEventsPage()
{
}

void CRGEventsPage::DoDataExchange(CDataExchange* pDX)
{
	CSystemEventsPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGEventsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGEventsPage, CSystemEventsPage)
	//{{AFX_MSG_MAP(CRGEventsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGEventsPage message handlers

BOOL CRGEventsPage::OnInitDialog() 
{
	CSystemEventsPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRGEventsPage::SaveColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CRGEventsPage") != pFrame->m_mapStream.end())
	{
		pFrame->m_mapStream["CRGEventsPage"]->freeze(0);
		delete pFrame->m_mapStream["CRGEventsPage"];
	}

	std::ostrstream* poss = new std::ostrstream;
	pFrame->m_mapStream["CRGEventsPage"] = poss;

	// Save Column info
	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
	{
		int nWidth = m_ListCtrlEvent.GetColumnWidth(it->m_nIndex);
		
		pFrame->m_mapStream["CRGEventsPage"]->write((char*)&(nWidth), sizeof(nWidth));
	}

	// Save column order info
	int* pnIndex = new int[m_vecColumnDef.size()];
	m_ListCtrlEvent.GetColumnOrderArray(pnIndex);
	pFrame->m_mapStream["CRGEventsPage"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
	delete[] pnIndex;
}

void CRGEventsPage::LoadColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CRGEventsPage") != pFrame->m_mapStream.end())
	{
		std::istrstream iss(pFrame->m_mapStream["CRGEventsPage"]->str(), pFrame->m_mapStream["CRGEventsPage"]->pcount());

		for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
			it != m_vecColumnDef.end() ;it++)
		{
			int nWidth;
			iss.read((char*)&nWidth, sizeof(int));

			it->m_bVisible = (nWidth != 0);

			m_ListCtrlEvent.SetColumnWidth(it->m_nIndex, nWidth);
		}

		int* pnIndex = new int[m_vecColumnDef.size()];
		iss.read((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
		m_ListCtrlEvent.SetColumnOrderArray(m_vecColumnDef.size(), pnIndex);
		delete[] pnIndex;
	}
}
