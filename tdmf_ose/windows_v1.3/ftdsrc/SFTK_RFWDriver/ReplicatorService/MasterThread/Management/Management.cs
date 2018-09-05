using System;
using System.Text;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Threading;
using MasterThread;
using System.Diagnostics;

namespace MasterThread.Management {
	/// <summary>
	/// Summary description for Management.
	/// </summary>
	public class Management {

		// Error Facility for the management
		public static ErrorFacility m_ErrorFacility = new ErrorFacility( "Replicator", "TdmfReplServer", null, null, 0, 0 );

		public const int N_MAX_IP = 4;


		/// <summary>
		/// Constructor
		/// </summary>
		public Management() {
		}



		/// <summary>
		/// Called once, when TDMF Agent starts.
		/// </summary>
		/// 
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Mngt_Initialize();
		public static void ManagementInitialize() {
			Mngt_Initialize();
		}

		/////////////////////////////////////////////////////////////////////////////
		// group state sent to Collector by StatThread()
		// called from another thread than ftd_mngt_performance_send_data()
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern void Mngt_Performance_Set_Group_Cp( Int16 sGrpNumber, int bIsCheckpoint );


		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern IntPtr Mngt_Create_Lg_Monit();
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern int Mngt_Init_Lg_Monit( IntPtr monitp, eRole role, [MarshalAs(UnmanagedType.LPTStr)]string pstore, [MarshalAs(UnmanagedType.LPTStr)]string phostname, [MarshalAs(UnmanagedType.LPTStr)]string jrnpath );
		public static int InitLgMonitor( LogicalGroup lgp ) {
			lgp.monitp = Mngt_Create_Lg_Monit();
			return Mngt_Init_Lg_Monit( lgp.monitp, lgp.cfgp.role, lgp.cfgp.pstore, lgp.cfgp.phostname, lgp.cfgp.jrnpath );
		}

		public void mngt_init_lg_monit( LogicalGroup lgp ) {
			// TODO: remove the new and fix the lgp.monitp
			Protocol.ftd_mngt_lg_monit_t monitp = new Protocol.ftd_mngt_lg_monit_t();
//			Protocol.ftd_mngt_lg_monit_t monitp = lgp.monitp;

			// requested period value between msgs to Collector
			monitp.monit_dt = ftd_mngt_get_repl_grp_monit_upload_period();
	
			if ( lgp.cfgp.role == eRole.Primary ) {
				// PStore
				string Drive = Path.GetPathRoot( lgp.cfgp.pstore );
				// get the actual size returned by windows explorer instead
				// of "real" disk extents...
				DriveInfo driveInfo = new DriveInfo();
				try {
					monitp.pstore_disk_total_sz = driveInfo.GetTotalSpace( Drive );
				} catch ( Exception e ) {
					OmException.LogException( new OmException ( "Warning: GetFreeSpace failed - " + e.Message, e ) );
					monitp.pstore_disk_total_sz = 0;
				}
			}
			else {
				// Journal
				string Drive = Path.GetPathRoot( lgp.cfgp.jrnpath );
				// get the actual size returned by windows explorer instead
				// of "real" disk extents...
				DriveInfo driveInfo = new DriveInfo();
				try {
					monitp.journal_disk_total_sz = driveInfo.GetFreeSpace( Drive );
				} catch ( Exception e ) {
					OmException.LogException( new OmException ( "Warning: GetFreeSpace failed - " + e.Message, e ) );
					monitp.journal_disk_total_sz = 0;
				}
				// Collector must known IP addr. of PRIMARY system. Get it from cfg file.
				// phostname can be an IP addr. or an host name.
				monitp.iReplGrpSourceIP = Protocol.name_to_ip( lgp.cfgp.phostname );
			}
		}
		// period at which the group monitoring (PStore and Journal size) are checked for any modification.
		long ftd_mngt_get_repl_grp_monit_upload_period() {
			return gTdmfPerfConfig.iReplGrpMonitPeriod / 10;	//1/10 second to seconds
		}



		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Mngt_Do_Lg_Monit( IntPtr monitp, eRole role, [MarshalAs(UnmanagedType.LPTStr)]string pstore, [MarshalAs(UnmanagedType.LPTStr)]string phostname, [MarshalAs(UnmanagedType.LPTStr)]string jrnpath, int lgnum, IntPtr jrnp );
		public static void DoLgMonitor( LogicalGroup lgp ) {
			Mngt_Do_Lg_Monit( lgp.monitp, lgp.cfgp.role, lgp.cfgp.pstore, lgp.cfgp.phostname, lgp.cfgp.jrnpath, lgp.cfgp.lgnum, IntPtr.Zero );
		}
		static int Mon_Timeout_Multiplier = 10;
		
