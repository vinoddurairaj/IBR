// RGSelectServerDialog.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGSelectServerDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGSelectServerDialog dialog


CRGSelectServerDialog::CRGSelectServerDialog(TDMFOBJECTSLib::IReplicationGroup *pRG /*=NULL*/, CWnd* pParent /*=NULL*/)
	: CDialog(CRGSelectServerDialog::IDD, pParent), m_pRG(pRG)
{
	//{{AFX_DATA_INIT(CRGSelectServerDialog)
	m_ServerSelected = _T("");
	m_Chk_DHCP = FALSE;
	//}}AFX_DATA_INIT

	try
	{
		if(m_pRG != NULL)
		{
			m_pServer = m_pRG->GetParent();
            m_Chk_DHCP = m_pRG->IsTargetDHCPAdressUsed();
                 
		}
		else
		{
			m_pServer = NULL;
		}
		
		if(m_pServer != NULL)
		{
			m_pDomain = m_pServer->GetParent();
		}
		else
		{
			m_pDomain = NULL;
		}
	}
	CATCH_ALL_LOG_ERROR(1013);
}


void CRGSelectServerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGSelectServerDialog)
	DDX_Control(pDX, IDC_SERVER_LIST, m_ServerListBox);
	DDX_LBString(pDX, IDC_SERVER_LIST, m_ServerSelected);
	DDX_Check(pDX, IDC_CHECK_DHCP, m_Chk_DHCP);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGSelectServerDialog, CDialog)
	//{{AFX_MSG_MAP(CRGSelectServerDialog)
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_SERVER_LIST, OnSelchangeServerList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGSelectServerDialog message handlers

BOOL CRGSelectServerDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	try
	{
		if(m_pDomain != NULL)
		{
			int dx = 0;
			CDC* pDC = m_ServerListBox.GetDC();
			
			TDMFOBJECTSLib::IServerPtr pServer;
			
			long numServer = m_pDomain->GetServerCount();
			for(int i=0; i<numServer; i++)
			{
				pServer = m_pDomain->GetServer(i);
				std::string str = pServer->GetName();
				str += " (";
				str += pServer->GetIPAddress(0);
				str += ")";		
				int nIndex = m_ServerListBox.InsertString(i, str.c_str());
				// Store a ptr on the server
				m_ServerListBox.SetItemData(nIndex, (DWORD)((TDMFOBJECTSLib::IServer*)pServer));
				pServer->AddRef();
				
				// Find the longest string in the list box.
				CString cstr = str.c_str();
				CSize sz = pDC->GetTextExtent(cstr);
				
				if (sz.cx > dx)
				{
					dx = sz.cx;
				}

           }
 			
           
            int nIndex;
            if(FindServerInListBox(nIndex))
            {
                m_ServerListBox.SetCurSel(nIndex);
                m_pServerSelected = (TDMFOBJECTSLib::IServer*)m_ServerListBox.GetItemData(nIndex);
            }
            else
            {
                m_ServerListBox.SetCurSel(0);
			    m_pServerSelected = (TDMFOBJECTSLib::IServer*)m_ServerListBox.GetItemData(0);
            }
		
			
			m_ServerListBox.ReleaseDC(pDC);
			
			// Set the horizontal extent so every character of all strings 
			// can be scrolled to.
			m_ServerListBox.SetHorizontalExtent(dx);
		}
	}
	CATCH_ALL_LOG_ERROR(1014);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRGSelectServerDialog::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// Cleanup
	int nCount = m_ServerListBox.GetCount();
	if(nCount != LB_ERR)
	{
		for(int nIndex = 0; nIndex < nCount; nIndex++)
		{
			TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)m_ServerListBox.GetItemData(nIndex);
			pServer->Release();
		}
	}	
}

void CRGSelectServerDialog::OnSelchangeServerList() 
{
	int nIndex = m_ServerListBox.GetCurSel();
    
    if( nIndex != CB_ERR )
    {
        m_ServerListBox.GetText(nIndex, m_ServerSelected );

   	    m_pServerSelected = (TDMFOBJECTSLib::IServer*)m_ServerListBox.GetItemData(nIndex);	
    }    
}

BOOL CRGSelectServerDialog::IsDHCPAdressSelected()
{
  return m_Chk_DHCP;
}

void CRGSelectServerDialog::OnOK() 
{
	OnSelchangeServerList() ;
	
	CDialog::OnOK();
}

BOOL CRGSelectServerDialog::FindServerInListBox(int& nindex)
{
   if (m_pRG == NULL) 
       return false;

   if (m_pRG->TargetServer == NULL)
       return false;

   TDMFOBJECTSLib::IServerPtr m_pServer;
  // If any item's data is equal to zero then reset it to -1.
    for (nindex =0;nindex < m_ServerListBox.GetCount();nindex++)
    {
       m_pServer = (TDMFOBJECTSLib::IServer*)m_ServerListBox.GetItemData(nindex);
       
       if(m_pServer != NULL)
       {
           if (m_pServer->IPAddress[0] == m_pRG->TargetServer->IPAddress[0])
           {
              return true;
           }
       }
       
    }
   return false;
}
