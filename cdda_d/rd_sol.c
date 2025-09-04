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
 *   Sun Solaris support
 *
 *   This software fragment contains code that interfaces the
 *   application to the Solaris operating environment.  The name "Sun"
 *   and "Solaris" are used here for identification purposes only.
 *
 *   Contributing author: Darragh O'Brien <darragh.obrien@sun.com>
 */
#ifndef lint
static char *_rd_sol_c_ident_ = "@(#)rd_sol.c	7.90 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_RD_SOL) && defined(CDDA_SUPPORTED)

#include <stropts.h>
#include <sys/cdio.h>
#include "libdi_d/slioc.h"
#include "cdda_d/rd_sol.h"

extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		sol_rsemid,		/* Semaphores identifier */
			sol_errcnt = 0;		/* Read error count */
STATIC sword32_t	sol_errblk = -1;	/* Block addr of read error */
STATIC cd_state_t	*sol_rcd;		/* CD state pointer */


/*
 * solcd_read_fillbuf
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
solcd_read_fillbuf(di_dev_t *devp)
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
	struct cdrom_cdda	cdda;

	/* Set heartbeat and interval */
	cdda_heartbeat(&sol_rcd->i->reader_hb, CDDA_HB_READER);
	hbcnt = hbint = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* Get lock */
	cdda_waitsem(sol_rsemid, LOCK);

	/* Enable device */
	slioc_enable(DI_ROLE_READER);

	/* Initialize buffer location */
	start = &sol_rcd->cdb->data[sol_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = sol_rcd->i->start_lba;

	(void) memset(&cdda, 0, sizeof(cdda));
	cdda.cdda_subcode = CDROM_DA_NO_SUBCODE;

	/* Fill up our buffer */
	i = 0;

	while (i < sol_rcd->cds->buffer_chunks && !sol_rcd->i->cdda_done) {
		remain_frms =
			sol_rcd->cds->chunk_blocks + sol_rcd->cds->olap_blocks;

		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * sol_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + sol_rcd->cds->chunk_blocks) >=
		    sol_rcd->i->end_lba) {
			remain_frms = sol_rcd->i->end_lba - lba;
			(void) memset(
				sol_rcd->cdb->data,
				0,
				sol_rcd->cds->chunk_bytes +
				(sol_rcd->cds->olap_bytes << 1)
			);
			sol_rcd->i->cdda_done = 1;
		}

		cdda.cdda_addr = lba;
		cdda.cdda_data = (caddr_t) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.cdda_length = ((nbytes < SOL_CDDA_MAXFER) ?
				    nbytes : SOL_CDDA_MAXFER) / CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !slioc_send(DI_ROLE_READER, CDROMCDDA,
				     &cdda, sizeof(cdda), TRUE, NULL);
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
			if (++sol_errcnt < MAX_RECOVERR) {
			    sol_errblk = (sword32_t) cdda.cdda_addr;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) sol_errblk);
			    break;
			}

			cdda_postsem(sol_rsemid, LOCK);
			return FALSE;
		    }

		    if (sol_errblk != (sword32_t) cdda.cdda_addr) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: CDROMCDDA blk=%d len=%d\n",
				    (int) cdda.cdda_addr,
				    (int) cdda.cdda_length);
		    }

		    cdda.cdda_addr += cdda.cdda_length;
		    cdda.cdda_data += (cdda.cdda_length * CDDA_BLKSZ);
		    remain_frms -= cdda.cdda_length;
		} while (remain_frms > 0);

		/* Do jitter correction */
		if (i == 0)
			p = &sol_rcd->cdb->data[sol_rcd->cds->olap_bytes];
		else
			p = cdda_rd_corrjitter(sol_rcd);

		/* Data end */
		end = p + sol_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (sol_rcd->i->jitter) {
			(void) memcpy(
				sol_rcd->cdb->olap,
				end - sol_rcd->cds->search_bytes,
				sol_rcd->cds->search_bytes << 1
			);
		}

		/* Copy in */
		(void) memcpy(
			&sol_rcd->cdb->b[
			    sol_rcd->cds->chunk_bytes * sol_rcd->cdb->nextin
			],
			p,
			sol_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		sol_rcd->cdb->nextin++;
		sol_rcd->cdb->nextin %= sol_rcd->cds->buffer_chunks;
		sol_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		i++;

		/* Update debug level */
		app_data.debug = sol_rcd->i->debug;

		/* Set heartbeat */
		if (--hbcnt == 0) {
			cdda_heartbeat(&sol_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = hbint;
		}
	}

	/* No room available now */
	cdda_waitsem(sol_rsemid, ROOM);

	/* Signal data available */
	cdda_postsem(sol_rsemid, DATA);

	/* Release lock */
	cdda_postsem(sol_rsemid, LOCK);

	return TRUE;
}


