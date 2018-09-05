using System;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.Runtime.InteropServices;


namespace MasterThread
{
	/// <summary>
	/// Summary description for RMDThread.
	/// </summary>
	public class RMDThread : XMDThread
	{

		// have a different class for applies
		//public bool apply;				    /* starts an apply rmd */
		public bool cpon = false;
		public Int32 consleep;					/* sleep value for reconnect */
		public eLgModes m_state;		// enumerated process state

		public string msg;						/* message */
		public Socket m_PMDsocket;				/* rmd needs pmd's socket -- dsockp -- */

		public LogicalGroup m_Lgp;

		/// <summary>
		/// Constructor
		/// </summary>
		public RMDThread()
		{
			this.m_Lgp = new LogicalGroup();
		}

		public void Start() {
			try {
				//
				// TODO: call the RMD code here
				//

				// execute the remote thread code
				ExecuteRMD();

			}
//			catch ( ThreadAbortException e ) {
//				OmException.LogException( new OmException( String.Format("Error in RMDThread Start ThreadAbortException: {0}, {1}", e.Message, e.ExceptionState ) ) );
//			}
			catch( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in RMDThread Start Exception: {0}", e.Message ), e ) );
			}
		}

		public static string Name( int lgnum ) {
			return "RMD_" + lgnum.ToString("D3");
		}


		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern Int32 RemoteThread( ref Args_t args );

