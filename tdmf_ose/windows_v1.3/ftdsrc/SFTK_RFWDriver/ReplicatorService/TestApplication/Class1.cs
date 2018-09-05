using System;
using MasterThread;
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.InteropServices;

namespace TestApplication
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class Class1
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		unsafe static void Main(string[] args)
		{
			// execute the master thread
			MasterThread.MasterThread masterThread = new MasterThread.MasterThread();
			masterThread.MasterThreadStart();

		}
	}
}
