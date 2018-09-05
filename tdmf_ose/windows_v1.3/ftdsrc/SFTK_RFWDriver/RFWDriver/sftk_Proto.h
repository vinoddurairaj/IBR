/**************************************************************************************

Module Name: sftk_Proto.h   
Author Name: Parag sanghvi
Description: Define all Private Structures used in drivers
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_PROTO_H_
#define _SFTK_PROTO_H_

//
// Function prototype definations defined in sftk_device.c
// Windows Driver, Windows CTL Dand its relative APIS
//
NTSTATUS
sftk_init_driver(	IN PDRIVER_OBJECT DriverObject,
					IN PUNICODE_STRING RegistryPath );

NTSTATUS
sftk_Deinit_driver(	IN PDRIVER_OBJECT DriverObject );

NTSTATUS
sftk_create_dir(PWCHAR DirectoryNameStr, PVOID PtrReturnedObject);

NTSTATUS
sftk_del_dir(PWCHAR DirectoryNameStr, PVOID PtrObject);

NTSTATUS 
sftk_CreateCtlDevice(IN PDRIVER_OBJECT DriverObject);

PDEVICE_EXTENSION
sftk_find_attached_deviceExt( IN ftd_dev_info_t  *In_dev_info, BOOLEAN	bGrabLock );

VOID 
AddDevExt_SftkConfigList( IN OUT PDEVICE_EXTENSION pDevExt );

VOID 
RemoveDevExt_SftkConfigList( PDEVICE_EXTENSION pDevExt );

VOID 
sftk_reattach_devExt_to_SftkDev();

VOID 
sftk_remove_devExt_from_SftkDev( PDEVICE_EXTENSION pDevExt );

PDEVICE_EXTENSION
LookupVolNum_DevExt_SftkConfigList( IN ULONG   PartVolumeNumber );

PDEVICE_EXTENSION
LookupDiskPartNum_DevExt_SftkConfigList( IN ULONG  DiskDeviceNumber, IN ULONG  PartitionNumber );

NTSTATUS 
UpdateDevExtDeviceInfo( PDEVICE_EXTENSION pDevExt );

NTSTATUS	
sftk_GenerateSignatureGuid( PDEVICE_EXTENSION pDevExt );

NTSTATUS 
sftk_GetVolumeDiskExtents( PDEVICE_EXTENSION DevExt);

NTSTATUS
sftk_StartSourceListenThread(PSFTK_CONFIG Sftk_Config);

NTSTATUS
sftk_StartTargetListenThread(PSFTK_CONFIG Sftk_Config);

VOID
sftk_StopSourceListenThread(PSFTK_CONFIG Sftk_Config);

VOID
sftk_StopTargetListenThread(PSFTK_CONFIG Sftk_Config);

NTSTATUS
sftk_ctl_config_begin( PIRP Irp );

NTSTATUS
sftk_ctl_config_end( PIRP Irp );

BOOLEAN 
sftk_ExceptionFilter(	PEXCEPTION_POINTERS ExceptionInformation,
						ULONG ExceptionCode);

BOOLEAN 
sftk_ExceptionFilterDontStop(	PEXCEPTION_POINTERS ExceptionInformation,
								ULONG ExceptionCode);
VOID
sftk_init_perf_monitor();

VOID
sftk_perf_monitor( PERF_TYPE	Type, PLARGE_INTEGER	StartTime, ULONG WorkLoad);

NTSTATUS
sftk_ctl_get_perf_info( PIRP Irp );

NTSTATUS
sftk_ctl_get_all_attached_diskInfo( PIRP Irp );

#if	MM_TEST_WINDOWS_SLAB
NTSTATUS
sftk_ctl_get_MM_Alloc_Info( PIRP Irp );
#endif


//
// Function prototype definations defined in sftk_ioctl.c
//
NTSTATUS
sftk_ctl_ioctl(dev_t dev, int cmd, int arg, int flag, PIRP	Irp);

NTSTATUS
sftk_lg_ioctl(dev_t dev, int cmd, int arg, int flag);

//
// Function prototype definations defined in sftk_lg.c
//
NTSTATUS
sftk_ctl_new_lg( PIRP Irp );

NTSTATUS
sftk_ctl_del_lg( dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_delete_lg( IN OUT	PSFTK_LG Sftk_Lg, BOOLEAN bGrabGlobalLock );

NTSTATUS 
sftk_Create_InitializeLGDevice(	IN		PDRIVER_OBJECT	DriverObject, 
								IN		ftd_lg_info_t	*LG_Info, 
								IN OUT	PSFTK_LG		*Sftk_Lg, 
								IN		BOOLEAN			CreatePstoreFile,
								IN		BOOLEAN			CreateRegistry);

#if TARGET_SIDE
PSFTK_LG
sftk_lookup_lg_by_lgnum(IN ULONG LgNumber, ROLE_TYPE RoleType);
#else
PSFTK_LG
sftk_lookup_lg_by_lgnum(IN ULONG LgNumber);
#endif

NTSTATUS
sftk_lg_open( PSFTK_LG pSftk_LG );

NTSTATUS
sftk_lg_close( PSFTK_LG Sftk_LG );

NTSTATUS
sftk_lg_create_named_event( PSFTK_LG Sftk_LG );

NTSTATUS
sftk_lg_close_named_event( PSFTK_LG Sftk_LG, BOOLEAN bGrabLgLock);

NTSTATUS
sftk_lg_change_State( PSFTK_LG	Sftk_Lg, LONG  PrevState, LONG  NewState, BOOLEAN	UserRequested );

VOID
sftk_get_lg_state_string(LONG  LgState, PWCHAR Wstring);

NTSTATUS
sftk_lg_Verify_State_change_conditions( PSFTK_LG	Sftk_Lg, LONG  PrevState, LONG  NewState);	// used only for debugging purpose

BOOLEAN
sftk_match_lg_dev_lastBitIndex( PSFTK_LG	Sftk_Lg, ULONG RefreshLastBitIndex	);

NTSTATUS
sftk_update_lg_dev_lastBitIndex( PSFTK_LG	Sftk_Lg, ULONG RefreshLastBitIndex	);

BOOLEAN
sftk_lg_State_is_not_refresh( PSFTK_LG	Sftk_Lg);

LONG
sftk_lg_get_state( PSFTK_LG	Sftk_Lg);

BOOLEAN
sftk_lg_get_refresh_thread_state( PSFTK_LG	Sftk_Lg);

NTSTATUS
sftk_ctl_lg_Get_All_StatsInfo( PIRP Irp);

NTSTATUS
sftk_ctl_lg_Get_StatsInfo( PIRP Irp);

NTSTATUS
sftk_ctl_lg_Get_Lg_TuningParam( PIRP Irp);

NTSTATUS
sftk_ctl_lg_Set_Lg_TuningParam( PIRP Irp);

NTSTATUS
sftk_ctl_get_lg_state_buffer(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_set_lg_state_buffer(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_get_num_devices(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_start(dev_t dev, int cmd, int arg, int flag);	// TODO : What to do here ??

NTSTATUS
sftk_ctl_lg_get_num_groups(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_get_devices_info(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_get_groups_info(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_get_device_stats(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_get_group_stats(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_clear_dirtybits(int arg, int type);

NTSTATUS
sftk_ctl_lg_group_state(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_set_iodelay(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_set_sync_depth(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_set_sync_timeout(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_lg_clear_bab(dev_t dev, int cmd, int arg, int flag);	// TODO 

NTSTATUS
sftk_ctl_get_bab_size(dev_t dev, int cmd, int arg, int flag); // TODO

NTSTATUS
sftk_ctl_lg_set_group_state(dev_t dev, int cmd, int arg, int flag);	

#if TARGET_SIDE

NTSTATUS
sftk_lg_failover( PIRP Irp);

NTSTATUS
sftk_lg_ctl_fullrefresh_pause_resume( PIRP Irp);

NTSTATUS
sftk_lg_fullrefresh_pause_resume( PSFTK_LG	Sftk_Lg , EXECUTION_STATE eExecState);

#endif


//
//	BOOLEAN	sftk_lg_is_connection_alive( LONG _state_, WCHAR *_wcharStr_ );
//
//	Returns State Wchar string from supplied state value
#define sftk_get_stateString(_state_, _wcharStr_)			{												\
			switch(_state_)		{																			\
				case SFTK_MODE_PASSTHRU:		swprintf(_wcharStr_, L"SFTK_MODE_PASSTHRU"); break;			\
				case SFTK_MODE_TRACKING:		swprintf(_wcharStr_, L"SFTK_MODE_TRACKING"); break;			\
				case SFTK_MODE_NORMAL:			swprintf(_wcharStr_, L"SFTK_MODE_NORMAL");	 break;			\
				case SFTK_MODE_FULL_REFRESH:	swprintf(_wcharStr_, L"SFTK_MODE_FULL_REFRESH"); break;		\
				case SFTK_MODE_SMART_REFRESH:	swprintf(_wcharStr_, L"SFTK_MODE_SMART_REFRESH"); break;	\
				case SFTK_MODE_BACKFRESH:		swprintf(_wcharStr_, L"SFTK_MODE_BACKFRESH"); break;		\
				default:						swprintf(_wcharStr_, L"0x%08x",_state_); break;				\
			}																								\
	}

//
//	BOOLEAN	sftk_lg_is_connection_alive( PSFTK_LG _Sftk_Lg_ );
//
//	Returns TRUE is Any one Socket Connection is alive for current SFTK_LG else returns FALSE
#define sftk_lg_is_socket_alive( _Sftk_Lg_)		( ((PSFTK_LG) (_Sftk_Lg_))->SessionMgr.nLiveSessions > 0)

//	Returns TRUE is Session manager has done Handshake successfully, else return FALSE
#define sftk_lg_is_sessionmgr_Handshack_done( _Sftk_Lg_)		( ((PSFTK_LG) (_Sftk_Lg_))->SessionMgr.bSendHandshakeInformation == FALSE)

#if TARGET_SIDE
//
//  BOOLEAN	
//	LG_IS_PRIMARY_MODE(	PSFTK_LG _pSftk_LG_ );
//	
//	Returns TRUE if current mode of LG is in Primary mode else FALSE 
//
#define LG_IS_PRIMARY_MODE(_pSftk_LG_)		( ((PSFTK_LG) (_pSftk_LG_))->Role.CurrentRole == PRIMARY)

//
//  BOOLEAN	
//	LG_IS_SECONDARY_MODE(	PSFTK_LG _pSftk_LG_ );
//	
//	Returns TRUE if current mode of LG is in Secondary mode else FALSE 
//
#define LG_IS_SECONDARY_MODE(_pSftk_LG_)		( ((PSFTK_LG) (_pSftk_LG_))->Role.CurrentRole == SECONDARY)

//
//  BOOLEAN	
//	LG_IS_SECONDARY_WITH_TRACKING(	PSFTK_LG _pSftk_LG_ );
//	
//	Returns TRUE if LG current mode is Secondary with Tracking Enabled. (during Failover case)
//
#define LG_IS_SECONDARY_WITH_TRACKING(_pSftk_LG_)		( LG_IS_SECONDARY_MODE(_pSftk_LG_) &&  \
												( ((PSFTK_LG) (_pSftk_LG_))->Role.FailOver == TRUE) )

//
//  BOOLEAN	
//	LG_IS_JOURNAL_ON(	PSFTK_LG _pSftk_LG_ );
//	
//	Returns TRUE if LG Is marked Journal ON else FALSE. (This API must call by Secondary Side code only...)
//
#define LG_IS_JOURNAL_ON(_pSftk_LG_)		( ((PSFTK_LG) (_pSftk_LG_))->Role.JEnable == TRUE) 

//
//  BOOLEAN	
//	LG_IS_APPLY_JOURNAL_ON(	PSFTK_LG _pSftk_LG_ );
//	
//	Returns TRUE if LG Is running Apply Journal Process else FALSE. (This API must call by Secondary Side code only...)
//
#define LG_IS_APPLY_JOURNAL_ON(_pSftk_LG_)		( ((PSFTK_LG) (_pSftk_LG_))->Role.JApplyRunning == TRUE) 
#endif

// Cache Manager Call back API
ULONG
sftk_GetLGState( PSFTK_LG	Sftk_LG);

//
// Function prototype definations defined in sftk_dev.c
//
NTSTATUS
sftk_ctl_new_device(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_dev_validation_with_configFile( PSFTK_DEV Sftk_Dev, ftd_dev_info_t *Dev_Info);

NTSTATUS
sftk_ctl_del_device(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_delete_SftekDev( IN OUT 	PSFTK_DEV		SftkDev, BOOLEAN bGrabLGLock, BOOLEAN bGrabGlobalLock );

PSFTK_DEV
sftk_lookup_dev_by_cdev(IN ULONG Cdev);

PSFTK_DEV
sftk_lookup_dev_by_cdev_in_SftkLG(IN PSFTK_LG Sftk_LG, IN ULONG Cdev);

PSFTK_DEV
sftk_lookup_dev_by_devid(IN PCHAR DevIdString);

NTSTATUS
sftk_CreateInitialize_SftekDev( IN		ftd_dev_info_t  *In_dev_info, 
								IN OUT 	PSFTK_DEV		*Sftk_Dev,
								IN		BOOLEAN			UpdatePstoreFile,
								IN		BOOLEAN			UpdateRegKey,
								IN		BOOLEAN			IgnoreDevicePresence);

NTSTATUS
sftk_ctl_get_dev_state_buffer(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_set_dev_state_buffer(dev_t dev, int cmd, int arg, int flag);

NTSTATUS
sftk_ctl_get_device_nums(dev_t dev, int cmd, int arg, int flag);

//
// Function prototype definations defined in sftk_driver.c
//

NTSTATUS
sftk_io_generic_completion(
    IN PDEVICE_OBJECT       PtrDeviceObject,
    IN PIRP                 Irp,
    IN PSFTK_DEV			Sftk_Dev);

// Logging Event Message API
#define MAX_INSERTION_STRING_ALLOWED	102	// 2 bytes for null termination

VOID
TruncateUnicodeString(	PUNICODE_STRING UniSource, 
						PUNICODE_STRING UniDestination, 
						USHORT			MaxSizeOfDestLength,
						OPTIONAL PCHAR	PreCharStringtoTruncate );

NTSTATUS    DrvLogEvent( IN  PVOID    DriverOrDeviceObject,
                         IN  ULONG    LogMessageCode,
                         IN  USHORT   EventCategory,
                         IN  UCHAR    MajorFunction,
                         IN  ULONG    IoControlCode,
                         IN  UCHAR    RetryCount,
                         IN  NTSTATUS FinalStatus,
                         IN  PVOID    DumpData,
                         IN  USHORT   DumpDataSize,
                         IN  LPWSTR   WszInsertionString, // Strings (if any) go here...
                         ... );
#if DBG
VOID
DbgDisplayIoctlMessage(PIO_STACK_LOCATION pCurrentStackLocation);
#endif

// ******************************
// Macro Wrappers for DriverUtils
// Event logging functions 
// ******************************

// sftk_LogEvent( pDriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0);
#define sftk_LogEvent( _pvObject_, _ulErrorCode_, _status_, _usCategory_ )                \
                DrvLogEvent( (_pvObject_), (ULONG)(_ulErrorCode_), (USHORT)(_usCategory_),  \
                                (UCHAR)0, (UCHAR)0, (ULONG)0, (NTSTATUS)_status_, NULL, 0, NULL )

// sftk_LogEventCharStr1Num1( pDriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0, "Hi, 10);
#define sftk_LogEventCharStr1Num1( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _CharString1_, _Num1_)		\
        {																									\
			WCHAR _wszString1_[128], _wszString2_[64];															\
			swprintf( _wszString1_, L"%s", (PCHAR) (_CharString1_) );											\
			swprintf( _wszString2_, L"%d",	(ULONG)(_Num1_));												\
			DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),												\
                         (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                         _wszString1_, _wszString2_, NULL );													\
        }

// sftk_LogEventNum1Wchar1( pDriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0, 10, L"Hi");
#define sftk_LogEventNum1Wchar1( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _Num1_,_Wchar1_)		\
        {																									\
			WCHAR _wszString1_[64];															\
			swprintf( _wszString1_, L"%d", (PCHAR) (_Num1_) );											\
			DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),												\
                         (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                         _wszString1_, _Wchar1_, NULL );													\
        }

// sftk_LogEventNum2Wchar1( pDriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0, 10, 10, L"Hi");
#define sftk_LogEventNum2Wchar1( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _Num1_,_Num2_,_Wchar1_)		\
        {																									\
			WCHAR _wszString1_[64], _wszString2_[64];															\
			swprintf( _wszString1_, L"%d", (_Num1_) );											\
			swprintf( _wszString2_, L"%d", (_Num2_) );											\
			DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),												\
                         (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                         _wszString1_, _wszString2_, _Wchar1_, NULL );													\
        }

// sftk_LogEventNum2Wchar2( pDriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0, 10, 10, L"Hi",L"Bye");
#define sftk_LogEventNum2Wchar2( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _Num1_,_Num2_,_Wchar1_, _Wchar2_)		\
        {																									\
			WCHAR _wszString1_[64], _wszString2_[64];															\
			swprintf( _wszString1_, L"%d", (PCHAR) (_Num1_) );											\
			swprintf( _wszString2_, L"%d", (PCHAR) (_Num2_) );											\
			DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),												\
                         (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                         _wszString1_, _wszString2_, _Wchar1_, _Wchar2_, NULL );													\
        }


// sftk_LogEventChar1Num2( pDriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, status, 0, __FILE__, __LINE__, GetExceptionCode())
#define sftk_LogEventChar1Num2( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _CharString1_,_Num1_,_Num2_)		\
        {																									\
			WCHAR _wszString1_[80], _wszString2_[80], _wszString3_[80];											\
			swprintf( _wszString1_, L"%s", (PCHAR) (_CharString1_) );											\
			swprintf( _wszString2_, L"%d", (PCHAR) (_Num1_) );												\
			swprintf( _wszString3_, L"%d", (PCHAR) (_Num2_) );												\
			DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),												\
                         (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                         _wszString1_, _wszString2_, _wszString3_, NULL );													\
        }

// sftk_LogNum3CharStr2( pDriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0, 10, 20, 40, "Hi, "Bye");
#define sftk_LogEventNum3CharStr2( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _Num1_, _Num2_, _Num3_, _CharString1_, _CharString2_) \
        {																									\
			WCHAR _wszString1_[128],_wszString2_[128],_wszString3_[128];											\
            swprintf(  _wszString1_, L"%d",	(ULONG)(_Num1_));												\
            swprintf(  _wszString2_, L"0x%08x",	(ULONG)(_Num2_));											\
            swprintf( _wszString3_, L"0x%08x",	(ULONG)(_Num3_));											\
			if (_CharString1_)																				\
			{																								\
				WCHAR _wszString4_[128];																		\
				swprintf( _wszString4_, L"%s", (PCHAR) (_CharString1_) );										\
				if (_CharString2_)																			\
				{																							\
					WCHAR _wszString5_[128];																	\
					swprintf( _wszString5_, "%s", (PCHAR) (_CharString2_) );									\
					DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),										\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                            _wszString1_, _wszString2_, _wszString3_, _wszString4_, _wszString5_, NULL );				\
				}																							\
				else																						\
				{																							\
					DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),											\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                            _wszString1_, _wszString2_, _wszString3_, _wszString4_, NULL );							\
				}																							\
			}																								\
			else																							\
			{																								\
				DrvLogEvent( _pvObject_, (ULONG)(_ulErrorCode_),												\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0, (NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,  \
                            _wszString1_, _wszString2_, _wszString3_, NULL );										\
			}																								\
        }

#define sftk_LogEventNoString( _pvObject_, _ulErrorCode_, _status_) \
        {																			\
            DrvLogEvent(	_pvObject_, (ULONG)(_ulErrorCode_),						\
                            (USHORT)(0), (UCHAR)0, (ULONG)0, (UCHAR)0,				\
							(NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,				\
                            NULL); \
        }

//
// sftk_LogEventString2( gp_DriverObject, MSG_REPL_PSTORE_READ_ERROR, STATUS_SUCCESS, 0, wString1);
//
#define sftk_LogEventString1( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _wsString1_) \
        {																			\
            DrvLogEvent(	_pvObject_, (ULONG)(_ulErrorCode_),						\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0,	\
							(NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,				\
                            _wsString1_, NULL); \
        }

//
// sftk_LogEventString2( gp_DriverObject, MSG_REPL_PSTORE_READ_ERROR, STATUS_SUCCESS, 0, wString1, wString2);
//
#define sftk_LogEventString2( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _wsString1_, _wsString2_) \
        {																			\
            DrvLogEvent(	_pvObject_, (ULONG)(_ulErrorCode_),						\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0,	\
							(NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,				\
                            _wsString1_, _wsString2_, NULL); \
        }

//
// sftk_LogEventString2( gp_DriverObject, MSG_REPL_PSTORE_READ_ERROR, STATUS_SUCCESS, 0, _wsString1_, _wsString2_, _wsString3_);
//
#define sftk_LogEventString3( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _wsString1_, _wsString2_, _wsString3_) \
        {																			\
            DrvLogEvent(	_pvObject_, (ULONG)(_ulErrorCode_),						\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0,	\
							(NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,				\
                            _wsString1_, _wsString2_, _wsString3_, NULL);			\
        }

//
// sftk_LogEventWchar4( gp_DriverObject, MSG_REPL_PSTORE_READ_ERROR, STATUS_SUCCESS, 0, _wsString1_, _wsString2_, _wsString3_);
//
#define sftk_LogEventWchar4( _pvObject_, _ulErrorCode_, _status_,_usCategory_, _wsString1_, _wsString2_, _wsString3_, _wsString4_) \
        {																			\
            DrvLogEvent(	_pvObject_, (ULONG)(_ulErrorCode_),						\
                            (USHORT)(_usCategory_), (UCHAR)0, (ULONG)0, (UCHAR)0,	\
							(NTSTATUS)_status_, (PVOID)NULL, (USHORT)0,				\
                            _wsString1_, _wsString2_, _wsString3_, _wsString4_, NULL);			\
        }


//
// Function prototype definations defined in sftk_Bitmap.c
//

NTSTATUS
sftk_Create_DevBitmaps( IN OUT 	PSFTK_DEV		Sftk_Dev, 
					    IN		ftd_dev_info_t  *In_dev_info );

NTSTATUS
sftk_Init_Bitmaps(	IN OUT 	PSFTK_BITMAP	Bitmap, 
					IN		PULONG			DiskSize,		// in sectors, Total Disk size = 2 Tera Bytes size limitation
					IN		PULONG			BitmapMemSize);	// total size of memory in bytes used to allocate Bitmap

NTSTATUS
sftk_DeInit_Bitmaps(IN OUT 	PSFTK_BITMAP	Bitmap);

NTSTATUS
sftk_Delete_DevBitmaps( IN OUT 	PSFTK_DEV	Sftk_Dev );

NTSTATUS
sftk_bit_range_from_offset( IN		PSFTK_BITMAP Bitmap, 
							IN		UINT64		 Offset,	// In bytes
							IN		ULONG		 Size,		// In bytes
							IN	OUT	PULONG		 StartBit,  // Returninus Starting Bit
							IN	OUT	PULONG		 EndBit);    // Ending Bit, include this bit number too.

NTSTATUS
sftk_offset_from_bit_range( IN		PSFTK_BITMAP Bitmap, 
							IN	OUT	PUINT64		 Offset,	// In bytes
							IN	OUT	PULONG		 Size,		// In bytes
							IN		ULONG		 StartBit,  // Returninus Starting Bit
							IN		ULONG		 EndBit);   // Ending Bit, include this bit number number too.

BOOLEAN		
sftk_Update_bitmap(		IN		PSFTK_DEV	 Sftk_Dev, 
						IN		UINT64		 Offset,	// In bytes
						IN		ULONG		 Size);		// In bytes    

LONG
sftk_get_msb(LONG num);

NTSTATUS
sftk_Prepare_bitmapA_to_bitmapB(IN		PSFTK_BITMAP	DestBitmap, 
								IN		PSFTK_BITMAP	SrcBitmap,
								IN		BOOLEAN			CleanDestBitmapToStart);
BOOLEAN
sftk_Check_bits_to_update(	IN		PSFTK_DEV	Sftk_Dev, 
							IN		BOOLEAN		LrdbCheck,	// TRUE means Lrdb else HRDB check
							IN		PULONG		StartBit, 
							IN		PULONG		EndBit);
//
// Function prototype definations defined in sftk_Registry.c
//
NTSTATUS 
sftk_IsRegKeyPresent(	CONST	PWCHAR	KeyPathName,		// like L"sftk_block\\Parameters"
						CONST	PWCHAR	KeyValueName);		// like L"LGNumber"

NTSTATUS 
sftk_CheckRegKeyPresent( CONST PWCHAR KeyPathName);	// L"sftk_block\\Parameters\\LGNum%08d"	// its Key

NTSTATUS 
sftk_CreateRegistryKey(	CONST PWCHAR KeyPathName );

NTSTATUS 
sftk_RegistryKey_DeleteValue(	CONST	PWCHAR	KeyPathName,		// like L"sftk_block\\Parameters"
								CONST	PWCHAR	KeyValueName);		// like L"LGNumber"

NTSTATUS 
sftk_RegistryKey_DeleteKey(	CONST PWCHAR KeyPathName);	// L"sftk_block\\Parameters\\LGNum%08d"	// its Key


NTSTATUS 
sftk_GetRegKey( CONST	PWCHAR	KeyPathName,		// L"sftk_block\\Parameters\\LGNum%08d"	// its Key
				CONST	PWCHAR	KeyValueName,		// like L"LGNumber"
						ULONG	ValueType,			// REG_DWORD, REG_SZ, etc..
						PVOID	ValueData,			// Actual Data
						ULONG	ValueLength);		// sizeof(ULONG), etc..	Data size

NTSTATUS 
QueryRegistryValue(	PWSTR	ValueName, 
					ULONG	ValueType,
					PVOID	ValueData,
					ULONG	ValueLength,
					PVOID	Context,
					PVOID	EntryContext);

NTSTATUS 
sftk_SetRegKey( CONST	PWCHAR	KeyPathName,		// like L"sftk_block\\Parameters\\LGNumber"
				CONST	PWCHAR	KeyValueName,		// like L"LGNumber"		
						ULONG	ValueType,			// REG_DWORD, REG_SZ, etc..
						PVOID	ValueData,			// Actual Data
						ULONG	ValueLength);		// sizeof(ULONG), etc..	Data size

NTSTATUS
sftk_lg_Create_RegKey(PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_lg_Delete_RegKey(PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_lg_update_lastshutdownKey(PSFTK_LG Sftk_Lg, ULONG LastShutDown);

NTSTATUS
sftk_lg_update_PstoreFileNameKey(PSFTK_LG Sftk_Lg, PWCHAR PstoreFileName);

NTSTATUS
sftk_lg_update_RegRoleAndSecondaryInfo(PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_dev_Create_RegKey(PSFTK_DEV Sftk_Dev);

NTSTATUS
sftk_dev_Delete_RegKey(PSFTK_DEV Sftk_Dev);

NTSTATUS
sftk_configured_from_registry();

NTSTATUS
sftk_Read_Registry_For_Dev( ftd_dev_info_t  *Dev_info, ROLE_TYPE CreationRole, ULONG LgNum, ULONG DevNum, PBOOLEAN	ValidDevNum);

//
// Function prototype definations defined in sftk_Thread.c
//
NTSTATUS
sftk_Create_DevThread( IN OUT 	PSFTK_DEV	Sftk_Dev, 
					   IN		ftd_dev_info_t  *In_dev_info );

NTSTATUS
sftk_Terminate_DevThread( IN OUT 	PSFTK_DEV	Sftk_Dev );

NTSTATUS
sftk_Create_LGThread(	IN OUT 	PSFTK_LG		Sftk_Lg, 
						IN		ftd_lg_info_t	*LG_Info );

NTSTATUS
sftk_Terminate_LGThread( IN OUT PSFTK_LG	Sftk_Lg);

VOID
sftk_dev_master_thread( PSFTK_DEV	Sftk_Dev);

NTSTATUS
sftk_dev_alloc_mem_for_new_writes(	PSFTK_DEV			Sftk_Dev, 
									PIO_STACK_LOCATION	IrpStack,
									PIRP				Irp);

NTSTATUS
sftk_write_completion(	IN PDEVICE_OBJECT       PtrDeviceObject,
						IN PIRP                 Irp,
						IN PVOID				Context);


VOID
sftk_acknowledge_lg_thread( PSFTK_LG	Sftk_LG);

NTSTATUS
sftk_ack_prepare_bitmaps( PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_ack_update_bitmap(PSFTK_DEV Sftk_Dev, LONGLONG Offset, ULONG Length);

#if TARGET_SIDE
VOID
sftk_Target_write_Thread( PSFTK_LG	Sftk_LG);

NTSTATUS
sftk_secondary_Recved_Pkt_Process( PSFTK_LG Sftk_LG, PMM_PROTO_HDR pMM_Proto_Hdr);

VOID
sftk_JApply_Thread( PSFTK_LG	Sftk_LG);
#endif

//
// Function prototype definations defined in sftk_Refresh.c
//

VOID
sftk_refresh_lg_thread( PSFTK_LG	Sftk_LG);

BOOLEAN
sftk_get_next_refresh_dev_offset(	PSFTK_DEV		Sftk_Dev, 
									BOOLEAN			FullRefresh, 
									PLARGE_INTEGER	ByteOffset, 
									PULONG			ReadSize,
									PLARGE_INTEGER	EndOffset);

NTSTATUS
sftk_CC_refresh_callback( PVOID Buffer, PVOID Context );

NTSTATUS
sftk_async_read_data(	PSFTK_DEV		Sftk_Dev, 
						PLARGE_INTEGER	ByteOffset, 
						PULONG			NumBytes, 
						PVOID			Buffer, 
						PVOID			Context);

NTSTATUS	
sftk_async_read_Completion(	IN PDEVICE_OBJECT	DeviceObject, 
							IN PIRP				Irp, 
							IN PVOID			Context );

//
// Function prototype definations defined in sftk_Pstore.c
//

NTSTATUS
sftk_is_file_exist( PUNICODE_STRING   FileName);	// To Check PSTORE File Existence

NTSTATUS
sftk_delete_pstorefile( IN PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_open_pstore( IN PUNICODE_STRING  FileName, OUT PHANDLE PtrFileHandle, BOOLEAN bCreate);

NTSTATUS
sftk_write_pstore(	HANDLE			FileHandle, 
					PLARGE_INTEGER	Offset,
					PVOID			Buffer, 
					ULONG			Size );

NTSTATUS
sftk_read_pstore(	HANDLE			FileHandle, 
					PLARGE_INTEGER	Offset,
					PVOID			Buffer, 
					ULONG			Size );


NTSTATUS
sftk_get_checksum( PUCHAR Buffer, ULONG Size, PLARGE_INTEGER	Checksum);

NTSTATUS
sftk_do_validation_PS_HDR( IN PSFTK_PS_HDR	pPsHdr, IN ULONG Size);

NTSTATUS
sftk_do_validation_PS_DEV( IN PSFTK_PS_DEV	PsDev, IN ULONG Size);

#if PS_BITMAP_CHECKSUM_ON
NTSTATUS
sftk_do_checksum_validation_of_PS_Bitmap( IN PSFTK_PS_BITMAP	PsBitmap, IN PUCHAR BitmapMem,  IN ULONG BitmapSize);
#endif

NTSTATUS
sftk_flush_psHdr_to_pstore(	IN			PSFTK_LG	Sftk_Lg, 
							IN			BOOLEAN		HandleValid,  // TRUE means FileHandle is valid used it
							IN OPTIONAL HANDLE		FileHandle,	  
							IN OPTIONAL BOOLEAN		UseCheckSum); // TRUE means cal. Checksum and write it to pstore file

NTSTATUS
sftk_flush_psDev_to_pstore(	IN			PSFTK_DEV	Sftk_Dev, 
							IN			BOOLEAN		HandleValid,  // TRUE means FileHandle is valid used it
							IN OPTIONAL HANDLE		FileHandle,	  
							IN OPTIONAL BOOLEAN		UseCheckSum); // TRUE means cal. Checksum and write it to pstore file

NTSTATUS
sftk_flush_all_bitmaps_to_pstore(	IN			PSFTK_DEV	Sftk_Dev, 
									IN			BOOLEAN		HandleValid,	// TRUE means FileHandle is valid used it
									IN OPTIONAL HANDLE		FileHandle,	  
									IN OPTIONAL BOOLEAN		UseCheckSum,	// TRUE means cal. Checksum and write it to pstore file
									IN OPTIONAL BOOLEAN		UpdatePsDevWithChecksum); // TRUE means Flush Psdev with new checksum

NTSTATUS
sftk_init_Lg_and_devs_from_Pstore(	IN PUNICODE_STRING	FileName,
									IN HANDLE			FileHandle ); // This API Not used anymore

NTSTATUS
sftk_format_pstorefile( IN PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_create_dev_in_pstorefile( IN PSFTK_LG Sftk_Lg, IN	PSFTK_DEV Sftk_Dev );

NTSTATUS
sftk_Delete_dev_in_pstorefile( IN	PSFTK_DEV Sftk_Dev );

NTSTATUS
sftk_flush_all_pstore( IN BOOLEAN ShutDownOn, BOOLEAN bGrabLGLock, BOOLEAN bGrabGlobalLock );	// TRUE means Shutdown time

NTSTATUS
sftk_Update_LG_from_pstore( PSFTK_LG Sftk_Lg);

NTSTATUS
sftk_Update_Dev_from_pstore( PSFTK_LG Sftk_Lg, PSFTK_DEV	Sftk_Dev);

NTSTATUS
sftk_delete_psdev_entry_in_pstore(	PSFTK_PS_DEV	PsDev, HANDLE FileHandle, 
									PLARGE_INTEGER Offset, ULONG Size, BOOLEAN UseChecksum);



//
// Function prototype definations defined in sftk_cache.c
//

#if	MM_TEST_WINDOWS_SLAB

NTSTATUS	
MM_LookAsideList_Test();

NTSTATUS	
MM_Init_LookAsideList();

NTSTATUS	
MM_TestMaxMemAlloc_LookAsideList( );

NTSTATUS	
MM_AllocMaxRawMem_LookAsideList( PMM_ANCHOR MmAnchor);

NTSTATUS	
MM_FreeAllSlabRawMem_LookAsideList( PMM_ANCHOR MmAnchor);

PVOID
MM_AllocMem_From_LookAsideList(ULONG Type);

NTSTATUS	
MM_FreeMemToOS_LookAsideList( ULONG Type, PVOID	Buffer);

NTSTATUS	
MM_DeInit_LookAsideList();

VOID
DisplayLookAsideListInternals( PMM_ANCHOR MmAnchor);

VOID
DisplayMMAnchorInfo_LookAsideList( PMM_ANCHOR MmAnchor);

#endif // #if	MM_TEST_WINDOWS_SLAB

//
// Function prototype definations defined in sftk_MM.c
//

NTSTATUS
Sftk_Ctl_MM_Start_And_Get_DB_Size( PIRP Irp );

NTSTATUS
Sftk_Ctl_MM_Set_DB_Size( PIRP Irp );

NTSTATUS
Sftk_Ctl_MM_Init_Raw_Memory( PIRP Irp );

NTSTATUS
Sftk_Ctl_MM_Stop( PIRP Irp );

NTSTATUS
mm_start(IN OUT PMM_DATABASE_SIZE_ENTRIES	MMDbSizeEntries,
		 IN OUT PMM_MANAGER					Mmgr);

NTSTATUS
mm_stop( IN OUT PMM_MANAGER		Mmgr);

NTSTATUS
mm_deinit(IN OUT PMM_MANAGER	Mmgr, IN BOOLEAN TerminateSMThread);

NTSTATUS
mm_type_alloc_init(	IN	OUT PMM_MANAGER					Mmgr,
					IN		PSET_MM_DATABASE_MEMORY		MMDb);

NTSTATUS
mm_type_alloc_deinit_all(	IN	OUT PMM_MANAGER	Mmgr);

PVOID
mm_type_alloc( UCHAR	MM_Type);

VOID
mm_type_free( PVOID	Memory, UCHAR	MM_Type);


NTSTATUS
mm_type_pages_add(	IN	OUT PMM_MANAGER				Mmgr,
					IN		PSET_MM_RAW_MEMORY		MMRawMem);

NTSTATUS
mm_type_pages_remove(	IN	OUT PMM_MANAGER				Mmgr,
						IN		PSET_MM_RAW_MEMORY		MMRawMem);

NTSTATUS
mm_locked_mdlInfo(	IN		PVIRTUAL_MM_INFO	VaddrInfo,
					IN OUT	PMM_MDL_INFO		MdlInfo );

NTSTATUS
mm_unlocked_mdlInfo(	IN OUT	PMM_MDL_INFO	MdlInfo );

NTSTATUS
mm_locked_and_init_Chunk(	IN		PVOID			Vaddr,
							IN		ULONG			ChunkSize,
							IN		ULONG			NumberOfPages,
							IN	OUT	PMM_CHUNK_ENTRY	MmChunkEntry);

NTSTATUS
mm_unlocked_and_deInit_Chunk(	IN		ULONG			ChunkSize,
								IN		ULONG			NumberOfPages,
								IN	OUT	PMM_CHUNK_ENTRY	MmChunkEntry);

// Followings are main API used outside of MM, to allocate and free memory !!
PMM_HOLDER
mm_alloc( ULONG Size, BOOLEAN LockedAndMapedVA);

VOID
mm_free( PMM_HOLDER MmHolder);

VOID
mm_free_ProtoHdr( PSFTK_LG Sftk_Lg, PMM_PROTO_HDR MmProtoHdr);

PMM_HOLDER
mm_alloc_buffer( PSFTK_LG Sftk_Lg, ULONG Size, HDR_TYPE Hdr_Type, BOOLEAN LockedAndMapedVA);

VOID
mm_free_buffer( PSFTK_LG Sftk_Lg, PMM_HOLDER MmHolder);

NTSTATUS
mm_reserve_buffer_for_refresh( PSFTK_LG Sftk_Lg );

NTSTATUS
mm_release_buffer_for_refresh( PSFTK_LG Sftk_Lg );

NTSTATUS
mm_get_buffer_from_refresh_pool( PSFTK_LG Sftk_Lg, PMM_HOLDER *MmHolder);

NTSTATUS
mm_free_buffer_to_refresh_pool( PSFTK_LG Sftk_Lg, PMM_HOLDER MmHolder );

NTSTATUS
mm_get_next_buffer_from_refresh_pool( PSFTK_LG Sftk_Lg );

PMM_HOLDER
mm_alloc_buffer_protoHdr_SoftHdr( PSFTK_LG Sftk_Lg, ULONG Size, ULONG	NumOfSoftHdr, BOOLEAN LockedAndMapedVA);

VOID
SM_AddCmd_ForMM( SM_EVENT_COMMAND SmCmd);


// ------------------- MM with Protocol Macros Definations ----------------------------------------
NTSTATUS
MM_COPY_BUFFER(	PMM_HOLDER pMmHolder, PVOID	SrcBuffer, ULONG Size, BOOLEAN CopyFromSrcBuffer );

// Locked Memory API will lock Mdl and get system virtual address which is stored inside MM_HOLDEr
// Caller can use MM_HOLDER->MDl Directly or MM_Holder->SystemVAddr, use memory size as MM_HOLDER->Size
NTSTATUS
MM_Locked_memory(	PMM_HOLDER MmHolder );

// UnLocked Memory API will Unlock Mdl and Make system virtual address to NULL inside MM_HOLDER
// Caller can not use MM_HOLDER->MDl Directly or MM_Holder->SystemVAddr, since its NULL now
// but MM_HOLDER->size field is till have valid value, and data is also valid.
NTSTATUS
MM_UnLocked_memory(	PMM_HOLDER MmHolder );


/*
//
//  VOID
//  MM_COPY_BUFFER(	PMM_HOLDER _pMmHolder_, PVOID	_SrcBuffer_, ULONG _Size_ );
//	
//	Copy Buffer from SrcBuffer into MM_HOLDER SystemVAddr memory for specified Size
//
#define MM_COPY_BUFFER(_pMmHolder_, _SrcBuffer_, _Size_)	{				\
					if( ((PMM_HOLDER) (_pMmHolder_))->SystemVAddr)	{		\
						OS_RtlCopyMemory( ((PMM_HOLDER) (_pMmHolder_))->SystemVAddr, _SrcBuffer_, _Size_);\
					}												\
					else	{										\
						OS_ASSERT(FALSE);							\
					}												\
				}
*/
//
//  PLIST_ENTRY
//  MM_GetContextList(	PMM_HOLDER _pMmHolder_);
//	
//	Returns Raw Data Buffer Pointer
//
#define MM_GetContextList(_pMmHolder_)		(&(((PMM_HOLDER) (_pMmHolder_))->IrpContextList))

