/* #ident "@(#)$Id: ftd_mngt_set_config.c,v 1.16 2003/11/13 02:48:22 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_SET_CONFIG_C_
#define _FTD_MNGT_SET_CONFIG_C_

void
mmp_mngt_recv_cfg_data(sock_t *s, mmp_mngt_header_t *msghdr, 
	mmp_mngt_ConfigurationMsg_t **ppRcvCfgData)
{
	int r;
	int toread;
	int iMsgSize;
	mmp_mngt_ConfigurationMsg_t agentCfg;

	*ppRcvCfgData = 0;

	memset(&agentCfg, 0x00, sizeof(mmp_mngt_ConfigurationMsg_t));
	/* msg header as been read by caller and hdr is availble from msghdr. */
	if (msghdr != NULL) {
		agentCfg.hdr = *msghdr;
	}

	/* reads other mmp_mngt_ConfigurationMsg_t fields */
	toread = sizeof(agentCfg.szServerUID);
	r = ftd_sock_recv(s, (char *)agentCfg.szServerUID, toread);
	if (r == toread) {
		/* now ready to read remainder of mmp_mngt_ConfigurationMsg_t msg */
		toread = sizeof(agentCfg.data);
		r = ftd_sock_recv(s, (char *)&agentCfg.data, toread);
		if (r == toread) {
			char *pData, *pWk;

			agentCfg.data.iType  = ntohl(agentCfg.data.iType);
			agentCfg.data.uiSize = ntohl(agentCfg.data.uiSize);

			/* read remainder of message in a CONTIGUOUS buffer */
			iMsgSize = sizeof(mmp_mngt_ConfigurationMsg_t) + agentCfg.data.uiSize ;

			sprintf(logmsg, "uiSize = %d, szFilename = %s\n", agentCfg.data.uiSize, agentCfg.data.szFilename);
			logout(9, F_mmp_mngt_recv_cfg_data, logmsg);

			pWk = pData = (char *)malloc(iMsgSize * sizeof(char));

			/* copy data already received to contiguous buffer */
			memcpy(pWk, &agentCfg.hdr, sizeof(agentCfg.hdr));
			pWk += sizeof(agentCfg.hdr);
			memcpy(pWk, &agentCfg.szServerUID, sizeof(agentCfg.szServerUID));
			pWk += sizeof(agentCfg.szServerUID);
			memcpy(pWk, &agentCfg.data, sizeof(agentCfg.data));
			pWk += sizeof(agentCfg.data);

			/* now ready to read the file data  */
			toread = (int)agentCfg.data.uiSize;
			if (toread != 0) {
				r = ftd_sock_recv(s, (char *)pWk, toread);
			} else { 
				r = toread;	/* config msg does not contain data */
			}

			if (r == toread) {   /* success, message entirely read */
				*ppRcvCfgData = (mmp_mngt_ConfigurationMsg_t *)pData;
			} else {
				free(pData);
			}
		}
	}
}

/*
 * to release ptr allocated by mmp_mngt_recv_cfg_data()
 */
void
mmp_mngt_free_cfg_data_mem(mmp_mngt_ConfigurationMsg_t **ppRcvCfgData) 
{
	if (*ppRcvCfgData != 0) {
		free((char *)*ppRcvCfgData);
	}
}

int
ftd_mngt_write_file_to_disk(const mmp_TdmfFileTransferData *filedata,
	const char* pData)
{
	int r;
	char fn[512+1];
	int fd;
	int permission;
	char *savedir;

	/* iType check */
	switch(filedata->iType) {
	case MMP_MNGT_FILETYPE_TDMF_CFG:
		permission = 0644;
		savedir = DTCCFGDIR;
		break;
	case MMP_MNGT_FILETYPE_TDMF_EXE:
		permission = 0744;
		savedir = TDMFFILEDIR;
		break;
	case MMP_MNGT_FILETYPE_TDMF_BAT:
		permission = 0744;
		savedir = TDMFFILEDIR;
		break;
	case MMP_MNGT_FILETYPE_TDMF_TAR:
		permission = 0644;
		savedir = TDMFFILEDIR;
		break;
	case MMP_MNGT_FILETYPE_TDMF_ZIP:
		permission = 0644;
		savedir = TDMFFILEDIR;
		break;
	default:
		logoutx(4, F_ftd_mngt_write_file_to_disk, "iType is not supported", "iType", filedata->iType);
		return(-1);
	}

	sprintf(fn, "%s/%s", savedir, filedata->szFilename);

	if (filedata->uiSize > 0) {
		if ((fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, permission)) < 0) {;
			r = -2;
		} else {
			if (write(fd, pData, filedata->uiSize) == filedata->uiSize) {
				if (filedata->iType == MMP_MNGT_FILETYPE_TDMF_CFG) {
					write(fd, "\n\n", 2);
				}
				r = 0;
			} else {
				r = -1;
			}
			close(fd);
		}
	} else {   
		/* request to delete file. */
		r = remove(fn);
		/* upon success, r is set to 0. */
		if (r == -1) {
			if (errno == ENOENT) {
				/* file does not exist */
				r = 0;
			}
		}
	}
	return r;
}

