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
 *   BSDI BSD/OS support
 *
 *   Contributing author for BSD/OS 2.x: Danny Braniss
 *   E-Mail: danny@cs.huji.ac.il
 *
 *   Contributing author for BSD/OS 3.x: Chris P. Ross
 *   E-Mail: cross@va.pubnix.com
 *
 *   This software fragment contains code that interfaces the
 *   application to the BSDI BSD/OS (version 2.0 or later)
 *   operating system.
 */
#ifndef lint
static char *_os_bsdi_c_ident_ = "@(#)os_bsdi.c	6.63 04/03/02";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if defined(__bsdi__) && defined(DI_SCSIPT) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;


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
/*ARGSUSED*/
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
#ifdef OS_BSDI_2
	/* BSD/OS 2.x */
	int			blen,
				n;
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

	if (datalen > BSDI_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: SCSI command error on %s: "
			"I/O data size too large.\n",
			APPNAME, devp->path);
		return FALSE;
	}

	/* Make sure that this command is supported by the driver */
	n = (int) cmdpt[0];
	if (ioctl(devp->fd, SDIOCADDCOMMAND, &n) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("SDIOCADDCOMMAND ioctl failed");
		return FALSE;
	}

	/* Send the command down via the "pass-through" interface */
	if (ioctl(devp->fd, SDIOCSCSICOMMAND, &cmdpt) < 0) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
					"%s: %s %s:\n%s=0x%02x: "
					"%s (errno=%d)\n",
					APPNAME,
					"SCSI command error on", devp->path,
					"Opcode", cmdpt[0],
					"SDIOCSCSICOMMAND failed", errno);
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
			if (app_data.scsierr_msg && prnerr)
				perror("data read failed");
			return FALSE;
		}
		break;
	case OP_DATAOUT:
		n = write(devp->fd, datapt, blen);
		if (n != blen && n != datalen) {
			if (app_data.scsierr_msg && prnerr)
				perror("data write failed");
			return FALSE;
		}
		break;
	default:
		break;
	}

	return TRUE;
#endif	/* OS_BSDI_2 */

#ifdef OS_BSDI_3
	/* BSD/OS 3.x */
	scsi_user_cdb_t		suc;
	char			*str,
				title[FILE_PATH_SZ + 20];
	req_sense_data_t	*rp,
				sense_data;

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

	if (senselen == 0) {
		sensept = (byte_t *) &sense_data;
		senselen = sizeof(sense_data);
	}

	(void) memset(sensept, 0, senselen);
	(void) memset(&suc, 0, sizeof(scsi_user_cdb_t));
	(void) memcpy(suc.suc_cdb, cmdpt, cmdlen);
	suc.suc_cdblen = cmdlen;
	suc.suc_datalen = datalen;
	suc.suc_data = (caddr_t) datapt;
	suc.suc_timeout = (u_int) (tmout ? tmout : DFLT_CMD_TIMEOUT);

	if (datalen > BSDI_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: SCSI command error on %s: "
			"I/O data size too large.\n",
			APPNAME, devp->path);
		return FALSE;
	}

	switch (rw) {
	case OP_NODATA:
	case OP_DATAIN:
		suc.suc_flags = SUC_READ;
		break;
	case OP_DATAOUT:
		suc.suc_flags = SUC_WRITE;
		break;
	default:
		break;
	}

	/* Send the command down via the "pass-through" interface */
	if (ioctl(devp->fd, SCSIRAWCDB, &suc) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("SCSIRAWCDB ioctl failed");
		return FALSE;
	}

	if (suc.suc_sus.sus_status != 0) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
					"%s: %s %s:\n%s=0x%02x %s=0x%02x\n",
					APPNAME,
					"SCSI command error on", devp->path,
					"Opcode", cmdpt[0],
					"Status", suc.suc_sus.sus_status);
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

	return TRUE;
#endif	/* OS_BSDI_3 */
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
#ifdef OS_BSDI_2
	int		val;
#endif

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

	if ((devp->fd = open(path, O_RDWR | O_NONBLOCK)) < 0 &&
	    (devp->fd = open(path, O_RDONLY | O_NONBLOCK)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		di_devfree(devp);
		return NULL;
	}

#ifdef OS_BSDI_2
	/* BSD/OS 2.x */

	/* Query if the driver is already in "format mode" */
	val = 0;
	if (ioctl(devp->fd, SDIOCGFORMAT, &val) < 0) {
		perror("SDIOCGFORMAT ioctl failed");
		pthru_close(devp);
		return NULL;
	}

	if (val != 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Device %s is in use by process %d\n", path, val);
		pthru_close(devp);
		return NULL;
	}

	/* Put the driver in "format mode" for SCSI pass-through */
	val = 1;
	if (ioctl(devp->fd, SDIOCSFORMAT, &val) < 0) {
		perror("SDIOCSFORMAT ioctl failed");
		pthru_close(devp);
		return NULL;
	}
#endif

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
#ifdef OS_BSDI_2
		/* BSD/OS 2.x */
		int	val = 0;

		/* Put the driver back to normal mode */
		(void) ioctl(devp->fd, SDIOCSFORMAT, &val);
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
	return BSDI_MAX_XFER;
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
#ifdef OS_BSDI_2
	return ("OS module for BSDI BSD/OS 2.x");
#endif
#ifdef OS_BSDI_3
	return ("OS module for BSDI/WindRiver BSD/OS 3.x and later");
#endif
}

#endif	/* __bsdi__ DI_SCSIPT DEMO_ONLY */

