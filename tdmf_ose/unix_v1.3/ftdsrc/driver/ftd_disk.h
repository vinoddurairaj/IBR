/*
 * This file defines the data structures for on disk storage.
 *
 * $Id: ftd_disk.h,v 1.3 2001/01/17 17:58:55 hutch Exp $
 *
 * The first 8k of the partition is reserved for the OS's disk labels and
 * boot code.  We continue to do this as disk space is way cheap and to avoid
 * overwriting the disk lable should the partition reside at offset 0.
 *
 * The first 1k contains information about the number of devices currently
 * active on the system.  It also contains checksums and magic numbers.  this
 * is called the ftd_wl_header_t.
 *
 * Next there are a fixed number of slots for the ftd device description. 
 * These are ftd_dev_meta_t
 *
 * next comes all the lrdb's
 *
 * then comes the persistant state info for each of the devices
 *
 * There are two types of state that is saved.  Either the entire memlog
 * is saved, or a hrdb is saved depending on how much is in the memlog
 * at shutdown time.
 */
