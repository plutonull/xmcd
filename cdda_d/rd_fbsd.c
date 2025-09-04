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
 *   FreeBSD/OpenBSD/NetBSD CDDA read ioctl support
 */
#ifndef lint
static char *_rd_fbsd_c_ident_ = "@(#)rd_fbsd.c	7.57 04/04/06";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_RD_FBSD) && defined(CDDA_SUPPORTED)

#include <sys/cdio.h>
#include "libdi_d/fbioc.h"
#include "cdda_d/rd_fbsd.h"

extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		fb_rsemid,		/* Semaphores identifier */
			fb_errcnt = 0;		/* Read error count */
STATIC sword32_t	fb_errblk = -1;		/* Block addr of read error */
STATIC cd_state_t	*fb_rcd;		/* CD state pointer */


/*
 * fbcd_read_fillbuf
 *	Before beginning to play we fill the data buffer.
 *
 * Args:
 *	devp - device descriptor
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
/*ARGSUSED*/
STATIC bool_t
fbcd_read_fillbuf(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p;
	int			i,
				j,
				hbcnt,
				hbint,
				lba,
				remain_frms,
				nbytes;
	struct ioc_read_audio	cdda;

	/* Set heartbeat and interval */
	cdda_heartbeat(&fb_rcd->i->reader_hb, CDDA_HB_READER);
	hbcnt = hbint = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* Get lock */
	cdda_waitsem(fb_rsemid, LOCK);

	/* Enable device */
	fbioc_enable(DI_ROLE_READER);

	/* Initialize buffer location */
	start = &fb_rcd->cdb->data[fb_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = fb_rcd->i->start_lba;

	(void) memset(&cdda, 0, sizeof(cdda));
	cdda.address_format = CD_LBA_FORMAT;

	/* Fill up our buffer */
	i = 0;

	while (i < fb_rcd->cds->buffer_chunks && !fb_rcd->i->cdda_done) {
		remain_frms =
			fb_rcd->cds->chunk_blocks + fb_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * fb_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if (lba + fb_rcd->cds->chunk_blocks >= fb_rcd->i->end_lba) {
			remain_frms = fb_rcd->i->end_lba - lba;
			(void) memset(
				fb_rcd->cdb->data,
				0,
				fb_rcd->cds->chunk_bytes +
				(fb_rcd->cds->olap_bytes << 1)
			);
			fb_rcd->i->cdda_done = 1;
		}

		cdda.address.lba = lba;
		cdda.buffer = (u_char *) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.nframes = ((nbytes < BSD_CDDA_MAXFER) ?
				    nbytes : BSD_CDDA_MAXFER) / CDDA_BLKSZ;
		    nbytes = cdda.nframes * CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !fbioc_send(DI_ROLE_READER, CDIOCREADAUDIO,
				     &cdda, TRUE);
			 j++) {

			if (j < MAX_CDDAREAD_RETRIES) {
			    if (i == 0 && app_data.spinup_interval > 0)
				util_delayms(app_data.spinup_interval * 1000);
			    else
				util_delayms(200);

			    (void) fprintf(errfp, "Retrying...\n");
			    continue;
			}

			/* Skip to the next chunk if within error
			 * recovery threshold.
			 */
			if (++fb_errcnt < MAX_RECOVERR) {
			    fb_errblk = (sword32_t) cdda.address.lba;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) fb_errblk);
			    break;
			}

			cdda_postsem(fb_rsemid, LOCK);
			return FALSE;
		    }

		    if (fb_errblk != (sword32_t) cdda.address.lba) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: CDIOCREADAUDIO blk=%d len=%d\n",
				    (int) cdda.address.lba,
				    (int) cdda.nframes);
		    }

		    cdda.address.lba += cdda.nframes;
		    cdda.buffer += nbytes;
		    remain_frms -= cdda.nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		if (i == 0)
			p = &fb_rcd->cdb->data[fb_rcd->cds->olap_bytes];
		else
			p = cdda_rd_corrjitter(fb_rcd);

		/* Data end */
		end = p + fb_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (fb_rcd->i->jitter) {
			(void) memcpy(
				fb_rcd->cdb->olap,
				end - fb_rcd->cds->search_bytes,
				fb_rcd->cds->search_bytes << 1
			);
		}

		/* Copy in */
		(void) memcpy(
			&fb_rcd->cdb->b[
			    fb_rcd->cds->chunk_bytes * fb_rcd->cdb->nextin
			],
			p,
			fb_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		fb_rcd->cdb->nextin++;
		fb_rcd->cdb->nextin %= fb_rcd->cds->buffer_chunks;
		fb_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);

		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		i++;

		/* Update debug level */
		app_data.debug = fb_rcd->i->debug;

		/* Set heartbeat */
		if (--hbcnt == 0) {
			cdda_heartbeat(&fb_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = hbint;
		}
	}

	/* No room available now */
	cdda_waitsem(fb_rsemid, ROOM);

	/* Signal data available */
	cdda_postsem(fb_rsemid, DATA);

	/* Release lock */
	cdda_postsem(fb_rsemid, LOCK);

	return TRUE;
}


