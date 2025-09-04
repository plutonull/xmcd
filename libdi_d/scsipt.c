/*
 *   libdi - CD Audio Device Interface Library
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
static char *_scsipt_c_ident_ = "@(#)scsipt.c	6.278 04/03/17";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"
#include "cdda_d/cdda.h"

#ifdef DI_SCSIPT

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;
extern char		**di_devlist;
extern sword32_t	di_clip_frames;

STATIC bool_t	scsipt_do_start_stop(di_dev_t *, bool_t, bool_t, bool_t),
		scsipt_do_pause_resume(di_dev_t *, bool_t),
		scsipt_run_ab(curstat_t *),
		scsipt_run_sample(curstat_t *),
		scsipt_run_prog(curstat_t *),
		scsipt_run_repeat(curstat_t *),
		scsipt_disc_ready(curstat_t *),
		scsipt_chg_ready(curstat_t *),
		scsipt_chg_rdstatus(void),
		scsipt_chg_start(curstat_t *);
STATIC int	scsipt_cfg_vol(int, curstat_t *, bool_t, bool_t);
STATIC void	scsipt_stat_poll(curstat_t *),
		scsipt_insert_poll(curstat_t *);


/* VU module entry jump table */
vu_tbl_t	scsipt_vutbl[MAX_VENDORS];

/* SCSI command CDB */
byte_t		cdb[MAX_CMDLEN];

/* Device file descriptors */
di_dev_t	*devp = NULL,			/* CD device descriptor */
		*chgp = NULL;			/* Medium changer descriptor */


STATIC int	scsipt_stat_interval,		/* Status poll interval */
		scsipt_ins_interval;		/* Insert poll interval */
STATIC long	scsipt_stat_id,			/* Play status poll timer id */
		scsipt_insert_id,		/* Disc insert poll timer id */
		scsipt_search_id;		/* FF/REW timer id */
STATIC bool_t	scsipt_not_open = TRUE,		/* Device not opened yet */
		scsipt_stat_polling,		/* Polling play status */
		scsipt_insert_polling,		/* Polling disc insert */
		scsipt_new_progshuf,		/* New program/shuffle seq */
		scsipt_start_search,		/* Start FF/REW play segment */
		scsipt_idx_pause,		/* Prev/next index pausing */
		scsipt_fake_stop,		/* Force a completion status */
		scsipt_playing,			/* Currently playing */
		scsipt_paused,			/* Currently paused */
		scsipt_bcd_hack,		/* Track numbers in BCD hack */
		scsipt_mult_autoplay,		/* Auto-play after disc chg */
		scsipt_mult_pause,		/* Pause after disc chg */
		scsipt_override_ap,		/* Override auto-play */
		scsipt_medmap_valid,		/* med chgr map is valid */
		*scsipt_medmap;			/* med chgr map */
STATIC sword32_t scsipt_sav_end_addr;		/* Err recov saved end addr */
STATIC word32_t	scsipt_next_sam;		/* Next SAMPLE track */
STATIC msf_t	scsipt_sav_end_msf;		/* Err recov saved end MSF */
STATIC byte_t	scsipt_dev_scsiver,		/* Device SCSI version */
		scsipt_sav_end_fmt,		/* Err recov saved end fmt */
		scsipt_route_left,		/* Left channel routing */
		scsipt_route_right,		/* Right channel routing */
		scsipt_cdtext_buf[SZ_RDCDTEXT];	/* CD-TEXT cache buffer */
STATIC mcparm_t	scsipt_mcparm;			/* Medium changer parameters */
STATIC cdda_client_t scsipt_cdda_client;	/* CDDA client struct */

/* VU module init jump table */
STATIC vuinit_tbl_t	vuinit[] = {
	{ NULL },
	{ chin_init },
	{ hita_init },
	{ nec_init },
	{ pion_init },
	{ sony_init },
	{ tosh_init },
	{ pana_init },
};


/* Request sense key to descriptive string mapping */
STATIC req_sense_keymap_t rsense_keymap[] = {
	{ 0x0,	"NO SENSE" },
	{ 0x1,	"RECOVERED ERROR" },
	{ 0x2,	"NOT READY" },
	{ 0x3,	"MEDIUM ERROR" },
	{ 0x4,	"HARDWARE ERROR" },
	{ 0x5,	"ILLEGAL REQUEST" },
	{ 0x6,	"UNIT ATTENTION" },
	{ 0x7,	"DATA PROTECT" },
	{ 0x8,	"BLANK CHECK" },
	{ 0x9,	"VENDOR-SPECIFIC" },
	{ 0xa,	"COPY ABORTED" },
	{ 0xb,	"ABORTED COMMAND" },
	{ 0xc,	"EQUAL" },
	{ 0xd,	"VOLUME OVERFLOW" },
	{ 0xe,	"MISCOMPARE" },
	{ 0xf,	"RESERVED" },
	{ -1,	NULL }
};


/***********************
 *  private functions  *
 ***********************/


/*
 * scsipt_init_vol
 *	Initialize volume, balance and channel routing controls
 *	to match the settings in the hardware, or to the preset
 *	value, if specified.
 *
 * Args:
 *	s      - Pointer to the curstat_t structure.
 *	preset - If a preset value is configured, whether to force to
 *		 the preset.
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_init_vol(curstat_t *s, bool_t preset)
{
	int	vol;

	/* Query current volume/balance settings */
	if ((vol = scsipt_cfg_vol(0, s, TRUE, FALSE)) >= 0)
		s->level = (byte_t) vol;
	else
		s->level = 0;

	/* Set volume to preset value, if so configured */
	if (app_data.startup_vol > 0 && preset) {
		s->level_left = s->level_right = 100;

		if ((vol = scsipt_cfg_vol(app_data.startup_vol, s,
					  FALSE, FALSE)) >= 0)
			s->level = (byte_t) vol;
	}

	/* Initialize sliders */
	SET_VOL_SLIDER(s->level);
	SET_BAL_SLIDER((int) (s->level_right - s->level_left) / 2);

	/* Set up channel routing */
	scsipt_route(s);
}


/*
 * scsipt_close
 *	Close SCSI pass-through I/O device.
 *
 * Args:
 *	dp - Device descriptor
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_close(di_dev_t *dp)
{
	DBGPRN(DBG_DEVIO)(errfp, "\nClose device: %s\n", dp->path);
	pthru_close(dp);
}


/*
 * scsipt_open
 *	Open SCSI pass-through I/O device.
 *
 * Args:
 *	path - The device path name
 *
 * Return:
 *	Pointer to the opened device descriptor
 *	
 */
STATIC di_dev_t *
scsipt_open(char *path)
{
	DBGPRN(DBG_DEVIO)(errfp, "\nOpen device: %s\n", path);
	return (pthru_open(path));
}


/***********************
 *  internal routines  *
 ***********************/


/*
 * scsipt_rezero_unit
 *	Send SCSI Rezero Unit command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id to send command for
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_rezero_unit(di_dev_t *dp, int role)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_REZERO;

	return (pthru_send(dp, role,
			   cdb, 6, NULL, 0, NULL, 0, OP_NODATA, 20,
			   (bool_t) ((app_data.debug & DBG_DEVIO) != 0)));
}


/*
 * scsipt_tst_unit_rdy
 *	Send SCSI Test Unit Ready command to the device
 *
 * Args:
 *	dp     - Device descriptor
 *	role   - role id for which to send command to
 *	sensep - Pointer to caller-supplied sense data buffer, or NULL
 *		 if no sense data is needed.
 *	prnerr - Whether to display error messages if the command fails.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure (drive not ready)
 */
bool_t
scsipt_tst_unit_rdy(
	di_dev_t	*dp,
	int		role,
	req_sense_data_t *sensep,
	bool_t		prnerr
)
{
	bool_t	ret;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_TEST;

	ret = pthru_send(
		dp, role, cdb, 6, NULL, 0,
		(byte_t *) sensep,
		(sensep == NULL) ? 0 : sizeof(req_sense_data_t),
		OP_NODATA, 20, prnerr
	);

	return (ret);
}


/*
 * scsipt_request_sense
 *	Send SCSI Request Sense command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id to send command for
 *	buf - Pointer to data buffer
 *	len - data buffer size
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_request_sense(di_dev_t *dp, int role, byte_t *buf, size_t len)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_RSENSE;
	cdb[4] = (byte_t) (len & 0xff);

	return (pthru_send(dp, role, cdb, 6, buf, len, NULL, 0, OP_DATAIN, 5,
			   (bool_t) ((app_data.debug & DBG_DEVIO) != 0)));
}


/*
 * scsipt_rdsubq
 *	Send SCSI-2 Read Subchannel command to the device
 *
 * Args:
 *	dp   - Device descriptor
 *	role - Role id to send command for
 *	fmt  - Data format desired (SUB_ALL, SUB_CURPOS, SUB_MCN or SUB_ISRC)
 *	trk  - Track number for which the information is requested
 *	       (relevant only for fmt == SUB_ISRC)
 *	sp   - Pointer to the caller-supplied cdstat_t return data buffer
 *	mcn  - Pointer to the caller-supplied MCN return string buffer.
 *	       The buffer must be at least 14 bytes long.
 *	isrc - Pointer to the caller-supplied ISRC return string buffer.
 *	       The buffer must be at least 13 bytes long.
 *	prnerr - Whether to output error message if command fails
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_rdsubq(
	di_dev_t	*dp,
	int		role,
	byte_t		fmt,
	byte_t		trk,
	cdstat_t	*sp,
	char		*mcn,
	char		*isrc,
	bool_t		prnerr
)
{
	byte_t		buf[SZ_RDSUBQ],
			*cp;
	subq_hdr_t	*h;
	subq_00_t	*p0;
	subq_01_t	*p1;
	subq_02_t	*p2;
	subq_03_t	*p3;
	int		i,
			xfer_len;
	char		*fmtstr,
			txtstr[STR_BUF_SZ];
	bool_t		ret,
			bad_data;

	/* If the drive doesn't support reading just the current position
	 * page, chances are it won't support the other pages either.  So
	 * we just read everything in one command.
	 */
	if (!app_data.curpos_fmt)
		fmt = SUB_ALL;

	switch (fmt) {
	case SUB_ALL:
		xfer_len = sizeof(subq_00_t) + sizeof(subq_hdr_t);
		fmtstr = "All subchannel data";
		break;
	case SUB_CURPOS:
		xfer_len = sizeof(subq_01_t) + sizeof(subq_hdr_t);
		fmtstr = "CD-ROM Current Position";
		break;
	case SUB_MCN:
		if (app_data.mcn_dsbl)
			return FALSE;
		xfer_len = sizeof(subq_02_t) + sizeof(subq_hdr_t);
		fmtstr = "Media Catalog Number";
		break;
	case SUB_ISRC:
		if (app_data.isrc_dsbl)
			return FALSE;
		xfer_len = sizeof(subq_03_t) + sizeof(subq_hdr_t);
		fmtstr = "International Standard Recording Code";
		break;
	default:
		/* Invalid format */
		return FALSE;
	}

	if (xfer_len > SZ_RDSUBQ)
		xfer_len = SZ_RDSUBQ;

	(void) memset(buf, 0, sizeof(buf));

	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_RDSUBQ;
	cdb[1] = app_data.subq_lba ? 0x00 : 0x02;
	cdb[2] = 0x40;
	cdb[3] = fmt;
	cdb[6] = trk;
	cdb[7] = (xfer_len & 0xff00) >> 8;
	cdb[8] = xfer_len & 0x00ff;

	if ((ret = pthru_send(dp, role, cdb, 10, buf, xfer_len, NULL, 0,
			      OP_DATAIN, 5, prnerr)) == FALSE)
		return FALSE;

	(void) sprintf(txtstr, "Read Subchannel data (%s)", fmtstr);
	DBGDUMP(DBG_DEVIO)(txtstr, buf, xfer_len);

	h = (subq_hdr_t *)(void *) buf;

	/* Check the subchannel data */
	cp = (byte_t *) h + sizeof(subq_hdr_t);
	switch (*cp) {
	case SUB_ALL:
	case SUB_CURPOS:
		if (sp != NULL) {
			p1 = (subq_01_t *)(void *) cp;

			/* Hack: to work around firmware anomalies
			 * in some drives.
			 */
			if (p1->trkno >= MAXTRACK &&
			    p1->trkno != LEAD_OUT_TRACK) {
				sp->status = CDSTAT_NOSTATUS;
			}

			/* Map SCSI status into cdstat_t */

			switch (h->audio_status) {
			case AUDIO_PLAYING:
				sp->status = CDSTAT_PLAYING;
				break;
			case AUDIO_PAUSED:
				sp->status = CDSTAT_PAUSED;
				break;
			case AUDIO_COMPLETED:
				sp->status = CDSTAT_COMPLETED;
				break;
			case AUDIO_FAILED:
				sp->status = CDSTAT_FAILED;
				break;
			case AUDIO_NOTVALID:
			case AUDIO_NOSTATUS:
			default:
				sp->status = CDSTAT_NOSTATUS;
				break;
			}

			if (scsipt_bcd_hack) {
				/* Hack: BUGLY CD drive firmware */
				sp->track = (int) util_bcdtol(p1->trkno);
				sp->index = (int) util_bcdtol(p1->idxno);
			}
			else {
				sp->track = (int) p1->trkno;
				sp->index = (int) p1->idxno;
			}

			if (app_data.subq_lba) {
				/* LBA mode */
				sp->abs_addr.addr = util_xlate_blk((sword32_t)
					util_bswap32(p1->abs_addr.logical)
				);
				util_blktomsf(
					sp->abs_addr.addr,
					&sp->abs_addr.min,
					&sp->abs_addr.sec,
					&sp->abs_addr.frame,
					MSF_OFFSET
				);

				sp->rel_addr.addr = util_xlate_blk((sword32_t)
					util_bswap32(p1->rel_addr.logical)
				);
				util_blktomsf(
					sp->rel_addr.addr,
					&sp->rel_addr.min,
					&sp->rel_addr.sec,
					&sp->rel_addr.frame,
					0
				);
			}
			else {
				/* MSF mode */
				sp->abs_addr.min = p1->abs_addr.msf.min;
				sp->abs_addr.sec = p1->abs_addr.msf.sec;
				sp->abs_addr.frame = p1->abs_addr.msf.frame;
				util_msftoblk(
					sp->abs_addr.min,
					sp->abs_addr.sec,
					sp->abs_addr.frame,
					&sp->abs_addr.addr,
					MSF_OFFSET
				);

				sp->rel_addr.min = p1->rel_addr.msf.min;
				sp->rel_addr.sec = p1->rel_addr.msf.sec;
				sp->rel_addr.frame = p1->rel_addr.msf.frame;
				util_msftoblk(
					sp->rel_addr.min,
					sp->rel_addr.sec,
					sp->rel_addr.frame,
					&sp->rel_addr.addr,
					0
				);
			}
		}

		if (*cp != SUB_ALL)
			break;

		p0 = (subq_00_t *)(void *) cp;

		/* Media catalog number data */
		if (mcn != NULL) {
			if (p0->mcval != 0) {
				/* Sanity check */
				bad_data = FALSE;
				for (i = 0; i < SZ_MCNDATA; i++) {
					if (!isdigit((int) p0->mcn[i])) {
						bad_data = TRUE;
						break;
					}
				}
				if (!bad_data)
					(void) strncpy(mcn, (char *) p0->mcn,
						       SZ_MCNDATA);
				else
					(void) memset(mcn, 0, SZ_MCNDATA);
			}
			else
				(void) memset(mcn, 0, SZ_MCNDATA);
		}

		/* ISRC data */
		if (isrc != NULL) {
			if (p0->tcval != 0) {
				/* Sanity check */
				bad_data = FALSE;
				for (i = 0; i < SZ_ISRCDATA; i++) {
					if (!isalnum((int) p0->isrc[i])) {
						bad_data = TRUE;
						break;
					}
				}
				if (!bad_data)
					(void) strncpy(isrc, (char *) p0->isrc,
						       SZ_ISRCDATA);
				else
					(void) memset(isrc, 0, SZ_ISRCDATA);
			}
			else
				(void) memset(isrc, 0, SZ_ISRCDATA);
		}
		break;

	case SUB_MCN:
		p2 = (subq_02_t *)(void *) cp;
		if (*cp != SUB_MCN)
			break;

		/* Media catalog number data */
		if (mcn != NULL) {
			if (p2->mcval != 0) {
				/* Sanity check */
				bad_data = FALSE;
				for (i = 0; i < SZ_MCNDATA; i++) {
					if (!isdigit((int) p2->mcn[i])) {
						bad_data = TRUE;
						break;
					}
				}
				if (!bad_data)
					(void) strncpy(mcn, (char *) p2->mcn,
						       SZ_MCNDATA);
				else
					(void) memset(mcn, 0, SZ_MCNDATA);
			}
			else
				(void) memset(mcn, 0, SZ_MCNDATA);
		}
		break;

	case SUB_ISRC:
		p3 = (subq_03_t *)(void *) cp;
		if (*cp != SUB_ISRC)
			break;

		/* ISRC data */
		if (isrc != NULL) {
			if (p3->tcval != 0) {
				/* Sanity check */
				bad_data = FALSE;
				for (i = 0; i < SZ_ISRCDATA; i++) {
					if (!isalnum((int) p3->isrc[i])) {
						bad_data = TRUE;
						break;
					}
				}
				if (!bad_data)
					(void) strncpy(isrc, (char *) p3->isrc,
						       SZ_ISRCDATA);
				else
					(void) memset(isrc, 0, SZ_ISRCDATA);
			}
			else
				(void) memset(isrc, 0, SZ_ISRCDATA);
		}
		break;

	default:
		/* Something is wrong with the data */
		DBGPRN(DBG_DEVIO)(errfp,
			"scsipt_rdsubq: unexpected data error\n");
		ret = FALSE;
		break;
	}

	return (ret);
}


/*
 * scsipt_modesense
 *	Send SCSI Mode Sense command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id to send command for
 *	buf - Pointer to the return data buffer
 *	pg_ctrl - Defines the type of parameters to be returned:
 *		0: Current values
 *		1: Changeable values
 *		2: Default values
 *	pg_code - Specifies which page or pages to return:
 *		PG_ERRECOV: Error recovery params page
 *		PG_DISCONN: Disconnect/reconnect params page
 *		PG_CDROMCTL: CD-ROM params page
 *		PG_AUDIOCTL: Audio control params page
 *		PG_ALL: All pages
 *	xfer_len - Data transfer page length (excluding header and
 *		block descriptor)
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_modesense(
	di_dev_t	*dp,
	int		role,
	byte_t		*buf,
	byte_t		pg_ctrl,
	byte_t		pg_code,
	size_t		xfer_len
)
{
	bool_t	ret;

	/* Add header and block desc length */
	if (app_data.msen_10)
		xfer_len += (app_data.msen_dbd ? 8 : 16);
	else
		xfer_len += (app_data.msen_dbd ? 4 : 12);

	SCSICDB_RESET(cdb);
	cdb[0] = app_data.msen_10 ? OP_M_MSENSE : OP_S_MSENSE;
	cdb[1] = (byte_t) (app_data.msen_dbd ? 0x08 : 0x00);
	cdb[2] = (byte_t) (((pg_ctrl & 0x03) << 6) | (pg_code & 0x3f));
	if (app_data.msen_10) {
		cdb[7] = (xfer_len & 0xff00) >> 8;
		cdb[8] = xfer_len & 0x00ff;
	}
	else
		cdb[4] = (byte_t) xfer_len;

	if ((ret = pthru_send(dp, role, cdb, app_data.msen_10 ? 10 : 6, buf,
			      xfer_len, NULL, 0, OP_DATAIN,
			      5, TRUE)) == TRUE) {
		DBGDUMP(DBG_DEVIO)("Mode Sense data", buf, xfer_len);
	}

	return (ret);
}


/*
 * scsipt_modesel
 *	Send SCSI Mode Select command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	buf - Pointer to the data buffer
 *	pg_code - Specifies which page or pages to return:
 *		PG_ERRECOV: Error recovery params page
 *		PG_DISCONN: Disconnect/reconnect params page
 *		PG_CDROMCTL: CD-ROM params page
 *		PG_AUDIOCTL: Audio control params page
 *		PG_ALL: All pages
 *	xfer_len - Data transfer page length (excluding header and
 *		block descriptor)
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
scsipt_modesel(
	di_dev_t	*dp,
	int		role,
	byte_t		*buf,
	byte_t		pg_code,
	size_t		xfer_len
)
{
	int			bdesclen;
	mode_sense_6_data_t	*ms_data6 = NULL;
	mode_sense_10_data_t	*ms_data10 = NULL;

	SCSICDB_RESET(cdb);
	cdb[0] = app_data.msen_10 ? OP_M_MSELECT : OP_S_MSELECT;
	cdb[1] = (xfer_len == 0) ? 0x00 : 0x10;;

	/* Add header and block desc length */
	if (app_data.msen_10) {
		ms_data10 = (mode_sense_10_data_t *)(void *) buf;
		bdesclen = (int)
			util_bswap16((word16_t) ms_data10->bdescr_len);
		xfer_len += (8 + bdesclen);

		cdb[7] = (xfer_len & 0xff00) >> 8;
		cdb[8] = xfer_len & 0x00ff;
	}
	else {
		ms_data6 = (mode_sense_6_data_t *)(void *) buf;
		bdesclen = (int) ms_data6->bdescr_len;
		xfer_len += (4 + bdesclen);

		cdb[4] = (byte_t) xfer_len;
	}

	DBGDUMP(DBG_DEVIO)("Mode Select data", buf, xfer_len);

	return (
		pthru_send(dp, role, cdb, app_data.msen_10 ? 10 : 6, buf,
			   xfer_len, NULL, 0, OP_DATAOUT, 5, TRUE)
	);
}


