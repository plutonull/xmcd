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
 *   Digital OSF1 (aka Digital UNIX, Compaq/HP Tru64 UNIX) and
 *   Digital Ultrix support
 *
 *   Contributing author: Matt Thomas
 *   E-Mail: thomas@lkg.dec.com
 *
 *   Contributing author: Gary Field
 *   E-Mail: gfield@zk3.dec.com
 *
 *   This software fragment contains code that interfaces the
 *   application to the Digital OSF1 and Ultrix operating systems.
 *   The term Digital, Compaq, HP, OSF1 and Ultrix are used here for
 *   identification purposes only.
 */
#ifndef lint
static char *_os_dec_c_ident_ = "@(#)os_dec.c	6.64 04/03/02";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if (defined(_OSF1) || defined(_ULTRIX)) && \
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
	UAGT_CAM_CCB		uagt;
	CCB_SCSIIO		ccb;
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
	(void) memset(&uagt, 0, sizeof(uagt));
	(void) memset(&ccb, 0, sizeof(ccb));

	/* Setup the user agent struct */
	uagt.uagt_ccb = (CCB_HEADER *) &ccb;
	uagt.uagt_ccblen = sizeof(CCB_SCSIIO);
#ifdef _OSF1
	uagt.uagt_snsbuf = ccb.cam_sense_ptr = sensept;
	uagt.uagt_snslen = ccb.cam_sense_len = senselen;
#endif

	/* Setup the CAM ccb */
	(void) memcpy((byte_t *) ccb.cam_cdb_io.cam_cdb_bytes, cmdpt, cmdlen);
	ccb.cam_cdb_len = cmdlen;
	ccb.cam_ch.my_addr = (CCB_HEADER *) &ccb;
	ccb.cam_ch.cam_ccb_len = sizeof(CCB_SCSIIO);
	ccb.cam_ch.cam_func_code = XPT_SCSI_IO;

	if (datapt != NULL && datalen > 0) {
		if (datalen > DEC_MAX_XFER) {
			DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s: "
				"I/O data size too large.\n",
				APPNAME, devp->path);
			return FALSE;
		}

		switch (rw) {
		case OP_DATAIN:
			ccb.cam_ch.cam_flags |= CAM_DIR_IN;
			break;
		case OP_DATAOUT:
			ccb.cam_ch.cam_flags |= CAM_DIR_OUT;
			break;
		case OP_NODATA:
		default:
			break;
		}
		uagt.uagt_buffer = (u_char *) datapt;
		uagt.uagt_buflen = datalen;
	}
	else
		ccb.cam_ch.cam_flags |= CAM_DIR_NONE;

#ifndef _OSF1
	ccb.cam_ch.cam_flags |= CAM_DIS_AUTOSENSE;
#endif

	ccb.cam_data_ptr = uagt.uagt_buffer;
	ccb.cam_dxfer_len = uagt.uagt_buflen;
	ccb.cam_timeout = tmout ? tmout : DFLT_CMD_TIMEOUT;

	ccb.cam_ch.cam_path_id = devp->bus;
	ccb.cam_ch.cam_target_id = devp->target;
	ccb.cam_ch.cam_target_lun = devp->lun;
    
	/* Set an invalid status so we can be sure it changed */
	ccb.cam_scsi_status = 0xff;

	if (ioctl(devp->fd, UAGT_CAM_IO, (caddr_t) &uagt) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("UAGT_CAM_IO ioctl failed");

		return FALSE;
    	}

	/* Check return status */
	if ((ccb.cam_ch.cam_status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
				       "%s: %s %s:\n%s=0x%x %s=0x%x\n",
				       APPNAME,
				       "SCSI command error on", devp->path,
				       "Opcode", cmdpt[0],
				       "Status", ccb.cam_scsi_status);
#ifdef _OSF1
			rp = (req_sense_data_t *)(void *) sensept;
			str = scsipt_reqsense_keystr((int) rp->key);

			(void) fprintf(errfp,
				    "Sense data: Key=0x%x (%s) "
				    "Code=0x%x Qual=0x%x\n",
				    rp->key,
				    str,
				    rp->code,
				    rp->qual);
#endif
		}

		if (ccb.cam_ch.cam_status & CAM_SIM_QFRZN) {
			(void) memset(&ccb, 0, sizeof(ccb));
			(void) memset(&uagt, 0, sizeof(uagt));

			/* Setup the user agent struct */
			uagt.uagt_ccb = (CCB_HEADER  *) &ccb;
			uagt.uagt_ccblen = sizeof(CCB_RELSIM);

			/* Setup the CAM ccb */
			ccb.cam_ch.my_addr = (struct ccb_header *) &ccb;
			ccb.cam_ch.cam_ccb_len = sizeof(CCB_RELSIM);
			ccb.cam_ch.cam_func_code = XPT_REL_SIMQ;

			ccb.cam_ch.cam_path_id = devp->bus;
			ccb.cam_ch.cam_target_id = devp->target;
			ccb.cam_ch.cam_target_lun = devp->lun;

			if (ioctl(devp->fd, UAGT_CAM_IO, (caddr_t) &uagt) < 0)
				return FALSE;
		}