int 
ftd_mngt_set_config(sock_t *sockID)
{
	int		    r = 0;
	int		    status;
	mmp_mngt_ConfigurationMsg_t     *pRcvCfgData;
	mmp_mngt_ConfigurationStatusMsg_t response;

	/* complete reception of group cfg data message */ 
	mmp_mngt_recv_cfg_data(sockID, NULL, &pRcvCfgData);

	if (pRcvCfgData == NULL) { 
		/* problem receiving the message data */ 
		sprintf(logmsg, "while receiving Logical group configuration from 0x%x %s\n", sockID->rip, sockID->rhostname);
		logout(4, F_ftd_mngt_set_config, logmsg);
		return -1;
	}

	/* 
	 * convertion from network bytes order to host byte order done
	 * in mmp_mngt_recv_cfg_data()
	 */

	sprintf(logmsg, "Set configuration msg received. Cfg file name = %s, size = %d bytes\n", pRcvCfgData->data.szFilename, pRcvCfgData->data.uiSize);
	logout(9, F_ftd_mngt_set_config, logmsg);

	/* 
	 * dump file to disk.
	 * file data is contiguous to mmp_mngt_ConfigurationMsg_t structure.
	 */
	r = ftd_mngt_write_file_to_disk(&pRcvCfgData->data, (char *)(pRcvCfgData + 1));
	if (r == 0) {
		/* success */
		status = 0;
		sprintf(logmsg, "Success writing, Cfg file %s to disk!\n", pRcvCfgData->data.szFilename);
		logout(9, F_ftd_mngt_set_config, logmsg);
	} else {
		/* err writing cfg file */
		status = 2;
		sprintf(logmsg, "writing, Cfg file %s\n", pRcvCfgData->data.szFilename);
		logout(4, F_ftd_mngt_set_config, logmsg);
	}

	/* prepare status message */
	memset(&response, 0x00, sizeof(mmp_mngt_ConfigurationStatusMsg_t));
	response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
	response.hdr.mngttype       = MMP_MNGT_SET_CONFIG_STATUS;
	response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
	response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
	mmp_convert_mngt_hdr_hton(&response.hdr);

	/* skip cfg file prefix('p' or 's') */
	response.usLgId = (unsigned short)strtoul(&pRcvCfgData->data.szFilename[1], NULL, 10);

	/* convert to network byte order */
	response.usLgId     = htons(response.usLgId);
	response.iStatus    = htonl(status);
	strcpy(response.szServerUID, pRcvCfgData->szServerUID);

	/* send status message to requester */   
	r = ftd_sock_send(sockID, (char *)&response, sizeof(response)); 

	/* release memory obtained from mmp_mngt_recv_cfg_data() */
	mmp_mngt_free_cfg_data_mem(&pRcvCfgData);

	return r;
}

