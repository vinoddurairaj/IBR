// ScriptServer.cpp: implementation of the CScriptServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptServer.h"
#include "Server.h"
#include "FsTdmfDb.h"
#include "mmp_API.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScriptServer::~CScriptServer()
{

}

CScriptServer::CScriptServer()
    :m_iDbScriptSrvId(-1), m_strFileName(_T("")),
        m_strType(_T("")), m_strExtension(_T("")),
        m_strContent(_T(""))
    {
        m_bNew    = true;
        m_pServer = NULL;
        m_iDbSrvId = -1;
    }

FsTdmfDb* CScriptServer::GetDB() 
{ 
    return m_pServer->GetDB(); 
}

MMP_API* CScriptServer::GetMMP() 
{ 
    return m_pServer->GetMMP(); 
}

//Initialize object from provided TDMF database record
bool CScriptServer::Initialize(FsTdmfRecSrvScript *pRec )
{
    //init data members from DB record
    m_iDbScriptSrvId        = atoi( pRec->FtdSel( FsTdmfRecSrvScript::smszFldScriptSrvId ) );
	m_iDbSrvId              = atoi( pRec->FtdSel( FsTdmfRecSrvScript::smszFldSrvId ) );

	m_strFileName           = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvScript::smszFldFileName );
	m_strType               = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvScript::smszFldFileType );
	m_strExtension          = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvScript::smszFldFileExtension );
	m_strContent            = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvScript::smszFldFileContent );

    m_strCreationDate        = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvScript::smszFldCreationDate );
    m_strModificationDate    = (LPCTSTR)pRec->FtdSel( FsTdmfRecSrvScript::smszFldModificationDate );
    m_bNew                   = false;
  
    return true;
}

bool CScriptServer::Copy(CScriptServer* pScriptServerSource)
{
    m_iDbScriptSrvId       = pScriptServerSource->m_iDbScriptSrvId;
    m_iDbSrvId             = pScriptServerSource->m_iDbSrvId;
    m_strFileName          = pScriptServerSource->m_strFileName;
    m_strType              = pScriptServerSource->m_strType;
    m_strExtension         = pScriptServerSource->m_strExtension;
    m_strContent           = pScriptServerSource->m_strContent;
    m_strCreationDate      = pScriptServerSource->m_strCreationDate;
    m_strModificationDate  = pScriptServerSource->m_strModificationDate;
    m_pServer              = pScriptServerSource->m_pServer;
    m_bNew                 = pScriptServerSource->m_bNew;
	return true;
}

