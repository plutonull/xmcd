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
 *   OSS (Open Sound System) and Linux sound driver support
 *   The Linux ALSA sound driver is also supported if the OSS emulation
 *   module is loaded.
 */
#ifndef	__WR_OSS_H__
#define	__WR_OSS_H__

#ifndef lint
static char *_wr_oss_h_ident_ = "@(#)wr_oss.h	7.20 03/12/12";
#endif

#if defined(CDDA_WR_OSS) && defined(CDDA_SUPPORTED)

#define DEFAULT_DEV_DSP		"/dev/dsp"	/* Default audio device */
#define DEFAULT_DEV_MIXER	"/dev/mixer"	/* Mixer device */

/* Exported function prototypes */
extern word32_t		oss_winit(void);
extern bool_t		oss_write(curstat_t *);
extern void		oss_wdone(bool_t);
extern void		oss_winfo(char *);

#else

#define oss_winit	NULL
#define oss_write	NULL
#define oss_wdone	NULL
#define oss_winfo	NULL

#endif	/* CDDA_WR_OSS CDDA_SUPPORTED */

#endif	/* __WR_OSS_H__ */

