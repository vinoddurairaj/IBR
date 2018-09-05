using System;
using System.IO;
using System.Net.Sockets;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Reco. - failover - primary to secondary 
	/// </summary>
	public class Reco : AllGroup
	{
		bool m_Disable = false;
		bool m_Verbose = false;
		bool m_Force = false;

		/// <summary>
		/// Constructor
		/// </summary>
		public Reco()
		{
			//
			// TODO: Add constructor logic here
			//
			m_CommandName = CLI.RECO;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage() {
			Console.WriteLine( String.Format( "\nUsage: {0} <options>", this.m_CommandName ) );
			Console.WriteLine( String.Format(	"OPTIONS:" ) );
			base.Usage();
			Console.WriteLine( String.Format( "The following options modify the behavior:" ) );
			Console.WriteLine( String.Format( "    -f              : Force mirror offline (ie. kill rmd if running)" ) );
			Console.WriteLine( String.Format( "    -d              : Disable the fail over (go back to primary)" ) );
			Console.WriteLine( String.Format( "    -v              : Verbose mode" ) );
			Environment.Exit( 1 );
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual void	Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 1 ) {
				Usage();
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					case "-d":
					case "/d":
						m_Disable = true;
						break;
					case "-v":
					case "/v":
						m_Verbose = true;
						break;
					case "-f":
					case "/f":
						m_Force = true;
						break;
					default:
						// handle the group and all commands
						if ( !base.Proc_Args( args, index ) ) {
							Usage();
						}
						break;
				}			
			}
			if (( !m_All ) && ( !m_Group )) {
				Usage();
			}
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args )
		{
			try {
				// process the command line arguments
				Proc_Args( args );

				// get paths of all groups 
				// Create a list of all existing Primary group(s), by refering to sxxx.cur
				// get the registry install Path
				m_ConfigInfos = Config.GetSecondaryConfigs();
				string InstallPath = RegistryAccess.InstallPath();

				foreach ( ConfigInfo config in m_ConfigInfos ) {

					string filename = InstallPath + "\\s" + config.lgnum.ToString("D3") + ".off";
					try {

						if (( m_All ) ||
							( config.lgnum == this.m_lgnum )) {
							if ( m_Verbose ) {
								Console.WriteLine( "Recovering logical group " + config.lgnum );
							}
							if ( m_Disable ) {
								try {
									File.Delete( filename );
								} catch ( Exception e ) {
									OmException.LogException( new OmException( String.Format( "Couldn't delete file {0}: {1}", filename, e.Message ), e ) );
								}
								continue;
							}
							// create a .off file for this group
							FileStream off = null;
							try {
								off = File.Open( filename, FileMode.Create, FileAccess.Write );
							} catch ( Exception e ) {
								OmException.LogException( new OmException( String.Format( "Couldn't open file {0}: {1}", filename, e.Message ), e ) );
							} finally {
								off.Close();
							}

							// create an ipc socket and connect it to the local master
							Socket MasterSocket = Protocol.IPCConnect();
							try {
								// send the StartRECO and wait for ackCLI response
								bool running = false;
								running = Protocol.SendStartRECO( MasterSocket, config.lgnum, m_Force );
								if ( running ) {
									if ( !m_Force ) {
										Console.WriteLine( " Use '" + this.m_CommandName + "' -f to kill and force the mirror offline" );
										Environment.Exit( 1 );
									}
								}
							}
							finally {
								MasterSocket.Close();
							}
						}
					} catch ( Exception e ) {
						File.Delete( filename );
						throw e;
					}
				}
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

	}
}
