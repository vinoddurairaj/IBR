//using System;
//using System.IO;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for Start.
//	/// </summary>
//	public class Start : AllGroup
//	{
//		public bool m_AutoStart = true;
//		bool m_Force = false;
//
//		/// <summary>
//		/// Constructor
//		/// </summary>
//		public Start()
//		{
//			m_CommandName = CLI.START;
//		}
//
//		/// <summary>
//		/// Displays help
//		/// </summary>
//		/// <param name="name"></param>
//		public override void Usage() {
//			Console.WriteLine( String.Format( "\nUsage: {0} <options>", this.m_CommandName ) );
//			Console.WriteLine( String.Format(	"OPTIONS:" ) );
//			base.Usage();
//			Console.WriteLine( String.Format( "    -f              : force" ) );
//			Console.WriteLine( String.Format( "    -b              : don't set automatic start state" ) );
//			Environment.Exit( 1 );
//		}
//
//		/// <summary>
//		/// process argument vector 
//		/// </summary>
//		/// <param name="args"></param>
//		public virtual void	Proc_Args( string[] args ) {
//			// At least one argument is required 
//			if ( args.Length < 1 ) {
//				Usage();
//			}
//			for ( int index = 0 ; index < args.Length ; index++ ) {
//				switch ( args[ index ].ToLower() ) {
//					case "-b":
//					case "/b":
//						m_AutoStart = false;
//						break;
//					case "-f":
//					case "/f":
//						m_Force = true;
//						break;
//					default:
//						// handle the group and all commands
//						if ( !base.Proc_Args( args, index ) ) {
//							Usage();
//						}
//						break;
//				}			
//			}
//			if (( !m_All ) && ( !m_Group )) {
//				Usage();
//			}
//		}
//
//
//		/// <summary>
//		/// Main execution call
//		/// </summary>
//		/// <param name="args"></param>
//		public override void Execute( string [] args )
//		{
//			try {
//				// process the command line arguments
//				Proc_Args( args );
//
//				if ( m_AutoStart ) {
//					// Create a list of all existing Primary started group(s), by refering to sxxx.cur
//					// get the registry install Path
//					m_ConfigInfos = Config.GetPrimaryConfigInfos();
////					m_ConfigInfos = Config.GetPrimaryStartedConfigs();
//				} else {
//					// get paths of all groups 
//					// Create a list of all existing Primary group(s), by refering to sxxx.cur
//					// get the registry install Path
//					m_ConfigInfos = Config.GetPrimaryConfigInfos();
//				}
//				// if none exist then exit  
//				if ( m_ConfigInfos.Count == 0 ) {
//					OmException e = new OmException( String.Format("Error in Start no logical group found." ) );
//					throw e;
//				}
//				foreach ( ConfigInfo config in m_ConfigInfos ) {
//					if (( m_All ) ||
//						( config.lgnum == this.m_lgnum )) {
//						// start the group
//						try {
//							LogicalGroup.StartGroup( config.lgnum, m_Force, m_AutoStart );
//						} catch ( OmException e ) {
//							OmException.LogException( e );
//						}
//					}
//				}
//			} catch ( Exception e ) {
//				OmException.LogException( new OmException( e.Message, e ) );
//				Environment.Exit( 1 );
//			}
//			Environment.Exit( 0 );
//		}
//
//	}
//}
