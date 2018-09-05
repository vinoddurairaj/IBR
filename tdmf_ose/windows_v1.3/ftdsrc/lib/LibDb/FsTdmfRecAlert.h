// FsTdmfRecAlert.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_ALERT_H_
#define _FS_TDMF_REC_ALERT_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecAlert : public FsTdmfRec
{
public:
	FsTdmfRecAlert  ( FsTdmfDb* pDb );
	~FsTdmfRecAlert ();

	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual CString FtdGetCreate (); // Returns the create string
//	virtual BOOL    FtdNew       ( CString pszTxt ); // Desc of the Alarm
	virtual BOOL    FtdNew       ( CString &     pszTxt,
                                   int           pserverHostId,
                                   int           pgroupNbr = -1,
                                   int           pdeviceId = -1,
                                   int           ptimestamp = -1,
                                   const char*   pszType    =  0,
                                   short         pseverity  = -1 );
    virtual BOOL    FtdNew       ( CString &     pszTxt,
                                   int           pserverHostId,
                                   int           pgroupNbr,
                                   int           pdeviceId,
                                   CString &     pszTimestamp,
                                   const char*   pszType,
                                   short         pseverity );
	virtual BOOL    FtdPos       ( CString pszIk );  // Using the internal Key
	virtual BOOL    FtdPos       ( int     piIk );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;
	static  CString smszFldIk;      // Internal Key

	static  CString smszFldPairFk;   // Key
	static  CString smszFldGrpFk;
	static  CString smszFldSrcFk;

	static  CString smszFldType;
	static  CString smszFldSev;
	static  CString smszFldTs;
	static  CString smszFldTxt;

	double  FtdGetAlertDateTime(int nAlertIk);

private:
    static  CString smszInsertQueryFrmt;

}; // class FsTdmfRecAlert

#endif