/*
 * scsipt_inquiry
 *	Send SCSI Inquiry command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	buf - Pointer to the return data buffer
 *	len - Maximum number of inquiry data bytes to transfer
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_inquiry(di_dev_t *dp, int role, byte_t *buf, size_t len)
{
	bool_t	ret;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_INQUIR;
	cdb[4] = (byte_t) len;

	if ((ret = pthru_send(dp, role, cdb, 6, buf, len, NULL, 0, OP_DATAIN,
			      5, TRUE)) == TRUE) {
		DBGDUMP(DBG_DEVIO)("Inquiry data", buf, len);
	}

	return (ret);
}


/*
 * scsipt_rdtoc
 *	Send Read TOC/PMA/ATIP command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	buf - Pointer to the return data buffer
 *	len - Size of return data buffer
 *	start - Starting track number for which the TOC data is returned
 *	fmt - Data to read:
 *		0 - Track TOC data
 *		1 - Session data
 *		2 - Full TOC data
 *		3 - PMA data
 *		4 - ATIP data
 *		5 - CD-TEXT data
 *	msf - Whether to use MSF or logical block address data format
 *	prnerr - Whether to output error message if command fails
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_rdtoc(
	di_dev_t	*dp,
	int		role,
	byte_t		*buf,
	size_t		len,
	int		start,
	int		fmt,
	bool_t		msf,
	bool_t		prnerr
)
{
	size_t		xfer_len,
			maxfer_len;
	toc_hdr_t	*thdr;
	bool_t		ret;

	/* Read the TOC header first */
	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_RDTOC;
	cdb[1] = msf ? 0x02 : 0x00;
	cdb[2] = (byte_t) (fmt & 0x0f);
	cdb[6] = (byte_t) start;
	cdb[7] = 0;
	cdb[8] = SZ_TOCHDR;

	/* Read header first to determine data length */
	if (!pthru_send(dp, role, cdb, 10, buf, SZ_TOCHDR, NULL, 0,
			OP_DATAIN, 10, prnerr)) {
		(void) memset(buf, 0, len);
		return FALSE;
	}

	thdr = (toc_hdr_t *)(void *) buf;

	switch (fmt) {
	case 0:	/* Read track TOC */
		if (start == 0)
			start = (int) thdr->first_trk;

		xfer_len = SZ_TOCHDR +
			(((int) thdr->last_trk - start + 2) * SZ_TOCENT);
		break;

	default:
		/* All other formats */
		xfer_len = (int) util_bswap16(thdr->data_len) + 2;
		break;
	}

	/* Check data size against max transfer size of system */
	maxfer_len = pthru_maxfer(dp);
	if (xfer_len > maxfer_len) {
		DBGPRN(DBG_DEVIO)(errfp,
			"scsipt_rdtoc: xfer_len (%d) > maxfer_len (%d).\n",
			(int) xfer_len, (int) maxfer_len);
		(void) memset(buf, 0, len);
		return FALSE;
	}

	if (xfer_len <= SZ_TOCHDR) {
		(void) memset(buf, 0, len);
		return FALSE;		/* No useful data to read */
	}
	else if (xfer_len > len)
		xfer_len = len;		/* Limit to buffer size */

	/* Do the full Read TOC/PMA/ATIP command */
	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_RDTOC;
	cdb[1] = msf ? 0x02 : 0x00;
	cdb[2] = (byte_t) (fmt & 0x0f);
	cdb[6] = (byte_t) start;
	cdb[7] = (xfer_len & 0xff00) >> 8;
	cdb[8] = xfer_len & 0x00ff;

	ret = pthru_send(dp, role, cdb, 10, buf, xfer_len, NULL, 0,
			 OP_DATAIN, 10, prnerr);

	if (!ret)
		(void) memset(buf, 0, len);
	else {
		char	*descr_str;

		switch (fmt) {
		case 0:
			descr_str = "Read TOC/PMA/ATIP data (Track TOC)";
			break;
		case 1:
			descr_str = "Read TOC/PMA/ATIP data (Session info)";
			break;
		case 2:
			descr_str = "Read TOC/PMA/ATIP data (Full TOC)";
			break;
		case 3:
			descr_str = "Read TOC/PMA/ATIP data (PMA)";
			break;
		case 4:
			descr_str = "Read TOC/PMA/ATIP data (ATIP)";
			break;
		case 5:
			descr_str = "Read TOC/PMA/ATIP data (CD-TEXT)";
			break;
		default:
			descr_str = "Read TOC/PMA/ATIP data";
			break;
		}

		DBGDUMP(DBG_DEVIO)(descr_str, buf, xfer_len);
	}

#ifdef __VMS
	/* VMS hack */
	if (ret) {
		int	i;

		ret = FALSE;

		/* Make sure that the return buffer is not all zeros */
		for (i = 0; i < xfer_len; i++) {
			if (buf[i] != 0) {
				ret = TRUE;
				break;
			}
		}
	}
#endif

	return (ret);
}


/*
 * scsipt_playmsf
 *	Send SCSI-2 Play Audio MSF command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	start - Pointer to the starting position MSF data
 *	end - Pointer to the ending position MSF data
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_playmsf(di_dev_t *dp, int role, msf_t *start, msf_t *end)
{
	byte_t	sm,
		ss,
		sf,
		em,
		es,
		ef;

	if (!app_data.playmsf_supp)
		return FALSE;

	/* If the start or end positions are less than the minimum
	 * position, patch them to the minimum positions.
	 */
	if (start->min == 0 && start->sec < 2) {
		sm = 0;
		ss = 2;
		sf = 0;
	}
	else {
		sm = start->min;
		ss = start->sec;
		sf = start->frame;
	}

	if (end->min == 0 && end->sec < 2) {
		em = 0;
		es = 2;
		ef = 0;
	}
	else {
		em = end->min;
		es = end->sec;
		ef = end->frame;
	}

	/* If start == end, just return success */
	if (sm == em && ss == es && sf == ef)
		return TRUE;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_PLAYMSF;
	cdb[3] = sm;
	cdb[4] = ss;
	cdb[5] = sf;
	cdb[6] = em;
	cdb[7] = es;
	cdb[8] = ef;

	return (
		pthru_send(dp, role, cdb, 10, NULL, 0,
			   NULL, 0, OP_NODATA, 20, TRUE)
	);
}


/*
 * scsipt_play10
 *	Send SCSI-2 Play Audio (10) command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	start - The starting logical block address
 *	len - The number of logical blocks to play (max=0xffff)
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_play10(di_dev_t *dp, int role, sword32_t start, sword32_t len)
{
	if (!app_data.play10_supp || len > 0xffff)
		return FALSE;

	/* Do block size conversion if needed */
	start = util_unxlate_blk(start);
	len = util_unxlate_blk(len);

	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_PLAY;
	cdb[2] = ((word32_t) start & 0xff000000) >> 24;
	cdb[3] = ((word32_t) start & 0x00ff0000) >> 16;
	cdb[4] = ((word32_t) start & 0x0000ff00) >> 8;
	cdb[5] = ((word32_t) start & 0x000000ff);
	cdb[7] = ((word32_t) len & 0xff00) >> 8;
	cdb[8] = ((word32_t) len & 0x00ff);

	return (
		pthru_send(dp, role, cdb, 10, NULL, 0, NULL, 0,
			   OP_NODATA, 20, TRUE)
	);
}


/*
 * scsipt_play12
 *	Send SCSI-2 Play Audio (12) command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	start - The starting logical block address
 *	len - The number of logical blocks to play
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_play12(di_dev_t *dp, int role, sword32_t start, sword32_t len)
{
	if (!app_data.play12_supp)
		return FALSE;

	/* Do block size conversion if needed */
	start = util_unxlate_blk(start);
	len = util_unxlate_blk(len);

	SCSICDB_RESET(cdb);
	cdb[0] = OP_L_PLAY;
	cdb[2] = ((word32_t) start & 0xff000000) >> 24;
	cdb[3] = ((word32_t) start & 0x00ff0000) >> 16;
	cdb[4] = ((word32_t) start & 0x0000ff00) >> 8;
	cdb[5] = ((word32_t) start & 0x000000ff);
	cdb[6] = ((word32_t) len & 0xff000000) >> 24;
	cdb[7] = ((word32_t) len & 0x00ff0000) >> 16;
	cdb[8] = ((word32_t) len & 0x0000ff00) >> 8;
	cdb[9] = ((word32_t) len & 0x000000ff);

	return (
		pthru_send(dp, role, cdb, 12, NULL, 0, NULL, 0,
			   OP_NODATA, 20, TRUE)
	);
}


/*
 * scsipt_play_trkidx
 *	Send SCSI-2 Play Audio Track/Index command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command for
 *	start_trk - Starting track number
 *	start_idx - Starting index number
 *	end_trk - Ending track number
 *	end_idx - Ending index number
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_play_trkidx(
	di_dev_t	*dp,
	int		role,
	int		start_trk,
	int		start_idx,
	int		end_trk,
	int		end_idx
)
{
	if (!app_data.playti_supp)
		return FALSE;

	if (scsipt_bcd_hack) {
		/* Hack: BUGLY CD drive firmware */
		start_trk = util_ltobcd(start_trk);
		start_idx = util_ltobcd(start_idx);
		end_trk = util_ltobcd(end_trk);
		end_idx = util_ltobcd(end_idx);
	}

	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_PLAYTI;
	cdb[4] = (byte_t) start_trk;
	cdb[5] = (byte_t) start_idx;
	cdb[7] = (byte_t) end_trk;
	cdb[8] = (byte_t) end_idx;

	return (
		pthru_send(dp, role, cdb, 10, NULL, 0, NULL, 0,
			   OP_NODATA, 20, TRUE)
	);
}


/*
 * scsipt_read_cdda
 *	Read CDDA (CD Digital Audio).
 *
 * Args:
 *	devp    - Device descriptor
 *	role    - Role id for which to send command to
 *	cdda    - Pointer to a caller-supplied cdda_req_t structure, which
 *		  contains the address, frame address, and data buffer
 *		  pointer.
 *	framesz - The size of a CDDA frame in bytes
 *	tout    - Command timeout interval (in seconds)
 *	sensep  - Pointer to caller-supplied sense data buffer, or NULL
 *		  if no sense data is needed.
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
bool_t
scsipt_read_cdda(
	di_dev_t		*devp,
	int			role,
	cdda_req_t		*cdda,
	size_t			framesz,
	int			tout,
	req_sense_data_t	*sensep
)
{
	sword32_t	addr;
	size_t		cmdlen;


	if (cdda->addrfmt == READFMT_LBA) {
		addr = cdda->pos.logical;
	}
	else if (cdda->addrfmt == READFMT_MSF) {
		util_msftoblk(
			cdda->pos.msf.min,
			cdda->pos.msf.sec,
			cdda->pos.msf.frame,
			&addr,
			MSF_OFFSET
		);
	}
	else {
		DBGPRN(DBG_DEVIO)(errfp,
		       "scsipt_read_cdda: Invalid addrfmt (0x%x)\n",
		       cdda->addrfmt);
		if (sensep != NULL) {
			memset(sensep, 0, sizeof(req_sense_data_t));
			sensep->valid = 1;
			sensep->key = 0x5;	/* Fake an "illegal request */
		}
		return FALSE;	/* Invalid address format */
	}

	SCSICDB_RESET(cdb);
	cdb[2] = (addr >> 24) & 0xff;
	cdb[3] = (addr >> 16) & 0xff;
	cdb[4] = (addr >>  8) & 0xff;
	cdb[5] = addr & 0xff;

	switch (app_data.cdda_scsireadcmd) {
	case CDDA_MMC:
		/* MMC Read CD (12) */
		cmdlen = 12;
		cdb[0] = OP_L_READCD;
		cdb[6] = (cdda->nframes >> 16) & 0xff;
		cdb[7] = (cdda->nframes >>  8) & 0xff;
		cdb[8] = cdda->nframes & 0xff;
		cdb[9] = 0xf8;	/* Specify SYNC, EDC and ECC */
		break;

	case CDDA_STD:
		/* SCSI Read (10) */
		cmdlen = 10;
		cdb[0] = OP_M_READ;
		cdb[7] = (cdda->nframes >> 8) & 0xff;
		cdb[8] = cdda->nframes & 0xff;
		break;

#ifndef DEMO_ONLY
	case CDDA_NEC:
		/* NEC Read CD-DA (10) */
		cmdlen = 10;
		cdb[0] = OP_VN_READCDDA;
		cdb[7] = (cdda->nframes >> 8) & 0xff;
		cdb[8] = cdda->nframes & 0xff;
		break;

	case CDDA_SONY:
		/* Sony Read CD-DA (12) */
		cmdlen = 12;
		cdb[0]  = OP_VS_READCDDA;
		cdb[6]  = (cdda->nframes >> 24) & 0xff;
		cdb[7]  = (cdda->nframes >> 16) & 0xff;
		cdb[8]  = (cdda->nframes >>  8) & 0xff;
		cdb[9]  = cdda->nframes & 0xff;
		cdb[10] = 0x00;	/* No subcode */
		break;
#endif	/* DEMO_ONLY */

	default:
		/* Invalid configuration */
		DBGPRN(DBG_DEVIO)(errfp,
			"Invalid cddaScsiReadCommand parameter (%d)\n",
			app_data.cdda_scsireadcmd);
		if (sensep != NULL) {
			memset(sensep, 0, sizeof(req_sense_data_t));
			sensep->valid = 1;
			sensep->key = 0x5;	/* Fake an "illegal request */
		}
		return FALSE;
	}

	return (
		pthru_send(devp, role, cdb, cmdlen, cdda->buf,
			   (size_t) (cdda->nframes * framesz),
			   (byte_t *) sensep,
			   (sensep == NULL) ? 0 : sizeof(req_sense_data_t),
			   OP_DATAIN, tout, TRUE)
	);
}


/*
 * scsipt_prev_allow
 *	Send SCSI Prevent/Allow Medium Removal command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role    - Role id for which to send command to
 *	prevent - Whether to prevent or allow medium removal
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_prev_allow(di_dev_t *dp, int role, bool_t prevent)
{
	if (!app_data.caddylock_supp)
		return FALSE;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_PREVENT;
	cdb[4] = prevent ? 0x01 : 0x00;

	return (
		pthru_send(dp, role, cdb, 6, NULL, 0, NULL, 0,
			   OP_NODATA, 5, TRUE)
	);
}


/*
 * scsipt_start_stop
 *	Send SCSI Start/Stop Unit command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command to
 *	start - Whether to start unit or stop unit
 *	loej - Whether caddy load/eject operation should be performed
 *	prnerr - Whether to print error messages if the command fails
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_start_stop(
	di_dev_t	*dp,
	int		role,
	bool_t		start,
	bool_t		loej,
	bool_t		prnerr
)
{
	byte_t		ctl;
	bool_t		ret;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (!start && PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) cdda_stop(dp, s);

		if (!scsipt_is_enabled(devp, DI_ROLE_MAIN))
			scsipt_enable(dp, DI_ROLE_MAIN);
	}

	if (start)
		ctl = 0x01;
	else
		ctl = 0x00;

	if (loej)
		ctl |= 0x02;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_START;
	if (start || loej)
		cdb[1] = 0x1;	/* Set immediate bit */
	cdb[4] = ctl;

	ret = pthru_send(dp, role, cdb, 6, NULL, 0, NULL, 0,
			 OP_NODATA, 20, prnerr);

	/* Delay a bit to let the CD load or eject.  This is a hack to
	 * work around firmware bugs in some CD drives.  These drives
	 * don't handle new commands well when the CD is loading/ejecting
	 * with the IMMED bit set in the Start/Stop Unit command.
	 */
	if (ret) {
		if (loej) {
			int	n;

			n = (app_data.ins_interval + 1000 - 1) / 1000;
			if (start)
				n *= 2;

			util_delayms(n * 1000);
		}
		else if (start && app_data.spinup_interval > 0)
			util_delayms(app_data.spinup_interval * 1000);
	}

	return (ret);
}


/*
 * scsipt_pause_resume
 *	Send SCSI-2 Pause/Resume command to the device
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command to
 *	resume - Whether to resume or pause
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_pause_resume(di_dev_t *dp, int role, bool_t resume)
{
	curstat_t	*s = di_clinfo->curstat_addr();

	if (!app_data.pause_supp)
		return FALSE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		return cdda_pause_resume(dp, s, resume);

	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_PAUSE;
	cdb[8] = resume ? 0x01 : 0x00;

	return (
		pthru_send(dp, role, cdb, 10, NULL, 0, NULL, 0,
			   OP_NODATA, 20, TRUE)
	);
}


/*
 * scsipt_move_medium
 *	Move a unit of media from a source element to a destination element
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command to
 *	te_addr - Transport element address
 *	src_addr - Source element address
 *	dst_addr - Destination element address
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_move_medium(
	di_dev_t	*dp,
	int		role,
	word16_t	te_addr,
	word16_t	src_addr,
	word16_t	dst_addr
)
{
	bool_t	ret;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_L_MOVEMED;
	cdb[2] = ((int) (te_addr & 0xff00) >> 8);
	cdb[3] = (te_addr & 0x00ff);
	cdb[4] = ((int) (src_addr & 0xff00) >> 8);
	cdb[5] = (src_addr & 0x00ff);
	cdb[6] = ((int) (dst_addr & 0xff00) >> 8);
	cdb[7] = (dst_addr & 0x00ff);

	ret = pthru_send(dp, role, cdb, 12, NULL, 0, NULL, 0, OP_NODATA, 30,
			 (bool_t) ((app_data.debug & DBG_DEVIO) != 0));

	if (ret)
		/* Wait a bit for the changer to load and settle */
		util_delayms((app_data.ins_interval + 1) * 1000);

	return (ret);
}


/*
 * scsipt_init_elemstat
 *	Request medium changer to check its element status.
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command to
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_init_elemstat(di_dev_t *dp, int role)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_S_INITELEMSTAT;

	return (
		pthru_send(
			dp,
			role,
			cdb,
			6,
			NULL,
			0,
			NULL,
			0,
			OP_NODATA,
			app_data.numdiscs * 10,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
		)
	);
}


/*
 * scsipt_read_elemstat
 *	Read medium changer element status.
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to send command to
 *	buf - Data buffer pointer
 *	len - Data buffer size
 *	nelem - Number of elements requested
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_read_elemstat(
	di_dev_t	*dp,
	int		role,
	byte_t		*buf,
	size_t		len,
	int		nelem
)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_L_READELEMSTAT;
	cdb[1] = 0;		/* Request all element types */
	cdb[2] = 0;		/* Start element address MSB */
	cdb[3] = 0;		/* Start element address LSB */
	cdb[4] = (byte_t) ((nelem & 0xff00) >> 8);  /* Elements count MSB */
	cdb[5] = (byte_t) (nelem & 0x00ff);	    /* Elements count LSB */
	cdb[7] = (byte_t) ((len & 0xff0000) >> 16); /* Alloc length MSB */
	cdb[8] = (byte_t) ((len & 0x00ff00) >> 8);  /* Alloc length */
	cdb[9] = (byte_t) (len & 0x0000ff);	    /* Alloc length LSB */

	return (
		pthru_send(
			dp,
			role,
			cdb,
			12,
			buf,
			len,
			NULL,
			0,
			OP_DATAIN,
			app_data.numdiscs * 20,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
		)
	);
}


/*
 * scsipt_disc_present
 *	Check if a disc is loaded
 *
 * Args:
 *	dp     - Device descriptor
 *	role   - Role id for which to check
 *	s      - Pointer to the curstat_t structure
 *	sensep - Pointer to caller-supplied sense data buffer, or NULL
 *		 if no sense data is needed.
 *
 * Return:
 *	TRUE - ready
 *	FALSE - not ready
 */
bool_t
scsipt_disc_present(
	di_dev_t	*dp,
	int		role,
	curstat_t	*s,
	req_sense_data_t *sensep
)
{
	int			i;
	bool_t			ret = FALSE;
	req_sense_data_t	sense_data;

	if (app_data.play_notur && s->mode != MOD_STOP &&
	    s->mode != MOD_NODISC && s->mode != MOD_BUSY) {
		/* For those drives that returns failure status to
		 * the Test Unit Ready command during audio playback,
		 * we just silently return success if the drive is
		 * supposed to be playing audio.  Shrug.
		 */
		return TRUE;
	}

	if (sensep == NULL)
		sensep = &sense_data;

	for (i = 0; i < 10; i++) {
		/* Send Test Unit Ready command to check
		 * if the drive is ready.
		 */
		ret = scsipt_tst_unit_rdy(
			dp, role, sensep,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
		);

		if (ret)
			break;

		if (sensep->key == 0x02 && sensep->code == 0x04) {
			/* Disc is spinning up, wait */
			util_delayms(1000);
		}
	}

	return (ret);
}


