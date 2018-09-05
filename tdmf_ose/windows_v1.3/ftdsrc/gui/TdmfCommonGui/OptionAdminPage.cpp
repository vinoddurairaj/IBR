// OptionAdminPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "OptionAdminPage.h"
#include "LogViewerDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionAdminPage property page

IMPLEMENT_DYNCREATE(COptionAdminPage, CPropertyPage)

COptionAdminPage::COptionAdminPage(CTdmfCommonGuiDoc* pDoc) :
	 CPropertyPage(COptionAdminPage::IDD), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(COptionAdminPage)
	m_bLog = FALSE;
	//}}AFX_DATA_INIT
}

COptionAdminPage::~COptionAdminPage()
{
}

void COptionAdminPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionAdminPage)
	DDX_Check(pDX, IDC_CHECK_LOG, m_bLog);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionAdminPage, CPropertyPage)
	//{{AFX_MSG_MAP(COptionAdminPage)
	ON_BN_CLICKED(IDC_BUTTON_VIEW_LOG, OnButtonViewLog)
	ON_BN_CLICKED(IDC_BUTTON_VIEW_KEYLOG, OnButtonViewKeylog)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT_KEYLOG, OnButtonExportKeylog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionAdminPage message handlers

BOOL COptionAdminPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_bLog = m_pDoc->m_pSystem->LogUsersActions;
	UpdateData(FALSE);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL COptionAdminPage::OnApply() 
{
	UpdateData();

	m_pDoc->m_pSystem->LogUsersActions = m_bLog;

	return CPropertyPage::OnApply();
}

void COptionAdminPage::OnButtonViewLog() 
{
	CLogViewerDlg LogViewer(m_pDoc);
	LogViewer.DoModal();
}

void COptionAdminPage::OnButtonViewKeylog() 
{
	CLogViewerDlg LogViewer(m_pDoc, FALSE);
	LogViewer.DoModal();	
}

void COptionAdminPage::OnButtonExportKeylog() 
{
	USES_CONVERSION;

	CFileDialog FileDlg(FALSE,
						"log",
						NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						"Log Files (*.log)|*.log|All Files (*.*)|*.*||",
						this);

	if (FileDlg.DoModal() == IDOK)
	{
		CString cstrFile = FileDlg.GetPathName();

		std::ofstream ofs(cstrFile);

		CComBSTR bstrDate;
		CComBSTR bstrHostname;
		long     nHostId;
		CComBSTR bstrKey;
		CComBSTR bstrExpirationDate;

		ofs << "\n";
		ofs << theApp.m_ResourceManager.GetFullProductName();
		ofs << " Registration Keys Log (" << (char*)m_pDoc->m_pSystem->GetName() << ")\n";
		ofs << "\n";
		ofs << "        Date                Hostname          Host Id                  Key                   Expiration Date    \n";
		ofs << "--------------------- --------------------- ------------ -------------------------------- ----------------------\n";
		ofs << "\n";
		UINT     nLineCount = 6;

		UINT     nNbEntries = 0;
		m_pDoc->m_pSystem->GetFirstKeyLogMsg(&bstrDate, &bstrHostname, &nHostId, &bstrKey, &bstrExpirationDate);
		while (bstrDate.Length() > 0)
		{
			ofs << " " << std::setw(20) << std::left << OLE2A(bstrDate) << " " 
				<< " " << std::setw(20) << OLE2A(bstrHostname) << " "
				<< " " << "0x" << std::uppercase << std::hex << std::right << std::setfill('0') << std::setw(8) << nHostId << "  " << std::left
				<< " " << std::setw(31) << OLE2A(bstrKey) << " " 
				<< " " << std::setw(21) << OLE2A(bstrExpirationDate) << "\n";
			nLineCount++;
			nNbEntries++;

			m_pDoc->m_pSystem->GetNextKeyLogMsg(&bstrDate, &bstrHostname, &nHostId, &bstrKey, &bstrExpirationDate);
		}

		ofs << "\n";
		ofs << "----------------------------------------------------------------------------------------------------------------\n";
		ofs << "\n";
		ofs << std::dec << nNbEntries << "/" << nNbEntries <<" Entries";
		ofs << "        " << (LPCTSTR)CTime::GetCurrentTime().Format("%c");
		nLineCount += 3;

		std::streampos Pos = ofs.tellp();
		UINT nNbChars = (UINT)Pos - (2 * nLineCount) + 34; // Remove line change chars (2 chars per line); +34 chars for the end of the signature
		ofs << "         #" << std::setw(24) << nNbChars << "\n";
	}
}
