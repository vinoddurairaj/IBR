/*****************************************************************************
 *                                                                           *
 *  This software is the licensed software of Fujitsu Software               *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2003 by Fujitsu Software Technology Corporation			 *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************/

//
// Written by Mike Pollett and Veera
// April 2003
//

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Runtime.Remoting.Contexts;
using System.Collections;
using System.Runtime.Serialization;
using System.Text;

namespace MasterThread
{

	/// <summary>
	/// Summary description for DiskPartitionAPI.
	/// </summary>
	/// 
	public class DiskPartitionAPI
	{

		//various disk & partition related items
		public static uint DISK_BLOCK_SIZE = 512;
		public static int MAX_DRIVE_SUPPORT = 256;
		public static int MAX_PARTITION_PER_DRIVE = 256;

		/// <summary>
		/// returns a string of the capacity rounded to the nearest kb, mb, gb or tb.
		/// </summary>
		/// <param name="Cap"></param>
		/// <returns></returns>
		public static string CapacityToString( double Cap ) {
			string cap;
			if ( Cap >= TERA_CAP ) {
				cap = string.Format( "{0}TB", (Cap/TERA_CAP).ToString("0.#") );
			} else if ( Cap >= GIGA_CAP ) {
				cap = string.Format( "{0}GB", (Cap/GIGA_CAP).ToString("0.#") );
			} else if ( Cap >= MEGA_CAP ) {
				cap = string.Format( "{0}MB", (Cap/MEGA_CAP).ToString("0.#") );
			} else if ( Cap >= KILO_CAP ) {
				cap = string.Format( "{0}KB", (Cap/KILO_CAP).ToString("0.#") );
			} else{
				cap = string.Format( "{0}Bytes", Cap );
			}
			return cap;
		}
		// Size constants
		public static double KILO_CAP = (uint)eSize.KILOBYTE;
		public static double MEGA_CAP = (uint)eSize.KILOBYTE * KILO_CAP;
		public static double GIGA_CAP = (uint)eSize.KILOBYTE * MEGA_CAP;
		public static double TERA_CAP = (uint)eSize.KILOBYTE * GIGA_CAP;



		// this is used to break out of the drive count loop
		private static int MAX_DRIVE_HOLE = 50;

		public static int MINIMUM_FREE_BLOCKS = (int)( ( (int)eSize.MEGABYTE * 16 ) / DISK_BLOCK_SIZE );	// 8MB
		//		public static uint DISK_START_GAP_SIZE = 32 * (uint)eSize.KILOBYTE;

		private static uint INQUIRY_BUFFER_SIZE = (uint)eSize.KILOBYTE;

		private static byte SCSI_IOCTL_DATA_IN = 1;

		//		private static Guid DiskClassGuid = new Guid( 0x53f56307, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b );

		/// <summary>
		/// collection of all of the disk guids seen from PMF driver
		/// </summary>
//		protected PmfAPI.DiskGuidCollection m_DiskGuids;

		[Serializable]
			public enum eWindowsPartitionType
		{
			PARTITION_ENTRY_UNUSED	= 0x0000,     // Entry unused
			PARTITION_FAT_16		= 0x0004,     // 16-bit FAT entries
			PARTITION_EXTENDED		= 0x0005,     // Extended partition entry
			PARTITION_HUGE			= 0x0006,		// Huge partition MS-DOS V4
			PARTITION_EXTENDED_LBA	= 0x000F,
			PARTITION_WINDOWS_DYNAMIC = 0x0042,
			PARTITION_NTFT			= 0x0080,		// NTFT partition
			SOFTEK_PARTITION_TYPE	= 0x00F9,		// Softek's partition type
		}

		[Serializable]
			public enum eDriveType
		{
			IDE_DRIVE_TYPE = 0,
			SCSI_BUS_DRIVE_TYPE = 1,
			FIBRE_CHANNEL_DRIVE_TYPE = 2,
			UNKNOWN_DRIVE_TYPE = 3
		}

		public enum MEDIA_TYPE
		{
			Unknown, 
			F5_1Pt2_512, 
			F3_1Pt44_512, 
			F3_2Pt88_512, 
			F3_20Pt8_512, 
			F3_720_512, 
			F5_360_512, 
			F5_320_512, 
			F5_320_1024, 
			F5_180_512, 
			F5_160_512, 
			RemovableMedia, 
			FixedMedia, 
			F3_120M_512, 
			F3_640_512, 
			F5_640_512, 
			F5_720_512, 
			F3_1Pt2_512, 
			F3_1Pt23_1024, 
			F5_1Pt23_1024, 
			F3_128Mb_512, 
			F3_230Mb_512, 
			F8_256_128, 
			F3_200Mb_512, 
			F3_240M_512, 
			F3_32M_512
		}

		public struct DRIVE_LAYOUT_INFORMATION
		{
			public uint Signature;
			public PARTITION_INFORMATION [] PartitionEntrys;	// one or more
		}

		[StructLayout(LayoutKind.Sequential)]
		public class PARTITION_INFORMATION
		{
			public ulong StartingOffset;
			public ulong PartitionLength;
			public int HiddenSectors;
			public int PartitionNumber;
			public byte PartitionType;
			public byte BootIndicator;
			public byte RecognizedPartition;
			public byte RewritePartition;
		}
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public class DISK_GEOMETRY
		{
			public ulong Cylinders;
			public MEDIA_TYPE MediaType;
			public int TracksPerCylinder;
			public int SectorsPerTrack;
			public int BytesPerSector;
		}

		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
			public class INQUIRYDATA
		{
			public byte DeviceType;			// 5 bits, DeviceTypeQualifier 3 bits;
			public byte DeviceTypeModifier;	// 7 bits, RemovableMedia 1 bit;
			public byte Versions;
			public byte ResponseDataFormat;	// 4 bits, HiSupport 1 bit, NormACA 1 bit,ReservedBit 1, AERC 1;
			public byte AdditionalLength;
			protected byte Reserved0;
			protected byte Reserved1;
			public byte SoftReset;				// 1 bit,CommandQueue 1,Reserved2 1, LinkedCommands 1,Synchronous 1, Wide16Bit 1, Wide32Bit 1, RelativeAddressing 1
			[MarshalAs(UnmanagedType.ByValArray, SizeConst=8)]
			public char[] VendorId;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst=16)]
			public char[] ProductId;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst=4)]
			public char[] ProductRevisionLevel;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst=20)]
			public char[] VendorSpecific;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=40)]
			protected string Reserved3;
		}

		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public class SCSI_ADDRESS
		{
			public uint Length;
			public byte PortNumber;
			public byte PathId;
			public byte TargetId;
			public byte Lun;
		}

		[Serializable]
			public class SectionSpaceCollection : CollectionBase
		{
			public int Add ( SECTION_SPACE x ) 
			{
				if ( Contains(x) ) return -1;
				int index = InnerList.Add(x);
				return index;
			}
			public bool Contains ( SECTION_SPACE x ) 
			{
				return InnerList.Contains(x);
			}
			public int IndexOf ( SECTION_SPACE x ) 
			{
				return InnerList.IndexOf ( x );
			}
			public void Remove ( SECTION_SPACE x ) 
			{
				InnerList.Remove(x);
			}
			public SECTION_SPACE this[int index] 
			{
				get { return (SECTION_SPACE) InnerList[index]; }
			}
			public void Insert( int index, SECTION_SPACE x ) 
			{
				InnerList.Insert( index, x );
			}
		}
		[Serializable]
			private class VolumeInfoCollection : CollectionBase
		{
			public int Add ( VolumeInfo x ) 
			{
				if ( Contains(x) ) return -1;
				int index = InnerList.Add(x);
				return index;
			}
			public bool Contains ( VolumeInfo x ) 
			{
				return InnerList.Contains(x);
			}
			public int IndexOf ( VolumeInfo x ) 
			{
				return InnerList.IndexOf ( x );
			}
			public void Remove ( VolumeInfo x ) 
			{
				InnerList.Remove(x);
			}
			public VolumeInfo this[int index] 
			{
				get { return (VolumeInfo) InnerList[index]; }
			}
			public void Insert( int index, VolumeInfo x ) 
			{
				InnerList.Insert( index, x );
			}
		}
		[Serializable]
			public class DiskExtentCollection : CollectionBase
		{
			public int Add ( DISK_EXTENT x ) 
			{
				if ( Contains(x) ) return -1;
				int index = InnerList.Add(x);
				return index;
			}
			public bool Contains ( DISK_EXTENT x ) 
			{
				return InnerList.Contains(x);
			}
			public int IndexOf ( DISK_EXTENT x ) 
			{
				return InnerList.IndexOf ( x );
			}
			public void Remove ( DISK_EXTENT x ) 
			{
				InnerList.Remove(x);
			}
			public DISK_EXTENT this[int index] 
			{
				get { return (DISK_EXTENT) InnerList[index]; }
			}
			public void Insert( int index, DISK_EXTENT x ) 
			{
				InnerList.Insert( index, x );
			}
		}

		public class SECTION_SPACE
		{
			public uint Start;
			public uint End;
			public bool WithinExtended = false;
			public int PartitionNumber = 0;
			public eWindowsPartitionType PartitionType;
			public bool RecognizedPartition;
			public bool FirstExtent;
			public string SectionPath;
			public string SectionName;
			public DiskExtentCollection DiskExtents;
			//Added by Veera
			//Stores the total Size of the volume
			public ulong LogicalSize;
		}

		//		[Serializable]
		//		public class DriveInformationCollection : CollectionBase
		//		{
		//			public int Add ( DRIVE_INFORMATION x ) {
		//				if ( Contains(x) ) return -1;
		//				int index = InnerList.Add(x);
		//				return index;
		//			}
		//			public bool Contains ( DRIVE_INFORMATION x ) {
		//				return InnerList.Contains(x);
		//			}
		//			public int IndexOf ( DRIVE_INFORMATION x ) {
		//				return InnerList.IndexOf ( x );
		//			}
		//			public void Remove ( DRIVE_INFORMATION x ) {
		//				InnerList.Remove(x);
		//			}
		//			public DRIVE_INFORMATION this[int index] {
		//				get { return (DRIVE_INFORMATION) InnerList[index]; }
		//			}
		//			public void Insert( int index, DRIVE_INFORMATION x ) {
		//				InnerList.Insert( index, x );
		//			}
		//		}


		/// <summary>
		/// This drive information class is passed to the remote machine
		/// </summary>
		[Serializable]
			public class DRIVE_INFORMATION
		{
			public bool Online;
			public Int32 PhysicalNumber;		// 0 referenced
			public string DrivePath;			// Windows device path name Symbolic link
			public byte	Port;					// Card or device
			public byte	Path;					// Bus or loop
			public byte	Target;					// SCSI Id
			public byte	Lun;					// SCSI LUN
			public eDriveType DriveType;		// SCSI or IDE
			public string DriveTypeString;		// ASCII
			public string Name;					// ASCII Model and/or Vendor Id - NULL terminated
			public string Model;				// ASCII Model and/or Vendor Id - NULL terminated
			public string Version;				// ASCII Firmware revision code - NULL terminated 
			public string SerialNumber;			// ASCII Serial number - NOT Guaranted NULL terminated 
			public string SerialNumberString;	// Cleaned-up SerialNumber - no spaces NUll terminated
			public UInt32 DiskSignature;		// Win2K's disk signature
			public ulong Cylinders;				// Geometry is from Windows - who knows?
			public Int32 Heads;					//
			public Int32 SectorsPerTrack;		//
			public Int32 BytesPerSector = 512;		// 
			public double Capacity;				// In bytes
			public string CapacityString;		// In ASCII Ex: 40.3GB; 2.1TB; 120.9GB; 653.4MB;
			public UInt32 TotalBlocks;			// MaxLBA + 1
			// This is the GUID that PMF driver writes on the drive
			public Guid DiskGuid;
			//keeps track of the extended partition
			public UInt32 ExtPartitionStart;
			public UInt32 ExtPartitionLength;
			public bool WindowsDynamic;			// true if drive is windows dynamic disk
			public SECTION_INFORMATION[] Sections;
			public bool LocalDynamic;	//Added by Veera to check if the Dynamic Disk is local or foreign
			//true for local false for foreign
		}

		[Serializable]
			//		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
			public class SECTION_INFORMATION
		{
			public Int32 SectionNumber;			// 0 referenced
			public string SectionPath;			// Windows symbol link to this partition
			public bool	SectionUsed;			// TRUE = Used or Allocated space, FALSE = Unused or unallocated space
			public bool	SectionAvailable;		// TRUE = Available for SDS use, FALSE = not available for SDS use
			public UInt32 StartingBlock;		// Starting LBA
			public UInt32 LengthInBlocks;		// Size in blocks
			public byte	PercentOfDrive;			// 0 - 100 %
			public double LengthInBytes;		// Size in bytes
			public Int32 PartitionNumber;		// Windows partition number
			public eWindowsPartitionType PartitionType; // Partition code in hex, 0 = none
			public string TypeDescription;		// ASCII type description
			public bool	WithinExtended;			// Section within an extended partition
			public bool	WindowsPartition;		// TRUE = Windows recognizes and can use the partition
			public eDynamicDiskType DynamicDiskType;
			public DiskExtentCollection DiskExtents;
			//Added by Veera
			//Stores the total Size of the volume
			public ulong LogicalSize;
		}

		[Serializable]
			public class DriveInformationCollection : CollectionBase {
			public int Add ( DiskPartitionAPI.DRIVE_INFORMATION x ) {
				if ( Contains(x) ) return -1;
				int index = InnerList.Add(x);
				return index;
			}
			public bool Contains ( DiskPartitionAPI.DRIVE_INFORMATION x ) {
				return InnerList.Contains(x);
			}
			public int IndexOf ( DiskPartitionAPI.DRIVE_INFORMATION x ) {
				return InnerList.IndexOf ( x );
			}
			public void Remove ( DiskPartitionAPI.DRIVE_INFORMATION x ) {
				InnerList.Remove(x);
			}
			public DiskPartitionAPI.DRIVE_INFORMATION this[int index] {
				get { return (DiskPartitionAPI.DRIVE_INFORMATION) InnerList[index]; }
			}
			public void Insert( int index, DiskPartitionAPI.DRIVE_INFORMATION x ) {
				InnerList.Insert( index, x );
			}
		}


		/// <summary>
		/// Constructor
		/// </summary>
		public DiskPartitionAPI()
		{
//			PmfAPI pmf = new PmfAPI();
//			m_DiskGuids = new PmfAPI.DiskGuidCollection();
//			pmf.GetDiskGuids( m_DiskGuids );
		}

		/// <summary>
		/// Returns the number of physical disks attached to the system.
		/// </summary>
		/// <returns></returns>
		public int DriveCount ()
		{
			int Drives = 0;
			int failed = 0;
			IntPtr disk = IntPtr.Zero;
			for ( int i = 0 ; i < MAX_DRIVE_SUPPORT ; i++ ) 
			{
				disk = Createfile.OpenDrive(i);
				if ( disk == IntPtr.Zero ) 
				{
					if ( failed++ > MAX_DRIVE_HOLE ) 
					{
						break;
					}
					continue;
				}
				Createfile.Close( disk );
				Drives++;
			}
			return Drives;
		}


		//		/// <summary>
		//		/// Returns the number of physical disks attached to the system.
		//		/// </summary>
		//		/// <returns></returns>
		//		public int DriveCount()
		//		{
		//			int Drives = 0;
		//			IntPtr devInfo = IntPtr.Zero;
		//
		//			devInfo = SetupDiGetClassDevs( ref DiskClassGuid, IntPtr.Zero, IntPtr.Zero,
		//									(int)( eDIGCF.PROFILE | eDIGCF.DEVICEINTERFACE | eDIGCF.PRESENT ) );
		//			if ( devInfo.ToInt32() == -1 ) {
		//				throw new OmException( "ERROR: DriveCount: call to SetupDiGetClassDevs failed." );
		//			}
		//			try {
		//				SP_DEVICE_INTERFACE_DATA devInterfaceData = new SP_DEVICE_INTERFACE_DATA();
		//				devInterfaceData.Size = Marshal.SizeOf( devInterfaceData );
		//
		//				for ( int nMemberIndex = 0 ; ; nMemberIndex++ ) {
		//					bool ret;
		//					ret = SetupDiEnumDeviceInterfaces( devInfo, IntPtr.Zero, ref DiskClassGuid, nMemberIndex, out devInterfaceData );
		//					if ( !ret ) {
		//						if ( Marshal.GetLastWin32Error() != (int)eERROR_CODES.ERROR_NO_MORE_ITEMS ) {
		//							throw new OmException( "SetupDiEnumDeviceInterfaces failed LastError= " + Marshal.GetLastWin32Error() );
		//						}
		//						break;
		//					}
		//					Drives++;
		//				}
		//			} finally {
		//				SetupDiDestroyDeviceInfoList( devInfo );
		//			}
		//			return Drives;
		//		}

		/// <summary>
		/// return a IntPtr (handle) for the physical Drive path.
		/// </summary>
		/// <param name="driveNumber"></param>
		/// <returns></returns>
		//		private IntPtr OpenDrive ( string drivePath )
		//		{
		//			return Createfile.OpenDrive( drivePath );
		//			IntPtr drive;
		//			try {
		//				drive = Createfile.OpenDrive( drivePath );
		//			} catch ( Exception e ) {
		//				throw new OmException( "ERROR: OpenDrive: File.Open failed= " + e.Message );
		//				return null;
		//			}
		//			return drive;
		//		}

		/// <summary>
		/// GetAllDriveInfo
		/// </summary>
		/// <param name="Drive"></param>
		/// <param name="DriveInfo"></param>
		public DriveInformationCollection GetAllDriveInfo()
		{
			// get the volumes created on Dynamic disks and the extents for each volume
			VolumeInfoCollection volumeInfos = GetAllVolumeExtents();
			// get the partitions created on basic disks and the symbolic name for each partition
			VolumeInfoCollection partitionInfos = GetAllPartitionExtents();

			// get all of the information for the real physical drives
			DriveInformationCollection DriveInfos = new DriveInformationCollection();

			int failed = 0;
			IntPtr disk = IntPtr.Zero;

			for ( int DriveNumber = 0 ; DriveNumber < MAX_DRIVE_SUPPORT ; DriveNumber++ ) 
			{
				string DrivePath = Createfile.PHYSICALDRIVE + DriveNumber;
				// Open the drive
				try 
				{
					disk = Createfile.OpenDrive ( DrivePath );
				} 
				catch ( Exception ) 
				{
					if ( failed++ > MAX_DRIVE_HOLE ) 
					{
						break;
					}
					continue;
				}
				try 
				{
					// Get the drive info and layout
					DRIVE_INFORMATION DriveInfo = new DRIVE_INFORMATION();
					DriveInfo.DrivePath = DrivePath;
					DriveInfo.PhysicalNumber = DriveNumber;
					DriveInfo.WindowsDynamic = IsWindowsDynamic( DriveNumber, volumeInfos );
					if(DriveInfo.WindowsDynamic)
					{
						//Rarely \Device\HarddiskX\DRX is becomming wrong so using \??\PhysicalDriveX
//						DriveInfo.LocalDynamic =   IsLocalDisk(string.Format("\\Device\\Harddisk{0}\\DR{0}",DriveNumber));
						DriveInfo.LocalDynamic =   IsLocalDisk(DriveNumber);
					}

					GetDriveInfoAndLayout( disk, DriveInfo, volumeInfos, partitionInfos );
					// add it to the collection
					DriveInfos.Add( DriveInfo );
				} 
				catch (Exception exp)
				{
					if ( exp is OmException ) {
						if ( ((OmException)exp).GetLastError == ERROR_INVALID_FUNCTION ) {
							continue;
						}
					}
					OmException.LogException(new OmException( String.Format("Error:: GetDriveInfoAndLayout Failed for Drive " + DriveNumber.ToString() + exp.Message ), exp ));
					continue;
				}
				finally 
				{
					// Close drive
					if ( disk != IntPtr.Zero ) 
					{
						Createfile.Close( disk );
						disk = IntPtr.Zero;
					}
				}
			}
			return DriveInfos;
		}
		
		//Error Code Returned by CODSet Partition
		
		const  int ERROR_INVALID_FUNCTION = 1;    // dderror

		//Veera 05-06-2003
		/// <summary>
		/// Returns Volume Extents for the specified Dynamic Volumes
		/// </summary>
		/// <returns></returns>
		private VolumeInfoCollection GetAllVolumeExtents()
		{
			// get the volumes created on Dynamic disks and the extents for each volume
			VolumeInfoCollection volumeInfos = GetVolumes();
			VolumeInfoCollection volumesToBeRemoved = new VolumeInfoCollection();
			IntPtr volume= IntPtr.Zero;
			foreach ( VolumeInfo volumeInfo in volumeInfos ) 
			{
				// Open the Volume
				try 
				{
					volume = OpenDeviceFile( volumeInfo.SymbolicName );
					try 
					{
						// Get the volume info and layout
						GetVolumeExtents( volume, volumeInfo.DiskExtents );
						//Added by Veera to set the Total Size of Volume
						GetVolumeSize(volume,volumeInfo);
					} 
					catch(OmException e)
					{
						if(e.GetLastError == ERROR_INVALID_FUNCTION) {
							volumesToBeRemoved.Add(volumeInfo);
//							volumeInfos.Remove(volumeInfo);
						}
						else {
							throw e;
						}
					}
					finally 
					{
						// Close volume
						if((object)volume != null && volume != IntPtr.Zero)	{
							ZwClose( volume );
						}
					}
				} 
				catch ( Exception e ) 
				{
					OmException.LogException( new OmException( "DiskPartitionAPI: GetAllVolumeExtents: caught an exception = " + e.Message ,e ) );
					continue;
				}
			}

			foreach(VolumeInfo volumeInfo in volumesToBeRemoved)
			{
				volumeInfos.Remove(volumeInfo);
			}
			if(volumesToBeRemoved.Count>0)
			{
				volumesToBeRemoved.Clear();
			}
			return volumeInfos;
		}

		//Veera 05-06-2003
		/// <summary>
		/// Returns Partition Extents for the specified Basic Partitions
		/// For COD Disks the error ERROR_INVALID_FUNCTION is returned
		/// </summary>
		/// <returns></returns>
		private VolumeInfoCollection GetAllPartitionExtents()
		{
			// get the volumes created on basic disks and the extents for each partition
			//Changed the way to get the basic volumes
			//Modified by Veera 05-16-2003
			//This function will get the basic partitions from the Windows Volume GUID.
			//So there will be no problem with un-detected windowds partitions
			//that are created byu CODSets and other Custom Volume types.
			VolumeInfoCollection partitionInfos = GetBasicVolumes1();
			VolumeInfoCollection partitionsToBeRemoved = new VolumeInfoCollection();
			IntPtr partition =IntPtr.Zero;
			foreach ( VolumeInfo partitionInfo in partitionInfos ) 
			{
				// Open the partition volume
				try 
				{
					partition = OpenDeviceFile( partitionInfo.SymbolicName );
					try 
					{
						// Get the volume info and layout
						GetVolumeExtents( partition, partitionInfo.DiskExtents );
						//Added by Veera to set the Total Size of Volume
						GetVolumeSize(partition,partitionInfo);
					} 
					catch(OmException e)
					{
						if(e.GetLastError == ERROR_INVALID_FUNCTION) {
							partitionsToBeRemoved.Add(partitionInfo);
//							partitionInfos.Remove(partitionInfo);
						}
						else {
							throw e;
						}
					}
					finally 
					{
						// Close partition
						if((object)partition != null && partition != IntPtr.Zero) {
							ZwClose( partition );
						}
					}
				} 
				catch ( OmException e ) 
				{
					OmException exp = new OmException( "DiskPartitionAPI: GetAllPartitionExtents: caught an exception = " + e.Message + ", unable to get extents for drive= " + partitionInfo.SymbolicName ,e );
					switch ( e.GetLastError ) {
						case 21:
							// device is not ready
						case 1117:
							// The request could not be performed because of an I/O device error. 
						case 1167:
							// device is not connected
							// this error case is caused by Foreign COD volumes have a //device/HardDiskvolumeX created for it. So we think it is a valid partition and then try to get the extents which fails.
							// This error will still be logged but will not be emailed.
						case 116:
							// The request could not be performed because of an I/O device error.
							// log locally only
							OmException.LogException( exp, EventLogEntryType.Warning );
							break;
						default:
							OmException.LogException( exp );
							break;
					}
				}
			}
			foreach(VolumeInfo partitionInfo in partitionsToBeRemoved)
			{
				partitionInfos.Remove(partitionInfo);
			}
			if(partitionsToBeRemoved.Count>0)
			{
				partitionsToBeRemoved.Clear();
			}

			return partitionInfos;
		}

		/// <summary>
		/// GetDriveOffset
		/// For either partitions or volumes
		/// </summary>
