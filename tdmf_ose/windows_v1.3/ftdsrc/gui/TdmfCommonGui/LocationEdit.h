#if !defined(AFX_LOCATIONEDIT_H__90B3CA64_B299_476B_A6D0_A374BCFF50C0__INCLUDED_)
#define AFX_LOCATIONEDIT_H__90B3CA64_B299_476B_A6D0_A374BCFF50C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LocationEdit.h : header file
//

#include "tdmfcommonguiDoc.h"


/////////////////////////////////////////////////////////////////////////////
// CLocationEdit window

class CLocationEdit : public CEdit
{
// Construction
public:
	CLocationEdit();

// Attributes
protected:
	CButton                    m_ButtonBrowse;
	CTdmfCommonGuiDoc*         m_pDoc;
	TDMFOBJECTSLib::IServerPtr m_pServer;


public:

// Operations
public:
	void Initialize(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IServer* pServer)
	{
		m_pDoc     = pDoc;
		m_pServer  = pServer;
	}

	void CreateBrowseButton()
	{
		if ((m_pDoc != NULL) && (m_pServer != NULL) && (m_ButtonBrowse.m_hWnd == NULL))	
		{
			CRect Rect;
			GetClientRect(&Rect);
			
			CRect RectButton(Rect);
			RectButton.left = Rect.right - 20;

			Rect.right -= 20;
			SetRect(Rect);

			m_ButtonBrowse.Create("...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, RectButton, this, IDC_BUTTON_BROWSE);
		}
	}

	void DestroyBrowseButton()
	{
		if ((GetFocus() != &m_ButtonBrowse) && m_ButtonBrowse.m_hWnd != NULL)	
		{
			CRect Rect;
			GetClientRect(&Rect);
			SetRect(Rect);

			m_ButtonBrowse.DestroyWindow();
		}
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLocationEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLocationEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLocationEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	afx_msg void OnBrowseButton();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCATIONEDIT_H__90B3CA64_B299_476B_A6D0_A374BCFF50C0__INCLUDED_)
