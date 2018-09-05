using System;
using System.Threading;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;

namespace MasterThread
{
	/// <summary>
	/// Summary description for Protocol.
	/// </summary>
	public class Protocol {


		// timeout values
		public const int RECVTIMEOUT = 30000;		// 30 seconds
		public const int SENDTIMEOUT = 30000;		// 30 seconds

		public const int LG_NET_NOT_READABLE = -1;

		public Protocol() {
		}


		/// <summary>
		/// set all of the socket options needed to send and receive the protocol on this socket
		/// </summary>
		/// <returns></returns>
		public static void InitProtocolSocket( Socket socket ) {
			socket.SetSocketOption( SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout, RECVTIMEOUT );
			socket.SetSocketOption( SocketOptionLevel.Socket, SocketOptionName.SendTimeout, SENDTIMEOUT );
			socket.SetSocketOption( SocketOptionLevel.Socket, SocketOptionName.KeepAlive, 1 );
		}




		/// <summary>
		/// Send the header to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static int SendHeader( Socket socket, ProtocolHeader header_ ) {
			header_.magicvalue = eMagic.MAGICHDR;
			int sentCount = 0;
			try {
				IntPtr ptr = Marshal.AllocHGlobal( Marshal.SizeOf( header_ ) );
				try {
					header_byte header = new header_byte();
					Marshal.StructureToPtr( header_, ptr, true );
					header = (header_byte)Marshal.PtrToStructure( ptr, typeof(header_byte) );

					sentCount = socket.Send( header.byteArray, header.byteArray.Length, SocketFlags.None );
					if ( sentCount != header.byteArray.Length ) {
						// did not send enough??
						throw new OmException ( "Error: Protocol SendHeader did not send enough data." );
					}
				} finally {
					Marshal.FreeHGlobal( ptr );
				}
			}
			finally {
			}
			return sentCount;
		}

