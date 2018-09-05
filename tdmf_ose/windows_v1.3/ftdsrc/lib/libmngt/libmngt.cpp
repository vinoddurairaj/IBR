// libmngt.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <crtdbg.h>
#include <limits.h>
#include <list> //STL

#include "libmngtdef.h"

/*
//////////////////////////////////////////////////////////////////
// section begin

// if precompiler header not in use, import content of stdfax.h here.

//
// section end
//////////////////////////////////////////////////////////////////
*/

#ifdef DEBUG    
#include <crtdbg.h> //for ASSERT()
#endif

//*****************************************************************
// The debugger can't handle symbols more than 255 characters long.
// STL often creates symbols longer than that.
// When symbols are longer than 255 characters, the warning is disabled.
#pragma warning(disable:4786)


#define DEF_SOCK_RECV_TIMEOUT   5   //5 sec
#define DEF_SOCK_RECV_CMD_STATUS_TIMEOUT    (20 * 60)


// *********************************************************
class oneTextFileLine 
{
public:
    oneTextFileLine()
    {
        line    = 0;
        linelen = 0;
    };
    ~oneTextFileLine()
    {
        delete [] line;
    };
    //copy constructor
    oneTextFileLine(const oneTextFileLine & aLine)
    {
        linelen = aLine.linelen;
        line    = new char[linelen];
        memcpy(line, aLine.line, linelen);
    };
    void fillLine(const char *line, int linelen)
    {
        this->linelen = linelen;
        this->line    = new char[linelen];
        memcpy(this->line, line, this->linelen);
    }

    char    *line;
    int     linelen;
};

typedef std::list<oneTextFileLine>  listTextFileLine_t;

//*****************************************************************
//int mmp_AgentInfoList_handle::iReferenceSignature   =   0xaabb000f;      
int mmp_handle::iReferenceSignature                 =   0xaabb003d;      


//*****************************************************************
// local prototypes
//
static bool     mmp_IsHandleValid(MMP_HANDLE handle);
//static bool     mmp_IsHandleValid(MMP_AGENTINFOLIST_HANDLE list);
static int      mmp_mngt_request_cfg_data( int rip, int rport, 
                                           /*in*/ mmp_mngt_ConfigurationMsg_t *pReqCfg, 
                                           /*out*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData); 

static int      mmp_mngt_request_tdmf_file( int rip, int rport, 
                                           /*in*/ mmp_mngt_FileMsg_t *pReqFileMsg, 
                                           /*out*/mmp_mngt_FileMsg_t ** ppRcvFileMsg); 

static void     textfilebuffer_to_linelist(const char * pCfgData, unsigned int uiDataSize, listTextFileLine_t & listTextFileLines);

//*************************************************************
// API function definition 
//*************************************************************

// ***********************************************************
LIBMNGT_API 
MMP_HANDLE  mmp_Create(char *szTDMFSrvIP, int iTDMFSrvPort)
{
    //
    unsigned long tdmfsrvIP,localIPlist[10];//reserve more tspace than required

    sock_startup();//initialize socket stack 

    sock_ipstring_to_ip(szTDMFSrvIP, &tdmfsrvIP);

    memset(localIPlist,0,sizeof(localIPlist));
    sock_get_local_ip_list(localIPlist);


    MMP_HANDLE handle = new mmp_handle( tdmfsrvIP,
                                        localIPlist[0], //keep only first local ip
                                        iTDMFSrvPort );

	return handle;//for now no need for a session specific structure
}

// ***********************************************************
LIBMNGT_API 
void	mmp_Destroy( /*in*/ MMP_HANDLE handle)
{
	if ( mmp_IsHandleValid(handle) )
    {
        //just in case our client try to reuse the object AFTER this function returns
        handle->iSignature = ~mmp_handle::iReferenceSignature;//invalidate object signature
        delete handle;
    }
}


// ***********************************************************
LIBMNGT_API 
int mmp_getAllTdmfAgentInfo( /*in*/ MMP_HANDLE handle )
{
    int ret ;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    mmp_mngt_BroadcastReqMsg_t  msg;
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_AGENT_INFO_REQUEST;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //because this message is from a TDMF client, no need to fill the data portion of it.
    memset(&msg.data,0,sizeof(msg.data));

    //send Broadcast Request message to TDMF Collector.
    //no answer expected but wait for socket to be disconnected by TDMF Collector.
    //this indicates the request is completly processed.
    //output buffer provided to mmp_sendmsg() in order to keep socket opened and wait for TDMF Collector response.
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, 0 );_ASSERT(ret==-4 || ret==0);
    if ( ret == -4 )// -4 = socket closed by TDMF Collector while waiting to recv data, this is the expected situation
        ret = 0;//success

    return ret;
}


// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfAgentRegistrationKey(/*in*/ MMP_HANDLE handle, 
                                    /*in*/ const char * szAgentId,
                                    /*in*/ bool  bIsHostId,
                                    /*out*/char * szRegKey,
                                    /*in*/ int len,
                                    /*out*/int *pExpirationTime )
{
    int ret;
    mmp_mngt_RegistrationKeyMsg_t  msg;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(msg.keydata.szServerUID) )
        return -10;

    szRegKey[0] = 0;
    if ( pExpirationTime )
        *pExpirationTime = 0;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_REGISTRATION_KEY;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //data
    msg.keydata.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg.keydata.szServerUID, HOST_ID_PREFIX);
    strcat(msg.keydata.szServerUID,szAgentId);
    //GET key : provide am empty key 
    msg.keydata.szRegKey[0] = 0;

    //send Request message to TDMF Collector and wait for answer
    //response is also a mmp_mngt_RegistrationKeyMsg_t.  save it in same buffer.
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg) );_ASSERT(ret==-4 || ret == sizeof(msg));
    if ( ret == sizeof(msg) )
    {
        ret = 0;//success

        mmp_convert_mngt_hdr_ntoh( &msg.hdr );
        int iExpirationTime = ntohl(msg.keydata.iKeyExpirationTime);

        if ( msg.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            if( szRegKey != NULL )//output buffer
            {
                if ( len >= strlen(msg.keydata.szRegKey)+1 )
                {
                    strcpy( szRegKey, msg.keydata.szRegKey );
                }
                else
                {
                    ret = -11;//output buffer too small
                }

                if ( pExpirationTime )
                {
                    if ( iExpirationTime == 1 )     //permanent key , never expires
                        *pExpirationTime = INT_MAX;
                    else
                        *pExpirationTime = iExpirationTime;//either expiration time (> 1) or error code ( <= 0 )
                }
            }
        }
        else
        {
            ret = -msg.hdr.mngtstatus;
        }
    }
    else if ( ret == 0 )
    {   //nothing recvd in response !
        ret = -4;
    }

    return ret;
}

// ***********************************************************
LIBMNGT_API 
int mmp_setTdmfAgentRegistrationKey(/*in*/ MMP_HANDLE handle, 
                                    /*in*/ const char * szAgentId,
                                    /*in*/ bool  bIsHostId,
                                    /*in*/ const char * szRegKey,
                                    /*out*/int *pExpirationTime )
{
    int ret;
    mmp_mngt_RegistrationKeyMsg_t  msg;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(msg.keydata.szServerUID) )
        return -10;
    if ( strlen(szRegKey)+1 > sizeof(msg.keydata.szRegKey) )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_REGISTRATION_KEY;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //data
    msg.keydata.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg.keydata.szServerUID, HOST_ID_PREFIX);
    strcat(msg.keydata.szServerUID,szAgentId);

    //SET key : provide the key 
    strcpy(msg.keydata.szRegKey,szRegKey);

    //send Request message to TDMF Collector.
    //no answer expected but wait for socket to be disconnected by TDMF Collector .
    //this indicates the request is completly processed.
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg) );_ASSERT(ret==-4 || ret==sizeof(msg));
    if ( ret == sizeof(msg) )
    {
        ret = 0;//success
        
        mmp_convert_mngt_hdr_ntoh( &msg.hdr );
        int iExpirationTime = ntohl(msg.keydata.iKeyExpirationTime);

        if ( msg.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            if( szRegKey != NULL )//output buffer
            {
                if ( strcmp(szRegKey, msg.keydata.szRegKey) == 0 )
                {
                    //ok, key sent was received, it means it was sent correctly to TDMF Agent
                }
                else
                {
                    ret = -11;//output buffer too small
                }

                if ( pExpirationTime )
                {
                    if ( iExpirationTime == 1 )     //permanent key , never expires
                        *pExpirationTime = INT_MAX;
                    else
                        *pExpirationTime = iExpirationTime;//either expiration time (> 1) or error code ( <= 0 )
                }
            }
        }
        else
        {
            ret = -msg.hdr.mngtstatus;
        }
    } 
    else 
    {
        ret = -1;
    }

    return ret;
}


// *********************************************************
LIBMNGT_API 
int mmp_mngt_setTdmfAgentConfig(/*in*/ MMP_HANDLE handle, 
                                /*in*/ const char * szAgentId,
                                /*in*/ bool  bIsHostId,
                                /*in*/ const mmp_TdmfServerInfo * agentCfg
                                )
{
    int ret;
    mmp_mngt_TdmfAgentConfigMsg_t   msg;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(msg.data.szServerUID) )
        return -10;
    if ( agentCfg == NULL )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_SET_AGENT_GEN_CONFIG;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //data
    msg.data = *agentCfg;
    msg.data.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg.data.szServerUID, HOST_ID_PREFIX);
    strcat(msg.data.szServerUID,szAgentId);

    mmp_convert_TdmfServerInfo_hton(&msg.data);

    //send general Agent config message to TDMF Collector.
    //wait to receive the response, same message with updated mngt status.
    //this indicates the request is completly processed.
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg) );
    if ( ret == sizeof(msg) )
    {
        mmp_convert_mngt_hdr_ntoh( &msg.hdr );
        if ( msg.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            ret = 0;//success
        }
        else
        {
            ret = -msg.hdr.mngtstatus;
        }
    }
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }
   
    return ret;
}

