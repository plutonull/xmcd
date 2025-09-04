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
 *   OSS (Open Sound System) and Linux sound driver support
 *   The Linux ALSA sound driver is also supported if the OSS emulation
 *   module is loaded.
 */
#ifndef lint
static char *_wr_oss_c_ident_ = "@(#)wr_oss.c	7.117 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_OSS) && defined(CDDA_SUPPORTED)

#ifdef USE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#include "cdda_d/soundcard.h"
#endif

#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_oss.h"


extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		oss_wsemid,		/* Semaphores identifier */
			oss_curr_vol0,		/* Current left volume */
			oss_curr_vol1;		/* Current right volume */
STATIC pid_t		oss_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*oss_wcd;		/* Shared memory pointer */
STATIC bool_t		oss_write_dev,		/* Write to audio device */
			oss_write_file,		/* Write to output file */
			oss_write_pipe,		/* Pipe to program */
			oss_mixer_supp = FALSE,	/* OSS PCM mixer supported */
			oss_mixer_2ch = FALSE,	/* OSS PCM mixer is stereo */
			oss_file_be = FALSE;	/* Big endian output file */
STATIC gen_desc_t	*oss_audio_desc = NULL,	/* Audio device desc */
			*oss_mixer_desc = NULL,	/* Mixer device desc */
			*oss_file_desc = NULL,	/* Output file desc */
			*oss_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	oss_file_mode;		/* Output file mode */


/*
 * ossaud_gethw_vol
 *	Get hardware volume control settings and update global variables.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
ossaud_gethw_vol(void)
{
	int	vol0,
		vol1,
		val;

	if (oss_mixer_supp) {
		if (!gen_ioctl(oss_mixer_desc,
			       SOUND_MIXER_READ_PCM, &val)) {
			(void) sprintf(oss_wcd->i->msgbuf,
				    "ossaud_gethw_vol: SOUND_MIXER_READ_PCM "
				    "failed (errno=%d)",
				    errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
			vol0 = 100;
			vol1 = 100;
		}
		else {
			vol0 = (int) (val & 0xff);
			vol1 = (int) ((val >> 8) & 0xff);
		}

		if (vol0 == oss_curr_vol0 && vol1 == oss_curr_vol1)
			/* No change */
			return TRUE;

		oss_curr_vol0 = vol0;
		oss_curr_vol1 = vol1;

		/* Update volume and balance */
		if (vol0 == vol1) {
			oss_wcd->i->vol = util_untaper_vol(vol0);
			oss_wcd->i->vol_left = 100;
			oss_wcd->i->vol_right = 100;
		}
		else if (vol0 > vol1) {
			oss_wcd->i->vol = util_untaper_vol(vol0);
			oss_wcd->i->vol_left = 100;
			oss_wcd->i->vol_right = (vol1 * 100) / vol0;
			val = vol0 >> 1;
			if (((vol1 * 100) % vol0) >= val)
			oss_wcd->i->vol_right++;
		}
		else {
			oss_wcd->i->vol = util_untaper_vol(vol1);
			oss_wcd->i->vol_right = 100;
			oss_wcd->i->vol_left = (vol0 * 100) / vol1;
			val = vol1 >> 1;
			if (((vol0 * 100) % vol1) >= val)
				oss_wcd->i->vol_left++;
		}
	}

	return TRUE;
}


/*
 * ossaud_sethw_vol
 *	Set hardware volume control settings.
 *
 * Args:
 *	vol0 - Left channel volume
 *	vol1 - Right channel volume
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
ossaud_sethw_vol(int vol0, int vol1)
{
	int	val;

	if (oss_mixer_supp) {
		/* PCM mixer setting */
		if (!oss_mixer_2ch) {
			/* Stereo not supported on the PCM mixer channel:
			 * force the two channels to be set the same.
			 */
			if (vol0 > vol1)
				vol1 = vol0;
			else
				vol0 = vol1;
		}

		val = ((vol1 & 0xff) << 8) | (vol0 & 0xff);

		if (!gen_ioctl(oss_mixer_desc,
			       SOUND_MIXER_WRITE_PCM, &val)) {
			(void) sprintf(oss_wcd->i->msgbuf,
				    "ossaud_sethw_vol: SOUND_MIXER_WRITE_PCM "
				    "failed (errno=%d)",
				    errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		}
	}

	return TRUE;
}


