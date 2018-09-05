using System;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Serialization.Formatters.Binary;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections;

namespace MasterThread.Management
{
	/// <summary>
	/// Summary description for Protocol.
	/// </summary>
	public class Protocol
	{
		public static string gszServerUID;

		// collector address
		public static IPAddress giTDMFCollectorIP;
		public static int  giTDMFCollectorPort;


		// available Tdmf commands.  to be used when calling mmp_mngt_sendTdmfCommand()
		public enum eCommands {
			FIRST_TDMF_CMD = 0x2000,
			START = FIRST_TDMF_CMD,
			STOP,
			INIT,
			OVERRIDE,
			INFO,
			HOSTINFO,
			LICINFO,
			RECO,
			SET,
			LAUNCH_PMD,
			LAUNCH_REFRESH,
			LAUNCH_BACKFRESH,
			KILL_PMD,
			KILL_RMD,
			KILL_REFRESH,
			KILL_BACKFRESH,
			CHECKPOINT,
			OS_CMD_EXE, //allows to launch cmd.exe commands on a TDMF Server/Agent
			TESTTDMF,  //a simple test program
			TRACE,
			HANDLE,
			PANALYZE,  //from 2.1.6 Merge
			LAST_TDMF_CMD = PANALYZE,
			INVALID_TDMF_CMD,	//beyond cmd list
		}






