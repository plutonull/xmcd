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
 *   Chinon CDx-431 and CDx-435 support
 *
 *   The name "Chinon" is a trademark of Chinon Industries, and is
 *   used here for identification purposes only.
 */
#ifndef lint
static char *_vu_chin_c_ident_ = "@(#)vu_chin.c	6.46 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"

#ifdef VENDOR_CHINON

extern appdata_t	app_data;
extern di_client_t	*di_clinfo;
extern vu_tbl_t		scsipt_vutbl[];
extern byte_t		cdb[];
extern di_dev_t		*devp;

STATIC bool_t		chin_bcd_hack;


/*
 * chin_playaudio
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
chin_playaudio(
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

	/* Chinon only supports the Play Audio MSF command on the
	 * CDx-43x, which is identical to the SCSI-2 command of
	 * the same name and opcode.
	 */

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

	if (!ret && (addr_fmt & ADDR_MSF))
		ret = scsipt_playmsf(devp, DI_ROLE_MAIN, start_msf, end_msf);

	return (ret);
}


/*
 * chin_start_stop
 *      Start/stop function.
 *
 * Args:
 *	start - TRUE: start unit, FALSE: stop unit
 *	loej - TRUE: eject caddy, FALSE: do not eject
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
chin_start_stop(bool_t start, bool_t loej)
{
	bool_t	ret;

	if (start)
		/* Chinon CDx-43x does not support a start command so
		 * just quietly return success.
		 */
		return TRUE;

	/* Stop the playback */
	SCSICDB_RESET(cdb);
	cdb[0] = OP_VC_STOP;

	ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
			 OP_NODATA, 10, TRUE);

	/* Eject the caddy if necessary */
	if (ret && loej) {
		SCSICDB_RESET(cdb);
		cdb[0] = OP_VC_EJECT;

		ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
				 OP_NODATA, 20, TRUE);
	}

	return (ret);
}


/*
 * chin_get_playstatus
 *	Send vendor-unique command to obtain current audio playback
 *	status.
 *
 * Args:
 *	sp - Pointer to the caller-supplied cdstat_t return structure.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
chin_get_playstatus(cdstat_t *sp)
{
	/* Chinon CDx-43x supports the Read Subchannel command which
	 * is identical to the SCSI-2 command of the same name and
	 * opcode, but the audio status codes are different.
	 */
	byte_t		buf[SZ_RDSUBQ],
			*cp;
	subq_hdr_t	*h;
	subq_01_t	*p;
	size_t		xfer_len;
	bool_t		ret;


	xfer_len = 48;
	(void) memset(buf, 0, sizeof(buf));

	SCSICDB_RESET(cdb);
	cdb[0] = OP_M_RDSUBQ;
	cdb[1] = app_data.subq_lba ? 0x00 : 0x02;
	cdb[2] = 0x40;
	cdb[3] = SUB_ALL;
	cdb[6] = 0;
	cdb[7] = (xfer_len & 0xff00) >> 8;
	cdb[8] = xfer_len & 0x00ff;

	if ((ret = pthru_send(devp, DI_ROLE_MAIN, cdb, 10, buf, xfer_len,
			      NULL, 0, OP_DATAIN, 5, TRUE)) == FALSE)
		return FALSE;

	DBGDUMP(DBG_DEVIO)("Read Subchannel data", buf, xfer_len);

	h = (subq_hdr_t *)(void *) buf;

	/* Check the subchannel data */
	cp = (byte_t *) h + sizeof(subq_hdr_t);
	switch (*cp) {
	case SUB_ALL:
	case SUB_CURPOS:
		p = (subq_01_t *)(void *) cp;

		/* Hack: to work around firmware anomalies
		 * in some CD-ROM drives.
		 */
		if (p->trkno >= MAXTRACK &&
		    p->trkno != LEAD_OUT_TRACK) {
			sp->status = CDSTAT_NOSTATUS;
			break;
		}

		/* Map Chinon status into cdstat_t */

		switch (h->audio_status) {
		case CAUD_PLAYING:
			sp->status = CDSTAT_PLAYING;
			break;
		case CAUD_PAUSED:
			sp->status = CDSTAT_PAUSED;
			break;
		case CAUD_INVALID:
		default:
			sp->status = CDSTAT_NOSTATUS;
			break;
		}

		if (chin_bcd_hack) {
			/* Hack: BUGLY CD-ROM firmware */
			sp->track = (int) util_bcdtol(p->trkno);
			sp->index = (int) util_bcdtol(p->idxno);
		}
		else {
			sp->track = (int) p->trkno;
			sp->index = (int) p->idxno;
		}

		if (app_data.subq_lba) {
			/* LBA mode */
			sp->abs_addr.addr = util_xlate_blk((sword32_t)
				util_bswap32(p->abs_addr.logical)
			);
			util_blktomsf(
				sp->abs_addr.addr,
				&sp->abs_addr.min,
				&sp->abs_addr.sec,
				&sp->abs_addr.frame,
				MSF_OFFSET
			);

			sp->rel_addr.addr = util_xlate_blk((sword32_t)
				util_bswap32(p->rel_addr.logical)
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
			sp->abs_addr.min = p->abs_addr.msf.min;
			sp->abs_addr.sec = p->abs_addr.msf.sec;
			sp->abs_addr.frame = p->abs_addr.msf.frame;
			util_msftoblk(
				sp->abs_addr.min,
				sp->abs_addr.sec,
				sp->abs_addr.frame,
				&sp->abs_addr.addr,
				MSF_OFFSET
			);

			sp->rel_addr.min = p->rel_addr.msf.min;
			sp->rel_addr.sec = p->rel_addr.msf.sec;
			sp->rel_addr.frame = p->rel_addr.msf.frame;
			util_msftoblk(
				sp->rel_addr.min,
				sp->rel_addr.sec,
				sp->rel_addr.frame,
				&sp->rel_addr.addr,
				0
			);
		}
		break;

	default:
		/* Something is wrong with the data */
		ret = FALSE;
		break;
	}

	return (ret);
}


