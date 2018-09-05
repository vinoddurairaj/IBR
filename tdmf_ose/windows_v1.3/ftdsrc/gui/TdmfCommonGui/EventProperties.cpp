// EventProperties.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguidoc.h"
#include "EventProperties.h"
#include "ViewNotification.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventProperties dialog

CEventProperties::CEventProperties(CTdmfCommonGuiDoc* pDoc, UINT nEventIndex, UINT nMaxIndex, CWnd* pParent /*=NULL*/)
	: CDialog(CEventProperties::IDD, pParent), m_pDoc(pDoc), m_nEventIndex(nEventIndex),
	m_nMaxIndex(nMaxIndex), m_pParent(pParent)
{
	//{{AFX_DATA_INIT(CEventProperties)
	m_cstrDescription = _T("");
	m_cstrDate = _T("");
	m_cstrGroupNumber = _T("");
	m_cstrPairNumber = _T("");
	m_cstrSource = _T("");
	m_cstrTime = _T("");
	m_cstrType = _T("");
	//}}AFX_DATA_INIT
}

void CEventProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEventProperties)
	DDX_Control(pDX, IDC_BUTTON_UP, m_ButtonUp);
	DDX_Control(pDX, IDC_BUTTON_DOWN, m_ButtonDown);
	DDX_Control(pDX, IDC_BUTTON_COPY, m_ButtonCopy);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_cstrDescription);
	DDX_Text(pDX, IDC_STATIC_DATE, m_cstrDate);
	DDX_Text(pDX, IDC_STATIC_GROUP_NUMBER, m_cstrGroupNumber);
	DDX_Text(pDX, IDC_STATIC_PAIR_NUMBER, m_cstrPairNumber);
	DDX_Text(pDX, IDC_STATIC_SOURCE, m_cstrSource);
	DDX_Text(pDX, IDC_STATIC_TIME, m_cstrTime);
	DDX_Text(pDX, IDC_STATIC_TYPE, m_cstrType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEventProperties, CDialog)
	//{{AFX_MSG_MAP(CEventProperties)
	ON_BN_CLICKED(IDC_BUTTON_COPY, OnButtonCopy)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventProperties message handlers

BOOL CEventProperties::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Set buttons icon
	HICON hIconUp   = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_UPARROW));
	HICON hIconDown = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_DOWNARROW));
	HICON hIconCopy = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_COPY));

	m_ButtonUp.SetIcon(hIconUp);
	m_ButtonDown.SetIcon(hIconDown);
	m_ButtonCopy.SetIcon(hIconCopy);

	// Display selected event's info
	RefreshDisplay();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

TDMFOBJECTSLib::IEventPtr CEventProperties::GetEventAt(long pnIndex)
{
	CWaitCursor* lpWaitCursor = NULL;
	TDMFOBJECTSLib::IEventPtr lIeventPtr;

	if (m_pDoc->GetSelectedReplicationGroup() != NULL)
	{
		if (!m_pDoc->GetSelectedReplicationGroup()->IsEventAt(pnIndex))
		{
			lpWaitCursor = new CWaitCursor;
		}
		lIeventPtr = m_pDoc->GetSelectedReplicationGroup()->GetEventAt(pnIndex);
	}
	else if (m_pDoc->GetSelectedServer() != NULL)
	{	
		if (!m_pDoc->GetSelectedServer()->IsEventAt(pnIndex))
		{
			lpWaitCursor = new CWaitCursor;
		}
		lIeventPtr = m_pDoc->GetSelectedServer()->GetEventAt(pnIndex);
	}
	else
	{
		if (!m_pDoc->m_pSystem->IsEventAt(pnIndex))
		{
			lpWaitCursor = new CWaitCursor;
		}
		lIeventPtr = m_pDoc->m_pSystem->GetEventAt(pnIndex);
	}

	if (lpWaitCursor)
	{
		delete lpWaitCursor;
	}

	return lIeventPtr;
}

bool CEventProperties::RefreshDisplay() 
{
	USES_CONVERSION;

	TDMFOBJECTSLib::IEventPtr pEvent = GetEventAt(m_nEventIndex);
	if (pEvent)
	{
		m_cstrType = OLE2A(pEvent->Type);
		m_cstrDate = OLE2A(pEvent->Date);
		m_cstrTime = OLE2A(pEvent->Time);
		
		if (pEvent->Source.length() != 0)
		{
			m_cstrSource = OLE2A(pEvent->Source);
		}
		else
		{
			m_cstrSource = OLE2A(m_pDoc->GetSelectedServer()->Name);
		}
		if (pEvent->GroupID >= 0) 
		{
			m_cstrGroupNumber.Format("%d", pEvent->GroupID);
		}
		else
		{
			m_cstrGroupNumber = "n/a";
		}
		if (pEvent->PairID >= 0)
		{
			m_cstrPairNumber.Format("%d", pEvent->PairID);
		}
		else
		{
			m_cstrPairNumber = "n/a";
		}

		m_cstrDescription = OLE2A(pEvent->Description);
		
		UpdateData(FALSE);

		UpdateButtonsStatus();

		return true;
	}

	return false;
}

void CEventProperties::OnButtonCopy() 
{
	CString cstrMsg;
	cstrMsg = m_cstrType + " " + m_cstrDate + " " + m_cstrTime + " " +
		m_cstrGroupNumber + " " + m_cstrPairNumber + " " + m_cstrDescription;

	if (OpenClipboard())
	{
		EmptyClipboard();

		// Allocate a global memory object for the text.
		HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, cstrMsg.GetLength() + 1);
		
		if (hglbCopy != NULL)
		{ 
			// Lock the handle and copy the text to the buffer.
			LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
			strcpy(lptstrCopy, cstrMsg);
			GlobalUnlock(hglbCopy);
			
			// Place the handle on the clipboard.
			SetClipboardData(CF_TEXT, hglbCopy);
		}
		
		CloseClipboard();
	}
}

void CEventProperties::OnButtonDown() 
{
	m_nEventIndex--;
	if (RefreshDisplay() == false) // put back original value
	{
		m_nEventIndex++;
	}
	else
	{
		if (m_pParent)
		{
			m_pParent->SendMessage(WM_EVENT_SELECTION_CHANGE, 1);
		}
	}
}

void CEventProperties::OnButtonUp()
{
	m_nEventIndex++;
	if (RefreshDisplay() == false) // put back original value
	{
		m_nEventIndex--;
	}
	else
	{
		if (m_pParent)
		{
			m_pParent->SendMessage(WM_EVENT_SELECTION_CHANGE, -1);
		}
	}
}

void CEventProperties::UpdateButtonsStatus()
{
	m_ButtonDown.EnableWindow(m_nEventIndex > 1);
	m_ButtonUp.EnableWindow(m_nEventIndex < m_nMaxIndex);
}

void CEventProperties::SetMaxEvent(UINT nEventMax)
{
	m_nMaxIndex = nEventMax;

	UpdateButtonsStatus();
}
