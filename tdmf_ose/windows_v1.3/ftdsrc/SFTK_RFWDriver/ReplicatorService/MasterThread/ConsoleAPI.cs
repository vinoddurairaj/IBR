using System;
using System.IO;
using System.Windows.Forms;
using System.Runtime.Serialization;
using System.Runtime.InteropServices;

namespace MasterThread
{
	/// <summary>
	/// Summary description for ConsoleAPI.
	/// </summary>
	/// 
	public class ConsoleAPI : System.MarshalByRefObject {
		public ConsoleAPI() {
		}

		/// <summary>
		/// Returns a GetConsoleApi object for this machine.
		/// </summary>
		/// <returns></returns>
		public static ConsoleAPI GetConsoleApi() {
			return GetConsoleApi( System.Environment.MachineName );
		}
		/// <summary>
		/// Returns a GetConsoleApi object for the machine.
		/// </summary>
		/// <param name="s3Server"></param>
		/// <returns></returns>
		public static ConsoleAPI GetConsoleApi( string server ) {
			return GetConsoleApi( server, RegistryAccess.MasterPort() );
		}
		public static ConsoleAPI GetConsoleApi( string server, int port ) {
			ConsoleAPI consoleAPI;
			port += 2;
			consoleAPI = (ConsoleAPI)Activator.GetObject (
				typeof( ConsoleAPI ),
				"tcp://" + server + ":" + port + "/ConsoleAPI" );
			return consoleAPI;
		}

		public Management.Protocol.DeviceInfoCollection GetAllDeviceInfos() {
			return Management.Management.ftd_mngt_acquire_alldevices();
		}

		// Used by console
		// get all primary configs 
		public LgConfigCollection GetAllPrimaryConfigsXML () {
			return Config.GetPrimaryLgConfigsXML();
		}


		public void CheckPointStopPrimary( int lgnum ) {
			// Send CP Stop to driver.
			DriverIOCTL driver;
			driver = new DriverIOCTL();
			try {
				driver.ps_set_group_checkpoint( lgnum, false );
			} finally {
				driver.Close();
			}	
		}


		// Used by the console
		public void WriteConfigFileXML( LgConfig config ) {
			Config.WriteXML( config );
		}

		public string [] GetIPAddresses() {
			return GetIPAddresses( Environment.MachineName );
		}

		public string [] GetIPAddresses( string host ) {
			return Management.Protocol.GetIPAddresses( host );
		}

		public ConsoleDefaultValues GetConsoleDefaultValues() {
			return ConsoleDefaultValues.LoadXML();
		}

		public void SetConsoleDefaultValues( ConsoleDefaultValues data ) {
			data.SaveXML();
		}

		public LgConfig ReadConfigXML( int lgnum, eRole role ) {
			return Config.ReadXML( lgnum, role );
		}

		public void WriteConfigXML( LgConfig config ) {
			Config.WriteXML( config );
		}

		public void DeletePrimaryConfigFileXML( int lgnum ) {
			Config.DeletePrimaryConfigFileXML( lgnum );
		}

		public eLgModes GetGroupState( int lgnum ) {
			DriverIOCTL driver = new DriverIOCTL();
			try {
				return driver.GetGroupState( lgnum );
			} finally {
				driver.Close();
			}
		}

		public string Version() {
			return System.Windows.Forms.Application.ProductVersion;
		}

		public ConsoleLgStatCollection ConsoleLgStats( int [] lgnums ) {
			DriverIOCTL driver = new DriverIOCTL();
			ConsoleLgStatCollection stats = new ConsoleLgStatCollection();
			try {
				foreach( int lgnum in lgnums ) {
					Statistics_t statistics = new Statistics_t();

					ConsoleLgStat stat = new ConsoleLgStat();
					stat.Lgnum = lgnum;
					try {
						stat.LgState = GetGroupState( lgnum );
					} catch ( Exception e ) {
						OmException.LogException( new OmException( "Error: ConsoleLgStats - GetGroupState failed - " + e ) );
						stat.LgState = eLgModes.INVALID;
					}
					try {
						driver.GetGroupStatistics ( lgnum, ref statistics );
						stat.connected = (eConnected)statistics.connected;
					} catch ( Exception e ) {
						OmException.LogException( new OmException( "Error: ConsoleLgStats - GetGroupStatistics failed - " + e ) );
						stat.connected = eConnected.Unknow;
					}
					stat.TargetStatus = -1;
					stat.TargetTime = DateTime.Now;
					stat.PercentRefreshComplete = 0;
				}
			} finally {
				driver.Close();
			}
			return stats;
		}

