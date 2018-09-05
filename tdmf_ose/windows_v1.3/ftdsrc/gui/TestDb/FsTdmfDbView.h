// FsTdmfDbView.h : interface of the CFsTdmfDbView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FSTDMFDBVIEW_H__891163AD_F955_4EE3_A545_10AEB8A884D6__INCLUDED_)
#define AFX_FSTDMFDBVIEW_H__891163AD_F955_4EE3_A545_10AEB8A884D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "lib\LibDb\FsTdmfDb.h"

class CFsTdmfDbView : public CFormView
{
protected: // create from serialization only
	CFsTdmfDbView();
	DECLARE_DYNCREATE(CFsTdmfDbView)

public:
	//{{AFX_DATA(CFsTdmfDbView)
	enum { IDD = IDD_FSTDMFDB_FORM };
	long	m_Ctr1;
	long	m_Ctr2;
	long	m_iNuOfTest;
	//}}AFX_DATA

// Attributes
public:
	CFsTdmfDbDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFsTdmfDbView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFsTdmfDbView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFsTdmfDbView)
	afx_msg void OnButCreateTbl();
	afx_msg void OnButTc();
	afx_msg void OnButDropTbl();
	afx_msg void OnButStress();
	afx_msg void OnButFill();
	afx_msg void OnButCreateDb();
	afx_msg void OnButCreateLogins();
	afx_msg void OnButHelpDemo();
	afx_msg void OnButRandom();
	afx_msg void OnButCreateTbl2();
	afx_msg void OnButWalkThr();
	afx_msg void OnButWlkThr();
	afx_msg void OnButExFillsTbl2();
	afx_msg void OnButCrTblSql();
	afx_msg void OnButFast();
	afx_msg void OnButFast2();
	afx_msg void OnButCrScenario();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	FsTdmfDb* mpDb;

	void FvDbDel        ();
	BOOL FvDbTc1        (); // Test Cases 1, access Db
	BOOL FvDbTcStress   (); // Test Cases Stress
	BOOL FvFillsTbls    ();
	void FvWt           (); // WalkThrough
	void FvExFillsTbls1 (); // Example
	void FvExFillsTbls2 (); // Example
	void FvExSelect1    (); // Example
	void FvExSelect2    (); // Example

	void FvExEncap1       ( CString pszName, CString pszDesc, int piBabSize, int piWinSize );
	void OnButCrTblSqlEff ();

}; // class CFsTdmfDbView

#ifndef _DEBUG  // debug version in FsTdmfDbView.cpp
inline CFsTdmfDbDoc* CFsTdmfDbView::GetDocument()
   { return (CFsTdmfDbDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FSTDMFDBVIEW_H__891163AD_F955_4EE3_A545_10AEB8A884D6__INCLUDED_)