//		public int GetDriveOffset( int DriveNumber, int StartBlock, ref bool dynamic )
//		{
//			// get the volumes created on Dynamic disks and the extents for each volume
//			VolumeInfoCollection volumeInfos = GetAllVolumeExtents();
//			// get the partitions created on basic disks and the symbolic name for each partition
//			VolumeInfoCollection partitionInfos = GetAllPartitionExtents();
//
//			// get all of the information for the real physical drive
//			IntPtr disk = IntPtr.Zero;
//
//			string DrivePath = Createfile.PHYSICALDRIVE + DriveNumber;
//			// Open the drive
//			try 
//			{
//				disk = Createfile.OpenDrive ( DrivePath );
//			} 
//			catch ( Exception e ) 
//			{
//				throw new OmException ( "GetDriveOffset: Failed to open drive handle= " + e.Message, e );
//			}
//			try 
//			{
//				// Get the drive info and layout
//				DRIVE_LAYOUT_INFORMATION PartLayout;
//				DRIVE_INFORMATION DriveInfo = new DRIVE_INFORMATION();
//				DriveInfo.DrivePath = DrivePath;
//				DriveInfo.PhysicalNumber = DriveNumber;
//				DriveInfo.WindowsDynamic = IsWindowsDynamic( DriveNumber, volumeInfos );
//
//				SectionSpaceCollection SectionSpaces = null;
//				if ( !DriveInfo.WindowsDynamic ) 
//				{
//					// Allocate memory for get partition info call
//					PartLayout = new DRIVE_LAYOUT_INFORMATION();
//
//					// Get Partition Info
//					GetPartitionInfo ( disk, ref PartLayout );
//
//					// check if Dynamic disk with no partitions
//					foreach ( PARTITION_INFORMATION partitionInfo in PartLayout.PartitionEntrys ) 
//					{
//						if ( partitionInfo.PartitionType == (byte)eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC ) 
//						{
//							DriveInfo.WindowsDynamic = true;
//							//Added to check for local foreign dynamic disks.
//							if(DriveInfo.WindowsDynamic)
//							{
//								DriveInfo.LocalDynamic =   IsLocalDisk(string.Format("\\Device\\Harddisk{0}\\DR{0}",DriveInfo.PhysicalNumber));
//							}
//							goto DynamicDisk;
//						}
//					}
//
//					// Check for valid partition layout
//					if ( ValidatePartitionLayout( PartLayout, DriveInfo ) == false ) 
//					{
//						throw new OmException ( "Error: GetDriveOffset: ValidatePartitionLayout failed." );
//					}
//
//					// Allocate and clear SectionSpace
//					SectionSpaces = new SectionSpaceCollection();
//
//					// Convert partition layout to SectionSpace
//					PartitionLayoutToSectionSpace ( PartLayout.PartitionEntrys, SectionSpaces, DriveInfo, partitionInfos );
//				}
//			DynamicDisk:
//				if ( DriveInfo.WindowsDynamic ) 
//				{
//					// set the return flag to dynamic
//					dynamic = true;
//					// Allocate and clear SectionSpace
//					SectionSpaces = new SectionSpaceCollection();
//
//					// Convert Volume Information to SectionSpace
//					VolumeInfoToSectionSpace ( volumeInfos, SectionSpaces, DriveInfo );
//				}
//
//				// Sort SectionSpace
//				SortSectionSpace( SectionSpaces );
//
//				// now find the partition having the starting block we are looking for and count along the way
//				for ( int count = 0 ; count < SectionSpaces.Count ; count++ ) 
//				{
//					if ( SectionSpaces[ count ].Start == StartBlock ) 
//					{
//						// this is the normal exit point
//						if ( StartBlock > DriveInfo.ExtPartitionStart ) 
//						{
//							// need to increment to account for extended partition in diskpart
//							count++;
//						}
//						// increment to account to 1 based
//						return ++count;
//					}
//				}
//			} 
//			finally 
//			{
//				// Close drive
//				if ( disk != IntPtr.Zero ) 
//				{
//					Createfile.Close( disk );
//					disk = IntPtr.Zero;
//				}
//			}
//			// if here then error partition not found.
//			throw new OmException ( "Error: GetDriveOffset: Failed to locate volume or partition on drive " + DriveNumber + ", StartBlock= " + StartBlock );
//		}


		private bool IsWindowsDynamic( int DriveNumber, VolumeInfoCollection volumeInfos )
		{
			foreach ( VolumeInfo volumeInfo in volumeInfos ) 
			{
				foreach ( DISK_EXTENT diskExtent in volumeInfo.DiskExtents ) 
				{
					if ( diskExtent.DiskNumber == DriveNumber ) 
					{
						return true;
					}
				}
			}
			return false;
		}

		//		/// <summary>
		//		/// GetAllDriveInfo
		//		/// </summary>
		//		/// <param name="Drive"></param>
		//		/// <param name="DriveInfo"></param>
		//		public DriveInformationCollection GetAllDriveInfo() {
		//			// get all of the information for the real physical drives
		//			DriveInformationCollection DriveInfos = new DriveInformationCollection();
		//
		//			IntPtr devInfo = IntPtr.Zero;
		//
		//			devInfo = SetupDiGetClassDevs( ref DiskClassGuid, IntPtr.Zero, IntPtr.Zero,
		//				(int)( eDIGCF.PROFILE | eDIGCF.DEVICEINTERFACE | eDIGCF.PRESENT ) );
		//			if ( devInfo.ToInt32() == -1 ) {
		//				throw new OmException( "ERROR: DriveCount: call to SetupDiGetClassDevs failed." );
		//			}
		//			try {
		//				SP_DEVICE_INTERFACE_DATA devInterfaceData = new SP_DEVICE_INTERFACE_DATA();
		//				devInterfaceData.Size = Marshal.SizeOf( devInterfaceData );
		//
		//				for ( int nMemberIndex = 0 ; ; nMemberIndex++ ) {
		//					bool ret;
		//					ret = SetupDiEnumDeviceInterfaces( devInfo, IntPtr.Zero, ref DiskClassGuid, nMemberIndex, out devInterfaceData );
		//					if ( !ret ) {
		//						if ( Marshal.GetLastWin32Error() != (int)eERROR_CODES.ERROR_NO_MORE_ITEMS ) {
		//							throw new OmException( "SetupDiEnumDeviceInterfaces failed LastError= " + Marshal.GetLastWin32Error() );
		//						}
		//						break;
		//					}
		//					string DrivePath = "";
		//					int pathSize = 1000;	// max path size
		//					IntPtr InterfaceDetailData = Marshal.AllocHGlobal( pathSize );
		//					try {
		//						DeviceIOControl.Zero ( InterfaceDetailData, pathSize );
		//						Marshal.WriteInt32( InterfaceDetailData, 0, 5 );
		//						SP_DEVINFO_DATA          devInfoData = new SP_DEVINFO_DATA();
		//						devInfoData.cbSize = Marshal.SizeOf( typeof( SP_DEVINFO_DATA ) );
		//						
		//						ret = SetupDiGetDeviceInterfaceDetail( devInfo, ref devInterfaceData, InterfaceDetailData, pathSize, ref pathSize, ref devInfoData );
		//						if ( !ret ) {
		//							throw new OmException( "ERROR: GetAllDriveInfo: SetupDiGetDeviceInterfaceDetail failed LastError= " + Marshal.GetLastWin32Error() );
		//						}
		//						DrivePath = Marshal.PtrToStringAnsi( (IntPtr)(InterfaceDetailData.ToInt32() + 4 ) );
		//
		//					} finally {
		//						Marshal.FreeHGlobal( InterfaceDetailData );
		//					}
		//
		//					IntPtr disk = IntPtr.Zero;
		//					// Open the drive
		//					disk = Createfile.OpenDrive ( DrivePath );
		//					try {
		//						// Get the drive info and layout
		//						DRIVE_INFORMATION DriveInfo = new DRIVE_INFORMATION();
		//						GetDriveInfoAndLayout( disk, DriveInfo );
		//						DriveInfos.Add( DriveInfo );
		//					} finally {
		//						// Close drive
		//						if ( disk != IntPtr.Zero ){
		//							Createfile.CloseDriver( disk );
		//							disk = IntPtr.Zero;
		//						}
		//					}
		//				}
		//			} finally {
		//				SetupDiDestroyDeviceInfoList( devInfo );
		//			}
		//
		//			// next get the volumes created on Dynamic disks and add them as a single disk
		//
		//
		//
		//
		//
		//			return DriveInfos;
		//		}


		//		/// <summary>
		//		/// GetDriveInfo
		//		/// </summary>
		//		/// <param name="DrivePath"></param>
		//		/// <param name="DriveInfo"></param>
		//		public void GetDriveInfo( string DrivePath, DRIVE_INFORMATION DriveInfo )
		//		{
		//			IntPtr disk = IntPtr.Zero;
		//
		//			// Open the drive
		//			disk = Createfile.OpenDrive ( DrivePath );
		//			try {
		//				// Get the drive info and layout
		//				GetDriveInfoAndLayout( disk, DriveInfo );
		//			} finally {
		//				// Close drive
		//				if ( disk != IntPtr.Zero ){
		//					Createfile.CloseDriver( disk );
		//					disk = IntPtr.Zero;
		//				}
		//			}
		//		}

		/// <summary>
		/// GetDriveInfoAndLayout
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="Drive"></param>
		/// <param name="DriveInfo"></param>
		private void GetDriveInfoAndLayout ( IntPtr disk, DRIVE_INFORMATION DriveInfo, VolumeInfoCollection volumeInfos, VolumeInfoCollection partitionInfos )
		{
			DRIVE_LAYOUT_INFORMATION PartLayout;
			SectionSpaceCollection SectionSpaces = null;

			// Get Drive Info
			GetDriveInformation ( disk, DriveInfo );

			// Check for on-line status
			if ( DriveInfo.Online == false ) 
			{
				return;
			}

			int freePartitions = 0;

			//Removed by Veera 04-21-2003 to obtain signature for Dynamic Disks also.		
			//			if ( !DriveInfo.WindowsDynamic ) {
		{
			// Allocate memory for get partition info call
			PartLayout = new DRIVE_LAYOUT_INFORMATION();

			// Get Partition Info
			GetPartitionInfo ( disk, ref PartLayout );

			// Set Disk Signature from Partition Info
			DriveInfo.DiskSignature = PartLayout.Signature;

			// check if Dynamic disk with no partitions
			foreach ( PARTITION_INFORMATION partitionInfo in PartLayout.PartitionEntrys ) 
			{
				if ( partitionInfo.PartitionType == (byte)eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC ) 
				{
					DriveInfo.WindowsDynamic = true;
					//Added for checking the foreign local dynamic disks Veera
					if(DriveInfo.WindowsDynamic)
					{
						//Rarely \Device\HarddiskX\DRX is becomming wrong so using \??\PhysicalDriveX
//						DriveInfo.LocalDynamic =   IsLocalDisk(string.Format("\\Device\\Harddisk{0}\\DR{0}",DriveInfo.PhysicalNumber));
						DriveInfo.LocalDynamic =   IsLocalDisk(DriveInfo.PhysicalNumber);
					}
					goto DynamicDisk;
				}
			}

			// Check for valid partition layout
			if ( ValidatePartitionLayout( PartLayout, DriveInfo ) == false ) 
			{
				return;
			}

			// Allocate and clear SectionSpace
			SectionSpaces = new SectionSpaceCollection();

			// Convert partition layout to SectionSpace
			PartitionLayoutToSectionSpace ( PartLayout.PartitionEntrys, SectionSpaces, DriveInfo, partitionInfos );

			// Calculate if the free space outside of extended partitions can be used.
			// If all the partition entries have values, then any free space outside of extended partitions is not available.
			// 
			if(PartLayout.PartitionEntrys.Length >0)
			{
				freePartitions = 4;
				for ( int i = 0 ; i < 4 ; i++ ) 
				{
					if ( (eWindowsPartitionType)PartLayout.PartitionEntrys[i].PartitionType != eWindowsPartitionType.PARTITION_ENTRY_UNUSED ) 
					{
						freePartitions--;
					}
				}
			}
			else
			{
				freePartitions = 0;
			}

		}
			DynamicDisk:
				if ( DriveInfo.WindowsDynamic ) 
				{
					// Allocate and clear SectionSpace
					SectionSpaces = new SectionSpaceCollection();

					// Convert Volume Information to SectionSpace
					VolumeInfoToSectionSpace ( volumeInfos, SectionSpaces, DriveInfo );
				}
			// add free space to SectionSpace
			AddFreeSpaceToSectionSpace ( SectionSpaces, DriveInfo );

			// Fill in DriveInfo with SectionSpace info
			SectionSpaceToDriveLayout ( disk, SectionSpaces, DriveInfo, freePartitions );

		}

		// Convert Volume Information to SectionSpace
		private void VolumeInfoToSectionSpace ( VolumeInfoCollection volumeInfos, SectionSpaceCollection SectionSpaces, DRIVE_INFORMATION DriveInfo )
		{
			foreach ( VolumeInfo volumeInfo in volumeInfos ) 
			{
				bool FirstExtent = true;
				foreach ( DISK_EXTENT diskExtent in volumeInfo.DiskExtents ) 
				{
					if ( diskExtent.DiskNumber == DriveInfo.PhysicalNumber ) 
					{
						// this volume extent is on this drive.
						SECTION_SPACE section;
						section = new SECTION_SPACE();
						// fill in the section information
						section.Start = (uint)( diskExtent.StartingOffset / DriveInfo.BytesPerSector );
						section.End = (uint)( (diskExtent.StartingOffset + diskExtent.ExtentLength - 1) / DriveInfo.BytesPerSector );
						section.PartitionType = eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC;
						section.RecognizedPartition = true;
						section.SectionPath = volumeInfo.SymbolicName;
						section.SectionName = volumeInfo.Name;
						section.FirstExtent = FirstExtent;
						section.DiskExtents = volumeInfo.DiskExtents;
						//Added by Veera to set the total Size of the Volume
						section.LogicalSize = volumeInfo.LogicalSize;

						// add the section to the collection
						SectionSpaces.Add ( section );
					}
					FirstExtent = false;
				}
			}
		}

		//		// Convert partition Information to SectionSpace
		//		private void partitionInfoToSectionSpace ( VolumeInfoCollection partitionInfos, SectionSpaceCollection SectionSpaces, DRIVE_INFORMATION DriveInfo ) {
		//			foreach ( VolumeInfo partitionInfo in volumeInfos ) {
		//				bool FirstExtent = true;
		//				foreach ( DISK_EXTENT diskExtent in volumeInfo.DiskExtents ) {
		//					if ( diskExtent.DiskNumber == DriveInfo.PhysicalNumber ) {
		//						// this volume extent is on this drive.
		//						SECTION_SPACE section;
		//						section = new SECTION_SPACE();
		//						// fill in the section information
		//						section.Start = (uint)( diskExtent.StartingOffset / DriveInfo.BytesPerSector );
		//						section.End = (uint)( (diskExtent.StartingOffset + diskExtent.ExtentLength - 1) / DriveInfo.BytesPerSector );
		//						section.PartitionType = eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC;
		//						section.RecognizedPartition = true;
		//						section.SectionPath = volumeInfo.SymbolicName;
		//						section.SectionName = volumeInfo.Name;
		//						section.FirstExtent = FirstExtent;
		//						section.DiskExtents = volumeInfo.DiskExtents;
		//						// add the section to the collection
		//						SectionSpaces.Add ( section );
		//					}
		//					FirstExtent = false;
		//				}
		//			}
		//		}

		/// <summary>
		/// fill in the free space to SectionSpace
		/// </summary>
		/// <param name="SectionSpaces"></param>
		/// <param name="DriveInfo"></param>
		private void AddFreeSpaceToSectionSpace ( SectionSpaceCollection SectionSpaces, DRIVE_INFORMATION DriveInfo )
		{

			SECTION_SPACE section;
			uint ExtStart = DriveInfo.ExtPartitionStart;
			uint ExtEnd = ExtStart + DriveInfo.ExtPartitionLength;
			bool Extended = DriveInfo.ExtPartitionLength != 0;

			// If no partitions and no extended, return one section - entire drive
			if (( SectionSpaces.Count == 0 ) && ( !Extended )) 
			{
				section = new SECTION_SPACE();
				section.Start = 0;
				section.End = (uint)DriveInfo.TotalBlocks;
				if ( DriveInfo.WindowsDynamic ) 
				{
					// substract the 1 mb used at the end
					section.End = (uint)(DriveInfo.TotalBlocks - ( MEGA_CAP / DriveInfo.BytesPerSector ));
				}
				section.PartitionNumber = 0;
				section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
				section.RecognizedPartition = false;
				//  add the section to the collection
				SectionSpaces.Add( section );
				// done Return
				return;
			}

			// Check if only extended is present
			if (( SectionSpaces.Count == 0 ) && ( Extended )) 
			{
				// Check for free section before extended
				if ( ExtStart >= MINIMUM_FREE_BLOCKS ) 
				{
					section = new SECTION_SPACE();
					section.Start = 0;
					section.End = ExtStart - 1;
					section.PartitionNumber = 0;
					section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
					section.RecognizedPartition = false;
					//  add the section to the collection
					SectionSpaces.Add( section );
				}

				// Set extended as free section
				section = new SECTION_SPACE();
				section.Start = ExtStart;
				section.End = ExtEnd - 1;
				section.PartitionNumber = 0;
				section.WithinExtended = true;
				section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
				section.RecognizedPartition = false;
				//  add the section to the collection
				SectionSpaces.Add( section );

				// Check for free section after extended
				if ((int)( DriveInfo.TotalBlocks - ExtEnd - 1 ) >= MINIMUM_FREE_BLOCKS ) 
				{
					section = new SECTION_SPACE();
					section.Start = ExtEnd;
					section.End = (uint)DriveInfo.TotalBlocks - 1;
					section.PartitionNumber = 0;
					section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
					section.RecognizedPartition = false;
					//  add the section to the collection
					SectionSpaces.Add( section );
				}
				// done Return
				return;
			}

			// Sort SectionSpace
			SortSectionSpace( SectionSpaces );

			// Check for free space and insert

			// Insert a dummy free space for the drive start and drive end
			// first the start of drive
			section = new SECTION_SPACE();
			section.Start = 0;
			section.End = 0;
			section.PartitionNumber = 0;
			section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
			section.RecognizedPartition = false;
			//  add the section to the collection begining
			SectionSpaces.Insert( 0, section );
			// next the end of drive
			section = new SECTION_SPACE();
			section.Start = (uint)DriveInfo.TotalBlocks;
			section.End = (uint)DriveInfo.TotalBlocks + 1;
			section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
			section.RecognizedPartition = false;
			section.PartitionNumber = 0;
			//  add the section to the collection end
			SectionSpaces.Add( section );

			// fill in all of the free space gaps
			for ( int i = 0 ; i < SectionSpaces.Count - 1 ; i++ ) 
			{
				if ((int)( SectionSpaces[i+1].Start - SectionSpaces[i].End ) > MINIMUM_FREE_BLOCKS ) 
				{
					// Insert the free space
					section = new SECTION_SPACE();
					section.Start = SectionSpaces[i].End + 1;
					section.End = SectionSpaces[i+1].Start - 1;
					section.PartitionNumber = 0;
					section.WithinExtended = (Extended) && (( section.Start >= ExtStart ) && ( section.End <= ExtEnd ));
					section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
					section.RecognizedPartition = false;
					//  add the section to the collection at this point plus 1
					SectionSpaces.Insert( i+1, section );
					i++;
				}
			}

			if ( !Extended ) 
			{
				goto CleanUp;
			}

			// Adjust free spaces for extended

			// Do extended start
			for ( int i = 0 ; i < SectionSpaces.Count ; i++ ) 
			{
				// Look for free space
				if ( SectionSpaces[i].PartitionNumber != 0 ) 
				{
					continue;
				}

				// Check if extended is 'between'
				if (( ExtStart >= SectionSpaces[i].Start ) &&
					( ExtStart <= SectionSpaces[i].End )) 
				{

					// Extended is between - check both sides

					// Add free space if both sides are > minimum
					// Adjust this free space if only one is > minimum

					if  (((int)( ExtStart - SectionSpaces[i].Start ) >= MINIMUM_FREE_BLOCKS ) &&
						((int)( SectionSpaces[i].End - ExtStart ) >= MINIMUM_FREE_BLOCKS )) 
					{

						// Add free space and adjust this free space
						section = new SECTION_SPACE();
						section.Start = SectionSpaces[i].Start;
						section.End = ExtStart - 1;
						section.WithinExtended = false;
						section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
						section.RecognizedPartition = false;

						SectionSpaces[i].Start = ExtStart;
						SectionSpaces[i].WithinExtended = true;
						// add the section to the collection
						SectionSpaces.Insert( i, section );
					} 
					else 
					{
						if ((int)( ExtStart - SectionSpaces[i].Start ) >= MINIMUM_FREE_BLOCKS ) 
						{
							// Adjust for first side
							SectionSpaces[i].End = ExtStart - 1;
							SectionSpaces[i].WithinExtended = false;
						} 
						else 
						{
							// Adjust for second side
							SectionSpaces[i].Start = ExtStart;
							SectionSpaces[i].WithinExtended = true;
						}
					}
				}

				// Do extended end
				// Check if extended is 'between'
				if (( ExtEnd >= SectionSpaces[i].Start ) &&
					( ExtEnd <= SectionSpaces[i].End )) 
				{
					// Extended is between - check both sides

					// Add free space if both sides are > minimum
					// Adjust this free space if only one is > minimum

					if (((int)( ExtEnd - SectionSpaces[i].Start ) >= MINIMUM_FREE_BLOCKS ) &&
						((int)( SectionSpaces[i].End - (ExtEnd) ) >= MINIMUM_FREE_BLOCKS )) 
					{

						// Add free space and adjust this free space
						section = new SECTION_SPACE();
						section.Start = SectionSpaces[i].Start;
						section.End = ExtEnd;
						section.WithinExtended = true;
						section.PartitionType = eWindowsPartitionType.PARTITION_ENTRY_UNUSED;
						section.RecognizedPartition = false;

						SectionSpaces[i].Start = ExtEnd + 1;
						SectionSpaces[i].WithinExtended = false;

						// add the section to the collection
						SectionSpaces.Insert( i, section );
					} 
					else 
					{
						// Adjust this free space - check which side
						// Check first side
						if ((int)( ExtEnd - SectionSpaces[i].Start ) >= MINIMUM_FREE_BLOCKS ) 
						{
							// Adjust for first side
							SectionSpaces[i].End = ExtEnd;
							SectionSpaces[i].WithinExtended = true;
						} 
						else 
						{
							// Adjust for second side
							SectionSpaces[i].Start = ExtEnd + 1;
							SectionSpaces[i].WithinExtended = false;
						}
					}
				}
			}
			CleanUp:
				// remove all section that are too small
				for ( int i = 0 ; i < SectionSpaces.Count ; i++ ) 
				{
					if ((int)( SectionSpaces[i].End - SectionSpaces[i].Start ) < MINIMUM_FREE_BLOCKS ) 
					{
						SectionSpaces.RemoveAt( i );
						i--;
					}
				}
		}

		protected void SortSectionSpace( SectionSpaceCollection SectionSpaces )
		{
			SECTION_SPACE SaveSection;
			// Sort SectionSpace
			for ( int i = 0 ; i < SectionSpaces.Count - 1 ; i++ ) 
			{
				if (( SectionSpaces[i].Start ) > ( SectionSpaces[ i + 1 ].Start )) 
				{
					SaveSection = SectionSpaces[i+1];
					SectionSpaces.RemoveAt( i+1 );
					SectionSpaces.Insert( i, SaveSection );
					i = -1;
				}
			}
		}

		/// <summary>
		/// PartitionLayoutToSectionSpace
		/// 
		/// Assumes both structures allocated.
		/// DRIVE_LAYOUT_INFORMATION is assumed valid.
		/// </summary>
		/// <returns></returns>
		private void PartitionLayoutToSectionSpace ( PARTITION_INFORMATION [] PartitionEntrys, SectionSpaceCollection SectionSpaces, DRIVE_INFORMATION DriveInfo, VolumeInfoCollection partitionInfos )
		{
			SECTION_SPACE section;
			long BlockCount;
			uint ExtStart = DriveInfo.ExtPartitionStart;
			uint ExtEnd = ExtStart + DriveInfo.ExtPartitionLength;
			bool Extended = DriveInfo.ExtPartitionLength != 0;

			// Put every partition into SectionSpace, set WithinExtended
			for ( int i = 0 ; i < PartitionEntrys.Length ; i++ ) 
			{
				// don't add the extended partition to the collection
				if (( (eWindowsPartitionType)PartitionEntrys[i].PartitionType == eWindowsPartitionType.PARTITION_EXTENDED ) ||
					( (eWindowsPartitionType)PartitionEntrys[i].PartitionType == eWindowsPartitionType.PARTITION_EXTENDED_LBA )) 
				{
					continue;
				}
				if ( PartitionEntrys[i].PartitionNumber > 0 ) 
				{
					section = new SECTION_SPACE();
					section.Start = (uint)( PartitionEntrys[i].StartingOffset / (ulong)DriveInfo.BytesPerSector );
					BlockCount = (long)(PartitionEntrys[i].PartitionLength / (ulong)DriveInfo.BytesPerSector);
					section.End = (uint)(section.Start + BlockCount - 1);
					section.PartitionNumber = PartitionEntrys[i].PartitionNumber;
					section.PartitionType = (eWindowsPartitionType)PartitionEntrys[i].PartitionType;
					section.RecognizedPartition = PartitionEntrys[i].RecognizedPartition == 1;


					// Check if within extended
					section.WithinExtended = (Extended) && (( section.Start >= ExtStart ) && ( section.End <= ExtEnd ));

					// add the symbolic name to the section information
					foreach ( VolumeInfo partitionInfo in partitionInfos ) 
					{
						foreach ( DISK_EXTENT extent in partitionInfo.DiskExtents ) 
						{
							if ( extent.DiskNumber == DriveInfo.PhysicalNumber ) 
							{
								if ( extent.StartingOffset/DriveInfo.BytesPerSector == section.Start ) 
								{
									// found the partition
									section.SectionPath = partitionInfo.SymbolicName;
									//Added by Veera to set the total Size of the Volume
									section.LogicalSize = partitionInfo.LogicalSize;
									goto next;
								}
							}
						}
					}

					//Normal for Shared Drives Veera
//					OmException.LogException( new OmException( "Symbolic Name not found for drive= " + DriveInfo.PhysicalNumber + ", Partition= " + section.Start ) );
				next:
					//  add the section to the collection
					SectionSpaces.Add( section );
				}
			}

		}

		/// <summary>
		/// Fill in the drive Information structure using the SectionSpace
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="SectionSpace"></param>
		/// <param name="DriveInfo"></param>
		public void SectionSpaceToDriveLayout ( IntPtr disk, SectionSpaceCollection SectionSpaces, DRIVE_INFORMATION DriveInfo, int FreeSpaceSectionAvailable )
		{
			SECTION_INFORMATION [] SectionInfos;
			DriveInfo.Sections = new SECTION_INFORMATION[ SectionSpaces.Count ];
			SectionInfos = DriveInfo.Sections;

			int count = 0;
			foreach ( SECTION_SPACE SectionSpace in SectionSpaces ) {
				SectionInfos[ count ] = new SECTION_INFORMATION();
				SECTION_INFORMATION sectionInfo = SectionInfos[ count ];
				if (( SectionSpace.PartitionNumber != 0 ) || 
					( DriveInfo.WindowsDynamic ) ) {
					if (( DriveInfo.WindowsDynamic ) && ( SectionSpace.PartitionType != eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC ) ) {
						// Dynamic, check if unused space
						sectionInfo.SectionUsed = false;
						sectionInfo.SectionAvailable = true;
					} 
					else {
						sectionInfo.SectionUsed = true;
						// set the available flag for this partition
						sectionInfo.SectionAvailable = CheckForFileSystem ( disk, SectionSpace, DriveInfo.BytesPerSector );
					}
				} 
				else {
					if ( SectionSpace.WithinExtended ) {
						sectionInfo.SectionAvailable = true;
					} 
					else if ( FreeSpaceSectionAvailable > 0 ) {
						sectionInfo.SectionAvailable = true;
						//						FreeSpaceSectionAvailable--;
					} 
					else {
						sectionInfo.SectionAvailable = false;
					}
				}
				sectionInfo.PartitionType = SectionSpace.PartitionType;
				sectionInfo.WindowsPartition = SectionSpace.RecognizedPartition;
				sectionInfo.LengthInBlocks = SectionSpace.End - SectionSpace.Start + 1;
				sectionInfo.LengthInBytes = (double)sectionInfo.LengthInBlocks * DriveInfo.BytesPerSector;
				sectionInfo.PartitionNumber = SectionSpace.PartitionNumber;
				sectionInfo.StartingBlock = SectionSpace.Start;
				sectionInfo.SectionNumber = count;
				sectionInfo.WithinExtended = SectionSpace.WithinExtended;

				//Added by Veera
				//To set the total size of the volume
				sectionInfo.LogicalSize = SectionSpace.LogicalSize;

				// By Saumya Tripathi 04/23/03
				sectionInfo.DiskExtents = SectionSpace.DiskExtents;
				
				sectionInfo.SectionPath = SectionSpace.SectionPath;
				// set eDynamicDiskType 
				if ( DriveInfo.WindowsDynamic ) {
					string VolumeName = SectionSpace.SectionName;
					if ( VolumeName != null ) {
						if ( VolumeName.StartsWith( "Volume" ) ) {
							sectionInfo.DynamicDiskType = eDynamicDiskType.Simple;
						} 
						else if ( VolumeName.StartsWith( "Stripe" ) ) {
							sectionInfo.DynamicDiskType = eDynamicDiskType.Stripe;
						} 
						else if ( VolumeName.StartsWith( "Mirror" ) ) {
							sectionInfo.DynamicDiskType = eDynamicDiskType.Mirror;
						} 
						else if ( VolumeName.StartsWith( "Spanned" ) ) {
							sectionInfo.DynamicDiskType = eDynamicDiskType.Spanned;
						}else if ( VolumeName.StartsWith( "Raid" ) ) {	//Added by Veera to check for RAID5
							sectionInfo.DynamicDiskType = eDynamicDiskType.Raid;
						}
					}
				}
				sectionInfo.PercentOfDrive = (byte)(( sectionInfo.LengthInBytes / 
					( (double)DriveInfo.TotalBlocks * (double)DriveInfo.BytesPerSector ) ) * 
					(double)100 );
				// set the partition type string
				int z;
				for ( z = 0 ; z < PartitionTypeTable.Length ; z++ ) {
					if ( PartitionTypeTable[z].Type == sectionInfo.PartitionType ) {
						sectionInfo.TypeDescription = PartitionTypeTable[z].Description;
						break;
					}
				}
				if ( z >= PartitionTypeTable.Length ) {
					OmException.LogException( new OmException( "Error: DiskPartition.dll: SectionSpaceToDriveLayout: Partition Type= " + sectionInfo.PartitionType + ", not found in table." ));
				}
				count++;
			}
		}

		//		protected void SetDriveInfoExtented( DRIVE_LAYOUT_INFORMATION Layout, DRIVE_INFORMATION DriveInfo )
		//		{
		//			// Check for extended partition
		//			for ( int i = 0 ; i < 4 ; i++ ) {
		//				if (( (eWindowsPartitionType)Layout.PartitionEntrys[i].PartitionType == eWindowsPartitionType.PARTITION_EXTENDED ) ||
		//					( (eWindowsPartitionType)Layout.PartitionEntrys[i].PartitionType == eWindowsPartitionType.PARTITION_EXTENDED_LBA )) {
		//					long ExtStart = Layout.PartitionEntrys[i].StartingOffset;
		//					long ExtEnd = ExtStart + Layout.PartitionEntrys[i].PartitionLength - 1;
		//					DriveInfo.ExtPartitionStart = (uint)(ExtStart / (long)DriveInfo.BytesPerSector);
		//					DriveInfo.ExtPartitionLength = (uint)((ExtEnd - ExtStart + 1) / (long)DriveInfo.BytesPerSector);
		//					break;
		//				}
		//			}
		//		}

		/// <summary>
		/// ValidatePartitionLayout
		/// </summary>
		/// <param name="Layout"></param>
		/// <param name="DriveInfo"></param>
		/// <returns></returns>
		public bool ValidatePartitionLayout( DRIVE_LAYOUT_INFORMATION Layout, DRIVE_INFORMATION DriveInfo )
		{
			bool Extended = false;
			long ExtStart = 0;
			long ExtEnd = 0;
			long CurrentStartBlock;
			long CurrentEndBlock;
			long StartBlock;
			long EndBlock;
			long BlockCount;

			// First check count - invalid if greater than 0 and less than 4 
			if ( Layout.PartitionEntrys.Length == 0) {
				return true;
			}
			if ( Layout.PartitionEntrys.Length < 4 ) {
				OmException.LogException(new OmException(String.Format("Error: DiskPartitionAPI: ValidatePartitionLayout failed PartitionCount less then 4 " )));
				return false;
			}

			// Check for extended partition
			for ( int i = 0 ; i < 4 ; i++ ) {
				if (( (eWindowsPartitionType)Layout.PartitionEntrys[i].PartitionType == eWindowsPartitionType.PARTITION_EXTENDED ) ||
					( (eWindowsPartitionType)Layout.PartitionEntrys[i].PartitionType == eWindowsPartitionType.PARTITION_EXTENDED_LBA )) {
					Extended = true;
					ExtStart = (long)Layout.PartitionEntrys[i].StartingOffset;
					ExtEnd = ExtStart + (long)Layout.PartitionEntrys[i].PartitionLength - 1;
					DriveInfo.ExtPartitionStart = (uint)(ExtStart / (long)DriveInfo.BytesPerSector);
					DriveInfo.ExtPartitionLength = (uint)((ExtEnd - ExtStart + 1) / (long)DriveInfo.BytesPerSector);
					break;
				}
			}

			// Check that no partition overlaps another or an extended
			for ( int i = 0 ; i < Layout.PartitionEntrys.Length ; i++ ) {
				if ( Layout.PartitionEntrys[i].PartitionNumber == 0 ) {
					continue;
				}
				CurrentStartBlock = (long)Layout.PartitionEntrys[i].StartingOffset;
				BlockCount = (long)Layout.PartitionEntrys[i].PartitionLength;
				CurrentEndBlock = CurrentStartBlock + BlockCount - 1;

				for ( int x = 0 ; x < Layout.PartitionEntrys.Length ; x++ ) {
					// Check for the same partition - skip
					if ( x == i ) {
						continue;
					}
					// Check only primary and logical  partitions
					if ( Layout.PartitionEntrys[x].PartitionNumber == 0 ) {
						continue;
					}
					// Check for any overlap
					StartBlock = (long)Layout.PartitionEntrys[x].StartingOffset;
					BlockCount = (long)Layout.PartitionEntrys[x].PartitionLength;
					EndBlock = StartBlock + BlockCount - 1;

					if ((( StartBlock >= CurrentStartBlock ) && ( StartBlock <= CurrentEndBlock )) ||
						(( EndBlock >= CurrentStartBlock ) && ( EndBlock <= CurrentEndBlock ))) {
						// Invalid
						OmException.LogException(new OmException(String.Format("Error: DiskPartitionAPI: ValidatePartitionLayout failed Partition over lap." )));
						return false;
					}
				}

				// Check for overlap with extended
				if ( Extended ) {
					if ((( ExtStart >= CurrentStartBlock ) && ( ExtStart <= CurrentEndBlock )) ||
						(( ExtEnd >= CurrentStartBlock ) && ( ExtEnd < CurrentEndBlock ))) {
						// Invalid
						OmException.LogException(new OmException(String.Format("Error: DiskPartitionAPI: ValidatePartitionLayout failed Partition over lap with extended." )));
						return false;
					}
				}
			}
			return true;
		}


		/// <summary>
		/// GetDriveInformation
		/// </summary>
		/// <param name="Drive"></param>
		/// <param name="disk"></param>
		/// <param name="DriveInfo"></param>
		public void	GetDriveInformation ( IntPtr disk, DRIVE_INFORMATION DriveInfo )
		{
			SCSI_ADDRESS ScsiAddress = new SCSI_ADDRESS();
			DISK_GEOMETRY Geometry = new DISK_GEOMETRY();

			DriveInfo.Online = false;

			try {
				// Get SCSI Address
				GetAddress ( disk, ref ScsiAddress );
	
			} 
			catch ( Exception ) {
				// this is the normal exit for COD and offline disks
				return;
			}
			// Set SCSI Address
			DriveInfo.Port = ScsiAddress.PortNumber;
			DriveInfo.Path = ScsiAddress.PathId;
			DriveInfo.Target = ScsiAddress.TargetId;
			DriveInfo.Lun =	ScsiAddress.Lun;

			// Get Geometry
			GetGeometry ( disk, ref Geometry );
	
			// Set Geometry
			DriveInfo.Cylinders = Geometry.Cylinders;
			DriveInfo.Heads = Geometry.TracksPerCylinder;
			DriveInfo.SectorsPerTrack = Geometry.SectorsPerTrack;
			DriveInfo.BytesPerSector = Geometry.BytesPerSector;
			DriveInfo.Capacity = (double)((double)Geometry.BytesPerSector *
				(double)Geometry.SectorsPerTrack * 
				(double)Geometry.TracksPerCylinder * 
				(double)DriveInfo.Cylinders);
			DriveInfo.TotalBlocks = (uint)(DriveInfo.Capacity / DriveInfo.BytesPerSector);

			// Set Capacity String
			DriveInfo.CapacityString = CapacityToString( DriveInfo.Capacity );

			// Get Inquiry Data
			try {
				GetInquiry ( disk, DriveInfo );
			} 
			catch ( OmException e ) {
				OmException exp = new OmException(String.Format("Error: DiskPartitionAPI: GetDriveInformation: caught an exception from GetInquiry, {0}", e.Message ));
				switch ( e.GetLastError ) {
					case 21:
						// device is not ready
					case 1117:
						// The request could not be performed because of an I/O device error. 
					case 1167:
						// device is not connected
					case 116:
						// The request could not be performed because of an I/O device error.
						// log locally only
						OmException.LogException( exp, EventLogEntryType.Warning );
						break;
					default:
						OmException.LogException( exp );
						break;
				}
				return;
			}
			// Get PMF GUID
//			if ( m_DiskGuids != null ) {
//				foreach ( PmfAPI.DISKGUID_INFO diskGuid in m_DiskGuids ) { 
//					if ( diskGuid.DiskNumber == DriveInfo.PhysicalNumber ) {
//						if ( diskGuid.DiskGUID_state == PmfAPI.eDiskGuidState.STATE_DISKGUID_FINE ) {
//							DriveInfo.DiskGuid = diskGuid.DiskGuid;
//						}
//						break;
//					}
//				}
//			}
			DriveInfo.Online = true;
		}

		/// <summary>
		/// GetGeometry
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="Geometry"></param>
		public void GetGeometry ( IntPtr disk, ref DISK_GEOMETRY Geometry )
		{
			int Bytes = Marshal.SizeOf( Geometry );
			IntPtr buffer = Marshal.AllocHGlobal( Bytes );
			try {
				DeviceIOControl.DeviceIoCtrl( disk, DeviceIOControl.IOCTL_DISK_GET_DRIVE_GEOMETRY, IntPtr.Zero, 0, buffer, ref Bytes );

				Marshal.PtrToStructure( buffer, Geometry );

			} 
			finally {
				Marshal.FreeHGlobal( buffer );
			}
		}

		/// <summary>
		/// GetPartitionInfo
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="Layout"></param>
		public static unsafe void GetPartitionInfo( IntPtr disk, ref DRIVE_LAYOUT_INFORMATION Layout )
		{
			int Bytes = Marshal.SizeOf( typeof (PARTITION_INFORMATION) ) * MAX_PARTITION_PER_DRIVE;
			IntPtr buffer = Marshal.AllocHGlobal( Bytes );
			try {
				for ( int i = 0 ; i < Bytes/8 ; i++ ) {
					Marshal.WriteInt64( buffer, i, 0 );
				}

				DeviceIOControl.DeviceIoCtrl( disk, DeviceIOControl.IOCTL_DISK_GET_DRIVE_LAYOUT, IntPtr.Zero, 0, buffer, ref Bytes );

				byte * pPartInfo = (byte*)buffer.ToPointer();
				// now marshal the returned data
				int PartitionCount = Marshal.ReadInt32( (IntPtr)pPartInfo );
				pPartInfo += 4;
				Layout.Signature = (uint)Marshal.ReadInt32( (IntPtr)pPartInfo );
				pPartInfo += 4;
				// allocate the partition array
				Layout.PartitionEntrys = new PARTITION_INFORMATION[ PartitionCount ];
				int size = Marshal.SizeOf ( typeof ( PARTITION_INFORMATION ) );
				for ( int i = 0 ; i < Layout.PartitionEntrys.Length ; i++ ) {
					Layout.PartitionEntrys[i] = new PARTITION_INFORMATION();
					Marshal.PtrToStructure( (IntPtr)pPartInfo, Layout.PartitionEntrys[i] );
					pPartInfo += size;
				}
			} 
			catch (Exception e) {
				throw e;
			}
			finally {
				Marshal.FreeHGlobal( buffer );
			}
		}

		/// <summary>
		/// GetAddress
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="Address"></param>
		public void GetAddress ( IntPtr disk, ref SCSI_ADDRESS Address )
		{
			int Bytes = Marshal.SizeOf ( Address );
			int expected = Bytes;

			IntPtr buffer = Marshal.AllocHGlobal( Bytes );
			try {
				DeviceIOControl.DeviceIoCtrl( disk, DeviceIOControl.IOCTL_SCSI_GET_ADDRESS, IntPtr.Zero, 0, buffer, ref Bytes );
				if ( Bytes != expected ) {
					throw new OmException ( "Invalid out buffer size." );
				}
				Marshal.PtrToStructure( buffer, Address );
			} 
			finally {
				Marshal.FreeHGlobal( buffer );
			}

		}


		/// <summary>
		/// GetInquiry
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="DriveInfo"></param>
		public void GetInquiry ( IntPtr disk, DRIVE_INFORMATION DriveInfo )
		{
			int Bytes = (int)INQUIRY_BUFFER_SIZE;
			DeviceIOControl.SCSI_PASS_THROUGH_DIRECT PassThrough;
			PassThrough = new DeviceIOControl.SCSI_PASS_THROUGH_DIRECT();
			UInt16 PassThroughLength = (UInt16)Marshal.SizeOf ( PassThrough );
			// Get/Set Inquiry Data
			IntPtr buffer = Marshal.AllocHGlobal( Bytes );
			try {
				DeviceIOControl.Zero( buffer, Bytes );

				PassThrough.Length					= PassThroughLength;
				PassThrough.CdbLength				= 6;
				PassThrough.DataIn					= SCSI_IOCTL_DATA_IN;
				PassThrough.DataTransferLength		= INQUIRY_BUFFER_SIZE;
				PassThrough.TimeOutValue			= 100000;
				PassThrough.DataBuffer				= buffer;
				PassThrough.Cdb0					= 0x12;
				PassThrough.Cdb4					= 0xff;

				DeviceIOControl.DeviceIoCtrl( disk, DeviceIOControl.IOCTL_SCSI_PASS_THROUGH_DIRECT, PassThrough, buffer, ref Bytes );

				// check if valid
				if ( PassThrough.ScsiStatus != 0 ) {
					return;
				}

				INQUIRYDATA InquiryData = new INQUIRYDATA();
				Marshal.PtrToStructure ( buffer, InquiryData );


				// Set Name from vendor and product ids

				DriveInfo.Name = "";
				//Checking whether Addinitional Info is present in the INQUIRYDATA
				//Veera 09-11-2003
				if(InquiryData.AdditionalLength > 0) {
					foreach ( char c in InquiryData.VendorId ) {
						DriveInfo.Name += c;
					}
					DriveInfo.Name = DriveInfo.Name.Trim();
				}
				DriveInfo.Model = "";
				//Checking whether Addinitional Info is present in the INQUIRYDATA
				//Veera 09-11-2003
				if(InquiryData.AdditionalLength > 0) {
					foreach ( char c in InquiryData.ProductId ) {
						DriveInfo.Model += c;
					}
					DriveInfo.Model = DriveInfo.Model.Trim();
				}

				// Set Version		
				DriveInfo.Version = "";
				//Checking whether Addinitional Info is present in the INQUIRYDATA
				//Veera 09-11-2003
				if(InquiryData.AdditionalLength > 0) {
					foreach ( char c in InquiryData.ProductRevisionLevel ) {
						DriveInfo.Version += c;
					}
				}
	
				// Set Drive Type
				switch ( InquiryData.Versions & 0x07 ) {
					case 0:
						DriveInfo.DriveType = eDriveType.IDE_DRIVE_TYPE;
						DriveInfo.DriveTypeString = "ATA/IDE";
						break;
					case 1:
					case 2:
						DriveInfo.DriveType = eDriveType.SCSI_BUS_DRIVE_TYPE;
						DriveInfo.DriveTypeString = "SCSI Bus";
						break;
					case 3:
						if ( (InquiryData.SoftReset & 0x20) != 0 ) {
							DriveInfo.DriveType = eDriveType.SCSI_BUS_DRIVE_TYPE;
							DriveInfo.DriveTypeString = "SCSI Bus";
						} 
						else {
							DriveInfo.DriveType = eDriveType.FIBRE_CHANNEL_DRIVE_TYPE;
							DriveInfo.DriveTypeString = "Fibre Channel";
						}
						break;
					default:
						DriveInfo.DriveType = eDriveType.UNKNOWN_DRIVE_TYPE;
						DriveInfo.DriveTypeString = "Unknown";
						break;
				}

				// Get Serial Number only if SCSI - ATA/IDE will fail
				if (( InquiryData.Versions & 0x07) != 0 ) {
					// Get Serial Number
					GetSerialNumber ( disk, ref DriveInfo.SerialNumber );
					// Set Serial Number String
					DriveInfo.SerialNumberString = DriveInfo.SerialNumber;
					DriveInfo.SerialNumberString = DriveInfo.SerialNumberString.Replace( " ", "" );
				}
			} 
			finally {
				Marshal.FreeHGlobal( buffer );
			}
		}

		/// <summary>
		/// GetSerialNumber
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="SerialNumber"></param>
		public unsafe void GetSerialNumber ( IntPtr disk, ref string SerialNumber )
		{
			int Bytes = (int)INQUIRY_BUFFER_SIZE;
			IntPtr buffer = Marshal.AllocHGlobal( Bytes );
			try {
				DeviceIOControl.SCSI_PASS_THROUGH_DIRECT PassThrough;
				PassThrough = new DeviceIOControl.SCSI_PASS_THROUGH_DIRECT();
				UInt16 PassThroughLength = (UInt16)Marshal.SizeOf ( PassThrough );

				for ( int i = 0 ; i < Bytes/8 ; i++ ) {
					Marshal.WriteInt64( buffer, i, 0 );
				}
				PassThrough.Length					= PassThroughLength;
				PassThrough.CdbLength				= 6;
				PassThrough.DataIn					= SCSI_IOCTL_DATA_IN;
				PassThrough.DataTransferLength		= INQUIRY_BUFFER_SIZE;
				PassThrough.TimeOutValue			= 100000;
				PassThrough.DataBuffer				= buffer;
				PassThrough.Cdb0					= 0x12;
				PassThrough.Cdb1					= 0x01;
				PassThrough.Cdb2					= 0x80;
				PassThrough.Cdb4					= 0xff;

				DeviceIOControl.DeviceIoCtrl( disk, DeviceIOControl.IOCTL_SCSI_PASS_THROUGH_DIRECT,	PassThrough, buffer, ref Bytes );

				SerialNumber = Marshal.PtrToStringAnsi( (IntPtr)((byte*)buffer.ToPointer() + 4) );
			} 
			finally {
				Marshal.FreeHGlobal( buffer );
			}

		}

		/// <summary>
		/// CheckForFileSystem
		/// </summary>
		/// <param name="disk"></param>
		/// <param name="sectionSpace"></param>
		/// <returns></returns>
		public bool CheckForFileSystem ( IntPtr disk, SECTION_SPACE sectionSpace, int BytesPerSector )
		{
			//			if(!IsVolumeShowable(m_pDeviceObject)
			//				||IsVolumeLoopBack(m_pDeviceObject)
			//				||!IsDiskInitialized(m_pDeviceObject)) {
			//				return false;
			//			}

			if (( sectionSpace.PartitionNumber == 0 ) &&
				( sectionSpace.PartitionType != eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC )) {
				return false;
			}
			// check if dynamic and not first extent. Then this section is used by windows
			if (( sectionSpace.PartitionType == eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC ) &&
				( !sectionSpace.FirstExtent ) ) {
				return false;
			}


			if (( !sectionSpace.RecognizedPartition ) && 
				( (eWindowsPartitionType)sectionSpace.PartitionType != eWindowsPartitionType.SOFTEK_PARTITION_TYPE )) {
				return false;
			}

			if (((eWindowsPartitionType)sectionSpace.PartitionType != eWindowsPartitionType.SOFTEK_PARTITION_TYPE ) && 
				((eWindowsPartitionType)sectionSpace.PartitionType != eWindowsPartitionType.PARTITION_WINDOWS_DYNAMIC ) &&
				(((eWindowsPartitionType)sectionSpace.PartitionType & ~eWindowsPartitionType.PARTITION_NTFT ) != eWindowsPartitionType.PARTITION_HUGE )) {
				return false;
			}

			// The partition type is HUGE, but it still could be a FAT filesystem.  Read sector 0
			// to see if it is.

			byte [] buffer = new byte[DISK_BLOCK_SIZE];

			try {
				// seek to the beginning for the partition
				Createfile.Seek( disk, sectionSpace.Start * BytesPerSector );
				// read one sector
				Createfile.Read ( disk, ref buffer );
			} 
			catch ( Exception ) {
				return false;
			}

			string name = "" + (char)buffer[0x36] + (char)buffer[0x37] + (char)buffer[0x38];

			if ( name == "FAT" ) {
				return false;
			}
			name = "" + (char)buffer[0x52] + (char)buffer[0x53] + (char)buffer[0x54];

			if ( name == "FAT" ) {
				return false;
			
			}

			name = "" + (char)buffer[0x03] + (char)buffer[0x04] + (char)buffer[0x05] + (char)buffer[0x06];
			if ( name == "NTFS" ) {
				return false;	
			}
			return true;
		}


		/// <summary>
		/// structure for PartitionTypeTable
		/// </summary>
		public struct PARTITION_TYPE
		{
			public eWindowsPartitionType Type;
			public string Description;
			public PARTITION_TYPE ( byte t, string d )
			{
				Type = (eWindowsPartitionType)t;
				Description = d;
			}
		}

		/// <summary>
		/// List all of the known partition types and ascii name
		/// </summary>
		protected PARTITION_TYPE [] PartitionTypeTable = {
															 new PARTITION_TYPE( 0x00, "Empty                          " ),
															 new PARTITION_TYPE( 0x01, "DOS FAT12                      " ),
															 new PARTITION_TYPE( 0x02, "XENIX root                     " ),
															 new PARTITION_TYPE( 0x03, "XENIX /usr                     " ),
															 new PARTITION_TYPE( 0x04, "DOS FAT16 < 32MB               " ),
															 new PARTITION_TYPE( 0x05, "Extended CHS                   " ),
															 new PARTITION_TYPE( 0x06, "DOS FAT16 > 32MB               " ),
															 new PARTITION_TYPE( 0x07, "NTFS/IFS/Advanced UNIX/QNX     " ),
															 new PARTITION_TYPE( 0x08, "OS2/AIX Boot/Split/DOS/DELL/QNX" ),
															 new PARTITION_TYPE( 0x09, "AIX Data/Coherent/QNX          " ),
															 new PARTITION_TYPE( 0x0A, "OS2 Boot/Coherent Swap/OPUS    " ),
															 new PARTITION_TYPE( 0x0B, "FAT32 CHS                      " ),
															 new PARTITION_TYPE( 0x0C, "FAT32 LBA                      " ),
															 new PARTITION_TYPE( 0x0E, "Win95 FAT16 LBA                " ),
															 new PARTITION_TYPE( 0x0F, "Extended LBA                   " ),
															 new PARTITION_TYPE( 0x10, "OPUS                           " ),
															 new PARTITION_TYPE( 0x11, "DOS Hidden FAT12               " ),
															 new PARTITION_TYPE( 0x12, "Config/Diagnostic/Service      " ),
															 new PARTITION_TYPE( 0x14, "DOS Hidden FAT16 < 32MB        " ),
															 new PARTITION_TYPE( 0x16, "DOS Hidden FAT16 > 32MB        " ),
															 new PARTITION_TYPE( 0x17, "Hidden IFS                     " ),
															 new PARTITION_TYPE( 0x18, "AST SmartSleep                 " ),
															 new PARTITION_TYPE( 0x1B, "Hidden FAT32 CHS               " ),
															 new PARTITION_TYPE( 0x1C, "Hidden FAT32 LBA               " ),
															 new PARTITION_TYPE( 0x1E, "Hidden FAT16 LBA               " ),
															 new PARTITION_TYPE( 0x21, "Reserved                       " ),
															 new PARTITION_TYPE( 0x23, "Reserved                       " ),
															 new PARTITION_TYPE( 0x24, "NEC DOS                        " ),
															 new PARTITION_TYPE( 0x26, "Reserved                       " ),
															 new PARTITION_TYPE( 0x31, "Reserved                       " ),
															 new PARTITION_TYPE( 0x32, "NOS                            " ),
															 new PARTITION_TYPE( 0x33, "Reserved                       " ),
															 new PARTITION_TYPE( 0x34, "Reserved                       " ),
															 new PARTITION_TYPE( 0x35, "JFS (OS2 or eCS)               " ),
															 new PARTITION_TYPE( 0x36, "Reserved                       " ),
															 new PARTITION_TYPE( 0x38, "THEOS                          " ),
															 new PARTITION_TYPE( 0x39, "Plan 9/THEOS                   " ),
															 new PARTITION_TYPE( 0x3A, "THEOS                          " ),
															 new PARTITION_TYPE( 0x3B, "THEOS                          " ),
															 new PARTITION_TYPE( 0x3C, "PartitionMagic                 " ),
															 new PARTITION_TYPE( 0x3D, "Hidden Netware                 " ),
															 new PARTITION_TYPE( 0x40, "Venix 80286                    " ),
															 new PARTITION_TYPE( 0x41, "Linux/MINIX/RISC/PPC           " ),
															 new PARTITION_TYPE( 0x42, "Windows Dynamic                " ),
															 new PARTITION_TYPE( 0x43, "Linux/DRDOS                    " ),
															 new PARTITION_TYPE( 0x44, "GoBack                         " ),
															 new PARTITION_TYPE( 0x45, "Boot-US/Priam/EUMEL            " ),
															 new PARTITION_TYPE( 0x46, "EUMEL                          " ),
															 new PARTITION_TYPE( 0x47, "EUMEL                          " ),
															 new PARTITION_TYPE( 0x48, "EUMEL                          " ),
															 new PARTITION_TYPE( 0x4A, "AdaOS/DOS ALFS                 " ),
															 new PARTITION_TYPE( 0x4C, "Oberan                         " ),
															 new PARTITION_TYPE( 0x4D, "QNX4.x                         " ),
															 new PARTITION_TYPE( 0x4E, "QNX4.x 2nd                     " ),
															 new PARTITION_TYPE( 0x4F, "QNX4.x 3rd/Oberon              " ),
															 new PARTITION_TYPE( 0x50, "OnTrack/Lynx/Oberon            " ),
															 new PARTITION_TYPE( 0x51, "OnTrack DM6/Novell             " ),
															 new PARTITION_TYPE( 0x52, "CP/M / SysV/AT                 " ),
															 new PARTITION_TYPE( 0x53, "OnTrack DM6 Aux 3              " ),
															 new PARTITION_TYPE( 0x54, "OnTrack DM6 DDO                " ),
															 new PARTITION_TYPE( 0x55, "EZ-Drive                       " ),
															 new PARTITION_TYPE( 0x56, "Golden Bow/DM EZBIOS           " ),
															 new PARTITION_TYPE( 0x57, "Drive Pro/VNDI                 " ),
															 new PARTITION_TYPE( 0x5C, "Priam EDisk                    " ),
															 new PARTITION_TYPE( 0x61, "SpeedStor                      " ),
															 new PARTITION_TYPE( 0x63, "UNIX System V/Mach/GNU HURD    " ),
															 new PARTITION_TYPE( 0x64, "PC-ARMOUR/Netware              " ),
															 new PARTITION_TYPE( 0x65, "Novell Netware                 " ),
															 new PARTITION_TYPE( 0x66, "Novell Netware SMS             " ),
															 new PARTITION_TYPE( 0x67, "Novell Netware                 " ),
															 new PARTITION_TYPE( 0x68, "Novell Netware                 " ),
															 new PARTITION_TYPE( 0x69, "Novell Netware 5+              " ),
															 new PARTITION_TYPE( 0x6E, "???                            " ),
															 new PARTITION_TYPE( 0x70, "DiskSecure                     " ),
															 new PARTITION_TYPE( 0x71, "Reserved                       " ),
															 new PARTITION_TYPE( 0x73, "Reserved                       " ),
															 new PARTITION_TYPE( 0x74, "Scramdisk/Reserved             " ),
															 new PARTITION_TYPE( 0x75, "IBM PC/IX                      " ),
															 new PARTITION_TYPE( 0x76, "Reserved                       " ),
															 new PARTITION_TYPE( 0x77, "VNDI/M2FS/M2CS                 " ),
															 new PARTITION_TYPE( 0x78, "XOSL FS                        " ),
															 new PARTITION_TYPE( 0x7E, "F.I.X.                         " ),
															 new PARTITION_TYPE( 0x80, "Old Minix                      " ),
															 new PARTITION_TYPE( 0x81, "Minix/Linux/Mitac              " ),
															 new PARTITION_TYPE( 0x82, "Solaris x86/Linux swap         " ),
															 new PARTITION_TYPE( 0x83, "Linux                          " ),
															 new PARTITION_TYPE( 0x84, "OS2/Hibernation                " ),
															 new PARTITION_TYPE( 0x85, "Linux Extended                 " ),
															 new PARTITION_TYPE( 0x86, "Linux RAID/NTFS Volume Set     " ),
															 new PARTITION_TYPE( 0x87, "NTFS Volume Set                " ),
															 new PARTITION_TYPE( 0x8A, "Linux Air-BOOT                 " ),
															 new PARTITION_TYPE( 0x8B, "Fault Tolerant FAT32 Volume    " ),
															 new PARTITION_TYPE( 0x8C, "Fault Tolerant FAT32 Volume LBA" ),
															 new PARTITION_TYPE( 0x8D, "FreeDOS FAT12                  " ),
															 new PARTITION_TYPE( 0x8E, "Linux LVM                      " ),
															 new PARTITION_TYPE( 0x90, "FreeDOS FAT16                  " ),
															 new PARTITION_TYPE( 0x91, "Hidden FreeDOS Extended        " ),
															 new PARTITION_TYPE( 0x92, "Hidden FreeDOS FAT16           " ),
															 new PARTITION_TYPE( 0x93, "Hidden Linux/Amoeba            " ),
															 new PARTITION_TYPE( 0x94, "Amoeba BBT                     " ),
															 new PARTITION_TYPE( 0x95, "MIT EXOPC                      " ),
															 new PARTITION_TYPE( 0x97, "Hidden FreeDOS FAT32 CHS       " ),
															 new PARTITION_TYPE( 0x98, "FreeDOS FAT32 LBA / DataLight  " ),
															 new PARTITION_TYPE( 0x99, "DCE376                         " ),
															 new PARTITION_TYPE( 0x9A, "Hidden FreeDOS FAT16 LBA       " ),
															 new PARTITION_TYPE( 0x9B, "Hidden FreeDOS Extended LBA    " ),
															 new PARTITION_TYPE( 0x9F, "BSD/OS                         " ),
															 new PARTITION_TYPE( 0xA0, "Laptop Hibernation             " ),
															 new PARTITION_TYPE( 0xA1, "Hibernation/HP-SpeedStor       " ),
															 new PARTITION_TYPE( 0xA3, "Reserved                       " ),
															 new PARTITION_TYPE( 0xA4, "Reserved                       " ),
															 new PARTITION_TYPE( 0xA5, "BSD386/FreeBSD/NetBSD          " ),
															 new PARTITION_TYPE( 0xA6, "OpenBSD                        " ),
															 new PARTITION_TYPE( 0xA7, "NeXTSTEP                       " ),
															 new PARTITION_TYPE( 0xA8, "Mac OS-X                       " ),
															 new PARTITION_TYPE( 0xA9, "NetBSD                         " ),
															 new PARTITION_TYPE( 0xAA, "Olivetti FAT12                 " ),
															 new PARTITION_TYPE( 0xAB, "Mac OS-X Boot/GO!              " ),
															 new PARTITION_TYPE( 0xAE, "ShagOS FS                      " ),
															 new PARTITION_TYPE( 0xAF, "ShagOS Swap                    " ),
															 new PARTITION_TYPE( 0xB0, "BootStar Dummy                 " ),
															 new PARTITION_TYPE( 0xB1, "Reserved                       " ),
															 new PARTITION_TYPE( 0xB3, "Reserved                       " ),
															 new PARTITION_TYPE( 0xB4, "Reserved                       " ),
															 new PARTITION_TYPE( 0xB6, "Windows NT Mirror Set FAT16    " ),
															 new PARTITION_TYPE( 0xB7, "NT Mirror Set/BSDI BSD/386 FS  " ),
															 new PARTITION_TYPE( 0xB8, "BSDI BSD/386 Swap              " ),
															 new PARTITION_TYPE( 0xBB, "Hidden Boot Wizard             " ),
															 new PARTITION_TYPE( 0xBE, "Solaris 8                      " ),
															 new PARTITION_TYPE( 0xC0, "CTOS/REAL32/NTFT/DRDOS/Novell  " ),
															 new PARTITION_TYPE( 0xC1, "DRDOS/Secured FAT12            " ),
															 new PARTITION_TYPE( 0xC2, "Hidden Linux/DR DOS            " ),
															 new PARTITION_TYPE( 0xC3, "Hidden Linux swap              " ),
															 new PARTITION_TYPE( 0xC4, "DRDOS/Secured FAT16 < 32MB     " ),
															 new PARTITION_TYPE( 0xC6, "DRDOS/Windows NT               " ),
															 new PARTITION_TYPE( 0xC7, "Windows NT/Syrinx              " ),
															 new PARTITION_TYPE( 0xC8, "Hidden Linux/DR DOS            " ),
															 new PARTITION_TYPE( 0xC9, "Hidden Linux/DR DOS            " ),
															 new PARTITION_TYPE( 0xCA, "Hidden Linux/DR DOS            " ),
															 new PARTITION_TYPE( 0xCB, "DRDOS/Secured FAT32            " ),
															 new PARTITION_TYPE( 0xCC, "DRDOS/Secured FAT32            " ),
															 new PARTITION_TYPE( 0xCD, "CTOS                           " ),
															 new PARTITION_TYPE( 0xCE, "DRDOS/secured FAT16            " ),
															 new PARTITION_TYPE( 0xD0, "REAL/32                        " ),
															 new PARTITION_TYPE( 0xD1, "Old Multiuser DOS              " ),
															 new PARTITION_TYPE( 0xD4, "Old Multiuser DOS              " ),
															 new PARTITION_TYPE( 0xD5, "Old Multiuser DOS              " ),
															 new PARTITION_TYPE( 0xD6, "Old Multiuser DOS              " ),
															 new PARTITION_TYPE( 0xD8, "CP/M-86                        " ),
															 new PARTITION_TYPE( 0xDA, "Non-FS data                    " ),
															 new PARTITION_TYPE( 0xDB, "CPM/DOS/CTOS/KDG               " ),
															 new PARTITION_TYPE( 0xDD, "Hidden CTOS                    " ),
															 new PARTITION_TYPE( 0xDE, "Dell Utility                   " ),
															 new PARTITION_TYPE( 0xDF, "DG/UX / BootIt                 " ),
															 new PARTITION_TYPE( 0xE0, "ST AVFS                        " ),
															 new PARTITION_TYPE( 0xE1, "DOS access/SpeedStor           " ),
															 new PARTITION_TYPE( 0xE3, "DOS / SpeedStor                " ),
															 new PARTITION_TYPE( 0xE4, "SpeedStor                      " ),
															 new PARTITION_TYPE( 0xE5, "Tandy DOS/Reserved             " ),
															 new PARTITION_TYPE( 0xE6, "Reserved                       " ),
															 new PARTITION_TYPE( 0xEB, "BeOS                           " ),
															 new PARTITION_TYPE( 0xED, "Spryt*x                        " ),
															 new PARTITION_TYPE( 0xEE, "EFI GPT                        " ),
															 new PARTITION_TYPE( 0xEF, "EFI FS                         " ),
															 new PARTITION_TYPE( 0xF0, "Linux/PA-RISC                  " ),
															 new PARTITION_TYPE( 0xF1, "SpeedStor                      " ),
															 new PARTITION_TYPE( 0xF2, "DOS secondary                  " ),
															 new PARTITION_TYPE( 0xF3, "Reserved                       " ),
															 new PARTITION_TYPE( 0xF4, "SpeedStor/Prologue             " ),
															 new PARTITION_TYPE( 0xF5, "Prologue                       " ),
															 new PARTITION_TYPE( 0xF6, "Reserved                       " ),
															 new PARTITION_TYPE( 0xF9, "Softek Provisioner             " ),
															 new PARTITION_TYPE( 0xFA, "Reserved                       " ),
															 new PARTITION_TYPE( 0xFB, "VMware                         " ),
															 new PARTITION_TYPE( 0xFC, "VMware                         " ),
															 new PARTITION_TYPE( 0xFD, "Linux raid                     " ),
															 new PARTITION_TYPE( 0xFE, "SS/LS/IBM/Linux/WinNT          " ),
															 new PARTITION_TYPE( 0xFF, "Xenix BBT                      " ),
		};


		//		[DllImport("SetupApi.dll",SetLastError=true)]
		//		extern static bool SetupDiGetDeviceInterfaceDetail (
		//			IntPtr DeviceInfoSet,
		//			ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
		//			IntPtr DeviceInterfaceDetailData, 
		//			int DeviceInterfaceDetailDataSize,
		//			ref int RequiredSize, 
		//			ref SP_DEVINFO_DATA DeviceInfoData 
		//			);
		//
		//
		//		[DllImport("Setupapi", SetLastError = true)]
		//		private static extern IntPtr SetupDiGetClassDevs (
		//			ref Guid ClassGuid,
		//			IntPtr Enumerator,
		//			IntPtr hwndParent,
		//			int Flags
		//			);
		//
		//		[DllImport("Setupapi", SetLastError = true)]
		//		private static extern bool SetupDiDestroyDeviceInfoList (
		//			IntPtr DeviceInfoSet
		//			);
		//
		//
		//		[DllImport("Setupapi", SetLastError = true)]
		//		private static extern bool SetupDiEnumDeviceInterfaces (
		//			IntPtr DeviceInfoSet,
		//			IntPtr DeviceInfoData,
		//			ref Guid InterfaceClassGuid,
		//			int MemberIndex,
		//			out SP_DEVICE_INTERFACE_DATA DeviceInterfaceData
		//			);
		//
		//		protected struct SP_DEVINFO_DATA
		//		{  
		//			public int cbSize;
		//			public Guid ClassGuid;
		//			public int DevInst;
		//			public IntPtr Reserved;
		//		};
		//
		//
		//
		//		private enum eDIGCF
		//		{
		//			DEFAULT = 0x00000001,  // only valid with DIGCF_DEVICEINTERFACE
		//			PRESENT = 0x00000002,
		//			ALLCLASSES = 0x00000004,
		//			PROFILE = 0x00000008,
		//			DEVICEINTERFACE = 0x00000010
		//		}
		//
		//		protected struct SP_DEVICE_INTERFACE_DATA
		//		{
		//			public int Size;
		//			public Guid InterfaceClassGuid;
		//			public int Flags;
		//			public IntPtr Reserved;
		//		};
		//
		//     
		//		private enum eERROR_CODES
		//		{
		//			ERROR_NO_MORE_ITEMS = 259
		//		};


		//		public void GetSymbolicLinks()
		//		{
		//			IntPtr devInfo = IntPtr.Zero;
		//			Guid GUID_DEVCLASS_DISKDRIVE = new Guid( 0x4d36e967, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );
		//			Guid DiskClassGuid = new Guid( 0x53f56307, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b );
		//			Guid VolumeClassGuid = new Guid( 0x53f5630d, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
		//			Guid PartitionClassGuid = new Guid( 0x53f5630a, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
		//			Guid FloppyClassGuid = new Guid( 0x53f56311, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
		//			Guid test = new Guid();
		//
		//			devInfo = SetupDiGetClassDevs(
		//				ref DiskClassGuid,
		//				IntPtr.Zero,
		//				IntPtr.Zero,
		//				(int)( eDIGCF.PROFILE | eDIGCF.DEVICEINTERFACE | eDIGCF.PRESENT )
		//				);
		//			if ( devInfo.ToInt32() == -1 ) {
		//				return;
		//			}
		//
		//			SP_DEVICE_INTERFACE_DATA devInterfaceData = new SP_DEVICE_INTERFACE_DATA();
		//
		//			devInterfaceData.Size = Marshal.SizeOf( devInterfaceData );
		//
		//			for ( int nMemberIndex = 0 ; ; nMemberIndex++ ) {
		//				bool ret;
		//				ret = SetupDiEnumDeviceInterfaces( devInfo, IntPtr.Zero, ref DiskClassGuid, nMemberIndex, out devInterfaceData );
		//				if ( !ret ) {
		//					if ( Marshal.GetLastWin32Error() != (int)eERROR_CODES.ERROR_NO_MORE_ITEMS ) {
		//						throw new OmException( "SetupDiEnumDeviceInterfaces failed " );
		//					}
		//					break;
		//				}
		//
		//				//				int Size;
		//
		//				//				SetupDiGetDeviceInterfaceDetail ( devInfo, &devInterfaceData, null, 0, &Size, null );
		//				//
		//				//				PSP_DEVICE_INTERFACE_DETAIL_DATA devInterfaceDataDetail = 
		//				//					(PSP_DEVICE_INTERFACE_DETAIL_DATA) Marshal.AllocHGlobal ( Marshal.SizeOf ( SP_DEVICE_INTERFACE_DETAIL_DATA ) + Size );
		//				//				devInterfaceDataDetail->cbSize = Marshal.SizeOf ( SP_DEVICE_INTERFACE_DETAIL_DATA );
		//				//
		//				//				SetupDiGetDeviceInterfaceDetail ( devInfo, &devInterfaceData, devInterfaceDataDetail, Size, 0, null );
		//				//
		//				//				trace( devInterfaceDataDetail->DevicePath );
		//				//				Marshal.FreeHGlobal( devInterfaceDataDetail );
		//			}
		//
		//			SetupDiDestroyDeviceInfoList( devInfo );
		//		}
		
		//Error Codes
		private const uint STATUS_SUCCESS = 0x00000000;
		private const uint  STATUS_BUFFER_TOO_SMALL = 0xC0000023;

		private enum eATTRIBUTE
		{
			OBJ_CASE_INSENSITIVE = 0x00000040
		}

		private enum eDIRECTORY
		{
			QUERY = 0x0001,
			TRAVERSE = 0x0002,
			CREATE_OBJECT = 0x0004,
			CREATE_SUBDIRECTORY = 0x0008,
			QUERY_DIRECTORY_BUF_SIZE = 0x200,
		}

		private enum DIRECTORYINFOCLASS
		{
			ObjectArray,
			ObjectByOne
		}


		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode, Pack=4 )]
			private class OBJECT_ATTRIBUTES
		{
			public uint Length;
			public IntPtr RootDirectory;
			public IntPtr ObjectName;
			public uint Attributes;
			public IntPtr SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
			public IntPtr SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
		}

		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
			private class UNICODE_STRING
		{
			public ushort Length;
			public ushort MaximumLength;
			[MarshalAs(UnmanagedType.LPWStr)]
			public string Buffer;

			public void SetString( string str )
			{
				Buffer = str;
				Length = (ushort)(Buffer.Length * 2 );
				MaximumLength = (ushort)(( Buffer.Length + 1 ) * 2 );
			}
		}
		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
			private struct UNICODE_STRING1
		{
			public ushort Length;
			public ushort MaximumLength;
			[MarshalAs(UnmanagedType.LPWStr)]
			public string Buffer;
		};

		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
			private struct OBJECT_NAMETYPE_INFO
		{
			public UNICODE_STRING ObjectName;
			public UNICODE_STRING ObjectType;
		}


		[DllImport("NTDLL", SetLastError = true)]
		private static extern int NtOpenDirectoryObject (
			out IntPtr handle,
			eDIRECTORY directory,
			OBJECT_ATTRIBUTES attribute
			);

		[DllImport("NTDLL", SetLastError = true)]
		private static extern void NtClose (
			IntPtr handle
			);		
		[DllImport("NTDLL", SetLastError = true)]
		private static extern int NtQueryDirectoryObject (
			IntPtr handle,
			IntPtr buffer,
			short size,
			short directoryInfo,
			bool first,
			ref int index,
			ref int retlen
			);
		
		[DllImport("NTDLL", SetLastError = true)]
		private static extern uint ZwQuerySymbolicLinkObject(
			IntPtr LinkHandle,
			IntPtr LinkTarget,
			ref int ReturnedLength //OPTIONAL
			);

		[DllImport("NTDLL", SetLastError = true)]
		private static extern uint ZwOpenSymbolicLinkObject(
			out IntPtr LinkHandle,
			eACCESS_MASK DesiredAccess,
			OBJECT_ATTRIBUTES ObjectAttributes
			);

		[DllImport("NTDLL", SetLastError = true)]
		private static extern int RtlInitUnicodeString(
			UNICODE_STRING DestinationString,
			string SourceString
			);

		/// <summary>
		/// Retrieve all of the dynamic volume information
		/// </summary>
		/// <returns></returns>
		unsafe private VolumeInfoCollection GetVolumes()
		{
			VolumeInfoCollection volumeInfos = new VolumeInfoCollection();
			IntPtr handle = IntPtr.Zero;
			int ret;
			bool first = true;
			int index = 0;
			int retlen = 0;
			OBJECT_ATTRIBUTES attribute = new OBJECT_ATTRIBUTES();
			OBJECT_ATTRIBUTES rootattribute = new OBJECT_ATTRIBUTES();

			//Enumerating the Dynamic Volumes
			string compName = System.Environment.MachineName;
			// set the UNICODE_STRING struct
			UNICODE_STRING path = new UNICODE_STRING();

			//changed to take into account the Dg0 Dg1 etc..
			//Veera 08-04-2003

			UNICODE_STRING rootpath = new UNICODE_STRING();

			rootpath.SetString("\\Device\\HarddiskDmVolumes");


			rootattribute.ObjectName = Marshal.AllocHGlobal( Marshal.SizeOf ( typeof(UNICODE_STRING) ) );


			try {
				Marshal.StructureToPtr( rootpath, rootattribute.ObjectName, false );
				rootattribute.Attributes = (uint)eATTRIBUTE.OBJ_CASE_INSENSITIVE;
				rootattribute.Length = (uint)Marshal.SizeOf( rootattribute );
				rootattribute.RootDirectory = IntPtr.Zero;
				rootattribute.SecurityDescriptor = IntPtr.Zero;
				rootattribute.SecurityQualityOfService = IntPtr.Zero;

				if ( (ret = NtOpenDirectoryObject ( out handle, eDIRECTORY.QUERY, rootattribute )) < 0 ) {
					// no dynamic volumes
					return volumeInfos;
				}
				IntPtr buffer = Marshal.AllocHGlobal( (int)eDIRECTORY.QUERY_DIRECTORY_BUF_SIZE );
				try {
					while( NtQueryDirectoryObject( handle, buffer, (short)eDIRECTORY.QUERY_DIRECTORY_BUF_SIZE, (short)DIRECTORYINFOCLASS.ObjectByOne, first, ref index, ref retlen ) >= 0 ) {
						OBJECT_NAMETYPE_INFO DirectoryObject = new OBJECT_NAMETYPE_INFO();
						DirectoryObject = (OBJECT_NAMETYPE_INFO)Marshal.PtrToStructure( buffer, typeof(OBJECT_NAMETYPE_INFO) );
						if ( DirectoryObject.ObjectName.Buffer.ToLower().StartsWith(compName.ToLower())) {
							path.SetString(rootpath.Buffer+"\\"+DirectoryObject.ObjectName.Buffer);
							break;
						}
						first = false;
					}
				} 
				finally {
					//Checking for Non Null values
					if(handle != IntPtr.Zero) {
						NtClose( handle );
						handle = IntPtr.Zero;
					}
					Marshal.FreeHGlobal( buffer );
					buffer = IntPtr.Zero;
				}
			} 
			finally {
				Marshal.FreeHGlobal( rootattribute.ObjectName );
			}

			//Fixed an issue of DynamicGroup Not available problem.
			if ( (object)path.Buffer == null ) {
				return volumeInfos;
			}

			////////////////////////////////////////

			first = true;
			index = 0;
			retlen = 0;

			                  
//			path.SetString( "\\Device\\HarddiskDmVolumes\\" + compName + "Dg0" );

			// init the OBJECT_ATTRIBUTES
			attribute.ObjectName = Marshal.AllocHGlobal( Marshal.SizeOf ( typeof(UNICODE_STRING) ) );
			try {
				Marshal.StructureToPtr( path, attribute.ObjectName, false );
				attribute.Attributes = (uint)eATTRIBUTE.OBJ_CASE_INSENSITIVE;
				attribute.Length = (uint)Marshal.SizeOf( attribute );
				attribute.RootDirectory = IntPtr.Zero;
				attribute.SecurityDescriptor = IntPtr.Zero;
				attribute.SecurityQualityOfService = IntPtr.Zero;

				if ( (ret = NtOpenDirectoryObject ( out handle, eDIRECTORY.QUERY, attribute )) < 0 ) {
					// no dynamic volumes
					return volumeInfos;
				}
				IntPtr buffer = Marshal.AllocHGlobal( (int)eDIRECTORY.QUERY_DIRECTORY_BUF_SIZE );
				try {
					while( NtQueryDirectoryObject( handle, buffer, (short)eDIRECTORY.QUERY_DIRECTORY_BUF_SIZE, (short)DIRECTORYINFOCLASS.ObjectByOne, first, ref index, ref retlen ) >= 0 ) {
						OBJECT_NAMETYPE_INFO DirectoryObject = new OBJECT_NAMETYPE_INFO();
						DirectoryObject = (OBJECT_NAMETYPE_INFO)Marshal.PtrToStructure( buffer, typeof(OBJECT_NAMETYPE_INFO) );
						if ( DirectoryObject.ObjectType.Buffer == "SymbolicLink" ) {
							VolumeInfo volumeInfo = new VolumeInfo();
							volumeInfo.Name = DirectoryObject.ObjectName.Buffer;
							volumeInfo.SymbolicName = path.Buffer + "\\" + DirectoryObject.ObjectName.Buffer;
							// add the volume info to the collection
							volumeInfos.Add( volumeInfo );
						}
						first = false;
					}
				} 
				finally {
					//Checking for Non Null values
					if(handle != IntPtr.Zero){
						NtClose( handle );
					}
					Marshal.FreeHGlobal( buffer );
				}
			} 
			finally {
				Marshal.FreeHGlobal( attribute.ObjectName );
			}
			return volumeInfos;
		}

		//Added by Veera 05-16-2003
		/// <summary>
		/// This function gets the basic volume information.
		/// This Function takes a different approach to getting the Basic Partitions from the
		/// actual Partitions that are visible to the user. using the Windows Volume GUID's
		/// This avoilds problems with CODSet volumes and any other custome volumes that might
		/// use the partition creation feature.
		/// </summary>
		/// <returns></returns>
		unsafe private VolumeInfoCollection GetBasicVolumes1()
		{
			VolumeInfoCollection volumeInfos = new VolumeInfoCollection();
			IntPtr hVolume = IntPtr.Zero;
			StringBuilder strVolume = new StringBuilder( BUFF_SIZE );
			bool bFlag = true;
			string strSymTarget = "";

			try {
				hVolume = FindFirstVolume ( strVolume, BUFF_SIZE );

				if ( hVolume == new IntPtr ( INVALID_HANDLE_VALUE ) ) {
					throw new OmException("Error: GetBasicVolumes1: FindFirstVolume Failed" );
				}

				strSymTarget = GetSymbolicLink("\\??\\"+strVolume.ToString().Substring(4,strVolume.ToString().Length-5));

				if ( strSymTarget.IndexOf("\\Device\\HarddiskVolume") !=-1 ) {
					VolumeInfo volumeInfo = new VolumeInfo();
					volumeInfo.Name = strSymTarget.Substring(strSymTarget.LastIndexOf("\\")+1);
					volumeInfo.SymbolicName = strSymTarget;
					// add the volume info to the collection
					volumeInfos.Add( volumeInfo );
				}
				//				}

				System.Console.WriteLine( strSymTarget );

				while ( bFlag ) {
					bFlag = FindNextVolume( hVolume,strVolume, BUFF_SIZE );
					if ( bFlag ) {
						strSymTarget = GetSymbolicLink( "\\??\\" + strVolume.ToString().Substring( 4, strVolume.ToString().Length - 5 ) );
						if ( strSymTarget.IndexOf("\\Device\\HarddiskVolume") !=-1 ) {
							VolumeInfo volumeInfo = new VolumeInfo();
							volumeInfo.Name = strSymTarget.Substring(strSymTarget.LastIndexOf("\\")+1);
							volumeInfo.SymbolicName = strSymTarget;
							// add the volume info to the collection
							volumeInfos.Add( volumeInfo );
						}
					}
				}
			}
			catch ( Exception e ) {
				throw new OmException( "Error: GetBasicVolumes1: Error =" + e.Message );
			}
			finally {
				if ( hVolume != IntPtr.Zero ) {
					FindVolumeClose( hVolume );
				}
			}
			return volumeInfos;
		}

		/// <summary>
		/// This function gets the basic volume information.
		/// </summary>
		/// <returns></returns>
		unsafe private VolumeInfoCollection GetBasicVolumes()
		{
			VolumeInfoCollection volumeInfos = new VolumeInfoCollection();
			IntPtr handle = IntPtr.Zero;
			int ret;
			bool first = true;
			int index = 0;
			int retlen = 0;
			OBJECT_ATTRIBUTES attribute = new OBJECT_ATTRIBUTES();

			//Enumerating the Dynamic Volumes
			string compName = System.Environment.MachineName;
			// set the UNICODE_STRING struct
			UNICODE_STRING path = new UNICODE_STRING();
			                  
			path.SetString( "\\Device");

			// init the OBJECT_ATTRIBUTES
			attribute.ObjectName = Marshal.AllocHGlobal( Marshal.SizeOf ( typeof(UNICODE_STRING) ) );
			try {
				Marshal.StructureToPtr( path, attribute.ObjectName, false );
				attribute.Attributes = (uint)eATTRIBUTE.OBJ_CASE_INSENSITIVE;
				attribute.Length = (uint)Marshal.SizeOf( attribute );
				attribute.RootDirectory = IntPtr.Zero;
				attribute.SecurityDescriptor = IntPtr.Zero;
				attribute.SecurityQualityOfService = IntPtr.Zero;

				if ( (ret = NtOpenDirectoryObject ( out handle, eDIRECTORY.QUERY, attribute )) < 0 ) {
					// no basic volumes
					return volumeInfos;
				}
				IntPtr buffer = Marshal.AllocHGlobal( (int)eDIRECTORY.QUERY_DIRECTORY_BUF_SIZE );
				try {
					while( NtQueryDirectoryObject( handle, buffer, (short)eDIRECTORY.QUERY_DIRECTORY_BUF_SIZE, (short)DIRECTORYINFOCLASS.ObjectByOne, first, ref index, ref retlen ) >= 0 ) {
						OBJECT_NAMETYPE_INFO DirectoryObject = new OBJECT_NAMETYPE_INFO();
						DirectoryObject = (OBJECT_NAMETYPE_INFO)Marshal.PtrToStructure( buffer, typeof(OBJECT_NAMETYPE_INFO) );
						if (( DirectoryObject.ObjectType.Buffer == "Device" ) && ( DirectoryObject.ObjectName.Buffer.IndexOf("HarddiskVolume") ) != -1 ) {
							VolumeInfo volumeInfo = new VolumeInfo();
							volumeInfo.Name = DirectoryObject.ObjectName.Buffer;
							volumeInfo.SymbolicName = path.Buffer + "\\" + DirectoryObject.ObjectName.Buffer;
							// add the volume info to the collection
							volumeInfos.Add( volumeInfo );
						}
						first = false;
					}
				}
				finally {
					//Checking if the handle is null or not
					if(handle != IntPtr.Zero){
						NtClose( handle );
					}
					Marshal.FreeHGlobal( buffer );
				}
			} 
			finally {
				Marshal.FreeHGlobal( attribute.ObjectName );
			}
			return volumeInfos;
		}



		private enum eACCESS_MASK
		{
			GENERIC_READ = unchecked( (int)0x80000000 ),
			GENERIC_WRITE = unchecked( (int)0x40000000 ),
			SYNCHRONIZE = unchecked( (int)0x00100000 )
		}

		private enum eATTRIBUTE_MASK
		{
			FILE_OPEN = unchecked( (int)0x00000001),
			FILE_ATTRIBUTE_NORMAL = unchecked( (int)0x00000080 ),
			FILE_SYNCHRONIZE_IO_ALERT = unchecked( (int)0x00000010),
			FILE_SYNCHRONIZE_IO_NONALERT = unchecked( (int)0x00000020)
		}

		private enum eSHARE_MASK
		{
			FILE_SHARE_READ = unchecked( (int)0x00000001),
			FILE_SHARE_WRITE = unchecked( (int)0x00000002)
		}

		private struct IO_STATUS_BLOCK
		{
			int Status;
			IntPtr Information;
		}

		[DllImport("NTDLL", SetLastError = true)]
		private static extern int ZwCreateFile(	out IntPtr FileHandle,
			eACCESS_MASK DesiredAccess,
			OBJECT_ATTRIBUTES ObjectAttributes,
			ref IO_STATUS_BLOCK IoStatusBlock,
			IntPtr AllocationSize,
			FILE_ATTRIBUTE FileAttributes,
			Createfile.eFileShare ShareAccess,
			Createfile.eFileCreate CreateDisposition,
			uint CreateOptions,
			IntPtr EaBuffer,
			uint EaLength
			);

		[DllImport("NTDLL", SetLastError = true)]
		private static extern int ZwCreateFile(	out IntPtr FileHandle,
			eACCESS_MASK DesiredAccess,
			OBJECT_ATTRIBUTES ObjectAttributes,
			ref IO_STATUS_BLOCK IoStatusBlock,
			IntPtr AllocationSize,
			uint FileAttributes,
			uint ShareAccess,
			uint CreateDisposition,
			uint CreateOptions,
			IntPtr EaBuffer,
			uint EaLength
			);


		[DllImport("NTDLL", SetLastError = true)]
		private static extern int ZwClose( IntPtr Handle );

		private enum FILE_ATTRIBUTE
		{
			NORMAL = 0x00000080
		}

		/// <summary>
		/// Open the Volume handle
		/// </summary>
		/// <param name="strDevice"></param>
		private IntPtr OpenDeviceFile( string name )
		{
			IntPtr handle = IntPtr.Zero;
			UNICODE_STRING path = new UNICODE_STRING();
			OBJECT_ATTRIBUTES attribute = new OBJECT_ATTRIBUTES();
			int status;
			IO_STATUS_BLOCK IoStatus = new IO_STATUS_BLOCK();

			// set the UNICODE_STRING struct
			path.SetString( name );

			// init the OBJECT_ATTRIBUTES
			attribute.ObjectName = Marshal.AllocHGlobal( Marshal.SizeOf ( typeof(UNICODE_STRING) ) );
			try {
				Marshal.StructureToPtr( path, attribute.ObjectName, false );
				attribute.Attributes = (uint)eATTRIBUTE.OBJ_CASE_INSENSITIVE;
				attribute.Length = (uint)Marshal.SizeOf( attribute );
				attribute.RootDirectory = IntPtr.Zero;
				attribute.SecurityDescriptor = IntPtr.Zero;
				attribute.SecurityQualityOfService = IntPtr.Zero;

				//				status = ZwCreateFile( out handle, eACCESS_MASK.GENERIC_READ, attribute, ref IoStatus, IntPtr.Zero,
				//					FILE_ATTRIBUTE.NORMAL, Createfile.eFileShare.READ | Createfile.eFileShare.WRITE,
				//					Createfile.eFileCreate.CREATE_NEW, 0, IntPtr.Zero, 0 );
				//Changed by Veera so that only synchronous operatitions exist.
				//05-27-2003

				status = ZwCreateFile( out handle, eACCESS_MASK.GENERIC_READ| eACCESS_MASK.SYNCHRONIZE, attribute,  ref IoStatus, IntPtr.Zero,  // alloc size = none 
					(uint)eATTRIBUTE_MASK.FILE_ATTRIBUTE_NORMAL, (uint)(eSHARE_MASK.FILE_SHARE_READ| eSHARE_MASK.FILE_SHARE_WRITE),  
					(uint)eATTRIBUTE_MASK.FILE_OPEN, (uint)eATTRIBUTE_MASK.FILE_SYNCHRONIZE_IO_NONALERT,  IntPtr.Zero,  0 );  // ealength
			} 
			finally {
				Marshal.FreeHGlobal( attribute.ObjectName );
			}
			if ( handle == IntPtr.Zero ) {
				//Added Error Code to the Exception Message Veera
				throw new OmException( "Error: OpenDeviceFile: ZwCreateFile failed to open volume= " + name +" GetLastError() = "+Marshal.GetLastWin32Error());
			}
			return handle;
		}


		/// <summary>
		/// volume information
		/// </summary>
		private class VolumeInfo
		{
			public string SymbolicName;
			public DiskExtentCollection DiskExtents;
			public string Name;
			//Added by Veera
			//Stores the totalsize of the Volume like for Dynamic disks 
			public ulong LogicalSize;
			public VolumeInfo () {
				DiskExtents = new DiskExtentCollection();
				LogicalSize = 0;
			}
		}

		[Serializable]
			[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode, Pack=8 )]
			public struct DISK_EXTENT
		{
			public int DiskNumber;
			public long StartingOffset;
			public long ExtentLength;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct VOLUME_DISK_EXTENTS
		{
			public int NumberOfDiskExtents;
			public DISK_EXTENT Extents;
		};



		private void GetVolumeSize(IntPtr handle, VolumeInfo volInfo)
		{
			int partitionSize = Marshal.SizeOf(typeof(PARTITION_INFORMATION));
			IntPtr buffer = Marshal.AllocHGlobal(partitionSize);

			try {
				DeviceIOControl.DeviceIoCtrl ( handle, DeviceIOControl.IOCTL_DISK_GET_PARTITION_INFO, IntPtr.Zero, 0, buffer, ref partitionSize );
				PARTITION_INFORMATION partSize = (PARTITION_INFORMATION)Marshal.PtrToStructure(buffer,typeof(PARTITION_INFORMATION));
				volInfo.LogicalSize = (ulong)partSize.PartitionLength;
			} 
			catch ( Exception ) {
				//Modified by Veera
				//This is commented out intentionally as the PMF will block this call when 
				//The volume is protected.
				//				OmException.LogException(new OmException(string.Format("Error:: GetVolumeSize failed:: " + e.Message)));
			}
			finally {
				if((object)buffer != null && buffer != IntPtr.Zero){
					Marshal.FreeHGlobal( buffer );
				}
			}
		}

		public static void GetVolumeInfo( IntPtr handle, ref PARTITION_INFORMATION partitionInfo) {
			int partitionSize = Marshal.SizeOf(typeof(PARTITION_INFORMATION));
			IntPtr buffer = Marshal.AllocHGlobal(partitionSize);

			try {
				DeviceIOControl.DeviceIoCtrl ( handle, DeviceIOControl.IOCTL_DISK_GET_PARTITION_INFO, IntPtr.Zero, 0, buffer, ref partitionSize );
				partitionInfo = (PARTITION_INFORMATION)Marshal.PtrToStructure(buffer,typeof(PARTITION_INFORMATION));
			} 
			catch ( Exception e ) {
				OmException.LogException(new OmException(string.Format("Error:: GetVolumeInfo failed:: " + e.Message)));
			}
			finally {
				if((object)buffer != null && buffer != IntPtr.Zero){
					Marshal.FreeHGlobal( buffer );
				}
			}
		}

		
		//Added by Veera 05-27-2003
		//Returns this error incase of more DISK_EXTENT available
		public static int ERROR_MORE_DATA = 234;


		/// <summary>
		/// Get all of the volume extents.
		/// </summary>
		/// <param name="handle"></param>
		/// <param name="volumeInfo"></param>
		//Changed by Veera 05-27-2003
		//Removed hardcoded 10 max diskextents per volume and made it work for any number
		//of diskextents per volume.
		unsafe private void GetVolumeExtents ( IntPtr handle, DiskExtentCollection DiskExtents )
		{
			int extentSize = Marshal.SizeOf( typeof( DISK_EXTENT ) );
			int size = Marshal.SizeOf( typeof( VOLUME_DISK_EXTENTS ) );
			int ExtentCount=0;
			IntPtr buffer = IntPtr.Zero;

			buffer = Marshal.AllocHGlobal( size );
			try {
				try {
					DeviceIOControl.DeviceIoCtrl ( handle, DeviceIOControl.IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, IntPtr.Zero, 0, buffer, ref size );
				}
				catch(OmException e) {
					if(e.GetLastError == ERROR_MORE_DATA) {
						ExtentCount = Marshal.ReadInt32( buffer );
						Marshal.FreeHGlobal( buffer );
						buffer = IntPtr.Zero;
						
						size = Marshal.SizeOf( typeof( VOLUME_DISK_EXTENTS ) )+(ExtentCount-1)*Marshal.SizeOf( typeof(DISK_EXTENT ) );
						buffer = Marshal.AllocHGlobal(size);

						DeviceIOControl.DeviceIoCtrl ( handle, DeviceIOControl.IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, IntPtr.Zero, 0, buffer, ref size );
					}
					else {
						throw e;
					}
				}
				ExtentCount = Marshal.ReadInt32( buffer );
				DISK_EXTENT DiskExtent = new DISK_EXTENT();
				byte * extent = (byte *)buffer + 8;
				for ( int i = 0 ; i < ExtentCount ; i++ ) {
					DiskExtent = (DISK_EXTENT)Marshal.PtrToStructure( (IntPtr)extent, typeof( DISK_EXTENT ) );
					DiskExtents.Add( DiskExtent );
					extent = extent + extentSize;
				}
			} 
			finally {
				if(buffer != IntPtr.Zero) {
					Marshal.FreeHGlobal( buffer );
				}
			}
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="strInputPath"></param>
		/// <returns></returns>
		static public string GetSymbolicLink ( string strInputPath )
		{
			IntPtr LinkHandle = IntPtr.Zero;
			OBJECT_ATTRIBUTES   cObjectAttributes = new OBJECT_ATTRIBUTES();;
			UNICODE_STRING    cObjectName = new UNICODE_STRING();
			UNICODE_STRING1 cObjectTarget = new UNICODE_STRING1();
			int ulReturnedLength = 0;
			uint status=0;

			IntPtr linkTarget = IntPtr.Zero;

			try {
				cObjectName.SetString( strInputPath );
				cObjectAttributes.ObjectName = Marshal.AllocHGlobal( Marshal.SizeOf ( typeof( UNICODE_STRING ) ) );
				Marshal.StructureToPtr( cObjectName, cObjectAttributes.ObjectName, false );

				cObjectAttributes.Attributes = (uint)eATTRIBUTE.OBJ_CASE_INSENSITIVE;
				cObjectAttributes.Length = (uint)Marshal.SizeOf( cObjectAttributes );
				cObjectAttributes.RootDirectory = IntPtr.Zero;
				cObjectAttributes.SecurityDescriptor = IntPtr.Zero;
				cObjectAttributes.SecurityQualityOfService = IntPtr.Zero;

				status = ZwOpenSymbolicLinkObject ( out LinkHandle, eACCESS_MASK.GENERIC_READ, cObjectAttributes );

				if ( LinkHandle == IntPtr.Zero ) {
					throw new OmException("Error: GetSymbolicLink: Unable to open symbolic link " + strInputPath );
				}

				// start with an initial buffer size of 512
				ulReturnedLength = 512;
			Allocate:
				linkTarget = Marshal.AllocHGlobal( ulReturnedLength + 8 );

				DeviceIOControl.Zero( linkTarget, ulReturnedLength + 8 );

				Marshal.WriteInt16 ( linkTarget, (short)((ulReturnedLength - 2) / 2)  );
				Marshal.WriteInt16 ( linkTarget, 2, (short)(ulReturnedLength / 2) );
				Marshal.WriteInt32 ( linkTarget, 4, linkTarget.ToInt32() + 8 );

				status = ZwQuerySymbolicLinkObject( LinkHandle, linkTarget, ref ulReturnedLength );
				if ( status == STATUS_BUFFER_TOO_SMALL ) {
					Marshal.FreeHGlobal( linkTarget );
					goto Allocate;
				}
				if ( status != STATUS_SUCCESS ) {
					throw new OmException("Error: GetSymbolicLink: Unable to query symbolic link " + strInputPath);
				}
				cObjectTarget = (UNICODE_STRING1)Marshal.PtrToStructure( linkTarget, typeof(UNICODE_STRING1) );
			}
			finally {
				if ( LinkHandle != IntPtr.Zero ) {
					ZwClose ( LinkHandle );
				}
				if ( cObjectAttributes.ObjectName != IntPtr.Zero ) {
					Marshal.FreeHGlobal( cObjectAttributes.ObjectName );
				}
				if ( linkTarget != IntPtr.Zero ) {
					Marshal.FreeHGlobal( linkTarget );
				}
			}
			return cObjectTarget.Buffer;
		}
		
		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern IntPtr FindFirstVolume(
			StringBuilder lpszVolumeName,   // output buffer
			int cchBufferLength    // size of output buffer
			);

		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern bool FindNextVolume(
			IntPtr hFindVolume,
			StringBuilder lpszVolumeName,   // output buffer
			int cchBufferLength    // size of output buffer
			);

		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern bool FindVolumeClose(
			IntPtr hFindVolume
			);

		private const int INVALID_HANDLE_VALUE = -1;
		private const int BUFF_SIZE = 1024;

		//		public int findVolumeIndex ( string strVolumeName )
		//		{
		//			StringBuilder strVolume = new StringBuilder( BUFF_SIZE );
		//			IntPtr hVolume= IntPtr.Zero;
		//			bool bFlag=true;
		//			string strSymTarget="";
		//			int volIndex = 0;
		//			bool bFound = false;
		//
		//			string [] volumes = new string [ 24 ];
		//			string [] names = new string [ 24 ];
		//
		//			
		//
		//			try {
		//				hVolume = FindFirstVolume(strVolume,BUFF_SIZE);
		//
		//				if(hVolume == new IntPtr(INVALID_HANDLE_VALUE)) {
		//					throw new OmException("Error: EnumerateVolumes: FindFirstVolume Failed");
		//				}
		//
		//				strSymTarget = GetSymbolicLink("\\??\\"+strVolume.ToString().Substring(4,strVolume.ToString().Length-5));
		//
		//				if ( strSymTarget.IndexOf( "\\Device\\HarddiskDmVolumes" ) != -1 ) {
		//					if ( strSymTarget.Equals( strVolumeName ) ) {
		//						names[volIndex] = strVolumeName;
		//						bFound = true;
		//					}
		//					volumes[ volIndex++ ] = strVolume.ToString();
		//				}
		//
		//				System.Console.WriteLine( strSymTarget );
		//
		//				while ( bFlag && !bFound ) {
		//					bFlag = FindNextVolume( hVolume,strVolume, BUFF_SIZE );
		//					if ( bFlag ) {
		//						strSymTarget = GetSymbolicLink( "\\??\\" + strVolume.ToString().Substring( 4, strVolume.ToString().Length - 5 ) );
		//						//						System.Console.WriteLine(strSymTarget);
		//						if ( strSymTarget.IndexOf( "\\Device\\HarddiskDmVolumes" ) != -1 ) {
		//							if ( string.Compare ( strSymTarget, strVolumeName, true ) == 0 ) {
		////								bFound = true;
		//								names[volIndex] = strVolumeName;
		////								break;
		//							}
		//							volumes[ volIndex++ ] = strVolume.ToString();
		//						}
		//					}
		//				}
		////				bFlag = FindVolumeClose ( hVolume );
		////				hVolume = IntPtr.Zero;
		//			}
		//			catch ( Exception e ) {
		//				throw new OmException( "Error: findVolumeIndex: Error =" + e.Message );
		//			}
		//			finally {
		//				if ( hVolume != IntPtr.Zero ) {
		//					FindVolumeClose( hVolume );
		//				}
		//			}
		//			for( int i = 0 ; i < volumes.Length - 1 ; i++ ) {
		//				if ( volumes[i+1] == null ) break;
		//				if ( volumes[i].CompareTo( volumes[i+1] ) > 0 ) {
		//					string temp = volumes[i];
		//					volumes[i] = volumes[i+1];
		//					volumes[i+1] = temp;
		//					temp = names[i];
		//					names[i] = names[i+1];
		//					names[i+1] = temp;
		//					i = 0;
		//				}
		//			}
		//			if ( bFound ) {
		//				return volIndex;
		//			}
		//			return -1;
		//		}

		/// <summary>
		/// returns the symbolic guid
		/// </summary>
		/// <param name="strVolumeName"></param>
		/// <returns></returns>
		static public string findVolumeGUID ( string strVolumeName )
		{
			StringBuilder strVolume = new StringBuilder( BUFF_SIZE );
			IntPtr hVolume = IntPtr.Zero;
			bool bFlag = true;
			string strSymTarget = "";

			try {
				hVolume = FindFirstVolume ( strVolume, BUFF_SIZE );

				if ( hVolume == new IntPtr ( INVALID_HANDLE_VALUE ) ) {
					throw new OmException("Error: findVolumeGUID: FindFirstVolume Failed" );
				}

				strSymTarget = GetSymbolicLink("\\??\\"+strVolume.ToString().Substring(4,strVolume.ToString().Length-5));

				//				Removed checking for only dynamic disks
				//				if ( strSymTarget.IndexOf( "\\Device\\HarddiskDmVolumes" ) != -1 ) {
				if ( string.Compare ( strSymTarget, strVolumeName, true ) == 0 ) {
					//Modified by Veera 05-13-2003
					//					return strVolumeName;
					return strVolume.ToString();
				}
				//				}

				System.Console.WriteLine( strSymTarget );

				while ( bFlag ) {
					bFlag = FindNextVolume( hVolume,strVolume, BUFF_SIZE );
					if ( bFlag ) {
						strSymTarget = GetSymbolicLink( "\\??\\" + strVolume.ToString().Substring( 4, strVolume.ToString().Length - 5 ) );
						if ( string.Compare ( strSymTarget, strVolumeName, true ) == 0 ) {
							return strVolume.ToString();
						}
					}
				}
			}
			catch ( Exception e ) {
				throw new OmException( "Error: findVolumeGUID: Error =" + e.Message );
			}
			finally {
				if ( hVolume != IntPtr.Zero ) {
					FindVolumeClose( hVolume );
				}
			}
			throw new OmException( "Error: findVolumeGUID: GUID not found?" );
		}

		[DllImport("ntdll.dll", SetLastError = true)]
		private static extern int NtReadFile(
			IntPtr  FileHandle,
			IntPtr  Event,
			IntPtr  ApcRoutine,
			IntPtr  ApcContext,
			ref IO_STATUS_BLOCK  IoStatusBlock,
			IntPtr  Buffer,
			int  Length,
			ref long  ByteOffset,
			IntPtr  Key
			);

		public bool IsLocalDisk(string diskpath)
		{
			return !IsForeignDisk(diskpath);
		}

		//Rarely \Device\HarddiskX\DRX is becomming wrong so using \??\PhysicalDriveX
		/// <summary>
		/// Checks whether the disk is local or foreign
		/// </summary>
		/// <param name="nDiskNumber">The Disk 	Number
		/// /// </param>
		/// <returns></returns>
		public bool IsLocalDisk(int nDiskNumber)
		{
			return !IsForeignDisk(String.Format("\\??\\PhysicalDrive{0}",nDiskNumber));
		}


		/// <summary>
		/// Checks if the Dynamic Dis local or foreign
		/// </summary>
		/// <param name="diskpath">The DiskPart is of form \Device\HarddiskX\DRX</param>
		/// <returns>false if local disk or true for foreign disk</returns>
		public bool IsForeignDisk(string diskpath)
		{
			byte [] lpBuffer = ZwReadDynamicDisk(diskpath,6,1);

			if(lpBuffer.Length != 0) {
				string compName = System.Environment.MachineName;

				//changed to take into account the Dg0 Dg1 etc..
				//Veera 08-04-2003
				string GroupName = compName+"Dg";

				GroupName = GroupName.ToLower();

				string GroupnameFromDisk ="";

				for(int i=240;i< 240+18;i++) {
					if(lpBuffer[i] == 0)	//Changed from ' ' to 0 Fix
						break;
					GroupnameFromDisk += Convert.ToChar(lpBuffer[i]).ToString();
				}
				GroupnameFromDisk = GroupnameFromDisk.ToLower();
				return !GroupnameFromDisk.StartsWith(GroupName.ToLower());
			}
			else {
				throw new OmException("Error: isForeignDisk: Unable to read the GroupName");
			}
		}

		//Changed Veera 04-21-2003
		//start offset changed from int to long to provide for larger disks
		/// <summary>
		/// Reads Data from the Diskpath The Device name is of form \Device\HarddiskX\DRX
		/// start is offset in sectors
		/// number is size in sectors
		/// </summary>
		public byte [] ZwReadDynamicDisk( string strDeviceName, long start, int number )
		{

			byte [] readdata= null;
			IntPtr hDevice = IntPtr.Zero;

			UNICODE_STRING    ustr = new UNICODE_STRING();
			OBJECT_ATTRIBUTES obj = new OBJECT_ATTRIBUTES();
			int     status;
			IO_STATUS_BLOCK IoStatus = new IO_STATUS_BLOCK();
			IntPtr lpBuffer= IntPtr.Zero;

			ustr.SetString( strDeviceName );

			// init the OBJECT_ATTRIBUTES
			obj.ObjectName = Marshal.AllocHGlobal( Marshal.SizeOf ( typeof(UNICODE_STRING) ) );
			try {
				Marshal.StructureToPtr( ustr, obj.ObjectName, false );
				obj.Attributes = (uint)eATTRIBUTE.OBJ_CASE_INSENSITIVE;
				obj.Length = (uint)Marshal.SizeOf( obj );
				obj.RootDirectory = IntPtr.Zero;
				obj.SecurityDescriptor = IntPtr.Zero;
				obj.SecurityQualityOfService = IntPtr.Zero;

				status = ZwCreateFile( out hDevice,  eACCESS_MASK.GENERIC_READ | eACCESS_MASK.GENERIC_WRITE | eACCESS_MASK.SYNCHRONIZE,
					obj,  ref IoStatus, IntPtr.Zero,  // alloc size = none 
					(uint)eATTRIBUTE_MASK.FILE_ATTRIBUTE_NORMAL, (uint)(eSHARE_MASK.FILE_SHARE_READ| eSHARE_MASK.FILE_SHARE_WRITE),  (uint)eATTRIBUTE_MASK.FILE_OPEN, 
					(uint)eATTRIBUTE_MASK.FILE_SYNCHRONIZE_IO_NONALERT,  IntPtr.Zero,  0 );  // ealength

				if ( status != STATUS_SUCCESS ) {
					throw new OmException("Error: ZwReadDynamicDisk: Unable to open the specified device, status = " + status );
				}
				lpBuffer = Marshal.AllocHGlobal( number * 512 + 1 );

				// clear the buffer
				DeviceIOControl.Zero( lpBuffer, number * 512 + 1 );
				//				for ( int k = 0 ; k < number * 512 + 1 ; k++ ) {
				//					Marshal.WriteByte( lpBuffer, k, 0 );
				//				}

				long foffset = (long)(start << 9);

				IO_STATUS_BLOCK iostat = new IO_STATUS_BLOCK();
				//				int dwLength=0;
				status = NtReadFile ( hDevice, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, ref iostat, lpBuffer, number*512, ref foffset, IntPtr.Zero );
				if ( status != STATUS_SUCCESS ) {
					throw new OmException("Error: ZwReadDynamicDisk: Unable to read the sectors from disk " + strDeviceName + ", status = " + status );
				}
				readdata = new byte[number*512];
				if ( readdata == null ) {
					throw new OmException("Error: ZwReadDynamicDisk: Unable to allocate memory for read bytes");
				}

				Marshal.Copy(lpBuffer,readdata,0,number*512);
			}
//			catch(OmException e) 
//			{
//				throw e;
//			}
			finally {
				int error = Marshal.GetLastWin32Error();
				if(hDevice != IntPtr.Zero) {
					ZwClose(hDevice);
				}
				if(lpBuffer != IntPtr.Zero) {
					Marshal.FreeHGlobal(lpBuffer);
				}
			}

			return readdata;
		}

	}
}

