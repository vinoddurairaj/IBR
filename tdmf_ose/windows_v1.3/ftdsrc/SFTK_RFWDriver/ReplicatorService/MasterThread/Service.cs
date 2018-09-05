using System;

namespace MasterThread
{
	/// <summary>
	/// Summary description for Service.
	/// </summary>
	public class Service
	{
		public Service()
		{
			//
			// TODO: Add constructor logic here
			//
		}


		public void StartService() {
			bool autostart = false;
			bool force = false;

			// get all primary configs 
			LgConfigCollection LgConfigs;

			// get paths of all groups 
			LgConfigs = Config.GetPrimaryConfigs( RegistryAccess.InstallPath() );

			// if none exist then exit  
			if ( LgConfigs.Count == 0 ) {
				OmException.LogException( new OmException( String.Format("Error in INITIAL STARTUP no logical group found." ) ) );
			}
			// TODO: Impliment
			//sftk_ioctl_config_begin();

			foreach ( LgConfig cfgp in LgConfigs ) {
				try {
					LogicalGroup.StartGroup( cfgp.lgnum, force, autostart );
				} catch ( OmException e ) {
					OmException.LogException( new OmException( String.Format("Error in INITIAL STARTUP calling start_group for lgnum- " + cfgp.lgnum ), e ) );
				}
			}
			// TODO: Impliment
			//sftk_ioctl_config_end();

		}




	}
}
