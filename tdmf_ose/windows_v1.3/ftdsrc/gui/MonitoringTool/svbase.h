#if !defined(AFX_SVBASE_H__B0DB8B8F_6712_47FB_B4D7_CE68365A61A8__INCLUDED_)
#define AFX_SVBASE_H__B0DB8B8F_6712_47FB_B4D7_CE68365A61A8__INCLUDED_

#include "MonitorRes.h"
#include "PageView.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_CHAR_LEN 1000


#define MESSAGE_DETAIL WM_USER+0	// needs to be moved to a common place

#define DESCENDING false
#define ASCENDING true


class IEnumeration;

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif
#include <afxcview.h>		// clistview
#include <afxtempl.h>		// carray
#include "SVColumn.h"
#include "SVSheet.h"
#include "SVFormatNumber.h"

//#include "SSpaceManager.h"
//using namespace SoftekSpaceManagement;
//#include "SMCommon.h"
#include "SVHeaderCtrl.h"

// start of copy book
	// export parameters
#define DELIMCOMMA		0
#define DELIMTAB		1
#define DELIMDEFAULT	DELIMCOMMA

#define SHOWALL			0
#define SHOWREVEALED	1
#define SHOWSELECTED	2
#define SHOWDEFAULT		SHOWREVEALED

#define DEFAULTHEADINGS true

// end of copy book
struct stCell
{
	CString csValue;
	
	stCell()
	{
		csValue = "";
	}
};
struct stRow
{

	CArray <stCell, stCell> caCells;
	bool bVisible;
	LPVOID pListViewBase;
	stRow()
	{
		caCells.RemoveAll();
		bVisible = true;
	}
};
	

struct stSortParm
{
	int nColumnNumber;
	void* p;
};

// SVBase.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SVBase form view
#define TESTDATESPAN 45

class SVBase : public CPageView
{	
	

protected:
#ifdef _DEBUG
	static int m_nCtr;
#endif
 
	virtual __int64 GetLowestValueInt64(int nColumn) {return 0;}
	virtual __int64 GetHighestValueInt64(int nColumn){return 0;}
	virtual int GetLowestValueInt(int nColumn){return 0;}
	virtual int GetHighestValueInt(int nColumn){return 0;}
	virtual double GetLowestValueDouble(int nColumn){return 0;}
	virtual double GetHighestValueDouble(int nColumn){return 0;}
	virtual COleDateTime GetLowestValueDateTime(int nColumn)
		{
			COleDateTimeSpan dts(TESTDATESPAN, 0, 0, 0);
			return COleDateTime::GetCurrentTime() - dts;
		}
	virtual COleDateTime GetHighestValueDateTime(int nColumn)
		{
			return COleDateTime::GetCurrentTime();
		}

	virtual bool GetGroupCountsInt(UINT nObjectNumber, int nColumn, int nCount, int* pIntTable)  {ASSERT (false);return false;}
	virtual bool GetGroupCountsDouble(UINT nObjectNumber, int nColumn, int nCount, double* pDoubleTable) {ASSERT (false);return false;}
	virtual bool GetGroupCounts64(UINT nObjectNumber, int nColumn, int nCount, __int64* pInt64Table) {ASSERT (false);return false;}
	virtual bool GetGroupCountsDateTime(UINT nObjectNumber, int nColumn, int nCount, COleDateTime* pDateTimeTable) {ASSERT (false);return false;}

public:
	SVHeaderCtrl m_HeaderCtrl;
	void LaunchColumnSelector(int nCol = -1);

	int HideSelected();
	int HideNotSelected(); 
	int RevealAll(bool bReSort = true);
	int SelectAll(bool bVisibleOnly = true);
	int SelectMatchedItems();
	POSITION GetFirstSelectedItem();
	int GetNextSelectedItem(POSITION& pos);
	
	CToolTipCtrl m_cTooltip;
	void OnDraw(CDC* pDC);

	SVBase();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(SVBase)
// Form Data
public:
	//{{AFX_DATA(SVBase)
	enum { IDD = IDD_V_LISTVIEWBASE }; //IDD_V_LISTVIEWBASE
	CButton	m_btTotal;
	CListCtrl	m_lvMain;
	//}}AFX_DATA

// Attributes
public:
protected:
	int GetHiddenRowCount();
	virtual void SetMenuItem(CMenu* pMenu, int nAfterPosition, int nColumn);
	stSortParm m_stSortParm;
/*virtual*/	CString GetValueAsText(UINT nObjectNumber, int nColumn);
	CString GetValueAsText(SVColumn* pCol, UINT nObjectNumber, int nColumn);
	CRect m_crTotalLine;
	bool m_bInit;
	int GetNumColumns(){return GetListCtrl().GetHeaderCtrl()->GetItemCount();}
	int m_nItemSelected;
	int m_nSubItemSelected;
	int m_nLastMessageSentObject;	// last object number that a message detail was sent to frame
	stRow m_stRow;
	int m_nHdrColCount;
	int* m_pnColHdrTable;
	SVFormatNumber m_SVFormatNumber;
	CImageList* m_pHdrImageList;
	CImageList* m_pImageList;
	CArray<SVColumn, SVColumn> m_caColumns;
	CArray<int, int> m_caDeletedObjects;	// int's represent object numbers

// Operations
public:
	void DrawCell(CDC* pDC, CRect crCell, CRect crTextCell, CRect crGraphCell, SVColumn* pCol, UINT nObjectNumber, int nColumn, int nItem,  bool bSelected);