// *********************************************************
LIBMNGT_API 
int mmp_mngt_getTdmfAgentConfig(/*in*/ MMP_HANDLE handle, 
                                /*in*/ const char * szAgentId,
                                /*in*/ bool  bIsHostId,
                                /*in*/ mmp_TdmfServerInfo * agentCfg
                                )
{
    int ret;
    mmp_mngt_TdmfAgentConfigMsg_t   msg;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(msg.data.szServerUID) )
        return -10;
    if ( agentCfg == NULL )
        return -10;

    memset(agentCfg,0,sizeof(mmp_TdmfServerInfo));

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_GET_AGENT_GEN_CONFIG;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //data
    msg.data.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg.data.szServerUID, HOST_ID_PREFIX);
    strcat(msg.data.szServerUID,szAgentId);

    //send general Agent config message to TDMF Collector.
    //wait to receive the response, same message with updated mngt status.
    //this indicates the request is completly processed.
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg) );
    if ( ret == sizeof(msg) )
    {
        mmp_convert_mngt_hdr_ntoh( &msg.hdr );
        if ( msg.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            ret = 0;//success
            mmp_convert_TdmfServerInfo_ntoh(&msg.data);
            *agentCfg = msg.data;
        }
        else
        {
            ret = -msg.hdr.mngtstatus;
        }
    }
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }
   
    return ret;
}

// *********************************************************
// SET one Logical Group configuration
LIBMNGT_API 
int mmp_setTdmfAgentLGConfig(/*in*/ MMP_HANDLE handle, 
                             /*in*/ const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*in*/ short sLogicalGroupId,
                             /*in*/ bool  bPrimary,
                             /*in*/ const char * pCfgData,
                             /*in*/ unsigned int uiDataSize
                             )
{
    int ret;
    char cfgfname[32];
    mmp_mngt_ConfigurationMsg_t *pCfgMsg;
    mmp_mngt_ConfigurationStatusMsg_t response;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(pCfgMsg->szServerUID) )
        return -10;


    if ( pCfgData == 0 )
        uiDataSize = 0;

    sprintf( cfgfname, "%c%03d.cfg", bPrimary ? 'p' : 's', (int)sLogicalGroupId );

    mmp_mngt_build_SetConfigurationMsg(&pCfgMsg, szAgentId, bIsHostId, SENDERTYPE_TDMF_CLIENTAPP,  
                                       cfgfname, pCfgData, uiDataSize );
    if ( pCfgMsg == 0 )
        return -1;

    //send Request message to TDMF Collector.
    //wait for response -> will be mmp_mngt_ConfigurationStatusMsg_t .
    int towrite = sizeof(mmp_mngt_ConfigurationMsg_t) + uiDataSize;
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)pCfgMsg, towrite, (char*)&response, sizeof(response) );_ASSERT(ret==-4 || ret==sizeof(response));
    if ( ret==sizeof(response) )
    {
        mmp_convert_mngt_hdr_ntoh( &response.hdr );

        if ( response.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            ret = ntohl(response.iStatus);
        }
        else
        {
            ret = -response.hdr.mngtstatus;
        }
    }
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }

    mmp_mngt_release_SetConfigurationMsg(&pCfgMsg);
        
    return ret;
}

// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfAgentLGConfig(/*in*/ MMP_HANDLE handle, 
                             /*in*/ const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*in*/ short sLogicalGroupId,
                             /*in*/ bool  bPrimary,
                             /*out*/ char ** ppCfgData,
                             /*out*/ unsigned int *puiDataSize
                             )
{
    int r;
    mmp_mngt_ConfigurationMsg_t msg;
    mmp_mngt_ConfigurationMsg_t *pRcvCfgData;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(msg.szServerUID) )
        return -10;

    *ppCfgData   = 0;
    *puiDataSize = 0;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_GET_LG_CONFIG;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //Agent id - machine name or Agent host id
    msg.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg.szServerUID, HOST_ID_PREFIX);
    strcat(msg.szServerUID,szAgentId);

    if ( sLogicalGroupId == -1 )
        sprintf( msg.data.szFilename, "*.cfg" );
    else
        sprintf( msg.data.szFilename, "%c%03d.cfg", bPrimary ? 'p' : 's', sLogicalGroupId );
    msg.data.iType  = htonl(MMP_MNGT_FILETYPE_TDMF_CFG);
    msg.data.uiSize = 0;

    //send Configuration data request to TDMF Collector.
    //no answer expected but wait for socket to be disconnected by TDMF Collector .
    //this indicates the request is completly processed.
    r = mmp_mngt_request_cfg_data( handle->iTDMFSrvIP, handle->iPort, /*in*/&msg, /*out*/&pRcvCfgData);
    if ( r == 0 )
    {
        if ( pRcvCfgData != 0 )
        {
            //transfer cfg file data to output buffer.
            *puiDataSize = pRcvCfgData->data.uiSize;
            if ( (*puiDataSize) > 0 )
            {
                *ppCfgData = new char [*puiDataSize];
                memcpy(*ppCfgData, pRcvCfgData+1, *puiDataSize);
                r = 0;//success
            }
            else
            {   
                if ( pRcvCfgData->hdr.mngtstatus == MMP_MNGT_STATUS_OK )
                {   //no data probably means logical group does not exist on Agent.
                    r = -6;
                }
                else
                {
                    r = -pRcvCfgData->hdr.mngtstatus;
                }
            }
        }
        else
        {
            r = -1;
        }
    }
    //to release ptr allocated by mmp_mngt_recv_cfg_data(), within mmp_mngt_request_cfg_data()
    mmp_mngt_free_cfg_data_mem(&pRcvCfgData);

    return r;
}

// *********************************************************
// SET TDMF file in a specific SERVER
LIBMNGT_API 
int mmp_setTdmfServerFile   (/*in*/ MMP_HANDLE handle, 
                             /*in*/ const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*in*/ const char * pFileName,
                             /*in*/ const char * pFileData,
                             /*in*/ unsigned int uiDataSize,
                             /*in*/ int iFileType 
                            )
{
	int ret;
    mmp_mngt_FileMsg_t *pFileMsg;
    mmp_mngt_FileStatusMsg_t response;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(pFileMsg->szServerUID) )
        return -10;

	// Specific setting to ERASE the file on targeted server
    if ( pFileData == 0 )
        uiDataSize = 0;

    mmp_mngt_build_SetTdmfFileMsg(&pFileMsg, szAgentId, bIsHostId, SENDERTYPE_TDMF_CLIENTAPP,  
                                  pFileName, pFileData, uiDataSize , iFileType);
    if ( pFileMsg == 0 )
        return -1;

    //send TDMF_MNGT_TDMF_SENDFILE Request message to TDMF Collector.
    //wait for response -> will be mmp_mngt_FileStatusMsg_t .
    int towrite = sizeof(mmp_mngt_FileMsg_t) + uiDataSize;
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)pFileMsg, towrite, (char*)&response, sizeof(response) );_ASSERT(ret==-4 || ret==sizeof(response));

	if ( ret==sizeof(response) )
    {
        mmp_convert_mngt_hdr_ntoh( &response.hdr );

        if ( response.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            ret = ntohl(response.iStatus);
        }
        else
        {
            ret = -response.hdr.mngtstatus;
        }
    }
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }

    mmp_mngt_release_SetTdmfFileMsg(&pFileMsg);
	
    return ret;
}

// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfServerFile(/*in*/ MMP_HANDLE handle, 
                          /*in*/ const char * szAgentId,
                          /*in*/ bool  bIsHostId,
                          /*in*/ const char * pFileName,
                          /*in*/ int iFileType,
                          /*out*/ char ** ppFileMsgData,
                          /*out*/ unsigned int *puiDataSize
                          )
{
    int r;
    mmp_mngt_FileMsg_t pFileMsg;
    mmp_mngt_FileMsg_t *pRcvFileMsg;

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(pFileMsg.szServerUID) )
        return -10;

    *ppFileMsgData   = 0;
    *puiDataSize = 0;

    //header    
    pFileMsg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    pFileMsg.hdr.mngttype    = MMP_MNGT_TDMF_GETFILE;
    pFileMsg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&pFileMsg.hdr);
    //Agent id - machine name or Agent host id
    pFileMsg.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( pFileMsg.szServerUID, HOST_ID_PREFIX);

    strcat(pFileMsg.szServerUID,szAgentId);
    strcpy( pFileMsg.data.szFilename, pFileName );
	pFileMsg.data.iType  = htonl(iFileType);
    pFileMsg.data.uiSize = 0;

    //send data request to TDMF Collector.
    //no answer expected but wait for socket to be disconnected by TDMF Collector .
    //this indicates the request is completly processed.

    r = mmp_mngt_request_tdmf_file( handle->iTDMFSrvIP, handle->iPort, /*in*/&pFileMsg, /*out*/&pRcvFileMsg);

    if ( r == 0 )
    {
        if ( pRcvFileMsg != 0 )
        {
            //transfer TDMF file data to output buffer.
            *puiDataSize = pRcvFileMsg->data.uiSize;
            if ( (*puiDataSize) > 0 )
            {
                *ppFileMsgData = new char [*puiDataSize];
                memcpy(*ppFileMsgData, pRcvFileMsg+1, *puiDataSize);
                r = 0;//success
            }
            else
            {   
                if ( pRcvFileMsg->hdr.mngtstatus == MMP_MNGT_STATUS_OK )
                {   //no data probably means does not exist on Agent.
                    r = -6;
                }
                else
                {
                    r = -pRcvFileMsg->hdr.mngtstatus;
                }
            }
        }
        else
        {
            r = -1;
        }
    }
    //to release ptr allocated by mmp_mngt_recv_cfg_data(), within mmp_mngt_request_cfg_data()
    mmp_mngt_free_file_data_mem(&pRcvFileMsg);

    return r;
}

