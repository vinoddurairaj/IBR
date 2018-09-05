using System;
using System.Runtime.Serialization;
using System.Runtime.InteropServices;

namespace MasterThread
{

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct disk_stats_t {
		public string devname;
		public int localbdisk;	/* block device number */
		public uint localdisksize;
		public int wlentries ;
		public int wlsectors ;
		public Int64 readiocnt;
		public Int64 writeiocnt;
		public Int64 sectorsread;
		public Int64 sectorswritten;
	}

	/* ftd_stat_s & ftd_stat_t */
	/// <summary>
	/// ftd_stat_t
	/// </summary>
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct Statistics_t {
		public int lgnum;
		public int connected;
		/* Statistics */
		public uint loadtimesecs ;			/* in seconds */
		public uint loadtimesystics ;      /* in sys ticks */
		public int wlentries ;
		public int wlsectors ;
		public int bab_free;
		public int bab_used;
		public eLgModes state;
		public int ndevs;
		public int sync_depth;
		public uint sync_timeout;
		public uint iodelay;
	}

	/// <summary>
	/// logical group tunable parameters ftd_tune_t
	/// </summary>
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct Tunables {
		public int chunksize;				/* TUNABLE: default - 256 Kbyte          */
		public int statinterval;			/* TUNABLE: default - 10 seconds         */
		public int maxstatfilesize;			/* TUNABLE: default - 64 Kbyte           */
		public int stattofileflag;			/* TUNABLE: default - 1                  */
		int tracethrottle;					/* TUNABLE: default - 0                  */
		public int syncmode;				/* TUNABLE: default - 0 sync mode switch */
		public int syncmodedepth;			/* TUNABLE: default - 1 deep queue       */
		public int syncmodetimeout;			/* TUNABLE: default - 30 seconds         */
		public int compression;				/* TUNABLE: default - none               */
		public int tcpwindowsize;			/* TUNABLE: default - 256 Kbyte          */
		public int netmaxkbps;				/* TUNABLE: default - -1                 */
		public int chunkdelay;				/* TUNABLE: default - 0                  */
		public int iodelay;					/* TUNABLE: default - 0                  */
		public int refrintrvl;				/* TUNABLE: default - 60 seconds         */
	}

	/*
 * The relevant fields for a version 1 persistent store
 */
//	public struct ps_version_1_attr_t {
//		public int max_dev;
//		public int max_group;
//		public int dev_attr_size;
//		public int group_attr_size;
//		public int lrdb_size;
//		public int hrdb_size;
//		public int dev_table_entry_size;
//		public int group_table_entry_size;
//		public int num_device;
//		public int num_group;
//		public int last_device;
//		public int last_group;
//	}

//	/* State stamp for group */
//	public enum eClusterActive {
//		INACTIVE = 0,
//		ACTIVE = 2
//	}

	/* device level statistics structure */
	public struct ftd_dev_stat_t 
	{
		public int devid;				/* device id                             */
		public int actual;				/* device actual (bytes)                 */
		public int effective;			/* device effective (bytes)              */
		public int entries;				/* device entries                        */
		public int entage;				/* time between BAB write and RMD recv   */
		public int jrnentage;			/* time between jrnl write and mir write */
		public int rsyncoff;			/* rsync sectors done                    */
		public int rsyncdelta;			/* rsync sectors changed                 */
		public float actualkbps;		/* kbps transfer rate                    */
		public float effectkbps;		/* effective transfer rate w/compression */
		public float pctdone;			/* % of refresh or backfresh complete    */
		public int sectors;				/* # of sectors in bab                   */
		public int pctbab;				/* pct of bab in use                     */
		public double local_kbps_read;	/* ftd device kbps read                  */
		public double local_kbps_written;	/* ftd device kbps written               */
		public ulong ctotread_sectors;
		public ulong ctotwrite_sectors;
	}

	/// <summary>
	/// info stored for each group
	/// ps_group_info_t
	/// </summary>
	public struct GroupInfo
	{
		public int		lgnum;
		public string	name;			/* group name */
		public eLgModes	state;			/* state of the group */
		public uint		hostid;			/* host id that owns this group */
		public bool ClusterActive;	// is this system the cluster active system, INACTIVE - ACTIVE
		public bool	checkpoint;		/* checkpoint flag */
	}

	/* used to change the state of a device */
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct ftd_state_t 
	{
		public int	lg_num;
		public int	state;
	}

	public struct ftd_param_t 
	{
		public int lgnum;
		public int value1;
	}

	//This Structure is passed to the Client
	//Client Spawns one Worker Thread per Socket and then calls the Cache Manager to Give Data
	//If No Data is available the Worker Threads Just Wait and Sleep
	//The Therads will wakeup either when there is some data in the cache
	//OR the User issues a Kill Operation stopping all operations.
	//The Worker Thread Calls the TDI_SEND once there is data in the Cache.
	//All the Worker Threads are locked using a FAST_MUTEX until an Event is signalled 
	//for the data in the Cache.

