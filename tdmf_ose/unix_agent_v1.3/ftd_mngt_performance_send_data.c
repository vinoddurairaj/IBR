/* #ident "@(#)$Id: ftd_mngt_performance_send_data.c,v 1.50 2003/11/13 02:48:22 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_PERFORMANCE_SEND_DATA_C_
#define _FTD_MNGT_PERFORMANCE_SEND_DATA_C_

char	secondaryname[256];
int	secondaryport = SECONDARYPORT;
int n_rmd;
 
static ftd_perf_instance_t *pPrevDeviceInstanceData = 0;
static int gbForceTxAllGrpState;
static int gbForceTxAllGrpPerf;
/* reduce malloc area */
static int prevNPerfData = 0;

static int cpfifo = -1;

static char infoline[128 * 500];

grpstate_t	grpstate[FTD_MAX_GROUPS]; 
char pmdlist[FTD_MAX_GROUPS]; 
char localrmdlist[FTD_MAX_GROUPS]; 

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
	struct hostent		*hp = 0;
	struct sockaddr_in	servaddr;
	int			len;
	int			i, n, r;
	int			sock;
	char		CheckRmd[] = "ftd get all process info RMD_\n";
	char		*answer;
	char		*ptr;
	char		buff[8];
	rmd_data	*tmpdata;

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

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		logoutx(4, F_checkrmd, "socket error", "errno", errno);
		return 0;
	}
	memset ((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(secondaryport);

	if ((hp = gethostbyname(secondaryname)) == 0) {
		logoutx(4, F_checkrmd, "gethostbyname error", "errno", errno);
		close(sock);
		return 0;
	}

	/* set the TCP SNDBUF, RCVBUF to the maximum allowed */
	n = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(int));

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
		logoutx(12, F_checkrmd, "connect error", "errno", errno);
		close(sock);
		return 0;
	}
}