//
//  PMM_HOLDER
//  MM_GetMMHolderFromContextList(	PLIST_ENTRY _pIrpContextList_);
//	
//	Returns Raw Data Buffer Pointer
//
#define MM_GetMMHolderFromContextList(_pIrpContextList_)	CONTAINING_RECORD( _pIrpContextList_, MM_HOLDER, IrpContextList)	

//
//  PVOID
//  MM_GetMdlPointer(	PMM_HOLDER _pMmHolder_);
//	
//	Returns MDL Pointer back to caller 
//
#define MM_GetMdlPointer(_pMmHolder_)		((PMM_HOLDER) (_pMmHolder_))->Mdl

//
//  PVOID
//  MM_GetDataBufferPointer(	PMM_HOLDER _pMmHolder_);
//	
//	Returns Raw Data Buffer Pointer
//
#define MM_GetDataBufferPointer(_pMmHolder_)		((PMM_HOLDER) (_pMmHolder_))->SystemVAddr

//
//  PVOID
//  MM_GetHdr(	PMM_HOLDER _pMmHolder_);
//	
//	Returns Hdr pointer from MM_HOLDER->pProtocolHdr
//	Hdr pointer would be either MM_PROTO_HDR or MM_SOFT_HDR
//
#define MM_GetHdr(_pMmHolder_)	(((PMM_HOLDER) (_pMmHolder_))->pProtocolHdr)

