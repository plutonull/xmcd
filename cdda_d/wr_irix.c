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
 *   SGI IRIX sound driver support
 *
 *   Volume and balance control capability originally added by
 *   Andrea Suatoni <a.suatoni@telefonica.net> and modified by Ti Kan
 */
#ifndef lint
static char *_wr_irix_c_ident_ = "@(#)wr_irix.c	7.49 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_IRIX) && defined(CDDA_SUPPORTED)

#include <math.h>
#include <dmedia/audio.h>
#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_irix.h"


#define OUT_RESNAME	".AnalogOut"

extern appdata_t	app_data;
extern FILE		*errfp;

STATIC ALconfig		irix_config;		/* Audio config */
STATIC ALport		irix_port = NULL;	/* Audio port */
STATIC int		irix_wsemid;		/* Semaphores identifier */
STATIC pid_t		irix_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*irix_wcd;		/* Shared memory pointer */
STATIC bool_t		irix_write_dev,		/* Write to audio device */
			irix_write_file,	/* Write to output file */
			irix_write_pipe,	/* Pipe to program */
			irix_file_be = FALSE,	/* Big endian output file */
			irix_has_ninf;          /* Support of -inf value */
STATIC gen_desc_t	*irix_file_desc = NULL,	/* Output file desc */
			*irix_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	irix_file_mode;		/* Output file mode */

STATIC al_hwvol_t	irix_vol_step = (al_hwvol_t) -1,
						/* Volume stepping */
			irix_min_vol,		/* Minimum volume value */
			irix_max_vol,		/* Maximum volume value */
			irix_curr_vol0,		/* Current left gain */
			irix_curr_vol1;		/* Current right gain */


#ifndef NEW_IRIX_AUDIO_API

/* Audio library error message strings - to support old audio library
 * interface.
 */
STATIC char		*irix_errs[] = {
    /* AL_BAD_NOT_IMPLEMENTED */  "Not implemented yet",
    /* AL_BAD_PORT */		  "Tried to use an invalid port",
    /* AL_BAD_CONFIG */		  "Tried to use an invalid configuration",
    /* AL_BAD_DEVICE */		  "Tried to use an invalid device",
    /* AL_BAD_DEVICE_ACCESS */	  "Unable to access the device",
    /* AL_BAD_DIRECTION */	  "Illegal direction given for port",
    /* AL_BAD_OUT_OF_MEM */	  "Operation has run out of memory",
    /* AL_BAD_NO_PORTS */	  "Not able to allocate a port",
    /* AL_BAD_WIDTH */		  "Invalid sample width given",
    /* AL_BAD_ILLEGAL_STATE */	  "An illegal state has occurred",
    /* AL_BAD_QSIZE */		  "Attempt to set an invalid queue size",
    /* AL_BAD_FILLPOINT */	  "Attempt to set an invalid fillpoint",
    /* AL_BAD_BUFFER_NULL */	  "Null buffer pointer",
    /* AL_BAD_COUNT_NEG */	  "Negative count",
    /* AL_BAD_PVBUFFER */	  "Param/val buffer doesn't make sense",
    /* AL_BAD_BUFFERLENGTH_NEG */ "Negative buffer length",
    /* AL_BAD_BUFFERLENGTH_ODD */ "Odd length parameter/value buffer",
    /* AL_BAD_CHANNELS */	  "Invalid channel specifier",
    /* AL_BAD_PARAM */		  "Invalid parameter",
    /* AL_BAD_SAMPFMT */	  "Attempt to set invalid sample format",
    /* AL_BAD_RATE */		  "Invalid sample rate token",
    /* AL_BAD_TRANSFER_SIZE */	  "Invalid size for sample read/write",
    /* AL_BAD_FLOATMAX */	  "Invalid size for floatmax",
    /* AL_BAD_PORTSTYLE */	  "Invalid port style"
};


/*
 * alGetErrorString
 *	Old IRIX audio library compatibility function.
 *	Return a text string describing an error specified by the code.
 *
 * Args:
 *	code - The error code
 *
 * Return:
 *	Error description text string.
 */
STATIC char *
alGetErrorString(int code)
{
	char	buf[STR_BUF_SZ];

	if (code >= 0 && code < (sizeof(irix_errs) / sizeof(char *)))
		return (irix_errs[code]);

	(void) sprintf(buf, "Unknown error %d", code);
	return (buf);
}
#endif	/* NEW_IRIX_AUDIO_API */


