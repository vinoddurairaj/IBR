using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace MasterThread
{
	/// <summary>
	/// Summary description for ErrorFacility.
	/// </summary>
	public class ErrorFacility
	{
		private static EventLog m_eventLog = new EventLog("Replicator", ".", "ReplicatorServer");
		public errfac_t m_ErrFac = new errfac_t();

		public enum eTraceLevel 
		{
			TRACEERR	= 1,
			TRACEWRN	= 2,
			TRACEINF	= 3,
			TRACEINF4	= 4,
			TRACEINF5	= 5,
			TRACEINF6	= 6
		}


		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public struct errfac_t {
			public eMagic magicvalue;			/* magic number for init determination */
			IntPtr last;					/* last reported error message */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string facility;			/* error facility name */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string msgdbpath;		/* error message database path */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string logpath;			/* error log path */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string logname;			/* current log file name */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string logname1;			/* previous log file name */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string hostname;			/* host name creating this object */
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
			public string procname;			/* process name creating this object */
			public IntPtr dbgfd;			/* trace file handle */
			public IntPtr logfd;			/* error log file handle */
			public int logerrs;				/* log errors flag */
			public int logstderr;			/* log errors to stderr ? */
			public int reportwhere;			/* report source file, line */
			public int errcnt;				/* # errors in list */
			public IntPtr emsgs;			/* list of error msgs */
		}

		public ErrorFacility( string facility, string procname, string msgdbpath, string logpath, int reportwhere, int logstderr ) {
			m_ErrFac.facility = facility;
			m_ErrFac.procname = procname;
			m_ErrFac.msgdbpath = msgdbpath;
			m_ErrFac.logpath = logpath;
			m_ErrFac.reportwhere = reportwhere;
			m_ErrFac.logstderr = logstderr;
			m_ErrFac.hostname = SystemInformation.ComputerName;
		}


		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Init_Errfac (string facility, string procname, string msgdbpath, string logpath,
			int reportwhere, int logstderr);
		public static void Init( string facility, string procname, string msgdbpath, string logpath, int reportwhere, int logstderr) {
			Init_Errfac ( facility, procname, msgdbpath, logpath, reportwhere, logstderr);
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Delete_Errfac();
		public static void Close() {
			Delete_Errfac ();
		}

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		private static extern void Set_Error_Trace_Level(string level);
		public static void SetErrorTraceLevel(string level) {
			Set_Error_Trace_Level(level);
		}

		/// <summary>
		/// error_format_datetime -- combine date-time , errfac_t information, message level, name, text into one text string.
		/// same formatting as done in error_logger()
		/// </summary>
		/// <param name="?"></param>
		public string error_format_datetime( string msglevel, string name, string msg ) {
			DateTime now = DateTime.Now;
			string retmsg = now.ToString("[yyyy/MM/dd HH:mm:ss]") + String.Format( " {0}: [{1}:{2}] [{3} / {4}] {5}\n",
				m_ErrFac.facility, m_ErrFac.hostname, m_ErrFac.procname, msglevel, name, msg );
			return retmsg.Trim();
		}


		/// <summary>
		/// error_syslog -- log the message to the NT system log
		/// </summary>
		/// <param name="errfac"></param>
		/// <param name="reporting"></param>
		/// <param name="format"></param>
		/// <param name="list"></param>
		public void error_syslog( EventLogEntryType reporting, string format, params string[] list ) {
			ErrorSyslog( reporting, format, list );
		}
		public void ErrorSyslog( EventLogEntryType reporting, string format, string[] list ) {

			string message = String.Format( format, list );

			m_eventLog.WriteEntry( message, reporting );

			// add to global msg list
			error_msg_list_addMessage( reporting, message );

		}

		public void error_msg_list_addMessage( EventLogEntryType priority, string statusmsg ) {
			Management.ManagementStatusThread.gStatusMsgList.Add( new Management.StatusMessage( statusmsg, priority ) );
		}



	}
}
