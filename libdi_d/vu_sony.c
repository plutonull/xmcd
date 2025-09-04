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

/*
 *   Sony vendor-unique support
 *
 *   The name "Sony" is a trademark of Sony Corporation, and is used
 *   here for identification purposes only.
 */
#ifndef lint
static char *_vu_sony_c_ident_ = "@(#)vu_sony.c	6.46 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#ifdef VENDOR_SONY

extern appdata_t	app_data;
extern di_client_t	*di_clinfo;
extern vu_tbl_t		scsipt_vutbl[];
extern byte_t		cdb[];
extern di_dev_t		*devp;

STATIC byte_t		sony_route_left,	/* left ch routing control */
			sony_route_right;	/* Right ch routing control */


/*
 * sony_route_val
 *	Return the channel routing control value used in the
 *	Sony Playback Status command.
 *
 * Args:
 *	route_mode - The channel routing mode value.
 *	channel - The channel number desired (0=left 1=right).
 *
 * Return:
 *	The routing control value.
 */
STATIC byte_t
sony_route_val(int route_mode, int channel)
{
	switch (channel) {
	case 0:
		switch (route_mode) {
		case 0:
			return 0x1;
		case 1:
			return 0x2;
		case 2:
			return 0x1;
		case 3:
			return 0x2;
		case 4:
			return 0x3;
		default:
			/* Invalid value */
			return 0x0;
		}
		/*NOTREACHED*/

	case 1:
		switch (route_mode) {
		case 0:
			return 0x2;
		case 1:
			return 0x1;
		case 2:
			return 0x1;
		case 3:
			return 0x2;
		case 4:
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
 * sony_playaudio
 *	Play audio function: send vendor-unique play audio command
 *	to the drive.
 *
 * Args:
 *	addr_fmt - Flags indicating which address formats are passed in
 *	If ADDR_BLK, then:
 *	    start_addr - The logical block starting address
 *	    end_addr - The logical block ending address
 *	If ADD_MSF, then:
 *	    start_msf - Pointer to the starting MSF address structure
 *	    end_msf - Pointer to the ending MSF address structure
 *	If ADDR_TRKIDX, then:
 *	    trk - The starting track number
 *	    idx - The starting index number
 *	If ADDR_OPTEND, then the ending address, if specified, can be
 *	ignored if possible.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
sony_playaudio(
	byte_t		addr_fmt,
	sword32_t	start_addr,
	sword32_t	end_addr,
	msf_t		*start_msf,
	msf_t		*end_msf,
	byte_t		trk,
	byte_t		idx
)
{
	msf_t		istart_msf,
			iend_msf;
	bool_t		ret = FALSE;

	if (!ret && (addr_fmt & ADDR_BLK) && !(addr_fmt & ADDR_MSF)) {
		/* Convert block address to MSF format */
		util_blktomsf(
			start_addr,
			&istart_msf.min, &istart_msf.sec, &istart_msf.frame,
			MSF_OFFSET
		);

		util_blktomsf(
			end_addr,
			&iend_msf.min, &iend_msf.sec, &iend_msf.frame,
			MSF_OFFSET
		);

		/* Let the ADDR_MSF code handle the request */
		start_msf = &istart_msf;
		end_msf = &iend_msf;
		addr_fmt |= ADDR_MSF;
		ret = FALSE;
	}

	if (!ret && addr_fmt & ADDR_MSF) {
		SCSICDB_RESET(cdb);
		cdb[0] = OP_VS_PLAYMSF;
		cdb[3] = start_msf->min;
		cdb[4] = start_msf->sec;
		cdb[5] = start_msf->frame;
		cdb[6] = end_msf->min;
		cdb[7] = end_msf->sec;
		cdb[8] = end_msf->frame;

		ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
				 OP_NODATA, 10, TRUE);
	}

	return (ret);
}


/*
 * sony_pause_resume
 *	Pause/resume function: send vendor-unique commands to implement
 *	the pause and resume capability.
 *
 * Args:
 *	resume - TRUE: resume, FALSE: pause
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
sony_pause_resume(bool_t resume)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_VS_PAUSE;
	cdb[1] = resume ? 0x00 : 0x10;

	return(pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
			  OP_NODATA, 5, TRUE));
}


/*
 * sony_get_playstatus
 *	Send vendor-unique command to obtain current audio playback
 *	status.
 *
 * Args:
 *	sp - Pointer to the caller-supplied cdstat_t return structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
sony_get_playstatus(cdstat_t *sp)
{
	byte_t		dbuf[sizeof(sstat_data_t)],
			qbuf[sizeof(ssubq_data_t)];
	sstat_data_t	*d;
	ssubq_data_t	*q;


	/*
	 * Send Sony Playback Status command
	 */

	(void) memset(dbuf, 0, sizeof(dbuf));

	SCSICDB_RESET(cdb);
	cdb[0] = OP_VS_PLAYSTAT;
	cdb[8] = SZ_VS_PLAYSTAT;

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, dbuf, SZ_VS_PLAYSTAT,
			NULL, 0, OP_DATAIN, 5, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("sony: Playback Status data bytes",
		dbuf, SZ_VS_PLAYSTAT);

	d = (sstat_data_t *)(void *) dbuf;

	/* Translate Sony audio status to cdstat_t status */
	switch (d->audio_status) {
	case SAUD_PLAYING:
	case SAUD_MUTED:
		sp->status = CDSTAT_PLAYING;
		break;

	case SAUD_PAUSED:
		sp->status = CDSTAT_PAUSED;
		break;

	case SAUD_COMPLETED:
		sp->status = CDSTAT_COMPLETED;
		break;

	case SAUD_ERROR:
		sp->status = CDSTAT_FAILED;
		break;

	case SAUD_NOTREQ:
		sp->status = CDSTAT_NOSTATUS;
		break;
	}

	/*
	 * Send Sony Read Subchannel command
	 */

	(void) memset(qbuf, 0, sizeof(qbuf));

	SCSICDB_RESET(cdb);
	cdb[0] = OP_VS_RDSUBQ;
	cdb[2] = 0x40;
	cdb[8] = SZ_VS_RDSUBQ;

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, qbuf, SZ_VS_RDSUBQ,
			NULL, 0, OP_DATAIN, 5, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("sony: Read Subchannel data bytes",
		qbuf, SZ_VS_RDSUBQ);

	q = (ssubq_data_t *)(void *) qbuf;

	sp->track = (int) q->trkno;
	sp->index = (int) q->idxno;

	sp->abs_addr.min = (byte_t) q->abs_min;
	sp->abs_addr.sec = (byte_t) q->abs_sec;
	sp->abs_addr.frame = (byte_t) q->abs_frame;
	sp->rel_addr.min = (byte_t) q->rel_min;
	sp->rel_addr.sec = (byte_t) q->rel_sec;
	sp->rel_addr.frame = (byte_t) q->rel_frame;
	util_msftoblk(
		sp->abs_addr.min, sp->abs_addr.sec, sp->abs_addr.frame,
		&sp->abs_addr.addr, MSF_OFFSET
	);
	util_msftoblk(
		sp->rel_addr.min, sp->rel_addr.sec, sp->rel_addr.frame,
		&sp->rel_addr.addr, 0
	);

	return TRUE;
}


/*
 * sony_get_toc
 *	Send vendor-unique command to obtain the disc table-of-contents
 *
 * Args:
 *	s - Pointer to the curstat_t structure, which contains the TOC
 *	    table to be updated.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
sony_get_toc(curstat_t *s)
{
	int		i,
			j,
			xfer_len;
	byte_t		buf[sizeof(stoc_data_t)];
	stoc_data_t	*d;
	stoc_ent_t	*e;

	(void) memset(buf, 0, sizeof(buf));

	/* Read TOC header to find the number of tracks */
	for (i = 1; i < MAXTRACK - 1; i++) {
		SCSICDB_RESET(cdb);
		cdb[0] = OP_VS_RDTOC;
		cdb[5] = (byte_t) i;
		cdb[8] = SZ_VS_TOCHDR;

		if (pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, SZ_VS_TOCHDR,
			       NULL, 0, OP_DATAIN, 5, TRUE))
			break;
	}
	if (i == MAXTRACK - 1)
		return FALSE;

	d = (stoc_data_t *)(void *) buf;

	s->first_trk = (byte_t) d->first_trk;
	s->last_trk = (byte_t) d->last_trk;

	xfer_len = SZ_VS_TOCHDR +
		   ((int) (d->last_trk - d->first_trk + 2) * SZ_VS_TOCENT);

	if (xfer_len > SZ_VS_RDTOC)
		xfer_len = SZ_VS_RDTOC;

	SCSICDB_RESET(cdb);
	cdb[0] = OP_VS_RDTOC;
	cdb[5] = (byte_t) s->first_trk;
	cdb[7] = (xfer_len & 0xff00) >> 8;
	cdb[8] = (xfer_len & 0x00ff);

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, xfer_len, NULL, 0,
			OP_DATAIN, 5, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("sony: Read TOC data bytes", buf, SZ_VS_RDTOC);

	/* Get the starting position of each track */
	for (i = 0, j = (int) s->first_trk; j <= (int) s->last_trk; i++, j++) {
		e = (stoc_ent_t *)(void *) &d->trkdata[i * SZ_VS_TOCENT];
		s->trkinfo[i].trkno = j;
		s->trkinfo[i].min = (byte_t) e->min;
		s->trkinfo[i].sec = (byte_t) e->sec;
		s->trkinfo[i].frame = (byte_t) e->frame;
		util_msftoblk(
			s->trkinfo[i].min,
			s->trkinfo[i].sec,
			s->trkinfo[i].frame,
			&s->trkinfo[i].addr,
			MSF_OFFSET
		);
		s->trkinfo[i].type = (e->trktype == 0) ? TYP_AUDIO : TYP_DATA;
	}
	s->tot_trks = (byte_t) i;

	/* Get the lead-out track position */
	e = (stoc_ent_t *)(void *) &d->trkdata[i * SZ_VS_TOCENT];
	s->trkinfo[i].trkno = LEAD_OUT_TRACK;
	s->discpos_tot.min = s->trkinfo[i].min = (byte_t) e->min;
	s->discpos_tot.sec = s->trkinfo[i].sec = (byte_t) e->sec;
	s->discpos_tot.frame = s->trkinfo[i].frame = (byte_t) e->frame;
	util_msftoblk(
		s->trkinfo[i].min,
		s->trkinfo[i].sec,
		s->trkinfo[i].frame,
		&s->trkinfo[i].addr,
		MSF_OFFSET
	);
	s->discpos_tot.addr = s->trkinfo[i].addr;
	return TRUE;
}