/*
 * ossaud_open_dev
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
ossaud_open_dev(void)
{
	char		*cp;
	struct stat	stbuf;

	if (!gen_set_eid(oss_wcd))
		return FALSE;

	/* Use environment variable if set */
	if ((cp = getenv("AUDIODEV")) == NULL)
		cp = DEFAULT_DEV_DSP;

	/* Check to make sure it's a device */
	if (stat(cp, &stbuf) < 0) {
		(void) sprintf(oss_wcd->i->msgbuf,
			    "ossaud_open_dev: stat of %s failed (errno=%d)",
			    cp, errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		(void) gen_reset_eid(oss_wcd);
		return FALSE;
	}
	if (!S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(oss_wcd->i->msgbuf,
			    "ossaud_open_dev: %s is not a character device",
			    cp);
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		(void) gen_reset_eid(oss_wcd);
		return FALSE;
	}

	/* Open audio device */
	oss_audio_desc = gen_open_file(
		oss_wcd, cp, O_WRONLY, 0, FILEFMT_RAW, 0
	);
	if (oss_audio_desc == NULL) {
		(void) sprintf(oss_wcd->i->msgbuf,
			    "ossaud_open_dev: open of %s failed (errno=%d)",
			    cp, errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		(void) gen_reset_eid(oss_wcd);
		return FALSE;
	}

	(void) gen_reset_eid(oss_wcd);
	return TRUE;
}


/*
 * ossaud_close_dev
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
ossaud_close_dev(void)
{
	if (!gen_set_eid(oss_wcd))
		return FALSE;

	/* Close mixer device */
	if (oss_mixer_desc != NULL && oss_mixer_desc != oss_audio_desc) {
		(void) gen_close_file(oss_mixer_desc);
		oss_mixer_desc = NULL;
	}

	/* Close audio device */
	(void) gen_close_file(oss_audio_desc);
	oss_audio_desc = NULL;

	(void) gen_reset_eid(oss_wcd);
	return TRUE;
}


/*
 * ossaud_mixer_init
 *	Detect and set up PCM mixer channel for volume control.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
ossaud_mixer_init(void)
{
	int		val;
	char		*cp;
	struct stat	stbuf;

	/* Use environment variable if set */
	cp = getenv("MIXERDEV");

	/* PCM mixer channel setup
	 * No error message is displayed if any ioctl fails in this
	 * function, because the OSS emulation in Linux ALSA seems to fail
	 * with EINVAL even when the ioctl "worked".
	 */
	val = 0;
	if (cp != NULL ||
	    !gen_ioctl(oss_audio_desc, SOUND_MIXER_READ_DEVMASK, &val)) {
		/* Try /dev/mixer instead.  This is a hack and technically
		 * incorrect, because the OSS API states that there is not
		 * necessarily an association between /dev/dsp and
		 * /dev/mixer devices on some sound cards (i.e., on cards
		 * with multiple DSPs and mixers, mixer0 may not control dsp0).
		 * However, the SOUND_MIXER_READ_DEVMASK ioctl fails on
		 * the ALSA sound driver operating in OSS compatibility mode
		 * when used on the audio device.
		 */

		if (!gen_set_eid(oss_wcd))
			return;

		if (cp == NULL)
			cp = DEFAULT_DEV_MIXER;
			
		/* Check to make sure it's a device */
		if (stat(cp, &stbuf) < 0 || !S_ISCHR(stbuf.st_mode)) {
			(void) gen_reset_eid(oss_wcd);
			return;
		}

		oss_mixer_desc = gen_open_file(
			oss_wcd,
			cp,
			O_RDONLY,
			0,
			FILEFMT_RAW,
			0
		);

		(void) gen_reset_eid(oss_wcd);

		if (oss_mixer_desc != NULL) {
			(void) gen_ioctl(
				oss_mixer_desc,
				SOUND_MIXER_READ_DEVMASK, &val
			);
		}
	}
	else
		oss_mixer_desc = oss_audio_desc;

	if ((val & SOUND_MASK_PCM) != 0) {
		oss_mixer_supp = TRUE;

		val = 0;
		(void) gen_ioctl(oss_mixer_desc,
				 SOUND_MIXER_READ_STEREODEVS, &val);

		if ((val & SOUND_MASK_PCM) != 0)
			oss_mixer_2ch = TRUE;
	}
}


/*
 * ossaud_config
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
ossaud_config(void)
{
	int	val;

	/*
	 * Initialize sound driver and hardware
	 */

