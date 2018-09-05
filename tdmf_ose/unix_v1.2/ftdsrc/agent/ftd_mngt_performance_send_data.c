/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/* #ident "@(#)$Id: ftd_mngt_performance_send_data.c,v 1.31 2018/04/04 18:08:18 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_PERFORMANCE_SEND_DATA_C_
#define _FTD_MNGT_PERFORMANCE_SEND_DATA_C_

char	secondaryname[256];
extern int secondaryport;

int n_rmd;
 
static ftd_perf_instance_t *pPrevDeviceInstanceData = NULL;
static int gbForceTxAllGrpState;
static int gbForceTxAllGrpPerf;
/* reduce malloc area */
static int prevNPerfData = 0;

static int cpfifo = -1;

static char infoline[128 * 1000];

grpstate_t	grpstate[FTD_MAX_GROUPS]; 
char pmdlist[FTD_MAX_GROUPS]; 
char localrmdlist[FTD_MAX_GROUPS]; 

int enable_debug_logging = 0;

static int dtcpanalyze_consecutive_errors_counter[1024];
#define CONSECUTIVE_PANALYZE_ERRORS_BEFORE_LOG 500


typedef struct devinfo {
	char		devname[64];
	int		entries;
	int		mode;
	int		actualkbps;
	int		effectkbps;
	int		done;
	int		babused;
	int		localread;
	int		localwrite;
} devinfo_t;

/* -- structure used for reading and parsing a line from the config file */
typedef struct _line_state {
	int	invalid;       /* bogus boolean. */
	int	lineno;
	char	line[256];     /* storage for parsed line */
	char	readline[256]; /* unparsed line */
	/* next four elements used for parsing throttles */
	char	word[256];
	int	plinepos;
	int	linepos;
	int	linelen;
	/* next two elements for parsing a value or a string */
	long	value;
	int	valueflag;
	/* ptrs to parsed parameters in line[] */
	char	*key;
	char	*p1;
	char	*p2;
	char	*p3;
} LineState;

typedef struct _rmd_data {
	char	shost[256];
	int		sport;
	char	rmdlist[BUF_SIZE +1];
} rmd_data;

rmd_data *rmdlist = NULL;

void
ftd_mngt_performance_reduce_stat_upload_period(int iFastPerfPeriodSeconds, int bWakeUpStatThreadNow)
{
	;
}

void
ftd_mngt_performance_send_all_to_Collector()
{
	gbForceTxAllGrpState  = 1;
	gbForceTxAllGrpPerf   = 1;
}

/* bit 0 indicates if group is started (1) or stopped (0) */
void
setStartState(grpstate_t *gp, int bNewStartState) {

	short prevState = gp->m_state.sState;

	/* set or clear bit 0 */
	gp->m_state.sState = (gp->m_state.sState & 0xfffe) | (bNewStartState ? 0x1 : 0x0);

	/* preserve a 'true' m_bStateChanged til hasGroupStateChanged() is called */
	gp->m_bStateChanged = gp->m_bStateChanged || (prevState != gp->m_state.sState);
}

void
setGroupNbr(grpstate_t *gp, short sGrpNbr) {
	gp->m_state.sRepGrpNbr = sGrpNbr;
}

short
getGroupNbr(grpstate_t *gp) {
	return (gp->m_state.sRepGrpNbr);
}

int
isGroupStarted(grpstate_t *gp) {
	/* bit 0 = group started/stopped */
	return (gp->m_state.sState & 0x1);
}

int
hasGroupStateChanged(grpstate_t	*gp) {

	int ret = gp->m_bStateChanged;

	gp->m_bStateChanged = 0;	/* auto-reset */
	return ret;
}

void
RepGrpState(grpstate_t *gp, short sGrpNbr, int bStarted) {
	gp->m_state.sState = 0;
	gp->m_state.sRepGrpNbr = sGrpNbr;
	setStartState(gp, bStarted);
}

void
getGroupState(grpstate_t *gp, mmp_TdmfGroupState *state) {
	state->sState = gp->m_state.sState;
	state->sRepGrpNbr = gp->m_state.sRepGrpNbr;
}

/* 
 * called from ftd_mngt_StatusMsgThread()
 * bit 1 indicates if group is in checkpoint mode (1) or not (0)
 */
void
setCheckpointState(int iGroupNbr, int bIsInCheckpoint)
{
	short prevState;
	grpstate_t *gp;

	if (iGroupNbr >= FTD_MAX_GROUPS) {
		return;
	}
	gp = &grpstate[iGroupNbr];
	prevState = gp->m_state.sState;
	/* set or clear bit 1 */
	/* preserve a 'true' m_bStateChanged until hasGroupStateChanged()
	   is called */
	gp->m_state.sState = (gp->m_state.sState & 0xfffd) | (bIsInCheckpoint ? 0x2 : 0x0);
	gp->m_bStateChanged = gp->m_bStateChanged || (prevState != gp->m_state.sState);

	sprintf(logmsg, "grp = %d, cp = %d, gpSt = %d, gpStChg = %d\n",
			iGroupNbr, bIsInCheckpoint, gp->m_state.sState, gp->m_bStateChanged);
	logout(11, F_StatThread, logmsg);
}

int
isinCheckpoint(grpstate_t *gp)
{
	if (gp->m_state.sState & 0x2) {
		return 1;
	} else {
		return 0;
	}
}

static int
ispmd(int num)
{
	return (int)pmdlist[num];
}

int
isrmdx(int num)
{
	return (int)localrmdlist[num];
}

