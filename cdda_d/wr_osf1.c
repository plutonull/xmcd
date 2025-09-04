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
 *   Digital/Compaq/HP Tru64 UNIX (OSF1) and OpenVMS
 *   Multi-media Extension subset (MME) audio driver support
 */
#ifndef lint
static char *_wr_osf1_c_ident_ = "@(#)wr_osf1.c	7.50 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_OSF1) && defined(CDDA_SUPPORTED)

#ifdef HAS_MME
#include <mme/mme_api.h>
#endif
#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_osf1.h"


extern appdata_t	app_data;
extern FILE		*errfp;

#ifdef HAS_MME
STATIC HWAVEOUT		osf1_devhandle = 0;	/* Audio device handle */
STATIC UINT		osf1_devid;		/* Audio device ID */
STATIC LPWAVEHDR	osf1_wavehdrp = NULL;	/* Wave header pointer */
#endif	/* HAS_MME */

STATIC int		osf1_wsemid,		/* Semaphores identifier */
			osf1_curr_vol0,		/* Current left volume */
			osf1_curr_vol1;		/* Current right volume */
STATIC pid_t		osf1_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*osf1_wcd;		/* Shared memory pointer */
STATIC bool_t		osf1_write_dev,		/* Write to audio device */
			osf1_write_file,	/* Write to output file */
			osf1_write_pipe,	/* Pipe to program */
			osf1_file_be = FALSE,	/* Big endian output file */
			osf1_vol_supp = FALSE,	/* Volume control supported */
			osf1_bal_supp = FALSE;	/* Balance control supported */
STATIC gen_desc_t	*osf1_file_desc = NULL,	/* Output file desc */
			*osf1_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	osf1_file_mode;		/* Output file mode */


#ifdef HAS_MME
/*
 * osf1aud_wave_cb
 *	Callback function used to notify that playing is done on a
 *	block of audio data.
 *
 * Args:
 *	waveout - waveout handle
 *	msg - message code
 *	instance - instance number
 *	p1 - parameter 1
 *	p2 - parameter 2
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
osf1aud_wave_cb(HANDLE waveout, UINT msg, DWORD instance, LPARAM p1, LPARAM p2)
{
	switch (msg) {
	case WOM_OPEN:
	case WOM_CLOSE:
	case WOM_DONE:
		break;
	default:
		DBGPRN(DBG_SND)(errfp,
			"osf1aud_wave_cb: unknown message %u received.\n",
			msg);
		break;
	}
}


/*
 * osf1aud_scale_vol
 *	Scale volume from xmcd 0-100 to MME audio device values
 *
 * Args:
 *	vol - xmcd volume 0-100
 *
 * Return:
 *	Device volume value
 */
STATIC int
osf1aud_scale_vol(int vol)
{
	int	n;

	n = ((vol * MME_MAX_VOL) / 100);
	if (((vol * MME_MAX_VOL) % 100) >= 50)
		n++;

	/* Range checking */
	if (n > MME_MAX_VOL)
		n = MME_MAX_VOL;

	return (n);
}


/*
 * osf1aud_unscale_vol
 *	Scale volume from MME audio device values to xmcd 0-100
 *
 * Args:
 *	vol - device volume value
 *
 * Return:
 *	xmcd volume 0-100
 */
STATIC int
osf1aud_unscale_vol(int vol)
{
	int	n;

	n = (vol * 100) / MME_MAX_VOL;
	if (((vol * 100) % MME_MAX_VOL) >= (MME_MAX_VOL >> 1))
		n++;

	/* Range checking */
	if (n < 0)
		n = 0;
	else if (n > 100)
		n = 100;

	return (n);
}


