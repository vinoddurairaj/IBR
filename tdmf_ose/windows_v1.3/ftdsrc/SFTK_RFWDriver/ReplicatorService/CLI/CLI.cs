using System;
using MasterThread;
using MasterThread.Management;

namespace CLI
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class CLI
	{
		public static string ACTIVATE			= "ACTIVATE";
		public static string APPLY				= "APPLY";
		public static string CHECKPOINT			= "CHECKPOINT";
		public static string FAILBACK			= "FAILBACK";
		public static string COMMIT				= "COMMIT";
		public static string FAILOVER			= "FAILOVER";
		public static string INFO				= "INFO";
		public static string OVERRIDE			= "OVERRIDE";
		public static string PAUSE				= "PAUSE";
		public static string ROLLBACK			= "ROLLBACK";
		public static string SET				= "SET";
		public static string START				= "START";
		public static string STOP				= "STOP";
		public static string TRACE				= "TRACE";

		//public static string KILL_BACKFRESH		= "KILLBACKFRESH";
		//public static string KILL_PMD			= "KILLPMD";
		//public static string KILL_REFRESH		= "KILLREFRESH";
		//public static string KILL_RMD			= "KILLRMD";
		//public static string LAUNCH_BACKFRESH	= "LAUNCHBACKFRESH";
		//public static string LAUNCH_PMD			= "LAUNCHPMD";
		//public static string LAUNCH_REFRESH		= "LAUNCHREFRESH";
		//public static string RECO				= "RECO";

		public static Command m_Command;		// current command to execute


		struct CommandData {
			public string name;
			public Command command;
			public string help;
			public CommandData (string n, Command cmd, string hlp ) {
				name = n;
				command = cmd;
				help = hlp;
			}
		}

		static CommandData [] m_Commands = {
			new CommandData( ACTIVATE, new Activate(), "Activate\t: Activate a system for control in a cluster enviroment" ),
			new CommandData( APPLY, new Apply(), "Apply\t: Apply configuration to system" ),
			new CommandData( CHECKPOINT, new CheckPoint(), "CheckPoint\t: Set CheckPoint on or off" ),
			new CommandData( FAILBACK, new FailBack(), "FailBack\t: Valid after a FailOver, revert back to primary and toss any secondary changes" ),
			new CommandData( FAILOVER, new FailOver(), "FailOver\t: FailOver to the secondary, making the secondary primary" ),
			new CommandData( INFO, new Info(), "Info\t: Display information about the system" ),
			new CommandData( OVERRIDE, new OverRide(), "OverRide\t: Force logical group transistions to a specific state" ),
			new CommandData( PAUSE, new Pause(), "Pause\t: Suspend data transfer to secondary and track any changes" ),
			new CommandData( ROLLBACK, new RollBack(), "RollBack\t: Valid after a FailOver, revert back to primary and keep all secondary changes" ),
			new CommandData( SET, new Set(), "Set\t: Set specific options" ),
			new CommandData( START, new Start(), "Start\t: Start the logical group, transition to normal state" ),
			new CommandData( STOP, new Stop(), "Stop\t: Stop the logical group, secondary will require a full refresh" ),
			new CommandData( TRACE, new Trace(), "Trace\t: Set trace facility" ),
		};




		/// <summary>
		/// Parse the primary command name
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		public static void GetCommand ( string name ) {
			name = name.ToUpper();
			foreach ( CommandData data in m_Commands ) {
				if ( data.name == name ) {
					m_Command = data.command;
					return;
				}
			}
			// not found, invalid command
			Usage();
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			// At least 2 arguments are required 
			if ( args.Length < 1 ) {
				Usage();
			}
			//init the error facility
			ErrorFacility.Init("Replicator", "CLI.exe", null, null, 0, 1);
			try {
				//send status msg to System Event Log and TDMF Collector
				Management.LogMessage( args );

				GetCommand ( args[0] );
				//clone the args and remove entry 0
				string [] args1 = new string[ args.Length-1 ];
				Array.Copy( args, 1, args1, 0, args.Length-1 );
				// execute the command
				m_Command.Execute( args1 );
			} 
			finally {
				ErrorFacility.Close();
			}
		}

		/// <summary>
		/// displays sub command usage
		/// </summary>
		public static void Usage() {
			Console.WriteLine( String.Format( "\nUsage: CLI <options>\n" ) );
			Console.WriteLine( String.Format( "One of the following sub commands are required:\n" ) );
			foreach ( CommandData data in m_Commands ) {
				Console.WriteLine( "\t" + data.help );
			}
			Console.WriteLine( String.Format( "\n For more detailed help on each subcommand use: CLI 'subcommand name -h'" ) );
			Environment.Exit( 1 );
		}
	}
}
