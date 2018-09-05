using System;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Collections;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for CheckPoint.
	/// </summary>
	public class CheckPoint : AllGroup
	{

		bool m_CP_On = false;
		bool m_CP_Off = false;
		bool m_Primary = true;
		bool m_Wait = false;

		/// <summary>
		/// Constructor
		/// </summary>
		public CheckPoint() {
			//
			// TODO: Add constructor logic here
			//
			m_CommandName = CLI.CHECKPOINT;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage() {
			Console.WriteLine( String.Format( "\nUsage: {0} <options>\n", this.m_CommandName ) );
			Console.WriteLine( String.Format(	"OPTIONS:" ) );
			base.Usage();
			Console.WriteLine( String.Format( "    -on             : turn checkpoint on" ) );
			Console.WriteLine( String.Format( "    -off            : turn checkpoint off" ) );
			Console.WriteLine( String.Format( "    -s              : turn checkpoint on or off from secondary" ) );
			Console.WriteLine( String.Format( "    -w              : wait for response" ) );
			Environment.Exit( 1 );
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual void	Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 2 ) {
				Usage();
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					case "-on":
					case "/on":
						if ( m_CP_Off ) {
							Usage();
						}
						m_CP_On = true;
						break;
					case "-off":
					case "/off":
						if ( m_CP_On ) {
							Usage();
						}
						m_CP_Off = false;
						break;
					case "-s":
					case "/s":
						m_Primary = false;
						break;
					case "-w":
					case "/w":
						m_Wait = true;
						break;
					default:
						// handle the group and all commands
						if ( !base.Proc_Args( args, index ) ) {
							Usage();
						}
						break;
				}			
			}
			if (( !m_All ) && ( !m_Group )) {
				Usage();
			}
			if (( !m_CP_On ) && ( !m_CP_Off )) {
				Usage();
			}
		}

		/// <summary>
		/// perform checkpoint for down group
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="MsgType"></param>
		/// <param name="pcpHostName"></param>
		/// <returns></returns>
		public void DoCheckPoint ( int lgnum, eProtocol MsgType ) {
			LogicalGroup lgp;
			//int rc;

			// perform checkpoint for down group
			try {
				lgp = LogicalGroup.Init( lgnum, eRole.Secondary, false );
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in DoCheckPoint Ftd_Lg_Init : {0}", e.Message ) ) );
				throw e;
			}

			string journal = String.Format( "j{0}", lgp.lgnum.ToString("D3") );

			// TODO: Impliment
//			if ( (lgp.jrnp = Journal.ftd_journal_create( lgp.cfgp.jrnpath, journal )) == null ) {
//				throw new OmException( "Error: in ftd_journal_create" );
//			}
//
//			Journal.ftd_journal_get_cur_state( lgp.jrnp, false );
//
//			// set group flags according to journal flags
//			if ( lgp.jrnp.flags.GET_JRN_CP_ON() ) {
//				lgp.flags.SET_LG_CPON();
//			}
//
//			if ( lgp.jrnp.flags.GET_JRN_CP_PEND() ) {
//				lgp.flags.SET_LG_CPPEND();
//			}
//
//			string Group = String.Format( "Group {0}", lgp.lgnum.ToString("D3") );
//
//			lgp.isockp = Protocol.IPCConnect();
//			
//			switch ( MsgType ) {
//				case eProtocol.ACKDOCPON:
//					// if checkpoint file exits then already in
//					// cp or transitioning to cp
//					if ( lgp.flags.GET_LG_CPPEND() ) {
//						OmException e = new OmException( "Received CheckPoint On again?" );
//						OmException.LogException( e );
//						throw e;
//					} 
//					else if ( lgp.flags.GET_LG_CPON() ) {
//						OmException e = new OmException( "Received CheckPoint On again?" );
//						OmException.LogException( e );
//						throw e;					}
//					else {
//						rc = MasterThread.CheckPoint.ftd_lg_cpstart( lgp );
//					}
//					break;
//				case eProtocol.ACKDOCPOFF:
//					if ( !lgp.flags.GET_LG_CPON() && !lgp.flags.GET_LG_CPPEND() ) {
//						OmException e = new OmException( "Warning: Check Point not on?" );
//						OmException.LogException( e );
//						throw e;					}
//					else {
//						MasterThread.CheckPoint.ftd_lg_cpstop( lgp );
//					}
//					break;
//			}
		}


		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			try {
				// process the command line arguments
				Proc_Args( args );

				//send status msg to System Event Log and TDMF Collector
				Management.LogMessage( args );

				// Run checkpoint cmd on Primary
				if ( m_Primary ) {
					// Create a list of all existing Primary started group(s), by refering to sxxx.cur
					m_ConfigInfos = Config.GetPrimaryConfigs();
//					m_ConfigInfos = Config.GetPrimaryStartedConfigs();
				}
				else {
					// Run checkpoint cmd on Secondary
					// Create a list of all existing Secondary group(s)
					m_ConfigInfos = Config.GetSecondaryConfigs();
				}
				// if none exist then exit  
				if ( m_ConfigInfos.Count == 0 ) {
					OmException e = new OmException( String.Format("Error in CheckPoint no logical group found." ) );
					throw e;
				}

				for ( int count = 0 ; count < m_ConfigInfos.Count ; ) {
					ConfigInfo configInfo = m_ConfigInfos[ count ];
					bool success = false;
					if (( m_All ) ||
						( configInfo.lgnum == this.m_lgnum )) {
						LgConfig config = Config.ReadConfig( configInfo.lgnum, configInfo.role );
						if ( m_Primary ) {
							if ( Protocol.Is_This_Host( config.phostname ) ) {
								success = SendCPCommand ( config, eRole.Primary );
							} else {
								continue;
							}
						} else {
							// check if loop back
							if ( String.Compare( config.phostname, config.shostname, true ) != 0 ) {
								success = SendCPCommand ( config, eRole.Primary );
							}
							else {
								success = SendCPCommand ( config, eRole.Secondary );
							}
						}
						if ( !success ) {
							m_ConfigInfos.Remove( configInfo );
						} else {
							count++;
						}
						Thread.Sleep ( 10000 );
					}
				}
				if ( this.m_Wait ) {
					if ( !CpWaitForResult ( m_ConfigInfos ) ) {
						Console.WriteLine( "Timed out waiting for Status Change" );
						Environment.Exit( 1 );
					}
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message ) );
				Console.WriteLine( e.Message );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}




		public bool SendCPCommand ( LgConfig config, eRole role ) {

			// create an ipc socket and connect it to the local master
			Socket MasterSocket = Protocol.IPCConnect();

			eProtocol ackType = eProtocol.Invalid;

			try {
				switch( role ) {
					case eRole.Primary:
						if ( this.m_CP_On ) {
							// send the SendCCPStartP and wait for ackCLI response
							ackType = Protocol.SendCCPStartP( MasterSocket, config.lgnum );
						} else {
							// send the SendCCPStartP and wait for ackCLI response
							ackType = Protocol.SendCCPStopP( MasterSocket, config.lgnum );
						}
						break;
					case eRole.Secondary:
						if ( this.m_CP_On ) {
							// send the SendCCPStartP and wait for ackCLI response
							ackType = Protocol.SendCCPStartS( MasterSocket, config.lgnum );
						} else {
							ackType = Protocol.SendCCPStopS( MasterSocket, config.lgnum );
							// send the SendCCPStartP and wait for ackCLI response
						}
						break;
				}
			} catch ( Exception e ) {
				OmException e1 = new OmException( String.Format("Error in SendCPCommand : {0}", e.Message ) );
				throw e1;
			}
			finally {
				MasterSocket.Close();
			}
			switch ( ackType ) {
				case eProtocol.ACKDOCPON:
				case eProtocol.ACKDOCPOFF:
					try {
						DoCheckPoint( config.lgnum, ackType );
					} catch ( Exception e ) {
						throw e;
					}
					break;
				case eProtocol.ACKERR:
				case eProtocol.ACKNORMD:
				case eProtocol.ACKNOPMD:
				default:
					string msg = String.Format( "Error: SendCCPSxxx received: " + ackType );
					OmException.LogException( new OmException( msg ) );
					return false;
			}
			return true;
		}


		public bool CpWaitForResult ( ConfigInfoCollection configs ) {
			int retry = 0;

			foreach ( LgConfig config in configs ) {
				while ( true ) {
					if ( CpWaitForPrimary( config.lgnum ) ) {
						string CP = m_CP_On ? "ON" : "OFF";
						Console.WriteLine( "Group " + config.lgnum + " is in CheckPoint mode " + CP );
						break;
					}
					if ( retry++ >= 5 ) {
						Console.WriteLine( "CheckPoint submited for group " + config.lgnum + " but cannot get status" );
						return false;
					}
					Thread.Sleep ( 1000 );
				}
			}
			return true;
		}


		/// <summary>
		/// 
		/// </summary>
		/// <param name="lgnum"></param>
		/// <returns>true if check point on, else false. throws errors</returns>
		bool CpWaitForPrimary ( int lgnum ) {
			LogicalGroup lgp;
			ps_group_info_t	groupInfo = new ps_group_info_t();

			try {
				lgp = LogicalGroup.Init( lgnum, eRole.Primary, true );
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in DoCheckPoint Ftd_Lg_Init : {0}", e.Message ) ) );
				throw e;
			}
			DriverIOCTL driver = new DriverIOCTL();
			try {
				driver.ps_get_group_info( lgp.lgnum, ref groupInfo );
			}
			finally {
				driver.Close();
			}
			if ( groupInfo.checkpoint == this.m_CP_On ) { // OK, CP on
				return true;
			}
			return false;
		}


	}
}
