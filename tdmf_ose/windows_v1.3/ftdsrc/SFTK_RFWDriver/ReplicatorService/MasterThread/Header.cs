using System;
using System.Text;
using System.Runtime.Serialization;
using System.Runtime.InteropServices;

namespace MasterThread {
	/// <summary>
	/// Summary description for Header.
	/// </summary>

	public enum eProtocol {
		Invalid = 0,
		CHANDSHAKE =1,
		CCHKCONFIG,
		CNOOP,
		CVERSION,
		CCHUNK,
		CHUP,
		CCPONERR,	   
		CCPOFFERR,	   
		CEXIT,		   
		CBFBLK,		   
		CBFSTART,	   
		CBFREAD,		   
		CBFEND,	   
		CCHKSUM,		   
		CRSYNCDEVS,	   
		CRSYNCDEVE,	   
		CRFBLK,		   
		CRFEND,		   
		CRFFEND,		   
		CKILL,
		CRFSTART,	   
		CRFFSTART,	   
		CCPSTARTP,	   
		CCPSTARTS,	   
		CCPSTOPP,	   
		CCPSTOPS,	   
		CCPON,		   
		CCPOFF,		   
		CSTARTAPPLY,	   
		CSTOPAPPLY,	   
		CSTARTPMD,
		CAPPLYDONECPON,
		CREFOFLOW,
		CSIGNALPMD,
		CSIGNALRMD,
		CSTARTRECO,

		/* acks */
		ACKERR ,   
		ACKRSYNC,
		ACKCHKSUM,	   
		ACKCHUNK,	   
		ACKHUP,		   
		ACKCPSTART,	   
		ACKCPSTOP,	   
		ACKCPON,		   
		ACKCPOFF,	   
		ACKCPOFFERR,	   
		ACKCPONERR,	   
		ACKNOOP,		   
		ACKCONFIG, 	   
		ACKKILL ,  
		ACKHANDSHAKE,   
		ACKVERSION,	   
		ACKCLI,		   
		ACKRFSTART,	   
		ACKNOPMD,	   
		ACKNORMD,	   
		ACKDOCPON,	   
		ACKDOCPOFF,	   
		ACKCLD,		   
		ACKCPERR,	   

		/* signals */
		CSIGUSR1,
		CSIGTERM,	   
		CSIGPIPE,		   

		/* management msg type */
		CMANAGEMENT,

		/* new debug level msg */
		CSETTRACELEVEL,
		CSETTRACELEVELACK,

		/* REMOTE WAKEUP msg */
		CREMWAKEUP,

		/* REMOTE DRIVE ERROR */
		CREMDRIVEERR,

		/* SMART REFRESH RELAUNCH FROM RMD,RMDA */
		CREFRSRELAUNCH
	};



	/// <summary>
	/// ftd_err_t - statndard error packet structure
	/// </summary>
	[Serializable]
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
	public struct Error_t {
		public uint errcode;					/* error severity code                 */
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=12)]
		public string errkey;					/* error mnemonic key                  */
		public int length;						/* length of error message string      */
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=512)]
		public string msg;						/* error message describing error      */
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
		public string fnm;						/* source file name					*/
		public int lnum;						/* line number						*/
	}

	/// <summary>
	/// ftd_err_u - this is a union of err_t and byte []
	/// </summary>
//	[Serializable]
//	[StructLayout(LayoutKind.Explicit, CharSet=CharSet.Ansi)]
//	public unsafe struct Error_u {
//		[FieldOffset(0)]
//		public Error_t error;
//		[FieldOffset(0)]
//		[MarshalAs(UnmanagedType.ByValArray, SizeConst=796)]
//		public byte[] byteArray;
//	}

	// logical group HEADER message structure
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
	public struct LogicalGroupHeader {
		public Int32 lgnum;						/* lgnum              */
		public Int32 devid;						/* device id              */
		public Int32 bsize;						/* device sector size (bytes) */
		public Int32 offset;					/* device i/o offset (sectors) */
		public Int32 len;						/* device i/o len (sectors)    */
		public int data;						/* store miscellaneous info */
		public int flag;						/* store miscellaneous info */
		public Int32 reserved;					/* not used */

		public string DataString{
			get {
				string str = "";
				byte c = Convert.ToByte(( data >> 0 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				c = Convert.ToByte(( data >> 8 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				c = Convert.ToByte(( data >> 16 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				c = Convert.ToByte(( data >> 24 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );

				c = Convert.ToByte(( flag >> 0 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				c = Convert.ToByte(( flag >> 8 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				c = Convert.ToByte(( flag >> 16 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				c = Convert.ToByte(( flag >> 24 ) & 0xff );
				if ( c == 0 ) return str;
				str += Convert.ToChar( c );
				return str;
			}
			set {
				data = 0;
				flag = 0;
				for ( int index = 0 ; index < 4 ; index++ ) {
					char c = value[ index ];
					if ( c == '\0' ) return;
					data += Convert.ToByte( c ) << (8 * index);
				}
				for ( int index = 4 ; index < 8 ; index++ ) {
					char c = value[ index ];
					if ( c == '\0' ) return;
					flag += Convert.ToByte( c ) << (8 * (index-4));
				}
			}
		}
	}

	// standard message header structure
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
	public class ProtocolHeader {
		public eMagic magicvalue;				/* indicate that this is a header packet */
		public int ts;						/* timestamp transaction was sent        */
		public eProtocol msgtype;				/* packet type                           */
		public Int32 cli;						/* packet is from cli				     */
		public Int32 compress;					/* compression algorithm employed        */
		public Int32 len;						/* data length that follows              */
		public Int32 uncomplen;					/* uncompressed data length              */
		public Int32 ackwanted;					/* 0 = No ACK                            */
		public LogicalGroupHeader msg;			/* logical group header msg          */
	}
//	[Serializable]
//	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
//	public struct header_string {
//		[MarshalAs(UnmanagedType.ByValTStr, SizeConst=64)]
//		public string buffer;
//	}
	[Serializable]
	[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
	public class header_byte {
		[MarshalAs(UnmanagedType.ByValArray, SizeConst=64)]
		public byte [] byteArray = new byte[64];

		public override string ToString() {
			return ToString( byteArray.Length );
		}
		public string ToString( int max ) {
			StringBuilder str = new StringBuilder( byteArray.Length );
			for ( int index = 0 ; index < max ; index++ ) {
				str.Append( Convert.ToChar( byteArray[index] ) );
				if ( byteArray[ index ] == 0 ) {
					break;
				}
			}
			return str.ToString();
		}
	}
}
