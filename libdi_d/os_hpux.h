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
 *   HP-UX support
 *
 *   This software fragment contains code that interfaces the
 *   application to the HP-UX Release 9.x and 10.x operating system.
 *   The name "HP" and "hpux" are used here for identification purposes
 *   only.
 */
#ifndef __OS_HPUX_H__
#define __OS_HPUX_H__

#if defined(__hpux) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_hpux_h_ident_ = "@(#)os_hpux.h	6.25 04/01/14";
#endif

#include <sys/utsname.h>
#include <sys/scsi.h>


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */
#define SETUID_ROOT	/* Setuid root privilege is required */

#ifdef SCSI_MAXPHYS	/* Max per-request data length */
#define HPUX_MAX_XFER	SCSI_MAXPHYS
#else
#define HPUX_MAX_XFER	(32 * 1024)
#endif


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

#endif	/* __hpux DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __OS_HPUX_H__ */

