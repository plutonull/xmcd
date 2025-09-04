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
 *   Linux CDDA read ioctl support
 */
#ifndef	__RD_LINUX_H__
#define	__RD_LINUX_H__

#ifndef lint
static char *_rd_linux_h_ident_ = "@(#)rd_linux.h	7.12 03/12/12";
#endif

#if defined(CDDA_RD_LINUX) && defined(CDDA_SUPPORTED)

#define LNX_CDDA_MAXFER (32 * 1024)	/* Max CDDA data length per request */

/* Exported function prototypes */
extern word32_t		linux_rinit(void);
extern bool_t		linux_read(di_dev_t *);
extern void		linux_rdone(bool_t);
extern void		linux_rinfo(char *);

#else

#define linux_rinit	NULL
#define linux_read	NULL
#define linux_rdone	NULL
#define linux_rinfo	NULL

#endif	/* CDDA_RD_LINUX CDDA_SUPPORTED */

#endif	/* __RD_LINUX_H__ */

