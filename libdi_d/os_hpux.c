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
 *   Author: Ti Kan
 *   Contributing author (m68k portion): Avi Cohen Stuart
 *   E-Mail: <avi@baan.nl>
 *
 *   This software fragment contains code that interfaces the
 *   application to the HP-UX Release 9.x and 10.x operating system.
 *   The name "HP" and "hpux" are used here for identification purposes
 *   only.
 */
#ifndef lint
static char *_os_hpux_c_ident_ = "@(#)os_hpux.c	6.63 04/03/02";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if defined(__hpux) && defined(DI_SCSIPT) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;

#ifdef HPUX_EXCLUSIVE_OPEN
STATIC struct utsname	utsn;
#endif


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
#ifdef __hp9000s300
	/* m68k systems */
	struct scsi_cmd_parms	scp;
	int			n,
				blen;
	byte_t			ch = 0;
	char			*str,
				title[FILE_PATH_SZ + 20];
	req_sense_data_t	*rp,
				sense_data;

	if (devp == NULL || devp->fd <= 0)
		return FALSE;

	if (app_data.debug & DBG_DEVIO) {
		time_t	t = time(NULL);

		(void) sprintf(title,
			       "%sSCSI CDB bytes (dev=%s rw=%d to=%d role=%d)",
			       asctime(localtime(&t)),
			       devp->path, rw, tmout, role);
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

	if (senselen == 0) {
		sensept = (byte_t *) &sense_data;
		senselen = sizeof(sense_data);
	}
	(void) memset(sensept, 0, senselen);

	if (datalen > HPUX_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s: "
				"I/O data size too large.\n",
				APPNAME, devp->path);
		return FALSE;
	}

	(void) memset(&scp, 0, sizeof(scp));

	(void) memcpy(scp.command, cmdpt, cmdlen);
	scp.cmd_type = cmdlen;
	if (rw == OP_DATAIN && datalen > 0)
		scp.cmd_mode = SCTL_READ;
	else
		scp.cmd_mode = 0;
	scp.clock_ticks = (tmout ? tmout : DFLT_CMD_TIMEOUT) * 50;

	/* Set up for sending the SCSI command */
	if (ioctl(devp->fd, SIOC_SET_CMD, &scp) < 0) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
					"%s: %s %s:\n%s=0x%02x: "
					"%s (errno=%d)\n",
					APPNAME,
					"SCSI command error on", devp->path,
					"Opcode", cmdpt[0],
					"SIOC_SET_CMD failed", errno);
		}

		/* Send Request Sense command */
		if (cmdpt[0] != OP_S_RSENSE &&
		    scsipt_request_sense(devp, role, sensept, senselen) &&
		    app_data.scsierr_msg && prnerr) {
			rp = (req_sense_data_t *)(void *) sensept;
			str = scsipt_reqsense_keystr((int) rp->key);

			(void) fprintf(errfp,
				    "Sense data: Key=0x%x (%s) "
				    "Code=0x%x Qual=0x%x\n",
				    rp->key,
				    str,
				    rp->code,
				    rp->qual);
		}
		return FALSE;
	}

	if (datalen == 0) {
		blen = 1;
		datapt = &ch;
	}
	else {
		blen = datalen;
	}

	switch (rw) {
	case OP_NODATA:
	case OP_DATAIN:
		n = read(devp->fd, datapt, blen);
		if (n != blen && n != datalen) {
			if (app_data.scsierr_msg && prnerr) {
				perror("data read failed");
				return FALSE;
			}
		}
		break;
	case OP_DATAOUT:
		n = write(devp->fd, datapt, blen);
		if (n != blen && n != datalen) {
			if (app_data.scsierr_msg && prnerr) {
				perror("data write failed");
				return FALSE;
			}
		}
	default:
		break;
	}

	return TRUE;
