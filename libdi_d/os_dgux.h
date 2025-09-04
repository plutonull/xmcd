/*
 *   libdi - scsipt SCSI Device Interface Library
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
 *   Data General DG/UX support
 *
 *   Contributing author: Karl Owen
 *   E-mail: owen@dg-rtp.dg.com
 *
 *   This software fragment contains code that interfaces the
 *   application to the Data General DG/UX operating system.
 *   The name "Data General" and DG/UX are used here for identification
 *   purposes only.
 */
#ifndef __OS_DGUX_H__
#define __OS_DGUX_H__

#if defined(DGUX) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_dgux_h_ident_ = "@(#)os_dgux.h	6.24 04/01/14";
#endif


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */

#define DGUX_MAX_XFER	(16 * 1024)	/* Max per-request data length */


/* Public function prototypes */
extern bool_t	pthru_send(di_dev_t *, int, byte_t *, size_t, byte_t *, size_t,
			   byte_t *, size_t, byte_t, int, bool_t);
extern di_dev_t	*pthru_open(char *);
extern void	pthru_close(di_dev_t *);
extern void	pthru_enable(di_dev_t *, int);
extern void	pthru_disable(di_dev_t *, int);
extern bool_t	pthru_is_enabled(di_dev_t *, int);
extern size_t	pthru_maxfer(di_dev_t *);
extern char	*pthru_vers(void);

#endif	/* DGUX DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __OS_DGUX_H__ */

