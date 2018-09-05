/*
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#ifndef _IPC_H_
#define _IPC_H_

#include "common.h"

typedef struct ipcpack {
  int msgtype;
  int len;
  int cnt;
} ipcpack_t;

enum states { 
	FTDPMD = 1,
	FTDBFD,
	FTDRFD,
	FTDRFDF,
	FTDRFDC
};

#define FTDQHUP		100
#define FTDQPMD		110
#define FTDQBFD		120
#define FTDQRFD		130
#define FTDQRFDF	140
#define FTDQLG		150
#define FTDQLGLIST	160
#define FTDQSIG 	170
#define FTDQAPPLY 	180
#define FTDQACK 	190
#define FTDQRFDC    200	

#define FTDQCP      1000	
#define FTDQCPONP   1010
#define FTDQCPONS 	1020
#define FTDQCPOFFP 	1030
#define FTDQCPOFFS 	1040

/* function prototypes */
extern void pmd_action(int);
extern void pmdhup(int, char**);
extern void pmdrsync(int, char**);
extern int connecttomaster(char *, int);
extern int getftdsock(void);
extern int createftdsock(void);
extern void tell_master(int, int);
extern int ipcrecv_sig(int, int*);
extern int ipcrecv_lg(int);
extern int ipcrecv_lg_list(int);
extern int ipcsend_sig(int, int, int*);
extern int ipcsend_lg(int, int);
extern int ipcsend_lg_list(int);

#endif