/*
 * solcd_read_cont
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
solcd_read_cont(di_dev_t *devp)
{
	byte_t			*start,
				*end,
				*p;
	int			j,
				hbcnt,
				lba,
				nbytes,
				remain_frms;
	struct cdrom_cdda	cdda;

	/* Initialize */
	sol_rcd->i->writer_hb_timeout = app_data.hb_timeout;
	start = &sol_rcd->cdb->data[sol_rcd->cds->olap_bytes];

	/* Set up for read */
	lba = sol_rcd->i->start_lba + sol_rcd->cds->buffer_blocks;

	(void) memset(&cdda, 0, sizeof(cdda));
	cdda.cdda_subcode = CDROM_DA_NO_SUBCODE;

	/* Set heartbeat interval */
	hbcnt = cdda_heartbeat_interval(FRAME_PER_SEC);

	/* While not stopped or finished */
	while (sol_rcd->i->state != CDSTAT_COMPLETED &&
	       !sol_rcd->i->cdda_done) {
		remain_frms =
			sol_rcd->cds->chunk_blocks + sol_rcd->cds->olap_blocks;
		/*
		 * If we are passing the end of the play segment we have to
		 * take special action. We read in up to end_lba and zero out
		 * the buffer. This is equivalent to rounding to the nearest
		 * sol_rcd->cds->chunk_bytes. We also set the cdda_done flag.
		 */
		if ((lba + sol_rcd->cds->chunk_blocks) >=
		    sol_rcd->i->end_lba) {
			remain_frms = sol_rcd->i->end_lba - lba;
			(void) memset(
				sol_rcd->cdb->data,
				0,
				sol_rcd->cds->chunk_bytes +
				(sol_rcd->cds->olap_bytes << 1)
			);
			sol_rcd->i->cdda_done = 1;
		}

		cdda.cdda_addr = lba;
		cdda.cdda_data = (caddr_t) start;

		do {
		    nbytes = remain_frms * CDDA_BLKSZ;
		    cdda.cdda_length = ((nbytes < SOL_CDDA_MAXFER) ?
				    nbytes : SOL_CDDA_MAXFER) / CDDA_BLKSZ;

		    /* Read audio from CD using ioctl method */
		    for (j = 0;
			 !slioc_send(DI_ROLE_READER, CDROMCDDA,
				     &cdda, sizeof(cdda), TRUE, NULL);
			 j++) {
			(void) fprintf(errfp,
			    "\nIOCTL: CDROMCDDA failed: %s\n", strerror(errno)
			);

			if (j < MAX_CDDAREAD_RETRIES) {
			    util_delayms(200);
			    (void) fprintf(errfp, "Retrying...\n");
			    continue;
			}

			/* Skip to the next chunk if within error
			 * recovery threshold.
			 */
			if (++sol_errcnt < MAX_RECOVERR) {
			    sol_errblk = (sword32_t) cdda.cdda_addr;
			    (void) fprintf(errfp,
					"Skipping chunk addr=0x%x.\n",
					(int) sol_errblk);
			    break;
			}

			return FALSE;
		    }

		    if (sol_errblk != (sword32_t) cdda.cdda_addr) {
			DBGPRN(DBG_DEVIO)(errfp,
				    "\nIOCTL: CDROMCDDA blk=%d len=%d\n",
				    (int) cdda.cdda_addr,
				    (int) cdda.cdda_length);
		    }

		    /* If we haven't encountered an error for a while,
		     * then clear the error count.
		     */
		    if (j == 0 && sol_errcnt > 0 &&
			((sword32_t) cdda.cdda_addr - sol_errblk) >
			ERR_CLRTHRESH)
			    sol_errcnt = 0;

		    cdda.cdda_addr += cdda.cdda_length;
		    cdda.cdda_data += (cdda.cdda_length * CDDA_BLKSZ);
		    remain_frms -= cdda.cdda_length;
		} while (remain_frms > 0);

		/* Do jitter correction */
		p = cdda_rd_corrjitter(sol_rcd);

		/* Data end */
		end = p + sol_rcd->cds->chunk_bytes;

		/* Reinitialize cd_olap */
		if (sol_rcd->i->jitter) {
			(void) memcpy(
				sol_rcd->cdb->olap,
				end - sol_rcd->cds->search_bytes,
				sol_rcd->cds->search_bytes << 1
			);
		}

		/* Get lock */
		cdda_waitsem(sol_rsemid, LOCK);

		/* Wait until there is room */
		while (sol_rcd->cdb->occupied >= sol_rcd->cds->buffer_chunks &&
		       sol_rcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(sol_rsemid, LOCK);
			cdda_waitsem(sol_rsemid, ROOM);
			cdda_waitsem(sol_rsemid, LOCK);
		}

		/* Break if completed */
		if (sol_rcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(sol_rsemid, LOCK);
			break;
		}

		/* Copy to free location */
		(void) memcpy(
			&sol_rcd->cdb->b[
			    sol_rcd->cds->chunk_bytes * sol_rcd->cdb->nextin
			],
			p,
			sol_rcd->cds->chunk_bytes
		);

		/* Update pointers */
		sol_rcd->cdb->nextin++;
		sol_rcd->cdb->nextin %= sol_rcd->cds->buffer_chunks;
		sol_rcd->cdb->occupied++;

		lba += ((end - start) / CDDA_BLKSZ);
		/* Check if we need to round up */
		if (((end - start) % CDDA_BLKSZ) >= (CDDA_BLKSZ >> 1))
			lba++;

		/* Signal data available */
		cdda_postsem(sol_rsemid, DATA);

		/* Release lock */
		cdda_postsem(sol_rsemid, LOCK);

		/* Update debug level */
		app_data.debug = sol_rcd->i->debug;

		/* Set heartbeat and interval */
		if (--hbcnt == 0) {
			cdda_heartbeat(&sol_rcd->i->reader_hb, CDDA_HB_READER);
			hbcnt = cdda_heartbeat_interval(
				sol_rcd->i->frm_per_sec > 0 ?
					sol_rcd->i->frm_per_sec : FRAME_PER_SEC
			);
		}
	}

	/* Wake up writer */
	cdda_postsem(sol_rsemid, DATA);

	return TRUE;
}


