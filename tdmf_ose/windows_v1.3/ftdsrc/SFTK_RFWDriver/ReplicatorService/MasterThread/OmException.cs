/*****************************************************************************
 *                                                                           *
 *  This software is the licensed software of Fujitsu Software               *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2002, 2003 by Fujitsu Software Technology Corporation      *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************/

//
// Written by Mike Pollett
// May 2004
//


using System;
using System.IO;
using System.Windows.Forms;
using System.Runtime.Serialization;
//using System.Web.Mail;
using System.Diagnostics;
using System.Threading;

namespace MasterThread
{
	/// <summary>
	/// Summary description for OmException.
	/// </summary>
	[Serializable]
	public class OmException : ApplicationException {

		public const string OM_Error = "OM_Error_";

		private static EventLog m_eventLog = new EventLog("Softek Event Log", ".", "Softek SPE OM");

		protected DateTime Date = DateTime.Now;

		protected string m_Stacktrace;

//		private static bool m_Entered;

		protected string m_ErrorFileName = "";
		protected string m_ZipErrorFileName;
		protected string m_ErrorMessageFileName;
		protected string m_FilePath;

		public string m_AliasMessage;

		// holds the Marshal.GetLastError may not always be valid.
		public int GetLastError = 0;

		/// <summary>
		/// default constructor
		/// </summary>
		public OmException ()
		{
		}

		/// <summary>
		/// constructor for de-serialization
		/// </summary>
		/// <param name="info"></param>
		/// <param name="context"></param>
		public OmException ( SerializationInfo info, StreamingContext context ) 
			: base ( info, context ) {}


		/// <summary>
		/// construtor
		/// </summary>
		/// <param name="message"></param>
		public OmException (String message) : base ( String.Format( "{0}: {1}: {2}: {3} ", SystemInformation.ComputerName, SystemInformation.UserName, DateTime.Now, message ) )
		{ 
		}

		/// <summary>
		/// constructor
		/// </summary>
		/// <param name="message"></param>
		/// <param name="inner"></param>
		public OmException (String message, Exception inner) : base( String.Format(  "{0}: {1}: {2}: {3} ", SystemInformation.ComputerName, SystemInformation.UserName, DateTime.Now, message ), inner )
		{
		}

		/// <summary>
		/// Stack trace sting.
		/// </summary>
		public string Stacktrace
		{
			get { return m_Stacktrace; }
		}
 
//		public static void LogException( Region region, OmException e )
//		{
//			LogException( region, e, EventLogEntryType.Error );
//		}

//		public static void LogException( Region region, OmException e, EventLogEntryType reporting )
//		{
//			if ( m_Entered ) { return; }
//
//			OmExceptionCollection Exps = null;
//
//			try {
//				m_Entered = true;
//				Softek.Virtualization.Server.ConfigurationProcessor cfgProc = new Softek.Virtualization.Server.ConfigurationProcessor();
//				e.m_FilePath = cfgProc.m_FilePath;
//				e.m_ErrorFileName = ErrorFilename( e.m_FilePath );
//				e.m_ZipErrorFileName = e.m_ErrorFileName;
//
//				// save the OM.xml for an attachment
//				try {
//					if ( region != null ) {
//						string message = e.Message;
//						string AliasMsg = "";
//						string alias;
//						string [] split = message.Split( ' ' );
//						foreach ( string str in split ) {
//							if ( str.Length > 10 ) {
//								// remove all commas ...
//								char lastchar = (char)0;
//								string s;
//								switch ( str[str.Length - 1] ) {
//									case ',':
//										lastchar = ',';
//										break;
//									case ':':
//										lastchar = ':';
//										break;
//									case ';':
//										lastchar = ';';
//										break;
//								}
//								if ( lastchar != 0 ) {
//									s = str.Remove( str.Length - 1, 1 );
//								} else {
//									s = str;
//								}
//								alias = region.LookUpAlias( new ID( s ) );
//							} else {
//								alias = "";
//							}
//							if (( alias == null ) || ( alias == "" )) {
//								AliasMsg += str + " ";
//							} else {
//								AliasMsg += alias + " ";
//							}
//						}
//						e.m_AliasMessage = AliasMsg;
//						Exps = region.OmExceptions;
//						string [] fileNames = Directory.GetFiles( e.m_FilePath, OM_Error + "*.XML" );
//						if (( fileNames != null ) && ( fileNames.Length > 30 )) {
//							// get all of the file dates and delete the oldest ones
//
//							int count = 0;
//							DateTime[] createTimes = new DateTime[ fileNames.Length ];
//							foreach ( string name in fileNames ) {
//								createTimes[count++] = File.GetCreationTime( name );
//							}
//							Array.Sort( createTimes, fileNames );
//							// now delete the old files
//							for ( int index = 0 ; index < (createTimes.Length - 30)  ; index++ ) {
//								File.Delete( Path.Combine ( e.m_FilePath, fileNames[index] ) );
//								try {
//									File.Delete( Path.Combine ( e.m_FilePath, fileNames[index] + ".zip" ) );
//								} catch ( Exception ) {}
//								try {
//									File.Delete( Path.Combine ( e.m_FilePath, fileNames[index] + ".txt" ) );
//								} catch ( Exception ) {}
//							}
//						}
//						cfgProc.SaveObjectModelXML( region, e.m_ErrorFileName );
//					}
//
//				} catch ( Exception ) {
////						Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "Save OM as XML Filed: " + e2.Message );
//				}
//				LogException( Exps, e, reporting );
//			} finally {
//				m_Entered = false;
//			}
//		}

