using System;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace MasterThread
{
	/// <summary>
	/// Summary description for CheckPoint.
	/// </summary>
	public class CheckPoint
	{

		// checkpoint script path prefixes
		const string PRE_CP_ON = "cp_pre_on_";
		const string PRE_CP_OFF = "cp_pre_off_";
		const string POST_CP_ON = "cp_post_on_";


		public CheckPoint()
		{
		}


//		/// <summary>
//		/// ftd_lg_cpstart -- start transition to checkpoint ON 
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <returns></returns>
//		public static int ftd_lg_cpstart( ftd_lg_t lgp ) {
//			header_t ack = new header_t();
//			string command;
//
//			if ( lgp.flags.GET_LG_CPON() ) {
//				OmException.LogException( new OmException( "ftd_lg_cpstart CheckPoint already On? Group " + lgp.lgnum ) );
//				if ( lgp.cfgp.role == eRole.Secondary ) {
//					// if connected send error message to partner
//					if ( lgp.dsockp.Connected ) {
//						Protocol.ftd_sock_send_err( lgp.dsockp, "ftd_lg_cpstart CheckPoint already On? Group " + lgp.lgnum);
//					}
//				}
//				throw new OmException( "ftd_lg_cpstart CheckPoint On again? Group " + lgp.lgnum );
//			}
//			try {
//				switch( lgp.cfgp.role ) {
//					case eRole.Primary:
//						// execute the pre cp on bat file
//						command = RegistryAccess.InstallPath() + "\\" + PRE_CP_ON + lgp.lgnum.ToString("D3") + ".bat";
//						// if it doesn't have any executable commands,
//						// don't try to run it
//						if ( existCommands( command ) ) {
//							Process process = new Process();
//							process.StartInfo.FileName = command;
//							process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
//							process.StartInfo.UseShellExecute = false;
//							process.Start();
//							// wait for completion
//							process.WaitForExit();
//
////							if ( LogicalGroup.ftd_lg_unlock_devs( lgp ) < 0 ) {
////								// tell secondary
////								Protocol.ftd_sock_send_err( lgp.dsockp, "Error: Unlock devs failed." );
////								try {
////									// send header to command
////									Protocol.SendCCPONERR( lgp.dsockp, ack );
////								}
////								catch( Exception e ) {
////									OmException.LogException( new OmException( String.Format("Error in Recv_Apply_Done_CpOn : {0}", e.Message ) ) );
////								}
////								throw new OmException ( "Unlock devs failed" );
////							} 
////							else {
//								// add sentinal to data stream
//								LogicalGroupIOCTL logicalGroupIOCTL = new LogicalGroupIOCTL( lgp.lgnum );
//								try {
//									logicalGroupIOCTL.ftd_ioctl_send_lg_message( LogicalGroupIOCTL.MSG_CPON );
//								} 
//								finally {
//									logicalGroupIOCTL.Close();
//								}
//								// execute the post cp on bat file
//								command = RegistryAccess.InstallPath() + "\\" + POST_CP_ON + lgp.lgnum.ToString("D3") + ".bat";
//								// if it doesn't have any executable commands,
//								// don't try to run it
//								if ( existCommands( command ) ) {
//									process = new Process();
//									process.StartInfo.FileName = command;
//									process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
//									process.StartInfo.UseShellExecute = false;
//									process.Start();
//									// wait for completion
//									process.WaitForExit();
//								}
//							}
////						} 
//						break;
//					case eRole.Secondary:
//						if ( lgp.dsockp.Connected ) {
//							// tell primary that we are going into checkpoint
//							try {
//								Protocol.SendACKCPSTART( lgp.dsockp, ack );
//							}
//							catch( Exception e ) {
//								throw e;
//								//OmException.LogException( new OmException( String.Format("Error in ftd_lg_cpstart : {0}", e.Message ) ) );
//								//return -1;
//							}
//						} else {
//							// mirror threads not running - do cppend shit
//							ftd_lg_cppend( lgp );
//						}
//						break;
//				}
//				if ( lgp.flags.GET_LG_CPSTART() == eLGCP.LG_CPSTARTP ) {	
//					// tell master 
//					try {
//						// send header to command
//						Protocol.SendCCPON( lgp.isockp, ack, lgp.fprocp.GetHashCode(), lgp.lgnum, lgp.cfgp.role );
//					}
//					catch( Exception e ) {
//						throw e;
//						//OmException.LogException( new OmException( String.Format("Error in ftd_lg_cpstart : {0}", e.Message ) ) );
//					}
//				}
//			} catch ( Exception e ) {
//				lgp.flags.UNSET_LG_CPSTART();
//				throw e;
//			}
//			return 0;
//		}
//
//		/// <summary>
//		/// ftd_lg_cpstop -- start transition to checkpoint OFF
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <returns></returns>
//		public static void ftd_lg_cpstop( ftd_lg_t lgp ) {
//			header_t ack = new header_t();
//			bool cperr = false;
//
//			try {
//				switch( lgp.cfgp.role ) {
//					case eRole.Primary:
//						if ( !lgp.flags.GET_LG_CPON() && ( lgp.flags.GET_LG_CPSTART() == 0 ) ) {
//							OmException.LogException( new OmException( "ftd_lg_cpstop CheckPoint Not On? Group " + lgp.lgnum ) );
//							cperr = true;
//						} else {
//							LogicalGroupIOCTL logicalGroupIOCTL = new LogicalGroupIOCTL( lgp.lgnum );
//							try {
//								// add sentinal to data stream
//								logicalGroupIOCTL.ftd_ioctl_send_lg_message( LogicalGroupIOCTL.MSG_CPOFF );
//							} 
//							finally {
//								logicalGroupIOCTL.Close();
//							}
//						}
//						if (lgp.flags.GET_LG_CPSTOP() == eLGCP.LG_CPSTOPP) {
//							// tell master to let command know 
//							try {
//								// send header to command
//								Protocol.SendCCPOFF( lgp.isockp, lgp.fprocp.GetHashCode(), lgp.lgnum, lgp.cfgp.role );
//							}
//							catch( Exception e ) {
//								OmException.LogException( new OmException( String.Format("Error in Recv_Apply_Done_CpOn : {0}", e.Message ) ) );
//							}
//						}
//						lgp.flags.UNSET_LG_CPSTOP();
//						break;
//					case eRole.Secondary:
//						if ( !lgp.flags.GET_LG_CPPEND() && !lgp.flags.GET_LG_CPON() ) {
//							if ( lgp.dsockp.Connected ) {
//								OmException e = new OmException( String.Format("Warning: ftd_lg_cpstop Check Point not on" ) );
//								OmException.LogException( e );
//								Protocol.ftd_sock_send_err( lgp.dsockp, e.Message );
//							}
//							cperr = true;
//						} else {
//							if ( lgp.dsockp.Connected ) {
//								// tell primary that we are going out of checkpoint
//								try {
//									// send header to command
//									Protocol.SendACKCPSTOP( lgp.dsockp, ack );
//								}
//								catch( Exception e ) {
//									//OmException.LogException( new OmException( String.Format("Error in Recv_Apply_Done_CpOn : {0}", e.Message ) ) );
//									throw e;
//								}
//							} else {
//								// mirror daemons not running - do cpoff shit
//								ftd_lg_cpoff( lgp );
//							}
//						}
//						if ( lgp.flags.GET_LG_CPSTOP() == eLGCP.LG_CPSTOPS ) {	
//							// tell master to let command know 
//							try {
//								// send header to command
//								Protocol.SendCCPOFF( lgp.isockp, lgp.fprocp.GetHashCode(), lgp.lgnum, lgp.cfgp.role );
//							}
//							catch( Exception e ) {
//								OmException.LogException( new OmException( String.Format("Error in Recv_Apply_Done_CpOn : {0}", e.Message ) ) );
//							}
//						}
//						if ( cperr ) {
//							lgp.flags.UNSET_LG_CPSTOP();
//						}
//						break;
//				}
//			}
//			catch ( Exception e ) {
//				throw e;
//			}
//		}
//
//
//		/// <summary>
//		/// ftd_lg_cpoff -- secondary system - take group out of checkpoint
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <returns></returns>
//		public static int ftd_lg_cpoff( ftd_lg_t lgp ) {
//			header_t ack = new header_t();
//			string command;
//			LG_Flags save_flags;
//
//			save_flags = lgp.flags;
//
//			try {
//				switch( lgp.cfgp.role ) {
//					case eRole.Primary:
//						// set checkpoint off in pstore
//						DriverIOCTL driver = new DriverIOCTL();
//						try {
//							driver.ps_set_group_checkpoint( lgp.cfgp.pstore, lgp.devname, false );
//						}
//						finally {
//							driver.Close();
//						}
//						lgp.flags.UNSET_LG_CPON();
//						lgp.flags.UNSET_LG_CPSTART();
//
//						OmException e = new OmException( String.Format("Info: ftd_lg_cpoff Primary Group- " + lgp.lgnum ) );
//						OmException.LogException( e );
//
//						Management.ftd_mngt_performance_set_group_cp( (short)lgp.lgnum, false );
//						break;
//					case eRole.Secondary:
//						command = RegistryAccess.InstallPath() + "\\" + PRE_CP_OFF + lgp.lgnum.ToString("D3") + ".bat";
//						// if it doesn't have any executable commands,
//						// don't try to run it
//						if ( existCommands( command ) ) {
//							Process process = new Process();
//							process.StartInfo.FileName = command;
//							process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
//							process.StartInfo.UseShellExecute = false;
//							process.Start();
//							// wait for completion
//							process.WaitForExit();
//
//							lgp.flags.UNSET_LG_CPON();
//							lgp.flags.UNSET_LG_CPSTART();
//							lgp.flags.UNSET_LG_CPPEND();
//
//							Journal.ftd_journal_get_all_files( lgp.jrnp );
//							Journal.ftd_journal_delete_cp(lgp.jrnp);
//		
//							if ( lgp.fprocp.GetType() == typeof( RMDThread ) ) {
//								// if we are the rmd - do this
//								Journal.ftd_lg_stop_journaling( lgp, eJournalFlags.FTDJRNCO, eJournalFlags.FTDJRNMIR );
//							} else {
//								try {
//									Protocol.SendStartApply( lgp.isockp, lgp.lgnum, 1 );
//								} catch ( Exception e1 ) {
//									OmException e2 = new OmException( String.Format("Error in RMD SendStartApply 1 : {0}", e1.Message ) );
//									OmException.LogException( e2 );
//									throw e2;
//								}
//							}
//							// out of checkpoint mode 
//							if ( lgp.dsockp.Connected ) {
//								// ack the pmd
//								try {
//									Protocol.SendACKCPOFF( lgp.dsockp, ack );
//								} catch ( Exception e1 ) {
//									OmException e2 = new OmException( String.Format("Error in RMD SendStartApply 1 : {0}", e1.Message ) );
//									OmException.LogException( e2 );
//									throw e2;
//								}
//							}
//						}
//						break;
//				}
//				lgp.flags.UNSET_LG_CPSTOP();
//			} catch ( Exception e ) {
//				string role = (lgp.cfgp.role == eRole.Primary) ? "Primary" : "Secondary";
//				OmException e1 = new OmException( "Warning: error detected in ftd_lg_cpoff " + role + " for group " + lgp.lgnum, e );
//				OmException.LogException( e1 );
//
//				if ( lgp.dsockp.Connected ) {
//					Protocol.ftd_sock_send_err( lgp.dsockp, e1.Message );
//				}
//				lgp.flags = save_flags;
//				lgp.flags.UNSET_LG_CPSTOP();
//				return -1;
//			}
//			return 0;
//		}
//
//
//		/// <summary>
//		/// ftd_lg_cppend -- cp mode on secondary pending apply of journaled data
//		/// </summary>
//		/// <param name="lgp"></param>
//		/// <returns></returns>
//		public static int ftd_lg_cppend( ftd_lg_t lgp ) {
//			ftd_journal_file_t	jrnfp = new ftd_journal_file_t();
//			int					rc;
//
//			// close mirror devices
//			// TODO: may need this?
//			//ftd_lg_close_devs(lgp);
//
//			Journal.ftd_journal_get_all_files( lgp.jrnp );
//
//			// create a checkpoint file 
//			if ((rc = Journal.ftd_journal_create_next_cp( lgp.jrnp, jrnfp )) < 0) {
//				return -1;
//			} else if ( rc == 0 ) {
//
//				// close current, opened journal file 
//				Journal.ftd_journal_file_close( lgp.jrnp.cur );
//
//				// create a new journal file to write to during apply 
//				jrnfp = Journal.ftd_journal_create_next( lgp.jrnp, eJournalFlags.FTDJRNCO );
//
//				try {
//					Protocol.SendStartApply( lgp.isockp, lgp.lgnum, 1 );
//				} catch ( Exception e ) {
//					OmException.LogException( new OmException( String.Format("Error in ftd_lg_cppend SendStartApply 1 : {0}", e.Message ) ) );
//					return -1;
//				}
//				//sprintf(groupstr, "Group: %03d Secondary", lgp.lgnum);
//				//reporterr(ERRFAC, M_CPPEND, ERRINFO, groupstr);
//			}
//
//			lgp.flags.SET_LG_CPPEND();
//			lgp.flags.UNSET_LG_CPSTART();
//			lgp.flags.UNSET_LG_CPON();
//			lgp.jrnp.flags.SET_JRN_MODE( eJournalFlags.FTDJRNMIR );
//	
//			return 0;
//		}
//
//
		/// <summary>
		/// go into cp mode for this group
		/// </summary>
		/// <param name="lgp"></param>
		/// <returns></returns>
		public static int LogicalGroupCpOn( LogicalGroup lgp, ProtocolHeader header ) {
			string command;

			switch ( lgp.cfgp.role ) {
				case eRole.Primary:
					// set checkpoint on in pstore
					DriverIOCTL driver = new DriverIOCTL();
					try {
						driver.ps_set_group_checkpoint( lgp.lgnum, true );
					} catch ( Exception e ) {
						OmException.LogException( new OmException( String.Format("Error in Recv_Refreshs_Relaunch : {0}", e.Message ) ) );
						return -1;
					}
					finally {
						driver.Close();
					}
					Management.Management.Mngt_Performance_Set_Group_Cp(( short)lgp.lgnum, 1 );
					break;
				case eRole.Secondary:
					try {
						//TODO: fix this the dsock is not valid
						Protocol.SendACKCPON ( lgp.dsockp, header );
					} catch ( Exception e ) {
						OmException.LogException( new OmException( String.Format("Error in Recv_Refreshs_Relaunch : {0}", e.Message ) ) );
						return -1;
					}

					command = RegistryAccess.InstallPath() + "\\" + POST_CP_ON + lgp.lgnum.ToString("D3") + ".bat";
					// if it doesn't have any executable commands, don't try to run it
					if ( existCommands( command ) ) {
						Process process = new Process();
						process.StartInfo.FileName = command;
						process.StartInfo.WindowStyle = ProcessWindowStyle.Hidden | ProcessWindowStyle.Minimized;
						process.StartInfo.UseShellExecute = false;
						process.Start();
						// wait for completion
						process.WaitForExit();
					}
					break;
			}
			return 0;
		}


		// DTurrin - Oct 17th, 2001
		//
		// This procedure checks if a .bat file exists. If it exists,
		// a check is made to see if any commands will be executed by
		// running the .bat file. If commands can be executed, then
		// the procedure returns TRUE.
		static bool existCommands ( string fileName ) {
			StreamReader file = null;
			string line;

			if ( Directory.Exists( fileName ) ) {
				try {
					file = new StreamReader( fileName );

					while ( ( line = file.ReadLine() ) != null ) {
						line.Trim();
						if ( line.StartsWith( "REM" ) ) {
							continue;
						}
						// If the line length is greater than 2
						// then we assume that it is a command line. So return TRUE.
						if ( line.Length > 2 ) {
							return true;
						}
					}
                } catch ( Exception ) {

				} finally {
					if ( file != null ) {
						file.Close();
					}
				}
			}
			return false;
		}





	}
}