		/// <summary>
		/// Called by Apply CLI
		/// </summary>
		public void ApplyAllGroupsXML() {
			Config.ApplyAllGroupsXML( ref MasterThread.m_LgConfigs );
		}


		/// <summary>
		/// Called by CLIs
		/// </summary>
		public void ExecuteCLI( CLIData data ) {
			try {
				DriverIOCTL driver = new DriverIOCTL();
				try {
					foreach ( LgConfig config in MasterThread.m_LgConfigs ) {
						if (( data.All ) ||
							( config.lgnum == data.lgnum )) {
							switch ( data.Type ) {
								case eExecuteType.ACTIVATEGROUP:
									LogicalGroup.ActivateGroup( config.lgnum, data.HostID, data.Force );
									break;
								case eExecuteType.TRANSITIONTONORMAL:
									// first get the current state
									eLgModes state = GetGroupState( config.lgnum );
									if ( state == eLgModes.PASSTHRU ) {
										driver.SetGroupState( config.lgnum, eLgModes.FULLREFRESH );
									} else if ( state == eLgModes.TRACKING ) {
										//TODO: fix this when parag tells me.
										driver.SetGroupState( config.lgnum, eLgModes.REFRESH );
									}
									break;
								case eExecuteType.STATE:
									if ( data.State != eLgModes.INVALID ) {
										driver.SetGroupState( config.lgnum, data.State );
									}
									break;
								default:
									throw new OmException( "Error: Invalid eExecuteType " + data.Type );
							}
						}
					}
				} finally {
					driver.Close();
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				throw e;
			}
		}

		/// <summary>
		/// Gets one level sub directories of a given folder
		/// </summary>
		/// <param name="parent"></param>
		public StringCollection Get1LevelSubFolders( string parent ) {
			StringCollection strs = new StringCollection();

			if ( !Directory.Exists( parent ) ) {
				return strs;
			}
			if ( parent.IndexOf(':') == (parent.Length - 1) ) {
				// Is it a drive without trailing backslash, append a backslash
				parent += "\\";
			}

			// Make a reference to the input directory
			DirectoryInfo dir = new DirectoryInfo( parent );

			// Get a reference to each directory in that directory
			DirectoryInfo[] dirArr = dir.GetDirectories();

			foreach( DirectoryInfo info in dirArr ) {
				if ( ( info.Attributes & FileAttributes.Encrypted ) > 0 ) {
					continue;
				}
				if ( ( info.Attributes & FileAttributes.Hidden ) > 0 ) {
					continue;
				}
				strs.Add( info.Name );
			}
			return strs;
		}

		public void CreateDirectory( string directory ) {
			if ( Directory.Exists( directory ) ) {
				// Throw an exception
				throw new System.Exception("Folder: \"" + directory + "\" already exists!");
			}
			Directory.CreateDirectory( directory );
		}

	
	}

	public enum eConnected {
		Disconnected = 0,
		Connected = 1,
		Unknow = -1,
	}
	[Serializable]
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
	public class ConsoleLgStat {
		public int Lgnum;			// stat_buffer_t->lg_num: LG Num
		public eLgModes LgState;	// LG state mode
		public eConnected connected;		// connected to remote
		public int TargetStatus;	// target status
		public DateTime TargetTime;
		public int PercentRefreshComplete;
		public ConsoleDeviceStatCollection DevStats;
	}

	[Serializable]
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
	public class ConsoleDeviceStat {
		public int PrimaryDevId;	// device ID
		public eConnected connected;		// connected to remote
		public int TargetStatus;	// target status
		public DateTime TargetTime;
		public int PercentRefreshComplete;
	}

}