		/// <summary>
		/// MMP management header message structure
		/// this is the management header used in all mngt messages
		/// </summary>
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class Header : Bytes {
			public eMagic magicnumber;	// indicates the following is a management message
			public eManagementCommands mngttype; // enum management, the management msg type.
			public eSenderType sendertype;		// indicates this message sender : TDMF_AGENT, TDMF_SERVER, CLIENT_APP
			public eStatus mngtstatus;		// enum mngt_status. this field is used to return a response status.  
			// used when the message is a response to a mngt request.

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( Header ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)magicnumber ) ).CopyTo( bytes, 0 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)mngttype ) ).CopyTo( bytes, 4 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)sendertype ) ).CopyTo( bytes, 8 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)mngtstatus ) ).CopyTo( bytes, 12 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in Header ToBytes" );
//				}
//				magicnumber = (uint)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 0 ) );
//				mngttype = (eManagementCommands)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 4 ) );
//				sendertype = (eSenderType)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 8 ) );
//				mngtstatus = (eStatus)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 12 ) );
//			}		
		}

		public enum eManagementCommands {
			/* management msg types */  /* 0x1000 = 4096 */
			SET_LG_CONFIG = 0x1000,	// receiving one logical group configuration data (.cfg file)
			SET_CONFIG_STATUS,		// receiving configuration data
			GET_LG_CONFIG,			// request to send one logical group configuration data (.cfg file)
			AGENT_INFO_REQUEST,		// request for host information: IP, listener socket port, ...
			AGENT_INFO,				// TDMF Agent response to a MMP_MNGT_AGENT_INFO_REQUEST message
			// OR
			// SET TDMF Agent general information (Server info)
			REGISTRATION_KEY,		// SET/GET registration key
			TDMF_CMD,				// management cmd, specified by a tdmf_commands sub-cmd
			CMD_STATUS,				// response to management cmd
			SET_AGENT_GEN_CONFIG,	// SET TDMF Agent general config. parameters
			GET_AGENT_GEN_CONFIG,	// GET TDMF Agent general config. parameters
			GET_ALL_DEVICES,		// GET list of devices (disks/volumes) present on TDMF Agent system
			SET_ALL_DEVICES,		// response to a MMP_MNGT_GET_ALL_DEVICES request message.
			ALERT_DATA,				// TDMF Agent sends this message to TDMF Collector to report an alert condition
			STATUS_MSG,
			PERF_MSG,				// TDMF Agent periodically sends performance data to TDMF Collector
			PERF_CFG_MSG,			// various performance counters and pacing values
			MONITORING_DATA_REGISTRATION,	// Request the TDMF Collector to be notified for any TDMF Agents status/mode/performance data changes.
			AGENT_ALIVE_SOCKET,		// Agent opens a socket on Collector and sends this msg.  Both peers keep socket connected at all times.  Used by Collector for Agent detection.
			AGENT_STATE,			// Agent sends this msg to report about its various states to Collector.
			GROUP_STATE,			// TDMF Agent periodically sends group states to TDMF Collector
			TDMFCOMMONGUI_REGISTRATION,	// TDMF Common GUI sends this msg periodically as a watchdog to keep or claim ownership over the Collector and TDMF DB.
			GROUP_MONITORING,		// TDMF Agent sends various rela-time group information to Collector
			SET_DB_PARAMS,			// TDMF Common GUI sends this msg to set various parameters related to the Tdmf DB
			GET_DB_PARAMS,			// TDMF Common GUI sends this msg to set various parameters related to the Tdmf DB
			AGENT_TERMINATE,		// TDMF Collector requests that an Repl. Server Agent stops running.
			GUI_MSG,				// Inter-GUI message
			COLLECTOR_STATE,		// TDMF collector sends this msg to GUI to report its state
			TDMF_SENDFILE,			// receiving a TDMF files (script or zip)
			TDMF_SENDFILE_STATUS,	// receiving TDMF files (script or zip)
			TDMF_GETFILE,			// request to send TDMF files (tdmf*.bat)
		};


		public enum eSenderType {
			AGENT = 2,
			SERVER = 0,
			CLIENTAPP = 1
		}

		public enum eFileType {
			CFG = 0,
			EXE,
			BAT,
			ZIP,
			TAR
		}

		//  This message will serve as response to a MMP_MNGT_GET_LG_CONFIG request.
		/// <summary>
		/// mmp_mngt_ConfigurationMsg_t
		/// </summary>
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_ConfigurationMsg_t : Bytes {
			//public Header hdr;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID; //name of machine identifying the TDMF Agent.
			[MarshalAs(UnmanagedType.Struct)]
			public mmp_TdmfFileTransferData data;		//cfg file is transfered 

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_ConfigurationMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte [ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 ); 
//				data.ToBytes().CopyTo( bytes, 80 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_mngt_ConfigurationMsg_t ToBytes" );
//				}
//				szServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0, 80 );
//				data = new mmp_TdmfFileTransferData();
//				byte [] b = new byte[ 264 ];
//				Array.Copy( bytes, 80, b, 0, b.Length );
//				data.FromBytes( b );
//			}		
		}

		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfFileTransferData : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szFilename;	//name of file at destination, WITHOUT path.
			public eFileType iType;			//enum tdmf_filetype
			public int iSize;		//size of following data. 
			//the file binary data must be contiguous to this structure.

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfFileTransferData ) ); }
//			} 
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szFilename ).CopyTo( bytes, 0 ); 
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iType ) ).CopyTo( bytes, 256 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iSize ) ).CopyTo( bytes, 260 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_TdmfFileTransferData ToBytes" );
//				}
//				szFilename = ASCIIEncoding.UTF8.GetString( bytes, 0, 256 );
//				iType = (eFileType)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 256 ) );
//				iSize = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 260 ) );
//			}		
		}

		//sent in response to a MMP_MNGT_SET_LG_CONFIG request.
		//indicates if configuration was updated succesfully
		/// <summary>
		/// mmp_mngt_ConfigurationStatusMsg_t
		/// </summary>
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_FileStatusMsg_t : Bytes {
			public Header header;                    
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string ServerUID;	//name of machine identifying the TDMF Agent.
			public ushort usLgId;		//logical group for which the config. set was requested
			public int iStatus;			//0 = OK, 1 = rx/tx error with TDMF Agent, 2 = err writing cfg file

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_FileStatusMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				ASCIIEncoding.UTF8.GetBytes( ServerUID ).CopyTo( bytes, 16 ); 
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (short)usLgId ) ).CopyTo( bytes, 96 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iStatus ) ).CopyTo( bytes, 98 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_mngt_ConfigurationStatusMsg_t ToBytes" );
//				}
//				header = new Header();
//				header.FromBytes( bytes );
//				ServerUID = ASCIIEncoding.UTF8.GetString( bytes, 16, 80 );
//				usLgId = (ushort)IPAddress.NetworkToHostOrder( BitConverter.ToInt16( bytes, 96 ) );
//				iStatus = (int)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 98 ) );
//			}		
		}

		[Serializable]
		public enum eStatus {
			OK = 0,    // success
			ERR_CONNECT_TDMFCOLLECTOR = 100,  // could not connect to TDMF Collector socket
			ERR_UNKNOWN_TDMFAGENT,               // specified TDMF Agent (szServerUID) is unknown to TDMF Collector
			ERR_CONNECT_TDMFAGENT,               // could not connect to TDMF Agent.
			ERR_MSG_RXTX_TDMFAGENT,              // unexpected communication rupture while rx/tx data with TDMF Agent
			ERR_MSG_RXTX_TDMFCOLLECTOR,          // unexpected communication rupture while rx/tx data with TDMF Collector
			ERR_SET_AGENT_GEN_CONFIG,            // Agent could not save the received Agent config.  Continuing using current config.
			ERR_INVALID_SET_AGENT_GEN_CONFIG,    // provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.
			ERR_COLLECTOR_INTERNAL_ERROR,        // basic functionality failed ...
			ERR_BAD_OR_MISSING_REGISTRATION_KEY, 
			ERR_ERROR_ACCESSING_TDMF_DB         //109
		};


		public Protocol()
		{
		}

		/// <summary>
		/// Only return IPAddress no alias names
		/// </summary>
		/// <param name="host"></param>
		/// <returns></returns>
		public static string[] GetIPAddresses( string host ) {
			// get the host address
			ArrayList addresses = new ArrayList();
			IPHostEntry hostInfo = Dns.Resolve( host );
			// Get the IP address list that resolves to the host name
			foreach ( IPAddress address in hostInfo.AddressList ) {
				if ( !addresses.Contains( address.ToString() ) ) {
					if ( address.ToString() != "127.0.0.1" ) {
						addresses.Add( address.ToString() );
					}
				}
			}
			// get the ip addresses for this host name
			hostInfo = Dns.Resolve( hostInfo.HostName );
			// Get the IP address list that resolves to the host
			foreach ( IPAddress address in hostInfo.AddressList ) {
				if ( !addresses.Contains( address.ToString() ) ) {
					if ( address.ToString() != "127.0.0.1" ) {
						addresses.Add( address.ToString() );
					}
				}
			}
			return (string[])addresses.ToArray( typeof (String) );
		}


		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref Header header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
			//validate
			if ( header.magicnumber != eMagic.MSG_MAGICNUMBER ) {
				throw new OmException( "Error: ReceiveHeader Invalid magic number in header" );
			}
			if ( header.sendertype != eSenderType.SERVER ) {
				throw new OmException( "Error: ReceiveHeader Invalid magic number in header" );
			}
		}

		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_mngt_ConfigurationMsg_t header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}

		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_TdmfRegistrationKey header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}
		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveData( Socket socket, ref byte [] bytes ) {
			DateTime startTime = DateTime.Now;
			TimeSpan TimeOut = new TimeSpan( 0,0,30 );
			// get all of the available data
			while ( socket.Available < bytes.Length ) {
				if (( DateTime.Now - startTime ) >= TimeOut ) {
					throw new OmException( "Error: ReceiveData Header Timeout" );
				}
				Thread.Sleep( 1 );
			}
			int len = socket.Receive ( bytes, bytes.Length, SocketFlags.None );
			if ( len != bytes.Length ) {
				// did not receive enough??
				throw new OmException ( "Error: Protocol ReceiveData did not receive enough data." );
			}

		}
		public static void SendSetConfigStatus( Socket socket, mmp_mngt_ConfigurationMsg_t RcvCfgData, int status ) {
			SendSetFileStatus( socket, RcvCfgData, status, eManagementCommands.SET_CONFIG_STATUS, Convert.ToUInt16( RcvCfgData.data.szFilename.Substring( 1, 3 ) ) );
		}

		public static void SendSetFileStatus( Socket socket, mmp_mngt_ConfigurationMsg_t RcvCfgData, int status ) {
			SendSetFileStatus( socket, RcvCfgData, status, eManagementCommands.TDMF_SENDFILE_STATUS, 0 );
		}

		public static void SendSetFileStatus( Socket socket, mmp_mngt_ConfigurationMsg_t RcvCfgData, int status, eManagementCommands cmd, ushort Lgnum ) {
			mmp_mngt_FileStatusMsg_t response = new mmp_mngt_FileStatusMsg_t();

			//p repare status message 
			response.header.magicnumber = (eMagic)IPAddress.HostToNetworkOrder( (int)eMagic.MSG_MAGICNUMBER );
			response.header.mngttype    = (eManagementCommands)IPAddress.HostToNetworkOrder( (int)cmd );
			response.header.sendertype  = (eSenderType)IPAddress.HostToNetworkOrder( (int)eSenderType.AGENT );
			response.header.mngtstatus  = (eStatus)IPAddress.HostToNetworkOrder( (int)eStatus.OK ); 
			// convert to network byte order
			response.usLgId    = (ushort)IPAddress.HostToNetworkOrder( (short)Lgnum );
			response.iStatus   = IPAddress.HostToNetworkOrder( status );
			response.ServerUID = RcvCfgData.szServerUID;

			// send status message to requester
			byte [] data = response.ToBytes();
			SendData( socket, data );
		}

		public static void SendRegistrationKey(  Socket socket, mmp_TdmfRegistrationKey data ) {
			mmp_mngt_RegistrationKeyMsg_t response = new mmp_mngt_RegistrationKeyMsg_t();

			//send registration key to requester
			response.header.magicnumber = eMagic.MSG_MAGICNUMBER;
			response.header.mngttype    = eManagementCommands.REGISTRATION_KEY;
			response.header.sendertype  = eSenderType.AGENT;
			response.header.mngtstatus  = eStatus.OK;
			response.keydata = data;
			//send status message to requester
			SendData( socket, response.ToBytes() );
			//SendData( socket, data.ToBytes() );
		}

		/// <summary>
		/// Send the header to the client Blocking mode
		/// </summary>
		/// <param name="client"></param>
		/// <param name="header"></param>
		public static int SendData( Socket socket, byte [] data ) {
			int sentCount = 0;
			sentCount = socket.Send( data, data.Length, SocketFlags.None );
			if ( sentCount != data.Length ) {
				// did not send enough??
				throw new OmException ( "Error: Protocol SendData did not send enough data." );
			}
			return sentCount;
		}

		public static void mmp_mngt_recv_cfg_data( Socket socket, out mmp_mngt_ConfigurationMsg_t RcvCfgData, out byte [] data ) {
			//reads other mmp_mngt_ConfigurationMsg_t fields
			RcvCfgData = new mmp_mngt_ConfigurationMsg_t();
			ReceiveHeader( socket, ref RcvCfgData );

			// read File data
			data = null;
			if ( RcvCfgData.data.iSize > 0 ) {
				data = new byte[ RcvCfgData.data.iSize ];
				ReceiveData( socket, ref data );
			}
		}


		public const string HOST_ID_PREFIX = "HostID=";
 
		/// <summary>
		/// Builds a mmp_mngt_ConfigurationMsg_t message ready to be sent on socket
		/// (host to network convertions done) pFileData can be NULL; uiDataSize can be 0.
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="AgentId"></param>
		/// <param name="SenderType"></param>
		/// <param name="type"></param>
		/// <param name="CfgFileName"></param>
		/// <param name="data"></param>
		public static void mmp_mngt_build_SetConfigurationMsg( Socket socket,
			string AgentId,
			eSenderType SenderType,
			eManagementCommands type,
			string CfgFileName,
			byte [] data
			 )
		{
			//build contiguous buffer for message 
			mmp_mngt_ConfigurationMsg_t msg = new mmp_mngt_ConfigurationMsg_t();
			byte [] bytes = new byte [ msg.Size + data.Length ];

			//build msg header
			Header header = new Header();
			header.magicnumber = eMagic.MSG_MAGICNUMBER;
			header.sendertype = SenderType;
			header.mngttype    = type;
			header.mngtstatus  = eStatus.OK;
    
			msg.szServerUID = HOST_ID_PREFIX + AgentId;
            msg.data.szFilename = CfgFileName;
			msg.data.iType  = eFileType.CFG;
			msg.data.iSize = data.Length;
    
			// append all of the data into 1 byte array
			header.ToBytes().CopyTo( bytes, 0 );
			msg.ToBytes().CopyTo( bytes, header.Size );
			data.CopyTo( bytes, header.Size + msg.Size );

			SendData ( socket, bytes );
		}

		//MMP_MNGT_REGISTRATION_KEY 
		//When sender provides a non-empty keydata.szRegKey , it is a SET key message. 
		//When sender provides an empty keydata.szRegKey ("" or "\0"), it is a GET key message. 
		//For a SET key, the same message is used as response. Upon success, the szRegKey member 
		//will contain the same key value.  On failure the szRegKey member returned will be an empty string.
		//For a GET key, the same message is used as response. Upon success, the szRegKey member 
		//will contain the key value.  On failure the szRegKey member returned will be an empty string.
		public class mmp_mngt_RegistrationKeyMsg_t : Bytes {
			public Header header;
			public mmp_TdmfRegistrationKey keydata;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_RegistrationKeyMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				keydata.ToBytes().CopyTo( bytes, header.Size );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_TdmfRegistrationKey ToBytes" );
//				}
//				header = new Header();
//				header.FromBytes( bytes );
//				keydata = new mmp_TdmfRegistrationKey();
//				keydata.FromBytes( bytes, header.Size );
//			}		
		}
		
		//MMP_MNGT_AGENT_INFO_REQUEST
		//sent by client app to TDMF Collector or by TDMF Collector to TDMF Agent
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfRegistrationKey : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string ServerUID;        //name of machine identifying the TDMF Agent.
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=40)]
			public string RegKey;         
			public int iKeyExpirationTime;	//returned by a MMP_MNGT_REGISTRATION_KEY GET key cmd. 
			//If > 0, it has to be a time_t value representing the key expiration date/time.
			//If <= 0, decode as follow:
			//	KEY_GENERICERR          0
			//	KEY_NULL               -1
			//	KEY_EMPTY              -2
			//	KEY_BADCHECKSUM        -3
			//	KEY_EXPIRED            -4
			//	KEY_WRONGHOST          -5
			//	KEY_BADSITELIC         -6
			//	KEY_WRONGMACHINETYPE   -7
			//	KEY_BADFEATUREMASK     -8

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfRegistrationKey ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( ServerUID ).CopyTo( bytes, 0 ); 
//				ASCIIEncoding.UTF8.GetBytes( RegKey ).CopyTo( bytes, 80 ); 
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iKeyExpirationTime ) ).CopyTo( bytes, 80 + 40 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				FromBytes( bytes, 0 );
//			}				
//			public void FromBytes ( byte [] bytes, int index ) {
//				if ( bytes.Length >= (Size + index) ) {
//					throw new OmException("Error: Invalid byte count in mmp_TdmfRegistrationKey ToBytes" );
//				}
//				ServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0 + index, 80 );
//				RegKey = ASCIIEncoding.UTF8.GetString( bytes, 80 + index, 40 );
//				iKeyExpirationTime = (int)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80 + 40 + index ) );
//			}
		}

		//MMP_MNGT_AGENT_INFO
		//  Sent by TDMF Agent to TDMF Collector to provide TDMF parameters
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfAgentConfigMsg_t : Bytes {
			public Header header;
			public mmp_TdmfServerInfo data;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfAgentConfigMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_mngt_TdmfAgentConfigMsg_t ToBytes" );
//				}
//				header = new Header();
//				header.FromBytes( bytes );
//				data = new mmp_TdmfServerInfo();
//				data.FromBytes( bytes, header.Size );
//			}		
		}

		//*****************************************************************************
		// Structures definitions
		//*****************************************************************************
		//MMP_MNGT_SET_GEN_CONFIG           // SET TDMF Agent general config. parameters
		//MMP_MNGT_GET_GEN_CONFIG           // GET TDMF Agent general config. parameters
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfServerInfo : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;    //tdmf unique host id 
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=20)]
			public string [] szIPAgent = new string[4];//dotted decimal format
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=20)]
			public string szIPRouter;           //dotted decimal format, can be "0.0.0.0" if no router 
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szTDMFDomain;    //name of TDMF domain 
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szMachineName;  //name of machine where this TDMF Agent runs.
			public int iPort;                      //port nbr exposed by this TDMF Agent
			public int iTCPWindowSize;             //KiloBytes
			public int iBABSizeReq;                //MegaBytes : requested BAB size
			public int iBABSizeAct;                //MegaBytes : actual BAB size
			public int iNbrCPU;
			public int iRAMSize;                   //KiloBytes
			public int iAvailableRAMSize;          //KiloBytes
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=30)]
			public string szOsType;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=50)]
			public string szOsVersion;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=40)]
			public string szTdmfVersion;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szTdmfPath;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szTdmfPStorePath;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szTdmfJournalPath;
			public byte ucNbrIP;                    //nbr of valid szIPAgent strings
	