/*
 * osf1aud_gethw_vol
 *	Get hardware volume control settings and update global variables.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
osf1aud_gethw_vol(void)
{
	MMRESULT	status;
	LPDWORD		vol;
	int		vol0,
			vol1,
			val;

	if (!osf1_vol_supp)
		/* Just do nothing */
		return;	

	vol = (LPDWORD) mmeAllocMem(sizeof(DWORD));
	if (vol == NULL) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_gethw_vol: Out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return;
	}
	(void) memset(vol, 0, sizeof(DWORD));

	status = waveOutGetVolume(osf1_devid, vol);
	if (status != MMSYSERR_NOERROR) {
		mmeFreeMem(vol);
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_gethw_vol: "
			    "waveOutGetVolume failed (status=%d)", status);
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
	}

	if (osf1_bal_supp) {
		vol0 = (*vol & 0x0000ffff);
		vol1 = (*vol & 0xffff0000) >> 16;
	}
	else
		vol0 = vol1 = (*vol & 0x0000ffff);

	mmeFreeMem(vol);

	vol0 = osf1aud_unscale_vol(vol0);
	vol1 = osf1aud_unscale_vol(vol1);

	if (vol0 == osf1_curr_vol0 && vol1 == osf1_curr_vol1)
		/* No change */
		return;

	osf1_curr_vol0 = vol0;
	osf1_curr_vol1 = vol1;

	/* Update volume and balance */
	if (vol0 == vol1) {
		osf1_wcd->i->vol = util_untaper_vol(osf1_curr_vol0);
		osf1_wcd->i->vol_left = 100;
		osf1_wcd->i->vol_right = 100;
	}
	else if (vol0 > vol1) {
		osf1_wcd->i->vol = util_untaper_vol(osf1_curr_vol0);
		osf1_wcd->i->vol_left = 100;
		osf1_wcd->i->vol_right =
			(osf1_curr_vol1 * 100) / osf1_curr_vol0;
		val = osf1_curr_vol0 >> 1;
		if (((osf1_curr_vol1 * 100) % osf1_curr_vol0) >= val)
		osf1_wcd->i->vol_right++;
	}
	else {
		osf1_wcd->i->vol = util_untaper_vol(osf1_curr_vol1);
		osf1_wcd->i->vol_right = 100;
		osf1_wcd->i->vol_left =
			(osf1_curr_vol0 * 100) / osf1_curr_vol1;
		val = osf1_curr_vol1 >> 1;
		if (((osf1_curr_vol0 * 100) % osf1_curr_vol1) >= val)
			osf1_wcd->i->vol_left++;
	}
}


/*
 * osf1aud_sethw_vol
 *	Set hardware volume control settings.
 *
 * Args:
 *	vol0 - Left channel volume
 *	vol1 - Right channel volume
 *
 * Return:
 *	Nothing.
 */
STATIC void
osf1aud_sethw_vol(int vol0, int vol1)
{
	MMRESULT	status;
	DWORD		vol;
	int		hvol0,
			hvol1;

	if (!osf1_vol_supp)
		/* Just do nothing */
		return;

	hvol0 = osf1aud_scale_vol(vol0);
	hvol1 = osf1aud_scale_vol(vol1);

	if (osf1_bal_supp)
		vol = ((hvol1 & 0xffff) << 16) | (hvol0 & 0xffff);
	else
		vol = (hvol0 & 0xffff);

	status = waveOutSetVolume(osf1_devid, vol);
	if (status != MMSYSERR_NOERROR) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_sethw_vol: "
			    "waveOutSetVolume failed (status=%d)", status);
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
	}

	osf1_curr_vol0 = vol0;
	osf1_curr_vol1 = vol1;
}

#endif	/* HAS_MME */


/*
 * osf1aud_open_dev
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
osf1aud_open_dev(void)
{
#ifndef HAS_MME
	(void) sprintf(osf1_wcd->i->msgbuf,
		    "osf1aud_open_dev: MMS audio support not compiled-in");
	DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
	return FALSE;
#else
	MMRESULT	status;
	LPWAVEOUTCAPS	caps;
	char		*cp;

	if (!gen_set_eid(osf1_wcd))
		return FALSE;

	if (waveOutGetNumDevs() < 1) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_open_dev: audio disabled:\n%s",
			    "No MMS audio device available.");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		(void) gen_reset_eid(osf1_wcd);
		return FALSE;
	}

	caps = (LPWAVEOUTCAPS) mmeAllocMem(sizeof(WAVEOUTCAPS));
	if (caps == NULL) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_open_dev: Out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		(void) gen_reset_eid(osf1_wcd);
		return FALSE;
	}
	(void) memset(caps, 0, sizeof(WAVEOUTCAPS));

	/* Use AUDIODEV environment for device ID if specified by user,
	 * otherwise use 0.
	 */
	if ((cp = getenv("AUDIODEV")) != NULL)
		osf1_devid = (UINT) atoi(cp);
	else
		osf1_devid = 0;

	status = waveOutGetDevCaps(osf1_devid, caps, sizeof(WAVEOUTCAPS));
	if (status != MMSYSERR_NOERROR) {
		mmeFreeMem(caps);
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_open_dev: "
			    "waveOutGetDevCaps failed (status=%d)", status);
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		(void) gen_reset_eid(osf1_wcd);
		return FALSE;
	}

	if ((caps->dwFormats & WAVE_FORMAT_4S16) == 0) {
		mmeFreeMem(caps);
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_open_dev: The audio device does not\n"
			    "support 44.1 kHz Stereo 16-bit audio.");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		(void) gen_reset_eid(osf1_wcd);
		return FALSE;
	}

	if ((caps->dwSupport & WAVECAPS_VOLUME) != 0) {
		osf1_vol_supp = TRUE;
	}
	if ((caps->dwSupport & WAVECAPS_LRVOLUME) != 0) {
		osf1_vol_supp = TRUE;
		osf1_bal_supp = TRUE;
	}

	mmeFreeMem(caps);

	DBGPRN(DBG_GEN|DBG_SND)(errfp,
		"osf1aud_open_dev: MME wave out: vol_supp=%d bal_supp=%d\n",
		(int) osf1_vol_supp, (int) osf1_bal_supp
	);

	(void) gen_reset_eid(osf1_wcd);
	return TRUE;
