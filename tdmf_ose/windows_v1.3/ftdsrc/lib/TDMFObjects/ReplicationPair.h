// ReplicationPair.h : Declaration of the CReplicationPair

#ifndef __REPLICATIONPAIR_H_
#define __REPLICATIONPAIR_H_

#include "Device.h"


class CReplicationGroup;

class FsTdmfDb;
class FsTdmfRecPair;


/////////////////////////////////////////////////////////////////////////////
// CReplicationPair

class CReplicationPair
{
public:

	enum ObjectState
	{
		RPO_SAVED    = 0,
		RPO_DELETED  = 1,
		RPO_MODIFIED = 2,
		RPO_NEW      = 3,
	};

	std::string m_strDescription;
	std::string m_strFileSystem;
    //std::string m_strSrcDiskPath;     available from CDevice
    //std::string m_strTgtDiskPath;     available from CDevice
	long        m_nPairNumber;
	long        m_nState;
	CDevice     m_DeviceSource;
	CDevice     m_DeviceTarget;
    int         m_iDbIk;
	int         m_iGrpFk;
	int         m_iSrcFk;

	CReplicationGroup* m_pParent;

	enum ObjectState   m_eObjectState;


	CReplicationPair() : m_nPairNumber(0), m_nState(0), m_iDbIk(-1), m_iGrpFk(-1), m_iSrcFk(-1),
						 m_pParent(NULL), m_eObjectState(RPO_NEW) {}

    /**
     * Initialize object from provided TDMF database record
     */
    bool Initialize(FsTdmfRecPair *pRec);

	FsTdmfDb* GetDB();

	std::string GetKey() const
	{
		std::ostringstream oss;
		oss << m_nPairNumber;
		return oss.str();
	}

	bool operator==(const CReplicationPair& RP) const
	{
		return (GetKey().compare(RP.GetKey()) == 0);
	}

	void operator=(const CReplicationPair& RP)
	{
		m_strDescription = RP.m_strDescription;
		m_strFileSystem  = RP.m_strFileSystem;
		m_nPairNumber    = RP.m_nPairNumber;
		m_nState         = RP.m_nState;
		m_DeviceSource   = RP.m_DeviceSource;
		m_DeviceTarget   = RP.m_DeviceTarget;
		m_iDbIk          = RP.m_iDbIk;
		m_iGrpFk         = RP.m_iGrpFk;
		m_iSrcFk         = RP.m_iSrcFk;
		//m_pParent        = RP.m_pParent;
		m_eObjectState   = RP.m_eObjectState;
	}

	void SetObjectState(enum ObjectState eNewState) {m_eObjectState = eNewState;}
};

#endif //__REPLICATIONPAIR_H_
