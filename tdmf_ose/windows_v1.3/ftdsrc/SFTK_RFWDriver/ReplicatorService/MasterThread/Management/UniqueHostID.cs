using System;

namespace MasterThread.Management
{
	/// <summary>
	/// Summary description for UniqueHostID.
	/// </summary>
	public class UniqueHostID
	{
		public static ulong ID;

		/// <summary>
		/// Constructor
		/// </summary>
		public UniqueHostID()
		{
		}

		/// <summary>
		/// ftd_mngt_getServerId
		/// </summary>
		/// <returns></returns>
		public static string ToStringID() {
			return ID.ToString();
		}

	}
}