		/// <summary>
		/// Called periodically to perform for various monitoring tasks
		/// related to logical group diagnostic.
		/// </summary>
		/// <param name="lgp"></param>
		public void sftk_mngt_do_lg_monit( LogicalGroup lgp ) {
			//ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath, int lgnum, ftd_journal_t *jrnp ) {
			DateTime now = DateTime.Now;

			// TODO: remove the new and fix the lgp.monitp
			Protocol.ftd_mngt_lg_monit_t monitp = new Protocol.ftd_mngt_lg_monit_t();
			// Protocol.ftd_mngt_lg_monit_t monitp = lgp.monitp;

			//  We implemented a dirty flag in the ftd_mngt_lg_monit_t
			//  structure. Right now this flag is used to check a 
			//  certain amount of time that has passed, but this could
			//  be used in the future to set this flag by an external
			//  function that could be notified when we are reconnecting
			//  to the collector.
			//
			//  The dirty flag right now waits for 10* the timeout
			//  to pass. i.e. if the timeout is 10secs, 
			//  we send the info every 100secs
			//
			//  Modified this to also be a variable that is currently
			//  not initialized. In future we may want to initialize
			//  this variable trough the GUI (Mon_Timeout_Multiplier)

			if ( now.Ticks >= ( monitp.last_monit_ts + monitp.monit_dt ) ) {
				bool sendmsg = false;

				// TODO: Impliment for secondary and journals.
				// check Journal file size ...
				//				if ( lgp.cfgp.role == eRole.Secondary ) {
				//					long jrnlTotalsz = ftd_journal_get_journal_files_total_size( jrnp );
				//
				//					monitp.iDirtyFlag++;
				//					//
				//					// Check if values different, or need to update (dirtyflag)
				//					//
				//					if (( jrnlTotalsz != monitp.last_journal_files_sz )
				//						||  ( monitp.iDirtyFlag > Mon_Timeout_Multiplier ) ) {
				//						//new values, need to update 
				//						sendmsg                         = true;
				//						monitp.iDirtyFlag              = 0;
				//						monitp.curr_journal_files_sz   = jrnlTotalsz;
				//
				//						DWORD   SectorsPerCluster,     // sectors per cluster
				//							BytesPerSector,        // bytes per sector
				//							NumberOfFreeClusters,  // free clusters
				//							TotalNumberOfClusters; // total clusters
				//						string szDrive;
				//						_splitpath(jrnpath,szDrive,0,0,0);
				//						szDrive[2] = '\\';
				//						szDrive[3] = '\0';
				//						if ( GetDiskFreeSpace( szDrive,     // root path
				//							&SectorsPerCluster,     // sectors per cluster
				//							&BytesPerSector,        // bytes per sector
				//							&NumberOfFreeClusters,  // free clusters
				//							&TotalNumberOfClusters  // total clusters
				//							) ) {
				//							monitp.journal_disk_free_sz  = (ftd_int64_t)BytesPerSector * SectorsPerCluster * NumberOfFreeClusters ;  
				//						}
				//						else {
				//							monitp.journal_disk_free_sz  = monitp.journal_disk_total_sz;
				//							//DWORD err = GetLastError();_ASSERT(0);
				//						}
				//					}
				//					//Collector must known IP addr. of PRIMARY system.  Get it from isockp.
				//					//monitp.iReplGrpSourceIP = lgp.isockp.sockp.rip; _ASSERT(lgp.isockp.sockp.rip != 0);
				//				}
				//...or check PStore file size

				if ( lgp.cfgp.role == eRole.Primary ) {
					// get file size
					FileInfo info = new FileInfo( lgp.cfgp.pstore );

					monitp.iDirtyFlag++;
					// Check if values different, or need to update (dirtyflag)
					if (( info.Length != monitp.last_pstore_file_sz ) ||
						( monitp.iDirtyFlag > Mon_Timeout_Multiplier )) {
						//new values, need to update 
						sendmsg = true;
						monitp.iDirtyFlag = 0;
						monitp.curr_pstore_file_sz = info.Length;
						string Drive = Path.GetPathRoot( lgp.cfgp.pstore );
						// get the actual size returned by windows explorer instead
						// of "real" disk extents...
						DriveInfo driveInfo = new DriveInfo();
						try {
							monitp.pstore_disk_free_sz = driveInfo.GetFreeSpace( Drive );
						} catch ( Exception e ) {
							OmException.LogException( new OmException ( "Warning: GetFreeSpace failed - " + e.Message, e ) );
							monitp.pstore_disk_free_sz = monitp.pstore_disk_total_sz;
						}
					}
				}
				// send msg to Collector
				bool waitNextTimeout = true;
				if (( gbTDMFCollectorPresent == false ) &&
					( gbTDMFCollectorPresentWarning == false ) &&
					( !Protocol.IsCollectorAvailable() ) ) {
					m_ErrorFacility.error_syslog( EventLogEntryType.Warning, "****Warning, Collector not found at IP={0}, Port={1}\n", Protocol.giTDMFCollectorIP.ToString(), Protocol.giTDMFCollectorPort.ToString() );
					gbTDMFCollectorPresentWarning = true;
				}
				if (( sendmsg ) &&
					( Protocol.IsCollectorAvailable() ) &&
					( gbTDMFCollectorPresent != false )) {
					try {
						Protocol.ftd_mngt_send_lg_monit( lgp.lgnum, lgp.cfgp.role == eRole.Primary, monitp );
					} catch ( Exception ) {
						// in case Collector could NOT been notified, 
						// make sure to retry on next pass
						waitNextTimeout = false;
					}
				}
				// if Collector has been notified successfully or nothing to be signaled
				if ( waitNextTimeout ) {
					monitp.last_journal_files_sz   = monitp.curr_journal_files_sz;
					monitp.last_pstore_file_sz     = monitp.curr_pstore_file_sz;
					monitp.last_monit_ts           = DateTime.Now.Ticks;
				}
			}
			// in case a new value has been received
			monitp.monit_dt = ftd_mngt_get_repl_grp_monit_upload_period();
		}



		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern int Mngt_Delete_Lg_Monit( IntPtr monitp );
		
		// NOT NEEDED IN C#
		//		/**
		//		 * Called in ftd_lg_cleanup() 
		//		 */
		//		int ftd_mngt_delete_lg_monit(ftd_mngt_lg_monit_t* monitp) {
		//			if ( monitp == NULL )
		//				return -1;
		//
		//			if ( monitp.msg ) {
		//				mmp_mngt_TdmfReplGroupMonitorMsg_t *pmsg = (mmp_mngt_TdmfReplGroupMonitorMsg_t *)monitp.msg;
		//				delete pmsg;
		//			}
		//
		//			free( monitp );
		//			return 0;
		//		}


		/// <summary>
		/// Send this Agent's basic management information message (MMP_MNGT_AGENT_INFO) to TDMF Collector.
		/// TDMF Collector coordinates are based on last time a MMP_MNGT_AGENT_INFO_REQUEST was received from it.
		/// return 0 on success,
		///    +1 if TDMF Collector coordinates are not known,
		///    -1 if cannot connect on TDMF Collector,
		///    -2 on tx error.
		/// </summary>
		[DllImport("RplLibWrapper", SetLastError = true)]
		public static extern int Mngt_Send_Agentinfo_Msg( int rip, int rport );
		public static int Mngt_Send_AgentInfo_Msg() {
			return Mngt_Send_Agentinfo_Msg( 0, 0 );
		}
		public void ftd_mngt_send_agentinfo_msg() {
			if ( Protocol.IsCollectorAvailable() ) {
				return;
			}
			// build Agent Info response msg
			Protocol.mmp_mngt_TdmfAgentConfigMsg_t msg = new Protocol.mmp_mngt_TdmfAgentConfigMsg_t();
			string szOriginServerUID;

			msg.header.mngtstatus = Protocol.eStatus.OK; 

			// This call gets the list of ipaddresses
			ftd_mngt_gather_server_info( msg.data );

			// the following section of code is done only once in NORMAL mode.
			// loop is performed in EMULATOR mode only.
			string szMachineName;
			int iEmulatorIdx = gTdmfAgentEmulator.iAgentRangeMin;
			szMachineName = msg.data.szMachineName;
			szOriginServerUID = msg.data.szServerUID;
			do {
				// send Status Messages to Collector
				Socket socket = Protocol.CollectorConnect();
				try {
					Protocol.SendAgentConfigMsg( socket, msg, Protocol.eManagementCommands.AGENT_INFO );
				} finally {
					socket.Close();
				}

				//emulate different Agents with different host ids
				if ( gTdmfAgentEmulator.bEmulatorEnabled ) {
					//prepare next message, emulating the rpesence of TDMF Agent
					ftd_mngt_emulatorGetHostID( szOriginServerUID, iEmulatorIdx, ref msg.data.szServerUID );
					ftd_mngt_emulatorGetMachineName( iEmulatorIdx, szMachineName, msg.data.szMachineName );
				}
			} while ( ( gTdmfAgentEmulator.bEmulatorEnabled ) && ( iEmulatorIdx++ <= gTdmfAgentEmulator.iAgentRangeMax ) );
		}