#if defined(SNDCTL_DSP_SETFRAGMENT)
	{
		int	n,
			x = oss_wcd->cds->chunk_bytes;

		for (n = 1; x >>= 1; n++)
			;
		if (oss_wcd->cds->chunk_bytes % 2)
			n++;
		if (n < 12)
			n = 12;
		else if (n > 16)
			n = 16;

		val = (0x7fff << 16) | n;
		if (!gen_ioctl(oss_audio_desc,
			       SNDCTL_DSP_SETFRAGMENT, &val)) {
			(void) sprintf(oss_wcd->i->msgbuf,
				    "ossaud_config: "
				    "SNDCTL_DSP_SETFRAGMENT failed (errno=%d)",
				    errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		}
		DBGPRN(DBG_SND)(errfp, "frags=0x%08x\n", val);
	}
#endif

	/* Sample format */
	val = AFMT_S16_LE;
	if (!gen_ioctl(oss_audio_desc, SNDCTL_DSP_SETFMT, &val)) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_config: SNDCTL_DSP_SETFMT failed "
				"(errno=%d)",
				errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}
	if (val != AFMT_S16_LE) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_config: Audio hardware does not "
				"support the required sample format.");
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}

	/* Stereo/mono */
	val = 2;
	if (!gen_ioctl(oss_audio_desc, SNDCTL_DSP_CHANNELS, &val)) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_config: SNDCTL_DSP_CHANNELS failed "
				"(errno=%d)",
				errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}
	if (val != 2) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_config: Audio hardware does not "
				"support stereo mode.");
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}

	/* Sampling rate */
	val = 44100;
	if (!gen_ioctl(oss_audio_desc, SNDCTL_DSP_SPEED, &val)) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_config: SNDCTL_DSP_SPEED failed "
				"(errno=%d)",
				errno);
		return FALSE;
	}
	if (abs(44100 - val) > 2205) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_config: Audio hardware does not "
				"support 44.1kHz sampling rate.");
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}

#ifdef SNDCTL_DSP_GETBLKSIZE
	/* This is informational only */
	val = 0;
	if (!gen_ioctl(oss_audio_desc, SNDCTL_DSP_GETBLKSIZE, &val)) {
		(void) sprintf(oss_wcd->i->msgbuf,
			    "ossaud_config: SNDCTL_DSP_GETBLKSIZE failed "
			    "(errno=%d)",
			    errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", oss_wcd->i->msgbuf);
	}
	DBGPRN(DBG_SND)(errfp, "buffersize=%d\n", val);
#endif

	/* Set up mixer */
	ossaud_mixer_init();

	/* Retrieve current settings */
	oss_curr_vol0 = oss_curr_vol1 = -1;
	if (!ossaud_gethw_vol())
		return FALSE;

	return TRUE;
}


