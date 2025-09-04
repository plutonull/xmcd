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
 *   IBM AIX ATAPI/IDE CDDA read ioctl support
 */
#ifndef lint
static char *_rd_aix_c_ident_ = "@(#)rd_aix.c	7.55 04/02/08";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_RD_AIX) && defined(CDDA_SUPPORTED)

#include <sys/ide.h>
#include <sys/idecdrom.h>
#include <sys/scdisk.h>
#include <sys/cdrom.h>
#include "libdi_d/aixioc.h"
#include "cdda_d/rd_aix.h"

extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		aix_rsemid,		/* Semaphores identifier */
			aix_errcnt = 0;		/* Read error count */
STATIC sword32_t	aix_errblk = -1;	/* Block addr of read error */
STATIC cd_state_t	*aix_rcd;		/* CD state pointer */
STATIC bool_t		aix_rcddamode = FALSE;	/* Set to CDDA mode */


/*
 * aixcd_rdse_errmsg
 *	Display error message after a IDE_CDIORDSE ioctl failure.
 *
 * Args:
 *	err    - The errno from the ioctl
 *	reqp   - Pointer to the I/O request structure
 *	sensep - Pointer to the sense data
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixcd_rdse_errmsg(int err, struct ide_rdwrt *reqp, byte_t *sensep)
{
	(void) fprintf(errfp, "\nIOCTL: IDE_CDIORDSE failed:\n%s\n",
		       strerror(err));

	if (reqp->status_validity & ATA_IDE_STATUS) {
		(void) fprintf(errfp, "ata_status=0x%x\n",
			    (int) reqp->ata_status);
	}
	if (reqp->status_validity & ATA_ERROR) {
		(void) fprintf(errfp, "ata_error=0x%x\n",
			    (int) reqp->ata_error);
	}
	if (reqp->status_validity & ATA_DIAGNOSTICS_ERROR) {
		(void) fprintf(errfp, "adapter_diagnostics=0x%x\n",
			    (int) reqp->adapter_diagnostics);
	}
	if (reqp->status_validity & ATAPI_VALID_SENSE) {
		(void) fprintf(errfp,
			    "Sense data: Key=0x%x Code=0x%x Qual=0x%x\n",
			    (int) (sensep[2] & 0x0f),
			    (int) sensep[12],
			    (int) sensep[13]);
	}
}


/*
 * aixcd_read_fillbuf
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
aixcd_read_fillbuf(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p,
				sense[20];
	int			i,
				j,
				hbcnt,
				hbint,
				lba,
				remain_frms,
				nframes,
				nbytes;
	struct mode_form_op	m;
	struct ide_rdwrt	cdda;

	/* Set heartbeat and interval */
	cdda_heartbeat(&aix_rcd->i->reader_hb, CDDA_HB_READER);
	hbcnt = hbint = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* Get lock */
	cdda_waitsem(aix_rsemid, LOCK);

	/* Enable device */
	aixioc_enable(DI_ROLE_READER);

	/* Initialize buffer location */
	start = &aix_rcd->cdb->data[aix_rcd->cds->olap_bytes];

	/* Set CDDA mode */
	(void) memset(&m, 0, sizeof(m));
	m.action = CD_CHG_MODE;
	m.cd_mode_form = CD_DA;
	if (!aixioc_send(DI_ROLE_READER, IDE_CDMODE, &m, TRUE))
		return FALSE;

	aix_rcddamode = TRUE;

	/* Set up for read */
	lba = (int) aix_rcd->i->start_lba;

	(void) memset(&cdda, 0, sizeof(cdda));
	(void) memset(&sense, 0, sizeof(sense));
	cdda.timeout_value = 5;
	cdda.req_sense_length = sizeof(sense);
	cdda.request_sense_ptr = (char *) &sense;

	/* Fill up our buffer */
	i = 0;

	while (i < aix_rcd->cds->buffer_chunks && !aix_rcd->i->cdda_done) {
		remain_frms =
			aix_rcd->cds->chunk_blocks + aix_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * aix_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + aix_rcd->cds->chunk_blocks) >=
		    aix_rcd->i->end_lba) {
			remain_frms = aix_rcd->i->end_lba - lba;
			(void) memset(
				aix_rcd->cdb->data,
				0,
				aix_rcd->cds->chunk_bytes +
				(aix_rcd->cds->olap_bytes << 1)
			);
			aix_rcd->i->cdda_done = 1;
		}

		cdda.logical_blk_addr = lba;
		cdda.buffer = (char *) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.data_length = (nbytes < AIX_CDDA_MAXFER) ?
					    nbytes : AIX_CDDA_MAXFER;
		    nframes = cdda.data_length / CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !aixioc_send(DI_ROLE_READER,
				      IDE_CDIORDSE, &cdda, TRUE);
			 j++){

			aixcd_rdse_errmsg(errno, &cdda, sense);

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
			if (++aix_errcnt < MAX_RECOVERR) {
			    aix_errblk = (sword32_t) cdda.logical_blk_addr;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) aix_errblk);
			    break;
			}

			cdda_postsem(aix_rsemid, LOCK);
			return FALSE;
		    }

		    if (aix_errblk != (sword32_t) cdda.logical_blk_addr) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: IDE_CDIORDSE blk=%d len=%d\n",
				    (int) cdda.logical_blk_addr,
				    (int) cdda.data_length);
		    }

		    cdda.logical_blk_addr += nframes;
		    cdda.buffer += cdda.data_length;
		    remain_frms -= nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		if (i == 0)
			p = &aix_rcd->cdb->data[aix_rcd->cds->olap_bytes];
		else
			p = cdda_rd_corrjitter(aix_rcd);

		/* Data end */
		end = p + aix_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (aix_rcd->i->jitter) {
			(void) memcpy(
				aix_rcd->cdb->olap,
				end - aix_rcd->cds->search_bytes,
				aix_rcd->cds->search_bytes << 1
			);
		}

		/* Copy in */
		(void) memcpy(
			&aix_rcd->cdb->b[
			    aix_rcd->cds->chunk_bytes * aix_rcd->cdb->nextin
			],
			p,
			aix_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		aix_rcd->cdb->nextin++;
		aix_rcd->cdb->nextin %= aix_rcd->cds->buffer_chunks;
		aix_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		i++;

		/* Update debug level */
		app_data.debug = aix_rcd->i->debug;

		/* Set heartbeat */
		if (--hbcnt == 0) {
			cdda_heartbeat(&aix_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = hbint;
		}
	}

	/* No room available now */
	cdda_waitsem(aix_rsemid, ROOM);

	/* Signal data available */
	cdda_postsem(aix_rsemid, DATA);

	/* Release lock */
	cdda_postsem(aix_rsemid, LOCK);

	return TRUE;
}


