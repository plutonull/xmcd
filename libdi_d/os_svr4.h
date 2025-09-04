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
#ifndef __OS_SVR4_H__
#define __OS_SVR4_H__

#if (defined(SVR4) || defined(SVR5)) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_svr4_h_ident_ = "@(#)os_svr4.h	6.31 04/01/14";
#endif

#if defined(_UNIXWARE) || (defined(_FTX) && defined(__hppa))
/*
 *   UNIX SVR4.x/x86, SVR5/x86 support (UnixWare, Caldera Open UNIX 8)
 *   Stratus UNIX SVR4/PA-RISC FTX 3.x support
 *   Portable Device Interface (PDI) / Storage Device Interface (SDI)
 *
 *   This software fragment contains code that interfaces the
 *   application to the UNIX System V Release 4.x (AT&T, USL,
 *   Univel/Novell/SCO UnixWare) and UNIX System V Release 5
 *   (SCO UnixWare 7, Caldera Open UNIX 8) operating systems
 *   for the Intel x86 hardware platforms and Stratus FTX 3.x on
 *   the Continuum systems.  The company and product names used here
 *   are for identification purposes only.
 */

#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */
#define SETUID_ROOT	/* Setuid root privilege is required */

#define PDI_MAX_XFER	(16 * 1024)	/* Max per-request data length */


#endif	/* _UNIXWARE _FTX __hppa */


#ifdef MOTOROLA
/*
 *   Motorola 88k UNIX SVR4 support
 *
 *   This software fragment contains code that interfaces the
 *   application to the System V Release 4 operating system from
 *   Motorola.  The name "Motorola" is used here for identification
 *   purposes only.
 */

#include <sys/param.h>
#include <sys/dk.h>


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */
#define SETUID_ROOT	/* Setuid root privilege is required */

#define MOT_MAX_XFER	(16 * 1024)	/* Max per-request data length */


#endif	/* MOTOROLA */


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

#endif	/* SVR4 SVR5 DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __OS_SVR4_H__ */

