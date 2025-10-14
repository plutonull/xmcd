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
#ifndef lint
static char *_os_linux_c_ident_ = "@(#)os_linux.c	6.77 04/03/11";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if defined(_LINUX) && defined(DI_SCSIPT) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;


/* This is the same as a portion of the sg_header structure.
 * Defined here because it doesn't exist in earlier kernels.
 */
typedef struct sg_status {
	unsigned int	twelve_byte:1;
	unsigned int	target_status:5;
	unsigned int	host_status:8;
	unsigned int	driver_status:8;
	unsigned int	other_flags:10;
} sg_status_t;


/*
 * find_sg
 *	Given a non-sg CD device path, locate the associated sg device
 *	and return its path.
 *
 * Args:
 *	path - The non-sg device path
 *
 * Return:
 *	The sg device path, or NULL if none found.  The return address
 *	points to a static internal buffer that is overwritten with each
 *	call to this function.
 */
STATIC char *
find_sg(char *path)
{
	int		i,
			fd,
			bus,
			bus2;
	scsi_idlun_t	idlun,
			idlun2;
	struct stat	stbuf;
	static char	retpath[12];

	if ((fd = open(path, O_RDONLY | O_NONBLOCK)) < 0)
		return NULL;

	/* Get the host, channel, id and lun of the device */
	if (ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bus) < 0) {
		/* Older kernels don't support this ioctl, so just fake it */
		bus = 0;
	}

	if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &idlun) < 0)
		return NULL;

	(void) close(fd);

	/* Look for matching sg device */
	for (i = 0; i < MAX_SG_DEVS; i++) {
		/* Check /dev/sg{n} where n is a number */
		(void) sprintf(retpath, "/dev/sg%d", i); 

		if (stat(retpath, &stbuf) < 0) {
			/* Cannot stat device */
			if (errno != ENOENT)
				continue;

			/* Check /dev/sg{n} where n is a letter */
			(void) sprintf(retpath, "/dev/sg%c", 'a' + i); 

			if (stat(retpath, &stbuf) < 0 ||
			    !S_ISCHR(stbuf.st_mode)) {
				/* Cannot stat device or not a character
				 * device
				 */
				continue;
			}
		}
		else if (!S_ISCHR(stbuf.st_mode)) {
			/* Not a character device */
			continue;
		}

		if ((fd = open(retpath, O_RDWR | O_NONBLOCK)) < 0) {
			/* Cannot open device */
			continue;
		}

		if (ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bus2) < 0) {
			/* Older kernels don't support this ioctl,
			 * so just fake it.
			 */
			bus2 = 0;
		}

		if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &idlun2) < 0) {
			(void) close(fd);
			continue;
		}

		if (bus != bus2 ||
		    idlun.four_in_one != idlun2.four_in_one ||
		    idlun.host_unique_id != idlun2.host_unique_id) {
			(void) close(fd);
			continue;
		}

		/* Found it */
		(void) close(fd);
		return (retpath);
	}

	return NULL;
}


/*
 * pthru_send
 *	Send SCSI command to the device.
 *
 * Args:
 *	devp - Device descriptor
 *	role - Role id to send command for
 *	cmdpt - Pointer to the SCSI command CDB
 *	cmdlen - SCSI command size (6, 10 or 12 bytes)
 *	datapt - Pointer to the data buffer
 *	datalen - Data transfer size (bytes)
 *	sensept - Pointer to the sense buffer
 *	senselen - Size of the sense buffer
 *	rw - Data transfer direction flag (OP_NODATA, OP_DATAIN or OP_DATAOUT)
 *	tmout - Command timeout interval (seconds)
 *	prnerr - Whether an error message should be displayed
 *		 when a command fails
 *
 * Return:
 *	TRUE - command completed successfully
 *	FALSE - command failed
 */