// *********************************************************
LIBMNGT_API 
int mmp_getTdmfAgentAllDevices(/*in*/ MMP_HANDLE handle, 
                               /*in*/ const char * szAgentId,
                               /*in*/ bool  bIsHostId,
                               /*out*/mmp_TdmfDeviceInfo ** ppTdmfDeviceInfoVector,
                               /*out*/unsigned int *puiNbrDeviceInfoInVector
                               )
{
    int r;
    mmp_mngt_TdmfAgentDevicesMsg_t *pTdmfDevicesMsg;//msg response from TDMF Agent

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    *ppTdmfDeviceInfoVector     = 0;
    *puiNbrDeviceInfoInVector   = 0;

    r = mmp_mngt_request_alldevices( handle->iTDMFSrvIP, handle->iPort, 0, 
                                     SENDERTYPE_TDMF_CLIENTAPP, szAgentId, bIsHostId,
                                    /*out*/&pTdmfDevicesMsg );
    if ( r == 0)
    {
        if ( pTdmfDevicesMsg != 0 )
        {
            *puiNbrDeviceInfoInVector   = pTdmfDevicesMsg->iNbrDevices;
            //allocate vector to hold all devices 
            *ppTdmfDeviceInfoVector   = new mmp_TdmfDeviceInfo[*puiNbrDeviceInfoInVector];
            //*ppTdmfDeviceInfoVector     = (mmp_TdmfDeviceInfo*)malloc( sizeof(mmp_TdmfDeviceInfo)*(*puiNbrDeviceInfoInVector) );
            memcpy( *ppTdmfDeviceInfoVector, pTdmfDevicesMsg+1, *puiNbrDeviceInfoInVector * sizeof(mmp_TdmfDeviceInfo) );
            //free temp work mem 
            mmp_mngt_free_alldevices_data(&pTdmfDevicesMsg);
        }
    }
    else 
    {   //error
        //translate error codes to MMPMNGT_STATUS_ERR_... codes
        switch(r)
        {
        case -1:    
            r = MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;  
            break;
        case -2:    
        case -3:    
        case -4:    
        case -5:    
            r = MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;  
            break;
        }
    }
    return r;
}


// *********************************************************
LIBMNGT_API 
int 
mmp_mngt_request_alldevices( int rip, int rport, sock_t *request,
                             int sendertype, //SENDERTYPE_TDMF_....
                             const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*out*/mmp_mngt_TdmfAgentDevicesMsg_t **ppTdmfDevicesMsg
                            )
{
    mmp_mngt_TdmfAgentDevicesMsg_t  msg;
    int r,toread,towrite;
    sock_t *s;

    if ( strlen(szAgentId)+1 > sizeof(msg.szServerUID) )
        return -10;

    *ppTdmfDevicesMsg    = 0;
    
    //create socket if necessary
    if ( request == 0 )
    {
        s = sock_create();
        r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
        if ( r >= 0 ) 
        {
            r = sock_connect(s, rport); _ASSERT(r>=0);
        }
    }
    else
    {
        s = request;
        r = 0;
    }
    if ( r >= 0 ) 
    {
        //header    
        msg.hdr.sendertype  = sendertype;
        msg.hdr.mngttype    = MMP_MNGT_GET_ALL_DEVICES;
        msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
        mmp_convert_mngt_hdr_hton(&msg.hdr);
        //Agent id - machine name
        msg.szServerUID[0] = 0;
        if ( bIsHostId )
            strcat( msg.szServerUID, HOST_ID_PREFIX);
        strcat(msg.szServerUID,szAgentId);

        //obviously no mmp_TdmfDeviceInfo provided in this request msg.
        msg.iNbrDevices     = 0;

        towrite = sizeof(msg);
        r = mmp_mngt_sock_send(s, (char*)&msg, towrite); _ASSERT(r == towrite);
        if ( r == towrite )
        {
            mmp_mngt_header_t hdr;
            //wait for response ...
            toread = sizeof(hdr);
            r = mmp_mngt_sock_recv(s, (char*)&hdr, toread, MMP_MNGT_TIMEOUT_GETDEVICES ); _ASSERT(r == toread); // ardev 030110
            if ( r == toread )
            {
                mmp_convert_mngt_hdr_ntoh(&hdr);
                _ASSERT(hdr.magicnumber == MNGT_MSG_MAGICNUMBER);
                _ASSERT(hdr.mngttype    == MMP_MNGT_SET_ALL_DEVICES);
                _ASSERT(hdr.sendertype  != sendertype);

                if ( hdr.magicnumber == MNGT_MSG_MAGICNUMBER     &&
                     hdr.mngttype    == MMP_MNGT_SET_ALL_DEVICES &&                         
                     hdr.mngtstatus  == MMP_MNGT_STATUS_OK        )
                {

                    mmp_mngt_recv_alldevices_data(s, &hdr, ppTdmfDevicesMsg);

                    //on success, *ppTdmfDevicesMsg points to non-NULL address 
                    //containing a mmp_mngt_TdmfAgentDevicesMsg_t msg
                    if ( *ppTdmfDevicesMsg != 0 )
                    {
                        r = 0;//success
                    }
                    else
                    {
                        r = -5;
                    }
                }
                else
                {
                    if ( hdr.mngtstatus != MMP_MNGT_STATUS_OK )
                        r = -hdr.mngtstatus;// <= -100
                    else
                        r = -4;
                }
            }
            else
            {
                r = -3;//-MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
            }
        }
        else 
        {
            r = -2;//-MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
        }
    }
    else
    {
        r = -1;//-MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;//-100
    }

    if ( request == 0 )
    {
        sock_disconnect(s);
        sock_delete(&s);//was created locally
    }

    return r;
}

/*
 * receive a mmp_mngt_TdmfAgentDevicesMsg_t message from an open socket 
 * header as been received
 * convert message data to host byte order.
 */
LIBMNGT_API
void mmp_mngt_recv_alldevices_data(sock_t *s, mmp_mngt_header_t *hdr, mmp_mngt_TdmfAgentDevicesMsg_t **ppRcvDevMsg)
{
    mmp_mngt_TdmfAgentDevicesMsg_t  msg;
    mmp_mngt_header_t               *pWkHdr = (mmp_mngt_header_t*)&msg;
    char                            *pWk    = (char*)(pWkHdr+1);
    int r,toread;

    *ppRcvDevMsg = 0;

    msg.hdr = *hdr;//hdr already in host byte order

    //header is already received.  continue reading response.
    toread = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t) - sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(s, pWk, toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        //known mmp_mngt_TdmfAgentDevicesMsg_t portion is read and stored to msg.
        //now read N devices info. that follows 
        msg.iNbrDevices = ntohl(msg.iNbrDevices);
        if ( msg.iNbrDevices > 0 )
        {   //read remainder of message in a CONTIGUOUS buffer
            int iMsgSize = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t) + msg.iNbrDevices*sizeof(mmp_TdmfDeviceInfo) ;
            char *pData = new char [ iMsgSize ];
            char *pWk = pData;

            //copy data already received to contiguous buffer
            memcpy( pWk, &msg, sizeof(mmp_mngt_TdmfAgentDevicesMsg_t) );
            pWk += sizeof(mmp_mngt_TdmfAgentDevicesMsg_t);

            //now ready to read the file data 
            toread = msg.iNbrDevices*sizeof(mmp_TdmfDeviceInfo);
            if ( toread != 0 )
            {
                r = mmp_mngt_sock_recv(s, (char*)pWk, toread); _ASSERT(r == toread);
            }
            else 
                r = toread;//config msg does not contain data

            if ( r == toread )
            {   //success, message entirely read
                *ppRcvDevMsg = (mmp_mngt_TdmfAgentDevicesMsg_t*)pData;
                //convert ntoh all device info structures received
                mmp_TdmfDeviceInfo *devInfo = (mmp_TdmfDeviceInfo *) ((*ppRcvDevMsg) + 1);
                for (int i=0; i<msg.iNbrDevices; i++ , devInfo++ )
                {
                    mmp_convert_TdmfAgentDeviceInfo_ntoh(devInfo);
                }
            }
            else
            {   //error
                delete [] pData;
            }
        }
        else
        {
            _ASSERT(0);
            //Illegal number of devices (%d) received from Agent %s 
        }
    }
}

//
// Release memory allocated by mmp_mngt_recv_alldevices_data() for 
// output ptr on mmp_mngt_TdmfAgentDevicesMsg_t message 
//
LIBMNGT_API
void mmp_mngt_free_alldevices_data(mmp_mngt_TdmfAgentDevicesMsg_t **ppRcvDevMsg)
{
    if ( ppRcvDevMsg != 0 && *ppRcvDevMsg != 0 )
    {
        char *p = (char*)*ppRcvDevMsg;
        delete [] p;//as allocated in mmp_mngt_recv_device_data()
        p = 0;
    }
}



// *********************************************************
void textfilebuffer_to_linelist(const char * pCfgData, unsigned int uiDataSize, listTextFileLine_t & listTextFileLines)
{
    const char *startline;
    const char *endline;
    const char *endofdata;
    //when necessary, replace  "\n" by "\r\n" on !_WINDOWS systems
    startline = pCfgData;
    endofdata = pCfgData + uiDataSize;
    while(startline < endofdata && endline != NULL)
    {
        oneTextFileLine  sline;

        endline = strpbrk( startline, "\r\n" );

        if ( endline != NULL )
        {
#if defined(_WINDOWS)
            //make sure line ends with \n
            sline.linelen   = endline - startline + 1;
            sline.line      = new char [sline.linelen];
            memcpy( sline.line, startline, sline.linelen - 1 );//data only, no \r or \n
            sline.line[sline.linelen - 1] = '\n';
#else
            //make sure line ends with \r\n
            sline.linelen   = endline - startline + 2;
            sline.line      = new char [sline.linelen];
            memcpy( sline.line, startline, sline.linelen - 2 );//data only, no \r or \n
            sline.line[sline.linelen - 2] = '\r';
            sline.line[sline.linelen - 1] = '\n';
#endif


            //add this line struct to the list
            listTextFileLines.push_back(sline);

            startline = endline + 1;//skip '\n' and position to start of next line
        }
    } ;
}

