// FsTdmfRecPerf.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_PERF_H_
#define _FS_TDMF_REC_PERF_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecPerf : public FsTdmfRec
{
public:
	FsTdmfRecPerf  ( FsTdmfDb* pDb );
	~FsTdmfRecPerf ();

	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( int iTimeStamp );//N seconds since 01-01-1970
	virtual BOOL    FtdNew       ( CString pszTs );
    //optimized version of FtdNew() 
	virtual BOOL    FtdNew       ( int        timestamp,
                                   __int64    actual,
                                   __int64    effective,
                                   __int64    bytesread,
                                   __int64    byteswritten,
                                   char       role,
                                   int        connection,
                                   int        drvmode,
                                   int        lgnum,
                                   int        insert,
                                   int        devid,
                                   int        rsyncoff,
                                   int        rsyncdelta,
                                   int        entries,
                                   int        sectors,
                                   int        pctdone,
                                   int        pctbab,
                                   const char *szInstName 
                                 );
	virtual BOOL    FtdPos       ( CString pszIk );  // Using the internal Key
	virtual BOOL    FtdPos       ( int     piIk );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;
	static  CString smszFldIk;			// Internal Key

	static  CString smszFldPairFk;		// Key
	static  CString smszFldGrpFk;
	static  CString smszFldSrcFk;

	static  CString smszFldTs;			// Time Stamp

    static  CString smszFldRole;
	static  CString smszFldConn;
	static  CString smszFldDrvMode;
	static  CString smszFldLgNum;
	static  CString smszFldGuiLstIns;
	static  CString smszFldDevId;
	static  CString smszFldRSyncOff;
	static  CString smszFldRSyncDelta;
	static  CString smszFldEntries;
	static  CString smszFldSectors;
	static  CString smszFldPctDone;
	static  CString smszFldPctBab;
	static  CString smszFldInstName;

	static  CString smszFldActual;
	static  CString smszFldEffective;
	static  CString smszFldBytesRead;
	static  CString smszFldBytesWritten;

private:
    //optimisation , used only by FtdNew(....)
    static  CString smszInsertQueryFrmt;

}; // class FsTdmfRecPerf

#endif


