// SVColumn.cpp : implementation file
//

#include "stdafx.h"
#include "SVColumn.h"
#include "SVGlobal.h"
#include "MonitorRes.h"    



SVColumn::SVColumn() {Clear();}

SVColumn::~SVColumn() 
{

}


SVColumn::SVColumn(SVColumn& pNew)			// copy constructor
{
	Copy(pNew);
}
void SVColumn::operator=(SVColumn& pNew)
{
	Copy(pNew);
}
void SVColumn::Copy(SVColumn& p)
{
	csHeaderText = p.csHeaderText;
	nHeaderJustify = p.nHeaderJustify;
	nJustify = p.nJustify;
	clr = p.clr;
	dMinValue = p.dMinValue;
	dMaxValue = p.dMaxValue;
	dTotal = p.dTotal;
	nMinValue = p.nMinValue;
	nMaxValue = p.nMaxValue;
	nTotal = p.nTotal;
	n64MinValue = p.n64MinValue;
	n64MaxValue = p.n64MaxValue;
	n64Total = p.n64Total;
	nSpike = p.nSpike;
	cdtMin = p.cdtMin;
	cdtMax = p.cdtMax;
	rt = p.rt;
	rs = p.rs;		
	clrRule[COLORRULE0] = p.clrRule[COLORRULE0];
	clrRule[COLORRULE1] = p.clrRule[COLORRULE1];
	csRuleValue = p.csRuleValue;
	cdtRuleValue = p.cdtRuleValue;
	cdtRuleValue2 = p.cdtRuleValue2;
	int64RuleValue = p.int64RuleValue;
	int64RuleValue2 = p.int64RuleValue2;
	nDateRadio = p.nDateRadio;
	nValueFromColumn = p.nValueFromColumn;
	dRuleValue = p.dRuleValue;
	dRuleValue2 = p.dRuleValue2;
	ut = p.ut;
	ct = p.ct;
	sc = p.sc;
	nSubColumns = p.nSubColumns;
	bText = p.bText;
	bTextBlack = p.bTextBlack;
	bGraphic = p.bGraphic;
	bFixedText = p.bFixedText;
	bRule = p.bRule;
	nImageTrue = p.nImageTrue;
	nImageFalse = p.nImageFalse;
	cShowStatus = p.cShowStatus;
	bMinMaxDirty = p.bMinMaxDirty;
	bMinMaxSet = p.bMinMaxSet;
	bColumnWidthDirty = p.bColumnWidthDirty;
	bGraphable = p.bGraphable;
	bIgnoreCase = p.bIgnoreCase;
	nPixelWidth = p.nPixelWidth;
	nPixelFixedWidth = p.nPixelFixedWidth;
	gdt = p.gdt;
}
void SVColumn::Clear()
{
	clr = ::GetSysColor(COLOR_WINDOWTEXT);
	csHeaderText.LockBuffer();
	csHeaderText = _T("");
	nHeaderJustify = DT_LEFT;
	nJustify = DT_LEFT;
	// rule fields
	rt  = rtNone;		// no rule
	gdt = gdtNone;	// no graphics
	clrRule[COLORRULE0] = ::GetSysColor(COLOR_WINDOWTEXT);
	clrRule[COLORRULE1] = ::GetSysColor(COLOR_WINDOWTEXT);
	csRuleValue = "";

	//	
	nImageTrue = -1;
	nImageFalse = -1;
	nPixelWidth = 150;
	nPixelFixedWidth = 0;
	ct = ctUnknown;
	sc = scNone;
	rs = rsNone;
	nSubColumns = 1;
		// min and max values in this sheet
	bMinMaxSet = false;

	ClearMinMax();
	nValueFromColumn = -1;
	bText = true;
	bGraphic = false;
	bFixedText = true;
	bRule = false;
	cShowStatus = SHOW;
	ut = utMixed;
	nSpike = 0;
	bColumnWidthDirty = false;
	bGraphable = false;				// assume it is not graphable;
	bIgnoreCase = false;
}
void SVColumn::ClearMinMax()
{

	ASSERT(bMinMaxSet == false);

	dMinValue = 99999999999;
	dMaxValue = -9999999999;
	dTotal = 0;

	nMinValue = 1999999999;
	nMaxValue = -1999999999;
	nTotal = 0;

	n64MinValue = 9999999999;
	n64MaxValue = -9999999999;
	n64Total = 0;

	cdtMax = COleDateTime( );
	cdtMin = COleDateTime(9999, 12, 31, 23, 59, 59);
	bMinMaxDirty = true;
	
}

CString SVColumn::GetColumnRuleInText()
{
	CString cs;
	switch (rt)
	{
		case rtEquals:
			cs = RESCHAR(IDS_V_EQUALS);
			break;
		case rtLow:
			cs = RESCHAR(IDS_V_LESSTHAN);	
			break;
		case rtHigh:
			cs = RESCHAR(IDS_V_GREATERTHAN);	
			break;
		case rtNone:
			return RESCHAR(IDS_V_NORULEDEFINED);
		case  rtNotEquals:
			cs = RESCHAR(IDS_V_NOTEQUALS);	
			break;
		case  rtLike:
			cs = RESCHAR(IDS_V_LIKE);	
			break;
		case  rtNotLike:
			cs = RESCHAR(IDS_V_NOTLIKE);	
			break;
		case  rtWithinHours:
			cs.Format(RESCHAR(IDS_V_WITHINHOURS), (int)dRuleValue);
			return cs;
		case  rtWithinDays:
			cs.Format(RESCHAR(IDS_V_WITHINDAYS), (int)dRuleValue);
			return cs;
		case  rtWithinWeeks:
			cs.Format(RESCHAR(IDS_V_WITHINWEEKS), (int)dRuleValue);
			return cs;
		case  rtCurrentDay:
			return RESCHAR(IDS_V_CURRENTDAY);
		case  rtCurrentMonth:
			return RESCHAR(IDS_V_CURRENTMONTH);
		default:
			return RESCHAR(IDS_V_UNKNOWN);	
	}
	cs += " " + GetStringRule(); 
	return cs;
}
CString SVColumn::GetStringRule()
{
	CString cs;
	if (ct == ctFloat)
		cs.Format(_T("%f"), dRuleValue);
	else if (IsNumber())
		cs.Format(_T("%f"), dRuleValue);		//formatint
//formatint	cs.Format(_T("%d"), (int)dRuleValue);
	else if (ct == ctBoolean)
		cs = (dRuleValue == 0) ? "true" : "false";
	else if (ct == ctDateTime)
		cs = cdtRuleValue.Format();
		else
	cs = csRuleValue;
	return cs;
}

int SVColumn:: GetNumberSubColumns()
{
	return nSubColumns;
}

bool SVColumn::IsNumber()
{
	return ( (ct >= COLTYPENUM) && (ct < COLTYPEDATE) );
}
bool SVColumn::IsFloat()
{
	return (ct == ctFloat);
}

bool SVColumn::IsGraph()
{
	if (!bGraphic)
		return false;
	return (gdt != gdtNone);
}

bool SVColumn::IsNumberOrDate()
{
	return (ct >= COLTYPENUM);
}
bool SVColumn::IsDate()
{
	return (ct >= COLTYPEDATE);
}

bool SVColumn::IsCharacter()
{
	return (ct == COLTYPECHAR);
}

bool SVColumn::IsRule()
{
	if (!bRule)
		return false;
	return (rt != rtNone);
}