/*
 * Builds a mmp_mngt_ConfigurationMsg_t message ready to be sent on socket
 * (host to network convertions done)
 * pFileData can be NULL; uiDataSize can be 0.
 */ 
void 
mmp_mngt_build_SetConfigurationMsg(mmp_mngt_ConfigurationMsg_t **ppCfgMsg, 
                                   const char * szAgentId, bool  bIsHostId, int iSenderType,  
                                   const char *szCfgFileName, 
                                   const char* pFileData, unsigned int uiDataSize )
{
    int totallen;

    *ppCfgMsg = 0;

#if 0
    listTextFileLine_t listTextFileLines;
    textfilebuffer_to_linelist(pFileData,uiDataSize,listTextFileLines);
    //compute total size of data
    listTextFileLine_t::iterator it = listTextFileLines.begin();
    for(totallen=0 ; it != listTextFileLines.end(); it++)
    {
        totallen += (*it).linelen;
    }
#else
    totallen = uiDataSize;
#endif

    //build contiguous buffer for message 
    mmp_mngt_ConfigurationMsg_t  *msg;
    int iMsgSize = sizeof(mmp_mngt_ConfigurationMsg_t) + totallen;
    char *pMsg = new char [ iMsgSize ];
    msg = (mmp_mngt_ConfigurationMsg_t*)pMsg;

    //build msg 
    //header    
    msg->hdr.sendertype  = iSenderType;
    msg->hdr.mngttype    = MMP_MNGT_SET_LG_CONFIG;
    msg->hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg->hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&msg->hdr);
    
    msg->szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg->szServerUID, HOST_ID_PREFIX);
    strcat(msg->szServerUID,szAgentId);


    strcpy(msg->data.szFilename,szCfgFileName);
    msg->data.iType  = htonl(MMP_MNGT_FILETYPE_TDMF_CFG);
    msg->data.uiSize = htonl(totallen);
    
    if ( totallen > 0 )
    {
        char *pWk = (char*)(msg+1);
#if 0
        it = listTextFileLines.begin();
        for( ; it != listTextFileLines.end(); it++ )
        {
            memcpy(pWk, (*it).line, (*it).linelen );
            pWk += (*it).linelen;
        }
#else
        memcpy(pWk, pFileData, totallen );
#endif
    }

    *ppCfgMsg = (mmp_mngt_ConfigurationMsg_t*)pMsg;
}

/*
 * release memory allocated by mmp_mngt_build_SetConfigurationMsg()
 */
void
mmp_mngt_release_SetConfigurationMsg(mmp_mngt_ConfigurationMsg_t **ppCfgMsg)
{
    if ( *ppCfgMsg != 0 )
    {
        delete [] ((char*)*ppCfgMsg);
    }
}

/*
 * Builds a mmp_mngt_FileMsg_t message ready to be sent on socket
 * (host to network convertions done)
 * pFileData can be NULL; uiDataSize can be 0.
 */ 
void 
mmp_mngt_build_SetTdmfFileMsg(mmp_mngt_FileMsg_t **ppFileMsg, 
                          const char * szAgentId, 
						  bool  bIsHostId, 
						  int iSenderType,  
                          const char *szFileName, 
                          const char *pFileData, 
						  unsigned int uiDataSize,
						  int iFileType
						  )
{
    int totallen;
    *ppFileMsg = 0;
    totallen = uiDataSize;

    //build contiguous buffer for message 
    mmp_mngt_FileMsg_t  *msg;
    int iMsgSize = sizeof(mmp_mngt_FileMsg_t) + totallen;
    char *pMsg = new char [ iMsgSize ];
    msg = (mmp_mngt_FileMsg_t*)pMsg;

    //build msg 
    //header    
    msg->hdr.sendertype  = iSenderType;
    msg->hdr.mngttype    = MMP_MNGT_TDMF_SENDFILE;
    msg->hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg->hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&msg->hdr);
    
    msg->szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg->szServerUID, HOST_ID_PREFIX);
    strcat(msg->szServerUID,szAgentId);

	msg->data.iType  = htonl(iFileType);
    strcpy(msg->data.szFilename,szFileName);
    msg->data.uiSize = htonl(totallen);
    
    if ( totallen > 0 )
    {
        char *pWk = (char*)(msg+1);
        memcpy(pWk, pFileData, totallen );
    }

    *ppFileMsg = (mmp_mngt_FileMsg_t*)pMsg;
}

/*
 * release memory allocated by mmp_mngt_build_SetTdmfFileMsg()
 */
void
mmp_mngt_release_SetTdmfFileMsg(mmp_mngt_FileMsg_t **ppFileMsg)
{
    if ( *ppFileMsg != 0 )
    {
        delete [] ((char*)*ppFileMsg);
    }
}

// ***********************************************************
LIBMNGT_API 
int mmp_mngt_sendTdmfCommand( /*in*/ MMP_HANDLE handle, 
                              /*in*/ const char * szAgentId,
                              /*in*/ bool  bIsHostId,
                              /*in*/ enum tdmf_commands cmd,
                              /*in*/ const char * szCmdOptions, 
                              /*out*/char ** ppszCmdOutput )
{
    int r,ret;
    mmp_mngt_TdmfCommandMsg_t msg;
    mmp_mngt_TdmfCommandStatusMsg_t response,
                                    *presponse = &response;

    if ( ppszCmdOutput != NULL )
        *ppszCmdOutput = 0; 

    if ( !mmp_IsHandleValid(handle) )
        return -10;
    if ( strlen(szAgentId)+1 > sizeof(msg.szServerUID) )
        return -10;
    if ( strlen(szCmdOptions)+1 > sizeof(msg.szCmdOptions) )
        return -10;
    if ( cmd < FIRST_TDMF_CMD || cmd > LAST_TDMF_CMD )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_TDMF_CMD;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    //Agent id - machine name or host id 
    msg.szServerUID[0] = 0;
    if ( bIsHostId )
        strcat( msg.szServerUID, HOST_ID_PREFIX);
    strcat(msg.szServerUID,szAgentId);
    msg.iSubCmd         = htonl(cmd);
    strcpy(msg.szCmdOptions, szCmdOptions);

    //send Command to TDMF Collector, tobe relayed to Tdmf Agent.
    //expects Server to answer back with a mmp_mngt_TdmfCommandStatusMsg_t
    //ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&response, sizeof(response));_ASSERT(ret==sizeof(response));
    int recvd = 0;
    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, handle->iTDMFSrvIP, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r < 0 ) 
        return -1;
    r = sock_connect(s, handle->iPort); 
    if ( r < 0 ) 
    {
        sock_disconnect(s);
        return -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;
    }
    r = mmp_mngt_sock_send(s, (char*)&msg, sizeof(msg)); _ASSERT(r == sizeof(msg));
    if ( r != sizeof(msg) )
        return -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;

    int timeout = 0;
    do
    {   //one second timeout on socket
        r = sock_check_recv(s, 1000000);
    } while ( r == 0 && ++timeout < DEF_SOCK_RECV_CMD_STATUS_TIMEOUT );

    if ( r != 1 )
        //no data could be read or socket disconnected/closed/...
        return MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;

    //response data from Collector
    //first receive fixed portion
    r = mmp_mngt_sock_recv(s, (char*)&response, sizeof(response), MMP_MNGT_TIMEOUT_DMF_CMD + 10); _ASSERT(r == sizeof(response));
    if ( r != sizeof(response) )
        return -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;

    //analyse response 
    mmp_convert_mngt_hdr_ntoh( &response.hdr );
    response.iLength = ntohl(response.iLength);

    if ( response.iLength > 0 )
    {   //read output message from command
        char *szTmp = new char [response.iLength+1];
		szTmp[0] = 0;
        //response data from Collector
        //first receive fixed portion
        r = mmp_mngt_sock_recv(s, (char*)szTmp, response.iLength); _ASSERT(r == response.iLength);
        if ( r >= 0 )
        {   //ensure to have a valid EOS
            szTmp[r] = 0;
        }

        if ( ppszCmdOutput != NULL )
            *ppszCmdOutput = szTmp;

    }

    if ( response.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
    {
        _ASSERT(ntohl(response.iSubCmd) == cmd);
        ret = ntohl(response.iStatus);//a value from enum tdmf_commands_status
    }
    else
    {
        ret = -response.hdr.mngtstatus;
    }

    sock_delete(&s);
    return ret;
}


// ***********************************************************
int mmp_setTdmfPerfConfig(/*in*/ MMP_HANDLE handle, 
                          /*in*/ const mmp_TdmfPerfConfig * perfCfg
                          )
{
    mmp_mngt_TdmfPerfCfgMsg_t   msg;
    int                         ret;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_PERF_CFG_MSG;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg.data            = *perfCfg;

    mmp_convert_mngt_hdr_hton(&msg.hdr);
    mmp_convert_TdmfPerfConfig_hton(&msg.data);

    //send Command to TDMF Collector, to be relayed to all Tdmf Agents alive.
    //expects no response from Server 
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), 0, 0);
    if (ret == 0)
    {   //ok
    }

    return ret;
}


