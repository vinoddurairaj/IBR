//using System;
//using MasterThread;
//
//namespace CLI
//{
//	/// <summary>
//	/// Summary description for Stop.
//	/// </summary>
//	public class Stop : AllGroup
//	{
//
//		//bool m_AutoStart = false;
//
//		public Stop()
//		{
//			m_CommandName = CLI.STOP;
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
//			Console.WriteLine( String.Format( "    -s              : don't clear autostart in pstore" ) );
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
////					case "-s":
////					case "/s":
////						m_AutoStart = false;
////						break;
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
//		/// <summary>
//		/// Main execution call
//		/// </summary>
//		/// <param name="args"></param>
//		public override void Execute( string [] args )
//		{
//			////------------------------------------------
//			//void ftd_util_force_to_use_only_one_processor();
//			//ftd_util_force_to_use_only_one_processor();
//			//------------------------------------------
//
//			try {
//				// process the command line arguments
//				Proc_Args( args );
//
//				// Create a list of all existing Primary started group(s), by refering to sxxx.cur
//				// get the registry install Path
//				m_ConfigInfos = Config.GetPrimaryConfigInfos();
////				m_ConfigInfos = Config.GetPrimaryStartedConfigs();
//
//				// if none exist then exit  
//				if ( m_ConfigInfos.Count == 0 ) {
//					OmException e = new OmException( String.Format("Error in Launch no logical group found." ) );
//					throw e;
//				}
//
//				foreach ( ConfigInfo config in m_ConfigInfos ) {
//
//					if (( m_All ) ||
//						( config.lgnum == this.m_lgnum )) {
//
//						//          if (ftd_lg_init(lgp, cfgp->lgnum, ROLEPRIMARY, 1) < 0) 
//						LogicalGroup lgp;
//						try {
//							lgp = LogicalGroup.Init( config.lgnum, eRole.Primary, true );
//							lgp.RemoveGroup();
//						} catch ( Exception e ) {
//							throw e;
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
//
//	}
//}
