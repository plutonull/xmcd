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
static char *_os_demo_c_ident_ = "@(#)os_demo.c	6.47 04/03/05";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#ifdef DEMO_ONLY

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;


/*
 * pthru_send
 *	Build SCSI CDB and send command to the device.
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
	simpkt_t	spkt,
			rpkt;
	static word32_t	pktid = 0;

	if (devp == NULL || devp->fd2 < 0 || devp->fd < 0)
		return FALSE;

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

	(void) memset(&spkt, 0, CDSIM_PKTSZ);
	(void) memset(&rpkt, 0, CDSIM_PKTSZ);

	(void) memcpy(spkt.cdb, cmdpt, cmdlen);
	spkt.cdbsz = (byte_t) cmdlen;
	spkt.len = (datalen > CDSIM_MAX_DATALEN) ? CDSIM_MAX_DATALEN : datalen;
	spkt.dir = rw;
	spkt.pktid = ++pktid;

	/* Reset packet id if overflowing */
	if (pktid == 0xffff)
		pktid = 0;

	/* Copy data from user buffer into packet */
	if (rw == OP_DATAOUT && datapt != NULL && spkt.len != 0)
		(void) memcpy(spkt.data, datapt, spkt.len);

	/* Send command packet */
	if (!cdsim_sendpkt("pthru", devp->fd, &spkt))
		return FALSE;

	/* Get response packet */
	if (!cdsim_getpkt("pthru", devp->fd2, &rpkt))
		return FALSE;

	/* Sanity check */
	if (rpkt.pktid != spkt.pktid) {
		if (app_data.scsierr_msg && prnerr)
			(void) fprintf(errfp, "pthru: packet sequence error.\n");

		return FALSE;
	}

	/* Check return status */
	if (rpkt.retcode != CDSIM_COMPOK) {
		if (app_data.scsierr_msg && prnerr)
			(void) fprintf(errfp,
				"pthru: cmd error (opcode=0x%x status=%d).\n",
				rpkt.cdb[0], rpkt.retcode);

		return FALSE;
	}

	/* Copy data from packet into user buffer */
	if (rw == OP_DATAIN && datapt != NULL && rpkt.len != 0)
		(void) memcpy(datapt, rpkt.data, rpkt.len);

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
 *	Device file descriptor, or -1 on failure.
 */
/*ARGSUSED*/
di_dev_t *
pthru_open(char *path)
{
	int		cdsim_sfd[2] = { -1, -1 },
			cdsim_rfd[2] = { -1, -1 };
	di_dev_t	*devp;

	if ((devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return NULL;
	}

	/* Open pipe for IPC */
	if (PIPE(cdsim_sfd) < 0 || PIPE(cdsim_rfd) < 0) {
		DI_FATAL("Cannot open pipe.");
		return FALSE;
	}

	devp->fd = cdsim_sfd[1];
	devp->fd2 = cdsim_rfd[0];
	devp->val = (int) FORK();

	/* Fork the CD simulator child */
	switch (devp->val) {
	case -1:
		DI_FATAL("Cannot fork.");
		return FALSE;

	case 0:
		/* Close some file descriptors */
		(void) close(cdsim_sfd[1]);
		(void) close(cdsim_rfd[0]);

		/* Child: run CD simulator */
		cdsim_main(cdsim_sfd[0], cdsim_rfd[1]);
		_exit(0);
		/*NOTREACHED*/

	default:
		/* Close some file descriptors */
		(void) close(cdsim_sfd[0]);
		(void) close(cdsim_rfd[1]);

		/* Parent: continue running the CD player */
		DBGPRN(DBG_DEVIO)(errfp, "pthru: forked cdsim child pid=%d\n",
			devp->val);
		break;
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
/*ARGSUSED*/
void
pthru_close(di_dev_t *devp)
{
	waitret_t	wstat;

	/* Close down pipes */
	(void) close(devp->fd);
	(void) close(devp->fd2);

	/* Shut down child */
	if (devp->val > 0 && kill((pid_t) devp->val, 0) == 0)
		(void) kill((pid_t) devp->val, SIGTERM);

	/* Wait for child to exit */
	(void) util_waitchild((pid_t) devp->val, 0, NULL, 0, FALSE, &wstat);

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
	return (CDSIM_MAX_DATALEN);
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
	return ("OS module (Demo Dummy)");
}

#endif	/* DEMO_ONLY */

