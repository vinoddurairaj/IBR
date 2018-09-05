using System;
using System.Net.Sockets;
using System.Threading;
using System.Runtime.InteropServices;

namespace MasterThread.Management {



	public class RepGrpState {

		public Protocol.mmp_TdmfGroupState m_state;
		public bool m_bStateChanged;

		public DateTime m_time_last_perf = new DateTime(0);	// time in second 
		public TimeSpan m_deltatime_perf = new TimeSpan( 0, 0, 1 );
		public UInt64 m_prev_actual;
		public UInt64 m_prev_effective;
		public UInt64 m_prev_bytesread;
		public UInt64 m_prev_byteswritten;

		/// <summary>
		/// Constructor
		/// </summary>
		public RepGrpState() {
			m_state.sState = 0;
			m_state.sRepGrpNbr = -1;
			m_prev_actual       = 0;
			m_prev_effective    = 0;
			m_prev_bytesread    = 0;
			m_prev_byteswritten = 0;
		}
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="sGrpNbr"></param>
		/// <param name="bStarted"></param>
		public RepGrpState ( short sGrpNbr, bool bStarted ) {
			m_state.sState = 0;
			setGroupNbr( sGrpNbr );
			setStartState( bStarted );
			m_prev_actual       = 0;
			m_prev_effective    = 0;
			m_prev_bytesread    = 0;
			m_prev_byteswritten = 0;
		}
		~RepGrpState() {
		}

//		public static bool operator < ( RepGrpState grpstate ) {
//			return grpstate.m_state.sRepGrpNbr < m_state.sRepGrpNbr;
//		}

		//bit 0 indicates if group is started (1) or stopped (0)
		public void setStartState ( bool bNewStartState ) {
			short prevState = m_state.sState;
			m_state.sState  = (short)(( m_state.sState & 0xfffe ) | ( bNewStartState ? 0x1 : 0x0 ));	// set or clear bit 0 
			//preserve a 'true' m_bStateChanged until hasGroupStateChanged() is called
			m_bStateChanged = m_bStateChanged || ( prevState != m_state.sState );
		}

		//bit 1 indicates if group is in checkpoint mode (1) or not (0)
		public void setCheckpointState( bool bIsInCheckpoint ) { 
			short prevState = m_state.sState;
			m_state.sState = (short)(( m_state.sState & 0xfffd ) | ( bIsInCheckpoint ? 0x2 : 0x0 ));	// set or clear bit 1
			//preserve a 'true' m_bStateChanged until hasGroupStateChanged() is called
			m_bStateChanged = m_bStateChanged || ( prevState != m_state.sState );
		}
		public void setGroupNbr( short sGrpNbr ) {
			m_state.sRepGrpNbr = sGrpNbr;
		}
		public short getGroupNbr() {
			return m_state.sRepGrpNbr;
		}
		public Protocol.mmp_TdmfGroupState getGroupState() { 
			return m_state; 
		}
		public bool isGroupStarted() { 
			return ( m_state.sState & 0x1 ) > 0;	// bit 0 = group started/stopped 
		}
		public bool hasGroupStateChanged() { 
			bool ret = m_bStateChanged;                            
			m_bStateChanged = false;		//auto-reset 
			return ret;
		}

		public void setRateDeltaTime() {
			DateTime now = DateTime.Now;
			m_deltatime_perf = now - m_time_last_perf;
			if ( m_deltatime_perf < new TimeSpan( 0, 0, 1 ) ) {
				m_deltatime_perf = new TimeSpan( 0, 0, 1 );
			}
			m_time_last_perf = now;//prepare for next call
		}

		public UInt64 getRate( UInt64 actVal,UInt64 prevVal ) {
			//1024 . in kilo Bytes per second
			if ( actVal > prevVal ) {
				return (actVal - prevVal) / (ulong)m_deltatime_perf.Seconds;
			} else {
				return 0;
			}
		}
	}


	/// <summary>
	/// Summary description for PerformanceStats.
	/// </summary>
	public class PerformanceStats {
		

		public const int SHARED_MEMORY_ITEM_COUNT = Globals.MAX_GRP_NUM * Globals.MAX_DEVICE_NUM;


		public static DateTime guiTdmfPerf_FastRateSendEndTime;

		static Protocol.ftd_perf_instance_t [] DeviceInstances;

