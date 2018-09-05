/* #ident "@(#)$Id: ftd_mngt_get_config.c,v 1.18 2003/11/13 02:48:21 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#ifndef _FTD_MNGT_GET_CONFIG_C_
#define _FTD_MNGT_GET_CONFIG_C_


#if defined(HPUX)
#if SYSVERS != 1020
#include <sys/ki_defs.h>
#else
typedef unsigned char kboolean_t;
#endif
#elif defined(linux)
typedef unsigned char kboolean_t;
#endif

#define HOST_ID_PREFIX          "HostID="
extern  char    gszServerUID[];

void ftd_mngt_get_all_cfg_files(char **pAllCfgFileData,
					 unsigned int *puiAllCfgFileSize);
void ftd_mngt_get_all_tdmf_files(char **pAllTdmfFileData,
					 unsigned int *puiAllTdmfFileSize, int iFileType);

/***********************************************************************/
/* might be added to ftd_mngt.c                                        */
/***********************************************************************/

/*
 * Builds a mmp_mngt_ConfigurationMsg_t message ready to be sent on socket
 * (host to network convertions done)
 * pFileData can be NULL; uiDataSize can be 0.
 */ 
void
mmp_mngt_build_SetConfigurationMsg(mmp_mngt_ConfigurationMsg_t **ppCfgMsg, 
#if defined(HPUX) || defined(linux)
                                   const char *szAgentId, kboolean_t bIsHostId, int iSenderType,  
#else
                                   const char *szAgentId, boolean_t bIsHostId, int iSenderType,  
#endif
                                   const char *szCfgFileName, 
                                   const char *pFileData, unsigned int uiDataSize)
{
    int totallen;
    mmp_mngt_ConfigurationMsg_t  *msg;
    int iMsgSize;
    char *pWk;

    *ppCfgMsg = 0;
    totallen = uiDataSize;

    /* build contiguous buffer for message */
    iMsgSize = sizeof(mmp_mngt_ConfigurationMsg_t) + totallen;
    msg = (mmp_mngt_ConfigurationMsg_t *)malloc(iMsgSize * sizeof(char));

    /* build msg -- header */
    msg->hdr.sendertype  = iSenderType;
    msg->hdr.mngttype    = MMP_MNGT_SET_LG_CONFIG;
    msg->hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
    msg->hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&msg->hdr);

    msg->szServerUID[0] = 0;
    if (bIsHostId) {
        strcat(msg->szServerUID, HOST_ID_PREFIX);
    }
    strcat(msg->szServerUID, szAgentId);

    strcpy(msg->data.szFilename, szCfgFileName);
    msg->data.iType  = htonl(MMP_MNGT_FILETYPE_TDMF_CFG);
    msg->data.uiSize = htonl(totallen);

    if (totallen > 0) {
        pWk = (char *)(msg + 1);
        memcpy(pWk, pFileData, totallen );
    }

    *ppCfgMsg = msg;
}

/*
 * release memory allocated by mmp_mngt_build_SetConfigurationMsg()
 */
void
mmp_mngt_release_SetConfigurationMsg(mmp_mngt_ConfigurationMsg_t **ppCfgMsg)
{
    if (*ppCfgMsg != 0) {
        free((char*)*ppCfgMsg);
    }
}

/*
 * Caller must free allocated memory with delete [] *ppData.
 */
int
ftd_mngt_read_file_from_disk(const char *pFname, char **ppData, 
	unsigned int *puiFileSize)
{
    int r;
    FILE *file;
    long filesize;

    *puiFileSize = 0;
    *ppData      = 0;

    file = fopen(pFname,"rb");
    if (file) {
        r = fseek(file, 0, SEEK_END);
        filesize = ftell(file);

        *ppData = (char *)malloc(filesize);
        *puiFileSize = (unsigned int)filesize;

        r = fseek(file, 0, SEEK_SET);
        if (fread(*ppData, sizeof(char), filesize,file) == (size_t)filesize) {
            r = 0;
        } else {
            r = -1;
        }
        fclose(file);
    } else {
        r = -2;
    }
    return r;
}

