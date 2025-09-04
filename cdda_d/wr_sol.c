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
static char *_wr_sol_c_ident_ = "@(#)wr_sol.c	7.166 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_SOL) && defined(CDDA_SUPPORTED)

#include <stropts.h>
#include <sys/stream.h>
#include <sys/audioio.h>
#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_sol.h"

extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		sol_wsemid,		/* Semaphores identifier */
			sol_outport = 0;	/* Output port */
STATIC pid_t		sol_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*sol_wcd;		/* Shared memory pointer */
STATIC bool_t		sol_write_dev,		/* Write to audio device */
			sol_write_file,		/* Write to output file */
			sol_write_pipe,		/* Pipe to program */
			sol_file_be = FALSE;	/* Big endian output file */
STATIC gen_desc_t	*sol_audio_desc = NULL,	/* Audio device desc */
			*sol_file_desc = NULL,	/* Output file desc */
			*sol_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	sol_file_mode,		/* Output file mode */
			sol_curr_vol,		/* Current volume */
			sol_curr_bal;		/* Current balance */


/*
 * solaud_scale_vol
 *	Scale volume from xmcd 0-100 to Solaris audio device 0-SOL_MAX_VOL
 *
 * Args:
 *	vol - xmcd volume 0-100
 *
 * Return:
 *	Solaris volume 0-SOL_MAX_VOL
 */
STATIC unsigned int
solaud_scale_vol(int vol)
{
	unsigned int	ret;

	ret = (vol * SOL_MAX_VOL) / 100;
	if (((vol * SOL_MAX_VOL) % 100) >= 50)
		ret++;

	/* Range checking */
	if (ret > SOL_MAX_VOL)
		ret = SOL_MAX_VOL;

	return (ret);
}


/*
 * solaud_unscale_vol
 *	Scale volume from Solaris audio device 0-SOL_MAX_VOL to xmcd 0-100
 *
 * Args:
 *	vol - Solaris volume 0-SOL_MAX_VOL
 *
 * Return:
 *	xmcd volume 0-100
 */
STATIC int
solaud_unscale_vol(unsigned int vol)
{
	int	ret;

	ret = (vol * 100) / SOL_MAX_VOL;
	if (((vol * 100) % SOL_MAX_VOL) >= SOL_HALF_VOL)
		ret++;

	/* Range checking */
	if (ret < 0)
		ret = 0;
	else if (ret > 100)
		ret = 100;

	return (ret);
}


/*
 * solaud_scale_bal
 *	Convert xmcd individual channel volume percentage values to
 *	Solaris audio balance value.
 *
 * Args:
 *	vol_left  - left volume percentage
 *	vol_right - right volume percentage
 *
 * Return:
 *	Solaris balance SOL_L_BAL-SOL_R_BAL
 */
STATIC uchar_t
solaud_scale_bal(int vol_left, int vol_right)
{
	uchar_t	ret = SOL_M_BAL;

	if (vol_left == 100 && vol_right == 100) {
		ret = SOL_M_BAL;
	}
	else if (vol_right < 100) {
		ret = (vol_right * SOL_M_BAL) / 100;
		if (((vol_right * SOL_M_BAL) % 100) >= 50)
			ret++;
	}
	else if (vol_left < 100) {
		ret = ((vol_left * SOL_M_BAL) / 100) +
			SOL_M_BAL;
		if (((vol_left * SOL_M_BAL) % 100) >= 50)
			ret++;
	}

	/* Range checking */
	if (ret > SOL_R_BAL)
		ret = SOL_R_BAL;

	return (ret);
}


/*
 * solaud_unscale_bal
 *	Convert Solaris audio balance value to xmcd individual channel
 *	volume percentage values.
 *
 * Args:
 *	bal       - Solaris audio balance value
 *	vol_left  - left volume percentage (return)
 *	vol_right - right volume percentage (return)
 *
 * Return:
 *	Nothing.
 */
