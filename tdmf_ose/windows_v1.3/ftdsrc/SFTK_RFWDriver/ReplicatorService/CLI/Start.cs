using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Start.
	/// </summary>
	public class Start : StateCommand
	{
		public Start()
		{
			m_CommandName = CLI.START;
		}
		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public override void Execute( string [] args ) {
			Execute ( args, eLgModes.NORMAL, eExecuteType.TRANSITIONTONORMAL );
		}
	}
}