/*
 * sony_volume
 *	Send vendor-unique command to query/control the playback volume.
 *
 * Args:
 *	vol - Volume level to set to
 *	s - Pointer to the curstat_t structure
 *	query - This call is to query the current volume setting rather
 *		than to set it.
 *
 * Return:
 *	The current volume value.
 */
int
sony_volume(int vol, curstat_t *s, bool_t query)
{
	int		vol1,
			vol2;
	byte_t		buf[sizeof(sstat_data_t)];
	sstat_data_t	*d;

	(void) memset(buf, 0, sizeof(buf));

	SCSICDB_RESET(cdb);
	cdb[0] = OP_VS_PLAYSTAT;
	cdb[8] = SZ_VS_PLAYSTAT;

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, SZ_VS_PLAYSTAT,
			NULL, 0, OP_DATAIN, 5, TRUE))
		return -1;

	DBGDUMP(DBG_DEVIO)("sony: Playback Status data bytes",
		buf, SZ_VS_PLAYSTAT);

	d = (sstat_data_t *)(void *) buf;

	if (query) {
		vol1 = util_untaper_vol(util_unscale_vol((int) d->vol0));
		vol2 = util_untaper_vol(util_unscale_vol((int) d->vol1));
		sony_route_left = (byte_t) d->sel0;
		sony_route_right = (byte_t) d->sel1;

		if (vol1 == vol2) {
			s->level_left = s->level_right = 100;
			vol = vol1;
		}
		else if (vol1 > vol2) {
			s->level_left = 100;
			s->level_right = (byte_t) ((vol2 * 100) / vol1);
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
		(void) memset(buf, 0, 10);

		d->vol0 = (byte_t) util_scale_vol(
			util_taper_vol(vol * (int) s->level_left / 100)
		);
		d->vol1 = (byte_t) util_scale_vol(
			util_taper_vol(vol * (int) s->level_right / 100)
		);
		d->sel0 = sony_route_left;
		d->sel1 = sony_route_right;

		DBGDUMP(DBG_DEVIO)("sony: Playback Control data bytes",
			buf, SZ_VS_PLAYSTAT);

		SCSICDB_RESET(cdb);
		cdb[0] = OP_VS_PLAYCTL;
		cdb[8] = SZ_VS_PLAYSTAT;

		if (pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
			       buf, SZ_VS_PLAYSTAT, NULL, 0,
			       OP_DATAOUT, 5, TRUE)) {
			/* Success */
			return (vol);
		}
		else if (d->vol0 != d->vol1) {
			/* Set the balance to the center
			 * and retry.
			 */
			d->vol0 = d->vol1 = util_scale_vol(
				util_taper_vol(vol)
			);

			DBGDUMP(DBG_DEVIO)("sony: Playback Control data bytes",
				buf, SZ_VS_PLAYSTAT);

			SCSICDB_RESET(cdb);
			cdb[0] = OP_VS_PLAYCTL;
			cdb[8] = SZ_VS_PLAYSTAT;

			if (pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
				       buf, SZ_VS_PLAYSTAT,
				       NULL, 0, OP_DATAOUT, 5, TRUE)) {
				/* Success: Warp balance control */
				s->level_left = s->level_right = 100;
				return (vol);
			}

			/* Still failed: just drop through */
		}
	}

	return -1;
}


