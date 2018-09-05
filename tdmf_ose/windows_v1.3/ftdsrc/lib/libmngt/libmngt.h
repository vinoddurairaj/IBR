/*
 * libmngt.h -	interface provided by TDMF Collector to 
 *				access its all services.
 * 
 * Copyright (c) 2002 Fujitsu SoftTek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef __LIBMNGT_H_
#define __LIBMNGT_H_

#include "libmngtdef.h" 

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBMNGT_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBMNGT_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
//#ifdef LIBMNGT_EXPORTS
#define LIBMNGT_API 

//avoid C++ name mangling problems when imported/used by C client program
#ifdef __cplusplus 
extern "C" {
#endif

//
//opaque structure handle for clients of this Library
//
#ifndef MMP_HANDLE
#define	MMP_HANDLE			void*
#endif


//*****************************************************************************
// API Functions declarations
//*****************************************************************************

// ***********************************************************
// Function name	: mmp_create
// Description	    : 
// Return type		: MMP_HANDLE 
// Argument         : unsigned int iTDMFSrvIP  IP address of TDMF
// 
// ***********************************************************
LIBMNGT_API 
MMP_HANDLE  mmp_Create(char *szTDMFSrvIP, int iTDMFSrvPort);

// ***********************************************************
// Function name	: mmp_destroy
// Description	    : 
// Return type		: 
// Argument         : MMP_HANDLE handle previously returned by mmp_create()
// 
// ***********************************************************
LIBMNGT_API 
void    mmp_Destroy( /*in*/ MMP_HANDLE handle);


// ***********************************************************
// Function name	: mmp_getAllTdmfAgentInfo
// Description	    : Sends request to refresh TDMF Agents info
//                    in database.  Upon success, the TDMF DB
//                    can later be accessed to retreive updated 
//                    TDMF agent information.
// Return type		: int 0 = ok, < 0 = error.
// Argument         : MMP_HANDLE handle from mmp_Create()
// 
// ***********************************************************
LIBMNGT_API 
int mmp_getAllTdmfAgentInfo( /*in*/ MMP_HANDLE handle );


// ***********************************************************
// Function name	: mmp_getTdmfAgentRegistrationKey
// Description	    : The application calls this function to retreive 
//                    the Registration key directly from a TDMF Agent.  
//                    This request should be performed when the key is not found 
//                    in the TDMF database. 
//                    When this call returns, the client can access the TDMF database 
//                    and retreive the Tdmf agent key.
//
// Return type		: int 0 = ok, < 0 = error.
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -10 = invalid argument
//                    -11 = provided output buffer too small.
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*out*/ char * szRegKey : optional.  Can be NULL.
//                      Upon success, buffer where the registration key string 
//                      can be copied.  
// Argument         : /*in*/ len : capacity in bytes of szRegKey buffer 
// Argument         : /*out*/*pExpirationTime : upon successful retreival of a valid key,
//                          the provided address is filled with a number of seconds since 1970/01/01
//                          to describe the expiration date-time for this key.
//                          Expiration time should considered as a valid time value only if value is > 1.
//                          If value <= 0, it refers to a KEY_... status value listed in libmngtdef.h, 
//                          see mmp_TdmfRegistrationKey definition.
// 
// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfAgentRegistrationKey(/*in*/ MMP_HANDLE handle, 
                                    /*in*/ const char * szAgentId,
                                    /*in*/ bool  bIsHostId,
                                    /*out*/char * szRegKey,
                                    /*in*/ int len, 
                                    /*out*/int *pExpirationTime = 0 );

