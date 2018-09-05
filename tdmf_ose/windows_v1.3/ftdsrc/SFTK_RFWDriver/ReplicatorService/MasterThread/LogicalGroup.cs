using System;
using System.IO;
using System.Net.Sockets;
using System.Threading;
using System.Diagnostics;

namespace MasterThread {



//	public enum eCheckPoint {
//		CPSTARTP = 0x04,
//		CPSTARTS = 0x08,
//		CPSTOPP = 0x10,
//		CPSTOPS = 0x20
//	}

//	public class LG_Flags {
//		byte unused = 0;
//		byte cp = 0;
//		byte flags1 = 0;
//		byte flags2 = 0;
//
//		public int Flags {
//			set { 
//				unused = (byte)((value >> 24) & 0xff);
//				cp    = (byte)((value >> 16) & 0xff);
//				flags1= (byte)((value >>  8) & 0xff);
//				flags2= (byte)((value >>  0) & 0xff);
//			}
//		}
//
////		public eLgStates GET_LG_STATE() {
////			return (eLgStates)state;
////		}
////		public void SET_LG_STATE( eLgStates s) {
////			state = (byte)s;
////		}
//		public bool GET_LG_CPON () {
//			return (cp & 0x01) > 0;
//		}
//		public void SET_LG_CPON() {
//			cp |= 0x01;
//		}
//		public void UNSET_LG_CPON() {
//			cp &= 0xfe;
//		}
//
//		public bool GET_LG_CPPEND () {
//			return (cp & 0x02) > 0;
//		}
//		public void SET_LG_CPPEND() {
//			cp |= 0x02;
//		}
//		public void UNSET_LG_CPPEND() {
//			cp &= 0xfd;
//		}
//
//		public eLGCP GET_LG_CPSTART () {
//			return (eLGCP)(cp & 0x0c);
//		}
//		public void SET_LG_CPSTART( byte c ) {
//			cp |= c;
//		}
//		public void UNSET_LG_CPSTART() {
//			cp &= 0xf3;
//		}
//
//		public eLGCP GET_LG_CPSTOP () {
//			return (eLGCP)(cp & 0x30);
//		}
//		public void SET_LG_CPSTOP( byte c ) {
//			cp |= c;
//		}
//		public void UNSET_LG_CPSTOP() {
//			cp &= 0xcf;
//		}
//
//		public bool GET_LG_CHKSUM () {
//			return (flags1 & 0x01) > 0;
//		}
//		public void SET_LG_CHKSUM() {
//			flags1 |= 0x01;
//		}
//		public void UNSET_LG_CHKSUM() {
//			flags1 &= 0xfe;
//		}
//
//		public bool GET_LG_BACK_FORCE () {
//			return (flags1 & 0x02) > 0;
//		}
//		public void SET_LG_BACK_FORCE() {
//			flags1 |= 0x02;
//		}
//		public void UNSET_LG_BACK_FORCE() {
//			flags1 &= 0xfd;
//		}
//
//		public bool GET_LG_SZ_INVALID () {
//			return (flags1 & 0x10) > 0;
//		}
//		public void SET_LG_SZ_INVALID() {
//			flags1 |= 0x10;
//		}
//		public void UNSET_LG_SZ_INVALID() {
//			flags1 &= 0xef;
//		}
//
//		/* SET_LG_FULLREFRESH_PHASE2 means that Refresh blocks and BAB entries will go in the target drive (like MIRONLY mode).
//		   This mode changes the behavior the SmartRefresh
//		*/	
//		public bool GET_LG_FULLREFRESH_PHASE2 () {
//			return (flags1 & 0x20) > 0;
//		}
//		public void SET_LG_FULLREFRESH_PHASE2() {
//			flags1 |= 0x20;
//		}
//		public void UNSET_LG_FULLREFRESH_PHASE2() {
//			flags1 &= 0xdf;
//		}
//
//		public bool GET_LG_RFSTART_ACKPEND () {
//			return (flags2 & 0x01) > 0;
//		}
//		public void SET_LG_RFSTART_ACKPEND() {
//			flags2 |= 0x01;
//		}
//		public void UNSET_LG_RFSTART_ACKPEND() {
//			flags2 &= 0xfe;
//		}
//
//		public bool GET_LG_RFDONE () {
//			return (flags2 & 0x02) > 0;
//		}
//		public void SET_LG_RFDONE() {
//			flags2 |= 0x02;
//		}
//		public void UNSET_LG_RFDONE() {
//			flags2 &= 0xfd;
//		}
//
//		public bool GET_LG_RFDONE_ACKPEND () {
//			return (flags2 & 0x04) > 0;
//		}
//		public void SET_LG_RFDONE_ACKPEND() {
//			flags2 |= 0x04;
//		}
//		public void UNSET_LG_RFDONE_ACKPEND() {
//			flags2 &= 0xfb;
//		}
//
//		public bool GET_LG_CHAINING () {
//			return (flags2 & 0x08) > 0;
//		}
//		public void SET_LG_CHAINING() {
//			flags2 |= 0x08;
//		}
//		public void UNSET_LG_CHAINING() {
//			flags2 &= 0xf7;
//		}
//
//		public bool GET_LG_STARTED () {
//			return (flags2 & 0x10) > 0;
//		}
//		public void SET_LG_STARTED() {
//			flags2 |= 0x10;
//		}
//		public void UNSET_LG_STARTED() {
//			flags2 &= 0xef;
//		}
//
//		public bool GET_LG_RFSTART () {
//			return (flags2 & 0x20) > 0;
//		}
//		public void SET_LG_RFSTART() {
//			flags2 |= 0x20;
//		}
//		public void UNSET_LG_RFSTART() {
//			flags2 &= 0xdf;
//		}
//
//		public bool GET_LG_RFTIMEO () {
//			return (flags2 & 0x40) > 0;
//		}
//		public void SET_LG_RFTIMEO() {
//			flags2 |= 0x40;
//		}
//		public void UNSET_LG_RFTIMEO() {
//			flags2 &= 0xbf;
//		}
//
//		public bool GET_LG_BAB_READY () {
//			return (flags2 & 0x80) > 0;
//		}
//		public void SET_LG_BAB_READY() {
//			flags2 |= 0x80;
//		}
//		public void UNSET_LG_BAB_READY() {
//			flags2 &= 0x7f;
//		}
//	}

//	public enum LG_Commands {
//		CINVALID	= 255,				/*  invalid input			*/
//		CPASSTHRU	= 1,				/*  passthru cmd				*/ 
//		CNORMAL		= 2,				/*  normal cmd				*/
//		CREFRESH	= 3,				/*  refresh cmd				*/
//		CREFRESHF	= 4,				/*  refresh full cmd			*/
//		CREFRESHC	= 5,				/*  refresh check cmd		*/
//		CBACKFRESH	= 6,				/*  backfresh cmd			*/
//		CTRACKING	= 7,				/*  tracking cmd				*/
//		CCPON		= 8,				/*  checkpoint on cmd		*/
//		CCPOFF		= 9,				/*  checkpoint off cmd		*/
//		CSTARTJRNL	= 10,				/*  start jrnling cmd		*/
//		CSTOPJRNL	= 11,				/*  stop jrnling cmd			*/
//		CCPSTART	= 12,				/*  checkpoint start cmd		*/
//		CCPSTOP		= 14,				/*  checkpoint stop cmd		*/
//		CCPPEND		= 15,				/*  checkpoint pend cmd		*/
//		CREFTIMEO	= 16,				/*  refresh timeout			*/
//	}

//	public enum eLgStates {
//		SPASSTHRU	= 0,				/*  passthru					*/ 
//		SNORMAL		= 1,				/*  normal					*/
//		SREFRESH	= 2,				/*  refresh					*/
//		SREFRESHF	= 3,				/*  refresh full				*/
//		SREFRESHC	= 4,				/*  refresh check			*/
//		SBACKFRESH	= 5,				/*  backfresh				*/
//		SREFOFLOW	= 6,				/*  refresh overflow			*/
//		STRACKING	= 7,				/*  tracking					*/
//		SAPPLY		= 8,				/*  journal apply state		*/
//		SINVALID	= 9,				/*  invalid state			*/
//		SBACKFRESHF	= 10,				/*  backfresh - force		*/
//		LG_BACK_FORCE = 0x100
//	}


