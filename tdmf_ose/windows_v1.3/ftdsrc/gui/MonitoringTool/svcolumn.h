#ifndef _SVColumn
#define _SVColumn

#define COLTYPEMISC	0
#define COLTYPECHAR	100
#define COLTYPENUM	200
#define COLTYPEDATE 300

#define MAXSUBCOLUMNS 6

#define RADIO_WITHIN_ON		0
#define RADIO_CURRENT_ON	1

#include "SVenUnitType.h"
#include <afxtempl.h>  // carray


enum _GraphicalDisplayType
{
	gdtNone,				
	gdtScatterPlot,			// type numeric or datetime only
	gdtBar,					// type numeric or datetime only
	gdtBarInFrame,			// type numeric or datetime only
	gdtState,				// type numeric or datetime only  make sure RuleValue <= Rule Value2
	gdt3Color,				// type numeric only
	gdtEmptyCircle,			// type boolean only (if rule is true = fill circle, false = empty circle) (if this is not set, if rule is true = fill circle, false = blank)
	gdtCircleWhenTrue,		// darw a circle of color clr when rule is true, else blank
	gdtCircleWhenFalse		// draw a circle of color clr when rule is false, else blank
};

enum _ColType
{
// ensure this order is preserved, othewise check the IsNumber and IsNumberOrDate routines below

	ctUnknown = COLTYPEMISC,
	ctBoolean,

	ctChar = COLTYPECHAR,
	
	ctInt = COLTYPENUM,
	ctInt64,
	ctFloat,
	
	
	ctDateTime = COLTYPEDATE
};

enum _RuleType
{
// rtEquals, rtLow, rtHigh are used to compare numeric items

	rtNone = -1,		// no rule in effect
	rtEquals = 1,
	rtLow = 2,
	rtHigh = 4,
	rtNotEquals = 6,
	rtLike = 8,
	rtNotLike = 16,
	rtWithinHours = 32,		// dRuleValue = number of hours
	rtWithinDays = 64,		// dRuleValue  = number of days
	rtWithinWeeks = 128,	// dRuleValue  = number of weeks
	rtCurrentDay = 256, 
	rtCurrentMonth = 512,
	rtEqualColumn = 1024,
	rtLowColumn = 2048,
	rtHighColumn = 4096
};

enum _SubColumns
{
	scNone,			// no subcolumns
	scSingleBar,	// one bar has all values
	scMultiBars,	// a separate bar for each value
	scMultiValues	//displayed as a dot and then the value
};
enum _RuleSuffix
{
	rsNone,
	rsKB,
	rsMB,
	rsGB,
	rsTB
};

class SVColumn : public CObject
{
public:

// headers
	CString csHeaderText;
	UINT nHeaderJustify;	// LVCFMT_LEFT, LVCFMT_CENTER, LVCFMT_RIGHT

// cell atributes
	UINT nJustify;			// DT_LEFT, DT_RIGHT, DT_CENTER
	COLORREF clr;			// color of text or bar as appropriate, 
	_UnitType ut;			// unit type for numeric
	_ColType ct;			// column type	
	_SubColumns sc;			// type of subcolumn
	int nSubColumns;		// if subcolumn, number of them
	bool bText;				// if coltype = graph or plot, true here means append text
	bool bTextBlack;		// if true, and if graph, color is COLOR_WINDOWTEXT or COLOR_HIGHLIGHTTEXT 
	_GraphicalDisplayType gdt;
	int nPixelWidth;	// if zero, then not visible
	int nPixelFixedWidth;
	bool bColumnWidthDirty;
	UINT nDateRadio;        // RADIO_WITHIN_ON, RADIO_CURRENT_ON
	int nValueFromColumn;	// if >= 0 refers to column index number whose value is used for rule value

	bool bGraphable;        
	bool bIgnoreCase;
// range for all cells in this column
	double dMinValue;
	double dMaxValue;
	double dTotal;
	__int64 n64MinValue;
	__int64 n64MaxValue;
	__int64 n64Total;
	int nMinValue;
	int nMaxValue;
	int nTotal;
	int nSpike;				// spike value to be drawn in gdtBar. Must be between nMinValue and nMaxValue
	COleDateTime cdtMin;
	COleDateTime cdtMax;
	bool bMinMaxDirty;  // have the dMinValue/dMinValue or cdtMin/cdtMax changed
	bool bMinMaxSet;	// true means that the report already set the min max values. If false, the min and max is set based on the range of the items in the collection
// rules
	bool bRule;
	_RuleType rt;			// rule type
	_RuleSuffix rs;

#define COLORRULE0 0		// also used for 3 color - High	
#define COLORRULE1 1		// also used for 3 color - Medium	
#define COLORRULE2 2		// also used for 3 color - Low		
	COLORREF clrRule[3];	
	CString csRuleValue;
	COleDateTime cdtRuleValue;
	COleDateTime cdtRuleValue2;
	__int64 int64RuleValue;
	__int64	int64RuleValue2;
	double dRuleValue;
	double dRuleValue2;
	bool bGraphic;			// true means an image rule is in effect
	bool bFixedText;
	int nImageTrue;			// image number for rule true's		// not implemented
	int nImageFalse;		// image number for rule false's	// not implemented


#define SHOW 'S'
#define HIDE 'H'
#define SCROLLLEFT 'L'
	TCHAR cShowStatus;
	SVColumn();
	~SVColumn();
	SVColumn(SVColumn& pNew);			// copy constructor
	void operator=(SVColumn& pNew);
	void Copy(SVColumn& p);
	void Clear();
	void ClearMinMax();
	CString GetColumnRuleInText();
	CString GetStringRule();
	int GetNumberSubColumns();
	bool IsNumber();
	bool IsFloat();
	bool IsGraph();
	bool IsNumberOrDate();
	bool IsDate();
	bool IsCharacter();
	bool IsRule();

};
#endif
