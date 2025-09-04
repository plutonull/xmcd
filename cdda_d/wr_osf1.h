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
 *   Digital/Compaq/HP Tru64 UNIX (OSF1) MMS audio driver support
 */
#ifndef	__WR_OSF1_H__
#define	__WR_OSF1_H__

#ifndef lint
static char *_wr_osf1_h_ident_ = "@(#)wr_osf1.h	7.10 04/02/25";
#endif

#if defined(CDDA_WR_OSF1) && defined(CDDA_SUPPORTED)

#define MME_MAX_VOL	0xffff		/* Max MME wave device volume */

/* Exported function prototypes */
extern word32_t	osf1_winit(void);
extern bool_t	osf1_write(curstat_t *);
extern void	osf1_wdone(bool_t);
extern void	osf1_winfo(char *);

#else

#define osf1_winit	NULL
#define osf1_write	NULL
#define osf1_wdone	NULL
#define osf1_winfo	NULL

#endif	/* CDDA_WR_OSF1 CDDA_SUPPORTED */

#endif	/* __WR_OSF1_H__ */

