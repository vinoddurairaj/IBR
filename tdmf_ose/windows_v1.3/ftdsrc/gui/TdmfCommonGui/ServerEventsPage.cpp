// ServerEventsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "MainFrm.h"
#include "ServerEventsPage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerEventsPage property page

IMPLEMENT_DYNCREATE(CServerEventsPage, CSystemEventsPage)

CServerEventsPage::CServerEventsPage()
{
	//{{AFX_DATA_INIT(CServerEventsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CServerEventsPage::~CServerEventsPage()
{
}

void CServerEventsPage::DoDataExchange(CDataExchange* pDX)
{
	CSystemEventsPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerEventsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerEventsPage, CSystemEventsPage)
	//{{AFX_MSG_MAP(CServerEventsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerEventsPage message handlers

BOOL CServerEventsPage::OnInitDialog() 
{
	CSystemEventsPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerEventsPage::SaveColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CServerEventsPage") != pFrame->m_mapStream.end())
	{
		pFrame->m_mapStream["CServerEventsPage"]->freeze(0);
		delete pFrame->m_mapStream["CServerEventsPage"];
	}

	std::ostrstream* poss = new std::ostrstream;
	pFrame->m_mapStream["CServerEventsPage"] = poss;

	// Save Column info
	for(std::vector<CListViewColumnDef>::iterator it = m_vecColumnDef.begin();
		it != m_vecColumnDef.end() ;it++)
	{
		int nWidth = m_ListCtrlEvent.GetColumnWidth(it->m_nIndex);
		
		pFrame->m_mapStream["CServerEventsPage"]->write((char*)&(nWidth), sizeof(nWidth));
	}

	// Save column order info
	int* pnIndex = new int[m_vecColumnDef.size()];
	m_ListCtrlEvent.GetColumnOrderArray(pnIndex);
	pFrame->m_mapStream["CServerEventsPage"]->write((char*)pnIndex, m_vecColumnDef.size() * sizeof(int));
	delete[] pnIndex;
}

void CServerEventsPage::LoadColumnDef()
{
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();

	if (pFrame->m_mapStream.find("CServerEventsPage") != pFrame->m_mapStream.end())
	{
		std::istrstream iss(pFrame->m_mapStream["CServerEventsPage"]->str(), pFrame->m_mapStream["CServerEventsPage"]->pcount());

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