// ***********************************************************
LIBMNGT_API 
int mmp_registerNotificationMessages(/*in*/ MMP_HANDLE handle, 
                                     /*in-out*/sock_t *pNotifSocket,
                                     /*in*/ int iPeriod /*seconds*/
                                     )
{
    mmp_mngt_TdmfMonitorRegistrationMsg_t   msg;
    int ret,r;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_MONITORING_DATA_REGISTRATION;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;

    msg.data.iMaxDataRefreshPeriod = htonl(iPeriod);

    mmp_convert_mngt_hdr_hton(&msg.hdr);

    //register to Collector for Notification messages.
    //socket remains openned.
    r = sock_init( pNotifSocket, NULL, NULL, 0, handle->iTDMFSrvIP, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(pNotifSocket, handle->iPort); //_ASSERT(r>=0);
        if ( r >= 0 ) 
        {
            int sendlen = sizeof(msg);
            r = mmp_mngt_sock_send(pNotifSocket, (char*)&msg, sendlen ); _ASSERT(r == sendlen);
            if ( r == sendlen )
            {   //no response expected immediatly
                ret = MMP_MNGT_STATUS_OK;
            }
            else if ( r != 0 )
            {
                ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
            }
        }
        else 
		{
			sock_disconnect(pNotifSocket);
            ret = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;
		}
    }
    else 
        ret = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;

    return ret;
}

// ***********************************************************
LIBMNGT_API 
int mmp_TDMFOwnership(/*in*/ MMP_HANDLE handle, 
					  /*in*/ const char *szClientUID,
                      /*in*/ bool bRequestOwnership,
					  /*out*/bool * bOwnershipGranted
                      )
{
    mmp_mngt_TdmfCommonGuiRegistrationMsg_t   msg;
    int ret;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

	*bOwnershipGranted = false;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_TDMFCOMMONGUI_REGISTRATION;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    strncpy(msg.szClientUID,szClientUID,MMP_MNGT_MAX_CLIENT_UID_SZ);
    msg.bRequestOwnership = bRequestOwnership ? 1 : 0;

    //send Command to TDMF Collector.
    //Server responds by filling msg.szClientUID with '1' or '0'
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg));
    if (ret == sizeof(msg))
    {   //ok
		bool bOwner = true;
		//Collector return all '1' if we own the TDMF DB
		for ( int i=0; i<sizeof(msg.szClientUID) && bOwner; i++) {
			if ( msg.szClientUID[i] != '1' )
				bOwner = false;
		}
		*bOwnershipGranted = bOwner;
		return 0;
	}
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }

    return ret;
}

// ***********************************************************
// Function name	: mmp_setSystemParameters
// Description	    : sends various parameters to Collector
// Return type		: 0 = ok , < 0 = error
// Argument         : const mmp_TdmfCollectorParams* params
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_setSystemParameters(/*in*/ MMP_HANDLE handle, 
                                 /*in*/ const mmp_TdmfCollectorParams* params )
{
    mmp_mngt_TdmfCollectorParamsMsg_t   msg;
    int ret;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_SET_DB_PARAMS;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    msg.data = *params;
    mmp_convert_hton(&msg.data);

    //send Command to TDMF Collector.
    //Server responds by a mmp_mngt_header_t 
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg.hdr) );
    if (ret == sizeof(msg.hdr))
    {   //ok
        if ( msg.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
            ret = 0;
        else
            ret = -msg.hdr.mngtstatus;
    }
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }
    return ret;
}

// ***********************************************************
// Function name	: mmp_getSystemParameters
// Description	    : retreive actual various parameters used by Collector
// Return type		: 0 = ok , < 0 = error
// Argument         : mmp_TdmfCollectorParams* params
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_getSystemParameters(/*in*/ MMP_HANDLE handle, 
                                 /*out*/ mmp_TdmfCollectorParams* params )
{
    mmp_mngt_TdmfCollectorParamsMsg_t   msg;
    int ret;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_GET_DB_PARAMS;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);

    //send Command to TDMF Collector.
    //Server responds by a mmp_mngt_header_t 
    ret = mmp_sendmsg(handle->iTDMFSrvIP, handle->iPort, (const char*)&msg, sizeof(msg.hdr), (char*)&msg, sizeof(msg) );
    if (ret == sizeof(msg))
    {   //ok
        mmp_convert_mngt_hdr_ntoh(&msg.hdr);
        if ( msg.hdr.mngtstatus == MMP_MNGT_STATUS_OK )
        {
            ret = 0;
            mmp_convert_ntoh(&msg.data);
            *params = msg.data;
        }
        else
            ret = -msg.hdr.mngtstatus;
    }
    else
    {
        if ( ret == 0 )
        {   //unexpected comm. link error with TDMF Collector 
            ret = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;//undefined error
        }
    }
    return ret;
}



// ***********************************************************
// Function name	: mmp_convert_mngt_hdr_ntoh
// Description	    : Converts the int values received from a socket
//                    from network byte order to host byte order.
//                    network order is big-endian; host could be little-endian,
//                    (Intel family is)
// Return type		: void 
// Argument         : mmp_mngt_header_t *hdr
// 
// ***********************************************************
LIBMNGT_API 
void mmp_convert_mngt_hdr_ntoh(mmp_mngt_header_t *hdr)
{
    hdr->magicnumber = ntohl(hdr->magicnumber);
    hdr->mngttype    = ntohl(hdr->mngttype);
    hdr->sendertype  = ntohl(hdr->sendertype);
    hdr->mngtstatus  = ntohl(hdr->mngtstatus);
}


// ***********************************************************
// Function name	: mmp_convert_mngt_hdr_hton
// Description	    : Converts the int values to be sent on a socket
//                    from host byte order to network byte order.
//                    network order is big-endian; host could be little-endian,
//                    (Intel family is)
// Return type		: static void 
// Argument         : mmp_mngt_header_t *hdr
// 
// ***********************************************************
LIBMNGT_API 
void mmp_convert_mngt_hdr_hton(mmp_mngt_header_t *hdr)
{
    hdr->magicnumber = htonl(hdr->magicnumber);
    hdr->mngttype    = htonl(hdr->mngttype);
    hdr->sendertype  = htonl(hdr->sendertype);
    hdr->mngtstatus  = htonl(hdr->mngtstatus);
}


// ***********************************************************
LIBMNGT_API 
void mmp_convert_TdmfServerInfo_hton(mmp_TdmfServerInfo *srvrInfo)
{
    srvrInfo->iPort             =   htonl(srvrInfo->iPort);
    srvrInfo->iTCPWindowSize    =   htonl(srvrInfo->iTCPWindowSize);
    srvrInfo->iBABSizeReq       =   htonl(srvrInfo->iBABSizeReq);
    srvrInfo->iBABSizeAct       =   htonl(srvrInfo->iBABSizeAct);
    srvrInfo->iNbrCPU           =   htonl(srvrInfo->iNbrCPU);
    srvrInfo->iRAMSize          =   htonl(srvrInfo->iRAMSize);
    srvrInfo->iAvailableRAMSize =   htonl(srvrInfo->iAvailableRAMSize);
}

// ***********************************************************
LIBMNGT_API 
void mmp_convert_TdmfServerInfo_ntoh(mmp_TdmfServerInfo *srvrInfo)
{
    srvrInfo->iPort             =   ntohl(srvrInfo->iPort);
    srvrInfo->iTCPWindowSize    =   ntohl(srvrInfo->iTCPWindowSize);
    srvrInfo->iBABSizeReq       =   ntohl(srvrInfo->iBABSizeReq);
    srvrInfo->iBABSizeAct       =   ntohl(srvrInfo->iBABSizeAct);
    srvrInfo->iNbrCPU           =   ntohl(srvrInfo->iNbrCPU);
    srvrInfo->iRAMSize          =   ntohl(srvrInfo->iRAMSize);
    srvrInfo->iAvailableRAMSize =   ntohl(srvrInfo->iAvailableRAMSize);
}

// ***********************************************************
LIBMNGT_API 
void mmp_convert_TdmfAgentDeviceInfo_hton(mmp_TdmfDeviceInfo *devInfo)
{
    devInfo->sFileSystem    = htons(devInfo->sFileSystem);
}

