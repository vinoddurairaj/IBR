using System;
using System.Runtime.InteropServices;
using System.Net.Sockets;
using System.Threading;

namespace MasterThread
{
	/// <summary>
	/// Summary description for StatisticsThread.
	/// </summary>
	public class StatisticsThread : XMDThread
	{

		/// <summary>
		/// Constructor
		/// </summary>
		public StatisticsThread()
		{
		}
		public void Start() {
			try {
				// execute the stat thread code
				ExecuteStatistics();
			}
			catch ( ThreadAbortException e ) {
				OmException.LogException( new OmException( String.Format("Error in StatisticsThread Start : {0}, {1}", e.Message, e.ExceptionState ), e ) );
			}
			catch( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in StatisticsThread Start Exception: {0}", e.Message ), e ) );
			}
		}

		public static string Name() {
			return "statd";
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern Int32 StatisticsThreadEntry( IntPtr args );

		/// <summary>
		/// ftd_proc_exec_throt -- execute the statistics thread
		/// </summary>
		/// <returns></returns>
		public unsafe void ExecuteStatistics() {
			m_Args = new Args_t();	
			// manual, not signaled
			MasterThread.m_StatdEvent = m_Args.procp.hEvent = CreateEvent ( IntPtr.Zero, true, false );
			try {
				if ( m_Args.procp.hEvent == IntPtr.Zero ) {
					OmException.LogException( new OmException ( "Error: ExecuteStatistics fail to CreateEvent" ) );
					return;
				}			
				IntPtr args = Marshal.AllocHGlobal( Marshal.SizeOf( m_Args ) );
				try {
					// clear the buffer
					DeviceIOControl.Zero( args, Marshal.SizeOf( m_Args ) );

					IntPtr proc = Marshal.AllocHGlobal( Marshal.SizeOf( m_Args.procp ) );
					try {
						// clear the buffer
						DeviceIOControl.Zero( proc, Marshal.SizeOf( m_Args.procp ) );

						Marshal.WriteIntPtr( (IntPtr)(args.ToInt32() + 24), proc );
						Marshal.StructureToPtr( m_Args.procp, proc, false );

						// call rmd thread function here
						StatisticsThreadEntry( args );

					} finally {
						Marshal.FreeHGlobal( proc );
					}
				} finally {
					Marshal.FreeHGlobal( args );
				}
		} finally {
				// close the event
				Closehandle ( m_Args.procp.hEvent );
			}
		}









	}
}