		/// <summary>
		/// Will add the exception into the Region.OmExceptions collection
		/// and trace the exception
		/// and place it into the Event log.
		/// </summary>
		/// <param name="OmExceptionCollection"></param>
		/// <param name="e"></param>
		public static void LogException( OmExceptionCollection OmExceptions, OmException e )
		{
			LogException( OmExceptions, e, EventLogEntryType.Error );
		}

		public static void LogException( OmExceptionCollection OmExceptions, OmException e, EventLogEntryType reporting )
		{
			// add it to the Exception collection
			if ( OmExceptions != null ) {
				OmExceptions.Add( e );
			}
			LogException( e, reporting );
		}

		public static void LogException( OmException e )
		{
			LogException( e, EventLogEntryType.Error );
		}

		public static void LogException( OmException e, EventLogEntryType reporting )
		{
			try {
				// get the build number and display
				string build = "2.2";
				try {
//					build = RegistryAccess.BuildNumber();
				} 
				catch ( Exception ) { 
					build = "NOT FOUND";
				}
				string company = "Softek";
				try {
//					company = RegistryAccess.Company();
				} 
				catch ( Exception ) { 
					company = "NOT FOUND";
				}

				// save the stack trace information
				string errorMessage;
				string date;
				date = File.GetLastWriteTime( Application.StartupPath + "/MasterThread.dll" ).ToString();

				e.m_Stacktrace = e.StackTrace;
				errorMessage = String.Format( "Company: {6}, Build: {3}, Service Version: {4}, Build Date: {5}, Exception Caught: {0}\r\n : {1}\r\n: {2}\n\r >> {7}\n\r", e.HResult, e.Message, e.StackTrace, build, Application.ProductVersion, date, company, e.m_AliasMessage );
				if ( e.GetBaseException() != null ) {
					string Stacktrace = "\r\n Extended Exception Stack Trace: " + e.GetBaseException().StackTrace;
					errorMessage += "\r\n" + String.Format("BaseException Caught: {0}\n : {1}\n: {2}\n >> {3}\n ", e.GetBaseException().HelpLink, e.GetBaseException().Message, Stacktrace, e.m_AliasMessage );
				}

				// trace the Exception
				Trace.WriteLineIf ( true, errorMessage );

				// save the error message to a file
				if ( e.m_ErrorFileName != "" ) {
					e.m_ErrorMessageFileName = e.m_ErrorFileName +  ".txt";
					try {
						StreamWriter ErrorMessageFile = File.CreateText( e.m_ErrorMessageFileName );
						try {
							ErrorMessageFile.Write( "Monach Middleware Error Mail:\r\n\r\n" + errorMessage ); 
						} finally {
							ErrorMessageFile.Close();
						}
					} catch ( Exception ) {
						// do nothing
					}
				}

				// By Saumya 08/20/02
				m_eventLog.WriteEntry(errorMessage, reporting );


				//				// Create the zip file
				//				// Zip the file up
				//				string ZipFileName = RegistryAccess.InstallPath() + "/Program/ZipIt.exe";
				//				if ( File.Exists( ZipFileName ) ) {
				//					m_ZipErrorFileName = m_ErrorFileName +  ".zip";
				//					System.Diagnostics.Process process = new System.Diagnostics.Process();
				//					try 
				//					{
				//						process.StartInfo.FileName = ZipFileName;
				//						string args;
				//						args = "\"" + m_ErrorMessageFileName;
				//						string name = m_ErrorFileName;
				//						if ( File.Exists( name ) ) 
				//						{
				//							args += " | " + name;
				//						}
				//						name = Path.Combine( m_FilePath, "SSPconfig.txt" );
				//						if ( File.Exists( name ) ) 
				//						{
				//							args += " | " + name;
				//						}
				//						name = Path.Combine( m_FilePath, "SSPconfigBak.txt" );
				//						if ( File.Exists( name ) ) 
				//						{
				//							args += " | " + name;
				//						}
				//						name = Path.Combine( m_FilePath, "OM.XML" );
				//						if ( File.Exists( name ) ) 
				//						{
				//							args += " | " + name;
				//						}
				//						args += "\"";
				//						process.StartInfo.Arguments = args + " \"" + m_ZipErrorFileName + "\"";
				//						process.Start();
				//						process.WaitForExit( 1000 * 60 * 2 );
				//					} 
				//					catch ( Exception ) {
				//						//do nothing
				//					}
				//					finally 
				//					{
				//						process.Close();
				//					}
				//				}

				// By Mike Pollett add email error information
				if ( reporting != EventLogEntryType.Error ) {
					return;
				}
				try {
					//					m_myMail = new MailMessage();
					//					m_myMail.From = "MonarchMiddleware@softek.com";
					//					m_myMail.To = "ssplogs@softek.com";
					//					m_myMail.Subject = "Error: " + e.Message.Substring(e.Message.LastIndexOf(':'));
					//					m_myMail.Priority = MailPriority.Normal;
					//					m_myMail.BodyFormat = MailFormat.Text;
					//					m_myMail.Body = "Monach Middleware Error Mail:\r\n\r\n" + errorMessage;
					//					if ( m_ZipErrorFileName != null ) {
					//						MailAttachment attachment = new MailAttachment( m_ZipErrorFileName );
					//						m_myMail.Attachments.Add( attachment );
					//						Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "Email with Attachment: " + m_ZipErrorFileName );
					//						m_ErrorFileName = null;
					//						m_ZipErrorFileName = null;
					//					} 
					//					else {
					//						Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "Email with OUT Attachment." );
					//					}

					// Create the worker thread.
//					EmailWorker emailWorker = new EmailWorker();
//					// save the needed parameters
//					emailWorker.m_ErrorFileName = e.m_ErrorFileName;
//					emailWorker.m_ZipErrorFileName = e.m_ZipErrorFileName;
//					emailWorker.m_ErrorMessageFileName = e.m_ErrorMessageFileName;
//					emailWorker.m_FilePath = e.m_FilePath;
//					emailWorker.e = e;
//					emailWorker.m_errorMessage = errorMessage;
//					// start the thread
//					(new Thread(new ThreadStart( emailWorker.Email ))).Start();
				} 
				catch ( Exception e1 ) {
					Trace.WriteLineIf( true, e1.Message );
				}
			} 
			catch ( Exception ) {
				return;
			}
		}

