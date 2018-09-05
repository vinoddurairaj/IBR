// FsTdmfRecKeyLog.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_KEY_LOG_H_
#define _FS_TDMF_REC_KEY_LOG_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"



class FsTdmfRecKeyLog : public FsTdmfRec
{
public:
	FsTdmfRecKeyLog ( FsTdmfDb* pDb );
	~FsTdmfRecKeyLog ();

	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszTimestamp, CString pszHostname, int nHostId, CString pszUser, CString pszMsg );
	virtual BOOL    FtdPos       ( int     piKa );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;

	static  CString smszFldKa;
	static  CString smszFldTs;
	static  CString smszFldHostname;
	static  CString smszFldHostId;
	static  CString smszFldKey;
	static  CString smszFldExpDate;

private:

}; // class FsTdmfRecKeyLog

#endif


