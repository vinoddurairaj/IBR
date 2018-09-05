// FsTdmfRecCmd.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_COMMAND_H_
#define _FS_TDMF_REC_COMMAND_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"



class FsTdmfRecCmd : public FsTdmfRec
{
public:
	FsTdmfRecCmd  ( FsTdmfDb* pDb );
	~FsTdmfRecCmd ();

	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszCmd );
	virtual BOOL    FtdPos       ( int     piKa );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;

	static  CString smszFldKa;
	static  CString smszFldCmd;
	static  CString smszFldTs;

private:

}; // class FsTdmfRecCmd

#endif


