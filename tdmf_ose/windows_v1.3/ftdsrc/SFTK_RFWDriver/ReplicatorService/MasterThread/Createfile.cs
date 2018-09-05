/*****************************************************************************
 *                                                                           *
 *  This software is the licensed software of Softek			             *
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
// March 2003
//
using System;
using System.Runtime.InteropServices;
using System.IO;
using System.Diagnostics;
using System.Runtime.Remoting.Contexts;
using System.Collections;
using System.Runtime.Serialization;

namespace MasterThread {
	/// <summary>
	/// Summary description for CreateFile.
	/// </summary>
	public class Createfile {

		public static string PHYSICALDRIVE = "\\\\.\\PHYSICALDRIVE";
		public static string DMVOLUME = "\\Device\\HarddiskDmVolumes\\PhysicalDmVolumes\\RawVolume";

		public enum eFileShare {
			READ = 0x01,
			WRITE = 0x02,
			DELETE = 0x04,
		}
 
		public enum eFileCreate {
			CREATE_NEW = 1,
			CREATE_ALWAYS = 2,
			OPEN_EXISTING = 3,
			OPEN_ALWAYS = 4,
			TRUNCATE_EXISTING = 5,
		}

		protected enum eFileAccess {
			READ = unchecked ((int)0x80000000),
			WRITE = 0x40000000,
			EXECUTE = 0x20000000,
			ALL = 0x10000000,
		}

		protected enum eFileAttribute {
			NORMAL = 0x00000080
		}

		protected enum eSeek {
			BEGIN	= 0,
			CURRENT	= 1,
			END		= 2
		}
		protected enum eSeekError {
			INVALID_SET_FILE_POINTER = -1
		}


		/// <summary>
		/// Constructor
		/// </summary>
		public Createfile() {
		}

		[DllImport("kernel32", SetLastError=true)]
		static extern unsafe IntPtr CreateFile (
			string filename,
			eFileAccess desiredAccess,
			eFileShare shareMode,
			uint attributes,   // really SecurityAttributes pointer
			eFileCreate creationDisposition,
			eFileAttribute flagsAndAttributes,
			uint templateFile
		);

		public static IntPtr OpenDrive( int driveNumber ) {
			return OpenDrive( PHYSICALDRIVE + driveNumber );
		}

		/// <summary>
		/// Open a handle to the drive
		/// </summary>
		/// <param name="DriverName"></param>
		/// <returns></returns>
		public static IntPtr OpenDrive( string DrivePath ) {
			IntPtr handle = CreateFile( DrivePath,
				eFileAccess.READ | eFileAccess.WRITE,
				eFileShare.READ | eFileShare.WRITE,
				0,
				eFileCreate.OPEN_EXISTING,
				eFileAttribute.NORMAL,
				0);
			if ( handle.ToInt32() == -1 ) {
				throw new OmException( "Error: OpenDrive: Failed to open drive Path= " + DrivePath );
			}
			return handle;
		}


		//  By Saumya Tripathi 09/23/03
		/// <summary>
		/// Open a handle to the driver
		/// </summary>
		/// <param name="Devicename"></param>
		/// <returns>IntPtr</returns>
		public static IntPtr OpenHandle( string Devicename ) {
			IntPtr handle = CreateFile( Devicename,
				eFileAccess.READ | eFileAccess.WRITE,
				eFileShare.READ | eFileShare.WRITE,
				0,
				eFileCreate.OPEN_EXISTING,
				0,
				0);
			if ( handle.ToInt32() == -1 ) {
				throw new OmException( "Error: OpenDrive: Failed to open driver Path= " + Devicename );
			}
			return handle;
		}

		[DllImport("kernel32", SetLastError=true)]
		static extern int SetFilePointer(
			IntPtr hFile,
			int lDistanceToMove,
			ref int lpDistanceToMoveHigh,
			eSeek dwMoveMethod
		);

		/// <summary>
		/// Seek to the offset within the file.
		/// </summary>
		/// <param name="handle"></param>
		/// <param name="offset"></param>
		/// <returns></returns>
		public static int Seek ( IntPtr handle, long offset ) {
			int ret;
			int offsetHigh = 0;
			int offsetLow;

			ret = SetFilePointer ( handle, 0, ref offsetHigh, eSeek.BEGIN );
			
			offsetLow = (int)offset;
			offsetHigh = (int)(offset >> 32);

			ret = SetFilePointer ( handle, offsetLow, ref offsetHigh, eSeek.BEGIN );

			if ( ret == (int)eSeekError.INVALID_SET_FILE_POINTER ) {
				throw new OmException( "Error: Seek: SetFilePointer Failed, Last Error= "  + Marshal.GetLastWin32Error() );
			}
			return ret;

		}

		[DllImport("kernel32", SetLastError=true)]
		static extern bool ReadFile(
			IntPtr hFile,
			IntPtr lpBuffer,
			int nNumberOfBytesToRead,
			ref int lpNumberOfBytesRead,
			IntPtr lpOverlapped
		);


		/// <summary>
		/// Read the file data in to the byte buffer.
		/// </summary>
		/// <param name="handle"></param>
		/// <param name="buffer"></param>
		public static void Read ( IntPtr handle, ref byte[] Buffer ) {
			int bytesRead = 0;
			IntPtr buffer;
			buffer = Marshal.AllocHGlobal( Buffer.Length );
			if ( !ReadFile( handle, buffer, Buffer.Length, ref bytesRead, IntPtr.Zero ) ) {
				throw new OmException( "Error: Createfile: Read: ReadFile failed, Last Error= "  + Marshal.GetLastWin32Error() );
			}
			IntPtr buf = buffer;
			for ( int i = 0 ; i < Buffer.Length ; i++ ) {
				Buffer[i] = Marshal.ReadByte( buf );
				buf = (IntPtr)( buf.ToInt32() + 1 );
			}
			Marshal.FreeHGlobal( buffer );
		}


		/// <summary>
		/// Open a handle to the driver
		/// </summary>
		/// <param name="DriverName"></param>
		/// <returns></returns>
		public static IntPtr OpenDriver( string DriverName ) {
			IntPtr handle = CreateFile( DriverName,
				eFileAccess.READ,
				eFileShare.READ | eFileShare.WRITE,
				0,
				eFileCreate.OPEN_ALWAYS,
				eFileAttribute.NORMAL,
				0);
			if ( handle == IntPtr.Zero ) {
				throw new OmException( "Error: OpenDriver: Failed to open driver name= " + DriverName );
			}
			return handle;
		}

		[DllImport("kernel32", SetLastError=true)]
		static extern unsafe bool CloseHandle(IntPtr Handle);

		/// <summary>
		/// Close a handle created by open
		/// </summary>
		/// <param name="handle"></param>
		/// <returns></returns>
		public static int Close( IntPtr handle ) {
			if( handle != IntPtr.Zero ) {
				CloseHandle( handle );
				handle = IntPtr.Zero;
			}
			return Marshal.GetLastWin32Error();
		}


	}
}
