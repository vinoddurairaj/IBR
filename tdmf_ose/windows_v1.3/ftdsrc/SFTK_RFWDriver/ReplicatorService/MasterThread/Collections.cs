using System;
using System.Collections;
using System.Net.Sockets;
using System.Threading;


namespace MasterThread
{
	/// <summary>
	/// Summary description for Collections.
	/// </summary>
	public class Collections
	{
		public Collections()
		{
		}
	}

	[Serializable]
	public class RMDThreadCollection : CollectionBase {

		public int Add ( RMDThread x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( RMDThread x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( RMDThread x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( RMDThread x ) {
			InnerList.Remove(x);
		}
		
		public RMDThread this[int index] {
			get { return (RMDThread) InnerList[index]; }
		}
	}

//	[Serializable]
//	public class PMDThreadCollection : CollectionBase {
//
//		public int Add ( PMDThread x ) {
//			if ( Contains(x) ) return -1;
//			int index = InnerList.Add(x);
//			return index;
//		}
//
//		public bool Contains ( PMDThread x ) {
//			return InnerList.Contains(x);
//		}
//	
//		public int IndexOf ( PMDThread x ) {
//			return InnerList.IndexOf ( x );
//		}
//	
//		public void Remove ( PMDThread x ) {
//			InnerList.Remove(x);
//		}
//		
//		public PMDThread this[int index] {
//			get { return (PMDThread) InnerList[index]; }
//		}
//	}
	
	[Serializable]
	public class XMDThreadCollection : CollectionBase {

		public int Add ( XMDThread x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( XMDThread x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( XMDThread x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( XMDThread x ) {
			InnerList.Remove(x);
		}
		
		public XMDThread this[int index] {
			get { return (XMDThread) InnerList[index]; }
		}
	}
	
	
	[Serializable]
	public class ThreadCollection : CollectionBase {

		public int Add ( Thread x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( Thread x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( Thread x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( Thread x ) {
			InnerList.Remove(x);
		}
		
		public Thread this[int index] {
			get { return (Thread) InnerList[index]; }
		}
	}



	[Serializable]
	public class SocketCollection : CollectionBase {

		public int Add ( Socket x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( Socket x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( Socket x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( Socket x ) {
			InnerList.Remove(x);
		}
		
		public Socket this[int index] {
			get { return (Socket) InnerList[index]; }
		}

		public SocketCollection Clone() {
			SocketCollection clone = new SocketCollection();
			foreach ( Socket socket in this.List ) {
				clone.Add( socket );
			}
			return clone;
		}
	}

	/// By Mike Pollett 08/09/02
	/// <summary>
	/// Holds a collection OmExceptions
	/// </summary>
	[Serializable]
	public class OmExceptionCollection : CollectionBase {
		public int Add ( OmException x ) {
			lock( this.InnerList.SyncRoot ) {
				if ( x == null ) {
					return -1;
				}
				int index = InnerList.Add(x);
				return index;
			}
		}

		public void AddRange ( OmExceptionCollection x ) {
			lock( this.InnerList.SyncRoot ) {
				InnerList.AddRange(x);
			}
		}

		public bool Contains ( OmException x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( OmException x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( OmException x ) {
			InnerList.Remove(x);
		}
		
		public OmException this[int index] {
			get { return (OmException) InnerList[index]; }
		}
	}

	

	[Serializable]
	public class DevConfigCollection : CollectionBase {

		public int Add ( DevConfig x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( DevConfig x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( DevConfig x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( DevConfig x ) {
			InnerList.Remove(x);
		}
		
		public DevConfig this[int index] {
			get { return (DevConfig) InnerList[index]; }
		}

		public DevConfigCollection Clone() {
			DevConfigCollection collection = new DevConfigCollection();
			foreach ( DevConfig dev in InnerList ) {
				collection.Add( dev.Clone() );
			}
			return collection;
		}
	}

	[Serializable]
	public class ConfigInfoCollection : CollectionBase {

		public int Add ( ConfigInfo x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( ConfigInfo x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( ConfigInfo x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( ConfigInfo x ) {
			InnerList.Remove(x);
		}
		
		public ConfigInfo this[int index] {
			get { return (ConfigInfo) InnerList[index]; }
		}
	}


	[Serializable]
	public class LgConfigCollection : CollectionBase {

		public int Add ( LgConfig x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( LgConfig x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( LgConfig x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( LgConfig x ) {
			InnerList.Remove(x);
		}
		
		public LgConfig this[int index] {
			get { return (LgConfig) InnerList[index]; }
		}

		public LgConfigCollection Clone() {
			LgConfigCollection collection = new LgConfigCollection();
			foreach ( LgConfig lgconfig in InnerList ) {
				collection.Add( lgconfig.Clone() );
			}
			return collection;
		}
		/// <summary>
		/// Find config using lgnum
		/// </summary>
		/// <param name="lgnum"></param>
		/// <returns></returns>
		public LgConfig Find ( int lgnum ) {
			foreach( LgConfig config in this.InnerList ) {
				if ( config.lgnum == lgnum ) {
					return config;
				}
			}
			throw new OmException( "Error: LgConfigCollection - Find - did not find - " + lgnum );
		}
		/// <summary>
		/// Find config using User Friendly Name
		/// </summary>
		/// <param name="friendly"></param>
		/// <returns></returns>
		public LgConfig Find ( string friendly ) {
			foreach( LgConfig config in this.InnerList ) {
				if ( config.UserFriendlyName == friendly ) {
					return config;
				}
			}
			throw new OmException( "Error: LgConfigCollection - Find - did not find friendly name - " + friendly );
		}
	}

	[Serializable]
	public class MasterListenerCollection : CollectionBase {

		public int Add ( MasterListener x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( MasterListener x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( MasterListener x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( MasterListener x ) {
			InnerList.Remove(x);
		}
		
		public MasterListener this[int index] {
			get { return (MasterListener) InnerList[index]; }
		}
	}



//	[Serializable]
//	public class ThrottleCollection : CollectionBase {
//
//		public int Add ( Throttle x ) {
//			if ( Contains(x) ) return -1;
//			int index = InnerList.Add(x);
//			return index;
//		}
//
//		public bool Contains ( Throttle x ) {
//			return InnerList.Contains(x);
//		}
//	
//		public int IndexOf ( Throttle x ) {
//			return InnerList.IndexOf ( x );
//		}
//	
//		public void Remove ( Throttle x ) {
//			InnerList.Remove(x);
//		}
//		
//		public Throttle this[int index] {
//			get { return (Throttle) InnerList[index]; }
//		}
//	}


//	[Serializable]
//	public class DeviceCollection : CollectionBase {
//
//		public int Add ( Device x ) {
//			if ( Contains(x) ) return -1;
//			int index = InnerList.Add(x);
//			return index;
//		}
//
//		public bool Contains ( Device x ) {
//			return InnerList.Contains(x);
//		}
//	
//		public int IndexOf ( Device x ) {
//			return InnerList.IndexOf ( x );
//		}
//	
//		public void Remove ( Device x ) {
//			InnerList.Remove(x);
//		}
//		
//		public Device this[int index] {
//			get { return (Device) InnerList[index]; }
//		}
//	}
//	[Serializable]
//	public class ftd_journal_file_tCollection : CollectionBase {
//
//		public int Add ( ftd_journal_file_t x ) {
//			if ( Contains(x) ) return -1;
//			int index = InnerList.Add(x);
//			return index;
//		}
//
//		public bool Contains ( ftd_journal_file_t x ) {
//			return InnerList.Contains(x);
//		}
//	
//		public int IndexOf ( ftd_journal_file_t x ) {
//			return InnerList.IndexOf ( x );
//		}
//	
//		public void Remove ( ftd_journal_file_t x ) {
//			InnerList.Remove(x);
//		}
//		
//		public ftd_journal_file_t this[int index] {
//			get { return (ftd_journal_file_t) InnerList[index]; }
//		}
//		public void Sort () {
//			InnerList.Sort();
//		}
//	}

	[Serializable]
	public class StringCollection : CollectionBase {

		public int Add ( String x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( String x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( String x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( String x ) {
			InnerList.Remove(x);
		}
		
		public String this[int index] {
			get { return (String) InnerList[index]; }
		}
	}

	[Serializable]
	public class ConsoleDeviceStatCollection : CollectionBase {

		public int Add ( ConsoleDeviceStat x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( ConsoleDeviceStat x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( ConsoleDeviceStat x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( ConsoleDeviceStat x ) {
			InnerList.Remove(x);
		}
		
		public ConsoleDeviceStat this[int index] {
			get { return (ConsoleDeviceStat) InnerList[index]; }
		}
	}

	[Serializable]
	public class ConsoleLgStatCollection : CollectionBase {

		public int Add ( ConsoleLgStat x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( ConsoleLgStat x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( ConsoleLgStat x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( ConsoleLgStat x ) {
			InnerList.Remove(x);
		}
		
		public ConsoleLgStat this[int index] {
			get { return (ConsoleLgStat) InnerList[index]; }
		}
	}


}
