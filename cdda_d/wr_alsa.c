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
 *   ALSA (Advanced Linux Sound Architecture) sound driver support
 *   (For ALSA 0.9.x and later only)
 */
#ifndef lint
static char *_wr_alsa_c_ident_ = "@(#)wr_alsa.c	7.63 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_ALSA) && defined(CDDA_SUPPORTED)

#include <alsa/asoundlib.h>
#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_alsa.h"


extern appdata_t	app_data;
extern FILE		*errfp;

STATIC snd_pcm_t	*alsa_pcm = NULL;	/* Audio device handle */
STATIC snd_mixer_t	*alsa_mixer = NULL;	/* Mixer device handle */
STATIC snd_mixer_elem_t	*alsa_mixer_elem;	/* PCM mixer element */
STATIC int		alsa_wsemid;		/* Semaphores identifier */
STATIC pid_t		alsa_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*alsa_wcd;		/* Shared memory pointer */
STATIC char		*alsa_pcm_name,		/* PCM device name */
			*alsa_mixer_name;	/* Mixer device name name */
STATIC bool_t		alsa_write_dev,		/* Write to audio device */
			alsa_write_file,	/* Write to output file */
			alsa_write_pipe,	/* Pipe to program */
			alsa_file_be = FALSE,	/* Big endian output file */
			alsa_vol_supp = FALSE,	/* PCM volume supported */
			alsa_bal_supp = FALSE,	/* PCM balance supported */
			alsa_vol_stereo = FALSE;/* PCM volume is stereo */
STATIC long		alsa_vol_max,		/* Max volume */
			alsa_vol_half,		/* Half volume */
			alsa_vol_offset,	/* Volume offset */
			alsa_curr_vol0,		/* Current volume L */
			alsa_curr_vol1;		/* Current volume R */
STATIC gen_desc_t	*alsa_file_desc = NULL,	/* Output file desc */
			*alsa_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	alsa_file_mode;		/* Output file mode */


/*
 * alsaaud_scale_vol
 *	Scale volume from xmcd 0-100 to ALSA audio device values
 *
 * Args:
 *	vol - xmcd volume 0-100
 *
 * Return:
 *	Device volume value
 */
STATIC long
alsaaud_scale_vol(int vol)
{
	int	n;

	n = ((vol * alsa_vol_max) / 100);
	if (((vol * alsa_vol_max) % 100) >= 50)
		n++;

	/* Range checking */
	if (n > alsa_vol_max)
		n = alsa_vol_max;

	return ((long) (n - alsa_vol_offset));
}


/*
 * alsaaud_unscale_vol
 *	Scale volume from ALSA audio device values to xmcd 0-100
 *
 * Args:
 *	vol - device volume value
 *
 * Return:
 *	xmcd volume 0-100
 */
STATIC int
alsaaud_unscale_vol(long vol)
{
	int	n;

	n = ((vol + alsa_vol_offset) * 100) / alsa_vol_max;
	if (((vol * 100) % alsa_vol_max) >= alsa_vol_half)
		n++;

	/* Range checking */
	if (n < 0)
		n = 0;
	else if (n > 100)
		n = 100;

	return (n);
}