static int
checkrmd(int lgnumber)
{
	struct sockaddr_in	servaddr;
#if !defined(FTD_IPV4)
	struct sockaddr_in6	servaddr6;
#endif
	int			len;
	int			i, n, r;
	int			sock;
	char		CheckRmd[] = "ftd get all process info RMD_\n";
	char		*answer;
	char		*ptr;
	char		buff[8];
	rmd_data	*tmpdata;
	char 		tmp[48];
	ipAddress_t rmd_address;
	int error,err;
	char * token_name;

#if defined(FTD_IPV4)
	struct hostent *hp = 0;
#else
	struct hostent *hp_host = 0;
	struct addrinfo *result ;
	struct addrinfo *res;
	struct addrinfo hints;
	bzero(&hints, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM; 
#endif


 
	/* check rmd list */
	if(rmdlist != NULL){
		for(i=0;i<n_rmd;++i){
			tmpdata = rmdlist + i;
			if(strcmp(tmpdata->shost, secondaryname) == 0 &&
										tmpdata->sport == secondaryport){
				memset (buff, 0, sizeof(buff));
				sprintf(buff, "RMD_%03d", lgnumber);
				ptr = strstr(tmpdata->rmdlist, buff);
				return ptr != NULL ? 1 : 0;
			}
		}
	}


#if defined(FTD_IPV4)
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		logoutx(4, F_checkrmd, "socket error", "errno", errno);
		return 0;
	}	
	if ((hp = gethostbyname(secondaryname)) == 0) {
		logoutx(4, F_checkrmd, "gethostbyname error", "errno", errno);
		close(sock);
		return 0;
	}
#else	
	if ( 0 != ( error = getaddrinfo( secondaryname ,  NULL , &hints, &result ) ) ) {
		

			logoutx(4, F_checkrmd, " CHECK RMD getaddrinfo error", gai_strerror(error), error);
			return 0;
	}


  			 sock = socket(result->ai_family, SOCK_STREAM, 0 );
#endif  // END OF FTD_IPV4


#if defined(FTD_IPV4)
	memset ((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(secondaryport);
#else
	if(result->ai_family == AF_INET) {
		memset ((char *)&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(secondaryport);
		
		if ((hp_host = gethostbyname(secondaryname)) == 0) {
		logoutx(4, F_checkrmd, "gethostbyname error", "errno", errno);
		close(sock);
		return 0;
		}

		memcpy((caddr_t)&servaddr.sin_addr, hp_host->h_addr_list[0], hp_host->h_length);
	}
	else
	{
		memset ((char *)&servaddr6, 0, sizeof(servaddr6));
		servaddr6.sin6_family = AF_INET6;
		servaddr6.sin6_port = htons(secondaryport);
		token_name = strtok(secondaryname,"%");
		if(token_name != NULL)
			strcpy(secondaryname, token_name);
		
		inet_pton(AF_INET6,secondaryname,&servaddr6.sin6_addr.s6_addr);
	//	servaddr6.sin6_scope_id = 1; 

	}
#endif
	/* set the TCP SNDBUF, RCVBUF to the maximum allowed */
	n = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(int));
#if defined(FTD_IPV4)
memcpy((caddr_t)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
	if ((connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr))) >= 0) {
		/* connect success */
		r = write(sock, CheckRmd, sizeof(CheckRmd));
		if (r !=sizeof(CheckRmd)) {
			logoutx(4, F_checkrmd, "write error", "errno", errno);
			/* write error */
			close(sock);
			return 0;
		}
		/* write success */
		/* answer read */
		answer = (char*)malloc(BUF_SIZE + 1);
		if(answer == NULL){
			logoutx(1, F_checkrmd, "malloc error", "errno", errno);
			close(sock);
			return 0;
		}
		if((r = read(sock, answer, BUF_SIZE)) < 0) {
			/* read error */
			logoutx(4, F_checkrmd, "read error", "errno", errno);
			free(answer);
			close(sock);
			return 0;
		}
		answer[r] = 0;
		/* save rmd list */
		rmdlist = (rmd_data *)realloc(rmdlist, sizeof(rmd_data) * (n_rmd + 1));
		if(rmdlist == NULL){
			logoutx(1, F_checkrmd, "realloc error", "errno", errno);
			logout(1, F_checkrmd, "... can not create RMD list.\n");
			n_rmd = 0;
		} else {
			tmpdata = rmdlist + n_rmd;
			memset(tmpdata, 0, sizeof(rmd_data));
			strcpy(tmpdata->shost, secondaryname);
			tmpdata->sport = secondaryport;
			strcpy(tmpdata->rmdlist, answer);
			++n_rmd;
		}
		memset (buff, 0, sizeof(buff));
		sprintf(buff, "RMD_%03d", lgnumber);
		ptr = strstr(answer, buff);
		close(sock);
		free(answer);
		return ptr != NULL ? 1 : 0;
	} else {
		logoutx(12, F_checkrmd, "connect error", "errno", 325);
		close(sock);
		return 0;
	}
#else
	if(result->ai_family == AF_INET){
			 if ((connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr))) >= 0) {
				/* connect success */
				r = write(sock, CheckRmd, sizeof(CheckRmd));
				if (r !=sizeof(CheckRmd)) {
					logoutx(4, F_checkrmd, " CHECK RMD write error", "errno", errno);
					/* write error */
					close(sock);
					return 0;
				}
				/* write success */
				/* answer read */
				answer = (char*)malloc(BUF_SIZE + 1);
				if(answer == NULL){
					logoutx(1, F_checkrmd, "CHECK RMD malloc error", "errno", errno);
					close(sock);
					return 0;
				}
				if((r = read(sock, answer, BUF_SIZE)) < 0) {
					/* read error */
					logoutx(4, F_checkrmd, " CHECK RMD read error", "errno", errno);
					free(answer);
					close(sock);
					return 0;
				}
				answer[r] = 0;
				/* save rmd list */
				rmdlist = (rmd_data *)realloc(rmdlist, sizeof(rmd_data) * (n_rmd + 1));
				if(rmdlist == NULL){
					logoutx(1, F_checkrmd, "realloc error", "errno", errno);
					logout(1, F_checkrmd, "... can not create RMD list.\n");
					n_rmd = 0;
				} else {
	
					tmpdata = rmdlist + n_rmd;
					memset(tmpdata, 0, sizeof(rmd_data));
					strcpy(tmpdata->shost, secondaryname);
					tmpdata->sport = secondaryport;
					strcpy(tmpdata->rmdlist, answer);
					++n_rmd;
				}
				memset (buff, 0, sizeof(buff));
				sprintf(buff, "RMD_%03d", lgnumber);
				ptr = strstr(answer, buff);
				close(sock);
				free(answer);
				return ptr != NULL ? 1 : 0;
		   	}else {
		   		logoutx(12, F_checkrmd, "connect error", "errno", 378);
		   		close(sock);
		  		return 0;
		  	} 
	}
	else {
			if ((connect(sock, (struct sockaddr *)&servaddr6, sizeof(servaddr6))) >= 0) {
				/* connect success */
				r = write(sock, CheckRmd, sizeof(CheckRmd));
				if (r !=sizeof(CheckRmd)) {
					logoutx(4, F_checkrmd, "CHECK RMD write error", "errno", errno);
					/* write error */
					close(sock);
					return 0;
				}
				/* write success */
				/* answer read */
				answer = (char*)malloc(BUF_SIZE + 1);
				if(answer == NULL){
					logoutx(1, F_checkrmd, "CHECK RMD malloc error", "errno", errno);
					close(sock);
					return 0;
				}
				if((r = read(sock, answer, BUF_SIZE)) < 0) {
					/* read error */
					logoutx(4, F_checkrmd, " CHECK RMD read error", "errno", errno);
					free(answer);
					close(sock);
					return 0;
				}
				answer[r] = 0;
				/* save rmd list */
				rmdlist = (rmd_data *)realloc(rmdlist, sizeof(rmd_data) * (n_rmd + 1));
				if(rmdlist == NULL){
					logoutx(1, F_checkrmd, "realloc error", "errno", errno);
					logout(1, F_checkrmd, "... can not create RMD list.\n");
					n_rmd = 0;
				} else {
					tmpdata = rmdlist + n_rmd;
					memset(tmpdata, 0, sizeof(rmd_data));
					strcpy(tmpdata->shost, secondaryname);
					tmpdata->sport = secondaryport;
					strcpy(tmpdata->rmdlist, answer);
					++n_rmd;
				}
				memset (buff, 0, sizeof(buff));
				sprintf(buff, "RMD_%03d", lgnumber);
				ptr = strstr(answer, buff);
				close(sock);
				free(answer);
				return ptr != NULL ? 1 : 0;
			} else {
				logoutx(12, F_checkrmd, "connect error", "errno", 430);
				close(sock);
				return 0;
			}

	}
#endif // end of FTD_IPV4


}

static int
#if defined(linux)
getline_linux(FILE *fd, LineState *ls)
#else
getline(FILE *fd, LineState *ls)
#endif
{
	int i, len;
	int blankflag;
	char *line;

	if (!ls->invalid) {
        ls->invalid = TRUE;
		return TRUE;
	}
	ls->invalid = TRUE;

	ls->key = ls->p1 = ls->p2 = ls->p3 = NULL;
	ls->word[0] = '\0';
	ls->linelen = 0;
	ls->linepos = 0;
	ls->plinepos = 0;
	line = ls->line;

	while (1) {
		if (fgets(line, 256, fd) == NULL) {
			return FALSE;
		}

		ls->lineno++;
		len = strlen(line);
		ls->linelen = len;
		if (len < 5) continue;

		/* ignore blank lines */
		blankflag = 1;
		for (i = 0; i < len; i++) {
			if (isgraph(line[i])) {
				blankflag = 0;
				break;
			}
        	}
		if (blankflag) continue;

        	strcpy(ls->readline, ls->line);

		/* -- get rid of leading whitespace -- */
		i = 0;
		while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
		if (i >= len) continue;  
 
		/* -- if the first non-whitespace character is a "#", ignore the line */
		if (line[i] == '#') continue;

		/* -- accumulate the key */
		ls->key = &line[i];
		while ((i < len) && (line[i] != ' ') && (line[i] != '\t') &&
	  	  (line[i] != '\n')) i++;
		line[i++] = 0; 

		/* -- bypass whitespace */
		while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
		if (i >= len) break;

		/* -- accumlate first parameter */
		ls->p1 = &line[i];
		while ((i < len) && (line[i] != ' ') && (line[i] != '\t') &&
	    	(line[i] != '\n')) i++;
		line[i++] = 0;

		/* -- bypass whitespace */
		while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
		if (i >= len) break;

		/* -- accumlate parameter */
		ls->p2 = &line[i];
		while ((i < len) && (line[i] != ' ') && (line[i] != '\t') &&
	    	(line[i] != '\n')) i++;
		line[i++] = 0;

		/* -- bypass whitespace */ 
		while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
		if (i >= len) break;

		/* -- accumlate parameter */
		ls->p3 = &line[i];
		while ((i < len) && (line[i] != ' ') && (line[i] != '\t') &&
	    	(line[i] != '\n')) i++;
		line[i++] = 0;

		break;
	}
	return TRUE;
}

