using System;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Serialization.Formatters.Binary;
using System.Runtime.InteropServices;
using System.Text;

namespace MasterThread
{
	/// <summary>
	/// Summary description for ManagementProtocol.
	/// </summary>
	public class ManagementProtocol
	{
		/// <summary>
		/// MMP management header message structure
		/// this is the management header used in all mngt messages
		/// </summary>
		public class Header {
			public uint magicnumber;	// indicates the following is a management message
			public eManagementCommands mngttype; // enum management, the management msg type.
			public eSenderType sendertype;		// indicates this message sender : TDMF_AGENT, TDMF_SERVER, CLIENT_APP
			public eStatus mngtstatus;		// enum mngt_status. this field is used to return a response status.  
			// used when the message is a response to a mngt request.

			public static int Size {
				get { return 16; }
			}
			public byte [] ToBytes () {
				byte [] bytes = new byte[ Size ];
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)magicnumber ) ).CopyTo( bytes, 0 );
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)mngttype ) ).CopyTo( bytes, 4 );
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)sendertype ) ).CopyTo( bytes, 8 );
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)mngtstatus ) ).CopyTo( bytes, 12 );
				return bytes;
			}
			public void FromBytes ( byte [] bytes ) {
				if ( bytes.Length >= Size ) {
					throw new OmException("Error: Invalid byte count in Header ToBytes" );
				}
				magicnumber = (uint)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 0 ) );
				mngttype = (eManagementCommands)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 4 ) );
				sendertype = (eSenderType)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 8 ) );
				mngtstatus = (eStatus)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 12 ) );
			}		
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

		public const uint MNGT_MSG_MAGICNUMBER = 0xe8ad4239;

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
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public class mmp_mngt_ConfigurationMsg_t {
			//public Header hdr;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string szServerUID; //name of machine identifying the TDMF Agent.
			[MarshalAs(UnmanagedType.Struct)]
			public mmp_TdmfFileTransferData data;		//cfg file is transfered 

			public static int Size {
				get { return 80+264; }
			}
			public byte [] ToBytes () {
				byte [] bytes = new byte [ Size ];
				ASCIIEncoding.UTF8.GetBytes( szServerUID ).CopyTo( bytes, 0 ); 
				data.ToBytes().CopyTo( bytes, 80 );
				return bytes;
			}
			public void FromBytes ( byte [] bytes ) {
				if ( bytes.Length >= Size ) {
					throw new OmException("Error: Invalid byte count in mmp_mngt_ConfigurationMsg_t ToBytes" );
				}
				szServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0, 80 );
				data = new mmp_TdmfFileTransferData();
				byte [] b = new byte[ 264 ];
				Array.Copy( bytes, 80, b, 0, b.Length );
				data.FromBytes( b );
			}		

		}

		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public class mmp_TdmfFileTransferData {
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
			public string szFilename;	//name of file at destination, WITHOUT path.
			public eFileType iType;			//enum tdmf_filetype
			public int iSize;		//size of following data. 
			//the file binary data must be contiguous to this structure.

			public static int Size {
				get { return 256+8; }
			} 
			public byte [] ToBytes () {
				byte [] bytes = new byte[ Size ];
				ASCIIEncoding.UTF8.GetBytes( szFilename ).CopyTo( bytes, 0 ); 
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iType ) ).CopyTo( bytes, 256 );
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iSize ) ).CopyTo( bytes, 260 );
				return bytes;
			}
			public void FromBytes ( byte [] bytes ) {
				if ( bytes.Length >= Size ) {
					throw new OmException("Error: Invalid byte count in mmp_TdmfFileTransferData ToBytes" );
				}
				szFilename = ASCIIEncoding.UTF8.GetString( bytes, 0, 256 );
				iType = (eFileType)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 256 ) );
				iSize = IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 260 ) );
			}		
		}

		//sent in response to a MMP_MNGT_SET_LG_CONFIG request.
		//indicates if configuration was updated succesfully
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public class mmp_mngt_ConfigurationStatusMsg_t {
			public Header hdr;                    
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
			public string ServerUID;	//name of machine identifying the TDMF Agent.
			public ushort usLgId;		//logical group for which the config. set was requested
			public int iStatus;			//0 = OK, 1 = rx/tx error with TDMF Agent, 2 = err writing cfg file

			public static int Size {
				get { return 16+80+6; }
			}
			public byte [] ToBytes () {
				byte [] bytes = new byte[ Size ];
				hdr.ToBytes().CopyTo( bytes, 0 );
				ASCIIEncoding.UTF8.GetBytes( ServerUID ).CopyTo( bytes, 16 ); 
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (short)usLgId ) ).CopyTo( bytes, 96 );
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iStatus ) ).CopyTo( bytes, 98 );
				return bytes;
			}
			public void FromBytes ( byte [] bytes ) {
				if ( bytes.Length >= Size ) {
					throw new OmException("Error: Invalid byte count in mmp_mngt_ConfigurationStatusMsg_t ToBytes" );
				}
				hdr = new Header();
				hdr.FromBytes( bytes );
				ServerUID = ASCIIEncoding.UTF8.GetString( bytes, 16, 80 );
				usLgId = (ushort)IPAddress.NetworkToHostOrder( BitConverter.ToInt16( bytes, 96 ) );
				iStatus = (int)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 98 ) );
			}		
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


		public ManagementProtocol()
		{
			//
			// TODO: Add constructor logic here
			//
		}

		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref Header header ) {
			byte [] bytes = new byte[ Header.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
			//validate
			if ( header.magicnumber != MNGT_MSG_MAGICNUMBER ) {
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
			byte [] bytes = new byte[ mmp_mngt_ConfigurationMsg_t.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}

		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveHeader( Socket socket, ref mmp_TdmfRegistrationKey header ) {
			byte [] bytes = new byte[ mmp_TdmfRegistrationKey.Size ];
			ReceiveData( socket, ref bytes );
			header.FromBytes( bytes );
		}
		/// <summary>
		/// Receive the protocol header
		/// </summary>
		/// <param name="socket"></param>
		/// <param name="header"></param>
		public unsafe static void ReceiveData( Socket socket, ref byte [] bytes ) {
			try {
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
			finally {

			}
		}

		public static void SendSetConfigStatus(  Socket socket, mmp_mngt_ConfigurationMsg_t RcvCfgData, int status ) {
			mmp_mngt_ConfigurationStatusMsg_t response = new mmp_mngt_ConfigurationStatusMsg_t();

			//prepare status message 
			response.hdr.magicnumber = (uint)IPAddress.HostToNetworkOrder( unchecked((int)MNGT_MSG_MAGICNUMBER) );
			response.hdr.mngttype    = (eManagementCommands)IPAddress.HostToNetworkOrder( (int)eManagementCommands.SET_CONFIG_STATUS );
			response.hdr.sendertype  = (eSenderType)IPAddress.HostToNetworkOrder( (int)eSenderType.AGENT );
			response.hdr.mngtstatus  = (eStatus)IPAddress.HostToNetworkOrder( (int)eStatus.OK ); 
			//skip cfg file prefix('p' or 's')
			response.usLgId = (ushort)Convert.ToUInt16( RcvCfgData.data.szFilename.Substring( 1 ) );
			//convert to network byte order
			response.usLgId     = (ushort)IPAddress.HostToNetworkOrder((short)response.usLgId);
			response.iStatus    = IPAddress.HostToNetworkOrder( status );
			response.ServerUID = RcvCfgData.szServerUID;
			//send status message to requester
			byte [] data = response.ToBytes();
			SendData( socket, data );
		}

		public static void SendRegistrationKey(  Socket socket, mmp_TdmfRegistrationKey data ) {
			mmp_mngt_RegistrationKeyMsg_t response = new mmp_mngt_RegistrationKeyMsg_t();

			//send registration key to requester
			response.header.magicnumber = unchecked((uint)MNGT_MSG_MAGICNUMBER );
			response.header.mngttype    = eManagementCommands.REGISTRATION_KEY;
			response.header.sendertype  = eSenderType.AGENT;
			response.header.mngtstatus  = eStatus.OK; 
			//send status message to requester
			SendData( socket, response.ToBytes() );
			SendData( socket, data.ToBytes() );
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

			//read File data
			data = new byte[ RcvCfgData.data.iSize ];
			ReceiveData( socket, ref data );
		}


		public const string HOST_ID_PREFIX = "HostID=";
		/*
 * Builds a mmp_mngt_ConfigurationMsg_t message ready to be sent on socket
 * (host to network convertions done)
 * pFileData can be NULL; uiDataSize can be 0.
 */ 
		public static void mmp_mngt_build_SetConfigurationMsg( Socket socket,
			string AgentId,
			eSenderType SenderType,
			string CfgFileName,
			byte [] data
			 )
		{
			//build contiguous buffer for message 
			mmp_mngt_ConfigurationMsg_t msg = new mmp_mngt_ConfigurationMsg_t();
			byte [] bytes = new byte [ mmp_mngt_ConfigurationMsg_t.Size + data.Length ];

			//build msg header
			Header header = new Header();
			header.magicnumber = MNGT_MSG_MAGICNUMBER;
			header.sendertype = SenderType;
			header.mngttype    = eManagementCommands.SET_LG_CONFIG;
			header.mngtstatus  = eStatus.OK;
    
			msg.szServerUID = HOST_ID_PREFIX + AgentId;
            msg.data.szFilename = CfgFileName;
			msg.data.iType  = eFileType.CFG;
			msg.data.iSize = data.Length;
    
			// append all of the data into 1 byte array
			header.ToBytes().CopyTo( bytes, 0 );
			msg.ToBytes().CopyTo( bytes, Header.Size );
			data.CopyTo( bytes, Header.Size + mmp_mngt_ConfigurationMsg_t.Size );

			SendData ( socket, bytes );
		}

		//MMP_MNGT_REGISTRATION_KEY 
		//When sender provides a non-empty keydata.szRegKey , it is a SET key message. 
		//When sender provides an empty keydata.szRegKey ("" or "\0"), it is a GET key message. 
		//For a SET key, the same message is used as response. Upon success, the szRegKey member 
		//will contain the same key value.  On failure the szRegKey member returned will be an empty string.
		//For a GET key, the same message is used as response. Upon success, the szRegKey member 
		//will contain the key value.  On failure the szRegKey member returned will be an empty string.
		public struct mmp_mngt_RegistrationKeyMsg_t {
			public Header header;
			public mmp_TdmfRegistrationKey keydata;

			public static int Size {
				get { return Header.Size + mmp_TdmfRegistrationKey.Size; }
			}
			public byte [] ToBytes () {
				byte [] bytes = new byte[ Size ];
				header.ToBytes().CopyTo( bytes, 0 );
				keydata.ToBytes().CopyTo( bytes, Header.Size );
				return bytes;
			}
			public void FromBytes ( byte [] bytes ) {
				if ( bytes.Length >= Size ) {
					throw new OmException("Error: Invalid byte count in mmp_TdmfRegistrationKey ToBytes" );
				}
				header = new Header();
				header.FromBytes( bytes );
				keydata = new mmp_TdmfRegistrationKey();
				keydata.FromBytes( bytes, Header.Size );
			}		
		}
		
		//MMP_MNGT_AGENT_INFO_REQUEST
		//sent by client app to TDMF Collector or by TDMF Collector to TDMF Agent
		[Serializable]
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public struct mmp_TdmfRegistrationKey {
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

			public static int Size {
				get { return 80+40+4; }
			}
			public byte [] ToBytes () {
				byte [] bytes = new byte[ Size ];
				ASCIIEncoding.UTF8.GetBytes( ServerUID ).CopyTo( bytes, 0 ); 
				ASCIIEncoding.UTF8.GetBytes( RegKey ).CopyTo( bytes, 80 ); 
				BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)iKeyExpirationTime ) ).CopyTo( bytes, 80 + 40 );
				return bytes;
			}
			public void FromBytes ( byte [] bytes ) {
				FromBytes( bytes, 0 );
			}				
			public void FromBytes ( byte [] bytes, int index ) {
				if ( bytes.Length >= (Size + index) ) {
					throw new OmException("Error: Invalid byte count in mmp_TdmfRegistrationKey ToBytes" );
				}
				ServerUID = ASCIIEncoding.UTF8.GetString( bytes, 0 + index, 80 );
				RegKey = ASCIIEncoding.UTF8.GetString( bytes, 80 + index, 40 );
				iKeyExpirationTime = (int)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, 80 + 40 + index ) );
			}		

		}



	}
}
