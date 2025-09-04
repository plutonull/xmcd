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
 *   HP-UX audio driver support
 */
#ifndef lint
static char *_wr_hpux_c_ident_ = "@(#)wr_hpux.c	7.66 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_HPUX) && defined(CDDA_SUPPORTED)

#include <sys/audio.h>
#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_hpux.h"


extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		hpux_wsemid,		/* Semaphores identifier */
			hpux_outport = 0,	/* Output port */
			hpux_vol_max,		/* Max volume */
			hpux_vol_half,		/* Half volume */
			hpux_vol_offset,	/* Volume offset */
			hpux_curr_vol0,		/* Previous ch0 volume */
			hpux_curr_vol1;		/* Previous ch1 volume */
STATIC pid_t		hpux_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*hpux_wcd;		/* Shared memory pointer */
STATIC bool_t		hpux_write_dev,		/* Write to audio device */
			hpux_write_file,	/* Write to output file */
			hpux_write_pipe,	/* Pipe to program */
			hpux_file_be = FALSE;	/* Big endian output file */
STATIC gen_desc_t	*hpux_audio_desc = NULL,/* Audio device desc */
			*hpux_file_desc = NULL,	/* Output file desc */
			*hpux_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	hpux_file_mode;		/* Output file mode */


/*
 * hpuxaud_scale_vol
 *	Scale volume from xmcd 0-100 to HP-UX audio device values
 *
 * Args:
 *	vol - xmcd volume 0-100
 *
 * Return:
 *	HP-UX volume value
 */
STATIC int
hpuxaud_scale_vol(int vol)
{
	int	n;

	n = ((vol * hpux_vol_max) / 100);
	if (((vol * hpux_vol_max) % 100) >= 50)
		n++;

	/* Range checking */
	if (n > hpux_vol_max)
		n = hpux_vol_max;

	return (n - hpux_vol_offset);
}


/*
 * hpuxaud_unscale_vol
 *	Scale volume from HP-UX audio device values to xmcd 0-100
 *
 * Args:
 *	vol - HP-UX volume value
 *
 * Return:
 *	xmcd volume 0-100
 */
STATIC int
hpuxaud_unscale_vol(int vol)
{
	int	n;

	n = ((vol + hpux_vol_offset) * 100) / hpux_vol_max;
	if (((vol * 100) % hpux_vol_max) >= hpux_vol_half)
		n++;

	/* Range checking */
	if (n < 0)
		n = 0;
	else if (n > 100)
		n = 100;

	return (n);
}


/*
 * hpuxaud_open_dev
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
hpuxaud_open_dev(void)
{
	char			*cp;
	struct stat		stbuf;

	if (!gen_set_eid(hpux_wcd))
		return FALSE;

	/* Use environment variable if set */
	if ((cp = getenv("AUDIODEV")) == NULL)
		cp = DEFAULT_DEV_AUDIO;

	/* Check to make sure it's a device */
	if (stat(cp, &stbuf) < 0 || !S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
				"hpuxaud_open_dev: "
				"%s is not a character device",
				cp);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		(void) gen_reset_eid(hpux_wcd);
		return FALSE;
	}

	/* Open audio device */
	hpux_audio_desc = gen_open_file(
		hpux_wcd, cp, O_RDWR, 0, FILEFMT_RAW, 0
	);
	if (hpux_audio_desc == NULL) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_open_dev: open of %s failed (errno=%d)",
			    cp, errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		(void) gen_reset_eid(hpux_wcd);
		return FALSE;
	}

	(void) gen_reset_eid(hpux_wcd);
	return TRUE;
}


/*
 * hpuxaud_close_dev
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
hpuxaud_close_dev(void)
{
	if (!gen_set_eid(hpux_wcd))
		return FALSE;

	/* Close audio device */
	(void) gen_close_file(hpux_audio_desc);
	hpux_audio_desc = NULL;

	(void) gen_reset_eid(hpux_wcd);
	return TRUE;
}


