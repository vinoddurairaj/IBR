//using System;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for LaunchBackfresh.
//	/// </summary>
//	public class LaunchBackfresh : Launch
//	{
//		/// <summary>
//		/// Constructor
//		/// </summary>
//		public LaunchBackfresh()
//		{
//			m_CommandName = CLI.LAUNCH_BACKFRESH;
//			m_State = eLgModes.BACKFRESH;
//		}
//
//		/// <summary>
//		/// Displays help
//		/// </summary>
//		/// <param name="name"></param>
//		public override void Usage()
//		{
//			if ( !m_Pending ) {
//				base.Usage();
//				Console.WriteLine( String.Format( "    -f              : Force" ) );
//			}
//			Environment.Exit( 1 );
//		}
//
//		/// <summary>
//		/// process argument vector 
//		/// </summary>
//		/// <param name="args"></param>
//		public virtual void Proc_Args( string[] args )
//		{
//			// At least one argument is required 
//			if ( args.Length < 1 ) {
//				Usage();
//			}
//			for ( int index = 0 ; index < args.Length ; index++ ) {
//				switch ( args[ index ].ToLower() ) {
//					case "-f":
//					case "/f":
//						// force - kill journals and apply
//						m_State = eLgModes.BACKFRESH | eLgModes.BACKFORCE;
//						break;
//					default:
//						if ( !base.Proc_Args( args, index ) ) {
//							Usage();
//						}
//						break;
//				}
//				if (( !m_All ) && ( !m_Group )) {
//					Usage();
//				}
//			}
//		}
//
//	}
//}