/*
 * alsaaud_gethw_vol
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
alsaaud_gethw_vol(void)
{
	int	val;
	long	vol0,
		vol1;

	if (alsa_vol_stereo) {
		if (alsa_bal_supp) {
			snd_mixer_selem_get_playback_volume(
				alsa_mixer_elem,
				SND_MIXER_SCHN_FRONT_LEFT,
				&vol0
			);
			snd_mixer_selem_get_playback_volume(
				alsa_mixer_elem,
				SND_MIXER_SCHN_FRONT_RIGHT,
				&vol1
			);
		}
		else {
			snd_mixer_selem_get_playback_volume(
				alsa_mixer_elem,
				SND_MIXER_SCHN_FRONT_LEFT,
				&vol0
			);
			vol1 = vol0;
		}
	}
	else {
		snd_mixer_selem_get_playback_volume(
			alsa_mixer_elem,
			SND_MIXER_SCHN_MONO,
			&vol0
		);
		vol1 = vol0;
	}

	alsa_curr_vol0 = alsaaud_unscale_vol(vol0);
	alsa_curr_vol1 = alsaaud_unscale_vol(vol1);

	if (alsa_curr_vol0 == alsa_curr_vol1) {
		alsa_wcd->i->vol = util_untaper_vol(alsa_curr_vol0);
		alsa_wcd->i->vol_left = 100;
		alsa_wcd->i->vol_right = 100;
	}
	else if (alsa_curr_vol0 > alsa_curr_vol1) {
		alsa_wcd->i->vol = util_untaper_vol(alsa_curr_vol0);
		alsa_wcd->i->vol_left = 100;
		alsa_wcd->i->vol_right =
			(alsa_curr_vol1 * 100) / alsa_curr_vol0;
		val = alsa_curr_vol0 >> 1;
		if (((alsa_curr_vol1 * 100) % alsa_curr_vol0) >= val)
			alsa_wcd->i->vol_right++;
	}
	else {
		alsa_wcd->i->vol = util_untaper_vol(alsa_curr_vol1);
		alsa_wcd->i->vol_right = 100;
		alsa_wcd->i->vol_left =
			(alsa_curr_vol0 * 100) / alsa_curr_vol1;
		val = alsa_curr_vol1 >> 1;
		if (((alsa_curr_vol0 * 100) % alsa_curr_vol1) >= val)
			alsa_wcd->i->vol_left++;
	}

	return TRUE;
}


/*
 * alsaaud_melem_cb
 *	Callback function for mixer element events
 *
 * Args:
 *	elem - Mixer element handle
 *	mask - Event mask
 *
 * Return:
 *	0      - success
 *	-errno - failure
 */
STATIC int
alsaaud_melem_cb(snd_mixer_elem_t *elem, unsigned int mask)
{
	if (alsa_mixer_elem == NULL || alsa_mixer_elem != elem)
		return 0;

	if (mask & SND_CTL_EVENT_MASK_VALUE) {
		/* Query current hardware volume settings */
		(void) alsaaud_gethw_vol();
	}

	return 0;
}


/*
 * alsaaud_mixer_cb
 *	Callback function for mixer events
 *
 * Args:
 *	mixer - Mixer handle
 *	mask  - Event mask
 *	elem  - Mixer element handle
 *
 * Return:
 *	0      - success
 *	-errno - failure
 */
STATIC int
alsaaud_mixer_cb(snd_mixer_t *mixer, unsigned int mask, snd_mixer_elem_t *elem)
{
	snd_mixer_selem_id_t	*sid;

	if (mask & SND_CTL_EVENT_MASK_ADD) {
		if (snd_mixer_selem_id_malloc(&sid) < 0) {
			(void) sprintf(alsa_wcd->i->msgbuf,
					"alsaaud_mixer_cb: out of memory");
			DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
			return -ENOMEM;
		}

		snd_mixer_selem_get_id(elem, sid);
		snd_mixer_elem_set_callback(elem, alsaaud_melem_cb);
		snd_mixer_selem_id_free(sid);
	}

	return 0;
}