/*
 * irixaud_gethw_vol
 *	Retrieve IRIX audio device volumes and convert to xmcd
 *	individual channel volume percentage values.
 *
 * Args:
 *	audiodev - Audio device id
 *
 * Return:
 *	Nothing.
 */
STATIC void
irixaud_gethw_vol(al_device_t audiodev)
{
	al_hwvol_t	range,
			vol0,
			vol1;
#ifdef NEW_IRIX_AUDIO_API
	al_fixed_t	gain[2];
	al_param_t	p[1];
	al_hwvol_t	vol_step;

	/* Get the current gain values */
	p[0].param = AL_GAIN;
	p[0].value.ptr = gain;
	p[0].sizeIn = 2;
	if (al_get_params(audiodev, p, 1) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_gethw_vol: Cannot get volume:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return;
	}

	/* Update volume and balance */
	if (irix_has_ninf && gain[0] == AL_NEG_INFINITY)
		vol0 = irix_min_vol;
	else
		vol0 = alFixedToDouble(gain[0]);

	if (irix_has_ninf && gain[1] == AL_NEG_INFINITY)
		vol1 = irix_min_vol;
	else
		vol1 = alFixedToDouble(gain[1]);

	/* This is a hack to work around the fact that the IRIX audio
	 * device control resolution is very poor at low settings.
	 */
	if ((irix_curr_vol0 - irix_min_vol) < 15.0 ||
	    (irix_curr_vol1 - irix_min_vol) < 15.0)
		vol_step = 10.0;
	else
		vol_step = irix_vol_step;

	if (fabs(irix_curr_vol0 - vol0) < vol_step &&
	    fabs(irix_curr_vol1 - vol1) < vol_step)
		/* No change */
		return;

	irix_curr_vol0 = vol0;
	irix_curr_vol1 = vol1;

#else
	al_param_t	p[4];

	p[0] = AL_LEFT_SPEAKER_GAIN;
	p[2] = AL_RIGHT_SPEAKER_GAIN;
	if (al_get_params(audiodev, p, 4) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_gethw_vol: Cannot get volume:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return;
	}

	vol0 = p[1];
	vol1 = p[3];

	if (irix_curr_vol0 == vol0 && irix_curr_vol1 == vol1)
		/* No change */
		return;

	irix_curr_vol0 = vol0;
	irix_curr_vol1 = vol1;

#endif	/* NEW_IRIX_AUDIO_API */

	range = irix_max_vol - irix_min_vol;

	/* Update volume and balance */
	if (vol0 == vol1) {
		irix_wcd->i->vol = util_untaper_vol(
			(int) (100 * (vol0 - irix_min_vol) / range)
		);
		irix_wcd->i->vol_left = irix_wcd->i->vol_right = 100;
	}
	else if (vol0 > vol1) {
		irix_wcd->i->vol = util_untaper_vol(
			(int) (100 * (vol0 - irix_min_vol) / range)
		);
		irix_wcd->i->vol_left = 100;
		if ((vol1 - irix_min_vol) >= irix_vol_step) {
		    irix_wcd->i->vol_right = (int)
		      (100 * (vol1 - irix_min_vol) / (vol0 - irix_min_vol));
		}
	}
	else {
		irix_wcd->i->vol = util_untaper_vol(
			(int) (100 * (vol1 - irix_min_vol) / range)
		);
		if ((vol0 - irix_min_vol) >= irix_vol_step) {
		    irix_wcd->i->vol_left = (int)
		      (100 * (vol0 - irix_min_vol) / (vol1 - irix_min_vol));
		}
		irix_wcd->i->vol_right = 100;
	}

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "irixaud_gethw_vol: "
#ifdef NEW_IRIX_AUDIO_API
		"xmcd_vol=(%d/%d/%d) hw_gain=(%lf/%lf)\n",
#else
		"xmcd_vol=(%d/%d/%d) hw_gain=(%ld/%ld)\n",
#endif
		irix_wcd->i->vol,
		irix_wcd->i->vol_left,
		irix_wcd->i->vol_right,
		vol0, vol1);
}


