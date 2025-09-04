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
 *   IBM AIX Ultimedia Services sound driver support
 */
#ifndef	__WR_AIX_H__
#define	__WR_AIX_H__

#ifndef lint
static char *_wr_aix_h_ident_ = "@(#)wr_aix.h	7.12 03/12/12";
#endif

#if defined(CDDA_WR_AIX) && defined(CDDA_SUPPORTED)

#define DEFAULT_PCIDEV	"/dev/paud0/1"	/* Default PCI audio device */
#define DEFAULT_MCADEV	"/dev/baud0/1"	/* Default MCA audio device */

#define AIX_AUDIO_BSIZE	200		/* Audio buffering in mS */
#define AIX_MAX_VOL	0x7fff		/* Max volume */
#define AIX_HALF_VOL	0x3fff		/* Half volume */

/* Exported function prototypes */
extern word32_t		aix_winit(void);
extern bool_t		aix_write(curstat_t *);
extern void		aix_wdone(bool_t);
extern void		aix_winfo(char *);

#else

#define aix_winit	NULL
#define aix_write	NULL
#define aix_wdone	NULL
#define aix_winfo	NULL

#endif	/* CDDA_WR_AIX CDDA_SUPPORTED */

#endif	/* __WR_AIX_H__ */