/*
 * alsaaud_open_dev
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
alsaaud_open_dev(void)
{
	snd_mixer_selem_id_t	*sid;
	char			*cp;
	long			vmax,
				vmin;
	int			ret;

	if (!gen_set_eid(alsa_wcd))
		return FALSE;

	if (alsa_pcm_name != NULL) {
		MEM_FREE(alsa_pcm_name);
		alsa_pcm_name = NULL;
	}
	if (alsa_mixer_name != NULL) {
		MEM_FREE(alsa_mixer_name);
		alsa_mixer_name = NULL;
	}

	/* Use environment variables if set */
	if ((cp = getenv("AUDIODEV")) == NULL)
		cp = DEFAULT_PCM_DEV;

	if (!util_newstr(&alsa_pcm_name, cp)) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_open_dev: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		(void) gen_reset_eid(alsa_wcd);
		return FALSE;
	}

	if ((cp = getenv("MIXERDEV")) == NULL)
		cp = DEFAULT_MIXER_DEV;

	if (!util_newstr(&alsa_mixer_name, cp)) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_open_dev: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		(void) gen_reset_eid(alsa_wcd);
		return FALSE;
	}

	/* Open audio device */
	if ((ret = snd_pcm_open(&alsa_pcm, alsa_pcm_name,
				SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_open_dev: open of %s failed:\n%s",
			    alsa_pcm_name, snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		(void) gen_reset_eid(alsa_wcd);
		return FALSE;
	}

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nOpened [%s]\n", alsa_pcm_name);

	/* Check if there is a PCM mixer channel */
	if (snd_mixer_selem_id_malloc(&sid) < 0) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_open_dev: out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	snd_mixer_selem_id_set_name(sid, "PCM");

	if ((snd_mixer_open(&alsa_mixer, 0)) < 0) {
		snd_mixer_selem_id_free(sid);
		DBGPRN(DBG_SND)(errfp,
				"alsaaud_open_dev: Cannot open mixer.\n");
		(void) gen_reset_eid(alsa_wcd);
		return TRUE;	/* Non fatal */
	}

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nOpened [%s]\n", alsa_mixer_name);

	if ((ret = snd_mixer_attach(alsa_mixer, alsa_mixer_name)) < 0) {
		snd_mixer_selem_id_free(sid);
		(void) snd_mixer_close(alsa_mixer);

		DBGPRN(DBG_SND)(errfp,
				"alsaaud_open_dev: "
				"Cannot attach mixer %s:\n%s\n",
				alsa_mixer_name, snd_strerror(ret));
		(void) gen_reset_eid(alsa_wcd);
		return TRUE;	/* Non fatal */
	}

	if ((ret = snd_mixer_selem_register(alsa_mixer, NULL, NULL)) < 0) {
		(void) snd_mixer_detach(alsa_mixer, alsa_mixer_name);
		(void) snd_mixer_close(alsa_mixer);

		DBGPRN(DBG_SND)(errfp,
				"alsaaud_open_dev: "
				"Cannot register mixer element:\n%s\n",
				snd_strerror(ret));
		(void) gen_reset_eid(alsa_wcd);
		return TRUE;	/* Non fatal */
	}

	/* Install callback function */
	snd_mixer_set_callback(alsa_mixer, alsaaud_mixer_cb);

	if ((ret = snd_mixer_load(alsa_mixer)) < 0) {
		snd_mixer_selem_id_free(sid);
		(void) snd_mixer_detach(alsa_mixer, alsa_mixer_name);
		(void) snd_mixer_close(alsa_mixer);

		DBGPRN(DBG_SND)(errfp,
				"alsaaud_open_dev: Cannot load mixer:\n%s\n",
				snd_strerror(ret));
		(void) gen_reset_eid(alsa_wcd);
		return TRUE;	/* Non fatal */
	}

	if ((alsa_mixer_elem = snd_mixer_find_selem(alsa_mixer, sid)) == NULL) {
		snd_mixer_selem_id_free(sid);
		(void) snd_mixer_free(alsa_mixer);
		(void) snd_mixer_detach(alsa_mixer, alsa_mixer_name);
		(void) snd_mixer_close(alsa_mixer);

		DBGPRN(DBG_SND)(errfp,
				"alsaaud_open_dev: "
				"Cannot find PCM mixer channel.\n");
		(void) gen_reset_eid(alsa_wcd);
		return TRUE;	/* Non fatal */
	}	

	snd_mixer_selem_id_free(sid);

	/* Check capabilities */
	if (snd_mixer_selem_has_common_volume(alsa_mixer_elem) ||
	    snd_mixer_selem_has_playback_volume(alsa_mixer_elem))
		alsa_vol_supp = TRUE;

	if (!snd_mixer_selem_has_playback_channel(alsa_mixer_elem,
			SND_MIXER_SCHN_MONO) ||
	    !snd_mixer_selem_is_playback_mono(alsa_mixer_elem))
	    	alsa_vol_stereo = TRUE;

	if (!snd_mixer_selem_has_playback_volume_joined(alsa_mixer_elem))
		alsa_bal_supp = TRUE;

	DBGPRN(DBG_SND)(errfp,
			"\nPCM mixer capabilities: "
			"volume=%d balance=%d stereo=%d\n",
			alsa_vol_supp, alsa_bal_supp, alsa_vol_stereo);

	if (alsa_vol_supp) {
		/* Get volume control range */
		snd_mixer_selem_get_playback_volume_range(
			alsa_mixer_elem, &vmin, &vmax
		);

		DBGPRN(DBG_SND)(errfp, "Volume control range: %ld - %ld\n",
				vmin, vmax);
	}

	alsa_vol_max = vmax - vmin;
	alsa_vol_half = alsa_vol_max >> 1;
	alsa_vol_offset = 0 - vmin;

	(void) gen_reset_eid(alsa_wcd);
	return TRUE;
}


/*
 * alsaaud_close_dev
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
alsaaud_close_dev(void)
{
	if (!gen_set_eid(alsa_wcd))
		return FALSE;

	/* Drain audio */
	(void) snd_pcm_drain(alsa_pcm);

	if (alsa_mixer != NULL) {
		/* Close mixer device */
		(void) snd_mixer_close(alsa_mixer);
		alsa_mixer = NULL;

		DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nClosed [%s]\n",
					alsa_mixer_name);
	}

	/* Close audio device */
	(void) snd_pcm_close(alsa_pcm);
	alsa_pcm = NULL;

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nClosed [%s]\n", alsa_pcm_name);

	(void) gen_reset_eid(alsa_wcd);
	return TRUE;
}


