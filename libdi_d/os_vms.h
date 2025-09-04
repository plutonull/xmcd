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
 *   Digital OpenVMS support
 *
 *   Contributing author: Rick Jones
 *   E-Mail: rjones@zko.dec.com
 *
 *   This software fragment contains code that interfaces the
 *   application to the Digital OpenVMS operating system.
 *   The terms Digital and OpenVMS are used here for identification
 *   purposes only.
 */
#ifndef __OS_VMS_H__
#define __OS_VMS_H__

#if defined(__VMS) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_vms_h_ident_ = "@(#)os_vms.h	6.31 04/01/14";
#endif

#include <fscndef>
#include <ssdef>
#include <iodef.h>
#include <dvidef.h>
#include <devdef.h>
#ifdef __alpha
#define __NEW_STARLET
#include <iledef.h>
#endif


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */

#define VMS_MAX_XFER	(32 * 1024)	/* Max per-request data length */

#define FLAGS_DISCONNECT        2
#define SCSI_STATUS_MASK        0x3e
#define GOOD_SCSI_STATUS        0
#define ATAPI_CMDLEN		12


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

#endif	/* __VMS DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __OS_VMS_H__ */