		static int gEmulatorStatMsgCntr = 0;
		static bool gbForceTxAllGrpState;
		static bool gbForceTxAllGrpPerf; 
		public static AutoResetEvent m_AutoEvent = new AutoResetEvent( false );
		static eLgModes gsMode;
		static DateTime gtLastRawPerfTx;

		static int iResendCollector = 0;
		static bool bResendCollector = false;

		/// <summary>
		/// Constructor
		/// </summary>
		public PerformanceStats() {
		}

		// element in map means group is started; 
		// map index is group number
		public static RepGrpStateCollection gmapRepGrpState = new RepGrpStateCollection();

		/////////////////////////////////////////////////////////////////////////////
		// group state sent to Collector by StatThread()
		// called from another thread than ftd_mngt_performance_send_data()
		void ftd_mngt_performance_set_group_cp( short sGrpNumber, bool bIsCheckpoint ) {
			RepGrpState grpstate = gmapRepGrpState[ sGrpNumber ];
			grpstate.setGroupNbr( sGrpNumber );
			grpstate.setCheckpointState( bIsCheckpoint );
		}

		/// <summary>
		/// This function is called within the StatThread()
		/// </summary>
		public static void ftd_mngt_performance_init() {
			gEmulatorStatMsgCntr = 0;

			DeviceInstances = new Protocol.ftd_perf_instance_t[ SHARED_MEMORY_ITEM_COUNT ];

			Protocol.gszServerUID = UniqueHostID.ToStringID();	//acquire only once   

			gmapRepGrpState.Clear();

			gbForceTxAllGrpState  = true;
			gbForceTxAllGrpPerf   = true; 
			gtLastRawPerfTx       = new DateTime( 0 );
			guiTdmfPerf_FastRateSendEndTime = new DateTime( 0 );
		}

		public static void ftd_mngt_performance_end() {
			gmapRepGrpState.Clear();
		}

		// immediat request to statd to acquire statistics
		public static void ftd_mngt_performance_force_acq() {
			m_AutoEvent.Set();
		}

		public static AutoResetEvent ftd_mngt_performance_get_force_acq_event() {
			return m_AutoEvent;
		}

		/// <summary>
		/// Make sure to update Collector with ALL the LATEST information.
		/// </summary>
		void ftd_mngt_performance_send_all_to_Collector() {
			gbForceTxAllGrpState  = true;
			gbForceTxAllGrpPerf   = true; 
		}