//
//  BOOLEAN
//  MM_IsProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_);
//	
//	Returns TRUE if Hdr is PROTO_HDR else return FALSE 
//
#define MM_IsProtoHdr(_pMMProtoHdr_)	OS_IsFlagSet(((PMM_PROTO_HDR) (_pMMProtoHdr_))->Flag, MM_FLAG_PROTO_HDR)

//
//  PMDL
//  MM_GetRootMdl(	PMM_HOLDER _pMMHolder_);
//
//	The Assumptions made for this function are 
//
//	OS_ASSERT(_pMMHolder_ != NULL);
//	OS_ASSERT(MM_GetHdr(_pMMHolder_) != NULL);
//	OS_ASSERT(MM_IsProtoHdr((PMM_PROTO_HDR) MM_GetHdr(_pMMHolder_)) == TRUE);
//
//	Returns the Root Mail of the following MM_HOLDER Chain. 
//
#define MM_GetMMHolderListRootMdl(_pMMHolder_)	((PMM_PROTO_HDR) MM_GetHdr(_pMMHolder_))->Mdl

//
//  VOID
//  MM_SetProtoHdrHasDataPkt(	PMM_PROTO_HDR _pMMProtoHdr_);
//	
//	Sets Current MM Proto Hdr has valid IO data pkt (with or without softhdr in it)
//
#define MM_SetProtoHdrHasDataPkt(_pMMProtoHdr_)	OS_SetFlag(((PMM_PROTO_HDR) (_pMMProtoHdr_))->Flag, MM_FLAG_DATA_PKTS_IN_SOFT_HDR)

