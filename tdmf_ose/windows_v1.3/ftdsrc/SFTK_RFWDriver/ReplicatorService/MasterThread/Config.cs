using System;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml.Serialization;


namespace MasterThread {

	/// <summary>
	/// enum of logical group configuration roles
	/// </summary>
	[Serializable]
	public enum eRole {
		Primary		= 1,
		Secondary	= 2,
		Apply		= 4,
		All			= -1
	}

	/// <summary>
	/// ftd_dev_cfg_t
	/// </summary>
	[Serializable]
	public struct DevConfig {
		public int devid;					/* device number					*/
		public string remark;				/* device remark from config file	*/
		public string devname;				/* device name					*/
		public string pdevname;				/* primary raw device name			*/
		public string sdevname;				/* secondary (mirror) device name	*/
		public string pdriveid;				/*?? primary volume drive id          */
		public string ppartstartoffset;	    /* good- primary volume partition starting offset */
		public string ppartlength;		    /* good- primary volume partition length  */
		public string sdriveid;				/* secondary volume drive id        */
		public string spartstartoffset;	    /* secondary volume partition starting offset */
		public string spartlength;		    /* secondary volume partition length*/
		public string vdevname;				/* volume device name	*/
		public string symlink1;				/* symbolic link in \Device\Harddisk(%x)\Partition(%x) format*/
		public string UniqueId;				/* symbolic link in \Device\HarddiskVolume(%x) format */
		public string SignatureUniqueId;	/* symbolic link in volume((disk_signature)-offset-partition_length) format */
	
		public DevConfig Clone() {
			DevConfig dev = new DevConfig();
			dev.devid = this.devid;
			dev.remark = this.remark;
			dev.devname = this.devname;
			dev.pdevname = this.pdevname;
			dev.sdevname = this.sdevname;
			dev.pdriveid = this.pdriveid;
			dev.ppartstartoffset = this.ppartstartoffset;
			dev.ppartlength = this.ppartlength;
			dev.sdriveid = this.sdriveid;
			dev.spartstartoffset = this.spartstartoffset;
			dev.spartlength = this.spartlength;
			dev.vdevname = this.vdevname;
			dev.symlink1 = this.symlink1;
			dev.UniqueId = this.UniqueId;
			dev.SignatureUniqueId = this.SignatureUniqueId;
			return dev;
		}

		/// <summary>
		/// used by the console
		/// </summary>
		public long pDeviceSize {
			get {
				long size = long.MaxValue;
				try {
					size = Convert.ToInt64( this.ppartlength );
				} catch( Exception ) {
					size = long.MaxValue;
				}
				return size;
			}
		}
		/// <summary>
		/// Used by the console
		/// </summary>
		public string pDisplay {
			get {
				if ( pDeviceSize == long.MaxValue ) {
					return this.pdevname + " (Unknown)";
				}
				return this.pdevname + String.Format( " ({0})", this.pDisplaySize);
			}
		}
		/// <summary>
		/// Used by the console
		/// </summary>
		public string pDisplaySize {
			get {
				if ( pDeviceSize == long.MaxValue ) {
					return "(Unknown)";
				}
				double SizeMB = Convert.ToDouble( pDeviceSize / (1024*1024) );
				return String.Format( "{0:N1}MB", SizeMB);
			}
		}
		/// <summary>
		/// used by the console
		/// </summary>
		public long sDeviceSize {
			get {
				long size = long.MaxValue;
				try {
					size = Convert.ToInt64( this.spartlength );
				} catch( Exception ) {
					size = long.MaxValue;
				}
				return size;
			}
		}
		/// <summary>
		/// Used by the console
		/// </summary>
		public string sDisplaySize {
			get {
				if ( sDeviceSize == long.MaxValue ) {
					return "(Unknown)";
				}
				double SizeMB = Convert.ToDouble( sDeviceSize / (1024*1024) );
				return String.Format( "{0:N1}MB", SizeMB);
			}
		}
		/// <summary>
		/// Used by the console
		/// </summary>
		public string sDisplay {
			get {
				if ( sDeviceSize == long.MaxValue ) {
					return this.sdevname + " (Unknown)";
				}
				return this.sdevname + String.Format( " ({0})", this.sDisplaySize);
			}
		}
	}

	/// <summary>
	/// ConfigInfo
	/// </summary>
	public struct ConfigInfo {
		public int lgnum;						// logical group number
		public eRole role;						// lg role: 1=primary, 2=secondary
		public string cfgpath;					/* logical group config file path	*/
	}

	[Serializable]
	public enum eGroupType {
		Generic,
		Migration,
		DisasterRecovery,
		Cluster,
		Mirror,
		OneToMany,
		Chaining,
	}


	/// <summary>
	/// ftd_lg_cfg_t
	/// </summary>
	[Serializable]
	public class LgConfig {
		public int lgnum;						// logical group number
		public eRole role;						// lg role: 1=primary, 2=secondary
		public string UserFriendlyName;			// User Friendly group Name
		public string cfgpath;					// logical group config file path
		public string pstore;					// pstore path from config file
		public string phostname;				// primary hostname
		public string shostname;				// secondary hostname
		public bool disableJournals;
		public string jrnpath;					// lg journal area path
		public string notes;					// lg notes from config file
		public int port;						// lg peer communications port num 
		public bool compression;
		public bool syncMode;
		public bool cluster;
		public bool chaining;					// lg mirror chain state (A.B.C)
//		public StreamReader streamReader;
		public int lineNumber;
		public StringCollection PNetworkAddresses = new StringCollection();
		public StringCollection SNetworkAddresses = new StringCollection();
		public eGroupType groupConsoleType;			// type of group created

		public DevConfigCollection devlist;		// list of device config state for lg

		// Migration
		public bool MigrationRunPrimaryPreScript;
		public bool MigrationRunSecondaryPostScript;
		public bool MigrationActionsAutoRollOver;

		// DR
		public bool DRRunPrimaryPreScript;
		public bool DRRunPrimaryPostScript;
		public bool DRRunSecondaryPreScript;
		public bool DRRunSecondaryPostScript;
		public int DRFailoverTime;
		public bool DRAutoFailOverCheckBox;
		public bool DRAutoRollBackCheckBox;

		// Notification
		public bool NotificationFlashCompletionMessage;
		public bool NotificationEmail;
		public string NotificationEmailAddress;

