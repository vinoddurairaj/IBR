// RplLibWrapper.cpp : Defines the entry point for the console application.
//
//#include <windows.h>
//#include <winioctl.h>


#include "stdafx.h"

extern "C" 
{
#include "ftd_mngt.h"
#include "ftd_stat.h"
#include "ftd_mngt_lg.h"
#include "ftd_mngt.h"
#include "ftd_mngt_key.h"
#include "ftd_error.h"
}

#include "RplLibWrapper.h"

extern "C" void ftd_mngt_msgs_log(const char **argv, int argc);
int  sftk_mngt_init_lg_monit(ftd_lg_t* lgp);
void sftk_mngt_do_lg_monit(ftd_lg_t *lgp);

//**************************
//
// Exported API Functions
//
//**************************

//
// DLLMain
//

BOOL WINAPI DllMain(HANDLE hInstance, ULONG Reason, LPVOID Reserved)
{
	switch(Reason){
		case DLL_PROCESS_ATTACH:
		{
			if (ftd_sock_startup() == -1) {
				return -1;
			}
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			ftd_sock_cleanup();
			break;
		}
	}
	return(TRUE);
}

extern "C" void RWAPI Mngt_Initialize( void )
{
	ftd_mngt_initialize();
}

/*
 * StatThread - primary statistics thread
 */
unsigned long RWAPI StatisticsThreadEntry( unsigned long * param )
{
	return StatThread ( param );
}

void RWAPI Init_Errfac(char *facility, char *procname, char *msgdbpath, char *logpath, int reportwhere, int logstderr)
{
	ftd_init_errfac( facility, procname, msgdbpath, logpath, reportwhere, logstderr );
}
void RWAPI Delete_Errfac()
{
	ftd_delete_errfac();
}

void RWAPI Set_Error_Trace_Level(char* level)
{
	error_SetTraceLevel( *level );
}

//
// Called to make sure we have read the collector values from the
// registry and know where to connect to.
//
void RWAPI Mngt_Initialize_Collector_Values(void)
{
	ftd_mngt_initialize_collector_values();
}

void RWAPI Mngt_Performance_Set_Group_Cp( short sGrpNumber, int bIsCheckpoint ) {
	ftd_mngt_performance_set_group_cp( sGrpNumber, bIsCheckpoint );
}

RWAPI ftd_mngt_lg_monit_t* Mngt_Create_Lg_Monit() {
	return ftd_mngt_create_lg_monit();
}

int RWAPI Mngt_Init_Lg_Monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath ) {
	return sftk_mngt_init_lg_monit( monitp, role, pstore, phostname, jrnpath );
}

void RWAPI Mngt_Do_Lg_Monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath, int lgnum, ftd_journal_t *jrnp ) {
	sftk_mngt_do_lg_monit( monitp, role, pstore, phostname, jrnpath, lgnum, jrnp );
}

int RWAPI Mngt_Delete_Lg_Monit( ftd_mngt_lg_monit_t* monitp ) {
	return ftd_mngt_delete_lg_monit( monitp );   
}

int RWAPI Mngt_Send_Agentinfo_Msg(int rip, int rport) {
	return ftd_mngt_send_agentinfo_msg( rip, rport );
}

void RWAPI Mngt_Send_Registration_Key() {
	ftd_mngt_send_registration_key();
}

void RWAPI Mngt_Shutdown() {
	ftd_mngt_shutdown();
}

void RWAPI Mngt_Msgs_Log( const char **argv, int argc ) {
	ftd_mngt_msgs_log( argv, argc);
}

RWAPI ftd_sock_t * Sock_Create( int type ) {
	return ftd_sock_create( type );
}

RWAPI int Mngt_Recv_Msg( ftd_sock_t *fsockp ) {
	return ftd_mngt_recv_msg( fsockp );
}

RWAPI bool IsRegistrationKeyValid( char* key ) {
	return isRegistrationKeyValid( key );
}

RWAPI HANDLE Performance_Get_Force_Aquire_Event() {
	return ftd_mngt_performance_get_force_acq_event();
}

RWAPI int Mngt_Key_Get_Licence_Key_Expiration_Date( char* key ) {
	return ftd_mngt_key_get_licence_key_expiration_date( key );
}
RWAPI int SharedMemoryObjectSize() {
	return SHARED_MEMORY_OBJECT_SIZE;
}


