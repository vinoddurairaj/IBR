//using System;
//
//namespace MasterThread
//{
//	/// <summary>
//	/// Summary description for LogicalGroupDevice.
//	/// </summary>
//	public class LogicalGroupIOCTL
//	{
//		IntPtr m_DriveHandle;
//		int m_Lgnum;
//
//		public static string MSG_INCO = "INCOHERENT"; // traditional SmartRefresh is using Journal
//		public static string MSG_CO = "COHERENT";
//		public static string MSG_CPON = "CHECKPOINT_ON";
//		public static string MSG_CPOFF = "CHECKPOINT_OFF";
//
//
//		public LogicalGroupIOCTL( int lgnum )
//		{
//			// get a handle to the driver
//			m_Lgnum =  lgnum;
//			m_DriveHandle = Createfile.OpenDriver( Name( lgnum ) );
//		}
//
//		~LogicalGroupIOCTL() {
//			if ( m_DriveHandle != IntPtr.Zero ) {
//				Createfile.Close ( m_DriveHandle );
//				m_DriveHandle = IntPtr.Zero;
//			}
//		}
//		public void Close() {
//			if ( m_DriveHandle != IntPtr.Zero ) {
//				Createfile.Close ( m_DriveHandle );
//				m_DriveHandle = IntPtr.Zero;
//			}
//		}
//
//		public static string Name( int lgnum ) {
//			return "\\\\.\\Global\\DTC\\lg" + lgnum;
//		}
//
//		/// <summary>
//		/// ftd_ioctl_send_lg_message -- Insert a sentinel into the BAB.
//		/// </summary>
//		/// <param name="devfd"></param>
//		/// <param name="lgnum"></param>
//		/// <param name="sentinel"></param>
//		/// <returns></returns>
////		public int ftd_ioctl_send_lg_message( string sentinel ) {
////			
////			//			stat_buffer_t	sb;
////			//			string msgbuf;
////			//			int maxtries, rc, i, sent, errnum;
////			//
////			//			memset(msgbuf, 0, sizeof(msgbuf));
////			//			strcpy(msgbuf, sentinel); 
////			//			memset(&sb, 0, sizeof(stat_buffer_t));
////			//    
////			//			sb.lg_num = lgnum;
////			//			sb.len = sizeof(msgbuf);
////			//			sb.addr = msgbuf;
////			//
////			//			maxtries = 50;
////			//			sent = 0;
////			//    
////			//			for (i = 0; i < maxtries; i++) {
////			//				rc = ftd_ioctl(devfd, FTD_SEND_LG_MESSAGE, &sb, sizeof(stat_buffer_t));
////			//				if (rc != 0) {
////			//					errnum = ftd_errno();
////			//					if (errnum == EAGAIN) {
////			//						sleep(50);
////			//						continue;
////			//					}
////			//					if (errnum == ENOENT) {
////			//						reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, "BAB FULL - cannot allocate entry\n");
////			//						return -1;
////			//					}
////			//					else {
////			//						reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, strerror(errnum));
////			//						return -1;
////			//					}
////			//				} 
////			//				else {
////			//					sent = 1;
////			//					break;
////			//				}
////			//			}
////			//
////			//			if ( !sent ) {
////			//				reporterr(ERRFAC, M_SENDLGMSG, ERRWARN, "BAB busy clearing memory - cannot allocate entry\n");
////			//				return -1;
////			//			}
////
////			return 0;
////		}
//
//
//
//
//
//
//
//	}
//}
