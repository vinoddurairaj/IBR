using System;
using MasterThread;

namespace CLI.New_Commands
{
	/// <summary>
	/// Summary description for FailOver.
	/// </summary>
	public class FailOver : StateCommand
	{
		public FailOver()
		{
			m_CommandName = CLI.FAILOVER;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.FAILOVER, eType.STATE );
		}
	}
}