int
isrmd(int num)
{
	FILE		*fp;
	LineState	lstate;
	struct stat	statbuf;
	char 		filename[MMP_MNGT_MAX_TDMF_PATH_SZ];
	char		primaryname[256];
	int		rc;
	int		isreadhost = 0;
	int		isreadport = 0;

	sprintf(filename, "%s/p%03d.cur", DTCCFGDIR, num);
	if (stat(filename, &statbuf) < 0) {
		sprintf(logmsg, "%s does not exist.\n", filename);
		logout(4, F_isrmd, logmsg);
		return 0;
	}
	if ((fp = fopen(filename, "r")) == NULL) {
		sprintf(logmsg, "%s fopen failed. errno = %d\n", filename, errno);
		logout(4, F_isrmd, logmsg);
		return 0;
	}

	/* initialize the parsing state */
	lstate.lineno = 0;
	lstate.invalid = 1;
	memset(secondaryname, 0x00, sizeof(secondaryname));
	memset(primaryname, 0x00, sizeof(primaryname));
	secondaryport = SECONDARYPORT;
	while(1) {
#if defined(linux)
		if (!getline_linux(fp, &lstate)) {
#else
		if (!getline(fp, &lstate)) {
#endif
			break;
		}
		if (strcmp("SYSTEM-TAG:", lstate.key) == 0 
			&& strcmp("PRIMARY", lstate.p2) == 0) {
			while(1) {
#if defined(linux)
				if (!getline_linux(fp, &lstate)) {
#else
				if (!getline(fp, &lstate)) {
#endif
					break;
				}
				if (strcmp("HOST:", lstate.key) == 0) {
					strcpy(primaryname, lstate.p1);
					break;
				}
			}
		}
		if (strcmp("SYSTEM-TAG:", lstate.key) == 0 
			&& strcmp("SECONDARY", lstate.p2) == 0) {
			while(1) {
#if defined(linux)
				if (!getline_linux(fp, &lstate)) {
#else
				if (!getline(fp, &lstate)) {
#endif
					break;
				}
				if (strcmp("HOST:", lstate.key) == 0) {
					strcpy(secondaryname, lstate.p1);
					isreadhost = 1;
					if(isreadport) break;
				}
				if (strcmp("SECONDARY-PORT:", lstate.key) == 0) {
					secondaryport = (int)atol(lstate.p1);
					isreadport = 1;
					if(isreadhost) break;
				}
			}
		}
	}
	fclose(fp);

	sprintf(logmsg, "lgnum(%d) PRIMARY = %s SECONDARY = %s\n", num, primaryname, secondaryname);
	logout(19, F_isrmd, logmsg);
	if (strlen(secondaryname) == 0) {
		/* cannot find */ 
		return 0;
	}
	if (strcmp(primaryname, secondaryname) == 0) {
		/* secondaryname = primaryname : loopback */
		rc = isrmdx(num);	
	} else {
		/* secondaryname != primaryname  */
		rc = checkrmd(num);
	}
	return rc;
}

/* return 0 : successful
 *        -1: failed
 */
int str_ftod(char *str)
{
	int     i;
	char    *p;

	p = strstr(str, ".");
	if (p == NULL)		/* if "." is not found */
		return -1;

	for (i = 0; i < 3; i++, p++) {
		*p = *(p + 1);
	}
	return 0;
}

int
getdevinfo(int lgnum, int devnum, devinfo_t *dip)
{
	#define	RETRY_MAX	(100)		/* try 100 times (do 99 times retry) */
	int		r = 0;
	int		i,j;
	struct stat	statbuf;
	char 		*linebuf;
	char 		filename[MMP_MNGT_MAX_TDMF_PATH_SZ];
	char 		act[16], eft[16], done[16], used[16];
	char		ent[4], mode[4], lread[16], lwrite[16];
	FILE		*f;
	char		temp[16];
	int			retry_count;
	int			c;
	int			line_count;
	long		offset;

	sprintf(filename, "%s/p%03d.prf", DTCVAROPTDIR, lgnum);
	if (stat(filename, &statbuf) < 0) {
		/* dtc is not started. */
		sprintf(logmsg, "%s does not exist.\n", filename);
		logout(4, F_getdevinfo, logmsg);
		return -1;
	}

	if(devnum == 0){
		/* get Device name, Acutual, and Effective */ 

		/* loop for RETRY_MAX times retry */
		for (retry_count = 1; retry_count <= RETRY_MAX; retry_count++) {
			/* define error return procedure for this part */
#define	DO_CLEARUP
#define	ERP(MESSAGE) { \
	logoutx(4, F_getdevinfo, MESSAGE, "errno", errno); \
	DO_CLEARUP; \
	return -1; \
}

			if ((f = fopen(filename, "r")) == NULL) ERP("fopen failed");
#undef	DO_CLEARUP
#define	DO_CLEARUP fclose(f)

			if (fseek(f, 0, SEEK_END) == -1) ERP("fseek failed");
			if ((offset = ftell(f)) == -1) ERP("ftell failed");

			/* loop until finding 2 lfs */
			line_count = 0;
			for (offset--; offset >= 0; offset--) {
				if (fseek(f, offset, SEEK_SET) == -1) ERP("fseek failed");
				if ((c = fgetc(f)) == EOF) ERP("fgetc failed");
				if (c == '\n' && ++line_count == 2) break;
				if (offset == 0) {
					if (fseek(f, 0, SEEK_SET) == -1) ERP("fseek failed");
				}
			}
			if (line_count >= 1) {
				/* save last complete line to infoline, and break */
				memset(infoline, 0, sizeof(infoline));
				if (fgets(infoline, sizeof(infoline), f) == NULL) ERP("fgets failed");
				DO_CLEARUP;
				break;
			}

			DO_CLEARUP;
#undef	DO_CLEARUP
#undef	ERP

			usleep(10000);
		}
		if (retry_count >= RETRY_MAX) {
			sprintf(logmsg, "The device information cannot be gotten from %s.\n", filename);
			logout(4, F_getdevinfo, logmsg);
			return -1;
		}
	}

	i = devnum * 11 + 3;
	linebuf = infoline; 
	for(j = 0; linebuf !=NULL && *linebuf != '\0' && j < i-1;j++){
		linebuf = (char *)strchr(linebuf, ' ');
		while(linebuf != NULL && linebuf != '\0' && *linebuf == ' ') {
			linebuf++;
		}
	}
	if (linebuf == NULL || *linebuf == '\0') {
		return -1;
	}

	r = sscanf(linebuf, "%s %s %s %s %s %s %s %s %s %s", dip->devname, act, eft, ent, temp, done, used, mode, lread, lwrite);

	if (r == 10) {
		if (str_ftod(act)) return -1;
		if (str_ftod(eft)) return -1;
		if (str_ftod(done)) return -1;
		if (str_ftod(used)) return -1;
		if (str_ftod(lread)) return -1;
		if (str_ftod(lwrite)) return -1;

		dip->actualkbps = atoi(act);
		dip->effectkbps = atoi(eft);
		dip->done = atoi(done);
		dip->babused = atoi(used);
		dip->localread = atoi(lread);
		dip->localwrite = atoi(lwrite);
		dip->entries = atoi(ent);
		dip->mode = atoi(mode);
	} else {
		return -1;
	}
	return 0;
}

void
setInstanceName(unsigned short *inp, int lgnum, char *name)
{
	char	line[MAX_SIZEOF_INSTANCE_NAME];
	char	*dp;
	int	strl, i;

	memset(line, 0, sizeof(line));
	memset(inp, 0, sizeof(unsigned short) * MAX_SIZEOF_INSTANCE_NAME);
	if (name == NULL) {
		sprintf(line, "p%03d", lgnum);
	} else {
		sprintf(line, "p%03d ==> %s", lgnum, name);
	}
	dp = line;
	strl = strlen(line);
	for (i = 0; i < strl; i++) {
		strncpy((char *)inp, dp, 1);
		dp++;
		inp++;
	}
	return;
}

static double
time_so_far_usec()
{
	struct timeval tp;

	gettimeofday(&tp, (struct timezone *) NULL);
	return ((double) (tp.tv_sec)*1000000.0) + (((double) tp.tv_usec) );
}

void
make_md_list()
{
	char	cmdline[128];
	char	line[256];
	char	slgnum[4];
	char	*p, *r;
	int		ilgnum;
	FILE	*f;

	sprintf(cmdline, "/bin/ps -ef | /bin/grep [PR]MD_[0-9][0-9][0-9] | /bin/grep -v grep");
	if ((f = popen(cmdline, "r")) == NULL) {
		logoutx(4, F_make_md_list, "popen failed", "errno", errno);
		return;	
	}
	memset(line, 0, sizeof(line));
	while(fgets(line, sizeof(line), f) != NULL) {
		p = strstr(line, "PMD_");
		r = strstr(line, "RMD_");
		if(p != NULL){
			p += 4;
			strncpy(slgnum, p, 3);
			slgnum[3] = 0;
			ilgnum = atoi(slgnum);
			pmdlist[ilgnum] = 1;
		}
		if(r != NULL){
			r += 4;
			strncpy(slgnum, r, 3);
			slgnum[3] = 0;
			ilgnum = atoi(slgnum);
			localrmdlist[ilgnum] = 1;
		}
	}
	pclose(f);
	return;
}

void debug_trace(char *trace_string)
{
    if (enable_debug_logging)
    {
        sprintf(logmsg, ">>> TRACING %s\n", trace_string);
        logout(4,F_main,logmsg);
    }
}

void debug_trace_and_value(char *trace_string, unsigned long value)
{
    if (enable_debug_logging)
    {
        sprintf(logmsg, ">>> TRACING %s: %lu\n", trace_string, value);
        logout(4,F_main,logmsg);
    }
}


// WI_338550 December 2017, implementing RPO / RTT
void log_RPO_data( ftd_perf_instance_t *lgdata, int lgnum )
{
    char logmsg[128];
    
    sprintf(logmsg, "---------------------------------------------------------\n");
    logout(4,F_main,logmsg);
    sprintf(logmsg, "RPO data for group %d\n", lgnum);
    logout(4,F_main,logmsg);
    sprintf(logmsg, "currentRpo: %d\n", lgdata->currentRpo);
    logout(4,F_main,logmsg);
    sprintf(logmsg, "maxRpo: %d\n", lgdata->maxRpo);
    logout(4,F_main,logmsg);
    sprintf(logmsg, "currentRtt: %d\n", lgdata->currentRtt);
    logout(4,F_main,logmsg);
    sprintf(logmsg, "timeRemaining: %d\n", lgdata->timeRemaining);
    logout(4,F_main,logmsg);
    sprintf(logmsg, "pendingMB: %d\n", lgdata->pendingMB);
    logout(4,F_main,logmsg);
    sprintf(logmsg, "---------------------------------------------------------\n");
    logout(4,F_main,logmsg);

    return;
}

// WI_338550 December 2017, implementing RPO / RTT
int calculate_group_pendingMBs_and_timeRemaining( int *pendingMB, int *timeRemaining, ftd_stat_t *lgstat, int lgnum, long long total_group_pending_bytes, int currentRtt, int percent_not_done)
{
    // If Normal mode: use the BAB-sectors-based calculation, i.e.: the quantity of data in the BAB for the group.
    // If FullRefresh: use the refresh-offset-based calculation (total_group_pending_bytes); i.e.:
    //      we take the offset where we are on each device, calculate from that the pending MBs (still to be sent to the target) for each device,
    //      then we take the sum of all devices' pending MBs, which gives us the global value of pending MBs sent to the DMC for the group.
    // If Tracking: use the bitmap-based (dtcpanalyze) calculation; i.e.: we call dtcpanalyze in system call mode for the group we are processing
    //      and we take the value returned by dtcpanalyze to report to the DMC.
    // For any Refresh state other that FullRefresh, we take the SMALLER of:
    //      > the refresh-offset-based calculation (see FullRefresh logic above);
    //      > the bitmap-based (dtcpanalyze) calculation.
    //      For instance, if we have a total disk space of 8 GB in the group (let's say 5 devices of 3 GB, 2 GB, 1 GB, 1 GB and 1 GB respectively),
    //      if the dirty bits (tracking bitmap) represent 3 GB of pending data, during the Smart Refresh, the Pending MBs reported to the DMC will be
    //      3 GB UNTIL THE REFRESH PERCENTAGE WILL GO BEYOND 62,5% (62,5% of 8 GB = 5 GB), at which point we know for sure that there is less pending data
    //      than the value reported by dtcpanalyze (which does not change until dirty bits get cleared later on), so then we use the refresh percentage
    //      as the base for our calculations.
    // IMPROVEMENT February 7, 2018:
    // If in Refresh mode, if we use the bitmap-based calculation (ctdpanalyze) multiply the Refresh-percentage NOT DONE by the bitmap-based calculated value.
    // For instance, if dtcanalyze reported 1 GB pending data and if this value has been chosen to be the basis of the calculation, if the refresh-percentage done
    // is 25%, report 1 GB * 75% = 750 MBs.
        char cmdline[256];
        FILE    *infd;
        char    tmpfilename[64];
        int     panalyze_pendingMBs, refresh_offset_pendingMB;
        int     result;
static 	char    tmpfile_prefix[32] = "/tmp/dtcpanalyze_output.";


    // If Normal mode: use the BAB-sectors-based calculation
    if (lgstat->state == FTD_MODE_NORMAL)
    {
        *pendingMB = lgstat->wlsectors / (2 * 1024);
        *timeRemaining = ( ( ((*pendingMB * 1024 * 1024) / lgstat->network_chunk_size_in_bytes) * currentRtt) / 1000);
        return 0;
    }
    

    // If FullRefresh: use the refresh-offset-based calculation (total_group_pending_bytes);
    if ( lgstat->state == FTD_MODE_FULLREFRESH )
    {
        *pendingMB = (int)((total_group_pending_bytes / 1024) / 1024);
        *timeRemaining = (int)( ( (total_group_pending_bytes / lgstat->network_chunk_size_in_bytes) * currentRtt) / 1000 );
        return 0;
    }

    // All the other modes may need the dtcpanalyze bitmap computations
    memset(cmdline, 0, sizeof(cmdline));
#if defined(_AIX)
    sprintf(cmdline, "/usr/dtc/bin/dtcpanalyze -g%d -s 1>/dev/null 2> /dev/null", lgnum);
#else
    sprintf(cmdline, "/usr/local/bin/dtcpanalyze -g%d -s 1>/dev/null 2> /dev/null", lgnum);
#endif
    result = (int)(WEXITSTATUS(system(cmdline)));
    if (result != 0)
    {
        // Avoid overloading the logs with this message if it is recurrent.
        // Use a counter of occurrence per group and reset the counter upon success; success means
        // it is not a constant error which would overflow the logs.
        if( (dtcpanalyze_consecutive_errors_counter[lgnum] % CONSECUTIVE_PANALYZE_ERRORS_BEFORE_LOG) == 0 )
        {
            sprintf(logmsg, "Error calling dtcpanalyze to get pendingMBs for group %d; the call was the following:\n", lgnum);
            logout(4,F_StatThread,logmsg);
            logout_failure(4, F_StatThread, cmdline);
        }
        dtcpanalyze_consecutive_errors_counter[lgnum] += 1;
        *pendingMB = -1;
        *timeRemaining = -1;
        return -1;
    }
    else
    {
        // Read dtcpanalyze output file
        sprintf(tmpfilename, "%s%d\0", tmpfile_prefix, lgnum);
        infd = fopen(tmpfilename,"r");
        if (infd <= (FILE *)0)
        {
            /* file open error */
            if( (dtcpanalyze_consecutive_errors_counter[lgnum] % CONSECUTIVE_PANALYZE_ERRORS_BEFORE_LOG) == 0 )
            {
                sprintf(logmsg, "Error reading output file of dtcpanalyze to get pendingMBs for group %d\n", lgnum);
                logout(4,F_StatThread,logmsg);
            }
            dtcpanalyze_consecutive_errors_counter[lgnum] += 1;
            *pendingMB = -1;
            *timeRemaining = -1;
            return -1;
        }
        else
        {
            memset(cmdline, 0, sizeof(cmdline));
            fgets(cmdline,sizeof(cmdline),infd);
            panalyze_pendingMBs = atoi(cmdline);
            fclose(infd);
            dtcpanalyze_consecutive_errors_counter[lgnum] = 0;
            // Delete the dtcpanalyze output file once we have the value
            remove(tmpfilename);
        }
    }
    
    // If Tracking: use the bitmap-based (dtcpanalyze) calculation
    if (lgstat->state == FTD_MODE_TRACKING)
    {
        *pendingMB = panalyze_pendingMBs;
        // The following is to apply the same behaviour as TDMFIP Windows in Tracking (timeRemaining: N/A)
        *timeRemaining = -1;
        return 0;
    }

    // For any Refresh state other that FullRefresh take the smaller of:
    //      > the refresh-offset-based calculation
    //      > the bitmap-based (dtcpanalyze) calculation
    if (lgstat->state == FTD_MODE_REFRESH)
    {
        refresh_offset_pendingMB = (int)((total_group_pending_bytes / 1024) / 1024);
        if( refresh_offset_pendingMB < panalyze_pendingMBs )
        {
            *pendingMB = refresh_offset_pendingMB;
            *timeRemaining = (int)( ( (total_group_pending_bytes / lgstat->network_chunk_size_in_bytes) * currentRtt) / 1000 );
            return 0;
        }
        else
        {
            // Apply the percentage of refresh NOT DONE to the panalyze value
            *pendingMB = (int)((float)panalyze_pendingMBs * ((float)percent_not_done / 100.0));
            *timeRemaining = (int)( ( ((*pendingMB * 1024 * 1024) / lgstat->network_chunk_size_in_bytes) * currentRtt) / 1000 );
            return 0;
        }
    }
    sprintf(logmsg, "---------------------------------------------------------------------------------\n");
    logout(4,F_main,logmsg);
    sprintf(logmsg, ">>> TRACING: calculate_group_pendingMBs_and_timeRemaining(): got an unexpected group state; lgstat->state: %d\n", lgstat->state);
    logout(4,F_main,logmsg);
    return 0;
    
}

void
StatThread()
{
	ftd_stat_t              lgstat;
	stat_buffer_t		sb;
	disk_stats_t		devstat;
	devinfo_t		devinfo;
	struct stat 		statrdev;
	char			devbuffer[FTD_PS_DEVICE_ATTR_SIZE];
	char			filename[MMP_MNGT_MAX_TDMF_PATH_SZ];
	ftd_perf_instance_t	*lgdata, *devdata, *pData;
	devstat_t		*statp;
	int			pmd_connect, rmd_connect;
	int			i, lgtemp, ctlfd;
  long long totalsects;
  ftd_perf_instance_t	*newpData = NULL;
	int			curNPerfData, newNPerfData;
	int 			stat_count = 0;
	DIR             	*dfd;
	struct dirent 		*dent;
	struct stat 		cfgstat;
	int			interval = 0;
	char			tmp[32];
	double			st_time, en_time, percent;
	long long		sleep_time;
	int			error_occurred;
    // WI_338550 December 2017, implementing RPO / RTT
    int         enable_RPO_reporting;
    time_t      CurrentSystemTime;
    static unsigned long   MaxRPOLastResetTime[1024];
    static int             MaxRPOLastHour[1024];
	struct stat statbuf;
    int     result;
    long long group_disks_size_in_sectors, total_group_done_sectors;
    long long group_disks_size_in_bytes, total_group_done_bytes, total_group_pending_bytes;
    static int got_valid_RPO[1024];
    int percent_not_done;

	memset(MaxRPOLastResetTime, 0, sizeof(MaxRPOLastResetTime));
	memset(MaxRPOLastHour, 0, sizeof(MaxRPOLastHour));
	memset(got_valid_RPO, 0, sizeof(got_valid_RPO));
	memset(dtcpanalyze_consecutive_errors_counter, 0, sizeof(dtcpanalyze_consecutive_errors_counter));

	memset(grpstate, 0, sizeof(grpstate_t) * FTD_MAX_GROUPS);
	for (i = 0; i < FTD_MAX_GROUPS; i++) {
		grpstate[i].m_state.sRepGrpNbr = -1;
	}

	/*
	 * get transmission interval of server information from Agent cfg file.
	 */
	interval = MMP_MNGT_DEFAULT_INTERVAL;
	if ( cfg_get_software_key_value(TRANSMITINTERVAL, tmp, sizeof(tmp)) == 0 ){
		interval = atoi(tmp);
		if(interval < MMP_MNGT_DEFAULT_INTERVAL){
			logout(7, F_StatThread, "\'"TRANSMITINTERVAL"\' is illegal value. [4 - 3600]\n");
			interval = MMP_MNGT_DEFAULT_INTERVAL;
		}
		if(interval > MMP_MNGT_MAX_INTERVAL){
			logout(7, F_StatThread, "\'"TRANSMITINTERVAL"\' is illegal value. [4 - 3600]\n");
			interval = MMP_MNGT_MAX_INTERVAL;
		}
	}
	logoutx(17, F_StatThread, "", "interval", interval);

	/* 
	 * get checkpoint state updates from a fifo shared with 
	 * ftd_mngt_StatusThread().
	 * wait for ftd_mngt_StatusThread() to open the fifo.
	 * mkfifo(CHECKPOINT_FIFO,) called by process StatusThread()
	 */
	while((cpfifo = open(CHECKPOINT_FIFO, O_NONBLOCK|O_RDWR)) == -1) {
		sleep(2);
	}

	gbForceTxAllGrpState  = 0;
	gbForceTxAllGrpPerf   = 0;
	st_time = 0;
	en_time = 0;
	percent = 0;
	sleep_time = 0;

	/* Allocate some memory to begin with */
	newpData = (ftd_perf_instance_t*)malloc(sizeof(ftd_perf_instance_t));

	while (1) {
		error_occurred = 0;
		percent = ((en_time - st_time)/((double)interval * 1000000.0));
		if (percent > 0.60) {
		    sleep_time = (double)interval * 1000000.0;
		}
		else {
		    sleep_time = ((double)interval * 1000000.0) - (en_time - st_time);
		}
		sprintf(logmsg, "process in sleep %ld micro sec.\n", (long)sleep_time);
		logout(17, F_StatThread, logmsg);
		if(sleep_time > 0){
		  if( sleep_time >= 1000000 )   /* usleep does not accept values >= 1000000 (pc071109) */
		     sleep( (int)(sleep_time / 1000000) );   /* Does the second part */
                  usleep( (long)sleep_time % 1000000 );          /* Does the microsecond part */
		}

		st_time = time_so_far_usec();

		if(++stat_count == 2){
			ftd_mngt_performance_send_all_to_Collector();
			stat_count = 0;
		}
		
		curNPerfData = 0;
		/* clear pmd/rmd list */
		memset(pmdlist, 0, FTD_MAX_GROUPS); 
		memset(localrmdlist, 0, FTD_MAX_GROUPS); 
		if(rmdlist != NULL) free(rmdlist);
		rmdlist = NULL;
		n_rmd = 0;
		make_md_list();

		if (((DIR*)NULL) == (dfd = opendir(DTCCFGDIR))){
			sprintf(logmsg, "cannot open %s directory.\n", DTCCFGDIR);
			logout(4, F_StatThread, logmsg);
			en_time = time_so_far_usec();
			continue;
		}
		while (NULL != (dent = readdir(dfd))) {
			if((lgtemp = check_cfg_filename(dent, PRIMARY_CFG)) == -1) continue;

			setGroupNbr(&grpstate[lgtemp], lgtemp);
			sprintf(filename, "%s/p%03d.cur", DTCCFGDIR, lgtemp);
			/* borrow statrdev... */
			if (stat(filename, &statrdev) == -1) {
				/* this logical group is not started. */
                got_valid_RPO[lgtemp] = 0;
				setStartState(&grpstate[lgtemp], 0);
				continue;
			}

			if ((ctlfd = open(FTD_CTLDEV, O_RDONLY)) < 0) {
				sprintf(logmsg, "%s open error, errno = %d\n", FTD_CTLDEV, errno);
				logout(4, F_StatThread, logmsg);
				setStartState(&grpstate[lgtemp], 0);
				continue;
			}																			  

			memset(&lgstat, -1, sizeof(ftd_stat_t));
			memset(&sb, 0, sizeof(stat_buffer_t));
			sb.lg_num = lgtemp;
			sb.dev_num = 0; /* not refered */
			sb.len = sizeof(ftd_stat_t);
			sb.addr = (ftd_uint64ptr_t)(unsigned long)&lgstat;
			if (FTD_IOCTL_CALL(ctlfd, FTD_GET_GROUP_STATS, &sb) < 0) {
				/* 
				 * Warning: It's may be that cur file is alive
				 * and this logical group is dead.
				 */
				logoutx(19, F_StatThread, "ioctl FTD_GET_GROUP_STATS error", "errno", errno);
				close(ctlfd);
				setStartState(&grpstate[lgtemp], 0);
				continue;
			}
			close(ctlfd);

            // WI_338550 December 2017, implementing RPO / RTT
            enable_RPO_reporting = lgstat.WriteOrderingMaintained;

			/* reduce malloc area */
			newNPerfData = 1 + lgstat.ndevs;
			newpData = realloc(newpData, sizeof(ftd_perf_instance_t) * (curNPerfData + newNPerfData));
			pData = newpData + curNPerfData;
			memset(pData, -1, sizeof(ftd_perf_instance_t) * newNPerfData);
			curNPerfData += newNPerfData;

			lgdata = pData++;
			memset(lgdata, 0, sizeof(ftd_perf_instance_t));
			setInstanceName(lgdata->wcszInstanceName, lgtemp, NULL);
			lgdata->role = 'p';
            
			/* mark up FTD_MODE_CHECKPOINT_JLESS state */
			lgdata->drvmode = (lgstat.state == FTD_MODE_CHECKPOINT_JLESS) ?
					FTD_MODE_NORMAL : lgstat.state;
            
            debug_trace("======================================================");
            debug_trace_and_value("TRACE, lgdata->drvmode: ", lgdata->drvmode);
            debug_trace_and_value("TRACE, lgstat.state: ", lgstat.state);
            
			lgdata->lgnum = -100;
			lgdata->devid = lgtemp;
			lgdata->pctdone = 100;
			lgdata->pctbab = 100 - ((((double)lgstat.bab_free) * 100.0) /
				(((double)lgstat.bab_used) + ((double)lgstat.bab_free)));
			lgdata->entries = lgstat.wlentries;
			lgdata->sectors = lgstat.bab_used / SECTORSIZE;

            // WI_338550 December 2017, implementing RPO / RTT
            if( enable_RPO_reporting )
            {
                // Current time in seconds
                time(&CurrentSystemTime);

                debug_trace("------------------------------------------------------");
                debug_trace_and_value("TRACE, lgstat.LastIOTimestamp: ", lgstat.LastIOTimestamp);
                debug_trace_and_value("TRACE, lgstat.LastConsistentIOTimestamp: ", lgstat.LastConsistentIOTimestamp);
                debug_trace_and_value("TRACE, lgstat.OldestInconsistentIOTimestamp: ", lgstat.OldestInconsistentIOTimestamp);
                debug_trace_and_value("TRACE, CurrentSystemTime: ", CurrentSystemTime);
                
                // Compute RPO
                // The RPO is computed by using 3 variables: 
                // - LastIOTimestamp: Time of the last IO received in any mode
                // - OldestInconsistentIOTimestamp: Time of the oldest inconsistent IO (IO received on the source but not applied on the target)
                // - LastConsistentIOTimestamp: Time of the last consistent IO (IO applied on both source and target)
                // These variables can take a special value, besides the normal timestamp, indicating the system state:
                // - LastIOTimestamp = 0: Initial state - no IO received
                // - LastConsistentIOTimestamp = 0: Consistent point is unknown on the target
                // - OldestInconsistentIOTimestamp = 0: No inconsistent data available
                if (lgstat.LastConsistentIOTimestamp == 0)
                {
                    // The consistency point is unknown

                    if( (lgstat.OldestInconsistentIOTimestamp == 0) || (lgstat.state == FTD_MODE_FULLREFRESH) )
                    {
                        debug_trace("A");
                        // Special value indicating that no consistency point exists
                        lgdata->currentRpo = -1;
                    }
                    else
                    {
                        debug_trace("B");
                        // RPO is the time difference between the current time and the oldest inconsistent IO
                        lgdata->currentRpo = (int)(CurrentSystemTime - lgstat.OldestInconsistentIOTimestamp);
                        got_valid_RPO[lgtemp] = 1;
                    }
                }
                else
                {
                    // There is a valid consistency point on the target
                    if (lgstat.LastIOTimestamp == 0 || 
                        lgstat.LastConsistentIOTimestamp >= lgstat.LastIOTimestamp)
                    {
                        debug_trace("C");
                        // The PMD reached a consistency point later than the last IO, or never got newer IO,
                        // which means that both source and target and synchronized
                        lgdata->currentRpo = 0;
                        got_valid_RPO[lgtemp] = 1;
                    }
                    else
                    {
                        // RPO is the time difference between the last consistent point on the source and the last consistent point on the target,
                        // but if we are in Tracking:
                        //      > if OldestInconsistentIOTimestamp == 0: we are in sync; when I/O will occur, OldestInconsistentIOTimestamp will no longer be 0
                        //      > if OldestInconsistentIOTimestamp != 0: we are not in sync; take (current time) - OldestInconsistentIOTimestamp
                        // and if we are in Smart Refresh mode:
                        //      > if OldestInconsistentIOTimestamp == 0: we are in sync; when I/O will occur, OldestInconsistentIOTimestamp will no longer be 0
                        //      > if OldestInconsistentIOTimestamp != 0: we are not in sync;
                        //          >> if OldestInconsistentIOTimestamp is more recent than the recorded LastConsistentIOTimestamp (this can happen if we were idle in Normal
                        //              mode and were not receiving I/O and we got I/O after going to Tracking mode):
                        //              take (current time) - OldestInconsistentIOTimestamp
                        //          >> else:
                        //              take (current time) - LastConsistentIOTimestamp
                        if( lgstat.state == FTD_MODE_TRACKING )
                        {
                            if (lgstat.OldestInconsistentIOTimestamp == 0)
                            {
                                debug_trace("D");
                                lgdata->currentRpo = 0;
                            }
                            else
                            {
                                debug_trace("E");
                                lgdata->currentRpo = (int)(CurrentSystemTime - lgstat.OldestInconsistentIOTimestamp);
                            }
                        }
                        else
                        {
                            if( lgstat.state == FTD_MODE_REFRESH )
                            {
                                if (lgstat.OldestInconsistentIOTimestamp == 0)
                                {
                                    debug_trace("F");
                                    lgdata->currentRpo = 0;
                                }
                                else
                                {
                                    if( lgstat.OldestInconsistentIOTimestamp > lgstat.LastConsistentIOTimestamp )
                                    {
                                        debug_trace("G");
                                        lgdata->currentRpo = (int)(CurrentSystemTime - lgstat.OldestInconsistentIOTimestamp);
                                    }
                                    else
                                    {
                                        debug_trace("H");
                                        lgdata->currentRpo = (int)(CurrentSystemTime - lgstat.LastConsistentIOTimestamp);
                                    }
                                }
                            }
                            else
                            {
                                debug_trace("I");
                                lgdata->currentRpo = (int)(lgstat.LastIOTimestamp - lgstat.LastConsistentIOTimestamp);
                            }
                        }
                        got_valid_RPO[lgtemp] = 1;
                    }
                }
                // Reset max rpo after one hour
                if( got_valid_RPO[lgtemp] )
                {
                    if (CurrentSystemTime > (MaxRPOLastResetTime[lgtemp] + 60*60))
                    {
                        MaxRPOLastHour[lgtemp] = -1;
                        MaxRPOLastResetTime[lgtemp] = CurrentSystemTime;
                    }
                    if (lgdata->currentRpo != -1)
                    {
                        MaxRPOLastHour[lgtemp] = lgdata->currentRpo > MaxRPOLastHour[lgtemp] ? lgdata->currentRpo : MaxRPOLastHour[lgtemp];
                    }
                    lgdata->maxRpo = MaxRPOLastHour[lgtemp];
                }
                else
                {
                    // Did not get a valid RPO value yet
                    lgdata->maxRpo = -1;
                }
                // Round-trip-time:
                if( lgstat.state == FTD_MODE_TRACKING )
                {
                    lgdata->currentRtt = -1;   // N/A in Tracking
                }
                else
                {
                    // We now report the average of the most recent NUM_OF_RTT_SAMPLES RTT samples to reduce fluctuations
                    lgdata->currentRtt = lgstat.average_of_most_recent_RTTs;
                }

                debug_trace_and_value("lgdata->currentRpo: ", lgdata->currentRpo);
                lgdata->features.rpoIndicators = 1;

            }
            else
            {
                lgdata->features.rpoIndicators = 0;
            }

			/* make sure PMD/RMD are alive */
			pmd_connect = ispmd(lgtemp);
			rmd_connect = isrmd(lgtemp);
			if (pmd_connect && rmd_connect) {
				/* connected */
        			lgdata->connection = 1;
			} else if ((pmd_connect && !rmd_connect) 
				|| (!pmd_connect && rmd_connect)) {
        			lgdata->connection = 0;
			} else {
				/* accumulate */
        			lgdata->connection = -1;
			}

            group_disks_size_in_sectors = 0;
            total_group_done_sectors = 0;
            
			/* set device data */
			for (i = 0; i < lgstat.ndevs; i++) {
				devdata = pData++;

				memset(&devinfo, 0, sizeof(devinfo_t));
				if (getdevinfo(lgtemp, i, &devinfo) == -1) {
					error_occurred = 1;
					break;
				}
				if (stat(devinfo.devname, &statrdev) == -1) {
	 				sprintf(logmsg, "%s does not exist.\n", devinfo.devname);
	 				logout(4, F_StatThread, logmsg);
					break;
				}

				if ((ctlfd = open(FTD_CTLDEV, O_RDONLY)) < 0) {
	 				sprintf(logmsg, "%s open error, errno = %d\n", FTD_CTLDEV, errno);
					logout(4, F_StatThread, logmsg);
					break;
				}																					

				memset(&sb, 0, sizeof(stat_buffer_t));
				sb.lg_num = lgtemp;
				sb.dev_num = statrdev.st_rdev;
				sb.len = FTD_PS_DEVICE_ATTR_SIZE;
				sb.addr = (ftd_uint64ptr_t)(unsigned long)devbuffer;

				if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEV_STATE_BUFFER, &sb) < 0) {
					logoutx(4, F_StatThread, "ioctl FTD_GET_DEV_STATE_BUFFER error", "errno", errno);
					close(ctlfd);
					break;
				}

				memset(&devstat, -1, sizeof(disk_stats_t));
				memset(&sb, 0, sizeof(stat_buffer_t));
				sb.lg_num = lgtemp;
				sb.dev_num = statrdev.st_rdev;
				sb.len = sizeof(disk_stats_t);
				sb.addr = (ftd_uint64ptr_t)(unsigned long)&devstat;

				if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICE_STATS, &sb) < 0) {
					logoutx(4, F_StatThread, "ioctl FTD_GET_DEVICE_STATS error", "errno", errno);
					close(ctlfd);
					break;
				}
				close(ctlfd);

				/* stats from pmd are in statp */
				statp = (devstat_t *)devbuffer;
       			devdata->connection = lgdata->connection;
				setInstanceName(devdata->wcszInstanceName, lgtemp, devinfo.devname);
				devdata->role = ' ';
				devdata->devid = statp->devid;
				devdata->lgnum = lgtemp;
	
				/* calculate percent done */
                totalsects = (long long)devstat.localdisksize;
                devdata->sectors = devstat.wlsectors;

				/* As much as possible, the information on p00X.prf is used. */
				/* mark up FTD_MODE_CHECKPOINT_JLESS state */
				devdata->drvmode = (lgstat.state == FTD_MODE_CHECKPOINT_JLESS) ?
						FTD_MODE_NORMAL : lgstat.state;
				devdata->pctbab = 100 - ((int)devinfo.babused / 100);
				devdata->entries = devinfo.entries;
				devdata->pctdone = 
					((statp->rfshoffset * 1.0) / (totalsects * 1.0)) * 100.0;
				devdata->bytesread = (ftd_int64)devinfo.localread * 1024 / 100;
				devdata->byteswritten = (ftd_int64)devinfo.localwrite * 1024 / 100;

				lgdata->bytesread += devdata->bytesread;
				lgdata->byteswritten += devdata->byteswritten;

				devdata->actual = (ftd_int64)devinfo.actualkbps * 1024 / 100;
				devdata->effective = (ftd_int64)devinfo.effectkbps * 1024 / 100;

				lgdata->actual += devdata->actual;
				lgdata->effective += devdata->effective;

				// >>> LEGACY LOGIC: /* lg % done should be equal to the lowest device percentage */
				// >>> LEGACY LOGIC: if (devdata->pctdone < lgdata->pctdone) {
				// >>> LEGACY LOGIC: 	lgdata->pctdone = devdata->pctdone;
				// >>> LEGACY LOGIC: }
                // Update cumulative group's refreshed total data
                total_group_done_sectors += statp->rfshoffset;
                // Update the group's cumulative total disks size
                group_disks_size_in_sectors += (long long)devstat.localdisksize;
			}
            group_disks_size_in_bytes = group_disks_size_in_sectors * SECTORSIZE;
            total_group_done_bytes = total_group_done_sectors * SECTORSIZE;
            total_group_pending_bytes = group_disks_size_in_bytes - total_group_done_bytes;
            lgdata->pctdone = ((total_group_done_bytes * 1.0) / (group_disks_size_in_bytes * 1.0)) * 100.0;
            percent_not_done = 100 - lgdata->pctdone;
            
			setStartState(&grpstate[lgtemp], 1);
            // WI_338550 December 2017, implementing RPO / RTT
            if( enable_RPO_reporting )
            {
                calculate_group_pendingMBs_and_timeRemaining( &(lgdata->pendingMB), &(lgdata->timeRemaining), &lgstat, lgtemp, total_group_pending_bytes, lgdata->currentRtt, percent_not_done );
                // The following is to apply the same behaviour as TDMFIP Windows in Tracking
                if( (lgstat.state == FTD_MODE_TRACKING) && (lgdata->pendingMB == 0) )
                {
                        lgdata->currentRpo = 0;
                }
                if( debug_RPO && (stat("/tmp/debug_RPO", &statbuf) == 0))
                {
                    log_RPO_data( lgdata, lgtemp );
                }
            }
		}
		closedir (dfd);
		/* reduce malloc area */
		if (!error_occurred) ftd_mngt_performance_send_data(newpData, curNPerfData);
		/* send journal info */
		ftd_mngt_send_lg_state();

		en_time = time_so_far_usec();