// ***********************************************************
// Function name	: mmp_setTdmfAgentRegistrationKey
// Description	    : The application calls this function to provide
//                    a new Registration key to the specified TDMF Agent.  
//                    When this call returns, the client can access the TDMF database 
//                    and retreive the Tdmf agent key.
//
// Return type		: int 0 = ok, < 0 = error
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ const char * szRegKey : string containing 
//                      the Registration Key to be assigned and sent to the TDMF Agent.  
//                      Upon success of transmission to Agent, the key 
//                      is also saved to the TDMF database.
// 
// ***********************************************************
LIBMNGT_API 
int mmp_setTdmfAgentRegistrationKey(/*in*/ MMP_HANDLE handle, 
                                    /*in*/ const char * szAgentId,
                                    /*in*/ bool  bIsHostId,
                                    /*in*/ const char * szRegKey,
                                    /*out*/int *pExpirationTime = 0 );



// ***********************************************************
// Function name	: mmp_mngt_getTdmfAgentConfig
// Description	    : Retreive the general configuration of the specified TDMF Agent.
// Return type		: int 
// Return type		: int 0 = ok, < 0 = error
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*out*/ mmp_TdmfAgentConfig * agentCfg : upon success, this mmp_TdmfAgentConfig
//                              structure is filled with config. data.
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_getTdmfAgentConfig(/*in*/ MMP_HANDLE handle, 
                                /*in*/ const char * szAgentId,
                                /*in*/ bool  bIsHostId,
                                /*out*/mmp_TdmfServerInfo * serverCfg
                                );

// ***********************************************************
// Function name	: mmp_mngt_setTdmfAgentConfig
// Description	    : Force the general configuration of the specified TDMF Agent.
// Return type		: int 0 = ok, < 0 = error
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
//                    -104 = unexpected communication rupture while rx/tx data with TDMF Collector
//                    -105 = Agent could not save the received Agent config.  Continuing using current config.
//                    -106 = provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.
//                    -107 = basic functionality failed ...
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ const mmp_TdmfAgentConfig * agentCfg : addr. of a mmp_TdmfAgentConfig
//                              structure filled with the config. data to be transmitted to the TDMF Agent.
//                          
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_setTdmfAgentConfig(/*in*/ MMP_HANDLE handle, 
                                /*in*/ const char * szAgentId,
                                /*in*/ bool  bIsHostId,
                                /*in*/ const mmp_TdmfServerInfo * serverCfg
                                );

// ***********************************************************
// Function name	: mmp_getTdmfAgentLGConfig
// Description	    : The application calls this function to request  
//                    a TDMF Agent for the current configuration of the specified logical group.  
//                    Upon success, the output buffer is filled with the content of the 
//                    requested logical group .cfg file.
//                    It is a copy of the .cfg file, as it appears on the TDMF Agent system.
//       
// Return type		: int 0 = ok, < 0 = error.
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -4  = error while receiving data from TDMF Collector
//                    -5  = internal communication error.
//                    -6  = the specified logical group is not configured on TDMF Agent (no .cfg file exist)
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
//                    -104 = unexpected communication rupture while rx/tx data with TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ short sLogicalGroupId : number identifying the logical group
// Argument         : /*in*/ bool  bPrimary : true = request for the PRIMARY system LG config,
//                                            false = request for the SECONDARY system LG config.
// Argument         : /*out*/ char **ppCfgData : addr. of a char* to be assigned to a buffer containing the entire cfg file data.
//                                                  When done with buffer, release memory reserved for it using delete [] *ppCfgData .
// Argument         : /*out*/ unsigned int *puiDataSize : addr. of a var. to be assigned with the size of data available in *ppCfgData.
//
// 
// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfAgentLGConfig(/*in*/ MMP_HANDLE handle, 
                             /*in*/ const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*in*/ short sLogicalGroupId,
                             /*in*/ bool  bPrimary,
                             /*out*/char **ppCfgData,
                             /*out*/unsigned int *puiDataSize
                             );


