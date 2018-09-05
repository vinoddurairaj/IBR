using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Stop.
	/// </summary>
	public class Stop : StateCommand
	{
		public Stop()
		{
			m_CommandName = CLI.STOP;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.PASSTHRU, eExecuteType.STATE );
		}
	}
}
