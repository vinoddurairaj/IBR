using System;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Tcp;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;
using System.Xml.Schema;
using System.Reflection;


namespace MasterThread
{
	/// <summary>
	/// Summary description for ConsoleDefaultValues.
	/// </summary>
	/// 
	[Serializable]
	public class ConsoleDefaultValues
	{

		public int PortNumber;		// site wide port number to use
		public int MaxNetworkConsumption;	// maximum network consuption to use for all groups on each path

		public StringCollection SourceNetworkPaths = new StringCollection();
		public StringCollection TargetNetworkPaths = new StringCollection();
		public string MetaDataDirectoryPath;
		public string JournalDirectoryPath;
		public bool DisableJournals;


		public ConsoleDefaultValues()
		{
		}

		public void SaveXML() {
			XmlSerializer serializer = new XmlSerializer( typeof( ConsoleDefaultValues ) );
			string name = Path.Combine( RegistryAccess.InstallPath(), "DefaultValues.xml" );
			TextWriter writer = new StreamWriter( name );
			try {
				// Serialize this class and close the TextWriter.
				serializer.Serialize ( writer, this );
			} finally {
				writer.Close();
			}
		}

		public static ConsoleDefaultValues LoadXML() {
			string name = Path.Combine( RegistryAccess.InstallPath(), "DefaultValues.xml" );

			XmlSerializer serializer = new XmlSerializer( typeof( ConsoleDefaultValues ) );
			FileStream fs = new FileStream( name, FileMode.Open );
			try {
				/* Use the Deserialize method to restore the object's state with
				data from the XML document. */
				return (ConsoleDefaultValues) serializer.Deserialize( fs );
			} finally {
				fs.Close();
			}
		}
	}
}
