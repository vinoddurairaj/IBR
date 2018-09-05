using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for FailBack.
	/// </summary>
	public class FailBack : StateCommand
	{
		public FailBack()
		{
			m_CommandName = CLI.FAILBACK;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.FAILBACK, eExecuteType.STATE );
		}
	}
}