void
mmp_mngt_recv_file_data(sock_t *s, mmp_mngt_header_t *msghdr, 
	mmp_mngt_FileMsg_t **ppRcvFileData)
{
	int r;
	int toread;
	int iMsgSize;
	mmp_mngt_FileMsg_t agentFile;

	*ppRcvFileData = 0;

	memset(&agentFile, 0x00, sizeof(mmp_mngt_FileMsg_t));
	/* msg header as been read by caller and hdr is availble from msghdr. */
	if (msghdr != NULL) {
		agentFile.hdr = *msghdr;
	}

	/* reads other mmp_mngt_FileMsg_t fields */
	toread = sizeof(agentFile.szServerUID);
	r = ftd_sock_recv(s, (char *)agentFile.szServerUID, toread);
	if (r == toread) {
		/* now ready to read remainder of mmp_mngt_FileMsg_t msg */
		toread = sizeof(agentFile.data);
		r = ftd_sock_recv(s, (char *)&agentFile.data, toread);
		if (r == toread) {
			char *pData, *pWk;

			agentFile.data.iType  = ntohl(agentFile.data.iType);
			agentFile.data.uiSize = ntohl(agentFile.data.uiSize);

			/* read remainder of message in a CONTIGUOUS buffer */
			iMsgSize = sizeof(mmp_mngt_FileMsg_t) + agentFile.data.uiSize ;

			sprintf(logmsg, "uiSize = %d, szFilename = %s\n", agentFile.data.uiSize, agentFile.data.szFilename);
			logout(9, F_mmp_mngt_recv_file_data, logmsg);

			pWk = pData = (char *)malloc(iMsgSize * sizeof(char));

			/* copy data already received to contiguous buffer */
			memcpy(pWk, &agentFile.hdr, sizeof(agentFile.hdr));
			pWk += sizeof(agentFile.hdr);
			memcpy(pWk, &agentFile.szServerUID, sizeof(agentFile.szServerUID));
			pWk += sizeof(agentFile.szServerUID);
			memcpy(pWk, &agentFile.data, sizeof(agentFile.data));
			pWk += sizeof(agentFile.data);

			/* now ready to read the file data  */
			toread = (int)agentFile.data.uiSize;
			if (toread != 0) {
				r = ftd_sock_recv(s, (char *)pWk, toread);
			} else { 
				r = toread;	/* file msg does not contain data */
			}

			if (r == toread) {   /* success, message entirely read */
				*ppRcvFileData = (mmp_mngt_FileMsg_t *)pData;
			} else {
				free(pData);
			}
		}
	}
}

/*
 * to release ptr allocated by mmp_mngt_recv_file_data()
 */
void
mmp_mngt_free_file_data_mem(mmp_mngt_FileMsg_t **ppRcvFileData) 
{
	if (*ppRcvFileData != 0) {
		free((char *)*ppRcvFileData);
	}
}

int 
ftd_mngt_set_file(sock_t *sockID)
{
	int r,status;	
	mmp_mngt_FileMsg_t      *pRcvFileData;
	mmp_mngt_FileStatusMsg_t response;

	/* complete reception of TDMF file message */ 
	mmp_mngt_recv_file_data(sockID, NULL, &pRcvFileData);

	if (pRcvFileData == NULL) { 
		/* problem receiving the message data */ 
		sprintf(logmsg, "while receiving Replicator file from 0x%x %s\n", sockID->rip, sockID->rhostname);
		logout(4, F_ftd_mngt_set_file, logmsg);
		return -1;
	}

	/* 
	 * convertion from network bytes order to host byte order done
	 * in mmp_mngt_recv_file_data()
	 */

	sprintf(logmsg, "Set Replicator file msg received. file name = %s, size = %d bytes\n", pRcvFileData->data.szFilename, pRcvFileData->data.uiSize);
	logout(9, F_ftd_mngt_set_file, logmsg);

	/* 
	 * dump file to disk.
	 * file data is contiguous to mmp_mngt_FileMsg_t structure.
	 */
	r = ftd_mngt_write_file_to_disk(&pRcvFileData->data, (char *)(pRcvFileData + 1));
	if (r == 0) {
		/* success */
		status = 0;
		sprintf(logmsg, "Success writing, file %s to disk!\n", pRcvFileData->data.szFilename);
		logout(9, F_ftd_mngt_set_file, logmsg);
	} else {
		/* err writing TDMF file */
		status = 2;
		sprintf(logmsg, "writing, file %s\n", pRcvFileData->data.szFilename);
		logout(4, F_ftd_mngt_set_file, logmsg);
	}

	/* prepare status message */
	memset(&response, 0x00, sizeof(mmp_mngt_FileStatusMsg_t));
	response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
	response.hdr.mngttype       = MMP_MNGT_TDMF_SENDFILE_STATUS;
	response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
	response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
	mmp_convert_mngt_hdr_hton(&response.hdr);

	/* convert to network byte order */
#if 0
	response.usLgId = (unsigned short)strtoul( &pRcvFileData->data.szFilename[1], NULL, 10 );
#else
	response.usLgId = 0;
#endif
	response.usLgId     = htons(response.usLgId);
	response.iStatus    = htonl(status);
	strcpy(response.szServerUID, pRcvFileData->szServerUID);

	/* send status message to requester */   
	r = ftd_sock_send(sockID, (char *)&response, sizeof(response)); 

	/* release memory obtained from mmp_mngt_recv_file_data() */
	mmp_mngt_free_file_data_mem(&pRcvFileData);

	return r;
}
#endif /* _FTD_MNGT_SET_CONFIG_C_ */