//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfServerInfo ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 ); 
//				ASCIIEncoding.UTF8.GetBytes( szIPAgent[0] ).CopyTo( bytes, 80 ); 
//				ASCIIEncoding.UTF8.GetBytes( szIPAgent[1] ).CopyTo( bytes, 80+20 ); 
//				ASCIIEncoding.UTF8.GetBytes( szIPAgent[2] ).CopyTo( bytes, 80+40 ); 
//				ASCIIEncoding.UTF8.GetBytes( szIPAgent[3] ).CopyTo( bytes, 80+60 ); 
//				ASCIIEncoding.UTF8.GetBytes( szIPRouter ).CopyTo( bytes, 80+(4*20) ); 
//				ASCIIEncoding.UTF8.GetBytes( szTDMFDomain ).CopyTo( bytes, 80+(4*20)+20 ); 
//				ASCIIEncoding.UTF8.GetBytes( szMachineName ).CopyTo( bytes, 80+(4*20)+20+80 ); 
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iPort ) ).CopyTo( bytes, 80+(4*20)+20+80+80 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iTCPWindowSize ) ).CopyTo( bytes, 80+(4*20)+20+80+80+4 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iBABSizeReq ) ).CopyTo( bytes, 80+(4*20)+20+80+80+8 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iBABSizeAct ) ).CopyTo( bytes, 80+(4*20)+20+80+80+12 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iNbrCPU ) ).CopyTo( bytes, 80+(4*20)+20+80+80+16 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iRAMSize ) ).CopyTo( bytes, 80+(4*20)+20+80+80+20 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iAvailableRAMSize ) ).CopyTo( bytes, 80+(4*20)+20+80+80+24 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				FromBytes( bytes, 0 );
//			}				
//			public void FromBytes ( byte [] bytes, int index ) {
//				if ( bytes.Length >= (Size + index) ) {
//					throw new OmException("Error: Invalid byte count in mmp_TdmfServerInfo ToBytes" );
//				}
//				szServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0 + index, 80 );
//				szIPAgent[0] = ASCIIEncoding.UTF8.GetString( bytes, 80 + index, 20 );
//				szIPAgent[1] = ASCIIEncoding.UTF8.GetString( bytes, 80+20 + index, 20 );
//				szIPAgent[2] = ASCIIEncoding.UTF8.GetString( bytes, 80+40 + index, 20 );
//				szIPAgent[3] = ASCIIEncoding.UTF8.GetString( bytes, 80+60 + index, 20 );
//				szIPRouter = ASCIIEncoding.UTF8.GetString( bytes, 80+80 + index, 20 );
//				szTDMFDomain = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20 + index, 80 );
//				szMachineName = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80 + index, 80 );
//				iPort = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80 + index ) );
//				iTCPWindowSize = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80+4 + index ) );
//				iBABSizeReq = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80+8 + index ) );
//				iBABSizeAct = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80+12 + index ) );
//				iNbrCPU = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80+16 + index ) );
//				iRAMSize = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80+20 + index ) );
//				iAvailableRAMSize = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+80+20+80+80+24 + index ) );
//				szOsType = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80+80+28 + index, 30 );
//				szOsVersion = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80+80+28+30 + index,50 );
//				szTdmfVersion = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80+80+28+30+50 + index, 40 );
//				szTdmfPath = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80+80+28+30+50+40 + index, 256 );
//				szTdmfPStorePath = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80+80+28+30+50+40+256 + index, 256 );
//				szTdmfJournalPath = ASCIIEncoding.UTF8.GetString( bytes, 80+80+20+80+80+28+30+50+40+256+256 + index, 256 );
//			}
		}
		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_TdmfServerInfo header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}
		public static void SendAgentConfigMsg( Socket socket, mmp_mngt_TdmfAgentConfigMsg_t cmdmsg, eManagementCommands MsgType ) {
			//complete response msg and send it to requester
			cmdmsg.header.magicnumber  = eMagic.MSG_MAGICNUMBER;
			cmdmsg.header.mngttype     = MsgType;
			cmdmsg.header.sendertype   = eSenderType.AGENT;
			SendData( socket, cmdmsg.ToBytes() );
		}

		//MMP_MNGT_STATUS_MSG
		//  Sent by TDMF Agents to TDMF Collector.  Collector saves the messages to the TDMF DB.
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfStatusMsgMsg_t : Bytes {
			public Header header;
			public mmp_TdmfStatusMsg data;
			
//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfStatusMsgMsg_t ) ); }
//			}
//			public byte [] ToBytes ( string msg ) {
//				byte [] bytes = new byte[ Size + msg.Length + 1 ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				ASCIIEncoding.UTF8.GetBytes( msg ).CopyTo( bytes, Size );
//				bytes[ bytes.Length - 1 ] = 0;
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_mngt_TdmfStatusMsgMsg_t ToBytes" );
//				}
//				header = new Header();
//				header.FromBytes( bytes );
//				data = new mmp_TdmfStatusMsg();
//				data.FromBytes( bytes, header.Size );
//			}		
		}
		//MMP_MNGT_STATUS_MSG
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfStatusMsg : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;	// name of machine identifying the TDMF Agent.
			public byte cPriority;		// can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
			public byte cTag;			// one of the TAG_... value
			public Int16 pad;			// in order so that the sizeof() value be a multiple of 8
			public int iTdmfCmd;		// tdmf command to which this message is related to.  0 if irrelevant.
			public int iTimeStamp; 
			public int iLength;			// length of message string following this structure, including terminating '\0' character.
					
