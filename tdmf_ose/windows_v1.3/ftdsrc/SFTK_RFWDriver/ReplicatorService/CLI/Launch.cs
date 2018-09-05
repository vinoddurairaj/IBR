using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using MasterThread;


namespace CLI
{
	/// <summary>
	/// Summary description for Launch.
	/// </summary>
	public class Launch : AllGroup
	{
		public bool	m_Pending = false;
		public eLgModes m_State;

		public eProtocol m_msgtype;

		/// <summary>
		/// Constructor
		/// </summary>
		public Launch()
		{
			//
			// TODO: Add constructor logic here
			//
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			bool TryAgain = false;
			m_msgtype = eProtocol.CSTARTPMD;

			try {
				// process the command line arguments
				Proc_Args( args, 0 );

				// Create a list of all existing Primary started group(s), by refering to sxxx.cur
				// get the registry install Path
				m_ConfigInfos = Config.GetPrimaryConfigs();
//				m_ConfigInfos = Config.GetPrimaryStartedConfigs();

				if ( m_ConfigInfos.Count == 0 ) {
					OmException e = new OmException( String.Format("Error in Launch no logical group found." ) );
					throw e;
				}

				// Use Pending mode to introduced a delayed launchrefresh
				if ( m_Pending ) {
					Thread.Sleep( 15000 );
					TryAgain = true;
				}

				foreach ( ConfigInfo config in m_ConfigInfos ) {
					if (( m_All ) ||
						( config.lgnum == this.m_lgnum )) {

						try {
							// Verify the checkpoint status **rddev 021202
							if ( IsCheckpoint( config.lgnum ) ) {
								if (( m_State == eLgModes.REFRESH ) ||
									( m_State == eLgModes.CHECKSUMREFRESH )) {
									string msg = String.Format( "Error: LG{0} Checkpoint is ON.\nLaunch a full refresh.", config.lgnum );
									Console.WriteLine ( msg );
									OmException e = new OmException( msg );
									throw e;
								}
							}
							if ( ( m_State == eLgModes.NORMAL ) || /*launch pmd */
								( m_State == eLgModes.REFRESH ) ) { /* launch smart refresh */
								if ( GetLgMode( config.lgnum ) == eLgModes.PASSTHRU ) {
									string msg = String.Format( "Error: LG{0} is in Passthru.", config.lgnum );
									Console.WriteLine( msg );
									OmException e = new OmException( msg );
									throw e;
								}
							}
						} catch ( Exception e ) {
							string msg = String.Format( "Error: Launch error - LG{0} - " + e.Message, config.lgnum );
							OmException e1 = new OmException( msg, e );
							throw e1;
						}

						TryAgain = false;
						do {
							// waits 20 secs before launching a SMART REFRESH or FULL REFRESH
							if ( m_Pending && (( m_State == eLgModes.REFRESH ) ||
								( m_State == eLgModes.FULLREFRESH ))) {
								Thread.Sleep(20000);                         
							}

							// create an ipc socket and connect it to the local master
							Socket MasterSocket = Protocol.IPCConnect();
							try {

								// send the StartPMD and wait for ackCLI response
								try {
									Protocol.SendStartPMD( MasterSocket, config.lgnum, m_State );
								} catch ( Exception e ) {
									OmException e1 = new OmException( String.Format("Error in Launch SendStartPMD : {0}", e.Message ) );
									throw e1;
								}
							}
							finally {
								MasterSocket.Close();
							}
							if ( m_Pending ) {
								// waits 20 secs before checking the LG state
								Thread.Sleep( 20000 );                         

								if ( GetLgMode( config.lgnum ) != eLgModes.TRACKING ) {
									TryAgain = true;
								}
							}
						} while ( TryAgain );
					}
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage()
		{
			Console.WriteLine( String.Format( "\nUsage: {0} <options>\n", this.m_CommandName ) );
			Console.WriteLine( String.Format( "One of the following two options is mandatory:" ));
			base.Usage();
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public override bool Proc_Args( string[] args, int index )
		{
			switch ( args[ index ].ToLower() ) {
				case "-c":
				case "/c":
					// perform verify secondary - All dirtybit/chksum
					m_State = eLgModes.CHECKSUMREFRESH;
					break;
				case "-p":
				case "/p":
					m_Pending = true;
					break;
				default:
					// handle the group and all commands
					if ( !base.Proc_Args( args, index ) ) {
						Usage();
					}
					break;
			}
			if (( !m_All ) && ( !m_Group )) {
				Usage();
			}
			return true;
		}

		/// <summary>
		/// Verify the check point state for this logical group
		/// </summary>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public bool IsCheckpoint ( int lgnum ) {
//			LogicalGroup lgp;
			ps_group_info_t	group_info = new ps_group_info_t();

			// Verify checkpoint status for down group
//			try {
//				lgp = LogicalGroup.Init( lgnum, eRole.Primary, true );
//			} catch ( Exception e ) {
//				throw new OmException( String.Format("Error in IsCheckpoint Init : {0}", e.Message ) );
//			}

			DriverIOCTL driver = new DriverIOCTL();
			try {
				try {
					driver.ps_get_group_info( lgnum, ref group_info);
				} catch ( Exception e ) {
					throw new OmException( String.Format("Error in IsCheckpoint ps_get_group_info : {0}", e.Message ), e );
				} 
			}
			finally {
				driver.Close();
			}
			return group_info.checkpoint;
		}

		public eLgModes GetLgMode ( int lgnum )
		{
			try {
				// Verify LG state for down group
				DriverIOCTL driver = new DriverIOCTL();
				try {
					return driver.GetGroupState( lgnum );
				}
				finally {
					driver.Close();
				}
			} catch ( Exception e ) {
				throw new Exception( "Error: GetLgMode failed : " + e.Message, e );
			}
		}



	}
}
