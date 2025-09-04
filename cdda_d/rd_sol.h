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
#ifndef	__RD_SOL_H__
#define	__RD_SOL_H__

#ifndef lint
static char *_rd_sol_h_ident_ = "@(#)rd_sol.h	7.24 03/12/12";
#endif

#if defined(CDDA_RD_SOL) && defined(CDDA_SUPPORTED)

#define SOL_CDDA_MAXFER (256 * 1024)	/* Max CDDA data length per request */

/* Exported function prototypes */
extern word32_t		sol_rinit(void);
extern bool_t		sol_read(di_dev_t *);
extern void		sol_rdone(bool_t);
extern void		sol_rinfo(char *);

#else

#define sol_rinit	NULL
#define sol_read	NULL
#define sol_rdone	NULL
#define sol_rinfo	NULL

#endif	/* CDDA_RD_SOL CDDA_SUPPORTED */

#endif	/* __RD_SOL_H__ */