//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfStatusMsg ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 ); 
//				bytes[80] = cPriority;
//				bytes[80+1] = cTag;
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (Int16)pad ) ).CopyTo( bytes, 80+1+1 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iTdmfCmd ) ).CopyTo( bytes, 80+1+1+2 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iTimeStamp ) ).CopyTo( bytes, 80+1+1+2+4 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iLength ) ).CopyTo( bytes, 80+1+1+2+4+4 );
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes, int index ) {
//				if ( bytes.Length >= Size ) {
//					throw new OmException("Error: Invalid byte count in mmp_TdmfStatusMsg ToBytes" );
//				}
//				szServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0 + index, 80 );
//				cPriority = bytes[ 80 + index ];
//				cTag = bytes [ 80 + 1 + index ];
//				pad = IPAddress.NetworkToHostOrder( BitConverter.ToInt16( bytes, 80+1+1 + index ) );
//				iTdmfCmd = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+1+1+2 + index ) );
//				iTimeStamp = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+1+1+2+4 + index ) );
//				iLength = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80+1+1+2+4+4 + index ) );
//			}		
		}

		public static void SendStatusMsg(  Socket socket, StatusMessage message, UniqueHostID ID ) {
			mmp_mngt_TdmfStatusMsgMsg_t response = new mmp_mngt_TdmfStatusMsgMsg_t();
			// prepare status message 
			response.header.magicnumber = (eMagic)IPAddress.HostToNetworkOrder( (int)eMagic.MSG_MAGICNUMBER );
			response.header.mngttype    = (eManagementCommands)IPAddress.HostToNetworkOrder( (int)eManagementCommands.STATUS_MSG );
			response.header.sendertype  = (eSenderType)IPAddress.HostToNetworkOrder( (int)eSenderType.AGENT );
			response.header.mngtstatus  = (eStatus)IPAddress.HostToNetworkOrder( (int)eStatus.OK ); 

			response.data.szServerUID = ID.ToString();

			response.data.cPriority  = (byte)message.Priority;
			response.data.cTag       = 0;
			// convert to network byte order
			response.data.iTdmfCmd   = IPAddress.HostToNetworkOrder( message.TdmfCmd );
			response.data.iTimeStamp = IPAddress.HostToNetworkOrder( Convert.ToInt32( message.TimeStamp ) );

			// send status message to requester
			byte [] data = response.ToBytes( message.Message );
			SendData( socket, data );
		}


		public static Socket CollectorConnect() {
			// create an  socket and connect it to the collector
			Socket socket = new Socket( AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp );

			IPAddress hostAddress = giTDMFCollectorIP;
			IPEndPoint hostEndPoint = new IPEndPoint( hostAddress, giTDMFCollectorPort );

			// connect the collector
			socket.Connect(hostEndPoint);

			if ( !socket.Connected ) {
				// Connection failed
				string msg = String.Format( "Error: Unable to connect to collector." );
				OmException e = new OmException( msg );
				throw e;
			}
			return socket;
		}

		//MMP_MNGT_AGENT_ALIVE_SOCKET
		//  Sent once by a TDMF Agent to TDMF Collector
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfAgentAliveMsg_t : Bytes {
			public Header header;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;	// name of machine identifying the TDMF Agent.
			public int iMsgLength; //number of bytes following this message.  
			//zero-terminated text string containing tags-values pairs can be contiguous to this message.

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfAgentAliveMsg_t ) ); }
//			}
//			public byte [] ToBytes ( string msg ) {
//				byte [] bytes = new byte[ Size + msg.Length ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, header.Size );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( iMsgLength ) ).CopyTo( bytes, header.Size + 80 );
//				ASCIIEncoding.UTF8.GetBytes( msg ).CopyTo( bytes, Size );
//				bytes[ bytes.Length - 1 ] = 0;
//				return bytes;
//			}
		}

		public static string MMP_MNGT_ALIVE_TAG_MMPVER = "MMPVER=";
		public static int MMP_PROTOCOL_VERSION = 4;

		//send the MMP_MNGT_AGENT_ALIVE_SOCKET msg
		public static void SendCollectorAlive( Socket socket, string message, UniqueHostID ID ) {
			mmp_mngt_TdmfAgentAliveMsg_t response = new mmp_mngt_TdmfAgentAliveMsg_t();

			//prepare status message 
			response.header.magicnumber = (eMagic)IPAddress.HostToNetworkOrder( (int)eMagic.MSG_MAGICNUMBER );
			response.header.mngttype        = (eManagementCommands)IPAddress.HostToNetworkOrder( (int)eManagementCommands.AGENT_ALIVE_SOCKET );
			response.header.sendertype  = (eSenderType)IPAddress.HostToNetworkOrder( (int)eSenderType.AGENT );
			response.header.mngtstatus      = eStatus.OK; 
			response.header.mngtstatus  = (eStatus)IPAddress.HostToNetworkOrder( (int)eStatus.OK ); 
			response.szServerUID = ID.ToString();

			// >>> make sure szAliveMsgTagValue can eat it all up ...
			if ( message == String.Empty ) {
				response.iMsgLength = 0;
			} else {
				response.iMsgLength = message.Length + 1;
			}
			//send message to collector
			byte [] data = response.ToBytes( message );
			SendData( socket, data );
		}


		//MMP_MNGT_GET_ALL_DEVICES
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfAgentDevicesMsg_t : Bytes {
			public Header header;
			public mmp_mngt_TdmfAgentDevices_t data;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfAgentDevicesMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				return ToBytes( "" );
//			}	
//			public byte [] ToBytes ( string msg ) {
//				byte [] bytes = new byte[ Size + msg.Length ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				ASCIIEncoding.UTF8.GetBytes( msg ).CopyTo( bytes, Size ); 
//				return bytes;
//			}
		}

		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfAgentDevices_t : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;	//name of machine identifying the TDMF Agent.
			public int iNbrDevices;	//number of mmp_TdmfDeviceInfo contiguous to this structure

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfAgentDevices_t ) ); }
//			}
//			public byte [] ToBytes () {
//				return ToBytes( "" );
//			}
//			public byte [] ToBytes ( string msg ) {
//				byte [] bytes = new byte[ Size + msg.Length ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( iNbrDevices ) ).CopyTo( bytes, 80 );
//				ASCIIEncoding.UTF8.GetBytes( msg ).CopyTo( bytes, Size ); 
//				return bytes;
//			}
		}

		[Serializable]
		public class DeviceInfoCollection : CollectionBase {

			public int Add ( DeviceInfo x ) {
				if ( Contains(x) ) return -1;
				int index = InnerList.Add(x);
				return index;
			}

			public bool Contains ( DeviceInfo x ) {
				return InnerList.Contains(x);
			}

			public int IndexOf ( DeviceInfo x ) {
				return InnerList.IndexOf ( x );
			}

			public void Remove ( DeviceInfo x ) {
				InnerList.Remove(x);
			}

			public DeviceInfo this[int index] {
				get { return (DeviceInfo) InnerList[index]; }
			}
		}

		/// <summary>
		/// My device info
		/// </summary>
		/// 
		[Serializable]
		public class DeviceInfo : mmp_TdmfDeviceInfo {
			public string DeviceLetterMountPoint;
			public long StartOffset;
			public long Length;
			public string UniqueId;
			public string SignatureUniqueId;
			// Added for new code 2.2
			public ulong FreeSpace;
		}


		//MMP_MNGT_GET_ALL_DEVICES
		[Serializable]
