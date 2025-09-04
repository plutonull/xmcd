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
 *   Linux CDDA read ioctl support
 */
#ifndef lint
static char *_rd_linux_c_ident_ = "@(#)rd_linux.c	7.60 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_RD_LINUX) && defined(CDDA_SUPPORTED)

#include <linux/cdrom.h>
#include "libdi_d/slioc.h"
#include "cdda_d/rd_linux.h"

extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		lx_rsemid,		/* Semaphores identifier */
			lx_errcnt = 0;		/* Read error count */
STATIC sword32_t	lx_errblk = -1;		/* Block addr of read error */
STATIC cd_state_t	*lx_rcd;		/* CD state pointer */


/*
 * lxcd_read_fillbuf
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
lxcd_read_fillbuf(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p;
	int			i,
				j,
				hbcnt,
				hbint,
				lba,
				nbytes,
				remain_frms;
	struct cdrom_read_audio	cdda;

	/* Set heartbeat and interval */
	cdda_heartbeat(&lx_rcd->i->reader_hb, CDDA_HB_READER);
	hbcnt = hbint = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* Get lock */
	cdda_waitsem(lx_rsemid, LOCK);

	/* Enable device */
	slioc_enable(DI_ROLE_READER);

	/* Initialize buffer location */
	start = &lx_rcd->cdb->data[lx_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = lx_rcd->i->start_lba;

	(void) memset(&cdda, 0, sizeof(cdda));
	cdda.addr_format = CDROM_LBA;

	/* Fill up our buffer */
	i = 0;

	while (i < lx_rcd->cds->buffer_chunks && !lx_rcd->i->cdda_done) {
		remain_frms =
			lx_rcd->cds->chunk_blocks + lx_rcd->cds->olap_blocks;
		
		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * lx_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + lx_rcd->cds->chunk_blocks) >= lx_rcd->i->end_lba) {
			remain_frms = lx_rcd->i->end_lba - lba;
			(void) memset(
				lx_rcd->cdb->data,
				0,
				lx_rcd->cds->chunk_bytes +
				(lx_rcd->cds->olap_bytes << 1)
			);
			lx_rcd->i->cdda_done = 1;
		}

		cdda.addr.lba = lba;
		cdda.buf = (u_char *) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.nframes = ((nbytes < LNX_CDDA_MAXFER) ?
				    nbytes : LNX_CDDA_MAXFER) / CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !slioc_send(DI_ROLE_READER, CDROMREADAUDIO,
				     &cdda, sizeof(cdda), TRUE, NULL);
			 j++) {
			(void) fprintf(errfp,
			    "\nIOCTL: CDROMREADAUDIO failed: %s\n",
			    strerror(errno)
			);

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
			if (++lx_errcnt < MAX_RECOVERR) {
			    lx_errblk = (sword32_t) cdda.addr.lba;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) lx_errblk);
			    break;
			}

			cdda_postsem(lx_rsemid, LOCK);
			return FALSE;
		    }

		    if (lx_errblk != (sword32_t) cdda.addr.lba) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: CDROMREADAUDIO blk=%d len=%d\n",
				    (int) cdda.addr.lba, (int) cdda.nframes);
		    }

		    cdda.addr.lba += cdda.nframes;
		    cdda.buf += (cdda.nframes * CDDA_BLKSZ);
		    remain_frms -= cdda.nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		if (i == 0)
			p = &lx_rcd->cdb->data[lx_rcd->cds->olap_bytes];
		else
			p = cdda_rd_corrjitter(lx_rcd);

		/* Data end */
		end = p + lx_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (lx_rcd->i->jitter) {
			(void) memcpy(
				lx_rcd->cdb->olap,
				end - lx_rcd->cds->search_bytes,
				lx_rcd->cds->search_bytes << 1
			);
		}

		/* Copy in */
		(void) memcpy(
			&lx_rcd->cdb->b[
			    lx_rcd->cds->chunk_bytes * lx_rcd->cdb->nextin
			],
			p,
			lx_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		lx_rcd->cdb->nextin++;
		lx_rcd->cdb->nextin %= lx_rcd->cds->buffer_chunks;
		lx_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		i++;

		/* Update debug level */
		app_data.debug = lx_rcd->i->debug;

		/* Set heartbeat */
		if (--hbcnt == 0) {
			cdda_heartbeat(&lx_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = hbint;
		}
	}

	/* No room available now */
	cdda_waitsem(lx_rsemid, ROOM);

	/* Signal data available */
	cdda_postsem(lx_rsemid, DATA);

	/* Release lock */
	cdda_postsem(lx_rsemid, LOCK);

	return TRUE;
}


