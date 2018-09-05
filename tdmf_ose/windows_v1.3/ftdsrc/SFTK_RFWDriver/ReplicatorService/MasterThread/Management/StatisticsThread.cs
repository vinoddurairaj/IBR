using System;
using System.Threading;

namespace MasterThread.Management
{
	/// <summary>
	/// Summary description for StatisticsThread.
	/// </summary>
	public class StatisticsThread : XMDThread {
		public StatisticsThread() {
		}

		/*
		 * StatThread - primary statistics thread
		 */
		public int StatThread( Args_t args ) {
//			Performance.ftd_perf_t perfp;
//			bool bMutex;
//			AutoResetEvent [] AutoEvents = new AutoResetEvent[2];
//			IntPtr lgfd;
//
//			string Event;
//
//			int	status;
//			DriverIOCTL.ALL_LG_STATISTICS	pAllStats = new DriverIOCTL.ALL_LG_STATISTICS();
//			DriverIOCTL.LG_STATISTICS		pLgStats;
//			DriverIOCTL.DEV_STATISTICS		pDevStats;
//
//			try {
//				proc_t procp = args.procp;
//
//				//ftd_init_errfac( "Replicator", "statd", NULL, NULL, 1, 0) == NULL);
//
//				DriverIOCTL driver = new DriverIOCTL();
//				try {
//
//					perfp = new Performance.ftd_perf_t();
//
//					Performance.ftd_perf_init( perfp, false );
//
//					PerformanceStats.ftd_mngt_performance_init();
//					try {
//
//						AutoEvents[0] = perfp.hGetEvent;  //signaled by a client app requests stats data
//						//			AutoEvents[1] = procp.hEvent;     //signaled when request to quit this thread
//#if USE_COLLECTOR
//						AutoEvents[1] = PerformanceStats.ftd_mngt_performance_get_force_acq_event();
//#endif
//
//						// run forever dump stats
//						while ( true ) {  
//							//Statistics_t lgtemp;
//							TimeSpan timeout;
//#if USE_COLLECTOR
//							// wake up PERIODICALLY to acquire data
//							// timeout = ftd_mngt_get_perf_upload_period()*100;//upload period specified in 0.100 of secs.
//							timeout = new TimeSpan( 0, 0, PerformanceStats.ftd_mngt_performance_get_wakeup_period() );
//#else
//							// wake up on request to acquire data
//							timeout = new TimeSpan( -1 );
//#endif
//							int index = WaitHandle.WaitAny( AutoEvents, timeout, false );
//							//	if (( index == WaitHandle.WaitTimeout ) ||
//
//							// Display ALL LG Info use Ioctl FTD_GET_ALL_STATS_INFO
//							int totalDev = 0;
//							int totalLG = 0;
//
//							// Get Total number LG exist in Driver 
//							totalLG = driver.Sftk_Get_TotalLgCount();
//
//							// Get Total number Devs exist for all LG in Driver 
//							totalDev = driver.Sftk_Get_TotalDevCount();
//
//							pAllStats.Size.NumOfLgEntries  = totalLG;
//							pAllStats.Size.NumOfDevEntries = totalDev;
//
//							// Get All Stat Information
//							pAllStats = driver.Sftk_Get_AllStatsInfo();
//
//							for( int count = 0 ; count < totalDev ; count++ ) {
//								string eventName = EventName( count );
//								lgfd = OpenEvent( EVENT_ALL_ACCESS, false, eventName );
//								if ( lgfd != null ) {
//									SetEvent( lgfd );
//									CloseHandle( lgfd );
//								}
//							}
//
//							// lock memory block
//							bMutex = perfp.hMutex.WaitOne ( Performance.SHARED_MEMORY_MUTEX_TIMEOUT, true );
//
//							//Protocol.ftd_perf_instance_t [] pData;
//							IntPtr MapFileViewData;
//							pData = perfp.MapFileViewData;
//
//							int indexx = 0;
//							foreach ( DriverIOCTL.LG_STATISTICS_ lgStatistic in pAllStats.LgStats ) {
//								Protocol.ftd_perf_instance_t lgdata;
//								lgdata = pData[ indexx++ ];
//								lgdata.lgnum = lgStatistic.LgNum;
//								lgdata.role = Convert.ToByte( 'p' );
//								lgdata.wcszInstanceName = "p" + lgStatistic.LgNum.ToString("d3");
//								if ( lgStatistic.LgState == eLgModes.CHECKPOINT_JLESS ) {
//									lgdata.drvmode = eLgModes.NORMAL;
//								} else {
//									lgdata.drvmode = lgStatistic.LgState;
//								}
//								lgdata.lgnum = -100;
//								lgdata.devid = pLgStats.lgStat.LgNum;
//
//								lgdata.pctdone = (int)(( lgStatistic.LgStat.TotalBitsDone * 100 ) / lgStatistic.LgStat.TotalBitsDirty);
//								lgdata.pctbab = (int)(( lgStatistic.LgStat.MM_TotalMemUsed * 100 ) / lgStatistic.LgStat.MM_TotalMemAllocated);
//
//								lgdata.entries = (int)lgStatistic.LgStat.RemoteWrPending;
//								lgdata.sectors = (int)(lgStatistic.LgStat.MM_TotalMemUsed / DiskPartitionAPI.DISK_BLOCK_SIZE);
//
//								foreach ( DriverIOCTL.DEV_STATISTICS pDevStat in pLgStats.DevStats ) {
//									Protocol.ftd_perf_instance_t devdata;
//
//									lgdata.connection = 1;
//									devdata.connection = 1;
//
//									devdata.role = Convert.ToByte( ' ' );
//
//									// because old checkpoint flag and reporting, and state filter, we need to return a normal mode
//									// when in fact the Driver is in checkpoint (tracking) and when Journal-Less is active
//									if ( pDevStat.DevState == eLgModes.CHECKPOINT_JLESS ) {
//										devdata.drvmode = eLgModes.NORMAL;
//									} else {
//										devdata.drvmode = pDevStat.DevState;
//									}
//
//									// devdata.devid = devtemp.localbdisk;
//									devdata.devid = pDevStats.cdev;
//									devdata.lgnum = pDevStats.LgNum;
//									// calculate percent done 
//									long totalsects = (long)pDevStats.Disksize;
//									// pctdone
//									devdata.pctdone = (int)((((pDevStats.DevStat.CurrentRefreshBitIndex * 1.0) / (totalsects * 1.0)) * 100.0 ) + 0.5 );
//									// pctbab
//									devdata.pctbab = (int)(((((double)pDevStats.DevStat.RemoteBlksWrPending * DiskPartitionAPI.DISK_BLOCK_SIZE ) * 100.0 ) / lgStatistic.LgStat.MM_TotalMemAllocated ) + 0.5 );
//
//									devdata.entries = (int)pDevStats.DevStat.RemoteWrPending;
//									devdata.sectors = (int)pDevStats.DevStat.RemoteBlksWrPending;
//
//									devdata.bytesread = pDevStats.DevStat.BlksRd * DiskPartitionAPI.DISK_BLOCK_SIZE;
//									devdata.byteswritten = pDevStats.DevStat.BlksWr * DiskPartitionAPI.DISK_BLOCK_SIZE;
//
//									lgdata.bytesread += devdata.bytesread;
//									lgdata.byteswritten += devdata.byteswritten;
//
//									devdata.actual = pDevStats.DevStat.SM_BytesSent;
//									devdata.effective = pDevStats.DevStat.SM_EffectiveBytesSent;
//
//									lgdata.actual += devdata.actual;
//									lgdata.effective += devdata.effective;
//
//									// lg % done should be equal to the lowest device percentage
//									if ( devdata.pctdone < lgdata.pctdone ) {
//										lgdata.pctdone = devdata.pctdone;
//									}
//								}
//							}
//
//#if USE_COLLECTOR
//							//send performance data to TDMF Collector
//							//pData ptr is located after the last valid ftd_perf_instance_t element in perfp.pData.
//							PerformanceStats.ftd_mngt_performance_send_data( perfp.pData );
//							//note : ftd_mngt_send_performance_data modifies the content of perfp.pData
//#endif
//
//							if ( bMutex ) {
//								perfp.hMutex.ReleaseMutex();
//							}
//							SetEvent(perfp.hSetEvent);
//						}
//
//					} finally {
//						PerformanceStats.ftd_mngt_performance_end();
//					}
//
//				//	ftd_delete_errfac();
//				} finally {
//					driver.Close();
//				}
//			}
//			catch ( ThreadAbortException e ) {
//				OmException.LogException( new OmException( String.Format("Error in StatisticsThread Start : {0}, {1}", e.Message, e.ExceptionState ), e ) );
//				// request to quit this thread
//				pData = perfp.pData;
//				memset(pData, -1, Performance.SHARED_MEMORY_OBJECT_SIZE);
//			}
//			catch( Exception e ) {
//				OmException.LogException( new OmException( String.Format("Error in StatisticsThread Start Exception: {0}", e.Message ), e ) );
//			}
			return 0;
		}


		public static string EventName( int group_num ) {
			return String.Format( "DTClg{0}perfget", group_num );
		}


	}
}
