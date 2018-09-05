#if !defined(AFX_TDMFDOCTEMPLATE_H__EABBA30B_83C7_4EE1_BF3C_D471E8041E4E__INCLUDED_)
#define AFX_TDMFDOCTEMPLATE_H__EABBA30B_83C7_4EE1_BF3C_D471E8041E4E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TdmfDocTemplate.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTdmfDocTemplate window

class CTdmfDocTemplate : public CSingleDocTemplate
{
	DECLARE_DYNAMIC(CTdmfDocTemplate)

protected:
	CWnd* m_pWndParent;

// Construction
public:
	CTdmfDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
		CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass)
		: CSingleDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass), m_pWndParent(NULL)
	{
		m_bAutoDelete = true;
	}
	
	virtual ~CTdmfDocTemplate() {}

	void    SetParentWnd(CWnd* pParentWindow) {m_pWndParent = pParentWindow;}

	virtual CFrameWnd* CreateNewFrame( CDocument* pDoc, CFrameWnd* pOther );
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFDOCTEMPLATE_H__EABBA30B_83C7_4EE1_BF3C_D471E8041E4E__INCLUDED_)
