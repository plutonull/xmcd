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
 *   Toshiba vendor-unique support
 *
 *   The name "Toshiba" is a trademark of Toshiba Corporation, and is
 *   used here for identification purposes only.
 */
#ifndef lint
static char *_vu_tosh_c_ident_ = "@(#)vu_tosh.c	6.40 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#ifdef VENDOR_TOSHIBA

extern appdata_t	app_data;
extern di_client_t	*di_clinfo;
extern vu_tbl_t		scsipt_vutbl[];
extern byte_t		cdb[];
extern di_dev_t		*devp;

STATIC bool_t		tosh_audio_muted;	/* Is audio muted? */


/*
 * tosh_playaudio
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
tosh_playaudio(
	byte_t		addr_fmt,
	sword32_t	start_addr,
	sword32_t	end_addr,
	msf_t		*start_msf,
	msf_t		*end_msf,
	byte_t		trk,
	byte_t		idx
)
{
	bool_t		ret = FALSE;

	if (!ret && addr_fmt & ADDR_MSF) {
		/* Position laser head at desired location
		 * and start play.
		 */
		SCSICDB_RESET(cdb);
		cdb[0] = OP_VT_AUDSRCH;
		cdb[1] = 0x01;
		cdb[2] = (byte_t) util_ltobcd(start_msf->min);
		cdb[3] = (byte_t) util_ltobcd(start_msf->sec);
		cdb[4] = (byte_t) util_ltobcd(start_msf->frame);
		cdb[9] = 0x40;

		ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
				 OP_NODATA, 10, TRUE);

		if (ret && !(addr_fmt & ADDR_OPTEND)) {
			/* Specify end location, muting, and start play */
			SCSICDB_RESET(cdb);
			cdb[0] = OP_VT_AUDPLAY;
			cdb[1] = tosh_audio_muted ? 0x00 : 0x03;
			cdb[2] = (byte_t) util_ltobcd(end_msf->min);
			cdb[3] = (byte_t) util_ltobcd(end_msf->sec);
			cdb[4] = (byte_t) util_ltobcd(end_msf->frame);
			cdb[9] = 0x40;

			ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
					 NULL, 0, NULL, 0,
					 OP_NODATA, 10, TRUE);
		}
	}

	if (!ret && addr_fmt & ADDR_BLK) {
		/* Position laser head at desired location
		 * and start play.
		 */
		SCSICDB_RESET(cdb);
		cdb[0] = OP_VT_AUDSRCH;
		cdb[1] = 0x01;
		cdb[2] = ((word32_t) start_addr & 0xff000000) >> 24;
		cdb[3] = ((word32_t) start_addr & 0x00ff0000) >> 16;
		cdb[4] = ((word32_t) start_addr & 0x0000ff00) >> 8;
		cdb[5] = ((word32_t) start_addr & 0x000000ff);
		cdb[9] = 0x00;

		ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
				 OP_NODATA, 10, TRUE);

		if (ret && !(addr_fmt & ADDR_OPTEND)) {
			/* Specify end location, muting, and start play */
			SCSICDB_RESET(cdb);
			cdb[0] = OP_VT_AUDPLAY;
			cdb[1] = tosh_audio_muted ? 0x00 : 0x03;
			cdb[2] = ((word32_t) end_addr & 0xff000000) >> 24;
			cdb[3] = ((word32_t) end_addr & 0x00ff0000) >> 16;
			cdb[4] = ((word32_t) end_addr & 0x0000ff00) >> 8;
			cdb[5] = ((word32_t) end_addr & 0x000000ff);
			cdb[9] = 0x00;

			ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
					 NULL, 0, NULL, 0,
					 OP_NODATA, 10, TRUE);
		}
	}

	return (ret);
}


