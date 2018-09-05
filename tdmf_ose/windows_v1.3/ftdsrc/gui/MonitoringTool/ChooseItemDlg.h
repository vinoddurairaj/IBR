#if !defined(AFX_CHOOSEITEMDLG_H__362EF182_615E_4DAF_84CA_3D2406023C9C__INCLUDED_)
#define AFX_CHOOSEITEMDLG_H__362EF182_615E_4DAF_84CA_3D2406023C9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseItemDlg.h : header file
//
#include <afxtempl.h>
/////////////////////////////////////////////////////////////////////////////
// CChooseItemDlg dialog

class CChooseItemDlg : public CDialog
{
// Construction
public:
	CChooseItemDlg(CWnd* pParent = NULL);   // standard constructor
   CChooseItemDlg(CStringArray* p,int nType, CString* pstrResult, CWnd* pParent = NULL);   

// Dialog Data
	//{{AFX_DATA(CChooseItemDlg)
	enum { IDD = IDD_CHOOSEITEM_DLG };
	CListBox	m_List_Item;
	//}}AFX_DATA

	int		m_nType; //type 1 = Server,2 = Group, 3 = pair
   CStringArray* m_pArray;
	CArray<int,int> m_aryIndex;
	CString*	m_pstrResult;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChooseItemDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	BOOL LoadTheListBox();
	// Generated message map functions
	//{{AFX_MSG(CChooseItemDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSEITEMDLG_H__362EF182_615E_4DAF_84CA_3D2406023C9C__INCLUDED_)
