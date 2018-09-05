using System;
using MasterThread;

namespace CLI
{
	/// <summary>
	/// Summary description for Command.
	/// </summary>
	public class Command
	{

		public string m_CommandName;
		public ConfigInfoCollection m_ConfigInfos;		// collection of current configurations
		public int m_lgnum;

		public Command()
		{
		}

		/// <summary>
		/// Main execution call
		/// </summary>
		/// <param name="args"></param>
		public virtual void Execute( string [] args ) {
		}

	}
}
