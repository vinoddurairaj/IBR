// FsTdmfRecLgGrp.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_LG_GRP_H_
#define _FS_TDMF_REC_LG_GRP_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecLgGrp : public FsTdmfRec
{
public:
	FsTdmfRecLgGrp  ( FsTdmfDb* pDb );
	~FsTdmfRecLgGrp ();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszGrpId );
	virtual BOOL    FtdNew       ( int     piGrpId );
	virtual BOOL    FtdPos       ( CString pszName );
	virtual BOOL    FtdPos       ( int     piKa );
	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;
	static  CString smszFldIk;      // Internal Key

	static  CString smszFldLgGrpId; // Key
	static  CString smszFldSrcFk;
	static  CString smszFldTgtFk;
    static  CString smszFldPStore;
	static  CString smszFldJournalVol;
	static  CString smszFldChainning;
	static  CString smszFldNotes;
	static  CString smszFldThrottles;
	static  CString smszFldChunkDelay;
	static  CString smszFldChunkSize;
	static  CString smszFldSyncMode;
	static  CString smszFldSyncModeDepth;
	static  CString smszFldSyncModeTimeOut;
	static  CString smszFldRefreshNeverTimeOut;
	static  CString smszFldRefreshTimeOut;
	static  CString smszFldEnableCompression;

	static  CString smszFldNetUsageThreshold;
	static  CString smszFldNetUsageValue;
	static  CString smszFldUpdateInterval;
	static  CString smszFldMaxFileStatSize;
	static  CString smszFldJournalLess;

	static  CString smszFldConnection;
	//static  CString smszFldStateConn;
	static  CString smszFldStateTimeStamp;
    
	static  CString smszFldPrimaryDHCPNameUsed;  
	static  CString smszFldPrimaryEditedIPUsed;
 	static	CString smszFldPrimaryEditedIP;
   
	static  CString smszFldTgtDHCPNameUsed;
	static  CString smszFldTgtEditedIPUsed;
 	static	CString smszFldTgtEditedIP;
	
	static  CString smszFldSymmetric;
	static  CString smszFldSymmetricGroupNumber;
	static  CString smszFldSymmetricNormallyStarted;
	static  CString smszFldFailoverInitialState;
	static  CString smszFldSymmetricJournalDirectory;
	static  CString smszFldSymmetricPStoreFile;

	int	    FtdGetTargetReplicationPairCount(int nServerFk);

private:

}; // class FsTdmfRecLgGrp

#endif


