using System;
using MasterThread;

namespace CLI {
	/// <summary>
	/// Summary description for RollBack.
	/// </summary>
	public class Commit : StateCommand {
		public Commit() {
			m_CommandName = CLI.COMMIT;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.NORMAL, eExecuteType.COMMIT );
		}
	}
}