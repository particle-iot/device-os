/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Required functions exported by the port-specific (os-dependent) driver
 * to common (os-independent) driver code.
 *
 * $Id: et_export.h 475115 2014-05-03 06:23:15Z sudhirbs $
 */

#ifndef _et_export_h_
#define _et_export_h_

/* misc callbacks */
extern void et_init(void *et, uint options);
extern void et_reset(void *et);
extern void et_link_up(void *et);
extern void et_link_down(void *et);
extern int et_up(void *et);
extern int et_down(void *et, int reset);
extern void et_dump(void *et, struct bcmstrbuf *b);
extern void et_intrson(void *et);
extern void et_discard(void *et, void *pkt);
extern int et_phy_reset(void *etc);
extern int et_set_addrs(void *etc);

/* for BCM5222 dual-phy shared mdio contortion */
extern void *et_phyfind(void *et, uint coreunit);
extern uint16 et_phyrd(void *et, uint phyaddr, uint reg);
extern void et_phywr(void *et, uint reg, uint phyaddr, uint16 val);
#ifdef HNDCTF
extern void et_dump_ctf(void *et, struct bcmstrbuf *b);
#endif
#ifdef BCMDBG_CTRACE
extern void et_dump_ctrace(void *et, struct bcmstrbuf *b);
#endif
#ifdef BCM_GMAC3
extern void et_dump_fwder(void *et, struct bcmstrbuf *b);
#endif
#ifdef ETFA
extern void et_fa_lock_init(void *et);
extern void et_fa_lock(void *et);
extern void et_fa_unlock(void *et);
extern void *et_fa_get_fa_dev(void *et);
extern bool et_fa_dev_on(void *dev);
extern void et_fa_set_dev_on(void *et);
extern void *et_fa_fs_create(void);
extern void et_fa_fs_clean(void);
#endif /* ETFA */
#endif	/* _et_export_h_ */