#ifdef  ENABLE_RPO_DEBUGGING
        debug_RPO = 0;
        remove("/tmp/debug_RPO");
#endif
	}
	if(rmdlist != NULL) free(rmdlist);
	rmdlist = NULL;
	n_rmd = 0;

	if (cpfifo != -1) {
		close(cpfifo);
	}

	if(pPrevDeviceInstanceData)
	{
	    free(pPrevDeviceInstanceData);
	    pPrevDeviceInstanceData = NULL;
	}
	
	/* reduce malloc area */
	if (newpData) {
	    free(newpData);
	    newpData = NULL;
	}
}

void
ftd_mngt_performance_send_data(ftd_perf_instance_t *pDeviceInstanceData, int iNPerfData)
{
	mmp_mngt_TdmfPerfMsg_t		perfmsg;
	mmp_mngt_TdmfGroupStateMsg_t	statemsg;
	struct sockaddr_in		server;
#if !defined(FTD_IPV4)
	struct sockaddr_in6		server6;
#endif
	ipAddress_t				tmp_address;
	static int			ilSockID = 0;
	char				tmp[32];
	int				lgnum;
	int				towrite;
	int				r;
	int				level;
	int				*lgtemp;
	mmp_TdmfGroupState		state, gstate;
	ftd_perf_instance_t		*pWkConv, *pWkEnd;
	char				*senddata, *p;
	grpstate_t 			*it, *end;
	/* reduce malloc area */
	ftd_perf_instance_t		*pWkDeviceInstanceData = NULL;
    int i =0;
	/* better safe than sorry ... */
	if (iNPerfData > SHARED_MEMORY_ITEM_COUNT) {
		iNPerfData = SHARED_MEMORY_ITEM_COUNT;
	}
			
	/* reduce malloc area */
	if (iNPerfData > 0) {
		/*
		 * work area = pDeviceInstanceData.
		 * pDeviceInstanceData content MUST NOT modified!!!
		 */
		pWkDeviceInstanceData = pDeviceInstanceData;
	}

	/* position pWkEnd at end of valid working data */
	pWkEnd = pWkDeviceInstanceData + iNPerfData;

	memset(tmp, 0x00, sizeof(tmp));
    if ( cfg_get_software_key_value(AGENT_CFG_IP, tmp, sizeof(tmp)) == 0 )
    {
        ipstring_to_ip(tmp,&tmp_address);
    } 

	 cfg_get_software_key_value(AGENT_CFG_IP, tmp, sizeof(tmp));
	if(giTDMFCollectorIP.Version == IPVER_4)
	{	
		bzero((char *)&server, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = htons(giTDMFCollectorPort);
		server.sin_addr.s_addr = inet_addr(tmp);
	 }
	else
	{							   
#if !defined(FTD_IPV4)
		memset ((char *)&server6, 0, sizeof(server6));
		server6.sin6_family = AF_INET6;
		server6.sin6_port = htons(giTDMFCollectorPort);


		memcpy(&server6.sin6_addr.s6_addr,tmp_address.Addr.V6.Word, sizeof(tmp_address.Addr.V6.Word)); 
#endif
	}
	/* 
	 * get checkpoint state updates from a fifo shared with 
	 * ftd_mngt_StatusThread()
	 */
	{
		int		r;
		cp_fifo_t	c;
		do {
			r = read(cpfifo, &c, sizeof(c));
			if (r == sizeof(c)) {
				setCheckpointState(c.lgnum, c.on_off);
			}
		} while (r == sizeof(c));
		if (r == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
			logoutx(4, F_ftd_mngt_performance_send_data, "checkpoint read failed", "errno", errno);
		}
	}

	{
		int	bForceTxAllGrpState = gbForceTxAllGrpState;
		int	bGrpStateHdrSent = 0;
		it = grpstate;
		end = it + FTD_MAX_GROUPS;
		while (it != end) {
			int	bGroupStopped = 1;
       		/* send this group state to Collector, if required */
			if (hasGroupStateChanged(it) || bForceTxAllGrpState) {
				if (!bGrpStateHdrSent) { 
					
#if defined(FTD_IPV4)
					ilSockID = socket(AF_INET,SOCK_STREAM, 0);
#else
					ilSockID = socket((giTDMFCollectorIP.Version == IPVER_4) ? AF_INET : AF_INET6,SOCK_STREAM, 0);
#endif				
					
					if (ilSockID < 0) {
						logoutx(4, F_ftd_mngt_performance_send_data, "socket error", "errno", errno);
						return;
					}
				


					if(giTDMFCollectorIP.Version == IPVER_4)
					{
						r = connect_st(ilSockID, (struct sockaddr *)&server, sizeof(server), &level);
					}
					else
					{
#if !defined(FTD_IPV4)
						r = connect_st(ilSockID, (struct sockaddr *)&server6, sizeof(server6), &level);
#endif
					}
				
					if (r < 0) {
						logoutx(level, F_ftd_mngt_performance_send_data, "connect error", "errno", errno);
						close(ilSockID);
						return;
					} else if (r == 1) {
						logout(level, F_ftd_mngt_performance_send_data, "recovery from connect error.\n");
					}
   					/* init some message members only once */
					memset(&statemsg, 0x00, sizeof(mmp_mngt_TdmfGroupStateMsg_t));
					statemsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
					statemsg.hdr.mngttype       = MMP_MNGT_GROUP_STATE;
					statemsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
					statemsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
					/* convert to network byte order now so it is ready to be sent on socket */
					mmp_convert_mngt_hdr_hton(&statemsg.hdr);
					/* get host name */
					strcpy(statemsg.szServerUID, gszServerUID);

					/* write msg hdr  */
					towrite = sizeof(statemsg);
					r = write(ilSockID, (char *)&statemsg, towrite);
					if (r < 0) {
						logoutx(4, F_ftd_mngt_performance_send_data, "write error", "errno", errno);
						close(ilSockID);
						return;
					}
					if (r == towrite) {
						bGrpStateHdrSent = 1;
					}
				}
		
				getGroupState(it, &state);
				if(!(state.sRepGrpNbr < 0)){

					mmp_convert_TdmfGroupState_hton(&state);
					towrite = sizeof(state);
					r = write(ilSockID, (char *)&state, towrite);
					if (r < 0) {
						logoutx(4, F_ftd_mngt_performance_send_data, "write error", "errno", errno);
						close(ilSockID);
						return;
					}
				}
			}
			it++;
		}
		 
		if (bGrpStateHdrSent) {
			/* send the last one that acts as a end of TX flag */
			gstate.sRepGrpNbr = -1;
			gstate.sState = -1;
			mmp_convert_TdmfGroupState_hton(&gstate);
			towrite = sizeof(mmp_TdmfGroupState);
			r = write(ilSockID, (char *)&gstate, towrite);
			if (r < 0) {
				logoutx(4, F_ftd_mngt_performance_send_data, "write error", "errno", errno);
				close(ilSockID);
				return;
			}
		}
		close(ilSockID);

		sleep(1);

		gbForceTxAllGrpState = 0;
	}

	/*
	 * send performance data to TDMF Collector
	 */

	/* in NORMAL mode avoid to resend same perf msg. */
	/* reduce malloc area */
	if (prevNPerfData != 0 &&
	    prevNPerfData == iNPerfData &&
	    0 == memcmp(pDeviceInstanceData, pPrevDeviceInstanceData, 
	    iNPerfData * sizeof(ftd_perf_instance_t)) && !gbForceTxAllGrpPerf) {
		/* do not return if must send Grp Perf data */
		/*
		 * all data is identical to last transmission data,
		 * so no need to resend this again to Collector
 		 */
		/* This free is not really necessary since we are reusing the pointer */
		/*free(pDeviceInstanceData);
		pDeviceInstanceData = NULL;*/
		return;
	}

	if (pPrevDeviceInstanceData != NULL) {
		pPrevDeviceInstanceData = realloc(pPrevDeviceInstanceData, sizeof(ftd_perf_instance_t) * iNPerfData);
	}
	else {
		pPrevDeviceInstanceData = (ftd_perf_instance_t*)malloc(sizeof(ftd_perf_instance_t) * iNPerfData);
	}
	
	memcpy(pPrevDeviceInstanceData, pDeviceInstanceData, (sizeof(ftd_perf_instance_t)* iNPerfData));
	prevNPerfData = iNPerfData;

	if (pWkEnd != pWkDeviceInstanceData) {
		memset(&perfmsg, 0x00, sizeof(mmp_mngt_TdmfPerfMsg_t));
		/* init some message members only once */
		perfmsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
		perfmsg.hdr.mngttype       = MMP_MNGT_PERF_MSG;
		perfmsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
		perfmsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK;

		/* 
		 * convert to network byte order now so it is ready to 
		 * be sent on socket
		 */
		mmp_convert_mngt_hdr_hton(&perfmsg.hdr);

		/* get host name */
		strcpy(perfmsg.data.szServerUID, gszServerUID);

		/* transfer to TDMF collector */
		perfmsg.data.iPerfDataSize = htonl(iNPerfData * sizeof(ftd_perf_instance_t));

		/*
		 * convert to network byte order before sending on socket
		 * note : data within pWkDeviceInstanceData is modified by hton .
		 */
		/* convert all ftd_perf_instance_t values to network byte order */
		for (pWkConv = pWkDeviceInstanceData; pWkConv < pWkEnd; pWkConv++) {
    			mmp_convert_perf_instance_hton(pWkConv);
		}

		/*
		 * the following section of code is done ONLY ONCE in NORMAL 
		 * mode. more iterations are performed in EMULATOR mode .
		 */
		/* this section of code is done only once in NORMAL mode. */
	
	
#if defined(FTD_IPV4)
		ilSockID = socket(AF_INET, SOCK_STREAM, 0);
#else
		ilSockID = socket((giTDMFCollectorIP.Version == IPVER_4) ? AF_INET : AF_INET6, SOCK_STREAM, 0);
#endif
	
	
	
		if (ilSockID < 0) {
			logoutx(4, F_ftd_mngt_performance_send_data, "socket error", "errno", errno);
			return;
		}
	
	
		if(giTDMFCollectorIP.Version == IPVER_4)
		{
			r = connect_st(ilSockID, (struct sockaddr *)&server, sizeof(server), &level);
		}
		else
		{
#if !defined(FTD_IPV4)
			r = connect_st(ilSockID, (struct sockaddr *)&server6, sizeof(server6), &level);	
#endif
		}




		if (r < 0) {
			logoutx(level, F_ftd_mngt_performance_send_data, "connect error", "errno", errno);
			close(ilSockID);
			return;
		} else if (r == 1) {
			logout(level, F_ftd_mngt_performance_send_data, "recovery from connect error.\n");
		}

		towrite = sizeof(mmp_mngt_TdmfPerfMsg_t);
		r = write(ilSockID, (char *)&perfmsg, towrite);
		if (r == towrite) {
			/* send vector of ftd_perf_instance_t */
			towrite = iNPerfData * sizeof(ftd_perf_instance_t);
			write(ilSockID, (char *)pWkDeviceInstanceData, towrite);
			gbForceTxAllGrpPerf = 0;
		}
		close(ilSockID);
	}

}

#endif /* _FTD_MNGT_PERFORMANCE_SEND_DATA_C_ */
