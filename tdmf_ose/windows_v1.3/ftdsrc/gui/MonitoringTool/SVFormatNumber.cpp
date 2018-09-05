// SVFormatNumber.cpp: implementation of the SVFormatNumber class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SVFormatNumber.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SVFormatNumber::SVFormatNumber()
{
	m_bMixed = false;
	m_bNeg = false;
	m_bRoundUp = false;
}

SVFormatNumber::~SVFormatNumber()
{
}

CString SVFormatNumber::FormatNumber(int nAmount, _UnitType ut)
{
	bool bCurrency = false;
	CString csTemp;
	if (ut == utNone || ut == utCurrency)
	{
		csTemp.Format(_T("%d"),nAmount);
		csTemp = FormatCSnumber(csTemp, ut);	// (uses regional settings)
		if (ut == utCurrency)
			bCurrency = true;
	}
	else
		csTemp = FormatNumberWithByteType((double)nAmount, ut, true);

	return RemoveLocaleDecimal(csTemp, bCurrency);
} 
CString SVFormatNumber::FormatNumber(__int64 nAmount, _UnitType ut)
{
	bool bCurrency = false;
	CString csTemp;
	if (ut == utNone || ut == utCurrency)
	{
		csTemp.Format(_T("%d"),nAmount);
		csTemp = FormatCSnumber(csTemp, ut);	// (uses regional settings)
		if (ut == utCurrency)
			bCurrency = true;
	}
	else
		csTemp = FormatNumberWithByteType((double)nAmount, ut, true);

	return RemoveLocaleDecimal(csTemp, bCurrency);
}
CString SVFormatNumber::FormatNumber(double dAmount, _UnitType ut, bool bWhole)
{	
	bool bCurrency = false;
	CString csTemp;
	if (ut == utNone || ut == utCurrency)
	{
		csTemp.Format(_T("%f"),dAmount);
		csTemp = FormatCSnumber(csTemp, ut);
		if (ut == utCurrency)
			bCurrency = true;
	}
	else
		csTemp = FormatNumberWithByteType(dAmount, ut, bWhole);

	if (bWhole)
		csTemp = RemoveLocaleDecimal(csTemp, bCurrency);

	return csTemp;
}



////////////////////////////////////////////////////////////////////////////////
// note: regional option settings control leading zero, decimal precision, etc
////////////////////////////////////////////////////////////////////////////////
CString SVFormatNumber::FormatCSnumber(CString csNumber, _UnitType ut)
{
	_TCHAR szWork[32];
	int nOut;
	if (ut == utCurrency)
		nOut = GetCurrencyFormat(LOCALE_USER_DEFAULT,
			0,					// allow user override
			csNumber,			// string to format
			NULL,				// use formatting from LOCALE
			szWork,				// output buffer
			sizeof(szWork));	// size of output buffer
	else
		nOut = GetNumberFormat(LOCALE_USER_DEFAULT,
			0,					// allow user override
			csNumber,			// string to format
			NULL,				// use formatting from LOCALE
			szWork,				// output buffer
			sizeof(szWork));	// size of output buffer

	if (nOut)
		return szWork;
	else
	{
		FormatSYSerr();
		return "";
	}
}