		/// <summary>
		/// Build the error file name
		/// </summary>
		/// <param name="path"></param>
		/// <returns></returns>
		private static string ErrorFilename( string path )
		{
			string name;
			string time;
			time = DateTime.Now.ToString( "MM-dd-yy HH.mm.ss.fffffff" );
			if (( path == null ) || ( path == "" )) {
				path = "C:/";
			}
			name = Path.Combine( path , OM_Error + time + ".XML");
			// make sure it is a unique name
again:
			if ( File.Exists( name ) ) { 
				name += name.Substring ( 0, name.Length - 4 ) + "@.XML";
				goto again;
			}
			return name;
		}
	}

//	public class EmailWorker
//	{
//		public MailMessage m_myMail;
//		public string m_ErrorFileName;
//		public string m_ZipErrorFileName;
//		public string m_ErrorMessageFileName;
//		public string m_FilePath;
//		public OmException e;
//		public string m_errorMessage;
//
//		protected static string locked = "";
//
//		public void Email()
//		{
//			try {
//				lock ( locked ) {
//					// test if enabled
//					if ( !RegistryAccess.IsPhoneHome() ) {
//						return;
//					}
//
//					// Create the zip file
//					// Zip the file up
//					string ZipFileName = RegistryAccess.InstallPath() + "/Program/ZipIt.exe";
//					if ( File.Exists( ZipFileName ) ) {
//						if (( m_ErrorFileName != null ) &&
//							( m_ErrorFileName != "" )) {
//							m_ZipErrorFileName = m_ErrorFileName +  ".zip";
//							System.Diagnostics.Process process = new System.Diagnostics.Process();
//							try 
//							{
//								process.StartInfo.FileName = ZipFileName;
//								string args;
//								args = "\"" + m_ErrorMessageFileName;
//								string name = m_ErrorFileName;
//								if ( File.Exists( name ) ) 
//								{
//									args += " | " + name;
//								}
//								name = Path.Combine( m_FilePath, "SSPconfig.txt" );
//								if ( File.Exists( name ) ) 
//								{
//									args += " | " + name;
//								}
//								name = Path.Combine( m_FilePath, "SSPconfigBak.txt" );
//								if ( File.Exists( name ) ) 
//								{
//									args += " | " + name;
//								}
//								name = Path.Combine( m_FilePath, "OM.XML" );
//								if ( File.Exists( name ) ) 
//								{
//									args += " | " + name;
//								}
//								args += "\"";
//								process.StartInfo.Arguments = args + " \"" + m_ZipErrorFileName + "\"";
//								process.Start();
//								process.WaitForExit( 1000 * 60 * 2 );
//							} 
//							catch ( Exception ) 
//							{
//								//do nothing
//							}
//							finally 
//							{
//								process.Close();
//							}
//						}
//					}
//				}
//				MailMessage myMail = new MailMessage();
//				myMail.From = "SSPMiddleware@softek.fujitsu.com";
//				myMail.To = "ssplogs@softek.fujitsu.com";
//				myMail.Subject = "Error: " + e.Message.Substring(e.Message.LastIndexOf(':'));
//				myMail.Priority = MailPriority.Normal;
//				myMail.BodyFormat = MailFormat.Text;
//				myMail.Body = "SSP Middleware Error Mail:\r\n\r\n" + m_errorMessage;
//				if ( m_ZipErrorFileName != null ) {
//					MailAttachment attachment = new MailAttachment( m_ZipErrorFileName );
//					myMail.Attachments.Add( attachment );
//					Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "Email with Attachment: " + m_ZipErrorFileName );
//					m_ErrorFileName = null;
//					m_ZipErrorFileName = null;
//				} 
//				else {
//					Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "Email with OUT Attachment." );
//				}
//
//				// get the pop address from a registry.
//				string popServer = RegistryAccess.GetPopServer();
//				if ( popServer == null ) {
//					SmtpMail.SmtpServer = "exchange.amdahl.com";
//				} else {
//					SmtpMail.SmtpServer = popServer;
//				}
//				// get the to address.
//				string toAddress = RegistryAccess.GetEmailToAddress();
//				if (( toAddress != null ) && ( toAddress != "" )) {
//					myMail.To = toAddress;
//				}
//
//				// send it
//				SmtpMail.Send( myMail );
//			} catch ( Exception e1 ) {
//				Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "EmailWorker: Caught exception: " + e1.Message + "\nStack Trace: \n" + e1.StackTrace );
//				try {
//					RegistryAccess.EmailFailed();
//				} catch ( Exception e2 ) {
//					Trace.WriteLineIf( MonarchTraceSwitch.Switch.TraceError, "EmailWorker: Caught exception: Emaili failed!" + e2.Message );
//				}
//			}
//		}
//	}

}