/*
 * ossaud_update
 *	Updates hardware volume and balance settings from GUI settings.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
ossaud_update(void)
{
	int		val,
			vol0,
			vol1;
	static int	curr_vol0 = -1,
			curr_vol1 = -1;

	app_data.vol_taper = oss_wcd->i->vol_taper;

	/* Update volume and balance */
	if (oss_wcd->i->vol_left == oss_wcd->i->vol_right) {
		vol0 = util_taper_vol(oss_wcd->i->vol);
		vol1 = vol0;
	}
	else if (oss_wcd->i->vol_left > oss_wcd->i->vol_right) {
		val = (oss_wcd->i->vol * oss_wcd->i->vol_right) / 100;
		if (((oss_wcd->i->vol * oss_wcd->i->vol_right) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(oss_wcd->i->vol);
		vol1 = util_taper_vol(val);
	}
	else {
		val = (oss_wcd->i->vol * oss_wcd->i->vol_left) / 100;
		if (((oss_wcd->i->vol * oss_wcd->i->vol_left) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(val);
		vol1 = util_taper_vol(oss_wcd->i->vol);
	}

	if (vol0 == curr_vol0 && vol1 == curr_vol1)
		/* No change */
		return;

	curr_vol0 = vol0;
	curr_vol1 = vol1;

	/* Apply new settings */
	(void) ossaud_sethw_vol(vol0, vol1);
}


/*
 * ossaud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
ossaud_status(void)
{
	/* Retrieve current volume settings */
	(void) ossaud_gethw_vol();
}


/*
 * ossaud_write
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
ossaud_write(curstat_t *s)
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
	le_data = (byte_t *) MEM_ALLOC("le_data", oss_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", oss_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(oss_wcd->i->msgbuf,
				"ossaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", oss_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, oss_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, oss_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (oss_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(oss_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (oss_wcd->cdb->occupied == 0 && oss_wcd->i->cdda_done) {
			oss_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(oss_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (oss_wcd->cdb->occupied <= 0 &&
		       oss_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(oss_wsemid, LOCK);
			cdda_waitsem(oss_wsemid, DATA);
			cdda_waitsem(oss_wsemid, LOCK);
		}

		/* Break if stopped */
		if (oss_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(oss_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(oss_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&oss_wcd->cdb->b[
			    oss_wcd->cds->chunk_bytes * oss_wcd->cdb->nextout
			],
			oss_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		oss_wcd->cdb->nextout++;
		oss_wcd->cdb->nextout %= oss_wcd->cds->buffer_chunks;
		oss_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&oss_wcd->i->writer_hb, CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && oss_wcd->i->frm_played > 0) {
				oss_wcd->i->frm_per_sec = (int)
					((float) oss_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				oss_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(oss_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(oss_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(oss_wcd->i->chroute,
				oss_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) oss_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) oss_wcd->cds->chunk_bytes);

		/* Write to device */
		if (oss_write_dev) {
			/* Always feed OSS little endian (AFMT_16_LE) data */
			dev_data = le_data;

			if (!gen_write_chunk(oss_audio_desc, dev_data,
					(size_t) oss_wcd->cds->chunk_bytes)) {
				oss_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(oss_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			ossaud_update();

			if ((oss_wcd->i->frm_played % FRAME_PER_SEC) == 0)
				ossaud_status();
		}

		/* Write to file */
		if (oss_write_file) {
			file_data = oss_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != oss_wcd->i->trk_idx) {
				trk_idx = oss_wcd->i->trk_idx;

				if (oss_file_desc != NULL &&
				    !gen_close_file(oss_file_desc)) {
					oss_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(oss_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				oss_file_desc = gen_open_file(
					oss_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) oss_file_mode,
					app_data.cdda_filefmt,
					oss_wcd->i->trk_len
				);
				if (oss_file_desc == NULL) {
					oss_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(oss_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(oss_file_desc, file_data,
					(size_t) oss_wcd->cds->chunk_bytes)) {
				oss_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(oss_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (oss_write_pipe) {
			pipe_data = oss_file_be ? be_data : le_data;

			if (!gen_write_chunk(oss_pipe_desc, pipe_data,
					(size_t) oss_wcd->cds->chunk_bytes)) {
				oss_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(oss_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		oss_wcd->i->frm_played += oss_wcd->cds->chunk_blocks;

		while (oss_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = oss_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(oss_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * oss_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
oss_winit(void)
{
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
}


/*
 * oss_write
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
oss_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "oss_write: Nothing to do.\n");
		return FALSE;
	}

	oss_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
	oss_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	oss_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	oss_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (oss_wcd == NULL) {
		(void) fprintf(errfp, "oss_write: out of memory\n");
		return FALSE;
	}
	(void) memset(oss_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((oss_wsemid = cdda_initipc(oss_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &oss_file_mode);
	/* Make sure file is at least accessible by user */
	oss_file_mode |= S_IRUSR | S_IWUSR;
	oss_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (oss_wcd->i->end_lba - oss_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&oss_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (oss_write_file || oss_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && oss_write_file) {
			oss_file_desc = gen_open_file(
				oss_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) oss_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (oss_file_desc == NULL)
				return FALSE;
		}

		if (oss_write_pipe) {
			oss_pipe_desc = gen_open_pipe(
				oss_wcd,
				app_data.pipeprog,
				&oss_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (oss_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nossaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  oss_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		oss_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	if (oss_write_dev) {
		/* Open audio device */
		if (!ossaud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!ossaud_config())
			return FALSE;

		/* Initial settings */
		ossaud_update();
	}

	/* Do the writing */
	if (!ossaud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * oss_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
oss_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\noss_wdone: Cleaning up writer\n");

	if (oss_audio_desc != NULL)
		(void) ossaud_close_dev();

	if (oss_file_desc != NULL) {
		(void) gen_close_file(oss_file_desc);
		oss_file_desc = NULL;
	}

	if (oss_pipe_desc != NULL) {
		(void) gen_close_pipe(oss_pipe_desc);
		oss_pipe_desc = NULL;
	}

	cdda_yield();

	if (oss_pipe_pid > 0) {
		(void) kill(oss_pipe_pid, SIGTERM);
		oss_pipe_pid = -1;
	}

	if (oss_wcd != NULL) {
		if (oss_wcd->i != NULL) {
			if (killreader && oss_wcd->i->reader != (thid_t) 0) {
				cdda_kill(oss_wcd->i->reader, SIGTERM);
				oss_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			oss_wcd->i->frm_played = 0;
		}

		MEM_FREE(oss_wcd);
		oss_wcd = NULL;
	}

	oss_write_dev = oss_write_file = oss_write_pipe = FALSE;
}


/*
 * oss_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
oss_winfo(char *str)
{
	(void) strcat(str, "Open Sound System (OSS)\n");
}

#endif	/* CDDA_WR_OSS CDDA_SUPPORTED */