#endif	/* HAS_MME */
}


/*
 * osf1aud_close_dev
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
osf1aud_close_dev(void)
{
#ifndef HAS_MME
	return FALSE;
#else
	MMRESULT	status;

	if (!gen_set_eid(osf1_wcd))
		return FALSE;

	if ((status = waveOutReset(osf1_devhandle)) != MMSYSERR_NOERROR) {
		DBGPRN(DBG_SND)(errfp, "osf1aud_close_dev: "
			    "waveOutReset failed (status=%d)\n", status);
	}
	if ((status = waveOutClose(osf1_devhandle)) != MMSYSERR_NOERROR) {
		DBGPRN(DBG_SND)(errfp, "osf1aud_close_dev: "
			    "waveOutClose failed (status=%d)\n", status);
	}

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nClosed [audio:%u]\n", osf1_devid);

	osf1_devhandle = 0;
	mmeFreeMem(osf1_wavehdrp);
	osf1_wavehdrp = NULL;

	(void) gen_reset_eid(osf1_wcd);
	return ((bool_t) status == MMSYSERR_NOERROR);
#endif	/* HAS_MME */
}


/*
 * osf1aud_config
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
osf1aud_config(void)
{
#ifndef HAS_MME
	(void) sprintf(osf1_wcd->i->msgbuf,
		    "osf1aud_config: MMS audio support not compiled-in");
	DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
	return FALSE;
#else
	MMRESULT	status;
	LPPCMWAVEFORMAT	wp;

	wp = (LPPCMWAVEFORMAT) mmeAllocMem(sizeof(PCMWAVEFORMAT));
	if (wp == NULL) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_config: Out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}

	wp->wf.nSamplesPerSec = 44100;
	wp->wf.nChannels = 2;
	wp->wBitsPerSample = 16;
	wp->wf.wFormatTag = WAVE_FORMAT_PCM;

	status = waveOutOpen(
		&osf1_devhandle,
		osf1_devid,
		(LPWAVEFORMAT) wp,
		osf1aud_wave_cb,
		0,
		WAVE_ALLOWSYNC | CALLBACK_FUNCTION
	);
	if (status != MMSYSERR_NOERROR) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_config: waveOutOpen failed (status=%d)",
			    status);
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}

	mmeFreeMem(wp);

	osf1_wavehdrp = (LPWAVEHDR) mmeAllocMem(sizeof(WAVEHDR));
	if (osf1_wavehdrp == NULL) {
		(void) waveOutClose(osf1_devhandle);
		osf1_devhandle = 0;
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_config: Out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}

	osf1_wavehdrp->lpData = (LPSTR)
				mmeAllocMem(osf1_wcd->cds->chunk_bytes);
	if (osf1_wavehdrp->lpData == NULL) {
		(void) waveOutClose(osf1_devhandle);
		osf1_devhandle = 0;
		mmeFreeMem(osf1_wavehdrp);
		osf1_wavehdrp = NULL;
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_config: Out of memory");
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}

	/* Retrieve current volume settings */
	osf1_curr_vol0 = osf1_curr_vol1 = -1;
	osf1aud_gethw_vol();

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nOpened [audio:%u]\n", osf1_devid);

	return TRUE;
#endif	/* HAS_MME */
}


/*
 * osf1aud_write_chunk
 *      Write a chunk of audio data to the audio device.  This function
 *      assumes 16-bit, 2-channel interleaved frames.
 *
 * Args:
 *      data - Pointer to the data buffer
 *      len - Number of bytes to write
 *
 * Return:
 *      FALSE - failure
 *      TRUE  - success
 */
