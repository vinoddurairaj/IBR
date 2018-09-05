using System;
using System.Net.Sockets;
using System.Threading;
using System.Runtime.InteropServices;

namespace MasterThread {

	// Thread exit codes
	public enum eExitCodes {
		EXIT_NORMAL = 0,
		EXIT_RESTART = 1,
		EXIT_NETWORK = 1,
		EXIT_DIE = 2,
		EXIT_DRV_FAIL = 3
	}


	/// <summary>
	/// ftd_proc_args_t - this struct is passed to the external call to RemoteThread
	/// </summary>
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public class Args_t {
		public int		lgnum;			/* used by RMDThread logical group number */
		public int		state;			/* used by RMDThread enumerated process state */
		public int		apply;			/* used by RMDThread starts an apply rmd */
		public IntPtr	dsock;			/* used by RMDThread rmd needs pmds' socket */
		public int		cpon;			// used by RMDThread
		public int		consleep;				
		public proc_t	procp;			// used by RMDThread

		/// <summary>
		/// Constructor
		/// </summary>
		public Args_t () {
			lgnum = -1;
			state = 0;
			apply = 0;
			dsock = IntPtr.Zero;
			cpon = 0;
			consleep = 0;
			// init the proc_t pointer
			procp = new proc_t();

		}
	}
	// this struct is passed to the external call to RemoteThread
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
	public class proc_t {
		public int pid;					/* used by RMDThread  process id/thread HANDLE			*/
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
		public string procname;			/* process (argv[0])/thread name	*/
		public string command;			// used by RMDThread
		public IntPtr function;
		public eProtocol nSignal;		// used by RMDThread
		public IntPtr hEvent;			// used by RMDThread
		public int dwId;
		public int hostid;				/* hostid process is running on		*/
		public int type;				/* process type (ie. proc, thread)	*/

		/// <summary>
		/// Constructor
		/// </summary>
		public proc_t () {
			pid = 0;
			function = IntPtr.Zero;
			nSignal =  eProtocol.Invalid;
			hEvent = IntPtr.Zero;
			dwId = 0;
			hostid = 0;
			type = 0;
		}
	}


	/// <summary>
	/// Summary description for XMDThread.
	/// This is the base class for the RMD and PMD threads
	/// </summary>
	public class XMDThread {

		public eExitCodes m_ExitCode;

		public Thread m_thisThread;		// the thread this instance is running in Owned by MasterThread
		public Socket m_IPCSocket;		// IPC socket connection to child thread owned by MasterThread
		public SocketCollection m_CommandSocketCollection;	// collection of command sockets for the thread. Owned by MasterThread

		public Int32 m_lgnum;			// logical group number
		
		protected Socket m_MasterSocket = null;	// isockp -- socket back to MasterThread owned by ChildThread PMD, RMD, etc.

		public Args_t m_Args;	// this is what was passed to the start thread

		/// <summary>
		/// Constructor
		/// </summary>
		public XMDThread() {
			this.m_CommandSocketCollection = new SocketCollection();
		}

		~XMDThread() {
			// clear all of the socket object
			if ( this.m_IPCSocket != null ) {
				this.m_IPCSocket.Close();
				this.m_IPCSocket = null;
			}
			foreach ( Socket socket in this.m_CommandSocketCollection ) {
				socket.Close();
			}
			this.m_CommandSocketCollection.Clear();
			if ( this.m_MasterSocket != null ) {
				this.m_MasterSocket.Close();
				this.m_MasterSocket = null;
			}
		}

		[DllImport("Kernel32", SetLastError = true)]
		private static extern IntPtr CreateEvent(
			IntPtr lpEventAttributes,
			bool bManualReset,
			bool bInitialState,
			IntPtr lpName
			);
		public IntPtr CreateEvent( IntPtr ptr, bool reset, bool state ) {

			return CreateEvent ( ptr, reset, state, IntPtr.Zero );
		}

		[DllImport("kernel32", SetLastError=true)]
		static extern unsafe bool CloseHandle(IntPtr Handle);
		public bool Closehandle ( IntPtr handle ) {
			return CloseHandle ( handle );
		}

//		[DllImport("kernel32", SetLastError=true)]
//		static extern unsafe bool SetEvent(IntPtr hEvent);
//		public int proc_signal( proc_t procp, eProtocol sig ) {
//
//			procp.nSignal = sig;
//
//			if ( procp.hEvent != IntPtr.Zero ) {
//				SetEvent( procp.hEvent );
//			} 
//			else {
//				return -1;
//			}
//			switch( sig ) {
//				case eProtocol.CSIGTERM:
//					throw new OmException ( "Error: FTDCSIGTERM should not be used to terminate the thread." );
//			}
//			return 0;
//		}




	}
}