//
//  BOOLEAN
//  MM_IsProtoHdrHasDataPkt(	PMM_PROTO_HDR _pMMProtoHdr_);
//	
//	Returns TRUE if Current Proto Hdr has valid I/O data pkt or has soft hdr and Valid I/O data pkt in it. 
//
#define MM_IsProtoHdrHasDataPkt(_pMMProtoHdr_)	OS_IsFlagSet(((PMM_PROTO_HDR) (_pMMProtoHdr_))->Flag, MM_FLAG_DATA_PKTS_IN_SOFT_HDR)

//
//  LONGLONG
//  MM_GetOffsetFromProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_);
//	Returns Offset value in bytes
//
#define MM_GetOffsetFromProtoHdr(_pMMProtoHdr_)		\
				((LONGLONG) (((PMM_PROTO_HDR) (_pMMProtoHdr_))->Hdr.msg.lg.offset << DEV_BSHIFT))

//
//  LONG
//  MM_GetOffsetInBlocksFromProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_);
//	Returns Offset value in blocks
//
#define MM_GetOffsetInBlocksFromProtoHdr(_pMMProtoHdr_)		( ((PMM_PROTO_HDR) (_pMMProtoHdr_))->Hdr.msg.lg.offset) 

//
//  ULONG
//  MM_GetLengthFromProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_);
//	Returns Length value in bytes
//
#define MM_GetLengthFromProtoHdr(_pMMProtoHdr_)		\
				((ULONG) (((PMM_PROTO_HDR) (_pMMProtoHdr_))->Hdr.msg.lg.len << DEV_BSHIFT))

