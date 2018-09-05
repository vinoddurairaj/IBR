// Messenger.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "Messenger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMessenger dialog


CMessenger::CMessenger(CWnd* pParent /*=NULL*/)
	: CDialog(CMessenger::IDD, pParent), m_pDoc(NULL)
{
	//{{AFX_DATA_INIT(CMessenger)
	m_cstrMessage = _T("");
	m_cstrCommunications = _T("");
	//}}AFX_DATA_INIT
}


void CMessenger::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessenger)
	DDX_Text(pDX, IDC_EDIT_MSG, m_cstrMessage);
	DDV_MaxChars(pDX, m_cstrMessage, 55);
	DDX_Text(pDX, IDC_EDIT_ALL_MSG, m_cstrCommunications);
	//}}AFX_DATA_MAP
}

void CMessenger::AddMessage(UINT nID, char* pszMsg)
{
	if (pszMsg)
	{
		CString cstrMsg;
		cstrMsg.Format("%d> %s\r\n", nID, pszMsg);

		m_cstrCommunications += cstrMsg;

		UpdateData(FALSE);
	}
}


BEGIN_MESSAGE_MAP(CMessenger, CDialog)
	//{{AFX_MSG_MAP(CMessenger)
	ON_BN_CLICKED(IDC_BUTTON_SEND, OnButtonSend)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessenger message handlers

void CMessenger::OnButtonSend() 
{
	UpdateData();

	m_pDoc->m_pSystem->SendTextMessage((LPCSTR)m_cstrMessage);

	m_cstrMessage = "";
	UpdateData(FALSE);
}
