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
// December 2002
//

using System;
using System.IO;
using System.Runtime.InteropServices;

namespace MasterThread
{
	/// <summary>
	/// This class can be used as a base class to give another class IOCTL support.
	/// </summary>
	public class DeviceIOControl : IDisposable {

		private static Int32 CTL_CODE( eIOCTL DeviceType, int Function, eMETHOD Method, eFILE_ACCESS Access ) {
			return ((((int)DeviceType) << 16) | (((int)Access) << 14) | ((Function) << 2) | ((int)Method));
		}

		// Modified by Saumya Tripathi 06/18/03
		private enum eMETHOD {
			METHOD_BUFFERED = 0,
			METHOD_IN_DIRECT = 1,
			METHOD_OUT_DIRECT = 2,
			METHOD_NEITHER = 3
		}
		private enum eFILE_ACCESS {
			FILE_ANY_ACCESS = 0,
			FILE_READ_ACCESS = 0x0001,
			FILE_WRITE_ACCESS = 0x0002
		}

		protected enum eIOCTL {
			IOCTL_SCSI_BASE = 4,
			IOCTL_DISK_BASE = 7,
			IOCTL_VOLUME_BASE = 'V',
			IOCTL_HA_BASE = 0x8000,
			DCSPMF_DEVICE_TYPE = 40001,
			FILE_DEVICE_UNKNOWN = 0x00000022
		}

		// Replicator Driver IOCTL Commands
		const int REPLICATOR_IOCTL_INDEX = 0x0800;
		public static int IOCTL_GET_LG_STATE_BUFFER = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x08, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SET_LG_STATE_BUFFER = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x15, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_GET_GROUP_STATS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x1c, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_SET_GROUP_STATE = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x1d, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_GET_GROUP_STATE = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x22, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_GET_BAB_SIZE = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x23, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SET_IODELAY = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x27, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_SET_SYNC_DEPTH = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x26, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_SET_SYNC_TIMEOUT = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x28	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_SET_TRACE_LEVEL = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x29	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_GET_DEV_STATS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x1b	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_GET_DEV_STATE_BUFFER = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x07	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SET_DEV_STATE_BUFFER = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x14	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_GET_GROUPS_INFO = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x1a	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		
		public static int IOCTL_GET_NUM_GROUPS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x18	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		
		public static int IOCTL_GET_DEVICES_INFO = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x19	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_DEL_LG = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x05	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_DEL_DEVICE = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x04	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_NEW_LG = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x03	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_NEW_DEVICE = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x02	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_GET_DEVICE_NUMS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x13	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_GET_ALL_STATS_INFO = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x55	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		

		// FTD_CONFIG_BEGIN:
		// At Service starting time, Tis IOCTL is the first IOCTL gets send to Driver
		// This IOCTL mainly is used to update or synch Pstore file based configuration with
		// service Config file at system start.
		// This IOCTL must followed with list of above mentioned IOCTLS to confirm LG/Devs etc.
		// This IOCTL must also end with FTD_CONFIG_END
		public static int IOCTL_CONFIG_BEGIN = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x32	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_CONFIG_END = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x33	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		

		// TDI IOCTLs
		public static int IOCTL_SFTK_IOCTL_TCP_ADD_CONNECTIONS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x47	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_TCP_REMOVE_CONNECTIONS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x48	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_TCP_ENABLE_CONNECTIONS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x49	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_TCP_DISABLE_CONNECTIONS = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x50	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_TCP_QUERY_SM_PERFORMANCE = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x51	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_TCP_SET_CONNECTIONS_TUNABLES = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x52	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_START_PMD = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x53	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_STOP_PMD = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x54	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_START_RMD = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x55	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SFTK_IOCTL_STOP_RMD = CTL_CODE( eIOCTL.FILE_DEVICE_UNKNOWN, REPLICATOR_IOCTL_INDEX + 0x56	, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);