//
//  ULONG
//  MM_GetLengthInBlocksFromProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_);
//	Returns Length value in bytes
//
#define MM_GetLengthInBlocksFromProtoHdr(_pMMProtoHdr_)		\
				((ULONG) (((PMM_PROTO_HDR) (_pMMProtoHdr_))->Hdr.msg.lg.len))

//
//  VOID
//  MM_SetRetPayloadInfoInMMProtoHdr(	PMM_PROTO_HDR	_pMMProtoHdr_, 
//										ftd_header_t	*_RetProtoHDr_,
//										PVOID			_RetBuffer_,
//										ULONG			_RetBufferSize_);
//
//	This API sets the supplied values inside MM_PROTO_HDR
//	Caller has to allocate and Copy/initialized _RetProtoHdr_ before calling this API
//	Caller has to allocate and Copy/initialized _RetBuffer_ before calling this API
//	if _RetBuffer_ is non null value, than _RetBufferSize_ must be greater than 0
//
#define MM_SetRetPayloadInfoInMMProtoHdr(_pMMProtoHdr_, _RetProtoHDr_, _RetBuffer_, _RetBufferSize_)	\
					((PMM_PROTO_HDR) (_pMMProtoHdr_))->RetProtoHDr	= _RetProtoHDr_;					\
					((PMM_PROTO_HDR) (_pMMProtoHdr_))->RetBuffer	= _RetBuffer_;						\
					((PMM_PROTO_HDR) (_pMMProtoHdr_))->RetBufferSize= _RetBufferSize_;					\
					if ( (_RetProtoHDr_ != NULL) || (_RetBuffer_ != NULL) )	{							\
						OS_ASSERT(((PMM_PROTO_HDR) (_pMMProtoHdr_))->Event != NULL);					\
					}
//
//  PKEVENT
//  MM_GetEventFromMMProtoHdr( PMM_PROTO_HDR _pMMProtoHdr_);
//
//	Returns Event pointer from MM_PROTO_HDR
//
#define MM_GetEventFromMMProtoHdr(_pMMProtoHdr_)	((PMM_PROTO_HDR) (_pMMProtoHdr_))->Event

//
//  ULONG
//  MM_SetEventInMMProtoHdr( PMM_PROTO_HDR _pMMProtoHdr_, PKEVENT _pEvent_);
//
//	Before calling this API, Event must have initialized.
//	This API Sets specified Event pointer into MM_PROTO_HDR, which gets used to signal pkt
//	once Ack gets recived
//
#define MM_SetEventInMMProtoHdr(_pMMProtoHdr_, _pEvent_)	((PMM_PROTO_HDR) (_pMMProtoHdr_))->Event = (_pEvent_)

//
//  ULONG
//  MM_IsEventValidInMMProtoHdr( PMM_PROTO_HDR _pMMProtoHdr_, PKEVENT _pEvent_);
//
//	Returns TRUE if Event is set and has valid value in MM_PROTO_HDR else FALSE
//
#define MM_IsEventValidInMMProtoHdr(_pMMProtoHdr_)  ( ( ((PMM_PROTO_HDR) (_pMMProtoHdr_))->Event)?TRUE:FALSE)

//
//  ULONG
//  MM_SignalEventInMMProtoHdr( PMM_PROTO_HDR _pMMProtoHdr_);
//
//	This API Signals Event set in MM_PROTO_HDR, if event set as NULL than does not do anything
//
#define MM_SignalEventInMMProtoHdr(_pMMProtoHdr_)							\
				if ( ((PMM_PROTO_HDR) (_pMMProtoHdr_))->Event )	{			\
					KeSetEvent( ((PMM_PROTO_HDR) (_pMMProtoHdr_))->Event, 0 , FALSE);	\
				}

//
//  VOID
//  MM_InsertSoftHdrIntoProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_, PMM_SOFT_HDR _pSoftHdr_);
//
//	This API Inserts Specified MM_SoftHeader into PMM_PROTO_HDR Anchor Link list,
//	It inserts at Tail of The Anchor
//
#define MM_InsertSoftHdrIntoProtoHdr(_pMMProtoHdr_, _pMMSoftHdr_)		\
				InsertTailList( &( (PMM_PROTO_HDR) (_pMMProtoHdr_))->MmSoftHdrList.ListEntry, &( (PMM_SOFT_HDR)(_pMMSoftHdr_))->MmSoftHdrLink);\
				((PMM_PROTO_HDR) (_pMMProtoHdr_))->MmSoftHdrList.NumOfNodes ++;
//
//  VOID
//  MM_GetSoftHdrFromProtoHdr(	PMM_PROTO_HDR _pMMProtoHdr_, PMM_SOFT_HDR _pSoftHdr_);
//	Returns Soft Hdr if it exist in MM_PROTO_HDR otherwise it return NULL
//
#define MM_GetSoftHdrFromProtoHdr(_pMMProtoHdr_, _pSoftHdr_)		\
			if ( ((PMM_PROTO_HDR) (_pMMProtoHdr_))->MmSoftHdrList.NumOfNodes == 0)	{						\
				_pSoftHdr_ = NULL;																			\
			}																								\
			else	{																						\
				PLIST_ENTRY _listEntry_;																	\
				_listEntry_ =  ((PMM_PROTO_HDR) (_pMMProtoHdr_))->MmSoftHdrList.ListEntry.Flink;			\
				OS_ASSERT( _listEntry_ != &(((PMM_PROTO_HDR) (_pMMProtoHdr_))->MmSoftHdrList.ListEntry) );	\
				_pSoftHdr_ = CONTAINING_RECORD( _listEntry_, MM_SOFT_HDR, MmSoftHdrLink);					\
			}
//
//  PVOID
//  MM_RemoveSoftHdrFromMMHolder(	PMM_HOLDER _pMmHolder_);
//	
//	Returns Hdr pointer from MM_HOLDER->pProtocolHdr
//	Hdr pointer would be either MM_PROTO_HDR or MM_SOFT_HDR
//
#define MM_RemoveSoftHdrFromMMHolder(_pMmHolder_)	\
			((PMM_HOLDER) (_pMmHolder_))->pProtocolHdr = NULL; \
			OS_ClearFlag( ((PMM_HOLDER) (_pMmHolder_))->Proto_type, MM_HOLDER_PROTO_TYPE_SOFT_HDR);
//
//  BOOLEAN
//  MM_IsSoftHdr(	PMM_SOFT_HDR _pMMSoftHdr_);
//	
//	Returns TRUE if Hdr is SOFT_HDR else return FALSE 
//
#define MM_IsSoftHdr(_pMMSoftHdr_)	OS_IsFlagSet(((PMM_SOFT_HDR) (_pMMSoftHdr_))->Flag, MM_FLAG_SOFT_HDR)

