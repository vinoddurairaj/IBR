using System;
using System.IO;
using System.Collections;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Set.
	/// </summary>
	public class Set : AllGroup
	{
		bool m_Quiet = false;

		public Set()
		{
			m_CommandName = CLI.SET;
		}

		public struct keyval_t {
			public int intval;
			public string key;
			public string val;
		}
		ArrayList m_Keys = new ArrayList();

		string [] m_KeyList = new string[] { //"CHUNKSIZE",
                                                "CHUNKDELAY",
												"SYNCMODE",
												"SYNCMODEDEPTH",
                                                "SYNCMODETIMEOUT",
												"COMPRESSION",
												"REFRESHTIMEOUT"};

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage( bool exit ) {
			Console.WriteLine( String.Format( "\nUsage: {0} <options>", this.m_CommandName ) );
			Console.WriteLine( String.Format(	"OPTIONS:n" ) );
			Console.WriteLine( String.Format( "    -g <group_num>  : {0} the group", this.m_CommandName ) );
			Console.WriteLine( String.Format( "    [ key=value ... ]" ) );
			Console.WriteLine( String.Format( "    -o              : Output to file" ) );
			Console.WriteLine( String.Format( "    -q              : Don't report errors" ) );
			Environment.Exit( 1 );
		}


		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual void	Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 2 ) {
				Usage( true );
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
//					case "-o": 
//					case "/o": 
//						m_Output = true;
//						break;
					case "-q":
					case "/q":
						m_Quiet = true;
						break;
					default:
						// check if its a key
						string [] keys = args[ index ].Split( '=' );
						bool found = false;

						foreach ( String str in m_KeyList ) {
							if ( String.Compare( keys[0], str, true ) == 0 ) {
								keyval_t keyval = new keyval_t();
								keyval.key = keys[0];
								if ( keys.Length > 1 ) {
									keyval.val = keys[1];
								} else {
									// must be the next arg
									keyval.val = args[++index];
								}
								m_Keys.Add( keyval );
								found = true;
								break;
							}
						}
						if ( found ) {
							break;
						}
						// handle the group and all commands
						if ( !base.Proc_Args( args, index, false ) ) {
							Usage( true );
						}
						break;
				}			
			}
			if ( !m_Group ) {
				Usage( true );
			}
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args )
		{
			//LogicalGroup lgp = new LogicalGroup();

			try {
				// process the command line arguments
				Proc_Args( args );

//				try {
//					lgp = LogicalGroup.Init( this.m_lgnum, eRole.Primary, false );
//				} catch ( Exception e ) {
//					OmException.LogException( new OmException( String.Format("Error in Set Init : {0}", e.Message ), e ) );
//					Environment.Exit( 1 );
//				}
//				string InstallPath = RegistryAccess.InstallPath() + "/";
//				// if no args, then just dump the contents of the persistent store
//				if ( m_Keys.Count == 0 ) {
//					FileStream outFile = null;
//					try {
//						if ( m_Output ) {
//							outFile = File.Open( InstallPath + "\\pstore.txt", FileMode.CreateNew, FileAccess.Write );
//						}
//						lgp.DumpPstore( outFile );
//						Environment.Exit( 0 );
//					} finally {
//						if ( m_Output ) {
//							outFile.Close();
//						}
//					}
//				}

				MasterThread.MasterThread masterThread = MasterThread.MasterThread.GetMasterThread();
				
				foreach ( keyval_t keyval in m_Keys ) {
					validate_toonable_value( keyval );
					try {
						masterThread.SetTunables( this.m_lgnum, keyval.key, keyval.intval );
						//lgp.SetToonableValue( keyval.key, keyval.intval );
					}
					catch ( Exception e ) {
						Console.WriteLine( "ERROR: " + e.Message );
						if ( !m_Quiet ) {
							Console.WriteLine( "BADTUNABLE : " + keyval.key );
						}
						Environment.Exit( 1 );
					}
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

		/// <summary>
		/// ftd_lg_parse_state_value -- assign and/or verify state value in tunables structure
		/// Validate the values
		/// </summary>
		/// <param name="key"></param>
		/// <param name="value"></param>
		/// <param name="tunables"></param>
		/// <param name="verify"></param>
		/// <param name="set"></param>
		/// <returns></returns>
		public void validate_toonable_value( keyval_t keyval ) {

			switch ( keyval.key.ToUpper() ) {
//				case "CHUNKSIZE":
//					intval = Convert.ToInt32(value);
//					if (intval < MIN_CHUNKSIZE || intval > MAX_CHUNKSIZE) {
//						reporterr(ERRFAC, M_CHUNKSZ, ERRWARN, intval,
//							MAX_CHUNKSIZE, MIN_CHUNKSIZE);
//						return false;
//					}
//					break;
				case "SYNCMODEDEPTH":
					keyval.intval = Convert.ToInt32(keyval.val);
					if ( keyval.intval == 0 ) {
						Console.WriteLine( "BADTUNABLE : " + keyval.key );
						Usage( true );
					}
					break;
				case "SYNCMODETIMEOUT":
					keyval.intval = Convert.ToInt32(keyval.val);
					if ( keyval.intval < 1 ) {
						Console.WriteLine( "BADTUNABLE : " + keyval.key );
						Usage( true );
					}
					break;
				case "SYNCMODE":
					switch( keyval.val.ToUpper() ) {
						case "ON":
						case "1":
							keyval.intval = 1;
							break;
						case "OFF":
						case "0":
							keyval.intval = 0;
							break;
						default:
							Console.WriteLine( "BADTUNABLE : " + keyval.key );
							Usage( true );
							break;
					}
					break;
				case "COMPRESSION":
					switch( keyval.val.ToUpper() ) {
						case "ON":
						case "1":
							keyval.intval = 1;
							break;
						case "OFF":
						case "0":
							keyval.intval = 0;
							break;
						default:
							Console.WriteLine( "BADTUNABLE : " + keyval.key );
							Usage( true );
							break;
					}
					break;
				case "CHUNKDELAY":
					keyval.intval = Convert.ToInt32(keyval.val);
					if ( keyval.intval < 0 ) {
						Console.WriteLine( "BADTUNABLE : " + keyval.key );
						Usage( true );
					}
					break;
				case "REFRESHTIMEOUT": 
					keyval.intval = Convert.ToInt32(keyval.val);
					if ( keyval.intval > 8639999 ) {
						Console.WriteLine( "BADTUNABLE : " + keyval.key );
						Usage( true );
					}
					if ( keyval.intval < 0 ) {
						Console.WriteLine( "BADTUNABLE : " + keyval.key );
						Usage( true );
					}
					break;
				default:
					Console.WriteLine( "BADTUNABLE : " + keyval.key );
					Usage( true );
					break;
			}
		}


	}
}