/*
 * aixcd_read_cont
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
aixcd_read_cont(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p,
				sense[20];
	int			j,
				lba,
				hbcnt,
				remain_frms,
				nframes,
				nbytes;
	struct ide_rdwrt	cdda;

	/* Initialize */
	aix_rcd->i->writer_hb_timeout = app_data.hb_timeout;
	start = &aix_rcd->cdb->data[aix_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = aix_rcd->i->start_lba;

	(void) memset(&cdda, 0, sizeof(cdda));
	(void) memset(&sense, 0, sizeof(sense));
	cdda.timeout_value = 5;
	cdda.req_sense_length = sizeof(sense);
	cdda.request_sense_ptr = (char *) &sense;

	/* Set heartbeat interval */
	hbcnt = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* While not stopped or finished */
	while (aix_rcd->i->state != CDSTAT_COMPLETED &&
	       !aix_rcd->i->cdda_done) {
		remain_frms =
			aix_rcd->cds->chunk_blocks + aix_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * aix_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + aix_rcd->cds->chunk_blocks) >=
		    aix_rcd->i->end_lba) {
			remain_frms = aix_rcd->i->end_lba - lba;
			(void) memset(
				aix_rcd->cdb->data,
				0,
				aix_rcd->cds->chunk_bytes +
				(aix_rcd->cds->olap_bytes << 1)
			);
			aix_rcd->i->cdda_done = 1;
		}

		cdda.logical_blk_addr = lba;
		cdda.buffer = (char *) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.data_length = (nbytes < AIX_CDDA_MAXFER) ?
					    nbytes : AIX_CDDA_MAXFER;
		    nframes = cdda.data_length / CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !aixioc_send(DI_ROLE_READER, IDE_CDIORDSE,
				      &cdda, TRUE);
			 j++){

			aixcd_rdse_errmsg(errno, &cdda, sense);

			if (j < MAX_CDDAREAD_RETRIES) {
			    util_delayms(200);
			    (void) fprintf(errfp, "Retrying...\n");
			    continue;
			}

			/* Skip to the next chunk if within error
			 * recovery threshold.
			 */
			if (++aix_errcnt < MAX_RECOVERR) {
			    aix_errblk = (sword32_t) cdda.logical_blk_addr;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) aix_errblk);
			    break;
			}

			return FALSE;
		    }

		    if (aix_errblk != (sword32_t) cdda.logical_blk_addr) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: IDE_CDIORDSE blk=%d len=%d\n",
				    (int) cdda.logical_blk_addr,
				    (int) cdda.data_length);
		    }

		    /* If we haven't encountered an error for a while,
		     * then clear the error count.
		     */
		    if (j == 0 && aix_errcnt > 0 &&
			((sword32_t) cdda.logical_blk_addr - aix_errblk) >
			ERR_CLRTHRESH)
			    aix_errcnt = 0;

		    cdda.logical_blk_addr += nframes;
		    cdda.buffer += cdda.data_length;
		    remain_frms -= nframes;
		} while (remain_frms > 0);

		/* Do jitter correction */
		p = cdda_rd_corrjitter(aix_rcd);

		/* Data end */
		end = p + aix_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (aix_rcd->i->jitter) {
			(void) memcpy(
				aix_rcd->cdb->olap,
				end - aix_rcd->cds->search_bytes,
				aix_rcd->cds->search_bytes << 1
			);
		}

		/* Get lock */
		cdda_waitsem(aix_rsemid, LOCK);

		/* Wait until there is room */
		while (aix_rcd->cdb->occupied >= aix_rcd->cds->buffer_chunks &&
		       aix_rcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(aix_rsemid, LOCK);
			cdda_waitsem(aix_rsemid, ROOM);
			cdda_waitsem(aix_rsemid, LOCK);
		}

		/* Break if completed */
		if (aix_rcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(aix_rsemid, LOCK);
			break;
		}

		/* Copy to free location */
		(void) memcpy(
			&aix_rcd->cdb->b[
			    aix_rcd->cds->chunk_bytes * aix_rcd->cdb->nextin
			],
			p,
			aix_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		aix_rcd->cdb->nextin++;
		aix_rcd->cdb->nextin %= aix_rcd->cds->buffer_chunks;
		aix_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		/* Signal data available */
		cdda_postsem(aix_rsemid, DATA);

		/* Update debug level */
		app_data.debug = aix_rcd->i->debug;

		/* Release lock */
		cdda_postsem(aix_rsemid, LOCK);

		/* Set heartbeat and interval */
		if (--hbcnt == 0) {
			cdda_heartbeat(&aix_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = cdda_heartbeat_interval(
				aix_rcd->i->frm_per_sec > 0 ?
					aix_rcd->i->frm_per_sec : FRAME_PER_SEC
			);
		}
	}

	/* Wake up writer */
	cdda_postsem(aix_rsemid, DATA);

	return TRUE;
}