bool_t
pthru_send(
	di_dev_t	*devp,
	int		role,
	byte_t		*cmdpt,
	size_t		cmdlen,
	byte_t		*datapt,
	size_t		datalen,
	byte_t		*sensept,
	size_t		senselen,
	byte_t		rw,
	int		tmout,
	bool_t		prnerr
)
{
	byte_t			*ptbuf;
	sg_request_t		*req,
				*rep;
	sg_status_t		*sp;
	req_sense_data_t	*rp;
	int			reqsz,
				ptbufsz,
				ret;
	char			*str,
				title[FILE_PATH_SZ + 20];
	sigset_t		sigs;

	if (devp == NULL || devp->fd <= 0)
		return FALSE;

	if (app_data.debug & DBG_DEVIO) {
		time_t	t = time(NULL);

		(void) sprintf(title,
			       "%sSCSI CDB bytes (dev=%s SG=%d opcode=0x%x rw=%d to=%d role=%d)",
			       asctime(localtime(&t)),
			       devp->path, (devp->flags) & 0x1, cmdpt[0], rw, tmout, role);
		util_dbgdump(title, cmdpt, cmdlen);
	}

	if (devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp,
				"%s: %s %s: %s\n%s=0x%x %s=%d\n",
				APPNAME, "SCSI command error on",
				devp->path, "I/O not enabled for role.",
				"Opcode", cmdpt[0],
				"Role", role);
		return FALSE;
	}

	if (senselen > 0)
		(void) memset(sensept, 0, senselen);

	if ((devp->flags & 0x1) == 0) {
		/* Use SCSI ioctl interface */

#ifdef BUGGY_TEST_UNIT_READY
		/* Old Linux kernel hack: use SCSI_IOCTL_TEST_UNIT_READY
		 * instead of SCSI_IOCTL_SEND_COMMAND for Test Unit Ready
		 * commands.
		 */

		if (cmdpt[0] == OP_S_TEST) {
			DBGPRN(DBG_DEVIO)(errfp,
				"\nSending SCSI_IOCTL_TEST_UNIT_READY (%s)\n",
				devp->path);

			ret = ioctl(devp->fd, SCSI_IOCTL_TEST_UNIT_READY,
				    NULL);
			if (ret == 0)
				return TRUE;

			if (app_data.scsierr_msg && prnerr) {
				(void) fprintf(errfp,
					"SCSI_IOCTL_TEST_UNIT_READY failed: "
					"ret=%d: %s\n",
					ret, strerror(errno)
				);
			}
			return FALSE;
		}
#endif

		/* Set up SCSI pass-through command/data buffer */
		ptbufsz = (sizeof(int) << 1) + cmdlen +
			  sizeof(req_sense_data_t);
		if (datapt != NULL && datalen > 0)
			ptbufsz += datalen;

		if ((ptbuf = (byte_t *) MEM_ALLOC("ptbuf", ptbufsz)) == NULL) {
			DI_FATAL(app_data.str_nomemory);
			return FALSE;
		}

		(void) memset(ptbuf, 0, ptbufsz);
		(void) memcpy(ptbuf + (sizeof(int) << 1), cmdpt, cmdlen);

		if (datapt != NULL && datalen > 0) {
			if (ptbufsz > SCSI_IOCTL_MAX_BUF) {
				DBGPRN(DBG_DEVIO)(errfp,
					"%s: SCSI command error on %s: "
					"I/O data size too large.\n",
					APPNAME, devp->path);
				MEM_FREE(ptbuf);
				return FALSE;
			}

			if (rw == OP_DATAOUT) {
				*((int *) ptbuf) = datalen;
				(void) memcpy(
					ptbuf + (sizeof(int) << 1) + cmdlen,
					datapt,
					datalen
				);
			}
			else
				*(((int *) ptbuf) + 1) = datalen;
		}

		/* Send command */
		ret = ioctl(devp->fd, SCSI_IOCTL_SEND_COMMAND, ptbuf);
		if (ret != 0) {
			rp = (req_sense_data_t *)(void *)
					(ptbuf + (sizeof(int) << 1));
			str = scsipt_reqsense_keystr((int) rp->key);

			if (senselen > 0)
				(void) memcpy(sensept, rp, senselen);

			if (app_data.scsierr_msg && prnerr) {
				(void) fprintf(errfp,
					    "%s: SCSI command error on %s:\n"
					    "%s=0x%02x %s=0x%x %s=0x%x "
					    "%s=0x%x %s=0x%x\n",
					    APPNAME, devp->path,
					    "Opcode", cmdpt[0],
					    "Status", status_byte(ret),
					    "Msg", msg_byte(ret),
					    "Host", host_byte(ret),
					    "Driver", driver_byte(ret));

				(void) fprintf(errfp,
					    "Sense data: Key=0x%x (%s) "
					    "Code=0x%x Qual=0x%x\n",
					    rp->key,
					    str,
					    rp->code,
					    rp->qual);
			}

			MEM_FREE(ptbuf);
			return FALSE;
		}

		if (datapt != NULL && rw == OP_DATAIN && datalen > 0)
			(void) memcpy(datapt, ptbuf + (sizeof(int) << 1),
				      datalen);

		MEM_FREE(ptbuf);
	}
	else {
		/* Use SCSI Generic interface */

		/* Set command timeout interval if needed
		 * Store current timeout in di_dev_t.
		 */
		if (tmout != devp->val) {
			unsigned long	jiffies;

			devp->val = tmout;
			jiffies = (unsigned long) tmout * HZ;

			if (ioctl(devp->fd, SG_SET_TIMEOUT, &jiffies) < 0) {
				(void) fprintf(errfp,
					"%s: SCSI command error on %s: "
					"SG_SET_TIMEOUT: %s\n",
					APPNAME, devp->path, strerror(errno));
				return FALSE;
			}
		}

		/* Allocate request and reply structures */
		req = (sg_request_t *) MEM_ALLOC(
			"sg_request_t", sizeof(sg_request_t)
		);
		if (req == NULL) {
			(void) fprintf(errfp,
					"%s: SCSI command error on %s: "
					"Out of memory\n",
					APPNAME, devp->path);
			return FALSE;
		}
		rep = (sg_request_t *) MEM_ALLOC(
			"sg_request_t", sizeof(sg_request_t)
		);
		if (rep == NULL) {
			(void) fprintf(errfp,
					"%s: SCSI command error on %s: "
					"Out of memory\n",
					APPNAME, devp->path);
			MEM_FREE(req);
			return FALSE;
		}

		(void) memset(req, 0, sizeof(sg_request_t));
		(void) memset(rep, 0, sizeof(sg_request_t));

		req->header.pack_len = sizeof(struct sg_header);
		req->header.reply_len = datalen + sizeof(struct sg_header);
		req->header.pack_id = ++devp->val2;
		req->header.twelve_byte = (int) (cmdlen == 12);

		(void) memcpy(&req->bytes[0], cmdpt, cmdlen);

		if (datalen > 0) {
			if (datalen > SG_BIG_BUFF) {
				DBGPRN(DBG_DEVIO)(errfp,
					"%s: SCSI command error on %s: "
					"I/O data size too large\n",
					APPNAME, devp->path);
				MEM_FREE(req);
				MEM_FREE(rep);
				return FALSE;
			}

			if (datapt != NULL && rw == OP_DATAOUT) {
				(void) memcpy(&req->bytes[cmdlen],
					      datapt, datalen);
			}
		}

		/* Send command */
		if (rw == OP_DATAOUT)
			reqsz = sizeof(struct sg_header) + cmdlen + datalen;
		else
			reqsz = sizeof(struct sg_header) + cmdlen;

		/* Block some signals */
		(void) sigemptyset(&sigs);
		(void) sigaddset(&sigs, SIGINT);
		(void) sigaddset(&sigs, SIGQUIT);
		(void) sigaddset(&sigs, SIGPIPE);
		(void) sigaddset(&sigs, SIGTERM);
		(void) sigaddset(&sigs, SIGCHLD);
		(void) sigprocmask(SIG_BLOCK, &sigs, NULL);

		if ((ret = write(devp->fd, req, reqsz)) < 0) {
			DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s:\n"
				"SG: reqsz:%d ret:%d cmdlen:%d write failure: %s\n",
				APPNAME, devp->path, reqsz,ret,cmdlen,strerror(errno));
			MEM_FREE(req);
			MEM_FREE(rep);
			return FALSE;
		}
		else if (ret != reqsz) {
			DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s:\n"
				"SG: Wrote %d bytes, expected to write %d.\n",
				APPNAME, devp->path, ret, reqsz);
		}

		MEM_FREE(req);

		/* Get command response */
		if (read(devp->fd, rep, sizeof(sg_request_t)) < 0) {
			DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s:\n"
				"SG: reply read failure: %s\n",
				APPNAME, devp->path, strerror(errno));
			MEM_FREE(req);
			MEM_FREE(rep);

			/* Unblock signals */
			(void) sigprocmask(SIG_UNBLOCK, &sigs, NULL);
			return FALSE;
		}

		/* Unblock signals */
		(void) sigprocmask(SIG_UNBLOCK, &sigs, NULL);

		if (rep->header.pack_id != devp->val2) {
			/* The reply data is not valid */
			DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s:\n"
				"SG: packet ID mismatch.\n",
				APPNAME, devp->path);
			MEM_FREE(rep);
			return FALSE;
		}

		sp = (sg_status_t *)(void *)
				(&rep->header.result + sizeof(int));

		if (rep->header.result != 0 || sp->target_status != 0 ||
		    sp->host_status != 0   || sp->driver_status != 0 ||
		    rep->header.sense_buffer[0] != 0) {
			rp = (req_sense_data_t *)(void *)
					&rep->header.sense_buffer;
			str = scsipt_reqsense_keystr((int) rp->key);

			if (senselen > 0)
				(void) memcpy(sensept, rp, senselen);

			if (app_data.scsierr_msg && prnerr) {
				(void) fprintf(errfp,
					    "%s: SCSI command error on %s:\n"
					    "%s=0x%02x %s=0x%x %s=0x%x "
					    "%s=0x%x %s=0x%x\n",
					    APPNAME, devp->path,
					    "Opcode", cmdpt[0],
					    "Result", rep->header.result,
					    "Status", sp->target_status,
					    "Host", sp->host_status,
					    "Driver", sp->driver_status);

				(void) fprintf(errfp,
					    "Sense data: Key=0x%x (%s) "
					    "Code=0x%x Qual=0x%x\n",
					    rp->key,
					    str,
					    rp->code,
					    rp->qual);
			}

			MEM_FREE(rep);
			return FALSE;
		}

		if (datapt != NULL && rw == OP_DATAIN && datalen > 0)
			(void) memcpy(datapt, rep->bytes, datalen);

		MEM_FREE(rep);
	}

	return TRUE;
}