/*
 * alsa_xrun_recovery
 *	Data underrun and suspend recovery function
 *
 * Args:
 *	err - Error code from a failed snd_pcm_writei() call.
 *
 * Return:
 *	error code
 */
STATIC int
alsa_xrun_recovery(int err)
{
	if (err == -EPIPE) {
		/* Under-run */
		if ((err = snd_pcm_prepare(alsa_pcm)) < 0) {
			DBGPRN(DBG_SND)(errfp,
				"alsa_xrun_recovery: prepare failed: %s\n",
				snd_strerror(err));
			return 0;
		}
	}
	else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(alsa_pcm)) == -EAGAIN) {
			/* Wait until suspend is released */
			util_delayms(1000);
		}

		if (err < 0 && (err = snd_pcm_prepare(alsa_pcm)) < 0) {
			DBGPRN(DBG_SND)(errfp,
				"alsa_xrun_recovery: "
				"prepare failed: %s\n",
				snd_strerror(err));
			return 0;
		}
	}

	return (err);
}


/*
 * alsa_write_chunk
 *	Write a chunk of audio data to the audio device.  This function
 *	assumes 16-bit, 2-channel interleaved frames.
 *
 * Args:
 *	data - Pointer to the data buffer
 *	frames - Number of frames to write
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
alsa_write_chunk(sword16_t *data, int frames)
{
	int	ret;

	while (frames > 0) {
		DBGPRN(DBG_SND)(errfp, "\nWrite [%s]: %d bytes\n",
				alsa_pcm_name, frames << 2);

		ret = snd_pcm_writei(alsa_pcm, data, frames);
		if (ret == -EAGAIN)
			continue;
		else if (ret < 0) {
			if (alsa_xrun_recovery(ret) < 0) {
				(void) sprintf(alsa_wcd->i->msgbuf,
					"alsa_write_chunk: write error:\n%s",
					snd_strerror(ret));
				DBGPRN(DBG_SND)(errfp, "%s\n",
					alsa_wcd->i->msgbuf);
				return FALSE;
			}
		}

		data += (ret << 1);
		frames -= ret;
	}

	return TRUE;
}


/*
 * alsaaud_config
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
alsaaud_config(void)
{
	snd_pcm_hw_params_t	*hwparams;
	snd_pcm_sw_params_t	*swparams;
#if (SND_LIB_MAJOR < 1)
	snd_pcm_sframes_t	bufsize,
				persize;
#else
	snd_pcm_uframes_t	bufsize,
				persize;
#endif
	snd_pcm_uframes_t	xferalign,
				startthresh,
				stopthresh;
	snd_output_t		*errout;
	unsigned int		buftime,
				pertime;
	int			rate,
				dir,
				ret;

	if ((ret = snd_output_stdio_attach(&errout, errfp, 0)) < 0) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_config: "
				"Cannot attach ALSA log output");
		DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	/*
	 * Set hardware parameters
	 */
	if (snd_pcm_hw_params_malloc(&hwparams) < 0) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_config: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_hw_params_any(alsa_pcm, hwparams)) < 0) {
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "snd_pcm_hw_params_any failed:\n%s",
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_hw_params_set_access(alsa_pcm, hwparams,
					SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: Unable to set access:\n%s",
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_hw_params_set_format(alsa_pcm, hwparams,
					SND_PCM_FORMAT_S16_LE)) < 0) {
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: Unable to set data format:\n%s",
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_hw_params_set_channels(alsa_pcm, hwparams,
					2)) < 0) {
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: Unable to set 2 channels:\n%s",
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	rate = 44100;
#if (SND_LIB_MAJOR < 1)
	if ((ret = snd_pcm_hw_params_set_rate_near(alsa_pcm, hwparams,
					rate, &dir)) < 0)
#else
	if ((ret = snd_pcm_hw_params_set_rate_near(alsa_pcm, hwparams,
					&rate, &dir)) < 0)
#endif
	{
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set sampling rate to %dHz:\n%s",
			    rate,
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}
#if (SND_LIB_MAJOR < 1)
	else if (ret != rate) {
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Hardware does not support %dHz sampling rate:\n"
			    "Set to %dHz.",
			    rate, ret);
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
	}