/*
 * fbcd_read_cont
 *	Function responsible for reading from the cd,
 *	correcting for jitter and writing to the buffer.
 *
 * Args:
 *	devp - device descriptor
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
/*ARGSUSED*/
STATIC bool_t
fbcd_read_cont(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p;
	int			j,
				hbcnt,
				lba,
				remain_frms,
				nbytes;
	struct ioc_read_audio	cdda;

	/* Initialize */
	fb_rcd->i->writer_hb_timeout = app_data.hb_timeout;
	start = &fb_rcd->cdb->data[fb_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = fb_rcd->i->start_lba + fb_rcd->cds->buffer_blocks;

	(void) memset(&cdda, 0, sizeof(cdda));
	cdda.address_format = CD_LBA_FORMAT;

	/* Set heartbeat interval */
	hbcnt = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* While not stopped or finished */
	while (fb_rcd->i->state != CDSTAT_COMPLETED && !fb_rcd->i->cdda_done) {
		remain_frms =
			fb_rcd->cds->chunk_blocks + fb_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * fb_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if (lba + fb_rcd->cds->chunk_blocks >= fb_rcd->i->end_lba) {
			remain_frms = fb_rcd->i->end_lba - lba;
			(void) memset(
				fb_rcd->cdb->data,
				0,
				fb_rcd->cds->chunk_bytes +
				(fb_rcd->cds->olap_bytes << 1)
			);
			fb_rcd->i->cdda_done = 1;
		}

		cdda.address.lba = lba;
		cdda.buffer = (u_char *) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.nframes = ((nbytes < BSD_CDDA_MAXFER) ?
				    nbytes : BSD_CDDA_MAXFER) / CDDA_BLKSZ;
		    nbytes = cdda.nframes * CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !fbioc_send(DI_ROLE_READER, CDIOCREADAUDIO,
				     &cdda, TRUE);
			 j++) {

			if (j < MAX_CDDAREAD_RETRIES) {
			    util_delayms(200);
			    (void) fprintf(errfp, "Retrying...\n");
			    continue;
			}

			/* Skip to the next chunk if within error
			 * recovery threshold.
			 */
			if (++fb_errcnt < MAX_RECOVERR) {
			    fb_errblk = (sword32_t) cdda.address.lba;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) fb_errblk);
			    break;
			}

			return FALSE;
		    }

		    if (fb_errblk != (sword32_t) cdda.address.lba) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: CDIOCREADAUDIO blk=%d len=%d\n",
				    (int) cdda.address.lba,
				    (int) cdda.nframes);
		    }

		    /* If we haven't encountered an error for a while,
		     * then clear the error count.
		     */
		    if (j == 0 && fb_errcnt > 0 &&
			((sword32_t) cdda.address.lba - fb_errblk) >
			ERR_CLRTHRESH)
			    fb_errcnt = 0;

		    cdda.address.lba += cdda.nframes;
		    cdda.buffer += nbytes;
		    remain_frms -= cdda.nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		p = cdda_rd_corrjitter(fb_rcd);

		/* Data end */
		end = p + fb_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (fb_rcd->i->jitter) {
			(void) memcpy(
				fb_rcd->cdb->olap,
				end - fb_rcd->cds->search_bytes,
				fb_rcd->cds->search_bytes << 1
			);
		}

		/* Get lock */
		cdda_waitsem(fb_rsemid, LOCK);

		/* Wait until there is room */
		while (fb_rcd->cdb->occupied >= fb_rcd->cds->buffer_chunks &&
		       fb_rcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(fb_rsemid, LOCK);
			cdda_waitsem(fb_rsemid, ROOM);
			cdda_waitsem(fb_rsemid, LOCK);
		}

		/* Break if completed */
		if (fb_rcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(fb_rsemid, LOCK);
			break;
		}

		/* Copy to free location */
		(void) memcpy(
			&fb_rcd->cdb->b[
			    fb_rcd->cds->chunk_bytes * fb_rcd->cdb->nextin
			],
			p,
			fb_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		fb_rcd->cdb->nextin++;
		fb_rcd->cdb->nextin %= fb_rcd->cds->buffer_chunks;
		fb_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);

		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		/* Signal data available */
		cdda_postsem(fb_rsemid, DATA);

		/* Release lock */
		cdda_postsem(fb_rsemid, LOCK);

		/* Update debug level */
		app_data.debug = fb_rcd->i->debug;

		/* Set heartbeat and interval */
		if (--hbcnt == 0) {
			cdda_heartbeat(&fb_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = cdda_heartbeat_interval(
				fb_rcd->i->frm_per_sec > 0 ?
					fb_rcd->i->frm_per_sec : FRAME_PER_SEC
			);
		}
	}

	/* Wake up writer */
	cdda_postsem(fb_rsemid, DATA);

	return TRUE;
}