/*
 * pthru_open
 *	Open SCSI pass-through device
 *
 * Args:
 *	path - device path name string
 *
 * Return:
 *	Device descriptor, or NULL on failure.
 */
di_dev_t *
pthru_open(char *path)
{
	int		i,
			mode;
	word32_t	regflag = 0;
	struct stat	stbuf;
	di_dev_t	*devp;
	char		*usepath,
			*auxpath,
			*sgpath,
			errstr[ERR_BUF_SZ],
			buf[SCSI_IOCTL_MAX_BUF];

	/* Check for validity of device node */
	if (stat(path, &stbuf) < 0) {
		(void) sprintf(errstr, app_data.str_staterr, path);
		DI_FATAL(errstr);
		return NULL;
	}
	if (!S_ISBLK(stbuf.st_mode) && !S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(errstr, app_data.str_noderr, path);
		DI_FATAL(errstr);
		return NULL;
	}

	if (S_ISCHR(stbuf.st_mode) &&
	    MAJOR(stbuf.st_rdev) == SCSI_GENERIC_MAJOR) {
		usepath = path;
		auxpath = NULL;
		regflag |= 0x1;
	}
	else if ((sgpath = find_sg(path)) != NULL) {
		usepath = sgpath;
		auxpath = path;
		regflag |= 0x1;
	}
	else {
		usepath = path;
		auxpath = NULL;
		regflag &= ~0x1;
	}

	/* Set open mode */
	if ((regflag & 0x1) == 0) {
		/* Use SCSI ioctl method */
		DBGPRN(DBG_DEVIO)(errfp,
			"Using SCSI ioctl interface on %s\n",
			path);
		mode = O_RDONLY | O_EXCL | O_NONBLOCK;
	}
	else {
		/* Use SCSI generic method */
		DBGPRN(DBG_DEVIO)(errfp,
			"Using SCSI Generic interface for %s: %s\n",
			path, usepath);
		mode = O_RDWR | O_EXCL | O_NONBLOCK;
	}

	sync();

	if ((devp = di_devalloc(usepath)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	devp->flags = regflag;
	if (auxpath != NULL && !util_newstr(&devp->auxpath, auxpath)) {
		di_devfree(devp);
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	if ((devp->fd = open(usepath, mode)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", usepath, errno);
		if (devp->auxpath != NULL)
			MEM_FREE(devp->auxpath);
		di_devfree(devp);
		return NULL;
	}

	if ((regflag & 0x1) != 0) {
		/* Eat any unwanted garbage */
		i = fcntl(devp->fd, F_GETFL);
		(void) fcntl(devp->fd, F_SETFL, i | O_NONBLOCK);
		while (read(devp->fd, buf, sizeof(buf)) != -1 ||
		       errno != EAGAIN)
			;	/* Empty buffer */
		(void) fcntl(devp->fd, F_SETFL, i & ~O_NONBLOCK);
	}

	return (devp);
}


/*
 * pthru_close
 *	Close SCSI pass-through device
 *
 * Args:
 *	devp - Device descriptor
 *
 * Return:
 *	Nothing.
 */
void
pthru_close(di_dev_t *devp)
{
	if (devp->auxpath != NULL)
		MEM_FREE(devp->auxpath);
	if (devp->fd > 0)
		(void) close(devp->fd);
	di_devfree(devp);
}


/*
 * pthru_enable
 *	Enable device in this process for I/O
 *
 * Args:
 *	devp - Device descriptor
 *	role - Role id for which I/O is to be enabled
 *
 * Return:
 *	Nothing.
 */
void
pthru_enable(di_dev_t *devp, int role)
{
	devp->role = role;
}


/*
 * pthru_disable
 *	Disable device in this process for I/O
 *
 * Args:
 *	devp - Device descriptor
 *	role - Role id for which I/O is to be disabled
 *
 * Return:
 *	Nothing.
 */
void
pthru_disable(di_dev_t *devp, int role)
{
	if (devp->role == role)
		devp->role = 0;
}


/*
 * pthru_is_enabled
 *	Check whether device is enabled for I/O in this process
 *
 * Args:
 *	role - Role id for which to check
 *
 * Return:
 *	TRUE  - enabled
 *	FALSE - disabled
 */
bool_t
pthru_is_enabled(di_dev_t *devp, int role)
{
	return ((bool_t) (devp->role == role));
}


/*
 * pthru_maxfer
 *	Return the maximum per-request data transfer length
 *
 * Args:
 *	devp - Device descriptor
 *
 * Return:
 *	The maximum data transfer length in bytes
 */
size_t
pthru_maxfer(di_dev_t *devp)
{
	if ((devp->flags & 0x1) == 0)
		return SCSI_IOCTL_MAX_BUF;	/* SCSI ioctl interface */
	else
		return SG_BIG_BUFF;		/* SCSI Generic interface */
}


/*
 * pthru_vers
 *	Return OS Interface Module version string
 *
 * Args:
 *	None.
 *
 * Return:
 *	Module version text string.
 */
char *
pthru_vers(void)
{
	return ("OS module for Linux");
}

#endif	/* _LINUX DI_SCSIPT DEMO_ONLY */

