using System;
using System.IO;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;


namespace MasterThread {
	/// <summary>
	/// Summary description for Win32API.
	/// </summary>

	public class DriveInfo {
		public enum eDriveType
		{
			DRIVE_UNKNOWN,
			DRIVE_NO_ROOT_DIR,
			DRIVE_REMOVABLE,
			DRIVE_FIXED,
			DRIVE_REMOTE,
			DRIVE_CDROM,
			DRIVE_RAMDISK
		}

		[DllImport("Kernel32.dll", SetLastError = true)]
		public static extern eDriveType GetDriveType( string lpDeviceName );

		[DllImport("Kernel32.dll", EntryPoint = "GetDiskFreeSpaceA", SetLastError = true)]
		public static extern bool GetDiskFreeSpace ( string lpRootPathName , ref int lpSectorsPerCluster, ref int lpBytesPerSector, ref int lpNumberOfFreeClusters, ref int lpTotalNumberOfClusters );
		[DllImport("Kernel32.dll", EntryPoint = "GetDiskFreeSpaceExA", SetLastError = true)]
		public static extern bool GetDiskFreeSpaceEx ( string lpRootPathName , out ulong lpFreeBytesAvailable, out ulong lpTotalNumberOfBytes, out ulong lpTotalNumberOfFreeBytes );
		[DllImport("Kernel32.dll", EntryPoint = "GetVolumeInformationA", SetLastError = true)]
		public static extern bool GetVolumeInformation ( string lpRootPathName, StringBuilder pVolumeNameBuffer, int nVolumeNameSize, out int lpVolumeSerialNumber, out int lpMaximumComponentLength, out int lpFileSystemFlags, StringBuilder lpFileSystemNameBuffer, int nFileSystemNameSize );

		// GetDriveName parameter
		string m_Drive;

		// GetVolumeInformation parameters
		string m_VolumeNameBuffer;
		int m_SerialNumber;
		int m_MaxFileNameLength;
		int m_FileSystemFlags;
		string m_FileSystemName;

		// GetDiskFreeSpaceEx parameters
		ulong m_FreeBytes;
		ulong m_TotalBytes;
		ulong m_TotalNumberOfFreeBytes;

		// GetDiskFreeSpaceEx parameters
		int m_SectorsPerCluster;
		int m_BytesPerSector;
		int m_NumberOfFreeClusters;
		int m_TotalNumberOfClusters;


		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="strDrive"></param>
		public DriveInfo() {

		}
		public void GetDriveInfo( string Drive ) {
			if ( IsValidDriveLetter( Drive ) ) {
				m_Drive = Drive;

				StringBuilder sbVolumeName = new StringBuilder(256);
				StringBuilder sbSystemName = new StringBuilder(256);
				bool iRes;
				iRes = GetVolumeInformation( Drive, sbVolumeName, sbVolumeName.Capacity, out m_SerialNumber, out m_MaxFileNameLength, out m_FileSystemFlags, sbSystemName, sbSystemName.Capacity);

				if ( iRes ) {
					m_VolumeNameBuffer = sbVolumeName.ToString();
					m_FileSystemName = sbSystemName.ToString();

					// get the disk free space
					DiskFreeSpaceEx( Drive );
				} else {
					throw new Exception("Error during GetVolumeInformation method call");
				}
			} else {
				throw new ArgumentException("Invalid drive name specified.");
			}
		}

		public long GetTotalSpace( string Drive ) {
			m_Drive = Drive;
			if ( GetDiskFreeSpace( Drive, ref m_SectorsPerCluster, ref m_BytesPerSector, ref m_NumberOfFreeClusters, ref m_TotalNumberOfClusters ) == false ) {
				throw new Exception("Error: DiskFreeSpace Unable to read drive data");
			}
			return m_BytesPerSector * m_SectorsPerCluster * m_TotalNumberOfClusters;
		}
		public long GetFreeSpace( string Drive ) {
			m_Drive = Drive;
			if ( GetDiskFreeSpace( Drive, ref m_SectorsPerCluster, ref m_BytesPerSector, ref m_NumberOfFreeClusters, ref m_TotalNumberOfClusters ) == false ) {
				throw new Exception("Error: DiskFreeSpace Unable to read drive data");
			}
			return m_BytesPerSector * m_SectorsPerCluster * m_NumberOfFreeClusters;
		}

		public void DiskFreeSpaceEx( string Drive )
		{
			m_Drive = Drive;
			if ( GetDiskFreeSpaceEx( Drive, out m_FreeBytes, out m_TotalBytes, out m_TotalNumberOfFreeBytes ) == false ) {
				throw new Exception("Error: DiskFreeSpaceEx Unable to read drive data");
			}
		}


