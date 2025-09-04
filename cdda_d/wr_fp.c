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
 *   File and pipe only (no audio playback) write method
 */
#ifndef lint
static char *_wr_fp_c_ident_ = "@(#)wr_fp.c	7.77 04/02/02";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_FP) && defined(CDDA_SUPPORTED)

#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_fp.h"


extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		fl_wsemid;		/* Semaphores identifier */
STATIC pid_t		fl_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*fl_wcd;		/* Shared memory pointer */
STATIC bool_t		fl_write_file,		/* Write to output file */
			fl_write_pipe,		/* Pipe to program */
			fl_file_be = FALSE;	/* Big endian output file */
STATIC gen_desc_t	*fl_file_desc = NULL,	/* Output file desc */
			*fl_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	fl_file_mode;		/* Output file mode */


/*
 * fpaud_write
 *	Function responsible for writing from the buffer to the audio
 *	file.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
fpaud_write(curstat_t *s)
{
	byte_t	*in_data,
		*sw_data,
		*le_data,
		*be_data,
		*file_data,
		*pipe_data;
	char	*fname = NULL;
	int	trk_idx = -1,
		hbcnt = 1;
	time_t	start_time,
		tot_time;
	bool_t	ret;

	/* Get memory for audio write buffer */
	le_data = (byte_t *) MEM_ALLOC("le_data", fl_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(fl_wcd->i->msgbuf,
				"fpaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", fl_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", fl_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(fl_wcd->i->msgbuf,
				"fpaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", fl_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, fl_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, fl_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (fl_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(fl_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (fl_wcd->cdb->occupied == 0 && fl_wcd->i->cdda_done) {
			fl_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(fl_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (fl_wcd->cdb->occupied <= 0 &&
		       fl_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(fl_wsemid, LOCK);
			cdda_waitsem(fl_wsemid, DATA);
			cdda_waitsem(fl_wsemid, LOCK);
		}

		/* Break if stopped */
		if (fl_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(fl_wsemid, LOCK);
			break;
		}

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&fl_wcd->cdb->b[
			    fl_wcd->cds->chunk_bytes * fl_wcd->cdb->nextout
			],
			fl_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		fl_wcd->cdb->nextout++;
		fl_wcd->cdb->nextout %= fl_wcd->cds->buffer_chunks;
		fl_wcd->cdb->occupied--;


		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&fl_wcd->i->writer_hb, CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && fl_wcd->i->frm_played > 0) {
				fl_wcd->i->frm_per_sec = (int)
					((float) fl_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				fl_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(fl_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(fl_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(fl_wcd->i->chroute,
				fl_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) fl_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) fl_wcd->cds->chunk_bytes);

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(fl_wcd);

		/* Write to file */
		if (fl_write_file) {
			file_data = fl_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != fl_wcd->i->trk_idx) {
				trk_idx = fl_wcd->i->trk_idx;

				if (fl_file_desc != NULL &&
				    !gen_close_file(fl_file_desc)) {
					fl_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(fl_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				fl_file_desc = gen_open_file(
					fl_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) fl_file_mode,
					app_data.cdda_filefmt,
					fl_wcd->i->trk_len
				);
				if (fl_file_desc == NULL) {
					fl_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(fl_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(fl_file_desc, file_data,
					(size_t) fl_wcd->cds->chunk_bytes)) {
				fl_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(fl_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (fl_write_pipe) {
			pipe_data = fl_file_be ? be_data : le_data;

			if (!gen_write_chunk(fl_pipe_desc, pipe_data,
					(size_t) fl_wcd->cds->chunk_bytes)) {
				fl_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(fl_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		fl_wcd->i->frm_played += fl_wcd->cds->chunk_blocks;

		while (fl_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = fl_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(fl_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * fp_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
fp_winit(void)
{
	return (CDDA_WRITEFILE | CDDA_WRITEPIPE);
}


/*
 * fp_write
 *	Opens audio file, attaches shared memory and semaphores.
 *	Continuously reads data from shared memory and writes it
 *	to the audio file.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
fp_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "fl_write: Nothing to do.\n");
		return FALSE;
	}

	fl_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	fl_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	fl_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (fl_wcd == NULL) {
		(void) fprintf(errfp, "fl_write: out of memory\n");
		return FALSE;
	}
	(void) memset(fl_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((fl_wsemid = cdda_initipc(fl_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &fl_file_mode);
	/* Make sure file is at least accessible by user */
	fl_file_mode |= S_IRUSR | S_IWUSR;
	fl_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (fl_wcd->i->end_lba - fl_wcd->i->start_lba + 1) * CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&fl_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (fl_write_file || fl_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && fl_write_file) {
			fl_file_desc = gen_open_file(
				fl_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) fl_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (fl_file_desc == NULL)
				return FALSE;
		}

		if (fl_write_pipe) {
			fl_pipe_desc = gen_open_pipe(
				fl_wcd,
				app_data.pipeprog,
				&fl_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (fl_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nfpaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  fl_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		fl_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	/* Do the writing */
	if (!fpaud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * fp_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
fp_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\nfp_wdone: Cleaning up writer\n");

	if (fl_file_desc != NULL) {
		(void) gen_close_file(fl_file_desc);
		fl_file_desc = NULL;
	}

	if (fl_pipe_desc != NULL) {
		(void) gen_close_pipe(fl_pipe_desc);
		fl_pipe_desc = NULL;
	}

	cdda_yield();

	if (fl_pipe_pid > 0) {
		(void) kill(fl_pipe_pid, SIGTERM);
		fl_pipe_pid = -1;
	}

	if (fl_wcd != NULL) {
		if (fl_wcd->i != NULL) {
			if (killreader && fl_wcd->i->reader != (thid_t) 0) {
				cdda_kill(fl_wcd->i->reader, SIGTERM);
				fl_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			fl_wcd->i->frm_played = 0;
		}

		MEM_FREE(fl_wcd);
		fl_wcd = NULL;
	}

	fl_write_file = fl_write_pipe = FALSE;
}


/*
 * fp_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
fp_winfo(char *str)
{
	(void) strcat(str, "file & pipe only\n");
}

#endif	/* CDDA_WR_FP CDDA_SUPPORTED */