		public static void ftd_mngt_emulatorGetHostID( string OriginServerUID, int EmulatorIdx, ref string AgentUniqueID ) {
			//host ID
			AgentUniqueID = EmulatorIdx.ToString("X8");
			// add an hidden information: the real host ID
			AgentUniqueID += "OH=" + OriginServerUID;
		}
		private void ftd_mngt_emulatorGetMachineName( int EmulatorIdx, string MachineName, string EmulatedMachineName ) {
			EmulatedMachineName = MachineName + "_" + EmulatorIdx.ToString();
		}


		// PUSH license key to Collector
		[DllImport("RplLibWrapper", SetLastError = true)]
		public static extern void Mngt_Send_Registration_Key();
		// PUSH license key to Collector
		void ftd_mngt_send_registration_key() {
			Protocol.mmp_TdmfRegistrationKey key = new Protocol.mmp_TdmfRegistrationKey();
			key.RegKey = RegistryAccess.Licence();
			key.ServerUID = UniqueHostID.ToStringID();
			key.iKeyExpirationTime = Management.LicenceKeyExpirationDate( key.RegKey );
			if ( Protocol.IsCollectorAvailable() ) {
				Socket socket = Protocol.CollectorConnect();
				try {
					Protocol.SendRegistrationKey ( socket, key );
				} finally {
					socket.Close();
				}
			}
		}



		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern void Mngt_Shutdown();
		void ftd_mngt_shutdown() {
			// By Saumya 03/06/04
			// With these modifications it will run in a collector less environment also
			// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
			// still work; all the CLIs will work too
			if ( gStatusMessageThread != null ) {
				if ( gStatusMessageThread.m_thisThread.IsAlive ) {

					gStatusMessageThread.m_thisThread.Abort();
				}
			}
		}



		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Mngt_Msgs_Log( IntPtr argv, int argc );
		void ftd_mngt_msgs_log(string [] argv, int argc) {
			DateTime now = DateTime.Now;
			string Message = now.ToString("[yyyy/MM/dd HH:mm:ss]") + String.Format( " {0}: [{1}:{2}] [INFO / TOOL]  {3}\n",
				m_ErrorFacility.m_ErrFac.facility, m_ErrorFacility.m_ErrFac.hostname, Path.GetFileName( argv[0] ), Path.GetFileName( argv[0] ) );
			for ( int i = 1 ; i < argv.Length ; i++ ) {
				Message = Message + argv[i] + " ";
			}
			// write to System Event Log and to gStatusMsgList
			m_ErrorFacility.error_syslog( EventLogEntryType.Information, Message );
			// send messages in gStatusMsgList to Collector
			ManagementStatusThread.ftd_mngt_msgs_send_status_msg_to_collector();
		}

		public unsafe static void LogMessage( string [] args ) {
			// copy the string [] into memory to send the dll
			IntPtr pnt = Marshal.AllocHGlobal( args.Length * sizeof(IntPtr) );
			try {
				for ( int index = 0 ; index < args.Length ; index++ ) {
					Marshal.WriteIntPtr ( pnt, index * sizeof(IntPtr), Marshal.StringToHGlobalAnsi( args[ index ] ) );
				}
				try {
					Mngt_Msgs_Log( pnt, args.Length );
				} finally {
					for ( int index = 0 ; index < args.Length ; index++ ) {
						Marshal.FreeHGlobal( Marshal.ReadIntPtr( pnt, index * sizeof(IntPtr) ) );
					}
				}
			} finally {
				Marshal.FreeHGlobal( pnt );
			}
		}

		/*
		 * A MMP message request has to be received and processed.
		 *
		 * Message is expected to be a MMP MANAGEMENT msg.
		 * Receive message from a SOCK_STREAM connection.
		 */
		/// <summary>
		/// ftd_sock_t
		/// </summary>
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public struct Sock_t {
			public eMagic magicvalue;					/* so we know it's been initialized */
			public int keepalive;					/* don't destroy if TRUE */
			public int contries;					/* peer connect tries */
			public int type;						/* connect type: GENERIC, LG */
			public IntPtr sockp;					/* socket object address */
		}