static int
getline(FILE *fd, LineState *ls)
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
		sprintf(logmsg, "%s is not exists.\n", filename);
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
		if (!getline(fp, &lstate)) {
			break;
		}
		if (strcmp("SYSTEM-TAG:", lstate.key) == 0 
			&& strcmp("PRIMARY", lstate.p2) == 0) {
			while(1) {
				if (!getline(fp, &lstate)) {
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
				if (!getline(fp, &lstate)) {
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

void
str_ftod(char *str)
{
	int     i;
	char    *p;

	p = strstr(str, ".");
	for (i = 0; i < 3; i++, p++) {
		*p = *(p + 1);
	}
	return;
}

int
getdevinfo(int lgnum, int devnum, devinfo_t *dip)
{
	int		r = 0;
	int		i,j;
	struct stat	statbuf;
	char 		*linebuf;
	char 		cmdline[256];
	char 		filename[MMP_MNGT_MAX_TDMF_PATH_SZ];
	char 		act[16], eft[16], done[16], used[16];
	char		ent[4], mode[4], lread[16], lwrite[16];
	FILE		*f;
	char		temp[16];
	int			retry_count;

	sprintf(filename, "%s/p%03d.prf", DTCVAROPTDIR, lgnum);
	if (stat(filename, &statbuf) < 0) {
		/* dtc is not started. */
		sprintf(logmsg, "%s is not exists.\n", filename);
		logout(4, F_getdevinfo, logmsg);
		return -1;
	}

	if(devnum == 0){
		/* get Device name, Acutual, and Effective */ 
		sprintf(cmdline, 
				"/usr/bin/tail -1 %s/p%03d.prf 2> /dev/null", DTCVAROPTDIR, lgnum);
		for (retry_count = 0; retry_count <= 1; retry_count ++) {
			if ((f = popen(cmdline, "r")) == NULL) {
				logoutx(4, F_getdevinfo, "popen failed", "errno", errno);
				return -1;
			}
			memset(infoline, 0, sizeof(infoline));
  
			if (fgets(infoline, 128 * 500, f) == NULL) {
				pclose(f);
				return -1;
			}
			pclose(f);
			if (strchr(infoline, '\n') != NULL) {
				break;
			}
			usleep(10000);
		}
		if (retry_count > 1) {
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
		str_ftod(act);
		str_ftod(eft);
		str_ftod(done);
		str_ftod(used);
		str_ftod(lread);
		str_ftod(lwrite);

		dip->actualkbps = atoi(act);
		dip->effectkbps = atoi(eft);
		dip->done = atoi(done);
		dip->babused = atoi(used);
		dip->localread = atoi(lread);
		dip->localwrite = atoi(lwrite);
		dip->mode = atoi(mode);
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
	ftd_perf_t		*perfp;
	int			pmd_connect, rmd_connect;
	int			i, lgtemp, ctlfd;
	long                    totalsects;
	ftd_perf_instance_t	*newpData;
	int			curNPerfData, newNPerfData;
	int stat_count = 0;
	DIR             *dfd;
	struct dirent *dent;
	struct stat cfgstat;
	int				interval = 0;
	char			tmp[32];
	double			st_time, en_time;
	long long		sleep_time;

	if ((perfp = (ftd_perf_t *)calloc(1, sizeof(ftd_perf_t))) == NULL) {
		logoutx(4, F_StatThread, "calloc error", "errno", errno);
    	}
    	perfp->magicvalue = FTDPERFMAGIC;

	memset(grpstate, 0, sizeof(grpstate_t) * FTD_MAX_GROUPS);
	for (i = 0; i < FTD_MAX_GROUPS; i++) {
		grpstate[i].m_state.sRepGrpNbr = -1;
	}

	/*
	 * get transmission interval of server information from Agent cfg file.
	 */
	interval = MMP_MNGT_DEFAULT_INTERVAL;
	if ( cfg_get_software_key_value("TransmitInterval", tmp, sizeof(tmp)) == 0 ){
		interval = atoi(tmp);
		if(interval <= 0 || interval > 3600){
			logout(7, F_StatThread, "\"TransmitInterval\" is illegal value. [1 - 3600]\n");
			interval = MMP_MNGT_DEFAULT_INTERVAL;
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
	sleep_time = 0;

	while (1) {
		sleep_time =  ((double)interval * 1000000.0) - (en_time - st_time);
		sprintf(logmsg, "process in sleep %ld micro sec.\n", (long)sleep_time);
		logout(17, F_StatThread, logmsg);
		if(sleep_time > 0){
			sleep((long)(sleep_time / 1000000));
			usleep((long)(sleep_time % 1000000));
		}
		st_time = time_so_far_usec();

		if(++stat_count == 2){
			ftd_mngt_performance_send_all_to_Collector();
			stat_count = 0;
		}

		perfp->pData = pPrevDeviceInstanceData;
		newpData = 0;
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
			sb.addr = (ftd_uint64ptr_t)&lgstat;
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
			lgdata->drvmode = lgstat.state;
			lgdata->lgnum = -100;
			lgdata->devid = lgtemp;
			lgdata->pctdone = 100;
			lgdata->pctbab = 100 - ((((double)lgstat.bab_used) * 100.0) /
				(((double)lgstat.bab_used) + ((double)lgstat.bab_free)));
			lgdata->entries = lgstat.wlentries;
			lgdata->sectors = lgstat.bab_used / SECTORSIZE;

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

			/* set device data */
			for (i = 0; i < lgstat.ndevs; i++) {
				devdata = pData++;

				memset(&devinfo, 0, sizeof(devinfo_t));
				if (getdevinfo(lgtemp, i, &devinfo) == -1) {
					break;
				}
				if (stat(devinfo.devname, &statrdev) == -1) {
					sprintf(logmsg, "%s is not exists.\n", devinfo.devname);
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
				sb.addr = (ftd_uint64ptr_t)devbuffer;

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
				sb.addr = (ftd_uint64ptr_t)&devstat;

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
				totalsects = (long)devstat.localdisksize;
				devdata->sectors = devstat.wlsectors;

				/* As much as possible, the information on p00X.prf is used. */
				devdata->drvmode = lgstat.state;
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

				/* lg % done should be equal to the lowest device percentage */
				if (devdata->pctdone < lgdata->pctdone) {
					lgdata->pctdone = devdata->pctdone;
				}
			}
			setStartState(&grpstate[lgtemp], 1);
		}
		closedir (dfd);
		/* reduce malloc area */
		ftd_mngt_performance_send_data(newpData, curNPerfData);
		/* send journal info */
		ftd_mngt_send_lg_state();

		en_time = time_so_far_usec();
	}
	if(rmdlist != NULL) free(rmdlist);
	rmdlist == NULL;
	n_rmd = 0;

	if (cpfifo != -1) {
		close(cpfifo);
	}

	free(pPrevDeviceInstanceData);
	pPrevDeviceInstanceData = 0;

	/* reduce malloc area */
	if (newpData) {
		free(newpData);
	}
	free(perfp);
}

void
ftd_mngt_performance_send_data(ftd_perf_instance_t *pDeviceInstanceData, int iNPerfData)
{
	mmp_mngt_TdmfPerfMsg_t		perfmsg;
	mmp_mngt_TdmfGroupStateMsg_t	statemsg;
	struct sockaddr_in		server;
	static int			ilSockID = 0;
	char				tmp[32];
	int				lgnum;
	int				lgstat = -1;
	int				towrite;
	int				r;
	int				level;
	int				*lgtemp;
	mmp_TdmfGroupState		state, gstate;
	ftd_perf_instance_t		*pWkConv, *pWkEnd;
	char				*senddata, *p;
	grpstate_t 			*it, *end;
	/* reduce malloc area */
	ftd_perf_instance_t		*pWkDeviceInstanceData;

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
	cfg_get_software_key_value("CollectorIP", tmp, sizeof(tmp));
	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(giTDMFCollectorPort);
	server.sin_addr.s_addr = inet_addr(tmp);

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
					ilSockID = socket(AF_INET, SOCK_STREAM, 0);
					if (ilSockID < 0) {
						logoutx(4, F_ftd_mngt_performance_send_data, "socket error", "errno", errno);
						return;
					}
					r = connect_st(ilSockID, (struct sockaddr *)&server, sizeof(server), &level);
					if (r < 0) {
						logoutx(level, F_ftd_mngt_performance_send_data, "connect error at send group state", "errno", errno);
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
		free(pDeviceInstanceData);
		return;
	}

	if (pPrevDeviceInstanceData != 0) {
		free(pPrevDeviceInstanceData);
	}
	pPrevDeviceInstanceData = pDeviceInstanceData;
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
		ilSockID = socket(AF_INET, SOCK_STREAM, 0);
		if (ilSockID < 0) {
			logoutx(4, F_ftd_mngt_performance_send_data, "socket error", "errno", errno);
			return;
		}
		r = connect_st(ilSockID, (struct sockaddr *)&server, sizeof(server), &level);
		if (r < 0) {
			logoutx(level, F_ftd_mngt_performance_send_data, "connect error at send perf msg", "errno", errno);
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
