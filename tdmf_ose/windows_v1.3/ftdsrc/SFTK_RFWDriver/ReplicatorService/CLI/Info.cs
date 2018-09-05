using System;
using System.Collections;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Info.
	/// </summary>
	public class Info : StateCommand
	{
		bool m_Other = false;
		bool m_Version = false;

		public Info()
		{
			m_CommandName = CLI.INFO;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage( bool exit ) {
			base.Usage( false );
			Console.WriteLine( String.Format( "    -o              : Display other info\n" +
											  "    -v              : Print Software Version and Build Sequence Number"
				) );
			Environment.Exit( 1 );
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public override void Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 1 ) {
				Usage( true );
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					case "-o":
					case "/o":
						/* Display Other Info ( Not for Customer Use ) */
						m_Other = true;
						break;
					case "-v":
					case "/v":
						/* Display Version */
						m_Version = true;
						break;
					default:
						// handle the group and all commands
						if ( !base.Proc_Args( args, index ) ) {
							Usage( true );
						}
						break;
				}			
			}
			if (( !m_All ) &&
				( !m_Other ) &&
				( !m_Version ) &&
				( !m_Group )) {
				Usage( true );
			}
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			try {
				// process the command line arguments
				Proc_Args( args );

				if ( m_Version ) {
					Console.WriteLine( String.Format( "\n{0}\n", Globals.qdsreleasenumber ) );
				}

				DriverIOCTL driver = new DriverIOCTL();
				try {
					// print out the Cache data
					PrintCacheInfo( driver );
				} finally {
					driver.Close();
				}

				// get all primary config files
				MasterThread.MasterThread masterThread = MasterThread.MasterThread.GetMasterThread();
				LgConfigCollection LgConfigs = masterThread.LgConfigsPrimary;

				foreach ( LgConfig config in LgConfigs ) {
					if (( m_All ) ||
						( config.lgnum == this.m_lgnum )) {
						PrintGroup( config, m_Other );
					}
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

		static void PrintGroup( LgConfig config, bool Other ) {
			GroupInfo groupInfo = new GroupInfo();
			Statistics_t lgstat = new Statistics_t();
			int dev_num;
			int i, ndevs;
			bool found;
			disk_stats_t [] devstats;

			Console.WriteLine( String.Format( "\nLogical Group {0} ({1} . {2})",
				config.lgnum, config.phostname, config.shostname ) );

			groupInfo.name = null;
			try {
				DriverIOCTL driver = new DriverIOCTL();
				try {
					try {
						driver.GetGroupInfo( config.lgnum, ref groupInfo );
						Console.WriteLine( String.Format( "{0}", config.pstore ) );
						/* Get the info for the group */
						driver.GetGroupStatistics( config.lgnum, ref lgstat );
					}
					catch ( Exception e ) {
						Console.WriteLine( "Get Group failed: " + e.Message );
						return;
					}

					/* print driver logical group info */
					PrintDriver( config, lgstat );

					/* print pstore logical group info */
					PrintPstore( groupInfo, lgstat );

					ndevs = config.devlist.Count;
					devstats = driver.GetDevStatistics( config.lgnum, -1, ndevs );

					foreach ( DevConfig devp in config.devlist ) {

						Console.WriteLine( String.Format( "\n    Device {0}:", devp.devname ) );
						found = false;
						// only if group has been started 
						// otherwise no sense in trying to get dev nums
						for ( i = 0 ; i < ndevs ; i++ ) {
							if ( String.Compare( devp.devname, devstats[i].devname.Substring( devstats[i].devname.IndexOf( ':' ) - 1 ), true ) == 0 ) {
								// found it
								found = true;
								break;
							}
						}
						if ( !found ) {
							// skip it
							continue;
						}
						dev_num = devstats[i].localbdisk;
						disk_stats_t [] DiskInfos = new disk_stats_t[1];

						//driver.ftd_ioctl_get_dev_stats( lgp.lgnum, dev_num, DiskInfos );

						printlocaldisk( DiskInfos[0], config, devp );
						printbab( DiskInfos[0] );
					}
				}
				finally {
					driver.Close();
				}
			} catch ( Exception e ) {
				Console.WriteLine( "Get Group failed: " + e.Message );
			}
			return;
		}

		static void	PrintCacheInfo( DriverIOCTL driver ) {
			//string buf;
//			uint actual, num_chunks, chunk_size, size, driverstate;

			/* get bab size from driver */
			// TODO: impliment
//			ftd_ioctl_get_bab_size( master, actual );
//
//			num_chunks = chunk_size = 0;
//
//			memset(buf, 0, sizeof(buf));
//			cfg_get_driver_key_value("num_chunks", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
//			num_chunks = strtol(buf, NULL, 0);
//
//			memset(buf, 0, sizeof(buf));
//			cfg_get_driver_key_value("chunk_size", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
//			chunk_size = strtol(buf, NULL, 0);
//
//			memset(buf, 0, sizeof(buf));
//			cfg_get_driver_key_value("Start", FTD_DRIVER_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
//			driverstate = strtol(buf, NULL, 0);
//
//			size = chunk_size * num_chunks;
//
//			Console.WriteLine( String.Format( "\nRequested BAB size ................ {0} (~ {1} MB)", size, size >> 20 ) );
//			Console.WriteLine( String.Format( "Actual BAB size ................... {0} (~ {1} MB)", actual, actual >> 20 ) );
			Console.WriteLine( String.Format( "Driver state ...................... Loaded at Boot-Time" ) );
			return;
		}

		static void  PrintDriver( LgConfig cfgp, Statistics_t Info ) {
			string state = "";

			bool found = false;
			foreach ( CommandData data in m_Commands ) {
				if ( Info.state == data.state ) {
					state = data.name;
					found = true;
					break;
				}
			}
			if ( !found ) {
				state = "Invalid State - " + Info.state.ToString();
			}
//			switch ( Info.state ) {
//				case eLgModes.PASSTHRU:
//					state = "Passthru";
//					break;
//				case eLgModes.NORMAL:
//					state = "Normal";
//					break;
//				case eLgModes.TRACKING:
//					state = "Tracking";
//					break;
//				case eLgModes.REFRESH:
//					state = "Refresh";
//					break;
//				case eLgModes.FULLREFRESH:
//					state = "FullRefresh";
//					break;
//				case eLgModes.CHECKSUMREFRESH:
//					state = "ChecksumRefresh";
//					break;
//				case eLgModes.BACKFRESH:
//					state = "Backfresh";
//					break;
//				default:
//					state = "Invalid State- " + Info.state.ToString();
//					break;
//			}

			Console.WriteLine( String.Format( "\n    Mode of operations.............. {0}", state ) );
			Console.WriteLine( String.Format( "    Entries in the BAB.............. {0}", Info.wlentries ) );
			Console.WriteLine( String.Format( "    Sectors in the BAB.............. {0}", Info.wlsectors ) );
			if ( Info.sync_depth != -1 ) {
				Console.WriteLine( String.Format( "    Sync/Async mode................. Sync" ) );
				Console.WriteLine( String.Format( "    Sync mode target depth.......... %u", Info.sync_depth ) );
				Console.WriteLine( String.Format( "    Sync mode timeout............... %u", Info.sync_timeout ) );
			} else {
				Console.WriteLine( String.Format( "    Sync/Async mode................. Async" ) );
			}
			Console.WriteLine( String.Format( "    Persistent Store................ {0}", cfgp.pstore ) );
			return;
		}

		static void PrintPstore ( GroupInfo ginfo, Statistics_t Info ) {
			if ( ginfo.checkpoint ) {
				if (( Info.state == eLgModes.TRACKING ) ||
					( Info.state == eLgModes.PASSTHRU ) ) {
					Console.WriteLine( String.Format( "    Checkpoint State(not effective). {0}", "Off" ) );
				}
				else { // New report message for WR17056
					Console.WriteLine( String.Format( "    Checkpoint State................ {0}", "On" ) );
				}
			} 
			else {
				Console.WriteLine( String.Format( "    Checkpoint State................ {0}", "Off" ) );
			}
			return;
		}

		static void printother( Statistics_t Info ) {
			string tb;
			DateTime lt;

			lt = new DateTime( Info.loadtimesecs * 10000 );
			tb = lt.ToString();
			Console.WriteLine( String.Format( "\n    Load time....................... {0}", tb ) );
			Console.WriteLine( String.Format( "    Load time system ticks.......... {0}", Info.loadtimesystics ) );
			Console.WriteLine( String.Format( "    Used (calc): {0} Free (calc): {1}", Info.bab_used, Info.bab_free ) );
			return;
		}

		static void printlocaldisk( disk_stats_t DiskInfo, LgConfig cfgp, DevConfig devp ) {
			Console.WriteLine( String.Format( "\n        Local disk device number........ 0x{0}", DiskInfo.localbdisk ) );
			Console.WriteLine( String.Format( "        Local disk size (sectors)....... {0}", DiskInfo.localdisksize ) );
			Console.WriteLine( String.Format( "        Local disk name................. {0}", devp.pdevname ) );
			Console.WriteLine( String.Format( "        Remote mirror disk.............. {0}:{1}", cfgp.shostname, devp.sdevname ) );
			return;
		}

		static void printbab( disk_stats_t DiskInfo ) {
			Console.WriteLine( String.Format( "        Read I/O count.................. {0}", DiskInfo.readiocnt ) );
			Console.WriteLine( String.Format( "        Total # of sectors read......... {0}", DiskInfo.sectorsread ) );
			Console.WriteLine( String.Format( "        Write I/O count................. {0}", DiskInfo.writeiocnt ) );
			Console.WriteLine( String.Format( "        Total # of sectors written...... {0}", DiskInfo.sectorswritten ) );
			Console.WriteLine( String.Format( "        Entries in the BAB.............. {0}", DiskInfo.wlentries ) );
			Console.WriteLine( String.Format( "        Sectors in the BAB.............. {0}", DiskInfo.wlsectors ) );
			return;
		}

	}
}
