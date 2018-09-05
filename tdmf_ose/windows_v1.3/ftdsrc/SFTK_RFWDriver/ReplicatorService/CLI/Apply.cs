using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Apply.
	/// </summary>
	public class Apply : Command {

		public Apply() {
			m_CommandName = CLI.APPLY;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			try {
				// apply all of the configurations
				ConsoleAPI api = new ConsoleAPI();
				api.ApplyAllGroupsXML();
			} catch ( Exception e ) {
				OmException.LogException( new OmException( e.Message, e ) );
				Environment.Exit( 1 );
			}
			Environment.Exit( 0 );
		}
	}
}
