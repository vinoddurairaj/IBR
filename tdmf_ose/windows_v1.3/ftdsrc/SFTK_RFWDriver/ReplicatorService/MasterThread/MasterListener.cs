using System;
using System.Net;
using System.Net.Sockets;

namespace MasterThread
{
	/// <summary>
	/// Summary description for MasterListener.
	/// This is the Listener socket that the masterThread will use
	/// Exposed the Socket object 
	/// </summary>
	public class MasterListener : TcpListener
	{

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="localaddr"></param>
		/// <param name="port"></param>
		public MasterListener( IPAddress localaddr, int port ) : base ( localaddr, port )
		{
		}

		/// <summary>
		/// Expose the Socket object
		/// </summary>
		public Socket GetSocket {
			get { return base.Server; }
		}
	}
}