/*
 * irixaud_sethw_vol
 *	Set IRIX audio device volumes
 *
 * Args:
 *	audiodev - Audio device id
 *	vol0 - Channel 0 volume
 *	vol1 - Channel 1 volume
 *
 * Return:
 *	Nothing.
 */
STATIC void
irixaud_sethw_vol(al_device_t audiodev, al_hwvol_t vol0, al_hwvol_t vol1)
{
#ifdef NEW_IRIX_AUDIO_API
	al_fixed_t	gain[2];
	al_param_t	p[1];

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "irixaud_sethw_vol: "
		"xmcd_vol=(%d/%d/%d) hw_gain=(%lf/%lf)\n",
		irix_wcd->i->vol,
		irix_wcd->i->vol_left,
		irix_wcd->i->vol_right,
		vol0, vol1);

	if (vol0 == irix_min_vol && irix_has_ninf)
		gain[0] = AL_NEG_INFINITY;
	else
		gain[0] = alDoubleToFixed(vol0);

	if (vol1 == irix_min_vol && irix_has_ninf)
		gain[1] = AL_NEG_INFINITY;
	else
		gain[1] = alDoubleToFixed(vol1);

	/* Set the volume */
	p[0].param = AL_GAIN;
	p[0].value.ptr = gain;
	p[0].sizeIn = 2;
	if (al_set_params(audiodev, p, 1) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_sethw_vol: Cannot set volume:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
	}
#else
	al_param_t	p[4];

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "irixaud_sethw_vol: "
		"xmcd_vol=(%d/%d/%d) hw_gain=(%ld/%ld)\n",
		irix_wcd->i->vol,
		irix_wcd->i->vol_left,
		irix_wcd->i->vol_right,
		vol0, vol1);

	p[0] = AL_LEFT_SPEAKER_GAIN;
	p[1] = vol0;
	p[2] = AL_RIGHT_SPEAKER_GAIN;
	p[3] = vol1;
	if (al_set_params(audiodev, p, 4) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_sethw_vol: Cannot set volume:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return;
	}
#endif	/* NEW_IRIX_AUDIO_API */

	irix_curr_vol0 = vol0;
	irix_curr_vol1 = vol1;
}


/*
 * irixaud_open_dev
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
irixaud_open_dev(void)
{
	al_device_t	audiodev;
	char		*cp;
#ifdef NEW_IRIX_AUDIO_API
	ALparamInfo	pi;
#endif

	if (!gen_set_eid(irix_wcd))
		return FALSE;

	if ((irix_config = al_new_config()) == 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_open_dev: audio config failed:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		(void) gen_reset_eid(irix_wcd);
		return FALSE;
	}

	/* Set audio parameters */
	(void) al_set_samp_fmt(irix_config, AL_SAMPFMT_TWOSCOMP);
	(void) al_set_width(irix_config, AL_SAMPLE_16);
	(void) al_set_channels(irix_config, AL_STEREO);
	(void) al_set_queuesize(irix_config, 131069);

#ifdef NEW_IRIX_AUDIO_API
	if ((cp = getenv("AUDIODEV")) == NULL) {
		/* Use default output port */
		audiodev = AL_DEFAULT_OUTPUT;
	}
	else {
		char	*devname;

		devname = (char *) MEM_ALLOC(
			"devname",
			strlen(cp) + strlen(OUT_RESNAME) + 1
		);
		if (devname == NULL) {
			(void) sprintf(irix_wcd->i->msgbuf,
				    "irixaud_open_dev: Out of memory");
			DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
			(void) gen_reset_eid(irix_wcd);
			return FALSE;
		}

		(void) sprintf(devname, "%s%s", cp, OUT_RESNAME);

		if ((audiodev = alGetResourceByName(AL_SYSTEM, devname,
						    AL_DEVICE_TYPE)) == 0) {
			(void) sprintf(irix_wcd->i->msgbuf,
				    "irixaud_open_dev: "
				    "Invalid audio resource: %s\n%s",
				    devname, alGetErrorString(oserror()));
			MEM_FREE(devname);
			DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
			(void) gen_reset_eid(irix_wcd);
			return FALSE;
		}

		MEM_FREE(devname);
	}

	/* Set the device */
	if (alSetDevice(irix_config, audiodev) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_open_dev: Cannot set device\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		(void) gen_reset_eid(irix_wcd);
		return FALSE;
	}
#else
	audiodev = AL_DEFAULT_DEVICE;
