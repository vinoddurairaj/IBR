/**************************************************************************************

Module Name:	sftk_Ioctl.C   
Author Name:	Parag sanghvi
Description:	All Ioctl Handling APIs are defined here
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include <sftk_main.h>


NTSTATUS
sftk_ctl_ioctl(dev_t dev, int cmd, int arg, int flag, PIRP	Irp)
{
	NTSTATUS	status = STATUS_SUCCESS;

    switch (IOC_CST(cmd)) 
    {
    case IOC_CST(FTD_CONFIG_BEGIN):	
        status = sftk_ctl_config_begin(Irp);
        break;

	case IOC_CST(FTD_NEW_LG):
        status = sftk_ctl_new_lg(Irp);
        break;

	case IOC_CST(FTD_NEW_DEVICE):
        status = sftk_ctl_new_device(dev, cmd, arg, flag);
        break;

	case IOC_CST(FTD_CONFIG_END):	
        status = sftk_ctl_config_end(Irp);
        break;

	case IOC_CST(FTD_GET_NUM_GROUPS): // returned ULONG * as a number of total LG configured in driver
        status = sftk_ctl_lg_get_num_groups(dev, cmd, arg, flag);  
        break;

	case IOC_CST(FTD_GET_DEVICE_NUMS): // returned ULONG * as a number of total Devices for all LG 
        status = sftk_ctl_get_device_nums(dev, cmd, arg, flag); 
        break;

	case IOC_CST(FTD_GET_NUM_DEVICES):	// returned ULONG * as total Devices under LG number
        status = sftk_ctl_lg_get_num_devices(dev, cmd, arg, flag);  
        break;

	case IOC_CST(FTD_GET_DEVICES_INFO):	// struct stat_buffer_t and ftd_dev_info_t
		status = sftk_ctl_lg_get_devices_info(dev, cmd, arg, flag);  
        break;

	case IOC_CST(FTD_GET_GROUPS_INFO):	// struct stat_buffer_t and ftd_lg_info_t
		status = sftk_ctl_lg_get_groups_info(dev, cmd, arg, flag);  
        break;

	case IOC_CST(FTD_GET_GROUP_STATE):	// struct ftd_state_t
        status = sftk_ctl_lg_group_state(dev, cmd, arg, flag);
        break;

	case IOC_CST(FTD_SET_GROUP_STATE): // struct ftd_state_t
        status = sftk_ctl_lg_set_group_state(dev, cmd, arg, flag);  
        break;
	
	case IOC_CST(FTD_SET_IODELAY):	// struct ftd_param_t	
        status = sftk_ctl_lg_set_iodelay(dev, cmd, arg, flag);
        break;

    case IOC_CST(FTD_SET_SYNC_DEPTH):	// struct ftd_param_t
        status = sftk_ctl_lg_set_sync_depth(dev, cmd, arg, flag);
        break;

    case IOC_CST(FTD_SET_SYNC_TIMEOUT):	// struct ftd_param_t
        status = sftk_ctl_lg_set_sync_timeout(dev, cmd, arg, flag);
        break;

	// New IOCTLs for Statistics infotmation
	case IOC_CST(FTD_GET_ALL_STATS_INFO):	// in and out: variable size of struct ALL_LG_STATISTICS
        status = sftk_ctl_lg_Get_All_StatsInfo( Irp );
        break;

	case IOC_CST(FTD_GET_LG_STATS_INFO):	// in and out: variable size of struct LG_STATISTICS
        status = sftk_ctl_lg_Get_StatsInfo( Irp );
        break;

	// New IOCTLs for Tuning Paramaters infotmation and settings
	case IOC_CST(FTD_GET_LG_TUNING_PARAM):	// in and out: variable size of struct ALL_LG_STATISTICS
        status = sftk_ctl_lg_Get_Lg_TuningParam( Irp );
        break;

	case IOC_CST(FTD_SET_LG_TUNING_PARAM):	// in and out: variable size of struct LG_STATISTICS
        status = sftk_ctl_lg_Set_Lg_TuningParam( Irp );
        break;

	//
	// Followings IOCTL are optional only used in MCLI...Service is not using this IOCTL anymore ... not needed
	//
    case IOC_CST(FTD_DEL_DEVICE):
        status = sftk_ctl_del_device(dev, cmd, arg, flag);
        break;

    case IOC_CST(FTD_DEL_LG):
        status = sftk_ctl_del_lg(dev, cmd, arg, flag);
        break;

	case IOC_CST(FTD_GET_All_ATTACHED_DISKINFO):
        status = sftk_ctl_get_all_attached_diskInfo(Irp);
        break;

	case IOC_CST(FTD_RECONFIGURE_FROM_REGISTRY):
        status = sftk_configured_from_registry();
		if (!NT_SUCCESS(status))
		{ // Failed
			DebugPrint((DBG_ERROR, "sftk_init_driver: sftk_configured_from_registry() Failed with error %08x, ignoring this error \n", status));
			//return status;
		}
        break;
#if TARGET_SIDE
	case IOC_CST(FTD_IOCTL_FULLREFRESH_PAUSE_RESUME):
		status = sftk_lg_ctl_fullrefresh_pause_resume(Irp);
		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_init_driver: sftk_lg_ctl_fullrefresh_pause_resume() Failed with error %08x \n", status));
			//return status;
		}
		break;

	case IOC_CST(FTD_IOCTL_FAILOVER):
        status = sftk_lg_failover(Irp);
		if (!NT_SUCCESS(status))
		{ // Failed
			DebugPrint((DBG_ERROR, "sftk_init_driver: sftk_lg_failover() Failed with error %08x \n", status));
			//return status;
		}
        break;
#endif
#if 1 // NOT NEEDED ANYMORE
	// Followings are old IOCTL not supported anymore,.. returning error, caller means 
	// new service must not call this IOCTL anymore......
	case IOC_CST(FTD_GET_CONFIG):
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_GET_CONFIG:: Not implemented (Not needed, Why Service still calls this IOCTL???), returning error 0x%08x \n",status));
        break;
	
    case IOC_CST(FTD_CTL_CONFIG):
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_CTL_CONFIG:: Not implemented (Same as previouse code), returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_GET_DEV_STATE_BUFFER):
        status = sftk_ctl_get_dev_state_buffer(dev, cmd, arg, flag);
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_GET_DEV_STATE_BUFFER:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_GET_LG_STATE_BUFFER):
        status = sftk_ctl_get_lg_state_buffer(dev, cmd, arg, flag);
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_GET_LG_STATE_BUFFER:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_SET_DEV_STATE_BUFFER):
        status = sftk_ctl_set_dev_state_buffer(dev, cmd, arg, flag);
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_SET_DEV_STATE_BUFFER:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_SET_LG_STATE_BUFFER):
        status = sftk_ctl_set_lg_state_buffer(dev, cmd, arg, flag);
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_SET_LG_STATE_BUFFER:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_START_LG):
        status  = sftk_ctl_lg_start(dev, cmd, arg, flag);  // TODO : What to do here ??
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_START_LG:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;
  
    case IOC_CST(FTD_GET_DEVICE_STATS):	// struct stat_buffer_t and disk_stats_t
        status = sftk_ctl_lg_get_device_stats(dev, cmd, arg, flag);  
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_GET_DEVICE_STATS:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
		break;

    case IOC_CST(FTD_GET_GROUP_STATS): // struct stat_buffer_t and ftd_stat_t
		status = sftk_ctl_lg_get_group_stats(dev, cmd, arg, flag);  
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_GET_GROUP_STATS:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_CLEAR_BAB):
        status = sftk_ctl_lg_clear_bab(dev, cmd, arg, flag);  
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_CLEAR_BAB:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_CLEAR_LRDBS):
        status = sftk_ctl_lg_clear_dirtybits(arg, FTD_LOW_RES_DIRTYBITS);	// TODO : Do We need this ??
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_CLEAR_LRDBS:: implemented but Do we need this ? (Why Service still calls this IOCTL???), returning status 0x%08x \n",status));
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_CLEAR_LRDBS:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_CLEAR_HRDBS):
        status = sftk_ctl_lg_clear_dirtybits(arg, FTD_HIGH_RES_DIRTYBITS); // TODO : Do We need this ??
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_CLEAR_HRDBS:: implemented but Do we need this ? (Why Service still calls this IOCTL???), returning status 0x%08x \n",status));
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_CLEAR_HRDBS:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_GET_BAB_SIZE):
		status = sftk_ctl_get_bab_size(dev, cmd, arg, flag);	// TODO 
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_GET_BAB_SIZE:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_UPDATE_LRDBS):
        // status = ftd_ctl_update_lrdbs(arg);	// TODO : Do We need this ????
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_UPDATE_LRDBS:: NOT implemented, Do we need this ? (Why Service still calls this IOCTL???), returning status 0x%08x \n",status));
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_UPDATE_LRDBS:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;

    case IOC_CST(FTD_UPDATE_HRDBS):
        // status = ftd_ctl_update_hrdbs(arg);	// TODO : Do We need this ????
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_UPDATE_HRDBS:: NOT implemented, Do we need this ? (Why Service still calls this IOCTL???), returning status 0x%08x \n",status));
		status = STATUS_INVALID_DEVICE_REQUEST; // special internal error, same as previouse code !!!
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: FTD_UPDATE_HRDBS:: Not Supported anymore, ** BUG Caller FIXME FIXME *** Don't call this IOCTL Anymore!! returning error 0x%08x \n",status));
        break;
#endif // #if 1 // NOT NEEDED ANYMORE

#if 1 // PARAG : Added New IOCTLs for new driver 
	case IOC_CST(FTD_GET_PERF_INFO):
        status = sftk_ctl_get_perf_info( Irp );
        break;

	case IOC_CST(IOCTL_GET_MM_START): 
		status = Sftk_Ctl_MM_Start_And_Get_DB_Size( Irp );
        break;

	case IOC_CST(IOCTL_SET_MM_DATABASE_MEMORY): 
		status = Sftk_Ctl_MM_Set_DB_Size( Irp );
        break;

	case IOC_CST(IOCTL_MM_INIT_RAW_MEMORY): 
		status = Sftk_Ctl_MM_Init_Raw_Memory( Irp );
        break;

	case IOC_CST(IOCTL_MM_STOP): 
		status = Sftk_Ctl_MM_Stop( Irp );
        break;

	case IOC_CST(IOCTL_MM_CMD): 
		status = Sftk_Ctl_MM_Cmd( Irp );	// Wait till command gets to execute
        break;

	case IOC_CST(IOCTL_MM_ALLOC_RAW_MEM): 

		if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
		{ // Already Started
			status = STATUS_UNSUCCESSFUL;
			DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: IOCTL_MM_ALLOC_RAW_MEM %d == FALSE, returning error 0x%08x !!! \n",
											GSftk_Config.Mmgr.MM_Initialized, status));
			break;
		} // Already Started
		/* // SM_IPC_SUPPORT	
		// Signal SM Worker Thread to do Allocation of new Raw memory
		GSftk_Config.Mmgr.SM_EventCmd = SM_Event_Alloc;
		KeSetEvent( &GSftk_Config.Mmgr.SM_Event, 0, FALSE);
		*/
		SM_AddCmd_ForMM(SM_Event_Alloc);
		status = STATUS_SUCCESS;
        break;

	case IOC_CST(IOCTL_MM_FREE_RAW_MEM): 

		if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
		{ // Already Started
			status = STATUS_UNSUCCESSFUL;
			DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: IOCTL_MM_ALLOC_RAW_MEM %d == FALSE, returning error 0x%08x !!! \n",
											GSftk_Config.Mmgr.MM_Initialized, status));
			break;
		} // Already Started
		/*
		// Signal SM Worker Thread to do Allocation of new Raw memory
		GSftk_Config.Mmgr.SM_EventCmd = SM_Event_Free;
		KeSetEvent( &GSftk_Config.Mmgr.SM_Event, 0, FALSE);
		*/
		SM_AddCmd_ForMM(SM_Event_Free);
		status = STATUS_SUCCESS;
        break;