#else
	else if (rate != 44100) {
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Hardware does not support %dHz sampling rate:\n"
			    "Set to %dHz.",
			    44100, rate);
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
	}
#endif

	buftime = ALSA_BUFFER_TIME;
#if (SND_LIB_MAJOR < 1)
	if ((ret = snd_pcm_hw_params_set_buffer_time_near(alsa_pcm,
					hwparams, (int) buftime, &dir)) < 0)
#else
	if ((ret = snd_pcm_hw_params_set_buffer_time_near(alsa_pcm,
					hwparams, &buftime, &dir)) < 0)
#endif
	{
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set buffer time to %d:\n%s",
			    ALSA_BUFFER_TIME, snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

#if (SND_LIB_MAJOR < 1)
	bufsize = snd_pcm_hw_params_get_buffer_size(hwparams);
#else
	if ((ret = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize)) < 0){
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to get buffer size:\n%s",
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}
#endif

	pertime = ALSA_PERIOD_TIME;
#if (SND_LIB_MAJOR < 1)
	if ((ret = snd_pcm_hw_params_set_period_time_near(alsa_pcm,
					hwparams, (int) pertime, &dir)) < 0)
#else
	if ((ret = snd_pcm_hw_params_set_period_time_near(alsa_pcm,
					hwparams, &pertime, &dir)) < 0)
#endif
	{
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set period time to %d:\n%s",
			    ALSA_PERIOD_TIME, snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

#if (SND_LIB_MAJOR < 1)
	persize = snd_pcm_hw_params_get_period_size(hwparams, &dir);
#else
	if ((ret = snd_pcm_hw_params_get_period_size(hwparams,
					&persize, &dir)) < 0) {
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to get period size:\n%s",
			    snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}
#endif

	if (persize == bufsize) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Period size cannot be equal to buffer size.");
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_hw_params(alsa_pcm, hwparams)) < 0) {
		snd_pcm_hw_params_free(hwparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set hardware parameters for playback:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		snd_pcm_hw_params_dump(hwparams, errout);
		return FALSE;
	}

	snd_pcm_hw_params_free(hwparams);

	/*
	 * Set software parameters
	 */
	if (snd_pcm_sw_params_malloc(&swparams) < 0) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_config: out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_sw_params_current(alsa_pcm, swparams)) < 0) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to get current software parameters:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_sw_params_get_xfer_align(swparams,
						&xferalign)) < 0) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to get xfer alignment:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	startthresh = (bufsize / xferalign) * xferalign;
	if ((ret = snd_pcm_sw_params_set_start_threshold(alsa_pcm, swparams,
						startthresh)) < 0) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set start threshold for playback:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	stopthresh = bufsize;
	if ((ret = snd_pcm_sw_params_set_stop_threshold(alsa_pcm, swparams,
						stopthresh)) < 0) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set stop threshold for playback:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_sw_params_set_avail_min(alsa_pcm, swparams,
						persize)) < 0) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set avail min for playback:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	if ((ret = snd_pcm_sw_params(alsa_pcm, swparams)) < 0) {
		snd_pcm_sw_params_free(swparams);
		(void) sprintf(alsa_wcd->i->msgbuf,
			    "alsaaud_config: "
			    "Unable to set software parameters for playback:\n"
			    "%s", snd_strerror(ret));
		DBGPRN(DBG_SND)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		snd_pcm_sw_params_dump(swparams, errout);
		return FALSE;
	}

	snd_pcm_sw_params_free(swparams);

	/* Query current hardware volume settings */
	alsa_curr_vol0 = alsa_curr_vol1 = -1;
	(void) alsaaud_gethw_vol();

	if (app_data.debug & DBG_SND)
		snd_pcm_dump(alsa_pcm, errout);

	snd_output_close(errout);

	return TRUE;
}


