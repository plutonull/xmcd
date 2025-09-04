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
 *   FreeBSD/NetBSD/OpenBSD support
 *
 *   Contributing author: Gennady B. Sorokopud (SCIOCCOMMAND version)
 *   E-Mail: gena@netvision.net.il
 *
 *   Contributing author: Kenneth D. Merry (CAM version)
 *   E-Mail: ken@kdm.org
 *
 *   This software fragment contains code that interfaces the
 *   application to the FreeBSD (version 2.0.5 or later), NetBSD
 *   (version 1.0A or later) and OpenBSD (2.x and later) operating
 *   systems.
 *
 *   SCIOCCOMMAND notes:
 *   The SCIOCCOMMAND code is used for FreeBSD 2.x, NetBSD and OpenBSD.
 *   Generic SCSI-support is required in the kernel configuration file.
 *   Also be sure "SCIOCCOMMAND" in the file "/usr/include/sys/scsiio.h"
 *   is not just a dummy.
 *
 *   CAM notes:
 *   The CAM code is only used for FreeBSD 3.x and later.  It is enabled
 *   when compiled with -DFREEBSD_CAM.
 */
#ifndef __OS_FNBSD_H__
#define __OS_FNBSD_H__

#if (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_fnbsd_h_ident_ = "@(#)os_fnbsd.h	6.30 04/01/14";
#endif

#ifdef FREEBSD_CAM
#include <cam/cam.h>
#include <cam/cam_ccb.h>
#include <cam/scsi/scsi_message.h>
#include <camlib.h>
#else
#include <sys/scsiio.h>
#endif


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */
#define SETUID_ROOT	/* Setuid root privilege is required */

#define BSD_MAX_XFER	(32 * 1024)	/* Max per-request data length */


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

#endif	/* __FreeBSD__ __NetBSD__ __OpenBSD__
	 * DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY
	 */

#endif	/* __OS_FNBSD_H__ */

