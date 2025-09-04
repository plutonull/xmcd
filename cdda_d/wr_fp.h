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
 *   File and pipe only (no audio playback) write support
 */
#ifndef	__WR_FP_H__
#define	__WR_FP_H__

#ifndef lint
static char *_wr_fp_h_ident_ = "@(#)wr_fp.h	7.13 03/12/12";
#endif

#if defined(CDDA_WR_FP) && defined(CDDA_SUPPORTED)

/* Exported function prototypes */
extern word32_t		fp_winit(void);
extern bool_t		fp_write(curstat_t *);
extern void		fp_wdone(bool_t);
extern void		fp_winfo(char *);

#else

#define fp_winit	NULL
#define fp_write	NULL
#define fp_wdone	NULL
#define fp_winfo	NULL

#endif	/* CDDA_WR_FP CDDA_SUPPORTED */

#endif	/* __WR_FP_H__ */