		/* socket structure */
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public struct sock_t {
			public IntPtr sock;			/* socket for remote communications		*/
			public IntPtr hEvent;
			public int	bBlocking;
			public int type;			/* protocol type - AF_INET, AF_UNIX		*/
			public int family;			/* protocol family - STREAM, DGRAM		*/
			public int tsbias;			/* time diff between local and remote	*/
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
			public string lhostname;	/* local host name						*/
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
			public string rhostname;	/* remote host name						*/
			public int lip;			/* local ip address						*/
			//[MarshalAs(UnmanagedType.ByValArray, SizeConst=4)]
			public int rip;			/* remote ip address					*/
			public int lhostid;		/* local hostid							*/
			public int rhostid;		/* remote hostid						*/
			public int port;			/* connection port number				*/
			public int readcnt;			/* number of bytes read					*/
			public int writecnt;		/* number of bytes written				*/
			public int flags;			/* state bits							*/
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static unsafe extern IntPtr Sock_Create( int type );

		public static int SOCK_GENERIC = 1;

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern unsafe int Mngt_Recv_Msg( IntPtr fsockp );
		public static unsafe int ReceiveMessage ( Socket socket, ref bool KeepSocket ) {

			Sock_t Soc = new Sock_t();
			sock_t soc = new sock_t();
			soc.bBlocking = 1;
			Soc.type = SOCK_GENERIC;
			soc.family = 2;
			//soc.lhostid = 1535676516;
			Soc.magicvalue = eMagic.Socket;
			Soc.contries = 0;
			soc.port = RegistryAccess.MasterPort();
			Soc.keepalive = 1;

			// dup the socket handle so the management C++ code can close it.
			Handle.Duplicate( socket.Handle, out soc.sock );
			// soc.sock = socket.Handle;

			soc.flags |= 1;

			//peer IP address
			IPEndPoint endPoint = (IPEndPoint)socket.RemoteEndPoint;
			//soc.rip  = (int)endPoint.Address.GetAddressBytes();
			soc.port = endPoint.Port;

			int size = (int)(Marshal.SizeOf( typeof(sock_t)));
			Soc.sockp = Marshal.AllocHGlobal( size );
			Marshal.StructureToPtr( soc, Soc.sockp, true );

			size = (int)(Marshal.SizeOf( typeof(Sock_t))) + (int)(Marshal.SizeOf( typeof(sock_t))) + 4 ;
			IntPtr ptr = Marshal.AllocHGlobal( size );
			Marshal.StructureToPtr( Soc, ptr, false );

			Mngt_Recv_Msg( ptr );

			KeepSocket = false;
			return 0;
		}


















		public static bool gbForceTxAllGrpState;
		public static bool gbForceTxAllGrpPerf; 

		// Took out the lastAliveMsgTime variable from the thread
		// this is safe because there is only one copy of this 
		// thread (otherwise we would have to create a more complex 
		// mechanism)
		public static DateTime m_LastAliveMsgTime = DateTime.Now;

		// Set alive message time stamp to current time
		// this allows us to not send a alive message
		// whenever there are other messages being
		// sent around by the agent to the collector
		// minimizing traffic
		public static void ftd_mngt_UpdateAliveMsgTime() {
			m_LastAliveMsgTime = DateTime.Now;
		}

		/// <summary>
		/// receiveMngtMsgThread
		/// </summary>
		/// <param name="socket"></param>
		/// <returns></returns>
		static void Mngt_Recv_Msg( Socket socket ) {
			//ftd_header_t header is read, now read the mmp_mngt_header_t structure 
			Protocol.Header header =  new Protocol.Header();
			Protocol.ReceiveHeader ( socket, ref header );

			// refresh local value used throughout mngt functions
			if ( Protocol.giTDMFCollectorIP != ((IPEndPoint)socket.RemoteEndPoint).Address ) {
				//save collector new IP to registry
				Protocol.giTDMFCollectorIP = ((IPEndPoint)socket.RemoteEndPoint).Address;
				RegistryAccess.CollectorIP( Protocol.giTDMFCollectorIP.ToString() );
			}
			//dispatch
			switch ( header.mngttype ) {
				case Protocol.eManagementCommands.SET_LG_CONFIG:	// receiving a logical group cfg file 
					ftd_mngt_set_config(socket);
					break;
				case Protocol.eManagementCommands.GET_LG_CONFIG:	// request to send back a logical group cfg file
					ftd_mngt_get_config(socket);
					break;
				case Protocol.eManagementCommands.REGISTRATION_KEY:	// SET/GET registration key
					ftd_mngt_registration_key_req(socket);
					break;
				case Protocol.eManagementCommands.TDMF_CMD:			// management cmd, specified by a tdmf_commands sub-cmd
					ftd_mngt_tdmf_cmd(socket);
					break;
				case Protocol.eManagementCommands.SET_AGENT_GEN_CONFIG:
				case Protocol.eManagementCommands.GET_AGENT_GEN_CONFIG:
					ftd_mngt_agent_general_config(socket, header.mngttype);
					break;
				case Protocol.eManagementCommands.GET_ALL_DEVICES:
					ftd_mngt_get_all_devices(socket);
					break;
				case Protocol.eManagementCommands.PERF_CFG_MSG:
					ftd_mngt_get_perf_cfg(socket);
					break;
				case Protocol.eManagementCommands.TDMF_SENDFILE:   // receiving a TDMF file 
					ftd_mngt_set_file(socket);
					break;
				case Protocol.eManagementCommands.TDMF_GETFILE:    // send TDMF file(s)
					ftd_mngt_get_file(socket);
					break;
				case Protocol.eManagementCommands.AGENT_INFO_REQUEST:      // request for host information: IP, listener socket port, ...
				case Protocol.eManagementCommands.SET_ALL_DEVICES:
				case Protocol.eManagementCommands.AGENT_INFO:     // receiving a eManagementCommands.AGENT_INFO response message ???
				default:
					throw new OmException("Error: Mngt_Recv_Msg invalid eManagementCommands - " + header.mngttype );
			}

		}

		/// <summary>
		/// handle reception of a cfg file data
		/// </summary>
		/// <param name="socket"></param>
		public static void ftd_mngt_set_config ( Socket socket ) {
			Protocol.mmp_mngt_ConfigurationMsg_t RcvCfgData;

			//complete reception of group cfg data message 
			byte [] fileData;
			Protocol.mmp_mngt_recv_cfg_data( socket, out RcvCfgData, out fileData );

			//dump file to disk. file data is contiguous to mmp_mngt_ConfigurationMsg_t structure.
			ftd_mngt_write_file_to_disk( RcvCfgData.data.szFilename, fileData );

			// send the data
			Protocol.SendSetConfigStatus( socket, RcvCfgData, 0 );
		}

		public static void ftd_mngt_write_file_to_disk( string fileName, byte[] fileData ) {
			StreamWriter file;

			if ( fileData.Length > 0 ) {
				file = File.CreateText( fileName );
				try {
					file.Write( fileData );
				} finally {
					file.Close();
				}
			}
			else {
				//request to delete file.
				File.Delete( fileName );
			}
		}

		public static void ReadFile( string fileName, out byte [] data ) {
			FileStream file;
			if ( !File.Exists( fileName ) ) {
				throw new OmException( "Error: ReadFile - file does not exist - " + fileName );
			}
			file = File.OpenRead( fileName );
			try {
				data = new byte [ file.Length ];
				file.Read( data, 0, (int)file.Length );
			} finally {
				file.Close();
			}
		}

		public static byte [] ReadConfigFiles( string fileName ) {
			if ( fileName.EndsWith( "*.cfg" ) ) {
				fileName = fileName.Replace( "*.cfg", "p*.cfg" );
			}
			return ReadFiles( fileName );
		}

		public static byte [] ReadFiles( string fileName ) {
			byte [] data = new byte [0];
			string path = RegistryAccess.InstallPath();
			string [] files = Directory.GetFiles( path, fileName );

			foreach ( string name in files ) {
				if ( fileName.ToLower() == "*.bat" ) {
					if (( name.ToLower() == "tdmfconfigtool.bat" ) ||
						( name.ToLower() == "tdmfmonitortool.bat" )) {
						continue;
					}
				}
				byte [] temp;
				ReadFile ( Path.Combine ( path, name ), out temp );
				// append the new data to the existing data
				byte [] tmp = data;
				Protocol.mmp_TdmfFileTransferData header = new Protocol.mmp_TdmfFileTransferData();
				data = new byte [ tmp.Length + temp.Length + header.Size ];
				// copy the old data
				tmp.CopyTo( data, 0 );
				//init mmp_TdmfFileTransferData portion of new file data
				header.iType = Protocol.eFileType.CFG;
				header.iSize = temp.Length;
				header.szFilename = name;
				// copy the header
				header.ToBytes().CopyTo( data, tmp.Length );
				// copy the data
				temp.CopyTo( data, tmp.Length + header.Size );
			}
			return data;
		}

		public static void ftd_mngt_get_config ( Socket socket ) {
			Protocol.mmp_mngt_ConfigurationMsg_t RcvCfgMsg;
			//complete reception of logical group cfg data message 
			byte [] fileData;
			Protocol.mmp_mngt_recv_cfg_data( socket, out RcvCfgMsg, out fileData );
			//create a vector of ( mmp_TdmfFileTransferData + file data )
			fileData = ReadConfigFiles( RcvCfgMsg.data.szFilename );
			// send status message and data to requester   
			Protocol.mmp_mngt_build_SetConfigurationMsg( socket,
				RcvCfgMsg.szServerUID,
				Protocol.eSenderType.AGENT,
				Protocol.eManagementCommands.SET_LG_CONFIG,
				RcvCfgMsg.data.szFilename,
				fileData );
		}


		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern int Mngt_Key_Get_Licence_Key_Expiration_Date ( [MarshalAs(UnmanagedType.LPTStr)]string key );

		public static int LicenceKeyExpirationDate( string key ) {
			return Mngt_Key_Get_Licence_Key_Expiration_Date( key );
		}
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern bool IsRegistrationKeyValid ( [MarshalAs(UnmanagedType.LPTStr)]string key );

		public static bool IsKeyValid( string key ) {
			return IsRegistrationKeyValid( key );
		}

		/// <summary>
		/// Received a mmp_mngt_header_t indicating a MMP_MNGT_REGISTRATION_KEY command
		/// read and process cmd
		/// </summary>
		/// <param name="socket"></param>
		public static void ftd_mngt_registration_key_req ( Socket socket ) {
			Protocol.mmp_TdmfRegistrationKey key = new Protocol.mmp_TdmfRegistrationKey();

			// at this point, mmp_mngt_header_t header is read.
			// now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
			Protocol.ReceiveHeader( socket, ref key );

			// no multi-byte value to convert from network bytes order to host byte order
			// if key is empty, it is a request to get the existing key.
			// otherwise , it is a request to set the existing key
			if ( key.RegKey.Length == 0 ) {
				//get key
				key.RegKey = RegistryAccess.Licence();
				key.iKeyExpirationTime = LicenceKeyExpirationDate( key.RegKey );
			}
			else {
				//set key
				if ( IsKeyValid( key.RegKey ) ) {
					RegistryAccess.Licence( key.RegKey );
					key.iKeyExpirationTime = LicenceKeyExpirationDate( key.RegKey );
				}
			}
			Protocol.SendRegistrationKey( socket, key );
		}

		public static bool isValidBABSize( int iBABSizeMb ) {
			int maxPhysMemKb;

			// If the OS version is Windows 2000 or greater,
			// GlobalMemoryStatusEx must be used instead of
			// GlobalMemoryStatus in case the memory size
			// exceeds 4 GBs. 
			Memory memory = new Memory();
			maxPhysMemKb = (int)(((memory.ullTotalPhys / 1024) * 6) / 10);

			// maxPhysMemKb is equal to the total physical memory
			// divided by 2. So if the Bab size in KB is lower than
			// or equal to 60% of the total physical memory,
			// return TRUE.
			if ( (iBABSizeMb * 1024) <= maxPhysMemKb ) {
				return true;
			}
			return false;
		}

		public static void ftd_mngt_gather_server_info( Protocol.mmp_TdmfServerInfo srvrInfo ) {

			string [] IPs = Protocol.GetIPAddresses( "localhost" );
			srvrInfo.ucNbrIP = (byte)IPs.Length;

			if ( srvrInfo.ucNbrIP > N_MAX_IP ) {
				srvrInfo.ucNbrIP = N_MAX_IP;
			}

			//build "a.b.c.d" strings for IP address
			for ( int i = 0 ; i < srvrInfo.ucNbrIP ; i++ ) {
				srvrInfo.szIPAgent[i] = IPs[i];
			}

			//get host id 
			srvrInfo.szServerUID = UniqueHostID.ToStringID();

			srvrInfo.szMachineName = SystemInformation.ComputerName;

			// todo : get router info
			srvrInfo.szIPRouter = "0.0.0.0";	//clean the field

			if ( RegistryAccess.IPTranslation().ToLower() == "on" ) {
				srvrInfo.szIPRouter = "*.*.*.*";
			}

			srvrInfo.szTDMFDomain = RegistryAccess.Domain();

			srvrInfo.iBABSizeReq = 0;

			// Report requested BAB size
			srvrInfo.iBABSizeReq = ( RegistryAccess.ChunkSize() * RegistryAccess.NumberChunks() ) / (1024*1024);
			srvrInfo.iTCPWindowSize = RegistryAccess.TCPWindowSize();
			srvrInfo.iPort = RegistryAccess.MasterPort();
			srvrInfo.szTdmfPath = RegistryAccess.InstallPath();
			srvrInfo.szTdmfPStorePath = srvrInfo.szTdmfPath;
			srvrInfo.szTdmfJournalPath = srvrInfo.szTdmfPath;
			if ( !srvrInfo.szTdmfPath.EndsWith( "\\" ) ) {
				srvrInfo.szTdmfPStorePath +=  "\\";
				srvrInfo.szTdmfJournalPath += "\\";
			}
			srvrInfo.szTdmfPStorePath += "PStore";
			srvrInfo.szTdmfJournalPath += "Journal";

			if ( gbEmulUnix ) {
				srvrInfo.szTdmfPStorePath = "/pstore";
				srvrInfo.szTdmfJournalPath = "/journal";
			}

			// get bab size from driver
			try {
				DriverIOCTL driver = new DriverIOCTL();
				try {
					driver.GetBabSize( ref srvrInfo.iBABSizeAct );
					srvrInfo.iBABSizeAct = srvrInfo.iBABSizeAct >> 20;	// convert to MB
				} finally {
					driver.Close();
				}

				// Verify that size == requested
				// Otherwise dump an error in the log
				if (srvrInfo.iBABSizeAct != srvrInfo.iBABSizeReq) {
					OmException.LogException( new OmException ( "Warning: Cache size is not requested size - " + srvrInfo.iBABSizeAct + ", " + srvrInfo.iBABSizeReq ) );
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException ( "Warning: Error opening driver - " + e.Message, e ) );
				srvrInfo.iBABSizeAct = 0; // Driver NOT AVAILABLE
			}

			// qdsreleasenumber is defined in version.c
			// strcpy( srvrInfo.szTdmfVersion, (char*)qdsreleasenumber );
			srvrInfo.szTdmfVersion = "v" + Application.ProductVersion;

			// get number of processor in system
			srvrInfo.iNbrCPU = RegistryAccess.CPUCount();

			Memory statex = new Memory();
			srvrInfo.iAvailableRAMSize = (int)statex.ullAvailPhys / 1024;//Bytes to KBytes
			srvrInfo.iRAMSize          = (int)statex.ullTotalPhys / 1024;//Bytes to KBytes

			srvrInfo.szOsType = "MS Windows";
			Version version = Environment.OSVersion.Version;
			switch ( version.Major ) {
				case 4:
					//Windows NT 4
					srvrInfo.szOsVersion = "NT4 ";
					break;
				case 5:
					if ( version.Minor == 0 ) {
						//Windows 2000
						srvrInfo.szOsVersion = "2000 ";
					} else if ( version.Minor == 1 ) {
						//Windows XP or .NET Server
						srvrInfo.szOsVersion = "XP ";
					} else if ( version.Minor == 2 ) {
						srvrInfo.szOsVersion = "2003 ";
					} else {
						srvrInfo.szOsVersion = "Unknown version!! ";
					}
					break;
			}
			// append service pack info, if any.
			srvrInfo.szOsVersion += "." + version.Build + "." + version.Revision;

			if ( gbEmulUnix ) {
				srvrInfo.szOsType = "Unix";
				srvrInfo.szOsVersion = "(emulation)";
			}
		}

		/// <summary>
		/// Manages MMP_MNGT_SET_AGENT_GEN_CONFIG or MMP_MNGT_GET_AGENT_GEN_CONFIG requests.
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="MngtType"></param>
		public static void ftd_mngt_agent_general_config ( Socket socket, Protocol.eManagementCommands MngtType ) {
			Protocol.mmp_mngt_TdmfAgentConfigMsg_t cmdmsg = new Protocol.mmp_mngt_TdmfAgentConfigMsg_t();
			Protocol.mmp_TdmfServerInfo info = new Protocol.mmp_TdmfServerInfo();
			bool requestRestart = false;

			// at this point, mmp_mngt_header_t header is read.
			// now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
			Protocol.ReceiveHeader( socket, ref info );

			try {
				//reuse cmdmsg for response to requester
				cmdmsg.header.mngtstatus  = Protocol.eStatus.OK;//assuming success

				if ( MngtType == Protocol.eManagementCommands.SET_AGENT_GEN_CONFIG ) {
					// validate entries before saving.
					// BAB size cannot be more than 60% of total RAM size
					if ( !isValidBABSize( cmdmsg.data.iBABSizeReq ) ) {
						cmdmsg.header.mngtstatus = Protocol.eStatus.ERR_INVALID_SET_AGENT_GEN_CONFIG;
					}
					else {
						// must check if critital values are changed. if so, a system restart will be requested.
						int NumChunks;
						int ChunkSize;
						int curValue;

						requestRestart = false;
            
						NumChunks = RegistryAccess.NumberChunks();
						ChunkSize = RegistryAccess.ChunkSize();
						curValue = ( NumChunks * ChunkSize ) / ( 1024 * 1024 );

						requestRestart = ( requestRestart || ( curValue != cmdmsg.data.iBABSizeReq ) );

						curValue = RegistryAccess.MasterPort();
						requestRestart = ( requestRestart || ( curValue != cmdmsg.data.iPort ) );

						if ( requestRestart && ( ChunkSize > 0 ) ) {
							NumChunks = ( cmdmsg.data.iBABSizeReq * ( 1024 * 1024 )) / ChunkSize;
							RegistryAccess.NumberChunks( NumChunks );
						}
						cmdmsg.data.iTCPWindowSize *= 1024;	//KBytes to Bytes
						RegistryAccess.TCPWindowSize( cmdmsg.data.iTCPWindowSize );
						RegistryAccess.MasterPort( cmdmsg.data.iPort );
						RegistryAccess.Domain( cmdmsg.data.szTDMFDomain );
						//TODO: Router IP ?
					}
				}
				else {
					//get config
					ftd_mngt_gather_server_info( cmdmsg.data );
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( "Error: ftd_mngt_agent_general_config - " + e.Message, e ) );
				cmdmsg.header.mngtstatus  = Protocol.eStatus.ERR_SET_AGENT_GEN_CONFIG;
			}
			finally {
				//complete response msg and send it to requester
				Protocol.SendAgentConfigMsg( socket, cmdmsg, MngtType );

				if ( requestRestart ) {  
//					ftd_mngt_sys_request_system_restart();
				}
			}
		}
//		// called when critical values like BAB size, TCP Port or TCP Window size have been modified
//		public static void ftd_mngt_sys_request_system_restart() {
//			gcMngtSysNeedToRestart = true;
//		}
//		// 0 = no need to restart, otherwise need to restart TDMF Agent FTD Master Thread
//		public static bool ftd_mngt_sys_need_to_restart_system() {
//			return gcMngtSysNeedToRestart;
//		}


		/* global variables */
		public static bool gbTDMFCollectorPresent;
		public static bool gbTDMFCollectorPresentWarning;
		public static bool gbEmulUnix;
		public static Protocol.mmp_TdmfPerfConfig gTdmfPerfConfig = new Protocol.mmp_TdmfPerfConfig();
		public static TDMFAgentEmulator gTdmfAgentEmulator = new TDMFAgentEmulator();
		public static bool gcMngtSysNeedToRestart = false;
		public static ManagementStatusThread gStatusMessageThread;


		public struct TDMFAgentEmulator {
			public bool bEmulatorEnabled;		// 0 = false, otherwise true.
			public int iAgentRangeMin;
			public int iAgentRangeMax;
		}

		/*
		 * Called once, when TDMF Agent starts.
		 */
		public static void ftd_mngt_initialize() {

			Protocol.giTDMFCollectorIP = IPAddress.Parse( RegistryAccess.CollectorIP() );

			if ( Protocol.IsCollectorAvailable() ) {
				Protocol.giTDMFCollectorPort = RegistryAccess.CollectorPort();

				gTdmfPerfConfig.iPerfUploadPeriod = RegistryAccess.PerfUploadPeriod();

				gTdmfPerfConfig.iReplGrpMonitPeriod = RegistryAccess.ReplGroupMonitUploadPeriod();

				gTdmfAgentEmulator.bEmulatorEnabled = RegistryAccess.TDMFAgentEmulator();

				gTdmfAgentEmulator.iAgentRangeMin = RegistryAccess.EmulatorRangeMin();

				gTdmfAgentEmulator.iAgentRangeMax = RegistryAccess.EmulatorRangeMax();

				gbEmulUnix = RegistryAccess.Emulator();
			}
			// launch the thread responsible for transfering Status messages
			// to TDMF Collector (messages sent to Event Viewer)
			// This thread also contains the processing related to the Agent Alive Socket
			// Create and start the Status thread.
			gStatusMessageThread = new ManagementStatusThread();
			gStatusMessageThread.m_thisThread = new Thread( new ThreadStart( gStatusMessageThread.Start ) );
			// start the statistics thread
			gStatusMessageThread.m_thisThread.Name = ManagementStatusThread.Name();
			gStatusMessageThread.m_thisThread.Start();

			gcMngtSysNeedToRestart = false;
		}

		/// <summary>
		/// Make sure to update Collector with ALL the LATEST information.
		/// </summary>
		public static void ftd_mngt_performance_send_all_to_Collector() {
			gbForceTxAllGrpState = true;
			gbForceTxAllGrpPerf = true; 
		}






		/// <summary>
		/// Manages MMP_MNGT_GET_ALL_DEVICES request
		/// </summary>
		/// <param name="socket"></param>
		public static void ftd_mngt_get_all_devices ( Socket socket ) {
			Protocol.mmp_mngt_TdmfAgentDevices_t data = new Protocol.mmp_mngt_TdmfAgentDevices_t();
			Protocol.DeviceInfoCollection DeviceInfos = new Protocol.DeviceInfoCollection();

			// at this point, mmp_mngt_header_t header is read.
			// now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
			// don't care about it, just empty socket to be able to response
			Protocol.ReceiveHeader( socket, ref data );

			try {
				DeviceInfos = ftd_mngt_acquire_alldevices();
			} catch ( Exception e ) {
				OmException.LogException( new OmException( "Error: ftd_mngt_get_all_devices - " + e.Message, e ) );
			} finally {
				//respond using same socket.
				Protocol.SendAgentDevices ( socket, DeviceInfos );
			}
		}

		public enum eFileSystemTypes {
			UNKNOWN = 0,		//unknown or unformatted partition/volume
			FAT = 10,			//FAT (Windows) File Systems 10 to 19
			FAT16 = 11,  
			FAT32 = 12,
			NTFS = 20,			//NTFS (Windows) File Systems 
			NTFS_DYN = 1010,
			FAT_DYN = 1011,
			FAT16_DYN = 1012,
			FAT32_DYN = 1020,
		}

		public static string SystemType( string Path, string systemName ) {
			// Steve add basic/dynamic detection
			// TODO: impliment			bool basic = IsDiskBasic( Path );
			bool basic = false;
			switch ( systemName.ToLower() ) {
				case "fat":
					if ( !basic ) {
						return eFileSystemTypes.FAT_DYN.ToString();
					}
					return eFileSystemTypes.FAT.ToString();
				case "fat16":
					if ( !basic ) {
						return eFileSystemTypes.FAT16_DYN.ToString();
					}
					return eFileSystemTypes.FAT16.ToString();
				case "fat32":
					if ( !basic ) {
						return eFileSystemTypes.FAT32_DYN.ToString();
					}
					return eFileSystemTypes.FAT32.ToString();
				case "ntfs":
					if ( !basic ) {
						return eFileSystemTypes.NTFS_DYN.ToString();
					}
					return eFileSystemTypes.NTFS.ToString();
				default:
					return eFileSystemTypes.UNKNOWN.ToString();
			}
		}

		public static Protocol.DeviceInfoCollection ftd_mngt_acquire_alldevices() {
			Protocol.DeviceInfoCollection DeviceInfos = new Protocol.DeviceInfoCollection();
			Protocol.DeviceInfo DeviceInfo;
			// get all of the valid drive letters.
			string [] drives = Directory.GetLogicalDrives();

			foreach ( string drive in drives ) {
				DeviceInfo = new Protocol.DeviceInfo();

				DriveInfo.eDriveType DriveType = DriveInfo.GetDriveType ( drive );

				switch ( DriveType ) {
					case DriveInfo.eDriveType.DRIVE_REMOVABLE:
					case DriveInfo.eDriveType.DRIVE_CDROM:
					case DriveInfo.eDriveType.DRIVE_RAMDISK:
					case DriveInfo.eDriveType.DRIVE_REMOTE:
						continue;
				}
				// let use these drive types
				// get the drive signature and volume info
				Config.GetDiskSignatureAndInfo( drive.Substring(0,2), out DeviceInfo.szDriveId, out DeviceInfo.szStartOffset, out DeviceInfo.szLength, out DeviceInfo.FreeSpace );
				DriveInfo driveInfo = new DriveInfo();
				driveInfo.GetDriveInfo( drive );
				DeviceInfo.szDrivePath = drive;
				SystemType( DeviceInfo.szDrivePath, driveInfo.GetFileSystemName );
				DeviceInfos.Add( DeviceInfo );

				// now find any mount points
				StringBuilder MountPt = new StringBuilder( Globals._MAX_PATH );

				VolumeMountPoint volumeMountPoint = new VolumeMountPoint();
				try {
					if ( volumeMountPoint.FindFirstVolumeMountPoint( drive, MountPt ) ) {
						do {
							DeviceInfo = new Protocol.DeviceInfo();
							string MountPtFullName;
							MountPtFullName = drive.Substring( 0, 1 ) + ":\\" + MountPt;

							try {
								Config.GetMntPtSigAndInfo( MountPtFullName, out DeviceInfo.szDriveId, out DeviceInfo.szStartOffset, out DeviceInfo.szLength, out DeviceInfo.FreeSpace );
							} 
							catch ( OmException e ) {
								OmException.LogException( new OmException( "Warning: GetMntPtSigAndInfo failed" + e.Message, e ) );
							}
							driveInfo = new DriveInfo();

							DeviceInfo.szDrivePath = MountPtFullName;
							DeviceInfos.Add( DeviceInfo );

						} while ( volumeMountPoint.FindNextVolumeMountPoint( MountPt ) );
					}
				} finally {
					volumeMountPoint.CloseVolumeMountPoint();
				}
			}
			return DeviceInfos;
		}

		public static void ftd_mngt_get_perf_cfg ( Socket socket ) {
			Protocol.mmp_TdmfPerfConfig data = new Protocol.mmp_TdmfPerfConfig();

			// at this point, mmp_mngt_header_t header is read.
			// now read the remainder of the mmp_mngt_TdmfPerfCfgMsg_t structure 
			Protocol.ReceiveHeader( socket, ref data );

			// save config to registry
			// do same thing for all values in msg.data
			if ( data.iPerfUploadPeriod > 0 ) {
				// modify dynamic instance of this value
				gTdmfPerfConfig.iPerfUploadPeriod = data.iPerfUploadPeriod;
				if ( gTdmfPerfConfig.iPerfUploadPeriod <= 0 ) {
					gTdmfPerfConfig.iPerfUploadPeriod = 100;	//10 seconds
				}
				RegistryAccess.PerfUploadPeriod( gTdmfPerfConfig.iPerfUploadPeriod );
			}
			if ( data.iReplGrpMonitPeriod > 0 ) {
				// modify dynamic instance of this value
				gTdmfPerfConfig.iReplGrpMonitPeriod = data.iReplGrpMonitPeriod;
				if ( gTdmfPerfConfig.iReplGrpMonitPeriod <= 0 ) {
					gTdmfPerfConfig.iReplGrpMonitPeriod = 100;	//10 seconds
				}
				RegistryAccess.ReplGroupMonitUploadPeriod( gTdmfPerfConfig.iReplGrpMonitPeriod );
			}
		}


		/// <summary>
		/// handle reception of a TDMF file
		/// </summary>
		/// <param name="socket"></param>
		public static void ftd_mngt_set_file ( Socket socket ) {
			Protocol.mmp_mngt_ConfigurationMsg_t RcvCfgData;

			//complete reception of group cfg data message 
			byte [] fileData;
			Protocol.mmp_mngt_recv_cfg_data( socket, out RcvCfgData, out fileData );

			//dump file to disk. file data is contiguous to mmp_mngt_ConfigurationMsg_t structure.
			ftd_mngt_write_file_to_disk( RcvCfgData.data.szFilename, fileData );

			// send the data
			Protocol.SendSetFileStatus( socket, RcvCfgData, 0 );
		}


		/// <summary>
		/// requested for actual configuration of one logical group cfg file
		/// </summary>
		/// <param name="socket"></param>
		public static void ftd_mngt_get_file ( Socket socket ) {
			Protocol.mmp_mngt_ConfigurationMsg_t RcvCfgMsg;
			//complete reception of logical group cfg data message 
			byte [] fileData;
			Protocol.mmp_mngt_recv_cfg_data( socket, out RcvCfgMsg, out fileData );
			//create a vector of ( mmp_TdmfFileTransferData + file data )
			fileData = ReadApplicationFiles( RcvCfgMsg.data.szFilename );
			// send status message and data to requester   
			Protocol.mmp_mngt_build_SetConfigurationMsg( socket,
				RcvCfgMsg.szServerUID,
				Protocol.eSenderType.AGENT,
				Protocol.eManagementCommands.TDMF_SENDFILE,
				RcvCfgMsg.data.szFilename, 
				fileData );
		}
		public static byte [] ReadApplicationFiles( string fileName ) {
			if ( fileName.ToLower().IndexOf( "*." ) != -1 ) {
				fileName = "*.bat";
			}
			return ReadFiles( fileName );
		}

		/// <summary>
		/// Received a mmp_mngt_header_t indicating a MMP_MNGT_TDMF_CMD command
		/// read and process cmd
		/// </summary>
		/// <param name="socket"></param>
		public static void ftd_mngt_tdmf_cmd ( Socket socket ) {
			int exitcode = -1;			//assume error
			string szShortApplicationName;// name of executable module
			string szApplicationName;// name of executable module
			string szCurrentDirectory;// current directory name

			Protocol.mmp_mngt_TdmfCommand data = new Protocol.mmp_mngt_TdmfCommand();

			// at this point, mmp_mngt_header_t header is read.
			// now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
			Protocol.ReceiveHeader( socket, ref data );

			// analyze mmp_mngt_TdmfCommandMsg_t content
			// validate
			if ( data.iSubCmd < Protocol.eCommands.FIRST_TDMF_CMD || data.iSubCmd > Protocol.eCommands.LAST_TDMF_CMD ) {
				throw new OmException( "Error: Invalid commands received - " + data.iSubCmd );
			}

			/////////////////////////////////////////
			//retreive values used to execute command
			szCurrentDirectory = RegistryAccess.InstallPath();

			bool bNeedToCaptureConsoleOutput = true;

			szApplicationName = szCurrentDirectory + "/";

			switch( data.iSubCmd ) {
				case Protocol.eCommands.START:
					szShortApplicationName = "start";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.STOP:
					szShortApplicationName = "stop";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.INIT:
					szShortApplicationName = "init";
					break;
				case Protocol.eCommands.OVERRIDE:
					szShortApplicationName = "override";
					break;
				case Protocol.eCommands.INFO:
					szShortApplicationName = "info";       
					break;
				case Protocol.eCommands.HOSTINFO:
					szShortApplicationName = "hostinfo";   
					break;
				case Protocol.eCommands.LICINFO:
					szShortApplicationName = "licinfo";    
					break;
				case Protocol.eCommands.RECO:
					szShortApplicationName = "reco";
					break;
				case Protocol.eCommands.SET:
					szShortApplicationName = "set";
					break;
				case Protocol.eCommands.LAUNCH_PMD:
					szShortApplicationName = "launchpmd";  
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.LAUNCH_REFRESH:
					szShortApplicationName = "launchrefresh";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.LAUNCH_BACKFRESH:
					szShortApplicationName = "launchbackfresh";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.KILL_PMD:
					szShortApplicationName = "killpmd";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.KILL_RMD:
					szShortApplicationName = "killrmd";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.KILL_REFRESH:
					szShortApplicationName = "killrefresh";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.KILL_BACKFRESH:
					szShortApplicationName = "killbackfresh";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 10 ), true );//for next 10 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.CHECKPOINT:
					szShortApplicationName = "checkpoint";
					PerformanceStats.ftd_mngt_performance_reduce_stat_upload_period( new TimeSpan( 0, 0, 30 ), true );//for next 30 seconds, sends stats to Collector at each second
					break;
				case Protocol.eCommands.TRACE:
					szShortApplicationName = "trace";
					break;
				case Protocol.eCommands.OS_CMD_EXE:
					szShortApplicationName = "cmd.exe /C";
					bNeedToCaptureConsoleOutput = false;
					break;
				case Protocol.eCommands.TESTTDMF:
					szShortApplicationName = "test";		//only used for test !!
					break;
				case Protocol.eCommands.HANDLE:
					szShortApplicationName = "Analyzer";
					break;
				default:
					throw new OmException( "Error: don't know executable file name for sub-cmd - " + data.iSubCmd );
			}
			string output = "";
			try {
				//launch process command
				Process process = new Process();
				if ( data.iSubCmd == Protocol.eCommands.OS_CMD_EXE ) {
					process.StartInfo.FileName = szShortApplicationName;
					process.StartInfo.Arguments = String.Format( "{0}", data.szCmdOptions );
				} else {
					process.StartInfo.FileName = "CLI.exe";
					process.StartInfo.Arguments = String.Format( "{0} {1}", szShortApplicationName, data.szCmdOptions );
				}
				process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
				process.StartInfo.UseShellExecute = false;
				process.StartInfo.CreateNoWindow = true;
				process.StartInfo.RedirectStandardError = true;
				process.Start();
				process.WaitForExit();
				// set the exit code
				exitcode = process.ExitCode;

				if ( bNeedToCaptureConsoleOutput ) {
					//accumulate all console output into pData buffer
					output = process.StandardError.ReadToEnd();

					// console messages are sent back to Collector as Status Messages using
					// StatusMsgThread (below)
					string pTimeAndConsoleMsg;
					pTimeAndConsoleMsg = m_ErrorFacility.error_format_datetime( "INFO", "CMD", output );
					m_ErrorFacility.error_msg_list_addMessage( EventLogEntryType.Information, pTimeAndConsoleMsg );
				}
				// filter some return codes:
				// cmd.exe returns 1 if directory already exists.
				if (( data.iSubCmd == Protocol.eCommands.OS_CMD_EXE ) && ( exitcode == 1 )) {
					exitcode = 0;	//ok
				}
			} finally {
				//send exit code back to caller
				Protocol.SendAgentStatus ( socket, exitcode, data, output );
			}
		}


//		//request that the perf upload period be 1 second for the next N seconds
//		//after that period, it falls back to the configured gTdmfPerfConfig.iPerfUploadPeriod.
//		public static void ftd_mngt_performance_reduce_stat_upload_period( TimeSpan iFastPerfPeriodSeconds, bool bWakeUpStatThreadNow ) {
//			DateTime now = DateTime.Now;
//			if ( PerformanceStats.guiTdmfPerf_FastRateSendEndTime > now ) {
//				//add on top of remainding delay already requested
//				PerformanceStats.guiTdmfPerf_FastRateSendEndTime = PerformanceStats.guiTdmfPerf_FastRateSendEndTime + iFastPerfPeriodSeconds;
//			}
//			else {
//				PerformanceStats.guiTdmfPerf_FastRateSendEndTime = now + iFastPerfPeriodSeconds;
//			}
//			if ( bWakeUpStatThreadNow ) {
//				PerformanceStats.ftd_mngt_performance_force_acq();//immediat wake up of StatThread
//			}
//		}

//		[DllImport("RplLibWrapper", SetLastError = true)]
//		public static extern IntPtr Performance_Get_Force_Aquire_Event();
//		[DllImport("kernel32", SetLastError=true)]
//		static extern unsafe bool SetEvent(IntPtr hEvent);
//
//		// immediate request to statd to acquire statistics
//		public static void ftd_mngt_performance_force_acq() {
//			if ( Performance_Get_Force_Aquire_Event() != IntPtr.Zero ) {
//				SetEvent( Performance_Get_Force_Aquire_Event() );
//			}
//		}

	}
}
