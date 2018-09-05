// Domain.cpp : Implementation of CDomain
#include "stdafx.h"
#include "Domain.h"
#include "Server.h"
#include "System.h"

#include "FsTdmfDb.h"
#include "mmp_API.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDomain

CServer& CDomain::AddNewServer()
{
	// Create a new domain
	CServer Server;
	Server.m_pParent = this;
	m_listServer.push_back(Server);

	return m_listServer.back();
}

FsTdmfDb* CDomain::GetDB()
{
    return m_pParent->GetDB();
}

MMP_API* CDomain::GetMMP()
{
    return m_pParent->GetMMP();
}

bool CDomain::Initialize(FsTdmfRecDom* pRec, bool bRecurse)
{
    //init data members from DB record
    m_strName           = (LPCTSTR)pRec->FtdSel( FsTdmfRecDom::smszFldName );
    m_strDescription    = (LPCTSTR)pRec->FtdSel( FsTdmfRecDom::smszFldDesc );
    m_eState            = STATE_UNDEF;//todo : 0 ?
	m_iDbK              = atoi( pRec->FtdSel( FsTdmfRecDom::smszFldKa ) );

    if ( bRecurse )
    {   
        //fill the list of CServer owned by this Domain
        int DomFk = atoi( pRec->FtdSel( FsTdmfRecDom::smszFldKa ) );
        FsTdmfRecSrvInf *pRecSrvInf = GetDB()->mpRecSrvInf;

        CString cszWhere;
        cszWhere.Format( " %s = %d ", FsTdmfRecSrvInf::smszFldDomFk , DomFk );
        BOOL bLoop = pRecSrvInf->FtdFirst( cszWhere );
        while ( bLoop )
        {
            CServer & newServer = AddNewServer();//server added to list
            newServer.Initialize(pRecSrvInf,bRecurse);//init from t_ServerInfo record
            bLoop = pRecSrvInf->FtdNext();
        }

        //now that ALL servers are added in list, 
        //we can complete the work by setting the Target server info. for each ReplGroup.
        std::list<CServer>::iterator itSrv  = m_listServer.begin();
        std::list<CServer>::iterator endSrv = m_listServer.end();
        while( itSrv != endSrv )
        {
            std::list<CReplicationGroup>::iterator itGrp  = (*itSrv).m_listReplicationGroup.begin();
            std::list<CReplicationGroup>::iterator endGrp = (*itSrv).m_listReplicationGroup.end();
            while( itGrp != endGrp )
            {
                int  iDbGrpTargetFk = (*itGrp).m_iDbTgtFk;
				if (iDbGrpTargetFk != 0)
				{
					//printf("\nServer 0x%08x, GroupId %d ...", (*itSrv).m_nHostID, (*itGrp).m_nGroupNumber );
					//scan all CServer to find which one is the Target server of the current Repl.Group
					std::list<CServer>::iterator it2Srv  = m_listServer.begin();
					std::list<CServer>::iterator end2Srv = m_listServer.end();
					while( it2Srv != end2Srv )
					{
						//CServer & ref = (*it2Srv);
						if ( (*it2Srv).m_iDbSrvId == iDbGrpTargetFk )
						{   //found the Target server of this Repl.Group
							(*itGrp).setTarget(&(*it2Srv));    
							//printf("\n   Found group Target Server : 0x%08x", (*it2Srv).m_nHostID );
							break;
						}
						it2Srv++;
					}
					//should exit loop WITHOUT having scanned all the server list !
					_ASSERT(it2Srv != end2Srv);
				}

                itGrp++;
            }

            itSrv++;
        }
    }

    return true;
}

int CDomain::SaveToDB()
{
	MMPAPI_Error Err = MMPAPI_Error::OK;

    FsTdmfRecDom *pDomTbl = GetDB()->mpRecDom;

    pDomTbl->FtdLock();

	// Update DB
	if (pDomTbl->BeginTrans())
	{
		try
		{
			bool bNewDomain = false;

			if ( !pDomTbl->FtdPos( m_iDbK ) )
			{   //domain not found, create it.
				bNewDomain = true;

				if ( !pDomTbl->FtdNew(m_strName.c_str()) )
				{   //error, could not create Domain!
					pDomTbl->FtdUnlock();
					
					throw MMPAPI_Error(MMPAPI_Error::ERR_CREATING_DB_RECORD);
				}
				// Set key field
				m_iDbK = atoi( pDomTbl->FtdSel( FsTdmfRecDom::smszFldKa ) );
			}
		
			if (!pDomTbl->FtdUpd( FsTdmfRecDom::smszFldName , m_strName.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}
			if (!pDomTbl->FtdUpd( FsTdmfRecDom::smszFldDesc , m_strDescription.c_str() ))
			{
				throw MMPAPI_Error(MMPAPI_Error::ERR_UPDATING_DB_RECORD);
			}

			// Commit changes
			if (pDomTbl->CommitTrans() == FALSE)
			{
				Err = MMPAPI_Error::ERR_UPDATING_DB_RECORD;
			}
			else
			{
				// Log
				CString cstrLogMsg;
				if (bNewDomain)
				{
					cstrLogMsg.Format("Domain - New: '%s'", m_strName.c_str());
				}
				else
				{
					cstrLogMsg.Format("Domain - Edit Properties: %d ('%s')", m_iDbK, m_strName.c_str());
				}
				m_pParent->LogUserAction(cstrLogMsg);

				// Advise other GUI
				GetMMP()->SendTdmfObjectChangeMessage(TDMF_DOMAIN, bNewDomain ? TDMF_ADD_NEW : TDMF_MODIFY, m_iDbK, 0, 0);
			}
		}
		catch(MMPAPI_Error eErr)
		{
			Err = eErr;

			// Rollback (save all or nothing)
			pDomTbl->Rollback();
		}
	}
	else
	{
		Err = MMPAPI_Error::ERR_DATABASE_TRANSACTION;
	}

    pDomTbl->FtdUnlock();

    return Err;
}

int CDomain::RemoveFromDB()
{
    FsTdmfRecDom *pDomTbl = GetDB()->mpRecDom;

    pDomTbl->FtdLock();

    if ( !pDomTbl->FtdDelete( m_iDbK ) )
	{
		//error, could not delete Domain!
	    pDomTbl->FtdUnlock();
		return MMPAPI_Error::ERR_DELETING_DB_RECORD;
	}

    pDomTbl->FtdUnlock();

	// Log
	CString cstrLogMsg;
	cstrLogMsg.Format("Domain - Remove: '%s'", m_strName.c_str());
	m_pParent->LogUserAction(cstrLogMsg);

	// Advise other GUI
	GetMMP()->SendTdmfObjectChangeMessage(TDMF_DOMAIN, TDMF_REMOVE, m_iDbK, 0, 0);

	return MMPAPI_Error::OK;
}

void CDomain::UpdateStatus()
{
	// Compute Domain state
	CElementState StateDomain(STATE_UNDEF);

	std::list<CServer>::iterator itServer;
	for (itServer = m_listServer.begin(); itServer != m_listServer.end(); itServer++)
	{
		StateDomain += itServer->m_bConnected ? itServer->m_eState : STATE_ERROR;
	}

	if (m_eState != StateDomain.State())
	{
		// Update domain state
		m_eState = StateDomain.State();
		
		// Send a notification to the client (GUI)
		PostMessage(m_pParent->m_hWnd, WM_DOMAIN_MODIFY, GetKey(), 0);
	}
}