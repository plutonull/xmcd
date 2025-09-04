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
 *   (version 1.0A or later) and OpenBSD (version 2.x and later)
 *   operating systems.
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
#ifndef lint
static char *_os_fnbsd_c_ident_ = "@(#)os_fnbsd.c	6.63 04/03/06";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && \
    defined(DI_SCSIPT) && !defined(DEMO_ONLY)

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
#ifdef FREEBSD_CAM
	struct cam_device	*cam_dev;
	union ccb		ccb;
#else
	static struct scsireq	ucmd;
#endif
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

	if (datalen > BSD_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s: "
				"I/O data size too large.\n",
				APPNAME, devp->path);
		return FALSE;
	}

#ifdef FREEBSD_CAM
	/* CAM method */
	(void) memset(&ccb, 0, sizeof(ccb));
	cam_dev = (struct cam_device *)(void *) devp->auxdesc;

	cam_fill_csio(
		&ccb.csio,
		0,			/* retries */
		NULL,			/* cbfcnp */
		(rw == OP_DATAIN) ? CAM_DIR_IN :
			((rw == OP_DATAOUT) ? CAM_DIR_OUT : CAM_DIR_NONE),
					/* flags */
		MSG_SIMPLE_Q_TAG,	/* tag_action */
		(u_int8_t *) datapt,	/* data_ptr */
		datalen,		/* dxfer_len */
		SSD_FULL_SIZE,		/* sense_len */
		cmdlen,			/* cdb_len */
		(tmout ? tmout : DFLT_CMD_TIMEOUT) * 1000
					/* timeout */
	);

	/* Disable freezing the device queue */
	ccb.ccb_h.flags |= CAM_DEV_QFRZDIS;

	(void) memcpy(ccb.csio.cdb_io.cdb_bytes, cmdpt, cmdlen);

	/* Send the command down via the CAM interface */
	if (cam_send_ccb(cam_dev, &ccb) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("cam_send_ccb failed");
		return FALSE;
	}

	if ((ccb.ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp, "%s: %s %s: %s=0x%x\n",
				       APPNAME,
				       "SCSI command error on", devp->path,
				       "Status", ccb.ccb_h.status);
		}

		if ((ccb.ccb_h.status & CAM_STATUS_MASK) ==
			CAM_SCSI_STATUS_ERROR &&
		    (ccb.ccb_h.status & CAM_AUTOSNS_VALID) != 0) {
			rp = (req_sense_data_t *)(void *) sensept;

			if (senselen > (size_t) ccb.csio.sense_len)
				senselen = (size_t) ccb.csio.sense_len;

			(void) memcpy(rp, &ccb.csio.sense_data, senselen);

			if (app_data.scsierr_msg && prnerr) {
				str = scsipt_reqsense_keystr((int) rp->key);

				(void) fprintf(errfp,
					    "Sense data: Key=0x%x (%s) "
					    "Code=0x%x Qual=0x%x\n",
					    rp->key,
					    str,
					    rp->code,
					    rp->qual);
			}
		}
		return FALSE;
	}
#else
	/* SCIOCCOMMAND ioctl method */
	(void) memset(&ucmd, 0, sizeof(ucmd));

	/* set up uscsi_cmd */
	(void) memcpy(ucmd.cmd, cmdpt, cmdlen);
	ucmd.cmdlen = cmdlen;
	ucmd.databuf = (caddr_t) datapt;	/* data buffer */
	ucmd.datalen = datalen;			/* buffer length */
	ucmd.timeout = (tmout ? tmout : DFLT_CMD_TIMEOUT) * 1000;
	if (datalen != 0) {
		switch (rw) {
		case OP_DATAIN:
			ucmd.flags |= SCCMD_READ;
			break;
		case OP_DATAOUT:
			ucmd.flags |= SCCMD_WRITE;
			break;
		case OP_NODATA:
		default:
			break;
		}
	}

	/* Send the command down via the "pass-through" interface */
	if (ioctl(devp->fd, SCIOCCOMMAND, &ucmd) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("SCIOCCOMMAND ioctl failed");
		return FALSE;
	}

	if (ucmd.retsts != SCCMD_OK) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
				       "%s: %s %s:\n%s=0x%x %s=0x%x\n",
				       APPNAME,
				       "SCSI command error on", devp->path,
				       "Opcode", cmdpt[0],
				       "Status", ucmd.retsts);
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
#endif	/* FREEBSD_CAM */

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
#ifdef FREEBSD_CAM
	/* CAM method */
	di_dev_t		*devp;
	struct cam_device	*cam_dev;

	if ((devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	if ((cam_dev = cam_open_device(path, O_RDWR)) == NULL) {
		DBGPRN(DBG_DEVIO)(errfp, "%s\n", cam_errbuf);
		di_devfree(devp);
		return NULL;
	}

	devp->fd = cam_dev->fd;
	devp->auxdesc = (void *) cam_dev;

	return (devp);
#else
	/* SCIOCCOMMAND ioctl method */
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

	if ((devp->fd = open(path, O_RDWR)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		di_devfree(devp);
		return NULL;
	}

	return (devp);
#endif	/* FREEBSD_CAM */
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
#ifdef FREEBSD_CAM
	struct cam_device	*cam_dev;

	/* CAM method */
	cam_dev = (struct cam_device *)(void *) devp->auxdesc;
	if (cam_dev != NULL)
		cam_close_device(cam_dev);
#else
	/* SCIOCCOMMAND ioctl method */
	if (devp->fd > 0)
		(void) close(devp->fd);
#endif	/* FREEBSD_CAM */

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
	return BSD_MAX_XFER;
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
#ifdef __FreeBSD__
#ifdef FREEBSD_CAM
	return ("OS module for FreeBSD (CAM)");
#else
	return ("OS module for FreeBSD (SCIOCCOMMAND)");
#endif
#else
#ifdef __NetBSD__
	return ("OS module for NetBSD");
#else
#ifdef __OpenBSD__
	return ("OS module for OpenBSD");
#else
	return ("OS module (unsupported BSD variant!)");
#endif	/* __OpenBSD__ */
#endif	/* __NetBSD__ */
#endif	/* __FreeBSD__ */
}

#endif	/* __FreeBSD__ __NetBSD__ __OpenBSD__ DI_SCSIPT DEMO_ONLY */

