#if !defined(FTD_KERN_CPROTO_H) && (defined(_KERNEL) || defined(KERNEL))
#define FTD_KERN_CPROTO_H

/* include me in all kernel xlation units */

#if defined(SOLARIS) || defined(HPUX)


/*-
 * output of cproto(1) using:
 * /usr/local/bin/cproto -e -DFTD_CPROTO  -DVERSION=\"4.3\" 
 * -DPRODUCTNAME=\"Replication\" -DPRODUCTNAME_TOKEN=\"Replication\" 
 * -DOEM=\"LGTO\" -DPKGNAME=\"LGTOdtc\" -DQNM=\"dtc\" -DCAPQ=\"DTC\" 
 * -DCAPPRODUCTNAME=\"REPLICATION\" -DCAPPRODUCTNAME_TOKEN=\"REPLICATION\" 
 * -DBLDSEQNUM=\"" Build 000"\" -DSYSVERS=570 -DSOLARIS -D_KERNEL -DKERNEL 
 * 
 */
/* ftd_sun.c */
#if defined(SOLARIS) 
#include <sys/modctl.h>

/* ftd_sun.c */
ftd_int32_t _init(ftd_void_t);
ftd_int32_t _fini(ftd_void_t);
ftd_int32_t _info(struct modinfo *modinfop);
FTD_PUBLIC struct buf *ftd_bioclone(ftd_dev_t *softp, struct buf *sbp, struct buf *dbp);
FTD_PRIVATE ftd_int32_t ftd_identify(dev_info_t *dev);
FTD_PUBLIC ftd_dev_t *ftd_get_softp(struct buf *bp);
FTD_PRIVATE ftd_int32_t ftd_probe(dev_info_t *devi);
FTD_PRIVATE ftd_int32_t ftd_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
FTD_PRIVATE ftd_int32_t ftd_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);
FTD_PRIVATE ftd_int32_t ftd_open(dev_t *devp, ftd_int32_t flag, ftd_int32_t otyp, cred_t *credp);
FTD_PRIVATE ftd_int32_t ftd_close(dev_t dev, ftd_int32_t flag, ftd_int32_t otyp, cred_t *credp);
FTD_PRIVATE ftd_int32_t ftd_info(dev_info_t *dip, ddi_info_cmd_t infocmd, ftd_void_t *arg, ftd_void_t **result);
FTD_PRIVATE ftd_int32_t ftd_read(dev_t dev, struct uio *uio, cred_t *cred_p);
FTD_PRIVATE ftd_int32_t ftd_write(dev_t dev, struct uio *uio, cred_t *cred_p);
FTD_PUBLIC ftd_void_t ftd_wakeup(ftd_lg_t *lgp);
FTD_PRIVATE ftd_int32_t ftd_chpoll(dev_t dev, ftd_int16_t events, ftd_int32_t anyyet, ftd_int16_t *reventsp, struct pollhead **phpp);
FTD_PUBLIC ftd_int32_t ftd_layered_open(struct dev_ops **dopp, dev_t *devp);
FTD_PUBLIC ftd_int32_t ftd_layered_close(struct dev_ops *dop, dev_t dev);
FTD_PRIVATE ftd_int32_t ftd_ioctl(dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag, cred_t *credp, ftd_int32_t *rvalp);
FTD_PRIVATE ftd_int32_t ftd_dev_ioctl(dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag, cred_t *credp, ftd_int32_t *rvalp);
FTD_PUBLIC ftd_int32_t ftd_ctl_get_device_nums(dev_t dev, ftd_intptr_t arg, ftd_int32_t flag);
#endif /* defined(SOLARIS)  */

#if defined(HPUX)

