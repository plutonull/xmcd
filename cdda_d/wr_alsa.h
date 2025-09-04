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
 *   ALSA (Advanced Linux Sound Architecture) sound driver support
 *   (For ALSA 0.9.x and later only)
 */
#ifndef	__WR_ALSA_H__
#define	__WR_ALSA_H__

#ifndef lint
static char *_wr_alsa_h_ident_ = "@(#)wr_alsa.h	7.10 03/12/12";
#endif

#if defined(CDDA_WR_ALSA) && defined(CDDA_SUPPORTED)

#define DEFAULT_PCM_DEV		"plug:dmix"	/* Default pcm device */
#define DEFAULT_MIXER_DEV	"default"	/* Default mixer device */

#define ALSA_BUFFER_TIME	500000
#define ALSA_PERIOD_TIME	100000

/* Exported function prototypes */
extern word32_t	alsa_winit(void);
extern bool_t	alsa_write(curstat_t *);
extern void	alsa_wdone(bool_t);
extern void	alsa_winfo(char *);

#else

#define alsa_winit	NULL
#define alsa_write	NULL
#define alsa_wdone	NULL
#define alsa_winfo	NULL

#endif	/* CDDA_WR_ALSA CDDA_SUPPORTED */

#endif	/* __WR_AIX_H__ */

