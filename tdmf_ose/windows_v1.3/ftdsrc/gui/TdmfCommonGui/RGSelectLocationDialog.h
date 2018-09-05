#if !defined(AFX_RGSELECTLOCATIONDIALOG_H__B2013DE8_BA32_4EC2_96FB_18A71E6F85A5__INCLUDED_)
#define AFX_RGSELECTLOCATIONDIALOG_H__B2013DE8_BA32_4EC2_96FB_18A71E6F85A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGSelectLocationDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRGSelectLocationDialog dialog

class CRGSelectLocationDialog : public CDialog
{
// Construction
public:
	CRGSelectLocationDialog(CWnd* pParent = NULL);   // standard constructor

	void AddString(LPCSTR pszLocation)
	{
		m_vecLocation.push_back(pszLocation);
	}

	BOOL GetSelection(CString& cstrLocation)
	{
		if (m_cstrLocation.GetLength() > 0)
		{
			cstrLocation = m_cstrLocation;
			return TRUE;
		}

		return FALSE;
	}

// Dialog Data
	//{{AFX_DATA(CRGSelectLocationDialog)
	enum { IDD = IDD_RG_SELECT_LOCATION_DIALOG };
	CListBox	m_ListLocation;
	CString	m_cstrLocation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRGSelectLocationDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	std::vector<CString> m_vecLocation;

	// Generated message map functions
	//{{AFX_MSG(CRGSelectLocationDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGSELECTLOCATIONDIALOG_H__B2013DE8_BA32_4EC2_96FB_18A71E6F85A5__INCLUDED_)
