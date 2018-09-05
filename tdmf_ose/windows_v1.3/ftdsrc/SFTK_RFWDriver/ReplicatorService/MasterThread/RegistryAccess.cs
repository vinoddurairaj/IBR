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
// May 2002
//
// Modified by Saumya Tripathi
// September 2002
//

using System;
using System.IO;
using System.Diagnostics;
using Microsoft.Win32;


namespace MasterThread
{
	/// <summary>
	/// Summary description for RegistryAccess.
	/// </summary>
	public class RegistryAccess
	{


#if (OEMNAME)
		public static string CompanyName = "OEMNAME";
#else
		public static string CompanyName = "Softek";
#endif

#if (OEMNAME)
		public static string MasterName = "OEMNAME";
#else
		public static string MasterName = "DtcServer";
#endif

#if (OEMNAME)
		public static string DRIVERNAME = "Dtcblock";
#else
		public static string DRIVERNAME = "Dtcblock";
#endif

		public static int MASTER_THREAD_PORT = 575;			// default
		public static int COLLECTOR_PORT = 576;				// default
		public static int TCPWINDOW_SIZE = 256 * 1024;		// default

		public static string MASTER_PATH = "Software\\" + CompanyName + "\\Dtc\\CurrentVersion\\" + MasterName;
		public static string SOFTWARE_PATH = "Software\\" + CompanyName + "\\Dtc\\CurrentVersion";
		public static string DRIVER_KEY = "System\\CurrentControlSet\\Services\\" + DRIVERNAME;



		/// <summary>
		/// Constructor
		/// </summary>
		public RegistryAccess()
		{
		}


		/// <summary>
		/// returns the Master thread Port number to use
		/// </summary>
		/// <returns></returns>
		public static int MasterPort() {
			try {
				return (int)KeyData( MASTER_PATH, "port" );
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				// OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return MASTER_THREAD_PORT;
			}
		}
		/// <summary>
		/// returns the Master thread Port number to use
		/// </summary>
		/// <returns></returns>
		public static void MasterPort( int data ) {
			KeyData( SOFTWARE_PATH, "port", data );
		}

		/// <summary>
		/// returns the Collector IP address to use
		/// </summary>
		/// <returns></returns>
		public static string CollectorIP() {
			try {
				string ip = (string)KeyData( SOFTWARE_PATH, "DtcCollectorIP" );
				if ( ip == null ) {
					return "0.0.0.0";
				}
				return ip;
			}
			catch ( Exception ) {
				return "0.0.0.0";
			}
		}
		/// <summary>
		/// returns the Collector IP address to use
		/// </summary>
		/// <returns></returns>
		public static void CollectorIP( string ip ) {
			KeyData( SOFTWARE_PATH, "DtcCollectorIP", ip );
		}

		/// <summary>
		/// returns the Collector Port number to use
		/// </summary>
		/// <returns></returns>
		public static int CollectorPort() {
			try {
				return (int)KeyData( SOFTWARE_PATH, "DtcCollectorPort" );
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				// OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return COLLECTOR_PORT;
			}
		}


		/// <summary>
		/// returns the TCPIP Window size to use
		/// </summary>
		/// <returns></returns>
		public static int TCPWindowSize() {
			try {
				return (int)KeyData( MASTER_PATH, "tcp_window_size" );
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return TCPWINDOW_SIZE;
			}
		}
		/// <summary>
		/// returns the TCPIP Window size to use
		/// </summary>
		/// <returns></returns>
		public static void TCPWindowSize( int size ) {
			KeyData( MASTER_PATH, "tcp_window_size", size );
		}


		/// <summary>
		/// returns the InstallPath
		/// </summary>
		/// <returns></returns>
		public static string InstallPath() {
			try {
				string path = (string)KeyData( MASTER_PATH, "InstallPath" );
				if ( path == null ) {
					return Directory.GetCurrentDirectory();
				}
				return path;
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return "";
			}
		}


