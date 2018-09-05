using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for LaunchRefresh.
	/// </summary>
	public class LaunchRefresh : Launch
	{
		/// <summary>
		/// Constructor
		/// </summary>
		public LaunchRefresh()
		{
			m_CommandName = CLI.LAUNCH_REFRESH;
			m_State = eLgModes.REFRESH;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage()
		{
			if ( !m_Pending ) {
				base.Usage();
				Console.WriteLine( String.Format( "    -f              : Perform a Full Refresh" ) );
				Console.WriteLine( String.Format( "    -C              : Perform a Checksum Refresh" ) );
			}
			Environment.Exit( 1 );
		}


		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual void Proc_Args( string[] args, string CommandName )
		{
			// At least one argument is required 
			if ( args.Length < 1 ) {
				Usage();
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					case "-f":
					case "/f":
						// perform Full refresh
						m_State = eLgModes.FULLREFRESH;
						break;
					case "-c":
					case "/c":
						// perform checksum refresh
						m_State = eLgModes.CHECKSUMREFRESH;
						break;
					default:
						if ( !base.Proc_Args( args, index ) ) {
							Usage();
						}
						break;
				}
				if (( !m_All ) && ( !m_Group )) {
					Usage();
				}
			}
		}





	}
}
