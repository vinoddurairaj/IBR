// FsTdmfRecSrvInf.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_SRV_INF_H_
#define _FS_TDMF_REC_SRV_INF_H_

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"

class FsTdmfRecSrvInf : public FsTdmfRec
{
public:
	FsTdmfRecSrvInf  ( FsTdmfDb* pDb );
	~FsTdmfRecSrvInf ();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszName );
	virtual BOOL    FtdNew       ( int     piHostId );
	virtual BOOL    FtdPos       ( CString pszName );
	virtual BOOL    FtdPos       ( int     piHostId );
	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;
	//static  CString smszFldIk;    // Internal Key

	static  CString smszFldSrvId; // Key
	static  CString smszFldDomFk;
	static  CString smszFldName;
	static  CString smszFldState;
	static  CString smszFldHostId;
	static  CString smszFldSrvIp1;
	static  CString smszFldSrvIp2;
	static  CString smszFldSrvIp3;
	static  CString smszFldSrvIp4;
	static  CString smszFldIpPort;
	static  CString smszFldDefaultJournalVol;
	static  CString smszFldDefaultPStoreFile;
	static  CString smszFldBabSizeReq;
	static  CString smszFldBabSizeAct;
	static  CString smszFldTcpWinSize;
	static  CString smszFldOsType;
	static  CString smszFldOsVersion;
	//static  CString smszFldFileSys;
	static  CString smszFldTdmfVersion;
	static  CString smszFldKey;
	static  CString smszFldKeyOld;
    static  CString smszFldKeyExpire;
	static  CString smszFldNumberOfCpu;
	static  CString smszFldRamSize;
	static  CString smszFldRtrIp1;
	static  CString smszFldRtrIp2;
	static  CString smszFldRtrIp3;
	static  CString smszFldRtrIp4;

	static  CString smszFldNotes;

	static  CString smszFldReplicationPort;

private:

}; // class FsTdmfRecSrvInf

#endif