STATIC void
solaud_unscale_bal(uchar_t bal, int *vol_left, int *vol_right)
{
	if (bal > SOL_R_BAL)
		bal = SOL_R_BAL;

	if (bal == SOL_M_BAL) {
		*vol_left = 100;
		*vol_right = 100;
	}
	else if (bal < SOL_M_BAL) {
		*vol_left = 100;
		*vol_right = (bal * 100) / SOL_M_BAL;
		if (((bal * 100) % SOL_M_BAL) >= SOL_HALF_VOL)
			(*vol_right)++;
	}
	else if (bal > SOL_M_BAL) {
		bal -= SOL_M_BAL;
		*vol_left = (bal * 100) / SOL_M_BAL;
		*vol_right = 100;
		if (((bal * 100) % SOL_M_BAL) >= SOL_HALF_VOL)
			(*vol_left)++;
	}

	/* Range checking */
	if (*vol_left < 0)
		*vol_left = 0;
	else if (*vol_left > 100)
		*vol_left = 100;

	if (*vol_right < 0)
		*vol_right = 0;
	else if (*vol_right > 100)
		*vol_right = 100;
}


/*
 * solaud_get_info
 *	Get audio hardware settings.
 *
 * Args:
 *	info - Info to return
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_get_info(audio_info_t *info)
{
	/* Retrieve settings */
	if (!gen_ioctl(sol_audio_desc, AUDIO_GETINFO, info)) {
		(void) sprintf(sol_wcd->i->msgbuf,
			"solaud_get_info: AUDIO_GETINFO failed: %s",
			strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", sol_wcd->i->msgbuf);
		return FALSE;
	}

	return TRUE;
}


/*
 * solaud_set_info
 *	Set audio hardware settings.
 *
 * Args:
 *	info - Info to set
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_set_info(audio_info_t *info)
{
	/* Apply settings */
	if (!gen_ioctl(sol_audio_desc, AUDIO_SETINFO, info)) {
		(void) sprintf(sol_wcd->i->msgbuf,
			"solaud_set_info: AUDIO_SETINFO failed: %s",
			strerror(errno));
		return FALSE;
	}

	return TRUE;
}


/*
 * solaud_flush
 *	Wrapper for STREAMS I_FLUSH ioctl.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_flush(gen_desc_t *gdp)
{
	/* Apply settings */
	if (!gen_ioctl(gdp, I_FLUSH, (void *) FLUSHRW)) {
		(void) sprintf(sol_wcd->i->msgbuf,
			"solaud_flush: I_FLUSH failed: %s",
			strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", sol_wcd->i->msgbuf);
		return FALSE;
	}

	return TRUE;
}


/*
 * solaud_drain
 *	Wrapper for AUDIO_DRAIN ioctl.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_drain(gen_desc_t *gdp)
{
	/* Apply settings */
	if (!gen_ioctl(gdp, AUDIO_DRAIN, NULL)) {
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_drain: AUDIO_DRAIN failed: %s",
				strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", sol_wcd->i->msgbuf);
		return FALSE;
	}

	return TRUE;
}


/*
 * solaud_open_dev
 *	Opens the audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_open_dev(void)
{
	char		*cp;
	struct stat	stbuf;

	if (!gen_set_eid(sol_wcd))
		return FALSE;

	/* Use environment variable if set */
	if ((cp = getenv("AUDIODEV")) == NULL)
		cp = DEFAULT_DEV_AUDIO;

	/* Check to make sure it's a device */
	if (stat(cp, &stbuf) < 0) {
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_open_dev: stat of %s failed: %s",
				cp, strerror(errno));
		DBGPRN(DBG_GEN)(errfp, "%s\n", sol_wcd->i->msgbuf);
		(void) gen_reset_eid(sol_wcd);
		return FALSE;
	}
	if (!S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_open_dev: %s is not a "
				"character device",
				cp);
		DBGPRN(DBG_SND)(errfp, "%s\n", sol_wcd->i->msgbuf);
		(void) gen_reset_eid(sol_wcd);
		return FALSE;
	}

	/* Open audio device */
	if ((sol_audio_desc = gen_open_file(sol_wcd, cp, O_WRONLY, 0,
					    FILEFMT_RAW, 0)) == NULL) {
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_open_dev: open of %s failed: %s",
				cp, strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", sol_wcd->i->msgbuf);
		(void) gen_reset_eid(sol_wcd);
		return FALSE;
	}

	(void) gen_reset_eid(sol_wcd);
	return TRUE;
}


