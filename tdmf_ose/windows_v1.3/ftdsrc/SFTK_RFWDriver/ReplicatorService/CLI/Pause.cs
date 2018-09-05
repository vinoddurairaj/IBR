using System;
using MasterThread;

namespace CLI
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
			Execute ( args, eLgModes.TRACKING, eExecuteType.STATE );
		}

	}
}