	public struct TDI_ADDRESS_IP {
		public ushort sin_port;
		public uint  in_addr;
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=8)]
		public string  sin_zero;
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct CONNECTION_INFO {
		public ushort			nNumberOfSessions;
		public TDI_ADDRESS_IP	ipLocalAddress;
		public TDI_ADDRESS_IP	ipRemoteAddress;
	}

	//This Structure is used to ( Add | Enable | Disable | Remove ) Connections to the System
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct CONNECTION_DETAILS {
		public int		lgnum;					// Logical Group Number 
		public int		nSendWindowSize;		//This is the Window Size that will be used per Socket Connection in MB
		public int		nReceiveWindowSize;
		public ushort	nConnections;
		public CONNECTION_INFO ConnectionDetails;
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct TDI_CONNECTION_INFO 
	{
		public uint State;                        // current state of the connection.
		public uint Event;                        // last event on the connection.
		public uint TransmittedTsdus;             // TSDUs sent on this connection.
		public uint ReceivedTsdus;                // TSDUs received on this connection.
		public uint TransmissionErrors;           // TSDUs transmitted in error/this connection.
		public uint ReceiveErrors;                // TSDUs received in error/this connection.
		public ulong Throughput;           // estimated throughput on this connection.
		public ulong Delay;                // estimated delay on this connection.
		public uint SendBufferSize;               // size of buffer for sends - only
		// meaningful for internal buffering
		// protocols like tcp
		public uint ReceiveBufferSize;            // size of buffer for receives - only
		// meaningful for internal buffering
		// protocols like tcp
		public bool Unreliable;                 // is this connection "unreliable".
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	//This structure contains the Performnace Metrics for one Connection
	public struct CONNECTION_PERFORMANCE_INFO{
		public TDI_ADDRESS_IP ipLocalAddress;
		public TDI_ADDRESS_IP ipRemoteAddress;
		public TDI_CONNECTION_INFO connectionInfo;
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct TDI_PROVIDER_INFO 
	{
		public uint Version;                      // TDI version: 0xaabb, aa=major, bb=minor
		public uint MaxSendSize;                  // max size of user send.
		public uint MaxConnectionUserData;        // max size of user-specified connect data.
		public uint MaxDatagramSize;              // max datagram length in bytes.
		public uint ServiceFlags;                 // service options, defined below.
		public uint MinimumLookaheadData;         // guaranteed min size of lookahead data.
		public uint MaximumLookaheadData;         // maximum size of lookahead data.
		public uint NumberOfResources;            // how many TDI_RESOURCE_STATS for provider.
		public ulong StartTime;            // when the provider became active.
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct TDI_PROVIDER_RESOURCE_STATS {
		public uint ResourceId;                   // identifies resource in question.
		public uint MaximumResourceUsed;          // maximum number in use at once.
		public uint AverageResourceUsed;          // average number in use.
		public uint ResourceExhausted;            // number of times resource not available.
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct TDI_PROVIDER_STATISTICS 
	{
		public uint Version;                      // TDI version: 0xaabb, aa=major, bb=minor
		public uint OpenConnections;              // currently active connections.
		public uint ConnectionsAfterNoRetry;      // successful connections, no retries.
		public uint ConnectionsAfterRetry;        // successful connections after retry.
		public uint LocalDisconnects;             // connection disconnected locally.
		public uint RemoteDisconnects;            // connection disconnected by remote.
		public uint LinkFailures;                 // connections dropped, link failure.
		public uint AdapterFailures;              // connections dropped, adapter failure.
		public uint SessionTimeouts;              // connections dropped, session timeout.
		public uint CancelledConnections;         // connect attempts cancelled.
		public uint RemoteResourceFailures;       // connections failed, remote resource problems.
		public uint LocalResourceFailures;        // connections failed, local resource problems.
		public uint NotFoundFailures;             // connections failed, remote not found.
		public uint NoListenFailures;             // connections rejected, we had no listens.
		public uint DatagramsSent;
		public ulong DatagramBytesSent;
		public uint DatagramsReceived;
		public ulong DatagramBytesReceived;
		public uint PacketsSent;                  // total packets given to NDIS.
		public uint PacketsReceived;              // total packets received from NDIS.
		public uint DataFramesSent;
		public ulong DataFrameBytesSent;
		public uint DataFramesReceived;
		public ulong DataFrameBytesReceived;
		public uint DataFramesResent;
		public ulong DataFrameBytesResent;
		public uint DataFramesRejected;
		public ulong DataFrameBytesRejected;
		public uint ResponseTimerExpirations;     // e.g. T1 for Netbios
		public uint AckTimerExpirations;          // e.g. T2 for Netbios
		public uint MaximumSendWindow;            // in bytes
		public uint AverageSendWindow;            // in bytes
		public uint PiggybackAckQueued;           // attempts to wait to piggyback ack.
		public uint PiggybackAckTimeouts;         // times that wait timed out.
		public ulong WastedPacketSpace;    // total amount of "wasted" packet space.
		public uint WastedSpacePackets;           // how many packets contributed to that.
		public uint NumberOfResources;            // how many TDI_RESOURCE_STATS follow.
		public TDI_PROVIDER_RESOURCE_STATS ResourceStats;    // variable array of them.
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	//This Structure contains the Preformance metrics for all the Connections in a Session Manager
	public struct SM_PERFORMANCE_INFO{
		public int							lgnum;										// Logical Group Number 
		public TDI_PROVIDER_INFO			providerInfo;				// Provides the Falgs that are supported by the TCP
		public TDI_PROVIDER_STATISTICS		providerStatistics;			// Provides Live Statistics that are supported 
		// by TCP
		public int						nConnections;			// Number of Connection Performance Information Structures
		public CONNECTION_PERFORMANCE_INFO conPerformanceInfo;	// The Connection Performance Array
	}

	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	//This will be sent along with the LAUNCH_PMD as the initilization sequence
	public struct SM_INIT_PARAMS{
		public int		lgnum;					// Logical Group Number 
		public uint		nSendWindowSize;			// The Size of the TDI Send Window 
		public ushort	nMaxNumberOfSendBuffers;	// The Max Number of Send Buffers that needs to be allocated
		public uint		nReceiveWindowSize;		// The Size of the TDI Receive Window
		public ushort	nMaxNumberOfReceiveBuffers;	// The Max Number of Receive Buffers that needs to be allocated
		public uint		nChunkSize;				// This is a test parameter, specifies the Send Chunks
		// By default this will be Zero.
		public uint		nChunkDelay;				// The time in milliseconds that will be used to Throttle,
		// data sent over the wire.
	}

	/// <summary>
	/// Following structure get passed as input paraneter for IOCTL_CONFIG_BEGIN
	/// </summary>
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public struct CONFIG_BEGIN_INFO {
		public uint HostId;				// The HostId that us used to install, uses for PMD to RMD Protocol communications
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)]
		public string Version;			// The Version of the Product, uses for PMD to RMD Protocol communications
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=64)]
		public string SystemName;		// computer name for local system...needed for pstore file access in cluster....
	}

	/// <summary>
	/// Summary description for DriverIOCTL.
	/// </summary>
	public class DriverIOCTL {
		//const string DRIVER_NAME = "\\\\.\\Global\\DTC" + "\\ctl";
		//const string DRIVER_NAME = "\\\\.\\Global\\SFTK" + "\\ctl";
		const string DRIVER_NAME = "\\\\.\\SFTK" + "\\ctl";


		IntPtr m_DriverHandle = IntPtr.Zero;

		public const int PS_GROUP_ATTR_SIZE		= 4*1024;
		public const int PS_DEVICE_ATTR_SIZE	= 4*1024;

		/// <summary>
		/// stat buffer descriptor
		/// </summary>
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public unsafe struct Stat_Buffer_t {
			public int lg_num;			/* logical group device id */
			public int dev_num;		/* device id. used only for GET_DEV_STATS */
			public int len;			/* length in bytes */
			public void* buffer;		/* pointer to the buffer */
		}

		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public struct ftd_devnum_t {
			public int b_major;
			public int c_major;
		}

		/// <summary>
		/// Constructor
		/// </summary>
		public DriverIOCTL() {
			// get a handle to the driver
			m_DriverHandle = Createfile.OpenDriver( DRIVER_NAME );
			if ( m_DriverHandle.ToInt32() == -1 ) {
				throw new OmException( "Driver NOT installed" );
			}
		}

		~DriverIOCTL() {
			if ( m_DriverHandle != IntPtr.Zero ) {
				Createfile.Close ( m_DriverHandle );
				m_DriverHandle = IntPtr.Zero;
			}
		}

		public void Close() {
			if ( m_DriverHandle != IntPtr.Zero ) {
				if ( m_DriverHandle.ToInt32() != -1 ) {
					Createfile.Close ( m_DriverHandle );
				}
				m_DriverHandle = IntPtr.Zero;
			}
		}

		/// <summary>
		/// Get the group state from the driver.
		/// ftd_ioctl_get_lg_state_buffer
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="buf"></param>
		/// <returns></returns>
		public unsafe void GetLgStateBuffer( int lgnum, out Tunables tunables ) {
			// allocate the return buffer
			tunables = new Tunables();

			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.len = PS_GROUP_ATTR_SIZE;
			statCmdBuf.buffer = (void*)Marshal.AllocHGlobal( PS_GROUP_ATTR_SIZE );

			int size = (int)(Marshal.SizeOf( typeof(Stat_Buffer_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				// read state from driver
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_LG_STATE_BUFFER, ptr, size, ptr, ref size);
				byte * pstatCmdBuf = (byte*)ptr.ToPointer();
				// bump up with 12 bytes
				pstatCmdBuf = pstatCmdBuf + 12;
				tunables = (Tunables)Marshal.PtrToStructure( (IntPtr)(pstatCmdBuf), typeof ( Tunables )  );

			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetLgState: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
				Marshal.FreeHGlobal( (IntPtr)statCmdBuf.buffer );
			}
		}

		/// <summary>
		/// SetLgStateBuffer -- Set the group state from the driver.
		/// ftd_ioctl_set_lg_state_buffer
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="buf"></param>
		/// <returns></returns>
		/// 
		public unsafe void SetLgStateBuffer( int lgnum, Tunables tunables ) {

			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.len = PS_GROUP_ATTR_SIZE;
			statCmdBuf.buffer = &tunables;

			int size = (int)(Marshal.SizeOf( typeof(Stat_Buffer_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				// read state from driver
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_LG_STATE_BUFFER, ptr, size, ptr, ref size);

			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetLgStateBuffer: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/*
		 * ftd_ioctl_get_group_stats -- returns the group stats from the driver
		 */
		public unsafe void GetGroupStatistics( int lgnum, ref Statistics_t statistics ) {
			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.len = Marshal.SizeOf( typeof(Statistics_t) );
			statCmdBuf.buffer = (void*)Marshal.AllocHGlobal( Marshal.SizeOf( typeof(Statistics_t) ) );

			int size = (int)(Marshal.SizeOf( typeof(Stat_Buffer_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_GROUP_STATS, ptr, size, ptr, ref size);
				byte * pstatCmdBuf = (byte*)ptr.ToPointer();
				// bump up with 12 bytes
				pstatCmdBuf = pstatCmdBuf + 12;
				Marshal.PtrToStructure( (IntPtr)(pstatCmdBuf), statistics );
				pstatCmdBuf += Marshal.SizeOf( typeof(Stat_Buffer_t) );
			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetGroupStatistics: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// Get the device info for a device
		/// ps_get_group_info
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="info"></param>
		public void GetGroupInfo( int lgnum, ref GroupInfo info ) {
			//			HANDLE				fd;
			//			int              ret, index;
			//			ps_hdr_t         hdr;
			//			ps_group_entry_t *table;
			//
			//			/* open the store */
			//			ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
			//			if (ret != PS_OK) {
			//				return (ret);
			//			}
			//			FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
			//
			//			if (info->name == NULL) {
			//				info->name = table[index].path;
			//			}
			//			info->hostid = table[index].hostid;
			//			info->state = table[index].state;
			//			info->state_stamp = table[index].state_stamp;
			//			info->checkpoint = table[index].checkpoint;
			//
			//			free(table);
			//
		}

		//		/// <summary>
		//		/// ftd_ioctl_send_lg_message -- Insert a sentinel into the BAB.
		//		/// </summary>
		//		/// <param name="devfd"></param>
		//		/// <param name="lgnum"></param>
		//		/// <param name="sentinel"></param>
		//		/// <returns></returns>
		//		public int ftd_ioctl_send_lg_message( IntPtr devfd, int lgnum, string sentinel ) {
		//			//			stat_buffer_t	sb;
		//			//			string msgbuf;
		//			//			int maxtries, rc, i, sent, errnum;
		//			//
		//			//			memset(msgbuf, 0, sizeof(msgbuf));
		//			//			strcpy(msgbuf, sentinel); 
		//			//			memset(&sb, 0, sizeof(stat_buffer_t));
		//			//    
		//			//			sb.lg_num = lgnum;
		//			//			sb.len = sizeof(msgbuf);
		//			//			sb.addr = msgbuf;
		//			//
		//			//			maxtries = 50;
		//			//			sent = 0;
		//			//    
		//			//			for (i = 0; i < maxtries; i++) {
		//			//				rc = ftd_ioctl(devfd, FTD_SEND_LG_MESSAGE, &sb, sizeof(stat_buffer_t));
		//			//				if (rc != 0) {
		//			//					errnum = ftd_errno();
		//			//					if (errnum == EAGAIN) {
		//			//						sleep(50);
		//			//						continue;
		//			//					}
		//			//					if (errnum == ENOENT) {
		//			//						reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, "BAB FULL - cannot allocate entry\n");
		//			//						return -1;
		//			//					}
		//			//					else {
		//			//						reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, strerror(errnum));
		//			//						return -1;
		//			//					}
		//			//				} 
		//			//				else {
		//			//					sent = 1;
		//			//					break;
		//			//				}
		//			//			}
		//			//
		//			//			if ( !sent ) {
		//			//				reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, "BAB busy clearing memory - cannot allocate entry\n");
		//			//				return -1;
		//			//			}
		//
		//			return 0;
		//		}
		//





		//
		//		/*
		//		 * A common action of almost all public group functions. Open the 
		//		 * persistent store, read the header, verify the header magic number, 
		//		 * read the group table, and search the group table for a matching path. 
		//		 * Returns group index, header info, and open file descriptor, 
		//		 * if no errors occur. 
		//		 */
		//		public int Open_Ps_Get_Group_Info( string ps_name, string group_name, IntPtr outfd, ps_hdr_t hdr, int group_index, ps_group_entry_t ret_table ) {
		//
		//			HANDLE			fd;
		//			int              i, pathlen;
		//			unsigned int     table_size;
		//			ps_group_entry_t *table;
		//
		//			*group_index = -1;
		//			*outfd = (HANDLE)-1;
		//
		//		
		//			/* get the device number */
		//			if ((pathlen = strlen(group_name)) == 0) {
		//				error_tracef(TRACEERR,"open_ps_get_group_info(): PS_BOGUS_DEVICE_NAME");		
		//				return PS_BOGUS_DEVICE_NAME;
		//			}
		//
		//			/* open the store */
		//			if ((fd = ftd_open(ps_name, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
		//				error_tracef(TRACEERR,"open_ps_get_group_info(): PS_BOGUS_PS_NAME");		
		//				return PS_BOGUS_PS_NAME;
		//			}
		//
		//			/* seek to the header location */
		//			ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);
		//
		//			/* read the header */
		//			ftd_read(fd, hdr, sizeof(ps_hdr_t));
		//    
		//			if (hdr->magic != PS_VERSION_1_MAGIC) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
		//				error_tracef(TRACEERR,"open_ps_get_group_info(): PS_BOGUS_HEADER");
		//				return PS_BOGUS_HEADER;
		//			}
		//
		//			table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr->data.ver1.max_group);
		//			if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				error_tracef( TRACEERR,"open_ps_get_group_info(): PS_MALLOC_ERROR");
		//				return PS_MALLOC_ERROR;
		//			}
		//
		//			/* get the device index array */
		//			if (ftd_llseek(fd, hdr->data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
		//				free(table);
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				error_tracef(TRACEERR,"open_ps_get_group_info(): PS_SEEK_ERROR");
		//				return PS_SEEK_ERROR;
		//			}
		//			if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
		//				free(table);
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				error_tracef(TRACEERR,"open_ps_get_group_info(): PS_READ_ERROR");
		//				return PS_READ_ERROR;
		//			}
		//
		//			/* search for the device number */
		//			for (i = 0; i < (int)hdr->data.ver1.max_group; i++) {
		//				if (    (table[i].pathlen == pathlen)
		//					&& (strncmp(table[i].path, group_name, pathlen) == 0) ) {
		//
		//					/* the only GOOD way out of here */
		//					*group_index = i;
		//					*outfd = fd;
		//					if (ret_table != NULL) {
		//						*ret_table = table;
		//					} else {
		//						free(table);
		//					}
		//					return PS_OK;
		//				}
		//			}
		//			free(table);
		//			FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//
		//			return PS_GROUP_NOT_FOUND;
		//		}



		public static void SetLogicalGroupGState( int lgnum, eLgModes state ) {
			// send the state to the driver
			DriverIOCTL driver = new DriverIOCTL();
			try {
				driver.SetGroupState( lgnum, state );
			} finally {
				driver.Close();
			}
		}


		/// <summary>
		/// ftd_ioctl_set_group_state -- sets the group state in the driver.
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="state"></param>
		public void SetGroupState( int lgnum, eLgModes state ) {
			ftd_state_t	lgState = new ftd_state_t();
			lgState.lg_num = lgnum;
			lgState.state = (int)state;

			int size = (int)(Marshal.SizeOf( typeof(ftd_state_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( lgState, ptr, false );
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_GROUP_STATE, ptr, size, ptr, ref size );
			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetGroupState: Error detected - " + e.Message, e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// ftd_ioctl_get_group_state -- returns the group state from the driver
		/// </summary>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public unsafe eLgModes GetGroupState( int lgnum ) {
			ftd_state_t	lgState = new ftd_state_t();
			lgState.lg_num = lgnum;

			int size = (int)(Marshal.SizeOf( typeof(ftd_state_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( lgState, ptr, false );
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_GROUP_STATE, ptr, size, ptr, ref size);

				byte * bPtr = (byte*)ptr.ToPointer();
				Marshal.PtrToStructure( (IntPtr)(bPtr), lgState );
				return (eLgModes)lgState.state;
			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetGroupState: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		//		/*
		//		 * ftd_ioctl_clear_lrdbs --
		//		 * clears out the lrdbs.
		//		 */
		//		public int ftd_ioctl_clear_lrdbs( int lgnum, int silent ) {
		//			//			int	rc;
		//			// TODO: this needs to be internal to the class : IntPtr ctlfd
		//			//
		//			//			rc = ftd_ioctl(ctlfd, FTD_CLEAR_LRDBS, &lgnum, sizeof(int));
		//			//			if (rc != 0) {
		//			//				if (!silent) {
		//			//					reporterr(ERRFAC, M_DRVERR, ERRWARN, "CLEAR_LRDBS", strerror(ftd_errno()));
		//			//				}
		//			//				return rc;
		//			//			}
		//			//
		//			return 0;
		//		}
		//
		//		/*
		//		 * ftd_ioctl_clear_hrdbs --
		//		 * clears out the hrdbs.
		//		 */
		//		public int ftd_ioctl_clear_hrdbs( int lgnum, int silent ) {
		//			//			int	rc;
		//			// TODO: this needs to be internal to the class : IntPtr ctlfd
		//			//			rc = ftd_ioctl(ctlfd, FTD_CLEAR_HRDBS, &lgnum, sizeof(int));
		//			//			if (rc != 0) {
		//			//				if (!silent) {
		//			//					reporterr(ERRFAC, M_DRVERR, ERRWARN, "CLEAR_HRDBS", strerror(ftd_errno()));
		//			//				}
		//			//				return rc;
		//			//			}
		//			//
		//			return 0;
		//		}

		/*
 * ftd_ioctl_clear_bab --
 * clears out the entries in the writelog.  Tries up to 10 
 * times to make this happen if EAGAIN is being returned.
 */
		//		/// <summary>
		//		/// 
		//		/// </summary>
		//		/// <param name="lgnum"></param>
		//		/// <returns>returns true if EAGAIN is set</returns>
		//		public bool ftd_ioctl_clear_bab( int lgnum ) {
		//			//			int	save_errno, i, rc;
		//			// TODO: this needs to be internal to the class : IntPtr ctlfd
		//			//
		//			//			for (i = 0; i < 10; i++) {
		//			//				rc = ftd_ioctl(ctlfd, FTD_CLEAR_BAB, &lgnum, sizeof(int));
		//			//				if ((rc < 0 && ftd_errno() != EAGAIN) || rc == 0) {
		//			//					break;
		//			//				}
		//			//				save_errno = ftd_errno();
		//			//				sleep(1);
		//			//			}
		//			//			if (rc != 0) {
		//			//				reporterr(ERRFAC, M_DRVERR, ERRWARN,
		//			//					"CLEAR_BAB", strerror(ftd_errno()));
		//			//				return rc;
		//			//			}
		//			//
		//			return false;
		//		}

		//		/// <summary>
		//		/// set the state value for the group
		//		/// </summary>
		//		/// <param name="ps_name"></param>
		//		/// <param name="group_name"></param>
		//		/// <param name="state"></param>
		//		/// <returns></returns>
		//		public void ps_set_group_state( string ps_name, string group_name, eLgModes state ) {
		//			//			HANDLE              fd;
		//			//			int                 ret, index;
		//			//			ps_hdr_t            hdr;
		//			//			ps_group_entry_t    *table;
		//			//			unsigned int        table_size;
		//			//
		//			//			/* open the store */
		//			//			ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
		//			//			if (ret != PS_OK) {
		//			//				return (ret);
		//			//			}
		//			//
		//			//			table[index].state = state;
		//			//
		//			//			if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
		//			//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//			//				free(table);
		//			//				return PS_SEEK_ERROR;
		//			//			}
		//			//
		//			//			table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
		//			//			if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
		//			//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//			//				free(table);
		//			//				return PS_WRITE_ERROR;
		//			//			}
		//			//
		//			//			/* close the store */
		//			//			FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//			//			free(table);
		//			//
		//			//			return PS_OK;
		//		}
		//



		//		/*
		//		 * get the attributes of a Version 1.0 persistent store
		//		 */
		//		public int ps_get_version_1_attr( string ps_name, ps_version_1_attr_t attr ) {
		//			//			HANDLE		fd;
		//			//			ps_hdr_t	hdr;
		//			//
		//			//			/* open the store */
		//			//			if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
		//			//				return PS_BOGUS_PS_NAME;
		//			//			}
		//			//
		//			//			/* seek to the header location */
		//			//
		//			//			ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);
		//			//
		//			//			/* read the header */
		//			//			ftd_read(fd, &hdr, sizeof(ps_hdr_t));
		//			//    
		//			//			FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//			//
		//			//			if (hdr.magic != PS_VERSION_1_MAGIC) {
		//			//				//reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
		//			//				return PS_BOGUS_HEADER;
		//			//			}
		//			//
		//			//			attr->max_dev = hdr.data.ver1.max_dev;
		//			//			attr->max_group = hdr.data.ver1.max_group;
		//			//			attr->dev_attr_size = hdr.data.ver1.dev_attr_size;
		//			//			attr->group_attr_size = hdr.data.ver1.group_attr_size;
		//			//			attr->lrdb_size = hdr.data.ver1.lrdb_size;
		//			//			attr->hrdb_size = hdr.data.ver1.hrdb_size;
		//			//			attr->dev_table_entry_size = hdr.data.ver1.dev_table_entry_size;
		//			//			attr->group_table_entry_size = hdr.data.ver1.group_table_entry_size;
		//			//			attr->num_device = hdr.data.ver1.num_device;
		//			//			attr->num_group = hdr.data.ver1.num_group;
		//			//			attr->last_device = hdr.data.ver1.last_device;
		//			//			attr->last_group = hdr.data.ver1.last_group;
		//			//
		//			return 0;
		//		}


		//		/// <summary>
		//		/// ftd_lg_set_pstore_run_state -- sets the lg run state in the pstore 
		//		/// </summary>
		//		/// <param name="lgnum"></param>
		//		/// <param name="cfgp"></param>
		//		/// <param name="state"></param>
		//		/// <returns></returns>
		//		public int ftd_lg_set_pstore_run_state( int lgnum, LgConfig cfgp, int state ) {
		//			//			string    statestr;
		//			//    
		//			//			string group_name = "\\\\.\\DTC\\lg" + lgnum;
		//			//
		//			//			switch ( state ) {
		//			//				case LG_States.SNORMAL:
		//			//					statestr = "NORMAL";
		//			//					break;
		//			//				case LG_States.SBACKFRESH:
		//			//					statestr = "BACKFRESH";
		//			//					break;
		//			//				case LG_States.SREFRESH:
		//			//					statestr = "REFRESH";
		//			//					break;
		//			//				case LG_States.SREFRESHC:
		//			//					statestr = "CHECK_REFRESH";
		//			//					break;
		//			//				case LG_States.SREFRESHF:
		//			//					statestr = "FULL_REFRESH";
		//			//					break;
		//			//				default:
		//			//					statestr = "ACCUMULATE";
		//			//					break;
		//			//			}
		//			//
		//			//			if ( (ps_set_group_key_value(cfgp->pstore, group_name, "_MODE:", statestr )) != 0 ) {
		//			//				return -1;
		//			//			}
		//			//
		//			//			error_tracef( TRACEINF4, "ftd_lg_set_pstore_run_state():statestr = %s", statestr );
		//			//    
		//			return 0;
		//		}
		//
		//		/*
		// * Set a key/value pair in the group buffer 
		// * If value is NULL, we delete the key/value pair.
		// */
		//		int
		//			ps_set_group_key_value(char *ps_name, char *group_name, char *key, char *value) {
		//			HANDLE		fd;
		//			int          ret, group_index, buf_len, found, i;
		//			int          num_ps, len, linelen;
		//			char         *inbuf, *outbuf, *temp;
		//			char         line[MAXPATHLEN];
		//			char         *ps_key[FTD_MAX_KEY_VALUE_PAIRS];
		//			char         *ps_value[FTD_MAX_KEY_VALUE_PAIRS];
		//			ps_hdr_t     header;
		//			unsigned int offset;
		//
		//			error_tracef(TRACEINF4,"ps_set_group_key_value() ");
		//
		//			/* open the store */
		//			ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
		//				&group_index, NULL);
		//			if (ret != PS_OK) {
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): Can't open the pstore");
		//				return (ret);
		//			}
		//
		//			/* allocate a buffer for the data */
		//			buf_len = header.data.ver1.group_attr_size;
		//			if ((inbuf = (char *)malloc(buf_len)) == NULL) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): PS_MALLOC_ERROR");
		//				return (PS_MALLOC_ERROR);
		//			}
		//			if ((outbuf = (char *)calloc(1, buf_len)) == NULL) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				free(inbuf);
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): PS_MALLOC_ERROR_2");
		//				return (PS_MALLOC_ERROR);
		//			}
		//
		//			/* read the data */
		//			offset = (header.data.ver1.group_attr_offset * 1024) +
		//				(group_index * header.data.ver1.group_attr_size);
		//			if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): PS_SEEK_ERROR");
		//				return PS_SEEK_ERROR;
		//			}
		//			if (ftd_read(fd, inbuf, buf_len) != buf_len) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): PS_READ_ERROR");
		//				return PS_READ_ERROR;
		//			}
		//
		//			/* parse the attributes into key/value pairs */
		//			temp = inbuf;
		//			num_ps = 0;
		//			while (ps_getline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
		//				num_ps++;
		//			}
		//
		//			found = FALSE;
		//			for (i = 0; i < num_ps; i++) {
		//				if (strcmp(key, ps_key[i]) == 0) {
		//					/* replace value */
		//					ps_value[i] = value;
		//					found = TRUE;
		//					break;
		//				}
		//			}
		//
		//			/* if we didn't find it, add it. */
		//			if (!found) {
		//				ps_key[num_ps] = key;
		//				ps_value[num_ps] = value;
		//				num_ps++;
		//			}
		//
		//			/* create a new buffer */
		//			len = 0;
		//			for (i = 0; i < num_ps; i++) {
		//				/* we may be deleting this pair ... */
		//				if (ps_value[i] != NULL) {
		//					sprintf(line, "%s %s\n", ps_key[i], ps_value[i]);
		//					linelen = strlen(line);
		//					strncpy(&outbuf[len], line, linelen);
		//					len += linelen;
		//				}
		//			}
		//
		//			/* write out the new buffer */
		//			if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				free(inbuf);
		//				free(outbuf);
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): PS_SEEK_ERROR_2");
		//				return PS_SEEK_ERROR;
		//			}
		//			if (ftd_write(fd, outbuf, buf_len) != buf_len) {
		//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//				free(inbuf);
		//				free(outbuf);
		//				error_tracef(TRACEERR,"ps_set_group_key_value(): PS_WRITE_ERROR");
		//				return PS_WRITE_ERROR;
		//			}
		//
		//			FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		//
		//			free(inbuf);
		//			free(outbuf);
		//
		//			return PS_OK;
		//		}
		//
		//
		//		/*
		//		 * ftd_lg_set_driver_run_state -- sets the lg run state in the driver
		//		 */
		//		public int SetDriverState( int lgnum, eLgModes dstate ) {
		//			//			HANDLE  ctlfd = INVALID_HANDLE_VALUE;
		//			//
		//			//			error_tracef( TRACEINF4, "ftd_lg_set_driver_run_state():LG%d, state %d", lgnum, dstate);
		//			//
		//			//			if ((ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
		//			//				error_tracef( TRACEERR, "ftd_lg_set_driver_run_state(%d): INVALID_HANDLE_VALUE error", lgnum);
		//			//				goto errret;
		//			//			}
		//			//
		//			//			switch(dstate) {
		//			//				case FTD_MODE_NORMAL:
		//			//				case FTD_MODE_REFRESH:
		//			//				case FTD_MODE_FULLREFRESH:
		//			//				case FTD_MODE_BACKFRESH:
		//			//				case FTD_MODE_TRACKING:
		//			//				case FTD_MODE_PASSTHRU:
		//			//					break;
		//			//				default:
		//			//					dstate = ftd_lg_get_driver_run_state(lgnum);
		//			//					break;
		//			//			}
		//			//
		//			//			if (ftd_ioctl_set_group_state(ctlfd, lgnum, dstate) < 0) {
		//			//				error_tracef( TRACEERR, "ftd_lg_set_driver_run_state(%d): ftd_ioctl_set_group_state error", lgnum);
		//			//				goto errret;
		//			//			}
		//			//    
		//			//			if (ctlfd != INVALID_HANDLE_VALUE) {
		//			//				FTD_CLOSE_FUNC(__FILE__,__LINE__,ctlfd);
		//			//			}
		//			//
		//			return 0;
		//			//
		//			//			errret:
		//			//
		//			//				if (ctlfd != INVALID_HANDLE_VALUE) {
		//			//					FTD_CLOSE_FUNC(__FILE__,__LINE__,ctlfd);
		//			//				}
		//			//
		//			//			return -1;
		//		}


		//		/// <summary>
		//		/// ftd_lg_get_pstore_run_state -- returns the lg state from the pstore 
		//		/// </summary>
		//		/// <param name="lgnum"></param>
		//		/// <param name="cfgp"></param>
		//		/// <returns></returns>
		//		public eLgStates ftd_lg_get_pstore_run_state( int lgnum, LgConfig cfgp ) {
		//			//			string            statestr, group_name;
		//			//			int             rc;
		//			eLgStates state = eLgStates.SINVALID;
		//			//    
		//			//			group_name = FTD_CREATE_GROUP_NAME( lgnum );
		//			//
		//			//			if (ftd_config_read(cfgp, TRUE, lgnum, ROLEPRIMARY, 1) < 0) {
		//			//				ftd_config_lg_delete(cfgp);
		//			//				error_tracef( TRACEERR, "ftd_lg_get_pstore_run_state(%d) ROLEPRIMARY:", lgnum);
		//			//				return -1;
		//			//			}
		//			//
		//			//			if ((rc = ps_get_group_key_value(cfgp->pstore, group_name,
		//			//				"_MODE:", statestr)) != 0) {
		//			//				return -1;
		//			//			}
		//			//
		//			//			error_tracef( TRACEINF4, "ftd_lg_get_pstore_run_state():statestr = %s", statestr );
		//			//    
		//			//			if (!strcmp(statestr, "NORMAL")) {
		//			//				state = LG_States.SNORMAL;
		//			//			} else if (!strcmp(statestr, "BACKFRESH")) {
		//			//				state = LG_States.SBACKFRESH;
		//			//			} else if (!strcmp(statestr, "REFRESH")) {
		//			//				state = LG_States.SREFRESH;
		//			//			} else if (!strcmp(statestr, "CHECK_REFRESH")) {
		//			//				state = LG_States.SREFRESHC;
		//			//			} else if (!strcmp(statestr, "FULL_REFRESH")) {
		//			//				state = LG_States.SREFRESHF;
		//			//			} else if (!strlen(statestr)) {
		//			//				state = LG_States.SNORMAL;
		//			//			}  
		//			//
		//			return state;
		//		}
		/*
		 * ftd_ioctl_get_bab_size --
		 * gets the bab size in bytes.
		 */
		public unsafe void GetBabSize(ref int BabSize) {

			int size = sizeof(int);
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_BAB_SIZE, ptr, size, ptr, ref size);
				byte * bPtr = (byte*)ptr.ToPointer();
				BabSize = (int)bPtr;

			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetBabSize: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/*
			 * set the checkpoint value for the group
			 */

		public void ps_set_group_checkpoint( int lgnum, bool checkpoint) {
			// TODO: has to throw exception for error cases
			// TODO: handle this function
			//			HANDLE              fd;
			//			int                 ret, index;
			//			ps_hdr_t            hdr;
			//			ps_group_entry_t    *table;
			//			uint        table_size;
			//
			//			error_tracef( TRACEINF4,"ps_set_group_checkpoint() ");
			//			/* open the store */
			//			ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
			//			if (ret != PS_OK) {
			//				error_tracef(TRACEERR,"ps_set_group_checkpoint(): PS_OK");
			//				throw new OmException ( "ps_set_group_checkpoint(): PS_OK" );
			//				return (ret);
			//			}
			//
			//			table[index].checkpoint = checkpoint;
			//
			//			if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
			//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
			//				free(table);
			//				error_tracef(TRACEERR,"ps_set_group_checkpoint(): PS_SEEK_ERROR");
			//				return PS_SEEK_ERROR;
			//			}
			//
			//			table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
			//			if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
			//				FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
			//				free(table);
			//				error_tracef(TRACEERR,"ps_set_group_checkpoint(): PS_WRITE_ERROR");
			//				return PS_WRITE_ERROR;
			//			}
			//
			//			/* close the store */
			//			FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
			//			free(table);

		}


		
		/// <summary>
		/// ftd_stat_connection_driver -- initialize lg stat in driver
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="state"></param>
		/// <returns></returns>
		public int ftd_stat_connection_driver( int lgnum, int state ) {
			//			char			devbuf[FTD_PS_DEVICE_ATTR_SIZE];
			//			int				rc;
			//			Statistics_t		lgstat;
			//			disk_stats_t	*devtemp, *devstat;
			//
			//			if (ctlfd == INVALID_HANDLE_VALUE)
			//				return -1;
			//
			//			if (ftd_ioctl_get_group_stats(ctlfd, lgnum, &lgstat, 1) < 0) {
			//				return -1;
			//			}
			//
			//			devstat = malloc((lgstat.ndevs + 1) * sizeof(disk_stats_t));
			//			if (devstat == NULL)
			//				return -1;
			//
			//			memset(devstat, -1, (lgstat.ndevs + 1) * sizeof(disk_stats_t));
			//			if (ftd_ioctl_get_dev_stats(ctlfd, lgnum, -1, devstat, lgstat.ndevs + 1) < 0) {
			//				free(devstat);
			//				return -1;
			//			}
			//	
			//			devtemp = devstat;
			//			while(devtemp->localbdisk != -1) {
			//				ftd_dev_stat_t	*statp;
			//
			//				rc = ftd_ioctl_get_dev_state_buffer(ctlfd, lgnum, devtemp->localbdisk, 
			//					sizeof(devbuf), devbuf, 0);
			//				if (rc != 0) {
			//					free(devstat);
			//					return rc;
			//				}
			//
			//				statp = (ftd_dev_stat_t*)devbuf;
			//		statp->connection = state;
			//
			//				rc = ftd_ioctl_set_dev_state_buffer(ctlfd, lgnum, devtemp->localbdisk, 
			//					sizeof(devbuf), devbuf, 0);
			//				if (rc != 0) {
			//					free(devstat);
			//					return rc;
			//				}
			//
			//				devtemp++;
			//			}
			//
			//			free(devstat);

			return 0;
		}

		/// <summary>
		/// SetTraceLevel -- Sets the driver diagnostic trace level.
		/// In Windows, traces are directed to the System Event Log (System Log)
		/// </summary>
		/// <param name="level"></param>
		/// <returns></returns>
		public unsafe void SetTraceLevel( int level ) {

			int size = sizeof(int);
			IntPtr ptr = Marshal.AllocHGlobal( size );
			ptr = (IntPtr)level;

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_TRACE_LEVEL, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetTraceLevel: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// ConfigStart -- sends the "config start" notification to the driver
		/// </summary>
		/// <returns></returns>
		public unsafe void ConfigStart() {
			CONFIG_BEGIN_INFO info = new CONFIG_BEGIN_INFO();
			info.HostId = MasterThread.m_MacAddress;
			info.SystemName = Environment.MachineName;
			info.Version = "5.0.0";

			int size = (Marshal.SizeOf( typeof(CONFIG_BEGIN_INFO)));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			Marshal.StructureToPtr ( info, ptr, true );
			try {
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_CONFIG_BEGIN, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "ConfigStart: Error detected." + e.Message, e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// ConfigEnd -- sends the "config stop" notification to the driver
		/// </summary>
		/// <returns></returns>
		public unsafe void ConfigEnd() 
		{

			int size = 0;
			IntPtr ptr = IntPtr.Zero;

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_CONFIG_END, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "ConfigEnd: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/*
		 * ftd_ioctl_get_dev_stats -- returns the devices stats from the driver
		 */
		public unsafe disk_stats_t[] GetDevStatistics( int lgnum, int devnum, int NoOfDevices ) {

			disk_stats_t[] disk_stats = new disk_stats_t[NoOfDevices];
			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.dev_num = devnum;
			statCmdBuf.len = NoOfDevices;
			statCmdBuf.buffer = (void*)Marshal.AllocHGlobal( (NoOfDevices) * ((int)(Marshal.SizeOf( typeof(disk_stats_t)))) );

			int size = (int)(Marshal.SizeOf( statCmdBuf ));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				// read state from driver
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_DEV_STATS, ptr, size, ptr, ref size);
				byte * bPtr = (byte*)ptr.ToPointer();

				// bump up the pointer by 12 bytes
				bPtr = bPtr + 12;

				for ( int i = 0 ; i < NoOfDevices ; i++ ) {
					Marshal.PtrToStructure((IntPtr)bPtr, disk_stats[i]);
					bPtr += Marshal.SizeOf( typeof(disk_stats_t) );
				}

				return disk_stats;
			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetLgState: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
				Marshal.FreeHGlobal( (IntPtr)statCmdBuf.buffer );
			}
		}

		/*
		 * GetDevsInfo --
		 * Get the device state from the driver.
		 */
		public unsafe ftd_dev_info_t[] GetDevsInfo( int lgnum, int devnum, int NoOfDevices ) {

			ftd_dev_info_t[] devs_info = new ftd_dev_info_t[NoOfDevices];
			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.buffer = (void*)Marshal.AllocHGlobal( (NoOfDevices) * ((int)(Marshal.SizeOf( typeof(ftd_dev_info_t)))) );

			int size = (int)(Marshal.SizeOf( statCmdBuf ));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				// read state from driver
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_DEVICES_INFO, ptr, size, ptr, ref size);
				byte * bPtr = (byte*)ptr.ToPointer();

				// bump up the pointer by 12 bytes
				bPtr = bPtr + 12;

				for ( int i = 0 ; i < NoOfDevices ; i++ ) {
					Marshal.PtrToStructure((IntPtr)bPtr, devs_info[i]);
					bPtr += Marshal.SizeOf( typeof(ftd_dev_info_t) );
				}

				return devs_info;
			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetLgState: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
				Marshal.FreeHGlobal( (IntPtr)statCmdBuf.buffer );
			}
		}


		/*
		 * ftd_ioctl_get_dev_state_buffer --
		 * Get the device state from the driver.
		 */
		public unsafe void GetDevStateBuffer(int lgnum, int devnum, out ftd_dev_stat_t DevState ) {
			DevState = new ftd_dev_stat_t();
			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.dev_num = devnum;
			statCmdBuf.len = PS_DEVICE_ATTR_SIZE;
			statCmdBuf.buffer = (void*)Marshal.AllocHGlobal(PS_DEVICE_ATTR_SIZE);

			int size = (int)(Marshal.SizeOf( statCmdBuf ));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				// read state from driver
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_DEV_STATE_BUFFER, ptr, size, ptr, ref size);
				byte * bPtr = (byte*)ptr.ToPointer();

				// bump up the pointer by 12 bytes
				bPtr = bPtr + 12;
				Marshal.PtrToStructure((IntPtr)bPtr, DevState);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "GetDevStateBuffer: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
				Marshal.FreeHGlobal( (IntPtr)statCmdBuf.buffer );
			}
		}

		/*
		 * ftd_ioctl_get_dev_state_buffer --
		 * Set the device state from the driver.
		 */
		public unsafe void SetDevStateBuffer(int lgnum, int devnum, ftd_dev_stat_t DevState ) {
			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
			statCmdBuf.lg_num = lgnum;
			statCmdBuf.dev_num = devnum;
			statCmdBuf.len = PS_DEVICE_ATTR_SIZE;
			statCmdBuf.buffer = &DevState;

			int size = (int)(Marshal.SizeOf( statCmdBuf ));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				Marshal.StructureToPtr( statCmdBuf, ptr, false );

				// read state from driver
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_DEV_STATE_BUFFER, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetDevStateBuffer: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/*
		 * ftd_ioctl_get_groups_info --
		 * Get the groups state from the driver.
		 */
//		public unsafe void GetGroupInfo(int lgnum, out ftd_lg_info_t lgp) {
//			lgp = new ftd_lg_info_t();
//			Stat_Buffer_t statCmdBuf = new Stat_Buffer_t();
//			statCmdBuf.lg_num = lgnum;
//			statCmdBuf.len = (int)(Marshal.SizeOf(typeof(ftd_lg_info_t)));
//			statCmdBuf.buffer = (void*)Marshal.AllocHGlobal(statCmdBuf.len);
//
//			int size = (int)(Marshal.SizeOf( statCmdBuf ));
//			IntPtr ptr = Marshal.AllocHGlobal( size );
//			
//			try {
//				// clear the buffer
//				DeviceIOControl.Zero( ptr, size );
//
//				Marshal.StructureToPtr( statCmdBuf, ptr, false );
//
//				// read state from driver
//				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_GROUPS_INFO, ptr, size, ptr, ref size);
//				byte * bPtr = (byte*)ptr.ToPointer();
//
//				// bump up the pointer by 12 bytes
//				bPtr = bPtr + 12;
//				Marshal.PtrToStructure((IntPtr)bPtr, lgp);
//			} 
//			catch ( Exception e ) {
//				throw ( new OmException( "GetGroupsInfo: Error detected.", e ) );
//			}
//			finally {
//				// free the memory
//				Marshal.FreeHGlobal( ptr );
//			}
//		}


		/// <summary>
		/// 
		/// </summary>
		/// <param name="lgp"></param>
		public void sftk_lg_add_connections( LogicalGroup lgp ) {
			//TODO impliment
			
		}


		/* info needed to add a logical group */
		public struct ftd_lg_info_t {
			public int lgdev;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string vdevname;
			public int statsize;   /* number of bytes in the statistics buffer */
		}

		/*
		 * ftd_ioctl_new_lg --
		 * create a new lg.
		 */
		public void ftd_ioctl_new_lg ( ftd_lg_info_t info ) {
			
			int size = (int)Marshal.SizeOf(typeof(ftd_lg_info_t));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( info, ptr, true );
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_NEW_LG, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "ftd_ioctl_new_lg: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		public enum eErrorCodes {
			GOOD = 0,
			FTD_DRIVER_ERROR_CODE = unchecked((int)0xE0000000),
			ENOENT = FTD_DRIVER_ERROR_CODE + 2,
			ENXIO = FTD_DRIVER_ERROR_CODE + 6,
			EAGAIN = FTD_DRIVER_ERROR_CODE + 11,
			EACCES = FTD_DRIVER_ERROR_CODE + 13,
			EFAULT = FTD_DRIVER_ERROR_CODE + 14,
			EBUSY = FTD_DRIVER_ERROR_CODE + 16,
			EINVAL = FTD_DRIVER_ERROR_CODE + 22,
			ENOTTY = FTD_DRIVER_ERROR_CODE + 25,
			EADDRINUSE = FTD_DRIVER_ERROR_CODE + 98  /* Address already in use */
		}

		/* info needed to add a device to a logical group */
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public struct ftd_dev_info_t {
			public int lgnum;		//*
			public int localcdev;	//* dev_t for raw disk device
			public int cdev;		//* dev_t for raw ftd disk device
			public int bdev;		//* dev_t for block ftd disk device
			public uint disksize;	//*
			public int lrdbsize32;	//* number of 32-bit words in LRDB
			public int hrdbsize32;	//* number of 32-bit words in HRDB
			public int lrdb_res;	// Not used
			public int hrdb_res;	// Not used
			public int lrdb_numbits; // Not used
			public int hrdb_numbits; // Not used
			public int statsize;	// Not used DEFAULT_STATE_BUFFER_SIZE (4096)
			public uint lrdb_offset; // Not used
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string devname;	//* in only
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string vdevname;	//* in only
			// Added the Remote Device Name this will be sent at handshake time
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=64)]
			public string strRemoteDeviceName;	//* Remote Drive Letter from Config File

			// OS supported Unique ID for Volume (Raw Disk/ Disk Partition)
			public bool bUniqueVolumeIdValid;	//? TRUE means UniqueIdLength and UniqueId has valid values
			public UInt16 UniqueIdLength;		//?
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string UniqueId;				//? 256 is enough, if requires bump up this value.

			// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
			public bool bSignatureUniqueVolumeIdValid;	//? TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
			public UInt16 SignatureUniqueIdLength;		//?
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
			public string SignatureUniqueId;			//? 256 is enough, if requires bump up this value.

			// Optional - If Device is formatted disk Partition, than associated drive letter symbolik link name info 
			public bool bSuggestedDriveLetterLinkValid;	//? TRUE means SuggestedDriveLetterLinkLength and SuggestedDriveLetterLink has valid values
			public UInt16 SuggestedDriveLetterLinkLength;	//
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
			public string SuggestedDriveLetterLink;		//? 256 is enough, if requires bump up this value.
		}


		/// <summary>
		/// ftd_ioctl_new_device -- create a new device.
		/// </summary>
		/// <param name="info"></param>
		public void ftd_ioctl_new_device( ftd_dev_info_t info ) {

			int size = (int)Marshal.SizeOf(typeof(ftd_dev_info_t));
			IntPtr ptr = Marshal.AllocHGlobal( size );
			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( info, ptr, false );
				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_NEW_DEVICE, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "ftd_ioctl_new_device: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/*
		* ftd_ioctl_get_device_nums --
		* get the device numbers for the target device.
		*/
		public unsafe ftd_devnum_t ftd_ioctl_get_device_nums() {
			ftd_devnum_t devnum = new ftd_devnum_t();
			int size = (int)(Marshal.SizeOf( typeof(ftd_devnum_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_DEVICE_NUMS, ptr, size, ptr, ref size);
				byte * pstatCmdBuf = (byte*)ptr.ToPointer();
				Marshal.PtrToStructure( (IntPtr)(pstatCmdBuf), devnum );
				return devnum;
			} 
			catch ( Exception e ) {
				throw ( new OmException( "ftd_ioctl_get_device_nums: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}


//		/// <summary>
//		/// ftd_ioctl_del_lg -- delete the group from the driver
//		/// </summary>
//		/// <param name="lgdev"></param>
//		/// <param name="silent"></param>
//		/// <returns></returns>
//		public unsafe void ftd_ioctl_del_lg( int lgdev ) {
//
//			int size = sizeof(int);
//			IntPtr ptr = Marshal.AllocHGlobal( size );
//
//			try {
//				// clear the buffer
//				DeviceIOControl.Zero( ptr, size );
//				Marshal.StructureToPtr( lgdev, ptr, false );
//
//				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_DEL_LG, ptr, size, ptr, ref size);
//			} 
//			catch ( Exception e ) {
//				throw ( new OmException( "ftd_ioctl_del_lg: Error detected.", e ) );
//			}
//			finally {
//				// free the memory
//				Marshal.FreeHGlobal( ptr );
//			}
//		}

//		/// <summary>
//		/*
//		* ftd_ioctl_del_device --
//		* delete the device from the driver
//		*/
//		/// </summary>
//		/// <param name="ctlfd"></param>
//		/// <param name="devnum"></param>
//		public unsafe void ftd_ioctl_del_device(int devnum){
//
//			int size = sizeof(int);
//			IntPtr ptr = Marshal.AllocHGlobal( size );
//
//			try {
//				// clear the buffer
//				DeviceIOControl.Zero( ptr, size );
//				Marshal.StructureToPtr( devnum, ptr, false );
//
//				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_DEL_DEVICE, ptr, size, ptr, ref size);
//			} 
//			catch ( Exception e ) {
//				throw ( new OmException( "ftd_ioctl_del_device: Error detected.", e ) );
//			}
//			finally {
//				// free the memory
//				Marshal.FreeHGlobal( ptr );
//			}
//		}

		/// <summary>
		/// SetSyncDepth -- Sets the sync depth for the logical group
		/// To turn off sync mode, set the depth to -1.
		/// ftd_ioctl_set_sync_depth
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="sync_depth"></param>
		/// <returns></returns>
		public void SetSyncDepth( int lgnum, int sync_depth ) {
			ftd_param_t	pa = new ftd_param_t();

			pa.lgnum	= lgnum;
			pa.value1	= sync_depth;
    
			int size = (int)(Marshal.SizeOf( typeof(ftd_param_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_SYNC_DEPTH, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetSyncDepth: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}


		/// <summary>
		/// SetIodelay -- Sets the iodelay in the driver
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="delay"></param>
		/// <returns></returns>
		public void SetIodelay( int lgnum, int delay ) {
			ftd_param_t	pa = new ftd_param_t();

			pa.lgnum	= lgnum;
			pa.value1	= delay;
    
			int size = (int)(Marshal.SizeOf( typeof(ftd_param_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_IODELAY, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetIodelay: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// SetSyncTimeout -- Sets the sync timeout for the logical group.
		/// Zero indicates no timeout.
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="sync_timeout"></param>
		/// <returns></returns>
		public void SetSyncTimeout( int lgnum, int sync_timeout ) {
			ftd_param_t	pa = new ftd_param_t();

			pa.lgnum	= lgnum;
			pa.value1	= sync_timeout;
    
			int size = (int)(Marshal.SizeOf( typeof(ftd_param_t)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SET_SYNC_TIMEOUT, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) {
				throw ( new OmException( "SetSyncTimeout: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}






		/// <summary>
		/// TCP_ADD_CONNECTIONS
		/// </summary>
		/// <returns></returns>
		public void TCP_ADD_CONNECTIONS( CONNECTION_DETAILS ConnectionDetails ) 
		{
			int size = (int)(Marshal.SizeOf( typeof(CONNECTION_DETAILS)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( ConnectionDetails, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_TCP_ADD_CONNECTIONS, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "TCP_ADD_CONNECTIONS: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// TCP_REMOVE_CONNECTIONS
		/// </summary>
		/// <returns></returns>
		public void TCP_REMOVE_CONNECTIONS( CONNECTION_DETAILS ConnectionDetails ) 
		{
			int size = (int)(Marshal.SizeOf( typeof(CONNECTION_DETAILS)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( ConnectionDetails, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_TCP_REMOVE_CONNECTIONS, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "TCP_REMOVE_CONNECTIONS: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// TCP_ENABLE_CONNECTIONS
		/// </summary>
		/// <returns></returns>
		public void TCP_ENABLE_CONNECTIONS( CONNECTION_DETAILS ConnectionDetails ) 
		{
			int size = (int)(Marshal.SizeOf( typeof(CONNECTION_DETAILS)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( ConnectionDetails, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_TCP_ENABLE_CONNECTIONS, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "TCP_ENABLE_CONNECTIONS: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// TCP_DISABLE_CONNECTIONS
		/// </summary>
		/// <returns></returns>
		public void TCP_DISABLE_CONNECTIONS( CONNECTION_DETAILS ConnectionDetails ) 
		{
			int size = (int)(Marshal.SizeOf( typeof(CONNECTION_DETAILS)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( ConnectionDetails, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_TCP_DISABLE_CONNECTIONS, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "TCP_DISABLE_CONNECTIONS: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// Start PMD
		/// </summary>
		/// <returns></returns>
		public void StartPMD( SM_INIT_PARAMS SMInitParams ) 
		{
			int size = (int)(Marshal.SizeOf( typeof(SM_INIT_PARAMS)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( SMInitParams, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_START_PMD, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "StartPMD: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// Start RMD
		/// </summary>
		/// <returns></returns>
		public void StartRMD( SM_INIT_PARAMS SMInitParams ) 
		{
			int size = (int)(Marshal.SizeOf( typeof(SM_INIT_PARAMS)));
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( SMInitParams, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_START_RMD, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "StartRMD: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// Stop PMD
		/// </summary>
		/// <returns></returns>
		public unsafe void StopPMD( int LgNum ) 
		{
			int size = sizeof(int);
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( LgNum, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_TCP_ENABLE_CONNECTIONS, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "StopPMD: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/// <summary>
		/// Stop RMD
		/// </summary>
		/// <returns></returns>
		public unsafe void StopRMD( int LgNum ) 
		{
			int size = sizeof(int);
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try 
			{
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );
				Marshal.StructureToPtr( LgNum, ptr, false );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_SFTK_IOCTL_STOP_RMD, ptr, size, ptr, ref size);
			} 
			catch ( Exception e ) 
			{
				throw ( new OmException( "StopRMD: Error detected.", e ) );
			}
			finally 
			{
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}


		/// <summary>
		/// Tell driver to activate the target host for this logical group.
		/// used in cluster enviroments
		/// </summary>
		/// <param name="lgnum"></param>
		public void Activate ( int lgnum, string HostID, bool force ) {
			//TODO: Impliment
		}

		/// <summary>
		/// new IOCTL notify the driver/TDI that the secondary is alive now and attempt any connections.
		/// </summary>
		/// <param name="lgnum"></param>
		public void SecondaryAlive( int lgnum ) {
			//TODO: Impliment
		}

		/*
		*  Function: 		
		*		ULONG	Sftk_Get_TotalLgCount()
		*
		*  Arguments: 	
		* 				...
		* Returns: returns Total Number of LG configured in driver
		*
		* Description:
		*		sends FTD_GET_NUM_GROUPS to driver and retrieves Total Num of LG configured in driver
		*/
		public unsafe int Sftk_Get_TotalLgCount() {
			int size = sizeof(Int32);
			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_NUM_GROUPS, ptr, size, ptr, ref size);
				return Marshal.ReadInt32( ptr );
			} 
			catch ( Exception e ) {
				throw ( new OmException( "StopRMD: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
		}

		/*
		*  Function: 		
		*		ULONG	Sftk_Get_TotalDevCount()
		*
		*  Arguments: 	
		* 				...
		* Returns: returns Total Number of Dev configured for all LG in driver
		*
		* Description:
		*		sends FTD_GET_DEVICE_NUMS to driver and retrieves Total Num of Devices exist for all LG in driver
		*/
		public unsafe int Sftk_Get_TotalDevCount() {
			ftd_devnum_t devNum;
			devNum = ftd_ioctl_get_device_nums();
			return devNum.b_major;
		}


		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class ALL_LG_STATISTICS {
			public ALL_LG_STATISTICS_ Size;		// Memories for Total Number of Entries for LG_STATISTICS struct for Dev under LG
			public Management.LgStatisticsCollection LgStats;	// collection of DevStats
		}
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class ALL_LG_STATISTICS_ {
			public int NumOfLgEntries;		// Memories for Total Number of Entries for LG_STATISTICS struct for Dev under LG
			public int NumOfDevEntries;		// Memories for Total Number of Entries for DEV_STATISTICS struct for Dev under LG
		}
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class LG_STATISTICS {
			public LG_STATISTICS_ lgStat;
			public Management.DeviceStatisticsCollection DevStats;	// collection of DevStats
		}
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class LG_STATISTICS_ {
			public int LgNum;			// stat_buffer_t->lg_num: LG Num
			public uint Flags;			// GET_ONLY_LG_INFO: This Flag defines what type of information caller wants
			public uint cdev;			// used only if Flags == GET_LG_AND_SPECIFIED_ONE_DEV_INFO to get specified Device stats info only
			public eLgModes LgState;		// LG state mode
			public STATISTICS_INFO	LgStat;	// LG Statistics information
			public uint NumOfDevStats;	// Number of entries passed in DevStats Array
		}

		// -------- Following are new Structures defined for statistics IOCTL ------
		//
		// STATISTICS_INFO structure will be used per LG and also per Src Device in LG.
		// For LG, it stores total stats values (accumulation of all src devices stats under that LG)
		// For DEV, it stores total stats values for that device only.
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public struct STATISTICS_INFO {
			// Foll. fields reflects User IO stats activity per Dev/or LG
			public UInt64 RdCount;			// readiocnt: Applications Total number of Read ocurred to src device, 
			public UInt64 WrCount;			// writeiocnt: Applications Total number of Write ocurred to src device, 
			public UInt64 BlksRd;			// sectorsread	:Applications Total number of Sectors Read ocurred to src device, 
			public UInt64 BlksWr;			// sectorswritten: Applications Total number of Sectors Write ocurred to src device, 

			public UInt64 RemoteWrPending;	// wlentries: number of incoming writes happened to device, and yet this 
			// writes is not commited to secondary side
			public UInt64 RemoteBlksWrPending;// wlsectors: Total number of incoming writes Sectors to device, and yet this 
			// writes is not commited to secondary side

			// Foll. fields reflect current view of Queue Management per Dev/or LG
			public UInt64 QM_WrPending;			// Num of Wr exist in pending queue 
			public UInt64 QM_BlksWrPending;		// total sectors of Wr exist in pending queue 

			public UInt64 QM_WrCommit;			// Num of Wr exist in Commit queue 
			public UInt64 QM_BlksWrCommit;		// total sectors of Wr exist in Commit queue 

			public UInt64 QM_WrRefresh;			// Num of Wr exist in Refresh queue 
			public UInt64 QM_BlksWrRefresh;		// total sectors of Wr exist in Refresh queue 

			public UInt64 QM_WrMigrate;			// Num of Wr exist in Migrate queue 
			public UInt64 QM_BlksWrMigrate;		// total sectors of Wr exist in Migrate queue 

			// Foll. fields reflect current view of Memory Management usage per Dev/or LG
			// Following 2 fields are gloabl for ALL lg and Devs, same...
			public UInt64 MM_TotalMemAllocated;		// At Present, Total Memory allocated for MM from OS 
			// used from MM_MANAGER->MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize
			public UInt64 MM_TotalMemIsUsed;		// At Present, Total Memory is used in MM by driver, 
			// used from MM_MANAGER->MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * MM_MANAGER->PageSize
	
			public UInt64 MM_TotalMemUsed;		// Till moment, total memory used for QM for current LG, this value never gets decremented
			public UInt64 MM_MemUsed;			// At Present, total memory allocated for QM for current LG
	
			public UInt64 MM_TotalRawMemUsed;	// Till moment, total RAW memory used for QM for current LG, this value never gets decremented
			public UInt64 MM_RawMemUsed;		// At Present, total RAW memory allocated for QM for current LG
	
			public UInt64 MM_TotalOSMemUsed;	// Till moment, total Direct OS memory used for QM for current LG, this value never gets decremented
			public UInt64 MM_OSMemUsed;			// At Present, total Direct OS memory allocated for QM for current LG

			// Foll. Fields reflects Refresh Activity stats
			public uint NumOfBlksPerBit;	// total number of sectors per bit values

			public UInt64 TotalBitsDirty;	// Total Number Of Bits are Dirty per dev/or LG
			public UInt64 TotalBitsDone;	// Total Number of Dirty bits recovered till moment per dev/or LG

			public uint CurrentRefreshBitIndex;	// rsyncoff: this values is in bit and valid only for Dev based, so CurrentRefreshBitIndex * NumOfBlksPerBit.

			// foll. used for pstore access stats
			public UInt64 PstoreLrdbUpdateCounts;	// Number of times LRDB bitmap updated to Pstore file
			public UInt64 PstoreHrdbUpdateCounts;	// Number of times LRDB bitmap updated to Pstore file

			public UInt64 PstoreFlushCounts;		// total Number of times Pstore Got Flush 
			public UInt64 PstoreBitmapFlushCounts;	// total Number of times Pstore Bitmaps Got Flush 

			// Following are the Session Manager Statistics metrics for all connections for a logical group

			public UInt64 SM_PacketsSent;			// The total TDI Packets Sent
			public UInt64 SM_BytesSent;				// The total Bytes Sent in those Packets
	
			public UInt64 SM_EffectiveBytesSent;	// Effective Bytes Sent along with the compressed data

			public UInt64 SM_PacketsReceived;		// The total TDI Packets Received
			public UInt64 SM_BytesReceived;			// The total Bytes Received

			public uint SM_AverageSendPacketSize;	// Average Send Packet Size.
			public uint SM_MaximumSendPacketSize;	// Maximum send Packet Size.
			public uint SM_MinimumSendPacketSize;	// Average Send Packet Size.

			public uint SM_AverageReceivedPacketSize;	// The Average Received Packet Size.
			public uint SM_MaximumReceivedPacketSize;	// Maximum Received packet Size.
			public uint SM_MinimumReceivedPacketSize;	// Minimum Received Packet Size.

			public UInt64 SM_AverageSendDelay;	// estimated delay on this connection.
			public UInt64 SM_Throughput;		// estimated throughput on this connection.
			// Follow. fields used for session manager for socket connections stats
		}

		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public struct DEV_STATISTICS {
			public int LgNum;				// stat_buffer_t->lg_num: LG Num
			public int cdev;				// stat_buffer_t->dev_num: Device Unique Id Per LG
			public eLgModes DevState;		// for future used, reflect device full/smart refresh completed or not
			public uint Disksize;			// in Sectors, Actual Device size = disksize * 512 for bytes
			public STATISTICS_INFO DevStat;	// Device Statistics information
		}

		/*
		*  Function: 		
		*		DWORD
		*		Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size )
		*
		*  Arguments: 	
		* 				...
		* Returns: returns NO_ERROR on sucess and All_LgStats has valid values else return SDK GetLastError()
		*
		* Description:
		*		sends FTD_GET_ALL_STATS_INFO to driver and retrieves All LG and their Devices Stats info from driver
		*/
		public unsafe ALL_LG_STATISTICS Sftk_Get_AllStatsInfo() {
			ALL_LG_STATISTICS pAllStats = new ALL_LG_STATISTICS();
			int size = MAX_SIZE_ALL_LG_STATISTICS( pAllStats.Size.NumOfLgEntries, pAllStats.Size.NumOfDevEntries );

			IntPtr ptr = Marshal.AllocHGlobal( size );

			try {
				// clear the buffer
				DeviceIOControl.Zero( ptr, size );

				DeviceIOControl.DeviceIoCtrl( m_DriverHandle, DeviceIOControl.IOCTL_GET_ALL_STATS_INFO, ptr, size, ptr, ref size);

				// need to marshal all of the data
				pAllStats.Size.NumOfLgEntries = Marshal.ReadInt32( ptr , 0 );
				pAllStats.Size.NumOfDevEntries = Marshal.ReadInt32( ptr , 4 );
				// create a new lg stat collection
				pAllStats.LgStats = new Management.LgStatisticsCollection();
				IntPtr pntr = (IntPtr)(ptr.ToInt32() + 8);
				for ( int lgCount = 0 ; lgCount < pAllStats.Size.NumOfLgEntries ; lgCount++ ) {
					LG_STATISTICS lgStat = new LG_STATISTICS(); 
					lgStat.lgStat = (LG_STATISTICS_)Marshal.PtrToStructure( pntr, typeof ( LG_STATISTICS_ ) );
					pntr = (IntPtr)(pntr.ToInt32() +Marshal.SizeOf( typeof ( LG_STATISTICS_ ) ));
					// create a new dev stat collection
					lgStat.DevStats = new Management.DeviceStatisticsCollection();
					for ( int devCount = 0 ; devCount < lgStat.lgStat.NumOfDevStats ; devCount++ ) {
						DEV_STATISTICS devStat = new DEV_STATISTICS();
						devStat = (DEV_STATISTICS)Marshal.PtrToStructure( pntr, typeof ( DEV_STATISTICS ) );
						pntr = (IntPtr)(pntr.ToInt32() +Marshal.SizeOf( typeof ( DEV_STATISTICS ) ));
						// add the dev stat to the collection
						lgStat.DevStats.Add( devStat );
					}
					// add the lg stat to the collection
					pAllStats.LgStats.Add( lgStat );
				}
			} 
			catch ( Exception e ) {
				throw ( new OmException( "StopRMD: Error detected.", e ) );
			}
			finally {
				// free the memory
				Marshal.FreeHGlobal( ptr );
			}
			return pAllStats;
		}

		// This API return total size of ALL_LG_STATISTICS required for FTD_GET_STATS_INFO
		public int MAX_SIZE_ALL_LG_STATISTICS( int _TotalLg_, int _TotalDev_) {
			return (( Marshal.SizeOf( typeof ( ALL_LG_STATISTICS_ ) ) ) +
					( Marshal.SizeOf( typeof ( LG_STATISTICS_ ) ) * (_TotalLg_) ) +
					( Marshal.SizeOf( typeof ( DEV_STATISTICS ) ) * (_TotalDev_) ));
		}

	}
}
