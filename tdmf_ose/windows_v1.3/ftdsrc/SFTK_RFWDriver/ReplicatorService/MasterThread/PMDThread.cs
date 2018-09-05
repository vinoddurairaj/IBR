//using System;
//using System.Threading;
//using System.Net.Sockets;
//using System.Runtime.InteropServices;
//
//namespace MasterThread
//{
//	/// <summary>
//	/// Summary description for PMDThread.
//	/// </summary>
//	public class PMDThread : XMDThread
//	{
//
//		// set to true if restart is needed
//		//public bool m_Restart; use ExitCode
//		public eLgModes m_State;		// enumerated process state
//
//		/// <summary>
//		/// Constructor
//		/// </summary>
//		public PMDThread()
//		{
//		}
//
//		public void Start() {
//			try {
//				ErrorFacility.Init( "Replicator", this.m_thisThread.Name, null, null, 1, 0 );
//
//				try {
//					//error_tracef(TRACEINF,"PrimaryThread %s : Id %d" ,this.m_thisThread.Name, GetCurrentThreadId());
//					//LogicalGroup lgp = LogicalGroup.Init ( this.m_lgnum , eRole.Primary, true );
//					//lgp.OpenDev();
//					//lgp.flags.SET_LG_STATE( m_State );
//
//					// create an ipc socket and connect it to the local master
//					this.m_MasterSocket = Protocol.IPCConnect();
//					try {
//						ProtocolHeader header = new ProtocolHeader();
//						header.cli = this.m_thisThread.GetHashCode();
//						Protocol.SendHeader( this.m_MasterSocket, header );
//
//						// send the state to the driver
//						DriverIOCTL.SetLogicalGroupGState( this.m_lgnum, m_State );
//
//						// start the command handler loop
//						while( true ) {
//							// check for incomming commands from the MasterThread
//							ProcessCommands();
//
//							// house cleaning
//							LogicalGroup lgp;
//							try {
//								lgp = LogicalGroup.Init( this.m_lgnum, eRole.Primary, false );
//								// Tdmf management related operations
//								Management.DoLgMonitor( lgp );
//							} catch ( Exception e ) {
//								OmException.LogException( new OmException( String.Format("Error in PMDThread Start LogicalGroup.Init : {0}", e.Message ), e ) );
//								continue;
//							}
//						}
//					}
//					finally {
//						this.m_MasterSocket.Close();
//					}
//				} finally {
//					ErrorFacility.Close();
//				}
//			}
//			catch ( ThreadAbortException ) {
//				return;
//			}
//			catch ( Exception e ) {
//				OmException.LogException( new OmException( String.Format("Error in PMDThread Start : {0}", e.Message ), e ) );
//			}		
//		}
//
//		/// <summary>
//		///  Create PMD Thread name
//		/// </summary>
//		/// <param name="lgnum"></param>
//		/// <returns></returns>
//		public static string Name( int lgnum ) {
//			return "PMD_" + lgnum.ToString("D3");
//		}
//
//		/// <summary>
//		/// Check for and process incomming commands
//		/// </summary>
//		public void ProcessCommands() {
//			// check for new IPC commands
//			SocketCollection sockets = new SocketCollection();
//			sockets.Add( this.m_IPCSocket );
//			Socket.Select( sockets, null, null, 1000000 );
//			// yes new messages or connections
//			foreach( Socket socket in sockets ) {
//				// handle the new incoming IPC msg 
//				ftd_sock_recv_lg_msg( socket );
//			}
//		}
//
//		/// <summary>
//		/// ftd_sock_recv_lg_msg -- read a ftd message header and dispatch the message accordingly
//		/// </summary>
//		/// <param name="socket"></param>
//		/// <param name="lgp"></param>
//		public void ftd_sock_recv_lg_msg( Socket socket ) {
//			
//			ProtocolHeader header = new ProtocolHeader();
//
//			if ( socket.Available < Marshal.SizeOf( header ) ) {
//				// not enough data yet return
//				return;
//			}
//
//			// receive the header data
//			Protocol.ReceiveHeader( socket, ref header );
//
//			switch ( header.msgtype ) {
//				case eProtocol.CCPSTARTP:
//				case eProtocol.CCPSTARTS:
//				case eProtocol.ACKCPSTART:
////					rc = ftd_sock_recv_cpstart(lgp, &header);
//					break;
//				case eProtocol.CCPSTOPP:
//				case eProtocol.CCPSTOPS:
//				case eProtocol.ACKCPSTOP:
////					bDbgLogON = 0;      
////					rc = ftd_sock_recv_cpstop(lgp, &header);
//					break;
//				case eProtocol.CSTARTPMD:
////					// just return the translated state
////					rc = header.msg.lg.data;
////        
////					// hack for backfresh -f
////					if ((rc & 0x000000ff) == FTD_SBACKFRESH) {
////						if ((rc & FTD_LG_BACK_FORCE)) {
////							rc = FTD_SBACKFRESH;
////							SET_LG_BACK_FORCE(lgp->flags);
////						}
////					}
////
////					return ftd_fsm_state_to_input(rc);
//					break;
//				default:
//					OmException.LogException( new OmException ( "Error: PMD received and invalid command " + header.msgtype ) );
//					break;
//			}
//		}
//
//
//	}
//}