/*
 * tosh_pause_resume
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
tosh_pause_resume(bool_t resume)
{
	SCSICDB_RESET(cdb);

	if (resume) {
		cdb[0] = OP_VT_AUDPLAY;
		cdb[1] = tosh_audio_muted ? 0x00 : 0x03;
		cdb[9] = 0xc0;

		return (
			pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
				   NULL, 0, NULL, 0, OP_NODATA, 5, TRUE)
		);
	}
	else {
		cdb[0] = OP_VT_STILL;

		return (
			pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
				   NULL, 0, NULL, 0, OP_NODATA, 5, TRUE)
		);
	}
}


/*
 * tosh_get_playstatus
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
tosh_get_playstatus(cdstat_t *sp)
{
	byte_t		buf[sizeof(tsubq_data_t)];
	tsubq_data_t	*d;


	(void) memset(buf, 0, sizeof(buf));

	SCSICDB_RESET(cdb);
	cdb[0] = OP_VT_RDSUBQ;
	cdb[1] = SZ_VT_RDSUBQ & 0x1f;

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, SZ_VT_RDSUBQ,
			NULL, 0, OP_DATAIN, 5, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("tosh: Read Subchannel data bytes",
		buf, SZ_VT_RDSUBQ);

	d = (tsubq_data_t *)(void *) buf;

	sp->track = (int) util_bcdtol((word32_t) d->trkno);
	sp->index = (int) util_bcdtol((word32_t) d->idxno);

	sp->abs_addr.min = (byte_t) util_bcdtol(d->abs_min);
	sp->abs_addr.sec = (byte_t) util_bcdtol(d->abs_sec);
	sp->abs_addr.frame = (byte_t) util_bcdtol(d->abs_frame);
	sp->rel_addr.min = (byte_t) util_bcdtol(d->rel_min);
	sp->rel_addr.sec = (byte_t) util_bcdtol(d->rel_sec);
	sp->rel_addr.frame = (byte_t) util_bcdtol(d->rel_frame);
	util_msftoblk(
		sp->abs_addr.min, sp->abs_addr.sec, sp->abs_addr.frame,
		&sp->abs_addr.addr, MSF_OFFSET
	);
	util_msftoblk(
		sp->rel_addr.min, sp->rel_addr.sec, sp->rel_addr.frame,
		&sp->rel_addr.addr, 0
	);

	/* Translate Toshiba audio status to SCSI-2 audio status */
	switch (d->audio_status) {
	case TAUD_PLAYING:
		sp->status = CDSTAT_PLAYING;
		break;

	case TAUD_SRCH_PAUSED:
	case TAUD_PAUSED:
		sp->status = CDSTAT_PAUSED;
		break;

	case TAUD_OTHER:
		sp->status = CDSTAT_COMPLETED;
		break;
	}

	return TRUE;
}