// ***********************************************************
// Function name	: mmp_setTdmfAgentLGConfig
// Description	    : The application calls this function to provide
//                    the logical group configuration to a TDMF Agent.
//                    The cfg information provided must be the entire 
//                    content of the .cfg file, as contiguous buffer 
//                    of binary data.
// Return type		: int 
//                     0  = success.
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -4  = error while receiving data from TDMF Collector
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
//                    -104 = unexpected communication rupture while rx/tx data with TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ short sLogicalGroupId : number identifying the logical group
// Argument         : /*in*/ bool  bPrimary : true = PRIMARY system LG config ( "pLGId.cfg" ),
//                                            false = SECONDARY system LG config ( "sLGId.cfg" ).
// Argument         : /*in*/ const char * pCfgData : raw binary content of .cfg file.
//                                                   if NULL, it indicates to delete the existing logical group .cfg file from the TDMF Agent.
// Argument         : /*in*/ unsigned int uiDataSize : size of .cfg file data provided.  Ignored if pCfgData is NULL.
// 
// ***********************************************************
LIBMNGT_API 
int mmp_setTdmfAgentLGConfig(/*in*/ MMP_HANDLE handle, 
                             /*in*/ const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*in*/ short sLogicalGroupId,
                             /*in*/ bool  bPrimary,
                             /*in*/ const char * pCfgData,
                             /*in*/ unsigned int uiDataSize
                             );

// ***********************************************************
// Function name	: mmp_setTdmfServerFile
// Description	    : The application calls this function to send
//                    a TDMF file (.bat, .exe, .zip) to a TDMF Agent.
//                    The file information provided must be the entire 
//                    content of the TDMF file, as contiguous buffer 
//                    of binary data.
// Return type		: int 
//                     0  = success.
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -4  = error while receiving data from TDMF Collector
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
//                    -104 = unexpected communication rupture while rx/tx data with TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ const char * pFileName : TDMF filename (with or without pathname).
// Argument         : /*in*/ const char * pFileData : raw binary content of TDMF file.
//                                                   if NULL, it indicates to delete the existing TDMF file from the TDMF Agent.
// Argument         : /*in*/ unsigned int uiDataSize : size of TDMF file data provided.  Ignored if pFileData is NULL.
// Argument         : /*in*/ int iFileType : File Type (BAT/EXE/ZIP/TAR).
//                                           use MMP_MNGT_FILETYPE_TDMF_EXE, _BAT, _ZIP or _TAR enums
// 
// ***********************************************************
LIBMNGT_API 
int mmp_setTdmfServerFile   (/*in*/ MMP_HANDLE handle, 
                             /*in*/ const char * szAgentId,
                             /*in*/ bool  bIsHostId,
                             /*in*/ const char * pFileName,
                             /*in*/ const char * pFileData,
                             /*in*/ unsigned int uiDataSize,
                             /*in*/ int   iFileType
                            );

// ***********************************************************
// Function name	: mmp_getTdmfServerFile
// Description	    : The application calls this function to request  
//                    a TDMF Agent for the existing TDMF files (scripts).  
//                    Specific usage for batch files
//                    Upon success, the output buffer is filled with the content of the 
//                    TDMF file (BAT/ZIP/TAR/EXE).
//                    It is a copy of the TDMF file, as it appears on the TDMF Agent system.
//       
// Return type		: int 0 = ok, < 0 = error.
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -4  = error while receiving data from TDMF Collector
//                    -5  = internal communication error.
//                    -6  = the specified logical group is not configured on TDMF Agent (no file exist)
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
//                    -104 = unexpected communication rupture while rx/tx data with TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ const char * pFileName : TDMF filename.
// Argument         : /*in*/ int iFileType : File Type (BAT/EXE/ZIP/TAR).
//                                           use MMP_MNGT_FILETYPE_TDMF_EXE, _BAT, _ZIP or _TAR enums
// Argument         : /*out*/ char **ppFileMsgData : addr. of a char* to be assigned to a buffer containing the entire cfg file data.
//                                                  When done with buffer, release memory reserved for it using delete [] *ppFileMsgData .
// Argument         : /*out*/ unsigned int *puiDataSize : addr. of a var. to be assigned with the size of data available in *ppFileMsgData.
//
// 
// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfServerFile(/*in*/ MMP_HANDLE handle, 
                          /*in*/ const char * szAgentId,
                          /*in*/ bool  bIsHostId,
                          /*in*/ const char * pFileName,
                          /*in*/ int iFileType,
                          /*out*/ char ** ppFileMsgData,
                          /*out*/ unsigned int *puiDataSize
                         );