/*
 * hpuxaud_gethw_vol
 *	Query hardware volume settings and update structures.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
hpuxaud_gethw_vol(void)
{
	int			vol0,
				vol1,
				val;
	struct audio_gain	again;

	/* Retrieve current gain settings */
	(void) memset(&again, 0, sizeof(again));
	again.channel_mask = AUDIO_CHANNEL_0 | AUDIO_CHANNEL_1;

	if (!gen_ioctl(hpux_audio_desc, AUDIO_GET_GAINS, &again)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_set_info: "
			    "AUDIO_GET_GAINS failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		return FALSE;
	}

	vol0 = hpuxaud_unscale_vol(again.cgain[0].transmit_gain);
	vol1 = hpuxaud_unscale_vol(again.cgain[1].transmit_gain);

	if (vol0 == hpux_curr_vol0 && vol1 == hpux_curr_vol1)
		/* No change */
		return TRUE;

	hpux_curr_vol0 = vol0;
	hpux_curr_vol1 = vol1;

	/* Update volume and balance */
	if (vol0 == vol1) {
		hpux_wcd->i->vol = util_untaper_vol(vol0);
		hpux_wcd->i->vol_left = 100;
		hpux_wcd->i->vol_right = 100;
	}
	else if (vol0 > vol1) {
		hpux_wcd->i->vol = util_untaper_vol(vol0);
		hpux_wcd->i->vol_left = 100;
		hpux_wcd->i->vol_right = (vol1 * 100) / vol0;
		val = vol0 >> 1;
		if (((vol1 * 100) % vol0) >= val)
			hpux_wcd->i->vol_right++;
	}
	else {
		hpux_wcd->i->vol = util_untaper_vol(vol1);
		hpux_wcd->i->vol_right = 100;
		hpux_wcd->i->vol_left =	(vol0 * 100) / vol1;
		val = vol1 >> 1;
		if (((vol0 * 100) % vol1) >= val)
			hpux_wcd->i->vol_left++;
	}

	return TRUE;
}


/*
 * hpuxaud_set_outport
 *	Set CDDA playback output port.
 *
 * Args:
 *	outport - New output port selector bitmap CDDA_OUT_XXX
 *
 * Return:
 *	Nothing.
 */