/*
 * Caller must free allocated memory with delete [] *ppData.
 */
int
ftd_mngt_read_file_from_diskx(const char *pFname, char **ppData, 
	unsigned int *puiFileSize)
{
    int r;
    FILE *file;
    long filesize;
    long filesizex;

    *puiFileSize = 0;
    *ppData      = 0;

    file = fopen(pFname,"rb");
    if (file) {
        r = fseek(file, 0, SEEK_END);
        filesize = ftell(file);
		if ((filesize % 4) != 0)
		{
			filesizex = filesize/4;
			filesizex++;
			filesizex = filesizex*4;
		}
		else 
		{
			filesizex = filesize;
		}

        *ppData = (char *)malloc(filesizex);
        *puiFileSize = (unsigned int)filesizex;
		memset(*ppData,0x0a,filesizex);

        r = fseek(file, 0, SEEK_SET);
        if (fread(*ppData, sizeof(char), filesize,file) == (size_t)filesize) {
            r = 0;
        } else {
            r = -1;
        }
        fclose(file);
    } else {
        r = -2;
    }
    return r;
}

int 
ftd_mngt_get_config(sock_t *sockID)
{
    int		    r = 0;
    int		    towrite;
    char            c;
    char            *pFileData;
    char            szFullPathCfgFname[256];
    unsigned int    uiFileSize;
    mmp_mngt_ConfigurationMsg_t     *pRcvCfgMsg;
    mmp_mngt_ConfigurationMsg_t     *pResponseCfgMsg;

    /* complete reception of logical group cfg data message */
    mmp_mngt_recv_cfg_data(sockID, NULL, &pRcvCfgMsg);

    /* in a GetCONFIG request, only pRcvCfgData->data.szFileName */
    if (pRcvCfgMsg == NULL) {
	/* problem receiving the message data */
	sprintf(logmsg, "while receiving Logical group configuration from 0x%x %s\n",
		sockID->rip, sockID->rhostname);
	logout(4,F_ftd_mngt_get_config,logmsg);
        return -1;
    }

    /* convertion from network bytes order to host byte order done 
						in mmp_mngt_recv_cfg_data() */
    sprintf(logmsg, "Get configuration msg received. Cfg file name = %s, size = %d bytes\n", pRcvCfgMsg->data.szFilename, pRcvCfgMsg->data.uiSize);
    logout(9, F_ftd_mngt_get_config, logmsg);

    strcpy(szFullPathCfgFname,DTCCFGDIR);

    c = szFullPathCfgFname[strlen(szFullPathCfgFname) - 1];
    if (c != '/' && c != '\\') {   
        strcat(szFullPathCfgFname, "/");
    }
    strcat(szFullPathCfgFname, pRcvCfgMsg->data.szFilename);


    pFileData   = 0;
    uiFileSize  = 0;
    if ( strcmp(pRcvCfgMsg->data.szFilename,"*.cfg") == 0 )
    {  /* request to recv ALL .cfg files content. */
	/* create a vector of ( mmp_TdmfFileTransferData + file data ) */
	ftd_mngt_get_all_cfg_files(&pFileData, &uiFileSize);
	/* prepare response message */
	/* make believe to mmp_mngt_build_SetConfigurationMsg() */
	/*  that pFileData contains one big file data,          */
	/* instead of being a vector of                         */
	/*             ( mmp_TdmfFileTransferData + file data ) */
	mmp_mngt_build_SetConfigurationMsg( &pResponseCfgMsg,
                                       	pRcvCfgMsg->szServerUID,
				       	1, /* it is a host id */
					SENDERTYPE_TDMF_AGENT,
					pRcvCfgMsg->data.szFilename,
					(char*)pFileData,
					uiFileSize );
    }
    else 
    {
	/* request to recv ONE .cfg files content.              */
	/* content of file is transfered to pFileData           */
	/*             (memory allocated within fnct)           */
	r = ftd_mngt_read_file_from_disk(szFullPathCfgFname,
					&pFileData,
					&uiFileSize);
	if ( r != 0 )
	{ /* err reading cfg file. maybe wrong lgid provided. */
	    sprintf(logmsg, "reading, Cfg file %s\n", szFullPathCfgFname);
	    logout(4,F_ftd_mngt_get_config,logmsg);
    	}

    	/* prepare response message */
    	mmp_mngt_build_SetConfigurationMsg(&pResponseCfgMsg, 
                                       	pRcvCfgMsg->szServerUID,
				       	1, /* it is a host id */
                                       	SENDERTYPE_TDMF_AGENT,  
                                       	pRcvCfgMsg->data.szFilename, 
                                       	pFileData,
				       	uiFileSize);
    }
    /* send status message to requester */
    towrite = sizeof(mmp_mngt_ConfigurationMsg_t) +
					 ntohl(pResponseCfgMsg->data.uiSize);
    r = ftd_sock_send(sockID, (char *)pResponseCfgMsg, towrite);

    /* release memory obtained from mmp_mngt_build_SetConfigurationMsg() */
    mmp_mngt_release_SetConfigurationMsg(&pResponseCfgMsg);
    /* release memory obtained from mmp_mngt_recv_cfg_data() */
    mmp_mngt_free_cfg_data_mem(&pRcvCfgMsg);

    if (pFileData) {
        free(pFileData);
    }

    return r;
}