// ***********************************************************
LIBMNGT_API 
void mmp_convert_TdmfAgentDeviceInfo_ntoh(mmp_TdmfDeviceInfo *devInfo)
{
    devInfo->sFileSystem    = ntohs(devInfo->sFileSystem);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfAlert_hton(mmp_TdmfAlertHdr *alertData)
{
    alertData->sLGid        = htons(alertData->sLGid);
    alertData->sDeviceId    = htons(alertData->sDeviceId);
    alertData->uiTimeStamp  = htonl(alertData->uiTimeStamp);
    alertData->cSeverity    = (char)htons((short)alertData->cSeverity);
    alertData->cType        = (char)htons((short)alertData->cType);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfAlert_ntoh(mmp_TdmfAlertHdr *alertData)
{
    alertData->sLGid        = ntohs(alertData->sLGid);
    alertData->sDeviceId    = ntohs(alertData->sDeviceId);
    alertData->uiTimeStamp  = ntohl(alertData->uiTimeStamp);
    alertData->cSeverity    = (char)ntohs((short)alertData->cSeverity);
    alertData->cType        = (char)ntohs((short)alertData->cType);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfStatusMsg_hton(mmp_TdmfStatusMsg *statusMsg)
{
    statusMsg->iLength      = htonl(statusMsg->iLength);
    statusMsg->iTdmfCmd     = htonl(statusMsg->iTdmfCmd);
    statusMsg->iTimeStamp   = htonl(statusMsg->iTimeStamp);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfStatusMsg_ntoh(mmp_TdmfStatusMsg *statusMsg)
{
    statusMsg->iLength      = ntohl(statusMsg->iLength);
    statusMsg->iTdmfCmd     = ntohl(statusMsg->iTdmfCmd);
    statusMsg->iTimeStamp   = ntohl(statusMsg->iTimeStamp);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfPerfData_hton(mmp_TdmfPerfData *perfData)
{
    perfData->iPerfDataSize = htonl(perfData->iPerfDataSize);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfPerfData_ntoh(mmp_TdmfPerfData *perfData)
{
    perfData->iPerfDataSize = ntohl(perfData->iPerfDataSize);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfPerfConfig_hton(mmp_TdmfPerfConfig *perfCfg)
{
    perfCfg->iPerfUploadPeriod   = htonl(perfCfg->iPerfUploadPeriod);
    perfCfg->iReplGrpMonitPeriod = htonl(perfCfg->iReplGrpMonitPeriod);
}

// ***********************************************************
LIBMNGT_API 
void    mmp_convert_TdmfPerfConfig_ntoh(mmp_TdmfPerfConfig *perfCfg)
{
    perfCfg->iPerfUploadPeriod   = ntohl(perfCfg->iPerfUploadPeriod);
    perfCfg->iReplGrpMonitPeriod = ntohl(perfCfg->iReplGrpMonitPeriod);
}

// ***********************************************************
LIBMNGT_API 
__int64     htonI64( __int64 int64InVal )
{
    __int64     int64OutVal;
    unsigned int *pIntOutVal = (unsigned int*)&int64OutVal;
    unsigned int *pIntInVal  = (unsigned int*)&int64InVal;

    pIntOutVal++;
	*pIntOutVal     =   htonl( *pIntInVal );//first 32-bits
    pIntInVal++;
    pIntOutVal--;
	*pIntOutVal     =   htonl( *pIntInVal );//last 32-bits

    return int64OutVal;
}

// ***********************************************************
LIBMNGT_API 
__int64     ntohI64( __int64 int64InVal )
{
    __int64     int64OutVal;
    unsigned int         *pIntOutVal = (unsigned int*)&int64OutVal;
    unsigned int         *pIntInVal  = (unsigned int*)&int64InVal;

    pIntOutVal++;
	*pIntOutVal     =   ntohl( *pIntInVal );//first 32-bits
    pIntInVal++;
    pIntOutVal--;
	*pIntOutVal     =   ntohl( *pIntInVal );//last 32-bits

    return int64OutVal;
}

// ***********************************************************
void    mmp_convert_hton(ftd_perf_instance_t *perf)
{
    perf->connection =   (int)htonl(perf->connection);
    perf->drvmode    =   (int)htonl(perf->drvmode);
    perf->lgnum      =   (int)htonl(perf->lgnum);
    perf->insert     =   (int)htonl(perf->insert);
    perf->devid      =   (int)htonl(perf->devid);
    perf->actual     =   htonI64(perf->actual);
    perf->effective  =   htonI64(perf->effective);
    perf->rsyncoff   =   (int)htonl(perf->rsyncoff);
    perf->rsyncdelta =   (int)htonl(perf->rsyncdelta);
    perf->entries    =   (int)htonl(perf->entries);
    perf->sectors    =   (int)htonl(perf->sectors);
    perf->pctdone    =   (int)htonl(perf->pctdone);
    perf->pctbab     =   (int)htonl(perf->pctbab);
    perf->bytesread  =   htonI64(perf->bytesread);
    perf->byteswritten = htonI64(perf->byteswritten);
}

// ***********************************************************
void    mmp_convert_ntoh(ftd_perf_instance_t *perf)
{
    perf->connection =   (int)ntohl(perf->connection);
    perf->drvmode    =   (int)ntohl(perf->drvmode);
    perf->lgnum      =   (int)ntohl(perf->lgnum);
    perf->insert     =   (int)ntohl(perf->insert);
    perf->devid      =   (int)ntohl(perf->devid);
    perf->actual     =   ntohI64(perf->actual);
    perf->effective  =   ntohI64(perf->effective);
    perf->rsyncoff   =   (int)ntohl(perf->rsyncoff);
    perf->rsyncdelta =   (int)ntohl(perf->rsyncdelta);
    perf->entries    =   (int)ntohl(perf->entries);
    perf->sectors    =   (int)ntohl(perf->sectors);
    perf->pctdone    =   (int)ntohl(perf->pctdone);
    perf->pctbab     =   (int)ntohl(perf->pctbab);
    perf->bytesread  =   ntohI64(perf->bytesread);
    perf->byteswritten = ntohI64(perf->byteswritten);
}

// ***********************************************************
void    mmp_convert_hton(mmp_TdmfGroupState *grpstate)
{
	grpstate->sRepGrpNbr	=	htons(grpstate->sRepGrpNbr);
	grpstate->sState		=	htons(grpstate->sState);
}

// ***********************************************************
void    mmp_convert_ntoh(mmp_TdmfGroupState *grpstate)
{
	grpstate->sRepGrpNbr	=	ntohs(grpstate->sRepGrpNbr);
	grpstate->sState		=	ntohs(grpstate->sState);
}

// ***********************************************************
void    mmp_convert_hton(mmp_TdmfReplGroupMonitor *grpmonit)
{
	grpmonit->sReplGrpNbr	    =	htons(grpmonit->sReplGrpNbr);
	grpmonit->iReplGrpSourceIP	=	htonl(grpmonit->iReplGrpSourceIP);
    grpmonit->liActualSz        =   htonI64(grpmonit->liActualSz);
    grpmonit->liDiskFreeSz      =   htonI64(grpmonit->liDiskFreeSz);
    grpmonit->liDiskTotalSz     =   htonI64(grpmonit->liDiskTotalSz);
}

// ***********************************************************
void    mmp_convert_ntoh(mmp_TdmfReplGroupMonitor *grpmonit)
{
	grpmonit->sReplGrpNbr	    =	ntohs(grpmonit->sReplGrpNbr);
	grpmonit->iReplGrpSourceIP	=	ntohl(grpmonit->iReplGrpSourceIP);
    grpmonit->liActualSz        =   ntohI64(grpmonit->liActualSz);
    grpmonit->liDiskFreeSz      =   ntohI64(grpmonit->liDiskFreeSz);
    grpmonit->liDiskTotalSz     =   ntohI64(grpmonit->liDiskTotalSz);
    
}

// ***********************************************************
void    mmp_convert_hton(mmp_TdmfCollectorParams *dbparams)
{
    dbparams->iCollectorBroadcastIPMask     =   htonl(dbparams->iCollectorBroadcastIPMask);
    dbparams->iCollectorBroadcastTimeout    =   htonl(dbparams->iCollectorBroadcastTimeout);
    dbparams->iCollectorTcpPort             =   htonl(dbparams->iCollectorTcpPort);
    dbparams->iDBAlertStatusTableMaxDays    =   htonl(dbparams->iDBAlertStatusTableMaxDays);
    dbparams->iDBAlertStatusTableMaxNbr     =   htonl(dbparams->iDBAlertStatusTableMaxNbr);
    dbparams->iDBCheckPeriodMinutes         =   htonl(dbparams->iDBCheckPeriodMinutes);
    dbparams->iDBPerformanceTableMaxNbr     =   htonl(dbparams->iDBPerformanceTableMaxNbr);
    dbparams->iDBPerformanceTableMaxDays    =   htonl(dbparams->iDBPerformanceTableMaxDays);
}

// ***********************************************************
void    mmp_convert_ntoh(mmp_TdmfCollectorParams *dbparams)
{
    dbparams->iCollectorBroadcastIPMask     =   ntohl(dbparams->iCollectorBroadcastIPMask);
    dbparams->iCollectorBroadcastTimeout    =   ntohl(dbparams->iCollectorBroadcastTimeout);
    dbparams->iCollectorTcpPort             =   ntohl(dbparams->iCollectorTcpPort);
    dbparams->iDBAlertStatusTableMaxDays    =   ntohl(dbparams->iDBAlertStatusTableMaxDays);
    dbparams->iDBAlertStatusTableMaxNbr     =   ntohl(dbparams->iDBAlertStatusTableMaxNbr);
    dbparams->iDBCheckPeriodMinutes         =   ntohl(dbparams->iDBCheckPeriodMinutes);
    dbparams->iDBPerformanceTableMaxNbr     =   ntohl(dbparams->iDBPerformanceTableMaxNbr);
    dbparams->iDBPerformanceTableMaxDays    =   ntohl(dbparams->iDBPerformanceTableMaxDays);
}

// ***********************************************************
void    mmp_convert_hton(mmp_TdmfAgentState *agentState)
{
    agentState->iState      = htonl(agentState->iState);
}

// ***********************************************************
void    mmp_convert_ntoh(mmp_TdmfAgentState *agentState)
{
    agentState->iState      = ntohl(agentState->iState);
}

// ***********************************************************
void    mmp_convert_ntoh(mmp_TdmfFileTransferData *data)
{
    data->iType     = ntohl(data->iType);
    data->uiSize    = ntohl(data->uiSize);
}

// ***********************************************************
void    mmp_convert_hton(mmp_TdmfFileTransferData *data)
{
    data->iType     = htonl(data->iType);
    data->uiSize    = htonl(data->uiSize);
}

// ***********************************************************
LIBMNGT_API 
void mmp_mngt_recv_cfg_data( /*in*/sock_t *s, /*in*/mmp_mngt_header_t *msghdr, /*out*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData) 
{
    int r;
    int toread ;
    mmp_mngt_ConfigurationMsg_t agentCfg;

    *ppRcvCfgData = 0;

    //msg header as been read by caller and hdr is availble from msghdr.
    if ( msghdr != NULL )
        agentCfg.hdr = *msghdr;

    //reads other mmp_mngt_ConfigurationMsg_t fields
    toread = sizeof(agentCfg.szServerUID);
    r = mmp_mngt_sock_recv(s, (char*)agentCfg.szServerUID, toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        //now ready to read remainder of mmp_mngt_ConfigurationMsg_t msg
        toread = sizeof(agentCfg.data);
        r = mmp_mngt_sock_recv(s, (char*)&agentCfg.data, toread); _ASSERT(r == toread);
        if ( r == toread )
        {
            agentCfg.data.iType  = ntohl(agentCfg.data.iType);
            agentCfg.data.uiSize = ntohl(agentCfg.data.uiSize);

            //read remainder of message in a CONTIGUOUS buffer
            int iMsgSize = sizeof(mmp_mngt_ConfigurationMsg_t) + agentCfg.data.uiSize ;
            char *pData = new char [ iMsgSize ];
            char *pWk = pData;

            //copy data already received to contiguous buffer
            memcpy( pWk, &agentCfg.hdr, sizeof(agentCfg.hdr) );
            pWk += sizeof(agentCfg.hdr);
            memcpy( pWk, &agentCfg.szServerUID, sizeof(agentCfg.szServerUID) );
            pWk += sizeof(agentCfg.szServerUID);
            memcpy( pWk, &agentCfg.data, sizeof(agentCfg.data) );
            pWk += sizeof(agentCfg.data);

            //now ready to read the file data 
            toread = (int)agentCfg.data.uiSize;
            if ( toread != 0 )
            {
                r = mmp_mngt_sock_recv(s, (char*)pWk, toread); _ASSERT(r == toread);
            }
            else 
                r = toread;//config msg does not contain data

            if ( r == toread )
            {   //success, message entirely read
                *ppRcvCfgData = (mmp_mngt_ConfigurationMsg_t*)pData;
            }
            else
            {
                delete [] pData;
            }
        }
    }
}

// ***********************************************************
//to release ptr allocated by mmp_mngt_recv_cfg_data()
void mmp_mngt_free_cfg_data_mem( /*in*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData) 
{
    if ( *ppRcvCfgData != 0 )
    {
        delete [] ((char*)*ppRcvCfgData);
    }
}

// *****************************************************************
// send GETCONFIG msg and wait to receive response (SETCONFIG) msg
// *****************************************************************
static 
int  mmp_mngt_request_cfg_data( int rip, int rport, /*in*/mmp_mngt_ConfigurationMsg_t *pReqCfg, /*out*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData) 
{
    int r,toread,towrite;

    *ppRcvCfgData= 0;

    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, rport); //_ASSERT(r>=0);
        if ( r >= 0 ) 
        {
            towrite = sizeof(mmp_mngt_ConfigurationMsg_t);
            r = mmp_mngt_sock_send(s, (char*)pReqCfg, towrite); _ASSERT(r == towrite);
            if ( r == towrite )
            {
                mmp_mngt_header_t hdr;

                toread = sizeof(hdr);
                r = mmp_mngt_sock_recv(s, (char*)&hdr, toread ); _ASSERT(r == toread);
                if ( r == toread )
                {
                    mmp_convert_mngt_hdr_ntoh(&hdr);
                    _ASSERT(hdr.magicnumber == MNGT_MSG_MAGICNUMBER);
                    _ASSERT(hdr.mngttype    == MMP_MNGT_SET_LG_CONFIG);
                    _ASSERT(hdr.sendertype  == SENDERTYPE_TDMF_SERVER);

                    if ( hdr.magicnumber == MNGT_MSG_MAGICNUMBER &&
                         hdr.mngttype    == MMP_MNGT_SET_LG_CONFIG     &&                         
                         hdr.mngtstatus  == MMP_MNGT_STATUS_OK    )
                    {
                        mmp_mngt_recv_cfg_data(s, &hdr, ppRcvCfgData);
                        //on success, *ppRcvCfgData points to non-NULL address containing a mmp_mngt_ConfigurationMsg_t msg
                        r = 0;
                    }
                    else
                    {
                        if ( hdr.mngtstatus != MMP_MNGT_STATUS_OK )
                            r = -hdr.mngtstatus;// <= -100
                        else
                            r = -5;
                    }
                }
                else
                {
                    r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
                }
            }
            else 
            {
                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
            }
        }
        else
        {
            r = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;//-100
        }
        sock_disconnect(s);
    }  
    else
    {
        r = -1;
    }
    sock_delete(&s);
    return r;
}

// ***********************************************************************
// send TDMF_GETFILE msg and wait to receive response (TDMF_SENDFILE) msg
// ***********************************************************************
static 
int  mmp_mngt_request_tdmf_file( int rip, int rport, /*in*/mmp_mngt_FileMsg_t *pReqFileMsg, /*out*/mmp_mngt_FileMsg_t ** ppRcvFileMsg) 
{
    int r,toread,towrite;

    *ppRcvFileMsg= 0;

    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, rport); //_ASSERT(r>=0);
        if ( r >= 0 ) 
        {
            towrite = sizeof(mmp_mngt_FileMsg_t);
            r = mmp_mngt_sock_send(s, (char*)pReqFileMsg, towrite); _ASSERT(r == towrite);
            if ( r == towrite )
            {
                mmp_mngt_header_t hdr;

                toread = sizeof(hdr);
                r = mmp_mngt_sock_recv(s, (char*)&hdr, toread ); _ASSERT(r == toread);
                if ( r == toread )
                {
                    mmp_convert_mngt_hdr_ntoh(&hdr);
                    _ASSERT(hdr.magicnumber == MNGT_MSG_MAGICNUMBER);
                    _ASSERT(hdr.mngttype    == MMP_MNGT_TDMF_SENDFILE);
                    _ASSERT(hdr.sendertype  == SENDERTYPE_TDMF_SERVER);

                    if ( hdr.magicnumber == MNGT_MSG_MAGICNUMBER &&
                         hdr.mngttype    == MMP_MNGT_TDMF_SENDFILE &&                         
                         hdr.mngtstatus  == MMP_MNGT_STATUS_OK    )
                    {
                        mmp_mngt_recv_file_data(s, &hdr, ppRcvFileMsg);
                        //on success, *ppRcvFileMsg points to non-NULL address containing a mmp_mngt_ConfigurationMsg_t msg
                        r = 0;
                    }
                    else
                    {
                        if ( hdr.mngtstatus != MMP_MNGT_STATUS_OK )
                            r = -hdr.mngtstatus;// <= -100
                        else
                            r = -5;
                    }
                }
                else
                {
                    r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
                }
            }
            else 
            {
                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
            }
        }
        else
        {
            r = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;//-100
        }
        sock_disconnect(s);
    }  
    else
    {
        r = -1;
    }
    sock_delete(&s);
    return r;
}


#if 0
//initial version 
void mmp_mngt_recv_cfg_data( /*in*/sock_t *s, /*in*/mmp_mngt_header_t *msghdr, /*out*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData) 
{
    int r;
    int toread ;
    mmp_mngt_ConfigurationMsg_t agentCfg;


    *ppRcvCfgData = 0;

    //msg header as been read by caller and hdr is availble from msghdr.
    if ( msghdr != NULL )
        agentCfg.hdr = *msghdr;

    //reads other mmp_mngt_ConfigurationMsg_t fields
    toread = sizeof(agentCfg.szServerUID);
    r = mmp_mngt_sock_recv(s, (char*)agentCfg.szServerUID, toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        //now ready to read remainder of mmp_mngt_ConfigurationMsg_t msg
        toread = sizeof(agentCfg.lgcfghdr);
        r = mmp_mngt_sock_recv(s, (char*)&agentCfg.lgcfghdr, toread); _ASSERT(r == toread);
        if ( r == toread )
        {
            int iNbrDev   = ntohl(agentCfg.lgcfghdr.uiNbrDevices);
            int iNbrThrot = ntohl(agentCfg.lgcfghdr.uiNbrThrottles);
            _ASSERT(iNbrThrot == 0);//not supporting throttles yet 

            //read remainder of message in a CONTIGUOUS buffer
            int iMsgSize = sizeof(mmp_mngt_ConfigurationMsg_t) + iNbrDev*sizeof(mmp_TdmfLGDeviceCfg) ;//+ iNbrThrot*sizeof(mmp_TdmfLGThrottleCfg);
            char *pData = new char [ iMsgSize ];
            char *pWk = pData;

            //copy data already received to contiguous buffer
            memcpy( pWk, &agentCfg.hdr, sizeof(agentCfg.hdr) );
            pWk += sizeof(agentCfg.hdr);
            memcpy( pWk, &agentCfg.szServerUID, sizeof(agentCfg.szServerUID) );
            pWk += sizeof(agentCfg.szServerUID);

            mmp_TdmfLGCfgHdr *pLGCfgHdr = (mmp_TdmfLGCfgHdr *)pWk;

            memcpy( pWk, &agentCfg.lgcfghdr, sizeof(agentCfg.lgcfghdr) );
            pWk += sizeof(agentCfg.lgcfghdr);
            //now pWk points after the mmp_mngt_ConfigurationMsg_t portion, ready to read the list of devices cfg data


            //now ready to read all the logical groups devices cfg data 
            toread = iNbrDev*sizeof(mmp_TdmfLGDeviceCfg);
            r = mmp_mngt_sock_recv(s, (char*)pWk, toread); _ASSERT(r == toread);
            if ( r == toread )
            {
                //Throttles
                /*
                pWk += iNbrDev*sizeof(mmp_TdmfLGDeviceCfg);
                //now ready to read all the logical groups throttles cfg data 
                toread = iNbrThrot*sizeof(mmp_TdmfLGThrottleCfg);
                r = mmp_mngt_sock_recv(s, (char*)pWk, toread ); _ASSERT(r == toread);
                if ( r == toread )
                {

                */

                    mmp_convert_TdmfLGConfiguration_ntoh(pLGCfgHdr);

                    //ptr to completly received message hdr and data
                    *ppRcvCfgData  = (mmp_mngt_ConfigurationMsg_t*)pData;

                //Throttles
                /*
                }
                */


            }
            else
            {
                delete [] pData;
            }
        }
    }
}
#endif


// ***********************************************************
// Function name	: mmp_sendmsg
// Description	    : sends a msg and, if required, receive response message 
//                    expected to be used to send msg to, and optionally recv response from, TDMF Collector.
// Return type		: int > 0 : nbr bytes sent or received, 
//                    otherwise :
//                     0 = socket closed by remote host while sending/recv operation.
//                    -1 = memory allocation failure
//                    -2 = unable to connect to specified remote host ip / port .
//                    -3 = error while sending data to remote host
//                    -4 = error while receiving data from remote host
// Argument         : unsigned long rip
// Argument         : const char* senddata
// Argument         : int sendlen
// Argument         : char* rcvdata
// Argument         : int *rcvsize
// 
// ***********************************************************
int mmp_sendmsg(unsigned long rip, unsigned long rport, const char* senddata, int sendlen, char* rcvdata, int rcvsize) 
{
    int r;
    int recvd = 0;

    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r >= 0 ) 
    {
        r = sock_connect(s, rport); //_ASSERT(r>=0);
        if ( r >= 0 ) 
        {
            r = mmp_mngt_sock_send(s, (char*)senddata, sendlen); _ASSERT(r == sendlen);
            if ( r == sendlen )
            {
                if ( rcvdata != NULL )
                {
                    int timeout = 0;
                    do
                    {   //one second timeout on socket
                        r = sock_check_recv(s, 1000000);
                    } while ( r == 0 && ++timeout < DEF_SOCK_RECV_TIMEOUT );

                    if ( r == 1 )
                    {   //data ready to be read
                        r = mmp_mngt_sock_recv(s, rcvdata, rcvsize); _ASSERT(r == rcvsize);
                        if ( r >= 0 )
                        {
                            recvd = r;//n bytes received
                        }
                        else if ( r != 0 )
                        {
                            r = -4;
                        }
                    }
                    else
                    {   //no data could be read or socket disconnected/closed/...
                        if ( r != 0 )
                        {
                            r = -4;//error while receiving data from remote host
                        }
                    }
                }
            }
            else if ( r != 0 )
            {
                r = -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;
            }
        }
        else
        {
            r = -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;
        }
        sock_disconnect(s);
    }  
    else
    {
        r = -1;
    }
    sock_delete(&s);
    return r < 0 ? r : recvd;
}


// ***********************************************************
// send data on socket.
// perform multiple sock_send() if necessary.
LIBMNGT_API 
int     mmp_mngt_sock_send(sock_t *s, char *buf, int tosend)
{
    int sent = 0;
    int r;

    do
    {
        r = sock_send(s,buf+sent,tosend-sent);
        if ( r > 0 )
        {
            sent += r;
        }
    } while( r > 0 && sent < tosend );

    return sent;
}


// ***********************************************************
// receive data from socket.
// perform multiple sock_recv() if necessary.
LIBMNGT_API 
int     mmp_mngt_sock_recv(sock_t *s, char *buf, int torecv, int timeout )
{
    int rcvd = 0;
    int r;

    do
    {   //basic thirty second timeout for some data to be received on socket
        r = sock_check_recv(s, timeout*1000000);
        if ( r == 1 )
        {
            r = sock_recv(s,buf+rcvd,torecv-rcvd);
            if ( r > 0 )
            {
                rcvd += r;
            }
        }
        else
        {   //to exit loop
            r = -1;
        }
    } while( r > 0 && rcvd < torecv );

    return rcvd;
}


// ***********************************************************
static 
bool mmp_IsHandleValid(MMP_HANDLE handle)
{
    return handle != NULL && handle->iSignature == mmp_handle::iReferenceSignature;
}


static const char *gszMMPMsgTypeText[]=
{
     "MMP_MNGT_SET_LG_CONFIG"	     
    ,"MMP_MNGT_SET_CONFIG_STATUS"
    ,"MMP_MNGT_GET_LG_CONFIG"
    ,"MMP_MNGT_AGENT_INFO_REQUEST"
    ,"MMP_MNGT_AGENT_INFO"
    ,"MMP_MNGT_REGISTRATION_KEY"
    ,"MMP_MNGT_TDMF_CMD"
    ,"MMP_MNGT_TDMF_CMD_STATUS"
    ,"MMP_MNGT_SET_AGENT_GEN_CONFIG"
    ,"MMP_MNGT_GET_AGENT_GEN_CONFIG"
    ,"MMP_MNGT_GET_ALL_DEVICES"
    ,"MMP_MNGT_SET_ALL_DEVICES"
    ,"MMP_MNGT_ALERT_DATA"
    ,"MMP_MNGT_STATUS_MSG"
    ,"MMP_MNGT_PERF_MSG"
    ,"MMP_MNGT_PERF_CFG_MSG"
    ,"MMP_MNGT_MONITORING_DATA_REGISTRATION"
    ,"MMP_MNGT_AGENT_ALIVE_SOCKET"
    ,"MMP_MNGT_AGENT_STATE"
    ,"MMP_MNGT_GROUP_STATE"
    ,"MMP_MNGT_TDMFCOMMONGUI_REGISTRATION"
    ,"MMP_MNGT_GROUP_MONITORING"
    ,"MMP_MNGT_SET_DB_PARAMS"
    ,"MMP_MNGT_GET_DB_PARAMS"
    ,"MMP_MNGT_AGENT_TERMINATE"
    ,"MMP_MNGT_GUI_MSG"
    ,"MMP_MNGT_COLLECTOR_STATE"
    ,"MMP_MNGT_TDMF_SENDFILE"
    ,"MMP_MNGT_TDMF_SENDFILE_STATUS"
    ,"MMP_MNGT_TDMF_GETFILE"
    ,"unexpected MMP msg type!"
};
static int giNbrMMPTypeText = sizeof(gszMMPMsgTypeText) / sizeof(char*) ;

const char* mmp_mngt_getMsgTypeText(int iMMPType)
{
    if ( iMMPType - MMP_MNGT_SET_LG_CONFIG <= giNbrMMPTypeText )
    {
        return gszMMPMsgTypeText[ iMMPType - MMP_MNGT_SET_LG_CONFIG ];
    }
    else
    {
        return gszMMPMsgTypeText[ giNbrMMPTypeText - 1 ];
    }
}

// ***********************************************************
LIBMNGT_API 
int mmp_mngt_sendGuiMsg( /*in*/ MMP_HANDLE handle, 
                         /*in*/ const BYTE* Msg,
						 /*in*/ const UINT  nLength)
{
    int r;
	mmp_mngt_TdmfGuiMsg_t msg;

    if ( !mmp_IsHandleValid(handle) )
        return -10;

    //header    
    msg.hdr.sendertype  = SENDERTYPE_TDMF_CLIENTAPP;
    msg.hdr.mngttype    = MMP_MNGT_GUI_MSG;
    msg.hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    mmp_convert_mngt_hdr_hton(&msg.hdr);
 
    memcpy(msg.data.szMsg, Msg, nLength);

    //send Msg to TDMF Collector, to be relayed to all other gui.
    int recvd = 0;
    sock_t * s = sock_create();
    r = sock_init( s, NULL, NULL, 0, handle->iTDMFSrvIP, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);
    if ( r < 0 ) 
        return -1;
    r = sock_connect(s, handle->iPort); 
    if ( r < 0 ) 
    {
        sock_disconnect(s);
        return -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR;
    }
    r = mmp_mngt_sock_send(s, (char*)&msg, sizeof(msg)); _ASSERT(r == sizeof(msg));
    if ( r != sizeof(msg) )
        return -MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR;

    sock_delete(&s);

    return 0;
}

// ***********************************************************
LIBMNGT_API 
void mmp_mngt_recv_file_data( /*in*/sock_t *s, /*in*/mmp_mngt_header_t *msghdr, /*out*/mmp_mngt_FileMsg_t ** ppRcvFileData) 
{
    int r;
    int toread ;
    mmp_mngt_FileMsg_t agentFile;

    *ppRcvFileData = 0;

    //msg header as been read by caller and hdr is availble from msghdr.
    if ( msghdr != NULL )
        agentFile.hdr = *msghdr;

    //reads other mmp_mngt_FileMsg_t fields
    toread = sizeof(agentFile.szServerUID);
    r = mmp_mngt_sock_recv(s, (char*)agentFile.szServerUID, toread); _ASSERT(r == toread);
    if ( r == toread )
    {
        //now ready to read remainder of mmp_mngt_FileMsg_t msg
        toread = sizeof(agentFile.data);
        r = mmp_mngt_sock_recv(s, (char*)&agentFile.data, toread); _ASSERT(r == toread);
        if ( r == toread )
        {
            agentFile.data.iType  = ntohl(agentFile.data.iType);
            agentFile.data.uiSize = ntohl(agentFile.data.uiSize);

            //read remainder of message in a CONTIGUOUS buffer
            int iMsgSize = sizeof(mmp_mngt_FileMsg_t) + agentFile.data.uiSize ;
            char *pData = new char [ iMsgSize ];
            char *pWk = pData;

            //copy data already received to contiguous buffer
            memcpy( pWk, &agentFile.hdr, sizeof(agentFile.hdr) );
            pWk += sizeof(agentFile.hdr);
            memcpy( pWk, &agentFile.szServerUID, sizeof(agentFile.szServerUID) );
            pWk += sizeof(agentFile.szServerUID);
            memcpy( pWk, &agentFile.data, sizeof(agentFile.data) );
            pWk += sizeof(agentFile.data);

            //now ready to read the file data 
            toread = (int)agentFile.data.uiSize;
            if ( toread != 0 )
            {
                r = mmp_mngt_sock_recv(s, (char*)pWk, toread); _ASSERT(r == toread);
            }
            else 
                r = toread;//config msg does not contain data

            if ( r == toread )
            {   //success, message entirely read
                *ppRcvFileData = (mmp_mngt_FileMsg_t*)pData;
            }
            else
            {
                delete [] pData;
            }
        }
    }
}

// ***********************************************************
//to release ptr allocated by mmp_mngt_recv_cfg_data()
void mmp_mngt_free_file_data_mem( /*in*/mmp_mngt_FileMsg_t ** ppRcvFileData) 
{
    if ( *ppRcvFileData != 0 )
    {
        delete [] ((char*)*ppRcvFileData);
    }
}
