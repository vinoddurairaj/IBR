// ComEvent.h : Declaration of the CComEvent

#ifndef __COMEVENT_H_
#define __COMEVENT_H_

#include "resource.h"       // main symbols
#include "Event.h"


/////////////////////////////////////////////////////////////////////////////
// CComEvent

class ATL_NO_VTABLE CComEvent : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComEvent, &CLSID_Event>,
	public IEvent
{
public:
	CEvent* m_pEvent;

public:
	CComEvent()	: m_pEvent(NULL) {}

DECLARE_REGISTRY_RESOURCEID(IDR_EVENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComEvent)
	COM_INTERFACE_ENTRY(IEvent)
END_COM_MAP()

// IEvent
public:
	STDMETHOD(get_PairID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_GroupID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Severity)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Source)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Time)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Date)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__EVENT_H_