		/// <summary>
		/// This function is called within the StatThread()
		/// </summary>
		/// <param name="pDeviceInstanceData"></param>
		/// <param name="iNPerfData"></param>
		public static  void ftd_mngt_performance_send_data( Protocol.ftd_perf_instance_t [] DeviceInstances ) {
			bool MessageSent = false;

			// do not try to talk to collector if collector not present.
			if ( !Protocol.IsCollectorAvailable() ) {
				return;
			}
			// Verify if any of the Repl. Group state have changed. If so, send it.
			bool bGrpStateHdrSent = false;

			Socket socket = Protocol.CollectorConnect();
			try {
				// send vector of group states. 
				bool bForceTxAllGrpState = gbForceTxAllGrpState;	// local copy = multi-thread protection

				// add started groups to gmapRepGrpState[] map
				foreach ( Protocol.ftd_perf_instance_t instance in DeviceInstances ) {
					// group stats : lgnum == -100 , devid = lg number
					if (( instance.lgnum == -100 ) && ( instance.devid >= 0 )) {
						// a group in DeviceInstances vector means it is started.
						RepGrpState grpstate = gmapRepGrpState[ instance.devid ];
						grpstate.setStartState( true );
						grpstate.setGroupNbr( (short)instance.devid );
					}
				}
				// send all group state to Collector
				foreach ( RepGrpState repGrpState in gmapRepGrpState ) {
					bool bGroupStopped = true;
					// now check for groups that have been stopped
					if ( repGrpState.isGroupStarted() ) {
						// verify if group is still active - started    
						foreach ( Protocol.ftd_perf_instance_t instance in DeviceInstances ) {
							// group stats : lgnum == -100 , devid = lg number
							if ( instance.lgnum == -100 && instance.devid >= 0 ) {
								if ( repGrpState.getGroupNbr() == instance.devid ) {
									// group still started
									bGroupStopped = false;
									break;
								}
							}
						}
						if ( bGroupStopped ) {
							// group in map is not found in DeviceInstances . group has been stopped
							repGrpState.setStartState( false );
						}
					}
					// send this group state to Collector, if required
					if (( bResendCollector = repGrpState.hasGroupStateChanged() ) ||
						( bForceTxAllGrpState ) ||
						( iResendCollector > 0 )) {
						// Saumya Tripathi 06/23/04
						// BofA Temporary Fix:: Tried on BofA system on 06/02/04
						if ( bResendCollector ) {
							iResendCollector = 25;
						}
						else {
							if ( iResendCollector > 0 ) {
								iResendCollector--;
							}
						}

						// now that we know we have to send a msg, make sure the header has been sent once.
						if ( !bGrpStateHdrSent ) {
							Protocol.SendGroupStateMsg( socket );
							bGrpStateHdrSent = true;//send hdr only once
						}
						// send the state information
						Protocol.SendGroupState( socket, repGrpState.getGroupState() );
						MessageSent = true;
					}
				}

				if ( bGrpStateHdrSent ) {
					// send the last one that acts as a end of TX flag
					Protocol.mmp_TdmfGroupState grpstate = new Protocol.mmp_TdmfGroupState( -1, -1 );
					Protocol.SendGroupState( socket, grpstate );
					MessageSent = true;
				}
				//reset flag
				gbForceTxAllGrpState = false;
			} finally {
				socket.Close();
			}
	
			// check if any groups are in REFRESH mode.
			// if so, make sure the stat update period remains at one second .
			gsMode = eLgModes.NORMAL;
			foreach ( Protocol.ftd_perf_instance_t instance in DeviceInstances ) {
				// group stats : lgnum == -100 , devid = lg number
				if ( instance.lgnum == -100 && instance.devid >= 0 ) {
					if ((( instance.drvmode & eLgModes.REFRESH ) == eLgModes.REFRESH ) ||
						(( instance.drvmode & eLgModes.BACKFRESH ) == eLgModes.BACKFRESH ) ) {
						gsMode = eLgModes.REFRESH;
						break;
					}
				}
			}

			DateTime now = DateTime.Now;
			// is it time to send performance data to TDMF Collector ??
			if (( ftd_mngt_performance_is_time_to_upld( now, now - gtLastRawPerfTx ) ) ||
				( gbForceTxAllGrpPerf )) {
				//yes
				if ( DeviceInstances.Length > 0 ) {
					// compute rate values
					foreach ( Protocol.ftd_perf_instance_t instance in DeviceInstances ) {
						// GROUP stats (not device stats) : lgnum == -100 , devid = lg number
						if ( instance.lgnum == -100 && instance.devid >= 0 ) {	
							UInt64 rate;
							RepGrpState grpstate = gmapRepGrpState[ instance.devid /*grp nbr*/ ];
							grpstate.setRateDeltaTime();

							rate                    = grpstate.getRate( instance.actual, grpstate.m_prev_actual );
							grpstate.m_prev_actual  = instance.actual;
							instance.actual         = rate;

							rate                    = grpstate.getRate( instance.effective, grpstate.m_prev_effective );
							grpstate.m_prev_effective = instance.effective;
							instance.effective      = rate;

							rate                    = grpstate.getRate( instance.bytesread, grpstate.m_prev_bytesread );
							grpstate.m_prev_bytesread = instance.bytesread;
							instance.bytesread      = rate;

							rate                    = grpstate.getRate( instance.byteswritten, grpstate.m_prev_byteswritten );
							grpstate.m_prev_byteswritten = instance.byteswritten;
							instance.byteswritten   = rate;
						}
					}

					// the following section of code is done ONLY ONCE when EMULATOR_MODE == false.
					// otherwise, more iterations are performed.
					int iEmulatorIdx = Management.gTdmfAgentEmulator.iAgentRangeMin;
					string ServerID = Protocol.gszServerUID;
					do {
						// section begin
						// this section of code is done only once when not emulating servers - EMULATOR_MODE == false
						socket = Protocol.CollectorConnect();
						try {
							// send Perf data and all of the ftd_perf_instance_t 
							Protocol.SendGroupState( socket, DeviceInstances, ServerID );
							gbForceTxAllGrpPerf = false;
							MessageSent = true;
						} finally {
							socket.Close();
						}						//section end

						if ( Management.gTdmfAgentEmulator.bEmulatorEnabled ) {
							// change host id
							Management.ftd_mngt_emulatorGetHostID( Protocol.gszServerUID, iEmulatorIdx, ref ServerID );
							// use 'insert' as a message counter. This allows to detect missed message.
							foreach ( Protocol.ftd_perf_instance_t instance in DeviceInstances ) {
								instance.insert = gEmulatorStatMsgCntr++;
							}
						}

					} while( Management.gTdmfAgentEmulator.bEmulatorEnabled && iEmulatorIdx++ <= Management.gTdmfAgentEmulator.iAgentRangeMax );

					gtLastRawPerfTx = now;
				}
			}
			// If we sent a message in this pass, update the alive message time stamp
			// (this means the alive message will wait +10 secs before being sent)
			if ( MessageSent ) {
				Management.ftd_mngt_UpdateAliveMsgTime();
			}
		}