/*
 * scsipt_do_playaudio
 *	General top-level play audio function
 *
 * Args:
 *	dp - Device descriptor
 *	addr_fmt - The address formats specified:
 *		ADDR_BLK: logical block address
 *		ADDR_MSF: MSF address
 *		ADDR_TRKIDX: Track/index numbers
 *		ADDR_OPTEND: Ending address can be ignored
 *	start_addr - Starting logical block address
 *	end_addr - Ending logical block address
 *	start_msf - Pointer to start address MSF data
 *	end_msf - Pointer to end address MSF data
 *	trk - Starting track number
 *	idx - Starting index number
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_do_playaudio(
	di_dev_t	*dp,
	byte_t		addr_fmt,
	sword32_t	start_addr,
	sword32_t	end_addr,
	msf_t		*start_msf,
	msf_t		*end_msf,
	byte_t		trk,
	byte_t		idx
)
{
	msf_t		emsf,
			*emsfp = NULL;
	sword32_t	tmp_saddr,
			tmp_eaddr;
	bool_t		ret = FALSE,
			do_playmsf = FALSE,
			do_play12 = FALSE,
			do_play10 = FALSE,
			do_playti = FALSE;
	curstat_t	*s = di_clinfo->curstat_addr();

	/* Fix addresses: Some CD drives will only allow playing to
	 * the last frame minus a few frames.
	 */
	if ((addr_fmt & ADDR_MSF) && end_msf != NULL) {
		emsf = *end_msf;	/* Structure copy */
		emsfp = &emsf;

		util_msftoblk(
			start_msf->min,
			start_msf->sec,
			start_msf->frame,
			&tmp_saddr,
			MSF_OFFSET
		);

		util_msftoblk(
			emsfp->min,
			emsfp->sec,
			emsfp->frame,
			&tmp_eaddr,
			MSF_OFFSET
		);

		if (tmp_eaddr > s->discpos_tot.addr)
			tmp_eaddr = s->discpos_tot.addr;

		if (tmp_eaddr >= di_clip_frames)
			tmp_eaddr -= di_clip_frames;
		else
			tmp_eaddr = 0;

		util_blktomsf(
			tmp_eaddr,
			&emsfp->min,
			&emsfp->sec,
			&emsfp->frame,
			MSF_OFFSET
		);

		if (tmp_saddr >= tmp_eaddr)
			return FALSE;

		emsfp->res = start_msf->res = 0;

		/* Save end address for error recovery */
		scsipt_sav_end_msf = *end_msf;
	}
	if (addr_fmt & ADDR_BLK) {
		if (end_addr > s->discpos_tot.addr)
			end_addr = s->discpos_tot.addr;

		if (end_addr >= di_clip_frames)
			end_addr -= di_clip_frames;
		else
			end_addr = 0;

		if (start_addr >= end_addr)
			return FALSE;

		/* Save end address for error recovery */
		scsipt_sav_end_addr = end_addr;
	}

	/* Save end address format for error recovery */
	scsipt_sav_end_fmt = addr_fmt;

	if (PLAYMODE_IS_STD(app_data.play_mode) &&
	    scsipt_vutbl[app_data.vendor_code].playaudio != NULL) {
		ret = scsipt_vutbl[app_data.vendor_code].playaudio(
			addr_fmt,
			start_addr, end_addr,
			start_msf, emsfp,
			trk, idx
		);
	}

	if (!ret) {
		/* If the device does not claim SCSI-2 compliance, and the
		 * device-specific configuration is not SCSI-2, then don't
		 * attempt to deliver SCSI-2 commands to the device.
		 */
		if (app_data.vendor_code != VENDOR_SCSI2 &&
		    scsipt_dev_scsiver < 2)
			return FALSE;
	
		do_playmsf = (addr_fmt & ADDR_MSF) && app_data.playmsf_supp;
		do_play12 = (addr_fmt & ADDR_BLK) && app_data.play12_supp;
		do_play10 = (addr_fmt & ADDR_BLK) && app_data.play10_supp;
		do_playti = (addr_fmt & ADDR_TRKIDX) && app_data.playti_supp;

		if (do_playmsf || do_play12 || do_play10 || do_playti) {
			if (scsipt_paused) {
				if (app_data.strict_pause_resume) {
					/* Resume first */
					(void) scsipt_do_pause_resume(
						dp, TRUE
					);
				}
			}
			else if (scsipt_playing) {
				if (app_data.play_pause_play) {
					/* Pause first */
					(void) scsipt_do_pause_resume(
						dp, FALSE
					);
				}
			}
			else {
				/* Spin up CD */
				(void) scsipt_do_start_stop(
					dp, TRUE, FALSE,
					(bool_t)
					    ((app_data.debug & DBG_DEVIO) != 0)
				);
			}
		}
	}

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		if (do_play12 || do_play10) {
			if (scsipt_is_enabled(devp, DI_ROLE_MAIN))
				scsipt_disable(dp, DI_ROLE_MAIN);

			ret = cdda_play(dp, s, start_addr, end_addr);
		}
		else if (do_playmsf) {
			util_msftoblk(
				start_msf->min,
				start_msf->sec,
				start_msf->frame,
				&start_addr,
				MSF_OFFSET
			);
			util_msftoblk(
				emsfp->min,
				emsfp->sec,
				emsfp->frame,
				&end_addr,
				MSF_OFFSET
			);
			ret = cdda_play(dp, s, start_addr, end_addr);
		}
	}
	else {
		if (!ret && do_playmsf)
			ret = scsipt_playmsf(dp, DI_ROLE_MAIN,
					     start_msf, emsfp);

		if (!ret && do_play12)
			ret = scsipt_play12(dp, DI_ROLE_MAIN, start_addr,
					    end_addr - start_addr);

		if (!ret && do_play10)
			ret = scsipt_play10(dp, DI_ROLE_MAIN, start_addr,
					    end_addr - start_addr);

		if (!ret && do_playti)
			ret = scsipt_play_trkidx(dp, DI_ROLE_MAIN,
						 trk, idx, trk, idx);
	}

	if (ret) {
		scsipt_playing = TRUE;
		scsipt_paused = FALSE;
	}

	return (ret);
}


/*
 * scsipt_do_pause_resume
 *	General top-level pause/resume function
 *
 * Args:
 *	dp - Device descriptor
 *	resume - Whether to resume or pause
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_do_pause_resume(di_dev_t *dp, bool_t resume)
{
	bool_t	ret = FALSE;

	if (scsipt_vutbl[app_data.vendor_code].pause_resume != NULL)
		ret = scsipt_vutbl[app_data.vendor_code].pause_resume(resume);

	/* If the device does not claim SCSI-2 compliance, and the
	 * device-specific configuration is not SCSI-2, then don't
	 * attempt to deliver SCSI-2 commands to the device.
	 */
	if (!ret && app_data.vendor_code != VENDOR_SCSI2 &&
	    scsipt_dev_scsiver < 2)
		return FALSE;

	if (!ret && app_data.pause_supp)
		ret = scsipt_pause_resume(dp, DI_ROLE_MAIN, resume);

	if (!ret && resume) {
		int		i;
		sword32_t	saddr,
				eaddr;
		msf_t		smsf,
				emsf;
		curstat_t	*s = di_clinfo->curstat_addr();

		/* Resume failed: try restarting playback */
		saddr = s->curpos_tot.addr;
		smsf.min = s->curpos_tot.min;
		smsf.sec = s->curpos_tot.sec;
		smsf.frame = s->curpos_tot.frame;

		if (s->program || s->shuffle) {
			i = -1;

			if (s->prog_cnt > 0)
			    i = (int) s->trkinfo[s->prog_cnt - 1].playorder;

			if (i < 0) {
				if (ret)
					scsipt_paused = !resume;

				return (ret);
			}

			eaddr = s->trkinfo[i+1].addr;
			emsf.min = s->trkinfo[i+1].min;
			emsf.sec = s->trkinfo[i+1].sec;
			emsf.frame = s->trkinfo[i+1].frame;
		}
		else {
			eaddr = s->discpos_tot.addr;
			emsf.min = s->discpos_tot.min;
			emsf.sec = s->discpos_tot.sec;
			emsf.frame = s->discpos_tot.frame;
		}

		ret = scsipt_do_playaudio(dp,
			ADDR_BLK | ADDR_MSF,
			saddr, eaddr, &smsf, &emsf, 0, 0
		);
	}

	if (ret)
		scsipt_paused = !resume;

	return (ret);
}


/*
 * scsipt_do_start_stop
 *	General top-level start/stop function
 *
 * Args:
 *	dp - Device descriptor
 *	start - Whether to start unit or stop unit
 *	loej - Whether caddy load/eject operation should be performed
 *	prnerr - Whether to print error messages if the command fails
 *		 (SCSI-2 mode only)
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_do_start_stop(di_dev_t *dp, bool_t start, bool_t loej, bool_t prnerr)
{
	bool_t	ret = FALSE;

	if (app_data.strict_pause_resume && scsipt_paused)
		(void) scsipt_do_pause_resume(dp, TRUE);

	if (!app_data.load_supp && start && loej)
		return FALSE;

	if (!app_data.eject_supp)
		loej = 0;

#if defined(SOL2_VOLMGT) && !defined(DEMO_ONLY)
	/* Sun Hack: Under Solaris 2.x with the Volume Manager
	 * we need to use a special SunOS ioctl to eject the CD.
	 */
	if (app_data.sol2_volmgt && !start && loej)
		ret = sol2_volmgt_eject(dp);
	else
#endif
	{
		if (!start && loej &&
		    scsipt_vutbl[app_data.vendor_code].eject != NULL)
			ret = scsipt_vutbl[app_data.vendor_code].eject();

		if (!ret &&
		    scsipt_vutbl[app_data.vendor_code].start_stop != NULL)
			ret = scsipt_vutbl[app_data.vendor_code].start_stop(
				start, loej
			);

		if (!ret)
			ret = scsipt_start_stop(dp, DI_ROLE_MAIN,
						start, loej, prnerr);
	}

	if (ret && !start)
		scsipt_playing = FALSE;

	return (ret);
}


/*
 * scsipt_play_recov
 *	Playback interruption recovery handler: Restart playback
 *	after skipping some frames.
 *
 * Args:
 *	blk   - Interruption frame address
 *	iserr - Whether interruption is due to error
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_play_recov(sword32_t blk, bool_t iserr)
{
	msf_t		recov_start_msf;
	sword32_t	recov_start_addr;
	bool_t		ret;

	ret = TRUE;

	recov_start_addr = blk + ERR_SKIPBLKS;
	util_blktomsf(
		recov_start_addr,
		&recov_start_msf.min,
		&recov_start_msf.sec,
		&recov_start_msf.frame,
		MSF_OFFSET
	);

	/* Check to see if we have skipped past
	 * the end.
	 */
	if (recov_start_msf.min > scsipt_sav_end_msf.min)
		ret = FALSE;
	else if (recov_start_msf.min == scsipt_sav_end_msf.min) {
		if (recov_start_msf.sec > scsipt_sav_end_msf.sec)
			ret = FALSE;
		else if ((recov_start_msf.sec ==
			  scsipt_sav_end_msf.sec) &&
			 (recov_start_msf.frame >
			  scsipt_sav_end_msf.frame)) {
			ret = FALSE;
		}
	}
	if (recov_start_addr >= scsipt_sav_end_addr)
		ret = FALSE;

	if (ret) {
		/* Restart playback */
		if (iserr) {
			(void) fprintf(errfp,
				"CD audio: %s (%02u:%02u.%02u)\n",
				app_data.str_recoverr,
				recov_start_msf.min,
				recov_start_msf.sec,
				recov_start_msf.frame
			);
		}

		ret = scsipt_do_playaudio(devp,
			scsipt_sav_end_fmt,
			recov_start_addr, scsipt_sav_end_addr,
			&recov_start_msf, &scsipt_sav_end_msf,
			0, 0
		);
	}

	return (ret);
}


/*
 * scsipt_get_playstatus
 *	Obtain and update current playback status information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - Audio playback is in progress
 *	FALSE - Audio playback stopped or command failure
 */
STATIC bool_t
scsipt_get_playstatus(curstat_t *s)
{
	cdstat_t		cdstat;
	int			trkno,
				idxno;
	bool_t			done,
				ret = FALSE;
	static int		errcnt = 0,
				nostatcnt = 0;
	static sword32_t	errblk = 0;
	static bool_t		in_scsipt_get_playstatus = FALSE;


	/* Lock this routine from multiple entry */
	if (in_scsipt_get_playstatus)
		return TRUE;

	in_scsipt_get_playstatus = TRUE;

	if (scsipt_vutbl[app_data.vendor_code].get_playstatus != NULL) {
		ret = scsipt_vutbl[app_data.vendor_code].get_playstatus(
			&cdstat
		);
	}

	/* If the device does not claim SCSI-2 compliance, and the
	 * device-specific configuration is not SCSI-2, then don't
	 * attempt to deliver SCSI-2 commands to the device.
	 */
	if (!ret && app_data.vendor_code != VENDOR_SCSI2 &&
	    scsipt_dev_scsiver < 2) {
		in_scsipt_get_playstatus = FALSE;
		return FALSE;
	}

	if (!ret) {
		if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
			ret = cdda_getstatus(devp, s, &cdstat);
			if (ret &&
			    (cdstat.level != s->level ||
			     cdstat.level_left  != s->level_left ||
			     cdstat.level_right != s->level_right)) {
				int	vol;

				/* Update volume & balance level */
				s->level_left = cdstat.level_left;
				s->level_right = cdstat.level_right;
				vol = scsipt_cfg_vol(
					(int) cdstat.level, s, FALSE, FALSE
				);

				if (vol >= 0) {
					s->level = (byte_t) vol;
					SET_VOL_SLIDER(s->level);
					SET_BAL_SLIDER((int)
						(s->level_right -
						 s->level_left) / 2
					);
				}

			}
		}
		else
			ret = scsipt_rdsubq(devp, DI_ROLE_MAIN, SUB_CURPOS, 0,
					    &cdstat, NULL, NULL, TRUE);
	}

	if (!ret) {
		/* Check to see if the disc had been manually ejected */
		if (!scsipt_disc_ready(s)) {
			scsipt_sav_end_addr = 0;
			scsipt_sav_end_msf.min = 0;
			scsipt_sav_end_msf.sec = 0;
			scsipt_sav_end_msf.frame = 0;
			scsipt_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_scsipt_get_playstatus = FALSE;
			return FALSE;
		}

		/* Can't get play status for some unknown reason.
		 * Just return success and hope the next poll
		 * succeeds.  We don't want to return FALSE here
		 * because that would stop the poll.
		 */
		in_scsipt_get_playstatus = FALSE;
		return TRUE;
	}

	trkno = cdstat.track;
	idxno = cdstat.index;
	s->curpos_tot = cdstat.abs_addr;	/* structure copy */
	s->curpos_trk = cdstat.rel_addr;	/* structure copy */

	s->tot_frm = cdstat.tot_frm;
	s->frm_played = cdstat.frm_played;
	s->frm_per_sec = cdstat.frm_per_sec;

	/* Update time display */
	DPY_TIME(s, FALSE);

	if (trkno != s->cur_trk) {
		s->cur_trk = trkno;
		/* Update track number display */
		DPY_TRACK(s);
	}

	if (idxno != s->cur_idx) {
		s->cur_idx = idxno;
		s->sav_iaddr = s->curpos_tot.addr;
		/* Update index number display */
		DPY_INDEX(s);
	}

	/* Update play mode display */
	DPY_PLAYMODE(s, FALSE);

	/* Hack: to work around the fact that some CD drives return
	 * "paused" status after issuing a Stop Unit command.
	 * Just treat the status as completed if we get a paused status
	 * and we don't expect the drive to be paused.
	 */
	if (cdstat.status == CDSTAT_PAUSED && s->mode != MOD_PAUSE &&
	    !scsipt_idx_pause)
		cdstat.status = CDSTAT_COMPLETED;

	/* Force completion status */
	if (scsipt_fake_stop)
		cdstat.status = CDSTAT_COMPLETED;

	/* Deal with playback status */
	switch (cdstat.status) {
	case CDSTAT_PLAYING:
	case CDSTAT_PAUSED:
		nostatcnt = 0;
		done = FALSE;

		/* If we haven't encountered an error for a while, then
		 * clear the error count.
		 */
		if (errcnt > 0 && (s->curpos_tot.addr - errblk) > ERR_CLRTHRESH)
			errcnt = 0;
		break;

	case CDSTAT_FAILED:
		nostatcnt = 0;
		/* Check to see if the disc had been manually ejected */
		if (!scsipt_disc_ready(s)) {
			scsipt_sav_end_addr = 0;
			scsipt_sav_end_msf.min = 0;
			scsipt_sav_end_msf.sec = 0;
			scsipt_sav_end_msf.frame = 0;
			scsipt_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_scsipt_get_playstatus = FALSE;
			return FALSE;
		}

		/* Audio playback stopped due to a disc error.  We will
		 * try to restart the playback by skipping a few frames
		 * and continuing.  This will cause a glitch in the sound
		 * but is better than just stopping.
		 */
		done = FALSE;

		/* Check for max errors limit */
		if (++errcnt > MAX_RECOVERR) {
			done = TRUE;
			(void) fprintf(errfp, "CD audio: %s\n",
				       app_data.str_maxerr);
		}

		errblk = s->curpos_tot.addr;

		if (!done && scsipt_play_recov(errblk, TRUE)) {
			in_scsipt_get_playstatus = FALSE;
			return TRUE;
		}

		/*FALLTHROUGH*/

	case CDSTAT_COMPLETED:
	case CDSTAT_NOSTATUS:
	default:
		if (cdstat.status == CDSTAT_NOSTATUS && nostatcnt++ < 20) {
			/* Allow 20 occurrences of nostatus, then stop */
			done = FALSE;
			break;
		}
		nostatcnt = 0;
		done = TRUE;

		if (!scsipt_fake_stop)
			scsipt_playing = FALSE;

		scsipt_fake_stop = FALSE;

		switch (s->mode) {
		case MOD_SAMPLE:
			done = !scsipt_run_sample(s);
			break;

		case MOD_PLAY:
		case MOD_PAUSE:
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			if (s->shuffle || s->program)
				done = !scsipt_run_prog(s);

			if (s->repeat && (s->program || !app_data.multi_play))
				done = !scsipt_run_repeat(s);

			if (s->repeat && s->segplay == SEGP_AB)
				done = !scsipt_run_ab(s);

			break;
		}

		break;
	}

	if (done) {
		byte_t	omode;
		bool_t	prog;

		/* Save some old states */
		omode = s->mode;
		prog = (s->program || s->onetrk_prog);

		/* Reset states */
		di_reset_curstat(s, FALSE, FALSE);
		s->mode = MOD_STOP;

		/* Cancel a->? if the user didn't select an end point */
		if (s->segplay == SEGP_A) {
			s->segplay = SEGP_NONE;
			DPY_PROGMODE(s, FALSE);
		}

		scsipt_sav_end_addr = 0;
		scsipt_sav_end_msf.min = scsipt_sav_end_msf.sec =
			scsipt_sav_end_msf.frame = 0;
		scsipt_sav_end_fmt = 0;
		errcnt = 0;
		errblk = 0;

		if (app_data.multi_play && omode == MOD_PLAY && !prog) {
			bool_t	cont;

			s->prev_disc = s->cur_disc;

			if (app_data.reverse) {
				if (s->cur_disc > s->first_disc) {
					/* Play the previous disc */
					s->cur_disc--;
					scsipt_mult_autoplay = TRUE;
				}
				else {
					/* Go to the last disc */
					s->cur_disc = s->last_disc;

					if (s->repeat) {
						s->rptcnt++;
						scsipt_mult_autoplay = TRUE;
					}
				}
			}
			else {
				if (s->cur_disc < s->last_disc) {
					/* Play the next disc */
					s->cur_disc++;
					scsipt_mult_autoplay = TRUE;
				}
				else {
					/* Go to the first disc.  */
					s->cur_disc = s->first_disc;

					if (s->repeat) {
						s->rptcnt++;
						scsipt_mult_autoplay = TRUE;
					}
				}
			}

			if ((cont = scsipt_mult_autoplay) == TRUE) {
				/* Allow recursion from this point */
				in_scsipt_get_playstatus = FALSE;
			}

			/* Change disc */
			scsipt_chgdisc(s);

			if (cont)
				return TRUE;
		}

		s->rptcnt = 0;
		DPY_ALL(s);

		if (app_data.done_eject) {
			/* Eject the disc */
			scsipt_load_eject(s);
		}
		else {
			/* Spin down the disc */
			(void) scsipt_do_start_stop(devp, FALSE, FALSE, TRUE);
		}
		if (app_data.done_exit) {
			/* Exit */
			di_clinfo->quit(s);
		}

		in_scsipt_get_playstatus = FALSE;
		return FALSE;
	}

	in_scsipt_get_playstatus = FALSE;
	return TRUE;
}


/*
 * scsipt_cfg_vol
 *	Audio volume control function
 *
 * Args:
 *	vol - Logical volume value to set to
 *	s - Pointer to the curstat_t structure
 *	query - If TRUE, query current volume only
 *	user - Whether a volume update is due to user action
 *
 * Return:
 *	The current logical volume value, or -1 on failure.
 */
STATIC int
scsipt_cfg_vol(int vol, curstat_t *s, bool_t query, bool_t user)
{
	int			vol1,
				vol2;
	mode_sense_6_data_t	*ms_data6 = NULL;
	mode_sense_10_data_t	*ms_data10 = NULL;
	blk_desc_t		*bdesc;
	audio_pg_t		*audiopg;
	int			bdesclen;
	byte_t			buf[SZ_MSENSE];
	bool_t			ret = FALSE;
	static bool_t		muted = FALSE;

	if (s->mode == MOD_BUSY)
		return -1;

	if (PLAYMODE_IS_STD(app_data.play_mode) &&
	    scsipt_vutbl[app_data.vendor_code].volume != NULL) {
		vol = scsipt_vutbl[app_data.vendor_code].volume(vol, s, query);
		return (vol);
	}

	if (PLAYMODE_IS_STD(app_data.play_mode) &&
	    scsipt_vutbl[app_data.vendor_code].mute != NULL) {
		if (!query) {
			if (vol < (int) s->level)
				vol = 0;
			else if (vol > (int) s->level ||
				 (vol != 0 && vol != 100))
				vol = 100;

			ret = scsipt_vutbl[app_data.vendor_code].mute(
				(bool_t) (vol == 0)
			);
			if (ret)
				muted = (vol == 0);
		}

		vol = muted ? 0 : MAX_VOL;

		/* Force volume slider to full mute or max positions */
		if (user)
			SET_VOL_SLIDER(vol);

		return (vol);
	}

	if (!app_data.mselvol_supp)
		return 0;

	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		return (cdda_vol(devp, s, vol, query));

	(void) memset(buf, 0, sizeof(buf));

	if (!scsipt_modesense(devp, DI_ROLE_MAIN, buf, 0,
			      PG_AUDIOCTL, SZ_AUDIOCTL))
		return -1;

	if (app_data.msen_10) {
		ms_data10 = (mode_sense_10_data_t *)(void *) buf;
		bdesc = (blk_desc_t *)(void *) ms_data10->data;
		bdesclen = (int)
			util_bswap16((word16_t) ms_data10->bdescr_len);
		ms_data10->data_len = 0;
		ms_data10->medium = 0;
	}
	else {
		ms_data6 = (mode_sense_6_data_t *)(void *) buf;
		bdesc = (blk_desc_t *)(void *) ms_data6->data;
		bdesclen = (int) ms_data6->bdescr_len;
		ms_data6->data_len = 0;
		ms_data6->medium = 0;
	}
	audiopg = (audio_pg_t *)(void *) ((byte_t *) bdesc + bdesclen);

	if (bdesclen > 0)
		bdesc->num_blks = 0;

	if (audiopg->pg_code == PG_AUDIOCTL) {
		if (query) {
			vol1 = util_untaper_vol(
				util_unscale_vol((int) audiopg->p0_vol)
			);
			vol2 = util_untaper_vol(
				util_unscale_vol((int) audiopg->p1_vol)
			);
			scsipt_route_left = (byte_t) audiopg->p0_ch_ctrl;
			scsipt_route_right = (byte_t) audiopg->p1_ch_ctrl;

			if (vol1 == vol2) {
				s->level_left = s->level_right = 100;
				vol = vol1;
			}
			else if (vol1 > vol2) {
				s->level_left = 100;
				s->level_right = (byte_t)((vol2 * 100) / vol1);
				vol = vol1;
			}
			else {
				s->level_left = (byte_t) ((vol1 * 100) / vol2);
				s->level_right = 100;
				vol = vol2;
			}
			return (vol);
		}
		else {
			audiopg->p0_vol = util_scale_vol(
			    util_taper_vol(vol * (int) s->level_left / 100)
			);
			audiopg->p1_vol = util_scale_vol(
			    util_taper_vol(vol * (int) s->level_right / 100)
			);

			audiopg->p0_ch_ctrl = scsipt_route_left;
			audiopg->p1_ch_ctrl = scsipt_route_right;

			audiopg->sotc = 0;
			audiopg->immed = 1;

			if (scsipt_modesel(devp, DI_ROLE_MAIN, buf,
					   PG_AUDIOCTL, SZ_AUDIOCTL)) {
				/* Success */
				return (vol);
			}
			else if (audiopg->p0_vol != audiopg->p1_vol) {
				/* Set the balance to the center
				 * and retry.
				 */
				audiopg->p0_vol = audiopg->p1_vol =
					util_scale_vol(util_taper_vol(vol));

				if (scsipt_modesel(devp, DI_ROLE_MAIN,
						   buf, PG_AUDIOCTL,
						   SZ_AUDIOCTL)) {
					/* Success: Warp balance control */
					s->level_left = s->level_right = 100;
					SET_BAL_SLIDER(0);

					return (vol);
				}

				/* Still failed: just drop through */
			}
		}
	}

	return -1;
}


