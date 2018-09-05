// FsTdmfRecHum.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_HUM_H_
#define _FS_TDMF_REC_HUM_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecHum : public FsTdmfRec
{
public:
	FsTdmfRecHum  ( FsTdmfDb* pDb );
	~FsTdmfRecHum ();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszName ); // First Name
	virtual BOOL    FtdPos       ( CString pszName ); // First Name
	virtual BOOL    FtdPos       ( int     piKa );
	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;

	static  CString smszFldKa;
	static  CString smszFldSubFk;
	static  CString smszFldNameFirst;
	static  CString smszFldNameMid;
	static  CString smszFldNameLast;
	static  CString smszFldJobTitle;
	static  CString smszFldDep;
	static  CString smszFldTel;
	static  CString smszFldCel;
	static  CString smszFldPager;
	static  CString smszFldEmail;
	static  CString smszFldNotes;

private:

}; // class FsTdmfRecHum

#endif