//
//  LONGLONG
//  MM_GetOffsetFromSoftHdr(	PMM_SOFT_HDR _pMMSoftHdr_);
//	Returns Offset value in bytes
//
#define MM_GetOffsetFromSoftHdr(_pMMSoftHdr_)	((LONGLONG) (((PMM_SOFT_HDR) (_pMMSoftHdr_))->Hdr.offset << DEV_BSHIFT))

//
//  ULONG
//  MM_GetOffsetInBlocksFromSoftHdr(	PMM_SOFT_HDR _pMMSoftHdr_)
//	Returns Offset value in blocks
//
#define MM_GetOffsetInBlocksFromSoftHdr(_pMMSoftHdr_)	(((PMM_SOFT_HDR) (_pMMSoftHdr_))->Hdr.offset)

//
//  ULONG
//  MM_GetLengthFromSoftHdr( PMM_SOFT_HDR _pMMSoftHdr_);
//	Returns Length value in bytes
//
#define MM_GetLengthFromSoftHdr(_pMMSoftHdr_)	((ULONG) (((PMM_SOFT_HDR) (_pMMSoftHdr_))->Hdr.length << DEV_BSHIFT))

//
//  ULONG
//  MM_GetLengthInBlocksFromSoftHdr(	PMM_SOFT_HDR _pMMSoftHdr_);
//	Returns Length value in bytes
//
#define MM_GetLengthInBlocksFromSoftHdr(_pMMSoftHdr_)		((ULONG) (((PMM_SOFT_HDR) (_pMMSoftHdr_))->Hdr.length))


//
//  VOID
//  MM_GetSftkDevFromMM_Holder( PMM_HOLDER _MMHolder_,PSFTK_DEV _SftkDev_)
//	Returns SftkDev ptr from MM_Holder if exist
//
#define MM_GetSftkDevFromMM_Holder(_MMHolder_, _SftkDev_)			{											\
		((PSFTK_DEV) (_SftkDev_)) = NULL;																		\
		if (MM_IsProtoHdr(MM_GetHdr(_MMHolder_)) == TRUE)														\
		{																										\
			((PSFTK_DEV) _SftkDev_) = (PSFTK_DEV) ((PMM_SOFT_HDR) (MM_GetHdr(_MMHolder_)))->SftkDev;			\
		}																										\
		else 																									\
		{																										\
			if (MM_IsSoftHdr(MM_GetHdr(_MMHolder_)) == TRUE)	{												\
				((PSFTK_DEV) _SftkDev_) = (PSFTK_DEV) ((PMM_PROTO_HDR) (MM_GetHdr(_MMHolder_)))->SftkDev;		\
			}																									\
		}																										\
	}

// ------------------- Protocol Macros Definations ----------------------------------------

//
//  VOID
//  Proto_MMSetStatusInProtoHdr( PMM_PROTO_HDR _MMProtoHdr_, NTSTATUS _Status_);
//	
//	Set Status Inside MM_PROTO_HDR used mainly for outband pkt.
//
#define Proto_MMSetStatusInProtoHdr(_MMProtoHdr_, _Status_)		((PMM_PROTO_HDR) (_MMProtoHdr_))->Status = _Status_

//
//  NTSTATUS
//  Proto_MMGetStatusFromProtoHdr( PMM_PROTO_HDR _MMProtoHdr_);
//	
//	Return Status from MM_PROTO_HDR used mainly for outband pkt.
//
#define Proto_MMGetStatusFromProtoHdr(_MMProtoHdr_)		((PMM_PROTO_HDR) (_MMProtoHdr_))->Status

//
//  ftd_header_t *
//  Proto_MMGetSoftHdr(	PMM_PROTO_HDR _MMProtoHdr_);
//	
//	Returns Actual Protocol Hdr Pointer from PMM_PROTO_HDR
//
#define Proto_MMGetProtoHdr(_MMProtoHdr_)		&( ((PMM_PROTO_HDR) (_MMProtoHdr_))->Hdr)

//
//  VOID
//  Set_MM_ProtoHdrMsgType(	PMM_PROTO_HDR _MMProtoHdr_, enum Protocol _MsgType_);
//	
//	Sets MsgType in MM_PROTO_HDR->msgtype field.
//
#define Set_MM_ProtoHdrMsgType(_MMProtoHdr_, _MsgType_)		( (PMM_PROTO_HDR) (_MMProtoHdr_) )->msgtype = _MsgType_

//
//  enum Protocol
//  Get_MM_ProtoHdrMsgType(	PMM_PROTO_HDR _MMProtoHdr_);
//	
//	Returns MsgType from MM_PROTO_HDR->msgtype field.
//
#define Get_MM_ProtoHdrMsgType(_MMProtoHdr_)	( (PMM_PROTO_HDR) (_MMProtoHdr_) )->msgtype

//
//  enum Protocol
//  Proto_Get_ProtoHdrMsgType(	PMM_PROTO_HDR _MMProtoHdr_);
//	
//	Returns MsgType from MM_PROTO_HDR->Hdr.msgType field.
//
#define Proto_Get_ProtoHdrMsgType(_MMProtoHdr_)		(( (PMM_PROTO_HDR) (_MMProtoHdr_) )->Hdr.msgtype)

//
//  wlheader_t *
//  Proto_MMGetSoftHdr(	PMM_SOFT_HDR _MMSoftHdr_);
//	
//	Returns Actual Soft Hdr Pointer from PMM_SOFT_HDR
//
#define Proto_MMGetSoftHdr(_MMSoftHdr_)		&( ((PMM_SOFT_HDR) (_MMSoftHdr_))->Hdr)

//
//  int
//  Proto_GetProtoHdrSize(	PMM_PROTO_HDR _MMProtoHdr_)
//	
//	Returns Actual Proto Hdr Size which is sizeof(ftd_header_t)
//
#define Proto_GetProtoHdrSize(_MMProtoHdr_)		sizeof( ((PMM_PROTO_HDR) (_MMProtoHdr_))->Hdr )

//
//  int
//  Proto_GetSoftHdrSize(	PMM_SOFT_HDR _MMSoftHdr_)
//	
//	Returns Actual Soft Hdr Size which is sizeof(wlheader_t)
//
#define Proto_GetSoftHdrSize(_MMSoftHdr_)		sizeof( ((PMM_SOFT_HDR) (_MMSoftHdr_))->Hdr )

//
//	int
//  Proto_GetMsgTypeFromProtoHdr( ftd_header_t * _ProtoHdr_)
//	
//	Returns msgtype values from ftd_header_t
//
#define Proto_GetMsgTypeFromProtoHdr(_ProtoHdr_)	((ftd_header_t *)(_ProtoHdr_))->msgtype

//
//	BOOLEAN
//  Proto_IsMsgRefhreshIOPktProtoHdr( ftd_header_t * _ProtoHdr_)
//	
//	Returns TRUE if Its Data pkt else FALSE
//
#define Proto_IsMsgRefhreshIOPktProtoHdr(_ProtoHdr_)	(((ftd_header_t *)(_ProtoHdr_))->msgtype == FTDCRFBLK)

//
//	BOOLEAN
//  Proto_IsSoftHdrNeedForMsg( enum Protocol _MsgType_)
//	
//	Returns TRUE if Its Data pkt else FALSE
//  TODO : Veera
#define Proto_IsSoftHdrNeedForMsg(_MsgType_)			\
			( ( (_MsgType_== FTDMSGINCO) || (_MsgType_== FTDMSGCO)	||				\
				(_MsgType_== FTDMSGCPON) || (_MsgType_== FTDMSGAVOIDJOURNALS) ||	\
				(_MsgType_== FTDMSGCPOFF) )	?TRUE: FALSE)

//
//	BOOLEAN
//  Proto_IsMsgBABIOPktProtoHdr( ftd_header_t * _ProtoHdr_)
//	
//	Returns TRUE if Its Data pkt else FALSE
//
#define Proto_IsMsgBABIOPktProtoHdr(_ProtoHdr_)	(((ftd_header_t *)(_ProtoHdr_))->msgtype == FTDCRFBLK)