		/// <summary>
		/// Send the header to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendHeader( TcpClient tcpClient, ProtocolHeader header_ ) {
			header_.magicvalue = eMagic.MAGICHDR;
			NetworkStream netStream = tcpClient.GetStream ();
			try {
				IntPtr ptr = IntPtr.Zero;
				header_byte header = new header_byte();
				Marshal.StructureToPtr( header_, ptr, true );
				Marshal.PtrToStructure( ptr, header );

				netStream.Write ( header.byteArray, 0, header.byteArray.Length );
			}
			finally {
				// Close the network stream.
				netStream.Close ();
			}
		}


		//		/// <summary>
		//		/// Send the ftd_err_t to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static int SendError( Socket socket, Error_u error ) {
		//			int sentCount = 0;
		//			try {
		//				sentCount = socket.Send( error.byteArray, error.byteArray.Length, SocketFlags.None );
		//				if ( sentCount != error.byteArray.Length ) {
		//					// did not send enough??
		//					throw new OmException ( "Error: Protocol SendError did not send enough data." );
		//				}
		//			}
		//			finally {
		//			}
		//			return sentCount;
		//		}

		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref ProtocolHeader header ) {
			try {
				// get all of the available data
				byte [] bytes = new byte [ Marshal.SizeOf( header ) ];
				int len = socket.Receive ( bytes, bytes.Length, SocketFlags.None );
				if ( len != bytes.Length ) {
					// did not receive enough??
					throw new OmException ( "Error: Protocol ReceiveHeader did not receive enough data." );
				}
				fixed ( void * pntr = &bytes[0] ) {
					header = (ProtocolHeader)Marshal.PtrToStructure( (IntPtr)pntr, typeof ( ProtocolHeader ) );
				}
			}
			finally {

			}
		}


		/// <summary>
		/// Receive AckCLI header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public static void ReceiveAckCLI( Socket socket, ref ProtocolHeader ack ) {
			// receive the ack
			ReceiveHeader( socket, ref ack );
			if ( ack.msgtype != eProtocol.ACKCLI ) {
				// did not receive the right response??
				throw new OmException ( "Error: Protocol ReceiveAckCLI did not receive AckCLI, received: " + ack.msgtype );
			}
		}

		/// <summary>
		/// Send the AckCLI to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendAckCLI( Socket socket, ProtocolHeader OldHeader, int data ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.ACKCLI;
			header.msg.data = data;
			SendHeader( socket, header );
		}

		//		/// <summary>
		//		/// Send the Noop to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendNOOP( Socket socket, header_t OldHeader ) {
		//			header_t header = new header_t();
		//			header.msgtype = eProtocol.CNOOP;
		//			SendHeader( socket, header );
		//		}

		/// <summary>
		/// Send the Noop to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendNOOP( Socket socket, ProtocolHeader OldHeader, int data ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.CNOOP;
			header.msg.data = data;
			SendHeader( socket, header );
		}


		/// <summary>
		/// Send the AckErr to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendACKERR( Socket socket, ProtocolHeader OldHeader ) {
			SendACKERR( socket, OldHeader, 0 );
		}

		/// <summary>
		/// Send the AckErr to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendACKERR( Socket socket, ProtocolHeader OldHeader, int ErrorLength ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.ACKERR;
			header.len = ErrorLength;
			SendHeader( socket, header );
		}

		//		/// <summary>
		//		/// Send the ACKNOPMD to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendACKNOPMD( Socket socket, ProtocolHeader OldHeader ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.ACKNOPMD;
		//			SendHeader( socket, header );
		//		}
		/// <summary>
		/// Send the ACKDOCPON to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendACKDOCPON( Socket socket, ProtocolHeader OldHeader ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.ACKDOCPON;
			SendHeader( socket, header );
		}
		/// <summary>
		/// Send the ACKDOCPOFF to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendACKDOCPOFF( Socket socket, ProtocolHeader OldHeader ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.ACKDOCPOFF;
			SendHeader( socket, header );
		}

		/// <summary>
		/// Send the FTDACKCPON to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendACKCPON( Socket socket, ProtocolHeader OldHeader ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.ACKCPON;
			SendHeader( socket, header );
		}

		//		/// <summary>
		//		/// Send the FTDACKCPSTART to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendACKCPSTART( Socket socket, ProtocolHeader OldHeader ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.ACKCPSTART;
		//			SendHeader( socket, header );
		//		}


		/// <summary>
		/// Send the CCPON to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendCCPON( Socket socket, ProtocolHeader OldHeader, int cli, int lgnum, eRole role ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.CCPON;
			header.cli = cli;
			header.msg.lgnum = lgnum;
			header.msg.data = (int)role; 
			SendHeader( socket, header );
		}

		//		/// <summary>
		//		/// Send the FTDCCPONERR to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendCCPONERR( Socket socket, ProtocolHeader OldHeader ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CCPONERR;
		//
		//			SendHeader( socket, header );
		//		}

		//		/// <summary>
		//		/// Send the FTDCSETTRACELEVELACK to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendCSETTRACELEVELACK( Socket socket, ProtocolHeader OldHeader ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CSETTRACELEVELACK;
		//			SendHeader( socket, header );
		//		}

		/// <summary>
		/// Send the FTDCREMDRIVEERR to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void SendCREMDRIVEERR( Socket socket, ProtocolHeader OldHeader, int data, int flags ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.CREMDRIVEERR;
			header.msg.data = data;
			header.msg.flag = flags;
			SendHeader( socket, header );
		}

		//		/// <summary>
		//		/// Send the FTDACKKILL to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendACKKILL( Socket socket )
		//		{
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.ACKKILL;
		//			SendHeader( socket, header );
		//		}


		/// <summary>
		/// Wait for the socket to disconnect or timeout then close the socket
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static void WaitForDisconnectAndClose( Socket socket ) {
			DateTime startTime = DateTime.Now;
			TimeSpan timeout = new TimeSpan( 0, 0, 30 ); // 30 sedconds

			if ( socket != null ) {
				do {
					if ( !socket.Connected ) {
						break;
					}
				} while ( DateTime.Now - startTime < timeout );
				socket.Close();
			}
		}

		
		/// <summary>
		/// send a start apply msg to master
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static int SendStartApply( Socket socket, int lgnum, int cpon ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = eProtocol.CSTARTAPPLY;
			header.ackwanted = 1;
			header.cli = 1;
			header.msg.lgnum = lgnum;
			header.msg.data = cpon;

			// the start apply to the master
			SendHeader( socket, header );
		
			// wait for ACK from master
			ProtocolHeader recvheader = new ProtocolHeader();
			ReceiveHeader( socket, ref recvheader );

			return header.msg.data;  // 1 = already running
		}

		//		/// <summary>
		//		/// send a start PMD msg to master
		//		/// </summary>
		//		public static int SendStartPMD( Socket socket, int lgnum, eLgModes data ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CSTARTPMD;
		//			header.ackwanted = 1;
		//			header.cli = 1;
		//			header.msg.lgnum = lgnum;
		//			header.msg.data = (int)data;
		//
		//			// the start apply to the master
		//			SendHeader( socket, header );
		//		
		//			// wait for ACKCLI from master
		//
		//			ProtocolHeader recvheader = new ProtocolHeader();
		//			ReceiveAckCLI( socket, ref recvheader );
		//
		//			return recvheader.msg.data;  // 1 = already running
		//		}

		//		/// <summary>
		//		/// send a start reco msg to master  
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="lgnum"></param>
		//		/// <param name="force"></param>
		//		/// <returns></returns>
		//		public static bool SendStartRECO( Socket socket, int lgnum, bool force ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CSTARTRECO;
		//			header.ackwanted = 1;
		//			header.cli = 1;
		//			header.msg.lgnum = lgnum;
		//			header.msg.data = force ? 1 : 0;
		//
		//			// the start apply to the master
		//			SendHeader( socket, header );
		//		
		//			// wait for ACKCLI from master
		//
		//			ProtocolHeader recvheader = new ProtocolHeader();
		//			ReceiveAckCLI( socket, ref recvheader );
		//
		//			if ( recvheader.msg.data < 0 ) {
		//				throw new OmException( "Start RECO command returned failed." );
		//			}
		//			return recvheader.msg.data == 1;  // 1 = group apply already running, -1 = error
		//		}
  
		/// <summary>
		/// send a start CP Primary msg to master  
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public static eProtocol SendCCPStartP( Socket socket, int lgnum ) {
			return SendCCP( socket, eProtocol.CCPSTARTP, lgnum );
		}
		/// <summary>
		/// send a start CP Secondary msg to master  
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public static eProtocol SendCCPStartS( Socket socket, int lgnum ) {
			return SendCCP( socket, eProtocol.CCPSTARTS, lgnum );
		}
		/// <summary>
		/// send a stop CP Primary msg to master  
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public static eProtocol SendCCPStopP( Socket socket, int lgnum ) {
			return SendCCP( socket, eProtocol.CCPSTOPP, lgnum );
		}
		/// <summary>
		/// send a stop CP Secondary msg to master  
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public static eProtocol SendCCPStopS( Socket socket, int lgnum ) {
			return SendCCP( socket, eProtocol.CCPSTOPS, lgnum );
		}
		/// <summary>
		/// send a start CP Primary msg to master  
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public static eProtocol SendCCP( Socket socket, eProtocol type, int lgnum ) {
			ProtocolHeader header = new ProtocolHeader();
			header.msgtype = type;
			header.ackwanted = 1;
			header.cli = 1;
			header.msg.lgnum = lgnum;

			// the header to the master
			SendHeader( socket, header );

			// wait for ACK from master
			ProtocolHeader recvheader = new ProtocolHeader();
			ReceiveAckCLI( socket, ref recvheader );

			return (eProtocol)recvheader.msgtype;
		}

		/// <summary>
		/// check if the local host is the target host
		/// </summary>
		/// <returns></returns>
		public static bool IsThisHost( string thostname ) {
			// get the host name for the local host
			ArrayList lnames = new ArrayList();
			// add the name localhost
			lnames.Add( "localhost" );
			IPHostEntry hostInfo = Dns.Resolve( "localhost" );
			// Get the IP address list that resolves to the host name
			foreach ( IPAddress address in hostInfo.AddressList ) {
				lnames.Add( address.ToString() );
			}
			foreach ( string alias in hostInfo.Aliases ) {
				lnames.Add( alias );
			}
			lnames.Add(  hostInfo.HostName );
			// get the ip addresses for this host name
			hostInfo = Dns.Resolve( hostInfo.HostName );
			// Get the IP address list that resolves to the host
			foreach ( IPAddress address in hostInfo.AddressList ) {
				lnames.Add( address.ToString() );
			}
			foreach ( string alias in hostInfo.Aliases ) {
				lnames.Add( alias );
			}

			// build the target address list
			ArrayList tnames = new ArrayList();
			// add the name localhost
			tnames.Add( thostname );
			hostInfo = Dns.Resolve( thostname );
			// Get the IP address list that resolves to the host name
			foreach ( IPAddress address in hostInfo.AddressList ) {
				tnames.Add( address.ToString() );
			}
			foreach ( string alias in hostInfo.Aliases ) {
				tnames.Add( alias );
			}

			// check if same host
			foreach ( string local in lnames ) {
				foreach ( string target in tnames ){
					if ( String.Compare( local, target, true ) == 0 ) {
						// then this is the local host
						return true;
					}
				}
			}
			return false;
		}


		//		/// <summary>
		//		/// send a kill back fresh FTDCSIGUSR1 to master  
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="lgnum"></param>
		//		/// <returns></returns>
		//		public static eProtocol SendCSIGUSR1( Socket socket, string name ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CSIGUSR1;
		//			header.ackwanted = 1;
		//			header.cli = 1;
		//
		//			header.msg.DataString = name;
		//
		//			ProtocolHeader recvheader;
		//			int retry = 0;
		//			do {
		//				// the header to the master
		//				SendHeader( socket, header );
		//
		//				// wait for ACK from master
		//				recvheader = new ProtocolHeader();
		//				ReceiveHeader( socket, ref recvheader );
		//
		//				if ( recvheader.msgtype != eProtocol.ACKERR ) {
		//					break;
		//				}
		//			} while ( retry++ < 3 );
		//
		//			return (eProtocol)recvheader.msg.data;
		//		}

		//		/// <summary>
		//		/// send a kill PMD FTDCSIGTERM to master  
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="lgnum"></param>
		//		/// <returns></returns>
		//		public static eProtocol SendCSIGTERM( Socket socket, string name ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CSIGTERM;
		//			header.ackwanted = 1;
		//			header.cli = 1;
		//
		//			header.msg.DataString = name;
		//
		//			ProtocolHeader recvheader;
		//			int retry = 0;
		//			do {
		//				// the header to the master
		//				SendHeader( socket, header );
		//
		//				// wait for ACK from master
		//				recvheader = new ProtocolHeader();
		//				ReceiveHeader( socket, ref recvheader );
		//
		//				if ( recvheader.msgtype != eProtocol.ACKERR ) {
		//					break;
		//				}
		//			} while ( retry++ < 3 );
		//
		//			return (eProtocol)recvheader.msg.data;
		//		}


		public static Socket IPCConnect() {
			// get the master thread port number
			int PortNumber = RegistryAccess.MasterPort();

			// create an ipc socket and connect it to the local master
			Socket MasterSocket = new Socket( AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp );
			// init the socket for protocol send and recv
			Protocol.InitProtocolSocket( MasterSocket );

			IPAddress hostAddress = IPAddress.Parse("127.0.0.1");	 //localhost
			IPEndPoint hostEndPoint = new IPEndPoint( hostAddress, PortNumber );

			// connect the ipc socket object
			MasterSocket.Connect(hostEndPoint);

			if ( !MasterSocket.Connected ) {
				// Connection failed
				string msg = String.Format( "Error: Unable to connect to master thread." );
				OmException e = new OmException( msg );
				throw e;
			}
			return MasterSocket;
		}


		/// <summary>
		/// get_connect_type -- read first four bytes from connection to determine connection request type
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <returns></returns>
		public unsafe static eConnectionTypes GetConnectionType( Socket socket, out ProtocolHeader header ) {
			int len;
			header = new ProtocolHeader();

			try {
				int retry = 0;
			again:
				// - check if data sent on connection?
				if ( socket.Available < 4 ) {
					if ( retry++ < 10 ) {
						Thread.Sleep( 20 );
						goto again;
					}
					// invalid connection no data
					return eConnectionTypes.CON_INVALID;
				}

				// read first 4 characters to see if this is a valid request 
				// this is a blocking read
				//IntPtr ptr = Marshal.AllocHGlobal( Marshal.SizeOf( header ) );
				byte [] bytes = new byte [ Marshal.SizeOf( header ) ];

				len = socket.Receive( bytes, 4, SocketFlags.None );
				if ( len < 4 ) {
					// connection error 
					return eConnectionTypes.CON_INVALID;
				}
				header_byte header_b = new header_byte();
				fixed ( void * pntr = &bytes[0] ) {
					//ptr = (IntPtr)(pntr);
					//Marshal.StructureToPtr( pntr, ptr, true );
					Marshal.PtrToStructure( (IntPtr)pntr, header_b );
				}
				if ( String.Compare ( header_b.ToString( 4 ), 0, "ftd ", 0, 4, true ) == 0 ) {
					// this is a string message and not a valid header
					fixed ( void * pntr = &bytes[0] ) {
						//Marshal.StructureToPtr( bytes, ptr, true );
						header = (ProtocolHeader)Marshal.PtrToStructure( (IntPtr)pntr, typeof ( ProtocolHeader ) );
					}
					return eConnectionTypes.CON_UTIL;
				}

				// get rest of protocol header
				len = socket.Receive( bytes, 4, bytes.Length - 4, SocketFlags.None );

				if (len != bytes.Length - 4 ) {
					return eConnectionTypes.CON_INVALID;
				}
				fixed ( void * pntr = &bytes[0] ) {
					//Marshal.StructureToPtr( bytes, ptr, true );
					header = (ProtocolHeader)Marshal.PtrToStructure( (IntPtr)pntr, typeof ( ProtocolHeader ) );
				}
				if ( header.cli == 1 ) {
					return eConnectionTypes.CON_CLI;
				} else if ( header.cli > 1 ) {
					return eConnectionTypes.CON_CHILD;
				} else {
					return eConnectionTypes.CON_PMD;
				}
			} catch( Exception e ) {
				OmException.LogException( new OmException( "Error: Get_Connection_Type caught an exception: " + e.Message, e ) );
				return eConnectionTypes.CON_INVALID;
			}
			finally {
			}
		}

		/// <summary>
		/// Receive the rest of a string message
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		/// <param name="start"></param>
		public static void ReceiveStringMessage ( Socket socket, ref header_byte header, int start ) {
			int i;
			for ( i = start ; i < header.byteArray.Length ; i++ ) {
				if ( ( socket.Receive( header.byteArray, i, 1, SocketFlags.None ) ) == 0 ) {
					throw new OmException( "Error: connection closed before sending all data" );
				}
				switch ( header.byteArray[i] ) {
					case (byte)'\n':
					case (byte)'\0':
						header.byteArray[i]= 0;
						break;
					default:
						continue;
				}
				break;
			}
		}

		//		/// <summary>
		//		/// send a set trace level FTDCSETTRACELEVEL msg to master  
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="lgnum"></param>
		//		/// <returns></returns>
		//		public static eProtocol SendCSETTRACELEVEL( Socket socket, int level ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CSETTRACELEVEL;
		//			header.ackwanted = 1;
		//			header.cli = 1;
		//			header.msg.data = level;
		//
		//			// the header to the master
		//			SendHeader( socket, header );
		//
		//			// wait for ACK from master
		//			ProtocolHeader recvheader = new ProtocolHeader();
		//			ReceiveCSETTRACELEVELACK( socket, ref recvheader );
		//
		//			return (eProtocol)recvheader.msgtype;
		//		}

		//		/// <summary>
		//		/// Receive FTDCSETTRACELEVELACK header
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="header"></param>
		//		public static void ReceiveCSETTRACELEVELACK( Socket socket, ref ProtocolHeader ack ) {
		//			// receive the ack
		//			ReceiveHeader( socket, ref ack );
		//			if ( ack.msgtype != eProtocol.CSETTRACELEVELACK ) {
		//				// did not receive the right response??
		//				throw new OmException ( "Error: Protocol ReceiveCSETTRACELEVELACK did not receive CSETTRACELEVELACK, received: " + ack.msgtype );
		//			}
		//		}
		//
		//		/// <summary>
		//		/// ftd_sock_send_err -- send err to peer
		//		/// </summary>
		//		/// <param name="socket"></param>
		//		/// <param name="msg"></param>
		//		/// <returns></returns>
		//		public static unsafe void SendErrorMessage( Socket socket, string msg ) {
		//			Error_u err = new Error_u() ;
		//			ProtocolHeader header = new ProtocolHeader();
		//
		//			SendACKERR( socket, header, err.byteArray.Length );
		//
		//			err.error.errcode = 0;
		//			err.error.length = msg.Length;
		//			err.error.msg = msg;
		//
		//			SendError( socket, err );
		//		}

		//		/// <summary>
		//		/// Send the FTDCCPOFF to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendCCPOFF( Socket socket, int pid, int lgnum, eRole role ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.CCPOFF;
		//			header.cli = pid;
		//			header.msg.lgnum = lgnum;
		//			header.msg.data = (int)role; 
		//			SendHeader( socket, header );
		//		}

		//		/// <summary>
		//		/// Send the FTDACKCPSTOP to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendACKCPSTOP( Socket socket, ProtocolHeader OldHeader ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.ACKCPSTOP;
		//			SendHeader( socket, header );
		//		}

		//		/// <summary>
		//		/// Send the FTDACKCPOFF to the client Blocking mode
		//		/// </summary>
		//		/// <param name="client"></param>
		//		/// <param name="header"></param>
		//		public static void SendACKCPOFF( Socket socket, ProtocolHeader OldHeader ) {
		//			ProtocolHeader header = new ProtocolHeader();
		//			header.msgtype = eProtocol.ACKCPOFF;
		//			SendHeader( socket, header );
		//		}




	}
}