#endif	/* NEW_IRIX_AUDIO_API */

	/* Open the port */
	if ((irix_port = al_open_port("xmcd-writer",
				      "w", irix_config)) == NULL){
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_open_dev: Cannot open port\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		(void) gen_reset_eid(irix_wcd);
		return FALSE;
	}

#ifdef NEW_IRIX_AUDIO_API
	/* Get the min/max volume and step values for this device */
	if (alGetParamInfo(audiodev, AL_GAIN, &pi) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_open_dev: "
			    "Cannot get volume parameters:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		(void) gen_reset_eid(irix_wcd);
		return FALSE;
	}

	irix_has_ninf = (bool_t) ((pi.specialVals & AL_NEG_INFINITY_BIT) != 0);
	irix_min_vol = alFixedToDouble(pi.min.ll);
	irix_max_vol = alFixedToDouble(pi.max.ll);
	irix_vol_step = alFixedToDouble(pi.minDelta.ll);

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "irixaud_open_dev: "
			"has_ninf=%d min_vol=%lf max_vol=%lf, vol_step=%lf\n",
			(int) irix_has_ninf,
			irix_min_vol, irix_max_vol, irix_vol_step);
#else
	if (ALgetminmax(audiodev, AL_LEFT_SPEAKER_GAIN,
			&irix_min_vol, &irix_max_vol) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_open_dev: "
			    "Cannot get volume min/max values:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);

		/* Just hardwire to 0-255 */
		irix_min_vol = 0;
		irix_max_vol = 255;
	}
	irix_vol_step = 1;

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "irixaud_open_dev: "
			"min_vol=%ld max_vol=%ld, vol_step=%ld\n",
			irix_min_vol, irix_max_vol, irix_vol_step);
#endif	/* NEW_IRIX_AUDIO_API */

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nOpened audio device [%d]\n",
				audiodev);

	(void) gen_reset_eid(irix_wcd);
	return TRUE;
}


/*
 * irixaud_close_dev
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
irixaud_close_dev(void)
{
	if (!gen_set_eid(irix_wcd))
		return FALSE;

	/* Close the port */
	if (irix_port != NULL) {
		/* Drain audio */
		while (al_get_filled(irix_port) > 0)
			util_delayms(10);

		al_close_port(irix_port);
		al_free_config(irix_config);
		irix_port = NULL;

		DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nClosed audio device\n");
	}

	(void) gen_reset_eid(irix_wcd);
	return TRUE;
}


/*
 * irixaud_get_device
 *	Get audio device id.
 *
 * Args:
 *	None
 *
 * Return:
 *	-1         - failure
 *	Device id  - success
 */
STATIC int
irixaud_get_device(void)
{
	al_device_t	audiodev;

#ifdef NEW_IRIX_AUDIO_API
	/* Retrieve the audio device */
	if ((audiodev = alGetDevice(irix_config)) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_get_device: Cannot get device:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
	}
#else
	audiodev = AL_DEFAULT_DEVICE;
#endif

	return (audiodev);
}


/*
 * irixaud_config
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
irixaud_config(void)
{
	al_device_t	audiodev;
	al_param_t	p[2];

	if ((audiodev = irixaud_get_device()) < 0)
		return FALSE;

	if (al_set_samp_fmt(irix_config, AL_SAMPFMT_TWOSCOMP) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_config: Cannot set sampling format:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return FALSE;
	}

	if (al_set_width(irix_config, AL_SAMPLE_16) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_config: Cannot set width:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return FALSE;
	}

	if (al_set_channels(irix_config, AL_STEREO) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_config: Cannot set channels:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return FALSE;
	}

#ifdef NEW_IRIX_AUDIO_API
	p[0].param = AL_RATE;
	p[0].value.ll = alDoubleToFixed(44100.0);
	p[1].param = AL_MASTER_CLOCK;
	p[1].value.i = AL_CRYSTAL_MCLK_TYPE;
#else
	p[0] = AL_OUTPUT_RATE;
	p[1] = AL_RATE_44100;
#endif
	if (al_set_params(audiodev, p, 2) < 0) {
		(void) sprintf(irix_wcd->i->msgbuf,
			    "irixaud_config: Cannot set sampling rate:\n%s",
			    alGetErrorString(oserror()));
		DBGPRN(DBG_SND)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return FALSE;
	}

	/* Retrieve the audio device volumes and update
	 * volume/balance GUI controls
	 */
	irix_curr_vol0 = (al_hwvol_t) -1;
	irix_curr_vol1 = (al_hwvol_t) -1;
	irixaud_gethw_vol(audiodev);

	return TRUE;
}