/*
 * scsipt_vendor_model
 *	Query and update CD drive vendor/model/revision information,
 *	and check the device type.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE  - success
 *	FALSE - The device is not a CD-ROM or WORM
 */
STATIC bool_t
scsipt_vendor_model(curstat_t *s)
{
	inquiry_data_t	inq;
	int		i;
	char		errstr[ERR_BUF_SZ];

	i = 0;
	do {
		(void) memset((byte_t *) &inq, 0, sizeof(inq));

		if (scsipt_inquiry(devp, DI_ROLE_MAIN,
				   (byte_t *) &inq, sizeof(inq)))
			break;

		util_delayms(1000);
	} while (++i < 3);

	if (i >= 3)
		/* Can't do inquiry: shrug. */
		return FALSE;

#ifdef _AIX
	/* Hack: Some versions of AIX 4.x returns
	 * bogus SCSI inquiry data.  Attempt to work
	 * around this.
	 */
	{
		int	j;
		char	*p;

		p = (char *) &inq;
		for (j = 0; j < 8; j++, p++) {
			if (!isalnum(*p) && !isspace(*p))
				break;
		}
		if (j == 8) {
			/* Fix up inquiry data */
			(void) memset((byte_t *) &inq, 0, sizeof(inq));

			p = (char *) &inq;
			(void) strncpy(s->vendor, p, 8);
			s->vendor[8] = '\0';

			p += 8;
			(void) strncpy(s->prod, p, 16);
			s->prod[16] = '\0';

			(void) strncpy((char *) inq.vendor, s->vendor, 8);
			(void) strncpy((char *) inq.prod, s->prod, 16);
			inq.type = DEV_ROM;
			inq.rmb = 1;
			inq.ver = 2;
		}
	}
#endif	/* AIX */

	DBGPRN(DBG_DEVIO)(errfp,
		"\nCD drive: vendor=\"%.8s\" prod=\"%.16s\" rev=\"%.4s\"\n",
		inq.vendor, inq.prod, inq.revnum);

	(void) strncpy(s->vendor, (char *) inq.vendor, 8);
	s->vendor[8] = '\0';

	(void) strncpy(s->prod, (char *) inq.prod, 16);
	s->prod[16] = '\0';

	(void) strncpy(s->revnum, (char *) inq.revnum, 4);
	s->revnum[4] = '\0';

#ifndef OEM_CDROM
	/* Check for errors.
	 * Note: Some OEM drives identify themselves
	 * as a hard disk instead of a CD-ROM drive
	 * (such as the Toshiba CD-ROM XM revision 1971
	 * OEMed by SGI).  In order to use those units
	 * this file must be compiled with -DOEM_CDROM.
	 */
	if ((inq.type != DEV_ROM && inq.type != DEV_WORM) || !inq.rmb){
		/* Not a CD-ROM or a WORM device */
		(void) sprintf(errstr, app_data.str_notrom, s->curdev);
		DI_FATAL(errstr);
		return FALSE;
	}
#endif

	/* Check for unsupported drives */
	scsipt_dev_scsiver = (byte_t) (inq.ver & 0x07);
	if (app_data.scsiverck &&
	    scsipt_dev_scsiver < 2 && app_data.vendor_code == VENDOR_SCSI2) {
		/* Not SCSI-2 or later */
		(void) sprintf(errstr, app_data.str_notscsi2, s->curdev);
		DI_WARNING(errstr);
	}

	return TRUE;
}


/*
 * scsipt_fix_toc
 *	CD Table Of Contents post-processing function.  This is to patch
 *	the end-of-audio position to handle "enhanced CD" or "CD Extra"
 *	multisession CDs.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_fix_toc(curstat_t *s)
{
	int			i;
	byte_t			*cp,
				buf[12];
	toc_hdr_t		*h;
	toc_trk_descr_t		*p;

	/*
	 * Try primitive method first.
	 * Set the end-of-audio to the first data track after the first
	 * track, minus 02:32:00, if applicable.  The 02:32:00 is the
	 * inter-session lead-out and lead-in time plus the 2-second
	 * pre-gap for the last session.
	 */
	for (i = 1; i < (int) s->tot_trks; i++) {
		if (s->trkinfo[i].type == TYP_DATA) {
			s->discpos_tot.addr = s->trkinfo[i].addr;
			s->discpos_tot.addr -= 11400;
			util_blktomsf(
				s->discpos_tot.addr,
				&s->discpos_tot.min,
				&s->discpos_tot.sec,
				&s->discpos_tot.frame,
				MSF_OFFSET
			);
			break;
		}
	}

	if (app_data.vendor_code != VENDOR_SCSI2)
		return;

	/* Check if the CD is a multi-session disc */
	if (!scsipt_rdtoc(devp, DI_ROLE_MAIN, buf, sizeof(buf), 0, 1, FALSE,
			  (bool_t) ((app_data.debug & DBG_DEVIO) != 0)))
		return;		/* Drive does not support multi-session */

	h = (toc_hdr_t *)(void *) buf;
	cp = (byte_t *) h + sizeof(toc_hdr_t);
	if (((int) util_bswap16(h->data_len) + 2) == sizeof(buf) &&
	    h->first_trk != h->last_trk) {
		/* Multi-session disc detected */
		p = (toc_trk_descr_t *)(void *) cp;

		s->discpos_tot.addr = util_xlate_blk((sword32_t)
			util_bswap32(p->abs_addr.logical)
		);

		/* Subtract 02:32:00 from the start of last session
		 * and fix up end of audio location.  The 02:32:00
		 * is the inter-session lead-out and lead-in time plus
		 * the 2 second pre-gap for the last session.
		 */
		s->discpos_tot.addr -= 11400;

		util_blktomsf(
			s->discpos_tot.addr,
			&s->discpos_tot.min,
			&s->discpos_tot.sec,
			&s->discpos_tot.frame,
			MSF_OFFSET
		);
	}
}


/*
 * scsipt_get_toc
 *	Query and update the CD Table Of Contents
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_get_toc(curstat_t *s)
{
	int			i,
				ntrks;
	byte_t			*cp,
				*toc_end,
				buf[SZ_RDTOC];
	bool_t			ret = FALSE;
	toc_hdr_t		*h;
	toc_trk_descr_t		*p;


	if (scsipt_vutbl[app_data.vendor_code].get_toc != NULL)
		ret = scsipt_vutbl[app_data.vendor_code].get_toc(s);

	if (ret) {
		scsipt_fix_toc(s);
		return TRUE;
	}

	/* If the device does not claim SCSI-2 compliance, and the
	 * device-specific configuration is not SCSI-2, then don't
	 * attempt to deliver SCSI-2 commands to the device.
	 */
	if (!ret && app_data.vendor_code != VENDOR_SCSI2 &&
	    scsipt_dev_scsiver < 2)
		return FALSE;

	(void) memset(buf, 0, sizeof(buf));

	if (!scsipt_rdtoc(devp, DI_ROLE_MAIN, buf, sizeof(buf), 0,
			  0, (bool_t) !app_data.toc_lba, TRUE))
		return FALSE;

	/* Fill curstat structure with TOC data */
	h = (toc_hdr_t *)(void *) buf;
	toc_end = (byte_t *) h + util_bswap16(h->data_len) + 2;

	s->first_trk = h->first_trk;
	s->last_trk = h->last_trk;

	ntrks = (int) (h->last_trk - h->first_trk) + 1;

	/* Hack: workaround CD drive firmware bug
	 * Some CD drives return track numbers in BCD
	 * rather than binary.
	 */
	cp = (byte_t *) h + sizeof(toc_hdr_t);

	for (i = 0; i <= ntrks; i++) {
		int	trk0,
			trk1;

		p = (toc_trk_descr_t *)(void *) cp;

		if (p->trkno == LEAD_OUT_TRACK && i != ntrks) {
			trk0 = util_bcdtol(h->first_trk);
			trk1 = util_bcdtol(h->last_trk);

			if (i == (trk1 - trk0 + 1)) {
				/* BUGLY firmware detected! */
				scsipt_bcd_hack = TRUE;
				s->first_trk = (byte_t) trk0;
				s->last_trk = (byte_t) trk1;
				break;
			}
		}

		cp += sizeof(toc_trk_descr_t);
	}

	/*
	 * Fill in TOC data
	 */
	cp = (byte_t *) h + sizeof(toc_hdr_t);

	for (i = 0; cp < toc_end && i < MAXTRACK; i++) {
		p = (toc_trk_descr_t *)(void *) cp;

		/* Hack: Work around firmware problem on some drives */
		if (i > 0 && s->trkinfo[i-1].trkno == s->last_trk &&
		    p->trkno != LEAD_OUT_TRACK) {
			(void) memset(buf, 0, sizeof(buf));

			if (!scsipt_rdtoc(devp, DI_ROLE_MAIN, buf, sizeof(buf),
					  (int) s->last_trk, 0,
					  (bool_t) !app_data.toc_lba, TRUE))
				return FALSE;

			cp = (byte_t *) h + sizeof(toc_hdr_t) +
			     sizeof(toc_trk_descr_t);

			toc_end = (byte_t *) h + util_bswap16(h->data_len) + 2;

			p = (toc_trk_descr_t *)(void *) cp;
		}

		if (scsipt_bcd_hack)
			/* Hack: BUGLY CD drive firmware */
			s->trkinfo[i].trkno = util_bcdtol(p->trkno);
		else
			s->trkinfo[i].trkno = p->trkno;

		s->trkinfo[i].type =
			(p->trktype == 0) ? TYP_AUDIO : TYP_DATA;

		if (app_data.toc_lba) {
			/* LBA mode */
			s->trkinfo[i].addr = util_xlate_blk((sword32_t)
				util_bswap32(p->abs_addr.logical)
			);
			util_blktomsf(
				s->trkinfo[i].addr,
				&s->trkinfo[i].min,
				&s->trkinfo[i].sec,
				&s->trkinfo[i].frame,
				MSF_OFFSET
			);
		}
		else {
			/* MSF mode */
			s->trkinfo[i].min = p->abs_addr.msf.min;
			s->trkinfo[i].sec = p->abs_addr.msf.sec;
			s->trkinfo[i].frame = p->abs_addr.msf.frame;
			util_msftoblk(
				s->trkinfo[i].min,
				s->trkinfo[i].sec,
				s->trkinfo[i].frame,
				&s->trkinfo[i].addr,
				MSF_OFFSET
			);
		}

		if (p->trkno == LEAD_OUT_TRACK ||
		    s->trkinfo[i-1].trkno == s->last_trk ||
		    i == (MAXTRACK - 1)) {
			s->discpos_tot.min = s->trkinfo[i].min;
			s->discpos_tot.sec = s->trkinfo[i].sec;
			s->discpos_tot.frame = s->trkinfo[i].frame;
			s->tot_trks = (byte_t) i;
			s->discpos_tot.addr = s->trkinfo[i].addr;

			break;
		}

		cp += sizeof(toc_trk_descr_t);
	}

	scsipt_fix_toc(s);

	return TRUE;
}


/*
 * scsipt_start_stat_poll
 *	Start polling the drive for current playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_start_stat_poll(curstat_t *s)
{
	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		scsipt_stat_id = di_clinfo->timeout(
			scsipt_stat_interval,
			scsipt_stat_poll,
			(byte_t *) s
		);

		if (scsipt_stat_id != 0)
			scsipt_stat_polling = TRUE;
	}
}


/*
 * scsipt_stop_stat_poll
 *	Stop polling the drive for current playback status
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_stop_stat_poll(void)
{
	if (scsipt_stat_polling) {
		/* Stop poll timer */
		if (di_clinfo->untimeout != NULL)
			di_clinfo->untimeout(scsipt_stat_id);

		scsipt_stat_polling = FALSE;
	}
}


/*
 * scsipt_start_insert_poll
 *	Start polling the drive for disc insertion
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_start_insert_poll(curstat_t *s)
{
	int	delay;
	bool_t	first = TRUE;

	if (scsipt_insert_polling || app_data.ins_disable ||
	    (s->mode != MOD_BUSY && s->mode != MOD_NODISC))
		return;

	if (app_data.numdiscs > 1 && app_data.multi_play)
		scsipt_ins_interval =
			app_data.ins_interval / app_data.numdiscs;
	else
		scsipt_ins_interval = app_data.ins_interval;

	if (scsipt_ins_interval < 500)
		scsipt_ins_interval = 500;

	if (first) {
		first = FALSE;
		delay = 50;
	}
	else
		delay = scsipt_ins_interval;

	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		scsipt_insert_id = di_clinfo->timeout(
			delay,
			scsipt_insert_poll,
			(byte_t *) s
		);

		if (scsipt_insert_id != 0)
			scsipt_insert_polling = TRUE;
	}
}


/*
 * stat_poll
 *	The playback status polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_stat_poll(curstat_t *s)
{
	if (!scsipt_stat_polling)
		return;

	/* Get current audio playback status */
	if (scsipt_get_playstatus(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			scsipt_stat_id = di_clinfo->timeout(
				scsipt_stat_interval,
				scsipt_stat_poll,
				(byte_t *) s
			);
		}
	}
	else
		scsipt_stat_polling = FALSE;
}


/*
 * insert_poll
 *	The disc insertion polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_insert_poll(curstat_t *s)
{
	/* Check to see if a disc is inserted */
	if (!scsipt_disc_ready(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			scsipt_insert_id = di_clinfo->timeout(
				scsipt_ins_interval,
				scsipt_insert_poll,
				(byte_t *) s
			);
		}
	}
	else
		scsipt_insert_polling = FALSE;
}


/*
 * scsipt_disc_ready
 *	Check if the disc is loaded and ready for use, and update
 *	curstat table.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - Disc is ready
 *	FALSE - Disc is not ready
 */
STATIC bool_t
scsipt_disc_ready(curstat_t *s)
{
	int		i;
	bool_t		no_disc,
			ret;
	static bool_t	first_open = TRUE,
			in_scsipt_disc_ready = FALSE;

#ifdef __VMS
	(void) fprintf(errfp, "");	/* HACK */
#endif

	/* Lock this routine from multiple entry */
	if (in_scsipt_disc_ready)
		return TRUE;

	in_scsipt_disc_ready = TRUE;

	/* If device has not been opened, attempt to open it */
	if (scsipt_not_open) {
		/* Check for another copy of the CD player running on
		 * the specified device.
		 */
		if (!s->devlocked && !di_devlock(s, app_data.device)) {
			s->mode = MOD_BUSY;
			DPY_TIME(s, FALSE);
			scsipt_start_insert_poll(s);
			in_scsipt_disc_ready = FALSE;
			return FALSE;
		}

		s->devlocked = TRUE;
		s->mode = MOD_NODISC;
		DPY_DISC(s);

		if (app_data.numdiscs > 1 &&
		    app_data.chg_method == CHG_SCSI_MEDCHG &&
		    chgp == NULL) {
			/* Open medium-changer device */
			if ((chgp = scsipt_open(di_devlist[1])) != NULL) {
				if (!scsipt_is_enabled(chgp, DI_ROLE_MAIN))
					scsipt_enable(chgp, DI_ROLE_MAIN);

				if (!scsipt_chg_start(s)) {
					scsipt_close(chgp);
					chgp = NULL;
				}
			}
			else {
				/* Changer init failed: set to
				 * single-disc mode.
				 */
				DBGPRN(DBG_DEVIO)(errfp, "Open of %s failed\n",
						di_devlist[1]);
				app_data.numdiscs = 1;
				app_data.chg_method = CHG_NONE;
				app_data.multi_play = FALSE;
				app_data.reverse = FALSE;
				s->first_disc = s->last_disc = s->cur_disc = 1;

				DI_INFO(app_data.str_medchg_noinit);
			}
		}

		if (chgp != NULL) {
			/* Try to load disc */
			for (i = 0; i < 2; i++) {
				ret = scsipt_move_medium(
					chgp, DI_ROLE_MAIN,
					scsipt_mcparm.mtbase,
					(word16_t) (scsipt_mcparm.stbase +
						    s->cur_disc - 1),
					scsipt_mcparm.dtbase
				);

				if (ret)
					break;

				if (scsipt_chg_ready(s))
					(void) scsipt_chg_rdstatus();
			}
		}

		if ((devp = scsipt_open(s->curdev)) != NULL) {
			scsipt_not_open = FALSE;

			if (!scsipt_is_enabled(devp, DI_ROLE_MAIN))
				scsipt_enable(devp, DI_ROLE_MAIN);

			/* Check if a disc is loaded and ready */
			no_disc = !scsipt_disc_present(
				devp, DI_ROLE_MAIN, s, NULL
			);
		}
		else {
			DBGPRN(DBG_DEVIO)(errfp, "Open of %s failed\n",
					s->curdev);
			no_disc = TRUE;
		}
	}
	else {
		/* Just return success if we're playing CDDA */
		if (!scsipt_is_enabled(devp, DI_ROLE_MAIN))
			no_disc = FALSE;
		else if ((no_disc = !scsipt_disc_present(devp, DI_ROLE_MAIN,
							 s, NULL)) == TRUE) {
			/* The disc was manually ejected */
			s->mode = MOD_NODISC;
			di_clear_cdinfo(s, FALSE);
		}
	}

	if (!no_disc) {
		if (first_open) {
			first_open = FALSE;

			/* Start up vendor-unique modules */
			if (scsipt_vutbl[app_data.vendor_code].start != NULL)
				scsipt_vutbl[app_data.vendor_code].start();

			/* Fill in inquiry data */
			if (!scsipt_vendor_model(s)) {
				scsipt_close(devp);
				devp = NULL;
				scsipt_not_open = TRUE;
				in_scsipt_disc_ready = FALSE;
				return FALSE;
			}

			/* Initialize volume/balance/routing controls */
			scsipt_init_vol(s, TRUE);
		}
		else {
			/* Force to current settings */
			(void) scsipt_cfg_vol(s->level, s, FALSE, FALSE);

			/* Set up channel routing */
			scsipt_route(s);
		}
	}

	/* Read disc table of contents if a new disc was detected */
	if (scsipt_not_open || no_disc ||
	    (s->mode == MOD_NODISC && !scsipt_get_toc(s))) {
		if (devp != NULL && app_data.eject_close) {
			scsipt_close(devp);
			devp = NULL;
			scsipt_not_open = TRUE;
		}

		di_reset_curstat(s, TRUE, (bool_t) !app_data.multi_play);
		DPY_ALL(s);

		if (app_data.multi_play) {
			/* This requested disc is not there
			 * or not ready.
			 */
			if (app_data.reverse) {
				if (s->cur_disc > s->first_disc) {
					/* Try the previous disc */
					s->cur_disc--;
				}
				else {
					/* Go to the last disc */
					s->cur_disc = s->last_disc;

					if (scsipt_mult_autoplay) {
					    if (s->repeat)
						s->rptcnt++;
					    else
						scsipt_mult_autoplay = FALSE;
					}
				}
			}
			else {
				if (s->cur_disc < s->last_disc) {
					/* Try the next disc */
					s->cur_disc++;
				}
				else if (!s->chgrscan) {
					/* Go to the first disc */
					s->cur_disc = s->first_disc;

					if (scsipt_mult_autoplay) {
					    if (s->repeat)
						s->rptcnt++;
					    else
						scsipt_mult_autoplay = FALSE;
					}
				}
				else {
					/* Done scanning - no disc */
					STOPSCAN(s);
				}
			}

			if (app_data.chg_method == CHG_SCSI_LUN)
				s->curdev = di_devlist[s->cur_disc-1];
		}

		scsipt_start_insert_poll(s);
		in_scsipt_disc_ready = FALSE;
		return FALSE;
	}

	if (s->mode != MOD_NODISC) {
		in_scsipt_disc_ready = FALSE;
		return TRUE;
	}

	/* Load saved track program, if any */
	PROGGET(s);

	s->mode = MOD_STOP;
	DPY_ALL(s);

	/* Load CD-TEXT information into cache, if so configured */
	(void) memset(scsipt_cdtext_buf, 0, sizeof(scsipt_cdtext_buf));
	if (!app_data.cdtext_dsbl) {
		(void) scsipt_rdtoc(
			devp, DI_ROLE_MAIN, scsipt_cdtext_buf,
			sizeof(scsipt_cdtext_buf), 0, 5, FALSE,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
		);
	}

	/* Get Media catalog number of CD, if available */
	if (!app_data.mcn_dsbl) {
		(void) scsipt_rdsubq(
			devp, DI_ROLE_MAIN, SUB_MCN, 0, NULL, s->mcn, NULL,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
		);
	}

	/* Get ISRC for each track, if available */
	if (!app_data.isrc_dsbl) {
		for (i = 0; i < (int) s->tot_trks; i++) {
			ret = scsipt_rdsubq(
				devp, DI_ROLE_MAIN, SUB_ISRC,
				(byte_t) s->trkinfo[i].trkno,
				NULL, NULL, s->trkinfo[i].isrc,
				(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
			);
			if (!ret) {
				if (i == 0) {
					/* If the command fails with the
					 * first track, then just skip the
					 * rest of the tracks.
					 */
					break;
				}
				else
					continue;
			}
			if (s->trkinfo[i].isrc[0] == '\0') {
				if (i == 0) {
					/* If there is no ISRC data for the
					 * first track, then just skip the
					 * rest of the tracks.
					 */
					break;
				}
				else
					continue;
			}
		}
	}

	/* Set caddy lock configuration */
	if (app_data.caddylock_supp)
		scsipt_lock(s, app_data.caddy_lock);

	if (app_data.load_play || scsipt_mult_autoplay) {
		scsipt_mult_autoplay = FALSE;

		/* Wait a little while for things to settle */
		util_delayms(1000);

		/* Start auto-play */
		if (!scsipt_override_ap)
			scsipt_play_pause(s);

		if (scsipt_mult_pause) {
			scsipt_mult_pause = FALSE;

			if (scsipt_do_pause_resume(devp, FALSE)) {
				scsipt_stop_stat_poll();
				s->mode = MOD_PAUSE;
				DPY_PLAYMODE(s, FALSE);
			}
		}
	}
	else if (app_data.load_spindown) {
		/* Spin down disc in case the user isn't going to
		 * play anything for a while.  This reduces wear and
		 * tear on the drive.
		 */
		(void) scsipt_do_start_stop(devp, FALSE, FALSE, TRUE);
	}

	in_scsipt_disc_ready = FALSE;

	/* Load CD information for this disc.
	 * This operation has to be done outside the scope of
	 * in_scsipt_disc_ready because it may recurse
	 * back into this function.
	 */
	(void) di_get_cdinfo(s);

	return TRUE;
}


/*
 * scsipt_chg_ready
 *	Check if the medium changer is ready for use.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - Changer is ready
 *	FALSE - Changer is not ready
 */
/*ARGSUSED*/
STATIC bool_t
scsipt_chg_ready(curstat_t *s)
{
	int			i;
	bool_t			ret;
	req_sense_data_t	sense_data;

	ret = FALSE;
	if (app_data.chg_method != CHG_SCSI_MEDCHG)
		return (ret);

	for (i = 0; i < 10; i++) {
		ret = scsipt_tst_unit_rdy(
			chgp,
			DI_ROLE_MAIN,
			&sense_data,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
		);
		if (ret)
			break;

		if (sense_data.key == 0x02) {
			/* Changer is not ready, wait */
			util_delayms(1000);
		}
	}

	return (ret);
}


/*
 * scsipt_run_rew
 *	Run search-rewind operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_run_rew(curstat_t *s)
{
	int			i,
				skip_blks;
	sword32_t		addr,
				end_addr;
	msf_t			smsf,
				emsf;
	static sword32_t	start_addr,
				seq;

	/* Find out where we are */
	if (!scsipt_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (scsipt_start_search) {
		scsipt_start_search = FALSE;
		seq = 0;
		i = (int) (addr - skip_blks);
	}
	else {
		if (app_data.skip_spdup > 0 && seq > app_data.skip_spdup)
			/* Speed up search */
			skip_blks *= 3;

		i = (int) (start_addr - skip_blks);
	}

	start_addr = (sword32_t) ((i > di_clip_frames) ? i : di_clip_frames);

	seq++;

	if (s->shuffle || s->program) {
		if ((i = di_curtrk_pos(s)) < 0)
			i = 0;

		if (start_addr < s->trkinfo[i].addr)
			start_addr = s->trkinfo[i].addr;
	}
	else if (s->segplay == SEGP_AB && start_addr < s->bp_startpos_tot.addr)
		start_addr = s->bp_startpos_tot.addr;

	end_addr = start_addr + MAX_SRCH_BLKS;

	util_blktomsf(
		start_addr,
		&smsf.min,
		&smsf.sec,
		&smsf.frame,
		MSF_OFFSET
	);
	util_blktomsf(
		end_addr,
		&emsf.min,
		&emsf.sec,
		&emsf.frame,
		MSF_OFFSET
	);

	/* Play next search interval */
	(void) scsipt_do_playaudio(devp,
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		scsipt_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			scsipt_run_rew,
			(byte_t *) s
		);
	}
}