/*
 * tosh_get_toc
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
tosh_get_toc(curstat_t *s)
{
	int		i,
			j;
	byte_t		buf[SZ_VT_RDINFO];
	tinfo_00_t	*t0;
	tinfo_01_t	*t1;
	tinfo_02_t	*t2;


	(void) memset(buf, 0, sizeof(buf));

	/* Find number of tracks */
	SCSICDB_RESET(cdb);
	cdb[0] = OP_VT_RDINFO;
	cdb[1] = 0x00;

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, SZ_VT_RDINFO,
			NULL, 0, OP_DATAIN, 5, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("tosh: Read Disc Info data bytes",
		buf, SZ_VT_RDINFO);

	t0 = (tinfo_00_t *) buf;
	s->first_trk = (byte_t) util_bcdtol(t0->first_trk);
	s->last_trk = (byte_t) util_bcdtol(t0->last_trk);

	/* Get the starting position of each track */
	for (i = 0, j = (int) s->first_trk; j <= (int) s->last_trk; i++, j++) {
		(void) memset(buf, 0, sizeof(buf));

		SCSICDB_RESET(cdb);
		cdb[0] = OP_VT_RDINFO;
		cdb[1] = 0x02;
		cdb[2] = (byte_t) util_ltobcd(j);

		if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
				buf, SZ_VT_RDINFO, NULL, 0,
				OP_DATAIN, 5, TRUE))
			return FALSE;

		DBGDUMP(DBG_DEVIO)("tosh: Read Disc Info data bytes",
			buf, SZ_VT_RDINFO);

		t2 = (tinfo_02_t *) buf;

		s->trkinfo[i].trkno = j;
		s->trkinfo[i].min = (byte_t) util_bcdtol(t2->min);
		s->trkinfo[i].sec = (byte_t) util_bcdtol(t2->sec);
		s->trkinfo[i].frame = (byte_t) util_bcdtol(t2->frame);
		util_msftoblk(
			s->trkinfo[i].min,
			s->trkinfo[i].sec,
			s->trkinfo[i].frame,
			&s->trkinfo[i].addr,
			MSF_OFFSET
		);
	}
	s->tot_trks = (byte_t) i;

	(void) memset(buf, 0, sizeof(buf));

	/* Get the lead out track position */
	SCSICDB_RESET(cdb);
	cdb[0] = OP_VT_RDINFO;
	cdb[1] = 0x01;

	if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, SZ_VT_RDINFO,
			NULL, 0, OP_DATAIN, 5, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("tosh: Read Disc Info data bytes",
		buf, SZ_VT_RDINFO);

	t1 = (tinfo_01_t *) buf;

	s->trkinfo[i].trkno = LEAD_OUT_TRACK;
	s->discpos_tot.min = s->trkinfo[i].min = (byte_t) util_bcdtol(t1->min);
	s->discpos_tot.sec = s->trkinfo[i].sec = (byte_t) util_bcdtol(t1->sec);
	s->discpos_tot.frame = s->trkinfo[i].frame =
		(byte_t) util_bcdtol(t1->frame);
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
 * tosh_mute
 *	Send vendor-unique command to mute/unmute the audio
 *
 * Args:
 *	mute - TRUE: mute audio, FALSE: unmute audio
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
tosh_mute(bool_t mute)
{
	curstat_t	*s = di_clinfo->curstat_addr();

	if (tosh_audio_muted != mute) {
		switch (s->mode) {
		case MOD_BUSY:
		case MOD_NODISC:
		case MOD_STOP:
		case MOD_PAUSE:
			break;

		default:
			SCSICDB_RESET(cdb);
			cdb[0] = OP_VT_AUDPLAY;
			cdb[1] = mute ? 0x00 : 0x03;
			cdb[9] = 0xc0;

			if (!pthru_send(devp, DI_ROLE_MAIN, cdb, 10,
					NULL, 0, NULL, 0, OP_NODATA, 5, TRUE))
				return FALSE;
			break;
		}

		tosh_audio_muted = mute;
	}

	return TRUE;
}


/*
 * tosh_eject
 *	Send vendor-unique command to eject the caddy
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
tosh_eject(void)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_VT_EJECT;
	cdb[1] = 0x01;	/* Set immediate bit */

	return (pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
			   OP_NODATA, 20, TRUE));
}


/*
 * tosh_init
 *	Initialize the vendor-unique support module
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
void
tosh_init(void)
{
	/* Register vendor_unique module entry points */
	scsipt_vutbl[VENDOR_TOSHIBA].vendor = "Toshiba";
	scsipt_vutbl[VENDOR_TOSHIBA].playaudio = tosh_playaudio;
	scsipt_vutbl[VENDOR_TOSHIBA].pause_resume = tosh_pause_resume;
	scsipt_vutbl[VENDOR_TOSHIBA].start_stop = NULL;
	scsipt_vutbl[VENDOR_TOSHIBA].get_playstatus = tosh_get_playstatus;
	scsipt_vutbl[VENDOR_TOSHIBA].volume = NULL;
	scsipt_vutbl[VENDOR_TOSHIBA].route = NULL;
	scsipt_vutbl[VENDOR_TOSHIBA].mute = tosh_mute;
	scsipt_vutbl[VENDOR_TOSHIBA].get_toc = tosh_get_toc;
	scsipt_vutbl[VENDOR_TOSHIBA].eject = tosh_eject;
	scsipt_vutbl[VENDOR_TOSHIBA].start = NULL;
	scsipt_vutbl[VENDOR_TOSHIBA].halt = NULL;
}


#endif	/* VENDOR_TOSHIBA */

