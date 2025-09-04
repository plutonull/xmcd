/*
 *   cdda - CD Digital Audio support
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
 *   SCSI pass-through CDDA read support
 */
#ifndef lint
static char *_rd_scsi_c_ident_ = "@(#)rd_scsi.c	7.75 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_RD_SCSIPT) && defined(CDDA_SUPPORTED)

#include "libdi_d/scsipt.h"
#include "cdda_d/rd_scsi.h"

extern appdata_t	app_data;
extern FILE		*errfp;

extern cdda_client_t	*cdda_clinfo;

STATIC int		spt_rsemid,		/* Semaphores identifier */
			spt_errcnt = 0;		/* Read error count */
STATIC sword32_t	spt_errblk = -1;	/* Block addr of read error */
STATIC di_dev_t		*spt_rdp;		/* CD device descriptor */
STATIC cd_state_t	*spt_rcd;		/* CD state pointer */
STATIC bool_t		spt_modesel = FALSE;	/* CDDA mode enabled */


/*
 * scsicd_read_fillbuf
 *	Before beginning to play we fill the data buffer.
 *
 * Args:
 *	devp - device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
scsicd_read_fillbuf(di_dev_t *devp, curstat_t *s)
{
	byte_t			*start,
				*end,
				*p;
	mode_sense_6_data_t	*ms_data6 = NULL;
	mode_sense_10_data_t	*ms_data10 = NULL;
	blk_desc_t		*bdesc;
	int			i,
				j,
				hbcnt,
				hbint,
				lba,
				max_xfrms,
				remain_frms;
	cdda_req_t		cdda;
	req_sense_data_t	sense;
	byte_t			buf[SZ_MSENSE];

	/* Set heartbeat and interval */
	cdda_heartbeat(&spt_rcd->i->reader_hb, CDDA_HB_READER);
	hbcnt = hbint = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* Get lock */
	cdda_waitsem(spt_rsemid, LOCK);

	/* Enable device */
	scsipt_enable(devp, DI_ROLE_READER);

	/* Make sure disc is ready */
	if (!scsipt_disc_present(devp, DI_ROLE_READER, s, NULL)) {
		cdda_postsem(spt_rsemid, LOCK);
		return FALSE;
	}

	/* Initialize buffer location */
	max_xfrms = pthru_maxfer(devp) / CDDA_BLKSZ;
	start = &spt_rcd->cdb->data[spt_rcd->cds->olap_bytes];

	if (app_data.cdda_modesel) {
		(void) memset(buf, 0, sizeof(buf));

		if (app_data.msen_10) {
			ms_data10 = (mode_sense_10_data_t *)(void *) buf;
			ms_data10->bdescr_len =	
				util_bswap16(sizeof(blk_desc_t));
			bdesc = (blk_desc_t *)(void *) ms_data10->data;
		}
		else {
			ms_data6 = (mode_sense_6_data_t *)(void *) buf;
			ms_data6->bdescr_len = sizeof(blk_desc_t);
			bdesc = (blk_desc_t *)(void *) ms_data6->data;
		}

		/* Set Density code for CDDA */
		bdesc->dens_code = (byte_t) app_data.cdda_scsidensity;

		/* Block size for CDDA */
		bdesc->blk_len = util_bswap24(CDDA_BLKSZ);

		/* Write new setting */
		if (!scsipt_modesel(devp, DI_ROLE_READER, buf, 0, 0)) {
			cdda_postsem(spt_rsemid, LOCK);
			return FALSE;
		}

		spt_modesel = TRUE;
	}

	/* Set up for read */
	lba = (int) spt_rcd->i->start_lba;

	/* Fill up our buffer */
	i = 0;

	while (i < spt_rcd->cds->buffer_chunks && !spt_rcd->i->cdda_done) {
		remain_frms =
			spt_rcd->cds->chunk_blocks + spt_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * spt_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + spt_rcd->cds->chunk_blocks) >=
		    spt_rcd->i->end_lba) {
			remain_frms = spt_rcd->i->end_lba - lba;
			(void) memset(
				spt_rcd->cdb->data,
				0,
				spt_rcd->cds->chunk_bytes +
				(spt_rcd->cds->olap_bytes << 1)
			);
			spt_rcd->i->cdda_done = 1;
		}

		cdda.pos.logical = (sword32_t) lba;
		cdda.buf = (byte_t *) start;
		cdda.addrfmt = READFMT_LBA;

		do {
		    j = 0;
		    cdda.nframes = (remain_frms < max_xfrms) ?
					remain_frms : max_xfrms;

		    for (;;) {
			bool_t	retry;

			/* Read CDDA */
			if (scsipt_read_cdda(devp, DI_ROLE_READER,
					     &cdda, CDDA_BLKSZ,
					     j == 0 ? 20 : 5, &sense))
			    break;

			switch (sense.key) {
			case 0x1:	/* recovered error */
			    retry = FALSE;
			    break;

			case 0x2:	/* not ready */
			case 0x6:	/* unit attention */
			    if (++j > MAX_CDDAREAD_RETRIES) {
				cdda_postsem(spt_rsemid, LOCK);
				return FALSE;
			    }

			    if (!scsipt_disc_present(devp, DI_ROLE_READER,
						     s, NULL)) {
				cdda_postsem(spt_rsemid, LOCK);
				return FALSE;
			    }
			    retry = TRUE;
			    break;

			case 0x3:	/* medium error */
			case 0x0:	/* no sense */
			    if (++j <= MAX_CDDAREAD_RETRIES) {
				(void) fprintf(errfp, "Retrying...\n");
				retry = TRUE;
				break;
			    }

			    /* Skip to the next chunk if within error
			     * recovery threshold.
			     */
			    if (++spt_errcnt < MAX_RECOVERR) {
				spt_errblk = (sword32_t) cdda.pos.logical;
				(void) fprintf(errfp,
					    "Skipping chunk addr=0x%x.\n",
					    (int) spt_errblk);
				retry = FALSE;
				break;
			    }
			    /*FALLTHROUGH*/

			default:
			    cdda_postsem(spt_rsemid, LOCK);
			    return FALSE;
			}

			if (!retry)
			    break;
		    }

		    cdda.pos.logical += cdda.nframes;
		    cdda.buf += (cdda.nframes * CDDA_BLKSZ);
		    remain_frms -= cdda.nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		if (i == 0)
			p = &spt_rcd->cdb->data[spt_rcd->cds->olap_bytes];
		else
			p = cdda_rd_corrjitter(spt_rcd);

		/* Data end */
		end = p + spt_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (spt_rcd->i->jitter) {
			(void) memcpy(
				spt_rcd->cdb->olap,
				end - spt_rcd->cds->search_bytes,
				spt_rcd->cds->search_bytes << 1
			);
		}

		/* Copy in */
		(void) memcpy(
			&spt_rcd->cdb->b[
			    spt_rcd->cds->chunk_bytes * spt_rcd->cdb->nextin
			],
			p,
			spt_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		spt_rcd->cdb->nextin++;
		spt_rcd->cdb->nextin %= spt_rcd->cds->buffer_chunks;
		spt_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		i++;

		/* Update debug level */
		app_data.debug = spt_rcd->i->debug;

		/* Set heartbeat */
		if (--hbcnt == 0) {
			cdda_heartbeat(&spt_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = hbint;
		}
	}

	/* No room available now */
	cdda_waitsem(spt_rsemid, ROOM);

	/* Signal data available */
	cdda_postsem(spt_rsemid, DATA);

	/* Release lock */
	cdda_postsem(spt_rsemid, LOCK);

	return TRUE;
}


/*
 * scsicd_read_cont
 *	Function responsible for reading from the cd,
 *	correcting for jitter and writing to the buffer.
 *
 * Args:
 *	devp - device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
scsicd_read_cont(di_dev_t *devp, curstat_t *s)
{
	byte_t			*start,
				*end,
				*p;
	int			j,
				hbcnt,
				lba,
				max_xfrms,
				remain_frms;
	cdda_req_t		cdda;
	req_sense_data_t	sense;

	/* Initialize */
	max_xfrms = pthru_maxfer(devp) / CDDA_BLKSZ;
	spt_rcd->i->writer_hb_timeout = app_data.hb_timeout;
	start = &spt_rcd->cdb->data[spt_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = spt_rcd->i->start_lba + spt_rcd->cds->buffer_blocks;

	/* Set heartbeat interval */
	hbcnt = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* While not stopped or finished */
	while (spt_rcd->i->state != CDSTAT_COMPLETED &&
	       !spt_rcd->i->cdda_done) {
		remain_frms =
			spt_rcd->cds->chunk_blocks + spt_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * spt_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + spt_rcd->cds->chunk_blocks) >=
		    spt_rcd->i->end_lba) {
			remain_frms = spt_rcd->i->end_lba - lba;
			(void) memset(
				spt_rcd->cdb->data,
				0,
				spt_rcd->cds->chunk_bytes +
				(spt_rcd->cds->olap_bytes << 1)
			);
			spt_rcd->i->cdda_done = 1;
		}

		cdda.pos.logical = (sword32_t) lba;
		cdda.buf = (byte_t *) start;
		cdda.addrfmt = READFMT_LBA;

		do {
		    j = 0;
		    cdda.nframes = (remain_frms < max_xfrms) ?
					remain_frms : max_xfrms;

		    for (;;) {
			bool_t	retry;

			/* Read CDDA */
			if (scsipt_read_cdda(devp, DI_ROLE_READER,
					     &cdda, CDDA_BLKSZ,
					     j == 0 ? 20 : 5, &sense))
			    break;

			switch (sense.key) {
			case 0x1:	/* recovered error */
			    retry = FALSE;
			    break;

			case 0x2:	/* not ready */
			case 0x6:	/* unit attention */
			    if (++j > MAX_CDDAREAD_RETRIES)
				return FALSE;

			    if (!scsipt_disc_present(devp, DI_ROLE_READER,
						     s, NULL))
				return FALSE;

			    retry = TRUE;
			    break;

			case 0x3:	/* medium error */
			case 0x0:	/* no sense */
			    if (++j <= MAX_CDDAREAD_RETRIES) {
				(void) fprintf(errfp, "Retrying...\n");
				retry = TRUE;
				break;
			    }

			    /* Skip to the next chunk if within error
			     * recovery threshold.
			     */
			    if (++spt_errcnt < MAX_RECOVERR) {
				spt_errblk = (sword32_t) cdda.pos.logical;
				(void) fprintf(errfp,
					    "Skipping chunk addr=0x%x.\n",
					    (int) spt_errblk);
				retry = FALSE;
				break;
			    }
			    /*FALLTHROUGH*/

			default:
			    return FALSE;
			}

			if (!retry)
			    break;
		    }

		    /* If we haven't encountered an error for a while,
		     * then clear the error count.
		     */
		    if (j == 0 && spt_errcnt > 0 &&
			((sword32_t) cdda.pos.logical - spt_errblk) >
			ERR_CLRTHRESH)
			    spt_errcnt = 0;

		    cdda.pos.logical += cdda.nframes;
		    cdda.buf += (cdda.nframes * CDDA_BLKSZ);
		    remain_frms -= cdda.nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		p = cdda_rd_corrjitter(spt_rcd);

		/* Data end */
		end = p + spt_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (spt_rcd->i->jitter) {
			(void) memcpy(
				spt_rcd->cdb->olap,
				end - spt_rcd->cds->search_bytes,
				spt_rcd->cds->search_bytes << 1
			);
		}

		/* Get lock */
		cdda_waitsem(spt_rsemid, LOCK);

		/* Wait until there is room */
		while (spt_rcd->cdb->occupied >= spt_rcd->cds->buffer_chunks &&
		       spt_rcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(spt_rsemid, LOCK);
			cdda_waitsem(spt_rsemid, ROOM);
			cdda_waitsem(spt_rsemid, LOCK);
		}

		/* Break if completed */
		if (spt_rcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(spt_rsemid, LOCK);
			break;
		}

		/* Copy to free location */
		(void) memcpy(
			&spt_rcd->cdb->b[
			    spt_rcd->cds->chunk_bytes * spt_rcd->cdb->nextin
			],
			p,
			spt_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		spt_rcd->cdb->nextin++;
		spt_rcd->cdb->nextin %= spt_rcd->cds->buffer_chunks;
		spt_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		/* Signal data available */
		cdda_postsem(spt_rsemid, DATA);

		/* Release lock */
		cdda_postsem(spt_rsemid, LOCK);

		/* Update debug level */
		app_data.debug = spt_rcd->i->debug;

		/* Set heartbeat and interval */
		if (--hbcnt == 0) {
			cdda_heartbeat(&spt_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = cdda_heartbeat_interval(
				spt_rcd->i->frm_per_sec > 0 ?
					spt_rcd->i->frm_per_sec : FRAME_PER_SEC
			);
		}
	}

	/* Wake up writer */
	cdda_postsem(spt_rsemid, DATA);

	return TRUE;
}


/*
 * scsi_rinit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
scsi_rinit(void)
{
	return CDDA_READAUDIO;
}


/*
 * scsi_read
 *	Attaches to shared memory and semaphores. Continuously reads
 *	data from the cd and places into shared memory.
 *
 * Args:
 *	devp - device descriptor
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
/*ARGSUSED*/
bool_t
scsi_read(di_dev_t *devp)
{
	curstat_t	*s = cdda_clinfo->curstat_addr();

	/* Check configuration */
	if (app_data.di_method != 0 /* DI_SCSIPT */) {
		(void) fprintf(errfp,
			"scsi_read: Inconsistent deviceInterfaceMethod "
			"and cddaReadMethod parameters.  Aborting.\n");
		return FALSE;
	}

	/* Save file descriptor */
	spt_rdp = devp;

	/* Allocate memory */
	spt_rcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (spt_rcd == NULL) {
		(void) fprintf(errfp, "scsi_read: out of memory\n");
		return FALSE;
	}

	(void) memset(spt_rcd, 0, sizeof(cd_state_t));

	/* Initialize cd pointers to point into shared memory */
	if ((spt_rsemid = cdda_initipc(spt_rcd)) < 0)
		return FALSE;

	DBGPRN(DBG_DEVIO)(errfp,
			  "\nscsi_read: Reading CDDA: "
			  "chunk_blks=%d, olap_blks=%d\n",
			  spt_rcd->cds->chunk_blocks,
			  spt_rcd->cds->olap_blocks);

	/* Fill data buffer */
	if (!scsicd_read_fillbuf(devp, s))
		return FALSE;

	/* Keep reading */
	if (!scsicd_read_cont(devp, s))
		return FALSE;

	return TRUE;
}


/*
 * scsi_rdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killwriter - whether to terminate the writer thread or process
 *
 * Return:
 *	Nothing.
 */
void
scsi_rdone(bool_t killwriter)
{
	mode_sense_6_data_t	*ms_data6 = NULL;
	mode_sense_10_data_t	*ms_data10 = NULL;
	blk_desc_t		*bdesc;
	byte_t			buf[SZ_MSENSE];

	DBGPRN(DBG_DEVIO)(errfp, "\nscsi_rdone: Cleaning up reader\n");

	/* Restore CD-ROM mode if necessary */
	if (spt_modesel && app_data.cdda_modesel) {
		(void) memset(buf, 0, sizeof(buf));

		if (app_data.msen_10) {
			ms_data10 = (mode_sense_10_data_t *)(void *) buf;
			ms_data10->bdescr_len =
				util_bswap16(sizeof(blk_desc_t));
			bdesc = (blk_desc_t *)(void *) ms_data10->data;
		}
		else {
			ms_data6 = (mode_sense_6_data_t *)(void *) buf;
			ms_data6->bdescr_len = sizeof(blk_desc_t);
			bdesc = (blk_desc_t *)(void *) ms_data6->data;
		}

		/* Restore density code */
		bdesc->dens_code = 0;

		/* Restore sector size */
		bdesc->blk_len = util_bswap24(app_data.drv_blksz);

		/* Restore setting */
		(void) scsipt_modesel(spt_rdp, DI_ROLE_READER, buf, 0, 0);

		spt_modesel = FALSE;
	}

	scsipt_disable(spt_rdp, DI_ROLE_READER);
	spt_rdp = NULL;

	cdda_yield();

	if (spt_rcd != NULL) {
		if (killwriter && spt_rcd->i != NULL &&
		    spt_rcd->i->writer != (thid_t) 0) {
			cdda_kill(spt_rcd->i->writer, SIGTERM);
			spt_rcd->i->state = CDSTAT_COMPLETED;
		}

		MEM_FREE(spt_rcd);
		spt_rcd = NULL;
	}
}


/*
 * scsi_rinfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
scsi_rinfo(char *str)
{
	(void) strcat(str, "SCSI pass-through\n");
}

#endif	/* CDDA_RD_SCSIPT CDDA_SUPPORTED */