/*
 * scsipt_stop_rew
 *	Stop search-rewind operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
scsipt_stop_rew(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(scsipt_search_id);
}


/*
 * scsipt_run_ff
 *	Run search-fast-forward operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
scsipt_run_ff(curstat_t *s)
{
	int			i,
				skip_blks;
	sword32_t		addr,
				end_addr;
	msf_t			smsf,
				emsf;
	static sword32_t	start_addr,
				seq;

	/* Find out where we are */
	if (!scsipt_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (scsipt_start_search) {
		scsipt_start_search = FALSE;
		seq = 0;
		start_addr = addr + skip_blks;
	}
	else {
		if (app_data.skip_spdup > 0 && seq > app_data.skip_spdup)
			/* Speed up search */
			skip_blks *= 3;

		start_addr += skip_blks;
	}

	seq++;

	if (s->shuffle || s->program) {
		if ((i = di_curtrk_pos(s)) < 0)
			i = s->tot_trks - 1;
		else if (s->cur_idx == 0)
			/* We're in the lead-in: consider this to be
			 * within the previous track.
			 */
			i--;
	}
	else
		i = s->tot_trks - 1;

	end_addr = start_addr + MAX_SRCH_BLKS;

	if (end_addr >= s->trkinfo[i+1].addr) {
		end_addr = s->trkinfo[i+1].addr;
		start_addr = end_addr - skip_blks;
	}

	if (s->segplay == SEGP_AB && end_addr > s->bp_endpos_tot.addr) {
		end_addr = s->bp_endpos_tot.addr;
		start_addr = end_addr - skip_blks;
	}

	util_blktomsf(
		start_addr,
		&smsf.min,
		&smsf.sec,
		&smsf.frame,
		MSF_OFFSET
	);
	util_blktomsf(
		end_addr,
		&emsf.min,
		&emsf.sec,
		&emsf.frame,
		MSF_OFFSET
	);

	/* Play next search interval */
	(void) scsipt_do_playaudio(devp,
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		scsipt_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			scsipt_run_ff,
			(byte_t *) s
		);
	}
}


/*
 * scsipt_stop_ff
 *	Stop search-fast-forward operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
scsipt_stop_ff(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(scsipt_search_id);
}


/*
 * scsipt_run_ab
 *	Run a->b segment play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
STATIC bool_t
scsipt_run_ab(curstat_t *s)
{
	msf_t	start_msf,
		end_msf;

	if ((s->bp_startpos_tot.addr + app_data.min_playblks) >=
	    s->bp_endpos_tot.addr) {
		DO_BEEP();
		return FALSE;
	}

	start_msf.min = s->bp_startpos_tot.min;
	start_msf.sec = s->bp_startpos_tot.sec;
	start_msf.frame = s->bp_startpos_tot.frame;
	end_msf.min = s->bp_endpos_tot.min;
	end_msf.sec = s->bp_endpos_tot.sec;
	end_msf.frame = s->bp_endpos_tot.frame;
	s->mode = MOD_PLAY;
	DPY_ALL(s);
	scsipt_start_stat_poll(s);

	return (
		scsipt_do_playaudio(devp,
			ADDR_BLK | ADDR_MSF,
			s->bp_startpos_tot.addr, s->bp_endpos_tot.addr,
			&start_msf, &end_msf,
			0, 0
		)
	);
}


/*
 * scsipt_run_sample
 *	Run sample play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_run_sample(curstat_t *s)
{
	sword32_t	saddr,
			eaddr;
	msf_t		smsf,
			emsf;

	if (scsipt_next_sam < s->tot_trks) {
		saddr = s->trkinfo[scsipt_next_sam].addr;
		eaddr = saddr + app_data.sample_blks,

		util_blktomsf(
			saddr,
			&smsf.min,
			&smsf.sec,
			&smsf.frame,
			MSF_OFFSET
		);
		util_blktomsf(
			eaddr,
			&emsf.min,
			&emsf.sec,
			&emsf.frame,
			MSF_OFFSET
		);

		/* Sample only audio tracks */
		if (s->trkinfo[scsipt_next_sam].type != TYP_AUDIO ||
		    scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
				        saddr, eaddr, &smsf, &emsf, 0, 0)) {
			scsipt_next_sam++;
			return TRUE;
		}
	}

	scsipt_next_sam = 0;
	return FALSE;
}


/*
 * scsipt_run_prog
 *	Run program/shuffle play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_run_prog(curstat_t *s)
{
	sword32_t	i,
			start_addr,
			end_addr;
	msf_t		start_msf,
			end_msf;
	bool_t		hasaudio,
			ret;

	if (!s->shuffle && !s->program)
		return FALSE;

	if (scsipt_new_progshuf) {
		scsipt_new_progshuf = FALSE;

		if (s->shuffle)
			/* New shuffle sequence needed */
			di_reset_shuffle(s);
		else
			/* Program play: simply reset the count */
			s->prog_cnt = 0;

		/* Do not allow a program that contains only data tracks */
		hasaudio = FALSE;
		for (i = 0; i < (int) s->prog_tot; i++) {
			if (s->trkinfo[s->trkinfo[i].playorder].type ==
			    TYP_AUDIO) {
				hasaudio = TRUE;
				break;
			}
		}

		if (!hasaudio) {
			DO_BEEP();
			return FALSE;
		}
	}

	if (s->prog_cnt >= s->prog_tot)
		/* Done with program/shuffle play cycle */
		return FALSE;

	if ((i = di_curprog_pos(s)) < 0)
		return FALSE;

	if (s->trkinfo[i].trkno == LEAD_OUT_TRACK)
		return FALSE;

	s->prog_cnt++;
	s->cur_trk = s->trkinfo[i].trkno;
	s->cur_idx = 1;

	start_addr = s->trkinfo[i].addr + s->curpos_trk.addr;
	util_blktomsf(
		start_addr,
		&s->curpos_tot.min,
		&s->curpos_tot.sec,
		&s->curpos_tot.frame,
		MSF_OFFSET
	);
	start_msf.min = s->curpos_tot.min;
	start_msf.sec = s->curpos_tot.sec;
	start_msf.frame = s->curpos_tot.frame;

	end_addr = s->trkinfo[i+1].addr;
	end_msf.min = s->trkinfo[i+1].min;
	end_msf.sec = s->trkinfo[i+1].sec;
	end_msf.frame = s->trkinfo[i+1].frame;

	s->curpos_tot.addr = start_addr;

	if (s->mode != MOD_PAUSE)
		s->mode = MOD_PLAY;

	DPY_ALL(s);

	if (s->trkinfo[i].type == TYP_DATA)
		/* Data track: just fake it */
		return TRUE;

	ret = scsipt_do_playaudio(devp,
		ADDR_BLK | ADDR_MSF,
		start_addr, end_addr,
		&start_msf, &end_msf,
		0, 0
	);

	if (s->mode == MOD_PAUSE) {
		(void) scsipt_do_pause_resume(devp, FALSE);

		/* Restore volume */
		scsipt_mute_off(s);
	}

	return (ret);
}


/*
 * scsipt_run_repeat
 *	Run repeat play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_run_repeat(curstat_t *s)
{
	msf_t	start_msf,
		end_msf;
	bool_t	ret;

	if (!s->repeat)
		return FALSE;

	if (s->shuffle || s->program) {
		ret = TRUE;

		if (s->prog_cnt < s->prog_tot)
			/* Not done with program/shuffle sequence yet */
			return (ret);

		scsipt_new_progshuf = TRUE;
		s->rptcnt++;
	}
	else {
		s->cur_trk = s->first_trk;
		s->cur_idx = 1;

		s->curpos_tot.addr = 0;
		s->curpos_tot.min = 0;
		s->curpos_tot.sec = 0;
		s->curpos_tot.frame = 0;
		s->rptcnt++;
		DPY_ALL(s);

		start_msf.min = s->trkinfo[0].min;
		start_msf.sec = s->trkinfo[0].sec;
		start_msf.frame = s->trkinfo[0].frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		ret = scsipt_do_playaudio(devp,
			ADDR_BLK | ADDR_MSF,
			s->trkinfo[0].addr, s->discpos_tot.addr,
			&start_msf, &end_msf, 0, 0
		);

		if (s->mode == MOD_PAUSE) {
			(void) scsipt_do_pause_resume(devp, FALSE);

			/* Restore volume */
			scsipt_mute_off(s);
		}

	}

	return (ret);
}


/*
 * scsipt_route_val
 *	Return the channel routing control value used in the
 *	SCSI-2 mode parameter page 0xE (audio parameters).
 *
 * Args:
 *	route_mode - The channel routing mode value.
 *	channel - The channel number desired (0=left 1=right).
 *
 * Return:
 *	The routing control value.
 */
STATIC byte_t
scsipt_route_val(int route_mode, int channel)
{
	switch (channel) {
	case 0:
		switch (route_mode) {
		case CHROUTE_NORMAL:
			return 0x1;
		case CHROUTE_REVERSE:
			return 0x2;
		case CHROUTE_L_MONO:
			return 0x1;
		case CHROUTE_R_MONO:
			return 0x2;
		case CHROUTE_MONO:
			return 0x3;
		default:
			/* Invalid value */
			return 0x0;
		}
		/*NOTREACHED*/

	case 1:
		switch (route_mode) {
		case CHROUTE_NORMAL:
			return 0x2;
		case CHROUTE_REVERSE:
			return 0x1;
		case CHROUTE_L_MONO:
			return 0x1;
		case CHROUTE_R_MONO:
			return 0x2;
		case CHROUTE_MONO:
			return 0x3;
		default:
			/* Invalid value */
			return 0x0;
		}
		/*NOTREACHED*/

	default:
		/* Invalid value */
		return 0x0;
	}
}


/*
 * scsipt_chg_rdstatus
 *	Get the current changer element status
 *
 * Args:
 *	None
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_chg_rdstatus(void)
{
	int		i,
			j,
			firstelem,
			numelem,
			datalen,
			desclen,
			pagelen;
	size_t		alloclen;
	byte_t		*buf,
			*cp;
	elemhdr_t	*h;
	elemstat_t	*s,
			*s_end;
	elemdesc_t	*d,
			*d_end;

	/* Initialize element status */
	if (!scsipt_init_elemstat(chgp, DI_ROLE_MAIN)) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Initialize Element Status command error\n",
			di_devlist[1]);
		return FALSE;
	}

	alloclen = (app_data.numdiscs * sizeof(elemdesc_t)) +
		   (4 * SZ_ELEMSTAT) + SZ_ELEMHDR;

	if ((buf = (byte_t *) MEM_ALLOC("elemstat", alloclen)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	(void) memset(buf, 0, alloclen);

	/* Read element status */
	if (!scsipt_read_elemstat(chgp, DI_ROLE_MAIN, buf, alloclen,
				  app_data.numdiscs + 3)) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Read Element Status command error\n",
			di_devlist[1]);
		return FALSE;
	}

	DBGDUMP(DBG_DEVIO)("Element Status data", buf, alloclen);

	h = (elemhdr_t *)(void *) buf;

	firstelem = (int) util_bswap16((word16_t) h->firstelem);
	numelem = (int) util_bswap16((word16_t) h->numelem);
	datalen = (int) util_bswap32((word32_t) h->datalen);

	/* Sanity check */
	if ((datalen + SZ_ELEMHDR) > alloclen) {
		DBGPRN(DBG_DEVIO)(errfp,
		    "%s: Read Element Status data error: datalen too large\n",
		    di_devlist[1]);
		MEM_FREE(buf);
		return FALSE;
	}

	if (app_data.debug & DBG_DEVIO) {
		(void) fprintf(errfp, "ELEMENT STATUS HEADER:\n");
		(void) fprintf(errfp, "  First element: %d\n", firstelem);
		(void) fprintf(errfp, "  Num elements: %d\n", numelem);
		(void) fprintf(errfp, "  Byte count: %d\n", datalen);
	}

	cp = buf + SZ_ELEMHDR;
	s = (elemstat_t *)(void *) cp;
	s_end = (elemstat_t *)(void *) (cp + datalen);

	/* Loop through the element status pages */
	for (i = 0; s < s_end; i++) {
		char	*typestr;

		desclen = (int) util_bswap16((word16_t) s->desclen);
		pagelen = (int) util_bswap32((word32_t) s->pagelen);

		if (app_data.debug & DBG_DEVIO) {
			(void) fprintf(errfp, "ELEMENT STATUS PAGE %d:\n", i);

			switch ((int) s->type) {
			case ELEMTYP_MT:
				typestr = "MT";
				break;
			case ELEMTYP_ST:
				typestr = "ST";
				break;
			case ELEMTYP_IE:
				typestr = "IE";
				break;
			case ELEMTYP_DT:
				typestr = "DT";
				break;
			default:
				typestr = "invalid";
				break;
			}
			(void) fprintf(errfp, "  Type: %d (%s)\n",
				       s->type, typestr);

			(void) fprintf(errfp, "  Pri voltag: %d\n",s->pvoltag);
			(void) fprintf(errfp, "  Alt voltag: %d\n",s->avoltag);
			(void) fprintf(errfp, "  Desc len: %d\n",desclen);
			(void) fprintf(errfp, "  Page len: %d\n",pagelen);
		}

		cp += SZ_ELEMSTAT;
		d = (elemdesc_t *)(void *) cp;
		d_end = (elemdesc_t *)(void *) (cp + pagelen);

		/* Loop through the element status descriptors */
		for (j = 0; d < d_end; j++) {
			word16_t	elemaddr;
			word16_t	srcaddr;

			elemaddr = util_bswap16((word16_t) d->mt.elemaddr);
			srcaddr = util_bswap16((word16_t) d->mt.srcaddr);

			if (app_data.debug & DBG_DEVIO) {
				(void) fprintf(errfp,
					       "  ELEMENT DESCRIPTOR %d:\n",
					       j);
				(void) fprintf(errfp,
					       "    Element addr: 0x%x\n",
					       (int) elemaddr);

				(void) fprintf(errfp, "    Flags1: 0x%02x",
					       (int) cp[2]);
				if (d->ie.full)
					(void) fprintf(errfp, " Full");
				if (d->ie.impexp)
					(void) fprintf(errfp, " ImpExp");
				if (d->ie.excpt)
					(void) fprintf(errfp, " Except");
				if (d->ie.access)
					(void) fprintf(errfp, " Access");
				if (d->ie.exenab)
					(void) fprintf(errfp, " ExEnab");
				if (d->ie.inenab)
					(void) fprintf(errfp, " InEnab");

				(void) fprintf(errfp, "\n    ASC: 0x%02x\n",
					       (int) d->dt.asc);

				(void) fprintf(errfp, "    ASCQ: 0x%02x\n",
					       (int) d->dt.ascq);

				(void) fprintf(errfp, "    Flags2: 0x%02x",
					       (int) cp[6]);
				if (d->dt.luvalid)
					(void) fprintf(errfp, " LUValid");
				if (d->dt.idvalid)
					(void) fprintf(errfp, " IDValid");
				if (d->dt.notbus)
					(void) fprintf(errfp, " NotBus");
				if (d->dt.luvalid)
					(void) fprintf(errfp, " LUN=%d",
						       (int) d->dt.lun);
				if (d->dt.idvalid)
					(void) fprintf(errfp, " ID=%d",
						       (int) d->dt.busaddr);

				(void) fprintf(errfp, "\n    Flags3: 0x%02x",
					       (int) cp[9]);
				if (d->dt.svalid)
					(void) fprintf(errfp, " SValid");
				if (d->dt.invert)
					(void) fprintf(errfp, " Invert");
				if (d->dt.svalid)
					(void) fprintf(errfp, " Src_addr=0x%x",
						       srcaddr);
				(void) fprintf(errfp, "\n");
			}

			if (s->type == ELEMTYP_ST && (d->st.full != 0)) {
				scsipt_medmap[j] = TRUE;
				scsipt_medmap_valid = TRUE;
			}

			cp += desclen;
			d = (elemdesc_t *)(void *) cp;
		}

		s = (elemstat_t *)(void *) d_end;
	}

	if (app_data.debug & DBG_DEVIO) {
		(void) fprintf(errfp, "\nMedium map:\n");
		for (i = 0; i < app_data.numdiscs; i++) {
			(void) fprintf(errfp, "  Slot[%d]: %s\n",
				       i+1,
				       scsipt_medmap[i] ? "full" : "empty");
		}
	}

	MEM_FREE(buf);
	return TRUE;
}