/*
 * fbsd_rinit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
fbsd_rinit(void)
{
	return CDDA_READAUDIO;
}


/*
 * fbsd_read
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
bool_t
fbsd_read(di_dev_t *devp)
{
	/* Check configuration */
	if (app_data.di_method != 2 /* DI_FBIOC */) {
		(void) fprintf(errfp,
			"fbsd_read: Inconsistent deviceInterfaceMethod "
			"and cddaReadMethod parameters.  Aborting.\n");
		return FALSE;
	}

	/* Allocate memory */
	fb_rcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (fb_rcd == NULL) {
		(void) fprintf(errfp, "fbsd_read: out of memory\n");
		return FALSE;
	}

	(void) memset(fb_rcd, 0, sizeof(cd_state_t));

	/* Initialize cd pointers to point into shared memory */
	if ((fb_rsemid = cdda_initipc(fb_rcd)) < 0)
		return FALSE;

	DBGPRN(DBG_DEVIO)(errfp,
			  "\nfbcd_read: Reading CDDA: "
			  "chunk_blks=%d, olap_blks=%d\n",
			  fb_rcd->cds->chunk_blocks, fb_rcd->cds->olap_blocks);

	/* Fill data buffer */
	if (!fbcd_read_fillbuf(devp))
		return FALSE;

	/* Keep reading */
	if (!fbcd_read_cont(devp))
		return FALSE;

	return TRUE;
}


/*
 * fbsd_rdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killwriter - whether to terminate the writer thread or process
 *
 * Return:
 *	Nothing.
 */
void
fbsd_rdone(bool_t killwriter)
{
	DBGPRN(DBG_DEVIO)(errfp, "\nfbcd_rdone: Cleaning up reader\n");

	/* Disable device */
	fbioc_disable(DI_ROLE_READER);

	cdda_yield();

	if (fb_rcd != NULL) {
		if (killwriter && fb_rcd->i != NULL &&
		    fb_rcd->i->writer != (thid_t) 0) {
			cdda_kill(fb_rcd->i->writer, SIGTERM);
			fb_rcd->i->state = CDSTAT_COMPLETED;
		}

		MEM_FREE(fb_rcd);
		fb_rcd = NULL;
	}
}


/*
 * fbsd_rinfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
fbsd_rinfo(char *str)
{
	(void) strcat(str, "FreeBSD ioctl\n");
}

#endif	/* CDDA_RD_FBSD CDDA_SUPPORTED */