/*
 * sony_route
 *	Configure channel routing via Sony Vendor-unique commands.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
sony_route(curstat_t *s)
{
	byte_t	val0,
		val1;

	val0 = sony_route_val(app_data.ch_route, 0);
	val1 = sony_route_val(app_data.ch_route, 1);

	if (val0 == sony_route_left && val1 == sony_route_right)
		/* No change: just return */
		return TRUE;

	sony_route_left = val0;
	sony_route_right = val1;

	/* Sony channel routing is done with the volume control */
	(void) sony_volume(s->level, s, FALSE);

	return TRUE;
}


/*
 * sony_init
 *	Initialize the vendor-unique support module
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
void
sony_init(void)
{
	/* Register vendor_unique module entry points */
	scsipt_vutbl[VENDOR_SONY].vendor = "Sony";
	scsipt_vutbl[VENDOR_SONY].playaudio = sony_playaudio;
	scsipt_vutbl[VENDOR_SONY].pause_resume = sony_pause_resume;
	scsipt_vutbl[VENDOR_SONY].start_stop = NULL;
	scsipt_vutbl[VENDOR_SONY].get_playstatus = sony_get_playstatus;
	scsipt_vutbl[VENDOR_SONY].volume = sony_volume;
	scsipt_vutbl[VENDOR_SONY].route = sony_route;
	scsipt_vutbl[VENDOR_SONY].mute = NULL;
	scsipt_vutbl[VENDOR_SONY].get_toc = sony_get_toc;
	scsipt_vutbl[VENDOR_SONY].eject = NULL;
	scsipt_vutbl[VENDOR_SONY].start = sony_start;
	scsipt_vutbl[VENDOR_SONY].halt = NULL;
}


