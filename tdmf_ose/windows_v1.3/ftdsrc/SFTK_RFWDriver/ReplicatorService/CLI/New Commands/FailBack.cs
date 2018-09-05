using System;
using MasterThread;

namespace CLI.New_Commands
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
			Execute ( args, eLgModes.FAILBACK, eType.STATE );
		}
	}
}
