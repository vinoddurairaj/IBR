#ifndef __RPL_LIB_WRAPPER_H__
#define __RPL_LIB_WRAPPER_H__

#define	RWAPI	__declspec(dllexport)


extern "C" {

void					RWAPI Mngt_Initialize( void );
unsigned long			RWAPI StatisticsThreadEntry( unsigned long * param );
void					RWAPI Init_Errfac( char *facility, char *procname, char *msgdbpath, char *logpath, int reportwhere, int logstderr );
void					RWAPI Delete_Errfac();
void					RWAPI Mngt_Initialize_Collector_Values(void);
void					RWAPI Mngt_Performance_Set_Group_Cp( short sGrpNumber, int bIsCheckpoint );
RWAPI ftd_mngt_lg_monit_t* Mngt_Create_Lg_Monit();
int						RWAPI Mngt_Init_Lg_Monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath );
void					RWAPI Mngt_Do_Lg_Monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath, int lgnum, ftd_journal_t *jrnp );
int						RWAPI Mngt_Delete_Lg_Monit( ftd_mngt_lg_monit_t* monitp );
int						RWAPI Mngt_Send_Agentinfo_Msg(int rip, int rport);
void					RWAPI Mngt_Send_Registration_Key();
void					RWAPI Mngt_Shutdown();
void					RWAPI Mngt_Msgs_Log( const char **argv, int argc);
RWAPI ftd_sock_t * Sock_Create( int type );
int						RWAPI Mngt_Recv_Msg( ftd_sock_t *fsockp );
void					RWAPI Set_Error_Trace_Level(char* level);

}


#endif