	public enum eLgModes {
		INVALID = 0,
		M_JNLUPDATE = 0x01,
		M_BITUPDATE = 0x02,
		PASSTHRU = 0x10,
		NORMAL = M_JNLUPDATE,
		TRACKING = M_BITUPDATE,
		REFRESH = (M_JNLUPDATE | M_BITUPDATE),
		BACKFRESH = 0x20,
		FULLREFRESH = (0x40 | M_BITUPDATE),
		CHECKSUMREFRESH = (0xC0 | M_BITUPDATE),
		CHECKPOINT_JLESS = (0x200 | M_BITUPDATE),

		BACKFORCE = 0x100,
		TRANSITIONTONORMAL = 0x8000,
		ACTIVATEGROUP = 0x8001,
		FAILBACK = 0x8002,
		FAILOVER = 0x8004,
		ROLLBACK = 0x8008,
	}


	/// <summary>
	/// Summary description for LogicalGroup.
	/// </summary>
	public class LogicalGroup {

		/* logical group structure */
		//public class ftd_lg_t {
			public int lgnum;					/* lg number				*/
			//public eLgModes eModes;			// new field holds modes replaces flags
			//public LG_Flags flags;				/* lg state bits			*/
			public DriverIOCTL driver = null;			/* master control device to driver*/
			//public LogicalGroupIOCTL devfd = null;		/* lg device file descriptor*/
			//public string devname;				/* lg control device name		*/
			public Socket isockp;			/* lg ipc socket object address		*/	
			public Socket dsockp;			/* lg data socket object address	*/	
			public LgConfig cfgp;			/* lg configuration file state		*/ 
			public Tunables tunables;		/* lg tunable parameters		*/
			//public ftd_journal_t jrnp;	/* lg journal object address		*/
			//		//TODO:		public ftd_lg_stat_t *statp;		/* lg runtime statistics		*/
			public IntPtr monitp = IntPtr.Zero;			// only used by old code; ftd_mngt_lg_monit_t *monitp; lg runtime monitoring
			//		public ThrottleCollection throttles;	/* lg throttle list			*/
			//		public DeviceCollection devlist;	/* lg device list			*/