/*
 * aix_rinit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
aix_rinit(void)
{
	return CDDA_READAUDIO;
}


/*
 * aix_read
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
aix_read(di_dev_t *devp)
{
	/* Check configuration */
	if (app_data.di_method != 3 /* DI_AIXIOC */) {
		(void) fprintf(errfp,
			"aix_read: Inconsistent deviceInterfaceMethod "
			"and cddaReadMethod parameters.  Aborting.\n");
		return FALSE;
	}

	/* Allocate memory */
	aix_rcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (aix_rcd == NULL) {
		(void) fprintf(errfp, "aix_read: out of memory\n");
		return FALSE;
	}

	(void) memset(aix_rcd, 0, sizeof(cd_state_t));

	/* Initialize cd pointers to point into shared memory */
	if ((aix_rsemid = cdda_initipc(aix_rcd)) < 0)
		return FALSE;

	DBGPRN(DBG_DEVIO)(errfp,
			  "\naixcd_read: Reading CDDA: "
			  "chunk_blks=%d, olap_blks=%d\n",
			  aix_rcd->cds->chunk_blocks,
			  aix_rcd->cds->olap_blocks);

	/* Fill data buffer */
	if (!aixcd_read_fillbuf(devp))
		return FALSE;

	/* Keep reading */
	if (!aixcd_read_cont(devp))
		return FALSE;

	return TRUE;
}


/*
 * aix_rdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killwriter - whether to terminate the writer thread or process
 *
 * Return:
 *	Nothing.
 */
void
aix_rdone(bool_t killwriter)
{
	struct mode_form_op	m;

	DBGPRN(DBG_DEVIO)(errfp, "\naix_rdone: Cleaning up reader\n");

	if (aix_rcddamode) {
		/* Restore CDROM mode */
		(void) memset(&m, 0, sizeof(m));
		m.action = CD_CHG_MODE;
		m.cd_mode_form = CD_MODE1;
		(void) aixioc_send(DI_ROLE_READER, IDE_CDMODE, &m, TRUE);

		aix_rcddamode = FALSE;
	}

	/* Disable device */
	aixioc_disable(DI_ROLE_READER);

	cdda_yield();

	if (aix_rcd != NULL) {
		if (killwriter && aix_rcd->i != NULL &&
		    aix_rcd->i->writer != (thid_t) 0) {
			cdda_kill(aix_rcd->i->writer, SIGTERM);
			aix_rcd->i->state = CDSTAT_COMPLETED;
		}

		MEM_FREE(aix_rcd);
		aix_rcd = NULL;
	}
}


/*
 * aix_rinfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
aix_rinfo(char *str)
{
	(void) strcat(str, "AIX IDE ioctl\n");
}

#endif	/* CDDA_RD_AIX CDDA_SUPPORTED */

