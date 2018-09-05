// FsTdmfRecSrvScript.h: interface for the FsTdmfRecSrvScript class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _FS_TDMF_REC_SRV_SCRIPT_H_
#define _FS_TDMF_REC_SRV_SCRIPT_H_

#include "FsTdmfRec.h"
#include "FsTdmfRec.h"

class FsTdmfRecSrvScript : public FsTdmfRec  
{
public:
	FsTdmfRecSrvScript(FsTdmfDb* pDb );
	~FsTdmfRecSrvScript();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       (int ServerID , 
                                 CString szFileName, 
                                 CString szFileType, 
                                 CString szFileExtension,
                                 CString szCreationDate, 
                                 CString SzFileContent );
   	virtual BOOL    FtdPos       ( int   nScriptServerId );
	virtual BOOL    FtdFirst     ( CString pszSelect = "", CString pszSort = "" );
	virtual BOOL    FtdUpd       ( CString pszFldName,     CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,     int     piVal );

	CString FtdFirstCurs ( CString pszSelect = "", CString pszSort = "" );
	BOOL    FtdFirstFast ( CString pszSelect = "", CString pszSort = "" );

	static  CString smszTblName;

    static  CString smszFldScriptSrvId;// Primary Key
    static  CString smszFldSrvId; 
	static  CString smszFldFileName;
	static  CString smszFldFileType;
    static  CString smszFldFileExtension;
	static  CString smszFldFileContent;
    static  CString smszFldCreationDate;
    static  CString smszFldModificationDate; 


private:

};

#endif 
