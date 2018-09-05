using System;
using System.Threading;
using System.Runtime.InteropServices;

namespace MasterThread.Management
{
	/// <summary>
	/// Summary description for Performance.
	/// </summary>
	public class Performance
	{

		public static string SHARED_MEMORY_OBJECT_NAME	= "Global\\DTC_PERF_DATA";
		public static string SHARED_MEMORY_MUTEX_NAME	= "Global\\DTC_PERF_DATA_MUTEX";
		public static string FTD_PERF_GET_EVENT_NAME	= "Global\\DTCperfget";
		public static string FTD_PERF_SET_EVENT_NAME	= "Global\\DTCperfset";
		public static int SHARED_MEMORY_MUTEX_TIMEOUT	= 1000;

		/// <summary>
		/// Constructor
		/// </summary>
		public Performance()
		{
		}

		public struct ftd_perf_t {
			public eMagic magicvalue;		/* so we know it's been initialized	*/
			public IntPtr hSharedMemory;
			public Mutex hMutex;
			public AutoResetEvent hGetEvent;
			public IntPtr hSetEvent;
			public IntPtr MapFileViewData;
			//public Protocol.ftd_perf_instance_t [] pData;
		}

		/// <summary>
		/// ftd_perf_create -- create a ftd_perf_t object
		/// </summary>
		/// <returns></returns>
		public ftd_perf_t ftd_perf_create() {
			ftd_perf_t perfp = new ftd_perf_t();

			perfp.magicvalue = eMagic.PERFMAGIC;
			return perfp;
		}

		[DllImport("Kernel32.dll", SetLastError = true)]
		public static extern IntPtr CreateFileMapping( IntPtr hFile,
			object lpAttributes,
			Int32 flProtect,
			Int32 dwMaximumSizeHigh,
			Int32 dwMaximumSizeLow,
			string lpName );

		[DllImport("Kernel32.dll", SetLastError = true)]
		public static extern IntPtr MapViewOfFile( IntPtr hFileMappingObject,
			Int32 dwDesiredAccess,
			Int32 dwFileOffsetHigh,
			Int32 dwFileOffsetLow,
			Int32 dwNumberOfBytesToMap );

		[DllImport("Kernel32.dll", SetLastError = true)]
		public static extern bool UnmapViewOfFile( IntPtr lpBaseAddress	);

		public static int PAGE_READWRITE = 0x04;
		public static int ERROR_ALREADY_EXISTS = 183;

		public static int FILE_MAP_WRITE = 0x0002;

		[DllImport("RplLibWrapper.dll", SetLastError = true)]
		public static unsafe extern int SharedMemoryObjectSize();


		public static void ftd_perf_init( ftd_perf_t perfp, bool bReadOnlyAccess ) {
//			int Status;
//			int CreateFileMappingStatus;
//			bool bMutex;

			// now create the file mapping
//			perfp.hSharedMemory = CreateFileMapping(
//				new IntPtr( 0xFFFFFFFF ),	// to create a temp file
//				null,						// default security
//				PAGE_READWRITE,			    // to allow read & write access
//				0,
//				SharedMemoryObjectSize(),	// file size
//				SHARED_MEMORY_OBJECT_NAME );// object name
//
//			// to see if this is the first opening
//			CreateFileMappingStatus = Marshal.GetLastWin32Error();
//
//			// return error if unsuccessful
//			if ( perfp.hSharedMemory == IntPtr.Zero ) {
//				// this is fatal, if we can't get data then there's no point in continuing.
//				throw new OmException( "Error: ftd_perf_init CreateFileMapping failed" );
//			}
//
//			// the application memory file was created/access successfully
//			// so we need to get the sync. events and mutex before we use it
//			perfp.hMutex = new Mutex( false, SHARED_MEMORY_MUTEX_NAME );
//
//			perfp.hGetEvent = CreateEvent(&sa, FALSE, FALSE, FTD_PERF_GET_EVENT_NAME);
//			if ( perfp.hGetEvent == null ) {
//				CloseHandle(perfp.hMutex);
//				throw new OmException( "Error: ftd_perf_init CreateEvent failed" );
//			}
//
//			perfp.hSetEvent = CreateEvent(&sa, FALSE, FALSE, FTD_PERF_SET_EVENT_NAME);
//			if ( perfp.hSetEvent == null ) {
//				CloseHandle(perfp.hGetEvent);
//				CloseHandle(perfp.hMutex);
//				throw new OmException( "Error: ftd_perf_init CreateEvent failed" );
//			}
//
//			// if successful then wait for ownership, otherwise,
//			// we'll just take our chances.
//			bMutex = perfp.hMutex.WaitOne ( SHARED_MEMORY_MUTEX_TIMEOUT, true );
//			try {
//				if ( perfp.MapFileViewData != IntPtr.Zero ) {
//					UnmapViewOfFile( perfp.MapFileViewData );
//					CloseHandle( perfp.hSharedMemory );
//				}
//
//				if ( CreateFileMappingStatus == ERROR_ALREADY_EXISTS ) {
//					// the status is ERROR_ALREADY_EXISTS which is successful
//					return;
//				}
//
//				// this is the first access to the file so initialize the
//				// instance count
//				perfp.MapFileViewData = MapViewOfFile(
//					perfp.hSharedMemory,  // shared mem handle
//					FILE_MAP_WRITE,         // access desired
//					0,                      // starting offset
//					0,
//					0);                     // map the entire object
//
//				if ( perfp.MapFileViewData == IntPtr.Zero ) {
//					// this is fatal, if we can't get data then there's no point in continuing.
//					throw new OmException( "Error: ftd_perf_init CreateEvent failed" );
//				} 
//			} finally {
//				// done with the shared memory so free the mutex if one was acquired
//				if ( bMutex ) {
//					perfp.hMutex.ReleaseMutex();
//				}
//			}
		}

	}
}
