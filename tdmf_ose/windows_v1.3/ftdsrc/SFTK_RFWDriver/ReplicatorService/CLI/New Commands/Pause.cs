using System;
using MasterThread;

namespace CLI.New_Commands
{
	/// <summary>
	/// Summary description for Pause.
	/// </summary>
	public class Pause : StateCommand {
		public Pause() {
			m_CommandName = CLI.PAUSE;
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.TRACKING, eType.STATE );
		}

	}
}
