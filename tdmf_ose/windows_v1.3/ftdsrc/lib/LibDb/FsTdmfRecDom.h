// FsTdmfRecDom.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_DOMAIN_H_
#define _FS_TDMF_REC_DOMAIN_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"



class FsTdmfRecDom : public FsTdmfRec
{
public:
	FsTdmfRecDom  ( FsTdmfDb* pDb );
	~FsTdmfRecDom ();

	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszName );
	virtual BOOL    FtdPos       ( CString pszName );
	virtual BOOL    FtdPos       ( int     piKa );
	virtual BOOL    FtdRename    ( CString pszOldName, CString pszNewName );
	virtual BOOL    FtdUpd       ( CString pszFldName, CString pszVal );
	virtual BOOL    FtdDelete    ( int     piKa );


	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;

	static  CString smszFldKa;
	static  CString smszFldName;
    static  CString smszFldDesc;
	static  CString smszFldState;

private:

}; // class FsTdmfRecDom

#endif