/*
 * scsipt_chg_start
 *	Multi-disc changer startup initialization
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
scsipt_chg_start(curstat_t *s)
{
	inquiry_data_t		*inq;
	mode_sense_6_data_t	*ms_data6 = NULL;
	mode_sense_10_data_t	*ms_data10 = NULL;
	blk_desc_t		*bdesc;
	dev_capab_t		*dcapab;
	elem_addr_t		*eaddr;
	int			bdesclen;
	byte_t			buf[SZ_MSENSE];

	inq = (inquiry_data_t *)(void *) buf;
	if (app_data.msen_10) {
		ms_data10 = (mode_sense_10_data_t *)(void *) buf;
		bdesc = (blk_desc_t *)(void *) ms_data10->data;
	}
	else  {
		ms_data6 = (mode_sense_6_data_t *)(void *) buf;
		bdesc = (blk_desc_t *)(void *) ms_data6->data;
	}

	switch (app_data.chg_method) {
	case CHG_SCSI_MEDCHG:
		break;

	case CHG_OS_IOCTL:
		DBGPRN(DBG_DEVIO)(errfp,
		"%s: OS-ioctl changer method not supported in this mode.\n",
		"SCSI pass-through");
		return FALSE;

	default:
		/* Nothing to do here */
		return TRUE;
	}

	(void) memset(buf, 0, sizeof(buf));

	/* Send SCSI Inquiry */
	if (!scsipt_inquiry(chgp, DI_ROLE_MAIN, buf, sizeof(inquiry_data_t))) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: SCSI Inquiry failed\n", di_devlist[1]);
		return FALSE;
	}

	DBGPRN(DBG_DEVIO)(errfp,
		"\nChanger: vendor=\"%.8s\" prod=\"%.16s\" rev=\"%.4s\"\n",
		inq->vendor, inq->prod, inq->revnum);

	if (inq->type != DEV_CHANGER) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s (type=%d) is not a medium changer device\n",
			di_devlist[1], inq->type);
		return FALSE;
	}

	/* Rezero Unit */
	if (!scsipt_rezero_unit(chgp, DI_ROLE_MAIN)) {
		DBGPRN(DBG_DEVIO)(errfp, "%s: Rezero Unit command error\n",
			di_devlist[1]);
		/* Move on anyway, since Rezero Unit is an optional cmd */
	}

	/* Clear any unit attention condition */
	(void) scsipt_chg_ready(s);

	(void) memset(buf, 0, sizeof(buf));

	/* Check device capabilities */
	if (!scsipt_modesense(chgp, DI_ROLE_MAIN, buf, 0,
			      PG_DEVCAPAB, SZ_DEVCAPAB)) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Mode sense command error\n", di_devlist[1]);
		return FALSE;
	}

	if (app_data.msen_10)
		bdesclen = (int)
			util_bswap16((word16_t) ms_data10->bdescr_len);
	else
		bdesclen = (int) ms_data6->bdescr_len;

	dcapab = (dev_capab_t *)(void *) ((byte_t *) bdesc + bdesclen);

	if (dcapab->pg_code != PG_DEVCAPAB) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Mode sense data error\n", di_devlist[1]);
		return FALSE;
	}

	if (dcapab->stor_dt == 0 || dcapab->stor_st == 0 ||
	    dcapab->move_st_dt == 0 || dcapab->move_dt_st == 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Missing required changer capabilities\n",
			di_devlist[1]);
		return FALSE;
	}

	(void) memset(buf, 0, sizeof(buf));

	/* Get element addresses */
	if (!scsipt_modesense(chgp, DI_ROLE_MAIN, buf, 0,
			      PG_ELEMADDR, SZ_ELEMADDR)) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Mode sense command error\n", di_devlist[1]);
		return FALSE;
	}

	if (app_data.msen_10)
		bdesclen = (int)
			util_bswap16((word16_t) ms_data10->bdescr_len);
	else
		bdesclen = (int) ms_data6->bdescr_len;

	eaddr = (elem_addr_t *)(void *) ((byte_t *) bdesc + bdesclen);

	if (eaddr->pg_code != PG_ELEMADDR) {
		DBGPRN(DBG_DEVIO)(errfp,
			"%s: Mode sense data error\n", di_devlist[1]);
		return FALSE;
	}

	scsipt_mcparm.mtbase = util_bswap16((word16_t) eaddr->mt_addr);
	scsipt_mcparm.stbase = util_bswap16((word16_t) eaddr->st_addr);
	scsipt_mcparm.iebase = util_bswap16((word16_t) eaddr->ie_addr);
	scsipt_mcparm.dtbase = util_bswap16((word16_t) eaddr->dt_addr);

	DBGPRN(DBG_DEVIO)(errfp,
		"\nMedium changer: MT=0x%x ST=0x%x IE=0x%x DT=0x%x\n",
		(int) scsipt_mcparm.mtbase,
		(int) scsipt_mcparm.stbase,
		(int) scsipt_mcparm.iebase,
		(int) scsipt_mcparm.dtbase);

	if ((int) util_bswap16((word16_t) eaddr->st_num) != app_data.numdiscs){
		DBGPRN(DBG_DEVIO)(errfp,
			"Number of discs configuration mismatch\n");
		return FALSE;
	}
	if ((int) util_bswap16((word16_t) eaddr->mt_num) > 1) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Multi-transport changer not supported\n");
		return FALSE;
	}
	if ((int) util_bswap16((word16_t) eaddr->dt_num) > 1) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Multi-spindle changer not supported\n");
		return FALSE;
	}      

	/* Allocate medium map array */
	if (scsipt_medmap == NULL) {
		scsipt_medmap = (bool_t *) MEM_ALLOC(
			"scsipt_medmap", app_data.numdiscs * sizeof(bool_t)
		);
		if (scsipt_medmap == NULL) {
			DI_FATAL(app_data.str_nomemory);
			return FALSE;
		}

		(void) memset(scsipt_medmap, 0,
			      app_data.numdiscs * sizeof(bool_t));
	}

	/* Check changer status */
	(void) scsipt_chg_rdstatus();	/* Update medium map */

	return TRUE;
}


/*
 * scsipt_chg_halt
 *	Multi-disc changer shutdown
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
scsipt_chg_halt(curstat_t *s)
{
	/* Close medium changer device */
	if (chgp != NULL) {
		scsipt_close(chgp);
		chgp = NULL;
	}

	/* Free medium changer map if allocated */
	if (scsipt_medmap != NULL) {
		MEM_FREE(scsipt_medmap);
		scsipt_medmap = NULL;
	}
}


/***********************
 *   public routines   *
 ***********************/


/*
 * scsipt_reqsense_keystr
 *	Given a request sense key, return an associated descriptive string.
 *
 * Args:
 *	key - The request sense key value
 *
 * Return:
 *	Descriptive text string.
 */
char *
scsipt_reqsense_keystr(int key)
{
	int	i;

	for (i = 0; rsense_keymap[i].text != NULL; i++) {
		if (key == rsense_keymap[i].key)
			return (rsense_keymap[i].text);
	}
	return ("UNKNOWN KEY");
}


/*
 * scsipt_enable
 *	Enable device for SCSI pass-through I/O.
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which I/O is to be enabled
 *
 * Return:
 *	Nothing.
 */
void
scsipt_enable(di_dev_t *dp, int role)
{
	DBGPRN(DBG_DEVIO)(errfp, "\nEnable device: %s role: %d\n",
			  dp->path, role);
	pthru_enable(dp, role);
}


/*
 * scsipt_disable
 *	Disable device for SCSI pass-through I/O.
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which I/O is to be enabled
 *
 * Return:
 *	Nothing.
 */
void
scsipt_disable(di_dev_t *dp, int role)
{
	if (dp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp,
			"\nscsipt_disable: invalid role: %d (device %s)\n",
			role, dp->path
		);
		return;
	}

	DBGPRN(DBG_DEVIO)(errfp, "\nDisable device: %s role: %d\n",
			  dp->path, role);
	pthru_disable(dp, role);
}


/*
 * scsipt_is_enabled
 *	Check whether device is enabled for SCSI pass-through I/O.
 *
 * Args:
 *	dp - Device descriptor
 *	role - Role id for which to check
 *
 * Return:
 *	TRUE  - Enabled.
 *	FALSE - Disabled.
 */
bool_t
scsipt_is_enabled(di_dev_t *dp, int role)
{
	return (pthru_is_enabled(dp, role));
}


/*
 * scsipt_init
 *	Top-level function to initialize the SCSI pass-through and
 *	vendor-unique modules.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_init(curstat_t *s, di_tbl_t *dt)
{
	int	i;

	if (app_data.di_method != DI_SCSIPT)
		/* SCSI pass-through not configured */
		return;

	/* Initialize libdi calling table */
	dt->load_cdtext = scsipt_load_cdtext;
	dt->playmode = scsipt_playmode;
	dt->check_disc = scsipt_check_disc;
	dt->status_upd = scsipt_status_upd;
	dt->lock = scsipt_lock;
	dt->repeat = scsipt_repeat;
	dt->shuffle = scsipt_shuffle;
	dt->load_eject = scsipt_load_eject;
	dt->ab = scsipt_ab;
	dt->sample = scsipt_sample;
	dt->level = scsipt_level;
	dt->play_pause = scsipt_play_pause;
	dt->stop = scsipt_stop;
	dt->chgdisc = scsipt_chgdisc;
	dt->prevtrk = scsipt_prevtrk;
	dt->nexttrk = scsipt_nexttrk;
	dt->previdx = scsipt_previdx;
	dt->nextidx = scsipt_nextidx;
	dt->rew = scsipt_rew;
	dt->ff = scsipt_ff;
	dt->warp = scsipt_warp;
	dt->route = scsipt_route;
	dt->mute_on = scsipt_mute_on;
	dt->mute_off = scsipt_mute_off;
	dt->cddajitter = scsipt_cddajitter;
	dt->debug = scsipt_debug;
	dt->start = scsipt_start;
	dt->icon = scsipt_icon;
	dt->halt = scsipt_halt;
	dt->methodstr = scsipt_methodstr;

	/* Initalize SCSI pass-through module */
	scsipt_stat_polling = FALSE;
	scsipt_stat_interval = app_data.stat_interval;
	scsipt_insert_polling = FALSE;
	scsipt_next_sam = FALSE;
	scsipt_new_progshuf = FALSE;
	scsipt_sav_end_addr = 0;
	scsipt_sav_end_msf.min = scsipt_sav_end_msf.sec =
		scsipt_sav_end_msf.frame = 0;
	scsipt_sav_end_fmt = 0;

#ifdef SETUID_ROOT
#ifdef SOL2_VOLMGT
	if (!app_data.sol2_volmgt)
#endif	/* SOL2_VOLMGT */
	{
		DBGPRN(DBG_DEVIO)(errfp, "\nSetting uid to 0\n");

#ifdef HAS_EUID
		if (util_seteuid(0) < 0 || geteuid() != 0)
#else
		if (setuid(0) < 0 || getuid() != 0)
#endif
		{
			DI_FATAL(app_data.str_moderr);
			return;
		}
	}
#endif	/* SETUID_ROOT */

	/* Initialize curstat structure */
	di_reset_curstat(s, TRUE, TRUE);

	/* Initialize the SCSI-2 entry of the scsipt_vutbl jump table */
	scsipt_vutbl[VENDOR_SCSI2].vendor = "SCSI-2";
	scsipt_vutbl[VENDOR_SCSI2].playaudio = NULL;
	scsipt_vutbl[VENDOR_SCSI2].pause_resume = NULL;
	scsipt_vutbl[VENDOR_SCSI2].start_stop = NULL;
	scsipt_vutbl[VENDOR_SCSI2].get_playstatus = NULL;
	scsipt_vutbl[VENDOR_SCSI2].volume = NULL;
	scsipt_vutbl[VENDOR_SCSI2].route = NULL;
	scsipt_vutbl[VENDOR_SCSI2].mute = NULL;
	scsipt_vutbl[VENDOR_SCSI2].get_toc = NULL;
	scsipt_vutbl[VENDOR_SCSI2].eject = NULL;
	scsipt_vutbl[VENDOR_SCSI2].start = NULL;
	scsipt_vutbl[VENDOR_SCSI2].halt = NULL;

	DBGPRN(DBG_DEVIO)(errfp, "%s\n\t%s\n",
		"libdi: SCSI pass-through method", "SCSI-2");

	/* Initialize all configured vendor-unique modules */
	for (i = 0; i < MAX_VENDORS; i++) {
		if (vuinit[i].init != NULL) {
			vuinit[i].init();
			DBGPRN(DBG_DEVIO)(errfp,
				"\t%s\n", scsipt_vutbl[i].vendor);
		}
	}

	if (app_data.vendor_code != VENDOR_SCSI2 &&
	    vuinit[app_data.vendor_code].init == NULL) {
		DI_FATAL(app_data.str_novu);
	}
}


/*
 * scsipt_load_cdtext
 *	Parse CD-TEXT raw data and fill in the di_cdtext_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	t - Pointer to the di_cdtext_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_load_cdtext(curstat_t *s, di_cdtext_t *t)
{
	size_t			len;
	int			npacks,
				i,
				j,
				k,
				n,
				prevtrk,
				trk,
				valid;
	toc_hdr_t		*h;
	toc_cdtext_pack_t	*p,
				*prevp;
	char			**obj = NULL,
				*prevobj = NULL,
				*info = NULL;
	bool_t			two_byte,
				trk_gap,
				upd_obj;

	/* Initialize */
	(void) memset(t, 0, sizeof(di_cdtext_t));
	t->cdtext_valid = FALSE;

	h = (toc_hdr_t *)(void *) scsipt_cdtext_buf;
	if ((len = util_bswap16(h->data_len)) == 0)
		return;		/* No CD-TEXT data */

	npacks = (len - 2) / sizeof(toc_cdtext_pack_t);
	if (npacks == 0)
		return;		/* No CD-TEXT data */

	/* Allocate scratch buffer */
	info = (char *) MEM_ALLOC("cdtext_info", SZ_CDTEXTINFO);
	if (info == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memset(info, 0, SZ_CDTEXTINFO);

	/*
	 * Parse cached data and fill di_cdtext_t structure
	 * This code is fugly because the raw CD-TEXT data
	 * format is so convoluted.
	 */

	prevp = NULL;
	prevtrk = 0;
	upd_obj = FALSE;
	p = (toc_cdtext_pack_t *)(void *) (scsipt_cdtext_buf + SZ_TOCHDR);
	j = 0;

	for (i = 0; i < npacks; i++, p++) {
		if (p->pack_type == PACK_SIZEINFO ||
		    p->pack_type == PACK_GENRE ||
		    p->pack_type == PACK_TOC ||
		    p->pack_type == PACK_TOC2) {
			n = 0;
			trk = -1;
			two_byte = FALSE;
			trk_gap = FALSE;
		}
		else {
			trk = (int) p->trk_no - (int) s->first_trk;
			two_byte = (bool_t) ((p->blk_char & 0x80) != 0);
			trk_gap = FALSE;

			n = (int) (p->blk_char & 0x0f);
			if (n > SZ_CDTEXTPKD)
				n = SZ_CDTEXTPKD;

			if (trk >= (int) s->tot_trks) {
				/* Invalid track number */
				DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
					"Invalid track number %02u in "
					"CD-TEXT data: skipped.\n",
					(unsigned int) p->trk_no);
				prevp = NULL;
				continue;
			}
			else if (trk == 0) {
				/* First track */
				prevobj = NULL;
			}
			else if (trk > 0 && trk > (prevtrk+1) && j > 0 &&
				 j < SZ_CDTEXTPKD) {
				/* Skipped a track: Because the
				 * residual data in the previous pack
				 * has all of the missing track's data.
				 * Fake it.
				 */
				trk_gap = TRUE;
				trk--;
				p--;
				i--;
			}
			prevtrk = trk;
		}

		switch ((int) p->pack_type) {
		case PACK_TITLE:
			if (trk < 0)
				obj = &t->disc.title;
			else
				obj = &t->track[trk].title;
			break;

		case PACK_PERFORMER:
			if (trk < 0)
				obj = &t->disc.performer;
			else
				obj = &t->track[trk].performer;
			break;

		case PACK_SONGWRITER:
			if (trk < 0)
				obj = &t->disc.songwriter;
			else
				obj = &t->track[trk].songwriter;
			break;

		case PACK_COMPOSER:
			if (trk < 0)
				obj = &t->disc.composer;
			else
				obj = &t->track[trk].composer;
			break;

		case PACK_ARRANGER:
			if (trk < 0)
				obj = &t->disc.arranger;
			else
				obj = &t->track[trk].arranger;
			break;

		case PACK_MESSAGE:
			if (trk < 0)
				obj = &t->disc.message;
			else
				obj = &t->track[trk].message;
			break;
	
		case PACK_IDENT:
			if (trk < 0)
				obj = &t->ident;
			else
				obj = NULL;
			break;

		case PACK_UPCEAN:
			if (trk < 0)
				obj = &t->disc.catno;
			else
				obj = &t->track[trk].catno;
			break;
		
		case PACK_GENRE:
		case PACK_TOC:
		case PACK_TOC2:
			/* These pack types are not supported for now:
			 * I can't find any documentation on its data format
			 * (supposed to be binary)
			 */
			obj = NULL;
			break;

		case PACK_SIZEINFO:
			/* This is just informational only */
			if ((app_data.debug & (DBG_DEVIO|DBG_CDI)) != 0) {
				switch ((int) p->trk_no) {
				case 0:
				    (void) fprintf(errfp,
						"\nCD-TEXT cooked sizeinfo\n");
				    (void) fprintf(errfp,
						"  First track: %d\n"
						"  last track: %d\n",
						(int) p->data[1],
						(int) p->data[2]);
				    if (p->data[3] & 0x80) {
					(void) fprintf(errfp,
						"  Program area CD-TEXT "
						"available\n");
					if (p->data[3] & 0x40) {
					    (void) fprintf(errfp,
						    "Program area copy-"
						    "protection available\n");
					}
				    }
				    if (p->data[3] & 0x07) {
					(void) fprintf(errfp,
						"  Album/Track names "
						"%scopyrighted\n",
						(p->data[3] & 0x01) ?
						"" : "not ");
					(void) fprintf(errfp,
						"  Performer/Songwriter/"
						"Composer/Arranger names "
						"%scopyrighted\n",
						(p->data[3] & 0x02) ?
						"" : "not ");
					(void) fprintf(errfp,
						"  Message info "
						"%scopyrighted\n",
						(p->data[3] & 0x04) ?
						"" : "not ");
				    }
				    (void) fprintf(errfp,
					    "  Album/track names: %d packs\n",
					    (int) p->data[4]);
				    (void) fprintf(errfp,
					    "  Performer names: %d packs\n",
					    (int) p->data[5]);
				    (void) fprintf(errfp,
					    "  Songwriter names: %d packs\n",
					    (int) p->data[6]);
				    (void) fprintf(errfp,
					    "  Composer names: %d packs\n",
					    (int) p->data[7]);
				    (void) fprintf(errfp,
					    "  Arranger names: %d packs\n",
					    (int) p->data[8]);
				    (void) fprintf(errfp,
					    "  Messages: %d packs\n",
					    (int) p->data[9]);
				    (void) fprintf(errfp,
					    "  Ident info: %d packs\n",
					    (int) p->data[10]);
				    (void) fprintf(errfp,
					    "  Genre info: %d packs\n",
					    (int) p->data[11]);
				    break;

				case 1:
				    (void) fprintf(errfp,
					    "  TOC info: %d packs\n",
					    (int) p->data[0]);
				    (void) fprintf(errfp,
					    "  TOC2 info: %d packs\n",
					    (int) p->data[1]);
				    (void) fprintf(errfp,
					    "  UPC/EAN ISRC: %d packs\n",
					    (int) p->data[6]);
				    (void) fprintf(errfp,
					    "  Size info: %d packs\n",
					    (int) p->data[7]);
				    (void) fprintf(errfp,
					    "  Last seq num blks 1-4: "
					    "0x%02x 0x%02x 0x%02x 0x%02x\n",
					    (int) p->data[8],
					    (int) p->data[9],
					    (int) p->data[10],
					    (int) p->data[11]);
				    break;

				case 2:
				    (void) fprintf(errfp,
					    "  Last seq num blks 5-8: "
					    "0x%02x 0x%02x 0x%02x 0x%02x\n",
					    (int) p->data[0],
					    (int) p->data[1],
					    (int) p->data[2],
					    (int) p->data[3]);
				    (void) fprintf(errfp,
					    "  Language codes blks 1-4: "
					    "0x%02x 0x%02x 0x%02x 0x%02x\n",
					    (int) p->data[4],
					    (int) p->data[5],
					    (int) p->data[6],
					    (int) p->data[7]);
				    (void) fprintf(errfp,
					    "  Language codes blks 5-8: "
					    "0x%02x 0x%02x 0x%02x 0x%02x\n",
					    (int) p->data[8],
					    (int) p->data[9],
					    (int) p->data[10],
					    (int) p->data[11]);
				    break;

				default:
				    /* Invalid */
				    break;
				}
			}

			obj = NULL;
			break;

		default:
			DBGPRN(DBG_DEVIO)(errfp,
				"scsipt_load_cdtext: unknown pack type 0x%x\n",
				p->pack_type);
			obj = NULL;
			break;
		}

		if (obj == NULL) {
			prevp = NULL;
			continue;
		}

		if (trk_gap) {
			while (p->data[++j] == '\0' && j < (SZ_CDTEXTPKD-1))
				;
			if (j == (SZ_CDTEXTPKD-1))
				continue;

			for (k = 0; j < SZ_CDTEXTPKD; j++, k++) {
				info[k] = (char) p->data[j];
				if (info[k] == '\0') {
					upd_obj = TRUE;
					break;
				}
			}
			if (j == SZ_CDTEXTPKD)
				(void) memset(info, 0, SZ_CDTEXTINFO);
		}
		else {
			if (prevp != NULL && n > 0 && n < SZ_CDTEXTPKD) {
				k = strlen(info);
				(void) strncpy(
				    &info[k],
				    (char *) &prevp->data[SZ_CDTEXTPKD] - n,
				    n
				);	
				info[k+n] = '\0';
				k = strlen(info);
			}

			for (j = 0, k = strlen(info);
			     j < SZ_CDTEXTPKD;
			     j++, k++) {
				info[k] = (char) p->data[j];
				if (info[k] == '\0') {
					upd_obj = TRUE;
					break;
				}
			}
		}

		if (info[0] != '\0' && upd_obj) {
			if (!two_byte && trk > 0 && prevobj != NULL &&
			    info[0] == '\t') {
				/* One Tab means same as previous track for
				 * single-byte languages.
				 */
				if (!util_newstr(obj, prevobj)) {
					DI_FATAL(app_data.str_nomemory);
					MEM_FREE(info);
					return;
				}
			}
			else if (two_byte && trk > 0 && prevobj != NULL &&
				 info[0] == '\t' && info[1] == '\t') {
				/* Two tabs means same as previous track for
				 * double-byte languages.
				 */
				if (!util_newstr(obj, prevobj)) {
					DI_FATAL(app_data.str_nomemory);
					MEM_FREE(info);
					return;
				}
			}
			else {
				if (*obj == NULL) {
					*obj = (char *) MEM_ALLOC(
						"cdtext_obj", strlen(info) + 1
					);
					if (*obj != NULL)
						**obj = '\0';
				}
				else {
					*obj = (char *) MEM_REALLOC(
						"cdtext_obj", *obj,
						strlen(*obj) + strlen(info) + 1
					);
				}
				if (*obj == NULL) {
					DI_FATAL(app_data.str_nomemory);
					MEM_FREE(info);
					return;
				}
				(void) strcat(*obj, info);
			}

			(void) memset(info, 0, SZ_CDTEXTINFO);
			upd_obj = FALSE;
		}

		prevobj = *obj;
		prevp = p;
	}
	
	MEM_FREE(info);

	/* Check data validity and display "cooked" CD-TEXT data in
	 * debug mode
	 */
	valid = 0;
	DBGPRN(DBG_DEVIO|DBG_CDI)(errfp, "\nCD-TEXT cooked data:\n");
	if (t->ident != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp, "  Ident: %s\n", t->ident);
	}
	if (t->disc.title != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album title: %s\n", t->disc.title);
	}
	if (t->disc.performer != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album performer: %s\n", t->disc.performer);
	}
	if (t->disc.songwriter != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album songwriter: %s\n", t->disc.songwriter);
	}
	if (t->disc.composer != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album composer: %s\n", t->disc.composer);
	}
	if (t->disc.arranger != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album arranger: %s\n", t->disc.arranger);
	}
	if (t->disc.message != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album message: %s\n", t->disc.message);
	}
	if (t->disc.catno != NULL) {
		valid++;
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"  Album UPC/EAN: %s\n", t->disc.catno);
	}

	for (i = 0; i < (int) s->tot_trks; i++) {
		if (t->track[i].title != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d title: %s\n",
				s->trkinfo[i].trkno, t->track[i].title);
		}
		if (t->track[i].performer != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d performer: %s\n",
				s->trkinfo[i].trkno, t->track[i].performer);
		}
		if (t->track[i].songwriter != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d songwriter: %s\n",
				s->trkinfo[i].trkno, t->track[i].songwriter);
		}
		if (t->track[i].composer != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d composer: %s\n",
				s->trkinfo[i].trkno, t->track[i].composer);
		}
		if (t->track[i].arranger != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d arranger: %s\n",
				s->trkinfo[i].trkno, t->track[i].arranger);
		}
		if (t->track[i].message != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d message: %s\n",
				s->trkinfo[i].trkno, t->track[i].message);
		}
		if (t->track[i].catno != NULL) {
			valid++;
			DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
				"  Track %02d ISRC: %s\n",
				s->trkinfo[i].trkno, t->track[i].catno);
		}
	}

	if (valid > 0)
		t->cdtext_valid = TRUE;
	else {
		(void) memset(t, 0, sizeof(di_cdtext_t));
		DBGPRN(DBG_DEVIO|DBG_CDI)(errfp,
			"No valid CD-TEXT information found.\n");
	}
}


