using System;
using System.Diagnostics;

namespace MasterThread.Management
{

	//
	// The types of events that can be logged.
	// replaced by EventLogEntryType
//	public enum eEVENTLOG {
//		SUCCESS = 0x0000,
//		ERROR = 0x0001,
//		WARNING = 0x0002,
//		INFORMATION = 0x0004,
//		AUDIT_SUCCESS = 0x0008,
//		AUDIT_FAILURE = 0x0010
//	}


	/// <summary>
	/// Summary description for StatusMessage.
	/// </summary>
	public class StatusMessage {

		string m_strMessage;
		EventLogEntryType m_EventLog;    // can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
		DateTime m_TimeStamp;
		int m_iTdmfCmd;

		/// <summary>
		/// Constructor
		/// </summary>
		public StatusMessage() {
			m_TimeStamp   = DateTime.Now;
			m_iTdmfCmd    = -1;
		}

		public StatusMessage( string szMessage, EventLogEntryType iPriority ) {
			m_strMessage = szMessage;
			m_EventLog   = iPriority;		// can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
			m_TimeStamp  = DateTime.Now;
			m_iTdmfCmd   = 0;
		}

		//******************************************
		public StatusMessage( string szMessage, EventLogEntryType iPriority, int iTdmfCmd ) {
			m_strMessage = szMessage;
			m_EventLog   = iPriority;		// can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
			m_TimeStamp  = DateTime.Now;
			m_iTdmfCmd   = iTdmfCmd;
		}

		//******************************************
		//copy ctr
		public StatusMessage( StatusMessage aMsg ) {
			m_strMessage = aMsg.m_strMessage;
			m_EventLog	= aMsg.m_EventLog;		// can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
			m_TimeStamp	= aMsg.m_TimeStamp;
		}

		public string Message {
			get { return this.m_strMessage; }
			set { this.m_strMessage = value; }
		}
		public DateTime TimeStamp {
			get { return this.m_TimeStamp; }
			set { this.m_TimeStamp = value; }
		}
		public int MessageLength {
			get { return this.m_strMessage.Length; }
		}

		public EventLogEntryType Priority {
			get { return this.m_EventLog; }
			set { this.m_EventLog = value; }
		}
		public int TdmfCmd {
			get { return this.m_iTdmfCmd; }
			set { this.m_iTdmfCmd = value; }
		}

//		//equality operator
//		public static bool operator == ( StatusMessage aMsg, StatusMessage bMsg ) {
//			return  m_strMessage == aMsg.m_strMessage    &&
//				aMsg.m_cPriority == bMsg.m_cPriority     &&
//				aMsg.m_iTdmfCmd  == bMsg.m_iTdmfCmd  ;
//		}
//		//inequality operator
//		public static bool operator != ( StatusMessage aMsg, StatusMessage bMsg ) {
//			return !( aMsg == bMsg );
//		}



	}
}
