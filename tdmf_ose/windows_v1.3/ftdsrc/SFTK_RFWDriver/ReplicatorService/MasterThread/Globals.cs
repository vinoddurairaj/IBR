using System;

namespace MasterThread {

	[Serializable]
	public enum eMagic {
		PERFMAGIC = unchecked ((int)0xBADF00D6),
		Socket = unchecked ((int)0xBADF00D3),
		MSG_MAGICNUMBER = unchecked ((int)0xe8ad4239),
		MAGICACK = unchecked ((int)0xbabeface),
		MAGICERR = unchecked ((int)0xdeadbabe),
		MAGICHDR = unchecked ((int)0xcafeface),
	}

	[Serializable]
	public enum eExecuteType {
		ACTIVATEGROUP,
		TRANSITIONTONORMAL,
		STATE,
		COMMIT
	}

	[Serializable]
	public struct CLIData {
		public bool All;
		public bool Force;
		public int lgnum;
		public eExecuteType Type;
		public eLgModes State;
		public string HostID;
		public string CommandName;
	}

	/// <summary>
	/// Summary description for Globals.
	/// </summary>
	public class Globals
	{

		public const int MAX_GRP_NUM = 100;
		public const int MAX_DEVICE_NUM = 100;
		public const int LGFLAG = 0x00100000;
		public const string qdsreleasenumber = "Version 2.2";

		public const int _MAX_PATH = 260;


		public Globals()
		{
		}
	}
}
