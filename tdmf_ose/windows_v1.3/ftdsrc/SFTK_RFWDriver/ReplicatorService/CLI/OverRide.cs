using System;
using System.Threading;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for OverRide.
	/// </summary>
	public class OverRide : StateCommand
	{
		eLgModes m_State = eLgModes.INVALID;

		/// <summary>
		/// Constructor
		/// </summary>
		public OverRide()
		{
			m_CommandName = CLI.OVERRIDE;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage( bool exit ) {
			base.Usage( false );
			Console.Write( "    [state[ " );

			foreach ( CommandData data in m_Commands ) {
				Console.Write( data.name + "|" );
			}
			Console.WriteLine( "]]" );
			Environment.Exit( 1 );
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public override void Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 3 ) {
				Usage( true );
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					case "-f":
					case "/f":
						m_Force = true;
						break;
					case "state":

						bool found = false;
						string state = args[ ++index ].ToLower();
						foreach ( CommandData data in m_Commands ) {
							if ( data.name.ToLower() == state ) {
								m_State = data.state;
								found = true;
								break;
							}
						}
						if ( !found ) {
							Usage( true );
						}
						break;

//						switch ( args[ ++index ].ToLower() ) {
//							case "passthru":
//								m_State = eLgModes.PASSTHRU;
//								break;
//							case "normal":
//								m_State = eLgModes.NORMAL;
//								break;
//							case "refresh":
//								m_State = eLgModes.REFRESH;
//								break;
//							case "fullrefresh":
//								m_State = eLgModes.FULLREFRESH;
//								break;
//							case "checksumrefresh":
//								m_State = eLgModes.CHECKSUMREFRESH;
//								break;
//							case "tracking":
//								m_State = eLgModes.TRACKING;
//								break;
//							case "backfresh":
//								m_State = eLgModes.BACKFRESH;
//								break;
//							default:
//								Usage( true );
//								break;
//						}
//						break;
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
			Execute ( args, m_State, eExecuteType.STATE );
		}

	}
}