STATIC bool_t
osf1aud_write_chunk(sword16_t *data, size_t len)
{
#ifndef HAS_MME
	(void) sprintf(osf1_wcd->i->msgbuf,
		    "osf1aud_write_chunk: MMS audio support not compiled-in");
	DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
	return FALSE;
#else
	MMRESULT	status;

	osf1_wavehdrp->dwBufferLength = len;
	(void) memcpy(osf1_wavehdrp->lpData, (byte_t *) data, len);

	DBGPRN(DBG_SND)(errfp, "\nWrite [audio:%u]: %d bytes\n",
			osf1_devid, len);

	status = waveOutWrite(osf1_devhandle, osf1_wavehdrp, sizeof(WAVEHDR));
	if (status != MMSYSERR_NOERROR) {
		(void) sprintf(osf1_wcd->i->msgbuf,
			    "osf1aud_write_chunk: "
			    "waveOutWrite failed (status=%d)", status);
		DBGPRN(DBG_SND)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}
	mmeWaitForCallbacks();
	mmeProcessCallbacks();

	return TRUE;
#endif	/* HAS_MME */
}


/*
 * osf1aud_update
 *	Updates hardware volume and balance settings from GUI settings.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
osf1aud_update(void)
{
#ifdef HAS_MME
	int		val,
			vol0,
			vol1;
	static int	curr_vol0 = -1,
			curr_vol1 = -1;

	app_data.vol_taper = osf1_wcd->i->vol_taper;

	/* Update volume and balance */
	if (osf1_wcd->i->vol_left == osf1_wcd->i->vol_right) {
		vol0 = util_taper_vol(osf1_wcd->i->vol);
		vol1 = vol0;
	}
	else if (osf1_wcd->i->vol_left > osf1_wcd->i->vol_right) {
		val = (osf1_wcd->i->vol * osf1_wcd->i->vol_right) / 100;
		if (((osf1_wcd->i->vol * osf1_wcd->i->vol_right) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(osf1_wcd->i->vol);
		vol1 = util_taper_vol(val);
	}
	else {
		val = (osf1_wcd->i->vol * osf1_wcd->i->vol_left) / 100;
		if (((osf1_wcd->i->vol * osf1_wcd->i->vol_left) % 100) >= 50)
			val++;
		vol0 = util_taper_vol(val);
		vol1 = util_taper_vol(osf1_wcd->i->vol);
	}

	if (vol0 == curr_vol0 && vol1 == curr_vol1)
		/* No change */
		return;

	curr_vol0 = vol0;
	curr_vol1 = vol1;

	/* Apply new settings */
	osf1aud_sethw_vol(vol0, vol1);
#endif	/* HAS_MME */
}


/*
 * osf1aud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
osf1aud_status(void)
{
#ifdef HAS_MME
	/* Retrieve current volume settings */
	osf1aud_gethw_vol();
#endif	/* HAS_MME */
}


