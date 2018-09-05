using System;
using System.Collections;


namespace MasterThread.Management
{

	[Serializable]
	public class StatusMsgCollection : CollectionBase {

		public int Add ( StatusMessage x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( StatusMessage x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( StatusMessage x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( StatusMessage x ) {
			InnerList.Remove(x);
		}
		
		public StatusMessage this[int index] {
			get { return (StatusMessage) InnerList[index]; }
		}
	}

	[Serializable]
	public class RepGrpStateCollection : CollectionBase {

		public int Add ( RepGrpState x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( RepGrpState x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( RepGrpState x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( RepGrpState x ) {
			InnerList.Remove(x);
		}
		
		public RepGrpState this[int index] {
			get { return (RepGrpState) InnerList[index]; }
		}
	}

	[Serializable]
	public class LgStatisticsCollection : CollectionBase {

		public int Add ( DriverIOCTL.LG_STATISTICS x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( DriverIOCTL.LG_STATISTICS x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( DriverIOCTL.LG_STATISTICS x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( DriverIOCTL.LG_STATISTICS x ) {
			InnerList.Remove(x);
		}
		
		public DriverIOCTL.LG_STATISTICS this[int index] {
			get { return (DriverIOCTL.LG_STATISTICS) InnerList[index]; }
		}
	}

	[Serializable]
	public class DeviceStatisticsCollection : CollectionBase {

		public int Add ( DriverIOCTL.DEV_STATISTICS x ) {
			if ( Contains(x) ) return -1;
			int index = InnerList.Add(x);
			return index;
		}

		public bool Contains ( DriverIOCTL.DEV_STATISTICS x ) {
			return InnerList.Contains(x);
		}
	
		public int IndexOf ( DriverIOCTL.DEV_STATISTICS x ) {
			return InnerList.IndexOf ( x );
		}
	
		public void Remove ( DriverIOCTL.DEV_STATISTICS x ) {
			InnerList.Remove(x);
		}
		
		public DriverIOCTL.DEV_STATISTICS this[int index] {
			get { return (DriverIOCTL.DEV_STATISTICS) InnerList[index]; }
		}
	}
	

}
