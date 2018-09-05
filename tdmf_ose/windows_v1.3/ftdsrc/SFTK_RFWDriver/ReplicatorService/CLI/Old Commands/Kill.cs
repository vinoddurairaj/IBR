//using System;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for Kill.
//	/// </summary>
//	public class Kill : AllGroup
//	{
//		public Kill()
//		{
//		}
//
//		/// <summary>
//		/// Displays help
//		/// </summary>
//		/// <param name="name"></param>
//		public override void Usage() {
//			Console.WriteLine( String.Format( "\nUsage: {0} <options>\n", this.m_CommandName ) );
//			Console.WriteLine( String.Format(	"OPTIONS:" ) );
//			base.Usage();
//			Environment.Exit( 1 );
//		}
//
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
//	}
//}