/*
 * lxcd_read_cont
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
lxcd_read_cont(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p;
	int			j,
				hbcnt,
				lba,
				nbytes,
				remain_frms;
	struct cdrom_read_audio	cdda;

	/* Initialize */
	lx_rcd->i->writer_hb_timeout = app_data.hb_timeout;
	start = &lx_rcd->cdb->data[lx_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = lx_rcd->i->start_lba + lx_rcd->cds->buffer_blocks;

	(void) memset(&cdda, 0, sizeof(cdda));
	cdda.addr_format = CDROM_LBA;

	/* Set heartbeat interval */
	hbcnt = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* While not stopped or finished */
	while (lx_rcd->i->state != CDSTAT_COMPLETED && !lx_rcd->i->cdda_done) {
		remain_frms =
			lx_rcd->cds->chunk_blocks + lx_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * lx_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if (lba + lx_rcd->cds->chunk_blocks >= lx_rcd->i->end_lba) {
			remain_frms = lx_rcd->i->end_lba - lba;
			(void) memset(
				lx_rcd->cdb->data,
				0,
				lx_rcd->cds->chunk_bytes +
				(lx_rcd->cds->olap_bytes << 1)
			);
			lx_rcd->i->cdda_done = 1;
		}

		cdda.addr.lba = lba;
		cdda.buf = (u_char *) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.nframes = ((nbytes < LNX_CDDA_MAXFER) ?
				    nbytes : LNX_CDDA_MAXFER) / CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !slioc_send(DI_ROLE_READER, CDROMREADAUDIO,
				     &cdda, sizeof(cdda), TRUE, NULL);
			 j++) {
			(void) fprintf(errfp,
			    "\nIOCTL: CDROMREADAUDIO failed: %s\n",
			    strerror(errno)
			);

			if (j < MAX_CDDAREAD_RETRIES) {
			    util_delayms(200);
			    (void) fprintf(errfp, "Retrying...\n");
			    continue;
			}

			/* Skip to the next chunk if within error
			 * recovery threshold.
			 */
			if (++lx_errcnt < MAX_RECOVERR) {
			    lx_errblk = (sword32_t) cdda.addr.lba;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) lx_errblk);
			    break;
			}

			return FALSE;
		    }

		    if (lx_errblk != (sword32_t) cdda.addr.lba) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: CDROMREADAUDIO blk=%d len=%d\n",
				    (int) cdda.addr.lba, (int) cdda.nframes);
		    }

		    /* If we haven't encountered an error for a while,
		     * then clear the error count.
		     */
		    if (j == 0 && lx_errcnt > 0 &&
			((sword32_t) cdda.addr.lba - lx_errblk) >
			ERR_CLRTHRESH)
			    lx_errcnt = 0;

		    cdda.addr.lba += cdda.nframes;
		    cdda.buf += (cdda.nframes * CDDA_BLKSZ);
		    remain_frms -= cdda.nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		p = cdda_rd_corrjitter(lx_rcd);

		/* Data end */
		end = p + lx_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (lx_rcd->i->jitter) {
			(void) memcpy(
				lx_rcd->cdb->olap,
				end - lx_rcd->cds->search_bytes,
				lx_rcd->cds->search_bytes << 1
			);
		}

		/* Get lock */
		cdda_waitsem(lx_rsemid, LOCK);

		/* Wait until there is room */
		while (lx_rcd->cdb->occupied >= lx_rcd->cds->buffer_chunks &&
		       lx_rcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(lx_rsemid, LOCK);
			cdda_waitsem(lx_rsemid, ROOM);
			cdda_waitsem(lx_rsemid, LOCK);
		}

		/* Break if completed */
		if (lx_rcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(lx_rsemid, LOCK);
			break;
		}

		/* Copy to free location */
		(void) memcpy(
			&lx_rcd->cdb->b[
			    lx_rcd->cds->chunk_bytes * lx_rcd->cdb->nextin
			],
			p,
			lx_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		lx_rcd->cdb->nextin++;
		lx_rcd->cdb->nextin %= lx_rcd->cds->buffer_chunks;
		lx_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		/* Signal data available */
		cdda_postsem(lx_rsemid, DATA);

		/* Update debug level */
		app_data.debug = lx_rcd->i->debug;

		/* Release lock */
		cdda_postsem(lx_rsemid, LOCK);

		/* Set heartbeat and interval */
		if (--hbcnt == 0) {
			cdda_heartbeat(&lx_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = cdda_heartbeat_interval(
				lx_rcd->i->frm_per_sec > 0 ?
					lx_rcd->i->frm_per_sec : FRAME_PER_SEC
			);
		}
	}

	/* Wake up writer */
	cdda_postsem(lx_rsemid, DATA);

	return TRUE;
}


