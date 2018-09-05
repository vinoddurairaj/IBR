using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Runtime.InteropServices;

namespace MasterThread.Management
{
	/// <summary>
	/// Summary description for ManagementStatusThread.
	/// </summary>
	public class ManagementStatusThread : XMDThread {

		//THE one and only Status msg list instance
		public static StatusMsgCollection gStatusMsgList;

		public static AutoResetEvent [] m_AutoEvents = new AutoResetEvent[1];


		/// <summary>
		/// constructor
		/// </summary>
		public ManagementStatusThread() {
		}

		public void Start() {
			try {
				// execute the status thread code
				ftd_mngt_StatusMsgThread();
			}
			catch ( ThreadAbortException e ) {
				OmException.LogException( new OmException( String.Format("Error in StatisticsThread Start : {0}, {1}", e.Message, e.ExceptionState ), e ) );
				return;
			}
			catch( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in StatisticsThread Start Exception: {0}", e.Message ), e ) );
			}
		}

		public static string Name() {
			return "management";
		}

		//Thread launched when TDMF Agent starts.
		public static void ftd_mngt_StatusMsgThread() {
			try {
				// stuff releated to the Agent Alive Socket
				Socket aliveSocket = new Socket( AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp );
				try {
					for( int i = 0 ; i < m_AutoEvents.Length ; i++ ) {
						m_AutoEvents[i] = new AutoResetEvent( false );
					}
					while ( true ) {
						// Wait for time out or message send event
						int index = WaitHandle.WaitAny( m_AutoEvents, new TimeSpan(0, 0, 3), false );
						if ( index != WaitHandle.WaitTimeout ) {
							foreach ( StatusMessage message in gStatusMsgList ) {
								// send Status Messages to Collector
								Socket socket = Protocol.CollectorConnect();
								try {
									Protocol.SendStatusMsg( socket, message, new UniqueHostID() );
								} finally {
									socket.Close();
								}
							}
							// clear the list
							gStatusMsgList.Clear();
						}					
						// here, we work with the Alive Socket
						ftd_mngt_alive_socket( ref aliveSocket );
					}
				} finally {
					aliveSocket.Close();
					for ( int i = 0 ; i < m_AutoEvents.Length ; i++ ) {
						m_AutoEvents[i].Close();
						m_AutoEvents[i] = null;
					}
				}
			}
			catch ( ThreadAbortException e ) {
				throw e;
			}
			catch ( Exception e ) {
				OmException.LogException( new OmException( "Error: ProcessIO caught an exception - " + e.Message, e ) );
			}
		}


		public static int ALIVE_TIMEOUT = 30;

		public static void ftd_mngt_alive_socket( ref Socket aliveSocket ) {
			if ( !aliveSocket.Connected ) {   
				if ( !Protocol.IsCollectorAvailable() ) {
					return;
				}
				try {
					aliveSocket = Protocol.CollectorConnect();
				} catch ( Exception ) {
					Management.gbTDMFCollectorPresent = false;
					return;
				}
				Management.gbTDMFCollectorPresent = true;
				Management.gbTDMFCollectorPresentWarning = false;

				// make sure AliveMsgTagValue can eat it all up ...
				string AliveMsgTagValue = String.Format ( "{0}{1} \0", Protocol.MMP_MNGT_ALIVE_TAG_MMPVER, Protocol.MMP_PROTOCOL_VERSION );

				// send status message to requester
				try {
					Protocol.SendCollectorAlive( aliveSocket, AliveMsgTagValue, new UniqueHostID() ); 
					// some information must be sent to Collector
					// begin by sending this repl. server general info
					Protocol.mmp_mngt_TdmfAgentConfigMsg_t config = new Protocol.mmp_mngt_TdmfAgentConfigMsg_t();
					config.header.mngtstatus  = Protocol.eStatus.OK;//assuming success
					Management.ftd_mngt_gather_server_info( config.data );
					Protocol.SendAgentConfigMsg( aliveSocket, config, Protocol.eManagementCommands.AGENT_INFO );
			
					Protocol.mmp_TdmfRegistrationKey key = new Protocol.mmp_TdmfRegistrationKey();
					key.RegKey = RegistryAccess.Licence();
					key.iKeyExpirationTime = Management.LicenceKeyExpirationDate( key.RegKey );
					Protocol.SendRegistrationKey ( aliveSocket, key );

					// now, update Collector with latest repl. group stats and state
					Management.ftd_mngt_performance_send_all_to_Collector();

					Management.ftd_mngt_UpdateAliveMsgTime();
				} catch ( Exception ) {
					aliveSocket.Close();
				}
			}
			else {
				// ok, socket still connected 
				if ( (DateTime.Now - Management.m_LastAliveMsgTime) >= new TimeSpan ( 0, 0, ALIVE_TIMEOUT ) ) {
					// time to send a Alive msg to Collector
					// send the MMP_MNGT_AGENT_ALIVE_SOCKET msg
					try {
						Protocol.SendCollectorAlive( aliveSocket, String.Empty, new UniqueHostID() ); 
					} catch ( Exception ) {
						aliveSocket.Close();
					}
					Management.ftd_mngt_UpdateAliveMsgTime();
				}
			}
		}


		/// <summary>
		/// Send all status message in gStatusMsgList to Collector.
		/// List is then cleared of all messages.
		/// </summary>
		public static void ftd_mngt_msgs_send_status_msg_to_collector() {
			// send all status msg in list
			m_AutoEvents[0].Set();
		}



	}
}
