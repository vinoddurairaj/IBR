//using System;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for LaunchPMD.
//	/// </summary>
//	public class LaunchPMD : Launch
//	{
//		/// <summary>
//		/// Constructor
//		/// </summary>
//		public LaunchPMD()
//		{
//			m_CommandName = CLI.LAUNCH_PMD;
//			m_State = eLgModes.NORMAL;
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
//			}
//			Environment.Exit( 1 );
//		}
//
//
//		/// <summary>
//		/// process argument vector 
//		/// </summary>
//		/// <param name="args"></param>
//		public virtual void Proc_Args( string[] args ) {
//
//			// At least one argument is required 
//			if ( args.Length < 1 ) {
//				Usage();
//			}
//			for ( int index = 0 ; index < args.Length ; index++ ) {
//				switch ( args[ index ].ToLower() ) {
//					default:
//						if ( !base.Proc_Args( args, index ) ) {
//							Usage();
//						}
//						break;
//				}
//			}
//			if (( !m_All ) && ( !m_Group )) {
//				Usage();
//			}
//			return;
//		}
//
//
//
//	}
//}