/*
 * solaud_close_dev
 *	Closes the audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_close_dev(void)
{
	if (!gen_set_eid(sol_wcd))
		return FALSE;

	/* Close audio device */
	if (!gen_close_file(sol_audio_desc)) {
		DBGPRN(DBG_SND)(errfp,
			"solaud_close_dev: close failed: %s\n",
			strerror(errno));
		(void) gen_reset_eid(sol_wcd);
		return FALSE;
	}

	sol_audio_desc = NULL;

	(void) gen_reset_eid(sol_wcd);
	return TRUE;
}


/*
 * solaud_config
 *	Sets up audio device for playing CD quality audio.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_config(void)
{
	audio_info_t	solaud_info;

	/* Initialize */
	SOL_AUDIO_INIT(&solaud_info, sizeof(audio_info_t));

	solaud_info.play.sample_rate = 44100;
	solaud_info.play.channels = 2;
	solaud_info.play.precision = 16;
	solaud_info.play.encoding = AUDIO_ENCODING_LINEAR;

	/* Select audio output port */
	if (sol_wcd->i->outport != 0 && sol_wcd->i->outport != sol_outport) {
		solaud_info.play.port = 0;
		if ((sol_wcd->i->outport & CDDA_OUT_SPEAKER) != 0)
			solaud_info.play.port |= AUDIO_SPEAKER;
		if ((sol_wcd->i->outport & CDDA_OUT_HEADPHONE) != 0)
			solaud_info.play.port |= AUDIO_HEADPHONE;
		if ((sol_wcd->i->outport & CDDA_OUT_LINEOUT) != 0)
			solaud_info.play.port |= AUDIO_LINE_OUT;

		sol_outport = sol_wcd->i->outport;
	}

	/* Apply new settings */
	if (!solaud_set_info(&solaud_info))
		return FALSE;

	/* Retrieve current settings */
	if (!solaud_get_info(&solaud_info))
		return FALSE;

	sol_curr_vol = sol_curr_bal = -1;

	/* Update volume */
	sol_wcd->i->vol = util_untaper_vol(
		solaud_unscale_vol(solaud_info.play.gain)
	);

	/* Update balance */
	solaud_unscale_bal(solaud_info.play.balance,
			   &sol_wcd->i->vol_left, &sol_wcd->i->vol_right);

	return TRUE;
}


/*
 * solaud_write_eof
 *	Puts an eof marker in the audio stream.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_write_eof(gen_desc_t *gdp)
{
	if (!gen_set_eid(sol_wcd))
		return FALSE;

	if (!gen_write_chunk(gdp, NULL, 0)) {
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_write_eof: write failed: %s",
				strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", sol_wcd->i->msgbuf);

		(void) gen_reset_eid(sol_wcd);
		return FALSE;
	}

	(void) gen_reset_eid(sol_wcd);
	return TRUE;
}


/*
 * solaud_set_paused
 *	Update Solaris audio driver paused state based on current status.
 *
 * Args:
 *	state - The current paused state
 *
 * Return:
 *	The current paused state
 */
STATIC int
solaud_paused(int state)
{
	audio_info_t	solaud_info;
	static int	prev_state = CDSTAT_NOSTATUS;

	if (prev_state == state)
		return (state);	/* No change */

	if (sol_write_dev) {
		if (!solaud_get_info(&solaud_info))
			return (state);	/* Can't get info, just return */

		/* Update paused */
		solaud_info.play.pause = (state == CDSTAT_PAUSED) ? 1 : 0;

		/* Apply new settings */
		(void) solaud_set_info(&solaud_info);
	}

	prev_state = state;
	return (state);
}


/*
 * solaud_update
 *	Updates hardware volume and balance settings from GUI.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
solaud_update(void)
{
	audio_info_t	solaud_info;
	int		vol,
			bal;
	static int	curr_vol = -1,
			curr_bal = -1;

	app_data.vol_taper = sol_wcd->i->vol_taper;

	vol = solaud_scale_vol(util_taper_vol(sol_wcd->i->vol));
	bal = solaud_scale_bal(sol_wcd->i->vol_left, sol_wcd->i->vol_right);

	if (vol == curr_vol && bal == curr_bal &&
	    (sol_wcd->i->outport == 0 || sol_wcd->i->outport == sol_outport))
		/* No change */
		return;

	/* Retrieve current settings */
	if (!solaud_get_info(&solaud_info))
		return;

	/* Select audio output port */
	if (sol_wcd->i->outport != 0 && sol_wcd->i->outport != sol_outport) {
		solaud_info.play.port = 0;
		if ((sol_wcd->i->outport & CDDA_OUT_SPEAKER) != 0)
			solaud_info.play.port |= AUDIO_SPEAKER;
		if ((sol_wcd->i->outport & CDDA_OUT_HEADPHONE) != 0)
			solaud_info.play.port |= AUDIO_HEADPHONE;
		if ((sol_wcd->i->outport & CDDA_OUT_LINEOUT) != 0)
			solaud_info.play.port |= AUDIO_LINE_OUT;

		sol_outport = sol_wcd->i->outport;
	}

	curr_vol = vol;
	curr_bal = bal;

	/* Update volume and balance */
	solaud_info.play.gain = (unsigned int) vol;
	solaud_info.play.balance = (unsigned int) bal;

	/* Apply new settings */
	(void) solaud_set_info(&solaud_info);
}