/*
 * irixaud_update
 *	Updates hardware volume and balance settings from GUI settings.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
irixaud_update(void)
{
	al_device_t		audiodev,
				tvol,
				val;
	al_hwvol_t		range,
				vol0,
				vol1;
	static al_hwvol_t	curr_vol0 = (al_hwvol_t) -1,
				curr_vol1 = (al_hwvol_t) -1;

	app_data.vol_taper = irix_wcd->i->vol_taper;

	/* Get the audio device */
	if ((audiodev = irixaud_get_device()) < 0)
		return;

	range = irix_max_vol - irix_min_vol;

	/* Convert the current xmcd volume and balance into IRIX L/R gains */
	tvol = util_taper_vol(irix_wcd->i->vol);

	if (irix_wcd->i->vol_left == irix_wcd->i->vol_right) {
		vol0 = (al_hwvol_t) tvol * range / (al_hwvol_t) 100;
		vol1 = vol0;
	}
	else if (irix_wcd->i->vol_left > irix_wcd->i->vol_right) {
		val = (irix_wcd->i->vol * irix_wcd->i->vol_right) / 100;
		if (((irix_wcd->i->vol * irix_wcd->i->vol_right) % 100) >= 50)
			val++;
		vol0 = (al_hwvol_t) tvol * range / (al_hwvol_t) 100;
		vol1 = (al_hwvol_t) val * range / (al_hwvol_t) 100;
	}
	else {
		val = (irix_wcd->i->vol * irix_wcd->i->vol_left) / 100;
		if (((irix_wcd->i->vol * irix_wcd->i->vol_left) % 100) >= 50)
			val++;
		vol0 = (al_hwvol_t) val * range / (al_hwvol_t) 100;
		vol1 = (al_hwvol_t) tvol * range / (al_hwvol_t) 100;
	}

#ifdef NEW_IRIX_AUDIO_API
	/* Round to the nearest step and shift to proper control range */
	vol0 = (rint(vol0 / irix_vol_step) * irix_vol_step) + irix_min_vol;
	if (vol0 > irix_max_vol)
		vol0 = irix_max_vol;

	vol1 = (rint(vol1 / irix_vol_step) * irix_vol_step) + irix_min_vol;
	if (vol1 > irix_max_vol)
		vol1 = irix_max_vol;
#endif	/* NEW_IRIX_AUDIO_API */

	if (vol0 == curr_vol0 && vol1 == curr_vol1)
		/* No change */
		return;

	curr_vol0 = vol0;
	curr_vol1 = vol1;

	/* Apply new settings */
	irixaud_sethw_vol(audiodev, vol0, vol1);
}


/*
 * irixaud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
irixaud_status(void)
{
	al_device_t	audiodev;

	/* Get the audio device */
	if ((audiodev = irixaud_get_device()) < 0)
		return;

	/* Retrieve the audio device volumes and update the
	 * volume/balance GUI controls
	 */
	irixaud_gethw_vol(audiodev);
}


