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
 *   SunOS and Solaris support
 *
 *   This software fragment contains code that interfaces the
 *   application to the SunOS operating systems.  The name "Sun"
 *   and "SunOS" are used here for identification purposes only.
 */
#ifndef __OS_SUN_H__
#define __OS_SUN_H__

#if (defined(_SUNOS4) || defined(_SOLARIS)) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_sun_h_ident_ = "@(#)os_sun.h	6.34 04/01/14";
#endif

#ifdef _SOLARIS

#include <sys/scsi/impl/uscsi.h>
#include <sys/dkio.h>

#ifndef SOL2_VOLMGT
#define SOL2_VOLMGT			/* Enable Solaris Vol Mgr support */
#endif

#define USCSI_STATUS_GOOD	0

#else	/* _SUNOS4 */

#include <scsi/impl/uscsi.h>
#undef USCSI_WRITE
#define USCSI_WRITE		0

/* This is a hack to work around a bug in SunOS 4.x's _IOWR macro
 * in <sys/ioccom.h> which makes it incompatible with ANSI compilers.
 * If Sun ever changes the definition of USCSICMD or _IOWR then
 * this will have to change...
 */
#undef _IOWR
#undef USCSICMD

#define _IOWR(x,y,t)	( \
				_IOC_INOUT | \
				((sizeof(t) & _IOCPARM_MASK) << 16) | \
				((x) << 8) | (y) \
			)
#define USCSICMD	_IOWR('u', 1, struct uscsi_cmd)

#endif	/* _SOLARIS */


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */
#define SETUID_ROOT	/* Setuid root privilege is required */

/* Arch codes */
#define ARCH_MASK	0xF0
#define ARCH_SUN2	0x00
#define ARCH_SUN3	0x10
#define ARCH_SUN4	0x20
#define ARCH_SUN386	0x30
#define ARCH_SUN3X	0x40
#define ARCH_SUN4C	0x50
#define ARCH_SUN4E	0x60
#define ARCH_SUN4M	0x70
#define ARCH_SUNX	0x80

/* Max per-request data lengths */
#define SUN4C_MAX_XFER	(124 * 1024)
#define SUN4M_MAX_XFER	(124 * 1024)
#define SUN386_MAX_XFER	(56 * 1024)
#define SUN3_MAX_XFER	(63 * 1024)


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

#ifdef _SOLARIS
extern bool_t	sol2_volmgt_eject(di_dev_t *);
#endif

#endif	/* _SUNOS4 _SOLARIS DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __OS_SUN_H__ */