/*
 * alsaaud_update
 *	Updates hardware volume and balance settings from GUI settings.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
alsaaud_update(void)
{
	int	vol0,
		vol1,
		hvol0,
		hvol1,
		val;

	app_data.vol_taper = alsa_wcd->i->vol_taper;

	if (alsa_wcd->i->vol_left == alsa_wcd->i->vol_right) {
		vol0 = util_taper_vol(alsa_wcd->i->vol);
		vol1 = vol0;
		hvol0 = alsaaud_scale_vol(vol0);
		hvol1 = hvol0;
	}
	else if (alsa_wcd->i->vol_left > alsa_wcd->i->vol_right) {
		val = (alsa_wcd->i->vol * alsa_wcd->i->vol_right) / 100;
		if (((alsa_wcd->i->vol * alsa_wcd->i->vol_right) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(alsa_wcd->i->vol);
		vol1 = util_taper_vol(val);
		hvol0 = alsaaud_scale_vol(vol0);
		hvol1 = alsaaud_scale_vol(vol1);
	}
	else {
		val = (alsa_wcd->i->vol * alsa_wcd->i->vol_left) / 100;
		if (((alsa_wcd->i->vol * alsa_wcd->i->vol_left) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(val);
		vol1 = util_taper_vol(alsa_wcd->i->vol);
		hvol0 = alsaaud_scale_vol(vol0);
		hvol1 = alsaaud_scale_vol(vol1);
	}

	if (vol0 == alsa_curr_vol0 && vol1 == alsa_curr_vol1)
		/* No change */
		return;

	if (alsa_vol_stereo) {
		if (alsa_bal_supp) {
			snd_mixer_selem_set_playback_volume(
				alsa_mixer_elem,
				SND_MIXER_SCHN_FRONT_LEFT,
				hvol0
			);
			snd_mixer_selem_set_playback_volume(
				alsa_mixer_elem,
				SND_MIXER_SCHN_FRONT_RIGHT,
				hvol1
			);
		}
		else {
			snd_mixer_selem_set_playback_volume(
				alsa_mixer_elem,
				SND_MIXER_SCHN_FRONT_LEFT,
				hvol0
			);
		}
	}
	else {
		snd_mixer_selem_set_playback_volume(
			alsa_mixer_elem,
			SND_MIXER_SCHN_MONO,
			hvol0
		);
	}

	alsa_curr_vol0 = vol0;
	alsa_curr_vol1 = vol1;
}


