using System;
using System.IO;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Diagnostics;

using System.Management;

using System.ComponentModel;
using System.Data;

using System.Reflection;

using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Tcp;
using System.Runtime.Serialization.Formatters;


namespace MasterThread {
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	public class MasterThread : System.MarshalByRefObject {

		public bool m_ShutDown = false;
		public bool m_Active = true;

		public static StringCollection m_MacAddresses = new StringCollection();
		public static uint m_MacAddress;

		// list of all configs sent to driver.
		public static LgConfigCollection m_LgConfigs = new LgConfigCollection();

		protected static string m_CollectorIP;
		protected static int m_CollectorPort;
		protected static int m_MasterPort;
		protected static MasterListenerCollection m_Listeners = new MasterListenerCollection();

		// save the threads RMD and PMD in the collection
		XMDThreadCollection m_XMDThreadCollection = new XMDThreadCollection();      // child threads PMD, RMD, Stat, etc.

		SocketCollection m_SocketCollection;		// save all of the socket in the this collection

		public static IntPtr m_StatdEvent;		// event to signal statd


		/// <summary>
		/// Constructor
		/// </summary>
		public MasterThread() {
		}

		/// MasterThread - master thread
		/// </summary>
		/// <returns></returns>
		public void MasterThreadStart() {
			try {
				ManagementClass mc;
				mc = new ManagementClass("Win32_NetworkAdapterConfiguration");
				ManagementObjectCollection moc = mc.GetInstances();

				// get the MacAddresses for this system
				foreach(ManagementObject mo in moc){
					if (mo["IPEnabled"].ToString()=="True") {
						m_MacAddresses.Add( mo["MacAddress"].ToString() ) ;
					}
				}
				if ( m_MacAddresses.Count > 0 ) {
					string address = m_MacAddresses[0];
					uint addr = Convert.ToByte( address.Substring( 6, 2 ), 16 );
					m_MacAddress = addr << 24;
					addr = Convert.ToByte( address.Substring( 9, 2 ), 16 );
					m_MacAddress += addr << 16;
					addr = Convert.ToByte( address.Substring( 12, 2 ), 16 );
					m_MacAddress += addr << 8;
					addr = Convert.ToByte( address.Substring( 15, 2 ), 16 );
					m_MacAddress += addr;
				} else {
					OmException.LogException( new OmException("Warning: this machine does not have a valid Mac Address." ) );
				}
				// get the collector ip and port values
				m_CollectorIP = RegistryAccess.CollectorIP();
				m_CollectorPort = RegistryAccess.CollectorPort();

				// Set the MasterListener port
				// get the port number from the registry
				m_MasterPort = RegistryAccess.MasterPort();

				// So the CLI can talk to us via remote objects
				BinaryServerFormatterSinkProvider provider = new BinaryServerFormatterSinkProvider();
				provider.TypeFilterLevel = TypeFilterLevel.Full;
				TcpServerChannel channel = new TcpServerChannel ( "CLI Server", m_MasterPort + 2, provider );
				ChannelServices.RegisterChannel ( channel );
				// Register the CLI to MasterThread communications object.
				RemotingConfiguration.RegisterWellKnownServiceType (
					typeof( MasterThread ),
					"MasterThread",
					WellKnownObjectMode.SingleCall );
				// Register the Console to MasterThread communications object.
				RemotingConfiguration.RegisterWellKnownServiceType (
					typeof( ConsoleAPI ),
					"ConsoleAPI",
					WellKnownObjectMode.SingleCall );

				ErrorFacility.Init("Replicator", "TdmfReplServer", null, null, 0, 0);
				try {
					int temp = (int)ErrorFacility.eTraceLevel.TRACEWRN;
					ErrorFacility.SetErrorTraceLevel(temp.ToString());

					// This fixes the multiple IP's on one machine bug
					// We need to know which IP the collector will talk to
					// To do this, we check the ip trough which he can reach us
					// Make sure the collector ip is known!
					Management.Protocol.Mngt_Initialize_Collector_Values();

					// Create and start the Statistics thread.
					StatisticsThread stat = new StatisticsThread();
					stat.m_thisThread = new Thread( new ThreadStart( stat.Start ) );
					// start the statistics thread
					stat.m_thisThread.Name = StatisticsThread.Name();
					stat.m_thisThread.Start();

					// save the threads RMD and PMD in the collection
					m_XMDThreadCollection.Add( stat );

					// use all local network cards
					// create the listener socket
					m_Listeners.Add( new MasterListener( IPAddress.Any, m_MasterPort ) );
					// Listen again for the older coder port
					int port = IPAddress.HostToNetworkOrder( (short)m_MasterPort );
					m_Listeners.Add( new MasterListener( IPAddress.Any, port ) );

					// Start listening for client requests.
					foreach( MasterListener listener in m_Listeners ) {
						listener.Start();
					}

					// Master Thread started for the first time, automatically signal the presence of SEC Master to
					// PRI Master (verify the existence of (sXXX.cfg files)
					// Start the Remote Wake-up thread
					// Send a WAKEUP Signal to Primary Master using ftd_rem_wakeup()
					Thread WakeUpThread = new Thread( new ThreadStart( RemoteWakeUpThread ) );
					// start the Wakeup thread
					WakeUpThread.Start();

					try {
#if USE_COLLECTOR
						///////////////////////////////////////////////////
						// Management stuff
						//	ManagementOld.ManagementInitialize();
						//if TDMF Collector is known, send our TDMF Agent information
						//	ManagementOld.Mngt_Send_AgentInfo_Msg();
						//	ManagementOld.Mngt_Send_Registration_Key();
						///////////////////////////////////////////////////
#endif

						// load all of the logical groups
						try {
							ConsoleAPI api = new ConsoleAPI();
							api.ApplyAllGroupsXML();
						} catch ( Exception e ) {
							OmException.LogException( new OmException ( "Error: ApplyAllGroups Failed - " + e.Message, e ) );
						}
						/* process IO events */
						ProcessIO();

					} finally {
#if USE_COLLECTOR
						//	ManagementOld.Mngt_Shutdown();
#endif
					}
				} finally {
					ErrorFacility.Close();
				}
			}
			catch( SocketException e ) {
				OmException.LogException( new OmException( String.Format("Error in MasterThreadStart Socket Error : {0}", e.Message ), e ) );
			}
			catch( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in MasterThreadStart : {0}", e.Message ), e ) );
			}
			finally {
				foreach ( MasterListener listener in m_Listeners ) {
					listener.Stop();
				}
				// kill all of the threads
				if ( m_XMDThreadCollection != null ) {
					foreach( XMDThread xmd in m_XMDThreadCollection ) {
						xmd.m_thisThread.Abort();
					}
					m_XMDThreadCollection.Clear();
				}
				// clear all of the sockets
				if ( m_SocketCollection != null ) {
					foreach( Socket socket in m_SocketCollection ) {
						socket.Close();
					}
					m_SocketCollection.Clear();
				}
			}
			m_ShutDown = true;
		}


		/// <summary>
		/// dispatch_io -- process io events requests
		/// </summary>
		protected void ProcessIO() {
			m_SocketCollection = new SocketCollection();

			while ( m_Active ) {
				try {
					// check for new connections and new messages
					SocketCollection sockets = m_SocketCollection.Clone();
					foreach ( MasterListener listener in m_Listeners ) {
						sockets.Add( listener.GetSocket );
					}
					Socket.Select( sockets, null, null, 1000000 );
					// yes new messages or connections
					foreach( Socket socket in sockets ) {
						// check for new connections
						bool found = false;
						foreach ( MasterListener listener in m_Listeners ) {
							if ( socket == listener.GetSocket ) {
								// yes new connection from listener
								ListenerConnect( listener );
								found = true;
								break;
							}
						}
						if ( !found ) {
							// yes new message from existing connection
							// handle the request from an existing connection
							// must be a child process
							Process_Child_Request( socket );
						}
					}
					// Do any periodic management stuff
					DoManagement();
					// Clean up any dead threads
					Thread_Cleaner();
				} catch ( Exception e ) {
					OmException.LogException( new OmException( "Error: ProcessIO caught an exception - " + e.Message, e ) );
				}
			}
		}

		/// <summary>
		/// new connection from listener
		/// </summary>
		/// <param name="listener"></param>
		public void ListenerConnect( MasterListener listener ) {
			// a place to hold the new header data
			ProtocolHeader header;
			bool keepSocket = false;

			try {
				// got a ipc connection request
				Socket NewSocket = listener.AcceptSocket();
				// set the socket option for this socket
				Protocol.InitProtocolSocket( NewSocket );

				// add it to the collection
				m_SocketCollection.Add( NewSocket );

				// query connection type 
				//<<AC>>:ret value is from the enum contypes, so above or equal to 0
				eConnectionTypes contype =  Protocol.GetConnectionType( NewSocket, out header );

				switch ( contype ) {
					case eConnectionTypes.CON_UTIL:
						// Connection from GUI
						// execute the ascii command string in header
						ProcessStringCommand ( NewSocket, header );
						break;
					case eConnectionTypes.CON_PMD:
						// Connection from a PMD
						// Check if this RMD already running?
						XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Secondary );

						if ( xmd == null ) {
							// create the RMD thread
							RMDThread RmdThread = new RMDThread();
							RmdThread.m_lgnum = header.msg.lgnum;
							// pass the socket to talk on.
							RmdThread.m_PMDsocket = NewSocket;

							RmdThread.m_thisThread = new Thread( new ThreadStart( RmdThread.Start ) );
							// set the thread name
							RmdThread.m_thisThread.Name = RMDThread.Name( RmdThread.m_lgnum );
							// add it to the collection
							m_XMDThreadCollection.Add( RmdThread );
							// start the new target RMD
							RmdThread.m_thisThread.Start();
						}
						break;
					case eConnectionTypes.CON_CLI:
						// got an IPC connect request from CLI
						keepSocket = ReceivedCLIMessage ( NewSocket, header );
						break;
					case eConnectionTypes.CON_CHILD:
						// In this situation a child process is a process running on the SAME host.
						// because recv_child_connect() relies on the header.cli and the pid 
						// also in this function, lhostname and rhostname are considered identical.

						// got a ipc connect request from child process
						Recv_Child_Connect( NewSocket, header );
						keepSocket = true;
						break;
				}
				// clean up the temporary socket
				if (( !keepSocket ) ||
					( !NewSocket.Connected )) {
					m_SocketCollection.Remove( NewSocket );
				}
			}
			catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in ProcessIO : {0}", e.Message ), e ) );
			}
		}

		/// <summary>
		/// proc_do_command --  get the rest of the command from the client 
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected bool ProcessStringCommand( Socket socket, ProtocolHeader header_ ) {
			header_byte header = new header_byte();
			BitConverter.GetBytes( (int)header_.magicvalue ).CopyTo( header.byteArray, 0 );

			// receive the rest of the string message already have the first 4 bytes
			Protocol.ReceiveStringMessage ( socket, ref header, 4 );

			if ( string.Compare( header.ToString(), 0, " get error messages", 0, 22) == 0) {

				// I don't believe this is used Mike
				throw new Exception( "Got a 'get error messages' message???" );
				//				i = 23;
				//				while (isspace(msg[i])) {
				//					i++;
				//				}
				//				sscanf(msg, "%d", &erroffset);
				//		
				//				get_log_msgs(ERRFAC, &errbuf, &errbufsize, &numerrmsgs, &erroffset);
				//			
				//				if (errbufsize > 0) {  
				//					sprintf(sizestring, "%d %d\n", numerrmsgs, erroffset);
				//					ftd_sock_send ( FALSE,fsockp, sizestring, strlen(sizestring));
				//					ftd_sock_send ( FALSE,fsockp, "{", 1);
				//					ftd_sock_send ( FALSE,fsockp, errbuf, strlen(errbuf));
				//					ftd_sock_send ( FALSE,fsockp, "}\n", 2);
				//					free(errbuf);
				//				} else {
				//					sprintf(sizestring, "0 %d\n{}\n", erroffset);
				//					ftd_sock_send ( FALSE,fsockp, sizestring, strlen(sizestring));
				//				}
				//				return 1;
			} else {
				Process_Dev_Info_Request( socket, header.ToString() );
			}
			// command serviced, return
			return true;
		}


		/// <summary>
		/// process_dev_info_request -- start processing a device list request
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected bool Process_Dev_Info_Request( Socket socket, string header ) {
			if ( string.Compare( header, 0, "ftd get all devices", 0, 19, true ) != 0 ) {
				// not the wright command
				return false;
			}
			int Lgnum;

			// get the group id for this command
			Lgnum = Convert.ToInt32( header.Substring( header.LastIndexOf( ' ' ) ) );
			
			StringCollection DeviceInfos = GetAllDevices( Lgnum );

			// build the send buffer
			string msg = "";
			foreach ( string device in DeviceInfos ) {
				msg += String.Concat ( device );
			}
			byte [] message = Encoding.UTF8.GetBytes( msg );

			// send the reply message containing all of the drive path names
			socket.Send ( message );
			return true;
		}

		public StringCollection GetAllDevices( int Lgnum ) {
			Management.Protocol.DeviceInfoCollection DeviceInfos;
			DeviceInfos = Management.Management.ftd_mngt_acquire_alldevices();

			StringCollection sDeviceInfos = new StringCollection();
			foreach ( Management.Protocol.mmp_TdmfDeviceInfo DeviceInfo in DeviceInfos ) {

				DriveInfo driveInfo = new DriveInfo();
				string Total = "unknow";
				try {
					driveInfo.DiskFreeSpaceEx( DeviceInfo.szDrivePath );
					Total = (driveInfo.GetTotalBytes / 1024 / 1024).ToString();
				} catch ( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Process_Dev_Info_Request : {0}", e.Message ), e ) );
				}
				string DiskID = String.Format( "{0} {1} {2}", DeviceInfo.szDriveId, DeviceInfo.szStartOffset, DeviceInfo.szLength );

				string disk;
				if ( DeviceInfo.szDrivePath.Length > 3 ) {
					// mount point
					disk = DeviceInfo.szDrivePath;
				} else {
					disk = DeviceInfo.szDrivePath.Substring( 0, 1 );
				}
				sDeviceInfos.Add( "[-" + disk + "-]  " + Total  + "MB {" + DiskID + "\\" );
			}
			return sDeviceInfos;
		}

		//		public string [] GetAllDevices( int Lgnum ) {
		//			// buffer for drive list
		//			string [] DeviceInfos = new string[1024];
		//
		//			// get all of the valid drive letters.
		//			string [] drives = Directory.GetLogicalDrives();
		//
		//			int count = 0;
		//			foreach ( string drive in drives ) {
		//				string DiskID;
		//
		//				DriveInfo.eDriveType DriveType = DriveInfo.GetDriveType ( drive );
		//
		//				switch ( DriveType ) {
		//					case DriveInfo.eDriveType.DRIVE_REMOVABLE:
		//					case DriveInfo.eDriveType.DRIVE_CDROM:
		//					case DriveInfo.eDriveType.DRIVE_RAMDISK:
		//					case DriveInfo.eDriveType.DRIVE_REMOTE:
		//						continue;
		//				}
		//				// let use these drive types
		//				// get the drive signature and volume info
		//				DiskID = Config.GetDiskSignatureAndInfo( drive.Substring(0,2) );
		//
		//				DriveInfo driveInfo = new DriveInfo();
		//				string Total = "unknow";
		//				try {
		//					driveInfo.DiskFreeSpaceEx( drive );
		//					Total = (driveInfo.GetTotalBytes / 1024 / 1024).ToString();
		//				} catch ( Exception e ) {
		//					OmException.LogException( new OmException( String.Format("Error in Process_Dev_Info_Request : {0}", e.Message ), e ) );
		//				}
		//
		//				DeviceInfos[count++] = "[-" + drive.Substring( 0, 0 ) + "-]  " + Total  + "MB {" + DiskID + "\\";
		//
		//				// now find any mount points
		//				StringBuilder MountPt = new StringBuilder( Globals._MAX_PATH );
		//
		//				VolumeMountPoint volumeMountPoint = new VolumeMountPoint();
		//				try {
		//					if ( volumeMountPoint.FindFirstVolumeMountPoint( drive.Substring( 0, 1 ), MountPt ) ) {
		//						do {
		//							string MountPtFullName;
		//							MountPtFullName = drive.Substring( 0, 1 ) + ":\\" + MountPt;
		//
		//							DiskID = Config.GetMntPtSigAndInfo( MountPtFullName );
		//							
		//							driveInfo = new DriveInfo();
		//							Total = "unknow";
		//							try {
		//								driveInfo.DiskFreeSpaceEx( MountPtFullName );
		//								Total = (driveInfo.GetTotalBytes / 1024 / 1024).ToString();
		//							} catch ( Exception e ) {
		//								OmException.LogException( new OmException( String.Format("Error in Process_Dev_Info_Request DiskFreeSpaceEx : {0}", e.Message ), e ) );
		//							}
		//
		//							DeviceInfos[count++] = "[-" + MountPtFullName + "-]  " + Total  + "MB {" + DiskID + "\\";
		//
		//						} while ( volumeMountPoint.FindNextVolumeMountPoint( MountPt ) );
		//					}
		//				} finally {
		//					volumeMountPoint.CloseVolumeMountPoint();
		//				}
		//			}
		//			return DeviceInfos;
		//		}

		/// <summary>
		/// read a ipc message header and dispatch accordingly
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		protected bool ReceivedCLIMessage( Socket socket, ProtocolHeader header ) {
			int rc;
			bool KeepSocket = false;

			switch ( header.msgtype ) {
				case eProtocol.CCPSTARTP:
				case eProtocol.CCPSTARTS:
				case eProtocol.CCPSTOPP:
				case eProtocol.CCPSTOPS:
					// from checkpoint 
					rc = Recv_CheckPoint( socket, header, ref KeepSocket );
					break;
				case eProtocol.CCPON:
				case eProtocol.CCPOFF:
				case eProtocol.ACKCPERR:
				case eProtocol.ACKCLD:
					// from pmd, rmd
					rc = Recv_Child_Ack( header );
					break;
					//				case eProtocol.CSTARTPMD:
					//					// from launch
					//					rc = RecvStartPMD( socket, header, ref KeepSocket );
					//					break;
				case eProtocol.CSTARTAPPLY:
					// from rmd
					rc = Recv_Start_Apply( socket, header, true );
					break;
				case eProtocol.CSTOPAPPLY:
					// from rmd
					rc = Recv_Stop_Apply( socket, header );
					break;
				case eProtocol.CAPPLYDONECPON:
					// from rmda
					rc = Recv_Apply_Done_CpOn( socket, header );
					break;
					//				case eProtocol.CSTARTRECO:
					//					// from dtcreco 
					//					rc = Recv_Start_Reco( socket, header );
					//					break;
					//				case eProtocol.CSIGTERM:
					//				case eProtocol.CSIGUSR1:
					//					// from kill
					//					rc = Recv_Signal( socket, header );
					//					break;
#if USE_COLLECTOR
				case eProtocol.CMANAGEMENT:
					rc = ManagementOld.ReceiveMessage( socket, ref KeepSocket );
					break;
#endif
					//				case eProtocol.CSETTRACELEVEL:
					//					// from tdmftrace
					//					rc = Recv_Trace_Level( socket, header );
					//					break;

				case eProtocol.CREMWAKEUP:
					// remote wakeup call !! 
					rc = RecvRemoteWakeUp( socket, header );
					break;

				case eProtocol.CREMDRIVEERR:
					// FTDCREMDRIVEERR sent from PMD to Primary Master!! WR17511
					rc = RecvRemoteDriveError( socket, header );
					break;

				case eProtocol.CREFRSRELAUNCH:
					// from rmda !! WR17511
					rc = RecvRefreshsRelaunch( socket, header );
					break;

				default:
					break;
			}
			return KeepSocket;
		}

		/// <summary>
		/// process an incoming SMART REFRESH RELAUNCH message from RMDA, Automatic Smart Refresh relaunch.
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int RecvRefreshsRelaunch( Socket socket, ProtocolHeader header ) {
			foreach ( XMDThread xmd in this.m_XMDThreadCollection ) {
				// find RMD the right lg
				if (( xmd.m_lgnum == header.msg.lgnum ) &&
					( xmd.GetType() == typeof( RMDThread ) )) {

					// send to RMD the FTDCREMDRIVEERR signal
					try {
						Protocol.SendCREMDRIVEERR ( xmd.m_IPCSocket, header, (int)~eRole.Secondary, (int)eRole.Secondary );
					} catch ( Exception e ) {
						OmException.LogException( new OmException( String.Format("Error in Recv_Trace_Level : {0}", e.Message ), e ) );
					}
					break;
				}
			}
			// ack the rmda process
			try {
				Protocol.SendAckCLI ( socket, header, 0 );
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in Recv_Refreshs_Relaunch : {0}", e.Message ), e ) );
				return -1;
			}
			return 0;
		}

		/// <summary>
		/// process an incoming start message from remote, Autoamtic Smart Refresh relaunch.
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int RecvRemoteDriveError( Socket socket, ProtocolHeader header ) {
			GroupInfo groupInfo = new GroupInfo();
			groupInfo.name = null;

			// get the group info to see if this group exists
			try {
				DriverIOCTL driver = new DriverIOCTL();
				try {
					driver.GetGroupInfo( header.msg.lgnum, ref groupInfo );
					// see if the group was ACTIVE previously
					if ( groupInfo.ClusterActive ) {
						switch ( groupInfo.state ) {
							case eLgModes.NORMAL:
							case eLgModes.REFRESH:
								
								// notify the driver that the secondary machine is up now
								driver.SecondaryAlive( header.msg.lgnum );

								//								// execute a launchrefresh process
								//								// we use the shell because the launch command has the -p parameter handling.
								//								Process process = new Process();
								//								process.StartInfo.FileName = "CLI.exe";
								//								process.StartInfo.Arguments = String.Format( "launchrefresh -g{0} -p", header.msg.lgnum.ToString("D3"));
								//								process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
								//								process.StartInfo.UseShellExecute = false;
								//								process.Start();
								break;
							case eLgModes.CHECKSUMREFRESH:
							case eLgModes.FULLREFRESH:
								break;
							default:
								break;             
						}
					}
				}
				finally {
					driver.Close();
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in Recv_Remote_Drive_Error : {0}", e.Message ), e ) );
				return -1;
			}
			return 0;
		}

		/// <summary>
		/// process an incoming start message from remote.
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int RecvRemoteWakeUp( Socket socket, ProtocolHeader header ) {
			GroupInfo groupInfo = new GroupInfo();
			groupInfo.name = null;

			// Verify the previous LG state, if it was LG_ACTIVE, start the PMD here
			try {
				DriverIOCTL driver = new DriverIOCTL();
				try {
					driver.GetGroupInfo( header.msg.lgnum, ref groupInfo );
					// see if the group was ACTIVE previously
					if ( groupInfo.ClusterActive ) {

						// notify the driver that the secondary machine is up now
						driver.SecondaryAlive( header.msg.lgnum );

						//						// verify that a PMD is not running
						//						XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Primary );
						//
						//						if ( xmd == null ) {
						//							switch ( groupInfo.state ) {
						//								case eLgModes.CHECKSUMREFRESH:
						//									CreatePMDThread ( header.msg.lgnum, eLgModes.CHECKSUMREFRESH );
						//									break;
						//								case eLgModes.FULLREFRESH:
						//									// create the PMD thread
						//									CreatePMDThread ( header.msg.lgnum, eLgModes.FULLREFRESH );
						//									break;
						//								case eLgModes.NORMAL:
						//								case eLgModes.REFRESH:
						//								default:
						//									CreatePMDThread ( header.msg.lgnum, eLgModes.NORMAL );
						//									break;
						//							}
						//						}
					}
				} 
				finally {
					driver.Close();
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in Recv_Remote_WakeUp : {0}", e.Message ), e ) );
				return -1;
			}
			return 0;
		}

		//		/// <summary>
		//		/// Creates and starts a new PMD thread
		//		/// </summary>
		//		/// <param name="lgnum"></param>
		//		/// <param name="state"></param>
		//		protected PMDThread CreatePMDThread ( int lgnum, eLgModes state ) {
		//			// create the PMD thread
		//			PMDThread PmdThread = new PMDThread();
		//			PmdThread.m_lgnum = lgnum;
		//			PmdThread.m_State = state;
		//			PmdThread.m_thisThread = new Thread( new ThreadStart( PmdThread.Start ) );
		//			// set the thread name
		//			PmdThread.m_thisThread.Name = PMDThread.Name( PmdThread.m_lgnum );
		//			// add it to the collection
		//			m_XMDThreadCollection.Add( PmdThread );
		//			// start the new source PMD
		//			PmdThread.m_thisThread.Start();
		//			return PmdThread;
		//		}
		//
		//		/// <summary>
		//		/// process an incoming FTDCSETTRACELEVEL message.
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="header"></param>
		//		/// <returns></returns>
		//		protected int Recv_Trace_Level( Socket socket, ProtocolHeader header )
		//		{
		//		    //the first byte of the all-purpose data buffer in the header contains the trace level
		//
		//			// TODO: impliment trace levels
		//		    //error_SetTraceLevel ( (unsigned char)header->msg.data[0] );
		//		    //error_tracef(TRACEWRN , "recv_trace_level(): new level=%d", (int)header->msg.data[0] );
		//		
		//		    // send back ACK
		//			try {
		//				Protocol.SendCSETTRACELEVELACK ( socket, header );
		//			} catch ( Exception e ) {
		//				OmException.LogException( new OmException( String.Format("Error in Recv_Trace_Level : {0}", e.Message ), e ) );
		//				return -1;
		//			}
		//		    return -1;//return < 0 because this fnct does not use the socket.
		//		}

		//		/// <summary>
		//		/// Handle the kill command
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="header"></param>
		//		/// <returns></returns>
		//		protected int Recv_Signal( Socket socket, ProtocolHeader header )
		//		{
		//			int data = 0;
		//			// First check if this RMD already running?
		//			XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, (eRole)header.msg.data );
		//
		//			if ( xmd != null ) {
		//				// kill it
		//				if ( xmd.m_thisThread.IsAlive ) {
		//					xmd.m_thisThread.Abort();
		//				}
		//				data = 1;    // run state
		//			} else {
		//				// target thread not running
		//				data = 0;    // run state
		//			}
		//			try {
		//				Protocol.SendAckCLI ( socket, header, data );
		//			} catch ( Exception e ) {
		//				OmException.LogException( new OmException( String.Format("Error in Recv_Signal : {0}", e.Message ), e ) );
		//				return -1;
		//			}
		//			return 0;
		//		}

		/// <summary>
		/// handle a start reco request
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int Recv_Start_Reco( Socket socket, ProtocolHeader header ) {
			int data = 0;

			try {
				// First check if this RMD already running?
				XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Secondary );

				// if it's not running then skip it
				// if group rmd running then can't do
				if ( xmd != null ) {
					// data field contains force flag
					int force = header.msg.data;
					if ( force != 0 ) {
						// kill it
						if ( xmd.m_thisThread.IsAlive ) {
							xmd.m_thisThread.Abort();
						}
					} else {
						data = 1;	// return running
						return -1;
					}
				}

				// start the apply process for the group
				// don't reply because we will send the ackcli
				if ( Recv_Start_Apply( socket, header, false ) < 0 ) {
					data = -1;	// invalidate the running flag
					return -1;
				}
			} finally {
				// it's dtcreco talking to us
				// send ackCLI
				// pass the original header to copy back data.
				try {
					Protocol.SendAckCLI ( socket, header, data );
				} catch ( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Recv_Start_Reco : {0}", e.Message ), e ) );
				}
			}
			return 0;
		}

		/// <summary>
		/// handle a cpon msg packet from apply process 
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int Recv_Apply_Done_CpOn( Socket socket, ProtocolHeader header ) {
			int errorRet = 0;
			// First check if this RMD already running?
			XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Secondary );

			if ( xmd != null ) {
				// send the rmd the msg
				try {
					// send header to command
					Protocol.SendCCPON( xmd.m_IPCSocket, header, 0, 0, 0 );
				}
				catch( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Recv_Apply_Done_CpOn : {0}", e.Message ), e ) );
					errorRet = -1;
				}
			} else {
				// if process not running - do cpon here 
				LogicalGroup lgp = new LogicalGroup();
				try {
					lgp = LogicalGroup.Init( header.msg.lgnum, eRole.Secondary, false );
					try {
						CheckPoint.LogicalGroupCpOn( lgp, header );
					} finally {
						lgp.Close();
					}
				} catch ( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Recv_Remote_Drive_Error Init : {0}", e.Message ), e ) );
					errorRet = -1;
				}

			}
			// ack the rmda process
			try {
				if ( errorRet == 0 ) {
					Protocol.SendAckCLI ( socket, header, 0 );
				} else {
					Protocol.SendACKERR ( socket, header );
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in Recv_Apply_Done_CpOn2 : {0}", e.Message ), e ) );
				return -1;
			}
			return errorRet;
		}

		/// <summary>
		/// handle a stop apply request from a child rmd
		/// </summary>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int Recv_Stop_Apply( Socket socket, ProtocolHeader header ) {
			// First check if this RMDA already running?
			XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Apply );

			// if it's not running then skip it
			if ( xmd != null ) {
				// terminate the thread
				if ( xmd.m_thisThread.IsAlive ) {
					xmd.m_thisThread.Abort();
				}
				// remove thread from list
				this.m_XMDThreadCollection.Remove ( xmd );
			}

			// Mike changed to reply to socket rather than m_IPCSocket should work if not then change back!
			// it is the rmd talking to us -
			// just send it a NOOP in case it's waiting
			//			xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Secondary );
			//		    if ( xmd != null ) {
			try {
				// send header to command
				//					Protocol.SendNOOP( xmd.m_IPCSocket, header, 0 );
				Protocol.SendNOOP( socket, header, 0 );
			}
			catch( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in Recv_Stop_Apply : {0}", e.Message ), e ) );
				return -1;
			}
			//		    }
			return 0;
		}

		/// <summary>
		/// handle a start apply request from a child rmd
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int Recv_Start_Apply ( Socket socket, ProtocolHeader header, bool sendReply ) {
			int running = 0;

			// First check if this RMDA already running?
			XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Apply );

			// if it's already running then skip it
			if ( xmd != null ) {
				running = 1;
			} else {
				// create the RMDA thread
				RMDAThread RmdAThread = new RMDAThread();
				RmdAThread.m_lgnum = header.msg.lgnum;
				RmdAThread.m_CpOn = header.msg.data;
				// pass the socket to talk on.
				// Apply thread does not have a socket to talk to the PMD
				// RmdAThread.PMDsocket = NewSocket;

				RmdAThread.m_thisThread = new Thread( new ThreadStart( RmdAThread.Start ) );
				// set the thread name
				RmdAThread.m_thisThread.Name = RMDAThread.Name( RmdAThread.m_lgnum );
				// add it to the collection
				m_XMDThreadCollection.Add( RmdAThread );
				// start the new target RMDA
				RmdAThread.m_thisThread.Start();
			}

			if ( sendReply ) {
				// it might be the rmd talking to us -
				// just send it a NOOP in case it's waiting
				// TODO: this does not make sence? why do we need to send a noop? to the primary? or back to the caller?
				// Mike removed the RMD check just send noop to sending socket
				//				xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Secondary );
				//				if ( xmd != null ) {
				//					try {
				//						// send header to command
				//						Protocol.SendNOOP( xmd.m_IPCSocket, header, running );
				//					}
				//					catch( Exception e ) {
				//						OmException.LogException( new OmException( String.Format("Error in Recv_Start_Apply : {0}", e.Message ) ) );
				//						return -1;
				//					}
				//				} else {
				// it's cli talking to us
				try {
					// send header to command
					Protocol.SendNOOP( socket, header, running );
				}
				catch( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Recv_Start_Apply : {0}", e.Message ), e ) );
					return -1;
				}
				//				}
			}
			return running;
		}


		//		/// <summary>
		//		/// handle a start pmd request from the launch command
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="header"></param>
		//		/// <returns></returns>
		//		protected int RecvStartPMD ( Socket socket, ProtocolHeader header, ref bool KeepSocket )
		//		{
		//			XMDThread xmd = Lgnum_To_Thread ( header.msg.lgnum, eRole.Primary );
		//
		//			if ( xmd != null ) {
		//
		//				if (( xmd.m_IPCSocket == null ) ||
		//					( !xmd.m_IPCSocket.Connected )) { 
		//					// child running but connection not valid -
		//					// ack the command and let it retry if it wants to
		////					header.msgtype = eProtocol.ACKERR;
		//					try {
		//						// send header to command
		//						Protocol.SendACKERR( socket, header );
		//					}
		//					catch( Exception e ) {
		//						OmException.LogException( new OmException( String.Format("Error in Recv_Start_PMD : {0}", e.Message ), e ) );
		//						return -1;
		//					}
		//					return 0;
		//				}
		//				// pass along to the PMD
		//				try {
		//					// send header to command
		//					Protocol.SendHeader( xmd.m_IPCSocket, header );
		//				}
		//				catch( Exception e ) {
		//					OmException.LogException( new OmException( String.Format("Error in Recv_Start_PMD1 : {0}", e.Message ), e ) );
		//					return -1;
		//				}
		//			} else {
		//				// create the PMD thread
		//				xmd = CreatePMDThread ( header.msg.lgnum, (eLgModes)header.msg.data );
		//			}
		//
		//			// save command sock - so command stays around until it's acked 
		//			xmd.m_CommandSocketCollection.Add ( socket );
		//			KeepSocket = true;
		//			return 0;
		//		}




		/// <summary>
		/// handle a checkpoint request command
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int Recv_CheckPoint ( Socket socket, ProtocolHeader header, ref bool KeepSocket ) {
			eRole role = eRole.All;
			XMDThread xmd;
			DriverIOCTL driver;

			// determine the role this command is intended for
			switch ( header.msgtype ) {
				case eProtocol.CCPSTARTP:
					// Send CP Start to driver.
					driver = new DriverIOCTL();
					try {
						driver.ps_set_group_checkpoint( header.msg.lgnum, true );
					} finally {
						driver.Close();
					}
					break;
				case eProtocol.CCPSTOPP:
					// Send CP Stop to driver.
					driver = new DriverIOCTL();
					try {
						driver.ps_set_group_checkpoint( header.msg.lgnum, false );
					} finally {
						driver.Close();
					}		
					break;
				case eProtocol.CCPSTARTS:
				case eProtocol.CCPSTOPS:
					role = eRole.Secondary;
					// get the thread for this logical group and role
					if ( ( xmd = Lgnum_To_Thread( header.msg.lgnum, role ) ) != null ) {
						// group process running OK
						if (( xmd.m_IPCSocket == null ) ||
							( !xmd.m_IPCSocket.Connected )) { 
							// child running but connection not valid -
							// ack the command and let it retry if it wants to
							//header.msgtype = eProtocol.ACKERR;
							try {
								// send header to command
								Protocol.SendACKERR( socket, header );
							}
							catch( Exception e ) {
								OmException.LogException( new OmException( String.Format("Error in Recv_CP1 : {0}", e.Message ), e ) );
								return -1;
							}
							return 0;
						}
						// pass along to the PMD or RMD
						try {
							// send header to command
							Protocol.SendHeader( xmd.m_IPCSocket, header );
						}
						catch( Exception e ) {
							OmException.LogException( new OmException( String.Format("Error in Recv_CP2 : {0}", e.Message ), e ) );
							return -1;
						}
						// save command sock - so command stays around until it's acked 
						xmd.m_CommandSocketCollection.Add ( socket );
						KeepSocket = true;
					}
					else {
						// send error ack to checkpoint command 
						try {
							// send header to command
							// group process not running
							// checkpoint secondary type ack
							switch ( header.msgtype ) {
								case eProtocol.CCPSTARTS:
									//header.msgtype = eProtocol.ACKDOCPON;
									Protocol.SendACKDOCPON( socket, header );
									break;
								default:
									//header.msgtype = eProtocol.ACKDOCPOFF;
									Protocol.SendACKDOCPOFF( socket, header );
									break;
							}

						}
						catch( Exception e ) {
							OmException.LogException( new OmException( String.Format("Error in Recv_CP3 : {0}", e.Message ), e ) );
							return -1;
						}
					}
					break;
			}
			return 0;
		}

		/// <summary>
		/// handle a connection from a child process
		/// </summary>
		/// <param name="listener"></param>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		protected void Recv_Child_Connect( Socket socket, ProtocolHeader header ) {
			// find the thread that sent this message
			foreach ( XMDThread xmd in this.m_XMDThreadCollection ) {
				if ( xmd.m_thisThread.GetHashCode() == header.cli ) {
					// found child thread
					// save child thread sock - so we can use it in the future 
					xmd.m_IPCSocket = socket;
					// check if PMD thread
					//					if ( xmd.GetType() == typeof( PMDThread ) ) {
					//						// we don't have a command channel on restart of PMD
					//						if ( xmd.m_CommandSocketCollection.Count > 0 ) {
					//							Socket sock = xmd.m_CommandSocketCollection[0];
					//							// send ack to launch command
					//							// pass the original header to copy back data.
					//							try {
					//								Protocol.SendAckCLI ( sock, header, 0 );
					//							} catch ( Exception e ) {
					//								OmException.LogException( new OmException( String.Format("Error in Recv_Child_Connect : {0}", e.Message ), e ) );
					//							}
					//							// remove sock object from connection list
					//							xmd.m_CommandSocketCollection.Remove( sock );
					//							this.m_SocketCollection.Remove( sock );
					//							// destroy the command socket object
					//							sock.Close();
					//						}
					//					}
					break;
				}
			}   
		}

		/// <summary>
		/// This function creates a list of 'remote' secondary groups and sends a
		/// FTDCREMWAKEUP msg to the related Primary Master Threads.
		/// </summary>
		protected void RemoteWakeUpThread() {
			int	PortNumber;
			ProtocolHeader header = new ProtocolHeader();

			// create the list to hold the config
			ConfigInfoCollection ConfigInfos = null;

			try {
				// get the master thread port number
				PortNumber = m_MasterPort;

				// Create a list of all existing SECONDARY group(s), by refering to sxxx.cfg
				// and tag this group if not in LOOPBACK mode
				// get the registry install Path
				ConfigInfos = Config.GetSecondaryConfigInfos();

				// build the header to send
				header.cli = 1;

				// For each validated SECONDARY group, send a FTDCREMWAKEUP msg to the related Primary Master Thread,
				foreach ( ConfigInfo configInfo in ConfigInfos ) {

					LgConfig config = new LgConfig();
					config = Config.ReadConfig( configInfo.lgnum, configInfo.role );
					// don't do loopback configs
					if ( String.Compare( config.phostname, config.shostname, true ) != 0 ) {
						// not a loopback config
						// init the header
						header.msg.lgnum = config.lgnum;
						header.msgtype = eProtocol.CREMWAKEUP;
						header.ackwanted = 0;

						TcpClient client = null;
						try {
							// Send Remote Start message to Primary Master for targeted group
							client = new TcpClient( config.phostname, PortNumber );

							Protocol.SendHeader( client, header );
						}
						catch ( Exception e ) {
							// don't do anything just log the errors
							OmException.LogException( new OmException( String.Format("Error in RemoteWakeUpThread : {0}", e.Message ), e ) );
						}
						client.Close();
					}
				}
			} finally {
				// delete the list
				ConfigInfos.Clear();
			}
		}


		/// <summary>
		/// Clean up any dead threads
		/// restart the thread if needed.
		/// ftd_proc_reaper
		/// </summary>
		protected void Thread_Cleaner () {
			// check all of the PMDThreads
			for ( int count = 0 ; count < this.m_XMDThreadCollection.Count ; ) {
				XMDThread thread = this.m_XMDThreadCollection[ count ];

				if ( thread.m_thisThread.IsAlive ) {
					// try next thread
					count++;
				} else {
					// remove it from the list
					this.m_XMDThreadCollection.Remove( thread );
					// remove the actual thread pointer
					thread.m_thisThread = null;

					//					if ( thread.GetType() == typeof( PMDThread ) ) {
					//						// relaunch PMD if it exited with reasonable status 
					//						if ( thread.m_ExitCode == eExitCodes.EXIT_RESTART ) {
					//
					//							// start a new pmd for the group here
					//							// create the PMD thread
					//							CreatePMDThread ( thread.m_lgnum, eLgModes.NORMAL );
					//
					////							PMDThread PmdThread = new PMDThread();
					////							PmdThread.m_lgnum = thread.m_lgnum;
					////
					////							PmdThread.m_thisThread = new Thread( new ThreadStart( PmdThread.Start ) );
					////							// set the thread name
					////							PmdThread.m_thisThread.Name = PMDThread.Name( PmdThread.m_lgnum );
					////							// add it to the collection
					////							m_XMDThreadCollection.Add( PmdThread );
					////							// restart the PMD
					////							PmdThread.m_thisThread.Start();
					//						}
					//					}
					//					else
					if ( thread.GetType() == typeof( RMDAThread ) ) {
						// tell rmd - just send it a NOOP in case it's waiting
						// don't believe we need this any more Mike
						// TODO:
						//						if ((fprocp = ftd_proc_lgnum_to_proc(proclist, (*fprocpp)->lgnum, ROLESECONDARY))) {
						//							memset(&header, 0, sizeof(header));
						//							header.msgtype = FTDCNOOP;
						//							if (FTD_SOCK_CONNECT(fprocp->fsockp)) {
						//								FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_proc_reaper",fprocp->fsockp, &header);
						//							}
						//						}
					}
						//					else if ( thread.GetType() == typeof( ThrottleThread ) ) {
						//						// relaunch Throttle Thread
						//						// create the Throttle thread
						//						ThrottleThread throttleThread = new ThrottleThread();
						//						throttleThread.m_lgnum = thread.m_lgnum;
						//
						//						throttleThread.m_thisThread = new Thread( new ThreadStart( throttleThread.Start ) );
						//						// set the thread name
						//						throttleThread.m_thisThread.Name = "Throttle_" + throttleThread.m_lgnum.ToString("D3");
						//						// add it to the collection
						//						m_XMDThreadCollection.Add( throttleThread );
						//						// restart the PMD
						//						throttleThread.m_thisThread.Start();
						//					}
					else if ( thread.GetType() == typeof( StatisticsThread ) ) {
						// relaunch statistics Thread
						// create the statistics thread
						StatisticsThread statThread = new StatisticsThread();
						statThread.m_lgnum = thread.m_lgnum;

						statThread.m_thisThread = new Thread( new ThreadStart( statThread.Start ) );
						// set the thread name
						statThread.m_thisThread.Name = "Statistics_" + statThread.m_lgnum.ToString("D3");
						// add it to the collection
						m_XMDThreadCollection.Add( statThread );
						// restart the PMD
						statThread.m_thisThread.Start();
					}
				}
			}
		}

		/// <summary>
		/// Do any periodic management routines
		/// </summary>
		protected void DoManagement () {
			// check all of the PMDThreads
			foreach ( LgConfig config in m_LgConfigs ) {
				if ( config.role == eRole.Primary ) {
					// house cleaning
					LogicalGroup lgp;
					try {
						lgp = LogicalGroup.Init( config.lgnum, eRole.Primary, false );
						// Tdmf management related operations
						try {
							Management.Management.DoLgMonitor( lgp );
						} finally {
							lgp.Close();
						}
					} catch ( Exception e ) {
						OmException.LogException( new OmException( String.Format("Error in PMDThread Start LogicalGroup.Init : {0}", e.Message ), e ) );
						continue;
					}
				}
			}
		}

		/// <summary>
		/// Process_Child_Request -- process child ipc requests
		/// </summary>
		/// <param name="socket"></param>
		protected void Process_Child_Request( Socket socket ) {
			ProtocolHeader header = new ProtocolHeader();

			// find thread for this socket
			foreach ( XMDThread thread in this.m_XMDThreadCollection ) {
				if ( socket == thread.m_IPCSocket ) {
					/// TODO: this should be a thread.
					// get ipc msg from child
					try {
						Protocol.ReceiveHeader( thread.m_IPCSocket, ref header );
					} catch ( Exception e ) {
						// socket error - or connection closed
						this.m_SocketCollection.Remove( thread.m_IPCSocket );
						thread.m_IPCSocket.Close();
						thread.m_IPCSocket = null;
						OmException.LogException( new OmException( String.Format("Error in Process_Child_Request : {0}", e.Message ), e ) );
						return;
					}
					// handle the CLI message
					ReceivedCLIMessage( socket, header );
					return;
				}
			}
			// if hear then thread exited so receive the data and toss it
			OmException.LogException( new OmException( String.Format("Warning: Connection from a dead thread" ) ) );
			try {
				Protocol.ReceiveHeader( socket, ref header );
			} catch ( Exception ) {
				m_SocketCollection.Remove( socket );
			}

		}

		/// <summary>
		/// handle an ack packet from child, ack the command socket, if any, that sent the command to pass to the child so it can go away
		/// </summary>
		/// <param name="header"></param>
		/// <returns></returns>
		protected int Recv_Child_Ack( ProtocolHeader header ) {
			XMDThread xmd;

			if ( (eRole)header.msg.data == eRole.Primary ) {
				throw new OmException( "Error: Recv_Child_Ack - no primary threads" );
			} else {
				// get the thread for this logical group and role
				if ( ( xmd = Lgnum_To_Thread( header.msg.lgnum, (eRole)header.msg.data ) ) == null ) {
					return -1;
				}
				// do we have a socket to reply too?
				if ( xmd.m_CommandSocketCollection.Count > 0 ) {
					Socket socket = xmd.m_CommandSocketCollection[0];
					try {
						switch ( header.msgtype ) {
							case eProtocol.CCPON:
							case eProtocol.CCPOFF:
							case eProtocol.ACKCPERR:
								//header.msgtype = eProtocol.ACKCLI;
								Protocol.SendAckCLI( socket, header, 0 );
								break;
							default:
								// passs on the same message
								// send header to command
								Protocol.SendHeader( socket, header );
								break;
						}
					}
					catch( Exception e ) {
						OmException.LogException( new OmException( String.Format("Error in Recv_Child_Ack : {0}", e.Message ), e ) );
						return -1;
					}
					finally {
						// remove sock object from proc connection list
						xmd.m_CommandSocketCollection.Remove( socket );
						this.m_SocketCollection.Remove( socket );
						// destroy the command socket object
						// the command line program will die when it notices the close
						socket.Close();
					}
				}
			}
			return 0;
		}


		/// <summary>
		/// return target thread object for the lgnum/role
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="role"></param>
		/// <returns></returns>
		protected XMDThread Lgnum_To_Thread ( int lgnum, eRole role ) {
			foreach ( XMDThread xmd in this.m_XMDThreadCollection ) {
				if ( xmd.m_lgnum == lgnum ) {
					switch ( role ) {
						case eRole.Primary:
							//							if ( xmd.GetType() == typeof( PMDThread ) ) {
							//								return xmd;
							//							}
							break;
						case eRole.Secondary:
							if ( xmd.GetType() == typeof( RMDThread ) ) {
								return xmd;
							}
							break;
						case eRole.Apply:
							if ( xmd.GetType() == typeof( RMDAThread ) ) {
								return xmd;
							}
							break;
						default:
							OmException.LogException( new OmException( String.Format("Error in Lgnum_To_Thread : switch Default entered?" ) ) );
							break;
					}
				}
			}
			return null;
		}


		/// <summary>
		/// Returns a MasterThread object for this machine.
		/// </summary>
		/// <returns></returns>
		public static MasterThread GetMasterThread() {
			return GetMasterThread( System.Environment.MachineName );
		}
		/// <summary>
		/// Returns a ConfigurationProcessor object for the s3Server machine.
		/// </summary>
		/// <param name="s3Server"></param>
		/// <returns></returns>
		public static MasterThread GetMasterThread( string server ) {
			return GetMasterThread( server, RegistryAccess.MasterPort() );
		}
		public static MasterThread GetMasterThread( string server, int port ) {
			MasterThread masterThread;
			port += 2;
			masterThread = (MasterThread)Activator.GetObject (
				typeof( MasterThread ),
				"tcp://" + server + ":" + port + "/MasterThread" );
			return masterThread;
		}