void
ftd_mngt_get_all_cfg_files(char **pAllCfgFileData,
			unsigned int *puiAllCfgFileSize)
{
    char *pFileData   = 0;
    char *pCumulDataBuf = 0;
    unsigned int uiFileSize  = 0;
    unsigned int uiCumulDataBufSize = 0;
    int r;
    int iNewDataSize;
    DIR *dfd;
    struct dirent *dent;
    mmp_TdmfFileTransferData *p;
    char full_filename[MMP_MNGT_MAX_FILENAME_SZ+1];

    *pAllCfgFileData = 0;
    *puiAllCfgFileSize = 0;

    if (((DIR*)NULL) != (dfd = opendir (DTCCFGDIR)))
    {
    	while (NULL != (dent = readdir(dfd))) {
		if (strcmp(dent->d_name, ".") == 0) continue;
		if (strcmp(dent->d_name, "..") == 0) continue;
		if (chk_pxxxcfg(dent->d_name) != 0) continue;
		/* file neme check OK */
		sprintf(full_filename,"%s/%s",DTCCFGDIR,dent->d_name);
		r = ftd_mngt_read_file_from_diskx(full_filename,
						     &pFileData, &uiFileSize);
		if ( r == 0 )
		{
			iNewDataSize = sizeof(mmp_TdmfFileTransferData) + 
								    uiFileSize;
			/* resize output buffer */
			pCumulDataBuf = (char *)realloc(pCumulDataBuf,	
					uiCumulDataBufSize + iNewDataSize);
			/* init mmp_TdmfFileTransferData portion of   	*/
			/*				 new file data 	*/
			p = (mmp_TdmfFileTransferData*)(pCumulDataBuf + uiCumulDataBufSize);
			p->iType  = MMP_MNGT_FILETYPE_TDMF_CFG;
			strcpy(p->szFilename, dent->d_name);
			p->uiSize = uiFileSize;
			mmp_convert_FileTransferData_hton(p);
#if 0
			mmp_convert_hton(p);
#endif
			/* append new file text data to cumulative buffer */
			memmove( p + 1, pFileData, uiFileSize);
			uiCumulDataBufSize += iNewDataSize;
		}		
    	}
    	(void) closedir (dfd);
    }

    *pAllCfgFileData    = pCumulDataBuf;
    *puiAllCfgFileSize  = uiCumulDataBufSize;
}