//	The input to all the Protocol Header or Soft Header are in Bytes and will be converted 
//	accordingly as per the requirements of the specific Command Type
//
//	VOID
//  Proto_MMInitSoftHDr(PMM_HOLDER _pMmHolder_, LONGLONG _Offset_, ULONG _Length_, PSFTK_DEV _SftkDev_)
//	
//	Initlaize Soft Hdr all required fields
//
#define Proto_MMInitSoftHDr(_pMmHolder_, _Offset_, _Length_, _pSftk_Dev_)		{							\
				wlheader_t		*_pSoftHdr_	 = Proto_MMGetSoftHdr( MM_GetHdr(_pMmHolder_) );				\
				LARGE_INTEGER	_currentTime_;																\
					OS_ZeroMemory(_pSoftHdr_,Proto_GetSoftHdrSize( MM_GetHdr(_pMmHolder_) ));				\
					_pSoftHdr_->majicnum	= DATASTAR_MAJIC_NUM;											\
					_pSoftHdr_->offset		= (ULONG) (_Offset_ >> DEV_BSHIFT);								\
					_pSoftHdr_->length		= (ULONG) (_Length_ >> DEV_BSHIFT);								\
					OS_PerfGetClock(&_currentTime_, NULL);													\
					_pSoftHdr_->timestamp	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));		\
					_pSoftHdr_->dev			= _pSftk_Dev_->bdev;											\
					_pSoftHdr_->diskdev		= _pSftk_Dev_->bdev;											\
					_pSoftHdr_->flags		= 0;															\
					_pSoftHdr_->complete	= 0;															\
					_pSoftHdr_->timoid		= 0;															\
					_pSoftHdr_->group_ptr	= _pSftk_Dev_->SftkLg;											\
					_pSoftHdr_->bp			= NULL;															\
					((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = _pSftk_Dev_;						\
				}
//
//	VOID
//  Proto_MMInitProtoHdrForRefresh(PMM_HOLDER _pMmHolder_, LONGLONG _Offset_, ULONG _Length_, PSFTK_DEV _pSftk_Dev_ , LONG _compress_)
//	
//	Initlaize Proto Hdr for refresh IO pkts with all required fields
//	Here the length and offset are in Bytes, hence offset by the DEV_BSHIFT (9) to convert to Sectors
//
#define Proto_MMInitProtoHdrForRefresh(_pMmHolder_, _Offset_, _Length_, _pSftk_Dev_ , _compress_)		{		\
				ftd_header_t	*_pProtoHdr_ =	Proto_MMGetProtoHdr( MM_GetHdr(_pMmHolder_) );					\
				LARGE_INTEGER	_currentTime_;																	\
					OS_ZeroMemory(_pProtoHdr_,Proto_GetProtoHdrSize( MM_GetHdr(_pMmHolder_)) );					\
					_pProtoHdr_->magicvalue			= MAGICHDR;													\
					OS_PerfGetClock(&_currentTime_, NULL);														\
					_pProtoHdr_->ts	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));					\
					_pProtoHdr_->msgtype			= FTDCRFBLK;												\
					_pProtoHdr_->cli				= 0;														\
					_pProtoHdr_->compress			= _compress_;												\
					_pProtoHdr_->len				= (ULONG) (_Length_);										\
					_pProtoHdr_->uncomplen			= _Length_;													\
					_pProtoHdr_->ackwanted			= 1;														\
					_pProtoHdr_->msg.lg.lgnum		= (int) ((PSFTK_DEV) (_pSftk_Dev_))->SftkLg->LGroupNumber;	\
					_pProtoHdr_->msg.lg.devid		= ((PSFTK_DEV) (_pSftk_Dev_))->bdev;						\
					_pProtoHdr_->msg.lg.bsize		= 0;														\
					_pProtoHdr_->msg.lg.offset		= (ULONG) (_Offset_ >> DEV_BSHIFT);							\
					_pProtoHdr_->msg.lg.len			= (ULONG) (_Length_ >> DEV_BSHIFT);							\
					_pProtoHdr_->msg.lg.data		= (int) ((PSFTK_DEV) (_pSftk_Dev_))->SftkLg;				\
					_pProtoHdr_->msg.lg.flags		= 0;														\
					_pProtoHdr_->msg.lg.reserved	= 0;														\
					((PMM_PROTO_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = (_pSftk_Dev_);						\
				}

//
//	VOID
//  Proto_MMInitProtoHdrForIOData(PMM_PROTO_HDR _pMmProtoHdr_, ULONG _Length_, LONG _compress_)
//	
//	Initlaize Proto Hdr for BAB pkts (New Incoming Writes) with all required fields
//	Here the Length is in Bytes which is total size of = n soft hdr + total n data pkts size 
//	length is nothing but following raw data total size.
//
#define Proto_MMInitProtoHdrForIOData(_pMmProtoHdr_, _Length_, _compress_)					{					\
				ftd_header_t	*_pProtoHdr_ = Proto_MMGetProtoHdr(_pMmProtoHdr_);								\
				LARGE_INTEGER	_currentTime_;																	\
					OS_ZeroMemory(_pProtoHdr_,Proto_GetProtoHdrSize(_pMmProtoHdr_));							\
					_pProtoHdr_->magicvalue	= MAGICHDR;															\
					OS_PerfGetClock(&_currentTime_, NULL);														\
					_pProtoHdr_->ts	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));					\
					_pProtoHdr_->msgtype			= FTDCCHUNK;												\
					_pProtoHdr_->cli				= 0;														\
					_pProtoHdr_->compress			= _compress_;												\
					_pProtoHdr_->len				= _Length_;													\
					_pProtoHdr_->uncomplen			= _Length_;													\
					_pProtoHdr_->ackwanted			= 1;														\
					_pProtoHdr_->msg.lg.lgnum		= 0;														\
					_pProtoHdr_->msg.lg.devid		= 0;														\
					_pProtoHdr_->msg.lg.bsize		= 0;														\
					_pProtoHdr_->msg.lg.offset		= 0;														\
					_pProtoHdr_->msg.lg.len			= _Length_;													\
					_pProtoHdr_->msg.lg.data		= 0;														\
					_pProtoHdr_->msg.lg.flags		= 0;														\
					_pProtoHdr_->msg.lg.reserved	= 0;														\
					((PMM_PROTO_HDR) (_pMmProtoHdr_))->SftkDev = NULL;											\
				}

//
//	VOID
//  Proto_MMInitProtoHdrForSentinal(PMM_HOLDER _pMmHolder_, LONGLONG _Offset_, ULONG _Length_, LONG _compress_)
//	
//	Initlaize Proto Hdr for refresh pkts with all required fields
//	The Length will allways be Bytes
//
#define Proto_MMInitProtoHdrForSentinal(_pMmHolder_, _Offset_, _Length_, _compress_)		{					\
				ftd_header_t	*_pProtoHdr_ = &((PMM_PROTO_HDR) (MM_GetHdr(_pMmHolder_)))->Hdr;				\
				LARGE_INTEGER	_currentTime_;																	\
					OS_ZeroMemory(_pProtoHdr_,Proto_GetProtoHdrSize((PMM_PROTO_HDR) (MM_GetHdr(_pMmHolder_))));	\
					_pProtoHdr_->magicvalue	= MAGICHDR;															\
					OS_PerfGetClock(&_currentTime_, NULL);														\
					_pProtoHdr_->ts	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));					\
					_pProtoHdr_->msgtype			= FTDCCHUNK;												\
					_pProtoHdr_->cli				= 0;														\
					_pProtoHdr_->compress			= _compress_;												\
					_pProtoHdr_->len				= _Length_;													\
					_pProtoHdr_->uncomplen			= _Length_;													\
					_pProtoHdr_->ackwanted			= 1;														\
					_pProtoHdr_->msg.lg.lgnum		= 0;														\
					_pProtoHdr_->msg.lg.devid		= 0;														\
					_pProtoHdr_->msg.lg.bsize		= 0;														\
					_pProtoHdr_->msg.lg.offset		= 0;														\
					_pProtoHdr_->msg.lg.len			= _Length_;													\
					_pProtoHdr_->msg.lg.data		= 0;								\
					_pProtoHdr_->msg.lg.flags		= 0;														\
					_pProtoHdr_->msg.lg.reserved	= 0;														\
					((PMM_PROTO_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = NULL;									\
				}


// Initializes MSG_INCO Sentinal

#define Proto_MMInitMSGINCOSentinal(_pMmHolder_ , _Length_)		{											\
				wlheader_t		*_pSoftHdr_	 = &((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->Hdr;				\
				LARGE_INTEGER	_currentTime_;																\
					OS_ASSERT(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr != NULL);							\
					OS_ASSERT(_pSoftHdr_ != NULL);															\
					OS_ZeroMemory(_pSoftHdr_,Proto_GetSoftHdrSize((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_))));	\
					_pSoftHdr_->majicnum	= DATASTAR_MAJIC_NUM;											\
					_pSoftHdr_->offset		= -1;															\
					_pSoftHdr_->length		= (ULONG) (_Length_ >> DEV_BSHIFT);								\
					OS_PerfGetClock(&_currentTime_, NULL);													\
					_pSoftHdr_->timestamp	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));		\
					_pSoftHdr_->dev			= -1;															\
					_pSoftHdr_->diskdev		= -1;															\
					_pSoftHdr_->flags		= 0;															\
					_pSoftHdr_->complete	= 1;															\
					_pSoftHdr_->timoid		= 0;															\
					_pSoftHdr_->group_ptr	= NULL;															\
					_pSoftHdr_->bp			= NULL;															\
					((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = NULL;								\
					OS_RtlCopyMemory(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr, MSG_INCO, sizeof(MSG_INCO));	\
				}
// Initializes the MSG_CO Sentinal

#define Proto_MMInitMSGCOSentinal(_pMmHolder_ , _Length_)		{											\
				wlheader_t		*_pSoftHdr_	 = &((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->Hdr;				\
				LARGE_INTEGER	_currentTime_;																\
					OS_ASSERT(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr != NULL);							\
					OS_ASSERT(_pSoftHdr_ != NULL);															\
					OS_ZeroMemory(_pSoftHdr_,Proto_GetSoftHdrSize((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_))));	\
					_pSoftHdr_->majicnum	= DATASTAR_MAJIC_NUM;											\
					_pSoftHdr_->offset		= -1;															\
					_pSoftHdr_->length		= (ULONG) (_Length_ >> DEV_BSHIFT);								\
					OS_PerfGetClock(&_currentTime_, NULL);													\
					_pSoftHdr_->timestamp	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));		\
					_pSoftHdr_->dev			= -1;															\
					_pSoftHdr_->diskdev		= -1;															\
					_pSoftHdr_->flags		= 0;															\
					_pSoftHdr_->complete	= 1;															\
					_pSoftHdr_->timoid		= 0;															\
					_pSoftHdr_->group_ptr	= NULL;															\
					_pSoftHdr_->bp			= NULL;															\
					((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = NULL;								\
					OS_RtlCopyMemory(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr, MSG_CO, sizeof(MSG_CO));		\
				}


// Initializes the MSG_AVOID_JOURNALS Sentinal

#define Proto_MMInitMSGAVOIDJOURNALSSentinal(_pMmHolder_ , _Length_)		{								\
				wlheader_t		*_pSoftHdr_	 = &((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->Hdr;				\
				LARGE_INTEGER	_currentTime_;																\
					OS_ASSERT(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr != NULL);							\
					OS_ASSERT(_pSoftHdr_ != NULL);															\
					OS_ZeroMemory(_pSoftHdr_,Proto_GetSoftHdrSize((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_))));	\
					_pSoftHdr_->majicnum	= DATASTAR_MAJIC_NUM;											\
					_pSoftHdr_->offset		= -1;															\
					_pSoftHdr_->length		= (ULONG) (_Length_ >> DEV_BSHIFT);								\
					OS_PerfGetClock(&_currentTime_, NULL);													\
					_pSoftHdr_->timestamp	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));		\
					_pSoftHdr_->dev			= -1;															\
					_pSoftHdr_->diskdev		= -1;															\
					_pSoftHdr_->flags		= 0;															\
					_pSoftHdr_->complete	= 1;															\
					_pSoftHdr_->timoid		= 0;															\
					_pSoftHdr_->group_ptr	= NULL;															\
					_pSoftHdr_->bp			= NULL;															\
					((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = NULL;								\
					OS_RtlCopyMemory(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr, MSG_AVOID_JOURNALS, sizeof(MSG_AVOID_JOURNALS));	\
				}


// Initializes the MSG_CPON Sentinal

#define Proto_MMInitMSGCPONSentinal(_pMmHolder_ , _Length_)		{											\
				wlheader_t		*_pSoftHdr_	 = &((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->Hdr;				\
				LARGE_INTEGER	_currentTime_;																\
					OS_ASSERT(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr != NULL);							\
					OS_ASSERT(_pSoftHdr_ != NULL);															\
					OS_ZeroMemory(_pSoftHdr_,Proto_GetSoftHdrSize((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_))));								\
					_pSoftHdr_->majicnum	= DATASTAR_MAJIC_NUM;											\
					_pSoftHdr_->offset		= -1;															\
					_pSoftHdr_->length		= (ULONG) (_Length_ >> DEV_BSHIFT);								\
					OS_PerfGetClock(&_currentTime_, NULL);													\
					_pSoftHdr_->timestamp	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));		\
					_pSoftHdr_->dev			= -1;															\
					_pSoftHdr_->diskdev		= -1;															\
					_pSoftHdr_->flags		= 0;															\
					_pSoftHdr_->complete	= 1;															\
					_pSoftHdr_->timoid		= 0;															\
					_pSoftHdr_->group_ptr	= NULL;															\
					_pSoftHdr_->bp			= NULL;															\
					((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = NULL;								\
					OS_RtlCopyMemory(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr, MSG_CPON, sizeof(MSG_CPON));	\
				}

// Initializes the MSG_CPOFF Sentinal

#define Proto_MMInitMSGCPOFFSentinal(_pMmHolder_ , _Length_)		{											\
				wlheader_t		*_pSoftHdr_	 = &((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->Hdr;					\
				LARGE_INTEGER	_currentTime_;																	\
					OS_ASSERT(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr != NULL);								\
					OS_ASSERT(_pSoftHdr_ != NULL);																\
					OS_ZeroMemory(_pSoftHdr_,Proto_GetSoftHdrSize((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_))));	\
					_pSoftHdr_->majicnum	= DATASTAR_MAJIC_NUM;												\
					_pSoftHdr_->offset		= -1;																\
					_pSoftHdr_->length		= (ULONG) (_Length_ >> DEV_BSHIFT);									\
					OS_PerfGetClock(&_currentTime_, NULL);														\
					_pSoftHdr_->timestamp	= (LONG) (_currentTime_.QuadPart / (ULONG)(10*1000*1000));			\
					_pSoftHdr_->dev			= -1;																\
					_pSoftHdr_->diskdev		= -1;																\
					_pSoftHdr_->flags		= 0;																\
					_pSoftHdr_->complete	= 1;																\
					_pSoftHdr_->timoid		= 0;																\
					_pSoftHdr_->group_ptr	= NULL;																\
					_pSoftHdr_->bp			= NULL;																\
					((PMM_SOFT_HDR) (MM_GetHdr(_pMmHolder_)))->SftkDev = NULL;									\
					OS_RtlCopyMemory(((PMM_HOLDER)(_pMmHolder_))->SystemVAddr, MSG_CPOFF, sizeof(MSG_CPOFF));	\
				}


//
//	VOID
//  SFTK_ACK_UPDATE_BITMAP(PSFTK_DEV _pSftk_Dev_, LONGLONG _Offset_, ULONG _Length_)
//	
//	Updates ALrdb and if marked also updares HRDB bitmap for specified Device with its specified range
//
#if TARGET_SIDE 
#define SFTK_ACK_UPDATE_BITMAP( _pSftk_Dev_, _Offset_, _Length_)		{										\
				ULONG			_numBits_, _startBit_, _endBit_;												\
				if (_pSftk_Dev_->SftkLg->UseSRDB == FALSE)														\
				{																								\
					sftk_bit_range_from_offset( &_pSftk_Dev_->ALrdb, _Offset_,_Length_,&_startBit_, &_endBit_ );\
					_numBits_ = (_endBit_ - _startBit_ + 1);													\
					OS_ASSERT( (_startBit_ + _numBits_) <= _pSftk_Dev_->ALrdb.TotalNumOfBits);					\
					RtlSetBits( _pSftk_Dev_->ALrdb.pBitmapHdr, _startBit_, _numBits_ );							\
				}																								\
				if (_pSftk_Dev_->SftkLg->UpdateHRDB == TRUE)													\
				{																								\
					sftk_bit_range_from_offset( &_pSftk_Dev_->Hrdb, _Offset_,_Length_,&_startBit_, &_endBit_ );	\
					_numBits_ = (_endBit_ - _startBit_ + 1);													\
					OS_ASSERT( (_startBit_ + _numBits_) <= _pSftk_Dev_->Hrdb.TotalNumOfBits);					\
					RtlSetBits( _pSftk_Dev_->Hrdb.pBitmapHdr, _startBit_, _numBits_ );							\
				}																								\
			}	