int CScriptServer::SaveToDB()
{

    if(!ExtractTypeAndExtension()) 
   {
        // Log
		CString cstrLogMsg;
		cstrLogMsg.Format("ScriptServer - The Extension of the filename '%s' is invalid", m_strFileName.c_str());
		m_pServer->m_pParent->m_pParent->LogUserAction(cstrLogMsg);
        return MMPAPI_Error::ERR_UPDATING_DB_RECORD;
    }
  

    if(m_strContent.size() > MAX_CHARACTERS_IN_FILE)
    {
      m_strContent.resize(MAX_CHARACTERS_IN_FILE);
    }

	MMPAPI_Error Err = MMPAPI_Error::OK;

	MMP_API* mmp = GetMMP();

	FsTdmfRecSrvScript *pRecSrvScript = GetDB()->mpRecScriptSrv;

    pRecSrvScript->FtdLock();

	// Find DB record
	if (pRecSrvScript->BeginTrans())
	{
		try
		{
            bool bNewScriptServer = false;
			if ( !pRecSrvScript->FtdPos( m_iDbScriptSrvId ) )
			{
				 //domain not found, create it.
				bNewScriptServer = true;

				if ( !pRecSrvScript->FtdNew(m_iDbSrvId , 
                                            m_strFileName.c_str(), 
                                            m_strType.c_str(), 
                                            m_strExtension.c_str(),
                                            m_strCreationDate.c_str(),
                                            m_strContent.c_str()))
				{   //error, could not create script!
					pRecSrvScript->FtdUnlock();
					
                 	throw MMPAPI_Error(MMPAPI_Error::ERR_CREATING_DB_RECORD);
				}
				// Set key field
				m_iDbScriptSrvId = atoi( pRecSrvScript->FtdSel( FsTdmfRecSrvScript::smszFldScriptSrvId ) );
                m_bNew = false;
			}

		    m_bNew = false;
       		// Save changes to db
			if (!pRecSrvScript->FtdUpd( FsTdmfRecSrvScript::smszFldSrvId, m_iDbSrvId ))
			{
           		throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pRecSrvScript->FtdUpd( FsTdmfRecSrvScript::smszFldFileName, m_strFileName.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
            if (!pRecSrvScript->FtdUpd( FsTdmfRecSrvScript::smszFldFileType, m_strType.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
            if (!pRecSrvScript->FtdUpd( FsTdmfRecSrvScript::smszFldFileExtension, m_strExtension.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
            if (!pRecSrvScript->FtdUpd( FsTdmfRecSrvScript::smszFldFileContent, m_strContent.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
            
            if(!bNewScriptServer)
            {
                COleDateTime cdtCurrent = COleDateTime::GetCurrentTime();  
                CString strDate = cdtCurrent.Format("%Y-%m-%d %H:%M:%S");
                if (!pRecSrvScript->FtdUpd( FsTdmfRecSrvScript::smszFldModificationDate, strDate))
			    {
				    throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			    }
         	}

			// Commit changes
			if (pRecSrvScript->CommitTrans() == FALSE)
			{
                if(bNewScriptServer)
                {
                    m_bNew = true;
                }
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			else
			{
				// Log
				CString cstrLogMsg;
				cstrLogMsg.Format("ScriptServer - Update Script server database info for file : '%s'", m_strFileName.c_str());
				m_pServer->m_pParent->m_pParent->LogUserAction(cstrLogMsg);
            }
		}
		catch(MMPAPI_Error eErr)
		{
			Err = eErr;

      		// Rollback (save all or nothing)
			pRecSrvScript->Rollback();
		}
	}
	else
	{
        Err = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

    pRecSrvScript->FtdUnlock();

	return Err;
}

int CScriptServer::RemoveFromDB()
{
	if (GetMMP()->delScriptServerFileFromDb(this).IsOK())
	{
		// Log
		CString cstrLogMsg;
		cstrLogMsg.Format("ScripServer - Remove '%s'", m_strFileName.c_str());
		m_pServer->m_pParent->m_pParent->LogUserAction(cstrLogMsg);
     
	}

	return MMPAPI_Error::OK;
}

int CScriptServer::SendToAgent()
{

    MMPAPI_Error Err = MMPAPI_Error::OK;

   //verify the OS of the server where we send the file...
	bool bIsWindows = ( strstr(m_pServer->m_strOSType.c_str(),"Windows") != 0 ||
						strstr(m_pServer->m_strOSType.c_str(),"windows") != 0 ||
						strstr(m_pServer->m_strOSType.c_str(),"WINDOWS") != 0 );

	if(!bIsWindows)
	{
		CString strValue = m_strContent.c_str();
		
		strValue.Replace( "\r\n","\n");
	   
		m_strContent = strValue;
    }

    if (GetMMP()->SendScriptServerFileToAgent(this).IsOK())
	{
		// Log
		CString cstrLogMsg;
		cstrLogMsg.Format("ScripServer - '%s' - Sent to Agent '%s'", m_strFileName.c_str(),m_pServer->m_strName.c_str());
		m_pServer->m_pParent->m_pParent->LogUserAction(cstrLogMsg);

    }
    else
    {
        Err =  MMPAPI_Error::ERR_UNKNOWN_SENDING_SCRIPT_SERVER_FILE_TO_AGENT;
    }

	return Err;
 
}

BOOL CScriptServer::ExtractTypeAndExtension()
{
  CString strFilename = m_strFileName.c_str();

  if(strFilename.IsEmpty())
      return FALSE;

  CString strExtension = strFilename.Right(3);

  if((strExtension.CompareNoCase(_T("bat")) == 0) ||
	 (strExtension.CompareNoCase(_T(".sh")) == 0))
  {
    m_strExtension = strExtension;
	CString strType;
    strType.Format("%d",MMP_MNGT_FILETYPE_TDMF_BAT); // batch file
    m_strType = strType;
  }
  else
  {
	return FALSE;
  }

  return TRUE;
}
