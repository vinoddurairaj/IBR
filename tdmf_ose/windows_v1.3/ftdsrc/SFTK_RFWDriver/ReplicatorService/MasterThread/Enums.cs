//
// Created by Mike Pollett
// June 2004
//

using System;

namespace MasterThread
{
	/// <summary>
	/// List of Enum(s).
	/// </summary>
	
	/// <summary>
	/// connection type
	/// </summary>
	[Serializable]
	public enum eConnectionTypes
	{
		CON_INVALID = -1,
		CON_UTIL = 0,
		CON_CLI,		//CON_IPC
		CON_PMD,
		CON_CHILD
	}

	[Serializable]
	public enum eDynamicDiskType {
		Unknown = 0,
		Simple,
		Stripe,
		Mirror,
		Spanned,
		Raid		//Added by Veera 04-21-2003
	}

	// Size constants
	public enum eSize {
		KILOBYTE = 1024,
		MEGABYTE = 1024 * 1024,
		GIGABYTE = 1024 * 1024 * 1024
	}




}