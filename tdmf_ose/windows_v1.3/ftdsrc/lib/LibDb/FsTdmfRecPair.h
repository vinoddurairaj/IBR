// FsTdmfRecPair.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_PAIR_H_
#define _FS_TDMF_REC_PAIR_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecPair : public FsTdmfRec
{
public:
	FsTdmfRecPair  ( FsTdmfDb* pDb );
	~FsTdmfRecPair ();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszPairId );
	virtual BOOL    FtdNew       ( int     piPairId );
	virtual BOOL    FtdPos       ( CString pszName );
	virtual BOOL    FtdPos       ( int     piKa );
	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );
    virtual BOOL    FtdUpd       ( CString pszFldName,     __int64 piVal );


	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;
	static  CString smszFldIk;      // Internal Key

	static  CString smszFldPairId;   // Key
	static  CString smszFldGrpFk;
	static  CString smszFldSrcFk;
	static  CString smszFldNotes;
	static  CString smszFldState;

	static  CString smszFldSrcDisk;
	static  CString smszFldSrcDriveId;
	static  CString smszFldSrcStartOff;
	static  CString smszFldSrcLength;

	static  CString smszFldTgtDisk;
	static  CString smszFldTgtDriveId;
	static  CString smszFldTgtStartOff;
	static  CString smszFldTgtLength;

    static  CString smszFldFS;

private:

}; // class FsTdmfRecPair

#endif