/*
 * chin_get_toc
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
chin_get_toc(curstat_t *s)
{
	int			i,
				ntrks;
	byte_t			buf[SZ_RDTOC],
				*cp,
				*toc_end;
	toc_hdr_t		*h;
	toc_trk_descr_t		*p;


	/* Chinon CDx-43x supports the Read TOC command which is
	 * identical to the SCSI-2 command of the same name and opcode.
	 */

	(void) memset(buf, 0, sizeof(buf));

	if (!scsipt_rdtoc(devp, DI_ROLE_MAIN, buf, sizeof(buf), 0, 0,
			  (bool_t) !app_data.toc_lba, TRUE))
		return FALSE;

	/* Fill curstat structure with TOC data */
	h = (toc_hdr_t *)(void *) buf;
	toc_end = (byte_t *) h + util_bswap16(h->data_len) + 2;

	s->first_trk = h->first_trk;
	s->last_trk = h->last_trk;

	ntrks = (int) (h->last_trk - h->first_trk) + 1;

	/* Hack: workaround CD-ROM firmware bug
	 * Some Chinon drives return track numbers in BCD
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
				chin_bcd_hack = TRUE;
				s->first_trk = (byte_t) trk0;
				s->last_trk = (byte_t) trk1;
				break;
			}
		}
	}

	/*
	 * Fill in TOC data
	 */
	cp = (byte_t *) h + sizeof(toc_hdr_t);

	for (i = 0; cp < toc_end && i < MAXTRACK; i++) {
		p = (toc_trk_descr_t *)(void *) cp;

		if (chin_bcd_hack)
			/* Hack: BUGLY CD-ROM firmware */
			s->trkinfo[i].trkno = util_bcdtol(p->trkno);
		else
			s->trkinfo[i].trkno = p->trkno;

		s->trkinfo[i].type = (p->trktype == 0) ?
			TYP_AUDIO : TYP_DATA;

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

	return TRUE;
}


/*
 * chin_eject
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
chin_eject(void)
{
	SCSICDB_RESET(cdb);
	cdb[0] = OP_VC_EJECT;

	return(
		pthru_send(devp, DI_ROLE_MAIN, cdb, 10, NULL, 0, NULL, 0,
			   OP_NODATA, 20, TRUE)
	);
}


/*
 * chin_init
 *	Initialize the vendor-unique support module
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
void
chin_init(void)
{
	/* Register vendor_unique module entry points */
	scsipt_vutbl[VENDOR_CHINON].vendor = "Chinon";
	scsipt_vutbl[VENDOR_CHINON].playaudio = chin_playaudio;
	scsipt_vutbl[VENDOR_CHINON].pause_resume = NULL;
	scsipt_vutbl[VENDOR_CHINON].start_stop = chin_start_stop;
	scsipt_vutbl[VENDOR_CHINON].get_playstatus = chin_get_playstatus;
	scsipt_vutbl[VENDOR_CHINON].volume = NULL;
	scsipt_vutbl[VENDOR_CHINON].route = NULL;
	scsipt_vutbl[VENDOR_CHINON].mute = NULL;
	scsipt_vutbl[VENDOR_CHINON].get_toc = chin_get_toc;
	scsipt_vutbl[VENDOR_CHINON].eject = chin_eject;
	scsipt_vutbl[VENDOR_CHINON].start = NULL;
	scsipt_vutbl[VENDOR_CHINON].halt = NULL;
}


#endif	/* VENDOR_CHINON */

