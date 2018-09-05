//using System;
//using System.Net.Sockets;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for KillRefresh.
//	/// </summary>
//	public class KillRefresh : Kill
//	{
//		public KillRefresh()
//		{
//			m_CommandName = CLI.KILL_REFRESH;
//		}
//
//		/// <summary>
//		/// Main execution call
//		/// </summary>
//		/// <param name="args"></param>
//		public override void Execute( string [] args ) {
//			string pname;
//
//			try {
//				// process the command line arguments
//				Proc_Args( args );
//
//				// Create a list of all existing Primary started group(s), by refering to sxxx.cur
//				// get the registry install Path
//				m_ConfigInfos = Config.GetPrimaryConfigInfos();
////				m_ConfigInfos = Config.GetPrimaryStartedConfigs();
//
//				if ( m_ConfigInfos.Count == 0 ) {
//					OmException e = new OmException( String.Format("Error in Launch no logical group found." ) );
//					throw e;
//				}
//
//				// kill targeted groups 
//				foreach ( ConfigInfo config in m_ConfigInfos ) {
//					if (( m_All ) ||
//						( config.lgnum == this.m_lgnum )) {
//						DriverIOCTL driver = new DriverIOCTL();
//						try {
//							eLgModes state = driver.GetGroupState( config.lgnum );
//							if (( state != eLgModes.REFRESH ) &&
//								( state != eLgModes.FULLREFRESH ) &&
//								( state != eLgModes.CHECKSUMREFRESH )) {
//								continue;
//							}
//							pname = PMDThread.Name( config.lgnum );
//							// create an ipc socket and connect it to the local master
//							Socket MasterSocket = Protocol.IPCConnect();
//							try {
//								// send the message
//								Protocol.SendCSIGUSR1( MasterSocket, pname );
//								// group not running - driver -> TRACKING
//								driver.SetGroupState( config.lgnum, eLgModes.TRACKING );
//							} finally {
//								MasterSocket.Close();
//							}
//						}
//						finally {
//							driver.Close();
//						}
//					}
//				}
//			} catch ( Exception e ) {
//				OmException.LogException( new OmException( e.Message, e ) );
//				Environment.Exit( 1 );
//			}
//			Environment.Exit( 0 );
//		}
//
//	}
//}