// ***********************************************************
// Function name	: mmp_mngt_sendTdmfCommand
// Description	    : sends a Tdmf command to the specified TDMF Agent
// Return type		: int 0 = ok, command sucessful.
//                    > 0 = error, cmd status value enum tdmf_commands_status
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
//                    -4  = error while receiving data from TDMF Collector
//                    -10 = invalid argument
//                    -100 = unable to connect to TDMF Collector
//                    -101 = unknown TDMF Agent (szAgentId)
//                    -102 = unable to connect to TDMF Agent
//                    -103 = unexpected communication rupture while rx/tx data with TDMF Agent
//                    -104 = unexpected communication rupture while rx/tx data with TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*in*/ enum tdmf_commands cmd : identifier for the tdmf___.exe command to be executed on the TDMF Agent.
// Argument         : /*in*/ const char * szCmdOptions : identical to cmd line options of tdmf tools
// Argument         : /*out*/char ** ppszCmdOutput : addr. of a ptr to be assigned to a string containing the output message of the tdmf tool
//                                                   when done with this buffer, memory should be released using delete [] *ppszCmdOutput;
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_sendTdmfCommand( /*in*/ MMP_HANDLE handle, 
                              /*in*/ const char * szAgentId,
                              /*in*/ bool  bIsHostId,
                              /*in*/ enum tdmf_commands cmd,
                              /*in*/ const char * szCmdOptions,
                              /*out*/char ** ppszCmdOutput );



// ***********************************************************
// Function name	: mmp_getTdmfAgentAllDevices
// Description	    : Retreive a list of devices (local disks) available on the TDMF Agent.
//                    Each available device is described with a mmp_TdmfDeviceInfo structure.
// Return type		: int 0 = ok, command sucessful.
//                     -10  = invalid argument
//                    -100  = unable to connect to TDMF Collector
//                    -104  = unexpected communication rupture while rx/tx data with TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const char * szAgentId : TDMF Agent Host ID (32-bit hexadecimal number)
// Argument         : /*in*/ bool bIsHostId : set to true when szAgentId is the text representation of the Agent's Host ID number;
//                                            set to false when szAgentId is the agent's Server Name.
// Argument         : /*out*/mmp_TdmfDeviceInfo ** ppTdmfDeviceInfoVector : addr. of a mmp_TdmfDeviceInfo ptr
//                                                                          to be assigned on a contiguous vector of mmp_TdmfDeviceInfo.
//                                                                          When done with vector, release memory using delete [] *ppTdmfDeviceInfoVector.
// Argument         : /*out*/unsigned int *puiNbrDeviceInfoInVector : addr. to be filled with the number of mmp_TdmfDeviceInfo struct. available in the vector
//                                                                      pointed to by *ppTdmfDeviceInfoVector .
// 
// ***********************************************************
LIBMNGT_API 
int mmp_getTdmfAgentAllDevices(/*in*/ MMP_HANDLE handle, 
                               /*in*/ const char * szAgentId,
                               /*in*/ bool  bIsHostId,
                               /*out*/mmp_TdmfDeviceInfo ** ppTdmfDeviceInfoVector,
                               /*out*/unsigned int *puiNbrDeviceInfoInVector );

// ***********************************************************
// Function name	: mmp_setTdmfPerfConfig
// Description	    : set the refresh performance configuration of 
//                    all TDMF Agents currently alive (on-line) with TDMF Collector.
// Return type		: int 
//                     0  = success 
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ const mmp_TdmfPerfConfig * perfCfg
// 
// ***********************************************************
LIBMNGT_API 
int mmp_setTdmfPerfConfig(/*in*/ MMP_HANDLE handle, 
                          /*in*/ const mmp_TdmfPerfConfig * perfCfg
                          );




