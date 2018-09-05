using System;
using System.IO;
using System.Collections;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32;

namespace ServerFilterEditor
{
	/// <summary>
	/// Summary description for ServerFilter.
	/// </summary>
	public class ServerFilter
	{
		private string     m_strFileSource = @"SourceFilter.txt";
		private string     m_strFileTarget = @"TargetFilter.txt";

		private bool       m_bSourceOnly;
		private bool       m_bIncludeSource;
		private SortedList m_SLDevicesSource = new SortedList();
		private bool       m_bIncludeTarget;
		private SortedList m_SLDevicesTarget = new SortedList();

		public ServerFilter()
		{
			string strPath = @"c:\";

			RegistryKey rk = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Softek\\Dtc\\CurrentVersion", true);
			strPath = (String)rk.GetValue("InstallPath");

			m_strFileSource = strPath + "\\" + m_strFileSource;
			m_strFileTarget = strPath + "\\" + m_strFileTarget;
		}

		[DllImport("kernel32.dll", SetLastError=true)] 
		private static extern int  GetDriveType(string  nDrive);

		const int DRIVE_UNKNOWN     = 0;
		const int DRIVE_NO_ROOT_DIR = 1;
		const int DRIVE_REMOVABLE   = 2; 
		const int DRIVE_FIXED       = 3; 
		const int DRIVE_REMOTE      = 4; 
		const int DRIVE_CDROM       = 5; 
		const int DRIVE_RAMDISK     = 6; 

		[DllImport("kernel32.dll", SetLastError=true)] 
		private static extern IntPtr FindFirstVolumeMountPoint(string str, StringBuilder strMountPt, ref int len);
		[DllImport("kernel32.dll", SetLastError=true)] 
		private static extern int FindNextVolumeMountPoint(IntPtr hDrive, StringBuilder strMountPt, ref int len);
		[DllImport("kernel32.dll", SetLastError=true)]
		private static extern int FindVolumeMountPointClose(IntPtr hDrive);

		public bool SourceOnly
		{
			get
			{
				return m_bSourceOnly;
			}
			set
			{
				m_bSourceOnly = value;
			}
		}

		public bool IncludeSource
		{
			get
			{
				return m_bIncludeSource;
			}
			set
			{
				m_bIncludeSource = value;
			}
		}

		public bool IncludeTarget
		{
			get
			{
				return m_bIncludeTarget;
			}
			set
			{
				m_bIncludeTarget = value;
			}
		}

		public SortedList DevicesSource
		{
			get
			{
				return m_SLDevicesSource;
			}
		}

		public SortedList DevicesTarget
		{
			get
			{
				return m_SLDevicesTarget;
			}
		}

		private bool loadFile(string strFilename, ref bool bIncludeDevice, ref SortedList SLDevices,  ref int nNbSelectedDevice)
		{
			nNbSelectedDevice = 0;

			if (File.Exists(strFilename))
			{
				// Open the file to read from.
				using (StreamReader sr = File.OpenText(strFilename)) 
				{
					string s = "";
					while ((s = sr.ReadLine()) != null) 
					{
						bool bInclude = (String.Compare(s, "Include", true) == 0);
						bool bExclude = (String.Compare(s, "Exclude", true) == 0);

						if ((bInclude == false) && (bExclude == false))
						{
							// Skip lines that are before the include/exclude statement
						}
						else
						{
							bIncludeDevice = bInclude;

							while ((s = sr.ReadLine()) != null) 
							{
								Device device = (Device)SLDevices[s]; 
								if (device != null)
								{
									device.Selected = true;
									nNbSelectedDevice++;
								}
								else
								{
									// Error: Unknown device
									// Ignore it
								}
							}
						}
					}
				}
			}
			else
			{
				// By default, include all
				foreach (DictionaryEntry DEDevice in SLDevices)
				{
					Device device = (Device)DEDevice.Value;
					device.Selected = true;
					nNbSelectedDevice++;
				}
			}

			return true;
		}

		public bool Load()
		{
			// Build internal OM
			m_bSourceOnly = false;
			
			m_bIncludeSource = true;
			m_SLDevicesSource.Clear();

			m_bIncludeTarget = true;
			m_SLDevicesTarget.Clear();

			// Load server's devices
			// build list of unselected devices
			try 
			{
				string[] drives = System.IO.Directory.GetLogicalDrives();
				foreach (string str in drives) 
				{
					try
					{
						int iDrive = GetDriveType(str);
						if ((iDrive != DRIVE_REMOVABLE) && (iDrive != DRIVE_CDROM) && (iDrive != DRIVE_RAMDISK) &&	(iDrive != DRIVE_REMOTE))
						{
							Device deviceSource = new Device(str);
							m_SLDevicesSource[str] = deviceSource;
							Device deviceTarget = new Device(str);
							m_SLDevicesTarget[str] = deviceTarget;

							// Mount points
							int len = 256;
							StringBuilder SBuilder = new StringBuilder((int)len);
							IntPtr hDrive = FindFirstVolumeMountPoint(str, SBuilder, ref len);
							if (hDrive.ToInt32() != -1)
							{
								do
								{
									string strMountPt = str + SBuilder.ToString();
									deviceSource = new Device(strMountPt);
									m_SLDevicesSource[strMountPt] = deviceSource;
									deviceTarget = new Device(strMountPt);
									m_SLDevicesTarget[strMountPt] = deviceTarget;

								} while (FindNextVolumeMountPoint(hDrive, SBuilder, ref len) > 0);

								FindVolumeMountPointClose(hDrive);
							}
						}
					}
					catch
					{
						System.Console.WriteLine("An Add error occurs.");
					}
				}
			}
			catch (System.IO.IOException) 
			{
				System.Console.WriteLine("An I/O error occurs.");
				return false;
			}
			catch (System.Security.SecurityException) 
			{
				System.Console.WriteLine("The caller does not have the required permission.");
				return false;
			}

			int nNbSelectedSource = 0;
			int nNbSelectedTarget = 0;

			loadFile(m_strFileSource, ref m_bIncludeSource, ref m_SLDevicesSource, ref nNbSelectedSource);
			loadFile(m_strFileTarget, ref m_bIncludeTarget, ref m_SLDevicesTarget, ref nNbSelectedTarget);

			// Check if it's a Source only Server
			if (((m_bIncludeTarget == false) && (nNbSelectedSource == m_SLDevicesSource.Count)) || 
				(m_bIncludeTarget && (nNbSelectedTarget == 0)))
			{
				m_bSourceOnly = true;
			}

			return true;
		}

		private bool SaveFile(string strFilename, bool bSourceFile)
		{
			// Save to file
			using (StreamWriter sw = File.CreateText(strFilename))
			{
				if (m_bSourceOnly && (bSourceFile == false))
				{
					sw.WriteLine("Include");
				}
				else
				{
					sw.WriteLine((bSourceFile ? m_bIncludeSource : m_bIncludeTarget) ? "Include" : "Exclude");
					int nCount = (bSourceFile ? m_SLDevicesSource : m_SLDevicesSource).Count;
					for (int nIndex = 0; nIndex < nCount; nIndex++)
					{
						Device device = (Device)(bSourceFile ? m_SLDevicesSource : m_SLDevicesTarget).GetByIndex(nIndex);
						if (device.Selected)
						{
							sw.WriteLine(device.Name);
						}
					}
				}
			}

			return true;
		}

		public bool Save()
		{
			SaveFile(m_strFileSource, true);
			SaveFile(m_strFileTarget, false);

			return true;
		}
	}
}
