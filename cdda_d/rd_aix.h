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
 *   IBM AIX ATAPI/IDE CDDA read ioctl support
 */
#ifndef	__RD_AIX_H__
#define	__RD_AIX_H__

#ifndef lint
static char *_rd_aix_h_ident_ = "@(#)rd_aix.h	7.11 03/12/12";
#endif

#if defined(CDDA_RD_AIX) && defined(CDDA_SUPPORTED)

#define AIX_CDDA_MAXFER	(32 * 1024)	/* Max CDDA data length per request */

/* Exported function prototypes */
extern word32_t		aix_rinit(void);
extern bool_t		aix_read(di_dev_t *);
extern void		aix_rdone(bool_t);
extern void		aix_rinfo(char *);

#else

#define aix_rinit	NULL
#define aix_read	NULL
#define aix_rdone	NULL
#define aix_rinfo	NULL

#endif	/* CDDA_RD_AIX CDDA_SUPPORTED */

#endif	/* __RD_AIX_H__ */