#else
	/* PA-RISC Systems */
	struct sctl_io		sctl;
	char			*str,
				title[FILE_PATH_SZ + 20];
	
	if (devp == NULL || devp->fd < 0)
		return FALSE;

	if (app_data.debug & DBG_DEVIO) {
		time_t	t = time(NULL);

		(void) sprintf(title,
			       "%sSCSI CDB bytes (dev=%s rw=%d to=%d role=%d)",
			       asctime(localtime(&t)),
			       devp->path, rw, tmout, role);
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

	if (datalen > HPUX_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s: "
				"I/O data size too large.\n",
				APPNAME, devp->path);
		return FALSE;
	}

	(void) memset(&sctl, 0, sizeof(sctl));

	/* set up sctl_io */
	(void) memcpy(sctl.cdb, cmdpt, cmdlen);
	sctl.cdb_length = cmdlen;
	sctl.data = datapt;
	sctl.data_length = (unsigned) datalen;
	if (rw == OP_DATAIN && datalen > 0)
		sctl.flags = SCTL_READ;
	else
		sctl.flags = 0;
	sctl.max_msecs = (tmout ? tmout : DFLT_CMD_TIMEOUT) * 1000;

	/* Send the command down via the "pass-through" interface */
	if (ioctl(devp->fd, SIOC_IO, &sctl) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("SIOC_IO ioctl failed");
		return FALSE;
	}

	if (sctl.cdb_status != S_GOOD) {
		if (senselen > 0 &&
		    sctl.sense_status == S_GOOD && sctl.sense_xfer > 2) {
			if (senselen > sctl.sense_xfer)
				senselen = sctl.sense_xfer;
			(void) memcpy(sensept, &sctl.sense, senselen);
		}

		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
				    "%s: %s %s:\n%s=0x%x %s=0x%x %s=0x%x\n",
				    APPNAME,
				    "SCSI command error on", devp->path,
				    "Opcode", cmdpt[0],
				    "Cdb_status", sctl.cdb_status,
				    "Sense_status", sctl.sense_status);

			if (sctl.sense_status == S_GOOD &&
			    sctl.sense_xfer > 2) {
				str = scsipt_reqsense_keystr(
					(int) sctl.sense[2] & 0x0f
				);

				(void) fprintf(errfp,
					    "Sense data: Key=0x%x (%s) "
					    "Code=0x%x Qual=0x%x\n",
					    sctl.sense[2] & 0x0f,
					    str,
					    sctl.sense[12],
					    sctl.sense[13]);
			}
		}

		return FALSE;
	}

	return TRUE;
#endif	/* __hp9000s300 */
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
	di_dev_t	*devp;
	struct stat	stbuf;
	char		errstr[ERR_BUF_SZ];

	/* Check for validity of device node */
	if (stat(path, &stbuf) < 0) {
		(void) sprintf(errstr, app_data.str_staterr, path);
		DI_FATAL(errstr);
		return NULL;
	}
	if (!S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(errstr, app_data.str_noderr, path);
		DI_FATAL(errstr);
		return NULL;
	}

	if ((devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

#ifdef __hp9000s300
	/* m68k systems */
	if ((devp->fd = open(path, O_RDWR)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		di_devfree(devp);
		return NULL;
	}

	/* Enable SCSI pass-through mode */
	{
		int	i = 1;

		if (ioctl(devp->fd, SIOC_CMD_MODE, &i) < 0) {
			DBGPRN(DBG_DEVIO)(errfp,
				"Cannot set SIOC_CMD_MODE: errno=%d\n",
				errno);
			pthru_close(devp);
			return NULL;
		}
	}
#else
	/* PA-RISC systems */
	if ((devp->fd = open(path, O_RDONLY)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		di_devfree(devp);
		return NULL;
	}

#ifdef HPUX_EXCLUSIVE_OPEN
	/* Find out the machine type */
	if (uname(&utsn) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"uname(2) failed (errno=%d): assume 9000/7xx\n",
			errno);
		/* Shrug: assume series 7xx */
		(void) strcpy(utsn.machine, "9000/7xx");
	}

	/* Obtain exclusive open (on Series 700 systems only) */
	if (utsn.machine[5] == '7' && ioctl(devp->fd, SIOC_EXCLUSIVE, 1) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot set SIOC_EXCLUSIVE: errno=%d\n", errno);
		(void) pthru_close(devp);
		return NULL;
	}
#endif	/* HPUX_EXCLUSIVE_OPEN */
#endif	/* __hp9000s300 */

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
	if (devp->fd > 0) {
#if !defined(__hp9000s300) && defined(HPUX_EXCLUSIVE_OPEN)
		/* Relinquish exclusive open (on Series 700 systems only) */
		if (utsn.machine[5] == '7')
			(void) ioctl(devp->fd, SIOC_EXCLUSIVE, 0);
#endif
		(void) close(devp->fd);
	}
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
/*ARGSUSED*/
size_t
pthru_maxfer(di_dev_t *devp)
{
	return HPUX_MAX_XFER;
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
	return ("OS module for HP-UX");
}

#endif	/* __hpux DI_SCSIPT DEMO_ONLY */