int
chk_pxxxcfg(char filename[])
{
	int i = 0;
	struct stat statbuf;
	char full_filename[MMP_MNGT_MAX_FILENAME_SZ+1];

	if (strlen(filename) != 8) return(-1);
	if (filename[i] != 'p') return(-1);
	i++;
	for (;i <= 3;i++)
	{
		if (isdigit(filename[i]) == 0) return(-1);
	}
	if (strcmp(&(filename[i]),".cfg") != 0) return(-1);
	
	/* filename check OK */
  	/* file status check */
	sprintf(full_filename,"%s/%s",DTCCFGDIR,filename);
	if (0 != stat (full_filename, &statbuf)) return(-1);
	if (!S_ISREG(statbuf.st_mode)) return(-1);

	/* file name and status check OK */
	return(0);

}

/*
 * Builds a mmp_mngt_FileMsg_t message ready to be sent on socket
 * (host to network convertions done)
 * pFileData can be NULL; uiDataSize can be 0.
 */ 
void
mmp_mngt_build_SetTdmfFileMsg(mmp_mngt_FileMsg_t **ppFileMsg,
#if defined(HPUX) || defined(linux)
						const char *szAgentId, kboolean_t bIsHostId, int iSenderType, 
#else
						const char *szAgentId, boolean_t bIsHostId, int iSenderType, 
#endif
						const char *szFileName, 
						const char *pFileData, unsigned int uiDataSize,
						int iFileType )
{
	/* build contiguous buffer for message */
	mmp_mngt_FileMsg_t  *msg;
	int iMsgSize;
	char *pWk;
	int totallen;

	*ppFileMsg = 0;
	totallen = uiDataSize;
	iMsgSize = sizeof(mmp_mngt_FileMsg_t) + totallen;
	msg = (mmp_mngt_FileMsg_t *)malloc(iMsgSize * sizeof(char));

	/* build msg -- header */
	msg->hdr.sendertype  = iSenderType;
	msg->hdr.mngttype    = MMP_MNGT_TDMF_SENDFILE;
	msg->hdr.magicnumber = MNGT_MSG_MAGICNUMBER;
	msg->hdr.mngtstatus  = MMP_MNGT_STATUS_OK;
	mmp_convert_mngt_hdr_hton(&msg->hdr);

	msg->szServerUID[0] = 0;
	if (bIsHostId) {
		strcat(msg->szServerUID, HOST_ID_PREFIX);
	}
	strcat(msg->szServerUID, szAgentId);

	msg->data.iType  = htonl(iFileType);
	strcpy(msg->data.szFilename, szFileName);
	msg->data.uiSize = htonl(totallen);

	if (totallen > 0) {
		pWk = (char *)(msg + 1);
		memcpy(pWk, pFileData, totallen );
	}

	*ppFileMsg = msg;
}

/*
 * release memory allocated by mmp_mngt_build_SetTdmfFileMsg()
 */
void
mmp_mngt_release_SetTdmfFileMsg(mmp_mngt_FileMsg_t **ppFileMsg)
{
    if (*ppFileMsg != 0) {
        free((char*)*ppFileMsg);
    }
}

