using System;
using System.Threading;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace MasterThread
{
	/// <summary>
	/// Summary description for RMDAThread.
	/// RMD Apply thread
	/// </summary>
	public class RMDAThread : XMDThread
	{

		public int m_CpOn = 0;

		/// <summary>
		/// Constructor
		/// </summary>
		public RMDAThread()
		{
		}

		public void Start() {
			try {
				//
				// TODO: call the RMD code here
				//
				// execute the remote thread code
				ExecuteApply();
			}
			catch ( ThreadAbortException e ) {
				OmException.LogException( new OmException( String.Format("Error in RMDThread Start : {0}, {1}", e.Message, e.ExceptionState ), e ) );
			}
			catch( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in RMDThread Start Exception: {0}", e.Message ), e ) );
			}
		}

		public static string Name( int lgnum ) {
			return "RMDA_" + lgnum.ToString("D3");
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern Int32 RemoteThread( ref Args_t args );

		/// <summary>
		/// ftd_proc_exec_apply -- execute the apply thread
		/// </summary>
		/// <returns></returns>
		public void ExecuteApply() {
			m_Args = new Args_t();

			m_Args.lgnum = this.m_lgnum;
			m_Args.apply = 1;
			m_Args.cpon = this.m_CpOn;
			//m_Args.dsock = this.m_PMDsocket.Handle;
			m_Args.procp = new proc_t();
			m_Args.procp.pid = this.m_thisThread.GetHashCode();

			// manual, not signaled
			m_Args.procp.hEvent = CreateEvent ( IntPtr.Zero, true, false );

			if ( m_Args.procp.hEvent == IntPtr.Zero ) {
				OmException.LogException( new OmException ( "Error: ExecuteApply fail to CreateEvent" ) );
				return;
			}
			// call rmd thread function here
			RemoteThread( ref m_Args );

			// close the event
			Closehandle ( m_Args.procp.hEvent );
		}


	}
}
