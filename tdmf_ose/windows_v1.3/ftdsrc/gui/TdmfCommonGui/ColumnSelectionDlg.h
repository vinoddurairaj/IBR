#if !defined(AFX_COLUMNSELECTIONDLG_H__A45F32F5_7BD3_44E2_B131_4696B866D824__INCLUDED_)
#define AFX_COLUMNSELECTIONDLG_H__A45F32F5_7BD3_44E2_B131_4696B866D824__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColumnSelectionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//

class CListViewColumnDef
{
public:
	UINT        m_nIndex;
	std::string m_strName;
	BOOL        m_bVisible;
	UINT        m_nWidth;
};

/////////////////////////////////////////////////////////////////////////////
// CColumnSelectionDlg dialog

class CColumnSelectionDlg : public CDialog
{
	std::vector<CListViewColumnDef>& m_vecColumnDef;

// Construction
public:
	CColumnSelectionDlg(std::vector<CListViewColumnDef>& vecColumnDef);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CColumnSelectionDlg)
	enum { IDD = IDD_COLUMN_SELECTION };
	CListCtrl	m_ListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColumnSelectionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CColumnSelectionDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLUMNSELECTIONDLG_H__A45F32F5_7BD3_44E2_B131_4696B866D824__INCLUDED_)