int 
ftd_mngt_get_file(sock_t *sockID)
{
	int		        r = 0;
	int		        towrite;
	char            c;
	char            *pFileData;
	char            szFullPathFname[256];
	unsigned int    uiFileSize;
	mmp_mngt_FileMsg_t     *pRcvFileMsg;
	mmp_mngt_FileMsg_t     *pResponseFileMsg;

	/* complete reception of TDMF file message */
	mmp_mngt_recv_file_data(sockID, NULL, &pRcvFileMsg);

	/* in a Get_TDMFFILE request, only pRcvFileData->data.szFileName */
	if (pRcvFileMsg == NULL) {
		/* problem receiving the message data */
		sprintf(logmsg, "while receiving Replicator file request from 0x%x %s\n", sockID->rip, sockID->rhostname);
		logout(4,F_ftd_mngt_get_file,logmsg);
		return -1;
	}

	/* iType check */
	switch(pRcvFileMsg->data.iType) {
	case MMP_MNGT_FILETYPE_TDMF_CFG:
		break;
	case MMP_MNGT_FILETYPE_TDMF_EXE:
		break;
	case MMP_MNGT_FILETYPE_TDMF_BAT:
		break;
	case MMP_MNGT_FILETYPE_TDMF_TAR:
		break;
	case MMP_MNGT_FILETYPE_TDMF_ZIP:
		break;
	default:
		logoutx(4, F_ftd_mngt_get_file, "iType is not supported", "iType", pRcvFileMsg->data.iType);
		return -1;
	}

	/* convertion from network bytes order to host byte order done in mmp_mngt_recv_file_data() */
	sprintf(logmsg, "Get Replicator FILE msg received. Replicator file requested = %s, size = %d bytes\n", pRcvFileMsg->data.szFilename, pRcvFileMsg->data.uiSize);
	logout(9, F_ftd_mngt_get_file, logmsg);

	strcpy(szFullPathFname,TDMFFILEDIR);

	c = szFullPathFname[strlen(szFullPathFname) - 1];
	if (c != '/' && c != '\\') {   
		strcat(szFullPathFname, "/");
	}
	strcat(szFullPathFname, pRcvFileMsg->data.szFilename);


	pFileData   = 0;
	uiFileSize  = 0;
	if ( strncmp(pRcvFileMsg->data.szFilename,"*.",2) == 0 ) {
		/* request to recv ALL TDMF files content. */
		/* create a vector of ( mmp_TdmfFileTransferData + file data ) */
		ftd_mngt_get_all_tdmf_files(&pFileData, &uiFileSize, pRcvFileMsg->data.iType);
		/* prepare response message */
		/* make believe to mmp_mngt_build_SetTdmfFileMsg() */
		/*          that pFileData contains one big file data,  */
		/* instead of being a vector of                         */
		/*             ( mmp_TdmfFileTransferData + file data ) */
		mmp_mngt_build_SetTdmfFileMsg( &pResponseFileMsg,
										pRcvFileMsg->szServerUID,
										1, /* it is a host id */
										SENDERTYPE_TDMF_AGENT,
										pRcvFileMsg->data.szFilename,
										(char*)pFileData, uiFileSize,
										pRcvFileMsg->data.iType );
	} else {
		/* request to recv ONE TDMF file content.               */
		/* content of file is transfered to pFileData           */
		/*                      (memory allocated within fnct)  */
		r = ftd_mngt_read_file_from_disk(szFullPathFname,
														&pFileData,
														&uiFileSize);
		if ( r != 0 ) {
			/* err reading TDMF file. maybe wrong lgid provided. */
			sprintf(logmsg, "reading, Replicator file %s\n", szFullPathFname);
			logout(4,F_ftd_mngt_get_file,logmsg);
    	}

		/* prepare response message */
		mmp_mngt_build_SetTdmfFileMsg(&pResponseFileMsg, 
										pRcvFileMsg->szServerUID,
										1, /* it is a host id */
										SENDERTYPE_TDMF_AGENT,  
										pRcvFileMsg->data.szFilename, 
										pFileData, uiFileSize,
										pRcvFileMsg->data.iType );
	}

	/* send status message to requester */
	towrite = sizeof(mmp_mngt_FileMsg_t) + ntohl(pResponseFileMsg->data.uiSize);
	r = ftd_sock_send(sockID, (char *)pResponseFileMsg, towrite);

	/* release memory obtained from mmp_mngt_build_SetTdmfFileMsg() */
	mmp_mngt_release_SetTdmfFileMsg(&pResponseFileMsg);
	/* release memory obtained from mmp_mngt_recv_file_data() */
	mmp_mngt_free_file_data_mem(&pRcvFileMsg);

	if (pFileData) {
		free(pFileData);
	}

	return r;
}

