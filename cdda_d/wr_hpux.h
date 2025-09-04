/*
 *   cdda - CD Digital Audio support
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

/*
 *   HP-UX audio driver support
 */
#ifndef	__WR_HPUX_H__
#define	__WR_HPUX_H__

#ifndef lint
static char *_wr_hpux_h_ident_ = "@(#)wr_hpux.h	7.11 03/12/12";
#endif

#if defined(CDDA_WR_HPUX) && defined(CDDA_SUPPORTED)

#define DEFAULT_DEV_AUDIO	"/dev/audio"	/* Default audio device */

#define HPUX_MAX_VOL	((AUDIO_MAX_GAIN) - (AUDIO_OFF_GAIN))
#define HPUX_HALF_VOL	(HPUX_MAX_VOL / 2)
#define HPUX_VOL_OFFSET	(0 - (AUDIO_OFF_GAIN))

/* Exported function prototypes */
extern word32_t		hpux_winit(void);
extern bool_t		hpux_write(curstat_t *);
extern void		hpux_wdone(bool_t);
extern void		hpux_winfo(char *);

#else

#define hpux_winit	NULL
#define hpux_write	NULL
#define hpux_wdone	NULL
#define hpux_winfo	NULL

#endif	/* CDDA_WR_HPUX CDDA_SUPPORTED */

#endif	/* __WR_HPUX_H__ */

