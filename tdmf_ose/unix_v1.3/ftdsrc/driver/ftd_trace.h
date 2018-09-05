/**************************************************************************
 * ftd_trace.h
 *
 *   (c) Copyright 2001 Fujitsu Software Technology Corporation
 *
 *   Description:  header file for activating trace in TDMF
 *
 "  History:
 *   09/14/2001 - Pierre Bouchard - original code
 *
 ***************************************************************************/

/*#define TDMF_TRACE */

#ifdef TDMF_TRACE
	#ifdef TDMF_TRACE_MAKER
		struct tdmf_ioctl_text
			{
			ftd_int32_t cmd;
			char cmd_txt[30];
			};

		struct tdmf_ioctl_text	tdmf_ioctl_text_array[]= 
		{
		{FTD_GET_CONFIG	     				  ,"FTD_GET_CONFIG              "},					       
		{FTD_NEW_DEVICE          			  ,"FTD_NEW_DEVICE              "},
		{FTD_NEW_LG              			  ,"FTD_NEW_LG                  "},
		{FTD_DEL_DEVICE          			  ,"FTD_DEL_DEVICE              "},
		{FTD_DEL_LG              			  ,"FTD_DEL_LG                  "},
		{FTD_CTL_CONFIG          			  ,"FTD_CTL_CONFIG              "},
		{FTD_GET_DEV_STATE_BUFFER			  ,"FTD_GET_DEV_STATE_BUFFER    "},
		{FTD_GET_LG_STATE_BUFFER 			  ,"FTD_GET_LG_STATE_BUFFER     "},
		{FTD_SEND_LG_MESSAGE     			  ,"FTD_SEND_LG_MESSAGE         "},
		{FTD_OLDEST_ENTRIES      			  ,"FTD_OLDEST_ENTRIES          "},
		{FTD_MIGRATE             			  ,"FTD_MIGRATE                 "},
		{FTD_GET_LRDBS           			  ,"FTD_GET_LRDBS               "},
		{FTD_GET_HRDBS           			  ,"FTD_GET_HRDBS               "},
		{FTD_SET_LRDBS           			  ,"FTD_SET_LRDBS               "},
		{FTD_SET_HRDBS           			  ,"FTD_SET_HRDBS               "},
		/*#ifdef DEPRECATED*/
			{FTD_GET_LRDB_INFO       		  ,"FTD_GET_LRDB_INFO           "},
			{FTD_GET_HRDB_INFO       		  ,"FTD_GET_HRDB_INFO           "},
		/*#endif 	 */
		{FTD_GET_DEVICE_NUMS     			  ,"FTD_GET_DEVICE_NUMS         "},
		{FTD_SET_DEV_STATE_BUFFER			  ,"FTD_SET_DEV_STATE_BUFFER    "},
		{FTD_SET_LG_STATE_BUFFER 			  ,"FTD_SET_LG_STATE_BUFFER     "},
		{FTD_START_LG            			  ,"FTD_START_LG                "},
		/*#ifdef DEPRECATED */
			{FTD_GET_NUM_DEVICES     		  ,"FTD_GET_NUM_DEVICES         "},
			{FTD_GET_NUM_GROUPS      		  ,"FTD_GET_NUM_GROUPS          "},
		/* #endif */
		{FTD_GET_DEVICES_INFO    			  ,"FTD_GET_DEVICES_INFO        "},
		{FTD_GET_GROUPS_INFO     			  ,"FTD_GET_GROUPS_INFO         "},
		{FTD_GET_DEVICE_STATS    			  ,"FTD_GET_DEVICE_STATS        "},
		{FTD_GET_GROUP_STATS     			  ,"FTD_GET_GROUP_STATS         "},
		{FTD_SET_GROUP_STATE     			  ,"FTD_SET_GROUP_STATE         "},
		{FTD_UPDATE_DIRTYBITS    			  ,"FTD_UPDATE_DIRTYBITS        "},
		{FTD_CLEAR_BAB           			  ,"FTD_CLEAR_BAB               "},
		{FTD_CLEAR_HRDBS         			  ,"FTD_CLEAR_HRDBS             "},
		{FTD_CLEAR_LRDBS         			  ,"FTD_CLEAR_LRDBS             "},
		{FTD_GET_GROUP_STATE     			  ,"FTD_GET_GROUP_STATE         "},
		{FTD_GET_BAB_SIZE        			  ,"FTD_GET_BAB_SIZE            "},
		{FTD_UPDATE_LRDBS        			  ,"FTD_UPDATE_LRDBS            "},
		{FTD_UPDATE_HRDBS        			  ,"FTD_UPDATE_HRDBS            "},
		{FTD_SET_SYNC_DEPTH      			  ,"FTD_SET_SYNC_DEPTH          "},
		{FTD_SET_IODELAY         			  ,"FTD_SET_IODELAY             "},
		{FTD_SET_SYNC_TIMEOUT    			  ,"FTD_SET_SYNC_TIMEOUT        "},
		{FTD_CTL_ALLOC_MINOR     			  ,"FTD_CTL_ALLOC_MINOR         "}
		#ifdef HPUX
			,
			{FTD_PANIC			  			  ,"FTD_PANIC                   "}
		#endif
		};

		#define MAX_TDMF_IOCTL 	(sizeof (tdmf_ioctl_text_array)/ sizeof (struct tdmf_ioctl_text))
	#endif	
#endif