/*
 * irixaud_write
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
irixaud_write(curstat_t *s)
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
	le_data = (byte_t *) MEM_ALLOC("le_data", irix_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(irix_wcd->i->msgbuf,
				"irixaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", irix_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(irix_wcd->i->msgbuf,
				"irixaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", irix_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, irix_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, irix_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (irix_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(irix_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (irix_wcd->cdb->occupied == 0 && irix_wcd->i->cdda_done) {
			irix_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(irix_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (irix_wcd->cdb->occupied <= 0 &&
		       irix_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(irix_wsemid, LOCK);
			cdda_waitsem(irix_wsemid, DATA);
			cdda_waitsem(irix_wsemid, LOCK);
		}

		/* Break if stopped */
		if (irix_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(irix_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(irix_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&irix_wcd->cdb->b[
			    irix_wcd->cds->chunk_bytes * irix_wcd->cdb->nextout
			],
			irix_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		irix_wcd->cdb->nextout++;
		irix_wcd->cdb->nextout %= irix_wcd->cds->buffer_chunks;
		irix_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&irix_wcd->i->writer_hb,CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && irix_wcd->i->frm_played > 0) {
				irix_wcd->i->frm_per_sec = (int)
					((float) irix_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				irix_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(irix_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(irix_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(irix_wcd->i->chroute,
				irix_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) irix_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) irix_wcd->cds->chunk_bytes);

		/* Write to device */
		if (irix_write_dev) {
			/* Always send big endian data */
			dev_data = be_data;

			if (al_write_samps(irix_port, dev_data,
				       irix_wcd->cds->chunk_bytes >> 1) < 0) {
				irix_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(irix_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			irixaud_update();

			if ((irix_wcd->i->frm_played % FRAME_PER_SEC) == 0)
				irixaud_status();
		}

		/* Write to file */
		if (irix_write_file) {
			file_data = irix_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != irix_wcd->i->trk_idx) {
				trk_idx = irix_wcd->i->trk_idx;

				if (irix_file_desc != NULL &&
				    !gen_close_file(irix_file_desc)) {
					irix_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(irix_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				irix_file_desc = gen_open_file(
					irix_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) irix_file_mode,
					app_data.cdda_filefmt,
					irix_wcd->i->trk_len
				);
				if (irix_file_desc == NULL) {
					irix_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(irix_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(irix_file_desc, file_data,
					(size_t) irix_wcd->cds->chunk_bytes)) {
				irix_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(irix_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (irix_write_pipe) {
			pipe_data = irix_file_be ? be_data : le_data;

			if (!gen_write_chunk(irix_pipe_desc, pipe_data,
					(size_t) irix_wcd->cds->chunk_bytes)) {
				irix_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(irix_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		irix_wcd->i->frm_played += irix_wcd->cds->chunk_blocks;

		while (irix_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = irix_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(irix_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * irix_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
irix_winit(void)
{
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
}


/*
 * irix_write
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
irix_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "irix_write: Nothing to do.\n");
		return FALSE;
	}

	irix_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
	irix_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	irix_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	irix_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (irix_wcd == NULL) {
		(void) fprintf(errfp, "irix_write: out of memory\n");
		return FALSE;
	}
	(void) memset(irix_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((irix_wsemid = cdda_initipc(irix_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &irix_file_mode);
	/* Make sure file is at least accessible by user */
	irix_file_mode |= S_IRUSR | S_IWUSR;
	irix_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (irix_wcd->i->end_lba - irix_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&irix_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (irix_write_file || irix_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && irix_write_file) {
			irix_file_desc = gen_open_file(
				irix_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) irix_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (irix_file_desc == NULL)
				return FALSE;
		}

		if (irix_write_pipe) {
			irix_pipe_desc = gen_open_pipe(
				irix_wcd,
				app_data.pipeprog,
				&irix_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (irix_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nirixaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  irix_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		irix_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	if (irix_write_dev) {
		/* Open audio device */
		if (!irixaud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!irixaud_config())
			return FALSE;

		/* Initial settings */
		irixaud_update();
 	}

	/* Do the writing */
	if (!irixaud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * irix_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
irix_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\nirix_wdone: Cleaning up writer\n");

	if (irix_port != NULL)
		(void) irixaud_close_dev();

	if (irix_file_desc != NULL) {
		(void) gen_close_file(irix_file_desc);
		irix_file_desc = NULL;
	}

	if (irix_pipe_desc != NULL) {
		(void) gen_close_pipe(irix_pipe_desc);
		irix_pipe_desc = NULL;
	}

	cdda_yield();

	if (irix_pipe_pid > 0) {
		(void) kill(irix_pipe_pid, SIGTERM);
		irix_pipe_pid = -1;
	}

	if (irix_wcd != NULL) {
		if (irix_wcd->i != NULL) {
			if (killreader && irix_wcd->i->reader != (thid_t) 0) {
				cdda_kill(irix_wcd->i->reader, SIGTERM);
				irix_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			irix_wcd->i->frm_played = 0;
		}

		MEM_FREE(irix_wcd);
		irix_wcd = NULL;
	}

	irix_write_dev = irix_write_file = irix_write_pipe = FALSE;
}


/*
 * irix_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
irix_winfo(char *str)
{
	(void) strcat(str, "SGI IRIX sound driver\n");
}

#endif	/* CDDA_WR_IRIX CDDA_SUPPORTED */