/*
 * osf1aud_write
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
osf1aud_write(curstat_t *s)
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
	le_data = (byte_t *) MEM_ALLOC("le_data", osf1_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(osf1_wcd->i->msgbuf,
				"osf1aud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", osf1_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(osf1_wcd->i->msgbuf,
				"osf1aud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", osf1_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, osf1_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, osf1_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (osf1_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(osf1_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (osf1_wcd->cdb->occupied == 0 && osf1_wcd->i->cdda_done) {
			osf1_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(osf1_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (osf1_wcd->cdb->occupied <= 0 &&
		       osf1_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(osf1_wsemid, LOCK);
			cdda_waitsem(osf1_wsemid, DATA);
			cdda_waitsem(osf1_wsemid, LOCK);
		}

		/* Break if stopped */
		if (osf1_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(osf1_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(osf1_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&osf1_wcd->cdb->b[
			    osf1_wcd->cds->chunk_bytes * osf1_wcd->cdb->nextout
			],
			osf1_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		osf1_wcd->cdb->nextout++;
		osf1_wcd->cdb->nextout %= osf1_wcd->cds->buffer_chunks;
		osf1_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&osf1_wcd->i->writer_hb,CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && osf1_wcd->i->frm_played > 0) {
				osf1_wcd->i->frm_per_sec = (int)
					((float) osf1_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				osf1_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(osf1_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(osf1_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(osf1_wcd->i->chroute,
				osf1_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) osf1_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) osf1_wcd->cds->chunk_bytes);

		/* Write to device */
		if (osf1_write_dev) {
			/* Always send little endian data */
			dev_data = le_data;

			if (!osf1aud_write_chunk((sword16_t *) dev_data,
					osf1_wcd->cds->chunk_bytes)) {
				osf1_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(osf1_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			osf1aud_update();

			if ((osf1_wcd->i->frm_played % FRAME_PER_SEC) == 0)
				osf1aud_status();
		}

		/* Write to file */
		if (osf1_write_file) {
			file_data = osf1_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != osf1_wcd->i->trk_idx) {
				trk_idx = osf1_wcd->i->trk_idx;

				if (osf1_file_desc != NULL &&
				    !gen_close_file(osf1_file_desc)) {
					osf1_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(osf1_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				osf1_file_desc = gen_open_file(
					osf1_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) osf1_file_mode,
					app_data.cdda_filefmt,
					osf1_wcd->i->trk_len
				);
				if (osf1_file_desc == NULL) {
					osf1_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(osf1_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(osf1_file_desc, file_data,
					(size_t) osf1_wcd->cds->chunk_bytes)) {
				osf1_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(osf1_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (osf1_write_pipe) {
			pipe_data = osf1_file_be ? be_data : le_data;

			if (!gen_write_chunk(osf1_pipe_desc, pipe_data,
					(size_t) osf1_wcd->cds->chunk_bytes)) {
				osf1_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(osf1_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		osf1_wcd->i->frm_played += osf1_wcd->cds->chunk_blocks;

		while (osf1_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = osf1_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(osf1_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * osf1_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
osf1_winit(void)
{
#ifdef HAS_MME
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
#else
	return (CDDA_WRITEFILE | CDDA_WRITEPIPE);
#endif
}


/*
 * osf1_write
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
osf1_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "osf1_write: Nothing to do.\n");
		return FALSE;
	}

#ifdef HAS_MME
	osf1_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
#else
	osf1_write_dev  = FALSE;
#endif
	osf1_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	osf1_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	osf1_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (osf1_wcd == NULL) {
		(void) fprintf(errfp, "osf1_write: out of memory\n");
		return FALSE;
	}
	(void) memset(osf1_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((osf1_wsemid = cdda_initipc(osf1_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &osf1_file_mode);
	/* Make sure file is at least accessible by user */
	osf1_file_mode |= S_IRUSR | S_IWUSR;
	osf1_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (osf1_wcd->i->end_lba - osf1_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&osf1_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (osf1_write_file || osf1_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && osf1_write_file) {
			osf1_file_desc = gen_open_file(
				osf1_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) osf1_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (osf1_file_desc == NULL)
				return FALSE;
		}

		if (osf1_write_pipe) {
			osf1_pipe_desc = gen_open_pipe(
				osf1_wcd,
				app_data.pipeprog,
				&osf1_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (osf1_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\nosf1aud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  osf1_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		osf1_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

#ifdef HAS_MME
	if (osf1_write_dev) {
		/* Open audio device */
		if (!osf1aud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!osf1aud_config())
			return FALSE;
	}
#endif

	/* Do the writing */
	if (!osf1aud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * osf1_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
osf1_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\nosf1_wdone: Cleaning up writer\n");

#ifdef HAS_MME
	if (osf1_wavehdrp != NULL)
		(void) osf1aud_close_dev();
#endif

	if (osf1_file_desc != NULL) {
		(void) gen_close_file(osf1_file_desc);
		osf1_file_desc = NULL;
	}

	if (osf1_pipe_desc != NULL) {
		(void) gen_close_pipe(osf1_pipe_desc);
		osf1_pipe_desc = NULL;
	}

	cdda_yield();

	if (osf1_pipe_pid > 0) {
		(void) kill(osf1_pipe_pid, SIGTERM);
		osf1_pipe_pid = -1;
	}

	if (osf1_wcd != NULL) {
		if (osf1_wcd->i != NULL) {
			if (killreader && osf1_wcd->i->reader != (thid_t) 0) {
				cdda_kill(osf1_wcd->i->reader, SIGTERM);
				osf1_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			osf1_wcd->i->frm_played = 0;
		}

		MEM_FREE(osf1_wcd);
		osf1_wcd = NULL;
	}

	osf1_write_dev = osf1_write_file = osf1_write_pipe = FALSE;
}


/*
 * osf1_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
osf1_winfo(char *str)
{
	(void) strcat(str, "Tru64 UNIX (OSF1) MMS audio driver\n");
}

#endif	/* CDDA_WR_OSF1 CDDA_SUPPORTED */