/*
 * scsipt_playmode
 *	Init/halt CDDA mode
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_playmode(curstat_t *s)
{
	bool_t		cdda,
			ret;
	static bool_t	prev_cdda = FALSE;

	cdda = (bool_t) PLAYMODE_IS_CDDA(app_data.play_mode);

	if (cdda == prev_cdda)
		return TRUE;	/* No change */

	if (cdda) {
		scsipt_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		scsipt_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		scsipt_cdda_client.warning_msg = di_clinfo->warning_msg;
		scsipt_cdda_client.info_msg = di_clinfo->info_msg;
		scsipt_cdda_client.info2_msg = di_clinfo->info2_msg;

		ret = cdda_init(s, &scsipt_cdda_client);
	}
	else {
		cdda_halt(devp, s);
		ret = TRUE;

		/* Initialize volume/balance/routing controls */
		scsipt_init_vol(s, FALSE);
	}

	if (ret)
		prev_cdda = cdda;

	return (ret);
}


/*
 * scsipt_check_disc
 *	Check if disc is ready for use
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
scsipt_check_disc(curstat_t *s)
{
	return (scsipt_disc_ready(s));
}


/*
 * scsipt_status_upd
 *	Force update of playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_status_upd(curstat_t *s)
{
	(void) scsipt_get_playstatus(s);
}


/*
 * scsipt_lock
 *	Caddy lock function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable caddy lock
 *
 * Return:
 *	Nothing.
 */
void
scsipt_lock(curstat_t *s, bool_t enable)
{
	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC) {
		SET_LOCK_BTN(FALSE);
		return;
	}
	else if (s->mode != MOD_STOP) {
		/* Only allow changing lock status when stopped */
		DO_BEEP();
		SET_LOCK_BTN((bool_t) !enable);
		return;
	}

	if (!scsipt_prev_allow(devp, DI_ROLE_MAIN, enable)) {
		/* Cannot lock/unlock caddy */
		DO_BEEP();
		SET_LOCK_BTN((bool_t) !enable);
		return;
	}

	s->caddy_lock = enable;
	SET_LOCK_BTN(enable);
}


/*
 * scsipt_repeat
 *	Repeat mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable repeat mode
 *
 * Return:
 *	Nothing.
 */
void
scsipt_repeat(curstat_t *s, bool_t enable)
{
	s->repeat = enable;

	if (!enable && scsipt_new_progshuf) {
		scsipt_new_progshuf = FALSE;
		if (s->rptcnt > 0)
			s->rptcnt--;
	}
	DPY_RPTCNT(s);
}


/*
 * scsipt_shuffle
 *	Shuffle mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable shuffle mode
 *
 * Return:
 *	Nothing.
 */
void
scsipt_shuffle(curstat_t *s, bool_t enable)
{
	if (s->segplay == SEGP_A) {
		/* Can't set shuffle during a->? mode */
		DO_BEEP();
		SET_SHUFFLE_BTN((bool_t) !enable);
		return;
	}

	switch (s->mode) {
	case MOD_STOP:
	case MOD_BUSY:
	case MOD_NODISC:
		if (s->program) {
			/* Currently in program mode: can't enable shuffle */
			DO_BEEP();
			SET_SHUFFLE_BTN((bool_t) !enable);
			return;
		}
		break;
	default:
		if (enable) {
			/* Can't enable shuffle unless when stopped */
			DO_BEEP();
			SET_SHUFFLE_BTN((bool_t) !enable);
			return;
		}
		break;
	}

	s->segplay = SEGP_NONE;	/* Cancel a->b mode */
	DPY_PROGMODE(s, FALSE);

	s->shuffle = enable;
	if (!s->shuffle)
		s->prog_tot = 0;
}


/*
 * scsipt_load_eject
 *	CD caddy load and eject function.  If disc caddy is not
 *	loaded, it will attempt to load it.  Otherwise, it will be
 *	ejected.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_load_eject(curstat_t *s)
{
	req_sense_data_t	sense;
	bool_t			dbg;
	
	if (devp == NULL)
		return;

	dbg = (bool_t) ((app_data.debug & DBG_DEVIO) != 0);

	(void) memset(&sense, 0, sizeof(sense));

	if (scsipt_is_enabled(devp, DI_ROLE_MAIN) &&
	    !scsipt_disc_present(devp, DI_ROLE_MAIN, s, &sense)) {
		/* No disc */
		switch ((int) sense.key) {
		case 0x02:	/* Not ready */
			switch ((int) sense.code) {
			case 0x3a:
				switch ((int) sense.qual) {
		    		case 0x02:
					/* Tray open */
					if (app_data.load_supp) {
						/* Close the tray */
						(void) scsipt_do_start_stop(
							devp, TRUE, TRUE, TRUE
						);
					}
					break;

				case 0x01:
					/* Closed but Empty tray */
					if (!app_data.eject_supp)
						break;

					/* Unlock caddy if supported */
					if (app_data.caddylock_supp)
						scsipt_lock(s, FALSE);

					/* Open the tray */
					(void) scsipt_do_start_stop(
						devp, FALSE, TRUE, TRUE
					);
					break;

				case 0x00:
				default:
					/* Can't tell if tray is open or
					 * closed with no disc.  Yuck.
					 */

					if (app_data.load_supp) {
						/* Close the tray */
						(void) scsipt_do_start_stop(
							devp, TRUE, TRUE, dbg
						);
					}

					/* Wait for disc */
					if (scsipt_disc_present(devp,
								DI_ROLE_MAIN,
								s, NULL))
						break;

					if (app_data.eject_supp) {
						/* Opening the tray */
						(void) scsipt_do_start_stop(
							devp, FALSE, TRUE, dbg
						);
					}
					break;
				}
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}

		scsipt_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (devp != NULL && app_data.eject_close) {
			scsipt_close(devp);
			devp = NULL;
			scsipt_not_open = TRUE;
		}

		scsipt_start_insert_poll(s);
		return;
	}

	/* Eject the disc */

	/* Spin down the CD */
	(void) scsipt_do_start_stop(devp, FALSE, FALSE, TRUE);

	if (!app_data.eject_supp) {
		DO_BEEP();

		scsipt_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (devp != NULL && app_data.eject_close) {
			scsipt_close(devp);
			devp = NULL;
			scsipt_not_open = TRUE;
		}

		scsipt_start_insert_poll(s);
		return;
	}

	/* Unlock caddy if supported */
	if (app_data.caddylock_supp)
		scsipt_lock(s, FALSE);

	scsipt_stop_stat_poll();
	di_reset_curstat(s, TRUE, TRUE);
	s->mode = MOD_NODISC;

	di_clear_cdinfo(s, FALSE);
	DPY_ALL(s);

	/* Eject the CD */
	(void) scsipt_do_start_stop(devp, FALSE, TRUE, TRUE);

	if (app_data.eject_exit)
		di_clinfo->quit(s);
	else {
		if (devp != NULL && app_data.eject_close) {
			scsipt_close(devp);
			devp = NULL;
			scsipt_not_open = TRUE;
		}

		scsipt_start_insert_poll(s);
	}
}


/*
 * scsipt_ab
 *	A->B segment play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_ab(curstat_t *s)
{
	if (!scsipt_run_ab(s))
		DO_BEEP();
}


/*
 * scsipt_sample
 *	Sample play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_sample(curstat_t *s)
{
	int	i;

	if (!scsipt_disc_ready(s)) {
		DO_BEEP();
		return;
	}

	if (s->shuffle || s->program || s->segplay != SEGP_NONE) {
		/* Sample is not supported in program/shuffle or a->b modes */
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_STOP:
		scsipt_start_stat_poll(s);
		/*FALLTHROUGH*/
	case MOD_PLAY:
		/* If already playing a track, start sampling the track after
		 * the current one.  Otherwise, sample from the beginning.
		 */
		if (s->cur_trk > 0 && s->cur_trk != s->last_trk) {
			i = di_curtrk_pos(s) + 1;
			s->cur_trk = s->trkinfo[i].trkno;
			scsipt_next_sam = (byte_t) i;
		}
		else {
			s->cur_trk = s->first_trk;
			scsipt_next_sam = 0;
		}
		
		s->cur_idx = 1;

		s->mode = MOD_SAMPLE;
		DPY_ALL(s);

		if (!scsipt_run_sample(s))
			return;

		break;

	case MOD_SAMPLE:
		/* Currently doing Sample playback, just call scsipt_play_pause
		 * to resume normal playback.
		 */
		scsipt_play_pause(s);
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * scsipt_level
 *	Audio volume control function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	level - The volume level to set to
 *	drag - Whether this is an update due to the user dragging the
 *		volume control slider thumb.  If this is FALSE, then
 *		a final volume setting has been found.
 *
 * Return:
 *	Nothing.
 */
void
scsipt_level(curstat_t *s, byte_t level, bool_t drag)
{
	int	actual;

	if (drag && app_data.vendor_code != VENDOR_SCSI2 &&
	    scsipt_vutbl[app_data.vendor_code].volume == NULL)
		return;

	/* Set volume level */
	if ((actual = scsipt_cfg_vol((int) level, s, FALSE, TRUE)) >= 0)
		s->level = (byte_t) actual;
}


/*
 * scsipt_play_pause
 *	Audio playback and pause function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_play_pause(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	scsipt_override_ap = TRUE;

	if (!scsipt_disc_ready(s)) {
		scsipt_override_ap = FALSE;
		DO_BEEP();
		return;
	}

	scsipt_override_ap = FALSE;

	if (s->mode == MOD_NODISC)
		s->mode = MOD_STOP;

	switch (s->mode) {
	case MOD_PLAY:
		/* Currently playing: go to pause mode */

		if (!scsipt_do_pause_resume(devp, FALSE)) {
			DO_BEEP();
			return;
		}
		scsipt_stop_stat_poll();
		s->mode = MOD_PAUSE;
		DPY_PLAYMODE(s, FALSE);
		break;

	case MOD_PAUSE:
		/* Currently paused: resume play */

		if (!scsipt_do_pause_resume(devp, TRUE)) {
			DO_BEEP();
			return;
		}
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		scsipt_start_stat_poll(s);
		break;

	case MOD_STOP:
		/* Currently stopped: start play */

		if (!di_prepare_cdda(s))
			return;

		if (s->shuffle || s->program) {
			scsipt_new_progshuf = TRUE;

			/* Start shuffle/program play */
			if (!scsipt_run_prog(s))
				return;
		}
		else if (s->segplay == SEGP_AB) {
			/* Play defined segment */
			if (!scsipt_run_ab(s))
				return;
		}
		else {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */

			/* Start normal play */
			if ((i = di_curtrk_pos(s)) < 0 || s->cur_trk <= 0) {
				/* Start play from the beginning */
				i = 0;
				s->cur_trk = s->first_trk;
				start_addr = s->trkinfo[0].addr +
					     s->curpos_trk.addr;
				util_blktomsf(
					start_addr,
					&start_msf.min,
					&start_msf.sec,
					&start_msf.frame,
					MSF_OFFSET
				);
			}
			else {
				/* User has specified a starting track */
				start_addr = s->trkinfo[i].addr +
					     s->curpos_trk.addr;
			}

			util_blktomsf(
				start_addr,
				&start_msf.min,
				&start_msf.sec,
				&start_msf.frame,
				MSF_OFFSET
			);

			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			if (s->trkinfo[i].type == TYP_DATA) {
				DPY_TRACK(s);
				DPY_TIME(s, FALSE);
				DO_BEEP();
				return;
			}

			s->cur_idx = 1;
			s->mode = MOD_PLAY;

			if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				s->mode = MOD_STOP;
				return;
			}
		}

		DPY_ALL(s);
		scsipt_start_stat_poll(s);
		break;

	case MOD_SAMPLE:
		/* Force update of curstat */
		if (!scsipt_get_playstatus(s)) {
			DO_BEEP();
			return;
		}

		/* Currently doing a->b or sample playback: just resume play */
		if (s->shuffle || s->program) {
			if ((i = di_curtrk_pos(s)) < 0 ||
			    s->trkinfo[i].trkno == LEAD_OUT_TRACK)
				return;

			start_msf.min = s->curpos_tot.min;
			start_msf.sec = s->curpos_tot.sec;
			start_msf.frame = s->curpos_tot.frame;
			end_msf.min = s->trkinfo[i+1].min;
			end_msf.sec = s->trkinfo[i+1].sec;
			end_msf.frame = s->trkinfo[i+1].frame;

			if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
					  s->curpos_tot.addr,
					  s->trkinfo[i+1].addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}
		}
		else {
			start_msf.min = s->curpos_tot.min;
			start_msf.sec = s->curpos_tot.sec;
			start_msf.frame = s->curpos_tot.frame;
			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
					  s->curpos_tot.addr,
					  s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}
		}
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * scsipt_stop
 *	Stop function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	stop_disc - Whether to actually spin down the disc or just
 *		update status.
 *
 * Return:
 *	Nothing.
 */
void
scsipt_stop(curstat_t *s, bool_t stop_disc)
{
	/* The stop_disc parameter will cause the disc to spin down.
	 * This is usually set to TRUE, but can be FALSE if the caller
	 * just wants to set the current state to stop but will
	 * immediately go into play state again.  Not spinning down
	 * the drive makes things a little faster...
	 */
	if (!scsipt_disc_ready(s))
		return;

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_SAMPLE:
	case MOD_STOP:
		/* Currently playing or paused: stop */

		if (stop_disc &&
		    !scsipt_do_start_stop(devp, FALSE, FALSE, TRUE)) {
			DO_BEEP();
			return;
		}
		scsipt_stop_stat_poll();

		di_reset_curstat(s, FALSE, FALSE);
		s->mode = MOD_STOP;
		s->rptcnt = 0;

		DPY_ALL(s);
		break;

	default:
		break;
	}
}


/*
 * scsipt_chgdisc
 *	Change disc function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_chgdisc(curstat_t *s)
{
	int	i,
		sav_rptcnt;
	bool_t	nochg,
		ret;

	if (s->first_disc == s->last_disc) {
		/* Single-CD drive: cannot change discs */
		DO_BEEP();
		return;
	}

	if (s->cur_disc < s->first_disc || s->cur_disc > s->last_disc) {
		/* Bogus disc number */
		s->cur_disc = s->first_disc;
		return;
	}

	if (s->segplay != SEGP_NONE) {
		s->segplay = SEGP_NONE;	/* Cancel a->b mode */
		DPY_PROGMODE(s, FALSE);
	}

	/* Check target slot against medium map if applicable */
	if (scsipt_medmap_valid && !scsipt_medmap[s->cur_disc-1]) {
		nochg = FALSE;

		if (app_data.multi_play) {
			/* This requested disc slot is empty:
			 * Go to the next loaded slot.
			 */
			nochg = TRUE;
			if (app_data.reverse) {
				while (s->cur_disc > s->first_disc) {
					/* Try the previous loaded slot */
					s->cur_disc--;
					if (scsipt_medmap[s->cur_disc-1]) {
						nochg = FALSE;
						break;
					}
				}
			}
			else {
				while (s->cur_disc < s->last_disc) {
					/* Go to the next loaded slot */
					s->cur_disc++;
					if (scsipt_medmap[s->cur_disc-1]) {
						nochg = FALSE;
						break;
					}
				}
			}
		}
		else
			nochg = TRUE;

		if (nochg)
			/* Can't load from an empty slot */
			return;

		DPY_DISC(s);
	}

	/* If we're currently in normal playback mode, after we change
	 * disc we want to automatically start playback.
	 */
	if ((s->mode == MOD_PLAY || s->mode == MOD_PAUSE) && !s->program)
		scsipt_mult_autoplay = TRUE;

	/* If we're currently paused, go to pause mode after disc change */
	scsipt_mult_pause = (s->mode == MOD_PAUSE);

	sav_rptcnt = s->rptcnt;

	/* Stop the CD first */
	scsipt_stop(s, TRUE);

	/* Unlock caddy if supported */
	if (app_data.caddylock_supp)
		scsipt_lock(s, FALSE);

	di_reset_curstat(s, TRUE, FALSE);
	s->mode = MOD_NODISC;
	di_clear_cdinfo(s, FALSE);

	s->rptcnt = sav_rptcnt;

	if (devp != NULL && app_data.eject_close) {
		scsipt_close(devp);
		devp = NULL;
		scsipt_not_open = TRUE;
	}

	switch (app_data.chg_method) {
	case CHG_SCSI_LUN:
		/* SCSI LUN addressing method */

		/* Set new device */
		s->curdev = di_devlist[s->cur_disc - 1];

		/* Load desired disc */
		(void) scsipt_disc_ready(s);

		break;

	case CHG_SCSI_MEDCHG:
		/* SCSI medium changer method */

		/* Unload old disc */
		if (s->prev_disc > 0) {
			for (i = 0; i < 2; i++) {
				ret = scsipt_move_medium(
					chgp, DI_ROLE_MAIN,
					scsipt_mcparm.mtbase,
					scsipt_mcparm.dtbase,
					(word16_t) (scsipt_mcparm.stbase +
						    s->prev_disc - 1)
				);

				if (ret)
					break;

				if (scsipt_chg_ready(s))
					(void) scsipt_chg_rdstatus();
			}

			s->prev_disc = -1;
		}

		/* Load desired disc */
		(void) scsipt_disc_ready(s);

		break;

	default:
		/* Do nothing */
		break;
	}
}