#else
#define SFTK_ACK_UPDATE_BITMAP( _pSftk_Dev_, _Offset_, _Length_)		{										\
				ULONG			_numBits_, _startBit_, _endBit_;												\
				sftk_bit_range_from_offset( &_pSftk_Dev_->ALrdb, _Offset_,_Length_,&_startBit_, &_endBit_ );	\
				_numBits_ = (_endBit_ - _startBit_ + 1);														\
				OS_ASSERT( (_startBit_ + _numBits_) <= _pSftk_Dev_->ALrdb.TotalNumOfBits);						\
				RtlSetBits( _pSftk_Dev_->ALrdb.pBitmapHdr, _startBit_, _numBits_ );								\
				if (_pSftk_Dev_->SftkLg->UpdateHRDB == TRUE)													\
				{																								\
					sftk_bit_range_from_offset( &_pSftk_Dev_->Hrdb, _Offset_,_Length_,&_startBit_, &_endBit_ );	\
					_numBits_ = (_endBit_ - _startBit_ + 1);													\
					OS_ASSERT( (_startBit_ + _numBits_) <= _pSftk_Dev_->Hrdb.TotalNumOfBits);					\
					RtlSetBits( _pSftk_Dev_->Hrdb.pBitmapHdr, _startBit_, _numBits_ );							\
				}																								\
			}	
#endif
//
// ULONGLONG 
// Get_Values_InBytesFromSectors(ULONG _ValuesInSectors_)
// Returns values in Bytes from Sector Values
//
#define Get_Values_InBytesFromSectors(_ValuesInSectors_)	((_ValuesInSectors_) << DEV_BSHIFT)

//
// ULONG
// Get_Values_InSectorsFromBytes(ULONGLONG _ValuesInBytes_)
// Returns values in Sectors from Bytes Values
//
#define Get_Values_InSectorsFromBytes(_ValuesInBytes_)	((_ValuesInBytes_) >> DEV_BSHIFT)

//
// Function prototype definations defined in sftk_SM.c
//
NTSTATUS
SM_Thread_Create(	IN OUT PMM_MANAGER	Mmgr);

NTSTATUS
SM_Thread_Terminate( IN OUT PMM_MANAGER	Mmgr);

NTSTATUS
SM_IPC_Open(PMM_MANAGER	Mmgr);

NTSTATUS
SM_IPC_Close(PMM_MANAGER	Mmgr);

VOID
SM_Thread( PMM_MANAGER	Mmgr);

NTSTATUS
Sftk_Ctl_MM_Cmd( PIRP Irp );

NTSTATUS
MM_CmdExecutedResult( PMM_MANAGER	Mmgr, IN PSM_CMD	pSM_Cmd );

NTSTATUS
MM_GetCmd( PMM_MANAGER	Mmgr, IN PSM_CMD	pSM_Cmd );


//
// Function prototype definations defined in sftk_Queue.c
//
VOID
QM_Init( PSFTK_LG	Sftk_Lg);

VOID
QM_DeInit( PSFTK_LG	Sftk_Lg);

NTSTATUS
QM_Insert( PSFTK_LG	Sftk_Lg, PVOID	MmContext, QUEUE_TYPE DstQueue, BOOLEAN GrabLock);

VOID
QM_Remove( PSFTK_LG	Sftk_Lg, PVOID	MmContext, QUEUE_TYPE SrcQueue, BOOLEAN GrabLock);

VOID
QM_Move( PSFTK_LG	Sftk_Lg, PVOID	MmContext, QUEUE_TYPE SrcQueue, QUEUE_TYPE DstQueue, BOOLEAN GrabLock);

NTSTATUS
QM_ScanAllQList(PSFTK_LG	Sftk_Lg, BOOLEAN FreeAllEntries, BOOLEAN UpdateBitmaps);

BOOLEAN
QM_MakeMdlChainOfSendQueuePackets(IN PANCHOR_LINKLIST			MmHolderAnchorList,
								  OUT OPTIONAL PMDL*			MdlPtr,	// Can be NULL
								  OUT PULONG					SizePtr,	// Returns Size of the Total Buffer
								  OUT PMM_HOLDER*				MmHolderPtr	// Can be NULL
								 );

BOOLEAN
QM_RetrievePkts(IN PSFTK_LG			Sftk_Lg, 
			   IN OUT PVOID			Buffer, 
			   IN OUT PULONG		Size,	// In Buffer Max size, OUT actual buffer data filled size
			   IN BOOLEAN			DontFreePackets,
			   OUT PMM_HOLDER		*MmRootMmHolderPtr, 
			   IN OUT PKSEMAPHORE	*WorkWaitEvent);

BOOLEAN
QM_RetrievePktsFromCommitQueue(	IN PSFTK_LG		Sftk_Lg,
								IN OUT PVOID	Buffer,
								IN OUT PULONG	Size,
								IN BOOLEAN		DontFreePackets,
								OUT PMM_HOLDER	*MmRootMmHolderPtr );	// In Buffer Max size, OUT actual buffer data filled size) 

NTSTATUS
QM_RemovePktsFromMigrateQueue(	IN				PSFTK_LG	Sftk_Lg,
								IN	OUT			PVOID		AckPacket,
								IN	OPTIONAL	PVOID		RetBuffer,		// Optional, Recieved extra buffer as payload along with Ack Pkt in recieve thread
								IN	OPTIONAL	ULONG		RetBufferSize );	// Optional, Size of Recieved extra buffer as payload along with Ack Pkt in recieve thread

NTSTATUS
QM_SendOutBandPkt(	PSFTK_LG Sftk_Lg, 
					BOOLEAN WaitForExecute, 
					BOOLEAN NoAckExepected, 
					enum protocol MsgType);

//
// Macro Defination used for QM
//

//
//	VOID
//  QM_GetTotalPkts( PSFTK_LG _Sftk_Lg_, ULONG _TotalPkts_)
//	
//	It returns Total number of All Queue's Pkts in _TotalPkts_ 
//
#define QM_GetTotalPkts(_Sftk_Lg_, _TotalPkts_)		{										\
				_TotalPkts_  = ((PSFTK_LG) (_Sftk_Lg_))->QueueMgr.PendingList.NumOfNodes;	\
				_TotalPkts_ += ((PSFTK_LG) (_Sftk_Lg_))->QueueMgr.CommitList.NumOfNodes;	\
				_TotalPkts_ += ((PSFTK_LG) (_Sftk_Lg_))->QueueMgr.RefreshList.NumOfNodes;	\
				_TotalPkts_ += ((PSFTK_LG) (_Sftk_Lg_))->QueueMgr.MigrateList.NumOfNodes;	\
				}

#endif // _SFTK_PROTO_H_