		/// <summary>
		/// Constructor
		/// </summary>
		public LogicalGroup() {
		}

		~LogicalGroup() {
			Close();
			if ( this.driver != null ) {
				this.driver.Close();
				this.driver = null;
			}
		}


		/// <summary>
		/// ftd_lg_init -- initialize the ftd_lg_t object
		/// </summary>
		/// <param name="lgnum"></param>
		/// <param name="role"></param>
		/// <param name="startflag"></param>
		/// <returns></returns>
		public static LogicalGroup Init ( int lgnum, eRole role, bool startflag ) {
			LogicalGroup lgp = new LogicalGroup();

			lgp.lgnum = lgnum;

			lgp.cfgp = Config.ReadConfig( lgp.lgnum, role );

			if (lgp.cfgp.role == eRole.Primary ) {
				// open the master control device
				try {
					lgp.driver = new DriverIOCTL();
				} catch( Exception ) {
					lgp.driver = null;
				}
			}

			Management.Management.InitLgMonitor( lgp );

			return lgp;
		}


		public void Close() {
			if ( monitp != IntPtr.Zero ) {
				Management.Management.Mngt_Delete_Lg_Monit( monitp );
				monitp = IntPtr.Zero;
			}
		}
		
//		/*
//		 * ftd_lg_open --
//		 * open the logical group control device 
//		 */
//		public bool OpenDev() {
//			// should we open the logical group to send sentinals or should we have an IOCL to send them
//			//devname = LogicalGroupIOCTL.Name( lgnum );
//			// open the group control device
//			try {
//				//devfd = new LogicalGroupIOCTL( lgnum );
//			} catch ( Exception e ) {
//				throw new OmException( "Error: OpenDev LogicalGroupIOCTL failed - " + e.Message, e );
//			}
//			return true;
//		}

//		/// <summary>
//		/// ftd_lg_add -- add the logical group state to the pstore and driver
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <param name="autostart"></param>
//		/// <returns></returns>
//		public void LgAdd() {
//			//			Statistics_t      lgstat;
//			eLgModes state = eLgModes.INVALID;
//
//			// add group
//			AddGroupToDriver( 0, ref state );
//
//			// open the logical group device
////			OpenDev();
//
//			try {
//				// tell the driver about the configured devices in this logical group
//				AddDevicesToDriver();
//
//				// configtool may not have been able to set the tunables in 
//				// the pstore because start had not been run, so it stores
//				// them in a temp file that we'll read in and delete now.
//				//
//				// This is now a batch file that will be executed. tmp files
//				// cannot be executed anyhow. The format is settunablesX.bat
//
//				string tunesetpath = RegistryAccess.InstallPath() + "/settunables" + lgnum + ".bat";
//				if ( File.Exists( tunesetpath ) ) {
//					Process process = new Process();
//					process.StartInfo.FileName = tunesetpath;
//					process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
//					process.StartInfo.UseShellExecute = false;
//					process.Start();
//					process.WaitForExit( 1000 * 60 * 2 );
//
//					File.Delete( tunesetpath );
//				}
//
//				// get tunables from driver 
//				GetDriverTunables();
//    
//				// init the tunables in the driver 
//				SetDriverTunables();
//
//			} finally {
//				Close();
//			}
//		}

//		/// <summary>
//		/// ftd_lg_set_driver_state -- set group state in driver
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <returns></returns>
//		private void SetDriverTunables() {
//			// set sync mode depth in driver 
//			if ( tunables.syncmode != 0 ) {
//				driver.SetSyncDepth( lgnum, tunables.syncmodedepth );
//			} 
//			else {
//				driver.SetSyncDepth( lgnum, -1 );
//			}
//			// set iodelay in driver 
//			driver.SetIodelay( lgnum, tunables.iodelay );
//			// set syncmodetimeout in driver 
//			driver.SetSyncTimeout( lgnum, tunables.syncmodetimeout );
//
//			driver.SetLgStateBuffer( lgnum, tunables );
//		}
//
//		/*
//		 * ftd_lg_get_driver_state -- get group state from driver
//		 */
//		public void GetDriverTunables() {
//			driver.GetLgStateBuffer( lgnum, out tunables );
//		}

//		/// <summary>
//		/// ftd_lg_rem -- remove group state from driver and pstore
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <param name="autostart"></param>
//		/// <param name="silent"></param>
//		/// <returns></returns>
//		public void RemoveGroup() {
//			// must close the lg device before we remove it in ftd_ioctl_del_lg
//			Close();
//			driver.ftd_ioctl_del_lg( lgnum );
//		}

//		/// <summary>
//		/// ftd_lg_set_state_value -- sets a tunable paramter in the driver and pstore given a <KEY>: and value.
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <param name="inkey"></param>
//		/// <param name="invalue"></param>
//		/// <returns></returns>
//		public void SetToonableValue( string inkey, int intval ) {
//			/* get current driver state */
//			GetDriverTunables();
//			/* set the new lg state value in the tunables structure */
//			SetToonableValue( tunables, inkey, intval );
//			/* set the lg state in the driver */
//			SetDriverTunables();
//		}
//
//		/// <summary>
//		/// ftd_lg_parse_state
//		/// set the new lg state value in the tunables structure
//		/// </summary>
//		/// <param name="key"></param>
//		/// <param name="value"></param>
//		/// <param name="tunables"></param>
//		/// <returns></returns>
//		private void SetToonableValue( Tunables tunables, string key, int intval ) {
//
//			switch ( key.ToUpper() ) {
//				case "SYNCMODEDEPTH":
//					tunables.syncmodedepth = intval;
//					break;
//				case "SYNCMODETIMEOUT":
//					tunables.syncmodetimeout = intval;
//					break;
//				case "SYNCMODE":
//					tunables.syncmode = intval;
//					break;
//				case "COMPRESSION":
//					tunables.compression = intval;
//					break;
//				case "CHUNKDELAY":
//					tunables.chunkdelay = intval;
//					break;
//				case "REFRESHTIMEOUT": 
//					tunables.refrintrvl = intval;
//					break;
//				default:
//					throw new OmException ( "SetToonableValue: Invalid key - " + key );
//			}
//		}


//		/// <summary>
//		/// Start the group
//		/// </summary>
//		/// <param name="lgnum"></param>
//		/// <param name="force"></param>
//		/// <param name="autostart"></param>
//		/// <returns></returns>
//		public static void StartGroup( int lgnum, bool force, bool autostart ) {
//			LogicalGroup lgp;
//
//			string cfgName = Config.PRIMARY_CFG_PREFIX + lgnum.ToString("D3") + "." + Config.PATH_CFG_SUFFIX;
//			//string curcfgName = Config.PRIMARY_CFG_PREFIX + lgnum.ToString("D3") + "." + Config.PATH_STARTED_CFG_SUFFIX;
//
//			// unlink the old .cur file in case we starting after an unorderly 
//			// shutdown.  (ftdstop should remove this file otherwise.)
//			//
//			// DTurrin - Sept 13th, 2001
//			// Changed from "unlink" command to "access". If the file
//			// exists, return an error message because this might be
//			// an attempt to start an already started group.
//			
////			string InstallPath = RegistryAccess.InstallPath() + "/";
//
////			if ( (File.Exists( InstallPath + curcfgName ))  && !force ) {
////				string msg = String.Format( "{0} still exists. Group %03d is already started.", curcfgName, lgnum );
////				Console.WriteLine( msg );
////				throw new OmException ( msg );
////			}
//    
//			try {
//				lgp = LogicalGroup.Init( lgnum, eRole.Primary, false );
//			} catch ( Exception e ) {
//				Console.WriteLine( String.Format( "Error - start_group(), Calling Init()" ) );
//				throw e;
//			}
//			DriverIOCTL driver = new DriverIOCTL();
//			try {
//				try {
//					driver.sftk_lg_add_connections( lgp );
//				} catch ( Exception e ) {
//					Console.WriteLine( String.Format( "Error - start_group(), Calling sftk_lg_add_connection()" ) );
//					throw e;
//				}
//				try {
//					lgp.LgAdd();
//				} catch ( Exception e ) {
//					Console.WriteLine( String.Format( "Error - start_group(), Calling LgAdd()" ) ); 
//					throw e;
//				}
//				try {
//					// set checkpoint off in pstore
//					driver.ps_set_group_checkpoint( lgp.lgnum, false );
//					driver.ftd_stat_connection_driver( lgp.lgnum, -1 );
//				}
//				catch ( Exception e ) {
//					Console.WriteLine( String.Format( "Error - start_group(), Calling ps_set_group_checkpoint()" ) ); 
//					throw e;
//				}
//			}
//			finally {
//				driver.Close();
//			}
//			/* now copy the .cfg file for this group to .cur */
////			File.Copy( InstallPath + cfgName, InstallPath + curcfgName, true );
//		}


		/// <summary>
		/// Tell driver to Activate this logical group
		/// </summary>
		/// <param name="lgnum"></param>
		public static void ActivateGroup( int lgnum, string HostID, bool force ) {
			DriverIOCTL driver = new DriverIOCTL();
			try {
				try {
					driver.Activate( lgnum, HostID, force );
				} catch ( Exception e ) {
					throw new OmException( String.Format("Error in ActivateGroup Activate : {0}", e.Message ), e );
				} 
			}
			finally {
				driver.Close();
			}
		}

	}
}

