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
 *   OpenVMS support
 *
 *   Contributing author: Rick Jones
 *   E-Mail: rjones@zko.dec.com
 *
 *   IDE/ATAPI drive support added by Ti Kan
 *
 *   This software fragment contains code that interfaces the
 *   application to the Digital OpenVMS operating system.
 *   The terms Digital and OpenVMS are used here for identification
 *   purposes only.
 */
#ifndef lint
static char *_os_vms_c_ident_ = "@(#)os_vms.c	6.68 04/03/02";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#if defined(__VMS) && defined(DI_SCSIPT) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;

/* Just a few, not the whole starlet.h */
extern int		sys$assign(),
			sys$dassgn(),
			sys$exit(),
			sys$getdvi(),
			sys$qiow();

STATIC int		context;	
STATIC char		*str;
STATIC bool_t		is_atapi = FALSE;


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
	int			ret,
				pthru_iosb[2],	/* Pass-through iosb */
				pthru_desc[15];	/* Pass-through cmd block */
	char 			*str,
				scsi_status,
				title[FILE_PATH_SZ + 20];
	byte_t			cmd_atapi[ATAPI_CMDLEN];
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

	if (datalen > VMS_MAX_XFER) {
		DBGPRN(DBG_DEVIO)(errfp,
				"%s: SCSI command error on %s: "
				"I/O data size too large.\n",
				APPNAME, devp->path);
		return FALSE;
	}

	/* Build command desc. for driver call */
	pthru_desc[0] = 1;		/* Opcode */
	pthru_desc[1] = (rw == OP_DATAOUT ? 0 : 1) | FLAGS_DISCONNECT;
					/* Flags */
	if (is_atapi) {
		/* VMS ATAPI commands are always 12-bytes long */
		(void) memset(cmd_atapi, 0, ATAPI_CMDLEN);
		(void) memcpy(cmd_atapi, cmdpt, cmdlen);
		pthru_desc[2] = (int) cmd_atapi;	/* Command address */
		pthru_desc[3] = ATAPI_CMDLEN;
	}
	else {
		/* SCSI */
		pthru_desc[2] = (int) cmdpt;		/* Command address */
		pthru_desc[3] = cmdlen;
	}

	pthru_desc[4] = (int) datapt;	/* Data address */
	pthru_desc[5] = datalen;	/* Data length */
	pthru_desc[6] = 0;		/* Pad length */
	pthru_desc[7] = 0;		/* Phase timeout */
	pthru_desc[8] = tmout ? tmout : DFLT_CMD_TIMEOUT;
					/* Disconnect timeout */
	pthru_desc[9] = 0;
	pthru_desc[10] = 0;
	pthru_desc[11] = 0;
	pthru_desc[12] = 0;
	pthru_desc[13] = 0;
	pthru_desc[14] = 0;

	/* Send the command to the driver */
	ret = sys$qiow(1, devp->fd, IO$_DIAGNOSE, pthru_iosb, 0, 0,
		       &pthru_desc[0], 15*4, 0, 0, 0, 0);
	if (!(ret & 1))
		sys$exit(ret);

	if (!(pthru_iosb[0] & 1))
		sys$exit(pthru_iosb[0] & 0xffff);

	scsi_status = (pthru_iosb[1] >> 24) & SCSI_STATUS_MASK;

	if (scsi_status != GOOD_SCSI_STATUS) {
		if (app_data.scsierr_msg && prnerr) {
			(void) fprintf(errfp,
				       "%s: %s %s:\n%s=0x%x %s=0x%x\n",
				       APPNAME,
				       "SCSI command error on", devp->path,
				       "Opcode", cmdpt[0],
				       "Status", scsi_status);
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
	unsigned short	chan;
	di_dev_t	*devp;
	int		status,
			pthru_desc[2];	/* Pass-through string desc */
#ifdef __alpha
	ILE3		ilst[2];
	unsigned int	devchar2;
	unsigned short	retlen;
#endif

	if (path == NULL) {
		(void) fprintf(errfp,
				"CD Audio: CD-ROM Device not defined.\n");
		sys$exit(SS$_NODEVAVL);
	}

	if ((devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	pthru_desc[0] = strlen(path);
	pthru_desc[1] = (int) path;

	status = sys$assign(&pthru_desc[0], &chan, 0, 0);
	if (!(status & 1)) {
		(void) fprintf(errfp,
			       "CD Audio: Error assigning to device %s\n",
			       path);
		sys$exit(status);
	}

	devp->fd = (int) chan;

#ifdef __alpha
        ilst[0].ile3$w_code = DVI$_DEVCHAR2;
        ilst[0].ile3$w_length = sizeof(devchar2);
        ilst[0].ile3$ps_bufaddr = &devchar2;
        ilst[0].ile3$ps_retlen_addr = &retlen;
        ilst[1].ile3$w_code = 0;
        ilst[1].ile3$w_length = 0;

        status = sys$getdvi(0, 0, &pthru_desc[0], ilst, 0, 0, 0, 0);
	is_atapi = ((devchar2 & DEV$M_SCSI) == 0);
#else
	is_atapi = FALSE;
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
	int	ret;

	ret = sys$dassgn(devp->fd);
	if (!(ret & 1)) {
		DBGPRN(DBG_DEVIO)(errfp,
			"CD Audio: Cannot deassign channel from device\n");
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
	return VMS_MAX_XFER;
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
	return ("OS module for OpenVMS");
}

#endif	/* __VMS DI_SCSIPT DEMO_ONLY */

