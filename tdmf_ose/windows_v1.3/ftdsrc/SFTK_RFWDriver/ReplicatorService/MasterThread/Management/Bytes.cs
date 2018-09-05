using System;
using System.Net;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text;


namespace MasterThread.Management
{
	/// <summary>
	/// Summary description for Bytes.
	/// </summary>
	[Serializable]
	public class Bytes
	{
		public Bytes()
		{
		}
		public int Size {
			get { return Marshal.SizeOf( this.GetType() ); }
		} 
		public byte [] ToBytes ( string msg ) {
			byte [] bytes = new byte[ Size + msg.Length + 1 ];
			this.ToBytes().CopyTo( bytes, 0 );
			ASCIIEncoding.UTF8.GetBytes( msg ).CopyTo( bytes, Size );
			bytes[ bytes.Length - 1 ] = 0;
			return bytes;
		}
		public byte [] ToBytes () {
			byte [] bytes = new byte[ Size ];

			Type thisType = this.GetType();
			MemberInfo[] mi = thisType.GetFields( BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance );

			// Serialize all of the allowed fields and remove the non persistent ones.
			int index = 0;
			foreach ( MemberInfo member in mi ) {
				if ((((FieldInfo)member).FieldType == typeof( short ) ) ||
					(((FieldInfo)member).FieldType == typeof( ushort ) )) {
					BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (short)((FieldInfo)member).GetValue( this ) ) ).CopyTo( bytes, index );
				} else if ((((FieldInfo)member).FieldType == typeof( int ) ) ||
					(((FieldInfo)member).FieldType == typeof( uint ) )) {
					BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)((FieldInfo)member).GetValue( this ) ) ).CopyTo( bytes, index );
				} else if ((((FieldInfo)member).FieldType == typeof( long ) ) ||
					(((FieldInfo)member).FieldType == typeof( ulong ) )) {
					BitConverter.GetBytes( IPAddress.HostToNetworkOrder( (int)((FieldInfo)member).GetValue( this ) ) ).CopyTo( bytes, index );
				} else if (((FieldInfo)member).FieldType == typeof( byte ) ) {
					bytes[ index ] = (byte)((FieldInfo)member).GetValue( this );
				} else if (((FieldInfo)member).FieldType == typeof( string ) ) {
					ASCIIEncoding.UTF8.GetBytes( (string)((FieldInfo)member).GetValue( this ) ).CopyTo( bytes, index ); 
				} else {
					Bytes b = (Bytes)((FieldInfo)member).GetValue( this );
					b.ToBytes().CopyTo( bytes, index );
				}
				index += Marshal.SizeOf( ((FieldInfo)member).FieldType );
			}
			return bytes;
		}
		public void FromBytes ( byte [] bytes ) {
			FromBytes ( bytes, 0 );
		}
		public void FromBytes ( byte [] bytes, int offset ) {
			if ( bytes.Length >= Marshal.SizeOf( this.GetType() ) ) {
				throw new OmException("Error: Invalid byte count in FromBytes" );
			}
			Type thisType = this.GetType();
			MemberInfo[] mi = thisType.GetFields( BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance );

			// Serialize all of the allowed fields and remove the non persistent ones.
			int index = offset;
			foreach ( MemberInfo member in mi ) {
				if ((((FieldInfo)member).FieldType == typeof( short ) ) ||
					(((FieldInfo)member).FieldType == typeof( ushort ) )) {
					((FieldInfo)member).SetValue( this, (short)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, index ) ));
				} else if ((((FieldInfo)member).FieldType == typeof( int ) ) ||
					(((FieldInfo)member).FieldType == typeof( uint ) )) {
					((FieldInfo)member).SetValue( this, (int)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, index ) ));
				} else if ((((FieldInfo)member).FieldType == typeof( long ) ) ||
					(((FieldInfo)member).FieldType == typeof( ulong ) )) {
					((FieldInfo)member).SetValue( this, (long)IPAddress.NetworkToHostOrder( BitConverter.ToInt32( bytes, index ) ));
				} else if (((FieldInfo)member).FieldType == typeof( byte ) ) {
					((FieldInfo)member).SetValue( this, bytes[ index ] );
				} else if (((FieldInfo)member).FieldType == typeof( string ) ) {
					((FieldInfo)member).SetValue( this, ASCIIEncoding.UTF8.GetString( bytes, index, Marshal.SizeOf( ((FieldInfo)member).FieldType ) ));
				} else {
					Bytes b = (Bytes)((FieldInfo)member).GetValue( this );
					b.FromBytes( bytes, index );
				}
				index += Marshal.SizeOf( ((FieldInfo)member).FieldType );
			}

		}		


	}
}
