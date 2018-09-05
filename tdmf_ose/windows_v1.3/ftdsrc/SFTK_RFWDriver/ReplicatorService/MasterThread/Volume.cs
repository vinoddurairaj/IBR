using System;
using System.Text;

namespace MasterThread
{
	/// <summary>
	/// Summary description for Volume.
	/// </summary>
	public class Volume {
		public Volume() {
		}

		//		public void OpenAVolume( string DriveLetter, DWORD dwAccessFlags) {       
		//			HANDLE hVolume;
		//			char szVolumeName[8];       
		//
		//			sprintf(szVolumeName, TEXT("\\\\.\\%c:"), cDriveLetter[0]);
		//
		//			hVolume = CreateFile(   szVolumeName,
		//				dwAccessFlags,
		//				FILE_SHARE_READ | FILE_SHARE_WRITE,
		//				NULL,
		//				OPEN_EXISTING, 0,
		//				NULL );
		//
		//			return hVolume;   
		//		}
		//
		public static IntPtr OpenAVolumeMountPoint( string VolumeMntPt ) {       
			//
			// VolumeName format : \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
			// or           format : H:\MountPoint1\
			//
			IntPtr hVolume;
			string name;

			if ( !IS_VALID_VOLUME_NAME( VolumeMntPt ) ) {
				try {
					name = VolumeMountPoint.GetVolumeNameForVolMntPt( VolumeMntPt );
				} catch ( Exception ) {
					return VolumeMountPoint.INVALID_HANDLE_VALUE;
				}
			}
			else {
				name = VolumeMntPt;
			}
			hVolume = Createfile.OpenDrive( name.Substring( 0, name.Length - 1 ) );

			return hVolume;
		}

		public static bool IS_VALID_VOLUME_NAME( string s ) {
			
			return (( s.StartsWith( "\\\\?\\Volume{" )) &&
				( s[19] == '-' ) &&
				( s[24] == '-' ) &&
				( s[29] == '-' ) &&
				( s[34] == '-' ) &&
				( s[47] == '}' ) );
		}


	}
}