#ifndef _OSF1
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
#endif

		return FALSE;
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
	di_dev_t	*devp;
	int		fd;
	struct stat	stbuf;
	char		errstr[ERR_BUF_SZ];
	int		saverr,
			bus,
			target,
			lun;

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

	if ((fd = open(path, O_RDONLY | O_NDELAY, 0)) >= 0) {
		struct devget	devget;

		if (ioctl(fd, DEVIOCGET, &devget) >= 0) {
#ifdef _OSF1
			/* OSF1, Digital UNIX, Tru64 UNIX */
			device_info_t   devi;

			if (ioctl(fd, DEVGETINFO, &devi) >= 0) {
				bus = devi.v1.businfo.bus.scsi.bus_num;
				target = devi.v1.businfo.bus.scsi.tgt_id;
				lun = devi.v1.businfo.bus.scsi.lun;
			}
			else {
				DBGPRN(DBG_DEVIO)(errfp,
					"DEVGETINFO failed: errno=%d: %s\n",
					errno, "Using DEVIOCGET data.");
				bus = ((devget.slave_num & 0700) >> 6);
				target = ((devget.slave_num & 0070) >> 3);
				lun = (devget.slave_num & 0007);
			}
#else
			/* Ultrix */
			bus = ((devget.slave_num & 0070) >> 3);
			target = (devget.slave_num & 0007);
			lun = 0;
#endif
			(void) close(fd);

			DBGPRN(DBG_DEVIO)(errfp,
				"Device bus,target,lun=%d,%d,%d\n",
				bus, target, lun);

			if ((devp = di_devalloc(path)) == NULL) {
				DI_FATAL(app_data.str_nomemory);
				return NULL;
			}

			if ((devp->fd = open(DEV_CAM, O_RDWR, 0)) >= 0 ||
			    (devp->fd = open(DEV_CAM, O_RDONLY, 0)) >= 0) {
				devp->bus = bus;
				devp->target = target;
				devp->lun = lun;

				return (devp);
			}

			DBGPRN(DBG_DEVIO)(errfp, "Cannot open %s: errno=%d\n",
			       DEV_CAM, errno);
		}
		else {
			(void) close(fd);

			DBGPRN(DBG_DEVIO)(errfp,
			       "DEVIOCGET ioctl failed on %s: errno=%d\n",
			       path, errno);
		}
	}
	else {
		saverr = errno;
		(void) sprintf(errstr, "Cannot open %s: errno=%d",
			       path, saverr);

		if (saverr != EIO)
			DI_FATAL(errstr);

		DBGPRN(DBG_DEVIO)(errfp, "%s\n", errstr);
	}

	return NULL;
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
/*ARGSUSED*/
size_t
pthru_maxfer(di_dev_t *devp)
{
	return DEC_MAX_XFER;
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
	return ("OS module for Tru64 UNIX (OSF1) & Ultrix");
}

#endif	/* _OSF1 _ULTRIX DI_SCSIPT DEMO_ONLY */

