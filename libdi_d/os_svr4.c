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
#ifndef lint
static char *_os_svr4_c_ident_ = "@(#)os_svr4.c	6.70 04/03/02";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if (defined(SVR4) || defined(SVR5)) && \
    defined(DI_SCSIPT) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;


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
	struct sb		sb;
	struct scb		*scbp;
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

	if (datalen > PDI_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s: "
				"I/O data size too large.\n",
				APPNAME, devp->path);
		return FALSE;
	}

	(void) memset(&sb, 0, sizeof(sb));

	sb.sb_type = ISCB_TYPE;
	scbp = &sb.SCB;

	/* set up scsicmd */
	scbp->sc_cmdpt = (caddr_t) cmdpt;
	scbp->sc_cmdsz = cmdlen;
	scbp->sc_datapt = (caddr_t) datapt;
	scbp->sc_datasz = datalen;
	switch (rw) {
	case OP_NODATA:
	case OP_DATAIN:
		scbp->sc_mode = SCB_READ;
		break;
	case OP_DATAOUT:
		scbp->sc_mode = SCB_WRITE;
		break;
	default:
		break;
	}
	scbp->sc_time = (tmout ? tmout : DFLT_CMD_TIMEOUT) * 1000;

	/* Send the command down via the "pass-through" interface */
	while (ioctl(devp->fd, SDI_SEND, &sb) < 0) {
		if (errno == EAGAIN) {
			/* Wait a little while and retry */
			util_delayms(1000);
		}
		else {
			if (app_data.scsierr_msg && prnerr)
				perror("SDI_SEND ioctl failed");
			return FALSE;
		}
	}

	if (scbp->sc_comp_code != SDI_ASW) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
				    "%s: %s %s:\n%s=0x%x %s=0x%x %s=0x%x\n",
				    APPNAME,
				    "SCSI command error on", devp->path,
				    "Opcode", cmdpt[0],
				    "Comp_code", (int) scbp->sc_comp_code,
				    "Target_status", (int) scbp->sc_status);
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
	dev_t		ptdev;
	struct stat	stbuf;
	char		tmppath[FILE_PATH_SZ],
			errstr[ERR_BUF_SZ];

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

	/* Open CD-ROM device */
	if ((fd = open(path, O_RDONLY)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		return NULL;
	}

	/* Get pass-through interface device number */
	if (ioctl(fd, B_GETDEV, &ptdev) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"B_GETDEV ioctl failed: errno=%d\n", errno);
		(void) close(fd);
		return NULL;
	}

	(void) close(fd);

	/* Make pass-through interface device node */
	(void) sprintf(tmppath, "%s/pass.%x", TEMP_DIR, (int) ptdev);

	if (mknod(tmppath, S_IFCHR | 0700, ptdev) < 0) {
		if (errno == EEXIST) {
			(void) UNLINK(tmppath);

			if (mknod(tmppath, S_IFCHR | 0700, ptdev) < 0) {
				DBGPRN(DBG_DEVIO)(errfp,
					"Cannot mknod %s: errno=%d\n",
					tmppath, errno);
				return NULL;
			}
		}
		else {
			DBGPRN(DBG_DEVIO)(errfp,
				"Cannot mknod %s: errno=%d\n", tmppath, errno);
			return NULL;
		}
	}

	if ((devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	if (!util_newstr(&devp->auxpath, tmppath)) {
		di_devfree(devp);
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	/* Open pass-through device node */
	if ((devp->fd = open(tmppath, O_RDONLY)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", tmppath, errno);
		di_devfree(devp);
		return NULL;
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
		(void) UNLINK(devp->auxpath);
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
	return PDI_MAX_XFER;
}


/*
 * pthru_vers
 *	Return OS Interface Module version string
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Module version text string.
 */
char *
pthru_vers(void)
{
#ifdef _FTX
	return ("OS module for Stratus SVR4 FTX 3.x");
#else
	return ("OS module for UNIX SVR4/SVR5 PDI/SDI");
#endif
}

#endif	/* _UNIXWARE _FTX __hppa */

#ifdef MOTOROLA
/*
 *   Motorola 88k UNIX SVR4 support
 *
 *   Contributing author: Mark Scott
 *   E-mail: mscott@urbana.mcd.mot.com
 *
 *   Note: Audio CDs sometimes produce "Blank check" warnings on the console, 
 *         just ignore these.
 *
 *   This software fragment contains code that interfaces the
 *   application to the System V Release 4 operating system
 *   from Motorola.  The name "Motorola" is used here for
 *   identification purposes only.
 */


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
	char			scsistat = '\0',
				*tmpbuf,
				title[FILE_PATH_SZ + 20];
	int			cdb_l = 0,
				i;
	long			residual = 0L;
	unsigned long		errinfo  = 0L,
				ccode = 0L;
	struct scsi_pass	spass,
				*sp = &spass;
	struct ext_sense	sense;

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
		sensept = (byte_t *) &sense;
		senselen = sizeof(sense);
	}
	(void) memset(sensept, 0, senselen);

	(void) memset(sp, 0, sizeof(struct scsi_pass));

	/* Setup pass-through structure */
	sp->resid = &residual;
	sp->sense_data = (struct ext_sense *)(void *) sensept;
	sp->status = &scsistat;
	sp->error_info = &errinfo;
	sp->ctlr_code = &ccode;
	sp->xfer_len = (unsigned long) datalen;

	/* Align on a page boundary */
	tmpbuf = NULL;
	if (sp->xfer_len > 0) {
		if (sp->xfer_len > MOT_MAX_XFER) {
			DBGPRN(DBG_DEVIO)(errfp,
					"%s: SCSI command error on %s: "
					"I/O data size too large.\n",
					APPNAME, devp->path);
			return FALSE;
		}

		tmpbuf = (char *) MEM_ALLOC("xfer_buf", 2 * NBPP);
		sp->data = tmpbuf;
		sp->data += NBPP - ((unsigned int) sp->data & (NBPP - 1));
	}
	else
		sp->data = tmpbuf;

	if (rw == OP_DATAOUT && sp->xfer_len > 0)	/* Write operation */
		(void) memcpy(sp->data, datapt, sp->xfer_len);

	(void) memcpy(sp->cdb, cmdpt, cmdlen);
	cdb_l = (char) (cmdlen << 4);

	/* Check CDB length & flags */

	if (!SPT_CHK_CDB_LEN(cdb_l))
		(void) fprintf(errfp, "%d: invalid CDB length\n", cmdlen);

	sp->flags = cdb_l | SPT_ERROR_QUIET;
	if (rw == OP_DATAIN || rw == OP_NODATA)
		sp->flags |= SPT_READ;

	if (SPT_CHK_FLAGS(cdb_l))
		(void) fprintf(errfp, "0x%2x: bad CDB flags\n", sp->flags);

	/* Send the command down via the "pass-through" interface */
	if (ioctl(devp->fd, DKPASSTHRU, sp) < 0) {
		if (app_data.scsierr_msg && prnerr)
			perror("DKPASSTHRU ioctl failed");

		if (tmpbuf != NULL)
			MEM_FREE(tmpbuf);

		return FALSE;
	}

	if (*sp->error_info != SPTERR_NONE) {
		if (*sp->error_info != SPTERR_SCSI && *sp->status != 2 &&
		    app_data.scsierr_msg && prnerr) {

			/* Request Sense is done automatically by the driver */
			(void) fprintf(errfp,
				"%s: %s %s:\n%s=0x%x %s=0x%x %s=0x%x\n",
				APPNAME,
				"SCSI command error on", devp->path,
				"opcode", cmdpt[0],
				"xfer_len", sp->xfer_len,
				"err_info", *sp->error_info);

			(void) fprintf(errfp, "%s=0x%x %s=0x%x %s=0x%x\n",
				"ctlr_code", *sp->ctlr_code,
				"status", *sp->status,
				"resid", *sp->resid);
		}

		if (tmpbuf != NULL)
			MEM_FREE(tmpbuf);

		return FALSE;
	}

	/* pass the data back to caller */
	if (sp->xfer_len > 0 && rw == OP_DATAIN)	/* read operation */
		(void) memcpy(datapt, sp->data, sp->xfer_len);

	if (tmpbuf != NULL)
		MEM_FREE(tmpbuf);

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

	/* Open CD-ROM device */
	if ((devp->fd = open(path, O_RDONLY | O_NDELAY | O_EXCL)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		di_devfree(devp);
		return NULL;
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
	return MOT_MAX_XFER;
}


/*
 * pthru_vers
 *	Return OS Interface Module version string
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Module version text string.
 */
char *
pthru_vers(void)
{
	return ("OS module for Motorola SVR4-m88k");
}

#endif	/* MOTOROLA */

#endif	/* SVR4 SVR5 DI_SCSIPT DEMO_ONLY */

