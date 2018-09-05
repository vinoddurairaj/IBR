using System;
using System.Net.Sockets;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for KillBackfresh.
	/// </summary>
	public class KillBackfresh : Kill
	{
		public KillBackfresh()
		{
			//
			// TODO: Add constructor logic here
			//
			m_CommandName = CLI.LAUNCH_BACKFRESH;

		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			string pname;

			try {
				// process the command line arguments
				Proc_Args( args );

				// Create a list of all existing Primary started group(s), by refering to sxxx.cur
				// get the registry install Path
				m_ConfigInfos = Config.GetPrimaryConfigs();
//				m_ConfigInfos = Config.GetPrimaryStartedConfigs();

				if ( m_ConfigInfos.Count == 0 ) {
					OmException e = new OmException( String.Format("Error in Launch no logical group found." ) );
					throw e;
				}

				// kill targeted groups 
				foreach ( ConfigInfo config in m_ConfigInfos ) {
					if (( m_All ) ||
						( config.lgnum == this.m_lgnum )) {

						DriverIOCTL driver = new DriverIOCTL();
						try {
							if ( driver.GetGroupState( config.lgnum ) != eLgModes.BACKFRESH ) {
								continue;
							}
							pname = PMDThread.Name( config.lgnum );

							// create an ipc socket and connect it to the local master
							Socket MasterSocket = Protocol.IPCConnect();
							try {
								// send the message
								Protocol.SendCSIGUSR1( MasterSocket, pname );
//								// pstore -> ACCUMULATE
//								driver.ftd_lg_set_pstore_run_state( config.lgnum, config, -1 );
								// driver -> PASSTHRU
								driver.SetGroupState( config.lgnum, eLgModes.PASSTHRU );

							} finally {
								MasterSocket.Close();
							}
						}
						finally {
							driver.Close();
						}
					}
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}


	}
}