STATIC void
hpuxaud_set_outport(int outport)
{
	int	val;

	if (outport != 0 && outport != hpux_outport) {
		val = 0;
		if ((outport & CDDA_OUT_SPEAKER) != 0)
			val |= AUDIO_OUT_SPEAKER;
		if ((outport & CDDA_OUT_HEADPHONE) != 0)
			val |= AUDIO_OUT_HEADPHONE;
		if ((outport & CDDA_OUT_LINEOUT) != 0)
			val |= AUDIO_OUT_LINE;

		if (!gen_ioctl(hpux_audio_desc, AUDIO_SET_OUTPUT,
			       (void *) val)) {
			(void) sprintf(hpux_wcd->i->msgbuf,
				    "hpuxaud_set_outport: "
				    "AUDIO_SET_OUTPUT (0x%x) "
				    "failed (errno=%d)",
				    val, errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		}

		hpux_outport = outport;
	}
}


/*
 * hpuxaud_config
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
hpuxaud_config(void)
{
	struct audio_limits	alimits;
	struct audio_describe	adesc;

	/* Set data format */
	if (!gen_ioctl(hpux_audio_desc, AUDIO_SET_DATA_FORMAT,
		       (void *) AUDIO_FORMAT_LINEAR16BIT)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_config: "
			    "AUDIO_SET_DATA_FORMAT failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		return FALSE;
	}

	/* Set number of channels */
	if (!gen_ioctl(hpux_audio_desc, AUDIO_SET_CHANNELS, (void *) 2)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_config: "
			    "AUDIO_SET_CHANNELS failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		return FALSE;
	}

	/* Set sampling rate */
	if (!gen_ioctl(hpux_audio_desc, AUDIO_SET_SAMPLE_RATE,
		       (void *) 44100)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_config: "
			    "AUDIO_SET_SAMPLE_RATE failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get volume control range */
	(void) memset(&adesc, 0, sizeof(adesc));
	if (!gen_ioctl(hpux_audio_desc, AUDIO_DESCRIBE, &adesc)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_set_info: "
			    "AUDIO_GET_GAINS failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);

		hpux_vol_max = HPUX_MAX_VOL;
		hpux_vol_half = HPUX_HALF_VOL;
		hpux_vol_offset = HPUX_VOL_OFFSET;
	}
	else {
		hpux_vol_max =
			adesc.max_transmit_gain - adesc.min_transmit_gain;
		hpux_vol_half = hpux_vol_max >> 1;
		hpux_vol_offset = 0 - adesc.min_transmit_gain;
	}

	/* Set audio output port */
	hpuxaud_set_outport(hpux_wcd->i->outport);

	/* Check buffer sizes - informational only */
	(void) memset(&alimits, 0, sizeof(alimits));
	if (!gen_ioctl(hpux_audio_desc, AUDIO_GET_LIMITS, &alimits)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_config: "
			    "AUDIO_GET_LIMITS failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
	}
	else {
		DBGPRN(DBG_SND)(errfp, "recv_bufs=%d xmit_bufs=%d\n",
				  alimits.max_receive_buffer_size,
				  alimits.max_transmit_buffer_size);
	}

	/* Query current volume settings */
	hpux_curr_vol0 = hpux_curr_vol1 = -1;
	if (!hpuxaud_gethw_vol())
		return FALSE;

	return TRUE;
}


/*
 * hpuxaud_update
 *	Updates hardware volume and balance settings from GUI settings.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
hpuxaud_update(void)
{
	int			vol0,
				vol1,
				val;
	struct audio_gain	again;
	static int		curr_vol0 = -1,
				curr_vol1 = -1;

	app_data.vol_taper = hpux_wcd->i->vol_taper;

	/* Set audio output port */
	hpuxaud_set_outport(hpux_wcd->i->outport);

	(void) memset(&again, 0, sizeof(again));

	if (hpux_wcd->i->vol_left == hpux_wcd->i->vol_right) {
		vol0 = util_taper_vol(hpux_wcd->i->vol);
		vol1 = vol0;
		again.cgain[0].transmit_gain = hpuxaud_scale_vol(vol0);
		again.cgain[1].transmit_gain = again.cgain[0].transmit_gain;
	}
	else if (hpux_wcd->i->vol_left > hpux_wcd->i->vol_right) {
		val = (hpux_wcd->i->vol * hpux_wcd->i->vol_right) / 100;
		if (((hpux_wcd->i->vol * hpux_wcd->i->vol_right) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(hpux_wcd->i->vol);
		vol1 = util_taper_vol(val);
		again.cgain[0].transmit_gain = hpuxaud_scale_vol(vol0);
		again.cgain[1].transmit_gain = hpuxaud_scale_vol(vol1);
	}
	else {
		val = (hpux_wcd->i->vol * hpux_wcd->i->vol_left) / 100;
		if (((hpux_wcd->i->vol * hpux_wcd->i->vol_left) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(val);
		vol1 = util_taper_vol(hpux_wcd->i->vol);
		again.cgain[0].transmit_gain = hpuxaud_scale_vol(vol0);
		again.cgain[1].transmit_gain = hpuxaud_scale_vol(vol1);
	}

	if (vol0 == curr_vol0 && vol1 == curr_vol1)
		/* No change */
		return;

	curr_vol0 = vol0;
	curr_vol1 = vol1;

	/* Apply new settings */
	again.channel_mask = AUDIO_CHANNEL_0 | AUDIO_CHANNEL_1;

	if (!gen_ioctl(hpux_audio_desc, AUDIO_SET_GAINS, &again)) {
		(void) sprintf(hpux_wcd->i->msgbuf,
			    "hpuxaud_set_info: "
			    "AUDIO_SET_GAINS failed (errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", hpux_wcd->i->msgbuf);
	}
}


/*
 * hpuxaud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
hpuxaud_status(void)
{
	(void) hpuxaud_gethw_vol();
}


/*
 * hpuxaud_write
 *	Function responsible for writing from the buffer to the audio
 *	device (and file if saving).
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
hpuxaud_write(curstat_t *s)
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
	le_data = (byte_t *) MEM_ALLOC("le_data", hpux_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(hpux_wcd->i->msgbuf,
				"hpuxaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", hpux_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(hpux_wcd->i->msgbuf,
				"hpuxaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", hpux_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, hpux_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, hpux_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (hpux_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(hpux_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (hpux_wcd->cdb->occupied == 0 && hpux_wcd->i->cdda_done) {
			hpux_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(hpux_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (hpux_wcd->cdb->occupied <= 0 &&
		       hpux_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(hpux_wsemid, LOCK);
			cdda_waitsem(hpux_wsemid, DATA);
			cdda_waitsem(hpux_wsemid, LOCK);
		}

		/* Break if stopped */
		if (hpux_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(hpux_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(hpux_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&hpux_wcd->cdb->b[
			    hpux_wcd->cds->chunk_bytes * hpux_wcd->cdb->nextout
			],
			hpux_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		hpux_wcd->cdb->nextout++;
		hpux_wcd->cdb->nextout %= hpux_wcd->cds->buffer_chunks;
		hpux_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&hpux_wcd->i->writer_hb,CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && hpux_wcd->i->frm_played > 0) {
				hpux_wcd->i->frm_per_sec = (int)
					((float) hpux_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				hpux_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(hpux_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(hpux_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(hpux_wcd->i->chroute,
				hpux_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) hpux_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) hpux_wcd->cds->chunk_bytes);

		/* Write to device */
		if (hpux_write_dev) {
			/* Always feed HP-UX audio driver big endian data */
			dev_data = be_data;

			if (!gen_write_chunk(hpux_audio_desc, dev_data,
					(size_t) hpux_wcd->cds->chunk_bytes)) {
				hpux_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(hpux_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			hpuxaud_update();

			if ((hpux_wcd->i->frm_played % FRAME_PER_SEC) == 0)
				hpuxaud_status();
		}

		/* Write to file */
		if (hpux_write_file) {
			file_data = hpux_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != hpux_wcd->i->trk_idx) {
				trk_idx = hpux_wcd->i->trk_idx;

				if (hpux_file_desc != NULL &&
				    !gen_close_file(hpux_file_desc)) {
					hpux_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(hpux_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				hpux_file_desc = gen_open_file(
					hpux_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) hpux_file_mode,
					app_data.cdda_filefmt,
					hpux_wcd->i->trk_len
				);
				if (hpux_file_desc == NULL) {
					hpux_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(hpux_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(hpux_file_desc, file_data,
					(size_t) hpux_wcd->cds->chunk_bytes)) {
				hpux_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(hpux_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (hpux_write_pipe) {
			pipe_data = hpux_file_be ? be_data : le_data;

			if (!gen_write_chunk(hpux_pipe_desc, pipe_data,
					(size_t) hpux_wcd->cds->chunk_bytes)) {
				hpux_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(hpux_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		hpux_wcd->i->frm_played += hpux_wcd->cds->chunk_blocks;

		while (hpux_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = hpux_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(hpux_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * hpux_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
hpux_winit(void)
{
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
}


/*
 * hpux_write
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
hpux_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "hpux_write: Nothing to do.\n");
		return FALSE;
	}

	hpux_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
	hpux_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	hpux_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	hpux_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (hpux_wcd == NULL) {
		(void) fprintf(errfp, "hpux_write: out of memory\n");
		return FALSE;
	}
	(void) memset(hpux_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((hpux_wsemid = cdda_initipc(hpux_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &hpux_file_mode);
	/* Make sure file is at least accessible by user */
	hpux_file_mode |= S_IRUSR | S_IWUSR;
	hpux_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (hpux_wcd->i->end_lba - hpux_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&hpux_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (hpux_write_file || hpux_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && hpux_write_file) {
			hpux_file_desc = gen_open_file(
				hpux_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) hpux_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (hpux_file_desc == NULL)
				return FALSE;
		}

		if (hpux_write_pipe) {
			hpux_pipe_desc = gen_open_pipe(
				hpux_wcd,
				app_data.pipeprog,
				&hpux_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (hpux_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nhpuxaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  hpux_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		hpux_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	if (hpux_write_dev) {
		/* Open audio device */
		if (!hpuxaud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!hpuxaud_config())
			return FALSE;

		/* Initial settings */
		hpuxaud_update();
	}

	/* Do the writing */
	if (!hpuxaud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * hpux_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
hpux_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\nhpux_wdone: Cleaning up writer\n");

	if (hpux_audio_desc != NULL)
		(void) hpuxaud_close_dev();

	if (hpux_file_desc != NULL) {
		(void) gen_close_file(hpux_file_desc);
		hpux_file_desc = NULL;
	}

	if (hpux_pipe_desc != NULL) {
		(void) gen_close_pipe(hpux_pipe_desc);
		hpux_pipe_desc = NULL;
	}

	cdda_yield();

	if (hpux_pipe_pid > 0) {
		(void) kill(hpux_pipe_pid, SIGTERM);
		hpux_pipe_pid = -1;
	}

	if (hpux_wcd != NULL) {
		if (hpux_wcd->i != NULL) {
			if (killreader && hpux_wcd->i->reader != (thid_t) 0) {
				cdda_kill(hpux_wcd->i->reader, SIGTERM);
				hpux_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			hpux_wcd->i->frm_played = 0;
		}

		MEM_FREE(hpux_wcd);
		hpux_wcd = NULL;
	}

	hpux_write_dev = hpux_write_file = hpux_write_pipe = FALSE;
}


/*
 * hpux_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
hpux_winfo(char *str)
{
	(void) strcat(str, "HP-UX audio driver\n");
}

#endif	/* CDDA_WR_HPUX CDDA_SUPPORTED */