void
ftd_mngt_get_all_tdmf_files(char **pAllTdmfFileData, unsigned int *puiAllTdmfFileSize, int iFileType)
{
	char *pFileData   = 0;
	char *pCumulDataBuf = 0;
	unsigned int uiFileSize  = 0;
	unsigned int uiCumulDataBufSize = 0;
	int r;
	int iNewDataSize;
	DIR *dfd;
	struct dirent *dent;
	mmp_TdmfFileTransferData *p;
	char full_filename[MMP_MNGT_MAX_FILENAME_SZ+1];

	*pAllTdmfFileData = 0;
	*puiAllTdmfFileSize = 0;

	if (((DIR*)NULL) != (dfd = opendir (TDMFFILEDIR))) {
		while (NULL != (dent = readdir(dfd))) {
			if (strcmp(dent->d_name, ".") == 0) continue;
			if (strcmp(dent->d_name, "..") == 0) continue;
			if (chk_tdmffile(dent->d_name, iFileType) != 0) continue;
			/* file neme check OK */
			sprintf(full_filename,"%s/%s",TDMFFILEDIR,dent->d_name);
			r = ftd_mngt_read_file_from_diskx(full_filename, &pFileData, &uiFileSize);
			if ( r == 0 ) {
				iNewDataSize = sizeof(mmp_TdmfFileTransferData) + uiFileSize;
				/* resize output buffer */
				pCumulDataBuf = (char *)realloc(pCumulDataBuf, uiCumulDataBufSize + iNewDataSize);
				/* init mmp_TdmfFileTransferData portion of new file data */
				p = (mmp_TdmfFileTransferData*)(pCumulDataBuf + uiCumulDataBufSize);
				p->iType  = iFileType;
				strcpy(p->szFilename, dent->d_name);
				p->uiSize = uiFileSize;
				mmp_convert_FileTransferData_hton(p);
#if 0
				mmp_convert_hton(p);
#endif
				/* append new file text data to cumulative buffer */
				memmove( p + 1, pFileData, uiFileSize);

				uiCumulDataBufSize += iNewDataSize;
			}		
		}
    	(void) closedir (dfd);
	}

	*pAllTdmfFileData    = pCumulDataBuf;
	*puiAllTdmfFileSize  = uiCumulDataBufSize;
}

int
chk_tdmffile(char filename[], int iFileType)
{
	char *p0;
	char *p1;
	struct stat statbuf;
	char full_filename[MMP_MNGT_MAX_FILENAME_SZ+1];
	int filelen;
	char *comparetype;
	int typelen;

	/* iType check */
	switch(iFileType) {
	case MMP_MNGT_FILETYPE_TDMF_CFG:
		comparetype = ".cfg";
		break;
	case MMP_MNGT_FILETYPE_TDMF_EXE:
		comparetype = ".exe";
		break;
	case MMP_MNGT_FILETYPE_TDMF_BAT:
		comparetype = ".sh";
		break;
	case MMP_MNGT_FILETYPE_TDMF_TAR:
		comparetype = ".tar";
		break;
	case MMP_MNGT_FILETYPE_TDMF_ZIP:
		comparetype = ".zip";
		break;
	default:
		return(-1);
	}

	filelen = strlen(filename);
	typelen = strlen(comparetype);
	
	if (filelen <= typelen) {
		return(-1);
	}

	if (strcmp(&filename[filelen-typelen], comparetype) != 0) {
		return(-1);
	}
		
	/* filename check OK */
	/* file status check */
	sprintf(full_filename,"%s/%s", TDMFFILEDIR, filename);
	if (0 != stat (full_filename, &statbuf)) return(-1);
	if (!S_ISREG(statbuf.st_mode)) return(-1);

	/* file name and status check OK */
	return(0);

}
#endif /* _FTD_MNGT_GET_CONFIG_C_ */
