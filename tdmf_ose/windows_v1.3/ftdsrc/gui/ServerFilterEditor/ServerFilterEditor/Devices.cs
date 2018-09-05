using System;

namespace ServerFilterEditor
{
	/// <summary>
	/// Summary description for Devices.
	/// </summary>
	/// 
	public class Device
	{
		private string m_strName = "";
		private bool   m_bSelected = false;

		public Device()
		{
		}

		public Device(string Name)
		{
			m_strName = Name;
			m_bSelected = false;
		}

		public string Name
		{
			get
			{
				return m_strName;
			}
			set
			{
				m_strName = value;
			}
		}

		public bool Selected
		{
			get
			{
				return m_bSelected;
			}
			set
			{
				m_bSelected = value;
			}
		}
	}

}