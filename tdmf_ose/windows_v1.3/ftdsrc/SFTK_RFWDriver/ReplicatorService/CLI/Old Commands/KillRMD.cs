//using System;
//using System.Net.Sockets;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for KillRMD.
//	/// </summary>
//	public class KillRMD : Kill
//	{
//		public KillRMD()
//		{
//			m_CommandName = CLI.KILL_RMD;
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
//				m_ConfigInfos = Config.GetSecondaryConfigInfos();
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
//							pname = RMDThread.Name( config.lgnum );
//							// create an ipc socket and connect it to the local master
//							Socket MasterSocket = Protocol.IPCConnect();
//							try {
//								// send the message
//								Protocol.SendCSIGTERM( MasterSocket, pname );
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
//
//	}
//}
