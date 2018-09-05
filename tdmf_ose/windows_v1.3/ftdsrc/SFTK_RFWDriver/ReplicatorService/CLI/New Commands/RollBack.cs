using System;
using MasterThread;

namespace CLI.New_Commands
{
	/// <summary>
	/// Summary description for RollBack.
	/// </summary>
	public class RollBack : StateCommand
	{
		public RollBack()
		{
			m_CommandName = CLI.ROLLBACK;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.ROLLBACK, eType.STATE );
		}
	}
}
