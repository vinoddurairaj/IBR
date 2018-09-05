using System;
using System.Net.Sockets;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Trace.
	/// </summary>
	public class Trace : Command {

		public int m_Level;
		public bool m_DriverTraceLvl = false;
		public Trace() {
			m_CommandName = CLI.TRACE;
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public void Usage() {
			Console.WriteLine( String.Format( "\nUsage: {0} [-lx | -off | -on] [-b]", this.m_CommandName ) );
			Console.WriteLine( String.Format(	"Set trace level of Server or Block device driver.\n" +
				"OPTIONS: -lx => set trace level to decimal value x.\n" +
				"                 levels : 0 = disable traces;\n" +
				"                         1 = errors only;\n" +
				"                         2 = warnings and higher priorities levels of traces;\n" +
				"                         3 = L0 information and higher priorities levels of traces;\n" +
				"                         4 = L1 information and higher priorities levels of traces;\n" +
				"                         5 = L2 information and higher priorities levels of traces;\n" +
				"         -on  => equivalent to -l3.\n" +
				"         -off => equivalent to -l0.\n" +
				"         -b   => send trace level to Block device driver.\n" +
				"                 Without -b, trace level request is sent only to Server."
				) );
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
					case "-l1":
					case "/l1":
						m_Level = 1;
						break;
					case "-l2":
					case "/l2":
						m_Level = 2;
						break;
					case "-l3":
					case "/l3":
						m_Level = 3;
						break;
					case "-l4":
					case "/l4":
						m_Level = 4;
						break;
					case "-l5":
					case "/l5":
						m_Level = 5;
						break;
					case "-on":
					case "/on":
						m_Level = 3;
						break;
					case "-off":
					case "/off":
						m_Level = 0;
						break;
					case "-b":
					case "/b":
						m_DriverTraceLvl = true;
						break;
					default:
						Usage();
						break;
				}			
			}
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="level"></param>
		/// <returns></returns>
		public static void setDriverTraceLevel( int level ) {
			// open the master control device 
			DriverIOCTL driver = new DriverIOCTL();
			try {
				driver.SetTraceLevel( level );
			}
			finally {
				driver.Close();
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

				if ( m_DriverTraceLvl ) {
					setDriverTraceLevel( m_Level );
					Console.WriteLine( "Block driver trace level successfully set to " +  m_Level );
				}

				MasterThread.MasterThread masterThread = MasterThread.MasterThread.GetMasterThread();
				masterThread.SetTraceLevel( m_Level );

				Console.WriteLine( "Server trace level successfully set to " + m_Level );

			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

	}
}
