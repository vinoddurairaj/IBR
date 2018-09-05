// SelectCounterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "SelectCounterDlg.h"
#include "ServerPerformanceMonitorPage.h"
#include <afxtempl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectCounterDlg dialog


CSelectCounterDlg::CSelectCounterDlg(CServerPerformanceMonitorPage* pServerPerformanceMonitorPage,CWnd* pParent /*=NULL*/)
	: CDialog(CSelectCounterDlg::IDD, pParent),m_pServerPerformanceMonitorPage(pServerPerformanceMonitorPage)
{
	//{{AFX_DATA_INIT(CSelectCounterDlg)
	m_Edit_Time = 1;
	//}}AFX_DATA_INIT
    m_pRG = NULL;
	
}


void CSelectCounterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectCounterDlg)
	DDX_Control(pDX, IDC_SPIN_TIMESLICE, m_Spin_Time);
	DDX_Control(pDX, ID_BUTTON_ADD, m_ButtonAdd);
	DDX_Control(pDX, IDC_LIST_STATS, m_ListStats);
	DDX_Control(pDX, IDC_LIST_GROUPS, m_ListGroup);
	DDX_Text(pDX, IDC_EDIT_TIMESLICE, m_Edit_Time);
	DDV_MinMaxUInt(pDX, m_Edit_Time, 10, 60);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectCounterDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectCounterDlg)
	ON_BN_CLICKED(ID_BUTTON_ADD, OnButtonAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectCounterDlg message handlers

BOOL CSelectCounterDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   m_Spin_Time.SetRange(10, 60 );
	m_ButtonAdd.EnableWindow(TRUE);

	if (m_pServer != NULL)
	{
		for (int nIndex = 0; nIndex < m_pServer->ReplicationGroupCount; nIndex++)
		{
			TDMFOBJECTSLib::IReplicationGroupPtr pGroup = m_pServer->GetReplicationGroup(nIndex);
          
			int nItem = m_ListGroup.AddString(pGroup->Name);
			CString name = (BSTR)pGroup->Name;
			int ngroupnumber =  pGroup->GroupNumber;
			if(!pGroup->IsSource)
			{
				int nValue =  10000000 +  pGroup->GroupNumber;
				m_ListGroup.SetItemData(nItem, nValue);
			}
			else
			{
				m_ListGroup.SetItemData(nItem, pGroup->GroupNumber);
			}
		}
	}

	int nItemStat; 
	nItemStat = m_ListStats.AddString("% Done");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::ePctDone);
	nItemStat = m_ListStats.AddString("Read Bytes");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::eReadBytes);
	nItemStat = m_ListStats.AddString("Write Bytes");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::eWriteBytes);
	nItemStat = m_ListStats.AddString("Actual Net");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::eActualNet);
	nItemStat = m_ListStats.AddString("Effective Net");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::eEffectiveNet);
	nItemStat = m_ListStats.AddString("Entries in BAB");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::eBABEntries);
	nItemStat = m_ListStats.AddString("% BAB Full");
	m_ListStats.SetItemData(nItemStat, CCounterInfo::ePctBABFull);

    if(m_pServer != NULL)
    {
        CUIntArray *pTheArrayOfStats; 
        if(m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfStatistics.Lookup((WORD)m_pServer->Key,( CObject*& ) pTheArrayOfStats) != 0)
        {
            int nStatCount = pTheArrayOfStats->GetSize();
            for(int i = 0; i < nStatCount; i++)
		      {
               int nStat = pTheArrayOfStats->GetAt(i);
             
                //search the item in the listbox
               int nCount = m_ListStats.GetCount();
               for(int j = 0; j < nCount; j++)
               {
                  int nListStat = m_ListStats.GetItemData(j);
                
                  if(nStat == nListStat)
                  {
                      m_ListStats.SetSel(j);
                      continue;
                  }
               }
		      }
        }
       

      
      //selection for groups
      if(m_pRG != NULL)
      {
         m_ListStats.EnableWindow(FALSE);
         CString strGroupName = (BSTR) m_pRG->Name;
         int nIndex = m_ListGroup.FindString(0,strGroupName);
         if(nIndex != LB_ERR)
         {
           m_ListGroup.SetSel(nIndex);
           m_ListGroup.EnableWindow(FALSE);
         }
       }
      else
      {
         CUIntArray *pArrayOfGroups; 
         if(m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfGroupNumber.Lookup((WORD)m_pServer->Key,( CObject*& ) pArrayOfGroups) != 0)
         {
            int nGroupCount = pArrayOfGroups->GetSize();
            for(int i = 0; i < nGroupCount; i++)
			{
				int nGroupNumber = pArrayOfGroups->GetAt(i);

				//search the item in the listbox
				int nCount = m_ListGroup.GetCount();
				for(int j = 0; j < nCount; j++)
				{
					int nListGroup = m_ListGroup.GetItemData(j);

					if(nGroupNumber == nListGroup)
					{
					  m_ListGroup.SetSel(j);
					  continue;
					}
				}
			}
         }
      }
    }
       
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectCounterDlg::OnButtonAdd() 
{
   // it is a server selection
   CUIntArray *parySelGroups; 

   if(m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfGroupNumber.Lookup((WORD)m_pServer->Key,( CObject*& ) parySelGroups) != 0)
   {
      parySelGroups->RemoveAll();
      m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfGroupNumber.RemoveKey((WORD)m_pServer->Key);
   }
   else
   {
      parySelGroups = new CUIntArray();
   }

   // Get the indexes of all the selected items.
   int nCount = m_ListGroup.GetSelCount();

   if(nCount > 0)
   {
      m_aryListBoxSelGroups.SetSize(nCount);
      m_ListGroup.GetSelItems(nCount, m_aryListBoxSelGroups.GetData()); 

      int nIndex;
      for (int i = 0; i < nCount; i++)
      {
	      nIndex = m_aryListBoxSelGroups[i];
	      parySelGroups->Add(m_ListGroup.GetItemData(nIndex));
      }
      m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfGroupNumber.SetAt((WORD)m_pServer->Key,parySelGroups);
   }
  
   CUIntArray *pTheArrayOfStats; 
   if(m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfStatistics.Lookup((WORD)m_pServer->Key,( CObject*& ) pTheArrayOfStats) != 0)
   {
      pTheArrayOfStats->RemoveAll();
      m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfStatistics.RemoveKey((WORD)m_pServer->Key);
   }
   else
   {
      pTheArrayOfStats = new CUIntArray();
   }

   int nStatsCount = m_ListStats.GetSelCount();

   if(nStatsCount > 0)
   {
      m_aryListBoxSelStats.SetSize(nStatsCount);
      m_ListStats.GetSelItems(nStatsCount, m_aryListBoxSelStats.GetData()); 

      int nStatIndex;
      for (int j = 0; j < nStatsCount; j++)
      {
        nStatIndex = m_aryListBoxSelStats[j];
        pTheArrayOfStats->Add(m_ListStats.GetItemData(nStatIndex));
      }
      m_pServerPerformanceMonitorPage->m_mapServerIDToArrayOfStatistics.SetAt((WORD)m_pServer->Key,pTheArrayOfStats);
   }

   CDialog::OnOK();

}
