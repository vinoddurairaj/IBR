using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Activate.
	/// </summary>
	public class Activate : StateCommand
	{
		public Activate()
		{
			m_CommandName = CLI.ACTIVATE;
		}
		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage( bool exit ) {
			base.Usage( false );
			Console.WriteLine( String.Format(	"[HostID=xxx] - Host ID of the target machine to activate. If empty then uses host of machine executing command on." ) );
			Console.WriteLine( String.Format(	"-f - Force activation of the new Host ID only used when current active host is not offline." ) );
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
					case "-f":
					case "/f":
						// perform Full refresh
						m_Force = true;
						break;
					default:
						// handle the group and all commands
						if ( !base.Proc_Args( args, index ) ) {
							Usage( true );
						}
						break;
				}			
			}
			if (( !m_All ) && ( !m_Group )) {
				Usage( true );
			}
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.INVALID, eExecuteType.ACTIVATEGROUP );
		}
	}
}
