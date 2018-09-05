// ScriptServer.h: interface for the CScriptServer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __SCRIPTSERVER_H_
#define __SCRIPTSERVER_H_

#define MAX_CHARACTERS_IN_FILE   7500

class CServer;
class FsTdmfDb;
class FsTdmfRecSrvScript;
class MMP_API;

class CScriptServer  
{
    public:
	    BOOL ExtractTypeAndExtension();
	    int SendToAgent();
	
	virtual ~CScriptServer();

    BOOL        m_bNew;
    int         m_iDbScriptSrvId;
    int         m_iDbSrvId;
    std::string m_strFileName;
    std::string m_strType;
    std::string m_strExtension;
    std::string m_strContent;
    std::string m_strCreationDate;
    std::string m_strModificationDate;
    CServer*    m_pServer;

    CScriptServer();
  
    /**
     * Initialize object from provided TDMF database record
     *
     */
    bool Initialize(FsTdmfRecSrvScript *pRec);

    /**
     * Add/Update this server information in the DB ScriptServer table.
     * Also send the appropriate TDMF commands to the agent
     */
    int SaveToDB();

    int RemoveFromDB();

    FsTdmfDb* GetDB();
	MMP_API*  GetMMP();

    long GetKey() const
    {
	    return m_iDbScriptSrvId;
    }

    bool operator==(const CScriptServer& ScriptServer) const
    {
	    return (GetKey() == ScriptServer.GetKey());
    }

    void operator=(const CScriptServer& ScriptServer)
    {

        m_iDbScriptSrvId        = ScriptServer.m_iDbScriptSrvId;
        m_iDbSrvId              = ScriptServer.m_iDbSrvId;
        m_strFileName           = ScriptServer.m_strFileName;
        m_strType               = ScriptServer.m_strType;
        m_strExtension          = ScriptServer.m_strExtension;
        m_strContent            = ScriptServer.m_strContent;
        m_strCreationDate       = ScriptServer.m_strCreationDate;
        m_strModificationDate   = ScriptServer.m_strModificationDate;
       
    	 m_pServer               = ScriptServer.m_pServer;

    }

    bool Copy(CScriptServer*);

    bool operator<(const CScriptServer& SS) const
    {
	    return m_iDbScriptSrvId < SS.m_iDbScriptSrvId;
    }

};

#endif // __SCRIPTSERVER_H_