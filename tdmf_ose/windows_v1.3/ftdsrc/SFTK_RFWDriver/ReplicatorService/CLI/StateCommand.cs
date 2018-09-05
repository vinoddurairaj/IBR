using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for StateCommand.
	/// </summary>
	public class StateCommand : AllGroup
	{
		public static string PASSTHRU = "Passthru";
		public static string NORMAL = "Normal";
		public static string REFRESH = "Refresh";
		public static string FULLREFRESH = "FullRefresh";
		public static string CHECKSUMREFRESH = "Checksumrefresh";
		public static string TRACKING = "Tracking";
		public static string BACKFRESH = "Backfresh";

		public struct CommandData {
			public string name;
			public eLgModes state;
			public CommandData (string n, eLgModes state ) {
				name = n;
				this.state = state;
			}
		}

		public static CommandData [] m_Commands = {
			new CommandData( PASSTHRU, eLgModes.PASSTHRU ),
			new CommandData( NORMAL, eLgModes.NORMAL ),
			new CommandData( REFRESH, eLgModes.REFRESH ),
			new CommandData( FULLREFRESH, eLgModes.FULLREFRESH ),
			new CommandData( CHECKSUMREFRESH, eLgModes.CHECKSUMREFRESH ),
			new CommandData( TRACKING, eLgModes.TRACKING ),
			new CommandData( BACKFRESH, eLgModes.BACKFRESH ),
		};


		public eLgModes m_Mode = eLgModes.INVALID;
		public bool m_Force = false;
		public string m_HostID = "";

		/// <summary>
		/// Constructor
		/// </summary>
		public StateCommand()
		{
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public override void Usage( bool exit ) {
			Console.WriteLine( String.Format( "\nUsage: {0} <options>", this.m_CommandName ) );
			Console.WriteLine( String.Format(	"OPTIONS:" ) );
			base.Usage( true );
			if ( exit ) {
				Environment.Exit( 1 );
			}
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual void Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 1 ) {
				Usage( true );
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
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
		protected void Execute( string [] args, eLgModes state, eExecuteType type ) {
			try {
				// process the command line arguments
				Proc_Args( args );

				CLIData data = new CLIData();
				data.All = m_All;
				data.CommandName = m_CommandName;
				data.Force = m_Force;
				data.HostID = m_HostID;
				data.lgnum = m_lgnum;
				data.State = state;
				data.Type = type;
				ConsoleAPI api = new ConsoleAPI();
				api.ExecuteCLI( data );
//				MasterThread.MasterThread masterThread = MasterThread.MasterThread.GetMasterThread();
//				masterThread.ExecuteCLI( data );
			} catch ( Exception e ) {
				Console.WriteLine( e.Message );
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

	}
}