/*
 * alsaaud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
alsaaud_status(void)
{
	int	ret;

	if (!alsa_vol_supp)
		return;

	if ((ret = snd_mixer_handle_events(alsa_mixer)) < 0) {
		DBGPRN(DBG_SND)(errfp,
			"snd_mixer_handle_events failed: %s\n",
			snd_strerror(ret));
	}
}


/*
 * alsaaud_write
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
alsaaud_write(curstat_t *s)
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
	le_data = (byte_t *) MEM_ALLOC("le_data", alsa_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", alsa_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(alsa_wcd->i->msgbuf,
				"alsaaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", alsa_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, alsa_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, alsa_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (alsa_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(alsa_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (alsa_wcd->cdb->occupied == 0 && alsa_wcd->i->cdda_done) {
			alsa_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(alsa_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (alsa_wcd->cdb->occupied <= 0 &&
		       alsa_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(alsa_wsemid, LOCK);
			cdda_waitsem(alsa_wsemid, DATA);
			cdda_waitsem(alsa_wsemid, LOCK);
		}

		/* Break if stopped */
		if (alsa_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(alsa_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(alsa_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&alsa_wcd->cdb->b[
			    alsa_wcd->cds->chunk_bytes * alsa_wcd->cdb->nextout
			],
			alsa_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		alsa_wcd->cdb->nextout++;
		alsa_wcd->cdb->nextout %= alsa_wcd->cds->buffer_chunks;
		alsa_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&alsa_wcd->i->writer_hb,CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && alsa_wcd->i->frm_played > 0) {
				alsa_wcd->i->frm_per_sec = (int)
					((float) alsa_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				alsa_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(alsa_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(alsa_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(alsa_wcd->i->chroute,
				alsa_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) alsa_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) alsa_wcd->cds->chunk_bytes);

		/* Write to device */
		if (alsa_write_dev) {
			/* Always send little endian data */
			dev_data = le_data;

			if (!alsa_write_chunk((sword16_t *)(void *) dev_data,
					alsa_wcd->cds->chunk_bytes >> 2)) {
				alsa_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(alsa_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			alsaaud_update();
			alsaaud_status();
		}

		/* Write to file */
		if (alsa_write_file) {
			file_data = alsa_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != alsa_wcd->i->trk_idx) {
				trk_idx = alsa_wcd->i->trk_idx;

				if (alsa_file_desc != NULL &&
				    !gen_close_file(alsa_file_desc)) {
					alsa_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(alsa_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				alsa_file_desc = gen_open_file(
					alsa_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) alsa_file_mode,
					app_data.cdda_filefmt,
					alsa_wcd->i->trk_len
				);
				if (alsa_file_desc == NULL) {
					alsa_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(alsa_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(alsa_file_desc, file_data,
					(size_t) alsa_wcd->cds->chunk_bytes)) {
				alsa_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(alsa_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (alsa_write_pipe) {
			pipe_data = alsa_file_be ? be_data : le_data;

			if (!gen_write_chunk(alsa_pipe_desc, pipe_data,
					(size_t) alsa_wcd->cds->chunk_bytes)) {
				alsa_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(alsa_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		alsa_wcd->i->frm_played += alsa_wcd->cds->chunk_blocks;

		while (alsa_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = alsa_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(alsa_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * alsa_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
alsa_winit(void)
{
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
}


/*
 * alsa_write
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
alsa_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "alsa_write: Nothing to do.\n");
		return FALSE;
	}

	alsa_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
	alsa_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	alsa_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	alsa_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (alsa_wcd == NULL) {
		(void) fprintf(errfp, "alsa_write: out of memory\n");
		return FALSE;
	}
	(void) memset(alsa_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((alsa_wsemid = cdda_initipc(alsa_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &alsa_file_mode);
	/* Make sure file is at least accessible by user */
	alsa_file_mode |= S_IRUSR | S_IWUSR;
	alsa_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (alsa_wcd->i->end_lba - alsa_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&alsa_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (alsa_write_file || alsa_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && alsa_write_file) {
			alsa_file_desc = gen_open_file(
				alsa_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) alsa_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (alsa_file_desc == NULL)
				return FALSE;
		}

		if (alsa_write_pipe) {
			alsa_pipe_desc = gen_open_pipe(
				alsa_wcd,
				app_data.pipeprog,
				&alsa_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (alsa_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nalsaaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  alsa_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		alsa_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	if (alsa_write_dev) {
		/* Open audio device */
		if (!alsaaud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!alsaaud_config())
			return FALSE;

		/* Initial settings */
		alsaaud_update();
	}

	/* Do the writing */
	if (!alsaaud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * alsa_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
alsa_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\nalsa_wdone: Cleaning up writer\n");

	if (alsa_pcm != NULL)
		(void) alsaaud_close_dev();

	if (alsa_file_desc != NULL) {
		(void) gen_close_file(alsa_file_desc);
		alsa_file_desc = NULL;
	}

	if (alsa_pipe_desc != NULL) {
		(void) gen_close_pipe(alsa_pipe_desc);
		alsa_pipe_desc = NULL;
	}

	cdda_yield();

	if (alsa_pipe_pid > 0) {
		(void) kill(alsa_pipe_pid, SIGTERM);
		alsa_pipe_pid = -1;
	}

	if (alsa_wcd != NULL) {
		if (alsa_wcd->i != NULL) {
			if (killreader && alsa_wcd->i->reader != (thid_t) 0) {
				cdda_kill(alsa_wcd->i->reader, SIGTERM);
				alsa_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			alsa_wcd->i->frm_played = 0;
		}

		MEM_FREE(alsa_wcd);
		alsa_wcd = NULL;
	}

	alsa_write_dev = alsa_write_file = alsa_write_pipe = FALSE;
}


/*
 * alsa_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
alsa_winfo(char *str)
{
	(void) strcat(str, "ALSA sound driver\n");
}

#endif	/* CDDA_WR_ALSA CDDA_SUPPORTED */