		/// <summary>
		/// ftd_proc_exec_rmd -- execute the target rmd process
		/// </summary>
		/// <returns></returns>
		public void ExecuteRMD() {
			m_Args = new Args_t();

			m_Args.lgnum = this.m_lgnum;
			m_Args.state = (int)this.m_state;
			m_Args.apply = 0;
			// dup the socket handle so the management C++ code can close it.
			Handle.Duplicate( m_PMDsocket.Handle, out m_Args.dsock );
			//m_Args.dsock = this.m_PMDsocket.Handle;
			m_Args.procp = new proc_t();
			m_Args.procp.pid = this.m_thisThread.GetHashCode();

			// manual, not signaled
			m_Args.procp.hEvent = CreateEvent ( IntPtr.Zero, true, false );

			try {
				if ( m_Args.procp.hEvent == IntPtr.Zero ) {
					OmException.LogException( new OmException ( "Error: ExecuteRMD fail to CreateEvent" ) );
					return;
				}
				// call rmd thread function here
				RemoteThread( ref m_Args );

			} finally {
				// close the event
				Closehandle ( m_Args.procp.hEvent );
			}
		}




//		public int proc_terminate( proc_t procp ) {
//
//			if (proc_signal(procp, FTDCSIGTERM) == -1)
//				return -1;
//
//			if (procp.pid != INVALID_HANDLE_VALUE) {
//				if ( WaitForSingleObject(procp.pid, 60000) == WAIT_TIMEOUT ) {
//					return -1;
//				}
//			}
//			return 0;
//		}

//		void
//			proc_kill(proc_t *procp) {
//			if ( !proc_terminate(procp) )
//				return;
//
//			if (procp.pid != INVALID_HANDLE_VALUE)
//				TerminateThread( procp.pid, FTD_EXIT_DIE );
//		}











//		/// <summary>
//		/// Secondary Mirror Thread
//		/// </summary>
//		/// <returns></returns>
//		public eExitCodes RemoteThread() {
//			lgp.lgnum = this.m_lgnum;
//
//			if (ftd_lic_verify() < 0) {
//				//				error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_lic_verify()", procname );
//				goto errexit;
//			}
//
//			// inits all of the logical group structure and the device structures
//			try {
//				lgp = LogicalGroup.Ftd_Lg_Init( m_lgnum, eRole.Secondary, 0 );
//			} catch ( Exception e ) {
//				OmException.LogException( new OmException( String.Format("Error in Recv_Remote_Drive_Error Ftd_Lg_Init : {0}", e.Message ) ) );
//				goto errexit;
//			}
//
//			lgp.flags.SET_LG_STATE( this.m_state );
//
//			// copy the PMD socket to the LG socket
//			lgp.dsockp = this.m_PMDsocket;
//
//			// create an ipc socket and connect it to the local master
//			m_MasterSocket = Protocol.IPCConnect();
//
//			try {
//				try {
//					// tell master this is a child connect
//					header_u header;
//					header.header.cli = this.m_thisThread.GetHashCode();
//
//					// send header to command
//					Protocol.SendHeader( this.m_MasterSocket, header );
//				}
//				catch( Exception e ) {
//					OmException.LogException( new OmException( String.Format("Error in Recv_Start_PMD : {0}", e.Message ) ) );
//					goto errexit;
//				}
//
//				string prefix = "j" + this.m_lgnum.ToString("D3");
//
//				if ((lgp.jrnp = ftd_journal_create(lgp.cfgp.jrnpath,	prefix)) == NULL) {
//					//				error_tracef( TRACEERR, "RemoteThread main():%s : ftd_journal_create()", procname );
//					goto errexit;
//				}
//				// ONLY RMD
//				// remote mirror 
//				eExitCodes rc = RMD();
//
//			errExit:
//				if ( ( rc == eExitCodes.EXIT_DIE ) || ( rc == eExitCodes.EXIT_DRV_FAIL ) ) {
//					//				err_msg_t *lerrmsg;
//
//					if ( this.m_PMDsocket.Connected ) {
//						if ( rc == eExitCodes.EXIT_DIE ) {
//							//						if (strstr(lerrmsg.msg, "[FATAL /")) {
//							//							// report the FATAL error to peer
//							//							ftd_sock_send_err(lgp.dsockp, lerrmsg);
//							//						} 
//							//						else {
//							// just tell peer not to try to reconnect
//							try {
//								Protocol.SendACKKILL ( this.m_PMDsocket );
//							} catch ( Exception e ) {
//								OmException.LogException( new OmException( String.Format("Error in RemoteThread SendACKKILL : {0}", e.Message ) ) );
//							}
//							//						}
//						}
//						else {
//							// just tell peer not to try to reconnect
//							// send to RMD the FTDCREMDRIVEERR signal
//							try {
//								header_u header;
//								Protocol.SendCREMDRIVEERR ( this.m_PMDsocket, header, (int)~eRole.Primary, (int)eRole.Primary );
//							} catch ( Exception e ) {
//								OmException.LogException( new OmException( String.Format("Error in RemoteThread SendCREMDRIVEERR : {0}", e.Message ) ) );
//							}
//
//						}
//						// wait for the connection to be closed by the peer
//						Protocol.WaitForDisconnectAndClose( this.m_PMDsocket );
//						this.m_PMDsocket = null;
//					}
//				}
//			}
//			// close the socket to the MasterThread ( IPC connection )
//			finally {
//				if ( m_MasterSocket != null ) {
//					m_MasterSocket.Close();
//					m_MasterSocket = null;
//				}
//			}
//			return eExitCodes.EXIT_DIE;
//		}
//
//
//		/// <summary>
//		/// secondary mirror process
//		/// </summary>
//		/// <returns></returns>
//		public eExitCodes RMD ()
//		{
//			int rc;
//
//			// get journal state 
//			if (ftd_journal_get_cur_state(lgp.jrnp, 1) < 0) {
//				reporterr(ERRFAC, M_JRNINIT, ERRCRIT, LG_PROCNAME(lgp));
//				return eExitCodes.EXIT_DIE;
//			}
//
//			// set group checkpoint state according to journal state 
//			if (GET_JRN_CP_ON(lgp.jrnp.flags)) {
//				SET_LG_CPON(lgp.flags);
//			}
//			if (GET_JRN_CP_PEND(lgp.jrnp.flags)) {
//				SET_LG_CPPEND(lgp.flags);
//			}
//
//			// we know the first three msg types - we need to
//			// process them first before continueing
//			//
//			// messages:
//			//
//			// 1. version exchange
//			// 2. handshake
//			// 3. config file exchange
//			//
//			// process: version, handshake, config info
//			int i = 0;
//			do {
//				if ((rc = Sock_Recv_Lg_Msg( this.m_PMDsocket, lgp, -1 )) != 0 ) {
//					if (rc < 0) {
//						return eExitCodes.EXIT_DIE;
//					} 
//					if (rc == FTD_LG_NET_NOT_READABLE) {
//						continue;
//					} 
//					else {
//						error_tracef( TRACEINF, "ftd_rmd:rc == %d",rc );
//						return rc;			
//					}
//				}
//			} while (i++ == 2);
//
//			// set net connection to non-blocking
////			ftd_sock_set_nonb(lgp.dsockp);
//
//			if ( lgp.flags.GET_LG_STATE() == LG_States.SREFRESHF) {
//				// got state from handshake - hammer journal files
//				ftd_journal_delete_all_files(lgp.jrnp);
//				SET_JRN_MODE(lgp.jrnp.flags, FTDMIRONLY);
//			}
//
//			// figure out if we are journaling or mirroring 
//			if (GET_JRN_MODE(lgp.jrnp.flags) == FTDMIRONLY) {
//				if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDMIRONLY) < 0) {
//					return eExitCodes.EXIT_DIE;
//				}
//			} else {
//				if (ftd_lg_start_journaling(lgp, GET_JRN_STATE(lgp.jrnp.flags)) < 0) {
//					return eExitCodes.EXIT_DIE;
//				}
//			}
//
//			// if coherent journals to apply then do it 
//			if ( lgp.flags.GET_LG_CPPEND() ) {
//				try {
//					Protocol.SendStartApply( this.m_MasterSocket, m_lgnum, 1 );
//				} catch ( Exception e ) {
//					OmException.LogException( new OmException( String.Format("Error in RMD SendStartApply 1 : {0}", e.Message ) ) );
//					return eExitCodes.EXIT_DIE;
//				}
//			} else if (GET_JRN_STATE(lgp.jrnp.flags) == FTDJRNCO
//				&& GET_JRN_MODE(lgp.jrnp.flags) != FTDMIRONLY
//				&& !GET_LG_CPON(lgp.flags) ) {
//				try {
//					Protocol.SendStartApply( this.m_MasterSocket, m_lgnum, 0 );
//				} catch ( Exception e ) {
//					OmException.LogException( new OmException( String.Format("Error in RMD SendStartApply 0 : {0}", e.Message ) ) );
//					return eExitCodes.EXIT_DIE;
//				}
//			}
//
//			lgp.flags.SET_LG_STATE( LG_States.SNORMAL);
//
//			// init the state machine 
//			ftd_fsm_init(ftd_ttfsm, ftd_ttstab, FTD_NSTATES);
//
//			// start the state machine 
//			ret = ftd_fsm(lgp, ftd_ttfsm, ftd_ttstab, FTD_SNORMAL, 0);
//
//			return ret;
//		}
//
//
//		/*
//		 * ftd_sock_recv_lg_msg --
//		 * read a ftd message header and dispatch the message accordingly
//		 */
//		public int Sock_Recv_Lg_Msg( Socket socket, ftd_lg_t *lgp, int timeo )
//		{
//			header_u    header;
//			int             rc;
//
//			SocketCollection reads = new SocketCollection();
//			reads.Add( socket );
//			// Only the sockets that contain a connection request
//			// will remain in listenList after Select returns.
//			Socket.Select( reads, null, null, timeo );
//
//			if ( reads.Count == 0 ) {
//				// nothing to read
//				return Protocol.LG_NET_NOT_READABLE;
//			}
//
//			try {
//				Protocol.ReceiveHeader( socket, header );
//			} catch ( Exception e ) {
//				// socket error - or connection closed
//				OmException.LogException( new OmException( String.Format("Error in Sock_Recv_Lg_Msg ReceiveHeader : {0}", e.Message ) ) );
//				throw e;
//			}
//
//			rc = 0; 
//
//			switch( header.header.msgtype ) {
//				case eProtocol.CNOOP:
//					rc = ftd_sock_recv_noop( socket, header);
//					break;
//				case eProtocol.CHANDSHAKE:
//					rc = ftd_sock_recv_handshake( socket, header, lgp);
//					break;
//				case eProtocol.CCHKCONFIG:
//					rc = ftd_sock_recv_chkconfig( socket, header, lgp);
//					break;
//				case eProtocol.CHUP:
//					rc = ftd_sock_recv_hup( socket, header);
//					break;
//				case eProtocol.CCHKSUM:
//					rc = ftd_sock_recv_rsync_chksum( socket, header, lgp);
//					break;
//				case eProtocol.CCHUNK:
//					rc = ftd_sock_recv_bab_chunk( socket, header, lgp);
//					break;
//				case eProtocol.CRFBLK:
//					rc = ftd_sock_recv_refresh_block( socket, header, lgp);
//					break;
//				case eProtocol.CBFBLK:
//					rc = ftd_sock_recv_backfresh_block( socket, header, lgp);
//					break;
//				case eProtocol.CRFSTART:
//				case eProtocol.CRFFSTART:
//				case eProtocol.CBFSTART:
//					rc = ftd_sock_recv_rsync_start(header, lgp);
//					break;
//				case eProtocol.CRFEND:
//				case eProtocol.CRFFEND:
//				case eProtocol.CBFEND:
//					rc = ftd_sock_recv_rsync_end(header.msgtype, lgp);
//					break;
//				case eProtocol.CRSYNCDEVS:
//					rc = ftd_sock_recv_rsync_devs( socket, header, lgp);
//					break;
//				case eProtocol.CRSYNCDEVE:
//					rc = ftd_sock_recv_rsync_deve( socket, header, lgp);
//					break;
//				case eProtocol.CVERSION:
//					rc = ftd_sock_recv_version( socket, header, lgp);
//					break;
//				case eProtocol.CREFOFLOW:
//					rc = ftd_sock_recv_refoflow(lgp);
//					break;
//				case eProtocol.CEXIT:
//					rc = ftd_sock_recv_exit( socket, header);
//					break;
//				case eProtocol.ACKERR:
//					rc = ftd_sock_recv_err( socket, header);
//					break;
//				case eProtocol.ACKHANDSHAKE:
//					rc = ftd_sock_recv_ack_handshake(lgp, header);
//					break;
//				case eProtocol.ACKCONFIG:
//					break;
//				case eProtocol.ACKRSYNC:
//					rc = ftd_sock_recv_ack_rsync( socket, header, lgp);
//					break;
//				case eProtocol.ACKHUP:
//					rc = ftd_sock_recv_ack_hup( socket, header);
//					break;
//				case eProtocol.ACKCHUNK:
//					rc = ftd_sock_recv_ack_chunk( socket, header, lgp);
//					break;
//				case eProtocol.ACKKILL:
//					rc = ftd_sock_recv_ack_kill();
//					break;
//				case eProtocol.ACKCHKSUM:
//					rc = ftd_sock_recv_ack_chksum( socket, header, lgp);
//					break;
//				case eProtocol.CCPSTARTP:
//				case eProtocol.CCPSTARTS:
//				case eProtocol.ACKCPSTART:
//					rc = ftd_sock_recv_cpstart(lgp, header);
//					break;
//				case eProtocol.CCPSTOPP:
//				case eProtocol.CCPSTOPS:
//				case eProtocol.ACKCPSTOP:
//					bDbgLogON = 0;      
//					rc = ftd_sock_recv_cpstop(lgp, header);
//					break;
//				case eProtocol.ACKRFSTART:
//					rc = ftd_sock_recv_ack_rfstart(lgp);
//					break;
//
//				case eProtocol.CCPON:
//					bDbgLogON = 1;  //no break, ok
//					rc = ftd_sock_recv_cpon();
//					break;
//				case eProtocol.ACKCPON:
//					rc = ftd_sock_recv_cpon();
//					break;
//
//				case eProtocol.CCPOFF:
//				case eProtocol.ACKCPOFF:
//					rc = ftd_sock_recv_cpoff();
//					break;
//				case eProtocol.CCPONERR:
//				case eProtocol.ACKCPONERR:
//					rc = ftd_sock_recv_cponerr( socket, lgp );
//					break;
//				case eProtocol.CCPOFFERR:
//				case eProtocol.ACKCPOFFERR:
//					rc = ftd_sock_recv_cpofferr( socket, lgp );
//					break;
//				case eProtocol.CSTARTPMD:
//        
//					// just return the translated state
//					rc = header.msg.lg.data;
//        
//					// hack for backfresh -f
//					if ((rc & 0x000000ff) == FTD_SBACKFRESH) {
//						if ((rc & FTD_LG_BACK_FORCE)) {
//							rc = FTD_SBACKFRESH;
//							SET_LG_BACK_FORCE(lgp.flags);
//						}
//					}
//
//					return ftd_fsm_state_to_input(rc);
//					break;
//
//					// FTDCREMDRIVEERR sent from Secondary Master to RMD or RMD to PMD peer!! WR17511
//				case eProtocol.CREMDRIVEERR:
//					rc = ftd_sock_recv_drive_err(lgp, header);
//					break;
//
//				default:
//					break;
//			}
//
//			return rc;
//		}





	}
}