		// disk commands
		public static int IOCTL_SCSI_GET_ADDRESS = CTL_CODE(eIOCTL.IOCTL_SCSI_BASE, 0x0406, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_SCSI_PASS_THROUGH_DIRECT = CTL_CODE(eIOCTL.IOCTL_SCSI_BASE, 0x0405, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_READ_ACCESS | eFILE_ACCESS.FILE_WRITE_ACCESS);
		public static int IOCTL_DISK_GET_DRIVE_GEOMETRY = CTL_CODE(eIOCTL.IOCTL_DISK_BASE, 0x0000, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_DISK_GET_DRIVE_LAYOUT = CTL_CODE(eIOCTL.IOCTL_DISK_BASE, 0x0003, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_READ_ACCESS);
		public static int IOCTL_DISK_GET_DRIVE_LAYOUT_EX = CTL_CODE(eIOCTL.IOCTL_DISK_BASE, 0x0014, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_DISK_LIST_DISKGUID = CTL_CODE(eIOCTL.DCSPMF_DEVICE_TYPE, 2067, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		public static int IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS = CTL_CODE(eIOCTL.IOCTL_VOLUME_BASE, 0, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		//Added by Veera to suport Volume Total Size
		public static int IOCTL_DISK_GET_PARTITION_INFO = CTL_CODE(eIOCTL.IOCTL_DISK_BASE, 0x0001, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_READ_ACCESS);
		
		// By Saumya Tripathi 06/18/03
		// For CapConfig Apply Process
//		public static int DCSCAP_I_CONFIG	= CTL_CODE(eIOCTL.FILE_DEVICE_UNKNOWN, 0x801, eMETHOD.METHOD_OUT_DIRECT, eFILE_ACCESS.FILE_ANY_ACCESS);
		// By Saumya Tripathi 08/22/03
//		public static int DCSCAP_I_TEST		= CTL_CODE(eIOCTL.FILE_DEVICE_UNKNOWN, 0x810, eMETHOD.METHOD_OUT_DIRECT, eFILE_ACCESS.FILE_ANY_ACCESS);
//		public static int DCSCAP_I_FLUSHCONFIG	= CTL_CODE(eIOCTL.FILE_DEVICE_UNKNOWN, 0x802, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
//		public static int DCSCAP_I_PERFORMTEST	= CTL_CODE(eIOCTL.FILE_DEVICE_UNKNOWN, 0x806, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
//		public static int IOCTL_SS_RESCAN	= CTL_CODE(eIOCTL.FILE_DEVICE_UNKNOWN, 0x805, eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
		
		//for HA pause and resume recovery.
//		public static int IOCTL_DCSHA_PAUSE_RESUME_RECOVERY	= CTL_CODE(eIOCTL.IOCTL_HA_BASE, 0x800+(31), eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);
//		public static int IOCTL_DCSHA_APPLY_REGISTRY_VALUE	= CTL_CODE(eIOCTL.IOCTL_HA_BASE, 0x800+(29), eMETHOD.METHOD_BUFFERED, eFILE_ACCESS.FILE_ANY_ACCESS);

		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
		public class SCSI_PASS_THROUGH_DIRECT {
			public UInt16 Length;
			public byte ScsiStatus;
			public byte PathId;
			public byte TargetId;
			public byte Lun;
			public byte CdbLength;
			public byte SenseInfoLength;
			public byte DataIn;
			public uint DataTransferLength;
			public uint TimeOutValue;
			public IntPtr DataBuffer;
			public uint SenseInfoOffset;
			public byte Cdb0 = 0;
			public byte Cdb1 = 0;
			public byte Cdb2 = 0;
			public byte Cdb3 = 0;
			public byte Cdb4 = 0;
			public byte Cdb5 = 0;
			public byte Cdb6 = 0;
			public byte Cdb7 = 0;
			public byte Cdb8 = 0;
			public byte Cdb9 = 0;
			public byte Cdb10= 0;
			public byte Cdb11= 0;
			public byte Cdb12= 0;
			public byte Cdb13= 0;
			public byte Cdb14= 0;
			public byte Cdb15= 0;
		}


		public DeviceIOControl()
		{
			//
			// TODoO: Add constructor logic here
			//
		}
		~DeviceIOControl() {
			Dispose();
		}

		public void Dispose()
		{
			GC.SuppressFinalize ( this );
		}


		[DllImport("KERNEL32", SetLastError = true)]
		private static extern bool DeviceIoControl(
										IntPtr hDevice,				// handle to device
										Int32 dwIoControlCode,		// operation
										IntPtr lpInBuffer,		// input data buffer
										Int32 nInBufferSize,		// size of input data buffer
										IntPtr lpOutBuffer,		// output data buffer
										Int32 nOutBufferSize,		// size of output data buffer
										out Int32 lpBytesReturned,	// byte count
										IntPtr lpOverlapped			// overlapped information
										);

		/// <summary>
		/// Execute a call to DeviceIoControl
		/// </summary>
		/// <param name="file"></param>
		/// <param name="Code"></param>
		/// <param name="InBuffer"></param>
		/// <param name="InBufLength"></param>
		/// <param name="OutBuffer"></param>
		/// <param name="OutBufLength"></param>
		public static void DeviceIoCtrl( FileStream file, Int32 Code, IntPtr InBuffer, Int32 InBufLength, IntPtr OutBuffer, ref Int32 OutBufLength )
		{
			DeviceIoCtrl( file.Handle, Code, InBuffer, InBufLength, OutBuffer, ref OutBufLength );
		}
		public static void DeviceIoCtrl( IntPtr handle, Int32 Code, IntPtr InBuffer, Int32 InBufLength, IntPtr OutBuffer, ref Int32 OutBufLength )
		{
			int BytesReturned = 0;
			bool status;

			// execute DeviceIoControl
			if ( handle.ToInt32() == -1 ) {
				throw new OmException( "Driver NOT installed?" );
			}
			status = DeviceIoControl ( handle, Code, InBuffer, InBufLength, OutBuffer, OutBufLength, out BytesReturned, IntPtr.Zero );
			if ( !status ) {
				if ( BytesReturned > OutBufLength ) {
					throw ( new OmException( "DeviceIoCtrl: Error Insufficient Buffer" ) );
				}
				int lastError = Marshal.GetLastWin32Error();
				OmException exp = new OmException( "DeviceIoCtrl: Error DeviceIoControl failed. GetLastError= " + lastError );
				exp.GetLastError = lastError;
				throw exp;
			}
			OutBufLength = BytesReturned;
		}

		/// <summary>
		/// Used for Pass through
		/// </summary>
		/// <param name="file"></param>
		/// <param name="Code"></param>
		/// <param name="InBuffer"></param>
		/// <param name="InBufLength"></param>
		/// <param name="OutBuffer"></param>
		/// <param name="OutBufLength"></param>
		public static void DeviceIoCtrl( IntPtr file, Int32 Code, SCSI_PASS_THROUGH_DIRECT PassThrough, IntPtr OutBuffer, ref Int32 OutBufLength )
		{
			int Bytes = Marshal.SizeOf( PassThrough );

			IntPtr inBuffer = Marshal.AllocHGlobal( Bytes );
			Zero( inBuffer, Bytes );
			Marshal.StructureToPtr( PassThrough, inBuffer, true );

			try {
				DeviceIoCtrl( file, Code, inBuffer, Bytes, OutBuffer, ref OutBufLength );

				Marshal.PtrToStructure( inBuffer, PassThrough );
			} finally {
				Marshal.FreeHGlobal( inBuffer );
			}
		}

		/// <summary>
		/// Fills the buffer with zeros
		/// </summary>
		/// <param name="Buffer"></param>
		/// <param name="length"></param>
		public static void Zero ( IntPtr Buffer, int length ) {
			for ( int i = 0 ; i < length ; i++ ) {
				Marshal.WriteByte( Buffer, i, 0 );
			}
		}


	}
}