/*
 * solaud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
solaud_status(void)
{
	audio_info_t	solaud_info;

	/* Retrieve current settings */
	if (!solaud_get_info(&solaud_info))
		return;

	if (solaud_info.play.gain == sol_curr_vol &&
	    solaud_info.play.balance == sol_curr_bal)
		/* No change */
		return;

	sol_curr_vol = solaud_info.play.gain;
	sol_curr_bal = solaud_info.play.balance;

	/* Update volume and balance */
	sol_wcd->i->vol = util_untaper_vol(solaud_unscale_vol(sol_curr_vol));
	solaud_unscale_bal(sol_curr_bal,
			   &sol_wcd->i->vol_left, &sol_wcd->i->vol_right);
}


/*
 * solaud_write
 *	Function responsible for writing from the buffer to the audio
 *	device (and file if saving). Each chunk is followed by an eof
 *	marker.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
solaud_write(curstat_t *s)
{
	byte_t	*in_data,
		*sw_data,
		*le_data,
		*be_data,
		*dev_data,
		*file_data,
		*pipe_data;
	char	*fname = NULL;
	int	trk_idx = -1,
		hbcnt = 1;
	time_t	start_time,
		tot_time;
	bool_t	ret;

	/* Get memory for audio device write buffer */
	le_data = (byte_t *) MEM_ALLOC("le_data", sol_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", sol_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", sol_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(sol_wcd->i->msgbuf,
				"solaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", sol_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, sol_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, sol_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (sol_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(sol_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (sol_wcd->cdb->occupied == 0 && sol_wcd->i->cdda_done) {
			if (sol_write_dev && !solaud_drain(sol_audio_desc)) {
				ret = FALSE;
				break;
			}
			sol_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(sol_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (sol_wcd->cdb->occupied <= 0 &&
		       sol_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(sol_wsemid, LOCK);
			cdda_waitsem(sol_wsemid, DATA);
			cdda_waitsem(sol_wsemid, LOCK);
		}

		/* Break if stopped */
		if (sol_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(sol_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(sol_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&sol_wcd->cdb->b[
			    sol_wcd->cds->chunk_bytes * sol_wcd->cdb->nextout
			],
			sol_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		sol_wcd->cdb->nextout++;
		sol_wcd->cdb->nextout %= sol_wcd->cds->buffer_chunks;
		sol_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&sol_wcd->i->writer_hb, CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && sol_wcd->i->frm_played > 0) {
				sol_wcd->i->frm_per_sec = (int)
					((float) sol_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				sol_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(sol_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(sol_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(sol_wcd->i->chroute,
				sol_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) sol_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) sol_wcd->cds->chunk_bytes);

		/* Write to device */
		if (sol_write_dev) {
			/* Feed native endian data to the audio driver */
#if _BYTE_ORDER_ == _B_ENDIAN_
			dev_data = be_data;
#else
			dev_data = le_data;
#endif

			if (!gen_write_chunk(sol_audio_desc, dev_data,
					(size_t) sol_wcd->cds->chunk_bytes)) {
				sol_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(sol_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			if (!solaud_write_eof(sol_audio_desc)) {
				sol_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(sol_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			solaud_update();

			if ((sol_wcd->i->frm_played % FRAME_PER_SEC) == 0)
				solaud_status();
		}

		/* Write to file */
		if (sol_write_file) {
			file_data = sol_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != sol_wcd->i->trk_idx) {
				trk_idx = sol_wcd->i->trk_idx;

				if (sol_file_desc != NULL &&
				    !gen_close_file(sol_file_desc)) {
					sol_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(sol_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				sol_file_desc = gen_open_file(
					sol_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) sol_file_mode,
					app_data.cdda_filefmt,
					sol_wcd->i->trk_len
				);
				if (sol_file_desc == NULL) {
					sol_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(sol_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(sol_file_desc, file_data,
					(size_t) sol_wcd->cds->chunk_bytes)) {
				sol_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(sol_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (sol_write_pipe) {
			pipe_data = sol_file_be ? be_data : le_data;

			if (!gen_write_chunk(sol_pipe_desc, pipe_data,
					(size_t) sol_wcd->cds->chunk_bytes)) {
				sol_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(sol_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		sol_wcd->i->frm_played += sol_wcd->cds->chunk_blocks;

		while (solaud_paused(sol_wcd->i->state) == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = sol_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(sol_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * sol_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
sol_winit(void)
{
#if defined(i386) || defined(__i386__)
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
#else
	int	arch = ((gethostid() >> 24) & ARCH_MASK);

	switch (arch) {
	case ARCH_SUN2:
	case ARCH_SUN3:
	case ARCH_SUN4C:
	case ARCH_SUN4E:
		DBGPRN(DBG_SND)(errfp, "sol_winit: %s\n%s\n",
			"CD-quality stereo playback is not supported on",
			"this machine architecture.");
		return (CDDA_WRITEFILE | CDDA_WRITEPIPE);

	case ARCH_SUN4M:
	case ARCH_SUNX:
	default:
		return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
	}
#endif
}


/*
 * sol_write
 *	Opens audio device/file, attaches shared memory and semaphores.
 *	Continuously reads data from shared memory and writes it
 *	to the audio device/file.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
sol_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "sol_write: Nothing to do.\n");
		return FALSE;
	}

	sol_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
	sol_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	sol_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	sol_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (sol_wcd == NULL) {
		(void) fprintf(errfp, "sol_write: out of memory\n");
		return FALSE;
	}
	(void) memset(sol_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((sol_wsemid = cdda_initipc(sol_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &sol_file_mode);
	/* Make sure file is at least accessible by user */
	sol_file_mode |= S_IRUSR | S_IWUSR;
	sol_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (sol_wcd->i->end_lba - sol_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&sol_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (sol_write_file || sol_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && sol_write_file) {
			sol_file_desc = gen_open_file(
				sol_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) sol_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (sol_file_desc == NULL)
				return FALSE;
		}

		if (sol_write_pipe) {
			sol_pipe_desc = gen_open_pipe(
				sol_wcd,
				app_data.pipeprog,
				&sol_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (sol_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nsolaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  sol_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		sol_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	if (sol_write_dev) {
		/* Open audio device */
		if (!solaud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!solaud_config())
			return FALSE;

		/* Initial settings */
		solaud_update();
	}

	/* Do the writing */
	if (!solaud_write(s))
		return FALSE;

	/* Flush audio */
	if (sol_write_dev && !solaud_flush(sol_audio_desc))
		return FALSE;

	return TRUE;
}


/*
 * sol_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
sol_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\nsol_wdone: Cleaning up writer\n");

	if (sol_audio_desc != NULL)
		(void) solaud_close_dev();

	if (sol_file_desc != NULL) {
		(void) gen_close_file(sol_file_desc);
		sol_file_desc = NULL;
	}

	if (sol_pipe_desc != NULL) {
		(void) gen_close_pipe(sol_pipe_desc);
		sol_pipe_desc = NULL;
	}

	cdda_yield();

	if (sol_pipe_pid > 0) {
		(void) kill(sol_pipe_pid, SIGTERM);
		sol_pipe_pid = -1;
	}

	if (sol_wcd != NULL) {
		if (sol_wcd->i != NULL) {
			if (killreader && sol_wcd->i->reader != (thid_t) 0) {
				cdda_kill(sol_wcd->i->reader, SIGTERM);
				sol_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			sol_wcd->i->frm_played = 0;
		}

		MEM_FREE(sol_wcd);
		sol_wcd = NULL;
	}

	sol_write_dev = sol_write_file = sol_write_pipe = FALSE;
}


/*
 * sol_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
sol_winfo(char *str)
{
	(void) strcat(str, "Solaris audio driver\n");
}

#endif	/* CDDA_WR_SOL CDDA_SUPPORTED */

