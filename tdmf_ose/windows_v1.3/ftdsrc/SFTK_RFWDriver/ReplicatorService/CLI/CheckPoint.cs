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
	public class CheckPoint : StateCommand
	{
		bool m_CP_On = false;
		bool m_CP_Off = false;
		bool m_Primary = true;
		bool m_Wait = false;

		/// <summary>
		/// Constructor
		/// </summary>
		public CheckPoint() {
			m_CommandName = CLI.CHECKPOINT;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage( bool exit ) {
			base.Usage( false );
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
		public override void Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 2 ) {
				Usage( true );
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					case "-on":
					case "/on":
						if ( m_CP_Off ) {
							Usage( true );
						}
						m_CP_On = true;
						break;
					case "-off":
					case "/off":
						if ( m_CP_On ) {
							Usage( true );
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
							Usage( true );
						}
						break;
				}			
			}
			if (( !m_All ) && ( !m_Group )) {
				Usage( true );
			}
			if (( !m_CP_On ) && ( !m_CP_Off )) {
				Usage( true );
			}
		}

		/// <summary>
		/// perform checkpoint for down group
		/// </summary>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public void DoCheckPoint ( int lgnum ) {
			LogicalGroup lgp;

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
//			if ( m_CP_ON ) {
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
//						throw e;
//					} else {
//						rc = MasterThread.CheckPoint.ftd_lg_cpstart( lgp );
//					}
//			} else {
//					if ( !lgp.flags.GET_LG_CPON() && !lgp.flags.GET_LG_CPPEND() ) {
//						OmException e = new OmException( "Warning: Check Point not on?" );
//						OmException.LogException( e );
//						throw e;					}
//					else {
//						MasterThread.CheckPoint.ftd_lg_cpstop( lgp );
//					}
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

				MasterThread.MasterThread masterThread = MasterThread.MasterThread.GetMasterThread();
				LgConfigCollection LgConfigs;
				if ( m_Primary ) {
					// Run checkpoint cmd on Primary
					// get a list of all existing Primary started group(s), by refering to sxxx.cur
					LgConfigs = masterThread.LgConfigsPrimary;
				}
				else {
					// Run checkpoint cmd on Secondary
					// get a list of all existing Secondary group(s)
					LgConfigs = masterThread.LgConfigsSecondary;
				}
				// if none exist then exit  
				if ( LgConfigs.Count == 0 ) {
					OmException e = new OmException( String.Format("Error in CheckPoint no logical group found." ) );
					throw e;
				}

				for ( int count = 0 ; count < LgConfigs.Count ; ) {
					LgConfig config = LgConfigs[ count ];
					bool success = false;
					if (( m_All ) ||
						( config.lgnum == this.m_lgnum )) {
						if ( m_Primary ) {
							if ( Protocol.IsThisHost( config.phostname ) ) {
								success = SendCPCommand ( masterThread, config, eRole.Primary );
							} else {
								continue;
							}
						} else {
							// check if loop back
							if ( String.Compare( config.phostname, config.shostname, true ) != 0 ) {
								success = SendCPCommand ( masterThread, config, eRole.Primary );
							}
							else {
								success = SendCPCommand ( masterThread, config, eRole.Secondary );
							}
						}
					}
					if ( !success ) {
						LgConfigs.Remove( config );
					} else {
						count++;
					}
					Thread.Sleep ( 10000 );
				}
				if ( this.m_Wait ) {
					if ( !CpWaitForResult ( LgConfigs ) ) {
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

		public bool SendCPCommand ( MasterThread.MasterThread masterThread, LgConfig config, eRole role ) {
			try {
				switch( role ) {
					case eRole.Primary:
						if ( this.m_CP_On ) {
							// send the SendCCPStartP and wait for ackCLI response
							masterThread.CheckPointStartPrimary( config.lgnum );
						} else {
							// send the SendCCPStartP and wait for ackCLI response
							masterThread.CheckPointStopPrimary( config.lgnum );
						}
						break;
					case eRole.Secondary:
						bool ret;
						if ( this.m_CP_On ) {
							// send the SendCCPStartP and wait for ackCLI response
							ret = masterThread.CheckPointStartSecondary( config.lgnum );
						} else {
							ret = masterThread.CheckPointStopSecondary( config.lgnum );
						}
						if ( !ret ) {
							try {
								DoCheckPoint( config.lgnum );
							} catch ( Exception e ) {
								throw e;
							}
						}
						break;
				}
			} catch ( Exception e ) {
				OmException e1 = new OmException( String.Format("Error in SendCPCommand : {0}", e.Message ) );
				throw e1;
			}
			return true;
		}

		public bool CpWaitForResult ( LgConfigCollection configs ) {
			int retry = 0;
			foreach ( LgConfig config in configs ) {
				int lgnum = config.lgnum;
				while ( true ) {
					if ( IsCheckPoint( lgnum ) ) {
						string CP = m_CP_On ? "ON" : "OFF";
						Console.WriteLine( "Group " + lgnum + " is in CheckPoint mode " + CP );
						break;
					}
					if ( retry++ >= 5 ) {
						Console.WriteLine( "CheckPoint submited for group " + lgnum + " but cannot get status" );
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
		bool IsCheckPoint ( int lgnum ) {
			GroupInfo groupInfo = new GroupInfo();

			DriverIOCTL driver = new DriverIOCTL();
			try {
				driver.GetGroupInfo( lgnum, ref groupInfo );
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
