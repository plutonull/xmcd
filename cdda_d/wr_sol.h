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
 *   Sun Solaris support
 *
 *   This software fragment contains code that interfaces the
 *   application to the Solaris operating environment.  The name "Sun"
 *   and "Solaris" are used here for identification purposes only.
 *
 *   Contributing author: Darragh O'Brien <darragh.obrien@sun.com>
 */
#ifndef	__WR_SOL_H__
#define	__WR_SOL_H__

#ifndef lint
static char *_wr_sol_h_ident_ = "@(#)wr_sol.h	7.34 03/12/12";
#endif

#if defined(CDDA_WR_SOL) && defined(CDDA_SUPPORTED)

#define DEFAULT_DEV_AUDIO	"/dev/audio"	/* default audio device */

#define SOL_MAX_VOL		AUDIO_MAX_GAIN
#define SOL_HALF_VOL		(SOL_MAX_VOL / 2)
#define SOL_M_BAL		AUDIO_MID_BALANCE
#define SOL_L_BAL		0
#define SOL_R_BAL		AUDIO_RIGHT_BALANCE

/* Adapted from <sys/audio.h> and put here because audio.h is not found
 * on all systems.  Renamed from AUDIO_INIT to avoid clash with other
 * platforms.
 */
#define SOL_AUDIO_INIT(I, S) {						\
		byte_t *__x__;						\
		for (__x__ = (byte_t *)(I);				\
			__x__ < (((byte_t *)(I)) + (S));		\
				*__x__++ = (byte_t)~0);		\
		}

/* Arch codes */
#define ARCH_MASK       0xF0
#define ARCH_SUN2       0x00
#define ARCH_SUN3       0x10
#define ARCH_SUN4       0x20
#define ARCH_SUN386     0x30
#define ARCH_SUN3X      0x40
#define ARCH_SUN4C      0x50
#define ARCH_SUN4E      0x60
#define ARCH_SUN4M      0x70
#define ARCH_SUNX       0x80

/* Exported function prototypes */
extern word32_t		sol_winit(void);
extern bool_t		sol_write(curstat_t *);
extern void		sol_wdone(bool_t);
extern void		sol_winfo(char *);

#else

#define sol_winit	NULL
#define sol_write	NULL
#define sol_wdone	NULL
#define sol_winfo	NULL

#endif	/* CDDA_WR_SOL CDDA_SUPPORTED */

#endif	/* __WR_SOL_H__ */