		/// <summary>
		/// returns the PerfUploadPeriod
		/// </summary>
		/// <returns></returns>
		public static int PerfUploadPeriod() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "PerfUploadPeriod" );
				if ( tmp == null ) {
					return 100;
				}
				return Convert.ToInt32( tmp );
			}
			catch ( Exception ) {
				return 100;
			}
		}
		/// <summary>
		/// returns the PerfUploadPeriod
		/// </summary>
		/// <returns></returns>
		public static void PerfUploadPeriod( int data ) {
			KeyData( MASTER_PATH, "PerfUploadPeriod", data.ToString() );
		}


		/// <summary>
		/// returns the ReplGroupMonitUploadPeriod
		/// </summary>
		/// <returns></returns>
		public static int ReplGroupMonitUploadPeriod() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "ReplGroupMonitUploadPeriod" );
				if ( tmp == null ) {
					return 100;
				}
				return Convert.ToInt32( tmp );
			}
			catch ( Exception ) {
				return 100;
			}
		}
		/// <summary>
		/// returns the ReplGroupMonitUploadPeriod
		/// </summary>
		/// <returns></returns>
		public static void ReplGroupMonitUploadPeriod( int data ) {
			KeyData( MASTER_PATH, "ReplGroupMonitUploadPeriod", data.ToString() );
		}

		/// <summary>
		/// returns the TDMFAgentEmulator
		/// </summary>
		/// <returns></returns>
		public static bool TDMFAgentEmulator() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "TDMFAgentEmulator" );
				if (( String.Compare( tmp, "true", true ) == 0 ) ||
					( String.Compare( tmp, "yes", true ) == 0 ) ||
					( String.Compare( tmp, "1", true ) == 0 )) {
					return true;
				}
				return false;
			}
			catch ( Exception ) {
				return false;
			}
		}

		/// <summary>
		/// returns the EmulatorRangeMin
		/// </summary>
		/// <returns></returns>
		public static int EmulatorRangeMin() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "EmulatorRangeMin" );
				if ( tmp == null ) {
					return 1;
				}
				return Convert.ToInt32( tmp );
			}
			catch ( Exception ) {
				return 1;
			}
		}

		/// <summary>
		/// returns the EmulatorRangeMax
		/// </summary>
		/// <returns></returns>
		public static int EmulatorRangeMax() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "EmulatorRangeMax" );
				if ( tmp == null ) {
					return 10;
				}
				return Convert.ToInt32( tmp );
			}
			catch ( Exception ) {
				return 10;
			}
		}

		/// <summary>
		/// returns the EmulUnix
		/// </summary>
		/// <returns></returns>
		public static bool Emulator() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "EmulUnix" );
				if ( tmp == null ) {
					return false;
				}
				return Convert.ToInt32( tmp ) > 0;
			}
			catch ( Exception ) {
				return false;
			}
		}

		/// <summary>
		/// returns the TraceLevel
		/// </summary>
		/// <returns></returns>
		public static int TraceLevel() {
			try {
				string tmp = (string)KeyData( SOFTWARE_PATH, "TraceLevel" );
				if ( tmp == null ) {
					return 0;
				}
				return Convert.ToInt32( tmp );
			}
			catch ( Exception ) {
				return 0;
			}
		}

		/// <summary>
		/// returns the Licence
		/// </summary>
		/// <returns></returns>
		public static string Licence() {
			try {
				string license = (string)KeyData( SOFTWARE_PATH, "license" );
				if ( license == null ) {
					return "";
				}
				return license;
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return "";
			}
		}
		/// <summary>
		/// returns the Licence
		/// </summary>
		/// <returns></returns>
		public static void Licence( string license ) {
			KeyData( SOFTWARE_PATH, "license", license );
		}

		/// <summary>
		/// returns the num_chunks
		/// </summary>
		/// <returns></returns>
		public static int NumberChunks() {
			try {
				string data = (string)KeyData( DRIVER_KEY, "num_chunks" );
				if ( data == null ) {
					return 0;
				}
				return Convert.ToInt32( data );
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return 0;
			}
		}
		/// <summary>
		/// returns the num_chunks
		/// </summary>
		/// <returns></returns>
		public static void NumberChunks( int chunks ) {
			KeyData( DRIVER_KEY, "num_chunks", chunks.ToString() );
		}		

		/// <summary>
		/// returns the chunk_size
		/// </summary>
		/// <returns></returns>
		public static int ChunkSize() {
			try {
				string data = (string)KeyData( DRIVER_KEY, "chunk_size" );
				if ( data == null ) {
					return 0;
				}
				return Convert.ToInt32( data );
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return 0;
			}
		}
		/// <summary>
		/// returns the chunk_size
		/// </summary>
		/// <returns></returns>
		public static void ChunkSize( int size ) {
			KeyData( DRIVER_KEY, "chunk_size", size.ToString() );
		}	

		/// <summary>
		/// returns the Domain
		/// </summary>
		/// <returns></returns>
		public static string Domain() {
			try {
				string data = (string)KeyData( SOFTWARE_PATH, "DtcDomain" );
				if ( data == null ) {
					return "unassigned domain";
				}
				return data;
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				return "unassigned domain";
			}
		}
		/// <summary>
		/// returns the Domain
		/// </summary>
		/// <returns></returns>
		public static void Domain( string data ) {
			KeyData( SOFTWARE_PATH, "DtcDomain", data );
		}	

		/// <summary>
		/// returns the IPTranslation
		/// </summary>
		/// <returns></returns>
		public static string IPTranslation() {
			try {
				string data = (string)KeyData( SOFTWARE_PATH, "IPTranslation" );
				if ( data == null ) {
					IPTranslation ( "off" );
					return "off";
				}
				return data;
			}
			catch ( Exception ) {
				// don't log the exception since this is the norm.
				//				OmException.LogException( (Region)null,	new OmException( String.Format("RegistryAccess : Registry entry 'RemotingPort' not found." ), e) );
				IPTranslation ( "off" );
				return "off";
			}
		}
		/// <summary>
		/// returns the IPTranslation
		/// </summary>
		/// <returns></returns>
		public static void IPTranslation( string data ) {
			KeyData( SOFTWARE_PATH, "DtcDomain", data );
		}	
		
		/// <summary>
		/// returns the CPUs
		/// </summary>
		/// <returns></returns>
		public static int CPUCount() {
			try {
				RegistryKey regKey = Registry.LocalMachine ;
				regKey = regKey.OpenSubKey( "HARDWARE\\DESCRIPTION\\System\\CentralProcessor" );
				if ( regKey == null ) {
					return 1;
				}
				try {
					string [] data = regKey.GetSubKeyNames();
                    return data.Length;
				}
				finally {
					regKey.Close();
				}
			}
			catch ( Exception ) {
				return 1;
			}
		}








		/// <summary>
		/// Helper methods to get and set key values
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		public static object KeyData( string path, string name )
		{
			RegistryKey regKey = Registry.LocalMachine ;
			regKey = regKey.OpenSubKey( path );
			if ( regKey == null ) {
				return null;
			}
			try
			{
				object data = regKey.GetValue( name );
				return data;
			}
			finally
			{
				regKey.Close();
			}
		}
		public static void KeyData( string path, string name, object data )
		{
			RegistryKey regKey = Registry.LocalMachine ;
			// Modified by Saumya 09/17/02
			regKey = regKey.OpenSubKey( path, true );
			if ( regKey == null ) {
				// need to write the key
				regKey = Registry.LocalMachine ;
				regKey = regKey.CreateSubKey( path );
			}
			try {
				regKey.SetValue( name, data );
			} 
			finally 
			{
				regKey.Close();
			}
		}


	}
}
