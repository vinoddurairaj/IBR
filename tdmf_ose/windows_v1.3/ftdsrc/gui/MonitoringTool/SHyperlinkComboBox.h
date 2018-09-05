#if !defined(AFX_HYPERCOMBO_H__B68E532F_C9F4_4FAE_9681_FDEED3DAB8A4__INCLUDED_)
#define AFX_HYPERCOMBO_H__B68E532F_C9F4_4FAE_9681_FDEED3DAB8A4__INCLUDED_

// SHyperlinkComboBox version 0.06

/*

How to use it
=============
1- Insert SHyperlinkComboBox.cpp to your project.
2- Add #include "SHyperlinkComboBox.h" in the header file of your form.
3- Add a combobox to your form.
4- Set Style type of combobox to Drop List.
5- Double click on the combobox while holding ctrl key.
6- Add a name and set category to Control and Variable type to CComboBox
7- In the header file of your form change the variable type CComboBox for SHyperlinkComboBox.

Now you can use it as a normal combobox.

	
==== KNOWN BUGS ====
	- If the parent receive the SelChange message, you could be need to send a WM_PAINT to the control (m_Combo.PostMessage(WM_PAINT, 0, 0); )


==== BUGS FIXED ====
	- Some glitch appear if string width is too small (item of 1 or 2 letters)
		FIXED - 2001/11/14	Add a MinimumWidth that can be changed with SetMinimumWidth. The underline has the same witdh of the MinimumWidth,
												because if the underline is reduced too much, only a triangle or a part of the triangle will be shown.



What's new:

Version 0.06
============
	Improve enable/disable control
	FIXED: There are no visual difference when the control is enabled or not.

Version 0.05
============
	Add UNICODE Support
	Activate EnableWindow

Version 0.04
============
	very minor modifications

Version 0.03
============
	2001/11/14	Add a MinimumWidth that can be changed with SetMinimumWidth. The underline has the same witdh of the MinimumWidth,
							because if the underline is reduced too much, only a triangle or a part of the triangle will be shown. If the text
							is smaller than the underline, the text is centered.

	2001/11/14: Add Comments

*/


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// - Remove some hardcode value 
//
//
//
//
//


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HyperCombo.h : header file
//


const COLORREF DEFAULT_FORE_COLOR = RGB(0,0,210);
const long HOVER_TIMER_ID = 1;


static const enum ColorPickerMode               
{
	ONE_COLOR_MODE,
	THREE_COLOR_MODE	
};

// TODO: Add set/get function for these variables
const int TRI_SIZE = 5;
const int TRI_SIZE_WIDTH = 4;


/////////////////////////////////////////////////////////////////////////////
// SHyperlinkComboBox window

class SHyperlinkComboBox : public CComboBox
{
// Construction
public:
	SHyperlinkComboBox();

	// Text and underline color
	COLORREF	GetForeColor();
	void			SetForeColor(COLORREF color);

	// Background color Note: SetUsingBackColor must be set to true
	COLORREF	GetBackColor() ;
	void			SetBackColor(COLORREF color) ;


	//Return Value: The zero-based index of the item selected if the message is successful. The return value is CB_ERR 
	//							if nSelect is greater than the number of items in the list.
	//Parameters::  id Specifies the zero-based index of the string to select.
	int			SetCurSel(int id);

	// Send a paint message
	void			Refresh();

	// This method deletes a string in the SHyperlinkComboBox
	int				DeleteString(UINT nIndex);

	// Get/Set the Text and underline color when the cursor is passed over the control
	COLORREF GetHoverForeColor();
	void SetHoverForeColor(COLORREF color);


	//=====================================================================================================
	// Color Scheme Stuff : Mode is selected automatically when AddColor is called.
	//
	// Note:  "AddColor(long color)" and "AddColor(long colorA, long colorB, long colorC)" and 
	//				"AddString/InsertString" should not used together in the same control. All these fonctions are 
	//				 corresponding to a different mode. 
	//
	//=====================================================================================================
	// ONE_COLOR_MODE
	void AddColor(long color);
	COLORREF GetColor(int itemID);

	// THREE_COLOR_MODE
	void AddColor(long colorA, long colorB, long colorC);
	COLORREF GetColorA(int itemID);
	COLORREF GetColorB(int itemID);
	COLORREF GetColorC(int itemID);


	// Using or not using the background defined with SetBackColor
	void SetUsingBackColor(bool value);
	bool isUsingBackColor();


	// Set/Get the minimum with of the control to have a decent display
	long GetMinimumWidth();
	void SetMinimumWidth(long value);
	
	/*BOOL EnableWindow(BOOL value)
	{
		if (value) {
			m_CurrentForeColor = m_ForeColor;
		} else {
			m_CurrentForeColor = RGB(128, 128, 128);
		}
	
		return CWnd::EnableWindow(value);
	};*/


private:
	bool m_isColorPicker;
	bool m_ColorPickerMode;

	COLORREF m_ForeColor;
	COLORREF m_BackColor;
	COLORREF m_CurrentForeColor;

	bool m_isUsingBackColor;

	long		m_MinimumWidth;

// Hover stuff
	bool m_HoverTimerSet;
	COLORREF m_HoverForeColor;

	void DrawColorBox(CDC* pdc, long itemID, CRect rect, bool isFocus = false) ;
	void DrawUnderline(CDC* pDC, CRect rcBounds, bool isEnable = true);


// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SHyperlinkComboBox)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~SHyperlinkComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(SHyperlinkComboBox)
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnSelchange();
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HYPERCOMBO_H__B68E532F_C9F4_4FAE_9681_FDEED3DAB8A4__INCLUDED_)