/*
 * sony_start
 *	Start the vendor-unique support module.  In the case of the Sony,
 *	we want to set the drive's CD addressing to MSF mode.  Some Sony
 *	drives use the Mode Select command (page 0x8) to do this while
 *	others use a special vendor-unique "Set Address Format" command.
 *	We determine which to use by overloading the meaning of the
 *	app_data.msen_dbd configuration parameter.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
void
sony_start(void)
{
	byte_t			buf[SZ_VS_CDPARM];
	mode_sense_6_data_t	*ms_data;
	blk_desc_t		*bdesc;
	cdparm_pg_t		*parm_pg;

	if (app_data.msen_dbd) {
		/* Send "Set Address Format" command */
		SCSICDB_RESET(cdb);
		cdb[0] = OP_VS_SETADDRFMT;
		cdb[8] = 0x01;

		(void) pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
				  NULL, 0, NULL, 0, OP_NODATA, 5, TRUE);
	}
	else {
		/* Do Mode Sense command, CD-ROM parameters page */
		if (!scsipt_modesense(devp, DI_ROLE_MAIN, buf, 0, PG_VS_CDPARM,
				      SZ_VS_CDPARM))
			return;		/* Error */

		ms_data = (mode_sense_6_data_t *)(void *) buf;
		bdesc = (blk_desc_t *)(void *) ms_data->data;
		parm_pg = (cdparm_pg_t *)(void *)
			  &ms_data->data[ms_data->bdescr_len];

		if (parm_pg->pg_code != PG_VS_CDPARM)
			return;		/* Error */

		ms_data->data_len = 0;
		if (ms_data->bdescr_len > 0)
			bdesc->num_blks = 0;

		/* Set the drive up for MSF address mode */
		parm_pg->lbamsf = 1;

		(void) scsipt_modesel(devp, DI_ROLE_MAIN, buf,
				      PG_VS_CDPARM, SZ_VS_CDPARM);
	}
}


#endif	/* VENDOR_SONY */