		public bool IsValidDriveLetter( string Drive ) {
			string [] Drives;

			Drives = Directory.GetLogicalDrives();
			foreach ( string drive in Drives ) {
				if ( drive.Equals( Drive ) ) {
					return true;
				}
			}
			return false;
		}

		public string GetVolumeName {
			get { return m_VolumeNameBuffer; }
		}
		public string GetDriveName {
			get { return m_Drive; }
		}

		public string GetFileSystemName {
			get { return m_FileSystemName; }
		}

		public int GetSerialNumber {
			get { return m_SerialNumber; }
		}
		public ulong GetFreeBytes {
			get { return m_FreeBytes; }
		}
		public ulong GetTotalBytes {
			get { return m_TotalBytes; }
		}
		public ulong TotalNumberOfFreeBytes {
			get { return m_TotalNumberOfFreeBytes; }
		}	
	}


	public class VolumeMountPoint {

		public static IntPtr INVALID_HANDLE_VALUE = (IntPtr)(-1);

		IntPtr m_Handle = IntPtr.Zero;

		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern IntPtr FindFirstVolumeMountPoint (
			string szVolumeGuid,			// root path of volume to be scanned
			StringBuilder PtBuf,			// pointer to output string
			int dwBufSize					// size of output buffer
			);

		public bool FindFirstVolumeMountPoint ( string VolumeGuid, StringBuilder buf ) {
			m_Handle = FindFirstVolumeMountPoint( VolumeGuid, buf, buf.Capacity );
			if ( m_Handle == INVALID_HANDLE_VALUE ) {
				return false;
			}
			return true;
		}


		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern bool FindVolumeMountPointClose ( IntPtr handle );

		public void CloseVolumeMountPoint( ) {
			if ( m_Handle != IntPtr.Zero ) {
				FindVolumeMountPointClose( m_Handle );
			}
		}


		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern bool FindNextVolumeMountPoint (
			IntPtr handle,
			StringBuilder buf,				// pointer to output string
			int BufSize						// size of output buffer
			);

		public bool FindNextVolumeMountPoint ( StringBuilder buf ) {
			bool ret = FindNextVolumeMountPoint( m_Handle, buf, buf.Capacity );
			return ret;
		}


		[DllImport("Kernel32.dll", SetLastError = true)]
		private static extern bool GetVolumeNameForVolumeMountPoint (
			[MarshalAs(UnmanagedType.LPStr)]
			string lpszVolumeMountPoint,    // input volume mount point or directory
			IntPtr buf,						// pointer to output string
			int BufSize						// size of output buffer
			);

		public static string GetVolumeNameForVolMntPt ( string VolumeMntPt ) {
			IntPtr buf = Marshal.AllocHGlobal( Globals._MAX_PATH );
			try {
				bool ret = GetVolumeNameForVolumeMountPoint( VolumeMntPt, buf, Globals._MAX_PATH );
				if ( ! ret ) {
					throw new OmException( "Error: GetVolumeNameForVolMntPt failed for - " + VolumeMntPt );
				}
				return Marshal.PtrToStringAnsi( buf );
			} finally {
				Marshal.FreeHGlobal( buf );
			}
		}
	}

	public class Memory {
		public int dwLength;
		public int dwMemoryLoad;
		public long ullTotalPhys;
		public long ullAvailPhys;
		public long ullTotalPageFile;
		public long ullAvailPageFile;
		public long ullTotalVirtual;
		public long ullAvailVirtual;
		public long ullAvailExtendedVirtual;

		[DllImport("Kernel32.dll", SetLastError = true)]
		public static extern bool GlobalMemoryStatusEx( Memory lpBuffer );

		public Memory() {
			dwLength = Marshal.SizeOf( this );
			Memory.GlobalMemoryStatusEx ( this );
		}
	}

	public class Handle {

		public enum eOptions {
			DUPLICATE_SAME_ACCESS = 0x00000002
		}

		[DllImport("Kernel32.dll", SetLastError = true)]
		public static extern Int32 DuplicateHandle( IntPtr hSourceProcessHandle,
			IntPtr hSourceHandle,
			IntPtr hTargetProcessHandle,
			out IntPtr lpTargetHandle,
			Int32 dwDesiredAccess,
			bool bInheritHandle,
			eOptions dwOptions );

		public static int Duplicate ( IntPtr source, out IntPtr target ) {
			Process process = Process.GetCurrentProcess();
			return DuplicateHandle( process.Handle, source, 
				process.Handle, out target, 0, true, eOptions.DUPLICATE_SAME_ACCESS );
		}
	}






}

