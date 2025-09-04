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
 *   Linux support
 *
 *   This software fragment contains code that interfaces the
 *   application to the Linux operating system.
 */
#ifndef __OS_LINUX_H__
#define __OS_LINUX_H__

#if defined(_LINUX) && \
    (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && \
    !defined(DEMO_ONLY)

#ifndef lint
static char *_os_linux_h_ident_ = "@(#)os_linux.h	6.32 04/01/14";
#endif

#include <sys/param.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

/* Command result word macros */
#ifndef status_byte
#define status_byte(result)		(((result) >> 1) & 0xf)
#endif
#ifndef msg_byte
#define msg_byte(result)		(((result) >> 8) & 0xff)
#endif
#ifndef host_byte
#define host_byte(result)		(((result) >> 16) & 0xff)
#endif
#ifndef driver_byte
#define driver_byte(result)		(((result) >> 24) & 0xff)
#endif

/* Linux SCSI ioctl commands */
#ifndef SCSI_IOCTL_SEND_COMMAND
#define SCSI_IOCTL_SEND_COMMAND		1
#endif
#ifndef SCSI_IOCTL_TEST_UNIT_READY
#define SCSI_IOCTL_TEST_UNIT_READY	2
#endif
#ifndef SCSI_IOCTL_GET_IDLUN
#define SCSI_IOCTL_GET_IDLUN		0x5382
#endif
#ifndef SCSI_IOCTL_GET_BUS_NUMBER
#define SCSI_IOCTL_GET_BUS_NUMBER	0x5386
#endif

/* Maximum data transfer length for ioctl mode.  This should correspond
 * to MAX_BUF in the Linux kernel drivers/scsi/scsi_ioctl.c file.
 */
#define SCSI_IOCTL_MAX_BUF		4096

/* Maximum data transfer length for SCSI generic mode.  This should be
 * defined in <scsi/sg.h>, but for old kernels this may not be the case.
 */
#ifndef SG_BIG_BUFF
#define SG_BIG_BUFF			32768
#endif

/* Max number of SCSI generic devices supported */
#define MAX_SG_DEVS			26


/* Structure used with the SCSI_IOCTL_GET_IDLUN ioctl */
typedef struct scsi_idlun {
	int			four_in_one;
	int			host_unique_id;
} scsi_idlun_t;


/* SCSI Generic request and response structure */
typedef struct sg_request {
	struct sg_header	header;
	byte_t			bytes[SG_BIG_BUFF];
} sg_request_t;


#define OS_MODULE	/* Indicate that this is compiled on a supported OS */
#define SETUID_ROOT	/* Setuid root privilege is required */


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

#endif	/* _LINUX DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __OS_LINUX_H__ */

