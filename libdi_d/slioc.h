/*
 *   libdi - CD Audio Device Interface Library
 *
 *   Copyright (C) 1993-2004  Ti Kan
 *   E-mail: xmcd@amb.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __SLIOC_H__
#define __SLIOC_H__

#if defined(DI_SLIOC) && !defined(DEMO_ONLY)

#ifndef lint
static char *_slioc_h_ident_ = "@(#)slioc.h	6.39 04/01/14";
#endif

/*
 * Linux specific definitions
 */
#ifdef _LINUX

#include <linux/cdrom.h>

/* older Linux systems don't define this */
#ifndef CDROMCLOSETRAY
#define CDROMCLOSETRAY		0x5319
#endif
#ifndef CDROM_DATA_TRACK
#define CDROM_DATA_TRACK	0x04
#endif


/* CD changer support */
#ifdef USE_UCDROM_H

#include <linux/ucdrom.h>

#else

/* ioctl services */
#define CDROM_SELECT_DISC	0x5323
#define CDROM_MEDIA_CHANGED	0x5325
#define CDROM_DRIVE_STATUS	0x5326
#define CDROM_CHANGER_NSLOTS	0x5328

/* CDROM_DRIVE_STATUS arg: denote current disc */
#define CDSL_CURRENT		((int) (~0U>>1))

/* CDROM_DRIVE_STATUS return */
#define CDS_NO_INFO		0
#define CDS_NO_DISC		1
#define CDS_TRAY_OPEN		2
#define CDS_DRIVE_NOT_READY	3
#define CDS_DISC_OK		4

#endif	/* USE_UCDROM_H */

#endif	/* _LINUX */


/*
 * SunOS 4.x specific definitions
 */
#ifdef _SUNOS4

#include <sys/buf.h>
#include <sun/dkio.h>
#include <scsi/targets/srdef.h>

/* This is a hack to work around a bug in SunOS 4.x's _IO macro family
 * in <sys/ioccom.h> which makes it incompatible with ANSI compilers.
 * If Sun ever changes the definition of these then this will have
 * to change...
 */
#undef _IO
#undef _IOR
#undef _IOW
#undef _IOWR
#undef CDROMPAUSE
#undef CDROMRESUME
#undef CDROMPLAYMSF
#undef CDROMPLAYTRKIND
#undef CDROMREADTOCHDR
#undef CDROMREADTOCENTRY
#undef CDROMSTOP
#undef CDROMSTART
#undef CDROMEJECT
#undef CDROMVOLCTRL
#undef CDROMSUBCHNL

#define _IO(x,y)	(_IOC_VOID | ((x) << 8) | (y))
#define _IOR(x,y,t)	( \
				_IOC_OUT | \
				((sizeof(t) & _IOCPARM_MASK) << 16) | \
				((x) << 8 ) | (y) \
			)
#define _IOW(x,y,t)	( \
				_IOC_IN | \
				((sizeof(t) & _IOCPARM_MASK) << 16) | \
				((x) << 8) | (y) \
			)
#define _IOWR(x,y,t)	( \
				_IOC_INOUT | \
				((sizeof(t) & _IOCPARM_MASK) << 16) | \
				((x) << 8) | (y) \
			)

#define CDROMPAUSE		_IO('c', 10)
#define CDROMRESUME		_IO('c', 11)
#define CDROMPLAYMSF		_IOW('c', 12, struct cdrom_msf)
#define CDROMPLAYTRKIND		_IOW('c', 13, struct cdrom_ti)
#define CDROMREADTOCHDR		_IOR('c', 103, struct cdrom_tochdr)
#define CDROMREADTOCENTRY	_IOWR('c', 104, struct cdrom_tocentry)
#define CDROMSTOP		_IO('c', 105)
#define CDROMSTART		_IO('c', 106)
#define CDROMEJECT		_IO('c', 107)
#define CDROMVOLCTRL		_IOW('c', 14, struct cdrom_volctrl)
#define CDROMSUBCHNL		_IOWR('c', 108, struct cdrom_subchnl)

#endif	/* _SUNOS4 */


/*
 * SunOS 5.x (Solaris) specific definitions
 */
#ifdef _SOLARIS

#include <sys/cdio.h>

#ifndef SOL2_VOLMGT
#define SOL2_VOLMGT		/* Enable Solaris Vol Mgr support */
#endif

#endif	/* _SOLARIS */


/*
 * QNX specific definitions
 */
#ifdef __QNX__
#include <sys/qioctl.h>
#include <sys/cdrom.h>
#endif


/* Ioctl name lookup table structure */
typedef struct {
	int	cmd;
	char	*name;
} iocname_t;


/*
 * Public functions
 */
extern void	slioc_enable(int);
extern void	slioc_disable(int);
extern bool_t	slioc_is_enabled(int);
extern bool_t	slioc_send(int, int, void *, size_t, bool_t, int *);
extern void	slioc_init(curstat_t *, di_tbl_t *);
extern bool_t	slioc_playmode(curstat_t *);
extern bool_t	slioc_check_disc(curstat_t *);
extern void	slioc_status_upd(curstat_t *);
extern void	slioc_lock(curstat_t *, bool_t);
extern void	slioc_repeat(curstat_t *, bool_t);
extern void	slioc_shuffle(curstat_t *, bool_t);
extern void	slioc_load_eject(curstat_t *);
extern void	slioc_ab(curstat_t *);
extern void	slioc_sample(curstat_t *);
extern void	slioc_level(curstat_t *, byte_t, bool_t);
extern void	slioc_play_pause(curstat_t *);
extern void	slioc_stop(curstat_t *, bool_t);
extern void	slioc_chgdisc(curstat_t *);
extern void	slioc_prevtrk(curstat_t *);
extern void	slioc_nexttrk(curstat_t *);
extern void	slioc_previdx(curstat_t *);
extern void	slioc_nextidx(curstat_t *);
extern void	slioc_rew(curstat_t *, bool_t);
extern void	slioc_ff(curstat_t *, bool_t);
extern void	slioc_warp(curstat_t *);
extern void	slioc_route(curstat_t *);
extern void	slioc_mute_on(curstat_t *);
extern void	slioc_mute_off(curstat_t *);
extern void	slioc_cddajitter(curstat_t *);
extern void	slioc_debug(void);
extern void	slioc_start(curstat_t *);
extern void	slioc_icon(curstat_t *, bool_t);
extern void	slioc_halt(curstat_t *);
extern char	*slioc_methodstr(void);

#else	/* DI_SLIOC DEMO_ONLY */

#define slioc_init	NULL

#endif	/* DI_SLIOC DEMO_ONLY */

#endif	/* __SLIOC_H__ */