		//request that the perf upload period be 1 second for the next N seconds
		//after that period, it falls back to the configured gTdmfPerfConfig.iPerfUploadPeriod.
		public static void ftd_mngt_performance_reduce_stat_upload_period( TimeSpan iFastPerfPeriodSeconds, bool bWakeUpStatThreadNow) {
			DateTime now = DateTime.Now;
			if ( guiTdmfPerf_FastRateSendEndTime > now ) {
				//add on top of remainding delay already requested
				guiTdmfPerf_FastRateSendEndTime += guiTdmfPerf_FastRateSendEndTime - now + iFastPerfPeriodSeconds;
			}
			else {
				guiTdmfPerf_FastRateSendEndTime = now + iFastPerfPeriodSeconds;
			}
			if ( bWakeUpStatThreadNow ) {
				// immediate wake up of StatThread
				ftd_mngt_performance_force_acq();	
			}
		}

		//time at which the Agent may stop sending perf stats at highest frequency.
		static DateTime ftd_mngt_get_force_fast_send_end_time() {
			return guiTdmfPerf_FastRateSendEndTime;
		}

		// STATD thread wake-up period
		// IMMEDIATE : timeout when a TDMF command has been received and fast perf stats/mode feedback is required.
		public const int IMMEDIAT_WAKEUP_PERIOD = 1; //  1 second
		// ACTION : timeout when at least one group is in REFRESH/BACKFRESH mode
		public const int ACTION_WAKEUP_PERIOD = 3; //  3 seconds
		// DEFAULT : timeout when nothing particular occurs on agent
		public const int DEFAULT_WAKEUP_PERIOD = 10; // 10 seconds

		// this function provides the statd timeout/wake-up value, 
		// it is based on group states / mode.
		public static int ftd_mngt_performance_get_wakeup_period() {
			//top priority
			if ( DateTime.Now <= ftd_mngt_get_force_fast_send_end_time() ) {
				return IMMEDIAT_WAKEUP_PERIOD;
			}
			else if ( gsMode != eLgModes.NORMAL ) {
				return ACTION_WAKEUP_PERIOD;
			}
			return DEFAULT_WAKEUP_PERIOD;
		}


		// RAW PERF TX up period
		public const int IDLE_PERF_DATA_TX_WAKEUP_PERIOD = 60;
		public const int FOCUS_PERF_DATA_TX_WAKEUP_PERIOD = 10;
		public const int ACTION_PERF_DATA_TX_WAKEUP_PERIOD = 3;
		public const int IMMEDIATE_PERF_DATA_TX_WAKEUP_PERIOD = 1;

		// period requested by Collector (from GUI)
		static bool ftd_mngt_performance_is_time_to_upld( DateTime now, TimeSpan deltat ) {
			//top priority
			if ( now <= ftd_mngt_get_force_fast_send_end_time() ) {
				return true;
			}
			else if ( gsMode != eLgModes.NORMAL ) {
				// action is going on here, (in REFRESH or other)
				return ( deltat.Seconds >= ACTION_PERF_DATA_TX_WAKEUP_PERIOD );
			}
			// mode NORMAL
			TimeSpan requested_upld_period = new TimeSpan( 0, 0, Management.gTdmfPerfConfig.iPerfUploadPeriod / 10 );
			return ( deltat >= requested_upld_period );
		}

	}
}