/*
 * linux_rinit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
linux_rinit(void)
{
	return CDDA_READAUDIO;
}


/*
 * linux_read
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
linux_read(di_dev_t *devp)
{
	/* Check configuration */
	if (app_data.di_method != 1 /* DI_SLIOC */) {
		(void) fprintf(errfp,
			"linux_read: Inconsistent deviceInterfaceMethod "
			"and cddaReadMethod parameters.  Aborting.\n");
		return FALSE;
	}

	/* Allocate memory */
	lx_rcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (lx_rcd == NULL) {
		(void) fprintf(errfp, "linux_read: out of memory\n");
		return FALSE;
	}

	(void) memset(lx_rcd, 0, sizeof(cd_state_t));

	/* Initialize cd pointers to point into shared memory */
	if ((lx_rsemid = cdda_initipc(lx_rcd)) < 0)
		return FALSE;

	DBGPRN(DBG_DEVIO)(errfp,
			  "\nlxcd_read: Reading CDDA: "
			  "chunk_blks=%d, olap_blks=%d\n",
			  lx_rcd->cds->chunk_blocks, lx_rcd->cds->olap_blocks);

	/* Fill data buffer */
	if (!lxcd_read_fillbuf(devp))
		return FALSE;

	/* Keep reading */
	if (!lxcd_read_cont(devp))
		return FALSE;

	return TRUE;
}


/*
 * linux_rdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killwriter - whether to terminate the writer thread or process
 *
 * Return:
 *	Nothing.
 */
void
linux_rdone(bool_t killwriter)
{
	DBGPRN(DBG_DEVIO)(errfp, "\nlinux_rdone: Cleaning up reader\n");

	/* Disable device */
	slioc_disable(DI_ROLE_READER);

	cdda_yield();

	if (lx_rcd != NULL) {
		if (killwriter && lx_rcd->i != NULL &&
		    lx_rcd->i->writer != (thid_t) 0) {
			cdda_kill(lx_rcd->i->writer, SIGTERM);
			lx_rcd->i->state = CDSTAT_COMPLETED;
		}

		MEM_FREE(lx_rcd);
		lx_rcd = NULL;
	}
}


/*
 * linux_rinfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
linux_rinfo(char *str)
{
	(void) strcat(str, "Linux ioctl\n");
}

#endif	/* CDDA_RD_LINUX CDDA_SUPPORTED */

