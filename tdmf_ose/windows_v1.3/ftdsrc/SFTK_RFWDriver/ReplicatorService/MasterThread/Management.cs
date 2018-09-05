using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;


namespace MasterThread {
	/// <summary>
	/// Summary description for Management.
	/// </summary>
	public class ManagementOld {

		public ManagementOld() {
		}

		/// <summary>
		/// Called once, when TDMF Agent starts.
		/// </summary>
		/// 
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Mngt_Initialize();
		public static void ManagementInitialize() {
			Mngt_Initialize();
		}

		/////////////////////////////////////////////////////////////////////////////
		// group state sent to Collector by StatThread()
		// called from another thread than ftd_mngt_performance_send_data()
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern void Mngt_Performance_Set_Group_Cp( Int16 sGrpNumber, int bIsCheckpoint );

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern IntPtr Mngt_Create_Lg_Monit();

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern int Mngt_Init_Lg_Monit( IntPtr monitp, eRole role, string pstore, string phostname, string jrnpath );

		public static int InitLgMonitor( LogicalGroup lgp ) {
			lgp.monitp = Mngt_Create_Lg_Monit();
			return Mngt_Init_Lg_Monit( lgp.monitp, lgp.cfgp.role, lgp.cfgp.pstore, lgp.cfgp.phostname, lgp.cfgp.jrnpath );
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Mngt_Do_Lg_Monit( IntPtr monitp, eRole role, string pstore, string phostname, string jrnpath, int lgnum, IntPtr jrnp );
		public static void DoLgMonitor( LogicalGroup lgp ) {
			Mngt_Do_Lg_Monit( lgp.monitp, lgp.cfgp.role, lgp.cfgp.pstore, lgp.cfgp.phostname, lgp.cfgp.jrnpath, lgp.cfgp.lgnum, IntPtr.Zero );
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern int Mngt_Delete_Lg_Monit( IntPtr monitp );


		/*
		 * Send this Agent's basic management information message (MMP_MNGT_AGENT_INFO) to TDMF Collector.
		 * TDMF Collector coordinates are based on last time a MMP_MNGT_AGENT_INFO_REQUEST was received from it.
		 * return 0 on success, 
		 *        +1 if TDMF Collector coordinates are not known, 
		 *        -1 if cannot connect on TDMF Collector,
		 *        -2 on tx error.
		 */
		[DllImport("RplLibWrapper", SetLastError = true)]
		public static extern int Mngt_Send_Agentinfo_Msg( int rip, int rport );
		public static int Mngt_Send_AgentInfo_Msg() {
			return Mngt_Send_Agentinfo_Msg( 0, 0 );
		}
		// PUSH license key to Collector
		[DllImport("RplLibWrapper", SetLastError = true)]
		public static extern void Mngt_Send_Registration_Key();

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern void Mngt_Shutdown();

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Mngt_Msgs_Log( IntPtr argv, int argc );
		public unsafe static void LogMessage( string [] args ) {
			// copy the string [] into memory to send the dll
			IntPtr pnt = Marshal.AllocHGlobal( args.Length * sizeof(IntPtr) );
			try {
				for ( int index = 0 ; index < args.Length ; index++ ) {
					Marshal.WriteIntPtr ( pnt, index * sizeof(IntPtr), Marshal.StringToHGlobalAnsi( args[ index ] ) );
				}
				try {
					Mngt_Msgs_Log( pnt, args.Length );
				} finally {
					for ( int index = 0 ; index < args.Length ; index++ ) {
						Marshal.FreeHGlobal( Marshal.ReadIntPtr( pnt, index * sizeof(IntPtr) ) );
					}
				}
			} finally {
				Marshal.FreeHGlobal( pnt );
			}
		}

		///////////////////////////////////////////////////////////////////////////////
		/*
		 * A MMP message request has to be received and processed.
		 *
		 * Message is expected to be a MMP MANAGEMENT msg.
		 * Receive message from a SOCK_STREAM connection.
		 */
		/// <summary>
		/// ftd_sock_t
		/// </summary>
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public struct Sock_t {
			public eMagic magicvalue;					/* so we know it's been initialized */
			public int keepalive;					/* don't destroy if TRUE */
			public int contries;					/* peer connect tries */
			public int type;						/* connect type: GENERIC, LG */
			public IntPtr sockp;					/* socket object address */
		}

		/* socket structure */
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public struct sock_t {
			public IntPtr sock;			/* socket for remote communications		*/
			public IntPtr hEvent;
			public int	bBlocking;
			public int type;			/* protocol type - AF_INET, AF_UNIX		*/
			public int family;			/* protocol family - STREAM, DGRAM		*/
			public int tsbias;			/* time diff between local and remote	*/
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
			public string lhostname;	/* local host name						*/
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
			public string rhostname;	/* remote host name						*/
			public int lip;			/* local ip address						*/
			//[MarshalAs(UnmanagedType.ByValArray, SizeConst=4)]
			public int rip;			/* remote ip address					*/
			public int lhostid;		/* local hostid							*/
			public int rhostid;		/* remote hostid						*/
			public int port;			/* connection port number				*/
			public int readcnt;			/* number of bytes read					*/
			public int writecnt;		/* number of bytes written				*/
			public int flags;			/* state bits							*/
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static unsafe extern IntPtr Sock_Create( int type );

		public static int SOCK_GENERIC = 1;

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern unsafe int Mngt_Recv_Msg( IntPtr socketHandle );
		public static unsafe int ReceiveMessage ( Socket socket, ref bool KeepSocket ) 
		{

//			Sock_t Soc = new Sock_t();
//			sock_t soc = new sock_t();
//			soc.bBlocking = 1;
//			Soc.type = SOCK_GENERIC;
//			soc.family = 2;
//			//soc.lhostid = 1535676516;
//			Soc.magicvalue = eMagic.Socket;
//			Soc.contries = 0;
//			soc.port = 575;
//			Soc.keepalive = 1;
//
//			// dup the socket handle so the management C++ code can close it.
//			Handle.Duplicate( socket.Handle, out soc.sock );
//			// soc.sock = socket.Handle;
//
//			soc.flags |= 1;
//
//			//peer IP address
//			IPEndPoint endPoint = (IPEndPoint)socket.RemoteEndPoint;
//			//soc.rip  = (int)endPoint.Address.GetAddressBytes();
//			soc.port = endPoint.Port;
//
//			int size = (int)(Marshal.SizeOf( typeof(sock_t)));
//			Soc.sockp = Marshal.AllocHGlobal( size );
//			Marshal.StructureToPtr( soc, Soc.sockp, true );
//
//			size = (int)(Marshal.SizeOf( typeof(Sock_t))) + (int)(Marshal.SizeOf( typeof(sock_t))) + 4 ;
//			IntPtr ptr = Marshal.AllocHGlobal( size );
//			Marshal.StructureToPtr( Soc, ptr, false );

			Mngt_Recv_Msg( socket.Handle );

			KeepSocket = true;
			return 0;
		}


		//
		// Called to make sure we have read the collector values from the
		// registry and know where to connect to.
		//
		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static extern void Mngt_Initialize_Collector_Values();


















	}
}