// ***********************************************************
// Function name	: mmp_registerNotificationMessages
// Description	    : establish a socket connection with Collector
//                    through which Notification Messages will be sent 
//                    and received.
//                    The created socket could be provided to the TDMFNotificationMessage object.
//                    When done with socket, it should be 
// Return type		: int 
//                     0  = success 
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in-out*/sock_t *pNotifSocket Address of a created but uninitialized pointer to a sock_t structure.
// 
// ***********************************************************
#ifndef _SOCK_H
struct sock_t;
#endif
LIBMNGT_API 
int mmp_registerNotificationMessages(/*in*/ MMP_HANDLE handle, 
                                     /*in-out*/sock_t *pNotifSocket,
                                     /*in*/ int iPeriod /*seconds*/
                                     );


// ***********************************************************
// Function name	: mmp_requestTDMFOwnership
// Description	    : Sends a msg to TDMF Collector requesting the exclusive ownership of the administration of TDMF system.
//					  On first call, it requests the exclusive right among tdmf apps.
//					  to access the TDMF DB. (only one tdmf gui app. should be accessing the TDMF DB)
//					  When granted, this function must be periodically called, as a watchdog,
//					  to retain the exclusive right on the TDMF system.  The maximum period between calls 
//					  to retain the exclusive right should be less than 60 seconds.
//
//					  Upon success, the caller shall access the TDMF DB.  Otherwise,
//					  it indicates another caller is currently owning the TDMF system.  Then,
//					  the caller should NOT attempt any operation on the TDMF DB and TDMF Collector 
//					  other than read-only accesses.  Not doing so can lead to corruption the TDMF data.
//					  
// Return type		: int 
//                     0  = success 
//                    -2  = unable to connect to TDMF Collector
//                    -3  = error while sending data to TDMF Collector
// Argument         : /*in*/ MMP_HANDLE handle
// Argument         : /*in*/ char *szClientUID : string providing a unique ID of the caller.  
//							 This ID is combined with the caller's IP address to provide unicity accross hosts.
// Argument         : /*in*/ bool bRequestOwnership : true to request/renew ownership, false to release ownership.
// Argument         : /*out*/ bool * bOwnershipGranted : contains true if application
// 
// ***********************************************************
LIBMNGT_API 
int mmp_TDMFOwnership(  /*in*/ MMP_HANDLE handle, 
					    /*in*/ const char *szClientUID,
                        /*in*/ bool bRequestOwnership,
						/*out*/bool * bOwnershipGranted
                        );

// ***********************************************************
// Function name	: mmp_getSystemParameters
// Description	    : retreive actual various parameters used by Collector
// Return type		: 0 = ok , < 0 = error
// Argument         : mmp_TdmfCollectorParams* params
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_getSystemParameters(/*in*/ MMP_HANDLE handle, 
                                 /*out*/ mmp_TdmfCollectorParams* params );

// ***********************************************************
// Function name	: mmp_setSystemParameters
// Description	    : sends various parameters to be used by Collector
// Return type		: 0 = ok , < 0 = error
// Argument         : const mmp_TdmfCollectorParams* params
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_setSystemParameters(/*in*/ MMP_HANDLE handle, 
                                 /*in*/ const mmp_TdmfCollectorParams* params );

// ***********************************************************
// Function name	: mmp_mngt_sendGuiMsg
// Description	    : sends various messages to the other gui via the Collector
// Return type		: 0 = ok , < 0 = error
// Argument         : const char * szMsg
// 
// ***********************************************************
LIBMNGT_API 
int mmp_mngt_sendGuiMsg( /*in*/ MMP_HANDLE handle, 
						 /*in*/ const BYTE* Msg,
						 /*in*/ const UINT  nLength);

//avoid C++ name mangling problems when imported/used by C client program
#ifdef __cplusplus 
}
#endif

#endif //__LIBMNGT_H_