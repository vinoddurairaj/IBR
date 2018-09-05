using System;
using MasterThread;

namespace CLI.New_Commands
{
	/// <summary>
	/// Summary description for StateCommand.
	/// </summary>
	public class StateCommand : AllGroup
	{
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
		public override void Usage() {
			Console.WriteLine( String.Format( "\nUsage: {0} <options>", this.m_CommandName ) );
			Console.WriteLine( String.Format(	"OPTIONS:" ) );
			base.Usage();
			Environment.Exit( 1 );
		}


		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual void Proc_Args( string[] args ) {
			// At least one argument is required 
			if ( args.Length < 1 ) {
				Usage();
			}
			for ( int index = 0 ; index < args.Length ; index++ ) {
				switch ( args[ index ].ToLower() ) {
					default:
						// handle the group and all commands
						if ( !base.Proc_Args( args, index ) ) {
							Usage();
						}
						break;
				}			
			}
			if (( !m_All ) && ( !m_Group )) {
				Usage();
			}
		}

		protected enum eType {
			ACTIVATEGROUP,
			TRANSITIONTONORMAL,
			STATE
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		protected void Execute( string [] args, eLgModes state, eType type ) {
			try {
				// process the command line arguments
				Proc_Args( args );

				// Create a list of all existing Primary group(s)
				m_ConfigInfos = Config.GetPrimaryConfigs();

				// if none exist then exit  
				if ( m_ConfigInfos.Count == 0 ) {
					OmException e = new OmException( String.Format("Error in " + this.m_CommandName + " no logical group found." ) );
					throw e;
				}
				DriverIOCTL driver = new DriverIOCTL();
				try {
					foreach ( ConfigInfo config in m_ConfigInfos ) {
						if (( m_All ) ||
							( config.lgnum == this.m_lgnum )) {
							switch ( type ) {
								case eType.ACTIVATEGROUP:
									LogicalGroup.ActivateGroup( config.lgnum, this.m_HostID, this.m_Force );
									break;
								case eType.TRANSITIONTONORMAL:
									driver.TransitionToNormal( config.lgnum );
									break;
								case eType.STATE:
									driver.SetGroupState( config.lgnum, state );
									break;
								default:
									throw new OmException( "Error: Invalid eType " + type );
							}
						}
					}
				} finally {
					driver.Close();
				}	
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}

	}
}
