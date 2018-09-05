// FsTdmfRecNvp.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_NVP_H_
#define _FS_TDMF_REC_NVP_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecNvp : public FsTdmfRec
{
public:
	FsTdmfRecNvp  ( FsTdmfDb* pDb );
	~FsTdmfRecNvp ();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszName );
	virtual BOOL    FtdNew       ( CString pszName,        CString pszVal );
	virtual BOOL    FtdPos       ( CString pszName );
	virtual BOOL    FtdPos       ( int     piKa );
	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );

	BOOL FtdUpdNvp    ( CString pszName,       CString pszVal );
	BOOL FtdUpdNvp    ( CString pszName,       int     piVal );
	BOOL FtdUpdNvp    ( CString pszName,       __int64 pjVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;

	static  CString smszFldKa;
	static  CString smszFldName;
	static  CString smszFldVal;
	static  CString smszFldDesc;

private:

}; // class FsTdmfRecNvp

#endif