#endif // #if 1 // PARAG : Added New IOCTLs for new driver 

#if	MM_TEST_WINDOWS_SLAB
	case IOC_CST(FTD_TEST_MM_MAX_ALLOC):
		status = MM_LookAsideList_Test();
		if (!NT_SUCCESS(status))
		{ // Failed
			DebugPrint((DBG_ERROR, "sftk_ctl_ioctl: MM_LookAsideList_Test() Failed with error %08x \n", status));
		}
		// status = STATUS_SUCCESS;
		break;

	case IOC_CST(FTD_TEST_GET_MM_ALLOC_INFO):
		status = sftk_ctl_get_MM_Alloc_Info( Irp );
        break;
#endif // #if	MM_TEST_WINDOWS_SLAB

//This is Veera's Code
#if 1 // _PROTO_TDI_CODE_
		//Added the IOCTL's Here for processing all the Add , Remove , Enable , Disable , and 
		//Setting the TUNABLE's and Getting Performance

	case IOC_CST(SFTK_IOCTL_TCP_ADD_CONNECTIONS):
		{
			PSFTK_LG			pSftk_Lg;
			PCONNECTION_DETAILS	pConnection_detail = (PCONNECTION_DETAILS) arg;

			KdPrint(("Processing SFTK_IOCTL_TCP_ADD_CONNECTIONS IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum, pConnection_detail->lgCreationRole );
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_ADD_CONNECTIONS: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			status = COM_AddConnections( &pSftk_Lg->SessionMgr, pConnection_detail );
			if (!NT_SUCCESS(status))
			{ // LG Already exist
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_ADD_CONNECTIONS: COM_AddConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				break;
			}
		}
		break;
	case IOC_CST(SFTK_IOCTL_TCP_REMOVE_CONNECTIONS):
		{
			PSFTK_LG			pSftk_Lg;
			PCONNECTION_DETAILS	pConnection_detail = (PCONNECTION_DETAILS) arg;

			KdPrint(("Processing SFTK_IOCTL_TCP_REMOVE_CONNECTIONS IOCTL \n"));

			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum, pConnection_detail->lgCreationRole );
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_REMOVE_CONNECTIONS: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			status = COM_RemoveConnections( &pSftk_Lg->SessionMgr, pConnection_detail );
			if (!NT_SUCCESS(status))
			{ // LG Already exist
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_REMOVE_CONNECTIONS: COM_RemoveConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				break;
			}
		}
		break;
	case IOC_CST(SFTK_IOCTL_TCP_ENABLE_CONNECTIONS):
		{
			PSFTK_LG			pSftk_Lg;
			PCONNECTION_DETAILS	pConnection_detail = (PCONNECTION_DETAILS) arg;

			KdPrint(("Processing SFTK_IOCTL_TCP_ENABLE_CONNECTIONS IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum, pConnection_detail->lgCreationRole );
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_ENABLE_CONNECTIONS: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			status = COM_EnableConnections( &pSftk_Lg->SessionMgr, pConnection_detail ,TRUE);
			if (!NT_SUCCESS(status))
			{ // LG Already exist
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_ENABLE_CONNECTIONS: COM_EnableConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				break;
			}
		}
		break;
	case IOC_CST(SFTK_IOCTL_TCP_DISABLE_CONNECTIONS):
		{
			PSFTK_LG			pSftk_Lg;
			PCONNECTION_DETAILS	pConnection_detail = (PCONNECTION_DETAILS) arg;

			KdPrint(("Processing SFTK_IOCTL_TCP_DISABLE_CONNECTIONS IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum, pConnection_detail->lgCreationRole );
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pConnection_detail->lgnum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_DISABLE_CONNECTIONS: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			status = COM_EnableConnections( &pSftk_Lg->SessionMgr, pConnection_detail ,FALSE);
			if (!NT_SUCCESS(status))
			{ // LG Already exist
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_TCP_DISABLE_CONNECTIONS: COM_EnableConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pConnection_detail->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				break;
			}
		}
		break;
	case IOC_CST(SFTK_IOCTL_TCP_QUERY_SM_PERFORMANCE):
		{
		}
		break;
	case IOC_CST(SFTK_IOCTL_TCP_SET_CONNECTIONS_TUNABLES):
		{
		}
		break;
	case IOC_CST(SFTK_IOCTL_START_PMD):
		{
			PSFTK_LG			pSftk_Lg;
			PSM_INIT_PARAMS	pSMParams = (PSM_INIT_PARAMS) arg;

			KdPrint(("Processing SFTK_IOCTL_START_PMD IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pSMParams->lgnum, pSMParams->lgCreationRole );
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pSMParams->lgnum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_START_PMD: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pSMParams->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			if (pSMParams->nSendWindowSize <= CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime) ) 
			{
				pSMParams->nSendWindowSize = CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime);
			}
			if (pSMParams->nReceiveWindowSize <= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit,pSftk_Lg->NumOfPktsRecvAtaTime))
			{
				pSMParams->nReceiveWindowSize = CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit,pSftk_Lg->NumOfPktsRecvAtaTime);
			}
			
			status = COM_StartPMD( &pSftk_Lg->SessionMgr, pSMParams);
			if (!NT_SUCCESS(status))
			{ // LG Already exist
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_START_PMD: COM_StartPMD(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pSMParams->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				break;
			}
		}
		break;
	case IOC_CST(SFTK_IOCTL_STOP_PMD):
		{
			PSFTK_LG			pSftk_Lg;
#if TARGET_SIDE
			ftd_state_t			*pFtd_State_state_s = (ftd_state_t *) arg;
#else
			PLONG		pLGNum = (PLONG) arg;
#endif

			KdPrint(("Processing SFTK_IOCTL_STOP_PMD IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pFtd_State_state_s->lg_num, pFtd_State_state_s->lgCreationRole);
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( *pLGNum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
#if TARGET_SIDE
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_STOP_PMD: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pFtd_State_state_s->lg_num, status)); 
#else
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_STOP_PMD: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								*pLGNum, status)); 
#endif
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			COM_StopPMD( &pSftk_Lg->SessionMgr);
		}
		break;
	case IOC_CST(SFTK_IOCTL_START_RMD):
		{
			PSFTK_LG			pSftk_Lg;
			PSM_INIT_PARAMS	pSMParams = (PSM_INIT_PARAMS) arg;

			KdPrint(("Processing SFTK_IOCTL_START_RMD IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pSMParams->lgnum,  pSMParams->lgCreationRole);
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pSMParams->lgnum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_START_RMD: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pSMParams->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			status = COM_StartRMD( &pSftk_Lg->SessionMgr, pSMParams);
			if (!NT_SUCCESS(status))
			{ // LG Already exist
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_START_RMD: COM_StartRMD(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pSMParams->lgnum, status)); 
				// TODO : Call Cache API to initialize Glbal Cache manager.
				break;
			}
		}
		break;
	case IOC_CST(SFTK_IOCTL_STOP_RMD):
		{
			PSFTK_LG			pSftk_Lg;
#if TARGET_SIDE
			ftd_state_t			*pFtd_State_state_s = (ftd_state_t *) arg;
#else
			PLONG pLGNum = (PLONG) arg;
#endif


			KdPrint(("Processing SFTK_IOCTL_STOP_RMD IOCTL \n"));
			// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
			pSftk_Lg = sftk_lookup_lg_by_lgnum( pFtd_State_state_s->lg_num, pFtd_State_state_s->lgCreationRole);
#else
			pSftk_Lg = sftk_lookup_lg_by_lgnum( *pLGNum );
#endif
			if (pSftk_Lg == NULL) 
			{ // LG Already exist
				status = STATUS_INVALID_PARAMETER;
#if TARGET_SIDE
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_STOP_RMD: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								pFtd_State_state_s->lg_num, status)); 
#else
				DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_STOP_RMD: sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
								*pLGNum, status)); 
#endif
				// TODO : Call Cache API to initialize Glbal Cache manager.
				return status;
			}
			// Call Veera's API like:
			COM_StopRMD( &pSftk_Lg->SessionMgr);
		}
		break;
