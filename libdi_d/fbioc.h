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
#ifndef __FBIOC_H__
#define __FBIOC_H__

#if defined(DI_FBIOC) && !defined(DEMO_ONLY)

#ifndef lint
static char *_fbioc_h_ident_ = "@(#)fbioc.h	6.27 04/04/06";
#endif

#include <sys/cdio.h>

#ifndef CDIOCREADAUDIO
#define CDIOCREADAUDIO	_IOWR('c',31,struct ioc_read_audio)

struct ioc_read_audio
{
	u_char          address_format;
	union msf_lba	address;
	int		nframes;
	u_char		*buffer;
};
#endif  /* CDIOCREADAUDIO */

/* Ioctl name lookup table structure */
typedef struct {
	unsigned int	cmd;
	char		*name;
} iocname_t;

/*
 * Public functions
 */
extern void	fbioc_enable(int);
extern void	fbioc_disable(int);
extern bool_t	fbioc_is_enabled(int);
extern bool_t	fbioc_send(int, unsigned int, void *, bool_t);
extern void	fbioc_init(curstat_t *, di_tbl_t *);
extern bool_t	fbioc_playmode(curstat_t *);
extern bool_t	fbioc_check_disc(curstat_t *);
extern void	fbioc_status_upd(curstat_t *);
extern void	fbioc_lock(curstat_t *, bool_t);
extern void	fbioc_repeat(curstat_t *, bool_t);
extern void	fbioc_shuffle(curstat_t *, bool_t);
extern void	fbioc_load_eject(curstat_t *);
extern void	fbioc_ab(curstat_t *);
extern void	fbioc_sample(curstat_t *);
extern void	fbioc_level(curstat_t *, byte_t, bool_t);
extern void	fbioc_play_pause(curstat_t *);
extern void	fbioc_stop(curstat_t *, bool_t);
extern void	fbioc_chgdisc(curstat_t *);
extern void	fbioc_prevtrk(curstat_t *);
extern void	fbioc_nexttrk(curstat_t *);
extern void	fbioc_previdx(curstat_t *);
extern void	fbioc_nextidx(curstat_t *);
extern void	fbioc_rew(curstat_t *, bool_t);
extern void	fbioc_ff(curstat_t *, bool_t);
extern void	fbioc_warp(curstat_t *);
extern void	fbioc_route(curstat_t *);
extern void	fbioc_mute_on(curstat_t *);
extern void	fbioc_mute_off(curstat_t *);
extern void	fbioc_cddajitter(curstat_t *);
extern void	fbioc_debug(void);
extern void	fbioc_start(curstat_t *);
extern void	fbioc_icon(curstat_t *, bool_t);
extern void	fbioc_halt(curstat_t *);
extern char	*fbioc_methodstr(void);

#else	/* DI_FBIOC DEMO_ONLY */

#define fbioc_init	NULL

#endif	/* DI_FBIOC DEMO_ONLY */

#endif	/* __FBIOC_H__ */