//		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto, Pack=1)]
		public class mmp_TdmfDeviceInfo : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szDrivePath;			// drive / path identifying the drive/volume. 
			public short sFileSystem;			// MMP_MNGT_FS_... values     
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=24)]
			public string szDriveId;			// volume id, 64 bit integer value in text format. eg: "1234567890987654"
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=24)]
			public string szStartOffset;		// volume starting offset, 64 bit integer value. eg: "1234567890987654"
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=24)]
			public string szLength;				// volume length, 64 bit integer value. eg: "1234567890987654"

			/// <summary>
			/// used by the console
			/// </summary>
			public long DeviceSize {
				get {
					long size = long.MaxValue;
					try {
						size = Convert.ToInt64( this.szLength );
					} catch( Exception ) {
						size = long.MaxValue;
					}
					return size;
				}
			}

			/// <summary>
			/// Returns the drive letter and colon ex: C: if mount point then return mount point Ex: Z:\mount1
			/// </summary>
			/// <returns></returns>
			public string DriveLetterColon() {
				if ( this.szDrivePath.Length > 3 ) {
					return this.szDrivePath;
				} else {
					return this.szDrivePath.Substring( 0, 2 );
				}
			}
			/// <summary>
			/// Returns the drive letter Ex: C if mount point then return mount point Ex: Z:\mount1
			/// </summary>
			/// <returns></returns>
			public string DriveLetter() {
				if ( this.szDrivePath.Length > 3 ) {
					return this.szDrivePath;
				} else {
					return this.szDrivePath.Substring( 0, 1 );
				}
			}

			/// <summary>
			/// Used by the console
			/// </summary>
			public string Display {
				get {
					if ( DeviceSize == long.MaxValue ) {
						return this.szDrivePath + " (Unknown)";
					}
					double SizeMB = Convert.ToDouble( DeviceSize / (1024*1024) );
					return this.szDrivePath + String.Format( " ({0:N1}MB)", SizeMB);
				}
			}

		}
		/// <summary>
		/// Receive the protocol mmp_mngt_TdmfAgentDevices_t
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_mngt_TdmfAgentDevices_t header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}
		public static void SendAgentDevices( Socket socket, DeviceInfoCollection DeviceInfos ) {
			mmp_mngt_TdmfAgentDevicesMsg_t cmdmsg = new mmp_mngt_TdmfAgentDevicesMsg_t();
			//complete response msg and send it to requester
			cmdmsg.header.magicnumber = eMagic.MSG_MAGICNUMBER;
			cmdmsg.header.mngttype    = eManagementCommands.SET_ALL_DEVICES;
			cmdmsg.header.sendertype  = eSenderType.AGENT;
			cmdmsg.header.mngtstatus  = eStatus.OK; 
			cmdmsg.data.szServerUID = "";//don't care
			cmdmsg.data.iNbrDevices    = DeviceInfos.Count;

			SendData( socket, cmdmsg.ToBytes() );

			foreach ( mmp_TdmfDeviceInfo DeviceInfo in DeviceInfos ) {
				SendData( socket, DeviceInfo.ToBytes() );
			}
		}

		//MMP_MNGT_TDMF_CMD 
		//ask a TDMF Agent to perform a specific action
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfCommandMsg_t : Bytes {
			public Header header;
			public mmp_mngt_TdmfCommand data;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfCommandMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				return bytes;
//			}
		}

		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfCommand : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;		//name of machine identifying the TDMF Agent.
			public eCommands iSubCmd;		//value from enum tdmf_commands
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szCmdOptions;		//list of options for sub-cmd, similar to a Tdmf exe cmd line

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfCommand ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 ); 
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iSubCmd ) ).CopyTo( bytes, 80 );
//				ASCIIEncoding.UTF8.GetBytes( szCmdOptions ).CopyTo( bytes, 80 + 4 ); 
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				FromBytes( bytes, 0 ); 	
//			}
//			public void FromBytes ( byte [] bytes, int index ) {
//				if ( bytes.Length >= Size + index ) {
//					throw new OmException("Error: Invalid byte count in mmp_mngt_TdmfCommand ToBytes" );
//				}
//				szServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0 + index, 80 );
//				iSubCmd = (eCommands)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80 + index ) );
//				szCmdOptions = ASCIIEncoding.UTF8.GetString( bytes, 0 + 80 + index, 256 );
//			}		
		}

		// MMP_MNGT_TDMF_CMD_STATUS, 
		// message sent by a TDMF Agent in response to a MMP_MNGT_TDMF_CMD request
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfCommandStatusMsg_t : Bytes {
			public Header header;
			public mmp_mngt_TdmfCommandStatus data;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfCommandStatusMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				return bytes;
//			}
		}
		
		public enum tdmf_commands_status {
			OK = 0,
			ERROR,
			ERR_COULD_NOT_REACH_TDMF_AGENT
		}

		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfCommandStatus : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;	// name of machine identifying the TDMF Agent.
			public eCommands iSubCmd;	// value from enum tdmf_commands
			public tdmf_commands_status iStatus;	// value from enum tdmf_commands_status
			public int iLength;			// length of string (including \0) following this structure
			// the string is the console output of the tdmf command tool performing the command.

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfCommandStatus ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 ); 
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iSubCmd ) ).CopyTo( bytes, 80 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iStatus ) ).CopyTo( bytes, 80 + 4 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( iLength ) ).CopyTo( bytes, 80 + 4 + 4 );
//				return bytes;
//			}
		}
		/// <summary>
		/// Receive the protocol mmp_mngt_TdmfAgentDevices_t
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_mngt_TdmfCommand header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}
		public static void SendAgentStatus( Socket socket, int exitcode, mmp_mngt_TdmfCommand data, string output  ) {
			mmp_mngt_TdmfCommandStatusMsg_t response = new mmp_mngt_TdmfCommandStatusMsg_t();
			//complete response msg and send it to requester
			response.header.magicnumber = eMagic.MSG_MAGICNUMBER;
			response.header.mngttype    = eManagementCommands.CMD_STATUS;
			response.header.sendertype  = eSenderType.AGENT;
			response.header.mngtstatus  = eStatus.OK;
			response.data.szServerUID = data.szServerUID;

			//for all TDMF tools, exit code : 0 = ok , else error.
			response.data.iStatus = exitcode == 0 ? tdmf_commands_status.OK : tdmf_commands_status.ERROR;
			response.data.iSubCmd = data.iSubCmd;
			response.data.iLength = output.Length;

			SendData( socket, response.ToBytes() );
			if ( output.Length > 0 ) {
				byte [] msg = ASCIIEncoding.UTF8.GetBytes( output );
				SendData( socket, msg );
			}
		}

		//MMP_MNGT_PERF_CFG_MSG
		//  Sent by TDMF Collector to all TDMF Agents.  
		//  Collector saves into the TDMF DB NVP table and sends to all Agents in DB
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfPerfCfgMsg_t : Bytes {
			public Header header;
			public mmp_TdmfPerfConfig data;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfPerfCfgMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				return bytes;
//			}
		}

		//MMP_MNGT_PERF_CFG_MSG
		[Serializable]
		[StructLayout(LayoutKind.Auto, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfPerfConfig : Bytes {
			// all periods specified in tenth of a second (0.1 sec) units
			// any values <= 0 will be considered as non-initialized and will not be refreshed by TDMF Agents
			public int iPerfUploadPeriod;      //period at which the TDMF Agent acquires the Performence data
			public int iReplGrpMonitPeriod;    //period at which the TDMF Agent acquires all Replication Groups Moinitoring Data
			[MarshalAs(UnmanagedType.ByValArray, SizeConst=12)]
			public int [] notused;            //for future use

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfPerfConfig ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( iPerfUploadPeriod ) ).CopyTo( bytes, 0 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( iReplGrpMonitPeriod ) ).CopyTo( bytes, 4 );
//				ASCIIEncoding.UTF8.GetBytes( notused ).CopyTo( bytes, 4 + 4 ); 
//				return bytes;
//			}
//			public void FromBytes ( byte [] bytes ) {
//				FromBytes( bytes, 0 );
//			}
//			public void FromBytes ( byte [] bytes, int index ) {
//				if ( bytes.Length >= Size + index ) {
//					throw new OmException("Error: Invalid byte count in mmp_TdmfPerfConfig ToBytes" );
//				}
//				iPerfUploadPeriod = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, index ) );
//				iReplGrpMonitPeriod = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 4 + index ) );
//			}		
		}
		/// <summary>
		/// Receive the protocol mmp_mngt_TdmfAgentDevices_t
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_TdmfPerfConfig header ) {
			byte [] bytes = new byte[ header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}

		//MMP_MNGT_TDMF_SENDFILE : 
		//  All members of the 'data' structure must be filled properly.
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public struct mmp_mngt_FileMsg_t {
			public Header header;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;				// name of machine identifying the TDMF Agent.
			public mmp_TdmfFileTransferData data;	// file data transfered 
		}


		//
		// Called to make sure we have read the collector values from the
		// registry and know where to connect to.
		//
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern void Mngt_Initialize_Collector_Values();
		//
		// Called to make sure we have read the collector values from the
		// registry and know where to connect to.
		//
		public static void ftd_mngt_initialize_collector_values() {
			// get the ip of the collector
			giTDMFCollectorIP = IPAddress.Parse( RegistryAccess.CollectorIP() );
			// Get the port of the collector
			giTDMFCollectorPort = RegistryAccess.CollectorPort();
		}

		public static bool IsCollectorAvailable () {
			if ( BitConverter.ToInt32( giTDMFCollectorIP.GetAddressBytes(), 0 ) == 0 ) {
				giTDMFCollectorIP = IPAddress.Parse( RegistryAccess.CollectorIP() );
				if ( BitConverter.ToInt32( giTDMFCollectorIP.GetAddressBytes(), 0 ) == 0 ) {
					return false;
				}
			}
			if ( giTDMFCollectorPort == 0 ) {
				giTDMFCollectorPort = RegistryAccess.CollectorPort();
				if ( giTDMFCollectorPort == 0 ) {
					return false;
				}
			}
			return true;
		}


		/* logical group monitoring information */
		public struct ftd_mngt_lg_monit_t {
			public long last_monit_ts;              /* last time monitoring data was sent to Collector */
			public long monit_dt;                   /* requested period value between msgs to Collector */
			public Int64 curr_pstore_file_sz;        /* current pstore file size value (bytes) sent */
			public Int64 curr_journal_files_sz;      /* current journal file size value (bytes) sent*/
			public Int64 last_pstore_file_sz;        /* last pstore file size value (bytes) sent, to avoid resending same information   */
			public Int64 last_journal_files_sz;      /* last journal file size value (bytes) sent, to avoid resending same information  */
			public Int64 journal_disk_total_sz;      /* data about the disk where the journal is stored, size in bytes */
			public Int64 journal_disk_free_sz;       /* data about the disk where the journal is stored, size in bytes */
			public Int64 pstore_disk_total_sz;       /* data about the disk where the PStore is stored, size in bytes */
			public Int64 pstore_disk_free_sz;        /* data about the disk where the PStore is stored, size in bytes */
			public int iReplGrpSourceIP;
			public int iDirtyFlag;			// Value to know if the monitor info should be sent to the collector regardless if it changed or not
		}

		//MMP_MNGT_GROUP_MONITORING
		//  Sent by a TDMF Agent to TDMF Collector
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfReplGroupMonitorMsg_t : Bytes {
			public Header header;
			public mmp_TdmfReplGroupMonitor data;

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfReplGroupMonitorMsg_t ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				header.ToBytes().CopyTo( bytes, 0 );
//				data.ToBytes().CopyTo( bytes, header.Size );
//				return bytes;
//			}
		}
		//MMP_MNGT_GROUP_MONITORING
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfReplGroupMonitor : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;		// name of machine identifying the TDMF Agent.
			public Int64 liActualSz;		// Total size (in bytes) of PStore file or Journal file for this group.
			public Int64 liDiskTotalSz;		// Total size (in MB) of disk on which PStore file or Journal file is maintained.
			public Int64 liDiskFreeSz;		// Total free space size (in MB) of disk on which PStore file or Journal file is maintained.
			public int iReplGrpSourceIP;	// IP address of the Source 
			public short sReplGrpNbr;		// replication group number, >= 0
			public byte isSource;			// indicates if this server is the SOURCE (primary) or TARGET (secondary) server of this replication group
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=1)]
			public string notused1;			// for future use, back to a 4-byte boundary
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string notused2;			// for future use

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfReplGroupMonitor ) ); }
//			}
//			public byte [] ToBytes () {
//				byte [] bytes = new byte[ Size ];
//				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( liActualSz ) ).CopyTo( bytes, 80 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( liDiskTotalSz ) ).CopyTo( bytes, 80 + 8 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( liDiskFreeSz ) ).CopyTo( bytes, 80 + 8 + 8 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( iReplGrpSourceIP ) ).CopyTo( bytes, 80 + 8 + 8 );
//				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( sReplGrpNbr ) ).CopyTo( bytes, 80 + 8 + 8 + 4 );
//				BitConverter.GetBytes( isSource ).CopyTo( bytes, 80 + 8 + 8 + 4 + 2 );
//				ASCIIEncoding.UTF8.GetBytes( notused1 ).CopyTo( bytes, 80 + 8 + 8 + 4 + 2 + 1 );
//				ASCIIEncoding.UTF8.GetBytes( notused2 ).CopyTo( bytes, 80 + 8 + 8 + 4 + 2 + 1 + 256 );
//				return bytes;
//			}
		}


		public static void ftd_mngt_send_lg_monit( int lgnum, bool isPrimary, ftd_mngt_lg_monit_t monitp ) {
			mmp_mngt_TdmfReplGroupMonitorMsg_t message = new mmp_mngt_TdmfReplGroupMonitorMsg_t();
    
			// build msg
			message.header.magicnumber     = eMagic.MSG_MAGICNUMBER;
			message.header.mngttype        = eManagementCommands.GROUP_MONITORING;
			message.header.mngtstatus      = eStatus.OK; 
			message.header.sendertype      = eSenderType.AGENT;
			message.data.iReplGrpSourceIP = 0;		// updated only if ROLESECONDARY
			message.data.liDiskFreeSz     = 0;
			message.data.liActualSz       = 0;
			message.data.szServerUID = UniqueHostID.ToStringID();
			message.data.sReplGrpNbr = (short)lgnum;

			//init dynamic data of msg
			if ( isPrimary ) {
				// a ReplGroup Primary machine uses the PStore 
				message.data.liDiskTotalSz = monitp.pstore_disk_total_sz;
				message.data.liDiskFreeSz = monitp.pstore_disk_free_sz;   
				message.data.liActualSz = monitp.curr_pstore_file_sz;
				message.data.isSource = 1;
			}
			else {
				//a ReplGroup Secondary machine uses the Journal
				message.data.liDiskTotalSz = monitp.journal_disk_total_sz;
				message.data.liDiskFreeSz = monitp.journal_disk_free_sz;   
				message.data.liActualSz = monitp.curr_journal_files_sz;
				message.data.isSource = 0;
				message.data.iReplGrpSourceIP = monitp.iReplGrpSourceIP;
			}

			// send Status Messages to Collector
			Socket socket = Protocol.CollectorConnect();
			try {
				SendData( socket, message.ToBytes() );
			} finally {
				socket.Close();
			}
		}

		public static int name_to_ip( string name ) {
			IPHostEntry hostInfo = Dns.GetHostByName( name );
			IPAddress[] address = hostInfo.AddressList;

			if ( address.Length == 0 ) {
				// its an IP address and not a machine name
				address[0] = IPAddress.Parse( name );
			}
			return BitConverter.ToInt32( address[0].GetAddressBytes(), 0 );
		}

		//MMP_MNGT_PERF_MSG
		//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfPerfMsg_t : Bytes {
			public Header header;
			public mmp_TdmfPerfData data;
			// this message structure is followed by a variable number of ftd_perf_instance_t structures
			// refer to mmp_TdmfPerfData members.
		