#include <sys/uio.h>
/* ftd_hpux.c */
FTD_PRIVATE ftd_int32_t ftd_dev_init (ftd_void_t);
#if defined(HPUX) && (SYSVERS >= 1100)
FTD_PUBLIC ftd_int32_t dtc_load (ftd_void_t*);
FTD_PUBLIC ftd_int32_t mds_load (ftd_void_t*);
FTD_PUBLIC ftd_int32_t stk_load (ftd_void_t*);
FTD_PUBLIC ftd_int32_t dtc_unload (ftd_void_t*);
FTD_PUBLIC ftd_int32_t mds_unload (ftd_void_t*);
FTD_PUBLIC ftd_int32_t stk_unload (ftd_void_t*);
#endif	/* defined(HPUX) && (SYSVERS >= 1100)  */
FTD_PUBLIC ftd_int32_t dtc_install (ftd_void_t);
FTD_PUBLIC ftd_int32_t mds_install (ftd_void_t);
FTD_PUBLIC ftd_int32_t stk_install (ftd_void_t);
FTD_PRIVATE ftd_open (dev_t dev, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_close (dev_t dev, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_read (dev_t dev, struct uio *uio);
FTD_PRIVATE ftd_dev_t *ftd_ld2soft (dev_t dev);
FTD_PUBLIC ftd_dev_t *ftd_get_softp (struct buf *bp);
FTD_PUBLIC ftd_void_t bufzero (struct buf *bp);
FTD_PUBLIC struct buf *ftd_bioclone (ftd_dev_t * softp, struct buf *sbp, struct buf *dbp);
FTD_PRIVATE ftd_int32_t ftd_write (dev_t dev, struct uio *uio);
FTD_PUBLIC ftd_void_t ftd_wakeup (ftd_lg_t * lgp);
FTD_PUBLIC ftd_int32_t ftd_select (dev_t dev, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_dev_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PUBLIC ftd_int32_t bdev_open (dev_t dev, ftd_int32_t flags);
FTD_PUBLIC ftd_int32_t cdev_open (dev_t dev, ftd_int32_t flags);
FTD_PRIVATE ftd_int32_t ftd_attach (ftd_void_t);
FTD_PRIVATE ftd_int32_t ftd_detach (ftd_void_t);
FTD_PUBLIC ftd_int32_t ftd_ctl_get_device_nums (dev_t dev, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PUBLIC struct buf *ftd_init_pvt_buf_pool (ftd_void_t);
FTD_PUBLIC ftd_void_t ftd_fini_pvt_buf_pool (ftd_void_t);
struct buf *getpvtrbuf (ftd_void_t);
ftd_void_t freepvtrbuf (struct buf *bp);

/* ftd_timers.c */
ftd_void_t ftd_timer_enqueue(ftd_lg_t *lgp, wlheader_t *wl);
ftd_void_t ftd_timer_dequeue(ftd_lg_t *lgp, wlheader_t *wl);
ftd_void_t ftd_finish_sync_io(void);
ftd_void_t ftd_timer(ftd_int32_t tm);
#endif /* defined(HPUX) */

/* HPUX doesn't link ftd_buf.o */
#if !defined(HPUX)
/* ftd_buf.c */
FTD_PUBLIC struct buf *ftd_init_pvt_buf_pool(ftd_void_t);
FTD_PUBLIC ftd_void_t ftd_fini_pvt_buf_pool(ftd_void_t);
FTD_PUBLIC ftd_void_t bufzero(struct buf *bp);
FTD_PUBLIC struct buf *getpvtrbuf(ftd_void_t);
FTD_PUBLIC ftd_void_t freepvtrbuf(struct buf *bp);
#endif /* !defined(HPUX) */

/* ftd_bab.c */
FTD_PUBLIC ftd_int32_t ftd_bab_init(ftd_int32_t size, ftd_int32_t num);
FTD_PUBLIC ftd_void_t ftd_bab_fini(ftd_void_t);
FTD_PRIVATE ftd_void_t bab_buffer_free(bab_buffer_t *buf);
FTD_PRIVATE bab_buffer_t *bab_buffer_alloc(ftd_void_t);
FTD_PUBLIC bab_mgr_t *ftd_bab_alloc_mgr(ftd_void_t);
FTD_PUBLIC ftd_int32_t ftd_bab_free_mgr(bab_mgr_t *mgr);
FTD_PUBLIC ftd_int32_t ftd_bab_alloc_memory(bab_mgr_t *mgr, ftd_int32_t len64);
FTD_PUBLIC ftd_int32_t ftd_bab_free_memory(bab_mgr_t *mgr, ftd_int32_t len64);
FTD_PUBLIC ftd_int32_t ftd_bab_commit_memory(bab_mgr_t *mgr, ftd_int32_t len64);
FTD_PUBLIC ftd_int32_t ftd_bab_get_free_length(bab_mgr_t *mgr);
FTD_PUBLIC ftd_int32_t ftd_bab_get_used_length(bab_mgr_t *mgr);
FTD_PUBLIC ftd_int32_t ftd_bab_get_committed_length(bab_mgr_t *mgr);
FTD_PUBLIC ftd_int32_t ftd_bab_copy_out(bab_mgr_t *mgr, ftd_int32_t offset64, ftd_uint64_t *addr, ftd_int32_t len64);
FTD_PRIVATE ftd_uint64_t *ftd_bab_get_ptr(bab_mgr_t *mgr, ftd_int32_t offset64, bab_buffer_t **buf);
FTD_PUBLIC ftd_uint64_t *ftd_bab_get_pending(bab_mgr_t *mgr);

/* ftd_bits.c */
FTD_PRIVATE ftd_void_t ftd_set_bits(ftd_uint32_t *ptr, ftd_int32_t x1, ftd_int32_t x2);
FTD_PUBLIC ftd_int32_t ftd_update_hrdb(ftd_dev_t *softp, struct buf *bp);
FTD_PUBLIC ftd_int32_t ftd_update_lrdb(ftd_dev_t *softp, struct buf *bp);
FTD_PUBLIC ftd_dev_t *ftd_lg_get_device(ftd_lg_t *lginfo, dev_t dev);
FTD_PUBLIC ftd_int32_t ftd_compute_dirtybits(ftd_lg_t *lginfo, ftd_int32_t type);

/* ftd_ioctl.c */
FTD_PRIVATE ftd_void_t ftd_clear_bab_and_stats(ftd_lg_t *lgp);
FTD_PRIVATE ftd_int32_t ftd_get_msb(ftd_int32_t num);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_config(ftd_void_t);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_num_devices(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_num_groups(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_devices_info(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_groups_info(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_device_stats(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_group_stats(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_set_group_state(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_bab_size(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_alloc_minor(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_update_lrdbs(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_update_hrdbs(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_group_state(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_clear_bab(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_clear_dirtybits(ftd_intptr_t arg, ftd_int32_t type);
FTD_PRIVATE ftd_int32_t ftd_ctl_new_device(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_open_persistent_store(ftd_lg_t *lgp);
FTD_PRIVATE ftd_int32_t ftd_ctl_new_lg(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_init_stop(ftd_lg_t *lgp);
FTD_PRIVATE ftd_int32_t ftd_ctl_del_device(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_del_lg(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_ctl_config(ftd_void_t);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_dev_state_buffer(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_get_lg_state_buffer(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_set_dev_state_buffer(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_set_lg_state_buffer(ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_ctl_set_iodelay(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_set_sync_depth(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_set_sync_timeout(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_ctl_start_lg(ftd_intptr_t arg);
FTD_PUBLIC ftd_int32_t ftd_ctl_ioctl(dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_lg_send_lg_message(ftd_lg_t *lgp, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_lg_oldest_entries(ftd_lg_t *lgp, ftd_intptr_t arg);
FTD_PRIVATE ftd_dev_t *ftd_lookup_dev(ftd_lg_t *lgp, dev_t d);
FTD_PRIVATE ftd_int32_t ftd_lg_update_dirtybits(ftd_lg_t *lgp);
FTD_PRIVATE ftd_int32_t ftd_lg_migrate(ftd_lg_t *lgp, ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t ftd_lg_get_dirty_bits(ftd_lg_t *lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_lg_get_dirty_bit_info(ftd_lg_t *lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PRIVATE ftd_int32_t ftd_lg_set_dirty_bits(ftd_lg_t *lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PUBLIC ftd_int32_t ftd_lg_ioctl(dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag);
FTD_PUBLIC ftd_int32_t compare_and_swap(ftd_uint32_t *waddr, ftd_uint32_t *oval, ftd_uint32_t nval);
FTD_PUBLIC ftd_int32_t fetch_and_and(ftd_uint32_t *waddr, ftd_uint32_t mask);
FTD_PUBLIC ftd_int32_t fetch_and_or(ftd_uint32_t *waddr, ftd_uint32_t mask);
FTD_PUBLIC ftd_void_t biodone_psdev(struct buf *bp);

/* ftd_all.c */
#if defined(SOLARIS) && (SYSVERS >= 570) 
FTD_PRIVATE ftd_void_t ftd_synctimo(ftd_void_t *argp);
#else /* defined(SOLARIS) && (SYSVERS >= 570) */
FTD_PRIVATE ftd_void_t ftd_synctimo (struct wlheader_s *hp);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
FTD_PUBLIC ftd_void_t ftd_biodone(struct buf *bp);
FTD_PUBLIC ftd_int32_t ftd_dev_n_open(ftd_ctl_t *ctlp);
FTD_PUBLIC ftd_int32_t ftd_rw(dev_t dev, struct uio *uio, ftd_int32_t flag);
FTD_PRIVATE ftd_void_t ftd_finish_io(ftd_dev_t *softp, struct buf *bp, ftd_int32_t dodone);
FTD_PRIVATE ftd_void_t ftd_get_error(struct buf *bp);
FTD_PRIVATE ftd_int32_t ftd_iodone_generic(struct buf *bp);
FTD_PRIVATE ftd_int32_t ftd_iodone_journal(struct buf *bp);
FTD_PRIVATE ftd_void_t ftd_do_read(ftd_dev_t *softp, struct buf *mybp);
FTD_PRIVATE ftd_void_t ftd_do_write(ftd_dev_t *softp, struct buf *userbp, struct buf *mybp);
FTD_PRIVATE ftd_int32_t ftd_do_io(ftd_dev_t *softp, struct buf *userbp, struct buf *mybp, ftd_context_t context);
FTD_PUBLIC ftd_int32_t ftd_strategy(struct buf *userbp);
FTD_PUBLIC ftd_int32_t ftd_flush_lrdb(ftd_dev_t *softp);
FTD_PUBLIC  ftd_int32_t ftd_dev_close(dev_t dev);
FTD_PUBLIC ftd_int32_t ftd_dev_open(dev_t dev);
FTD_PUBLIC ftd_void_t ftd_do_sync_done(ftd_lg_t *lgp);
FTD_PUBLIC ftd_void_t ftd_clear_dirtybits(ftd_lg_t *lgp);
FTD_PUBLIC ftd_void_t ftd_set_dirtybits(ftd_lg_t *lgp);
FTD_PRIVATE ftd_void_t ftd_clear_hrdb(ftd_lg_t *lgp);
FTD_PUBLIC ftd_int32_t ftd_lg_close(dev_t dev);
FTD_PUBLIC ftd_int32_t ftd_lg_open(dev_t dev);
FTD_PRIVATE ftd_int32_t ftd_close_persistent_store(ftd_lg_t *lgp);
FTD_PUBLIC ftd_void_t ftd_del_lg (ftd_lg_t * lgp, minor_t minor);

FTD_PUBLIC ftd_int32_t ftd_ctl_close(ftd_void_t);
FTD_PUBLIC ftd_int32_t ftd_ctl_open(ftd_void_t);

/* ftd_klog.c */
FTD_PUBLIC ftd_void_t ftd_err(ftd_int32_t s, ftd_char_t *m, ftd_int32_t l, ftd_char_t *f, ...);
FTD_PRIVATE ftd_int32_t _sprintf(ftd_char_t *buf, ftd_char_t *cfmt, ...);
FTD_PRIVATE ftd_char_t *ksprintn(ftd_uint32_t ul, ftd_int32_t base, ftd_int32_t *lenp);
FTD_PRIVATE ftd_int32_t kvprintf(ftd_char_t *fmt, ftd_void_t (*func)(int, ftd_void_t *), ftd_void_t *arg, ftd_int32_t radix, va_list ap);

/* memset.c */
FTD_PUBLIC ftd_void_t *ftd_memset(ftd_void_t *b, ftd_int32_t c, size_t len);

/* version.o */

/* ../version.c */
#endif /* defined(SOLARIS) */

#endif /* !defined(FTD_KERN_CPROTO_H) */