/*
 * scsipt_prevtrk
 *	Previous track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_prevtrk(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;
	bool_t		go_prev;

	if (!scsipt_disc_ready(s)) {
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		/* Find appropriate track to start */
		if (s->shuffle || s->program) {
			if (s->prog_cnt > 0) {
				s->prog_cnt--;
				scsipt_new_progshuf = FALSE;
			}
			i = di_curprog_pos(s);
		}
		else
			i = di_curtrk_pos(s);

		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		go_prev = FALSE;

		if (i == 0 && s->cur_idx == 0) {
			i = 0;
			start_addr = di_clip_frames;
			util_blktomsf(
				start_addr,
				&start_msf.min,
				&start_msf.sec,
				&start_msf.frame,
				MSF_OFFSET
			);
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 0;
		}
		else {
			start_addr = s->trkinfo[i].addr;
			start_msf.min = s->trkinfo[i].min;
			start_msf.sec = s->trkinfo[i].sec;
			start_msf.frame = s->trkinfo[i].frame;
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 1;

			/* If the current track has been playing for less
			 * than app_data.prev_threshold blocks, then go
			 * to the beginning of the previous track (if we
			 * are not already on the first track).
			 */
			if ((s->curpos_tot.addr - start_addr) <=
			    app_data.prev_threshold)
				go_prev = TRUE;
		}

		if (go_prev) {
			if (s->shuffle || s->program) {
				if (s->prog_cnt > 0) {
					s->prog_cnt--;
					scsipt_new_progshuf = FALSE;
				}
				if ((i = di_curprog_pos(s)) < 0)
					return;

				start_addr = s->trkinfo[i].addr;
				start_msf.min = s->trkinfo[i].min;
				start_msf.sec = s->trkinfo[i].sec;
				start_msf.frame = s->trkinfo[i].frame;
				s->cur_trk = s->trkinfo[i].trkno;
			}
			else if (i == 0) {
				/* Go to the very beginning: this may be
				 * a lead-in area before the start of track 1.
				 */
				start_addr = di_clip_frames;
				util_blktomsf(
					start_addr,
					&start_msf.min,
					&start_msf.sec,
					&start_msf.frame,
					MSF_OFFSET
				);
				s->cur_trk = s->trkinfo[i].trkno;
			}
			else if (i > 0) {
				i--;

				/* Skip over data tracks */
				while (s->trkinfo[i].type == TYP_DATA) {
					if (i <= 0)
						break;
					i--;
				}

				if (s->trkinfo[i].type != TYP_DATA) {
					start_addr = s->trkinfo[i].addr;
					start_msf.min = s->trkinfo[i].min;
					start_msf.sec = s->trkinfo[i].sec;
					start_msf.frame = s->trkinfo[i].frame;
					s->cur_trk = s->trkinfo[i].trkno;
				}
			}
		}

		if (s->mode == MOD_PAUSE)
			/* Mute: so we don't get a transient */
			scsipt_mute_on(s);

		if (s->shuffle || s->program) {
			/* Program/Shuffle mode: just stop the playback
			 * and let scsipt_run_prog go to the previous track
			 */
			scsipt_fake_stop = TRUE;

			/* Force status update */
			(void) scsipt_get_playstatus(s);
		}
		else {
			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			s->curpos_tot.addr = start_addr;
			s->curpos_tot.min = start_msf.min;
			s->curpos_tot.sec = start_msf.sec;
			s->curpos_tot.frame = start_msf.frame;
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			DPY_TRACK(s);
			DPY_INDEX(s);
			DPY_TIME(s, FALSE);

			if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();

				/* Restore volume */
				scsipt_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) scsipt_do_pause_resume(devp, FALSE);

				/* Restore volume */
				scsipt_mute_off(s);
			}
		}

		break;

	case MOD_STOP:
		if (s->shuffle || s->program) {
			/* Pre-selecting tracks not supported in shuffle
			 * or program mode.
			 */
			DO_BEEP();
			return;
		}

		/* Find previous track */
		if (s->cur_trk <= 0) {
			s->cur_trk = s->trkinfo[0].trkno;
			DPY_TRACK(s);
		}
		else {
			i = di_curtrk_pos(s);

			if (i > 0) {
				s->cur_trk = s->trkinfo[i-1].trkno;
				DPY_TRACK(s);
			}
		}
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * scsipt_nexttrk
 *	Next track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_nexttrk(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	if (!scsipt_disc_ready(s)) {
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->shuffle || s->program) {
			if (s->prog_cnt >= s->prog_tot) {
				/* Disallow advancing beyond current
				 * shuffle/program sequence if
				 * repeat mode is not on.
				 */
				if (s->repeat && !app_data.multi_play)
					scsipt_new_progshuf = TRUE;
				else
					return;
			}

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				scsipt_mute_on(s);

			/* Program/Shuffle mode: just stop the playback
			 * and let scsipt_run_prog go to the next track.
			 */
			scsipt_fake_stop = TRUE;

			/* Force status update */
			(void) scsipt_get_playstatus(s);

			return;
		}
		else if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		/* Find next track */
		if ((i = di_curtrk_pos(s)) < 0)
			return;

		if (i > 0 || s->cur_idx > 0)
			i++;

		/* Skip over data tracks */
		while (i < MAXTRACK && s->trkinfo[i].type == TYP_DATA)
			i++;

		if (i < MAXTRACK &&
		    s->trkinfo[i].trkno >= 0 &&
		    s->trkinfo[i].trkno != LEAD_OUT_TRACK) {

			start_addr = s->trkinfo[i].addr;
			start_msf.min = s->trkinfo[i].min;
			start_msf.sec = s->trkinfo[i].sec;
			start_msf.frame = s->trkinfo[i].frame;
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 1;

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				scsipt_mute_on(s);

			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			s->curpos_tot.addr = start_addr;
			s->curpos_tot.min = start_msf.min;
			s->curpos_tot.sec = start_msf.sec;
			s->curpos_tot.frame = start_msf.frame;
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			DPY_TRACK(s);
			DPY_INDEX(s);
			DPY_TIME(s, FALSE);

			if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) scsipt_do_pause_resume(devp, FALSE);

				/* Restore volume */
				scsipt_mute_off(s);
			}
		}

		break;

	case MOD_STOP:
		if (s->shuffle || s->program) {
			/* Pre-selecting tracks not supported in shuffle
			 * or program mode.
			 */
			DO_BEEP();
			return;
		}

		/* Find next track */
		if (s->cur_trk <= 0) {
			s->cur_trk = s->trkinfo[0].trkno;
			DPY_TRACK(s);
		}
		else {
			i = di_curtrk_pos(s) + 1;

			if (i > 0 && s->trkinfo[i].trkno != LEAD_OUT_TRACK) {
				s->cur_trk = s->trkinfo[i].trkno;
				DPY_TRACK(s);
			}
		}
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * scsipt_previdx
 *	Previous index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_previdx(curstat_t *s)
{
	msf_t		start_msf,
			end_msf;
	byte_t		idx;

	if (s->shuffle || s->program) {
		/* Index search is not supported in program/shuffle mode */
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		/* Find appropriate index to start */
		if (s->cur_idx > 1 &&
		    (s->curpos_tot.addr - s->sav_iaddr) <=
			    app_data.prev_threshold)
			idx = s->cur_idx - 1;
		else
			idx = s->cur_idx;
		
		/* This is a Hack...
		 * Since there is no standard SCSI-2 command to start
		 * playback on an index boundary and then go on playing
		 * until the end of the disc, we will use the PLAY AUDIO
		 * TRACK/INDEX command to go to where we want to start,
		 * immediately followed by a PAUSE.  We then find the
		 * current block position and issue a PLAY AUDIO MSF
		 * or PLAY AUDIO(12) command to start play there.
		 * We mute the audio in between these operations to
		 * prevent unpleasant transients.
		 */

		/* Mute */
		scsipt_mute_on(s);

		if (!scsipt_do_playaudio(devp, ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk, idx)) {
			/* Restore volume */
			scsipt_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		scsipt_idx_pause = TRUE;

		if (!scsipt_do_pause_resume(devp, FALSE)) {
			/* Restore volume */
			scsipt_mute_off(s);
			scsipt_idx_pause = FALSE;
			return;
		}

		/* Use scsipt_get_playstatus to update the current status */
		if (!scsipt_get_playstatus(s)) {
			/* Restore volume */
			scsipt_mute_off(s);
			scsipt_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			scsipt_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			scsipt_idx_pause = FALSE;
			return;
		}

		scsipt_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) scsipt_do_pause_resume(devp, FALSE);

			/* Restore volume */
			scsipt_mute_off(s);

			/* Force update of curstat */
			(void) scsipt_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * scsipt_nextidx
 *	Next index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_nextidx(curstat_t *s)
{
	msf_t		start_msf,
			end_msf;

	if (s->shuffle || s->program) {
		/* Index search is not supported in program/shuffle mode */
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		/* Find appropriate index to start */
		
		/* This is a Hack...
		 * Since there is no standard SCSI-2 command to start
		 * playback on an index boundary and then go on playing
		 * until the end of the disc, we will use the PLAY AUDIO
		 * TRACK/INDEX command to go to where we want to start,
		 * immediately followed by a PAUSE.  We then find the
		 * current block position and issue a PLAY AUDIO MSF
		 * or PLAY AUDIO(12) command to start play there.
		 * We mute the audio in between these operations to
		 * prevent unpleasant transients.
		 */

		/* Mute */
		scsipt_mute_on(s);

		if (!scsipt_do_playaudio(devp, ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk,
				  (byte_t) (s->cur_idx + 1))) {
			/* Restore volume */
			scsipt_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		scsipt_idx_pause = TRUE;

		if (!scsipt_do_pause_resume(devp, FALSE)) {
			/* Restore volume */
			scsipt_mute_off(s);
			scsipt_idx_pause = FALSE;
			return;
		}

		/* Use scsipt_get_playstatus to update the current status */
		if (!scsipt_get_playstatus(s)) {
			/* Restore volume */
			scsipt_mute_off(s);
			scsipt_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			scsipt_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!scsipt_do_playaudio(devp, ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			scsipt_idx_pause = FALSE;
			return;
		}

		scsipt_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) scsipt_do_pause_resume(devp, FALSE);

			/* Restore volume */
			scsipt_mute_off(s);

			/* Force update of curstat */
			(void) scsipt_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * scsipt_rew
 *	Search-rewind function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_rew(curstat_t *s, bool_t start)
{
	sword32_t	i;
	msf_t		start_msf,
			end_msf;
	byte_t		vol;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		scsipt_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				scsipt_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) scsipt_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE,
					FALSE
				);
			}

			/* Start search rewind */
			scsipt_start_search = TRUE;
			scsipt_run_rew(s);
		}
		else {
			/* Button release */

			scsipt_stop_rew(s);

			/* Update display */
			(void) scsipt_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				scsipt_mute_on(s);
			else
				/* Restore volume */
				scsipt_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == LEAD_OUT_TRACK) {
					/* Restore volume */
					scsipt_mute_off(s);
					return;
				}

				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_msf.min = s->trkinfo[i+1].min;
				end_msf.sec = s->trkinfo[i+1].sec;
				end_msf.frame = s->trkinfo[i+1].frame;

				if (!scsipt_do_playaudio(devp,
						ADDR_BLK | ADDR_MSF,
						s->curpos_tot.addr,
						s->trkinfo[i+1].addr,
						&start_msf, &end_msf,
						0, 0)) {
					DO_BEEP();

					/* Restore volume */
					scsipt_mute_off(s);
					return;
				}
			}
			else {
				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_msf.min = s->discpos_tot.min;
				end_msf.sec = s->discpos_tot.sec;
				end_msf.frame = s->discpos_tot.frame;

				if (!scsipt_do_playaudio(devp,
						ADDR_BLK | ADDR_MSF,
						s->curpos_tot.addr,
						s->discpos_tot.addr,
						&start_msf, &end_msf,
						0, 0)) {
					DO_BEEP();

					/* Restore volume */
					scsipt_mute_off(s);
					return;
				}
			}

			if (s->mode == MOD_PAUSE) {
				(void) scsipt_do_pause_resume(devp, FALSE);

				/* Restore volume */
				scsipt_mute_off(s);
			}
			else
				scsipt_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * scsipt_ff
 *	Search-fast-forward function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_ff(curstat_t *s, bool_t start)
{
	sword32_t	i,
			start_addr,
			end_addr;
	msf_t		start_msf,
			end_msf;
	byte_t		vol;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		scsipt_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				scsipt_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) scsipt_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE,
					FALSE
				);
			}

			/* Start search forward */
			scsipt_start_search = TRUE;
			scsipt_run_ff(s);
		}
		else {
			/* Button release */

			scsipt_stop_ff(s);

			/* Update display */
			(void) scsipt_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				scsipt_mute_on(s);
			else
				/* Restore volume */
				scsipt_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == LEAD_OUT_TRACK) {
					/* Restore volume */
					scsipt_mute_off(s);
					return;
				}

				start_addr = s->curpos_tot.addr;
				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_addr = s->trkinfo[i+1].addr;
				end_msf.min = s->trkinfo[i+1].min;
				end_msf.sec = s->trkinfo[i+1].sec;
				end_msf.frame = s->trkinfo[i+1].frame;

				if (!scsipt_do_playaudio(devp,
						ADDR_BLK | ADDR_MSF,
						start_addr, end_addr,
						&start_msf, &end_msf,
						0, 0)) {
					DO_BEEP();

					/* Restore volume */
					scsipt_mute_off(s);
					return;
				}
			}
			else if (s->segplay == SEGP_AB &&
				 (s->curpos_tot.addr + app_data.min_playblks) >
					s->bp_endpos_tot.addr) {
				/* No more left to play */
				/* Restore volume */
				scsipt_mute_off(s);
				return;
			}
			else {
				start_addr = s->curpos_tot.addr;
				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;

				if (s->segplay == SEGP_AB) {
					end_addr = s->bp_endpos_tot.addr;
					end_msf.min = s->bp_endpos_tot.min;
					end_msf.sec = s->bp_endpos_tot.sec;
					end_msf.frame = s->bp_endpos_tot.frame;
				}
				else {
					end_addr = s->discpos_tot.addr;
					end_msf.min = s->discpos_tot.min;
					end_msf.sec = s->discpos_tot.sec;
					end_msf.frame = s->discpos_tot.frame;
				}

				if (!scsipt_do_playaudio(devp,
						ADDR_BLK | ADDR_MSF,
						start_addr, end_addr,
						&start_msf, &end_msf,
						0, 0)) {
					DO_BEEP();

					/* Restore volume */
					scsipt_mute_off(s);
					return;
				}
			}
			if (s->mode == MOD_PAUSE) {
				(void) scsipt_do_pause_resume(devp, FALSE);

				/* Restore volume */
				scsipt_mute_off(s);
			}
			else
				scsipt_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * scsipt_warp
 *	Track warp function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_warp(curstat_t *s)
{
	sword32_t	start_addr,
			end_addr;
	msf_t		start_msf,
			end_msf;
	int		i;

	start_addr = s->curpos_tot.addr;
	start_msf.min = s->curpos_tot.min;
	start_msf.sec = s->curpos_tot.sec;
	start_msf.frame = s->curpos_tot.frame;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		scsipt_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->shuffle || s->program) {
			if ((i = di_curtrk_pos(s)) < 0) {
				DO_BEEP();
				return;
			}

			end_addr = s->trkinfo[i+1].addr;
			end_msf.min = s->trkinfo[i+1].min;
			end_msf.sec = s->trkinfo[i+1].sec;
			end_msf.frame = s->trkinfo[i+1].frame;
		}
		else {
			end_addr = s->discpos_tot.addr;
			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;
		}

		if (s->segplay == SEGP_AB) {
			if (start_addr < s->bp_startpos_tot.addr) {
				start_addr = s->bp_startpos_tot.addr;
				start_msf.min = s->bp_startpos_tot.min;
				start_msf.sec = s->bp_startpos_tot.sec;
				start_msf.frame = s->bp_startpos_tot.frame;
			}

			if (end_addr > s->bp_endpos_tot.addr) {
				end_addr = s->bp_endpos_tot.addr;
				end_msf.min = s->bp_endpos_tot.min;
				end_msf.sec = s->bp_endpos_tot.sec;
				end_msf.frame = s->bp_endpos_tot.frame;
			}
		}

		if ((end_addr - app_data.min_playblks) < start_addr) {
			/* No more left to play: just stop */
			if (!scsipt_do_start_stop(devp, FALSE, FALSE, TRUE))
				DO_BEEP();
		}
		else {
			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				scsipt_mute_on(s);

			if (!scsipt_do_playaudio(devp,
						 ADDR_BLK | ADDR_MSF,
						 start_addr, end_addr,
						 &start_msf, &end_msf,
						 0, 0)) {
				DO_BEEP();

				/* Restore volume */
				scsipt_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) scsipt_do_pause_resume(devp, FALSE);

				/* Restore volume */
				scsipt_mute_off(s);
			}
		}
		break;

	default:
		break;
	}
}


/*
 * scsipt_route
 *	Channel routing function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_route(curstat_t *s)
{
	byte_t	val0,
		val1;

	if (!app_data.chroute_supp)
		return;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) cdda_chroute(devp, s);
		return;
	}

	if (scsipt_vutbl[app_data.vendor_code].route != NULL) {
		(void) scsipt_vutbl[app_data.vendor_code].route(s);
		return;
	}

	val0 = scsipt_route_val(app_data.ch_route, 0);
	val1 = scsipt_route_val(app_data.ch_route, 1);

	if (val0 == scsipt_route_left && val1 == scsipt_route_right)
		/* No change: just return */
		return;

	scsipt_route_left = val0;
	scsipt_route_right = val1;

	/* With SCSI-2, channel routing is done with the volume control */
	(void) scsipt_cfg_vol(s->level, s, FALSE, TRUE);
}


/*
 * scsipt_mute_on
 *	Mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_mute_on(curstat_t *s)
{
	(void) scsipt_cfg_vol(0, s, FALSE, FALSE);
}


/*
 * scsipt_mute_off
 *	Un-mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_mute_off(curstat_t *s)
{
	(void) scsipt_cfg_vol((int) s->level, s, FALSE, FALSE);
}


/*
 * scsipt_cddajitter
 *	CDDA jitter correction setting change notification function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_cddajitter(curstat_t *s)
{
	sword32_t	curblk = 0;
	byte_t		omode;
	
	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		omode = s->mode;
		if (omode == MOD_PAUSE)
			omode = MOD_PLAY;

		/* Save state */
		switch (omode) {
		case MOD_PLAY:
		case MOD_SAMPLE:
			curblk = s->curpos_tot.addr;
			scsipt_stop_stat_poll();
			break;

		default:
			break;
		}

		/* Restart CDDA */
		cdda_halt(devp, s);
		util_delayms(1000);

		scsipt_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		scsipt_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		scsipt_cdda_client.warning_msg = di_clinfo->warning_msg;
		scsipt_cdda_client.info_msg = di_clinfo->info_msg;
		scsipt_cdda_client.info2_msg = di_clinfo->info2_msg;

		(void) cdda_init(s, &scsipt_cdda_client);

		/* Restore old state */
		switch (omode) {
		case MOD_PLAY:
		case MOD_SAMPLE:
			if (scsipt_play_recov(curblk, FALSE))
				scsipt_start_stat_poll(s);
			break;

		default:
			break;
		}

		s->mode = omode;
	}
}


/*
 * scsipt_debug
 *	Debug level change notification function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
scsipt_debug(void)
{
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		cdda_debug(app_data.debug);
}


/*
 * scsipt_start
 *	Start the SCSI pass-through module.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_start(curstat_t *s)
{
	/* Check to see if disc is ready */
	if (di_clinfo->timeout != NULL)
		scsipt_start_insert_poll(s);
	else
		(void) scsipt_disc_ready(s);

	/* Update display */
	DPY_ALL(s);
}


/*
 * scsipt_icon
 *	Handler for main window iconification/de-iconification
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	iconified - Whether the main window is iconified
 *
 * Return:
 *	Nothing.
 */
void
scsipt_icon(curstat_t *s, bool_t iconified)
{
	/* This function attempts to reduce the status polling frequency
	 * when possible to cut down on CPU and SCSI bus usage.  This is
	 * done when the CD player is iconified.
	 */

	/* Increase status polling interval by 4 seconds when iconified */
	if (iconified)
		scsipt_stat_interval = app_data.stat_interval + 4000;
	else
		scsipt_stat_interval = app_data.stat_interval;

	switch (s->mode) {
	case MOD_BUSY:
	case MOD_NODISC:
	case MOD_STOP:
	case MOD_PAUSE:
		break;

	case MOD_SAMPLE:
		/* No optimization in these modes */
		scsipt_stat_interval = app_data.stat_interval;
		break;

	case MOD_PLAY:
		if (!iconified) {
			/* Force an immediate update */
			scsipt_stop_stat_poll();
			scsipt_start_stat_poll(s);
		}
		break;

	default:
		break;
	}
}


/*
 * scsipt_halt
 *	Shut down the SCSI pass-through and vendor-unique modules.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
scsipt_halt(curstat_t *s)
{
	int	i;

	/* If playing CDDA, stop it */
	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			(void) scsipt_do_start_stop(devp, FALSE, FALSE, TRUE);
			s->mode = MOD_STOP;
			scsipt_stop_stat_poll();
			break;
		default:
			break;
		}
	}

	/* Shut down CDDA */
	cdda_halt(devp, s);
	app_data.play_mode = PLAYMODE_STD;

	/* Re-enable front-panel eject button */
	if (app_data.caddylock_supp)
		scsipt_lock(s, FALSE);

	if (s->mode != MOD_BUSY && s->mode != MOD_NODISC) {
		if (app_data.exit_eject && app_data.eject_supp) {
			/* User closing application: Eject disc */
			(void) scsipt_do_start_stop(devp, FALSE, TRUE, TRUE);
		}
		else {
			if (app_data.exit_stop)
				/* User closing application: Stop disc */
				(void) scsipt_do_start_stop(devp, FALSE,
							    FALSE, TRUE);

			switch (s->mode) {
			case MOD_PLAY:
			case MOD_PAUSE:
			case MOD_SAMPLE:
				scsipt_stop_stat_poll();
				break;
			default:
				break;
			}
		}
	}

	/* Shut down the vendor unique modules */
	for (i = 0; i < MAX_VENDORS; i++) {
		if (scsipt_vutbl[i].halt != NULL)
			scsipt_vutbl[i].halt();
	}

	/* Close CD device */
	if (devp != NULL) {
		scsipt_close(devp);
		devp = NULL;
		scsipt_not_open = TRUE;
	}

	/* Shut down multi-disc changer */
	if (app_data.numdiscs > 1)
		scsipt_chg_halt(s);
}


/*
 * scsipt_methodstr
 *	Return a text string indicating the SCSI pass-through module's
 *	version number and which SCSI-1 vendor-unique modes are
 *	supported in this binary.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Version text string.
 */
char *
scsipt_methodstr(void)
{
	static char	str[STR_BUF_SZ * 4];

	(void) sprintf(str,
		   "SCSI pass-through\n    %s\n    %s%s interface\n",
		   pthru_vers(),
		   scsipt_vutbl[app_data.vendor_code].vendor == NULL ?
		       "Unknown" :
		       scsipt_vutbl[app_data.vendor_code].vendor,
		   app_data.vendor_code == VENDOR_SCSI2 ?
		       "" : " vendor unique");

	return (str);
}

#endif	/* DI_SCSIPT */

