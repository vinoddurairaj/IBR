// SVFormatNumber.h: interface for the SVFormatNumber class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SVFormatNumber_H__5507C3F0_9909_4AEE_8A11_4F2F37EC3598__INCLUDED_)
#define AFX_SVFormatNumber_H__5507C3F0_9909_4AEE_8A11_4F2F37EC3598__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _SVFormatNumber
#define _SVFormatNumber
#include "SVenUnitType.h"

#endif

class SVFormatNumber  
{
public:

	SVFormatNumber();
	virtual ~SVFormatNumber();
	CString FormatNumber(int nAmount, _UnitType ut = utNone);
	CString FormatNumber(__int64 nAmount, _UnitType ut);
	CString FormatNumber(double dAmount, _UnitType ut = utNone, bool bWhole = false);
	CString FormatCSnumber(CString csNumber, _UnitType ut = utNone);

protected:

	CString FormatNumberWithByteType(double dAmount, _UnitType ut, bool bWhole = false);
	CString RemoveLocaleDecimal(CString csNumber, bool bCurrency = false);

	void FormatSYSerr();
	bool m_bMixed;
	bool m_bNeg;
	bool m_bRoundUp;

};

#endif // !defined(AFX_SVFormatNumber_H__5507C3F0_9909_4AEE_8A11_4F2F37EC3598__INCLUDED_)