//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_mngt_TdmfPerfMsg_t ) ); }
//			}
		}

		//MMP_MNGT_PERF_MSG
		//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_TdmfPerfData : Bytes {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;		// name of machine identifying the TDMF Agent.
			public int iPerfDataSize;		// size of performance data (in bytes) following this structure.
			// the performance data is in the form of a number of ftd_perf_instance_t structures (iPerfDataSize / sizeof(ftd_perf_instance_t))
			// If, for the reception peer, the result of iPerfDataSize / sizeof(ftd_perf_instance_t)  has a remainder ( iPerfDataSize % sizeof(ftd_perf_instance_t) != 0 ), 
			// it could mean that both peers are not synchronized on the ftd_perf_instance_t struct. definition.

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( mmp_TdmfPerfData ) ); }
//			}
		}

		//MMP : pack ftd_perf_instance_t structure to be sent over socket 
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode, Pack=1)]
		public class ftd_perf_instance_t : Bytes {
			public UInt64 actual;
			public UInt64 effective;
			public UInt64 bytesread;
			public UInt64 byteswritten;
			public int connection;	/* 0 - pmd only, 1 - connected, -1 - accumulate */
			public eLgModes drvmode;	/* driver mode                          */
			public int lgnum;		// group number
			public int insert;		// gui list insert
			public int devid;		/* device id */
			public int rsyncoff;	/* rsync sectors done */
			public int rsyncdelta;	/* rsync sectors changed */
			public int entries;	/* # of entries in bab */
			public int sectors;	/* # of sectors in bab */
			public int pctdone;
			public int pctbab;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=48)]
			public string wcszInstanceName; // SZ instance name
			public int Reserved1;		// unused
			public int Reserved2;		// unused
			public byte role;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst=35)]
			public byte [] padding;// to align structure on a 8 byte boundary.