//		/// <summary>
//		/// Called by Apply CLI
//		/// </summary>
//		public void ApplyAllGroups() {
//			Config.ApplyAllGroups( ref m_LgConfigs );
//		}

//		/// <summary>
//		/// Apply all groups
//		/// </summary>
//		private void ApplyAllGroupsXML() {
//			ConsoleAPI api = new ConsoleAPI();
//			api.ApplyAllGroupsXML();
//		}

//		/// <summary>
//		/// Called by CLIs
//		/// </summary>
//		public void ExecuteCLI( CLIData data ) {
//			ConsoleAPI api = new ConsoleAPI();
//			api.ExecuteCLI( data );
//		}

		/// <summary>
		/// Set the trace level
		/// </summary>
		/// <param name="level"></param>
		public void SetTraceLevel ( int level ) {
			// TODO: impliment trace levels
			//error_SetTraceLevel ( (unsigned char)header->msg.data[0] );
		}

		public void SetTunables( int lgnum, string inkey, int intval ) {
			Config.SetToonableValue( lgnum, inkey, intval );
		}


		public LgConfigCollection LgConfigsPrimary {
			get { return m_LgConfigs; }
		}
		public LgConfigCollection LgConfigsSecondary {
			get { return Config.GetLgConfigs( eRole.Secondary ); }
		}

		public void CheckPointStartPrimary( int lgnum ) {
			// Send CP Start to driver.
			DriverIOCTL driver;
			driver = new DriverIOCTL();
			try {
				driver.ps_set_group_checkpoint( lgnum, true );
			} finally {
				driver.Close();
			}
		}
		public void CheckPointStopPrimary( int lgnum ) {
			// Send CP Stop to driver.
			DriverIOCTL driver;
			driver = new DriverIOCTL();
			try {
				driver.ps_set_group_checkpoint( lgnum, false );
			} finally {
				driver.Close();
			}	
		}
		public bool CheckPointStartSecondary( int lgnum ) {
			// get the thread for this logical group and role
			XMDThread xmd;
			if ( ( xmd = Lgnum_To_Thread( lgnum, eRole.Secondary ) ) != null ) {
				// group process running OK
				if (( xmd.m_IPCSocket == null ) ||
					( !xmd.m_IPCSocket.Connected )) { 
					// child running but connection not valid -
					// ack the command and let it retry if it wants to
					//header.msgtype = eProtocol.ACKERR;
					return false;
					//						// send header to command
					//						Protocol.SendACKERR( socket, header );
					//					}
					//					catch( Exception e ) {
					//						OmException.LogException( new OmException( String.Format("Error in Recv_CP1 : {0}", e.Message ), e ) );
					//						return -1;
					//					}
					//					return 0;
				}
				// pass along to the PMD or RMD
				try {
					// send header to command
					ProtocolHeader header = new ProtocolHeader();
					Protocol.SendHeader( xmd.m_IPCSocket, header );
				}
				catch( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Recv_CP2 : {0}", e.Message ), e ) );
					return false;
				}
			}
			else {
				// send error ack to checkpoint command
				return false;
				//				try {
				//					// send header to command
				//					// group process not running
				//					// checkpoint secondary type ack
				//					switch ( header.msgtype ) {
				//						case eProtocol.CCPSTARTS:
				//							//header.msgtype = eProtocol.ACKDOCPON;
				//							Protocol.SendACKDOCPON( socket, header );
				//							break;
				//						default:
				//							//header.msgtype = eProtocol.ACKDOCPOFF;
				//							Protocol.SendACKDOCPOFF( socket, header );
				//							break;
				//					}
				//
				//				}
				//				catch( Exception e ) {
				//					OmException.LogException( new OmException( String.Format("Error in Recv_CP3 : {0}", e.Message ), e ) );
				//					return -1;
				//				}
			}
			return true;
		}
		public bool CheckPointStopSecondary( int lgnum ) {
			// get the thread for this logical group and role
			XMDThread xmd;
			if ( ( xmd = Lgnum_To_Thread( lgnum, eRole.Secondary ) ) != null ) {
				// group process running OK
				if (( xmd.m_IPCSocket == null ) ||
					( !xmd.m_IPCSocket.Connected )) { 
					// child running but connection not valid -
					// ack the command and let it retry if it wants to
					return false;
				}
				// pass along to the RMD
				try {
					// send header to command
					ProtocolHeader header = new ProtocolHeader();
					Protocol.SendHeader( xmd.m_IPCSocket, header );
				}
				catch( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in Recv_CP2 : {0}", e.Message ), e ) );
					return false;
				}
			} else {
				// send error ack to checkpoint command
				return false;
			}
			return true;
		}

		public string Version() {
			return System.Windows.Forms.Application.ProductVersion;
		}

	}
}
