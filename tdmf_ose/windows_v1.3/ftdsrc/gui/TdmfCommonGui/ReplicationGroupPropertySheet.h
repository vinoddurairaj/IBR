#if !defined(AFX_REPLICATIONGROUPPROPERTYSHEET_H__E4CEEDE4_8632_41DA_8E13_69CDFF5FBF42__INCLUDED_)
#define AFX_REPLICATIONGROUPPROPERTYSHEET_H__E4CEEDE4_8632_41DA_8E13_69CDFF5FBF42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReplicationGroupPropertySheet.h : header file
//

#include "RGGeneralPage.h"
#include "RGTunablePage.h"
#include "RGReplicationPairsAdmPage.h"
#include "RGThrottlePage.h"
#include "RGSymmetricPage.h"
#include "TdmfCommonGuiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// CReplicationGroupPropertySheet

class CReplicationGroupPropertySheet : public CPropertySheet
{
private:
	CRGGeneralPage			   m_RGGeneralPage;
	CRGTunablePage             m_RGTunablePage;
	CRGReplicationPairsAdmPage m_RGReplicationPairsAdmPage;
	CRGThrottlePage			    m_RGThrottlePage;
	CRGSymmetricPage           m_RGSymmetricPage;

	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;
	BOOL                                 m_bNewItem;

	DECLARE_DYNAMIC(CReplicationGroupPropertySheet)

// Construction
public:
	CTdmfCommonGuiDoc*         m_pDoc;

	CReplicationGroupPropertySheet(	CTdmfCommonGuiDoc* pDoc, UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0, TDMFOBJECTSLib::IReplicationGroup* pRG = NULL, bool bNewItem = false, bool bNewSymmetric = false);
	CReplicationGroupPropertySheet(	CTdmfCommonGuiDoc* pDoc, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0, TDMFOBJECTSLib::IReplicationGroup* pRG = NULL, bool bNewItem = false, bool bNewSymmetric = false);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReplicationGroupPropertySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CReplicationGroupPropertySheet();
    BOOL IsInChainingMode();
	// Generated message map functions
protected:
	//{{AFX_MSG(CReplicationGroupPropertySheet)
	afx_msg void OnApplyNow();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPLICATIONGROUPPROPERTYSHEET_H__E4CEEDE4_8632_41DA_8E13_69CDFF5FBF42__INCLUDED_)
