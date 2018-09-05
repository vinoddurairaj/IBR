//using System;
//using System.IO;
//
//namespace MasterThread
//{
//
//
////
////	public enum eJournalFlags {
////		/* journal states */
////		FTDJRNCO = 0x0001,
////		FTDJRNINCO = 0x0002,
////		FTDJRN = FTDJRNCO | FTDJRNINCO,
////		/* journal modes */
////		FTDMIRONLY = 0x0004,
////		FTDJRNONLY = 0x0008,
////		FTDJRNMIR = (FTDMIRONLY | FTDJRNONLY),
////		/* journal file lock states */
////		JRN_LOCK = 0x0010,
////		/* journal flags */
////		JRN_CPON = 0x0100,
////		JRN_CPPEND = 0x0200
////	}
////
////	public class JRN_Flags {
////		public eJournalFlags flags;
////
////
////		public eJournalFlags GET_JRN_STATE () {
////			return ( flags & eJournalFlags.FTDJRN );
////		}
////		public void SET_JRN_STATE ( eJournalFlags y ) {
////			flags = (flags & ~eJournalFlags.FTDJRN) | y;
////		}
////		public eJournalFlags GET_JRN_MODE() {
////			return flags & eJournalFlags.FTDJRNMIR;
////		}
////		public void SET_JRN_MODE( eJournalFlags y ) {
////			flags = (flags & ~eJournalFlags.FTDJRNMIR) | y;
////		}
////		public bool GET_JRN_LOCK() {
////			return (flags & eJournalFlags.JRN_LOCK) == eJournalFlags.JRN_LOCK;
////		}
////		public void SET_JRN_LOCK() {
////			flags |= eJournalFlags.JRN_LOCK;
////		}
////		public void UNSET_JRN_LOCK() {
////			flags &= ~eJournalFlags.JRN_LOCK;
////		}
////		public bool GET_JRN_CP_ON() {
////			return (flags & eJournalFlags.JRN_CPON) == eJournalFlags.JRN_CPON;
////		}
////		public void SET_JRN_CP_ON() {
////			flags |= eJournalFlags.JRN_CPON;
////		}
////		public void UNSET_JRN_CP_ON() {
////			flags &= ~eJournalFlags.JRN_CPON;
////		}
////		public bool GET_JRN_CP_PEND() {
////			return (flags & eJournalFlags.JRN_CPPEND) == eJournalFlags.JRN_CPPEND;
////		}
////		public void SET_JRN_CP_PEND() {
////			flags |= eJournalFlags.JRN_CPPEND;
////		}
////		public void UNSET_JRN_CP_PEND() {
////			flags &= eJournalFlags.JRN_CPPEND;
////		}
////
////
////	}
////
////	public struct ftd_journal_file_t {
////		public string name;				/* journal file name             */
////		public FileStream fd;			/* journal file handle           */
////		public JRN_Flags flags;			/* journal file state bits       */
////		public Int64 offset;			/* journal file offset           */
////		public Int64 size;				/* journal file size (bytes)     */
////		public Int64 locked_offset;		/* journal file locked offset    */
////		public Int64 locked_length;		/* journal file locked length (bytes) */
////		public int chunksize;			/* journal I/O chunk size        */
////
//////		public ftd_journal_file_t( int zero ) {
//////			fd = null;
//////			offset = 0;
//////			size = 0;
//////			chunksize = (int)JournalConst.JRN_CHUNKSIZE;
//////		}
////	}
////	public enum JournalConst {
////		JRNCPONAGAIN = 100,
////		JRNCPPENDING = 200,
////		JRN_MAXFILES = 1000 ,
////		JRN_CHUNKSIZE = 2*1024*1024
////	}
////
////	public class ftd_journal_t {
////		public FileStream fd;					/* journal directory file handle   */
////		public JRN_Flags flags;					/* journal state bits              */
////		public int cpnum;						/* checkpoint journal file number  */
////		public string jrnpath;					/* journal area path               */
////		public string prefix;					/* prefix to name file objects */
////		public int buflen;						/* I/O buffer length */
////		public IntPtr buf;						/* I/O buffer address */
////		public ftd_journal_file_t cur;			/* active journal file address */
////		public ftd_journal_file_tCollection jrnlist;	/* list of files in the journal */
////	}
////
////	
////	/// <summary>
////	/// Summary description for Journal.
////	/// </summary>
////	public class Journal
////	{
////
////		public const string CP_SUFFIX = "p";
////		public const string CO_SUFFIX = "c";
////		public const string INCO_SUFFIX = "i";
////
////		/// <summary>
////		/// Constructor
////		/// </summary>
////		public Journal()
////		{
////		}
////
////		/*
////		* ftd_journal_create -- create a ftd_journal_t object
////		*/
////		public static ftd_journal_t ftd_journal_create( string jrnpath, string fileprefix ) {
////			ftd_journal_t jrnp = new ftd_journal_t();
////
////			jrnp.prefix = fileprefix;
////
////			jrnp.jrnpath = jrnpath;
////
////			jrnp.jrnlist = new ftd_journal_file_tCollection();
////
////			return jrnp;
////		}
////
////		/// <summary>
////		/// get current state of FTD journal
////		/// </summary>
////		/// <param name="jrnp"></param>
////		/// <param name="prune_inco"></param>
////		/// <returns></returns>
////		public static int ftd_journal_get_cur_state( ftd_journal_t jrnp, bool prune_inco ) {
////			bool inco_flag = false;
////
////			// set default journal state 
////			jrnp.flags.UNSET_JRN_CP_ON();
////			jrnp.flags.UNSET_JRN_CP_PEND();
////
////			jrnp.flags.SET_JRN_STATE ( eJournalFlags.FTDJRNCO );
////			jrnp.flags.SET_JRN_MODE ( eJournalFlags.FTDMIRONLY );
////
////			jrnp.cpnum = 0;
////
////			ftd_journal_get_all_files( jrnp );
////
////			/* get current journal mode and state */
////			bool first = true;
////			foreach ( ftd_journal_file_t jrnfp in jrnp.jrnlist ) {
////				if ( ftd_journal_file_is_cp( jrnfp ) ) {
////					/* set checkpoint state */
////					if ( first ) {
////						jrnp.flags.SET_JRN_CP_ON();
////					} else {
////						jrnp.flags.SET_JRN_CP_PEND();
////					}
////					jrnp.flags.SET_JRN_MODE( eJournalFlags.FTDJRNONLY );
////				} else {
////                
////					int num;
////					eJournalFlags state;
////					eJournalFlags mode;
////					ftd_journal_parse_name( jrnp.flags, jrnfp.name, out num, out state, out mode );
//// 
////					if ( state == eJournalFlags.FTDJRNINCO ) {
////						/* weed out inco journals - if we are asked to */
////						if ( prune_inco ) {
////							ftd_journal_file_delete( jrnp, jrnfp );
////							continue;
////						} else {
////							jrnp.flags.SET_JRN_STATE( state );
////							jrnp.flags.SET_JRN_MODE( mode );
////							inco_flag = true;
////							break;
////						}
////					} else {
////						string filename = jrnp.jrnpath + "\\" + jrnfp.name;
////						FileInfo fileInfo = new FileInfo(filename);
////
////						// if file size == 0 then blow it away 
////						if (fileInfo.Length == 0) {
////							ftd_journal_file_delete( jrnp, jrnfp );
////							continue;
////						}
////						/* coherent journal found - we know we */
////						/* are in coherent journaling mode */
////						jrnp.flags.SET_JRN_STATE( state );
////						jrnp.flags.SET_JRN_MODE( mode );
////						break;
////					}
////				}
////				first = false;
////			}
////			if ( jrnp.flags.GET_JRN_CP_ON() || jrnp.flags.GET_JRN_CP_PEND() ) {
////				jrnp.flags.SET_JRN_MODE( eJournalFlags.FTDJRNONLY );
////			}
////			if ( inco_flag ) {
////				/* INCO journals found - we were prev in an INCO state */
////				jrnp.flags.SET_JRN_STATE( eJournalFlags.FTDJRNINCO );
////				jrnp.flags.SET_JRN_MODE( eJournalFlags.FTDJRNONLY );
////			}
////			return 0;
////		}
////
////		/// <summary>
////		/// ftd_journal_delete_cp  -- delete checkpoint journal file from journal
////		/// </summary>
////		/// <param name="jrnp"></param>
////		/// <returns></returns>
////		public static void ftd_journal_delete_cp( ftd_journal_t jrnp ) {
////			/* clobber checkpoint file object */
////			foreach ( ftd_journal_file_t jrnfpp in jrnp.jrnlist ) {
////				if ( ftd_journal_file_is_cp( jrnfpp ) ) {
////					ftd_journal_file_delete( jrnp, jrnfpp );
////					jrnp.cpnum = -1;
////				}
////			}
////		}
////
////		/// <summary>
////		/// ftd_journal_file_delete -- delete a ftd_journal_file_t object
////		/// </summary>
////		/// <param name="jrnp"></param>
////		/// <param name="jrnfp"></param>
////		/// <returns></returns>
////		public static void ftd_journal_file_delete( ftd_journal_t jrnp, ftd_journal_file_t jrnfp ) {
////
////			string filename;
////
////			if ( jrnfp.fd != null ) {
////				// Before closing mark file as read
////				jrnfp.fd.SetLength( 0 );
////			}
////    
////			// first unlink and close the file 
////			filename = jrnp.jrnpath + "\\" + jrnfp.name;
////
////			// close the journal file object 
////			ftd_journal_file_close( jrnfp );
////
////			// unlink will fail on NT if opened by someone else (ie. rmd)
////			File.Delete( filename );
////
////			// remove it from the journal file list 
////			jrnp.jrnlist.Remove( jrnfp );
////		}
////
////
////		/// <summary>
////		/// get the current list of ftd_journal_file_t objects including .cp file
////		/// </summary>
////		/// <param name="jrnp"></param>
////		/// <returns></returns>
////		public static void ftd_journal_get_all_files( ftd_journal_t jrnp ) {
////			string [] paths;
////			int lnum;
////			eJournalFlags lstate;
////			eJournalFlags lmode;
////
////			paths = ftd_journal_get_file_names( jrnp.jrnpath, jrnp.prefix );
////
////			if ( paths.Length == 0 ) {
////				throw new OmException ( "ftd_journal_get_all_files():Journal count error" );
////			}
////
////			// remove from list any deleted journal files
////			for ( int count = 0 ; count < jrnp.jrnlist.Count ; ) {
////				ftd_journal_file_t jrnfp = jrnp.jrnlist[count];
////				// check for existence of file in list 
////				bool found = false;
////				foreach ( string path in paths ) {
////					if ( String.Compare ( jrnfp.name, path, true ) == 0 ) {
////						found = true;
////						break;
////					}
////				}
////				if ( !found ) {
////					ftd_journal_file_close( jrnfp );
////					jrnp.jrnlist.Remove( jrnfp );
////					continue;
////				}
////				count++;
////			}
////
////			// add to list any new journal files
////			foreach ( string path in paths ) {
////				if ( ftd_journal_file_name_is_cp( path ) ) {
////					ftd_journal_parse_name( jrnp.flags, path, out lnum, out lstate, out lmode );
////					jrnp.cpnum = lnum;
////				}
////				bool found = false;
////				// check for existence of file in list
////				foreach ( ftd_journal_file_t jrnfp in jrnp.jrnlist ) {
////					if ( String.Compare ( jrnfp.name, path, true ) == 0 ) {
////						found = true;
////						break;
////					}
////				}
////				if ( found ) {
////					continue;
////				}
////				// This is in case a journal file was on disk
////				// and we are coming back from a reboot? 
////				// add to list 
////				ftd_journal_file_t jrnf = new ftd_journal_file_t();
////				jrnf.name = path;
////				jrnp.jrnlist.Add ( jrnf );
////			}
////			// first field in struct is journal name string
////			jrnp.jrnlist.Sort();
////		}
////
////
////		/// <summary>
////		/// close ftd_journal_file_t object
////		/// </summary>
////		/// <param name="jrnfp"></param>
////		/// <returns></returns>
////		public static void ftd_journal_file_close( ftd_journal_file_t jrnfp ) {
////			if ( jrnfp.fd != null ) {
////				if ( jrnfp.flags.GET_JRN_LOCK() ) {
////					// unlock before closing
////					ftd_journal_file_unlock( jrnfp,	jrnfp.locked_offset, jrnfp.locked_length );
////				}
////				jrnfp.fd.Close();
////				jrnfp.fd = null;
////			}
////		}
////
////		
////
////		/// <summary>
////		/// ftd_journal_file_unlock -- unlock a ftd_journal_file_t object
////		/// </summary>
////		/// <param name="jrnfp"></param>
////		/// <param name="offset"></param>
////		/// <param name="lock_len"></param>
////		/// <returns></returns>
////		public static void ftd_journal_file_unlock ( ftd_journal_file_t jrnfp, Int64 offset, Int64 lock_len ) {
////			jrnfp.fd.Unlock( offset, lock_len );
////			ftd_journal_file_unlock_header( jrnfp );
////			jrnfp.locked_offset = 0;
////			jrnfp.locked_length = 0;
////		}
////
////		public struct ftd_jrnheader_t {
////			//uint magicnum;
////			public int state;
////			public int mode;
////		}
////
////		/// <summary>
////		/// ftd_journal_file_unlock -- unlock a ftd_journal_file_t object
////		/// </summary>
////		/// <param name="jrnfp"></param>
////		public static unsafe void ftd_journal_file_unlock_header( ftd_journal_file_t jrnfp ) {
////			jrnfp.fd.Unlock( 0, sizeof(ftd_jrnheader_t) - 1 );
////		}
////
////		/// <summary>
////		/// get the current sorted list of FTD journal file names
////		/// </summary>
////		/// <param name="srchpath"></param>
////		/// <param name="prefix"></param>
////		/// <returns></returns>
////		public static string [] ftd_journal_get_file_names( string srchpath, string prefix ) {
////			string [] paths;
////			paths =	Directory.GetFiles( srchpath, prefix + "*" );
////			Array.Sort(paths);
////			return paths;
////		}
////		
////		/// <summary>
////		/// is the target journal file a checkpoint journal file ?
////		/// </summary>
////		/// <param name="jrnfp"></param>
////		/// <returns></returns>
////		public static bool ftd_journal_file_is_cp( ftd_journal_file_t jrnfp ) {
////			return ftd_journal_file_name_is_cp( jrnfp.name );
////		}
////
////		/// <summary>
////		/// is the target journal file a coherent journal file ?
////		/// </summary>
////		/// <param name="jrnfp"></param>
////		/// <returns></returns>
////		public static bool ftd_journal_file_is_co( ftd_journal_file_t jrnfp ) {
////			return jrnfp.name.EndsWith( CO_SUFFIX );
////		}
////
////		/// <summary>
////		/// is the target journal file a incoherent journal file ?
////		/// </summary>
////		/// <param name="jrnfp"></param>
////		/// <returns></returns>
////		public static bool ftd_journal_file_is_inco( ftd_journal_file_t jrnfp ) {
////			return jrnfp.name.EndsWith( INCO_SUFFIX );
////		}
////
////		/// <summary>
////		/// is the target journal file name a checkpoint journal file name ?
////		/// </summary>
////		/// <param name="name"></param>
////		/// <returns></returns>
////		public static bool ftd_journal_file_name_is_cp( string name ) {
////			return name.EndsWith( CP_SUFFIX );
////		}
////
////		/// <summary>
////		/// parse the components from journal name 
////		/// </summary>
////		/// <param name="flags"></param>
////		/// <param name="name"></param>
////		/// <param name="num"></param>
////		/// <param name="state"></param>
////		/// <param name="mode"></param>
////		/// <returns></returns>
////		public static void ftd_journal_parse_name( JRN_Flags flags, string name, out int num, out eJournalFlags state, out eJournalFlags mode ) {
////			int lnum;
////			eJournalFlags lstate;
////			eJournalFlags lmode;
////
////			lnum = Convert.ToInt32( name.Substring( name.Length-6, 4 ));
////
////			switch ( name.Substring ( name.Length-1 ) ) {
////				case INCO_SUFFIX:
////					state = eJournalFlags.FTDJRNINCO;
////					mode = eJournalFlags.FTDJRNONLY;
////					num = lnum;
////					break;
////				case CO_SUFFIX:
////					state = eJournalFlags.FTDJRNCO;
////					mode = flags.GET_JRN_CP_ON() ? eJournalFlags.FTDJRNONLY : eJournalFlags.FTDJRNMIR;
////					num = lnum;
////					break;
////				case CP_SUFFIX:
////					lstate = flags.GET_JRN_STATE();
////					lmode = flags.GET_JRN_MODE();
////					state = lstate != 0 ? lstate : eJournalFlags.FTDJRNCO;
////					mode = lmode != 0 ? lmode : eJournalFlags.FTDMIRONLY;
////					num = lnum;
////					break;
////				default:
////					/* error */
////					state = eJournalFlags.FTDJRNINCO;
////					mode = eJournalFlags.FTDMIRONLY;
////					num = -1;
////					break;
////			}
////		}
////
////
////		/// <summary>
////		/// ftd_journal_create_next_cp -- create the next-in-sequence-number-order ftd_journal_file object as a checkpoint marker 
////		/// </summary>
////		/// <param name="jrnp"></param>
////		/// <param name="jrnfp"></param>
////		/// <returns></returns>
////		public static int ftd_journal_create_next_cp( ftd_journal_t jrnp, ftd_journal_file_t jrnfp ) {
//////			ftd_journal_file_t  **ljrnfpp, *ljrnfp;
//////			int                 jrncnt, lnum, lstate, lmode, i; 
//////
//////			/* if checkpoint already on or pending then return */
//////			i = 0;
//////			foreach ( ftd_journal_file_t ljrnfpp in jrnp.jrnlist ) {
//////				if (ftd_journal_file_is_cp((*ljrnfpp))) {
//////					if (i == 0) {
//////						/* checkpoint is currently on */
//////						return JRNCPONAGAIN;
//////					} else {
//////						/* checkpoint is pending */
//////						return JRNCPPENDING;
//////					}
//////				}
//////				i++;
//////			}
//////
//////			jrncnt = ftd_journal_get_all_files(jrnp);
//////
//////			lnum = 0;
//////			if (jrncnt > 0) {
//////				ljrnfpp = TailOfLL(jrnp.jrnlist);
//////				ftd_journal_parse_name(jrnp.flags,
//////					(*ljrnfpp).name, &lnum, &lstate, &lmode);
//////			}
//////			lnum++;
//////
//////			/* cp not on or pending - create the checkpoint journal file object */
//////			ljrnfp = ftd_journal_file_create(jrnp);
//////    
//////
//////			sprintf(ljrnfp.name, "%s%04d.p", jrnp.prefix, lnum);
//////
//////			/* create a checkpoint journal file object */
//////			// XXX JRL why dont we open this write/create?
//////			// TM - because we never write to it
//////			if ((ftd_journal_file_open(jrnp, ljrnfp, O_CREAT, 0600)) == -1) {
//////				return -1;
//////			}
//////
//////			ftd_journal_file_close(ljrnfp);
//////
//////			ftd_journal_add_to_list(jrnp.jrnlist, &ljrnfp);
//////
//////			jrnfp = ljrnfp;
////
////			return 0;
////		}
////
////
////		/// <summary>
////		/// ftd_journal_create_next -- create the next-in-sequence-number-order
////		///  ftd_journal_file object for the journal 
////		/// </summary>
////		/// <param name="jrnp"></param>
////		/// <param name="tstate"></param>
////		/// <returns></returns>
////		public static ftd_journal_file_t ftd_journal_create_next( ftd_journal_t jrnp, eJournalFlags tstate ) {
////			ftd_journal_file_t jrnfp = new ftd_journal_file_t();
//////			ftd_jrnheader_t     jrnheader;
//////			char                *statestr;
//////			int                 jrncnt, lnum, lstate, lmode, n; 
//////
//////			lnum = 0;
//////
//////			// we are the only process that 'creates' files - get the list
//////			jrncnt = ftd_journal_get_all_files(jrnp);
//////
//////			if (jrncnt < 0) {
//////				return NULL;
//////			} else if (jrncnt > 0) {
//////				// get state of last journal file in the list 
//////				jrnfpp = TailOfLL(jrnp.jrnlist);
//////
//////				ftd_journal_parse_name(jrnp.flags,
//////					(*jrnfpp).name, &lnum, &lstate, &lmode);
//////			}
//////
//////			if (lnum++ >= FTD_MAX_JRN_NUM) {
//////				// we're fucked - blindly roll back to 1 for now 
//////				error_tracef( TRACEERR, "ftd_journal_create_next(): Roll Back to 1");
//////				lnum = 1;
//////			}
//////
//////			if ((jrnfp = ftd_journal_file_create(jrnp)) == NULL) {
//////				error_tracef( TRACEERR, "ftd_journal_create_next(): File create error");
//////				return NULL;
//////			}
//////			statestr = (tstate == eJournalFlags.FTDJRNCO) ? "c": "i";
//////    
//////			// create the journal file name from the state info 
//////
//////			sprintf(jrnfp.name, "%s%04d.%s", jrnp.prefix, lnum, statestr);
//////  
//////			// open the new journal file object for the journal 
//////
//////			// TODO: don't we need to create this locked somehow ? 
//////			// if rmda is running it can clobber before we write the header 
//////
//////			if ((ftd_journal_file_open(jrnp, jrnfp, O_WRONLY | O_CREAT, 0600)) == -1) {
//////				error_tracef( TRACEERR, "ftd_journal_create_next(): File open error");
//////				return NULL;
//////			}
//////			ftd_journal_parse_name(jrnfp.flags, jrnfp.name,
//////				&lnum, &lstate, &lmode);
//////
//////			// write header to journal 
//////			jrnheader.magicnum = FTDJRNMAGIC;
//////			jrnheader.state = tstate;
//////			jrnheader.mode = lmode; 
//////
//////			if (ftd_llseek(jrnfp.fd, (offset_t)0, SEEK_SET) == (ftd_uint64_t)-1) {
//////				reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
//////					jrnfp.name,
//////					0,
//////					ftd_strerror());
//////				return NULL;
//////			}
//////			n = sizeof(ftd_jrnheader_t);
//////			if (ftd_write(jrnfp.fd, (char*)&jrnheader, n) != n) { {
//////				ftd_uint64_t    len64 = n;
//////
//////				if (ftd_errno() == ENOSPC) {
//////					reporterr(ERRFAC, M_JRNSPACE, ERRCRIT, jrnp.jrnpath);
//////				} else {
//////					reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
//////						jrnfp.name, 
//////						0,
//////						0,
//////						len64,
//////						ftd_strerror());
//////				}
//////				return NULL;
//////			}
//////			}
//////			ftd_fsync(jrnfp.fd);
//////
//////			jrnfp.size = sizeof(ftd_jrnheader_t);
//////			jrnfp.offset = jrnfp.size = sizeof(ftd_jrnheader_t);
//////			jrnfp.chunksize = JRN_CHUNKSIZE;
//////
//////			ftd_journal_add_to_list(jrnp.jrnlist, &jrnfp);
//////
//////			// reset journal cur address to new file object address 
//////			jrnp.cur = jrnfp;
//////
//////			// set journal state and mode 
//////			SET_JRN_STATE(jrnp.flags, lstate);
//////			SET_JRN_MODE(jrnp.flags, lmode);
//////
//////			error_tracef( TRACEINF, "ftd_journal_create_next jrnmode, jrnstate = %d, %s, %s", GET_JRN_CP_ON(jrnp.flags), DisplayJrnMode(jrnp.flags), (GET_JRN_STATE(jrnp.flags) == FTDJRNCO ? "COH":"INCOH"));
////			return jrnfp;
////		}
////
////		/// <summary>
////		/// ftd_lg_stop_journaling -- stop journaling entries on secondary system.
////		/// </summary>
////		/// <param name="lgp"></param>
////		/// <param name="jrnstate"></param>
////		/// <param name="jrnmode"></param>
////		/// <returns></returns>
////		public static int ftd_lg_stop_journaling( ftd_lg_t lgp, eJournalFlags jrnstate, eJournalFlags jrnmode ) {
//////			ftd_journal_file_t jrnfp;
//////			int rc;
//////
//////			ftd_journal_set_state( lgp.jrnp, jrnstate );
//////
//////			// depends on target journal mode   
//////			switch( jrnmode ) {
//////				case eJournalFlags.FTDJRNMIR: 
//////					// apply any coherent journals while journaling 
//////					jrnfp = ftd_journal_create_next( lgp.jrnp, jrnstate );
//////					if ( !lgp.flags.GET_LG_CPON() ) {
//////						if ((rc = ftd_sock_send_start_apply(lgp.lgnum, lgp.isockp, 0)) < 0) {
//////							return rc;
//////						}
//////					}
//////					break;
//////				case eJournalFlags.FTDMIRONLY:
//////					// stop journaling and open mirrors 
//////					lgp.jrnp.cur = null;
//////					//
//////					// However, this is often the initial open, so leave the open
//////					//
//////					rc = ftd_lg_open_devs(lgp, O_RDWR | O_EXCL, 0, 5);      
//////					if ( rc < 0) {
//////						if ( rc == FTD_LG_SEC_DRIVE_ERR ) //WR 17511
//////							return rc; 
//////						else
//////							return -1;
//////					}
//////					break;
//////				default:
//////					break;
//////			}
//////
//////			lgp.jrnp.flags.SET_JRN_MODE( jrnmode );
//////			lgp.jrnp.flags.SET_JRN_STATE( jrnstate );
////
////			return 0;
////		}
////
////		/*
////		 * ftd_journal_set_state --
////		 * set state of journal 
////		 */
////		public static int ftd_journal_set_state( ftd_journal_t jrnp, eJournalFlags tstate ) {
//////			int jrncnt, num, state, mode;
//////			string path, tpath, statec;
//////
//////			ftd_journal_get_all_files(jrnp);
//////
//////			// XXX JRL is this the right place to put this?
//////			ftd_journal_close_all_files(jrnp);
//////
//////			foreach ( ftd_journal_file_t jrnfp in jrnp.jrnlist ) {
//////				if (ftd_journal_file_is_cp( jrnfp )) {
//////					continue;
//////				}
//////
//////				ftd_journal_parse_name(jrnp.flags, jrnfp.name, &num, &state, &mode); 
//////
//////				if ( state == tstate ) {
//////					/* don't need to change - already there */
//////					continue;
//////				}
//////
//////				sprintf(path, "%s\\%s", jrnp.jrnpath, jrnfp.name);
//////
//////				if ( tstate == eJournalFlags.FTDJRNCO ) {
//////					statec = "c";
//////				} else if ( tstate == eJournalFlags.FTDJRNINCO ) {
//////					statec = "i";
//////				}
//////
//////				sprintf(tpath, "%s\\%s%04d.%s", jrnp.jrnpath, jrnp.prefix, num, statec);
//////
//////				rename(path, tpath);
//////			}
//////
//////			jrnp.flags.SET_JRN_STATE( tstate );
////
////			return 0;
////		}
////
////
////	}
//}