#endif // #if 1 // _PROTO_TDI_CODE_

    default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		DebugPrint((DBG_ERROR, "sftk_ctl_ioctl() : default IoControlCode - %x , returning status 0x%08x \n", cmd, status)); 
        break;
    } // switch (IOC_CST(cmd)) 
    
    return (status);
} // sftk_ctl_ioctl()


NTSTATUS
sftk_lg_ioctl(dev_t dev, int cmd, int arg, int flag)
{
    NTSTATUS	status		= STATUS_SUCCESS;
	PSFTK_LG	pSftk_Lg	= NULL;

    if(!OS_IsFlagSet(GSftk_Config.Flag, SFTK_CONFIG_FLAG_CACHE_CREATED) )
    { // Global Cache Manager is not initialized, we can not do anything ....
		// TODO : we must try here to initialize Global Cache Manager....
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl() Flag is not set SFTK_CONFIG_FLAG_CACHE_CREATED, returning status 0x%08x \n", status)); 
		// TODO : Call Cache API to initialize Glbal Cache manager.
        return status;
    }

	// Retrieve Specified LG Device from its LG Number
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( dev, (ROLE_TYPE) flag );
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( dev );
#endif
    if (pSftk_Lg == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: specified Lgdev number = %d Not Found!!! returning error 0x%08x \n",
							dev, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }
    // minor_t minor = getminor(dev) & ~FTD_LGFLAG;

    switch (IOC_CST(cmd)) 
    {
    case IOC_CST(FTD_OLDEST_ENTRIES):
        // status = ftd_lg_oldest_entries(pSftk_Lg, arg, flag);
		// We do not need this since it returns from specified offset of BAB memory to user mode 
		// We do not share BAB memory anymore with USer mode Service since PMD is moved to Kernel.
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_OLDEST_ENTRIES:: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL???, This copy BAB mem from kernel to user !!), returning status 0x%08x \n",status));	
        break;

    case IOC_CST(FTD_MIGRATE):
		// status = ftd_lg_migrate(pSftk_Lg, arg, flag);
		// user mode service informs to Kernel BAB that specified Length of BAB is sent and comiited on secondary side 
		// so free this bab memory and update statestics for this.
		// Do Not Need this anymore
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_MIGRATE:: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL???, This frees Kernel specified BAB mem since data is comited on secondary side!!), returning status 0x%08x \n",status));	
        break;

    case IOC_CST(FTD_GET_LRDBS):
    case IOC_CST(FTD_GET_HRDBS):
		// status = ftd_lg_get_dirty_bits(pSftk_Lg, cmd, arg, flag);
		// Copies specified Bitmaps to caller specified buffer for every devices under specified LG
		// Service PMD used to get this bitmap for smart refresh mode, 
		// Do Not Need this anymore
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_GET_LRDBS OR FTD_GET_HRDBS 0x%08x :: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL???, This copies bitmap from Kernel to service memory!!), returning status 0x%08x \n",
							cmd, status));	
        break;
        
    case IOC_CST(FTD_SET_LRDBS):
    case IOC_CST(FTD_SET_HRDBS):
		// status = ftd_lg_set_dirty_bits(pSftk_Lg, cmd, arg, flag);
		// Copies Bitmaps from caller specified buffer to kenrel for every devices under specified LG
		// Service PMD used to set this bitmap for smart refresh mode, 
		// Do Not Need this anymore
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_SET_LRDBS OR FTD_SET_HRDBS 0x%08x :: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL ???, This copies bitmap from user to kenrel memory!!), returning status 0x%08x \n",
							cmd, status));	
        break;

    case IOC_CST(FTD_GET_LRDB_INFO):
    case IOC_CST(FTD_GET_HRDB_INFO):
        // status = ftd_lg_get_dirty_bit_info(pSftk_Lg, cmd, arg, flag);
		// Copies specified Bitmaps to caller specified buffer for every devices under specified LG
		// Service PMD used to get this bitmap for smart refresh mode, 
		// Do Not Need this anymore
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_GET_LRDB_INFO OR FTD_GET_HRDB_INFO 0x%08x :: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL ???, This copies bitmap from kernel to user memory!!), returning status 0x%08x \n",
							cmd, status));	
        break;

    case IOC_CST(FTD_UPDATE_DIRTYBITS):
		// status = ftd_lg_update_dirtybits(pSftk_Lg);
		// Copies Bitmaps from caller specified buffer to kenrel for every devices under specified LG
		// Service PMD used to set this bitmap for smart refresh mode, 
		// Do Not Need this anymore
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_UPDATE_DIRTYBITS OR FTD_UPDATE_DIRTYBITS 0x%08x :: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL ???, This copies bitmap from user to kenrel memory!!), returning status 0x%08x \n",
							cmd, status));	
        break;

    case IOC_CST(FTD_SEND_LG_MESSAGE):	// TODO : When this IOCTL gets call from service.
        // status = ftd_lg_send_lg_message(pSftk_Lg, cmd, arg, flag);
		// Service sends Packets to Kernel , kernel allocates BAB with header and copies this data to bab....
		// Do Not Need this anymore
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_SEND_LG_MESSAGE:: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL ???, This copies packets from user to kenrel memory into BAB!!), returning status 0x%08x \n",
							status));	
        break;

    case IOC_CST(FTD_INIT_STOP):
        // status = ftd_ctl_init_stop(pSftk_Lg);
		// This calls gets send before LG gets deleted. 
		// This Call marks each and every devices under LG with flag FTD_STOP_IN_PROGRESS. IF any devices under 
		// LG has flag marked with FTD_DEV_OPEN than this IOCTL gets failed with error .
		// Do Not Need this anymore, since we don't care of deletion of device, we can do at anytime now.
		// PNP supports makes this enable.
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: FTD_INIT_STOP:: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL ???, This prepares all devices under LG with FTD_STOP_IN_PROGRESS, no needed anymore!!), returning status 0x%08x \n",
							status));	
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
		DebugPrint((DBG_ERROR, "sftk_lg_ioctl: default 0x%08x :: NOT implemented, NOT NEEDED ANYMORE? (Why Service still calls this IOCTL ???, returning status 0x%08x \n",
							cmd, status));	
        break;
    }
    
    return status;
} // sftk_lg_ioctl()