		// Cluster
		public StringCollection ClusterServers = new StringCollection();





		/// <summary>
		/// Constructor
		/// </summary>
		public LgConfig () {
		}
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="num"></param>
		public LgConfig ( int num ) {
			lgnum = num;
			cfgpath = FileNamePath();
			// default to system 32
			pstore = Path.Combine( Environment.SystemDirectory, "pstore" + lgnum.ToString("D3") + ".dat" );
			jrnpath = Path.Combine( RegistryAccess.InstallPath(), "Journal" );
			port = RegistryAccess.MasterPort();
			chaining = false;
			devlist = new DevConfigCollection();
		}

		public void ChangePStorePath( string path ) {
			pstore = Path.Combine( path, "pstore" + lgnum.ToString("D3") + ".dat" );
		}
		public string PStorePath() {
			return Path.GetDirectoryName( pstore );
		}

		public string Name() {
			string prefix = "";
			switch ( role ) {
				case eRole.Primary:
					prefix = Config.PRIMARY_CFG_PREFIX;
					break;
				case eRole.Secondary:
					prefix = Config.SECONDARY_CFG_PREFIX;
					break;
				case eRole.All:
					prefix = "";
					break;
			}
			return prefix + lgnum.ToString("D3");
		}
		public string FileName() {
			return Name() + "." + Config.PATH_CFG_SUFFIX;
		}
		public string FileNameXML() {
			return Name() + "." + "XML";
		}
		public string FileNamePath() {
			return Path.Combine( RegistryAccess.InstallPath(), FileName() );
		}
		public string FileNamePathXML() {
			return Path.Combine( RegistryAccess.InstallPath(), FileNameXML() );
		}
		public LgConfig Clone() {
			LgConfig config = new LgConfig();
			config.lgnum = this.lgnum;
			config.role = this.role;
			config.cfgpath = this.cfgpath;
			config.pstore = this.pstore;
			config.phostname = this.phostname;
			config.shostname = this.shostname;
			config.jrnpath = this.jrnpath;
			config.notes = this.notes;
			config.port = this.port;
			config.chaining = this.chaining;
			config.lineNumber = this.lineNumber;
			config.devlist = this.devlist.Clone();
			return config;
		}
	}

	/// <summary>
	/// Summary description for Config.
	/// </summary>
	public class Config {

		// defaults
		public static int DEFAULT_STATE_BUFFER_SIZE = 4096;
		public static int DEFAULT_LRDB_BITMAP_SIZE = 8192;
		public static int DEFAULT_HRDB_BITMAP_SIZE = 131072;


		// Prefix on our primary-system config files
		public static string PRIMARY_CFG_PREFIX = "p";

		// Prefix on our secondary-system config files
		public static string SECONDARY_CFG_PREFIX = "s";

		// Suffix on our config files
		public static string PATH_CFG_SUFFIX = "cfg";
//		public static string PATH_STARTED_CFG_SUFFIX = "cur";
	
		//public const uint CFGMAGIC = 0xBADF00D5;

		/// <summary>
		/// Constructor
		/// </summary>
		public Config() {
		}



		/// <summary>
		/// returns the list of secondary configuration files.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <returns></returns>
		public static ConfigInfoCollection GetSecondaryConfigInfos() {
			return GetConfigInfos( eRole.Secondary );
		}
//		/// <summary>
//		/// returns the list of secondary started configuration files.
//		/// </summary>
//		/// <param name="cfgpath"></param>
//		/// <returns></returns>
//		public static ConfigInfoCollection GetSecondaryStartedConfigs() {
//			return GetConfigs( eRole.Secondary, true );
//		}
		/// <summary>
		/// returns the list of primary configuration files.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <returns></returns>
		public static ConfigInfoCollection GetPrimaryConfigInfos() {
			return GetConfigInfos( eRole.Primary );
		}

//		/// <summary>
//		/// returns the list of primary started configuration files.
//		/// </summary>
//		/// <param name="cfgpath"></param>
//		/// <returns></returns>
//		public static ConfigInfoCollection GetPrimaryStartedConfigs() {
//			return GetConfigs( eRole.Primary, true );
//		}

		/// <summary>
		/// returns the list of all configuration files.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <returns></returns>
		public static ConfigInfoCollection GetAllConfigInfos() {
			return GetConfigInfos( eRole.All );
		}

