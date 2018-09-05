#if !defined(AFX_SVDColumnSelector_H__A8B1E941_B756_11D5_94FF_000086386252__INCLUDED_)
#define AFX_SVDColumnSelector_H__A8B1E941_B756_11D5_94FF_000086386252__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <afxtempl.h>		// carray
#include "SVColumn.h"
#include "SHyperlink.h"

// SVDColumnSelector.h : header file
//

enum _buttonType
{
	btPlain,
	btDropdown,
	btOpen
};

/////////////////////////////////////////////////////////////////////////////
// SVDColumnSelector dialog

class SVDColumnSelector : public CDialog
{
// Construction
public:
	void AddRows();
	void AddRow(int nCol);
	void RedrawControl(int nCtlID);
	SVDColumnSelector(CArray<SVColumn, SVColumn>* p, int nStartColumn = -1, CWnd* pParent = NULL);   // standard constructor
	~SVDColumnSelector();
// Dialog Data
	//{{AFX_DATA(SVDColumnSelector)
	enum { IDD = IDD_V_COLUMNSELECTOR };
	SHyperlink	m_shCurrentDate;
	SHyperlink	m_shWithinDate;
	SHyperlink	m_shGraph2;
	SHyperlink	m_shRuleSuffix;
	SHyperlink	m_shConditionC;
	SHyperlink	m_shConditionN;
	SHyperlink	m_shGraph;
	SHyperlink	m_shUnits;
	CListBox	m_lbColumns;
	CButton	m_btnColor;
	CString	m_csProductName;
	BOOL	m_bShowNum;
	BOOL	m_bShowGraph;
	BOOL	m_bRadioJLeft;
	BOOL	m_bRadioJCenter;
	BOOL	m_bRadioJRight;
	BOOL	m_bHighlightCondition;
	double	m_dNumValue;
	CString	m_csCharValue;
	BOOL	m_bHighlightCondition2;
	BOOL	m_bShowGraph2;
	UINT	m_nDateValue;
	int		m_nDateStyle;
	BOOL	m_bIgnoreCase;
	BOOL	m_bShowDateText;
	int		m_nRadioColval;
	CString	m_csColumnName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SVDColumnSelector)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddJustify(SVColumn* pCol);
	HICON m_hIcon3Values; 
	HICON m_hIcon3Bars; 
	HICON m_hIcon1Bar; 
	int m_nGraphResourceID;
	COLORREF m_clrSelectedLine;
	COLORREF m_clrNotSelectedLine;
	void SetCheckBoxes();
	void SetJustifyButton(int nJustify, int nTextFormat);
	void DrawButtonRuleColor( LPDRAWITEMSTRUCT lpDrawItemStruct);
	void DrawListviewColumn( LPDRAWITEMSTRUCT lpDrawItemStruct);

	int m_nStartColumn;
	int m_nPreviousItem;
	int GetPreviousItem() {return m_nPreviousItem;}
	int m_nSelectedItem;
	int GetSelectedItem() {return m_nSelectedItem;}
	COLORREF AskColor(COLORREF clr);
	void SetControls(int nItem);
	CListCtrl* m_pLCColumns;
	void InitCheckBox();
	void TermCheckBox();
	bool CopyCheckBox(CDC* pToDC, CRect* pcrTo, int nImage);
	CBitmap *m_pbmCheckOn, *m_pbmCheckOff;				// bitmap for tape volume
    BITMAP m_bmpInfoCheckOn, m_bmpInfoCheckOff;		// info for tape volume

	CImageList* m_pImage;
	bool m_bDirty;
	CArray<SVColumn, SVColumn>* m_pColumn;
	int ColumnUnitToItem(_UnitType ut);
	void SetUnitTypes();
	_UnitType SelectedUnitToColumn(long nID);
	void SetUnitFromColumn();
	void OnClicked(WPARAM wParam, LPARAM lParam);
	int m_nClickedID;
	int ColumnGraphToItem(_GraphicalDisplayType gdt);
	_GraphicalDisplayType SelectedGraphToColumn(long nID, bool bNumber);
	void OnUnitTypeSelected();
	void OnGraphStyleSelected();
	void SetGraph();
	void SetGraphFromColumn();
	void SetConditionN();
	void SetConditionC();
	void OnConditionNumSelected();
	_RuleType SelectedRuleNumToColumn(long nID);
	int ColumnRuleNumToItem(_RuleType rt);
	void SetConditionFromColumn();
	void SetRuleSuffix();
	void OnRuleSuffixSelected();
	void SetRuleSuffixFromColumn();
	int ColumnRuleSuffixToItem(_RuleSuffix rs);
	_RuleSuffix SelectedRuleSuffixToColumn(long nID);
	double RuleSuffixToMultiple(_RuleSuffix rs);
	void OnConditionCharSelected();
	_RuleType SelectedRuleCharToColumn(long nID);
	int ColumnRuleCharToItem(_RuleType rt);
	void SetDate();
	void SetDateRuleFromColumn();
	int ColumnRuleDateWithToItem(_RuleType rt);
	_RuleType SelectedRuleDateWithToColumn(long nID);
	int ColumnRuleDateCurToItem(_RuleType rt);
	_RuleType SelectedRuleDateCurToColumn(long nID);
	void OnWithinDateSelected();
	void OnCurrentDateSelected();

	// Generated message map functions
	//{{AFX_MSG(SVDColumnSelector)
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct); 
	afx_msg void OnButtonRuleColor();
	virtual BOOL OnInitDialog();
	afx_msg void OnClickListviewColumn(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListviewColumn(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCheckShownum();
	afx_msg void OnCheckShowgraph();
	afx_msg void OnRadioJcenter();
	afx_msg void OnRadioJleft();
	afx_msg void OnRadioJright();
	afx_msg void OnUnits();
	afx_msg void OnGraph();
	afx_msg void OnConditionNum();
	afx_msg void OnConditionChar();
	afx_msg void OnCheckFilter();
	afx_msg void OnCheckFilter2();
	afx_msg void OnRulesuffix();
	afx_msg void OnKillfocusNumvalue();
	afx_msg void OnKillfocusCharvalue();
	afx_msg void OnChangeCharvalue();
	afx_msg void OnChangeNumvalue();
	afx_msg void OnCheckShowgraph2();
	afx_msg void OnRadioDate();
	afx_msg void OnWithindate();
	afx_msg void OnCurrentdate();
	afx_msg void OnKillfocusDatevalue();
	afx_msg void OnChangeDatevalue();
	afx_msg void OnRadioBooleanFalseCircle();
	afx_msg void OnRadioBooleanFalseBlank();
	afx_msg void OnRadioSubcolumns3values();
	afx_msg void OnRadioSubcolumns3bars();
	afx_msg void OnRadioSubcolumns1bar();
	afx_msg void OnCheckIgnoreCase();
	afx_msg void OnCheckShowdatetext();
	afx_msg void OnHelp();
	afx_msg void OnRadioColvalVal();
	afx_msg void OnRadioColvalCol();
	afx_msg void OnHelpCharStd();
	afx_msg void OnHelpNumStd();
	afx_msg void OnHelpNumNograph();
	afx_msg void OnHelpNumSubcolumns();
	afx_msg void OnHelpNumValuecolumn();
	afx_msg void OnHelpDateStd();
	afx_msg void OnHelpBoolStd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVDColumnSelector_H__A8B1E941_B756_11D5_94FF_000086386252__INCLUDED_)
