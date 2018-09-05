#if !defined(AFX_SHYPERLINK_H__8435315A_F0CB_4044_B30B_779ABC0CC441__INCLUDED_)
#define AFX_SHYPERLINK_H__8435315A_F0CB_4044_B30B_779ABC0CC441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// SHyperlink version 0.03

/*
How to use it
=============
1- Insert SHyperlink.cpp to your project.
2- Add #include "SHyperlink.h" in the header file of your form.
3- Add a static control to your form and change the ID name.
4- Set SS_NOTIFY(notify) Style.
5- Double click on the static control while holding ctrl key.
6- Add a name and set category to Control and Variable type to CStatic
7- In the header file of your form change the variable type CStatic for SHyperlink.


How to convert a SHyperlinkComboBox to a SHyperlink
===================================================

Runtime creation:
------------------------
- Add SS_NOTIFY style (IMPORTANT)
- Remove CBS_DROPDOWNLIST style and others style that are not approriate
- The "Create" prototype definition is different (need to add an empty string as first parameter)
- Probably the height must be changed.

Resource creation:
------------------------
- A CStatic control must be used in the resourse instead of a CComboBox 
- Add SS_NOTIFY style (IMPORTANT)
- Strings add to the combobox from resource must be added at runtime.



==== KNOWN BUGS ====


=====WHAT'S NEW=====:

	Version 0.03
	============
	Improve enable/disable control

	Version 0.02
	============
	Add UNICODE Support



*/


#include <afxtempl.h>

const long WM_S_HL_CHANGED =	WM_USER + 235;

const int HL_MODE = 0;
const int BUTTON_MODE = 1;

class SHyperlink : public CStatic
{
// Construction
public:
	SHyperlink();

	// Using or not using the background defined with SetBackColor
	void SetUsingBackColor(bool value);
	bool isUsingBackColor();
	
	// Text and underline color
	COLORREF GetForeColor();
	void SetForeColor(COLORREF color);

	// Background color Note: SetUsingBackColor must be set to true
	COLORREF GetBackColor() ;
	void SetBackColor(COLORREF color);

	// Get/Set the Text and underline color when the cursor is passed over the control
	COLORREF GetHoverForeColor();
	void SetHoverForeColor(COLORREF color);

	long GetUserData();
	void SetUserData(long UserData);

	long GetCurSel();
	void SetCurSel(long CurSel);

	void GetLBText(int nIndex, CString& rString);

	CString GetMenuTitle();
	void SetMenuTitle(CString sMenuTitle);

	int GetCount();

	static void AddMenuTitle(CMenu* popup, LPCTSTR title);

	void AddString(LPCTSTR value);

	// Send a paint message
	void Refresh();

	void ResetContent();

	void SetWindowText(LPCTSTR value);

	// Deprecated, LoadBitmaps must be used instead
	void SetBitmaps(UINT nIDBitmapResource, UINT nIDBitmapResourceSel = 0, UINT nIDBitmapResourceFocus = 0, UINT nIDBitmapResourceDisabled = 0 );

	BOOL LoadBitmaps(UINT nIDBitmapResource, UINT nIDBitmapResourceSel = 0, UINT nIDBitmapResourceFocus = 0, UINT nIDBitmapResourceDisabled = 0 );

	// HL_MODE or BUTTON_MODE
	void SetOperationMode(int value);

	BOOL EnableWindow(BOOL value)
	{
		BOOL ret = CWnd::EnableWindow(value);
		if (m_OperationMode == BUTTON_MODE)
		{
			m_Button.EnableWindow(value);
		}

		Refresh();
		return ret;
	};

private:
	COLORREF	m_BackColor;
	COLORREF	m_ForeColor;
	COLORREF	m_CurrentForeColor;

	CBitmapButton m_Button;

	bool m_isUsingBackColor;

	int m_OperationMode;

	long m_UserData;
	long m_CurSel;

	// Hover stuff
	bool			m_HoverTimerSet;
	COLORREF	m_HoverForeColor;

	CString m_sMenuTitle;

	long GetNBSeparator(int pos = -1);

	long ConvertPos(long pos);

	void DrawUnderline(CDC* pDC, CRect rcBounds, bool isEnable = true);

	CArray<CString, LPCTSTR> m_arrayString;

	void OnFileMenuItems(UINT nID);

	void OnButtonClicked();

	UINT m_nIDBitmapResource; 
	UINT m_nIDBitmapResourceSel; 
	UINT m_nIDBitmapResourceFocus;
	UINT m_nIDBitmapResourceDisabled;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SHyperlink)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~SHyperlink();

	// Generated message map functions
protected:
	//{{AFX_MSG(SHyperlink)
	afx_msg void OnPaint();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg BOOL OnClicked();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHYPERLINK_H__8435315A_F0CB_4044_B30B_779ABC0CC441__INCLUDED_)
