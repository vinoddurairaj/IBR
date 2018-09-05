#if !defined(AFX_SEVERSELECTIONDIALOG_H__DE25427B_04AC_4200_87CF_95484295E684__INCLUDED_)
#define AFX_SEVERSELECTIONDIALOG_H__DE25427B_04AC_4200_87CF_95484295E684__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SeverSelectionDialog.h : header file
//


#include "TdmfCommonGuiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// CSeverSelectionDialog dialog

class CSeverSelectionDialog : public CDialog
{
private:
	TDMFOBJECTSLib::IDomainPtr m_pDomain;
	CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	CSeverSelectionDialog(TDMFOBJECTSLib::IDomain* pDomain, CTdmfCommonGuiDoc* pDoc, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSeverSelectionDialog)
	enum { IDD = IDD_SERVER_SELECTION };
	CListCtrl	m_ListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeverSelectionDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSeverSelectionDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDeleteitemListServer(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void GetServerDependencies(TDMFOBJECTSLib::IServerPtr pServer, std::map<long, CAdapt<TDMFOBJECTSLib::IServerPtr> >& mapServerDependent);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEVERSELECTIONDIALOG_H__DE25427B_04AC_4200_87CF_95484295E684__INCLUDED_)