/*
 * sol_rinit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
sol_rinit(void)
{
	return CDDA_READAUDIO;
}


/*
 * sol_read
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
sol_read(di_dev_t *devp)
{
	/* Check configuration */
	if (app_data.di_method != 1 /* DI_SLIOC */) {
		(void) fprintf(errfp,
			"sol_read: Inconsistent deviceInterfaceMethod "
			"and cddaReadMethod parameters.  Aborting.\n");
		return FALSE;
	}

	/* Allocate memory */
	sol_rcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (sol_rcd == NULL) {
		(void) fprintf(errfp, "sol_read: out of memory\n");
		return FALSE;
	}

	(void) memset(sol_rcd, 0, sizeof(cd_state_t));

	/* Initialize cd pointers to point into shared memory */
	if ((sol_rsemid = cdda_initipc(sol_rcd)) < 0)
		return FALSE;

	DBGPRN(DBG_DEVIO)(errfp,
			  "\nsolcd_read: Reading CDDA: "
			  "chunk_blks=%d, olap_blks=%d\n",
			  sol_rcd->cds->chunk_blocks,
			  sol_rcd->cds->olap_blocks);

	/* Fill data buffer */
	if (!solcd_read_fillbuf(devp))
		return FALSE;

	/* Keep reading */
	if (!solcd_read_cont(devp))
		return FALSE;

	return TRUE;
}


/*
 * sol_rdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killwriter - whether to terminate the writer thread or process
 *
 * Return:
 *	Nothing.
 */
void
sol_rdone(bool_t killwriter)
{
	DBGPRN(DBG_DEVIO)(errfp, "\nsol_rdone: Cleaning up reader\n");

	/* Disable device */
	slioc_disable(DI_ROLE_READER);

	cdda_yield();

	if (sol_rcd != NULL) {
		if (killwriter && sol_rcd->i != NULL &&
		    sol_rcd->i->writer != (thid_t) 0) {
			cdda_kill(sol_rcd->i->writer, SIGTERM);
			sol_rcd->i->state = CDSTAT_COMPLETED;
		}

		MEM_FREE(sol_rcd);
		sol_rcd = NULL;
	}
}


/*
 * sol_rinfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
sol_rinfo(char *str)
{
	(void) strcat(str, "Solaris ioctl\n");
}

#endif	/* CDDA_RD_SOL CDDA_SUPPORTED */