CString SVFormatNumber::FormatNumberWithByteType(double dTotal, _UnitType ut, bool bWhole)
{
	if (dTotal < 0)
	{
		m_bNeg = true;
		dTotal = -1 * dTotal;		// make positive
	}
	CString csType,csTemp;
	double dTemp = 0;

	switch (ut)
	{
	case utMixed:
		m_bMixed = true;
		if (dTotal >= 1099511627776)
			csTemp = FormatNumberWithByteType(dTotal, utTera, bWhole);
		else if(dTotal >= 1073741824)
			csTemp = FormatNumberWithByteType(dTotal, utGiga, bWhole);
		else if(dTotal >= 1048576)
			csTemp = FormatNumberWithByteType(dTotal, utMega, bWhole);
		else if(dTotal >= 1024)
			csTemp = FormatNumberWithByteType(dTotal, utKilo, bWhole);
		else
			csTemp = FormatNumberWithByteType(dTotal, utByte, bWhole);

		return csTemp;

	case utByte:
		csType = " B ";
		dTemp = dTotal;
		if (m_bMixed && dTemp > 999)
			csType = " KB";
		else
			bWhole = true;
		break;

	case utKilo:
		csType = " KB";
		dTemp = dTotal / 1024;
		if (m_bMixed && dTemp > 999)
			csType = " MB";
		break;

	case utMega:
		csType = " MB";
		dTemp = dTotal / 1048576;		// (1048576 == 1024 * 1024)
		if (m_bMixed && dTemp > 999)
			csType = " GB";
		break;

 	case utGiga:
		csType = " GB";
		dTemp = dTotal / 1073741824;	// (1073741824 == 1024 * 1024 * 1024)
		if (m_bMixed && dTemp > 999)
			csType = " TB";
		break;
 
	case utTera:
		csType = " TB";
		dTemp = dTotal / 1099511627776;	// (1099511627776 == 1024 * 1024 * 1024 * 1024)
		if (m_bMixed && dTemp > 999)
			csType = " QB";
		break;

	default:
		csType = " ??";					// unknown unit type - error or compatibility prob
		dTemp = dTotal;
		break;
	}

	if (m_bMixed && dTemp > 999)		// if mixed, do not allow more than 3 digits
		dTemp = dTemp / 1024;			// divide to the next K level

	if (bWhole)							// if whole number requested
	{
		csTemp.Format(_T("%.f"),dTemp);		// round to whole number
		dTemp = _tcstod(csTemp,0);		// reset to double
	}

	// following code forces rounding up if there is any value within 3 digits after the
	// last formatted decimal (regional options number setting controls number of digits
	// after decimal).  E.g. if number of decimals formats to 2 and the actual number is 
	// 1.00001 this code will force formating to "1.01" (5 digits out - 3 past formatted last)
	if (m_bRoundUp)
	{
		int n;
		double d = .999;				// number of digits controls how far to force "rounding"
		_TCHAR buf[5];
		GetLocaleInfo(LOCALE_USER_DEFAULT,	// get number of decimal digits 
			LOCALE_IDIGITS,				// number of decimal digits
			buf, sizeof(buf));			// char output = number of digits
		int nDig = _ttoi(buf);			// convert to int
		for (n = nDig; n > 0; n--)		// push .999 past last locale formatted digit
			d = d / 10;
		dTemp += d;						// add (e.g.).00999

		// must remove decimal portion beyond locale digit so this code may be run
		// through normal code path (FormatNumber may otherwise incorrectly re-round)
		
		csTemp.Format(_T("%f"),dTemp);		// cstring format the total
		n = csTemp.Find(_T("."));			// get position of decimal point
		n += ++nDig;					// point past last locale decimal digit
		int nLen = csTemp.GetLength() * sizeof( _TCHAR);	// total length
		nLen -= n;						// length to remove
		csTemp.Delete(n,nLen);			// remove extra decimal digits (no rounding)
		dTemp = _tcstod(csTemp,0);		// reset double without fractional garbage
	}
	if (m_bNeg)
		dTemp = -1 * dTemp;				// reset back to negative

	csTemp = FormatNumber(dTemp, utNone, bWhole);	// (rounds to last locale decimal) 

	csTemp += csType;

	m_bMixed = m_bNeg = false;
	return csTemp;

}

// this method receives locale formatted numbers and removes the decimal
// note: csNumber must be locale formatted using LOCALE_USER_DEFAULT
CString SVFormatNumber::RemoveLocaleDecimal(CString csNumber, bool bCurrency)
{
	_TCHAR buf[5];
	// get number of decimal digits (from regional options setting)
	if (bCurrency)
		GetLocaleInfo(LOCALE_USER_DEFAULT,
		LOCALE_ICURRDIGITS,				// decimal digits for currency output
		buf, sizeof(buf));				// buf will get number of digits (in char)
	else
		GetLocaleInfo(LOCALE_USER_DEFAULT,
		LOCALE_IDIGITS,					// decimal digits for numeric output
		buf, sizeof(buf));				// char output of number of digits
	int nDig = _ttoi(buf);				// convert to int

	// get decimal symbol (may be more than one char)
	int nLen;							// symbol length returned here (includes \0)
	if (bCurrency)
		nLen = GetLocaleInfo(LOCALE_USER_DEFAULT,
		LOCALE_SMONDECIMALSEP,			// monetary decimal separator symbol
		buf, sizeof(buf));				// actual symbol returned in buf
	else
		nLen = GetLocaleInfo(LOCALE_USER_DEFAULT,
		LOCALE_SDECIMAL,				// numeric decimal point (or symbol)
		buf, sizeof(buf));				// actual symbol returned in buf

	nLen += --nDig;						// total len of decimal sign + decimals
	int nPos = csNumber.Find(buf);		// get position of decimal
	if (nPos >=0)						// if decimal
		csNumber.Delete(nPos,nLen);		// remove decimal part only

	return csNumber;								
} 

void SVFormatNumber::FormatSYSerr()
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		0, 
		(LPTSTR) &lpMsgBuf,
		0,
		NULL );
	AfxMessageBox((LPCTSTR)lpMsgBuf);
	LocalFree( lpMsgBuf );
	return;
}