		/// <summary>
		/// returns the list of primary or secondary or both configuration files.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <param name="role"></param>
		/// <param name="startflag"></param>
		/// <returns></returns>
		public static ConfigInfoCollection GetConfigInfos( eRole role ) {
		//public static ConfigInfoCollection GetConfigs( eRole role, bool startflag ) {
			string cfgName;
			ConfigInfoCollection ConfigInfos = new ConfigInfoCollection();

			// build the wild card file name
			string prefix = "";
			switch ( role ) {
				case eRole.Primary:
					prefix = PRIMARY_CFG_PREFIX;
					break;
				case eRole.Secondary:
					prefix = SECONDARY_CFG_PREFIX;
					break;
				case eRole.All:
					prefix = "";
					break;
			}
			cfgName = prefix + "*." + PATH_CFG_SUFFIX;

			// get the list of files
			string cfgpath = RegistryAccess.InstallPath();
			string [] fileNames = Directory.GetFiles( cfgpath, cfgName );

			try {
				/* Create the cfglist for each LG present */
				foreach ( string fileName in fileNames ) {
					ConfigInfo configInfo = new ConfigInfo();
					configInfo.cfgpath = Path.GetFileName( fileName );
					configInfo.lgnum = Convert.ToInt32(configInfo.cfgpath.Substring( PRIMARY_CFG_PREFIX.Length, 3 ));
					configInfo.role = role;

//					if ( startflag ) { // *.cur file mapping
//						try {
//							// we do nothing with the data, just determine if it is valid
//							// if not then clean up
//							Tunables tunables;
//							DriverIOCTL driver = new DriverIOCTL();
//							try {
//								driver.GetLgStateBuffer( configInfo.lgnum, out tunables );
//							} finally {
//								driver.Close();
//							}
//						} catch ( Exception e ) {
//							OmException.LogException( new OmException( String.Format("Error in GetConfigs calling GetLgState: {0}", e.Message ), e ) );
//							// .cur file is invalid
//							// so delete the .cur file
//		//					File.Delete( lgConfig.cfgpath );
//							/* Modify the file name suffix .cur to .cfg */
//		//					lgConfig.cfgpath = lgConfig.cfgpath.Substring( 0, lgConfig.cfgpath.Length - 3 ) + PATH_CFG_SUFFIX;
//						}       
//					}
					ConfigInfos.Add( configInfo );
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in GetConfigs: {0}", e.Message ), e ) );
			}
			return ConfigInfos;
		}

		/// <summary>
		/// returns the list of primary or secondary or both configuration files.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <param name="role"></param>
		/// <param name="startflag"></param>
		/// <returns></returns>
		public static ConfigInfoCollection GetConfigInfosXML( eRole role ) {
			//public static ConfigInfoCollection GetConfigs( eRole role, bool startflag ) {
			string cfgName;
			ConfigInfoCollection ConfigInfos = new ConfigInfoCollection();

			// build the wild card file name
			string prefix = "";
			switch ( role ) {
				case eRole.Primary:
					prefix = PRIMARY_CFG_PREFIX;
					break;
				case eRole.Secondary:
					prefix = SECONDARY_CFG_PREFIX;
					break;
				case eRole.All:
					prefix = "";
					break;
			}
			cfgName = prefix + "*.XML";

			// get the list of files
			string cfgpath = RegistryAccess.InstallPath();
			string [] fileNames = Directory.GetFiles( cfgpath, cfgName );

			try {
				/* Create the cfglist for each LG present */
				foreach ( string fileName in fileNames ) {
					ConfigInfo configInfo = new ConfigInfo();
					configInfo.cfgpath = Path.GetFileName( fileName );
					configInfo.lgnum = Convert.ToInt32(configInfo.cfgpath.Substring( PRIMARY_CFG_PREFIX.Length, 3 ));
					configInfo.role = role;
					ConfigInfos.Add( configInfo );
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( String.Format("Error in GetConfigInfosXML: {0}", e.Message ), e ) );
			}
			return ConfigInfos;
		}

		/// <summary>
		/// returns the list of primary logical group configuration.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <returns></returns>
		public static LgConfigCollection GetPrimaryLgConfigs() {
			return GetLgConfigs( eRole.Primary );
		}
		/// <summary>
		/// returns the list of secondary logical group configuration.
		/// </summary>
		/// <param name="cfgpath"></param>
		/// <returns></returns>
		public static LgConfigCollection GetSecondaryLgConfigs() {
			return GetLgConfigs( eRole.Secondary );
		}
		/// <summary>
		/// returns a collection of all the logical group configurations.
		/// </summary>
		/// <param name="role"></param>
		public static LgConfigCollection GetLgConfigs( eRole role ) {

			LgConfigCollection LgConfigs = new LgConfigCollection();
			ConfigInfoCollection ConfigInfos = GetConfigInfos( role );

			foreach ( ConfigInfo info in ConfigInfos ) {
				try {
					LgConfig config = ReadConfig( info.lgnum, info.role );
					LgConfigs.Add ( config );
				} catch ( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in GetLgConfigs: {0}", e.Message ), e ) );
				}
			}
			return LgConfigs;
		}

		/// <summary>
		/// returns the list of primary logical group configuration.
		/// </summary>
		/// <returns></returns>
		public static LgConfigCollection GetPrimaryLgConfigsXML() {
			return GetLgConfigsXML( eRole.Primary );
		}
		/// <summary>
		/// returns a collection of all the logical group configurations.
		/// </summary>
		/// <param name="role"></param>
		public static LgConfigCollection GetLgConfigsXML( eRole role ) {
			LgConfigCollection LgConfigs = new LgConfigCollection();
			ConfigInfoCollection ConfigInfos = GetConfigInfosXML( role );
			foreach ( ConfigInfo info in ConfigInfos ) {
				try {
					LgConfig config = Config.ReadXML( info.lgnum, role );
					LgConfigs.Add ( config );
				} catch ( Exception e ) {
					OmException.LogException( new OmException( String.Format("Error in GetLgConfigsXML: {0}", e.Message ), e ) );
				}
			}
			return LgConfigs;
		}


		/// <summary>
		/// parse a configuration file
		/// </summary>
		/// <param name="cfgp"></param>
		/// <param name="bIsWindowsFile"></param>
		/// <param name="lgnum"></param>
		/// <param name="role"></param>
		/// <param name="startflag"></param>
		public static LgConfig ReadConfig( int lgnum, eRole role ) {

			/* initialize the config object */
			LgConfig config = new LgConfig();
			ConfigInit( ref config, lgnum, role );

			StreamReader streamReader = File.OpenText( config.cfgpath );

			/* initialize the parsing state */
			string line;
			//bool more = true;
			config.lineNumber = 0;

			do {
				/* if we need a line and don't get it, then we're done */
				line = ReadLine( ref config, streamReader );

				if ( line == null ) {
					break;
				}

				do {
					string [] args = SplitOnSpace ( line );

					/* major sections of the file */
        			switch ( args[0].ToUpper() ) {
						case "SYSTEM-TAG:":
							// check if primary or secondary
							eRole role_;
							switch ( args[2].ToUpper() ) {
								case "PRIMARY":
									role_ = eRole.Primary;
									break;
								case "SECONDARY":
									role_ = eRole.Secondary;
									break;
								default:
									throw new OmException( "Error: Bad line in config file - " + config.cfgpath + ", Line number - " + config.lineNumber );
							}
							line = parse_system ( ref config, role_, streamReader );
							break;
						case "PROFILE:":
							line = parse_profile ( ref config, streamReader );
							break;
						case "NOTES:":
							line = "";
							continue;
						default:
							throw new OmException( "Error: Bad line in config file - " + config.cfgpath + ", Line number - " + config.lineNumber );
					}			
				} while ( (line != null) && (line.Length > 5) );
			} while ( true );

			streamReader.Close();
			streamReader = null;

			if ( config.role == eRole.Primary ) {
				verify_primary_entries( ref config );
			} else if ( config.role == eRole.Secondary )     {
				verify_secondary_entries( ref config );
			}
			return config;
		}



		public static string [] SplitOnSpace( string line ) {
			char [] Delimeters = new char[2] {' ','\t'};

			string [] temp = line.Trim().Split( Delimeters );
			int last = 0;
			for ( int index = 0 ; index < temp.Length ; index++ ) {
				if ( temp[ index] != "" ) {
					if ( last > 0 ) {
						temp[ last ] = temp[ index ];
					}
					last++;
				}
			}
			string [] args = new string [ last ];
			Array.Copy( temp, 0, args, 0, last );
			return args;
		}

		/// <summary>
		/// parse_system
		/// </summary>
		/// <param name="cfgp"></param>
		/// <param name="role"></param>
		/// <returns></returns>
		public static string parse_system( ref LgConfig cfgp, eRole role, StreamReader streamReader ) {
			bool more = true;
			string line;

			do {
				line = ReadLine( ref cfgp, streamReader );

				if ( line == null ) {
					more = false;
					continue;
				}

				string [] args = SplitOnSpace ( line );

				switch ( args[0].ToUpper() ) {
					case "HOST:":
						if (role == eRole.Primary) {
							cfgp.phostname = args[1];
						} else {
							cfgp.shostname = args[1];
						}
						break;
					case "PSTORE:":
						cfgp.pstore = line.Substring( 7 ).Trim();
						break;
					case "JOURNAL:":
						cfgp.jrnpath = line.Substring( 8 ).Trim();
						break;
					case "SECONDARY-PORT:":
						cfgp.port = Convert.ToInt32( args[1] );
						break;
					case "CHAINING:":
						cfgp.chaining = ( String.Compare( args[1], "on", true ) == 0 );
						break;
					default:
						more = false;
						break;
				}
			} while ( more );
			// we must have either the name or the IP address for the host
			if (( role == eRole.Primary ) &&
				( cfgp.phostname.Length == 0 )) {
				throw new OmException( "Error: Bad section in config file for Primary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
			}
			// we must have either the name or the IP address for the host
			if (( role == eRole.Secondary ) &&
				( cfgp.shostname.Length == 0 )) {
				throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
			}
			/* we must have secondary journal path */
			if (( role == eRole.Secondary ) &&
				( cfgp.jrnpath.Length == 0 )) {
				throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
			}
			return line;
		}

		/// <summary>
		/// Read zero or more lines of device definitions.
		/// Read lines until we don't match the key or EOF.
		/// </summary>
		/// <param name="cfgp"></param>
		public static string parse_profile( ref LgConfig cfgp, StreamReader streamReader ) {
			DevConfig devcfgp;
			bool moreprofiles;

			string line;

			do {
				moreprofiles = false;
				bool more = true;
				devcfgp = new DevConfig();

				do {
					line = ReadLine( ref cfgp, streamReader );
					if ( line == null ) {
						more = false;
						continue;
					}
					string [] args = SplitOnSpace ( line );

					switch ( args[0].ToUpper() ) {
							// create and link in a group profile to both systems
						case "REMARK:":
							if ( args.Length > 1 ) {
								devcfgp.remark = args[1];
							}
							break;
						case "PRIMARY:":
							// do nothing
							break;
						case "SECONDARY:":
							// do nothing
							break;
						case "TDMF-DEVICE:":
						case "DTC-DEVICE:":
						case "DEVICE:":
							if ( args.Length < 2 ) {
								throw new OmException( "Error: Bad section in config file for DTC-DEVICE - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
							}
							// handle a mount point
							string name = line.Substring( args[0].Length ).Trim();
							int end = name.LastIndexOf( ':' );
							devcfgp.devname = name.Substring(0, end ).Trim();
							if ( devcfgp.devname.Length < 2 ) {
								devcfgp.devname += ":";
							}
							devcfgp.devid = Convert.ToInt32( name.Substring( end + 1 ).Trim() );
							break;
						case "DATA-DISK:":
							if ( args.Length < 5 ) {
								throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
							}
							// handle path names
							int index = 1;
							devcfgp.pdevname = args[1];
							if ( args[ index ].Length > 2 ) {
								// its a mount point
								// fine the last \
								for ( int i = 1 ; i < args.Length ; i++ ) {
									if ( args[i].EndsWith("\\") ) {
										index = i;
									}
								}
								int start = line.IndexOf( ':', 10 ) - 1;
								end = line.LastIndexOf( '\\' );
								devcfgp.pdevname = line.Substring( start, end-start + 1 ).Trim();
							}
							index++;
							devcfgp.pdriveid = args[index++];
							devcfgp.ppartstartoffset = args[index++];
							devcfgp.ppartlength = args[index++];
							// SAUMYA_FIX_CONFIG_FILE_PARSING
							if ( args.Length >= index + 3 ) {
								devcfgp.symlink1 = args[index++];
								devcfgp.UniqueId = args[index++];
								devcfgp.SignatureUniqueId = args[index++];
							}
							break;
						case "MIRROR-DISK:":
							if ( args.Length < 5 ) {
								throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
							}
							// handle path names
							index = 1;
							devcfgp.sdevname = args[1];
							if ( args[ index ].Length > 2 ) {
								// its a mount point
								// fine the last \
								for ( int i = 1 ; i < args.Length ; i++ ) {
									if ( args[i].EndsWith("\\") ) {
										index = i;
									}
								}
								int start = line.IndexOf( ':', 12 ) - 1;
								end = line.LastIndexOf( '\\' );
								devcfgp.sdevname = line.Substring( start, end-start + 1 ).Trim();
							}
							index++;
							devcfgp.sdriveid = args[index++];
							devcfgp.spartstartoffset = args[index++];
							devcfgp.spartlength = args[index++];
							break;
						case "PROFILE:":
							moreprofiles = true;
							more = false;
							break;
						default:
							throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
					}
				} while ( more );

				/* We must have: sddevname, devname, mirname, and secondary journal */
				if (( devcfgp.devname == null ) ||
					( devcfgp.devname.Length == 0 )) {
					throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
				}
				if (( devcfgp.sdevname == null ) ||
					( devcfgp.sdevname.Length == 0 )) {
					throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
				}
				if (( devcfgp.pdevname == null ) ||
					( devcfgp.pdevname.Length == 0 )) {
					throw new OmException( "Error: Bad section in config file for Secondary - " + cfgp.cfgpath + ", Line number - " + cfgp.lineNumber );
				}
				// add device to devlist
				cfgp.devlist.Add( devcfgp );

			} while ( moreprofiles );
			return line;

		}


		/// <summary>
		/// 
		/// </summary>
		/// <param name="cfgp"></param>
		/// <returns></returns>
		public static string ReadLine( ref LgConfig cfgp, StreamReader streamReader ) {
			string line;
			bool more = true;

			do {
				line = streamReader.ReadLine();
				if ( line == null ) {
					more = false;
					continue;
				}
				cfgp.lineNumber++;
				line = line.Trim();
				if ( line.Length < 5 ) {
					continue;
				}
				// comment line
				if ( line[0] == '#' ) {
					continue;
				}
				more = false;
			} while( more );
			return line;
		}


		/// <summary>
		/// verify_primary_entries -- verify the primary system config info
		/// </summary>
		/// <param name="cfgp"></param>
		/// <returns></returns>
		public static void verify_primary_entries ( ref LgConfig cfgp ) {
			string szDiskInfo;

			if ( cfgp.devlist.Count == 0 ) {
				throw new OmException( "Error: verify_primary_entries no devices in config" );
			}
			foreach ( DevConfig devpp in cfgp.devlist ) {
				// TODO: add this if we need it
				//devpp.vdevname = getDeviceNameSymbolicLink( devpp.pdevname );
				//
				// szDir format : \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
				//
				if ( devpp.pdevname.Length == 2 ) {
					/* Drive Letter  */
					// get the drive signature and volume info
					szDiskInfo = GetDiskSignatureAndInfo( devpp.pdevname );
				}
				else {
					/* Mount Point  */
					szDiskInfo = GetMntPtSigAndInfo( devpp.sdevname );
				}
				string id = String.Format( "{0} {1} {2}",devpp.pdriveid , devpp.ppartstartoffset, devpp.ppartlength );
				if ( String.Compare ( szDiskInfo, id ) != 0 ) {
					OmException.LogException( new OmException( "Error: verify_primary_entries verifyDrive failed Config = " + id + ", Found = " + szDiskInfo ) );
				}
			}
		}

		/// <summary>
		/// verify_secondary_entries -- verify the secondary system config info
		/// </summary>
		/// <param name="cfgp"></param>
		/// <returns></returns>
		public static void verify_secondary_entries( ref LgConfig cfgp ) {
			string szDiskInfo;

			if ( cfgp.devlist.Count == 0 ) {
				throw new OmException( "Error: verify_secondary_entries no devices in config" );
			}

			foreach ( DevConfig devpp in cfgp.devlist ) {
				// TODO: add this if we need it
				//devpp.vdevname = getDeviceNameSymbolicLink( devpp.sdevname );
				if ( devpp.sdevname.Length == 2 ) {
					/* Drive Letter  */
					// get the drive signature and volume info
					szDiskInfo = GetDiskSignatureAndInfo( devpp.sdevname );
				}
				else {
					/* Mount Point  */
					szDiskInfo = GetMntPtSigAndInfo( devpp.sdevname );
				}
				string id = String.Format( "{0} {1} {2}",devpp.sdriveid , devpp.spartstartoffset, devpp.spartlength );
				if ( String.Compare ( szDiskInfo, id ) != 0 ) {
					OmException.LogException( new OmException( "Error: verify_secondary_entries verifyDrive failed" ) );
					//reporterr(ERRFAC, M_DEVVERIFY, ERRCRIT, (*devpp).sdevname);\
					//return FTD_CFG_MIR_STAT; // rddeb 020917
				}
			}
			// verify journal area
			if ( !File.Exists( cfgp.jrnpath ) ) {
				throw new OmException( "Error: verify_secondary_entries journal file does not exist" );
			}
		}

//		public static string getDeviceNameSymbolicLink( string lpDeviceName ) {
//			string szVolumePath;
//			string lpTargetPath;
//
//			if ( lpDeviceName.Length == 2 ) {
//				QueryDosDevice(lpDeviceName, lpTargetPath, ucchMax);
//			}
//			else {
//				// Must be a Mount Point  format:  H:\MyMountPoint1\ //
//				if (!getVolumeNameForVolMntPt( lpDeviceName,     // input volume mount point or directory
//					szVolumePath,     // output volume name buffer
//					MAX_PATH )) {       // size of volume name buffer
//					return null;
//				}
//				//  Volume GUID format:  \??\Volume{....} //
//				szVolumePath[1] = '?';
//				szVolumePath[48] = '\0';
//				if( QueryVolMntPtInfoFromDevName( szVolumePath, MAX_PATH, lpTargetPath, MAX_PATH, DEVICE_NAME) != VALID_MNT_PT_INFO ) {
//					return null;
//				}
//
//			}
//			return lpTargetPath;
//		}

//		public static int verifyDrive( string szCfgFile, string szSystemValue ) {
//			int iRc = 0;
//			string strReadValue;
//			StreamReader file;
//
//			file = new StreamReader( szCfgFile );
//			try {
//				strReadValue = file.ReadToEnd();
//				if ( String.Compare( strReadValue, szSystemValue ) == 0 ) {
//					return -1;
//				}
//			} finally {
//				file.Close();
//			}
//			return iRc;
//		}



		/// <summary>
		/// initialize cfg object
		/// </summary>
		/// <param name="cfgp"></param>
		/// <param name="lgnum"></param>
		/// <param name="role"></param>
		/// <param name="startflag"></param>
		/// <returns></returns>
		public static int ConfigInit( ref LgConfig cfgp, int lgnum, eRole role ) {
			
//			string prefix;
			cfgp.lgnum = lgnum;
			cfgp.role = role;
			cfgp.devlist = new DevConfigCollection();

//			if ( cfgp.role == eRole.Secondary ) {
//				prefix = SECONDARY_CFG_PREFIX;
//			} else {
//				prefix = PRIMARY_CFG_PREFIX;
//			}
			cfgp.cfgpath = RegistryAccess.InstallPath() + "\\" + cfgp.FileName();
			return 0;
		}

		public static string GetMntPtSigAndInfo( string name ) {
			string ID;
			string Offset;
			string Length;
			ulong FreeSpace;
			GetMntPtSigAndInfo ( name, out ID, out Offset, out Length, out FreeSpace  );
			return String.Format( "{0} {1} {2}", ID, Offset, Length );
		}
		public static void GetMntPtSigAndInfo ( string name, out string DriveID, out string Offset, out string Length, out ulong FreeSpace ) {
			IntPtr hVolume = IntPtr.Zero;
			DriveID = "";

			try {
				// szGUID format : \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
				// get the partition starting sector and length
				DiskPartitionAPI.PARTITION_INFORMATION partInfo = new DiskPartitionAPI.PARTITION_INFORMATION();
				partInfo.PartitionLength = 0;
				partInfo.StartingOffset = 0;

				if ( (hVolume = Volume.OpenAVolumeMountPoint( name )) != VolumeMountPoint.INVALID_HANDLE_VALUE ) {
					
					try {
						// get drive serial number or signature
						DriveID = GetDriveid( hVolume ).ToString();
					} catch ( Exception ) {
						DriveID = GetDiskIdFromGuid( name ).ToString();
					}
					try {
						DiskPartitionAPI.GetVolumeInfo ( hVolume, ref partInfo );
					} catch ( OmException e ) {
						throw e;
					}
				}
				// get the actual size returned by windows explorer instead
				// of "real" disk extents...
				DriveInfo driveInfo = new DriveInfo();

				try {
					driveInfo.DiskFreeSpaceEx ( name );
					partInfo.PartitionLength = driveInfo.GetTotalBytes;
					FreeSpace = driveInfo.TotalNumberOfFreeBytes;
				} catch ( Exception e ) {
					throw e;
				}
				Offset = partInfo.StartingOffset.ToString();
				Length = partInfo.PartitionLength.ToString();
			} finally {
				if ( hVolume != IntPtr.Zero ) {
					Createfile.Close( hVolume );
				}
			}
		}

		/// <summary>
		/// return the drive serial number
		/// </summary>
		/// <param name="hVolume"></param>
		/// <returns></returns>
		public static long GetDriveid(IntPtr hVolume ) {
			// Allocate memory for get partition info call
			DiskPartitionAPI.DRIVE_LAYOUT_INFORMATION PartLayout = new DiskPartitionAPI.DRIVE_LAYOUT_INFORMATION();

			// Get Partition Info
			DiskPartitionAPI.GetPartitionInfo ( hVolume, ref PartLayout );

			// Set Disk Signature from Partition Info
			return PartLayout.Signature;
	
		}

		/// <summary>
		/// Return the first part of Volume GUID as Disk ID , WIN2K ONLY
		/// This function accepts either a GUID or the mount point directory
		/// as the IN parameter...
		/// If szDrive is a mountpoint directory, we get the GUID before 
		/// returning the partial GUID...
		/// </summary>
		/// <param name="Drive"></param>
		/// <returns></returns>
		public static long GetDiskIdFromGuid( string Drive ) {
			string VolumeName;
			string name;

			if ( !Volume.IS_VALID_VOLUME_NAME( Drive ) ) {
				VolumeName = VolumeMountPoint.GetVolumeNameForVolMntPt( Drive );
				// ensure it starts with \\
				VolumeName = "\\\\" + VolumeName.Substring( 2 );;
				name = VolumeName.ToString();
			}
			else {
				name = Drive;
			}
			
			name = name.Substring( name.IndexOf('{') + 1, 8 ).Trim();

			return Convert.ToInt32( name, 16 );
		}

		public static string GetDiskSignatureAndInfo ( string name ) {
			string ID;
			string Offset;
			string Length;
			ulong FreeSpace;
			GetDiskSignatureAndInfo ( name, out ID, out Offset, out Length, out FreeSpace );
			return String.Format( "{0} {1} {2}", ID, Offset, Length );
		}

		public static void GetDiskSignatureAndInfo ( string name, out string DriveID, out string Offset, out string Length, out ulong FreeSpace ) {
			IntPtr hVolume = IntPtr.Zero;
			//long   DriveID = 0;

			try {
				// open the drive for read access.
				hVolume = Createfile.OpenDrive( "\\\\.\\" + name );
				//				// Get the right logical group
				//				if((hVolume = ftd_dev_lock(szDirParse, iGroupID)) == INVALID_HANDLE_VALUE ) {
				//					return "";
				//				}
				//				else {
				//					bLocked = TRUE;
				//				}

				try {
					// get drive serial number or signature
					DriveID = Config.GetDriveid( hVolume ).ToString();
				} catch ( Exception ) {
					DriveID = Config.GetDiskIdFromGuid( name ).ToString();
				}

				// get the partition starting sector and length
				DiskPartitionAPI.PARTITION_INFORMATION partInfo = new DiskPartitionAPI.PARTITION_INFORMATION();
				partInfo.PartitionLength = 0;
				partInfo.StartingOffset = 0;

				try {
					DiskPartitionAPI.GetVolumeInfo ( hVolume, ref partInfo );
				} catch ( OmException e ) {
					throw e;
				}
				// get the actual size returned by windows explorer instead
				// of "real" disk extents...
				DriveInfo driveInfo = new DriveInfo();
				try {
					driveInfo.DiskFreeSpaceEx ( name );
					partInfo.PartitionLength = driveInfo.GetTotalBytes;
					FreeSpace = driveInfo.TotalNumberOfFreeBytes;
				} catch ( Exception e ) {
					throw e;
				}
                Offset = partInfo.StartingOffset.ToString();
				Length = partInfo.PartitionLength.ToString();

			} finally {
				if ( hVolume != IntPtr.Zero ) {
					Createfile.Close( hVolume );
				}
			}
		}

		/// <summary>
		/// Configure all primary groups
		/// </summary>
		public static void ApplyAllGroupsXML( ref LgConfigCollection masterLgConfigs ) {
			// clear the existing master config collection
			masterLgConfigs.Clear();

			// get all primary configs 
			LgConfigCollection LgConfigs = GetPrimaryLgConfigsXML();

			if ( LgConfigs.Count == 0 ) {
				return;
			}
			DriverIOCTL driver = new DriverIOCTL();
			try {
				driver.ConfigStart();
				try {
					foreach ( LgConfig config in LgConfigs ) {
						try {
							AddLogicalGroupToDriver( driver, config );
							// Add this config to the master list.
							masterLgConfigs.Add( config );
							//ApplyGroup( driver, config );
						} catch ( OmException e ) {
							OmException.LogException( new OmException( String.Format("Error in INITIAL STARTUP calling start_group for lgnum- " + config.lgnum ), e ) );
						}
					}
				}
				finally {
					driver.ConfigEnd();
				}
			}
			finally {
				driver.Close();
			}
		}

		/// <summary>
		/// Configure all primary groups
		/// </summary>
		public static void ApplyAllGroups( ref LgConfigCollection masterLgConfigs ) {
			// clear the existing master config collection
			masterLgConfigs.Clear();

			// get all primary configs 
			LgConfigCollection LgConfigs = GetPrimaryLgConfigs();

			if ( LgConfigs.Count == 0 ) {
				return;
			}
			DriverIOCTL driver = new DriverIOCTL();
			try {
				driver.ConfigStart();
				try {
					foreach ( LgConfig config in LgConfigs ) {
						try {
							AddLogicalGroupToDriver( driver, config );
							// Add this config to the master list.
							masterLgConfigs.Add( config );
							//ApplyGroup( driver, config );
						} catch ( OmException e ) {
							OmException.LogException( new OmException( String.Format("Error in INITIAL STARTUP calling start_group for lgnum- " + config.lgnum ), e ) );
						}
					}
				}
				finally {
					driver.ConfigEnd();
				}
			}
			finally {
				driver.Close();
			}
		}

//		/// <summary>
//		/// Configure the group
//		/// </summary>
//		/// <param name="lgnum"></param>
//		/// <param name="force"></param>
//		/// <param name="autostart"></param>
//		/// <returns></returns>
//		public static void ApplyGroup( DriverIOCTL driver, LgConfig config ) {
//			
//			LogicalGroup lgp;
//			try {
//				lgp = LogicalGroup.Init( config.lgnum, eRole.Primary, false );
//			} catch ( Exception e ) {
//				Console.WriteLine( String.Format( "Error - ConfigureGroup(), Calling Init()" ) );
//				throw e;
//			}
//			try {
//				lgp.LgAdd();
//			} catch ( Exception e ) {
//				Console.WriteLine( String.Format( "Error - start_group(), Calling LgAdd()" ) ); 
//				throw e;
//			}
//		}

		/// <summary>
		/// ftd_lg_add -- add the logical group state to the driver
		/// </summary>
		/// <param name="lgp"></param>
		/// <param name="autostart"></param>
		/// <returns></returns>
		public static void AddLogicalGroupToDriver( DriverIOCTL driver, LgConfig config ) {

			// add group
			DriverIOCTL.ftd_lg_info_t info;
			info.vdevname = "\\??\\" + config.pstore;
			info.lgdev = config.lgnum;
			info.statsize = 4 * 1024; // PS_GROUP_ATTR_SIZE;
			driver.ftd_ioctl_new_lg( info );

			// tell the driver about the configured devices in this logical group
			AddDevicesToDriver( driver, config );

			// get tunables from driver 
			Tunables tunables = new Tunables();
			GetDriverTunables( driver, ref tunables, config.lgnum  );
   
			// init the tunables in the driver 
			SetDriverTunables( driver, ref tunables, config.lgnum );
		}

		/// <summary>
		/// ftd_lg_set_driver_state -- set group state in driver
		/// </summary>
		/// <param name="lgp"></param>
		/// <returns></returns>
		private static void SetDriverTunables( DriverIOCTL driver, ref Tunables tunables, int lgnum ) {
			// set sync mode depth in driver 
			if ( tunables.syncmode != 0 ) {
				driver.SetSyncDepth( lgnum, tunables.syncmodedepth );
			} 
			else {
				driver.SetSyncDepth( lgnum, -1 );
			}
			// set iodelay in driver 
			driver.SetIodelay( lgnum, tunables.iodelay );
			// set syncmodetimeout in driver 
			driver.SetSyncTimeout( lgnum, tunables.syncmodetimeout );

			driver.SetLgStateBuffer( lgnum, tunables );
		}

		/// <summary>
		/// ftd_lg_get_driver_state -- get group state from driver
		/// </summary>
		/// <param name="driver"></param>
		/// <param name="tunables"></param>
		/// <param name="lgnum"></param>
		public static void GetDriverTunables( DriverIOCTL driver, ref Tunables tunables, int lgnum ) {
			driver.GetLgStateBuffer( lgnum, out tunables );
		}
		/// <summary>
		/// ftd_lg_set_state_value -- sets a tunable paramter in the driver and pstore given a <KEY>: and value.
		/// </summary>
		/// <param name="lgp"></param>
		/// <param name="inkey"></param>
		/// <param name="invalue"></param>
		/// <returns></returns>
		public static void SetToonableValue( int lgnum, string inkey, int intval ) {
			DriverIOCTL driver = new DriverIOCTL();
			try {
				/* get current driver state */
				Tunables tunables = new Tunables();
				GetDriverTunables( driver, ref tunables, lgnum );
				/* set the new lg state value in the tunables structure */
				SetToonableValue( ref tunables, inkey, intval );
				/* set the lg state in the driver */
				SetDriverTunables( driver, ref tunables, lgnum);
			} finally {
				driver.Close();
			}
		}

		/// <summary>
		/// ftd_lg_parse_state
		/// set the new lg state value in the tunables structure
		/// </summary>
		/// <param name="key"></param>
		/// <param name="value"></param>
		/// <param name="tunables"></param>
		/// <returns></returns>
		private static void SetToonableValue( ref Tunables tunables, string key, int intval ) {

			switch ( key.ToUpper() ) {
				case "SYNCMODEDEPTH":
					tunables.syncmodedepth = intval;
					break;
				case "SYNCMODETIMEOUT":
					tunables.syncmodetimeout = intval;
					break;
				case "SYNCMODE":
					tunables.syncmode = intval;
					break;
				case "COMPRESSION":
					tunables.compression = intval;
					break;
				case "CHUNKDELAY":
					tunables.chunkdelay = intval;
					break;
				case "REFRESHTIMEOUT": 
					tunables.refrintrvl = intval;
					break;
				default:
					throw new OmException ( "SetToonableValue: Invalid key - " + key );
			}
		}

		/// <summary>
		/// ftd_lg_add_devs -- add devices to driver
		/// </summary>
		/// <param name="lgp"></param>
		/// <returns></returns>
		private static void AddDevicesToDriver( DriverIOCTL driver, LgConfig config ) {
			//ftd_dev_t       devp;
			DriverIOCTL.ftd_dev_info_t  devinfo = new DriverIOCTL.ftd_dev_info_t();

			try {
				// create the devices
				foreach (DevConfig devcfg in config.devlist ) {       
					//devp = ftd_lg_devid_to_dev( lgp, devcfg.devid );

					//comment: find ftd_dev_add( to see how to fill in devinfop
					// add it
					devinfo.lgnum = config.lgnum;
					devinfo.localcdev = devcfg.devid;
					devinfo.cdev = devcfg.devid;
					devinfo.bdev = devcfg.devid;
					devinfo.disksize = (uint)(devcfg.pDeviceSize / DiskPartitionAPI.DISK_BLOCK_SIZE);
					devinfo.lrdbsize32 = DEFAULT_LRDB_BITMAP_SIZE/4;
					devinfo.hrdbsize32 = DEFAULT_HRDB_BITMAP_SIZE/4;
					
					devinfo.lrdb_res		= 0;	// Not Used 
					devinfo.hrdb_res		= 0;	// Not Used 
					devinfo.lrdb_numbits	= 0;	// Not Used 
					devinfo.hrdb_numbits	= 0;	// Not Used 
					devinfo.statsize		= DEFAULT_STATE_BUFFER_SIZE;	// Not Used 
					devinfo.lrdb_offset	= 0;	// Not Used 

					devinfo.devname = config.lgnum.ToString() + "_" + devcfg.devid.ToString();

					devinfo.SuggestedDriveLetterLink = devcfg.devname;
					devinfo.SuggestedDriveLetterLinkLength = (ushort)devcfg.devname.Length;
					devinfo.bSuggestedDriveLetterLinkValid = devcfg.devname.Length > 0;

					devinfo.vdevname = devcfg.vdevname;
					devinfo.strRemoteDeviceName = devcfg.sdevname;



					// TODO: fill in rest of devinfop
					//devinfop.bSignatureUniqueVolumeIdValid = devcfg.symlink1

					driver.ftd_ioctl_new_device( devinfo ); 
				}
			} catch ( Exception e ) {
				OmException.LogException ( new OmException ( "Error: Add_Devices AddDevicesToDriver.", e ) );
			}
		}


		/// <summary>
		/// Create a config file
		/// </summary>
		/// <param name="config"></param>
		public static void WriteConfigFile( LgConfig config ) {
			string path = config.FileNamePath();
			StreamWriter sw = new StreamWriter( path, false );
			try {
				sw.WriteLine( "#===============================================================" );
				sw.WriteLine( "#   Configuration File:   " + config.FileName() );
				sw.WriteLine( "#   Version  " + Application.ProductVersion );
				sw.WriteLine( "#" );
				sw.WriteLine( "#  Last Updated: " + DateTime.Now.ToLongDateString() );
				sw.WriteLine( "#===============================================================" );
				sw.WriteLine( "#" );
				sw.WriteLine( "NOTES: " + config.notes );
				sw.WriteLine( "#" );
				sw.WriteLine( "#" );
				sw.WriteLine( "# Primary System Definition:" );
				sw.WriteLine( "#" );
				sw.WriteLine( "SYSTEM-TAG:          SYSTEM-A                  PRIMARY" );
				sw.WriteLine( "  HOST:                 " + config.phostname );
				sw.WriteLine( "  PSTORE:               " + config.pstore );
				sw.WriteLine( "#" );
				sw.WriteLine( "# Secondary System Definition:" );
				sw.WriteLine( "#" );
				sw.WriteLine( "SYSTEM-TAG:          SYSTEM-B                  SECONDARY" );
				sw.WriteLine( "  HOST:                 " + config.shostname );
				sw.WriteLine( "  JOURNAL:              " + config.jrnpath );
				sw.WriteLine( "  SECONDARY-PORT:       " + config.port );
				sw.WriteLine( "  CHAINING:             " + (config.chaining ? "on" : "off") );
				sw.WriteLine( "#" );
				sw.WriteLine( "# Device Definitions:" );
				sw.WriteLine( "#" );
				foreach ( DevConfig dev in config.devlist ) {
					sw.WriteLine( "PROFILE:" );
					sw.WriteLine( "  REMARK:    " + dev.remark );
					sw.WriteLine( "  PRIMARY:          SYSTEM-A" );
					sw.WriteLine( "  DEVICE:       " + dev.devname + dev.devid );
					sw.WriteLine( "  DATA-DISK:         " + dev.pdevname + " " + dev.sdriveid + " " + dev.ppartstartoffset + " " + dev.ppartlength  );
					sw.WriteLine( "  SECONDARY:        SYSTEM-B" );
					sw.WriteLine( "  MIRROR-DISK:       " + dev.sdevname + " " + dev.sdriveid + " " + dev.spartstartoffset + " " + dev.spartlength );
				}
				sw.WriteLine( "#" );
				sw.WriteLine( "#" );
				sw.WriteLine( "#" );
			} finally {
				sw.Close();
			}
		}

		public static void WriteXML( LgConfig config ) {
			XmlSerializer serializer = new XmlSerializer( typeof( LgConfig ) );
			TextWriter writer = new StreamWriter( config.FileNamePathXML() );
			try {
				// Serialize this class and close the TextWriter.
				serializer.Serialize ( writer, config );
			} finally {
				writer.Close();
			}
		}

		public static LgConfig ReadXML( int lgnum, eRole role ) {
			LgConfig config = new LgConfig( lgnum );
			config.role = role;
			XmlSerializer serializer = new XmlSerializer( typeof( LgConfig ) );
			FileStream fs = new FileStream( config.FileNamePathXML(), FileMode.Open );
			try {
				/* Use the Deserialize method to restore the object's state with
				data from the XML document. */
				return (LgConfig) serializer.Deserialize( fs );
			} finally {
				fs.Close();
			}
		}


		/// <summary>
		/// Delete the config file
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="role"></param>
		public static void DeleteConfigFileXML( int lgnum, eRole role ) {
			LgConfig config = new LgConfig( lgnum );
			config.role = role;
			string name = config.FileNamePathXML();
			File.Delete( name );
		}

		/// <summary>
		/// Delete the config file
		/// </summary>
		/// <param name="lgnum"></param>
		public static void DeletePrimaryConfigFileXML( int lgnum ) {
			DeleteConfigFileXML( lgnum, eRole.Primary );
		}
	}
}