//			public static int Size {
//				get { return Marshal.SizeOf( typeof( ftd_perf_instance_t ) ); }
//			}
		}


		// MMP_MNGT_GROUP_STATE
		// Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
		public class mmp_mngt_TdmfGroupStateMsg_t : Bytes {
			public Header header;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID;		// name of machine identifying the TDMF Agent.
			// this message structure is followed by a variable number of mmp_TdmfGroupState structures
			// the last mmp_TdmfGroupState structure will show an invalid group number value.
		}
		public static void SendGroupStateMsg( Socket socket ) {
			mmp_mngt_TdmfGroupStateMsg_t response = new mmp_mngt_TdmfGroupStateMsg_t();
			//complete response msg and send it to requester
			response.header.magicnumber = eMagic.MSG_MAGICNUMBER;
			response.header.mngttype    = eManagementCommands.GROUP_STATE;
			response.header.sendertype  = eSenderType.AGENT;
			response.header.mngtstatus  = eStatus.OK;
			response.szServerUID = gszServerUID;

			SendData( socket, response.ToBytes() );
		}

		//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
		public class mmp_TdmfGroupState : Bytes {
			public short sRepGrpNbr;		// valid number have values >= 0.
			public short sState;			// bit0 : 0 = group stopped, 1 = group started.

			public mmp_TdmfGroupState ( short number, short state ) {
				sRepGrpNbr = number;
				sState = state;
			}
		}
		public static void SendGroupState( Socket socket, mmp_TdmfGroupState state ) {
			SendData( socket, state.ToBytes() );
		}

		public static void SendGroupState( Socket socket, ftd_perf_instance_t [] DeviceInstances, string ServerID ) {
			mmp_mngt_TdmfPerfMsg_t header = new mmp_mngt_TdmfPerfMsg_t();
			header.header.magicnumber = eMagic.MSG_MAGICNUMBER;
			header.header.mngttype    = eManagementCommands.PERF_MSG;
			header.header.sendertype  = eSenderType.AGENT;
			header.header.mngtstatus  = eStatus.OK;
			header.data.szServerUID = ServerID;
			// calculate the size of the data to follow
			header.data.iPerfDataSize = Marshal.SizeOf( typeof( ftd_perf_instance_t ) ) * DeviceInstances.Length;

			SendData( socket, header.ToBytes() );

			// send the performance data
			foreach ( ftd_perf_instance_t instance in DeviceInstances ) {
				SendData( socket, instance.ToBytes() );
			}
		}


	}
}


