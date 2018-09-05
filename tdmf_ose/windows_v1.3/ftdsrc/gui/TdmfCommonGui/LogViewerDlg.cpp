// LogViewerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "LogViewerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogViewerDlg dialog


CLogViewerDlg::CLogViewerDlg(CTdmfCommonGuiDoc* pDoc, BOOL bActionsLog /*=TRUE*/, CWnd* pParent /*=NULL*/)
	: CDialog(CLogViewerDlg::IDD, pParent), m_pDoc(pDoc), m_bActionsLog(bActionsLog)
{
	//{{AFX_DATA_INIT(CLogViewerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CLogViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogViewerDlg)
	DDX_Control(pDX, IDC_LIST_MSG, m_ListMsg);
	//}}AFX_DATA_MAP
}

void CLogViewerDlg::RefreshData()
{
	USES_CONVERSION;

	CComBSTR bstrDate;
	CComBSTR bstrSource;
	long     nHostId;
	CComBSTR bstrInfo;
	CComBSTR bstrDetail;

	if (m_bActionsLog)
	{
		m_pDoc->m_pSystem->GetFirstLogMsg(&bstrDate, &bstrSource, &bstrInfo, &bstrDetail);
	}
	else
	{
		m_pDoc->m_pSystem->GetFirstKeyLogMsg(&bstrDate, &bstrSource, &nHostId, &bstrInfo, &bstrDetail);
	}

	while (bstrDate.Length() > 0)
	{
		m_ListMsg.InsertItem(0, OLE2A(bstrDate));
		m_ListMsg.SetItemText(0, 1, OLE2A(bstrSource));

		if (m_bActionsLog)
		{
			m_ListMsg.SetItemText(0, 2, OLE2A(bstrInfo));
			m_ListMsg.SetItemText(0, 3, OLE2A(bstrDetail));
		}
		else
		{
			CString cstrHostId;
			cstrHostId.Format("0x%08x", nHostId);
			m_ListMsg.SetItemText(0, 2, cstrHostId);
			m_ListMsg.SetItemText(0, 3, OLE2A(bstrInfo));
			m_ListMsg.SetItemText(0, 4, OLE2A(bstrDetail));
		}

		if (m_bActionsLog)
		{
			m_pDoc->m_pSystem->GetNextLogMsg(&bstrDate, &bstrSource, &bstrInfo, &bstrDetail);
		}
		else
		{
			m_pDoc->m_pSystem->GetNextKeyLogMsg(&bstrDate, &bstrSource, &nHostId, &bstrInfo, &bstrDetail);
		}
	}
}

BEGIN_MESSAGE_MAP(CLogViewerDlg, CDialog)
	//{{AFX_MSG_MAP(CLogViewerDlg)
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_MSG, OnColumnclickListMsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogViewerDlg message handlers

BOOL CLogViewerDlg::OnInitDialog() 
{
	if (m_bActionsLog == FALSE)
	{
		SetWindowText("Keys Changes Log");
	}

	CDialog::OnInitDialog();

	m_ListMsg.InsertColumn(0, "Date");
	m_ListMsg.SetColumnWidth(0,  120);

	if (m_bActionsLog)
	{
		m_ListMsg.InsertColumn(1, "Source");
		m_ListMsg.SetColumnWidth(1,  100);

		m_ListMsg.InsertColumn(2, "User");
		m_ListMsg.SetColumnWidth(2, 100);

		m_ListMsg.InsertColumn(3, "Message");
		m_ListMsg.SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER );
	}
	else
	{
		m_ListMsg.InsertColumn(1, "Hostname");
		m_ListMsg.SetColumnWidth(1,  100);

		m_ListMsg.InsertColumn(2, "HostId");
		m_ListMsg.SetColumnWidth(2,  100);

		m_ListMsg.InsertColumn(3, "Key");
		m_ListMsg.SetColumnWidth(3, 200);
		
		m_ListMsg.InsertColumn(4, "Expiration Date");
		m_ListMsg.SetColumnWidth(4, LVSCW_AUTOSIZE_USEHEADER );
	}

	RefreshData();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLogViewerDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	if (::IsWindow(m_ListMsg.m_hWnd))
	{
		m_ListMsg.MoveWindow(0, 0, cx, cy);

		if (m_bActionsLog)
		{
			m_ListMsg.SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER );
		}
		else
		{
			m_ListMsg.SetColumnWidth(4, LVSCW_AUTOSIZE_USEHEADER );
		}
	}
}

void CLogViewerDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if (m_bActionsLog)
	{
		CMenu Menu;
		VERIFY(Menu.CreatePopupMenu());
		
		// Fill Context Menu
		Menu.AppendMenu(MF_STRING | MF_ENABLED, 1, "Refresh");
		Menu.AppendMenu(MF_STRING | MF_ENABLED, 2, "Delete All");
		
		// Display Context Menu
		UINT nCmd = ::TrackPopupMenuEx(Menu.m_hMenu,
			TPM_LEFTALIGN|TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,
			point.x, point.y, GetActiveWindow()->m_hWnd, NULL);
		
		// Take action
		if (nCmd != 0)
		{
			switch (nCmd)
			{
			case 1:
				m_ListMsg.DeleteAllItems();
				RefreshData();
				break;
				
			case 2:
				if (m_pDoc->m_pSystem->DeleteAllLogMsg())
				{
					m_ListMsg.DeleteAllItems();
				}
				break;
			}
		}
	}
}

class CCompareData
{
public:
	CListCtrl* m_pListCtrl;
	int        m_nColumn;
	BOOL       m_bAscending;

	CCompareData() : m_bAscending(FALSE) {}
};

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CCompareData* pCompareData = (CCompareData*)lParamSort;
	CString strItem1 = pCompareData->m_pListCtrl->GetItemText(lParam1, pCompareData->m_nColumn);
	CString strItem2 = pCompareData->m_pListCtrl->GetItemText(lParam2, pCompareData->m_nColumn);

	return strcmp(strItem2, strItem1) * (pCompareData->m_bAscending ? 1 : -1);
}

void CLogViewerDlg::OnColumnclickListMsg(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	static CCompareData CompareData;
	CompareData.m_pListCtrl = &m_ListMsg;
	CompareData.m_nColumn = pNMListView->iSubItem;
	CompareData.m_bAscending = !CompareData.m_bAscending;

#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)
#endif
	m_ListMsg.SendMessage(LVM_SORTITEMSEX, (WPARAM)&CompareData, (LPARAM)CompareItems);

	*pResult = 0;
}
