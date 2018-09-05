// LoginDlg.cpp : implementation file
//


// Mike Pollett
#include "../../tdmf.inc"

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "LoginDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CEncodedBuffer

class CEncodedBuffer
{
private:
	LPBYTE m_rgByteBuffer;
	int    m_nSize;
	int    m_nKeyPos;
	BYTE   m_byteKey;

public:

	CEncodedBuffer(const CString& cstrPlainText) : 
		m_rgByteBuffer(NULL), m_nSize(0), m_nKeyPos(3), m_byteKey(0x66)
	{
		Encode(cstrPlainText);
	}

	CEncodedBuffer(const LPBYTE pbyteEncoded, int nSize) : 
		m_rgByteBuffer(NULL), m_nSize(0), m_nKeyPos(3), m_byteKey(0x66)
	{
		m_nSize        = nSize;
		m_rgByteBuffer = new BYTE[m_nSize];
		if (m_nKeyPos > (m_nSize-1))
		{
			m_nKeyPos = m_nSize-1;
		}

		if (m_rgByteBuffer != NULL)
		{
			memcpy(m_rgByteBuffer, pbyteEncoded, nSize);
		}
	}

	~CEncodedBuffer()
	{
		if (m_nSize > 0)
		{
			delete [] m_rgByteBuffer;

			m_nSize        = 0;
			m_rgByteBuffer = NULL;
		}
	}

	int Size() const
	{
		return m_nSize;
	}

	void Encode(const CString& cstrPlainText)
	{
		if (m_nSize > 0)
		{
			delete [] m_rgByteBuffer;

			m_nSize        = 0;
			m_rgByteBuffer = NULL;
		}

		m_nSize        = cstrPlainText.GetLength() + 1;
		m_rgByteBuffer = new BYTE[m_nSize];
		if (m_nKeyPos > (m_nSize-1))
		{
			m_nKeyPos = m_nSize-1;
		}

		if (m_rgByteBuffer != NULL)
		{
			for (int i = 0; i < m_nSize; i++)
			{
				if (i < m_nKeyPos)
				{
					m_rgByteBuffer[i] = cstrPlainText.GetAt(i) ^ m_byteKey;
				}
				else if (i == m_nKeyPos)
				{
					m_rgByteBuffer[i] = m_byteKey;
				}
				else
				{
					m_rgByteBuffer[i] = cstrPlainText.GetAt(i-1) ^ m_byteKey;
				}
			}
		}
	}

	CString Decode() const
	{
		CString cstrPlainText;

		if (m_nSize > 0)
		{
			for (int i = 0; i < m_nSize; i++)
			{
				if (i < m_nKeyPos)
				{
					cstrPlainText += BYTE(m_rgByteBuffer[i] ^ m_byteKey);
				}
				else if (i > m_nKeyPos)
				{
					cstrPlainText += BYTE(m_rgByteBuffer[i] ^ m_byteKey);
				}
			}
		}

		return cstrPlainText;
	}

	operator LPBYTE() const
	{
		return m_rgByteBuffer;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog

// Mike Pollett
#include "../../tdmf.inc"

CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	m_cstrPassword = _T("");
	m_cstrUserID = _T("");
	m_bSaveInfo = FALSE;
	m_cstrCollectorName = _T("");
	m_nAuthentication = 0;
	//}}AFX_DATA_INIT

	DWORD dwType;
	DWORD dwValue;
	ULONG nSize = sizeof(DWORD);
	#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"

	if (SHGetValue(HKEY_CURRENT_USER, FTD_SOFTWARE_KEY, "DtcSaveConnectionInfo", &dwType, &dwValue, &nSize) == ERROR_SUCCESS)
	{
		if (dwValue != 0)
		{
			m_bSaveInfo = TRUE;
			
			char szValue[512];
			nSize = 512;
			if (SHGetValue(HKEY_CURRENT_USER, FTD_SOFTWARE_KEY, "DtcConnectionData", &dwType, szValue, &nSize) == ERROR_SUCCESS)
			{
				CEncodedBuffer EncodedBuffer((LPBYTE)szValue, nSize);
				CString cstrConnectionData =  EncodedBuffer.Decode();
				int nIndex = cstrConnectionData.Find(';');
				m_nAuthentication = cstrConnectionData[0] - '0';
				m_cstrUserID      = cstrConnectionData.Mid(1, nIndex-1);
				m_cstrPassword    = cstrConnectionData.Mid(nIndex+1);
			}
		}

		// Load only first collector name
		char  szValue[512];
		nSize = 512;
		if (SHGetValue(HKEY_CURRENT_USER, FTD_SOFTWARE_KEY, "DtcCollectorNames", &dwType, szValue, &nSize) == ERROR_SUCCESS)
		{
			CString cstrCollectorName = szValue;
			int nIndex = cstrCollectorName.Find(';');

			if (nIndex > 0)
			{
				cstrCollectorName = cstrCollectorName.Left(nIndex);
			}
			cstrCollectorName.TrimLeft('\"');
			cstrCollectorName.TrimRight('\"');

			m_cstrCollectorName = cstrCollectorName;
		}
		
		if (m_cstrCollectorName.IsEmpty())
		{
			// Read DB Server from registry
			char szCollectorName[MAX_PATH];
			DWORD dwType;
			ULONG nSize = MAX_PATH;
			#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
			SHGetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "DtcDbServer", &dwType, szCollectorName, &nSize);
			
			m_cstrCollectorName = szCollectorName;
			m_cstrCollectorName = m_cstrCollectorName.Left(m_cstrCollectorName.Find('\\'));
		}
	}
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Control(pDX, IDC_APP_ICON, m_CtrlAppIcon);
	DDX_Control(pDX, IDC_EDIT_USERID, m_EditUserId);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_EditPassword);
	DDX_Control(pDX, IDC_COMBO_COLLECTOR_NAME, m_ComboBoxCollector);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_cstrPassword);
	DDX_Text(pDX, IDC_EDIT_USERID, m_cstrUserID);
	DDX_Check(pDX, IDC_CHECK_SAVE, m_bSaveInfo);
	DDX_CBString(pDX, IDC_COMBO_COLLECTOR_NAME, m_cstrCollectorName);
	DDX_Radio(pDX, IDC_AUTHENTICATION, m_nAuthentication);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	ON_BN_CLICKED(IDC_CHECK_SAVE, OnCheckSave)
	ON_BN_CLICKED(IDC_AUTHENTICATION, OnAuthentication)
	ON_BN_CLICKED(IDC_AUTHENTICATION_MSDE, OnAuthenticationMsde)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers

BOOL CLoginDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	DWORD dwType;
	char  szValue[512];
	DWORD nSize = 512;
	if (SHGetValue(HKEY_CURRENT_USER, FTD_SOFTWARE_KEY, "DtcCollectorNames", &dwType, szValue, &nSize) == ERROR_SUCCESS)
	{
		CString cstrCollectorList = szValue;

		while (!cstrCollectorList.IsEmpty())
		{
			CString cstrCollectorName;
			int nIndex = cstrCollectorList.Find(';');

			if (nIndex > 0)
			{
				cstrCollectorName = cstrCollectorList.Left(nIndex);
				cstrCollectorList.Delete(0, cstrCollectorName.GetLength() + 1);
			}
			else
			{
				cstrCollectorName = cstrCollectorList;
				cstrCollectorList.Empty();
			}

			cstrCollectorName.TrimLeft('\"');
			cstrCollectorName.TrimRight('\"');
			m_ComboBoxCollector.AddString(cstrCollectorName);
		}

		m_ComboBoxCollector.GetLBText(0, m_cstrCollectorName);
		UpdateData(FALSE);
	}

	if (m_cstrCollectorName.IsEmpty())
	{
		// Read DB Server from registry
		char szCollectorName[MAX_PATH];
		DWORD dwType;
		ULONG nSize = MAX_PATH;
		#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
		SHGetValue(HKEY_LOCAL_MACHINE, FTD_SOFTWARE_KEY, "DtcDbServer", &dwType, szCollectorName, &nSize);

		m_cstrCollectorName = szCollectorName;
		m_cstrCollectorName = m_cstrCollectorName.Left(m_cstrCollectorName.Find('\\'));
	}


	if (m_nAuthentication == 0)
	{
		m_EditUserId.EnableWindow(FALSE);
		m_EditPassword.EnableWindow(FALSE);

		m_cstrUserID   = "";
		m_cstrPassword = "";
		UpdateData(FALSE);
	}

	CString cstrTitle = theApp.m_ResourceManager.GetFullProductName();
	cstrTitle += " Login";
	SetWindowText(cstrTitle);

	m_CtrlAppIcon.SetIcon(theApp.m_ResourceManager.GetApplicationIcon());

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoginDlg::OnCheckSave() 
{
}

void CLoginDlg::OnOK() 
{
	UpdateData();

	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
					   FTD_SOFTWARE_KEY,
					   NULL,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey,
					   NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, "DtcSaveConnectionInfo", 0, REG_DWORD, (LPBYTE)&m_bSaveInfo, sizeof(m_bSaveInfo));

		if (m_bSaveInfo)
		{
			CString cstrConnectionData;
			cstrConnectionData.Format("%d%s;%s", m_nAuthentication, m_cstrUserID, m_cstrPassword);
			CEncodedBuffer EncodedBuffer(cstrConnectionData);
			RegSetValueEx(hKey, "DtcConnectionData", 0, REG_BINARY, (LPBYTE)EncodedBuffer, EncodedBuffer.Size());
		}
		else
		{
			RegSetValueEx(hKey, "DtcConnectionData", 0, REG_BINARY, (LPBYTE)"", 1);
		}

		CString cstrCollectorList = "\"" + m_cstrCollectorName + "\"";
		CString cstrCollectorName;

		for (int i = 0; i < m_ComboBoxCollector.GetCount(); i++)
		{
			m_ComboBoxCollector.GetLBText(i, cstrCollectorName);
			cstrCollectorName = "\"" + cstrCollectorName + "\"";

			if (cstrCollectorList.Find(cstrCollectorName) == -1)
			{
				cstrCollectorList += ";";
				cstrCollectorList += cstrCollectorName;
			}
		}
		RegSetValueEx(hKey, "DtcCollectorNames", 0, REG_SZ, (LPBYTE)((LPCSTR)cstrCollectorList), cstrCollectorList.GetLength());

		RegFlushKey(hKey);
		RegCloseKey(hKey);
	}
	
	CDialog::OnOK();
}

void CLoginDlg::OnAuthentication() 
{
	// Disable Login/password
	m_nAuthentication = 0;
	m_cstrUserID      = "";
	m_cstrPassword    = "";
	UpdateData(FALSE);

	m_EditUserId.EnableWindow(FALSE);
	m_EditPassword.EnableWindow(FALSE);
}

void CLoginDlg::OnAuthenticationMsde() 
{
	// Enable Login/password
	m_EditUserId.EnableWindow();
	m_EditPassword.EnableWindow();	
}
