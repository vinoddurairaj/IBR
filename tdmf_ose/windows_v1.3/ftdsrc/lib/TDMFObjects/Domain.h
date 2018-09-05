// Domain.h : Declaration of the CDomain

#ifndef __DOMAIN_H_
#define __DOMAIN_H_

#include "Server.h"


class CSystem;
class FsTdmfDb;
class FsTdmfRecDom;
class MMP_API;

/////////////////////////////////////////////////////////////////////////////
// CDomain
class CDomain
{
public:
	std::string m_strName;
	std::string m_strDescription;
	enum ElementStates m_eState;
	int         m_iDbK;

	CSystem*    m_pParent;

	CDomain() : m_eState(STATE_UNDEF), m_pParent(NULL), m_iDbK(0) {}


	std::list<CServer> m_listServer;
	CServer& AddNewServer();

    /**
     * Initialize object from provided TDMF database record
     */
	bool Initialize(FsTdmfRecDom* pRec, bool bRecurse);

    /**
     * Add/Update this domain information in the DB t_Domain table.
     */
    int SaveToDB();

	/**
     * Remove domain information from the DB t_Domain table.
     */
	int RemoveFromDB();

    FsTdmfDb* GetDB();
    MMP_API*  GetMMP();

	long GetKey() const
	{
		return m_iDbK;
	}

	bool operator==(const CDomain& Domain) const
	{
		return (GetKey() == Domain.GetKey());
	}

	void UpdateStatus();
};


#endif //__DOMAIN_H_