	bool SelectItem(int nItem);
	void DisableView();
	CString GetColumnName(int nColumn=-1);
	bool UpdateObject(UINT nObjectNumber);
	bool RedrawObject(UINT nObjectNumber);
	virtual void DestroySavedValues(UINT nObjectNumber);
	 bool UpdateView(void *p);	// new collection
	virtual bool StartView(void* pObject, SVSheet* pSheet, void* pMaps = 0, int nSortColumn = -1, bool bSortDirection = ASCENDING);
	virtual bool StartSubView(void* pObject, void* pMaps = 0);

	int GetCountSelected();
	int GetCountNotSelected();

	int DeleteRowsThatMatch(int nColumn, _TCHAR* lpszmyString, bool bDelete = true, int nStart=0);
	int DeleteRowsThatMatch(int nColumn, int nLow, int nHigh, bool bDelete, int nStart);
	int DeleteRowsThatMatch(int nColumn, double dLow, double dHigh, bool bDelete, int nStart);
	int DeleteRowsThatMatch(int nColumn, COleDateTime cdtLow, COleDateTime cdtHigh, bool bDelete, int nStart);
	void DeleteRow(int nItem);
	LRESULT SendDetailMessage( int nColumn = 0);
	LRESULT SendDetailMessage(UINT nObjectNumber, int nColumn);

	bool AddColumns(SVSheet* p);
	int AddColumn(SVColumn* pCol);
	int AddRow(UINT nObjectNumber);
	int AddRowsN(IEnumeration* p);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SVBase)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	bool m_bFixedOverride;		// when on, the graphic part is static, the text is movable
	bool m_bFixedOverrideChange;	// set on when user changes fixed piece (by holding control key)
	bool m_bCurrentlyTrackingHeader;

	bool m_bDisabled;
	bool EnsureSelectedLineVisible();
	bool m_bOrderDirty;
	
	CString FormatSysMessage(long lMsg);
	CString GetDefaultPath();
	CString GetPathOnly(CString csFile);
	CStdioFile m_cfDump;


	virtual int GetMainColumnNumber();
	COleDateTime GetRandomDateTime();
	COLORREF m_clrTextColor;
	COLORREF m_clrGraphicColor;
	COLORREF m_clrBackgroundColor;
	double ConvertXPixel(double dValue, double dMinValue, double dMaxValue, double dWidthPixel);
	void SetColors(CDC* pDC, CRect crCell, int nRule, int nColumn, bool bSelected, SVColumn* pCol, int nObjectIndex);
	bool GetMultiPixelTable(int nCount, int* nValueTable, int* nPixelTable, int nPixelTotal);
	CString GetFormattedNumber(SVColumn *pCol, UINT nObjectNumber, int nColumn);
	int GetXPixel(SVColumn* pCol, UINT nObjectNumber, int nColumn, int nTotalWidth);
	int GetXPixel(SVColumn *pCol, COleDateTime cdt, int nTotalWidth);

	virtual CString GetTotalsType();
	SVSheet* m_pActiveSheet;
	void SetColumnWidthsInSheet();
	
	bool SetSelectedCell(LVHITTESTINFO* plv);
	bool SetSelectedCell(int nItem, int nSubitem);
	virtual COleDateTime GetValueDateTime(UINT nObjectNumber, int nColumnNumber);
	virtual double GetValueDouble(UINT nObjectNumber, int nColumnNumber);
	virtual int GetValueInt(UINT nObjectNumber, int nColumnNumber);
	virtual __int64 GetValueInt64(UINT nObjectNumber, int nColumnNumber);
	virtual CString GetValueString(UINT nObjectNumber, int nColumnNumber);
	virtual bool GetValueBool(UINT nObjectNumber, int nColumnNumber);

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lptl);
	void DeleteAllRows();
	void DrawCircle(CDC* pDC, CRect crect, COLORREF clrPen, COLORREF clrBackground);
	void SetCellDimensions(SVColumn* pCol, CRect crCella, CRect &crTextCell, CRect &crGraphCell);
	void SetColumnWidth(int nColumn, int nWidth);
	void SetMinMaxValues(int nColumn);
	double GetDoubleValue(CString csValue);

	virtual COLORREF GetDefaultBackgroundColor(int nDisplayRowNumber);
	bool CompareMask(const TCHAR* mp, const TCHAR* fp);
	int TestColumnRule(double dvalue, UINT nObjectNumber, int nColumn);
	int TestColumnRule(COleDateTime cdtvalue, UINT nObjectNumber, int nColumn);
	int TestColumnRule(CString csValue, SVColumn* pCol);
	void PositionHeader();
	void SetHdrImage(int nColumn);
	void SetHdrImage(int nColumn, int nImageNumber);
	int m_nLastSort;
	int m_nDateTimeMargin;
	int m_nNumMarginLarge;
	int m_nNumMarginSmall;
	int m_nNumMarginUnits;

	int zOnToolHitTest(CPoint point, TOOLINFO * pti) const;
	POINT m_ptSelectedCell;
	CListCtrl& GetListCtrl() const { return *(CListCtrl*)&m_lvMain; }
	virtual ~SVBase();


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL OnNeedText( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
	// Generated message map functions
	//{{AFX_MSG(SVBase)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclickListctrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListctrlItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnSelector();
	afx_msg void OnSelectSame();
	afx_msg void OnHideSelected();
	afx_msg void OnHideNotSelected();
	afx_msg void OnRecoverDeleted();
	afx_msg void OnHelp();
	afx_msg void OnSelectAll();
	afx_msg void OnClickListctrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnHeaderEndDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnHeaderChanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg bool OnToolTipText(UINT id, NMHDR * pnmhdr, LRESULT * presult );
	afx_msg void OnHideSelectUI(CCmdUI* pCmdUI);
	afx_msg void OnHideNotSelectUI(CCmdUI* pCmdUI);
	afx_msg void OnRevealUI(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVBASE_H__B0DB8B8F_6712_47FB_B4D7_CE68365A61A8__INCLUDED_)
